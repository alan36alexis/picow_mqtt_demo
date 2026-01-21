#ifndef SECRETS_H
#define SECRETS_H

// Configuración WiFi
#define WIFI_SSID       "TU_SSID_AQUI"
#define WIFI_PASS       "TU_PASSWORD_AQUI"

// Configuración MQTT
#define MQTT_SERVER_IP  "192.168.X.X" // IP del Broker
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "pico_w_template"
#define MQTT_KEEPALIVE  60

#define MQTT_RETRY_MS   5000

#endif