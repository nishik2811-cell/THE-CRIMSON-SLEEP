#pragma once
// ---------------------------------------------------------------------
// Enemy AI toolkit (used by the Weavers - the spiders).  All hand-written,
// on the project's own data structures:
//   * A*            - chase / investigate routes   (PriorityQueue = open set)
//   * BFS           - escape routes and search spots (Queue)
//   * line of sight - a ray stepped across the tile grid
// Paths are lists of tile indices (row * GRID_W + col), start excluded.
// ---------------------------------------------------------------------
#include <SFML/System/Vector2.hpp>
#include "ds/TileGrid.h"

namespace ai {
const int MAX_PATH = 96;

bool walkable(const TileGrid& m, int c, int r);
inline int tileOf(sf::Vector2f p) { return (int)(p.y / 32) * GRID_W + (int)(p.x / 32); }
inline sf::Vector2f centreOf(int t) { return sf::Vector2f((t % GRID_W) * 32.f + 16, (t / GRID_W) * 32.f + 16); }

// True if nothing solid lies on the straight line from a to b.
bool lineOfSight(const TileGrid& m, sf::Vector2f a, sf::Vector2f b);

// A* (4-way, Manhattan heuristic).  Returns path length, 0 if unreachable.
// `expanded` receives how many tiles were taken off the open set.
int aStar(const TileGrid& m, int from, int to, int* path, int* expanded);

// BFS outward from `from` (up to `depth` steps); picks the reachable tile
// farthest from `threat` and returns the path to it.
int bfsEscape(const TileGrid& m, int from, sf::Vector2f threat, int depth, int* path);

// BFS outward from `around` (up to `depth` steps) and picks a random tile
// it reached - where to look next while searching.
int bfsSearchSpot(const TileGrid& m, int around, int depth);
}
