# Raspberry Pi + ParrotOS + LVGL 9
Guía básica y práctica para compilar y ejecutar proyectos usando **LVGL v9**,  
con driver **fbdev** para framebuffer y **evdev** para pantalla táctil SPI.

---

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

## 📚 2. Instalar dependencias necesarias

```bash
sudo apt install cmake
sudo apt install libevdev-dev
```

> `libevdev-dev` es obligatorio para usar el driver de touch.

---

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

## ▶️ 4. Ejecutar demo oficial (opcional)

```bash
./build/bin/lvglsim
```

---

# 📝 5. Deshabilitar demos y ejemplos

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

# 🚀 6. Compilar tu aplicación con LVGL

Se recomienda **usar g++ para enlazar** debido a dependencias internas
del port Linux que requieren `libstdc++`.

### Comando:

```bash
g++ main.c componentes/top_bar.c -o zero-volts-ui \
  -DLV_CONF_INCLUDE_SIMPLE \
  -I. \
  -I./componentes \
  -I/home/zerovolts/git/lv_port_linux/lvgl \
  -I/home/zerovolts/git/lv_port_linux/lvgl/src/drivers/display/fb \
  -I/home/zerovolts/git/lv_port_linux/lvgl/src/drivers/evdev \
  -I/home/zerovolts/git/lv_port_linux/build \
  /home/zerovolts/git/lv_port_linux/build/lvgl/lib/liblvgl.a \
  -lm -lpthread -ldl
```

---
# 7. Habilitar touch

Para poder habilitar el touch y ver los eventos de los controles se debe agregar el siguiente codigo: 

```bash
lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event4");
```

Para conocer cual es el path del dispositvo se puede correr el siguiente comando
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

Se identifica el hardware por el nombre y  el path se obtiene desde sysfs.

Es necesario tambien calibrar la pantalla y para esto se debe agregar el codigo:
```bash
lv_evdev_set_calibration
```

Para conocer las coordenadas al hacer el touch se puede saber ejecutando y probando los clicks
```bash
evtest /dev/input/event4
```

---
# 🧪 8. Ejecutar tu aplicación

```bash
./zero-volts-ui
```
