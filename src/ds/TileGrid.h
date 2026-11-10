#pragma once
#include <SFML/System/Vector2.hpp>
// ---------------------------------------------------------------------
// Array: a fixed-size 2D array of tiles (one room map).  Replaces the
// original std::vector<std::vector<sf::Vector2i>> so the per-frame map
// lookup is a plain O(1) array index with no copying.
// Each cell is (tileColumn, tileRow) in texture.png, or (-1,-1) for floor.
// ---------------------------------------------------------------------
const int GRID_W = 32;
const int GRID_H = 24;

struct TileGrid {
	sf::Vector2i cell[GRID_H][GRID_W];
	bool loaded = false;

	bool inside(int col, int row) const { return col >= 0 && row >= 0 && col < GRID_W && row < GRID_H; }
	sf::Vector2i at(int col, int row) const { return inside(col, row) ? cell[row][col] : sf::Vector2i(0, 3); }
	void set(int col, int row, sf::Vector2i v) { if (inside(col, row)) cell[row][col] = v; }
	// Solid for walking purposes: any tile on texture row >= 1.
	bool solid(int col, int row) const { return at(col, row).y > 0; }
	// Finds the first interaction tile with the given id (texture row 1).
	bool find(int tileId, int* col, int* row) const {
		for (int r = 0; r < GRID_H; ++r)
			for (int c = 0; c < GRID_W; ++c)
				if (cell[r][c].y == 1 && cell[r][c].x == tileId) { *col = c; *row = r; return true; }
		return false;
	}
};
