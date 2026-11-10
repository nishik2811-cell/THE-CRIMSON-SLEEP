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

    def render(self, outline=True, outline_color=None):
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
            out[edge] = (*(outline_color or OUTLINE), 255)
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


# ------------------------------------------------- Patrolling sheet ghosts
# Classic sheet ghost: pale blue-white cloth, hollow black eyes and mouth,
# one arm raised, a ragged tail that drips loose pixels.  24x48 frames,
# 4 walk frames x 4 directions (down, left, right, up) like the originals.
RAMPS['ghost'] = [(40, 42, 86), (84, 88, 146), (138, 146, 196), (188, 198, 230), (226, 232, 248), (246, 248, 255)]
RAMPS['ghostg'] = [(44, 40, 60), (88, 84, 112), (140, 136, 164), (186, 184, 206), (222, 222, 236), (244, 244, 250)]
GHOST_OUTLINE = (12, 10, 28)

def sheet_ghost(f, direction, mat='ghost', seed=0):
    c = Canvas(24, 48)
    rng = np.random.default_rng(seed * 100 + f * 7 + direction)
    bob = (0, -1, -1, 0)[f]
    face_dx = {0: 0, 1: -2, 2: 2, 3: 0}[direction]
    cx = 12 + {0: 0, 1: 1, 2: -1, 3: 0}[direction] * 0.5
    top = 9 + bob
    X, Y = c.xx, c.yy
    # head dome + body that widens and leans with the motion
    k = np.clip((Y - (top + 6)) / 30, 0, 1)
    lean = {0: 0, 1: 2.5, 2: -2.5, 3: 0}[direction] * k
    half = 5.6 + 3.2 * k
    head = ellipse(c, cx, top + 6, 6, 6.5)
    body = (np.abs(X - cx - lean) <= half) & (Y >= top + 6) & (Y <= 42 + bob)
    # ragged tail: each column ends at a different height, swaying per frame
    tail_end = 40 + bob + 3 * np.sin(X * 1.3 + f * 1.6) + rng.integers(0, 3, 24)[None, :]
    body &= Y <= tail_end
    cloth = head | body
    # light from the upper left; the cloth folds run diagonally
    shade = 0.78 - 0.32 * (X - cx - lean) / 8 - 0.25 * k + 0.12 * np.sin((X - cx) * 1.1 - Y * 0.35 + f)
    c.paint(cloth, mat, shade)
    c.paint(cloth & (Y > tail_end - 3), mat, shade * 0.55)          # darker, wet hem
    # loose drips under the tail
    for _ in range(5):
        dx = int(rng.integers(4, 20)); dy = int(42 + bob + rng.integers(1, 5))
        if dy < 48:
            c.paint((np.abs(X - dx - 0.5) < 0.5) & (np.abs(Y - dy - 0.5) < 0.5), mat, 0.15)
    # one raised arm (on the side it moves toward; both drift when facing you)
    arms = {0: (-1,), 1: (-1,), 2: (1,), 3: (1,)}[direction]
    for side in arms:
        ax = cx + side * 6
        arm = line(c, ax, top + 14, ax + side * 4, top + 9 + (f % 2), 1.3) | ellipse(c, ax + side * 4.3, top + 8.5 + (f % 2), 1.8, 2.2)
        c.paint(arm, mat, 0.55 + 0.2 * (side < 0))
    # face (not visible from behind)
    if direction != 3:
        for ex in (-2.3, 2.3):
            if abs(ex + face_dx) > 4.5:
                continue
            c.paint(ellipse(c, cx + ex + face_dx, top + 5.5, 1.2, 1.7), 'void', 0)
        c.paint(ellipse(c, cx + face_dx * 0.8, top + 9.3, 0.9, 1.3 + 0.3 * (f % 2)), 'void', 0)
    return c.render(outline_color=GHOST_OUTLINE)

