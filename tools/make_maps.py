#!/usr/bin/env python3
"""Generates the two extra rooms (map9 Hidden Tunnel, map10 Ossuary) using
tiles that already exist in texture.png.  Doors use interaction ids
4..9 on texture row 1 (door number = id - 3)."""
import os, sys
W, H = 32, 24
OUT = os.path.join(os.path.dirname(__file__), '..', 'assets', 'maps')

def blank():
    return [['0,0'] * W for _ in range(H)]

def room(g, x0, y0, x1, y1, floor='x,x'):
    """Walled room: wall-top (3,2) border, wall-face (3,3) under the top wall."""
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            edge = x in (x0, x1) or y in (y0, y1)
            g[y][x] = '3,2' if edge else floor
    for x in range(x0 + 1, x1):
        g[y0 + 1][x] = '3,3'
    for x in range(x0, x1 + 1):
        g[y1 + 1][x] = '3,3'

def save(g, name):
    with open(os.path.join(OUT, name), 'w') as f:
        for row in g:
            f.write(' '.join(row) + ' \n')

# ---- map9: Hidden Tunnel ------------------------------------------------
g = blank()
room(g, 7, 2, 24, 20, floor='2,0')
for y in range(2, 21):           # pinch it into a winding tunnel
    for x in range(8, 24):
        if (y in range(6, 9) and x < 18) or (y in range(12, 15) and x > 13):
            g[y][x] = '3,2'
for x in range(8, 18): g[9][x] = '3,3'
for x in range(14, 24): g[15][x] = '3,3'
g[3][15] = '5,1'                 # door 2 -> Ossuary (top)
g[20][15] = 'x,x'; g[21][15] = '4,1'   # door 1 -> Chapel (bottom)
for (x, y) in [(10, 11), (11, 11), (20, 17), (12, 18), (21, 5)]:
    g[y][x] = '21,1'             # spikes
g[10][21] = '22,1'               # fire pit
for (x, y) in [(9, 4), (22, 10), (9, 16)]:
    g[y][x] = '13,0'             # rubble (walkable decoration)
save(g, 'map9.txt')

# ---- map10: Ossuary -----------------------------------------------------
g = blank()
room(g, 5, 2, 26, 20, floor='x,x')
g[20][15] = 'x,x'; g[21][15] = '4,1'   # door 1 -> Hidden Tunnel
g[6][15] = '12,1'                # skeleton clutching the locket
for x in (11, 12, 18, 19):
    g[5][x] = '22,1'             # ritual fire
for (x, y) in [(8, 8), (23, 8), (8, 15), (23, 15)]:
    g[y][x] = '6,2'              # bone piles (solid)
for (x, y) in [(10, 11), (21, 11), (13, 14), (18, 14)]:
    g[y][x] = '14,1'             # open graves
g[10][15] = '19,1'               # inscription
for x in range(13, 19):
    g[17][x] = '%d,0' % (7 + (x - 13) % 3)   # red carpet
save(g, 'map10.txt')
print('maps written')
