#!/usr/bin/env python3
"""Adjusts the ORIGINAL hero sprite (assets/graphics/original/player.png)
without redrawing it: red coat -> deep blue coat, blond hair & beard ->
dark brown.  Every pixel keeps its original brightness, so all the
original shading and detail survive."""
import os, colorsys
import numpy as np
from PIL import Image

G = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics')
im = Image.open(os.path.join(G, 'original', 'player.png')).convert('RGBA')
a = np.asarray(im).copy()
for y in range(a.shape[0]):
    for x in range(a.shape[1]):
        r, g, b, al = a[y, x]
        if al == 0: continue
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        hd = h * 360
        if (hd < 6 or hd > 354) and s > 0.25:          # the red coat
            h, s, v = 222 / 360, min(1, s * 0.85), v * 0.95
        elif 33 <= hd <= 42 and s > 0.45:              # blond hair / beard / straps
            h, s, v = 24 / 360, s * 0.7, v * 0.72
        else:
            continue
        a[y, x, :3] = [int(round(c * 255)) for c in colorsys.hsv_to_rgb(h, s, v)]
Image.fromarray(a, 'RGBA').save(os.path.join(G, 'player.png'))
print('hero adjusted')