# Bigger 32x48 sheet ghost after the reference: head dome, huge oval eyes,
# small open mouth, one arm flung out, a tail sweeping diagonally away and
# black drips splattering off the hem.
RAMPS['ghost2'] = [(30, 28, 70), (62, 62, 120), (112, 118, 178), (166, 176, 220), (214, 222, 244), (240, 244, 255)]

def sheet_ghost2(f, direction, seed=0):
    W = 32
    c = Canvas(W, 48)
    X, Y = c.xx, c.yy
    rng = np.random.default_rng(seed * 97 + f * 13 + direction)
    mirror = direction == 2                      # right = mirror of left
    d = 1 if direction in (0, 1, 3) else 1
    bob = (0, -1, -1, 0)[f]
    sway = (0, 1, 2, 1)[f]
    cx = 14.5 if direction in (0, 3) else 17
    top = 6 + bob
    # body: dome head flowing into a cloth that leans and sweeps to the right
    k = np.clip((Y - top - 6) / 32, 0, 1)
    sweep = (5 + sway) * k ** 1.6                # tail drifts sideways as it falls
    left_edge = cx - 6 - 2.5 * k + sweep * 0.3
    right_edge = cx + 6 + 3.5 * k + sweep
    head = ellipse(c, cx, top + 6, 6.2, 6.6)
    body = (X >= left_edge) & (X <= right_edge) & (Y >= top + 6)
    # ragged hem that rises toward the left (diagonal sweep)
    hem = 38 + bob + (X - cx) * 0.35 + 2.5 * np.sin(X * 1.4 + f * 1.3)
    body &= Y <= hem
    cloth = head | body
    # light from the upper right, deep indigo on the left side and in folds
    rel = (X - left_edge) / np.maximum(right_edge - left_edge, 1)
    folds = 0.16 * np.sin((X - cx) * 0.9 - (Y - top) * 0.45 + f * 0.7)
    shade = 0.25 + 0.75 * np.clip(rel * 1.25, 0, 1) - 0.18 * k + folds
    shade = np.where(head, sphere_light(c, cx, top + 6, 6.2, 6.6, lx=0.5, ly=-0.6) + 0.15, shade)
    c.paint(cloth, 'ghost2', shade)
    c.paint(cloth & (Y > hem - 3), 'ghost2', shade * 0.45)
    # dark vertical streaks (tattered strands) on the right
    streak = cloth & (Y > top + 16) & (np.sin(X * 2.3 + 1) > 0.75) & (X > cx + 2)
    c.paint(streak, 'ghost2', 0.12)
    # the arm, flung out to the left and slightly up, dark underside
    ax0, ay0 = cx - 5, top + 15
    ax1, ay1 = cx - 11 - (f % 2) * 0.5, top + 10 + (f % 2)
    arm = line(c, ax0, ay0, ax1, ay1, 2.0) | ellipse(c, ax1 - 0.5, ay1 - 1.5, 2.3, 3.0)
    c.paint(arm, 'ghost2', 0.55 - 0.35 * ((Y - ay1) > 0))
    # face (front / side only)
    if direction != 3:
        fx = cx + (0 if direction == 0 else 1.5)
        for ex in (-2.4, 2.4):
            c.paint(ellipse(c, fx + ex, top + 5, 1.6, 2.3), 'void', 0)
        c.paint(ellipse(c, fx + 0.2, top + 9.6, 1.1, 1.5 + 0.3 * (f % 2)), 'void', 0)
    img = c.render(outline_color=(6, 4, 14))
    a = np.asarray(img).copy()
    # black drips and splatter under the hem / off the arm
    for _ in range(9):
        x = int(np.clip(rng.normal(cx - 3, 5), 1, W - 2))
        y0 = int(np.clip(38 + bob + rng.integers(-2, 3), 0, 46))
        ln = int(rng.integers(1, 6))
        for y in range(y0, min(47, y0 + ln)):
            if a[y, x, 3] == 0: a[y, x] = (8, 6, 18, 255)
    for _ in range(4):
        x, y = int(rng.integers(1, 12)), int(rng.integers(top + 14, 44))
        if a[y, x, 3] == 0: a[y, x] = (8, 6, 18, 255)
    img = Image.fromarray(a, 'RGBA')
    return img.transpose(Image.FLIP_LEFT_RIGHT) if mirror else img

