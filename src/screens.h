#pragma once
#include <SFML/Graphics.hpp>
#include "globals.h"
#include "game.h"
#include "ds/Queue.h"

// Boot page, title menu, controls, pause, death and ending screens.
class Screens {
public:
	Screens();
	void startBoot();
	int boot(sf::RenderWindow* w, float dt);            // returns next state
	int title(sf::RenderWindow* w, float dt);
	int controls(sf::RenderWindow* w, float dt);
	int pause(sf::RenderWindow* w, float dt);
	int retry(sf::RenderWindow* w, float dt, const Game& g);
	int ending(sf::RenderWindow* w, float dt, const RunStats& s);
	void resetEnding() { endT = 0; }
	void startDeath() { deathT = 0; }
	int menuIndex = 0;

private:
	struct Slide { const char* line1; const char* line2; float seconds; bool logo; };
	Queue<Slide> slides;                                  // boot sequence (FIFO)
	float slideT = 0, t = 0, endT = 0;
	sf::Font fontTitle, fontSerif, fontItalic;
	sf::Texture jumpTex;
	sf::Sprite jump{ jumpTex };
	float deathT = 10.f;   // time since you died (the jump scare plays first)
	sf::Texture bgTex, logoTex, floorTex;
	sf::Sprite bg{ bgTex }, logo{ logoTex };
	bool hasLogo = false;
	struct Mote { float x, y, vx, vy, r, a; };
	Mote motes[70];
	struct Soul { float x, y, phase, speed; };
	Soul souls[10];

	void drawBackdrop(sf::RenderWindow* w, sf::Color tint = sf::Color::White);
	void drawMotes(sf::RenderWindow* w, float dt, sf::Color c, bool rising);
	void drawLogo(sf::RenderWindow* w, float x, float y, float height, float alpha);
	int menu(sf::RenderWindow* w, const char* const* items, int n, float x, float y, float size);
	void centered(sf::RenderWindow* w, const sf::Font& f, const std::string& s, unsigned size, sf::Color c, float y, float cx = 512);
};
