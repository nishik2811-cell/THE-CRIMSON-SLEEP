#include "enemy_medium.h"
#include "globals.h"

Enemy_medium::Enemy_medium() {
	cTexture.loadFromFile(asset("graphics/enemy_medium.png"));
	characterImage.setTexture(cTexture);
	characterImage.setOrigin(0, 24);
}
void Enemy_medium::movement(Character*, sf::RenderWindow* window) {
	if (characterImage.getPosition().y > 544) { source.y = Up; velocity = -velocity; }
	if (characterImage.getPosition().y < 128) { source.y = Down; velocity = -velocity; }
	characterImage.move(0, velocity);
	animate();
	characterImage.setTextureRect(sf::IntRect(source.x * 24, source.y * 48, 24, 48));
	window->draw(characterImage);
}
void Enemy_medium::initialize() {
	characterSpeed = 0.60f;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Down);
}
