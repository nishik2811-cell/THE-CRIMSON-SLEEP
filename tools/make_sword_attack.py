"""Builds assets/graphics/player_sword.png: the hero swinging the Blessed
Sword.  4 frames (wind-up, swing, STRIKE, recover) x 4 directions (rows:
down, left, right, up), 72x72 cells with the hero at (24, 12) so the blade
can reach past his body.  The sword is drawn pixel by pixel in the colours of
the Blessed Sword pickup (pink steel, gold guard, brown grip), and the swing
and strike frames carry a crimson slash arc.
usage: python3 tools/make_sword_attack.py raw-assets/player_walk_only.png assets/graphics/player_sword.png"""
import sys, math
from PIL import Image

src = Image.open(sys.argv[1]).convert('RGBA')
FW, FH, CELL, HX, HY = 24, 48, 72, 24, 12
FACING = {0: 90, 1: 180, 2: 0, 3: 270}                 # degrees, y down: down, left, right, up
HAND = {0: (17, 30), 1: (6, 29), 2: (17, 29), 3: (7, 27)}
SWING = [-105, -45, 12, 50]                            # sword angle relative to facing, per frame
LEAN = [(-1, 0), (0, 0), (1, 0), (0, 0)]               # body leans back, then into the blow
L = 22                                                 # blade + grip length in px

STEEL_HI, STEEL, STEEL_DK = (226, 196, 196, 255), (178, 132, 132, 255), (98, 58, 64, 255)
GOLD, GOLD_DK, GRIP = (214, 170, 80, 255), (140, 100, 40, 255), (96, 58, 32, 255)

def put(img, x, y, c):
    x, y = int(round(x)), int(round(y))
    if 0 <= x < CELL and 0 <= y < CELL: img.putpixel((x, y), c)

def sword(img, hx, hy, ang):
    a = math.radians(ang)
    dx, dy = math.cos(a), math.sin(a)
    px, py = -dy, dx                                   # perpendicular
    for i in range(0, L * 2 + 1):
        t = i / 2
        x, y = hx + dx * t, hy + dy * t
        if t < 3.5:                                    # grip + pommel
            put(img, x, y, GOLD if t < 1 else GRIP)
        elif t < 5:                                    # cross-guard, wide
            for s in (-3, -2, -1, 0, 1, 2, 3):
                put(img, x + px * s, y + py * s, GOLD if abs(s) < 3 else GOLD_DK)
        else:                                          # blade: dark edge, steel, bright spine, tapering
            w = 1 if t > L - 3 else 1.5
            put(img, x + px * w, y + py * w, STEEL_DK)
            put(img, x - px * w, y - py * w, STEEL)
            put(img, x, y, STEEL_HI)

def slash(img, hx, hy, a0, a1, strong):
    steps = 28
    for k in range(steps + 1):
        a = math.radians(a0 + (a1 - a0) * k / steps)
        fade = k / steps                               # brightest at the leading edge
        for r, col in ((L + 4, (255, 235, 235)), (L + 3, (255, 120, 130)), (L + 2, (230, 40, 60)), (L + 1, (170, 10, 30))):
            alpha = int((235 if strong else 160) * fade)
            if alpha <= 0: continue
            x, y = hx + math.cos(a) * r, hy + math.sin(a) * r
            xi, yi = int(round(x)), int(round(y))
            if 0 <= xi < CELL and 0 <= yi < CELL:
                base = img.getpixel((xi, yi))
                if base[3] < alpha: img.putpixel((xi, yi), col + (alpha,))

sheet = Image.new('RGBA', (CELL * 4, CELL * 4))
for row in range(4):
    hero = src.crop((0, row * FH, FW, row * FH + FH))
    face = FACING[row]
    fx, fy = math.cos(math.radians(face)), math.sin(math.radians(face))
    for k in range(4):
        cell = Image.new('RGBA', (CELL, CELL))
        lean = LEAN[k][0]
        ox, oy = HX + round(fx * lean), HY + round(fy * lean)
        hx, hy = ox + HAND[row][0], oy + HAND[row][1]
        ang = face + SWING[k]
        blade = Image.new('RGBA', (CELL, CELL))
        sword(blade, hx, hy, ang)
        if k in (1, 2): slash(blade, hx, hy, face + SWING[k - 1], ang, k == 2)
        if row == 3:                                   # facing away: the sword is behind him
            cell.alpha_composite(blade); cell.alpha_composite(hero, (ox, oy))
        else:
            cell.alpha_composite(hero, (ox, oy)); cell.alpha_composite(blade)
        sheet.paste(cell, (k * CELL, row * CELL))
sheet.save(sys.argv[2])
print('wrote', sys.argv[2], sheet.size)
