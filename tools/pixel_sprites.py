#!/usr/bin/env python3
"""Refined pixel-art sprites for the new horrors.

Every pixel is evaluated at the sprite's native resolution: shapes are
signed-distance tests, each material is lit with a simple normal/lambert
term, then mapped onto a short hand-picked colour ramp using a 4x4 Bayer
ordered dither (the classic way pixel artists blend ramps).  Finally a
1px selective outline is added.  No blur, no anti-aliasing, no smooth
gradients survive - so the result sits with the original RPG sprites."""
import os, math
import numpy as np
from PIL import Image

G = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics')
BAYER = (np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) + 0.5) / 16.0

RAMPS = {
    'gown':  [(34, 32, 44), (62, 60, 76), (98, 96, 112), (140, 138, 152), (182, 180, 192), (218, 216, 224), (240, 238, 242)],
    'hair':  [(6, 3, 6), (16, 8, 13), (30, 15, 24), (48, 26, 38)],
    'face':  [(70, 74, 80), (112, 116, 122), (156, 160, 164), (196, 198, 200), (226, 226, 226)],
    'skin':  [(44, 46, 46), (76, 80, 78), (112, 118, 112), (152, 156, 146), (190, 192, 180)],
    'blood': [(60, 0, 8), (104, 0, 14), (150, 8, 22)],
    'eye':   [(120, 0, 10), (210, 16, 30), (255, 90, 90)],
    'coat':  [(14, 10, 14), (28, 22, 28), (46, 38, 44), (66, 56, 62)],
    'rope':  [(52, 38, 24), (88, 66, 40), (124, 96, 60)],
    'void':  [(2, 0, 1), (10, 2, 4)],
    'teeth': [(110, 100, 84), (170, 160, 136), (210, 200, 176)],
}
OUTLINE = (14, 4, 8)

class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.mat = np.full((h, w), '', dtype=object)
        self.val = np.zeros((h, w), np.float32)
        self.alpha = np.zeros((h, w), np.uint8)
        self.yy, self.xx = np.mgrid[0:h, 0:w].astype(np.float32)
        self.xx += 0.5; self.yy += 0.5

    def paint(self, mask, mat, value, alpha=255):
        value = np.broadcast_to(np.asarray(value, np.float32), mask.shape)
        self.mat[mask] = mat
        self.val[mask] = np.clip(value[mask], 0, 1)
        self.alpha[mask] = alpha

    def render(self, outline=True):
        out = np.zeros((self.h, self.w, 4), np.uint8)
        for y in range(self.h):
            for x in range(self.w):
                m = self.mat[y, x]
                if not m or self.alpha[y, x] == 0:
                    continue
                ramp = RAMPS[m]
                v = self.val[y, x] * (len(ramp) - 1) + (BAYER[y % 4, x % 4] - 0.5)
                i = int(np.clip(round(v), 0, len(ramp) - 1))
                out[y, x] = (*ramp[i], self.alpha[y, x])
        if outline:
            solid = out[..., 3] > 0
            grown = np.zeros_like(solid)
            for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                sh = np.zeros_like(solid)
                ys = slice(max(dy, 0), self.h + min(dy, 0)); yd = slice(max(-dy, 0), self.h + min(-dy, 0))
                xs = slice(max(dx, 0), self.w + min(dx, 0)); xd = slice(max(-dx, 0), self.w + min(-dx, 0))
                sh[ys, xs] = solid[yd, xd]
                grown |= sh
            edge = grown & ~solid
            out[edge] = (*OUTLINE, 255)
        return Image.fromarray(out, 'RGBA')

def ellipse(c, cx, cy, rx, ry):
    return ((c.xx - cx) / rx) ** 2 + ((c.yy - cy) / ry) ** 2 <= 1

def sphere_light(c, cx, cy, rx, ry, lx=-0.5, ly=-0.6):
    nx = np.clip((c.xx - cx) / rx, -1, 1); ny = np.clip((c.yy - cy) / ry, -1, 1)
    nz = np.sqrt(np.clip(1 - nx * nx - ny * ny, 0, 1))
    return np.clip(0.35 + 0.65 * (nx * lx + ny * ly + nz * 0.6), 0, 1)

def line(c, x0, y0, x1, y1, w=0.6):
    px, py = c.xx - x0, c.yy - y0
    dx, dy = x1 - x0, y1 - y0
    t = np.clip((px * dx + py * dy) / (dx * dx + dy * dy + 1e-6), 0, 1)
    return (px - t * dx) ** 2 + (py - t * dy) ** 2 <= w * w

