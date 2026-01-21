# Pico W MQTT Client - Demostración IoT

Cliente MQTT completo para Raspberry Pi Pico W que implementa un sistema IoT funcional con control de LED remotamente a través de un Broker MQTT (Mosquitto). El proyecto demuestra comunicación bidireccional, subscripción a tópicos, publicación de mensajes y manejo de estados de conexión.

## 📋 Descripción General

Este proyecto implementa un cliente MQTT en C para la Raspberry Pi Pico W que:

- **Conecta a WiFi** automáticamente usando credenciales configurables
- **Se conecta a un Broker MQTT** (Mosquitto) ejecutándose en la red local
- **Controla el LED onboard** mediante comandos MQTT con 4 modos diferentes:
  - `ON`: LED encendido constante
  - `OFF`: LED apagado
  - `BLINK FAST`: Parpadeo rápido (100ms)
  - `BLINK SLOW`: Parpadeo lento (500ms)
- **Publica heartbeats** periódicos con contador de latidos
- **Maneja reconexión automática** con reintentos en caso de desconexión
- **Implementa Last Will & Testament (LWT)** para notificar desconexión

## 🔧 Requisitos Previos

### Hardware
- **Raspberry Pi Pico W** (microcontrolador con WiFi integrado)
- **Cable USB** para programación y alimentación
- **Adaptador CMSIS-DAP** (opcional, para debugging)

