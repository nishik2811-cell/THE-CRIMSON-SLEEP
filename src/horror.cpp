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
		else if (lostTrail()) { if (l > 1) pos -= d / l * 10.f * dt; }
		else if (l > 1) pos += d / l * 52.f * dt;
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
const char* Spider::stateName(State s) {
	switch (s) { case IDLE: return "IDLE"; case ALERT: return "ALERT"; case CHASE: return "CHASE";
	             case ATTACK: return "ATTACK"; case FLEE: return "FLEE"; default: return "SEARCH"; }
}
void Spider::reset(bool on, sf::Vector2f at) {
	active = on; pos = at; burst = 0; rest = frand(0.5f, 2.f); flee = 0; bite = 0; dir = std::rand() % 4;
	sawYou = heardYou = false; algo = ""; expanded = 0; searches = 0; lostT = 0;
	enter(IDLE);
}
static bool spiderBlocked(const TileGrid& m, sf::Vector2f p) {
	for (int k = 0; k < 4; ++k) {                       // a little box around the body
		float x = p.x + (k % 2 ? 9.f : -9.f), y = p.y + (k / 2 ? 9.f : -9.f);
		if (!ai::walkable(m, (int)(x / 32), (int)(y / 32))) return true;
	}
	return false;
}
// SOUND: footsteps carry a little way, a flare a long way.  Standing still is silent.
void Spider::hear(sf::Vector2f at, float radius) {
	if (!active || state == FLEE || state == ATTACK) return;
	sf::Vector2f d = at - pos;
	if (len(d) > radius) return;
	heardYou = true; lastKnown = at;
	if (state != CHASE) { enter(ALERT); algo = "A*"; }
}
// SIGHT: within 260 px, inside a 120-degree cone ahead of it, and no wall in between.
bool Spider::canSee(sf::Vector2f p, const TileGrid& map) const {
	sf::Vector2f d = p - pos;
	float l = len(d);
	if (l > 260) return false;
	if (l < 40) return true;                            // right on top of it: it feels you
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
	// slide along walls axis by axis
	if (!spiderBlocked(map, pos + sf::Vector2f(step.x, 0))) pos.x += step.x;
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
void Spider::update(float dt, sf::Vector2f player, const TileGrid& map) {
	if (!active) return;
	if (bite > 0) bite -= dt;
	stateT += dt;
	float dist = len(player - pos);
	bool sees = canSee(player, map);
	sawYou = sees;
	if (sees && state != FLEE && state != ATTACK) { lastKnown = player; lostT = 0; if (state != CHASE) enter(CHASE); }

	switch (state) {
	case IDLE: {                                        // skitter in bursts, freeze, repeat
		heardYou = false;
		if (rest > 0) { rest -= dt; break; }
		if (burst > 0) {
			burst -= dt;
			sf::Vector2f v = dir == 0 ? sf::Vector2f(0, 1) : dir == 1 ? sf::Vector2f(-1, 0) : dir == 2 ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
			sf::Vector2f before = pos;
			stepTowards(pos + v * 40.f, 110.f, dt, map);
			if (len(pos - before) < 0.01f) burst = 0;   // hit a wall
			break;
		}
		burst = frand(0.3f, 0.9f); rest = frand(0.4f, 1.6f); dir = std::rand() % 4;
		break;
	}
	case ALERT:                                         // go and look where the noise came from
		if (pathLen == 0) planTo(map, lastKnown);
		if (!followPath(dt, 95.f, map)) { enter(SEARCH); searches = 0; }
		break;
	case CHASE:
		if (!sees) { lostT += dt; if (lostT > 1.2f) { enter(SEARCH); searches = 0; break; } }
		if (dist < 34) { enter(ATTACK); break; }
		repathT -= dt;
		if (repathT <= 0 || pathStep >= pathLen) { planTo(map, lastKnown); repathT = 0.35f; }
		if (!followPath(dt, 120.f, map)) stepTowards(lastKnown, 120.f, dt, map);
		break;
	case ATTACK:                                        // a short lunge straight at you
		stepTowards(player, 210.f, dt, map);
		if (stateT > 0.5f) { enter(FLEE); flee = 2.5f; }
		break;
	case FLEE:                                          // BFS: run to the farthest tile it can reach
		flee -= dt;
		if (pathLen == 0) { pathLen = ai::bfsEscape(map, ai::tileOf(pos), player, 8, path); pathStep = 0; algo = "BFS"; expanded = 0; }
		if (!followPath(dt, 160.f, map) && flee <= 0) enter(IDLE);
		if (flee <= -3.f) enter(IDLE);
		break;
	case SEARCH:                                        // BFS picks nearby spots; look around a few times
		if (pathLen == 0) {
			if (searches >= 3) { enter(IDLE); break; }
			int spot = ai::bfsSearchSpot(map, ai::tileOf(lastKnown), 4);
			pathLen = ai::aStar(map, ai::tileOf(pos), spot, path, &expanded); pathStep = 0;
			algo = "BFS+A*"; ++searches;
		}
		if (!followPath(dt, 80.f, map)) pathLen = 0;
		break;
	}
}
void Spider::draw(sf::RenderWindow* w) {
	if (!active) return;
	int frame = (int)(frameT * 16) % 6;                 // 6-frame walk cycle
	sprite.setTextureRect(sf::IntRect({ frame * 64, dir * 64 }, { 64, 64 }));
	sprite.setPosition(pos);
	w->draw(sprite);
}