def ghost_sheet(mat, seed):
    sh = Image.new('RGBA', (128, 192), (0, 0, 0, 0))
    for d in range(4):
        for f in range(4):
            g = sheet_ghost2(f, d, seed)
            if mat == 'ghostg':          # second ghost: greyer, more faded
                arr = np.asarray(g).astype(np.float32)
                l = arr[..., :3].mean(-1, keepdims=True)
                arr[..., :3] = arr[..., :3] * 0.55 + l * 0.45
                arr[..., 3] *= np.where(arr[..., 3] > 0, 0.92, 1)
                g = Image.fromarray(arr.astype(np.uint8), 'RGBA')
            sh.alpha_composite(g, (32 * f, 48 * d))
    return sh

# ------------------------------------------- Flat "8-bit poster" style
# Tall, thin silhouettes; flat colour blocks (1-2 tones per material, no
# dithering, no outline); dot faces; stick legs; ghosts dissolve into loose
# pixels.  All four characters are original to The Crimson Sleep.
def flat_img(spec):
    """spec: list of (mask, rgb) painted in order on a 24x48 canvas."""
    a = np.zeros((48, 24, 4), np.uint8)
    for mask, rgb in spec:
        a[mask] = (*rgb, 255)
    return Image.fromarray(a, 'RGBA')

def grid():
    yy, xx = np.mgrid[0:48, 0:24]
    return xx, yy

def rect(xx, yy, x0, y0, x1, y1):
    return (xx >= x0) & (xx <= x1) & (yy >= y0) & (yy <= y1)

def bride_flat(f):
    xx, yy = grid()
    rng = np.random.default_rng(40 + f)
    sw = (0, 1, 0, -1)[f]
    DRESS, DRESS2, HAIR, SKIN, RED = (196, 214, 222), (150, 176, 190), (14, 16, 20), (206, 214, 214), (220, 20, 36)
    spec = []
    # dress: narrow at the shoulders, flaring, dissolving at the hem
    dress = np.zeros((48, 24), bool)
    for y in range(14, 44):
        half = 2 + (y - 14) * 0.17
        cx = 12 + sw * (y - 14) / 30
        dress[y, int(round(cx - half)):int(round(cx + half)) + 1] = True
    spec.append((dress, DRESS))
    fold = dress & (xx == np.round(12 + sw * (yy - 14) / 30).astype(int) + 1) & (yy > 18)
    spec.append((fold, DRESS2))
    # dissolve: random pixels drop out near the hem and scatter below
    hole = dress & (yy > 36) & (rng.random((48, 24)) < (yy - 36) / 10)
    spec.append((hole, (0, 0, 0)))
    scatter = (yy > 40) & (yy < 47) & (np.abs(xx - 12) < 8) & (rng.random((48, 24)) < 0.12)
    spec.append((scatter, DRESS2))
    # thin arms hanging straight down
    spec.append((rect(xx, yy, 8, 15, 8, 28), SKIN))
    spec.append((rect(xx, yy, 16, 15, 16, 28 - (f % 2)), SKIN))
    # long black hair: covers the face and falls to the waist
    spec.append((rect(xx, yy, 9, 4, 15, 26), HAIR))
    spec.append((rect(xx, yy, 10, 3, 14, 3), HAIR))
    spec.append((rect(xx, yy, 11, 9, 13, 13), SKIN))          # sliver of face
    spec.append(((xx == 11) & (yy == 10), RED))
    spec.append(((xx == 13) & (yy == 10), RED))
    a = np.asarray(flat_img(spec)).copy()
    a[(a[..., 0] == 0) & (a[..., 1] == 0) & (a[..., 2] == 0)] = (0, 0, 0, 0)
    return Image.fromarray(a, 'RGBA')

