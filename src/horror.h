#pragma once
#include <SFML/Graphics.hpp>
#include "collision.h"

// ---------------------------------------------------------------------
// New horrors for The Crimson Sleep.
// ---------------------------------------------------------------------

// The Weeping Bride: drifts THROUGH walls toward the player.  She hunts by
// sound - stand still and she loses you.  A lantern flare (F) banishes her;
// carrying her Crimson Locket pacifies her for good.
class Bride {
public:
	enum State { GONE, FADE_IN, HUNT, FADE_OUT };
	sf::Texture tex;
	sf::Sprite sprite{ tex };
	sf::Vector2f pos;
	State state = GONE;
	float timer = 0, alpha = 0, frameT = 0, wobble = 0;
	bool active = false;     // allowed in this room
	bool pacified = false;
	float stillT = 0;        // how long the player has been standing still
	Bride();
	void reset(bool allowed, float firstDelay);
	void banish(float respawn);
	void update(float dt, sf::Vector2f player, bool playerMoving);
	void draw(sf::RenderWindow* w);
	bool visible() const { return state == HUNT || state == FADE_IN; }
	// She hunts by sound: once you have stood still for a moment she loses
	// your trail - she stops coming for you and cannot hurt you.
	bool lostTrail() const { return stillT > 0.3f; }
	float distanceTo(sf::Vector2f p) const;
};

// The Hollow Child: crawls fast along the floor, but only while you are
// NOT looking at it.  Face it to freeze it; a flare drives it away.
class Crawler {
public:
	sf::Texture tex;
	sf::Sprite sprite{ tex };
	Collision col;
	sf::Vector2f pos, home;
	bool active = false;
	bool frozen = true;
	float stun = 0, frameT = 0;
	sf::Vector2f vel;            // smoothed velocity (eases in/out instead of lurching)
	float walked = 0, bob = 0;   // distance-driven walk cycle
	bool faceLeft = false;
	float wake = 0;              // seconds before it starts hunting
	float speed = 115.f;         // hunting speed (px/s)
	int frame = 0;
	Crawler();
	void reset(bool allowed, sf::Vector2f spawn);
	void update(float dt, sf::Vector2f player, int playerFacing);
	void draw(sf::RenderWindow* w);
	float distanceTo(sf::Vector2f p) const;
	bool watched(sf::Vector2f player, int facing) const;
};

