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
	void stepTo(sf::Vector2f target, Character* player, sf::RenderWindow* window);   // follow a planned path
	void finishFrame(Character* player, sf::RenderWindow* window);                   // animate and draw
};