def child_flat(f):
    xx, yy = grid()
    st = (0, 1, 0, -1)[f]
    SKIN, SKIN2, HAIR, DRESS, EYE = (176, 180, 170), (130, 136, 126), (26, 20, 18), (120, 40, 46), (230, 20, 34)
    spec = [
        (rect(xx, yy, 7, 35, 16, 39), DRESS),                   # torn little dress, on all fours
        (rect(xx, yy, 6, 40, 6, 46) | rect(xx, yy, 5 - st, 45, 6, 46), SKIN2),
        (rect(xx, yy, 17, 40, 17, 46) | rect(xx, yy, 17, 45, 18 + st, 46), SKIN2),
        (rect(xx, yy, 9 + st, 40, 9 + st, 46), SKIN),
        (rect(xx, yy, 14 - st, 40, 14 - st, 46), SKIN),
        (rect(xx, yy, 8, 27, 15, 34), SKIN),                    # head, twisted up at you
        (rect(xx, yy, 8, 26, 15, 28) | rect(xx, yy, 7, 27, 7, 33), HAIR),
        (rect(xx, yy, 9, 30, 10, 31) | rect(xx, yy, 13, 30, 14, 31), (8, 4, 6)),
        (((xx == 10) | (xx == 13)) & (yy == 30), EYE),
        (rect(xx, yy, 11, 33, 12, 33 + (f % 2)), (8, 4, 6)),
    ]
    return flat_img(spec)

def hanged_flat():
    xx, yy = grid()
    spec = [
        (rect(xx, yy, 12, 0, 12, 7), (120, 92, 56)),            # rope
        (rect(xx, yy, 10, 8, 14, 13), (150, 158, 150)),         # grey head, tilted
        (rect(xx, yy, 10, 8, 14, 9), (40, 30, 26)),
        (((xx == 11) | (xx == 13)) & (yy == 11), (10, 6, 8)),
        (rect(xx, yy, 9, 14, 15, 32), (44, 52, 70)),            # long coat
        (rect(xx, yy, 12, 14, 12, 22), (120, 20, 30)),          # red tie
        (rect(xx, yy, 8, 15, 8, 29) | rect(xx, yy, 16, 15, 16, 29), (150, 158, 150)),
        (rect(xx, yy, 10, 33, 10, 44) | rect(xx, yy, 14, 33, 14, 43), (30, 30, 36)),
    ]
    return flat_img(spec)

def faceless_flat():
    xx, yy = grid()
    spec = [
        (rect(xx, yy, 10, 2, 14, 9), (222, 222, 216)),          # smooth blank face
        (rect(xx, yy, 10, 2, 14, 3), (24, 22, 24)),
        (rect(xx, yy, 9, 10, 15, 36), (16, 14, 18)),            # tall black suit
        (rect(xx, yy, 12, 10, 12, 18), (176, 22, 40)),
        (rect(xx, yy, 7, 11, 7, 34) | rect(xx, yy, 17, 11, 17, 34), (16, 14, 18)),   # too-long arms
        (rect(xx, yy, 7, 35, 7, 37) | rect(xx, yy, 17, 35, 17, 37), (222, 222, 216)),
        (rect(xx, yy, 10, 37, 10, 46) | rect(xx, yy, 14, 37, 14, 46), (16, 14, 18)),
    ]
    return flat_img(spec)

