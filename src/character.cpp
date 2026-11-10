#include "character.h"
#include <cmath>

void Character::loadSprite(const std::string& path) {
	if (!cTexture.loadFromFile(path)) return;
	characterImage.setTexture(cTexture, true);
	characterImage.setOrigin({ 0.f, 24.f });
}
void Character::setPosition(int x, int y) { characterImage.setPosition({ (float)x, (float)y }); }
void Character::movement(sf::RenderWindow*) {}
void Character::movement(Character*, sf::RenderWindow*) {}

int Character::isCollisionCharacter(Character* other) {
	return std::fabs(other->characterImage.getPosition().x - characterImage.getPosition().x) < 24 &&
		std::fabs(other->characterImage.getPosition().y - characterImage.getPosition().y) < 24;
}

void Character::animate() {
	frameCounter += frameSpeed * clock.restart().asSeconds();
	if (frameCounter > switchFrame) {
		frameCounter = 0;
		source.x++;
		if (source.x * frameW >= (int)cTexture.getSize().x) source.x = 0;
	}
}

void Character::setFrame() { characterImage.setTextureRect(sf::IntRect({ source.x * frameW, source.y * 48 }, { frameW, 48 })); }
