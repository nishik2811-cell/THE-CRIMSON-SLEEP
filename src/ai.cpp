#include "ai.h"
#include "ds/Queue.h"
#include "ds/PriorityQueue.h"
#include <cmath>
#include <cstdlib>

namespace ai {

bool walkable(const TileGrid& m, int c, int r) {
	if (!m.inside(c, r)) return false;
	sf::Vector2i t = m.at(c, r);
	return !m.solid(c, r) && !(t.x == 0 && t.y == 0);
}

bool lineOfSight(const TileGrid& m, sf::Vector2f a, sf::Vector2f b) {
	sf::Vector2f d = b - a;
	float len = std::sqrt(d.x * d.x + d.y * d.y);
	int steps = (int)(len / 8) + 1;                  // sample every 8 px: never skips a 32 px tile
	for (int i = 1; i < steps; ++i) {
		sf::Vector2f p = a + d * ((float)i / steps);
		int c = (int)(p.x / 32), r = (int)(p.y / 32);
		if (!walkable(m, c, r)) return false;
	}
	return true;
}

static const int DC[4] = { 0, -1, 1, 0 }, DR[4] = { 1, 0, 0, -1 };

// Rebuilds start -> goal from the parent links (Stack-like reversal into `path`).
static int unwind(const int* parent, int from, int to, int* path) {
	int rev[GRID_W * GRID_H]; int n = 0;
	for (int t = to; t != from && t >= 0 && n < GRID_W * GRID_H; t = parent[t]) rev[n++] = t;
	int len = 0;
	for (int i = n - 1; i >= 0 && len < MAX_PATH; --i) path[len++] = rev[i];
	return len;
}

struct OpenNode { int priority; int tile; int g; };   // priority = -f, so the max-heap pops the lowest f

int aStar(const TileGrid& m, int from, int to, int* path, int* expanded) {
	static int best[GRID_W * GRID_H], parent[GRID_W * GRID_H];
	static bool closed[GRID_W * GRID_H];
	for (int i = 0; i < GRID_W * GRID_H; ++i) { best[i] = 1 << 29; parent[i] = -1; closed[i] = false; }
	int gc = to % GRID_W, gr = to / GRID_W;
	auto h = [&](int t) { return std::abs(t % GRID_W - gc) + std::abs(t / GRID_W - gr); };
	static PriorityQueue<OpenNode, 2048> open;
	open.clear();
	best[from] = 0;
	open.insert(OpenNode{ -h(from), from, 0 });
	int n = 0;
	OpenNode cur;
	while (open.extractMax(&cur)) {
		if (closed[cur.tile]) continue;              // stale duplicate entry
		closed[cur.tile] = true; ++n;
		if (cur.tile == to) { if (expanded) *expanded = n; return unwind(parent, from, to, path); }
		int c = cur.tile % GRID_W, r = cur.tile / GRID_W;
		for (int d = 0; d < 4; ++d) {
			int nc = c + DC[d], nr = r + DR[d], nt = nr * GRID_W + nc;
			if (!m.inside(nc, nr)) continue;            // no wrapping round the map edge
			if (!walkable(m, nc, nr) && nt != to) continue;
			int g = cur.g + 1;
			if (g >= best[nt]) continue;
			best[nt] = g; parent[nt] = cur.tile;
			open.insert(OpenNode{ -(g + h(nt)), nt, g });
		}
	}
	if (expanded) *expanded = n;
	return 0;
}

int bfsEscape(const TileGrid& m, int from, sf::Vector2f threat, int depth, int* path) {
	static int parent[GRID_W * GRID_H], dist[GRID_W * GRID_H];
	for (int i = 0; i < GRID_W * GRID_H; ++i) { parent[i] = -1; dist[i] = -1; }
	Queue<int> q;
	q.enqueue(from); dist[from] = 0;
	int bestT = from; float bestD = -1;
	int cur;
	while (q.dequeue(&cur)) {
		sf::Vector2f c = centreOf(cur) - threat;
		float d = c.x * c.x + c.y * c.y;
		if (d > bestD) { bestD = d; bestT = cur; }
		if (dist[cur] >= depth) continue;
		int cc = cur % GRID_W, cr = cur / GRID_W;
		for (int k = 0; k < 4; ++k) {
			int nc = cc + DC[k], nr = cr + DR[k], nt = nr * GRID_W + nc;
			if (!walkable(m, nc, nr) || dist[nt] >= 0) continue;
			dist[nt] = dist[cur] + 1; parent[nt] = cur;
			q.enqueue(nt);
		}
	}
	return unwind(parent, from, bestT, path);
}

int bfsSearchSpot(const TileGrid& m, int around, int depth) {
	static int dist[GRID_W * GRID_H], seen[GRID_W * GRID_H];
	for (int i = 0; i < GRID_W * GRID_H; ++i) dist[i] = -1;
	Queue<int> q;
	q.enqueue(around); dist[around] = 0;
	int n = 0, cur;
	while (q.dequeue(&cur)) {
		seen[n++] = cur;
		if (dist[cur] >= depth) continue;
		int cc = cur % GRID_W, cr = cur / GRID_W;
		for (int k = 0; k < 4; ++k) {
			int nc = cc + DC[k], nr = cr + DR[k], nt = nr * GRID_W + nc;
			if (!walkable(m, nc, nr) || dist[nt] >= 0) continue;
			dist[nt] = dist[cur] + 1;
			q.enqueue(nt);
		}
	}
	return n > 1 ? seen[1 + std::rand() % (n - 1)] : around;
}
}