# ------------------------------ Poster style (matched to the team's sheet)
# Measured from the reference sheet: ~40px tall figures, 5-6px heads,
# 6-7px torsos, 1px arms, long 1px legs, flat colour with 2-3 tones per
# material (lighter column on the left), no outlines, dissolving ghost hems.
class Px:
    def __init__(self):
        self.a = np.zeros((48, 24, 4), np.uint8)
    def put(self, x, y, c):
        if 0 <= x < 24 and 0 <= y < 48: self.a[y, x] = (*c, 255)
    def box(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): self.put(x, y, c)
    def shaded(self, x0, y0, x1, y1, light, mid, dark):
        """Flat block, light left column, dark right column."""
        self.box(x0, y0, x1, y1, mid)
        self.box(x0, y0, x0, y1, light)
        self.box(x1, y0, x1, y1, dark)
    def clear(self, x, y):
        if 0 <= x < 24 and 0 <= y < 48: self.a[y, x] = 0
    def img(self): return Image.fromarray(self.a, 'RGBA')

def bride_poster(f):
    rng = np.random.default_rng(70 + f)
    g = Px()
    IV, IV2, IV3 = (226, 236, 232), (186, 212, 210), (128, 170, 172)      # pale ghost ivory
    HAIR, HAIR2 = (16, 18, 22), (40, 48, 54)
    SKIN, RED = (196, 210, 208), (214, 24, 40)
    sw = (0, 1, 0, -1)[f]
    # gown: 7 wide at the chest flaring to 11 at the hem, leaning with the sway
    for y in range(17, 46):
        k = (y - 17) / 28
        half = 3 + int(round(2.4 * k))
        cx = 12 + int(round(sw * k))
        for x in range(cx - half, cx + half + 1):
            c = IV if x <= cx - half + 1 else IV3 if x >= cx + half else IV2
            g.put(x, y, c)
    # dissolving hem: holes and loose specks, as on the sheet's ghost figure
    for y in range(38, 46):
        for x in range(5, 19):
            if g.a[y, x, 3] and rng.random() < (y - 37) / 12: g.clear(x, y)
    for _ in range(10):
        g.put(int(rng.integers(5, 19)), int(rng.integers(42, 48)), IV3 if rng.random() < .6 else IV2)
    # arms: 1px, hanging, pale hands
    for y in range(18, 29): g.put(8, y, SKIN); g.put(16, y, SKIN)
    g.put(8, 29, IV2); g.put(16, 29 - (f % 2), IV2)
    # head + long black hair over the face, falling to the waist
    g.box(10, 7, 14, 12, SKIN)
    g.box(9, 6, 15, 7, HAIR); g.box(9, 8, 9, 27, HAIR); g.box(15, 8, 15, 27, HAIR)
    g.box(10, 8, 10, 25, HAIR); g.box(14, 8, 14, 25, HAIR)
    g.box(11, 13, 13, 22, HAIR); g.put(12, 23, HAIR)
    g.box(9, 6, 9, 20, HAIR2)                                           # light edge on the hair
    g.put(11, 10, RED); g.put(13, 10, RED)                              # eyes through the gap
    # torn veil
    for x in range(9, 16): g.put(x, 5, IV2 if x % 2 else IV3)
    return g.img()

def child_poster(f):
    g = Px()
    SKIN, SKIN2, SKIN3 = (186, 188, 176), (150, 154, 142), (110, 114, 104)
    GOWN, GOWN2 = (150, 40, 48), (110, 26, 34)
    HAIR, EYE = (30, 22, 20), (226, 22, 36)
    st = (0, 1, 0, -1)[f]
    # on all fours: little nightgown body, stick limbs
    g.shaded(7, 35, 16, 39, (176, 54, 62), GOWN, GOWN2)
    for x, dx in ((7, -1), (9, 0), (14, 0), (16, 1)):
        for y in range(40, 46): g.put(x + (st if dx == 0 else 0), y, SKIN2)
        g.put(x + dx + (st if dx == 0 else 0), 46, SKIN3)
    # head craned up toward you, too big for the body
    g.shaded(8, 27, 14, 33, SKIN, SKIN2, SKIN3)
    g.box(8, 26, 14, 27, HAIR); g.box(7, 27, 7, 33, HAIR); g.box(15, 27, 15, 31, HAIR)
    g.put(10, 30, EYE); g.put(13, 30, EYE)
    g.put(9, 30, (10, 6, 8)); g.put(12, 30, (10, 6, 8))
    g.box(11, 32, 12, 32 + (f % 2), (10, 6, 8))
    return g.img()