# --------------------------------------------------------------- Bride
def bride(f):
    c = Canvas(24, 48)
    sway = math.sin(f / 4 * 2 * math.pi)
    cx = 12 + 0.6 * sway
    # gown: a flaring cone with vertical folds, lit from the upper left
    top, bot = 16, 44
    k = np.clip((c.yy - top) / (bot - top), 0, 1)
    half = 3.2 + 6.8 * k ** 0.9
    gx = c.xx - (cx + sway * 1.6 * k)
    rng = np.random.default_rng(f)
    hem = bot + np.interp(c.xx[0], np.arange(24) + 0.5, rng.integers(-1, 3, 24).astype(float))[None, :]
    gown = (np.abs(gx) <= half) & (c.yy >= top) & (c.yy <= hem)
    n = gx / half
    shade = 0.62 - 0.38 * n + 0.14 * np.sin(gx * 1.7 + 1.2 + f * 0.8) - 0.25 * k ** 3
    c.paint(gown, 'gown', shade, 235)
    c.paint(gown & (c.yy > hem - 2), 'gown', shade * 0.6, 170)          # ghostly tattered hem
    # thin arms hanging, long bony fingers
    for side in (-1, 1):
        ax = cx + side * 5.2
        arm = line(c, ax, 18, ax + side * (1.2 + 0.6 * sway * side), 29, 0.55)
        c.paint(arm, 'face', 0.55 + 0.2 * side * -1)
        c.paint(line(c, ax + side * 1.3, 29, ax + side * 1.6, 32, 0.45), 'face', 0.4)
    # hair: a heavy curtain that hangs past the shoulders
    hair = (ellipse(c, cx, 9, 6.2, 7.0) | ((np.abs(c.xx - cx) <= 5.6 - 0.05 * (c.yy - 9)) & (c.yy >= 9) & (c.yy <= 26 + (c.xx.astype(int) % 3))))
    strands = 0.35 + 0.35 * np.sin(c.xx * 2.6 + c.yy * 0.25) + 0.3 * sphere_light(c, cx, 9, 6, 7)
    c.paint(hair, 'hair', strands * 0.8)
    # a sliver of face between the strands
    face = (np.abs(c.xx - cx) <= 2.2) & (c.yy >= 8) & (c.yy <= 15.5)
    c.paint(face, 'face', sphere_light(c, cx, 11, 2.6, 5) * 0.9)
    for ex in (cx - 1.1, cx + 1.1):
        c.paint((np.abs(c.xx - ex) < 0.55) & (np.abs(c.yy - 10.5) < 0.6), 'eye', 0.85)
        c.paint((np.abs(c.xx - ex) < 0.55) & (c.yy > 11) & (c.yy < 13.5 + (f + int(ex)) % 2), 'blood', 0.7)
    return c.render()

# ---------------------------------------------------------- Hollow Child
def child(f):
    c = Canvas(24, 48)
    st = (0, 1, 0, -1)[f]
    cy = 37
    body = ellipse(c, 12, cy, 6.5, 3.4)
    c.paint(body, 'skin', sphere_light(c, 12, cy, 6.5, 3.4) * 0.85)
    ribs = body & (np.sin(c.xx * 2.2) > 0.6) & (c.yy > cy - 1)
    c.paint(ribs, 'skin', 0.25)
    # four spindly limbs, bent at the elbow, stepping
    limbs = [((7, 38), (4 - st, 42), (5 - st, 46)), ((17, 38), (20 + st, 42), (19 + st, 46)),
             ((9, 39), (9 + st, 43), (10 + st, 46)), ((15, 39), (15 - st, 43), (14 - st, 46))]
    for (a, b, d) in limbs:
        c.paint(line(c, *a, *b, 0.6) | line(c, *b, *d, 0.55), 'skin', 0.45)
    # head twisted up toward the player, too big for the body
    hx, hy = 12 + 0.5 * st, 29
    head = ellipse(c, hx, hy, 4.6, 4.4)
    c.paint(head, 'skin', sphere_light(c, hx, hy, 4.6, 4.4))
    c.paint(ellipse(c, hx, hy - 3.2, 4.8, 2.2) & head, 'hair', 0.4)
    for ex in (hx - 1.8, hx + 1.8):
        c.paint(ellipse(c, ex, hy + 0.2, 1.2, 1.4), 'void', 0.2)
        c.paint((np.abs(c.xx - ex) < 0.5) & (np.abs(c.yy - hy - 0.2) < 0.5), 'eye', 0.9)
    c.paint(ellipse(c, hx, hy + 2.8, 1.5, 0.8 + 0.4 * (f % 2)), 'void', 0)
    return c.render()

