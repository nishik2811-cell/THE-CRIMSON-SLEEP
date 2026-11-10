#include "map_parser.h"
#include "globals.h"
#include <cstdio>

// Parses one number; anything non-numeric (e.g. "x") yields -1.
static int parseField(const char* s, int len) {
	if (len == 0) return -1;
	int v = 0;
	for (int i = 0; i < len; ++i) {
		if (s[i] < '0' || s[i] > '9') return -1;
		v = v * 10 + (s[i] - '0');
	}
	return v;
}

bool Map_parser::parseMap(const std::string& filename, TileGrid& out) {
	FILE* f = std::fopen(asset("maps/" + filename).c_str(), "rb");
	if (!f) return false;
	for (int r = 0; r < GRID_H; ++r)
		for (int c = 0; c < GRID_W; ++c) out.cell[r][c] = sf::Vector2i(0, 0);

	char token[32]; int tl = 0;
	int row = 0, col = 0;
	auto flush = [&]() {
		if (tl == 0) return;
		int comma = -1;
		for (int i = 0; i < tl; ++i) if (token[i] == ',') { comma = i; break; }
		int x = comma < 0 ? -1 : parseField(token, comma);
		int y = comma < 0 ? -1 : parseField(token + comma + 1, tl - comma - 1);
		if (row < GRID_H && col < GRID_W) out.cell[row][col] = sf::Vector2i(x, y);
		++col; tl = 0;
	};
	int ch;
	while ((ch = std::fgetc(f)) != EOF) {
		if (ch == ' ' || ch == '\t') flush();
		else if (ch == '\n') { flush(); if (col > 0) { ++row; col = 0; } }
		else if (ch == '\r') {}
		else if (tl < 31) token[tl++] = (char)ch;
	}
	flush();
	std::fclose(f);
	out.loaded = true;
	return true;
}