def faceless_poster():
    g = Px()
    SUIT, SUIT2, SUIT3 = (52, 52, 60), (26, 26, 32), (12, 12, 16)
    FACE, FACE2 = (226, 224, 216), (188, 186, 178)
    g.box(10, 5, 14, 11, FACE); g.box(14, 6, 14, 11, FACE2)               # smooth blank head
    g.box(10, 4, 14, 5, (30, 26, 26))
    g.put(12, 12, FACE2)
    g.shaded(9, 13, 15, 32, SUIT, SUIT2, SUIT3)                          # tall suit
    g.box(12, 13, 12, 20, (168, 24, 40))                                 # red tie
    g.box(11, 13, 11, 14, (220, 220, 214)); g.box(13, 13, 13, 14, (220, 220, 214))
    for y in range(14, 36): g.put(8, y, SUIT2); g.put(16, y, SUIT3)      # arms far too long
    g.put(8, 36, FACE2); g.put(16, 36, FACE2)
    for y in range(33, 46): g.put(10, y, SUIT3); g.put(14, y, SUIT3)     # stick legs
    g.put(9, 46, SUIT3); g.put(10, 46, SUIT3); g.put(14, 46, SUIT3); g.put(15, 46, SUIT3)
    return g.img()

def hanged_poster():
    g = Px()
    ROPE, ROPE2 = (150, 118, 72), (110, 84, 50)
    SKIN, SKIN2 = (150, 164, 160), (112, 126, 122)
    COAT, COAT2, COAT3 = (78, 70, 64), (56, 50, 46), (36, 32, 30)
    for y in range(0, 9): g.put(12, y, ROPE if y % 2 else ROPE2)
    g.box(10, 9, 14, 14, SKIN); g.box(14, 9, 14, 14, SKIN2)              # head, tilted
    g.box(10, 8, 14, 9, (40, 30, 26)); g.put(9, 10, (40, 30, 26))
    g.put(11, 11, (14, 10, 10)); g.put(13, 12, (14, 10, 10))
    g.box(11, 13, 13, 13, (70, 20, 26))                                  # tongue / noose bruise
    g.shaded(9, 15, 15, 32, COAT, COAT2, COAT3)
    for y in range(16, 30): g.put(8, y, SKIN2); g.put(16, y, SKIN2)
    for y in range(33, 44): g.put(11, y, COAT3); g.put(13, y, COAT3)     # dangling legs
    g.put(11, 44, (20, 18, 18)); g.put(13, 45, (20, 18, 18))             # feet pointing down
    return g.img()

def sheet(frames):
    sh = Image.new('RGBA', (24 * len(frames), 48), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        sh.alpha_composite(f, (24 * i, 0))
    return sh

if __name__ == '__main__':
    sheet([bride_poster(f) for f in range(4)]).save(os.path.join(G, 'wraith.png'))
    sheet([child_poster(f) for f in range(4)]).save(os.path.join(G, 'hollow_child.png'))
    sheet([hanged_poster(), faceless_poster()]).save(os.path.join(G, 'apparitions.png'))
    if not os.path.exists(os.path.join(G, 'jumpscare.png')):
        jumpscare().resize((1024, 768), Image.NEAREST).save(os.path.join(G, 'jumpscare.png'))
    ghost_sheet('ghost', 1).save(os.path.join(G, 'enemy_easy.png'))
    ghost_sheet('ghostg', 2).save(os.path.join(G, 'enemy_medium.png'))
    print('pixel sprites written')
