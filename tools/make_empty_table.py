#!/usr/bin/env python3
"""Make the 'empty table' tile: the Library's round table (texture.png
column 10, row 1) with the red diary lifted off it, written into the free
cell at column 24, row 1.

    python3 tools/make_empty_table.py [assets/graphics/texture.png]

The book covers the middle of the table (columns 8-23).  The table itself is
symmetric, so its edges are kept exactly as drawn, and the middle is rebuilt
as the rest of the oval top: a dark rim along the back edge, a lit lip, the
red-brown wood with a soft sheen, then the front lip and its shadow - in the
same colours the table already uses.  When you take the diary the game swaps
that tile for this one.
"""
import sys
from PIL import Image

PATH = sys.argv[1] if len(sys.argv) > 1 else "assets/graphics/texture.png"
SRC, DST = (10 * 32, 32), (24 * 32, 32)


def hexc(h, a=255):
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


RIM_BACK = hexc("280f12")                 # the dark back edge of the oval
LIP = [hexc("3e1919"), hexc("441e1b"), hexc("4a211c")]
WOOD = [hexc("5e2a22"), hexc("5d2a22"), hexc("5f2b23"), hexc("5a2821"), hexc("5c2922")]
SHEEN = [hexc("6a3127"), hexc("6e3329"), hexc("67302a")]
FRONT = hexc("5f2b23")
FRONT_SHADOW = hexc("371616")

tex = Image.open(PATH).convert("RGBA")
tile = tex.crop((SRC[0], SRC[1], SRC[0] + 32, SRC[1] + 32))
px = tile.load()

X0, X1 = 8, 23
for y in range(0, 20):
    for x in range(X0, X1 + 1):
        h = (x * 7 + y * 13) % 5                     # a little grain, the same every run
        if y < 11:
            c = (0, 0, 0, 0)                         # the book stood here - now nothing
        elif y == 11:
            c = RIM_BACK
        elif y == 12:
            mid = abs(x - 15.5) < 5
            c = LIP[2] if mid else LIP[(x + y) % 2]
        elif y in (13, 14) and 11 <= x <= 20:
            c = SHEEN[h % 3]                         # lamplight on the polished top
        elif y <= 17:
            c = WOOD[h]
        elif y == 18:
            c = FRONT
        else:
            c = FRONT_SHADOW
        px[x, y] = c

tex.paste(tile, DST)
tex.save(PATH)
print("wrote the empty table to", PATH, "column 24, row 1")
