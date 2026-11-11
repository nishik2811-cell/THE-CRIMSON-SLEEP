#pragma once
#include "ds/Str.h"
#include "ds/TileGrid.h"

// Hand-written tokenizer for the "x,y x,y ..." map format (no streams).
class Map_parser {
public:
	bool parseMap(const Str& filename, TileGrid& out);
};
