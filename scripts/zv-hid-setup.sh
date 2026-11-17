#!/usr/bin/env bash
# https://www.usb.org/sites/default/files/documents/hid1_11.pdf
# https://usb.org/sites/default/files/hut1_3_0.pdf
# https://docs-kernel-org.translate.goog/usb/gadget_configfs.html?_x_tr_sl=en&_x_tr_tl=es&_x_tr_hl=es&_x_tr_pto=tc
# https://docs-kernel-org.translate.goog/filesystems/configfs.html?_x_tr_sl=en&_x_tr_tl=es&_x_tr_hl=es&_x_tr_pto=tc
# https://randomnerdtutorials.com/raspberry-pi-zero-usb-keyboard-hid/

GADGET_DIR=/sys/kernel/config/usb_gadget/zerovolts-hid
UDC_PATH="$GADGET_DIR/UDC"
STRING_LANGUAGE_PATH="$GADGET_DIR/strings/0x409"
CONFIG_LINK="$GADGET_DIR/configs/c.1/hid.usb0"

# If the gadget is already bind just exit!
if [ -d "$GADGET_DIR" ] && [ -s "$UDC_PATH" ]; then
    echo "[0/10] Gadget already exists and is bound to $(cat "$UDC_PATH"). Nothing to do."
    exit 0
fi

# 1.- Mounting configfs if is not (this allow to create the gadget)
if ! mountpoint -q /sys/kernel/config; then
    echo "[1/10] Mounting configfs in  /sys/kernel/config..."
    mount -t configfs none /sys/kernel/config
else
    echo "[1/10] configfs already mounted."
fi

# 2.- Loading the libcomposite module
echo "[2/10] Loading libcomposite module ..."
if ! lsmod | grep -q '^libcomposite'; then
    modprobe libcomposite
else 
    echo "*** libcomposite already loaded"
fi

# 3 .- Making the gadget directory (virtual obeject)
echo "[3/10] Making the gadget directory $GADGET_DIR"
if [ -d "$GADGET_DIR" ]; then
    echo "  *** The gadget already exists"
else
    mkdir -p "$GADGET_DIR"
fi

cd "$GADGET_DIR"

# 4.- Defining the device descriptor (E.1 Device Descriptor - PDF).
echo "[4/10] Writing the device descriptor"
printf '0x1d6b' > idVendor      # Linux Foundation
printf '0x0104' > idProduct     # Multifunction Composite Gadget
printf '0x0100' > bcdDevice     # v1.0.0
printf '0x0200' > bcdUSB        # USB2
echo "  *** idVendor: $(cat idVendor), idProduct: $(cat idProduct), bcdUSB: $(cat bcdUSB)"

# 5 .- Strings configuration
# The 0x409 = en-US
echo "[5/10] Writing device string configuration"
mkdir -p "$STRING_LANGUAGE_PATH"
printf 'Zerovolts'          > "$STRING_LANGUAGE_PATH/manufacturer"
printf 'zv-hid'             > "$STRING_LANGUAGE_PATH/product"
printf 'zv2500000001'       > "$STRING_LANGUAGE_PATH/serialnumber"
echo "  *** manufacturer: $(cat "$STRING_LANGUAGE_PATH/manufacturer"), product: $(cat "$STRING_LANGUAGE_PATH/product")"

# 6 .- Making the gadget configuration (E.2 Configuration Descriptor - PDF)
echo "[6/10] Gadget configuration"
mkdir -p "$GADGET_DIR/configs/c.1/strings/0x409"
printf 'Config : HID keyboard'  > "$GADGET_DIR/configs/c.1/strings/0x409/configuration"
printf '100'                    > "$GADGET_DIR/configs/c.1/MaxPower" #100mA

echo "[7/10] Making HID function "
mkdir -p "$GADGET_DIR/functions/hid.usb0"
printf '1' > "$GADGET_DIR/functions/hid.usb0/protocol"
printf '1' > "$GADGET_DIR/functions/hid.usb0/subclass"

