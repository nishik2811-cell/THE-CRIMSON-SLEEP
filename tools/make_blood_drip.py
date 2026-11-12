"""Cuts the blood-drip photo off its white background (un-premultiplying the
colour, so thin runs and soft edges stay translucent instead of pink) and
saves a strip for the top edge of the survival-note card.
usage: python3 tools/make_blood_drip.py raw-assets/blood_drips_source.png assets/graphics/blood_drip.png"""
import sys
from PIL import Image

src = Image.open(sys.argv[1]).convert('RGB')
W, H = src.size
out = Image.new('RGBA', (W, H))
sp, op = src.load(), out.load()
for y in range(H):
    for x in range(W):
        r, g, b = sp[x, y]
        a = (255 - min(g, b)) / 255.0            # how far from white (blood keeps red, loses green/blue)
        a = max(0.0, min(1.0, (a - 0.06) / 0.94))  # drop the faint grey haze of the paper
        if a <= 0: op[x, y] = (0, 0, 0, 0); continue
        un = lambda c: int(max(0, min(255, (c - (1 - a) * 255) / a)))
        op[x, y] = (un(r), un(g), un(b), int(a * 255))
# Trim the empty margins, then scale to the card's width
bbox = out.getbbox()
out = out.crop(bbox)
target_w = 360
out = out.resize((target_w, int(out.height * target_w / out.width)), Image.LANCZOS)
out.save(sys.argv[2])
print('wrote', sys.argv[2], out.size)
