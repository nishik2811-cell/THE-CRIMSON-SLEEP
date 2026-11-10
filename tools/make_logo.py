#!/usr/bin/env python3
"""Turns the team's Crimson Sleep logo (dark ink on white) into a
transparent crimson PNG for the title screen: white -> alpha
(colour-to-alpha), ink re-coloured to the title crimson."""
import sys, os
import numpy as np
from PIL import Image
src = sys.argv[1]
out = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics', 'title_logo.png')
im = Image.open(src).convert('RGBA')
a4 = np.asarray(im, np.float32) / 255.0
a = a4[..., :3]
ink = 1.0 - a.min(axis=2)                 # how far each pixel is from white
alpha = np.clip((ink - 0.04) / 0.55, 0, 1) * a4[..., 3]   # respect existing transparency
rgb = np.zeros_like(a)
rgb[...] = np.array([176, 30, 58]) / 255.0   # crimson, as on the Godot title
res = np.dstack([rgb, alpha]) * 255
img = Image.fromarray(res.astype(np.uint8), 'RGBA')
img = img.crop(img.getbbox())
img.save(out)
print('logo', img.size)
