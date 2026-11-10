#include "npc.h"
#include "globals.h"
#include <cmath>

Npc::Npc() {
	loadSprite(asset("graphics/servent.png"));
}

bool Npc::near(Character* player) const {
	sf::Vector2f d = player->characterImage.getPosition() - characterImage.getPosition();
	return std::sqrt(d.x * d.x + d.y * d.y) < 40.f;
}

// Talking is handled by Game (it hands over the lantern); the NPC itself
// only stands, waits while talking, then walks out of the hall.
void Npc::movement(Character*, sf::RenderWindow* window) {
	if (state == moving) {
		if (characterImage.getPosition().x > 480 && characterImage.getPosition().y < 600) {
			source.y = Left;
			characterImage.move({ -velocity, 0.f });
		}
		else if (characterImage.getPosition().y < 645) {
			source.y = Down;
			characterImage.move({ 0.f, velocity });
		}
		animate();
	}
	setFrame();
	window->draw(characterImage);
}

void Npc::initialize() {
	characterSpeed = 0.50f;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Down);
}
