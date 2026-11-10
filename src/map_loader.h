#pragma once
#include <SFML/Graphics.hpp>
#include "ds/TileGrid.h"

class Map_loader {
	sf::Sprite sprite;
	sf::Clock animClock;
public:
	static const int anime_startIndex = 21;
	TileGrid map;
	void initializeMap(const std::string& mapFile, sf::Texture& tileset);
	void drawMap(sf::RenderWindow*);
};
