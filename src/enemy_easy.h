#pragma once
#include "character.h"

// Patrolling ghost (horizontal).
class Enemy_easy : public Character {
public:
	float velocity = 0.5f;
	Enemy_easy();
	void initialize();
	void movement(Character*, sf::RenderWindow*) override;
};
