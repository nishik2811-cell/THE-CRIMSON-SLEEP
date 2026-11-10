#include "enemy_easy.h"
#include "globals.h"

Enemy_easy::Enemy_easy() {
	cTexture.loadFromFile(asset("graphics/enemy_easy.png"));
	characterImage.setTexture(cTexture);
	characterImage.setOrigin(0, 24);
}
void Enemy_easy::movement(Character*, sf::RenderWindow* window) {
	if (col.isCollisionMap(characterImage.getPosition(), source.y)) {
		source.y = (source.y == Right) ? Left : Right;
		velocity = -velocity;
	}
	characterImage.move(velocity, 0);
	animate();
	characterImage.setTextureRect(sf::IntRect(source.x * 24, source.y * 48, 24, 48));
	window->draw(characterImage);
}
void Enemy_easy::initialize() {
	characterSpeed = 0.50f;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Right);
}
