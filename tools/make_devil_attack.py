#!/usr/bin/env python3
"""Build the Devil's scythe-swing sprite sheet from his walk sheet.

    python3 tools/make_devil_attack.py [assets/graphics/enemy_boss.png] [assets/graphics/enemy_boss_attack.png]

The walk sheet is 4 columns x 4 rows of 24x48 cells (rows: Down, Left,
Right, Up).  For each direction we:
  1. take the first walk frame and cut the scythe out of it - the orange
     shaft pixels, plus the pale blade found by a flood fill from its tip;
  2. patch the holes the shaft leaves in the robe from the pixels beside it;
  3. redraw a scythe of our own, rotated around his hands, in 4 poses:
     raise, wind-up, SWING (with a crimson slash arc) and follow-through.
The output is 4 columns (frames) x 4 rows (directions) of 72x72 cells; the
24x48 body sits at (24, 16) inside each cell so the blade can leave it.
"""
import math
import sys
from PIL import Image, ImageDraw

SRC = sys.argv[1] if len(sys.argv) > 1 else "assets/graphics/enemy_boss.png"
DST = sys.argv[2] if len(sys.argv) > 2 else "assets/graphics/enemy_boss_attack.png"
CW, CH = 24, 48          # walk cell
AW = AH = 72             # attack cell
OX, OY = 24, 16          # where the body sits in an attack cell

# Per direction: blade-tip seed, which side the blade is on, hand (grip)
# position, the four shaft angles (degrees, screen space, grip -> head),
# which way the blade points (+1 = clockwise of the shaft), and whether the
# scythe is drawn behind the body (facing away from us).
DIRS = [
    dict(name="down",  seed=(1, 17),  side=-1, grip=(11, 24), ang=(-45, -105, 55, 140),  blade=+1, behind=False),
    dict(name="left",  seed=(2, 19),  side=-1, grip=(8, 22),  ang=(-50, -100, -180, -230), blade=-1, behind=False),
    dict(name="right", seed=(21, 18), side=+1, grip=(14, 23), ang=(-130, -80, 0, 50),    blade=+1, behind=False),
    dict(name="up",    seed=(21, 15), side=+1, grip=(14, 17), ang=(120, 30, -90, -150),  blade=-1, behind=True),
]

SHAFT, SHAFT_DARK = (128, 84, 44, 255), (52, 32, 18, 255)
BLADE, BLADE_EDGE, BLADE_DARK = (176, 178, 190, 255), (240, 242, 250, 255), (34, 34, 44, 255)


def is_shaft(p):
    r, g, b, a = p
    return a > 128 and r > g + 15 and g >= b and r > 80


def is_pale(p):
    r, g, b, a = p
    return a > 128 and not is_shaft(p) and (r + g + b) / 3 > 70 and not b > r + 20


def strip_scythe(cell, d):
    w, h = cell.size
    px = cell.load()
    gone = set()
    for y in range(h):
        for x in range(w):
            if is_shaft(px[x, y]):
                gone.add((x, y))
    # The blade: everything pale connected to its tip, on the blade's side.
    lim = 7 if d["side"] < 0 else 17
    stack = [d["seed"]]
    while stack:
        x, y = stack.pop()
        if (x, y) in gone or not (0 <= x < w and 0 <= y < h):
            continue
        if (d["side"] < 0 and x > lim) or (d["side"] > 0 and x < lim):
            continue
        if not is_pale(px[x, y]):
            continue
        gone.add((x, y))
        stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    # Remove, then patch holes inside the robe from their left/right neighbours.
    out = cell.copy()
    op = out.load()
    for (x, y) in gone:
        op[x, y] = (0, 0, 0, 0)
    for (x, y) in sorted(gone, key=lambda q: q[1]):
        l = next((op[i, y] for i in range(x - 1, -1, -1) if (i, y) not in gone and op[i, y][3] > 128), None)
        r = next((op[i, y] for i in range(x + 1, w) if (i, y) not in gone and op[i, y][3] > 128), None)
        if l is not None and r is not None:
            op[x, y] = l if (x + y) % 2 else r
    # Drop crumbs of the old blade: keep only the body (largest 8-connected blob).
    seen, best = set(), set()
    for y in range(h):
        for x in range(w):
            if op[x, y][3] < 128 or (x, y) in seen:
                continue
            blob, st = set(), [(x, y)]
            while st:
                q = st.pop()
                if q in seen or not (0 <= q[0] < w and 0 <= q[1] < h) or op[q][3] < 128:
                    continue
                seen.add(q); blob.add(q)
                st += [(q[0] + i, q[1] + j) for i in (-1, 0, 1) for j in (-1, 0, 1) if i or j]
            if len(blob) > len(best):
                best = blob
    for y in range(h):
        for x in range(w):
            if op[x, y][3] >= 128 and (x, y) not in best:
                op[x, y] = (0, 0, 0, 0)
    return out


