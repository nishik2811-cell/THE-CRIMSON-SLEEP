"""Places hiding wardrobes (tile 23,1) and drawers with health kits (tile 0,1)
into the room maps.  A spot is only used if it is floor, has a wall behind it
(the tile above is solid), is not next to a door, and - checked with a BFS
before and after - placing it does not cut any part of the room off.
Deterministic, and idempotent (skips rooms that already have them).
usage: python3 tools/place_furniture.py"""
from collections import deque

# room map -> (spawn col,row, wardrobes, drawers)
PLAN = {
    'map2.txt':  ((11, 8), 1, 0),   # Grand Corridor
    'map3.txt':  ((25, 6), 1, 0),   # Library
    'map4.txt':  ((15, 4), 0, 1),   # Collapsed Bedroom
    'map5.txt':  ((16, 17), 0, 1),  # Nursery
    'map6.txt':  ((16, 4), 1, 1),   # Chapel
    'map7.txt':  ((16, 14), 0, 1),  # Antechamber
    'map9.txt':  ((15, 4), 1, 1),   # Hidden Tunnel
    'map10.txt': ((15, 5), 1, 1),   # Ossuary
}

def load(p):
    return [l.split() for l in open(p) if l.strip()]

def tile(g, c, r):
    if r < 0 or r >= len(g) or c < 0 or c >= len(g[r]): return (0, 3)
    t = g[r][c]
    return (-1, -1) if t == 'x,x' else tuple(map(int, t.split(',')))

def walk(g, c, r):
    x, y = tile(g, c, r)
    return y <= 0 and not (x == 0 and y == 0)

def reach(g, s):
    seen = {s}; q = deque([s])
    while q:
        c, r = q.popleft()
        for n in ((c + 1, r), (c - 1, r), (c, r + 1), (c, r - 1)):
            if n not in seen and walk(g, *n): seen.add(n); q.append(n)
    return seen

def near_door(g, c, r):
    for dc in (-2, -1, 0, 1, 2):
        for dr in (-2, -1, 0, 1, 2):
            x, y = tile(g, c + dc, r + dr)
            if y == 1 and 4 <= x <= 9: return True
    return False

for name, (spawn, wardrobes, drawers) in PLAN.items():
    path = 'assets/maps/' + name
    g = load(path)
    flat = ' '.join(' '.join(r) for r in g)
    if '23,1' in flat or (drawers and name not in ('map2.txt', 'map3.txt') and ' 0,1' in ' ' + flat):
        print(name, 'already furnished'); continue
    base = reach(g, spawn)
    want = [('23,1', wardrobes), ('0,1', drawers)]
    placed = []
    cands = sorted(base, key=lambda p: (-(abs(p[0] - spawn[0]) + abs(p[1] - spawn[1])), p))
    for code, n in want:
        for (c, r) in cands:
            if n == 0: break
            if (c, r) in placed: continue
            if abs(c - spawn[0]) + abs(r - spawn[1]) < 4: continue
            if walk(g, c, r - 1) or not walk(g, c, r + 1): continue      # wall behind, floor in front
            if near_door(g, c, r): continue
            if any(abs(c - pc) + abs(r - pr) < 4 for pc, pr in placed): continue
            old = g[r][c]; g[r][c] = code
            if len(reach(g, spawn)) == len(base) - 1:                    # nothing else cut off
                placed.append((c, r)); base = reach(g, spawn); n -= 1
            else: g[r][c] = old
    with open(path, 'w') as f:
        for row in g: f.write(' '.join(row) + ' \n')
    print(name, 'placed', placed)
