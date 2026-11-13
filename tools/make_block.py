"""Builds assets/graphics/player_block.png: the hero raising the Blessed Sword
to BLOCK.  3 frames (raise, guard, CLASH) x 4 directions (rows: down, left,
right, up), 72x72 cells with the hero at (24, 12) - the same layout as
player_sword.png, so the game draws it the same way.  The guard holds the
blade across the body, facing the attack; the clash frame leans the hero back
a pixel and throws a spray of white-gold sparks off the blade where the
scythe hits it.
usage: python3 tools/make_block.py raw-assets/player_walk_only.png assets/graphics/player_block.png"""
import sys, math
from PIL import Image

src = Image.open(sys.argv[1]).convert('RGBA')
FW, FH, CELL, HX, HY = 24, 48, 72, 24, 12
HAND = {0: (17, 30), 1: (6, 29), 2: (17, 29), 3: (7, 27)}
# Blade angle (degrees, y down) per direction for: raise, guard, clash.
POSE = {0: (-115, -158, -162),     # facing you: blade swept up across the chest
        1: (-135, -84, -80),       # facing left: blade upright in front
        2: (-45, -96, -100),       # facing right: blade upright in front
        3: (-65, -22, -18)}        # facing away: blade across, behind him
BACK = {0: (0, -1), 1: (1, 0), 2: (-1, 0), 3: (0, 1)}   # recoil on the clash
L = 22

STEEL_HI, STEEL, STEEL_DK = (226, 196, 196, 255), (178, 132, 132, 255), (98, 58, 64, 255)
GOLD, GOLD_DK, GRIP = (214, 170, 80, 255), (140, 100, 40, 255), (96, 58, 32, 255)


def put(img, x, y, c):
    x, y = int(round(x)), int(round(y))
    if 0 <= x < CELL and 0 <= y < CELL:
        img.putpixel((x, y), c)


def sword(img, hx, hy, ang):
    a = math.radians(ang)
    dx, dy = math.cos(a), math.sin(a)
    px, py = -dy, dx
    for i in range(0, L * 2 + 1):
        t = i / 2
        x, y = hx + dx * t, hy + dy * t
        if t < 3.5:
            put(img, x, y, GOLD if t < 1 else GRIP)
        elif t < 5:
            for s in (-3, -2, -1, 0, 1, 2, 3):
                put(img, x + px * s, y + py * s, GOLD if abs(s) < 3 else GOLD_DK)
        else:
            w = 1 if t > L - 3 else 1.5
            put(img, x + px * w, y + py * w, STEEL_DK)
            put(img, x - px * w, y - py * w, STEEL)
            put(img, x, y, STEEL_HI)


def sparks(img, cx, cy):
    """A starburst of sparks: a white core, gold rays, a few loose embers."""
    for k in range(10):
        a = k * math.pi / 5 + 0.3
        n = 5 if k % 2 == 0 else 3
        for r in range(1, n + 1):
            col = (255, 255, 240, 255) if r <= 1 else (255, 220, 120, 255 - r * 30) if r < 4 else (255, 150, 60, 150)
            put(img, cx + math.cos(a) * r, cy + math.sin(a) * r, col)
    for (ex, ey) in ((6, -4), (-5, 5), (7, 3), (-6, -5)):
        put(img, cx + ex, cy + ey, (255, 200, 90, 200))
    put(img, cx, cy, (255, 255, 255, 255))


sheet = Image.new('RGBA', (CELL * 4, CELL * 4))
for row in range(4):
    hero = src.crop((0, row * FH, FW, row * FH + FH))
    for k in range(3):
        cell = Image.new('RGBA', (CELL, CELL))
        bx, by = BACK[row] if k == 2 else (0, 0)
        ox, oy = HX + bx, HY + by
        hx, hy = ox + HAND[row][0], oy + HAND[row][1]
        ang = POSE[row][k]
        blade = Image.new('RGBA', (CELL, CELL))
        sword(blade, hx, hy, ang)
        if k == 2:                                     # where the scythe strikes the blade
            a = math.radians(ang)
            sparks(blade, hx + math.cos(a) * 13, hy + math.sin(a) * 13)
        if row == 3:
            cell.alpha_composite(blade); cell.alpha_composite(hero, (ox, oy))
        else:
            cell.alpha_composite(hero, (ox, oy)); cell.alpha_composite(blade)
        sheet.paste(cell, (k * CELL, row * CELL))
sheet.save(sys.argv[2])
print('wrote', sys.argv[2], sheet.size)
