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
	// Her trail: while you move she hears you and comes straight on.  Stop,
	// and she slows, listening around where she last heard you; only after
	// LOSE_AFTER seconds of silence has she really lost you - then she drifts off.
	static constexpr float LOSE_AFTER = 2.5f;
	bool lostTrail() const { return stillT > LOSE_AFTER; }
	bool listening() const { return stillT > 0.15f && stillT <= LOSE_AFTER; }
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

// The Weaver: a spider the size of a hand-spread, with an eight-state hunting
// brain built on the project's own data structures (see ai.h):
//   IDLE        frozen, listening
//   PATROL      walks between spots near its nest (BFS picks them, A* walks there)
//   INVESTIGATE heard something (footsteps, a flare)   -> A* to the noise
//   SEARCH      lost you                                -> BFS spots around where you were
//   CHASE       sees you (line of sight, view cone)     -> A* straight at you
//   ATTACK      close enough: a lunge
//   STUNNED     a flare hit it: it curls up for a moment, then scuttles home
//   RETURN      after a bite: back off a few tiles (BFS escape), then come again;
//               after a stun: A* back to its nest
// Stealth: with the lantern OFF it only sees you up close; hiding in a
// wardrobe makes you invisible and silent.
class Spider {
public:
	enum State { IDLE, PATROL, INVESTIGATE, SEARCH, CHASE, ATTACK, STUNNED, RETURN, STATE_COUNT };
	sf::Texture tex;
	sf::Sprite sprite{ tex };
	sf::Vector2f pos, home;      // centre and nest, in room pixels
	bool active = false;
	int dir = 0;                 // facing: Down, Left, Right, Up (sheet rows)
	State state = IDLE;
	float stateT = 0, frameT = 0, bite = 0, repathT = 0, lostT = 0, wait = 0, stun = 0;
	int searches = 0, patrols = 0;
	sf::Vector2f lastKnown;      // where it last saw / heard you
	bool sawYou = false, heardYou = false;
	int path[96] = {}, pathLen = 0, pathStep = 0, expanded = 0;
	const char* algo = "";       // which algorithm planned the current path (for the console)
	Spider();
	void reset(bool on, sf::Vector2f at);
	void hear(sf::Vector2f at, float radius);           // a noise at `at`, audible within `radius`
	// playerVisible: false while the player hides; sightRange shrinks with the lantern off.
	void update(float dt, sf::Vector2f player, bool playerVisible, float sightRange, const TileGrid& map);
	void flareHit() { enter(STUNNED); stun = 1.2f; }    // the light scares it off: brief stun, then RETURN
	void bitten() { bite = 4.5f; regroup = true; enter(RETURN); }   // it bit you: backs off a few tiles, then comes again
	bool regroup = false;        // RETURN means "back off, then come again" rather than "go home"
	void startChase(sf::Vector2f target) { lastKnown = target; enter(CHASE); }
	bool dangerous() const { return active && state != STUNNED && state != RETURN; }
	void draw(sf::RenderWindow* w);
	void drawEyes(sf::RenderWindow* w, sf::Vector2f player);   // two red glints, seen through the dark
	static const char* stateName(int s);
private:
	void enter(State s) { state = s; stateT = 0; pathLen = 0; pathStep = 0; }
	bool canSee(sf::Vector2f player, float range, const TileGrid& map) const;
	void planTo(const TileGrid& map, sf::Vector2f target);
	bool followPath(float dt, float speed, const TileGrid& map);   // false when the path is used up
	void stepTowards(sf::Vector2f target, float speed, float dt, const TileGrid& map);
};
