#include "horror.h"
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
		// She hunts by sound: a still player is only sensed up close.
		float speed = pacified ? 0 : (playerMoving || l < 70 ? 52.f : 14.f);
		if (l > 1) pos += d / l * speed * dt;
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
	float k = std::min(1.f, dt * 8.f);
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
