#include <cstdint>
#include "horror.h"
#include "ai.h"
#include "ds/MinMax.h"
#include "ds/MinMax.h"
#include "globals.h"
#include <cmath>
#include <cstdlib>

static float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
static float len(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// ------------------------------- Bride --------------------------------
Bride::Bride() {
	if (tex.loadFromFile(asset("graphics/wraith.png"))) sprite.setTexture(tex, true);
	sprite.setOrigin({ 12.f, 40.f });
}
void Bride::reset(bool allowed, float firstDelay) {
	active = allowed; state = GONE; timer = firstDelay; alpha = 0;
}
void Bride::banish(float respawn) { state = FADE_OUT; timer = respawn; }
float Bride::distanceTo(sf::Vector2f p) const { return len(sf::Vector2f(p.x + 12, p.y + 12) - pos); }

void Bride::update(float dt, sf::Vector2f player, bool playerMoving) {
	if (!active) return;
	sf::Vector2f target(player.x + 12, player.y + 12);
	frameT += dt; wobble += dt;
	stillT = playerMoving ? 0.f : stillT + dt;
	switch (state) {
	case GONE:
		timer -= dt;
		if (timer <= 0) {
			// Manifest somewhere around the player, just outside the light.
			float a = frand(0, 6.283f);
			pos = target + sf::Vector2f(std::cos(a), std::sin(a)) * frand(170, 230);
			if (pos.x < 40) pos.x = 40;
			if (pos.x > 984) pos.x = 984;
			if (pos.y < 60) pos.y = 60;
			if (pos.y > 740) pos.y = 740;
			state = FADE_IN; alpha = 0;
		}
		break;
	case FADE_IN:
		alpha += dt * 160; if (alpha >= 200) { alpha = 200; state = HUNT; timer = frand(14, 20); }
		break;
	case HUNT: {
		sf::Vector2f d = target - pos;
		float l = len(d);
		// She hunts by sound: while you move she comes straight for you; stand
		// still and she loses the trail and slowly drifts away.
		if (pacified) {}
		else if (lostTrail()) { if (l > 1) pos -= d / l * 14.f * dt; }                 // lost you: she drifts away
		else if (listening()) {                                                            // you stopped: she slows, listening
			float k = 1.f - stillT / LOSE_AFTER;                                           // ...slower and slower
			if (l > 1) pos += d / l * 24.f * k * dt;
		}
		else if (l > 1) pos += d / l * 52.f * dt;                                          // she hears your steps: straight at you
		pos.x += std::sin(wobble * 2.1f) * 12 * dt;
		timer -= dt;
		if (timer <= 0 || (pacified && timer < 12)) banish(frand(18, 28));
		alpha = 175 + 45 * std::sin(wobble * 7) - ((std::rand() % 40 == 0) ? 120 : 0);
		break;
	}
	case FADE_OUT:
		alpha -= dt * 220;
		if (alpha <= 0) { alpha = 0; state = GONE; }
		break;
	}
}
void Bride::draw(sf::RenderWindow* w) {
	if (!active || state == GONE) return;
	int f = (int)(frameT * 5) % 4;
	sprite.setTextureRect(sf::IntRect({ f * 24, 0 }, { 24, 48 }));
	sprite.setPosition({ pos.x, pos.y + std::sin(wobble * 3) * 2 });
	std::uint8_t a = (std::uint8_t)(alpha < 0 ? 0 : alpha > 255 ? 255 : alpha);
	sprite.setColor(sf::Color(255, 235, 235, a));
	w->draw(sprite);
}

// ------------------------------ Crawler -------------------------------
Crawler::Crawler() {
	if (tex.loadFromFile(asset("graphics/hollow_child.png"))) sprite.setTexture(tex, true);
	sprite.setOrigin({ 0.f, 24.f });
}
void Crawler::reset(bool allowed, sf::Vector2f spawn) { active = allowed; pos = home = spawn; stun = 0; }
float Crawler::distanceTo(sf::Vector2f p) const { return len(p - pos); }

// Is the player facing toward the crawler (within a ~100 degree cone)?
bool Crawler::watched(sf::Vector2f p, int facing) const {
	sf::Vector2f d = pos - p;
	float l = len(d); if (l < 1) return true;
	d /= l;
	sf::Vector2f look = facing == 0 ? sf::Vector2f(0, 1) : facing == 1 ? sf::Vector2f(-1, 0) : facing == 2 ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
	return d.x * look.x + d.y * look.y > 0.64f && l < 260;
}
void Crawler::update(float dt, sf::Vector2f p, int facing) {
	if (!active) return;
	if (stun > 0) { stun -= dt; frozen = true; vel = { 0, 0 }; return; }
	if (wake > 0) { wake -= dt; frozen = true; return; }
	frozen = watched(p, facing);
	sf::Vector2f target(0, 0);
	if (!frozen) {
		sf::Vector2f d = p - pos;
		float l = len(d);
		if (l > 1) target = d / l * speed;          // faster than walking speed (slower in the Nursery)
	}
	// ease toward the target velocity: smooth starts and stops
	float k = minOf(1.f, dt * 8.f);
	vel += (target - vel) * k;
	// axis-separated movement so it slides along walls instead of sticking
	sf::Vector2f before = pos;
	if (std::fabs(vel.x) > 1 && !col.isCollisionMap(pos, vel.x > 0 ? 2 : 1)) pos.x += vel.x * dt;
	if (std::fabs(vel.y) > 1 && !col.isCollisionMap(pos, vel.y > 0 ? 0 : 3)) pos.y += vel.y * dt;
	float moved = len(pos - before);
	if (std::fabs(vel.x) > 8) faceLeft = vel.x < 0;
	// the walk cycle follows distance covered, so it never moonwalks
	walked += moved;
	if (walked > 7.f) { walked = 0; frame = (frame + 1) % 4; }
	bob = moved > 0.05f ? bob + moved * 0.45f : bob * 0.85f;
}
void Crawler::draw(sf::RenderWindow* w) {
	if (!active) return;
	sprite.setTextureRect(sf::IntRect({ frame * 24, 0 }, { 24, 48 }));
	// Drawn 1.6x larger, anchored at the feet (bottom-centre of the frame) so it
	// still stands where its 24x24 foot box is; flipped to face its direction.
	const float SCALE = 1.6f;
	float lift = std::fabs(std::sin(bob)) * 2.f;         // little step bounce
	sprite.setOrigin({ 12.f, 48.f });
	sprite.setScale({ faceLeft ? -SCALE : SCALE, SCALE });
	sprite.setPosition({ pos.x + 12.f, pos.y + 24.f - lift });
	sprite.setColor(stun > 0 ? sf::Color(255, 120, 120, 200) : sf::Color::White);
	w->draw(sprite);
}

// ------------------------------------------------------------------ spider
Spider::Spider() {
	if (tex.loadFromFile(asset("graphics/spider.png"))) sprite.setTexture(tex, true);
	sprite.setOrigin({ 32.f, 32.f });
	sprite.setScale({ 0.6f, 0.6f });
}
const char* Spider::stateName(int s) {
	static const char* n[] = { "IDLE", "PATROL", "INVESTIGATE", "SEARCH", "CHASE", "ATTACK", "STUNNED", "RETURN" };
	return s >= 0 && s < STATE_COUNT ? n[s] : "?";
}
void Spider::reset(bool on, sf::Vector2f at) {
	active = on; pos = home = at; bite = 0; stun = 0; dir = std::rand() % 4;
	sawYou = heardYou = false; algo = ""; expanded = 0; searches = patrols = 0; lostT = 0; wait = frand(0.5f, 2.f);
	enter(IDLE);
}
static bool spiderBlocked(const TileGrid& m, sf::Vector2f p) {
	for (int k = 0; k < 4; ++k) {                       // a little box around the body
		float x = p.x + (k % 2 ? 9.f : -9.f), y = p.y + (k / 2 ? 9.f : -9.f);
		if (!ai::walkable(m, (int)(x / 32), (int)(y / 32))) return true;
	}
	return false;
}
// SOUND: footsteps carry a few tiles, a flare a long way.  Standing still is silent.
void Spider::hear(sf::Vector2f at, float radius) {
	if (!active || state == STUNNED || state == ATTACK || state == CHASE) return;
	if (len(at - pos) > radius) return;
	heardYou = true; lastKnown = at;
	if (state != INVESTIGATE) enter(INVESTIGATE);
}
// SIGHT: inside a 120-degree cone ahead of it, within range, no wall in between.
bool Spider::canSee(sf::Vector2f p, float range, const TileGrid& map) const {
	sf::Vector2f d = p - pos;
	float l = len(d);
	if (l > range) return false;
	if (l < 36) return true;                            // right on top of it: it feels you
	sf::Vector2f look = dir == 0 ? sf::Vector2f(0, 1) : dir == 1 ? sf::Vector2f(-1, 0) : dir == 2 ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
	if ((d.x * look.x + d.y * look.y) / l < 0.5f) return false;   // cos 60
	return ai::lineOfSight(map, pos, p);
}
void Spider::planTo(const TileGrid& map, sf::Vector2f target) {
	pathLen = ai::aStar(map, ai::tileOf(pos), ai::tileOf(target), path, &expanded);
	pathStep = 0; algo = "A*";
}
void Spider::stepTowards(sf::Vector2f target, float speed, float dt, const TileGrid& map) {
	sf::Vector2f d = target - pos;
	float l = len(d);
	if (l < 0.5f) return;
	sf::Vector2f step = d / l * minOf(speed * dt, l);
	if (!spiderBlocked(map, pos + sf::Vector2f(step.x, 0))) pos.x += step.x;   // slide along walls
	if (!spiderBlocked(map, pos + sf::Vector2f(0, step.y))) pos.y += step.y;
	dir = std::fabs(d.x) > std::fabs(d.y) ? (d.x > 0 ? 2 : 1) : (d.y > 0 ? 0 : 3);
	frameT += dt;
}
bool Spider::followPath(float dt, float speed, const TileGrid& map) {
	if (pathStep >= pathLen) return false;
	sf::Vector2f t = ai::centreOf(path[pathStep]);
	if (len(t - pos) < 3.f) { ++pathStep; return pathStep < pathLen; }
	stepTowards(t, speed, dt, map);
	return true;
}
void Spider::update(float dt, sf::Vector2f player, bool visible, float sightRange, const TileGrid& map) {
	if (!active) return;
	if (bite > 0) bite -= dt;
	stateT += dt;
	float dist = len(player - pos);
	bool sees = visible && state != STUNNED && canSee(player, sightRange, map);
	sawYou = sees;
	if (sees && state != ATTACK && state != RETURN) { lastKnown = player; lostT = 0; if (state != CHASE) enter(CHASE); }

	switch (state) {
	case IDLE:                                          // frozen, listening
		heardYou = false;
		if ((wait -= dt) <= 0) { enter(PATROL); patrols = 0; }
		break;
	case PATROL:                                        // BFS picks a spot near the nest, A* walks there
		if (pathLen == 0) {
			if (patrols >= 3) { enter(IDLE); wait = frand(1.f, 3.f); break; }
			int spot = ai::bfsSearchSpot(map, ai::tileOf(home), 5);
			pathLen = ai::aStar(map, ai::tileOf(pos), spot, path, &expanded); pathStep = 0;
			algo = "BFS+A*"; ++patrols;
		}
		if (!followPath(dt, 70.f, map)) pathLen = 0;
		break;
	case INVESTIGATE:                                   // go and look where the noise came from
		if (pathLen == 0) planTo(map, lastKnown);
		if (!followPath(dt, 95.f, map)) { enter(SEARCH); searches = 0; }
		break;
	case SEARCH:                                        // BFS picks spots around where you were last seen
		if (pathLen == 0) {
			if (searches >= 3) { enter(RETURN); break; }
			int spot = ai::bfsSearchSpot(map, ai::tileOf(lastKnown), 4);
			pathLen = ai::aStar(map, ai::tileOf(pos), spot, path, &expanded); pathStep = 0;
			algo = "BFS+A*"; ++searches;
		}
		if (!followPath(dt, 80.f, map)) pathLen = 0;
		break;
	case CHASE:
		if (!sees) { lostT += dt; if (lostT > 1.2f) { enter(SEARCH); searches = 0; break; } }
		if (dist < 27 && bite <= 0) { enter(ATTACK); break; }   // right at your feet: inside the stomp ring
		if (dist < 34 && bite > 0) { regroup = true; enter(RETURN); algo = ""; break; }   // can't bite yet: never sits on your feet - it backs off
		repathT -= dt;
		if (repathT <= 0 || pathStep >= pathLen) { planTo(map, lastKnown); repathT = 0.35f; }
		if (!followPath(dt, 115.f, map)) stepTowards(lastKnown, 115.f, dt, map);
		break;
	case ATTACK:                                        // rears up (your chance to STOMP), then lunges
		if (stateT > 0.55f) stepTowards(player, 200.f, dt, map);
		if (stateT > 1.0f) { regroup = true; enter(RETURN); algo = ""; }   // missed or not: back off, then come again
		break;
	case STUNNED:                                       // curled up and twitching - it can be squashed now
		if ((stun -= dt) <= 0) { regroup = false; enter(RETURN); }   // scared off by the light: all the way home
		break;
	case RETURN:
		if (regroup) {
			// After a bite it clings for a moment, then backs off a few tiles
			// (BFS away from you) - slowly, keeping its eyes on you - crouches,
			// and comes at you again.
			float cling = clung ? 0.7f : 0.f;
			if (stateT < cling) { pos += sf::Vector2f(frand(-0.6f, 0.6f), frand(-0.6f, 0.6f)); break; }   // still on you, twitching
			if (pathLen == 0 && algo[0] == 0) { pathLen = ai::bfsEscape(map, ai::tileOf(pos), player, 4, path); pathStep = 0; algo = "BFS"; }
			sf::Vector2f look = player - pos;               // keep facing you while it backs away
			bool moving = followPath(dt, 135.f, map);           // darts away
			if (std::fabs(look.x) > std::fabs(look.y)) dir = look.x > 0 ? 2 : 1; else dir = look.y > 0 ? 0 : 3;
			if (!moving && stateT > cling + 3.2f) {   // ...crouch a moment, then come at you again
				clung = false;
				regroup = false; lastKnown = player; lostT = 0;
				enter(sees ? CHASE : INVESTIGATE);
			}
			break;
		}
		if (pathLen == 0) planTo(map, home);            // A* back to the nest, then rest
		if (!followPath(dt, 130.f, map) && stateT > 0.3f) { enter(IDLE); wait = frand(1.5f, 3.f); }
		break;
	default: break;
	}
}
void Spider::draw(sf::RenderWindow* w) {
	if (!active) return;
	int frame = (int)(frameT * 16) % 6;                 // 6-frame walk cycle
	sprite.setTextureRect(sf::IntRect({ frame * 64, dir * 64 }, { 64, 64 }));
	sprite.setPosition(pos);
	if (state == ATTACK && stateT < 0.55f) {             // rearing up: bigger, twitching - stomp NOW
		sprite.setScale({ 0.72f, 0.72f });
		sprite.setPosition(pos + sf::Vector2f(frand(-1.f, 1.f), frand(-1.f, 1.f)));
	}
	if (state == STUNNED) {                             // curled up: smaller, greyed, shaking
		sprite.setPosition(pos + sf::Vector2f(frand(-1.5f, 1.5f), frand(-1.5f, 1.5f)));
		sprite.setScale({ 0.48f, 0.48f });
		sprite.setColor(sf::Color(150, 150, 170));
		sprite.setRotation(sf::degrees(std::sin(stateT * 30.f) * 8.f));
	}
	w->draw(sprite);
	sprite.setScale({ 0.6f, 0.6f }); sprite.setColor(sf::Color::White); sprite.setRotation(sf::degrees(0));
}
void Spider::drawEyes(sf::RenderWindow* w, sf::Vector2f player) {
	if (!active || state == STUNNED) return;
	float l = len(player - pos);
	if (l > 340) return;
	float a = l < 120 ? 0.f : minOf(1.f, (l - 120) / 80.f);   // in the lantern light the body shows instead
	if (a <= 0) return;
	sf::Vector2f f = dir == 0 ? sf::Vector2f(0, 1) : dir == 1 ? sf::Vector2f(-1, 0) : dir == 2 ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
	sf::Vector2f side(f.y, -f.x);
	float glow = 0.75f + 0.25f * std::sin(stateT * 6.f);
	for (int s = -1; s <= 1; s += 2) {
		sf::CircleShape e(2.f); e.setOrigin({ 2.f, 2.f });
		e.setPosition(pos + f * 9.f + side * (3.5f * s));
		e.setFillColor(sf::Color(255, 40, 50, (std::uint8_t)(230 * a * glow)));
		w->draw(e, sf::BlendAdd);
	}
}
