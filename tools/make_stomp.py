"""Adds a 3-frame STOMP animation to the hero sheet (columns 4-6 of every
direction row; columns 0-3 stay the walk cycle):
  4  lift   - body rises 2 px, one foot comes up
  5  stamp  - body drops and squashes, the foot slams down, dust kicks out
  6  settle - body 1 px low, dust fading
usage: python3 tools/make_stomp.py raw-assets/player_walk_only.png assets/graphics/player.png"""
import sys
from PIL import Image

src = Image.open(sys.argv[1]).convert('RGBA')
FW, FH = 24, 48
out = Image.new('RGBA', (FW * 7, src.height))
out.paste(src.crop((0, 0, FW * 4, src.height)), (0, 0))

def shifted(img, dy):
    o = Image.new('RGBA', img.size)
    o.paste(img, (0, dy))
    return o

DUST = (176, 150, 132)
def dust(f, spread, alpha):
    p = f.load()
    for dx, dy in ((-spread, 0), (spread, 0), (-spread - 2, -1), (spread + 2, -1), (-spread + 1, -2), (spread - 1, -2)):
        for x, y in ((12 + dx, 46 + dy), (12 + dx + 1, 46 + dy)):
            if 0 <= x < FW and 0 <= y < FH and p[x, y][3] == 0: p[x, y] = DUST + (alpha,)

for row in range(4):
    base = src.crop((0, row * FH, FW, row * FH + FH))
    top, legs = base.crop((0, 0, FW, 38)), base.crop((0, 38, FW, FH))
    side = 'left' if row != 2 else 'right'                 # which foot lifts (the one nearer the viewer)
    lx0, lx1 = (0, 12) if side == 'left' else (12, FW)
    # 4: lift - whole body up 2, the lifting foot up 4 more
    lift = Image.new('RGBA', (FW, FH))
    lift.alpha_composite(top, (0, -2) if False else (0, 0)); lift = shifted(lift, -2)
    other = legs.copy(); raised = Image.new('RGBA', legs.size)
    raised.paste(legs.crop((lx0, 0, lx1, legs.height)), (lx0, 0))
    o = other.load()
    for y in range(legs.height):
        for x in range(lx0, lx1): o[x, y] = (0, 0, 0, 0)
    lift.alpha_composite(other, (0, 36)); lift.alpha_composite(raised, (0, 31))
    # 5: stamp - body squashed 2 px into the floor, both feet down, dust
    stamp = Image.new('RGBA', (FW, FH))
    sq = top.resize((FW, 36), Image.NEAREST)
    stamp.alpha_composite(sq, (0, 2)); stamp.alpha_composite(legs, (0, 38))
    dust(stamp, 9, 230)
    # 6: settle - 1 px low, dust spreading and fading
    settle = shifted(base, 1)
    dust(settle, 11, 120)
    for k, fr in enumerate((lift, stamp, settle)):
        out.paste(fr, (FW * (4 + k), row * FH))
out.save(sys.argv[2])
print('wrote', sys.argv[2], out.size)