# ------------------------------------------------------------ Apparitions
def hanged():
    c = Canvas(24, 48)
    c.paint(line(c, 12, 0, 12, 9, 0.5), 'rope', 0.6)
    head = ellipse(c, 12.6, 12, 3.2, 3.4)
    c.paint(head, 'face', sphere_light(c, 12.6, 12, 3.2, 3.4) * 0.7)
    c.paint(ellipse(c, 12.6, 9.6, 3.3, 1.3) & head, 'hair', 0.5)
    for ex in (11.4, 14):
        c.paint(ellipse(c, ex, 12.3, 0.7, 0.6), 'void', 0)
    coat = (np.abs(c.xx - 12) <= 4.2 - 0.04 * (c.yy - 16)) & (c.yy >= 15.5) & (c.yy <= 33)
    c.paint(coat, 'coat', 0.6 - 0.2 * (c.xx - 12) / 4)
    for lx in (10.4, 13.6):
        c.paint((np.abs(c.xx - lx) <= 0.9) & (c.yy > 33) & (c.yy < 44 - (lx > 12)), 'coat', 0.35)
    for side in (-1, 1):
        c.paint(line(c, 12 + side * 4, 17, 12 + side * 4.6, 31, 0.6), 'face', 0.35)
    return c.render()

def faceless():
    c = Canvas(24, 48)
    head = ellipse(c, 12, 8.5, 3.6, 4.6)
    c.paint(head, 'face', sphere_light(c, 12, 8.5, 3.6, 4.6))
    c.paint(ellipse(c, 12, 4.8, 3.6, 1.5) & head, 'hair', 0.5)
    suit = (np.abs(c.xx - 12) <= 5.0 - 0.02 * (c.yy - 14)) & (c.yy >= 13.5) & (c.yy <= 40)
    c.paint(suit, 'coat', 0.5 - 0.3 * (c.xx - 12) / 5)
    c.paint((np.abs(c.xx - 12) <= 0.8) & (c.yy > 14) & (c.yy < 23), 'blood', 0.8)
    for side in (-1, 1):
        c.paint(line(c, 12 + side * 5, 15, 12 + side * 7.5, 36, 0.8), 'coat', 0.3)
        c.paint(line(c, 12 + side * 7.5, 36, 12 + side * 7.8, 39, 0.5), 'face', 0.4)
        c.paint((np.abs(c.xx - (12 + side * 2.2)) <= 1.1) & (c.yy > 40) & (c.yy < 46), 'coat', 0.2)
    return c.render()

# ------------------------------------------------------- Jump-scare face
# Chunky close-up (128x96 pixels -> 1024x768, every pixel an 8x8 block):
# wrinkled brow, huge hollow ringed eyes, a ridged nose, boil-covered
# cheeks and floating teeth over a grinning chin - five crimson tones,
# heavily dithered so it reads like an ink print.
RAMPS['crimson'] = [(0, 0, 0), (52, 0, 12), (112, 6, 26), (176, 22, 48), (232, 96, 112), (255, 206, 206)]

