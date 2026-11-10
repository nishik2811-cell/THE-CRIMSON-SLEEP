#pragma once
#include "character.h"

// Patrolling ghost (vertical).
class Enemy_medium : public Character {
public:
	float velocity = 0.6f;
	Enemy_medium();
	void initialize();
	void movement(Character*, sf::RenderWindow*) override;
};