### Software
- **Visual Studio Code** con extensión **Raspberry Pi Pico**
- **Pico C/C++ SDK** v2.2.0 (configurado en `~/.pico-sdk`)
- **CMake** 3.13+ y **Ninja** (incluidos en el SDK)
- **OpenOCD** 0.12.0+ (para flashing)
- **[Eclipse Mosquitto](https://mosquitto.org/download/)** (Broker MQTT)

### Requisitos de Red
- **PC y Pico W conectadas a la misma red WiFi local**
- Acceso administrativo para configurar Mosquitto
- Conocer la IP local del PC (ej: `192.168.1.100`)

## 🚀 Configuración Inicial

### Paso 1: Configurar Credenciales WiFi y MQTT

El proyecto usa un sistema de gestión de secretos para evitar exponer credenciales en el repositorio.

1. **Localiza el archivo plantilla** `secret_config.h` en la raíz del proyecto
2. **Crea una copia local** llamada `my_secret_config.h`:
   ```bash
   cp secret_config.h my_secret_config.h
   ```
3. **Edita `my_secret_config.h`** con tus datos reales:
   ```c
   #define WIFI_SSID       "Tu_Red_WiFi"
   #define WIFI_PASS       "Tu_Contraseña_WiFi"
   #define MQTT_SERVER_IP  "192.168.1.100"    // IP de tu PC
   #define MQTT_CLIENT_ID  "pico_w_client"
   #define MQTT_KEEPALIVE  60
   #define MQTT_PORT       1883
   #define MQTT_RETRY_MS   5000
   ```
4. **Asegúrate que `.gitignore` incluya** `my_secret_config.h` para no subir secretos

### Paso 2: Configurar el Broker Mosquitto en Windows

Por defecto, Mosquitto solo acepta conexiones locales. Para permitir que la Pico W se conecte desde la red:

#### Opción A: Usar archivo de configuración personalizado

1. **Crea un archivo** `mqtt.conf` con el siguiente contenido:
   ```text
   listener 1883
   allow_anonymous true
   ```

2. **Inicia Mosquitto** con esta configuración:
   ```powershell
   # En Windows, desde la carpeta de instalación de Mosquitto
   mosquitto.exe -c mqtt.conf
   ```

#### Opción B: Editar la configuración predeterminada

- Edita `C:\Program Files\mosquitto\mosquitto.conf` (o tu ruta de instalación)
- Descomenta/añade:
  ```text
  listener 1883
  allow_anonymous true
  ```

### Paso 3: Compilar y Cargar el Proyecto

1. **Abre el proyecto en VS Code**:
   ```bash
   code picow_mqtt_demo
   ```

2. **Compilar el proyecto** (Ctrl+Shift+B):
   ```bash
   # O manualmente:
   mkdir build
   cd build
   cmake ..
   ninja
   ```

3. **Cargar en la Pico W**:
   - Conecta la Pico W por USB
   - Presiona el botón **BOOTSEL** mientras conectas USB (entra en modo de carga)
   - Ejecuta la tarea "Run Project" o:
   ```bash
   picotool.exe load build/pico_mqtt_test.uf2 -fx
   ```

## 📡 Tópicos MQTT

El cliente se suscribe y publica en los siguientes tópicos:

### Tópicos de Subscripción (Entrada - desde Broker hacia Pico)

| Tópico | Comandos | Descripción |
|--------|----------|-------------|
| `pico/led` | `ON`, `OFF`, `BLINK FAST`, `BLINK SLOW` | Controla el LED onboard |
| `pico/count` | `RESET` | Reinicia el contador de heartbeats |

### Tópicos de Publicación (Salida - desde Pico hacia Broker)

| Tópico | Contenido | Frecuencia | Descripción |
|--------|----------|-----------|-------------|
| `pico/status` | `online` | Al conectar | Notifica que la Pico está online |
| `pico/heartbeat` | `alive N` | Cada 10 segundos | Latido con contador (N = contador) |
| `pico/will` | `offline` | Al desconectar | Mensaje de Last Will (desconexión) |

## 💻 Ejemplos de Uso con MQTT

### Controlar el LED con Mosquitto

Desde tu PC, abre un terminal y usa `mosquitto_pub`:

```bash
# Encender el LED
mosquitto_pub -h localhost -t pico/led -m "ON"

# Apagar el LED
mosquitto_pub -h localhost -t pico/led -m "OFF"

# Parpadeo rápido
mosquitto_pub -h localhost -t pico/led -m "BLINK FAST"

# Parpadeo lento
mosquitto_pub -h localhost -t pico/led -m "BLINK SLOW"

# Reiniciar contador de heartbeats
mosquitto_pub -h localhost -t pico/count -m "RESET"
```

### Suscribirse a Mensajes de Pico

```bash
# Escuchar todos los mensajes de pico
mosquitto_sub -h localhost -t "pico/#"

# Escuchar solo heartbeats
mosquitto_sub -h localhost -t "pico/heartbeat"
```

## 🏗️ Estructura del Código

### `main.c` - Programa Principal

#### Máquina de Estados (`system_state_t`)
```c
STATE_WIFI_OK           // Conectado a WiFi, esperando MQTT
STATE_MQTT_CONNECTING   // Intentando conectar a MQTT
STATE_MQTT_CONNECTED    // Conectado a MQTT
STATE_ERROR             // Error en la conexión
```

#### Modo del LED (`led_mode_t`)
```c
LED_MODE_OFF            // LED apagado
LED_MODE_ON             // LED encendido
LED_MODE_BLINK_FAST     // Parpadeo cada 100ms
LED_MODE_BLINK_SLOW     // Parpadeo cada 500ms
```

#### Funciones Principales

- **`mqtt_incoming_publish_cb()`**: Se ejecuta al recibir un tópico, identifica si es `pico/led` o `pico/count`
- **`mqtt_incoming_data_cb()`**: Procesa el payload y ejecuta la acción correspondiente
- **`mqtt_connection_cb()`**: Maneja el estado de conexión (éxito o fallo)
- **`mqtt_try_connect()`**: Intenta conectar al broker MQTT con reintentos
- **`mqtt_process()`**: Máquina de estados para manejo de reconexión
- **`main()`**: Loop principal que gestiona WiFi, MQTT y control del LED no-bloqueante

### `CMakeLists.txt` - Configuración de Compilación

- Define el proyecto como `pico_mqtt_test`
- Incluye `lwipopts.h` globalmente para configuración de lwIP
- Vincula librerías necesarias:
  - `pico_stdlib`: APIs estándar de Pico
  - `pico_cyw43_arch_lwip_threadsafe_background`: WiFi + lwIP
  - `pico_lwip_mqtt`: Cliente MQTT
  - `pico_lwip_arch`: Arquitectura de red

### `lwipopts.h` - Configuración de lwIP

Configura el stack de red lwIP para:
- DHCP automático
- Soporte MQTT
- Timeouts adecuados
- Optimización de memoria para sistemas embebidos

### `secret_config.h` - Plantilla de Secretos

Archivo plantilla con constantes que deben reemplazarse en `my_secret_config.h`

## 🔌 Tareas de Compilación en VS Code

Las siguientes tareas están disponibles:

| Tarea | Descripción |
|-------|-------------|
| **Compile Project** | Compila el proyecto usando Ninja |
| **Run Project** | Carga el binario `.uf2` a la Pico W |
| **Flash** | Flashea usando OpenOCD (requiere adaptador CMSIS-DAP) |
| **Rescue Reset** | Reinicia la Pico W en modo recovery |

## 📊 Flujo de Operación

```
1. Inicio
   ↓
2. Inicializar cyw43 (chip WiFi)
   ↓
3. Conectar a WiFi (30 segundos timeout)
   ↓
4. LED parpadea 1 vez (confirmación WiFi)
   ↓
5. Entrar en loop principal:
   - Procesar eventos de WiFi (cyw43_arch_poll)
   - Intentar conectar a MQTT (cada 5 segundos si está desconectado)
   - Si conectado: suscribirse a pico/led y pico/count
   - Si conectado: publicar heartbeat cada 10 segundos
   - Controlar LED según modo actual (no-bloqueante)
```

## 🔍 Debugging

### Ver Logs por UART

El proyecto configura UART (GPIO 0/1) como puerto serial:

```powershell
# En Windows, con herramientas como PuTTY o screen:
# Puerto: COM3 (ajusta según tu dispositivo)
# Velocidad: 115200 baud

# Salida esperada:
# [PICO W MQTT CLIENT]
# [WIFI] Connecting...
# [WIFI] Connected
# [WIFI] IP: 192.168.1.50
# [MQTT] Connecting...
# [MQTT] Connected
# [MQTT RX] Topic: pico/led (2 bytes)
# [MQTT RX] Payload: ON
# --> LED MODE: ON
# [MQTT RX] Topic: pico/heartbeat (11 bytes)
```

### Monitorear la Pico con MQTT

En otra terminal, suscríbete a todos los mensajes:
```bash
mosquitto_sub -h 127.0.0.1 -t "pico/#" -v
```

## 🐛 Solución de Problemas

### "WiFi failed"
- Verifica SSID y contraseña en `my_secret_config.h`
- Asegúrate que la red es 2.4 GHz (Pico W no soporta 5 GHz)
- Comprueba que está dentro del rango de cobertura

### "MQTT Connection failed"
- Verifica que Mosquitto está corriendo: `tasklist | findstr mosquitto`
- Confirma que `MQTT_SERVER_IP` es la IP correcta de tu PC
- Prueba conectividad: `ping 192.168.X.X`
- Revisa firewall (permite puerto 1883)

### LED no responde
- Verifica la conexión MQTT con `mosquitto_sub -t "pico/#"`
- Comprueba que los comandos son exactos: `ON`, `OFF`, `BLINK FAST`, `BLINK SLOW`
- Lee los logs UART para ver si recibe los mensajes

### Pico no se carga
- Presiona BOOTSEL mientras conectas USB
- Verifica que aparece como unidad removible
- Intenta compilar nuevamente: `ninja -C build`

## 📝 Notas de Implementación

- **No-bloqueante**: El control del LED usa timers no-bloqueantes, no `sleep_ms()`, para mantener responsivo el sistema
- **Reconexión automática**: Si MQTT se desconecta, reintentar cada 5 segundos indefinidamente
- **Memory-safe**: El código está diseñado para boards con ~264KB de RAM
- **Heartbeat**: Contador que incrementa en cada heartbeat para confirmar funcionamiento continuo
- **LWT (Last Will & Testament)**: Cuando la Pico se desconecta abruptamente, Mosquitto publica `offline` automáticamente

## 📚 Referencias

- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
- [Pico C SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [lwIP Documentation](https://savannah.nongnu.org/projects/lwip/)
- [MQTT Specification](https://mqtt.org/mqtt-specification)
- [Eclipse Mosquitto](https://mosquitto.org/)

## 📄 Licencia

Proyecto de demostración educativo. Libre para usar y modificar.

---

**Última actualización:** Enero 2026
