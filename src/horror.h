#pragma once
#include <SFML/Graphics.hpp>
#include "collision.h"
#include "ds/TileGrid.h"

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

// The Weaver: a spider the size of a hand-spread, with a real hunting brain
// (see ai.h).  A state machine drives it:
//   IDLE    skitters about in short bursts
//   ALERT   heard you (footsteps, a flare) -> A* to where the sound came from
//   CHASE   sees you (line of sight inside its view cone) -> A* straight at you
//   ATTACK  close enough: a lunge
//   FLEE    after a bite or a flare -> BFS to the tile farthest from you
//   SEARCH  lost you -> BFS picks spots around where you were last seen
class Spider {
public:
	enum State { IDLE, ALERT, CHASE, ATTACK, FLEE, SEARCH };
	sf::Texture tex;
	sf::Sprite sprite{ tex };
	sf::Vector2f pos;            // centre, in room pixels
	bool active = false;
	int dir = 0;                 // facing: Down, Left, Right, Up (sheet rows)
	State state = IDLE;
	float stateT = 0, burst = 0, rest = 1, frameT = 0, flee = 0, bite = 0, repathT = 0, lostT = 0;
	int searches = 0;
	sf::Vector2f lastKnown;      // where it last saw / heard you
	bool sawYou = false, heardYou = false;
	int path[96] = {}, pathLen = 0, pathStep = 0, expanded = 0;
	const char* algo = "";       // which algorithm planned the current path (for the console)
	Spider();
	void reset(bool on, sf::Vector2f at);
	void hear(sf::Vector2f at, float radius);   // a noise at `at`, audible within `radius`
	void update(float dt, sf::Vector2f player, const TileGrid& map);
	void scatter() { enter(FLEE); flee = 4.f; }
	void startChase(sf::Vector2f target) { lastKnown = target; enter(CHASE); }
	void draw(sf::RenderWindow* w);
	static const char* stateName(State s);
private:
	void enter(State s) { state = s; stateT = 0; pathLen = 0; pathStep = 0; }
	bool canSee(sf::Vector2f player, const TileGrid& map) const;
	void planTo(const TileGrid& map, sf::Vector2f target);
	bool followPath(float dt, float speed, const TileGrid& map);   // false when the path is used up
	void stepTowards(sf::Vector2f target, float speed, float dt, const TileGrid& map);
};
