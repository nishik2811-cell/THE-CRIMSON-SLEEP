"""Builds assets/graphics/spider_dead.png: the spider squashed flat - body
pressed into a dark splat, legs splayed and crooked.
usage: python3 tools/make_spider_dead.py raw-assets/spider_source.png assets/graphics/spider_dead.png"""
import sys, random
from PIL import Image, ImageDraw, ImageEnhance

random.seed(7)
src = Image.open(sys.argv[1]).convert('RGBA')
out = Image.new('RGBA', (64, 64))
d = ImageDraw.Draw(out)
# the splat: a dark-red pool with a few flung droplets
for i in range(16):
    rx, ry = random.randint(7, 15), random.randint(4, 9)
    cx, cy = 32 + random.randint(-13, 13), 36 + random.randint(-6, 6)
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=(110, 12, 26, 235))
for i in range(12):
    r = random.randint(1, 3); cx, cy = random.randint(4, 60), random.randint(20, 56)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(130, 16, 30, 235))
d.ellipse([22, 30, 42, 42], fill=(150, 24, 40, 240))          # wet centre where the body burst
# the body pressed flat (a third of its height), legs splayed wide, darkened
flat = src.resize((80, 26), Image.NEAREST)
flat = ImageEnhance.Brightness(flat).enhance(0.5)
out.alpha_composite(flat.crop((8, 0, 72, 26)), (0, 24))
out.save(sys.argv[2])
print('wrote', sys.argv[2])