def scythe(draw, grip, ang_deg, blade_dir, length=30, butt=11):
    a = math.radians(ang_deg)
    dx, dy = math.cos(a), math.sin(a)
    gx, gy = grip
    head = (gx + dx * length, gy + dy * length)
    tail = (gx - dx * butt, gy - dy * butt)
    draw.line([tail, head], fill=SHAFT_DARK, width=4)
    draw.line([tail, head], fill=SHAFT, width=2)
    # Crescent blade from the head, perpendicular to the shaft, curving back.
    px, py = -dy * blade_dir, dx * blade_dir        # perpendicular (towards the cut)
    outer, inner = [], []
    for i in range(9):
        t = i / 8
        bend = math.sin(t * math.pi) * 6 + t * t * 7    # curls back towards the shaft
        L = t * 19
        ox = head[0] + px * L - dx * bend
        oy = head[1] + py * L - dy * bend
        thick = 4.5 * (1 - t) + 0.6
        outer.append((ox + dx * thick * 0.4, oy + dy * thick * 0.4))
        inner.append((ox - dx * thick, oy - dy * thick))
    poly = outer + inner[::-1]
    draw.polygon(poly, fill=BLADE, outline=BLADE_DARK)
    draw.line(outer, fill=BLADE_EDGE, width=1)       # the honed edge catches the light
    return head, outer[-1]


def tip_point(grip, ang_deg, blade_dir, length=30):
    a = math.radians(ang_deg)
    dx, dy = math.cos(a), math.sin(a)
    head = (grip[0] + dx * length, grip[1] + dy * length)
    px, py = -dy * blade_dir, dx * blade_dir
    return (head[0] + px * 19 - dx * 7, head[1] + py * 19 - dy * 7)


def slash_arc(img, grip, a0, a1, blade_dir, strength):
    """A crimson smear along the path the blade tip just travelled."""
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    steps = 14
    pts = [tip_point(grip, a0 + (a1 - a0) * i / steps, blade_dir) for i in range(steps + 1)]
    for i in range(steps):
        k = (i + 1) / steps
        d.line([pts[i], pts[i + 1]], fill=(255, 60, 70, int(150 * k * strength)), width=max(1, int(4 * k)))
        d.line([pts[i], pts[i + 1]], fill=(255, 230, 230, int(170 * k * k * strength)), width=1)
    img.alpha_composite(layer)


def main():
    src = Image.open(SRC).convert("RGBA")
    sheet = Image.new("RGBA", (AW * 4, AH * 4), (0, 0, 0, 0))
    for row, d in enumerate(DIRS):
        body = strip_scythe(src.crop((0, row * CH, CW, row * CH + CH)), d)
        grip = (OX + d["grip"][0], OY + d["grip"][1])
        for f, ang in enumerate(d["ang"]):
            cell = Image.new("RGBA", (AW, AH), (0, 0, 0, 0))
            lean = (0, 1, 2, 1)[f]                                   # he leans into the blow
            ldx = lean * (1 if row == 2 else -1 if row == 1 else 0)
            ldy = lean * (1 if row == 0 else -1 if row == 3 else 0)
            front = Image.new("RGBA", (AW, AH), (0, 0, 0, 0))
            scythe(ImageDraw.Draw(front), (grip[0] + ldx, grip[1] + ldy), ang, d["blade"])
            if f == 2:
                slash_arc(front, (grip[0] + ldx, grip[1] + ldy), d["ang"][1], ang, d["blade"], 1.0)
            if f == 3:
                slash_arc(front, (grip[0] + ldx, grip[1] + ldy), d["ang"][2], ang, d["blade"], 0.45)
            if d["behind"]:
                cell.alpha_composite(front)
            cell.alpha_composite(body, (OX + ldx, OY + ldy))
            if not d["behind"]:
                cell.alpha_composite(front)
            sheet.alpha_composite(cell, (f * AW, row * AH))
    sheet.save(DST)
    print("wrote", DST, sheet.size)


if __name__ == "__main__":
    main()
