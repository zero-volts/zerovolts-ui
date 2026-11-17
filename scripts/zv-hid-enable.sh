#!/usr/bin/env bash
set -euo pipefail
GADGET_DIR=/sys/kernel/config/usb_gadget/zerovolts-hid
UDC_NAME=$(ls /sys/class/udc)
printf '%s\n' "$UDC_NAME" > "$GADGET_DIR/UDC"
