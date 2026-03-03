#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import platform
import shutil
import subprocess
import sys
import tempfile

# Assuming 1024x1024 canvas, the squircle content area is ~864x864 with
# ~80px transparent padding on each side
CONTENT_RATIO = 864 / 1024

DIR_ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
DIR_SRC = os.path.join(DIR_ROOT, "src", "qt", "res", "src")
DIR_OUT = os.path.join(DIR_ROOT, "src", "qt", "res", "icons")

# Icon Composer exports to runtime icon map
ICONS = [
    ("macos_devnet.png", "dash_macos_devnet.png"),
    ("macos_mainnet.png", "dash_macos_mainnet.png"),
    ("macos_regtest.png", "dash_macos_regtest.png"),
    ("macos_testnet.png", "dash_macos_testnet.png"),
]
TRAY = os.path.join(DIR_SRC, "tray.svg")

# Canvas to filename mapping for bundle icon
ICNS_MAP = [
    (16, "icon_16x16.png"),
    (32, "icon_16x16@2x.png"),
    (32, "icon_32x32.png"),
    (64, "icon_32x32@2x.png"),
    (128, "icon_128x128.png"),
    (256, "icon_128x128@2x.png"),
    (256, "icon_256x256.png"),
    (512, "icon_256x256@2x.png"),
    (512, "icon_512x512.png"),
    (1024, "icon_512x512@2x.png"),
]

# Maximum height of canvas is 22pt, we use max height instead of recommended
# 16pt canvas to prevent the icon from looking undersized due to icon width.
# See https://bjango.com/articles/designingmenubarextras/
TRAY_MAP = [
    (22, "dash_macos_tray.png"),
    (44, "dash_macos_tray@2x.png")
]


def sips_resample_padded(src, dst, canvas_size):
    content_size = max(round(canvas_size * CONTENT_RATIO), 1)
    subprocess.check_call(
        ["sips", "-z", str(content_size), str(content_size), "-p", str(canvas_size), str(canvas_size), src, "--out", dst],
        stdout=subprocess.DEVNULL,
    )


def sips_svg_to_png(svg_path, png_path, height):
    subprocess.check_call(
        ["sips", "-s", "format", "png", "--resampleHeight", str(height), svg_path, "--out", png_path],
        stdout=subprocess.DEVNULL,
    )


def generate_icns(tmpdir):
    iconset = os.path.join(tmpdir, "dash.iconset")
    os.makedirs(iconset)

    src_main = os.path.join(DIR_SRC, ICONS[1][0])
    for canvas_px, filename in ICNS_MAP:
        sips_resample_padded(src_main, os.path.join(iconset, filename), canvas_px)

    icns_out = os.path.join(DIR_OUT, "dash.icns")
    subprocess.check_call(["iconutil", "-c", "icns", iconset, "-o", icns_out])
    print(f"Created: {icns_out}")


def check_source(path):
    if not os.path.isfile(path):
        sys.exit(f"Error: Source image not found: {path}")


def main():
    if platform.system() != "Darwin":
        sys.exit("Error: This script requires macOS (needs sips, iconutil).")

    for tool in ("sips", "iconutil"):
        if shutil.which(tool) is None:
            sys.exit(f"Error: '{tool}' not found. Install Xcode command-line tools.")

    check_source(TRAY)
    for src_name, _ in ICONS:
        check_source(os.path.join(DIR_SRC, src_name))

    os.makedirs(DIR_OUT, exist_ok=True)

    # Generate bundle icon
    with tempfile.TemporaryDirectory(prefix="dash_icons_") as tmpdir:
        generate_icns(tmpdir)

    # Generate runtime icons
    for src_name, dst_name in ICONS:
        src = os.path.join(DIR_SRC, src_name)
        dst = os.path.join(DIR_OUT, dst_name)
        sips_resample_padded(src, dst, 256)
        print(f"Created: {dst}")

    # Generate tray icons
    for canvas_px, filename in TRAY_MAP:
        dst = os.path.join(DIR_OUT, filename)
        sips_svg_to_png(TRAY, dst, canvas_px)
        print(f"Created: {dst}")

if __name__ == "__main__":
    main()
