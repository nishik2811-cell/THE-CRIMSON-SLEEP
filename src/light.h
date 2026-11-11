#pragma once
#include <SFML/Graphics.hpp>

// Lantern light: a dark-crimson layer with a soft hole punched around the
// player.  `flicker` (0..1) briefly dims the whole scene for haunted events.
class Light {
public:
	sf::RenderTexture flashlightTexture;
	sf::RenderTexture layer;
	sf::Sprite flashlight{ flashlightTexture.getTexture() };   // re-bound once the textures are sized
	sf::Sprite sprite{ layer.getTexture() };
	sf::Clock clock;
	float flicker = 0.f;
	float dim = 1.f;          // 1 = lantern lit; small = lantern off (only a faint glow, eyes adjusting)
	Light();
	void lighteffect(sf::Vector2f, sf::RenderWindow*);
};
