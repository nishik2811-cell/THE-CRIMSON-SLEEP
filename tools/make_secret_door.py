#!/usr/bin/env python3
"""Draw the Chapel's secret door (texture.png column 7, row 1).

    python3 tools/make_secret_door.py [assets/graphics/texture.png]

The passage from the Chapel of Ash down to the Hidden Tunnel sits in the
chapel's south wall.  It used to borrow the narrow side-door strip, which in
a south wall looked like a loose plank.  This draws a proper door for it,
built from the house's front door (column 4): the wood aged and darkened,
two iron straps with rivets, an iron ring for a handle, and a rough stone
arch around it - an old, forgotten door.  Column 7 is used only here.
"""
import sys
from PIL import Image

PATH = sys.argv[1] if len(sys.argv) > 1 else "assets/graphics/texture.png"
tex = Image.open(PATH).convert("RGBA")
door = tex.crop((4 * 32, 32, 5 * 32, 64))
px = door.load()

# 1. Age the wood: darker, a little colder, with a few cracks.
for y in range(32):
    for x in range(32):
        r, g, b, a = px[x, y]
        if a == 0:
            continue
        px[x, y] = (int(r * 0.62), int(g * 0.55), int(b * 0.58), a)
for (x, y0, y1) in ((11, 4, 12), (20, 17, 27), (15, 24, 30)):
    for y in range(y0, y1):
        px[x, y] = (24, 14, 14, 255)

# 2. Iron straps across the door, with rivets.
IRON, IRON_HI, RIVET = (52, 50, 56, 255), (92, 90, 98, 255), (130, 126, 132, 255)
for sy in (8, 22):
    for x in range(4, 28):
        px[x, sy] = IRON_HI
        px[x, sy + 1] = IRON
        px[x, sy + 2] = (34, 32, 38, 255)
    for x in (6, 13, 19, 26):
        px[x, sy + 1] = RIVET

# 3. An iron ring handle.
for (x, y) in ((22, 14), (23, 14), (24, 15), (24, 16), (23, 17), (22, 17), (21, 16), (21, 15)):
    px[x, y] = IRON_HI
px[22, 15] = IRON

# 4. A rough stone arch framing it.
STONE, STONE_DK = (112, 100, 104, 255), (70, 62, 66, 255)
for x in range(0, 32):
    px[x, 0] = STONE_DK
    px[x, 1] = STONE if (x // 4) % 2 == 0 else (98, 88, 92, 255)
for y in range(0, 32):
    for x in (0, 1, 30, 31):
        px[x, y] = STONE if (y // 4 + x) % 2 == 0 else STONE_DK
for (x, y) in ((2, 2), (3, 2), (2, 3), (29, 2), (28, 2), (29, 3)):
    px[x, y] = STONE_DK

tex.paste(door, (7 * 32, 32))
tex.save(PATH)
print("drew the Chapel's secret door into", PATH, "column 7, row 1")
