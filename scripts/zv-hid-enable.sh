#!/usr/bin/env bash
# MUST be run as root

GADGET_DIR=/sys/kernel/config/usb_gadget/zerovolts-hid
UDC_NAME=$(ls /sys/class/udc)
echo "$UDC_NAME" > "$GADGET_DIR/UDC"

echo "Script enabled"