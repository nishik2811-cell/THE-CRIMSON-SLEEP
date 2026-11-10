#!/usr/bin/env python3
"""Use your own artwork as the jump scare:
    python3 tools/make_jumpscare_from.py path/to/art.png
Crops away the decorative frame and title text (top/bottom bands), remaps
its colours by brightness onto the crimson ramp and letterboxes it to
1024x768 (nearest-neighbour, so pixels stay crisp)."""
import sys, os
import numpy as np
from PIL import Image

RAMP = np.array([(0, 0, 0), (52, 0, 12), (112, 6, 26), (176, 22, 48), (232, 96, 112), (255, 206, 206)], np.float32)
src = Image.open(sys.argv[1]).convert('RGB')
w, h = src.size
# Frame lines sit in the outer ~4% and the title text in the top/bottom ~5%.
src = src.crop((int(w * 0.045), int(h * 0.048), int(w * 0.955), int(h * 0.952)))
a = np.asarray(src, np.float32)
# Tone = mostly HSV value (so saturated dark blues keep their own step),
# plus a little luminance to separate lilac from white.
lum = (0.6 * a.max(axis=2) + 0.4 * (0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2])) / 255.0
lo, hi = 0.06, np.percentile(lum, 99.7)
lum = np.clip((lum - lo) / max(hi - lo, 1e-3), 0, 1)
idx = np.clip(np.round(lum * (len(RAMP) - 1)), 0, len(RAMP) - 1).astype(int)
out = Image.fromarray(RAMP[idx].astype(np.uint8), 'RGB')
s = min(1024 / out.width, 768 / out.height)
out = out.resize((max(1, int(out.width * s)), max(1, int(out.height * s))), Image.NEAREST)
canvas = Image.new('RGBA', (1024, 768), (0, 0, 0, 255))
canvas.paste(out, ((1024 - out.width) // 2, (768 - out.height) // 2))
dst = os.path.join(os.path.dirname(__file__), '..', 'assets', 'graphics', 'jumpscare.png')
canvas.save(dst)
print('wrote', dst)
