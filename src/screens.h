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
	int credits(sf::RenderWindow* w, float dt);          // rolling credits over static
	void startCredits() { crT = 0; crStarted = false; }
	bool hasSave = false;                                // shows "Continue" on the title
	int retry(sf::RenderWindow* w, float dt, const Game& g);
	int ending(sf::RenderWindow* w, float dt, const RunStats& s);
	void resetEnding() { endT = 0; }
	void startDeath() { deathT = 0; }
	float deathTime() const { return deathT; }
	int menuIndex = 0;
	int demoChoice = 0;                                  // which ending "Watch the dream" plays
	bool watchMenu = false;                              // the Watch-the-dream sub-menu is open

private:
	struct Slide { const char* line1; const char* line2; float seconds; bool logo; };
	Queue<Slide> slides;                                  // boot sequence (FIFO)
	float slideT = 0, t = 0, endT = 0;
	// Boot -> title hand-over: the logo glides from the centre to its menu
	// spot while the menu fades in (-1 = no hand-over running).
	float introT = -1, introFromY = 130, introFromA = 1;
	float menuAlpha = 1; bool menuInput = true;
	float logoWidth(float h) const { return hasLogo ? h * logoTex.getSize().x / logoTex.getSize().y : 380.f; }
	sf::Font fontTitle, fontSerif, fontItalic;
	sf::Texture jumpTex;
	sf::Sprite jump{ jumpTex };
	float deathT = 10.f;   // time since you died (the jump scare plays first)
	sf::Texture bgTex, logoTex, floorTex;
	sf::Texture samTex; sf::Sprite sam{ samTex };          // Samara, at peace, on the true ending
	sf::Texture dawnTex; sf::Sprite dawnSprite{ dawnTex }; bool hasDawn = false;   // the true ending's sunrise
	sf::Sprite bg{ bgTex }, logo{ logoTex };
	bool hasLogo = false;
	// credits: a horror sting, then a song that builds as the static fades
	sf::Music crHorror, crSong, crStatic;
	float crT = 0; bool crStarted = false;
	sf::Image noiseImg{ sf::Vector2u(256, 192) };
	sf::Texture noiseTex{ sf::Vector2u(256, 192) };
	sf::Sprite noise{ noiseTex };
	void stopCredits();
	struct Mote { float x, y, vx, vy, r, a; };
	Mote motes[70];
	struct Soul { float x, y, phase, speed; };
	Soul souls[10];

	void drawBackdrop(sf::RenderWindow* w, sf::Color tint = sf::Color::White);
	void drawMotes(sf::RenderWindow* w, float dt, sf::Color c, bool rising);
	void drawLogo(sf::RenderWindow* w, float x, float y, float height, float alpha);
	int menu(sf::RenderWindow* w, const char* const* items, int n, float x, float y, float size);
	void centered(sf::RenderWindow* w, const sf::Font& f, const Str& s, unsigned size, sf::Color c, float y, float cx = 512);
};
