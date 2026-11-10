#pragma once
#include "character.h"

// The Devil: chases the player directly; can walk over the dig site.
class Enemy_boss : public Character {
public:
	Enemy_boss();
	int net_x = 0;
	int net_y = 0;
	void movement(Character*, sf::RenderWindow*) override;
	int colSpecialTile();
};
