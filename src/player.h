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
	bool hiddenAway = false; // hiding inside a wardrobe: not drawn
	bool stomping = false;   // playing the 3-frame stomp (columns 4-6 of the sheet)
	bool scriptedWalk = false; // walking through a doorway: animate the walk though input is off
	sf::Clock stompClock;
	void startStomp() { stomping = true; stompClock.restart(); }
	static constexpr float STOMP_FRAME = 0.11f;   // seconds per stomp frame: lift, STAMP, settle
	// Sword swing: 4 frames (wind-up, swing, STRIKE, recover) from player_sword.png,
	// 72x72 cells with the hero at (24, 12) inside each cell.
	sf::Texture swordTex;
	sf::Sprite swordSprite{ swordTex };
	bool swinging = false;
	sf::Clock swingClock;
	void startSwing() { swinging = true; swingClock.restart(); }
	static constexpr float SWING_FRAME = 0.075f;
	// Block (C held): the Blessed Sword raised across the body, from
	// player_block.png - raise, guard, and a CLASH frame with sparks.
	sf::Texture blockTex;
	sf::Sprite blockSprite{ blockTex };
	bool blocking = false;
	float blockT = 0, clashT = 0;
	float cutT = 0;                              // just cut by the scythe: flash red instead of blinking out                // time in the guard / clash spark left

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
