#pragma once
#include <SFML/Graphics.hpp>

// Lantern light: a dark-crimson layer with a soft hole punched around the
// player.  `flicker` (0..1) briefly dims the whole scene for haunted events.
class Light {
public:
	sf::RenderTexture flashlightTexture;
	sf::RenderTexture layer;
	sf::Sprite flashlight;
	sf::Sprite sprite;
	sf::Clock clock;
	float flicker = 0.f;
	Light();
	void lighteffect(sf::Vector2f, sf::RenderWindow*);
};
