#!/usr/bin/env python3
"""Procedural blood-moon backdrop for the title / boot screens (1024x768).
Used only when assets/graphics/title_bg.jpg (your own picture) is absent."""
import os, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

W, H = 1024, 768
OUT = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics', 'title_bg_generated.png')
rng = random.Random(1953)
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)

# Sky: near-black with a deep red bloom around the moon.
mx, my, mr = 520.0, 520.0, 360.0
d = np.sqrt((xx - mx) ** 2 + (yy - my) ** 2)
img = np.zeros((H, W, 3), np.float32)
glow = np.exp(-np.maximum(d - mr, 0) / 160.0) * 0.35
img[..., 0] = 10 + 90 * glow
img[..., 1] = 1 + 4 * glow
img[..., 2] = 3 + 8 * glow

# Moon disc with mottled surface and a hot rim on the upper-left.
inside = d < mr
noise = Image.effect_noise((W // 4, H // 4), 60).resize((W, H), Image.BICUBIC)
n = (np.asarray(noise, np.float32) - 128) / 128.0
shade = 0.55 + 0.25 * ((mx - xx) + (my - yy)) / (2 * mr)
moon_r = (95 + 25 * n) * shade + 20
img[..., 0] = np.where(inside, moon_r, img[..., 0])
img[..., 1] = np.where(inside, 6 + 3 * n, img[..., 1])
img[..., 2] = np.where(inside, 8 + 3 * n, img[..., 2])
ang = np.arctan2(yy - my, xx - mx)
rimw = np.exp(-np.abs(d - mr) / 3.0) * np.clip(-np.sin(ang + 0.6), 0, 1)
img[..., 0] += 200 * rimw
img[..., 1] += 25 * rimw
img[..., 2] += 20 * rimw

horizon = 560
# Hot glow behind the peak.
pk = np.exp(-(((xx - 600) / 90.0) ** 2 + ((yy - horizon) / 50.0) ** 2))
img[..., 0] += 230 * pk
img[..., 1] += 10 * pk
# Water: darker reflection of everything above, rippled.
for y in range(horizon, H):
    src = 2 * horizon - y - 1
    k = (y - horizon) / (H - horizon)
    off = int(6 * math.sin(y * 0.35))
    row = np.roll(img[max(src, 0)], off, axis=0)
    img[y] = row * (0.35 * (1 - k)) + np.array([14, 0, 2]) * k
img = np.clip(img, 0, 255).astype(np.uint8)
im = Image.fromarray(img, 'RGB')
dr = ImageDraw.Draw(im)

# Mountain silhouette on the horizon.
pts = [(80, horizon)]
x = 80
while x < 980:
    x += rng.randint(8, 22)
    peak = 80 * math.exp(-((x - 600) / 70.0) ** 2) + 30 * math.exp(-((x - 430) / 90.0) ** 2) + 18 * math.exp(-((x - 820) / 60.0) ** 2)
    pts.append((x, horizon - 4 - peak - rng.randint(0, 9)))
pts.append((980, horizon))
dr.polygon(pts, fill=(4, 0, 1))

# Bare branches reaching in from the right edge.
def branch(x, y, a, length, width, depth):
    if depth == 0 or length < 6:
        return
    x2 = x + math.cos(a) * length
    y2 = y + math.sin(a) * length
    dr.line([(x, y), (x2, y2)], fill=(3, 0, 0), width=max(1, int(width)))
    for _ in range(rng.randint(1, 3)):
        branch(x2, y2, a + rng.uniform(-0.75, 0.75), length * rng.uniform(0.6, 0.8), width * 0.68, depth - 1)
dr.polygon([(1000, 0), (W, 0), (W, H), (985, H), (995, 420)], fill=(3, 0, 0))
for y0, a in [(90, math.pi * 1.08), (330, math.pi * 0.97), (520, math.pi * 1.12), (690, math.pi * 1.0)]:
    branch(1000, y0, a, 140, 11, 6)
im = im.filter(ImageFilter.GaussianBlur(0.6))

# Vignette.
v = np.clip(1.15 - 0.9 * np.sqrt(((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2) * 1.6, 0.25, 1)
im = Image.fromarray((np.asarray(im, np.float32) * v[..., None]).astype(np.uint8))
im.save(OUT)
print('wrote', OUT)
