"""Adds a wardrobe (a hiding spot) to the tileset at row 1, column 23
(interaction id 23).  Drawn in the same dark-wood palette as the drawer.
usage: python3 tools/make_wardrobe.py assets/graphics/texture.png"""
import sys
from PIL import Image, ImageDraw

path = sys.argv[1]
tex = Image.open(path).convert('RGBA')
if tex.width < 800:                                   # make room for column 23 (and a spare)
    wide = Image.new('RGBA', (800, tex.height))
    wide.paste(tex, (0, 0))
    tex = wide

W = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
d = ImageDraw.Draw(W)
wood, dark, edge, hi = (85, 50, 39, 255), (52, 28, 22, 255), (30, 14, 12, 255), (122, 78, 60, 255)
d.rectangle([3, 1, 28, 31], fill=edge)                # body outline
d.rectangle([4, 2, 27, 30], fill=wood)
d.rectangle([4, 2, 27, 4], fill=hi)                   # cornice
d.line([(4, 5), (27, 5)], fill=edge)
d.rectangle([6, 7, 15, 28], fill=dark)                # left door
d.rectangle([16, 7, 25, 28], fill=dark)               # right door
d.rectangle([7, 8, 14, 27], outline=wood)
d.rectangle([17, 8, 24, 27], outline=wood)
d.line([(15, 7), (15, 28)], fill=(10, 4, 4, 255))     # the gap between the doors: something could be inside
d.line([(16, 7), (16, 28)], fill=(10, 4, 4, 255))
d.point([(13, 17), (18, 17)], fill=(200, 170, 110, 255))   # brass knobs
d.rectangle([4, 29, 27, 30], fill=edge)               # plinth
for y in (11, 20):                                    # grain
    d.line([(8, y), (12, y + 1)], fill=(64, 36, 28, 255))
    d.line([(19, y + 2), (23, y + 3)], fill=(64, 36, 28, 255))
tex.paste(W, (23 * 32, 32), W)
tex.save(path)
print('wardrobe added at row 1, column 23 of', path)
