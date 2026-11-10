#include "npc.h"
#include "globals.h"
Npc::Npc(){
	cTexture.loadFromFile(asset("graphics/servent.png"));
	characterImage.setTexture(cTexture);
	characterImage.setOrigin(0, 24);
}
void Npc::movement(Character* player, sf::RenderWindow* window){

	if (isCollisionCharacter(player) && state == standing){
		if (key_hit(sf::Keyboard::Return) || key_hit(sf::Keyboard::E)){
			clock1.restart();
			state = counting;
			dialogue = "Alfred:\n\tYou came... good.\n\tThe house sleeps in crimson since 1953.\n\tYou are our only hope to stop the Devil.\n\tBe careful !!";
		}
	}
	if (state == counting){
	if (clock1.getElapsedTime().asSeconds() > 6.0f)
		dialogue = "Alfred:\n\tTake the lantern. The lines were cut.\n\tThe dark here... it watches.";
		if (clock1.getElapsedTime().asSeconds() > 10){
			dialogue = "Alfred:\n\tIts getting dark. I should leave.\n\n\n\n\n\n\t\t\t\t\t*Press spacebar to dismiss*";
			state = moving;
		}
	}
	if (state == moving){
		if (characterImage.getPosition().x > 480 && characterImage.getPosition().y < 600){
			source.y = Left;
			characterImage.move(-velocity, 0);
		}
		else if (characterImage.getPosition().y < 645){
			source.y = Down;
			characterImage.move(0, velocity);
		}
		frameCounter += frameSpeed * clock.restart().asSeconds();
		if (frameCounter > switchFrame){
			frameCounter = 0;
			source.x++;
			if (source.x * 24 >= cTexture.getSize().x)
				source.x = 0;
		}
	}
	characterImage.setTextureRect(sf::IntRect(source.x * 24, source.y * 48, 24, 48));
	(*window).draw(characterImage);
}
void Npc::initialize(){
	characterSpeed = 0.50;
	velocity = characterSpeed;
	source = sf::Vector2i(0, Down);
}