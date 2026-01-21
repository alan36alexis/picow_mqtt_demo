#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/mqtt.h"

#include "secret_config.h"
//#include "my_secret_config.h"

/* ================= STATE ================= */

typedef enum {
    STATE_WIFI_OK,
    STATE_MQTT_CONNECTING,
    STATE_MQTT_CONNECTED,
    STATE_ERROR
} system_state_t;

typedef struct {
    mqtt_client_t *client;
    system_state_t state;
    uint32_t last_retry;
    bool mqtt_connected;
} mqtt_ctx_t;

static mqtt_ctx_t ctx;

typedef enum {
    TOPIC_NONE,
    TOPIC_LED,
    TOPIC_COUNT
} current_topic_t;

static current_topic_t current_topic = TOPIC_NONE;
static uint32_t heartbeat_count = 0;

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK_FAST,
    LED_MODE_BLINK_SLOW
} led_mode_t;

static led_mode_t led_mode = LED_MODE_OFF;

/* ================= UTILS ================= */

static void led_blink(uint32_t n, uint32_t ms) {
    for (uint32_t i = 0; i < n; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(ms);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(ms);
    }
}

/* ================= MQTT CALLBACKS ================= */

// 1. Identificamos el tópico entrante
static void mqtt_incoming_publish_cb(void *arg,
                                     const char *topic,
                                     uint32_t tot_len) {
    printf("[MQTT RX] Topic: %s (%u bytes)\n", topic, tot_len);
    
    // Verificamos si el tópico es pico/led
    if (strcmp(topic, "pico/led") == 0) {
        current_topic = TOPIC_LED;
    } else if (strcmp(topic, "pico/count") == 0) {
        current_topic = TOPIC_COUNT;
    } else {
        current_topic = TOPIC_NONE;
    }
}

// 2. Procesamos el payload según el tópico identificado
static void mqtt_incoming_data_cb(void *arg,
                                  const uint8_t *data,
                                  uint16_t len,
                                  uint8_t flags) {
    printf("[MQTT RX] Payload: ");
    for (u16_t i = 0; i < len; i++) {
        putchar(data[i]);
    }
    printf("\n");

    if (current_topic == TOPIC_LED) {
        // Compara ignorando mayúsculas/minúsculas o estrictamente. Aquí estricto:
        if (strncmp((const char*)data, "ON", len) == 0) {
            led_mode = LED_MODE_ON;
            printf("--> LED MODE: ON\n");
        } 
        else if (strncmp((const char*)data, "OFF", len) == 0) {
            led_mode = LED_MODE_OFF;
            printf("--> LED MODE: OFF\n");
        }
        else if (strncmp((const char*)data, "BLINK FAST", len) == 0) {
            led_mode = LED_MODE_BLINK_FAST;
            printf("--> LED MODE: BLINK FAST\n");
        }
        else if (strncmp((const char*)data, "BLINK SLOW", len) == 0) {
            led_mode = LED_MODE_BLINK_SLOW;
            printf("--> LED MODE: BLINK SLOW\n");
        }
    } else if (current_topic == TOPIC_COUNT) {
        if (strncmp((const char*)data, "RESET", len) == 0) {
            heartbeat_count = 0;
            printf("--> HEARTBEAT RESET\n");
        }
    }
}

static void mqtt_connection_cb(mqtt_client_t *client,
                               void *arg,
                               mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] Connected\n");
        ctx.mqtt_connected = true;
        ctx.state = STATE_MQTT_CONNECTED;
        led_blink(2, 100);

        // Suscribirse al tópico de control del LED
        mqtt_sub_unsub(client, "pico/led", 0, NULL, NULL, 1);
        mqtt_sub_unsub(client, "pico/count", 0, NULL, NULL, 1);
        
        mqtt_publish(client, "pico/status",
                     (const uint8_t *)"online", 6, 1, 0, NULL, NULL);
    } else {
        printf("[MQTT] Connection failed: %d\n", status);
        ctx.mqtt_connected = false;
        ctx.state = STATE_WIFI_OK;
        ctx.last_retry = to_ms_since_boot(get_absolute_time());
    }
}

/* ================= MQTT LOGIC ================= */

static void mqtt_try_connect(void) {
    ip_addr_t ip;

    if (ctx.state == STATE_MQTT_CONNECTING) {
        return;
    }

    if (!ip4addr_aton(MQTT_SERVER_IP, &ip)) {
        printf("[ERR] Invalid broker IP\n");
        ctx.state = STATE_ERROR;
        return;
    }

    if (!ctx.client) {
        ctx.client = mqtt_client_new();
        mqtt_set_inpub_callback(ctx.client,
                                mqtt_incoming_publish_cb,
                                mqtt_incoming_data_cb,
                                NULL);
    }

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = MQTT_CLIENT_ID;
    ci.keep_alive = MQTT_KEEPALIVE;
    ci.will_topic = "pico/will";
    ci.will_msg = (const u8_t *)"offline";
    ci.will_msg_len = 7;
    ci.will_qos = 1;

    printf("[MQTT] Connecting...\n");

    err_t err = mqtt_client_connect(ctx.client, &ip,
                                    MQTT_PORT,
                                    mqtt_connection_cb,
                                    NULL, &ci);
    if (err != ERR_OK) {
        printf("[MQTT] Connect error: %d\n", err);
        ctx.last_retry = to_ms_since_boot(get_absolute_time());
    } else {
        ctx.state = STATE_MQTT_CONNECTING;
    }
}

static void mqtt_process(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    switch (ctx.state) {
    case STATE_WIFI_OK:
        if (now - ctx.last_retry > MQTT_RETRY_MS) {
            mqtt_try_connect();
        }
        break;

    case STATE_MQTT_CONNECTED:
        break;

    case STATE_ERROR:
        break;

    default:
        break;
    }
}

/* ================= MAIN ================= */

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\n[PICO W MQTT CLIENT]\n");

    if (cyw43_arch_init()) {
        printf("[ERR] cyw43 init failed\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("[WIFI] Connecting...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASS,
            CYW43_AUTH_WPA2_MIXED_PSK,
            30000)) {
        printf("[ERR] WiFi failed\n");
        return 1;
    }

    printf("[WIFI] Connected\n");
    printf("[WIFI] IP: %s\n",
           ip4addr_ntoa(netif_ip4_addr(netif_default)));

    led_blink(1, 200);

    ctx.state = STATE_WIFI_OK;
    ctx.last_retry = 0;
    ctx.client = NULL;

    uint32_t last_heartbeat = 0;
    uint32_t last_led_toggle = 0;
    bool led_state = false;

    while (true) {
        cyw43_arch_poll();

        mqtt_process();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Manejo del LED no bloqueante
        switch (led_mode) {
            case LED_MODE_OFF:
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                break;
            case LED_MODE_ON:
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                break;
            case LED_MODE_BLINK_FAST:
                if (now - last_led_toggle > 100) {
                    led_state = !led_state;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
                    last_led_toggle = now;
                }
                break;
            case LED_MODE_BLINK_SLOW:
                if (now - last_led_toggle > 500) {
                    led_state = !led_state;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
                    last_led_toggle = now;
                }
                break;
        }

        if (ctx.mqtt_connected &&
            now - last_heartbeat > 10000) {
            char buf[32];
            int len = snprintf(buf, sizeof(buf), "alive %u", heartbeat_count++);
            mqtt_publish(ctx.client,
                         "pico/heartbeat",
                         (const u8_t *)buf, len,
                         1, 0, NULL, NULL);
            last_heartbeat = now;
        }

        sleep_ms(1);
    }
}