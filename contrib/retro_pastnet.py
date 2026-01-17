#!/usr/bin/env python3
import sys
import os
import json
import hashlib
from datetime import datetime
from textwrap import fill

import requests
import secrets
import subprocess

WAYBACK_API = "http://archive.org/wayback/available"
CHAIN_FILE = "retro_chain.jsonl"
IDENTITY_FILE = "retro_identity.json"


def open_in_browser(url: str):
    """
    Intenta abrir la URL en el navegador usando termux-open-url.
    """
    print(f"🌐 Intentando abrir en el navegador: {url}")
    try:
        subprocess.run(["termux-open-url", url], check=True)
        print("✅ Comando termux-open-url ejecutado.")
    except Exception as e:
        print("⚠️ No se pudo abrir automáticamente el navegador.")
        print("   Error:", e)
        print("👉 Copia y pega manualmente esta URL en tu navegador:")
        print("   ", url)


def load_or_create_identity(path=IDENTITY_FILE):
    """
    Identidad local sencilla:
    - secret: clave secreta aleatoria (hex)
    - id: hash de esa clave (tu 'DID' local)
    """
    if os.path.exists(path):
        with open(path, "r") as f:
            data = json.load(f)
        return data["secret"], data["id"]
    else:
        secret = secrets.token_hex(32)  # 256 bits aleatorios
        id_ = hashlib.sha256(secret.encode()).hexdigest()
        data = {
            "secret": secret,
            "id": id_,
            "created_at": datetime.utcnow().isoformat() + "Z"
        }
        with open(path, "w") as f:
            json.dump(data, f, indent=2)
        return secret, id_


def get_snapshot(url: str, date_str: str):
    """
    Pide a Wayback Machine el snapshot más cercano a 'date_str' (YYYY-MM-DD)
    para la URL dada. Wayback ya devuelve el 'closest'.
    """
    try:
        d = datetime.strptime(date_str, "%Y-%m-%d")
    except ValueError:
        raise ValueError("Fecha inválida, usa formato YYYY-MM-DD")
    ts = d.strftime("%Y%m%d235959")
    params = {"url": url, "timestamp": ts}
    r = requests.get(WAYBACK_API, params=params, timeout=15)
    r.raise_for_status()
    data = r.json()
    snaps = data.get("archived_snapshots", {})
    if "closest" not in snaps:
        raise RuntimeError("No se encontró snapshot para esa fecha/URL")
    closest = snaps["closest"]
    return closest["url"], closest["timestamp"], closest.get("status")


def compute_day_delta(requested_date: str, snap_ts: str) -> int:
    """
    Calcula la diferencia en días entre la fecha pedida (YYYY-MM-DD)
    y la fecha real del snapshot (timestamp Wayback en formato YYYYMMDDhhmmss).
    """
    d_req = datetime.strptime(requested_date, "%Y-%m-%d")
    d_snap = datetime.strptime(snap_ts[:8], "%Y%m%d")
    delta = d_snap - d_req
    return delta.days


def fetch_content(snap_url: str):
    """Descarga el contenido HTML del snapshot."""
    r = requests.get(snap_url, timeout=20)
    r.raise_for_status()
    return r.text


def get_last_hash(path=CHAIN_FILE):
    """Devuelve el hash del último bloque de la cadena local."""
    if not os.path.exists(path):
        return "0" * 64
    last = None
    with open(path, "r") as f:
        for line in f:
            if line.strip():
                last = json.loads(line)
    if last is None:
        return "0" * 64
    return last["entry_hash"]


def hash_entry(entry: dict) -> str:
    """
    Hash determinista de la entrada (sin el campo entry_hash).
    """
    data = {k: v for k, v in entry.items() if k != "entry_hash"}
    dumped = json.dumps(data, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(dumped).hexdigest()


def sign_entry(secret: str, entry: dict) -> str:
    """
    'Firma' simple: H(secret || mensaje) con SHA-256.
    (MAC simbólico para tu demo.)
    """
    msg = json.dumps(entry, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(secret.encode() + msg).hexdigest()


def main():
    # Soporte simple para flag --wormhole
    wormhole_mode = False
    args = sys.argv[1:]

    if not args:
        print("Uso:")
        print("  retro_pastnet.py [--wormhole] <YYYY-MM-DD> <URL>")
        print("Ejemplo:")
        print("  retro_pastnet.py --wormhole 2009-12-01 https://bitcoin.org")
        sys.exit(1)

    if args[0] == "--wormhole":
        wormhole_mode = True
        if len(args) < 3:
            print("Uso con wormhole:")
            print("  retro_pastnet.py --wormhole <YYYY-MM-DD> <URL>")
            sys.exit(1)
        date_str = args[1]
        url = args[2]
    else:
        if len(args) < 2:
            print("Uso:")
            print("  retro_pastnet.py <YYYY-MM-DD> <URL>")
            sys.exit(1)
        date_str = args[0]
        url = args[1]

    # 1) Identidad tipo DID local
    secret, node_id = load_or_create_identity()
    modo = "WORMHOLE" if wormhole_mode else "NORMAL"
    print(f"✅ Nodo local ID: {node_id[:16]}...  (modo {modo})")

    # 2) Consultar Wayback Machine
    print(f"🔎 Buscando snapshot de '{url}' alrededor de {date_str} en Wayback...")
    snap_url, snap_ts, status = get_snapshot(url, date_str)
    day_delta = compute_day_delta(date_str, snap_ts)

    print(f"   ➜ Snapshot encontrado: {snap_url}")
    print(f"   ➜ Timestamp archivado : {snap_ts} (status HTTP original: {status})")
    print(f"   ➜ Desfase temporal    : {day_delta} días respecto a {date_str}")

    if wormhole_mode:
        print("🌀 WORMHOLE: este snapshot es la aproximación más cercana que Wayback tiene a tu fecha objetivo.")

    # 2b) Abrir en el navegador
    open_in_browser(snap_url)

    # 3) Descargar contenido (opcional, pero lo usamos para la preview)
    html = fetch_content(snap_url)
    preview = html[:3000]  # primeras 3000 chars

    print("\n===== PREVIEW CONTENIDO ARCHIVADO =====\n")
    for line in preview.splitlines():
        if line.strip():
            print(fill(line, width=100))
    print("\n===== FIN PREVIEW =====\n")

    # 4) Crear 'bloque' de navegación y 'firmarlo'
    now = datetime.utcnow().isoformat() + "Z"
    prev_hash = get_last_hash()
    session_entry = {
        "url": url,
        "requested_date": date_str,
        "snapshot_url": snap_url,
        "snapshot_ts": snap_ts,
        "visited_at_utc": now,
        "viewer_id": node_id,
        "prev_hash": prev_hash,
        "wormhole_mode": wormhole_mode,
        "day_delta": day_delta,
    }

    signature = sign_entry(secret, session_entry)
    session_entry["signature"] = signature

    entry_hash = hash_entry(session_entry)
    session_entry["entry_hash"] = entry_hash

    with open(CHAIN_FILE, "a") as f:
        f.write(json.dumps(session_entry, sort_keys=True) + "\n")

    print(f"🧱 Entrada añadida a tu cadena local {CHAIN_FILE}")
    print(f"   ➜ entry_hash: {entry_hash}")
    print(f"   ➜ prev_hash : {prev_hash}")


if __name__ == "__main__":
    main()
