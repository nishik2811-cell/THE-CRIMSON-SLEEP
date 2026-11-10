#!/usr/bin/env python3
"""Crimson re-theme + new horror art.  Reads the untouched sprites from
assets/graphics/original/ and writes the themed versions next to them, so
it can be re-run safely.  Theme rule: near-black and deep blood red - no
pink washes, highlights are darkened rather than tinted."""
import os, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

G = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics')
ORIG = os.path.join(G, 'original')

def load(name):
    return np.asarray(Image.open(os.path.join(ORIG, name)).convert('RGBA'), np.float32)

def save(arr, name):
    Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), 'RGBA').save(os.path.join(G, name))

def lum(a):
    return (0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]) / 255.0

# Floor: dark maroon flagstones (luminance -> black..oxblood ramp).
a = load('bg.png'); l = lum(a) ** 1.35
out = a.copy()
out[..., 0] = 10 + 62 * l; out[..., 1] = 3 + 12 * l; out[..., 2] = 5 + 13 * l
save(out, 'bg.png')

# Tileset: desaturate, push toward red, darken highlights (no pink).
a = load('texture.png'); l = lum(a)[..., None] * 255
rgb = a[..., :3] * 0.55 + l * 0.45                      # 45% desaturate
rgb *= np.array([1.0, 0.72, 0.72])                       # blood shift
rgb *= (0.92 - 0.25 * (l / 255.0))                       # darken bright areas more
a[..., :3] = rgb
save(a, 'texture.png')

# Patrolling ghosts and the Devil: drained, blood-lit.
for name, tint in [('enemy_easy.png', (1.0, 0.45, 0.45)), ('enemy_medium.png', (1.0, 0.45, 0.48)), ('enemy_boss.png', (1.1, 0.35, 0.3))]:
    a = load(name); l = lum(a)[..., None] * 255
    a[..., :3] = (a[..., :3] * 0.4 + l * 0.6) * np.array(tint)
    save(a, name)

# ---- New horror sprites (refined pixel art) ------------------------------
import subprocess, sys
subprocess.run([sys.executable, os.path.join(os.path.dirname(__file__), 'pixel_sprites.py')], check=True)
print('art done')
