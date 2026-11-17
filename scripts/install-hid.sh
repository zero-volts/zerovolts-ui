#!/usr/bin/env bash

set -euo pipefail

# TODO: Find or investigate the way to use only the user who is running the app
# no the root user, for now work in this way

# We nned root permission to copy these files
if [[ $EUID -ne 0 ]]; then
    echo "Uso: sudo bash scripts/install-hid.sh" >&2
    exit 1
fi

# Checking for extra tools used by zv-hid-gadget-setup.sh
if ! command -v xxd >/dev/null || ! command -v hexdump >/dev/null; then
    echo "Faltan herramientas del setup. Instala: sudo apt install xxd bsdextrautils" >&2
    exit 1
fi

# Getting the root path from this script is stored even if is executed from another directory
scripts_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

# The project directory is the one store the "scripts"
project_root=$(dirname -- "$scripts_dir")

# Where all this files will be copied
install_dir=/usr/local/lib/zerovolts/hid

case "$project_root" in
    *[\"\\\$%]*|*$'\n'*|*$'\r'*)
        echo "La ruta del proyecto contiene caracteres no admitidos por este instalador." >&2
        exit 1
        ;;
esac

install -d -m 755 "$install_dir"
install -o root -g root -m 755 \
    "$scripts_dir"/zv-hid-gadget-setup.sh \
    "$scripts_dir/zv-hid-session.py" \
    "$install_dir/"

# Copy the main service that initialize the HID descriptor setup
# the permission 644 allow read it but only modify it by the root
install -o root -g root -m 644 \
    "$scripts_dir/zv-hid-gadget.service" \
    /etc/systemd/system/

project_sed=${project_root//&/\\&}
project_sed=${project_sed//#/\\#}

# Fill the project path in the session and UI service templates.
for service in zv-hid-session zerovolts-ui; do
    sed "s#@PROJECT_ROOT@#$project_sed#g" \
        "$scripts_dir/$service.service" \
        > "/etc/systemd/system/$service.service"
    chmod 644 "/etc/systemd/system/$service.service"
done

systemctl daemon-reload
systemctl enable --now zv-hid-gadget.service
# Replace previous enablement links, including the old graphical.target link.
systemctl reenable --now zerovolts-ui.service

echo 'HID files installed; hid.config_scripts keeps the script files'
echo 'Services installed and enabled; The UI will start only if the binary exists.'
