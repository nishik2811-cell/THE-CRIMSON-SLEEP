#include "collision.h"
#include "globals.h"

int Collision::isCollisionMap(sf::Vector2f c, int direction) {
	switch (direction) {
	case 0:
		next_tile1 = sf::Vector2i((int)(c.x / 32), (int)((c.y + 24 + 1) / 32));
		next_tile2 = sf::Vector2i((int)((c.x + 24) / 32), (int)((c.y + 24 + 1) / 32));
		break;
	case 1:
		next_tile1 = sf::Vector2i((int)((c.x - 1) / 32), (int)(c.y / 32));
		next_tile2 = sf::Vector2i((int)((c.x - 1) / 32), (int)((c.y + 24) / 32));
		break;
	case 2:
		next_tile1 = sf::Vector2i((int)((c.x + 24 + 1) / 32), (int)(c.y / 32));
		next_tile2 = sf::Vector2i((int)((c.x + 24 + 1) / 32), (int)((c.y + 24) / 32));
		break;
	default:
		next_tile1 = sf::Vector2i((int)(c.x / 32), (int)((c.y - 1) / 32));
		next_tile2 = sf::Vector2i((int)((c.x + 24) / 32), (int)((c.y - 1) / 32));
		break;
	}
	if (!current_map) return 0;
	return current_map->solid(next_tile1.x, next_tile1.y) || current_map->solid(next_tile2.x, next_tile2.y);
}

sf::Vector2i Collision::facingTile(int direction) const {
	switch (direction) {
	case 0: return sf::Vector2i(tile_location.x, tile_location.y + 1);
	case 1: return sf::Vector2i(tile_location.x - 1, tile_location.y);
	case 2: return sf::Vector2i(tile_location.x + 1, tile_location.y);
	default: return sf::Vector2i(tile_location.x, tile_location.y - 1);
	}
}

// Returns the interaction id (texture column on row 1) of the tile the
// character is facing, or -1.
int Collision::isInteractionMap(sf::Vector2f c, int direction) {
	tile_location = sf::Vector2i((int)((c.x + 12) / 32), (int)((c.y + 12) / 32));
	if (!current_map) return -1;
	sf::Vector2i f = facingTile(direction);
	sf::Vector2i t = current_map->at(f.x, f.y);
	return t.y == 1 ? t.x : -1;
}
