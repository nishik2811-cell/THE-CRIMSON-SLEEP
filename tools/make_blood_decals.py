#!/usr/bin/env python3
"""Cuts the team's four blood decals out of their reference sheet
(10 screen px per art pixel, one decal per quadrant on a checkerboard)
into assets/graphics/blood_decals.png: 4 cells of 16x16, transparent.
    python3 tools/make_blood_decals.py path/to/blood.webp"""
import os, sys
import numpy as np
from PIL import Image

PITCH = 10
CELL = 16
src = Image.open(sys.argv[1]).convert('RGB')
a = np.asarray(src).astype(int)
H, W, _ = a.shape
red = (a[..., 0] > a[..., 1] + 35) & (a[..., 0] > a[..., 2] + 35)
# find the pixel-grid phase from the reds' left/top edges
xs = np.nonzero(red.any(0))[0]; ys = np.nonzero(red.any(1))[0]
ox, oy = xs.min() % PITCH, ys.min() % PITCH
sheet = Image.new('RGBA', (CELL * 4, CELL), (0, 0, 0, 0))
quads = [(0, 0), (W // 2, 0), (0, H // 2), (W // 2, H // 2)]   # splat, drip, smear, scratch
for i, (qx, qy) in enumerate(quads):
    sub = red[qy:qy + H // 2, qx:qx + W // 2]
    yy, xx = np.nonzero(sub)
    x0, x1, y0, y1 = xx.min() + qx, xx.max() + qx, yy.min() + qy, yy.max() + qy
    # sample the centre of each art pixel
    gx = np.arange(x0 - (x0 - ox) % PITCH + PITCH // 2, x1 + 1, PITCH)
    gy = np.arange(y0 - (y0 - oy) % PITCH + PITCH // 2, y1 + 1, PITCH)
    cell = np.zeros((len(gy), len(gx), 4), np.uint8)
    for j, y in enumerate(gy):
        for k, x in enumerate(gx):
            if red[y, x]:
                cell[j, k] = (*a[y, x], 240)
    img = Image.fromarray(cell, 'RGBA')
    img = img.crop(img.getbbox())
    if img.width > CELL or img.height > CELL:
        img.thumbnail((CELL, CELL), Image.NEAREST)
    sheet.alpha_composite(img, (i * CELL + (CELL - img.width) // 2, (CELL - img.height) // 2))
    print('decal', i, img.size)
out = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics', 'blood_decals.png')
sheet.save(out)
print('wrote', out)
