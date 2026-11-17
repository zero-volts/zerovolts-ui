#!/usr/bin/env bash
set -euo pipefail
GADGET_DIR=/sys/kernel/config/usb_gadget/zerovolts-hid
printf '\n' > "$GADGET_DIR/UDC"
