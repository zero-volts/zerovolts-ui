#!/usr/bin/env python3
import os
import time
import json
import subprocess

CONFIG_PATH = "/home/zerovolts/git/zerovolts-ui/app-config.json"
GADGET_NAME = "zerovolts-hid"

def log(msg):
    # /home/zerovolts/git/zerovolts-ui/scripts/hid_watcher.py
    with open("/home/zerovolts/git/zerovolts-ui/zv-hid-session.log", "a") as f:
        f.write(msg + "\n")

def get_udc_state():
    udc_root = "/sys/class/udc"
    try:
        names = os.listdir(udc_root)
        if not names:
            return None
        name = names[0]
        with open(os.path.join(udc_root, name, "state")) as f:
            return f.read().strip()
    except Exception as e:
        print(f"[ZV] Error leyendo UDC state: {e}")
        return None


def load_selected_script():
    if not os.path.exists(CONFIG_PATH):
        log(f"[ZV] Config no existe: {CONFIG_PATH}")
        return None, False

    try:
        with open(CONFIG_PATH, "r") as f:
            data = json.load(f)

        hid = data.get("hid", {})
        log(f"[ZV] hid: {hid}")

        script = hid.get("script_selected_path", "")

        log(f"[ZV] script: {script}")
        enabled = hid.get("is_enabled", False)
        log(f"[ZV] enabled: {enabled}")

        if not script:
            return None, enabled

        return script, enabled

    except Exception as e:
        log(f"[ZV] Error leyendo JSON: {e}")
        return None, False


def main():
    script, enabled = load_selected_script()

    if not enabled:
        log("[ZV] HID no está habilitado (is_enabled=false)")
        return

    if not script:
        log("[ZV] No hay script seleccionado")
        return

    if not os.path.exists(script):
        log(f"[ZV] Script no existe: {script}")
        return

    log("[ZV] Esperando enumeración del host (UDC configured)...")

    last_state = None
    while True:
        state = get_udc_state()

        if state != last_state:
            log(f"[ZV] UDC state: {state}")
            last_state = state

        if state == "configured":
            log(f"[ZV] Host conectado. Ejecutando script: {script}")
            subprocess.run(["python3", script])
            break

        time.sleep(0.5)


if __name__ == "__main__":
    main()
