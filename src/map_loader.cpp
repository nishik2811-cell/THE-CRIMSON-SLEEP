#include "map_loader.h"
#include "map_parser.h"
#include <cstdlib>

void Map_loader::initializeMap(const Str& mapFile, sf::Texture& tex) {
	Map_parser parser;
	parser.parseMap(mapFile, map);
	tileset = &tex;
}

void Map_loader::drawMap(sf::RenderWindow* window) {
	if (!tileset) return;
	sf::Sprite sprite(*tileset);
	// Animated hazards (spikes / fire, ids >= 21) flicker between frames.
	bool step = animClock.getElapsedTime().asMilliseconds() > 120;
	if (step) animClock.restart();
	for (int i = 0; i < GRID_H; i++) {
		for (int j = 0; j < GRID_W; j++) {
			sf::Vector2i& t = map.cell[i][j];
			if (t.x < 0 || t.y < 0) continue;
			if (step && t.x >= anime_startIndex && t.y > 0 && (std::rand() % 2)) {
				t.y = (t.y + 1) % 4;
				if (t.y == 0) t.y = 1;
			}
			sprite.setPosition({ (float)(j * 32), (float)(i * 32) });
			sprite.setTextureRect(sf::IntRect({ t.x * 32, t.y * 32 }, { 32, 32 }));
			window->draw(sprite);
		}
	}
}
