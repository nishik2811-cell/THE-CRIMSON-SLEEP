#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "collision.h"

class Character {
public:
	enum Direction { Down, Left, Right, Up };
	float frameCounter = 0, frameSpeed = 1000, switchFrame = 100;
	sf::Texture cTexture;
	sf::Clock clock;
	sf::SoundBuffer soundbuffer_beep, soundbuffer_dead;
	sf::Sound beep_sound, dead_sound;

	sf::Vector2i source;
	sf::Sprite characterImage;
	float characterSpeed = 1.f;
	Collision col;
	virtual ~Character() {}
	void setPosition(int, int);
	virtual void movement(sf::RenderWindow*);
	virtual void movement(Character*, sf::RenderWindow*);
	int isCollisionCharacter(Character*);
	void animate();
};
