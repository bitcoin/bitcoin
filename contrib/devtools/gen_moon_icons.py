#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import math
import os
import struct
import zlib

OUTPUT_PX = 64
CENTER = OUTPUT_PX / 2
RADIUS = 26
STROKE_WIDTH = 2.0
INNER_GAP_INSET = 3.0


def make_png(pixels):
    """Encode RGBA pixel data as a PNG file."""
    raw = bytearray()
    for y in range(OUTPUT_PX):
        raw.append(0)  # filter byte: None
        for x in range(OUTPUT_PX):
            raw.extend(pixels[y][x])

    def chunk(chunk_type, data):
        c = chunk_type + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)

    ihdr = struct.pack('>IIBBBBB', OUTPUT_PX, OUTPUT_PX, 8, 6, 0, 0, 0)  # 8-bit RGBA
    idat = zlib.compress(bytes(raw), 9)

    out = b'\x89PNG\r\n\x1a\n'
    out += chunk(b'IHDR', ihdr)
    out += chunk(b'IDAT', idat)
    out += chunk(b'IEND', b'')
    return out


def moon_frame(frame):
    """
    frame 0 = new moon        (filled disc + thin inner outline gap)
    frame 1 = waxing crescent (small sliver on right)
    frame 2 = first quarter   (right half illuminated)
    frame 3 = waxing gibbous  (mostly illuminated)
    frame 4 = full moon       (outline only)
    frame 5 = waning gibbous  (mostly illuminated, from left)
    frame 6 = last quarter    (left half illuminated)
    frame 7 = waning crescent (small sliver on left)
    """
    if frame > 4:
        # Waning: horizontal mirror of the corresponding waxing frame
        pixels = moon_frame(8 - frame)
        for y in range(OUTPUT_PX):
            pixels[y] = pixels[y][::-1]
        return pixels

    # Precompute frame-specific parameters
    if frame == 0:
        gap_outer = RADIUS - INNER_GAP_INSET
        gap_inner = gap_outer - STROKE_WIDTH
    else:
        frame_angle = frame * math.pi / 4

    pixels = [[(0, 0, 0, 0)] * OUTPUT_PX for _ in range(OUTPUT_PX)]
    for y in range(OUTPUT_PX):
        for x in range(OUTPUT_PX):
            dx = x - CENTER + 0.5
            dy = y - CENTER + 0.5
            dist = math.sqrt(dx * dx + dy * dy)
            if dist > RADIUS + 1:
                continue
            disc_alpha = min(1.0, RADIUS + 1 - dist)
            if frame == 0:
                # New moon: filled disc with a thin inner outline gap
                if dist < gap_inner - 0.5:
                    fill = 1.0
                elif dist < gap_inner + 0.5:
                    fill = gap_inner + 0.5 - dist
                elif dist < gap_outer - 0.5:
                    fill = 0.0
                elif dist < gap_outer + 0.5:
                    fill = dist - (gap_outer - 0.5)
                else:
                    fill = 1.0
            else:
                # Frames 1-4: terminator (shadow boundary) + outline
                if abs(dy) >= RADIUS:
                    terminator_x = 0
                else:
                    half_chord = math.sqrt(RADIUS * RADIUS - dy * dy)
                    terminator_x = math.cos(frame_angle) * half_chord
                edge_dist = terminator_x - dx
                if edge_dist > 1:
                    dark_alpha = 1.0
                elif edge_dist < -1:
                    dark_alpha = 0.0
                else:
                    dark_alpha = (edge_dist + 1) / 2
                outline_alpha = 0.0
                inner_edge = RADIUS - STROKE_WIDTH
                if dist > inner_edge:
                    outline_alpha = min(1.0, (dist - inner_edge) / STROKE_WIDTH)
                fill = max(dark_alpha, outline_alpha)
            alpha = int(max(0, min(255, disc_alpha * fill * 255)))
            if alpha > 0:
                pixels[y][x] = (0, 0, 0, alpha)
    return pixels


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.join(script_dir, '..', '..')
    icon_dir = os.path.join(repo_root, 'src', 'qt', 'res', 'icons')
    for frame in range(8):
        pixels = moon_frame(frame)
        png_data = make_png(pixels)
        filename = f'moon_{frame}.png'
        filepath = os.path.join(icon_dir, filename)
        with open(filepath, 'wb') as f:
            f.write(png_data)
        print(f'  {filename} ({len(png_data)} bytes)')


if __name__ == '__main__':
    main()