def jumpscare():
    W, H = 128, 96
    c = Canvas(W, H)
    rng = np.random.default_rng(1953)
    cx = 64
    grain = rng.normal(0, 0.09, (H, W)).astype(np.float32)
    X, Y = c.xx, c.yy

    # head mass: wide skull shape, darker toward the edges
    head = ellipse(c, cx, 40, 46, 38) & (Y < 70)
    edge = np.sqrt(((X - cx) / 46) ** 2 + ((Y - 40) / 38) ** 2)
    base = 0.62 - 0.55 * edge ** 2
    # forehead: bright centre, deep horizontal wrinkles bending down at the sides
    bend = 0.012 * (X - cx) ** 2
    wr = np.sin((Y - bend) * 1.15) * np.exp(-((Y - 14) / 10) ** 2)
    base += 0.38 * wr + 0.35 * np.exp(-((X - cx) / 14) ** 2 - ((Y - 10) / 8) ** 2)
    # vertical brow ridge / nose bridge
    base += 0.3 * np.exp(-((X - cx) / 3.0) ** 2) * (Y > 14) * (Y < 50)
    base += 0.25 * np.sin(Y * 1.6) * np.exp(-((X - cx) / 3.5) ** 2) * (Y > 20) * (Y < 50)
    c.paint(head, 'crimson', base + grain)

    # eyes: thick swollen lids, rings of flesh, black holes
    for ex in (cx - 24, cx + 24):
        ey = 34
        d = np.sqrt((X - ex) ** 2 + ((Y - ey) * 1.05) ** 2)
        lids = (d < 17)
        ring = 0.6 + 0.38 * np.cos((d - 11.5) * 0.95) - 0.3 * np.clip((d - 14.5) / 3, 0, 1)
        ring += 0.18 * ((X - ex) * -0.6 + (Y - ey) * -0.8) / 17       # lit from upper left
        c.paint(lids, 'crimson', ring + grain * 0.8)
        c.paint(d < 12.5, 'crimson', 0.25 + 0.1 * np.cos(d * 1.4) + grain)
        c.paint(d < 9.5, 'crimson', 0.0)
        # bags under the eyes
        bag = ellipse(c, ex, ey + 16, 14, 4) & ~(d < 17)
        c.paint(bag, 'crimson', 0.22 + grain)

    # nose: bulbous tip and black nostrils
    tip = ellipse(c, cx, 55, 7, 5)
    c.paint(tip, 'crimson', sphere_light(c, cx, 55, 7, 5, lx=-0.2, ly=-0.9) * 0.95 + grain)
    for nx in (cx - 6, cx + 6):
        c.paint(ellipse(c, nx, 59, 3.4, 2.2), 'crimson', 0.0)

    # cheeks crowded with boils, each lit with a bright speck
    for _ in range(34):
        side = rng.choice([-1, 1])
        bx = cx + side * rng.uniform(12, 44)
        by = rng.uniform(46, 66) - 0.004 * (bx - cx) ** 2
        r = rng.uniform(2.2, 5.2)
        boil = ellipse(c, bx, by, r, r * 0.85) & ellipse(c, cx, 42, 48, 30)
        c.paint(boil, 'crimson', sphere_light(c, bx, by, r, r * 0.85) * 1.05 + grain * 0.5)
    c.paint(ellipse(c, cx, 64, 40, 5) & head, 'crimson', 0.12 + grain)  # shadow under the cheeks

    # mouth: pure black void with two rows of floating teeth
    mouth = ellipse(c, cx, 76, 30, 10)
    c.paint(mouth & (Y > 66), 'crimson', 0.0)
    for row, ty in enumerate((71, 78)):
        n = 7 if row == 0 else 6
        for k in range(n):
            tx = cx + (k - (n - 1) / 2) * 5.0
            hgt = 3.4 if abs(k - (n - 1) / 2) < 2 else 2.4
            tooth = (np.abs(X - tx) < 1.9) & (np.abs(Y - ty) < hgt)
            c.paint(tooth, 'crimson', 0.35 + 0.35 * ((tx - cx) * -0.03 + 0.6) + 0.25 * (Y < ty - 1) + grain * 0.5)

    # grinning chin: a crescent curling up at both corners
    chin_out = ellipse(c, cx, 78, 34, 16) & (Y > 80)
    chin_in = ellipse(c, cx, 74, 28, 12)
    crescent = chin_out & ~chin_in
    curl = (np.abs(X - cx) > 22) & (np.abs(X - cx) < 34) & (Y > 64) & (Y < 82) & ~mouth & (np.abs(Y - (82 - 0.9 * (np.abs(X - cx) - 22))) < 3)
    c.paint(crescent | curl, 'crimson', 0.42 - 0.3 * np.abs(X - cx) / 34 + grain)
    lip = ellipse(c, cx, 88, 9, 3.5)
    c.paint(lip, 'crimson', sphere_light(c, cx, 88, 9, 3.5, lx=0, ly=-1) * 0.9 + grain * 0.5)

    # ragged edges: chew away the outline so it dissolves into the dark
    img = c.render(outline=False)
    a = np.asarray(img).copy()
    lum = a[..., :3].sum(-1)
    a[(lum < 40)] = (0, 0, 0, 255)
    a[..., 3] = 255
    return Image.fromarray(a, 'RGBA')

def sheet(frames):
    sh = Image.new('RGBA', (24 * len(frames), 48), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        sh.alpha_composite(f, (24 * i, 0))
    return sh

if __name__ == '__main__':
    sheet([bride(f) for f in range(4)]).save(os.path.join(G, 'wraith.png'))
    sheet([child(f) for f in range(4)]).save(os.path.join(G, 'hollow_child.png'))
    sheet([hanged(), faceless()]).save(os.path.join(G, 'apparitions.png'))
    jumpscare().resize((1024, 768), Image.NEAREST).save(os.path.join(G, 'jumpscare.png'))
    print('pixel sprites written')
