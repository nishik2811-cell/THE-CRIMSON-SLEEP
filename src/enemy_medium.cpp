#include "enemy_medium.h"
#include "globals.h"

Enemy_medium::Enemy_medium() {
	loadSprite(asset("graphics/enemy_medium.png"));
	frameW = 32;                         // wider cells: room for the arm and tail
	characterImage.setOrigin({ 4.f, 24.f });
}
void Enemy_medium::movement(Character*, sf::RenderWindow* window) {
	if (characterImage.getPosition().y > 544) { source.y = Up; velocity = -velocity; }
	if (characterImage.getPosition().y < 128) { source.y = Down; velocity = -velocity; }
	characterImage.move({ 0.f, velocity });
	animate();
	setFrame();
	window->draw(characterImage);
}
void Enemy_medium::initialize() {
	characterSpeed = 0.60f;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Down);
}
