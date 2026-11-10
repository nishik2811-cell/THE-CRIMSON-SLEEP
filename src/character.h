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
	int frameW = 24;                             // sprite-sheet cell width (ghosts use 32)
	Collision col;
	Character() : beep_sound(soundbuffer_beep), dead_sound(soundbuffer_dead), characterImage(cTexture) {}
	virtual ~Character() {}
	void loadSprite(const std::string& path);   // loads the sheet and binds it to the sprite
	void setPosition(int, int);
	void setPosition(sf::Vector2f p) { characterImage.setPosition(p); }
	virtual void movement(sf::RenderWindow*);
	virtual void movement(Character*, sf::RenderWindow*);
	int isCollisionCharacter(Character*);
	void animate();
	void setFrame();                             // texture rect from source (24x48 cells)
};
