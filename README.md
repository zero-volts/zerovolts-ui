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
g++ main.c -o zero-volts-ui \
 -DLV_CONF_INCLUDE_SIMPLE \
 -I/home/zerovolts/git/lv_port_linux/lvgl \
 -I/home/zerovolts/git/lv_port_linux/lvgl/src/drivers/display/fb \
 -I/home/zerovolts/git/lv_port_linux/lvgl/src/drivers/evdev \
 -I/home/zerovolts/git/lv_port_linux/build \
 /home/zerovolts/git/lv_port_linux/build/lvgl/lib/liblvgl.a
 -lm -lpthread -ldl
```

---
# 🧪 7. Ejecutar tu aplicación

```bash
./zero-volts-ui
```
