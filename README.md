# ZeroVolts UI

ZeroVolts UI es un proyecto personal que estoy desarrollando en C con LVGL, usando una Raspberry Pi 4 con Parrot OS.

La idea es construir una herramienta portátil con pantalla, botones y distintos módulos para experimentar con señales de radio y la comunicación con otros dispositivos. Por ahora estoy trabajando con infrarrojo, Bluetooth Low Energy, USB HID y Sub-GHz. Con este proyecto voy aprendiendo sobre electrónica, programación y cómo integrar todo en una misma interfaz.

El proyecto todavía está en desarrollo, así que la estructura, el protocolo y las funciones pueden cambiar. Hay partes que ya se pueden usar y otras que son experimentales o todavía necesitan trabajo.

## Contenido

- [Hardware y funcionamiento general](#hardware-y-funcionamiento-general)
- [Preparación: compilar LVGL primero](#preparación-compilar-lvgl-primero)
- [Compilar y ejecutar ZeroVolts UI](#compilar-y-ejecutar-zerovolts-ui)
- [Funciones actuales](#funciones-actuales)
- [Conexiones y pines](#conexiones-y-pines)
- [Comunicación con el ESP32](#comunicación-con-el-esp32)
- [Arquitectura y directorios](#arquitectura-y-directorios)
- [Configuración y datos](#configuración-y-datos)
- [Ejemplos y pruebas](#ejemplos-y-pruebas)
- [Estado y próximos aprendizajes](#estado-y-próximos-aprendizajes)

## Hardware y funcionamiento general

La Raspberry Pi se encarga de la interfaz y de los archivos. El ESP32 maneja BLE y el módulo CC1101 para Sub-GHz. Los dos se comunican por UART.

```text
Pantalla táctil + botones
           │
     Raspberry Pi
      ZeroVolts UI
       ├── Infrarrojo RX/TX
       ├── USB HID
       └── UART ── ESP32
                     ├── Bluetooth Low Energy
                     └── SPI ── CC1101 (Sub-GHz)
```

| Elemento | Uso actual |
|---|---|
| Raspberry Pi 4 con Parrot OS | Ejecuta la aplicación y guarda configuración, mandos y señales. |
| Pantalla SPI touch de 320 × 240 | Interfaz LVGL mediante framebuffer y entrada táctil mediante `evdev`; el montaje actual usa ADS7846 para el touch. |
| Dos pulsadores físicos | NAV para desplazarse y SELECT para seleccionar. |
| Receptor y transmisor IR | Aprender botones de mandos y volver a enviar sus señales. |
| ESP32 DevKit | Ejecuta el firmware de BLE y Sub-GHz y responde por UART. |
| CC1101 conectado al ESP32 | Captura y retransmisión experimental de señales de radio. |
| Conexión USB de datos de la Raspberry Pi | Función USB HID mediante el modo gadget de Linux. |

El firmware del ESP32 se mantiene en otro proyecto: [`zerovolts_firmware`](../zerovolts_firmware/).

## Preparación: compilar LVGL primero

**Antes de compilar la aplicación, hay que compilar LVGL.** El `Makefile` usa la biblioteca externa `liblvgl.a`; al ejecutar `make` en este proyecto, LVGL no se descarga ni se compila automáticamente.

En el [tutorial de LVGL para Raspberry Pi](https://zerovolts-site.pages.dev/meta/03-lvgl-raspberry/#2-herramientas-necesarias-y-descarga-de-lvgl) explico cómo preparar la pantalla, las herramientas y la biblioteca. Recomiendo seguirlo primero. Aquí dejo un resumen de lo que necesita este proyecto.

### Dependencias

Estas son las dependencias que uso en mi instalación:

```bash
sudo apt install build-essential cmake git python3 pkg-config libevdev-dev libgpiod-dev v4l-utils evtest
```

`libevdev` permite trabajar con la entrada táctil; `libgpiod`, con los pulsadores; e `ir-ctl`, incluido en `v4l-utils`, se usa para infrarrojo. El driver actual de botones utiliza la API de **libgpiod 1.x**; una instalación con la API 2.x requiere adaptar ese driver.

### Descargar y compilar el port de Linux

La ubicación predeterminada es `~/git/lv_port_linux`:

```bash
mkdir -p ~/git
cd ~/git
git clone --recurse-submodules https://github.com/lvgl/lv_port_linux.git
cd lv_port_linux
cmake -B build -DCONFIG=fbdev
cmake --build build -j"$(nproc)"
```

Si ya clonaste el repositorio, descarga los submódulos con `git submodule update --init --recursive` antes de compilar.

La configuración de LVGL debe incluir framebuffer (`LV_USE_LINUX_FBDEV`), entrada táctil (`LV_USE_EVDEV`), acceso a archivos (`LV_USE_FS_STDIO`) y las fuentes que usa la interfaz. El código actual referencia explícitamente Montserrat de tamaños **10 y 12**, además de la fuente predeterminada de LVGL. Para los recursos PNG, habilita `LV_USE_LODEPNG`; la configuración usada en este montaje reserva `256 * 1024U` bytes mediante `LV_MEM_SIZE` cuando se utiliza el asignador interno.

En el port local, `build/lv_conf.h` se genera desde la configuración de LVGL. Mantén los ajustes en el archivo de configuración de origen que corresponda a tu versión del port y recompila para que la biblioteca y sus cabeceras coincidan.

Antes de continuar, debe existir:

```text
~/git/lv_port_linux/build/lvgl/lib/liblvgl.a
```

Además de esa biblioteca, el [Makefile](Makefile) utiliza las cabeceras de `lvgl/`, los drivers de framebuffer y `evdev`, y la configuración generada en `build/`. El proyecto usa LVGL 9; los cambios entre versiones del port pueden requerir ajustes.

## Compilar y ejecutar ZeroVolts UI

Con LVGL compilado y el hardware configurado, desde la raíz de este repositorio:

```bash
make
./bin/zero-volts-ui
```

También puedes usar `make run`. Para otra ubicación de LVGL:

```bash
make LVPORT=/ruta/a/lv_port_linux
```

Para limpiar el binario principal y los ejemplos, usa `make clean`.

La aplicación necesita acceso al framebuffer, al dispositivo táctil y a los dispositivos que use cada módulo, como GPIO, UART o IR. Todo esto debe estar configurado en la Raspberry Pi.

### Pantalla y touch

El framebuffer se configura en `app-config.json`; actualmente es `/dev/fb0`. El touch se busca automáticamente en `/proc/bus/input/devices`, primero por sus propiedades y después por el nombre del dispositivo. Si no se encuentra, la aplicación no completa el arranque.

La rotación y la calibración están ajustadas a mi pantalla en [main.c](main.c): se intercambian los ejes y se usan los valores `296, 294, 3931, 3843`. Si cambias de pantalla, revisa esos valores. Para identificar y comprobar el touch:

```bash
cat /proc/bus/input/devices
evtest /dev/input/eventN
```

Sustituye `eventN` por el dispositivo de tu pantalla.

## Funciones actuales

### Navegación táctil y con pulsadores

Desde la pantalla principal se puede entrar a Bad USB, Infrared, Bluetooth y Sub-GHz. La interfaz se puede usar con el touch o con los botones NAV y SELECT. NAV mueve el foco y permite desplazarse por los desplegables y el teclado en pantalla; SELECT confirma la opción.

La navegación entre pantallas está en [page/navigation_page.c](page/navigation_page.c), y la lectura de los pulsadores está en [drivers/gpio_buttons.c](drivers/gpio_buttons.c). El filtrado básico de clicks y la traducción de pulsaciones a teclas LVGL siguen en `main.c`.

### USB HID / Bad USB

Desde la interfaz se pueden ver y seleccionar scripts, además de activar o desactivar la sesión HID. La idea es que la Raspberry Pi funcione como un dispositivo USB de entrada y ejecute acciones mediante scripts.

Actualmente, [service/hid_service.c](service/hid_service.c) delega la activación en los scripts de [scripts/](scripts/) y en `zv-hid-session.service`. Requiere preparar previamente el gadget USB de Linux; los scripts incluidos conectan y desconectan un gadget ya creado, no lo construyen desde cero.

**La ejecución completa depende de componentes externos al estado actual del repositorio:** la unidad de sesión referencia `scripts/hid_watcher.py`, que no está incluido. También hay rutas locales en el servicio que deben adaptarse a otra instalación y llamadas a `sudo` que necesitan permisos sin interacción.

### Infrarrojo

Permite crear mandos, aprender las señales de sus botones, guardarlas y volver a enviarlas. Por ejemplo, se puede crear un mando de televisión y guardar el botón de encendido para usarlo después desde la pantalla.

El backend implementado utiliza `ir-ctl` y los dispositivos IR de Linux. Las señales se guardan como archivos `.raw` dentro de `data/ir/remotes/<mando>/buttons/`. La aplicación comprueba las capturas y ofrece reintentos cuando no son válidas.

Código principal: [page/ir/](page/ir/) y [service/ir_service.c](service/ir_service.c). Más adelante me gustaría implementar el acceso con `lircdev`, que creo que sería una mejor opción.

### Bluetooth Low Energy (BLE)

Permite buscar dispositivos BLE cercanos, filtrar la lista, ver su nombre, dirección e intensidad de señal y conectarse a los que lo permitan. También se pueden descubrir sus servicios y características GATT para explorar las funciones que ofrece cada dispositivo.

El ESP32 maneja la radio y la Raspberry Pi envía las solicitudes y muestra los resultados. Por ahora se puede escanear, conectar y descubrir servicios, pero todavía no hay una interfaz general para leer o escribir características.

Código principal: [page/bt/](page/bt/) y [service/uart_service.c](service/uart_service.c).

### Sub-GHz

Aquí se puede elegir una frecuencia, capturar una señal, ver sus pulsos en un gráfico y guardar o descartar el resultado. Las señales guardadas aparecen en la pantalla de reproducción, donde también se puede elegir cuántas veces enviarlas.

La interfaz ofrece **315, 380, 381, 433.92, 868 y 915 MHz**. Su uso efectivo depende del firmware y del módulo y la antena instalados. La Raspberry Pi guarda las señales como archivos `.sub` en `data/subghz/signals/`; el ESP32 controla el CC1101 y realiza la captura o transmisión.

La reproducción actual admite señales RAW, es decir, una secuencia de duraciones de pulsos, con un límite de 256 valores. La comunicación envía los datos en partes y espera confirmaciones del ESP32. Esto sigue siendo experimental: guardar una señal no garantiza que pueda reproducirse correctamente en cualquier dispositivo.

Código principal: [page/sub_ghz/](page/sub_ghz/), [service/subghz/](service/subghz/) y el firmware del ESP32.

## Conexiones y pines

En las tablas dejo tanto el número GPIO como el pin físico del conector de 40 pines para facilitar el cableado.

### Raspberry Pi

| Función | GPIO de Raspberry Pi | Pin físico | Conexión |
|---|---|---|---|
| Receptor IR | GPIO 5 | 29 | Salida de datos del receptor infrarrojo. |
| Transmisor IR | GPIO 6 | 31 | Entrada del circuito transmisor infrarrojo. |
| UART TX | GPIO 12 | 32 | RX del ESP32, GPIO 16. |
| UART RX | GPIO 13 | 33 | TX del ESP32, GPIO 17. |
| Tierra UART | GND | 34 | GND del ESP32. |
| Botón SELECT | GPIO 26 | 37 | Pulsador para confirmar. |
| Botón NAV | GPIO 21 | 40 | Pulsador para desplazarse. |
| Tierra para botones | GND | 39 | Referencia común para los pulsadores. |

Los pulsadores se detectan al pasar de nivel alto a bajo: al pulsar deben llevar la entrada a GND. El circuito debe mantener un nivel alto estable al soltarlos; el driver actual no configura una resistencia pull-up por software. Las señales GPIO y UART trabajan a 3.3 V.

En este montaje, el conector de la pantalla SPI y su touch ocupa la zona de los pines físicos 1–26. Se ha dejado esa zona reservada para la pantalla; no significa que todos esos pines transporten datos. Si cambias de pantalla, revisa su distribución antes de reutilizarlos.

Para habilitar IR y la UART utilizada, la configuración de arranque del montaje incluye:

```ini
dtoverlay=gpio-ir,gpio_pin=5
dtoverlay=gpio-ir-tx,gpio_pin=6
dtoverlay=uart5
```

En Parrot OS se ha usado `/boot/firmware/config.txt`. Tras reiniciar, comprueba los nombres de los dispositivos y ajústalos en `app-config.json`: el montaje actual usa `/dev/lirc0` para transmitir, `/dev/lirc1` para recibir y `/dev/ttyAMA5` para UART. La numeración de los dispositivos puede variar.

USB HID no utiliza un GPIO del conector: usa el puerto USB de datos en modo gadget. Su preparación incluye `dwc2` y la configuración del gadget mediante `configfs`; añadir el overlay por sí solo no completa esa instalación.

### CC1101 conectado al ESP32

El CC1101 se conecta al **ESP32**, no directamente a los GPIO de la Raspberry Pi. Los GPIO de esta tabla corresponden al ESP32:

| Señal del CC1101 | Conexión en ESP32 | Para qué sirve |
|---|---|---|
| VCC | 3.3 V | Alimentación. |
| GND | GND | Tierra común. |
| SCK | GPIO 14 | Reloj de comunicación SPI. |
| MOSI | GPIO 13 | Datos hacia el módulo. |
| MISO / GDO1 | GPIO 25 | Datos desde el módulo. |
| CSN | GPIO 5 | Selección del módulo en el bus SPI. |
| GDO0 | GPIO 27 | Señal utilizada para los pulsos de radio. |
| GDO2 | GPIO 26 | Conexión prevista en la definición del módulo del firmware. |

Estas asignaciones están definidas en `include/zv_spi_bus.h`, `include/zv_subghz.h` e `include/zv_cc1101.h` del proyecto de firmware. Si cambian allí, revisa también el cableado.

## Comunicación con el ESP32

Para comunicar la interfaz con el ESP32 hice un protocolo de texto sencillo que funciona por UART a 115200 baudios. Me sirve para aprender y ver qué mensajes se intercambian entre los dos. Todavía puede cambiar a medida que avance el proyecto.

Cada mensaje ocupa una línea. Algunos son órdenes cortas; otros llevan datos separados por `|`. Por ejemplo:

```text
SCAN
CONNECT|AA:BB:CC:DD:EE:FF|0
DISCOVER
SUBGHZ_CAPTURE|freq_hz=433920000|timeout_ms=30000
```

La interfaz pide buscar dispositivos, conectar, descubrir servicios o capturar una señal. El ESP32 devuelve mensajes de inicio, resultados, finalización o error, como `SCAN:START`, `SCAN:DONE` o `CONNECT:OK`.

El servicio UART recibe las líneas y las entrega al módulo correspondiente. También permite limitar la espera de una respuesta para informar de errores o tiempos agotados. No hace falta conocer todos los campos para usar la aplicación; para modificarlo, las referencias son [service/uart_commands.h](service/uart_commands.h), [service/uart_service.c](service/uart_service.c) y los controladores BLE y Sub-GHz.

## Arquitectura y directorios

Intento organizar el código por responsabilidades. En los módulos principales, la **vista** dibuja la pantalla, el **controlador** coordina las acciones y el **servicio** se comunica con el sistema o el hardware.

Por ejemplo, al reproducir una señal Sub-GHz: la vista recoge la selección, el controlador solicita la reproducción y el servicio lee el archivo y envía sus datos al ESP32. La respuesta recorre el camino de vuelta para actualizar la pantalla.

```text
                  Raspberry Pi - ZeroVolts UI
                  ==========================
                  Vistas (pantallas y componentes)
                               ^
                               |
                               v
                         Controladores
                               ^
                               |
                               v
                           Servicios
                               ^
                               |
             +-----------------+-----------------+
             |                 |                 |
             v                 v                 v
         Archivos        Hardware local        UART
      (configuracion,     (IR y USB HID)         ^
       mandos, senales)                          |
                                                 v
                                               ESP32
                                                 ^
                                                 |
                                      +----------+----------+
                                      |                     |
                                      v                     v
                                     BLE             Sub-GHz (CC1101)
```

Las flechas dobles representan las solicitudes y los resultados que actualizan la interfaz. IR y USB HID se gestionan en la Raspberry Pi; BLE y Sub-GHz se gestionan en el ESP32, conectado por UART.

Todavía estoy mejorando esta organización. Algunas tareas, como el arranque y la adaptación de los botones a LVGL, siguen en `main.c`.

```text
zerovolts-ui/
├── main.c                  # Arranque, pantalla, botones y bucle principal
├── app_context.[ch]        # Dispositivos BLE y sesiones de captura compartidas
├── config.[ch]             # Carga, guardado y resolución de rutas
├── app-config.json         # Configuración local
├── types.h                 # Tipos y límites comunes
├── Makefile                # Compilación de la aplicación y ejemplos
├── drivers/                # Acceso a hardware: gpio_buttons.[ch]
├── components/             # Controles visuales LVGL reutilizables y tema
├── page/                   # Pantallas, navegación y controladores
│   ├── hid/
│   ├── ir/
│   ├── bt/
│   └── sub_ghz/
├── service/                # Comunicación y operaciones del sistema
│   ├── hid_service.[ch]
│   ├── ir_service.[ch]
│   ├── uart_service.[ch]
│   ├── uart_commands.h
│   ├── subghz/              # Captura, archivos y envío de señales
│   └── ducky_script/        # Intérprete en desarrollo, aún sin integrar
├── utils/                  # Archivos, cadenas, JSON, logs y errores
├── assets/                 # Recursos gráficos
├── data/                   # Scripts, mandos IR, señales Sub-GHz y log
├── scripts/                # Soporte HID y unidades de systemd
├── examples/               # Programas de ejemplo y comprobaciones aisladas
├── tests/                  # Pruebas de navegación, UART y Sub-GHz
├── docs/                   # Notas y planificación
├── case/                   # Archivos relacionados con la carcasa
└── bin/                    # Ejecutables generados
```

## Configuración y datos

[app-config.json](app-config.json) define las rutas de datos y los dispositivos del montaje:

| Sección | Qué configura |
|---|---|
| `version` | Versión mostrada en la barra superior. |
| `display` | Framebuffer, actualmente `/dev/fb0`. |
| `uart` | Puerto del ESP32 y velocidad, actualmente `/dev/ttyAMA5` a 115200. |
| `hid` | Carpeta de scripts, selección y estado de activación. |
| `ir` | Carpeta de mandos, backend, dispositivos RX/TX y tiempo de aprendizaje. |
| `subghz` | Carpeta de señales guardadas. |

La aplicación busca la configuración un nivel por encima de la carpeta del ejecutable. Las carpetas de mandos IR y scripts HID se resuelven respecto a la raíz del proyecto. La ruta Sub-GHz todavía se usa tal como está configurada: si es relativa, depende del directorio desde el que ejecutes la aplicación. Por eso los comandos de esta guía se ejecutan desde la raíz del repositorio. Mantén la distribución `bin/` junto a `app-config.json` y las carpetas de recursos al mover la instalación.

Los datos del montaje se organizan así:

```text
data/
├── badusb/                  # Scripts para la función HID
├── ir/remotes/
│   └── <mando>/
│       ├── meta.json
│       └── buttons/*.raw
├── subghz/signals/*.sub     # Capturas de radio guardadas
└── app-log.log              # Registro de la aplicación
```

## Ejemplos y pruebas

Incluí algunos ejemplos para probar partes concretas sin tener que abrir toda la interfaz:

| Archivo | Qué permite explorar |
|---|---|
| [main_hello_world.c](examples/main_hello_world.c) | Arranque básico de LVGL y texto en pantalla. |
| [main_touch_calibration.c](examples/main_touch_calibration.c) | Lectura y calibración del touch. |
| [main_touch_switch.c](examples/main_touch_switch.c) | Navegación con pulsadores GPIO. |

Para compilar y ejecutar el ejemplo básico:

```bash
make example NAME=hello_world
./bin/example-hello_world
```

La regla genérica de ejemplos del `Makefile` solo compila el archivo principal, por lo que todavía requiere ajustes para ellos. El ejemplo de pulsadores conserva su propia inicialización GPIO y su configuración de pantalla.

## Estado y próximos aprendizajes

Quiero seguir reuniendo herramientas para explorar dispositivos y señales desde un mismo equipo y aprender cómo funciona cada parte mientras lo construyo. Todavía tengo trabajo pendiente con la captura y reproducción de radio, la integración HID y la organización del código.

La configuración que aparece aquí corresponde a mi montaje. Si usas otra pantalla, una versión diferente de LVGL o cambias el firmware, puede que tengas que adaptar algunas cosas.
