"""Redraws the drawer (tileset row 1, column 0 - interaction id 0) as a dark
1950s nightstand in the same palette as the wardrobe, so it sits in the
crimson rooms instead of standing out pale.
usage: python3 tools/make_drawer.py assets/graphics/texture.png"""
import sys
from PIL import Image, ImageDraw

path = sys.argv[1]
tex = Image.open(path).convert('RGBA')
T = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
d = ImageDraw.Draw(T)
edge, wood, dark, hi, grain = (30, 14, 12, 255), (85, 50, 39, 255), (52, 28, 22, 255), (122, 78, 60, 255), (64, 36, 28, 255)
brass = (200, 170, 110, 255)
d.rectangle([7, 30, 25, 31], fill=(10, 4, 4, 140))     # shadow on the floor
d.rectangle([4, 6, 27, 29], fill=edge)                 # body outline
d.rectangle([5, 7, 26, 28], fill=wood)
d.rectangle([4, 4, 27, 7], fill=edge)                  # top slab, slightly overhanging
d.rectangle([5, 5, 26, 6], fill=hi)
for y0 in (9, 18):                                     # two drawer fronts, recessed
    d.rectangle([7, y0, 24, y0 + 7], fill=dark)
    d.rectangle([8, y0 + 1, 23, y0 + 6], outline=wood)
    d.line([(10, y0 + 2), (14, y0 + 3)], fill=grain)
    d.line([(17, y0 + 4), (21, y0 + 5)], fill=grain)
    d.rectangle([14, y0 + 3, 17, y0 + 4], fill=brass)  # brass pull
    d.point([(15, y0 + 3)], fill=(240, 220, 160, 255))
d.rectangle([6, 27, 8, 29], fill=edge)                 # little feet
d.rectangle([23, 27, 25, 29], fill=edge)
tex.paste(Image.new('RGBA', (32, 32), (0, 0, 0, 0)), (0, 32))
tex.paste(T, (0, 32), T)
tex.save(path)
print('drawer redrawn at row 1, column 0 of', path)
