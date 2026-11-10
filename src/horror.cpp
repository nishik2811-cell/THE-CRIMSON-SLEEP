#include "horror.h"
#include "globals.h"
#include <cmath>
#include <cstdlib>

static float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
static float len(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// ------------------------------- Bride --------------------------------
Bride::Bride() {
	tex.loadFromFile(asset("graphics/wraith.png"));
	sprite.setTexture(tex);
	sprite.setOrigin(12, 40);
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
			if (pos.x < 40) pos.x = 40; if (pos.x > 984) pos.x = 984;
			if (pos.y < 60) pos.y = 60; if (pos.y > 740) pos.y = 740;
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
	sprite.setTextureRect(sf::IntRect(f * 24, 0, 24, 48));
	sprite.setPosition(pos.x, pos.y + std::sin(wobble * 3) * 2);
	sf::Uint8 a = (sf::Uint8)(alpha < 0 ? 0 : alpha > 255 ? 255 : alpha);
	sprite.setColor(sf::Color(255, 235, 235, a));
	w->draw(sprite);
}

// ------------------------------ Crawler -------------------------------
Crawler::Crawler() {
	tex.loadFromFile(asset("graphics/hollow_child.png"));
	sprite.setTexture(tex);
	sprite.setOrigin(0, 24);
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
	if (stun > 0) { stun -= dt; frozen = true; return; }
	frozen = watched(p, facing);
	if (frozen) return;
	sf::Vector2f d = p - pos;
	float l = len(d); if (l < 1) return;
	float step = 150.f * dt;   // faster than you - never turn your back for long
	// Axis-separated movement so it slides along walls.
	int dirX = d.x > 0 ? 2 : 1, dirY = d.y > 0 ? 0 : 3;
	if (std::fabs(d.x) > 2 && !col.isCollisionMap(pos, dirX)) pos.x += (d.x > 0 ? 1 : -1) * step * std::fabs(d.x) / l;
	if (std::fabs(d.y) > 2 && !col.isCollisionMap(pos, dirY)) pos.y += (d.y > 0 ? 1 : -1) * step * std::fabs(d.y) / l;
	frameT += dt;
	if (frameT > 0.09f) { frameT = 0; frame = (frame + 1) % 4; }
}
void Crawler::draw(sf::RenderWindow* w) {
	if (!active) return;
	sprite.setTextureRect(sf::IntRect(frame * 24, 0, 24, 48));
	sprite.setPosition(pos);
	sprite.setColor(stun > 0 ? sf::Color(255, 120, 120, 200) : sf::Color::White);
	w->draw(sprite);
}

// ----------------------------- Apparition -----------------------------
Apparition::Apparition() {
	tex.loadFromFile(asset("graphics/apparitions.png"));
	sprite.setTexture(tex);
	sprite.setOrigin(12, 24);
}
void Apparition::spawn(sf::Vector2f at, int kind) {
	sprite.setTextureRect(sf::IntRect((kind % 2) * 24, 0, 24, 48));
	sprite.setPosition(at);
	life = 1.6f;
}
void Apparition::update(float dt, sf::Vector2f p) {
	if (life <= 0) return;
	life -= dt;
	// Vanishes the moment you get close.
	if (len(sprite.getPosition() - sf::Vector2f(p.x + 12, p.y)) < 60) life = 0;
}
void Apparition::draw(sf::RenderWindow* w) {
	if (life <= 0) return;
	float a = life > 1.2f ? (1.6f - life) / 0.4f : life / 1.2f;
	sprite.setColor(sf::Color(255, 255, 255, (sf::Uint8)(220 * a)));
	w->draw(sprite);
}