# This is the size of the data send when is pressed a key in the keyboard
# B.1 Protocol 1 (Keyboard) - PDF
#----------------------------------------------
#| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
#----------------------------------
# 0 Modifier keys
# 1 Reserved
# 2 Keycode 1
# 3 Keycode 2
# 4 Keycode 3
# 5 Keycode 4
# 6 Keycode 5
# 7 Keycode 6
printf '8' > "$GADGET_DIR/functions/hid.usb0/report_length" 
echo "  *** protocol: $(cat "$GADGET_DIR/functions/hid.usb0/protocol")"

# 8.- Report descriptor (E.6 Report Descriptor (Keyboard) - PDF)
# 05 01 Usage Page (Generic Desktop)
# 09 06: Usage (Keyboard)
# a1 01: Collection (Application)
    # 05 07: Usage Page (Keyboard)
    # 19 e0: Usage Minimum (Keyboard LeftControl)
    # 29 e7: Usage Maximum (Keyboard Right GUI)
    # 15 00: Logical Minimum (0)
    # 25 01: Logical Maximum (1)
    # 75 01: Report Size (1)
    # 95 08: Report Count (8)
    # 81 02: Input (Data, Variable, Absolute)
    # 95 01: Report Count (1)
    # 75 08: Report Size (8)
    # 95 05: Report Count (5)
    # 75 01: Report Size (1)
    # 05 08: Usage Page (LEDs)
    # 19 01: Usage Minimum (Num Lock)
    # 29 05: Usage Maximum (Kana)
    # 91 02: Output (Data, Variable, Absolute)
    # 95 01: Report Count (1)
    # 75 03: Report Size (3)
    # 95 06: Report Count (6)
    # 75 08: Report Size (8)
    # 15 00: Logical Minimum (0)
    # 25 65: Logical Maximum (101)
    # 05 07: Usage Page (Keyboard)
    # 19 00: Usage Minimum (0)
    # 29 65: Usage Maximum (101)
    # 81 00: Input (Data, Array)
# c0: End Collection
echo "[8/10] Writing report description"
cat > /tmp/tmp_descriptor.hex <<'EOF'
    05 01 09 06 a1 01 05 07 19 e0
    29 e7 15 00 25 01 75 01 95 08
    81 02 95 01 75 08 81 03 95 05
    75 01 05 08 19 01 29 05 91 02
    95 01 75 03 91 03 95 06 75 08
    15 00 25 65 05 07 19 00 29 65
    81 00 c0
EOF

xxd -r -p /tmp/tmp_descriptor.hex > "$GADGET_DIR/functions/hid.usb0/report_desc"
hexdump -C "$GADGET_DIR/functions/hid.usb0/report_desc"

# 9.- Associating the functions with their configurations
echo "[9/10] Linking the function to the configuration"
# checking configs/c.1 already exists before linking
if [ ! -d "$GADGET_DIR/configs/c.1" ]; then
    echo " ERROR $GADGET_DIR/configs/c.1 doesn't existes. Making it..."
    mkdir -p "$GADGET_DIR/configs/c.1/strings/0x409"
fi

if [ ! -L "$CONFIG_LINK" ]; then
    ln -s "$GADGET_DIR/functions/hid.usb0" "$CONFIG_LINK"
    echo "  *** Linked hid.usb0 -> configs/c.1"
else
    echo "  *** Link hid.usb0 already exists"
fi

echo "[10/10] Showing UDC available in the system"
ls /sys/class/udc

echo "To bind manually replace <UDC_NAME> by 'ls /sys/class/udc' result"
echo "  *** sudo bash -c 'echo <UDC_NAME> > $UDC_PATH'"
echo "To unbind"
echo "  *** sudo bash -c 'echo \"\" > $UDC_PATH'"