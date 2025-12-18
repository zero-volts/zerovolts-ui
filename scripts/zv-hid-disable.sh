#!/usr/bin/env bash
# MUST be run as root

GADGET_DIR=/sys/kernel/config/usb_gadget/zerovolts-hid
echo "" > "$GADGET_DIR/UDC"

echo "Script disabled"