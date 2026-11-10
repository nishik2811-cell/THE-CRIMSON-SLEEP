#include "player.h"
#include "globals.h"

static bool down(sf::Keyboard::Key k) { return sf::Keyboard::isKeyPressed(k); }

Player::Player() : object_sound(soundbuffer_object), door_sound(soundbuffer_door), end_sound(soundbuffer_end) {
	loadSprite(asset("graphics/player.png"));
	characterSpeed = 1.0f;
	source = sf::Vector2i(0, Up);
	view.zoom(0.5f);

	if (soundbuffer_door.loadFromFile(asset("audio/open_door.wav"))) door_sound.setBuffer(soundbuffer_door);
	if (soundbuffer_object.loadFromFile(asset("audio/sfx_pick.flac"))) object_sound.setBuffer(soundbuffer_object);
	if (soundbuffer_dead.loadFromFile(asset("audio/dead.ogg"))) dead_sound.setBuffer(soundbuffer_dead);
	if (soundbuffer_end.loadFromFile(asset("audio/end.ogg"))) end_sound.setBuffer(soundbuffer_end);
	if (soundbuffer_beep.loadFromFile(asset("audio/beep.wav"))) beep_sound.setBuffer(soundbuffer_beep);
}

bool Player::has(int id) { return inventory.find([id](const InventoryItem& it) { return it.id == id; }) != nullptr; }
void Player::give(int id) { if (!has(id)) inventory.insertFront(InventoryItem{ id }); }
bool Player::take(int id) { return inventory.removeIf([id](const InventoryItem& it) { return it.id == id; }); }

bool Player::isMoving() const {
	return !frozen && (down(Key::Down) || down(Key::Up) || down(Key::Left) || down(Key::Right) ||
		down(Key::W) || down(Key::A) || down(Key::S) || down(Key::D));
}

void Player::movement(sf::RenderWindow* window) {
	if (!frozen) {
		sf::Vector2f p = characterImage.getPosition();
		if (down(Key::Down) || down(Key::S)) {
			source.y = Down;
			if (!col.isCollisionMap(p, source.y)) characterImage.move({ 0.f, characterSpeed });
		}
		else if (down(Key::Left) || down(Key::A)) {
			source.y = Left;
			if (!col.isCollisionMap(p, source.y)) characterImage.move({ -characterSpeed, 0.f });
		}
		else if (down(Key::Right) || down(Key::D)) {
			source.y = Right;
			if (!col.isCollisionMap(p, source.y)) characterImage.move({ characterSpeed, 0.f });
		}
		else if (down(Key::Up) || down(Key::W)) {
			source.y = Up;
			if (!col.isCollisionMap(p, source.y)) characterImage.move({ 0.f, -characterSpeed });
		}
	}
	if (isMoving()) animate(); else clock.restart();

	scale_x = (4.f / 3 * window->getSize().y / window->getSize().x);
	if (scale_x > 1.f) scale_x = 1.f;
	view.setViewport(sf::FloatRect({ (1 - scale_x) / 2, 0.f }, { scale_x, 1.f }));
	view.setCenter(characterImage.getPosition());
	window->setView(view);
	setFrame();
	// Blink while invulnerable after being hurt.
	bool blink = invulnerable > 0 && ((int)(invulnerable * 12) % 2 == 0);
	if (!blink) window->draw(characterImage);
	if (hasLight()) light.lighteffect(characterImage.getPosition(), window);
}
