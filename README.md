# ZeroVolts UI

Proyecto **personal de aprendizaje** sobre Raspberry Pi 4 + Parrot OS, construido para
estudiar y experimentar con:

- **C** (memoria, punteros, arquitectura por capas, modularidad).
- **LVGL 9** sobre Linux (`fbdev` para framebuffer y `evdev` para touch SPI).
- **IoT y comunicación con microcontroladores** (UART RPi ↔ ESP32).
- **Bluetooth Low Energy** (escaneo, GATT, descubrimiento de servicios y características).
- **Señales infrarrojas** (captura y reproducción de mandos a distancia).
- **USB HID / BadUSB** (gadgets USB en Linux con `configfs`).
- **GPIO de Raspberry Pi** (botones tactiles, drivers de IR vía device tree).

No es un producto, es un banco de pruebas. Cada módulo se incorpora cuando hay
algo nuevo que aprender o probar: a veces se reescribe entero y a veces se
deja a medio camino. Si algo falla, normalmente lo aprendido se queda.

---

## Tabla de contenidos

- [1. Hardware actual](#1-hardware-actual)
- [2. Inicio rápido](#2-inicio-rápido)
- [3. Compilar e integrar LVGL](#3-compilar-e-integrar-lvgl)
- [4. Touch y framebuffer](#4-touch-y-framebuffer)
- [5. Ejemplos mínimos](#5-ejemplos-mínimos)
- [6. Funcionalidades implementadas](#6-funcionalidades-implementadas)
  - [6.1 USB HID / BadUSB](#61-usb-hid--badusb)
  - [6.2 Infrarrojo (IR)](#62-infrarrojo-ir)
  - [6.3 Bluetooth Low Energy (BLE)](#63-bluetooth-low-energy-ble)
- [7. Mapa de GPIO](#7-mapa-de-gpio)
- [8. Arquitectura del proyecto](#8-arquitectura-del-proyecto)
- [9. Configuración (app-config.json)](#9-configuración-app-configjson)
- [10. Estructura de directorios](#10-estructura-de-directorios)

---

## 1. Hardware actual

| Pieza | Detalle |
|---|---|
| SBC | Raspberry Pi 4 con Parrot OS |
| Pantalla | SPI táctil 320×240 (controlador `ADS7846`, framebuffer `/dev/fb0`) |
| Botones físicos | 2 tactile switches (NAV + SELECT) leídos por `libgpiod` |
| IR | Módulo TX + RX usando `gpio-ir` / `gpio-ir-tx` del kernel (`/dev/lirc0`, `/dev/lirc1`) |
| BLE | ESP32 DevKit conectado por **UART** (firmware en repo aparte: [`zerovolts_firmware`](../zerovolts_firmware/)) |
| HID | Modo *USB gadget* del propio Raspberry Pi (puerto USB-C como dispositivo) |

---

## 2. Inicio rápido

Desde el directorio del proyecto:

```bash
make
./bin/zero-volts-ui
```

Limpiar binarios:

```bash
make clean
```

Compilar todos los ejemplos (`examples/main_*.c`):

```bash
make examples
```

Compilar un ejemplo específico:

```bash
make example NAME=hello_world      # genera bin/example-hello_world
make example NAME=touch_switch     # genera bin/example-touch_switch
make example NAME=touch_calibration
```

> El `Makefile` espera **LVGL** clonado en `~/git/lv_port_linux` y compilado
> con `liblvgl.a` en `build/lvgl/lib/`. Ver siguiente sección.

---

## 3. Compilar e integrar LVGL

LVGL no se distribuye con este repositorio: se compila aparte como librería
estática y se enlaza desde el `Makefile`.

### 3.1 Clonar el port oficial de Linux

```bash
cd ~/git
git clone https://github.com/lvgl/lv_port_linux.git
cd lv_port_linux/
git submodule update --init --recursive
```

Esto descarga:

- LVGL (motor gráfico).
- Drivers Linux: `fbdev` (framebuffer) y `evdev` (touch).
- Demos y ejemplos oficiales.

### 3.2 Instalar dependencias del sistema

```bash
sudo apt install cmake libevdev-dev libgpiod-dev v4l-utils ir-ctl
```

- `libevdev-dev` → driver de touch.
- `libgpiod-dev` → botones físicos por GPIO (NAV/SELECT).
- `ir-ctl` → backend usado por el módulo IR (paquete `v4l-utils`).

### 3.3 Compilar la librería LVGL

```bash
cd ~/git/lv_port_linux
cmake -B build -DCONFIG=fbdev
cmake --build build -j$(nproc)
```

Salida relevante para este proyecto:

```
~/git/lv_port_linux/build/lvgl/lib/liblvgl.a
~/git/lv_port_linux/lvgl/src/drivers/display/fb/    # lv_linux_fbdev.h
~/git/lv_port_linux/lvgl/src/drivers/evdev/         # lv_evdev.h
```

Estos paths son los que importa el `Makefile` mediante `-I` y `LIBS`.

```
lv_port_linux/lvgl/src/drivers/display/fb/
lv_port_linux/lvgl/src/drivers/evdev/
```
 habilitar LV_USE_LODEPNG a 1 en el lv_config.h
 y cambiar LV_MEM_SIZE a 256 en vez de 64
---

<a id="sec-4"></a>
## ▶️ 4. Ejecutar demo oficial (opcional)

```bash
./build/bin/lvglsim
```

### 3.5 Deshabilitar demos y ejemplos al integrar

Editar `~/git/lv_port_linux/CMakeLists.txt`:

```cmake
set(LVGL_ENABLE_DEMO ON)     -> OFF
set(LVGL_ENABLE_EXAMPLES ON) -> OFF
```

Y recompilar:

```bash
cmake --build build -j$(nproc)
```

### 3.6 Compilar la app

<a id="sec-6"></a>
## 🚀 6. Compilar la aplicación con LVGL

Forma recomendada (usa el `Makefile` del proyecto):

```bash
make
```

El `Makefile` enlaza:

```bash
bin/zero-volts-ui
```

Para compilar manualmente, tomar como referencia el comando generado por `make`
y los includes/librerías definidos en `Makefile`.

---

## 4. Touch y framebuffer

LVGL no detecta el touch automáticamente: hay que indicarle el `event` correcto.

### 4.1 Encontrar el dispositivo de entrada

```bash
cat /proc/bus/input/devices
```

Buscar la entrada cuyo `Name=` corresponde al touch. Ejemplo:

```
I: Bus=001c Vendor=0000 Product=1ea6 Version=0000
N: Name="ADS7846 Touchscreen"
P: Phys=spi0.1/input0
H: Handlers=mouse0 event4
```

Aquí el path sería `/dev/input/event4`. La aplicación principal lo
**autodetecta** vía `detect_touch_event_path()` (busca el `INPUT_PROP_DIRECT`).

Para inspección manual:

```bash
evtest /dev/input/event4
```

### 4.2 Calibración

La pantalla actual está rotada 90°, así que usamos `lv_evdev_set_swap_axes(touch, true)`
y los puntos de calibración se obtuvieron tocando las esquinas con `evtest`:

```c
lv_evdev_set_calibration(touch, 296, 294, 3931, 3843);
```

Esto vive en [main.c:54-62](main.c#L54-L62) y se replica en
[examples/main_touch_switch.c:141-156](examples/main_touch_switch.c#L141-L156).

---

## 5. Ejemplos mínimos

Pequeños programas autocontenidos que usan la misma `liblvgl.a`. Sirven para
probar partes aisladas sin levantar toda la app.

| Ejemplo | Qué muestra |
|---|---|
| [examples/main_hello_world.c](examples/main_hello_world.c) | Inicialización mínima de LVGL + label centrado en el framebuffer. |
| [examples/main_touch_calibration.c](examples/main_touch_calibration.c) | Cómo calibrar el touch SPI usando `lv_evdev_set_calibration`. |
| [examples/main_touch_switch.c](examples/main_touch_switch.c) | Botones LVGL + grupo de foco controlado por dos GPIO físicos (NAV/SELECT) usando `libgpiod`. |

Compilar y ejecutar:

```bash
make example NAME=hello_world
./bin/example-hello_world
```

---

## 6. Funcionalidades implementadas

Cada módulo sigue el mismo patrón: **vista LVGL → controller (lógica) → service (sistema/hardware)**.
Ver [§8 Arquitectura](#8-arquitectura-del-proyecto).

### 6.1 USB HID / BadUSB

Aprovecha que el puerto USB-C del Raspberry Pi 4 soporta **modo gadget**: el
RPi se presenta al host como un teclado/ratón USB y reproduce un script.

#### Pines / hardware

- **Sin GPIO dedicado**: todo ocurre en USB.
- El Raspberry Pi se conecta al equipo víctima con un cable USB-C.
- En `/boot/firmware/config.txt` y `/boot/cmdline.txt` debe estar habilitado `dwc2`:
  ```
  dtoverlay=dwc2
  modules-load=dwc2
  ```

#### Componentes

| Capa | Archivo | Rol |
|---|---|---|
| Vista | [page/hid/hid_view.c](page/hid/hid_view.c) | Lista de scripts, switch on/off, label de seleccionado. |
| Controller | [page/hid/hid_controller.c](page/hid/hid_controller.c) | Persiste el script seleccionado en `app-config.json`, valida estados. |
| Service | [service/hid_service.c](service/hid_service.c) | Llama a los shell scripts y al servicio systemd. |

#### Scripts y servicios externos

Todos en [scripts/](scripts/):

| Archivo | Función |
|---|---|
| [scripts/zv-hid-enable.sh](scripts/zv-hid-enable.sh) | Activa el gadget escribiendo en `/sys/kernel/config/usb_gadget/zerovolts-hid/UDC` el nombre del UDC disponible. |
| [scripts/zv-hid-disable.sh](scripts/zv-hid-disable.sh) | Desactiva el gadget escribiendo `""` en el mismo `UDC`. |
| [scripts/zv-hid-session.service](scripts/zv-hid-session.service) | Unidad systemd que ejecuta `hid_watcher.py` en background; detecta cuándo el host conecta y dispara el script seleccionado. |

> El gadget USB (`configfs`) se configura **una sola vez** en arranque
> (vendor/product, descriptor HID, etc.). Los scripts solo conectan/desconectan
> el bind con el UDC.

#### Flujo del toggle ON/OFF

```
hid_view  →  hid_controller_toggle(true)
                ├─ hid_service_enable()         → sudo zv-hid-enable.sh
                ├─ hid_service_start_session()  → sudo systemctl start zv-hid-session.service
                └─ guarda script seleccionado en app-config.json
```

#### Ubicación de los payloads

Configurado en `app-config.json`:

```json
"hid": {
  "script_list_path": "data/badusb/",
  "script_selected_path": "",
  "is_enabled": false
}
```

> Requiere `sudo` sin password para los scripts (ver `/etc/sudoers.d/`),
> ya que se invocan desde el binario sin sesión interactiva.

---

### 6.2 Infrarrojo (IR)

Captura señales de mandos IR (TV, ventilador, aire acondicionado, etc.) y las
re-emite. Sirve para entender modulación IR, codificación pulso/espacio y el
subsistema **CIR (Consumer IR)** del kernel Linux.

#### Pines / hardware

| Función | GPIO | Pin físico | Device tree overlay |
|---|---|---|---|
| IR RX (receptor) | GPIO 5 | 29 | `dtoverlay=gpio-ir,gpio_pin=5` |
| IR TX (transmisor) | GPIO 6 | 31 | `dtoverlay=gpio-ir-tx,gpio_pin=6` |

Esto debe ir en `/boot/firmware/config.txt`. Tras un reboot aparecen `/dev/lirc0` (TX)
y `/dev/lirc1` (RX).

#### Componentes

| Capa | Archivo | Rol |
|---|---|---|
| Vista (hub) | [page/ir/ir.c](page/ir/ir.c) | Página principal IR. |
| Vistas | [page/ir/remotes.c](page/ir/remotes.c), [page/ir/new_remote.c](page/ir/new_remote.c), [page/ir/learn_button.c](page/ir/learn_button.c), [page/ir/send_signal.c](page/ir/send_signal.c) | Listar mandos, crear mando, aprender botón, enviar señal. |
| Controller | [page/ir/ir_controller.c](page/ir/ir_controller.c) | Crea estructura de directorios, sanitiza nombres, gestiona reintentos de captura. |
| Helper | [page/ir/ir_raw_helper.c](page/ir/ir_raw_helper.c) | Validación del archivo `.raw`, normalización de capturas con número impar de tokens. |
| Service | [service/ir_service.c](service/ir_service.c) | Wrapper sobre `ir-ctl` (`v4l-utils`). |

#### Backend

Hoy solo está implementado el backend **`irctl`** (configurable en `app-config.json`).
El backend `lircdev` está stubbed (`IR_ERR_UNSUPPORTED`).

Comandos reales lanzados:

```bash
# Aprender (RX)
timeout <N>s ir-ctl -r -d '/dev/lirc1' > '<button>.raw.tmp'

# Enviar (TX)
timeout 3s ir-ctl -d '/dev/lirc0' -s '<button>.raw'
```

#### Estructura en disco

```
data/ir/remotes/
└── <remote_name>/
    ├── meta.json
    └── buttons/
        ├── on.raw
        ├── off.raw
        └── ...
```

Path raíz configurable en `app-config.json` → `ir.remotes_path`.

#### Formato del archivo `.raw`

Lo escribe `ir-ctl` directamente; nosotros solo validamos:

- Tokens con signo, alternando pulso/espacio: `+1270 -397 +1279 -396 ...`
- Cantidad mínima: `IR_RAW_MIN_TOKENS` (160).
- Cantidad **par** (cada pulso debe llevar su gap).

#### Detalle interesante: capturas con tokens impares

A veces `ir-ctl` corta justo después de un pulso `+...` sin emitir el `-...`
final. Para no descartar la captura, [ir_raw_helper.c](page/ir/ir_raw_helper.c)
añade un **gap sintético** (`-20000`) y revalida. Si pasa, se acepta.

Si la captura sigue inválida, los intentos fallidos quedan en disco para
diagnóstico:

```
<button>.raw.invalid1
<button>.raw.invalid2
<button>.raw.invalid3
```

Útiles para comparar contra un `.raw` que sí funciona:

```bash
REMOTES_PATH="data/ir/remotes"
wc -w "$REMOTES_PATH/<remote>/buttons/off.raw" \
      "$REMOTES_PATH/<remote>/buttons/<button>.raw.invalid1"
```

---

### 6.3 Bluetooth Low Energy (BLE)

Escaneo de dispositivos BLE cercanos, conexión y enumeración de **servicios y
características GATT**. La parte de radio NO la hace el RPi: la hace un
**ESP32** que actúa como módulo BLE dedicado y se comunica por **UART**.

Ventajas principales:
- Menor acoplamiento entre UI y hardware.
- Mejor testabilidad de la logica sin depender de dispositivos reales.
- Más facilidad para cambiar backends sin tocar las vistas.
- Mantenimiento más simple cuando crece el proyecto.

- Aprender a comunicar dos sistemas distintos (RPi ↔ MCU).
- Aprender el stack **Bluedroid** de ESP-IDF.
- Aislar la radio en hardware dedicado, sin pelearse con BlueZ.
- Liberar al RPi para la UI y el resto de funcionalidades.

#### Pines / hardware (UART)

| RPi | ESP32 | Función |
|---|---|---|
| GPIO 12 (Pin 32) — TX | GPIO 16 — RX | RPi → ESP32 |
| GPIO 13 (Pin 33) — RX | GPIO 17 — TX | ESP32 → RPi |
| GND (Pin 34) | GND | Referencia común |

En `/boot/firmware/config.txt`:

```
dtoverlay=uart5
```

Tras reiniciar aparece `/dev/ttyAMA5` (configurado en `app-config.json` →
`uart.device`). Velocidad: **115200 baudios**.

#### Componentes en el RPi (este repo)

| Capa | Archivo | Rol |
|---|---|---|
| Vistas | [page/bt/bt_view.c](page/bt/bt_view.c), [page/bt/bt_scanner.c](page/bt/bt_scanner.c), [page/bt/bt_device_detail.c](page/bt/bt_device_detail.c) | Hub, lista de scan con filtro (All/Near/Connectable), detalle del dispositivo con servicios y propiedades. |
| Controller | [page/bt/bt_controller.c](page/bt/bt_controller.c) | Parser del protocolo, máquina de estados de conexión, almacén de servicios y características. |
| UUID registry | [page/bt/bt_uuid_registry.c](page/bt/bt_uuid_registry.c) | Mapeo de UUID estándar 16-bit a nombres legibles (Battery, HID, GAP, …). |
| Service | [service/uart_service.c](service/uart_service.c) | Apertura no-bloqueante del puerto, parser línea-a-línea, despacho a callbacks por **tag**. |
| Constantes | [service/uart_commands.h](service/uart_commands.h) | Strings del protocolo (REQ/RES). |
| Contexto | [app_context.c](app_context.c) | Almacén de dispositivos descubiertos persistente entre pantallas. |

#### Componentes en el ESP32 (repo aparte)

Firmware: [`zerovolts_firmware`](../zerovolts_firmware/), construido con **PlatformIO + ESP-IDF**.
Stack: **Bluedroid**. Capas:

- `zv_bt_gap.c` → GAP (escaneo, parsing de advertising data, fabricantes).
- `zv_bt_gattc.c` → GATT Client (MTU, services, characteristics).
- `zv_uart.c` → mismo protocolo de texto que se documenta abajo.

#### Protocolo UART (inventado para este proyecto)

**Transporte:** texto plano, una línea por mensaje (`\n` como delimitador).
El UI envía comandos sin newline; el `uart_service` añade el `\n` final.
El ESP32 acumula bytes hasta encontrar `\n`.

**Formato general:**

```
TIPO[:SUBTIPO][|key=value|key=value|...]
```

- Separador de campos clave-valor: `|`
- Separador `key`/`value`: `=`
- Algunos comandos request usan `|` posicionalmente: `CONNECT|<MAC>|<addr_type>`

##### Comandos (RPi → ESP32)

| Comando | Significado |
|---|---|
| `SCAN` | Iniciar escaneo BLE. |
| `CONNECT|<mac>|<addr_type>` | Conectar a un dispositivo (`addr_type` 0=public, 1=random). |
| `DISCONNECT` | Cerrar la conexión activa. |
| `DISCOVER` | Enumerar servicios y características del dispositivo conectado. |

##### Respuestas / eventos (ESP32 → RPi)

**Scan:**

```
SCAN:START
SCAN:DEVICE|name=Mi Banda|mac=AA:BB:CC:DD:EE:FF|rssi=-67|manufacturer=Xiaomi|service=...|appearance=Watch|connectable=1|addr_type=1
SCAN:DEVICE|...
SCAN:DONE
```

**Connect:**

```
CONNECT:START
CONNECT:OK
CONNECT:FAIL|reason=timeout
CONNECT:LOST|reason=...
CONNECT:ERROR
DISCONNECT:OK
```

**Discover:**

```
DISCOVER:START
DISCOVER:SERVICE|svc=0|uuid=00001800-0000-1000-8000-00805f9b34fb
DISCOVER:CHAR|svc=0|char=0|uuid=00002a00-0000-1000-8000-00805f9b34fb|props=0x02|handle=3
DISCOVER:CHAR|svc=0|char=1|uuid=...|props=0x0a|handle=5
DISCOVER:SERVICE|svc=1|uuid=...
...
DISCOVER:DONE
DISCOVER:FAIL|reason=...
```

`props` es el bitmask GATT estándar (`ESP_GATT_CHAR_PROP_BIT_*`):
`READ=0x02`, `WRITE_NR=0x04`, `WRITE=0x08`, `NOTIFY=0x10`, `INDICATE=0x20`.
La UI lo dibuja como pills en [bt_device_detail.c:47-54](page/bt/bt_device_detail.c#L47-L54).

#### Bus de eventos sobre UART

El service expone un sistema de **suscripción por tag** ([service/uart_service.c](service/uart_service.c)):

```c
add_event_callback(my_handler, "BT_TAG_CONTROLLER");
```

`uart_process_loop()` se llama desde el loop principal en [main.c:344](main.c#L344);
cada línea recibida se reparte a todos los handlers registrados, que filtran por
prefijo (`SCAN:`, `CONNECT:`, `DISCOVER:`).

#### Límites actuales

Definidos en [types.h](types.h):

- `BT_ALLOWED_MAX_DEVICES = 20`
- `BT_MAX_SERVICES = 20`
- `BT_MAX_CHARS_PER_SERVICE = 16`

---

## 7. Mapa de GPIO

Header de 40 pines del Raspberry Pi 4. Los pines 1-26 los reserva la pantalla
SPI + touch, así que todos los periféricos nuevos viven del 27 hacia adelante.

```
     3.3V  [ 1] [ 2]  5V
    GPIO2  [ 3] [ 4]  5V
    GPIO3  [ 5] [ 6]  GND
    GPIO4  [ 7] [ 8]  GPIO14    ─┐
      GND  [ 9] [10]  GPIO15     │
   GPIO17  [11] [12]  GPIO18     │
   GPIO27  [13] [14]  GND        │  Reservados por la
   GPIO22  [15] [16]  GPIO23     │  pantalla SPI + touch
     3.3V  [17] [18]  GPIO24     │
   GPIO10  [19] [20]  GND        │
    GPIO9  [21] [22]  GPIO25     │
   GPIO11  [23] [24]  GPIO8      │
      GND  [25] [26]  GPIO7     ─┘
    GPIO0  [27] [28]  GPIO1
🟠 GPIO5   [29] [30]  GND          ← IR RX  (gpio-ir,    /dev/lirc1)
🟠 GPIO6   [31] [32]  GPIO12 🔵    ← IR TX  (gpio-ir-tx, /dev/lirc0)  /  UART5 TX → ESP32 RX
🔵 GPIO13  [33] [34]  GND          ← UART5 RX ← ESP32 TX
   GPIO19  [35] [36]  GPIO16
🟢 GPIO26  [37] [38]  GPIO20       ← Botón SELECT (enter)
      GND  [39] [40]  GPIO21 🟢    ← Botón NAV (mover foco)
```

**Leyenda:**
- 🟠 IR  · 🔵 UART (ESP32) · 🟢 Botones físicos

`/boot/firmware/config.txt` mínimo para activar todo:

```
dtoverlay=gpio-ir,gpio_pin=5
dtoverlay=gpio-ir-tx,gpio_pin=6
dtoverlay=uart5
dtoverlay=dwc2          # para el HID gadget
```

---

## 8. Arquitectura del proyecto

Tres capas estrictas, una por funcionalidad:

- **view** → pantallas LVGL, eventos de UI, formato de strings.
- **controller** → reglas de negocio, validaciones, máquina de estados, sanitización de paths.
- **service** → I/O con sistema operativo / hardware (`ir-ctl`, `systemctl`, UART, filesystem).

Ventajas que ya se notan:

- Cambiar el backend de IR (`irctl` → `lircdev`) no toca la UI.
- Sustituir el ESP32 por otro transporte (TCP, USB) solo afectaría a `uart_service`.
- Los controllers se pueden probar sin pantalla.

```
┌──────────────────────────────────────────────────────────────────┐
│                              main.c                              │
│        init LVGL + config + IR + UART + nav GPIO + bucle         │
└──────────┬───────────────┬───────────────┬───────────────────────┘
           │               │               │
   ┌───────▼─────┐ ┌───────▼─────┐ ┌───────▼─────┐
   │     HID     │ │      IR     │ │     BLE     │
   └─────┬───────┘ └─────┬───────┘ └─────┬───────┘
         │               │               │
   ┌─────▼─────────┐ ┌───▼─────────┐ ┌───▼──────────────┐
   │ page/hid/     │ │ page/ir/    │ │ page/bt/         │
   │  view, ctrl   │ │ view, ctrl  │ │  view, ctrl      │
   └─────┬─────────┘ └───┬─────────┘ └───┬──────────────┘
         │               │               │
   ┌─────▼─────────┐ ┌───▼─────────┐ ┌───▼──────────────┐
   │ hid_service   │ │ ir_service  │ │ uart_service     │
   │ scripts +     │ │ ir-ctl +    │ │ termios + bus de │
   │ systemctl     │ │ filesystem  │ │ eventos por tag  │
   └───────────────┘ └─────────────┘ └────┬─────────────┘
                                          │ UART 115200 8N1
                                  ┌───────▼────────────┐
                                  │   ESP32 firmware   │
                                  │  (Bluedroid stack) │
                                  └────────────────────┘
```

#### Flujo de datos típico

```
Evento UI (touch o GPIO)
  └─> Controller (valida + decide)
        └─> Service (I/O al hardware o al SO)
              └─> Resultado / evento asíncrono
                    └─> Controller (actualiza estado)
                          └─> View (refresca LVGL)
```

#### Contexto de aplicación

[app_context.c](app_context.c) mantiene estado **compartido entre pantallas**
(p. ej. lista de dispositivos BLE escaneados, dispositivo seleccionado para
ver detalles). Evita re-pedir datos cuando navegas entre páginas.

#### Navegación con botones físicos

[components/nav.c](components/nav.c) + el `keypad_read` en [main.c:125-173](main.c#L125-L173)
inyectan eventos `LV_KEY_*` en el grupo focal de LVGL leyendo los GPIO 21
(NAV) y 26 (SELECT) con `libgpiod`. La interpretación cambia según el widget
focado (ej.: en un dropdown abierto, NAV envía `LV_KEY_DOWN`).

---

## 9. Configuración (app-config.json)

Archivo cargado al arrancar (ver [main.c:65-87](main.c#L65-L87)). Las rutas
relativas se resuelven contra el directorio del binario.

```json
{
  "version": "0.3",
  "hid": {
    "script_selected_path": "",
    "script_list_path": "data/badusb/",
    "is_enabled": false
  },
  "ir": {
    "remotes_path": "data/ir/remotes/",
    "backend": "irctl",
    "tx_device": "/dev/lirc0",
    "rx_device": "/dev/lirc1",
    "learn_timeout_ms": 5000,
    "use_on_screen_keyboard": true
  },
  "display": {
    "fb_device": "/dev/fb0"
  },
  "uart": {
    "device": "/dev/ttyAMA5",
    "baudrate": 115200
  }
}
```

---

## 10. Estructura de directorios

```
zerovolts-ui/
├── main.c                       # Init LVGL/IR/UART, navegación, bucle principal
├── config.[ch]                  # Carga/guardado de app-config.json
├── app_context.[ch]             # Estado global compartido entre pantallas
├── types.h                      # Tipos comunes (device_t, bt_service_t, …)
├── Makefile                     # App + ejemplos
├── app-config.json
│
├── components/                  # Widgets LVGL reutilizables
│   ├── top_bar.* / nav.*
│   ├── list/ui_list.*
│   ├── ui_pills.* / ui_loading_btn.* / ui_info_panel.*
│   ├── ui_theme.h               # Colores y tipografía
│   └── component_helper.*
│
├── page/                        # Pantallas + controllers (capas view/controller)
│   ├── base_view.*              # Tipo base de pantalla
│   ├── home_view.*              # Hub principal
│   ├── hid/                     # BadUSB
│   ├── ir/                      # Mandos IR
│   └── bt/                      # BLE scanner + detalle
│
├── service/                     # Capa hardware / SO
│   ├── hid_service.*            # configfs + scripts + systemctl
│   ├── ir_service.*             # ir-ctl
│   ├── uart_service.*           # termios + bus de eventos
│   └── uart_commands.h          # Constantes del protocolo BLE
│
├── utils/                       # Utilidades transversales
│   ├── cJSON.*                  # JSON
│   ├── file.*                   # FS helpers
│   ├── string_utils.*           # Sanitización, prefijos, etc.
│   ├── logger.* / error_handler.*
│
├── scripts/                     # Scripts de soporte (HID gadget)
│   ├── zv-hid-enable.sh
│   ├── zv-hid-disable.sh
│   └── zv-hid-session.service
│
├── examples/                    # Programas mínimos sobre la misma liblvgl.a
│   ├── main_hello_world.c
│   ├── main_touch_calibration.c
│   └── main_touch_switch.c
│
├── data/                        # Assets, iconos, mandos IR, scripts BadUSB
│   ├── assets/  icons/
│   └── (en runtime) ir/remotes/<name>/buttons/*.raw
│
├── docs/                        # Documentación interna y planificación
│   ├── GPIO_PINOUT.md
│   ├── ESP32_MODULO_PASO_A_PASO.md
│   ├── BLUETOOTH_COMUNICACION_PASO_A_PASO.md
│   └── ...
│
└── bin/                         # Binarios generados
```

---

## Notas finales

- El proyecto **depende** de un repo hermano para el firmware del ESP32:
  [`zerovolts_firmware`](../zerovolts_firmware/). Sin ese ESP32 flasheado, la
  pantalla BLE no recibe respuestas (pero la app sigue funcionando).
- Hay un `gpio_buttons.c` referenciado desde el `Makefile` que vive **fuera**
  del repo (`../tactile_switch/`). Es el wrapper sobre `libgpiod` reutilizado
  por `main.c` y por el ejemplo `touch_switch`.
- El `/etc/sudoers.d/` debe permitir ejecutar los scripts de HID y `systemctl`
  sin contraseña, ya que se invocan desde código no-interactivo.
