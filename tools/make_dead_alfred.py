#!/usr/bin/env python3
"""Alfred after the house took him: built from his own sprite (servent.png).
Glasses -> empty black sockets with a red glint, skin -> corpse grey,
blood running from the sockets, coat darkened."""
import os, colorsys
import numpy as np
from PIL import Image

G = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics')
a = np.asarray(Image.open(os.path.join(G, 'servent.png')).convert('RGBA')).copy()
H, W = a.shape[:2]
lens = np.zeros((H, W), bool)
for y in range(H):
    for x in range(W):
        r, g, b, al = a[y, x]
        if not al: continue
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        hd = h * 360
        if 180 <= hd <= 230 and v > 0.72 and s > 0.15:          # glass lenses
            lens[y, x] = True
            a[y, x, :3] = (12, 2, 4)
        elif 10 <= hd <= 40 and s > 0.25 and v > 0.45:          # skin
            l = v * 0.78
            a[y, x, :3] = (int(150 * l), int(162 * l), int(150 * l))
        elif 200 <= hd <= 250:                                  # coat: darker, colder
            a[y, x, :3] = (np.array(a[y, x, :3]) * 0.6).astype(np.uint8)
# a red glint in each socket, and blood running down from it
for y in range(1, H - 1):
    for x in range(W):
        if lens[y, x] and not lens[y + 1, x]:                     # bottom edge of a lens
            if (x + y) % 3 == 0:
                for k in range(1, 5):
                    if y + k < H and a[y + k, x, 3] and not lens[y + k, x]:
                        a[y + k, x, :3] = (120, 6, 16) if k < 4 else (80, 0, 10)
ys, xs = np.nonzero(lens)
for cy in range(0, H, 48):                                      # glint per frame-row region
    for cx in range(0, W, 24):
        m = (ys >= cy) & (ys < cy + 48) & (xs >= cx) & (xs < cx + 24)
        if m.sum() > 4:
            for half in (xs[m] < cx + 12, xs[m] >= cx + 12):
                if half.sum():
                    yy, xx = ys[m][half], xs[m][half]
                    a[int(yy.mean()), int(xx.mean()), :3] = (230, 20, 36)
Image.fromarray(a, 'RGBA').save(os.path.join(G, 'servent_dead.png'))
print('dead Alfred written')
