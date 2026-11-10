#include "player.h"
#include "globals.h"

Player::Player() {
	cTexture.loadFromFile(asset("graphics/player.png"));
	characterImage.setTexture(cTexture);
	characterImage.setOrigin(0, 24);
	characterSpeed = 1.0f;
	source = sf::Vector2i(0, Up);
	view.zoom(0.5f);

	soundbuffer_door.loadFromFile(asset("audio/open_door.wav"));
	door_sound.setBuffer(soundbuffer_door);
	soundbuffer_object.loadFromFile(asset("audio/sfx_pick.flac"));
	object_sound.setBuffer(soundbuffer_object);
	soundbuffer_dead.loadFromFile(asset("audio/dead.ogg"));
	dead_sound.setBuffer(soundbuffer_dead);
	soundbuffer_end.loadFromFile(asset("audio/end.ogg"));
	end_sound.setBuffer(soundbuffer_end);
	soundbuffer_beep.loadFromFile(asset("audio/beep.wav"));
	beep_sound.setBuffer(soundbuffer_beep);
}

bool Player::has(int id) { return inventory.find([id](const InventoryItem& it) { return it.id == id; }) != nullptr; }
void Player::give(int id) { if (!has(id)) inventory.insertFront(InventoryItem{ id }); }
bool Player::take(int id) { return inventory.removeIf([id](const InventoryItem& it) { return it.id == id; }); }

bool Player::isMoving() const {
	using K = sf::Keyboard;
	return !frozen && (K::isKeyPressed(K::Down) || K::isKeyPressed(K::Up) || K::isKeyPressed(K::Left) || K::isKeyPressed(K::Right) ||
		K::isKeyPressed(K::W) || K::isKeyPressed(K::A) || K::isKeyPressed(K::S) || K::isKeyPressed(K::D));
}

void Player::movement(sf::RenderWindow* window) {
	using K = sf::Keyboard;
	if (!frozen) {
		if (K::isKeyPressed(K::Down) || K::isKeyPressed(K::S)) {
			source.y = Down;
			if (!col.isCollisionMap(characterImage.getPosition(), source.y)) characterImage.move(0, characterSpeed);
		}
		else if (K::isKeyPressed(K::Left) || K::isKeyPressed(K::A)) {
			source.y = Left;
			if (!col.isCollisionMap(characterImage.getPosition(), source.y)) characterImage.move(-characterSpeed, 0);
		}
		else if (K::isKeyPressed(K::Right) || K::isKeyPressed(K::D)) {
			source.y = Right;
			if (!col.isCollisionMap(characterImage.getPosition(), source.y)) characterImage.move(characterSpeed, 0);
		}
		else if (K::isKeyPressed(K::Up) || K::isKeyPressed(K::W)) {
			source.y = Up;
			if (!col.isCollisionMap(characterImage.getPosition(), source.y)) characterImage.move(0, -characterSpeed);
		}
	}
	if (isMoving()) animate(); else clock.restart();

	scale_x = (4.f / 3 * window->getSize().y / window->getSize().x);
	if (scale_x > 1.f) scale_x = 1.f;
	view.setViewport(sf::FloatRect((1 - scale_x) / 2, 0, scale_x, 1));
	view.setCenter(characterImage.getPosition());
	window->setView(view);
	characterImage.setTextureRect(sf::IntRect(source.x * 24, source.y * 48, 24, 48));
	// Blink while invulnerable after being hurt.
	bool blink = invulnerable > 0 && ((int)(invulnerable * 12) % 2 == 0);
	if (!blink) window->draw(characterImage);
	if (hasLight()) light.lighteffect(characterImage.getPosition(), window);
}
