# Raspberry Pi + ParrotOS + LVGL 9
Guía básica y práctica para compilar y ejecutar proyectos usando **LVGL v9**,  
con driver **fbdev** para framebuffer y **evdev** para pantalla táctil SPI.

### Inicio rápido (proyecto actual)
Desde `zerovolts-ui/`:

```bash
make
./bin/zero-volts-ui
```

Para limpiar:

```bash
make clean
```

## Índice
- [1. Descargar repositorio oficial de LVGL para Linux](#sec-1)
- [2. Instalar dependencias necesarias](#sec-2)
- [3. Compilar LVGL (fbdev + evdev)](#sec-3)
- [4. Ejecutar demo oficial (opcional)](#sec-4)
- [5. Deshabilitar demos y ejemplos](#sec-5)
- [6. Compilar tu aplicación con LVGL](#sec-6)
- [7. Habilitar touch](#sec-7)
- [8. Ejecutar tu aplicación](#sec-8)
- [9. IR: cómo se guardan y se envían señales](#sec-9)
- [10. Arquitectura actual](#sec-10)

---

<a id="sec-1"></a>
## 🔧 1. Descargar repositorio oficial de LVGL para Linux

```bash
git clone https://github.com/lvgl/lv_port_linux.git
cd lv_port_linux/
git submodule update --init --recursive
```

Esto descarga:

- LVGL (motor gráfico)
- Drivers Linux (`fbdev`, `evdev`)
- Ejemplos y demos

---

<a id="sec-2"></a>
## 📚 2. Instalar dependencias necesarias

```bash
sudo apt install cmake
sudo apt install libevdev-dev
```

> `libevdev-dev` es obligatorio para usar el driver de touch.

---

<a id="sec-3"></a>
## 🛠️ 3. Compilar LVGL (fbdev + evdev)

```bash
cmake -B build -DCONFIG=fbdev
cmake --build build -j$(nproc)
```

La librería compilada queda en:

```
lv_port_linux/build/lvgl/lib/liblvgl.a
```

Drivers disponibles:

```
lv_port_linux/lvgl/src/drivers/display/fb/
lv_port_linux/lvgl/src/drivers/evdev/
```

---

<a id="sec-4"></a>
## ▶️ 4. Ejecutar demo oficial (opcional)

```bash
./build/bin/lvglsim
```

---

<a id="sec-5"></a>
## 📝 5. Deshabilitar demos y ejemplos

Editar:

```
lv_port_linux/CMakeLists.txt
```

Cambiar:

```cmake
set(LVGL_ENABLE_DEMO ON) → OFF  
set(LVGL_ENABLE_EXAMPLES ON) → OFF  
```

Recompilar:

```bash
cmake --build build -j$(nproc)
```

---

<a id="sec-6"></a>
## 🚀 6. Compilar tu aplicación con LVGL

Forma recomendada (usa el `Makefile` del proyecto):

```bash
make
```

Salida principal:

```bash
bin/zero-volts-ui
```

Si quieres compilar manualmente, toma como referencia el comando generado por `make`
y los includes/librerías definidos en `Makefile`.

---
<a id="sec-7"></a>
## 7. Habilitar touch

Para habilitar el touch y ver eventos de entrada:

```bash
lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event4");
```

Para conocer cuál es el path del dispositivo:
```bash
cat /proc/bus/input/devices
``` 

y da como resultado algo como esto:
```bash
I: Bus=001c Vendor=0000 Product=1ea6 Version=0000
N: Name="ADS7846 Touchscreen"
P: Phys=spi0.1/input0
S: Sysfs=/devices/platform/soc/fe204000.spi/spi_master/spi0/spi0.1/input/input4
U: Uniq=
H: Handlers=mouse0 event4
B: PROP=0
B: EV=b
B: KEY=400 0 0 0 0 0
B: ABS=1000003
```

Se identifica el hardware por el nombre y el path se obtiene desde `sysfs`.

También es necesario calibrar la pantalla:
```bash
lv_evdev_set_calibration
```

Para conocer coordenadas al tocar la pantalla:
```bash
evtest /dev/input/event4
```

---
<a id="sec-8"></a>
## 🧪 8. Ejecutar tu aplicación

```bash
./bin/zero-volts-ui
```

---

<a id="sec-9"></a>
## 📡 9. IR: cómo se guardan y se envían señales

## Estructura de archivos

Las señales aprendidas se guardan en la ruta configurada en `app-config.json`:

```bash
ir.remotes_path/<remote>/buttons/<button>.raw
```

Ejemplo con la configuración actual:

```bash
/home/zerovolts/git/zerovolts-ui/data/ir/remotes/ventilador/buttons/off.raw
```

## Captura (learn)

El backend actual usa `ir-ctl`:

```bash
timeout 7s ir-ctl -r -1 -d '/dev/lirc1' > '<button>.raw.tmp'
```

Luego el archivo temporal se valida y, si pasa, se renombra a `*.raw`.

## Envío (send)

Para transmitir una señal guardada:

```bash
ir-ctl -d '/dev/lirc0' -s '<button>.raw'
```

## Formato esperado del archivo `.raw`

- Tokens con signo (`+` pulso, `-` espacio), por ejemplo: `+1270 -397 +1279 -396 ...`
- Debe tener cantidad mínima de tokens (`IR_RAW_MIN_TOKENS`, hoy 160).
- Debe tener cantidad **par** de tokens (pulso/espacio).

## Falla común: captura impar al final

A veces una captura válida en apariencia termina en `+...` (pulso) sin el `-...` final (gap),
dejando un total impar de tokens. En ese caso la validación falla.

Se agregó una normalización automática:

- Si el archivo queda impar y termina en pulso (`+`), se agrega un gap sintético:
  `-20000`
- Se vuelve a validar.
- Si pasa, se acepta y se guarda como `*.raw`.

## Archivos inválidos para diagnóstico

Si la captura sigue fallando, el intento se conserva para análisis:

```bash
<button>.raw.invalid1
<button>.raw.invalid2
<button>.raw.invalid3
```

Esto permite comparar contra un archivo que sí funciona (`off.raw`, por ejemplo).

## Comandos útiles de comparación

Primero define `REMOTES_PATH`:

```bash
REMOTES_PATH="/home/zerovolts/git/zerovolts-ui/data/ir/remotes"
```

Luego ejecuta:

```bash
ls -l "$REMOTES_PATH/<remote>/buttons/"
wc -w "$REMOTES_PATH/<remote>/buttons/off.raw" "$REMOTES_PATH/<remote>/buttons/<button>.raw.invalid1"
head -n 1 "$REMOTES_PATH/<remote>/buttons/off.raw"
head -n 1 "$REMOTES_PATH/<remote>/buttons/<button>.raw.invalid1"
```

---

<a id="sec-10"></a>
## 🧱 10. Arquitectura actual

El proyecto usa una arquitectura por capas por funcionalidad:
- `view`: pantallas LVGL y manejo de eventos de UI.
- `controller`: reglas de negocio, validaciones y orquestación.
- `service`: interacción con sistema operativo/hardware (IR, HID, filesystem, comandos).

Ventajas principales:
- Menor acoplamiento entre UI y hardware.
- Mejor testabilidad del negocio sin depender de dispositivos reales.
- Más facilidad para cambiar backends sin tocar las vistas.
- Mantenimiento más simple cuando crece el proyecto.

## Diagrama ASCII (arquitectura actual)

```text
┌──────────────────────────────────────────────┐
│                    main.c                    │
│ init LVGL + config + navegación + bucle      │
└───────────────────────┬──────────────────────┘
                        │
        ┌───────────────┴───────────────┐
        │                               │
┌───────▼────────────────────┐  ┌───────▼────────────────────┐
│          Módulo IR         │  │          Módulo HID        │
└───────┬────────────────────┘  └───────┬────────────────────┘
        │                                │
┌───────▼─────────────┐          ┌───────▼─────────────┐
│ page/ir/*.c         │          │ page/hid/hid_view.c │
│ (vistas LVGL)       │          │ (vista LVGL)        │
└───────┬─────────────┘          └───────┬─────────────┘
        │                                │
┌───────▼──────────────────┐     ┌───────▼──────────────────┐
│ page/ir/ir_controller.c  │     │ page/hid/hid_controller.c│
│ (lógica de negocio IR)   │     │ (lógica de negocio HID)  │
└───────┬──────────────────┘     └───────┬──────────────────┘
        │                                │
┌───────▼──────────────────┐     ┌───────▼──────────────────┐
│ service/ir_service.c     │     │ service/hid_service.c    │
│ ir-ctl + filesystem      │     │ scripts + systemctl      │
└──────────────────────────┘     └──────────────────────────┘
```

## Flujo de datos

```text
Evento UI
  -> Controller (validación/reglas)
    -> Service (I/O)
      -> Resultado/estado
        -> UI
```

## Estructura de directorios (resumen)

```text
zerovolts-ui/
├── main.c
├── config.*
├── components/
├── page/
│   ├── base_view.*
│   ├── home_view.*
│   ├── hid/
│   │   ├── hid_view.*
│   │   └── hid_controller.*
│   └── ir/
│       ├── ir.*
│       ├── views (remotes/new_remote/learn_button/send_signal)
│       ├── ir_controller.*
│       └── ir_raw_helper.*
├── service/
│   ├── ir_service.*
│   └── hid_service.*
├── utils/
├── examples/
└── scripts/
```
