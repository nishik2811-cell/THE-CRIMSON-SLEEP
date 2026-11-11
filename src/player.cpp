#include "player.h"
#include "globals.h"

static bool down(sf::Keyboard::Key k) { return key_held(k); }

Player::Player() : object_sound(soundbuffer_object), door_sound(soundbuffer_door), end_sound(soundbuffer_end) {
	loadSprite(asset("graphics/player.png"));
	characterSpeed = 1.0f;
	source = sf::Vector2i(0, Up);
	view.zoom(0.5f);

	if (soundbuffer_door.loadFromFile(asset("audio/door_open_1.ogg"))) door_sound.setBuffer(soundbuffer_door);
	if (soundbuffer_object.loadFromFile(asset("audio/pickup_1.ogg"))) object_sound.setBuffer(soundbuffer_object);
	if (soundbuffer_dead.loadFromFile(asset("audio/death.ogg"))) dead_sound.setBuffer(soundbuffer_dead);
	if (soundbuffer_end.loadFromFile(asset("audio/sting_2.ogg"))) end_sound.setBuffer(soundbuffer_end);
	if (soundbuffer_beep.loadFromFile(asset("audio/beep.wav"))) beep_sound.setBuffer(soundbuffer_beep);
}

bool Player::has(int id) { return inventory.find([id](const InventoryItem& it) { return it.id == id; }) != nullptr; }
void Player::give(int id) { if (!has(id)) inventory.insertFront(InventoryItem{ id }); }
bool Player::take(int id) { return inventory.removeIf([id](const InventoryItem& it) { return it.id == id; }); }

static sf::Vector2f dirVec(int d) {
	return d == Character::Down ? sf::Vector2f(0, 1) : d == Character::Left ? sf::Vector2f(-1, 0) :
		d == Character::Right ? sf::Vector2f(1, 0) : sf::Vector2f(0, -1);
}

// Moves one step in `dir`.  If the way is blocked but an opening is just a
// little to one side (the 24px feet vs 32px corridors problem), the player
// is slid sideways into line with it - "corner sliding".  It only ever slides
// toward open floor, never into a wall, pit or fire (those are all solid).
void Player::tryMove(int dir) {
	source.y = dir;
	sf::Vector2f p = characterImage.getPosition(), v = dirVec(dir);
	if (!col.isCollisionMap(p, dir)) { characterImage.move(v * characterSpeed); return; }
	const int SLIDE = 14;                                      // how far off-line we still help
	int sideA = (dir == Down || dir == Up) ? Left : Up;
	int sideB = (dir == Down || dir == Up) ? Right : Down;
	for (int o = 1; o <= SLIDE; ++o) {
		for (int side : { sideA, sideB }) {
			sf::Vector2f s = dirVec(side);
			bool clear = true;                                 // can we actually get there sideways?
			for (int k = 0; k < o && clear; ++k)
				if (col.isCollisionMap(p + s * (float)k, side)) clear = false;
			if (clear && !col.isCollisionMap(p + s * (float)o, dir)) {
				characterImage.move(s * characterSpeed);       // ease into line with the opening
				sliding = true;
				return;
			}
		}
	}
}

bool Player::isMoving() const {
	if (forcedDir >= 0) return !frozen;   // demo / test walking counts as moving
	if (lockInput) return !frozen && autoDir >= 0;
	return !frozen && (down(Key::Down) || down(Key::Up) || down(Key::Left) || down(Key::Right) ||
		down(Key::W) || down(Key::A) || down(Key::S) || down(Key::D));
}

void Player::movement(sf::RenderWindow* window) {
	sliding = false;
	if (!frozen && forcedDir >= 0) tryMove(forcedDir);
	else if (!frozen && lockInput) { if (autoDir >= 0) tryMove(autoDir); }
	else if (!frozen) {
		if (down(Key::Down) || down(Key::S)) tryMove(Down);
		else if (down(Key::Left) || down(Key::A)) tryMove(Left);
		else if (down(Key::Right) || down(Key::D)) tryMove(Right);
		else if (down(Key::Up) || down(Key::W)) tryMove(Up);
	}
	if (isMoving()) animate(); else clock.restart();

	scale_x = g_gameVP.size.x;                 // the 4:3 game area (see layoutScreen)
	view.setViewport(g_gameVP);
	view.setCenter(characterImage.getPosition());
	window->setView(view);
	setFrame();
	// Blink while invulnerable after being hurt.
	bool blink = invulnerable > 0 && ((int)(invulnerable * 12) % 2 == 0);
	if (!blink) window->draw(characterImage);
	if (hasLight()) light.lighteffect(characterImage.getPosition(), window);
}
