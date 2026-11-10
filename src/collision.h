#pragma once
#include <SFML/Graphics.hpp>

class Collision {
public:
	sf::Vector2i tile_location;
	sf::Vector2i next_tile1;
	sf::Vector2i next_tile2;
	int isCollisionMap(sf::Vector2f, int);
	int isInteractionMap(sf::Vector2f, int);
	sf::Vector2i facingTile(int direction) const;
};
