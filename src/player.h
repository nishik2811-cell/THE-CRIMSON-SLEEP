#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "character.h"
#include "light.h"
#include "world.h"
#include "ds/Stack.h"
#include "ds/LinkedList.h"

// Player State Manager: position + health/fear + the two data structures
// that track progress - the movement-history Stack and the inventory
// Linked List.
class Player : public Character {
public:
	sf::View view;
	float scale_x = 1.f;
	sf::SoundBuffer soundbuffer_object, soundbuffer_door, soundbuffer_end;
	sf::Sound object_sound, door_sound, end_sound;
	bool is_mattock = false;
	Light light;

	float health = 100.f;   // 0 -> death
	float fear = 0.f;       // 100 -> the mind breaks
	float invulnerable = 0.f;
	bool frozen = false;    // input disabled (jump scare)

	Stack<int> history;                 // rooms visited, top = current room
	LinkedList<InventoryItem> inventory; // items carried

	Player();
	void movement(sf::RenderWindow*) override;
	bool has(int itemId);
	void give(int itemId);
	bool take(int itemId);
	bool hasLight() { return has(LANTERN); }
	bool isMoving() const;
	void tryMove(int dir);
	bool sliding = false;
	bool lockInput = false; // e.g. while talking to Alfred
	int autoDir = -1;       // direction walked automatically while input is locked
	int forcedDir = -1;     // test hook: hold this direction (CS_HOLD)   // being eased around a corner this frame
};
