#pragma once
#include <SFML/Graphics.hpp>
#include "ds/TileGrid.h"
#include "ds/Str.h"

class Map_loader {
	const sf::Texture* tileset = nullptr;
	sf::Clock animClock;
public:
	static const int anime_startIndex = 21;
	TileGrid map;
	void initializeMap(const Str& mapFile, sf::Texture& tileset);
	void drawMap(sf::RenderWindow*);
};
