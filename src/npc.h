#pragma once
#include "character.h"

// Alfred, the frightened butler in the Entrance Hall.
class Npc : public Character {
public:
	float velocity = 0.5f;
	Npc();
	enum NpcState { standing, counting, moving };
	int state = standing;
	sf::Clock clock1;
	void initialize();
	bool near(Character* player) const;
	void movement(Character*, sf::RenderWindow*) override;
};
