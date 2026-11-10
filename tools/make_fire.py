#!/usr/bin/env python3
"""Redraws the fire hazard (texture.png column 22, rows 1-3) as a pixel-art
iron brazier with flames: three animation frames, hard pixels, no blur."""
import os
import numpy as np
from PIL import Image

P = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics', 'texture.png')
tex = Image.open(P).convert('RGBA')
IRON = [(22, 16, 18), (46, 38, 42), (78, 68, 72), (110, 98, 100)]
FIRE = [(110, 8, 10), (178, 26, 16), (232, 82, 22), (252, 160, 40), (255, 226, 120), (255, 250, 214)]

def frame(seed, tall):
    rng = np.random.default_rng(seed)
    a = np.zeros((32, 32, 4), np.uint8)
    def put(x, y, c):
        if 0 <= x < 32 and 0 <= y < 32: a[y, x] = (*c, 255)
    # flames: column heights shaped like a tongue, flickering per frame
    base = 21
    for x in range(8, 24):
        k = 1 - ((x - 15.5) / 8.0) ** 2
        h = int(max(0, k * tall + rng.integers(-2, 3)))
        for dy in range(h):
            y = base - dy
            heat = 1 - dy / max(h, 1)                    # hottest at the bottom
            core = 1 - abs(x - 15.5) / 8.0
            v = heat * 0.65 + core * 0.45
            put(x, y, FIRE[int(np.clip(v * (len(FIRE) - 1), 0, len(FIRE) - 1))])
        if h > 3 and rng.random() < 0.35:                # flame tips break off
            put(x, base - h - 1, FIRE[1])
    for _ in range(3):                                   # sparks
        put(int(rng.integers(9, 23)), int(rng.integers(3, 12)), FIRE[3])
    # iron brazier bowl + legs
    for x in range(7, 25): put(x, 22, IRON[3] if x < 12 else IRON[2])
    for x in range(8, 24):
        for y in range(23, 26): put(x, y, IRON[2] if x < 12 else IRON[1])
    for x in range(10, 22): put(x, 26, IRON[1])
    for x, y in ((10, 27), (10, 28), (10, 29), (21, 27), (21, 28), (21, 29), (15, 27), (16, 27), (15, 28), (16, 28)):
        put(x, y, IRON[0])
    for x in (9, 12, 15, 18, 21): put(x, 24, IRON[0])    # rivets
    # outline
    solid = a[..., 3] > 0
    grow = np.zeros_like(solid)
    grow[1:] |= solid[:-1]; grow[:-1] |= solid[1:]; grow[:, 1:] |= solid[:, :-1]; grow[:, :-1] |= solid[:, 1:]
    a[grow & ~solid] = (12, 4, 6, 255)
    return Image.fromarray(a, 'RGBA')

for row, (seed, tall) in zip((1, 2, 3), ((1, 9), (2, 13), (3, 16))):
    cell = (22 * 32, row * 32, 23 * 32, (row + 1) * 32)
    tex.paste(Image.new('RGBA', (32, 32), (0, 0, 0, 0)), cell[:2])
    tex.alpha_composite(frame(seed, tall), cell[:2])
tex.save(P)
print('fire tiles redrawn')
