#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Configuración común para Raspberry Pi Pico W (NO_SYS=1 para bare metal)
#define NO_SYS                      1
#define LWIP_SOCKET                 0

#if !NO_SYS
#define LWIP_TIMEVAL_PRIVATE        0
#endif

#define MEMP_NUM_SYS_TIMEOUT 24
#define LWIP_TIMERS 1


// Configuración de Memoria
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// Protocolos Habilitados
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_MQTT                   1
#define LWIP_MQTT_KEEP_ALIVE        60
#define MQTT_REQ_MAX_IN_FLIGHT      2
#define MQTT_OUTPUT_RINGBUF_SIZE    1024
#define MQTT_VAR_HEADER_BUFFER_LEN  128
#define MQTT_INPUT_RINGBUF_SIZE     1024
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// Configuración TCP
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// Callbacks y Hooks
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0

// Estadísticas (Deshabilitadas para ahorrar memoria)
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0

// Debug
#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_PLATFORM_DIAG(x)       do { printf x; } while(0)
#endif

#endif