"""Builds assets/graphics/spider.png (6 frames x 4 directions, 64px cells)
from the single spider drawing.  Each of the eight legs is cut out and swung
around the joint where it meets the body; the legs move in the two
alternating sets a real spider uses (left 1+3 with right 2+4, then the
others), with a little body sway.  Rows: down, left, right, up.
usage: python3 tools/make_spider.py spider_source.png assets/graphics/spider.png"""
import sys, math
from PIL import Image

src = Image.open(sys.argv[1]).convert('RGBA')
px = src.load()
for y in range(64):
    for x in range(64):
        if px[x, y][3] == 0: px[x, y] = (0, 0, 0, 0)

BODY_L, BODY_R = 24, 41                      # body columns [24, 41)
BANDS = [(8, 17, 16), (17, 26, 22), (26, 34, 29), (34, 64, 36)]   # rows of each leg pair + joint row

def cut(x0, x1, y0, y1):
    p = Image.new('RGBA', (64, 64))
    p.paste(src.crop((x0, y0, x1, y1)), (x0, y0))
    return p

body = cut(BODY_L, BODY_R, 0, 64)

# Split the leg pixels into the 8 legs: seed each leg where it meets the body,
# then grow all seeds at once along the leg pixels (multi-source BFS), so every
# pixel - tips included - goes to the leg it is actually connected through.
from collections import deque
def is_leg(x, y): return 0 <= x < 64 and 0 <= y < 64 and px[x, y][3] > 0 and not (BODY_L <= x < BODY_R)
owner = {}
q = deque()
seeds = []                                   # (side, index, joint)
for i, (y0, y1, jy) in enumerate(BANDS):
    for side, xs, jx in (('L', range(BODY_L - 4, BODY_L), BODY_L), ('R', range(BODY_R, BODY_R + 4), BODY_R - 1)):
        k = len(seeds); seeds.append((side, i, (jx, jy)))
        for y in range(y0, min(y1, 41) if i == 3 else y1):
            for x in xs:
                if is_leg(x, y) and (x, y) not in owner: owner[(x, y)] = k; q.append((x, y))
while q:
    x, y = q.popleft()
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            n = (x + dx, y + dy)
            if n not in owner and is_leg(*n): owner[n] = owner[(x, y)]; q.append(n)
legs = []                                    # (image, joint, side, index)
for k, (side, i, joint) in enumerate(seeds):
    img = Image.new('RGBA', (64, 64))
    ip = img.load()
    for (x, y), o in owner.items():
        if o == k: ip[x, y] = px[x, y]
    legs.append((img, joint, side, i))
stray = [(x, y) for y in range(64) for x in range(64) if is_leg(x, y) and (x, y) not in owner]
print('unassigned leg pixels:', len(stray))

FRAMES, SWING = 6, 16.0                      # degrees each leg sweeps either way
def frame(k):
    ph = 2 * math.pi * k / FRAMES
    f = Image.new('RGBA', (64, 64))
    for img, joint, side, i in legs:
        group_a = (side == 'L') == (i % 2 == 0)          # L1,L3,R2,R4 vs the rest
        a = SWING * math.sin(ph) * (1 if group_a else -1)
        if side == 'R': a = -a                           # mirror so both sides sweep the same way
        f.alpha_composite(img.rotate(a, resample=Image.NEAREST, center=joint))
    sway = round(math.sin(ph * 2))                       # body rocks a pixel as it walks
    f.alpha_composite(body, (sway, 0)) if sway >= 0 else f.alpha_composite(body.crop((-sway, 0, 64, 64)), (0, 0))
    return f

cycle = [frame(k) for k in range(FRAMES)]
sheet = Image.new('RGBA', (64 * FRAMES, 256))
turn = {0: 0, 1: -90, 2: 90, 3: 180}         # source faces down; PIL rotates counter-clockwise
for row in range(4):
    for col, f in enumerate(cycle):
        sheet.paste(f.rotate(turn[row]), (col * 64, row * 64))
sheet.save(sys.argv[2])
print('wrote', sys.argv[2], sheet.size)
