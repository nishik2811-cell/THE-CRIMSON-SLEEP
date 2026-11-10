#include "enemy_easy.h"
#include "globals.h"

Enemy_easy::Enemy_easy() {
	loadSprite(asset("graphics/enemy_easy.png"));
	frameW = 32;                         // wider cells: room for the arm and tail
	characterImage.setOrigin({ 4.f, 24.f });
}
void Enemy_easy::movement(Character*, sf::RenderWindow* window) {
	if (col.isCollisionMap(characterImage.getPosition(), source.y)) {
		source.y = (source.y == Right) ? Left : Right;
		velocity = -velocity;
	}
	characterImage.move({ velocity, 0.f });
	animate();
	setFrame();
	window->draw(characterImage);
}
void Enemy_easy::initialize() {
	characterSpeed = 0.50f;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Right);
}
