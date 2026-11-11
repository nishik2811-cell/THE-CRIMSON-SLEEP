#include "light.h"
#include <cmath>
#include <cstdint>

Light::Light() {
	(void)layer.resize({ 1024, 768 });
	(void)flashlightTexture.resize({ 400, 400 });
	flashlightTexture.clear(sf::Color(255, 255, 255, 255));
	// Concentric circles, each more transparent: alpha is what remains of
	// the darkness layer after the BlendMultiply below.
	for (unsigned int i = 0; i < 47; ++i) {
		sf::CircleShape circle;
		float r; int a;
		if (i < 6) { r = 200 - i * 20.f; a = 239 - (int)i; }
		else if (i < 31) { r = 106 - i * 1.f; a = 240 - (int)i * 2; }
		else if (i < 46) { r = 106 - i * 1.f; a = 516 - (int)i * 11; }
		else { r = 60; a = 10; }
		circle.setRadius(r);
		circle.setOrigin({ r, r });
		circle.setFillColor(sf::Color(255, 255, 255, (std::uint8_t)a));
		circle.setPosition({ 200.f, 200.f });
		flashlightTexture.draw(circle, sf::BlendNone);
	}
	flashlightTexture.display();
	flashlight.setTexture(flashlightTexture.getTexture(), true);
	flashlight.setOrigin({ 200.f, 200.f });
	sprite.setTexture(layer.getTexture(), true);
}

void Light::lighteffect(sf::Vector2f c, sf::RenderWindow* window) {
	float t = clock.getElapsedTime().asSeconds();
	// The lantern flame breathes slightly; haunted flickers shrink it hard.
	float s = 1.f + 0.03f * std::sin(t * 9.f) + 0.02f * std::sin(t * 23.f) - 0.45f * flicker;
	s *= dim;
	flashlight.setScale({ s, s });
	flashlight.setPosition({ c.x + 12, c.y + 12 });
	layer.clear(sf::Color(14, 0, 3, 255));   // near-black, blood-tinted dark
	layer.draw(flashlight, sf::BlendMultiply);
	layer.display();
	window->draw(sprite);
}
