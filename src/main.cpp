// The Crimson Sleep - a 2D pixelated horror escape.
// Extended from "Mansion Mystery" (SFML) by Saurabh Kumar Prasad.
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "globals.h"
#include "game.h"
#include "screens.h"
#include "audio.h"

#define GAME_WIDTH 1024
#define GAME_HEIGHT 768

static const char* env(const char* k) { const char* v = std::getenv(k); return v && *v ? v : nullptr; }

int main() {
	std::srand((unsigned)std::time(nullptr));
	sf::RenderWindow window(sf::VideoMode(GAME_WIDTH, GAME_HEIGHT), "The Crimson Sleep", sf::Style::Default);
	window.setFramerateLimit(120);
	window.setKeyRepeatEnabled(false);

	Audio audio;
	Screens screens;
	Game* game = nullptr;
	int controlsReturn = ST_TITLE;

	sf::Texture floorTex;
	floorTex.loadFromFile(asset("graphics/bg.png"));
	sf::Sprite floor(floorTex);

	// --- smoke-test hooks (used to screenshot the game headlessly) ---
	int shotFrame = env("CS_FRAMES") ? std::atoi(env("CS_FRAMES")) : -1;
	const char* shotPath = env("CS_SHOT");
	if (env("CS_ROOM")) {
		game = new Game(audio);
		game->devStartAt(std::atoi(env("CS_ROOM")));
		game_state = ST_PLAYING;
		if (env("CS_HINT")) game->devForceHint();
		if (env("CS_BRIDE")) game->devSpawnBride();
		if (env("CS_JUMP")) game->devJumpscare();
		if (env("CS_OPEN")) game->devOpen(std::atoi(env("CS_OPEN")));
	}
	if (const char* st = env("CS_STATE")) {
		if (!std::strcmp(st, "title")) game_state = ST_TITLE;
		if (!std::strcmp(st, "controls")) game_state = ST_CONTROLS;
		if (!std::strncmp(st, "ending", 6)) { ending_type = std::atoi(st + 7); game_state = ST_ENDING; if (!game) game = new Game(audio); }
		if (!std::strcmp(st, "retry")) { if (!game) game = new Game(audio); game->deathReason = "The Devil took you."; game->deathTip = TIP_DEVIL; game_state = ST_RETRY; }
	}
	float simDt = env("CS_DT") ? (float)std::atof(env("CS_DT")) : 0.f;

	sf::Clock frameClock;
	int frame = 0;
	while (window.isOpen()) {
		key_reset();
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) window.close();
			if (event.type == sf::Event::KeyPressed) {
				key_record(event.key.code);
				if (event.key.code == sf::Keyboard::Escape && game_state == ST_PLAYING) { game_state = ST_PAUSED; key_reset(); }
			}
			if (event.type == sf::Event::LostFocus && game_state == ST_PLAYING && shotFrame < 0) game_state = ST_PAUSED;
		}
		float dt = frameClock.restart().asSeconds();
		if (dt > 0.1f) dt = 0.1f;
		if (simDt > 0) dt = simDt;
		window.clear(sf::Color::Black);

		switch (game_state) {
		case ST_BOOT:
		case ST_WARNING:
			audio.music(TRACK_TITLE);
			game_state = screens.boot(&window, dt);
			break;
		case ST_TITLE:
			audio.music(TRACK_TITLE);
			if (game) { delete game; game = nullptr; }
			game_state = screens.title(&window, dt);
			if (game_state == ST_CONTROLS) controlsReturn = ST_TITLE;
			break;
		case ST_CONTROLS: {
			int r = screens.controls(&window, dt);
			if (r == -100) game_state = controlsReturn;
			break;
		}
		case ST_NEW_GAME:
			delete game;
			game = new Game(audio);
			game_state = ST_PLAYING;
			break;
		case ST_PLAYING:
			audio.music(TRACK_GAME);
			audio.pauseAll(false);
			window.draw(floor);
			game->update(&window, dt);
			if (game_state == ST_ENDING) screens.resetEnding();
			break;
		case ST_PAUSED:
			game_state = screens.pause(&window, dt);
			if (game_state == ST_CONTROLS) controlsReturn = ST_PAUSED;
			break;
		case ST_RETRY:
			game_state = screens.retry(&window, dt, *game);
			if (game_state == ST_PLAYING) game->retryRoom();
			break;
		case ST_ENDING:
			audio.music(TRACK_ENDING);
			game_state = screens.ending(&window, dt, game ? game->stats() : RunStats());
			break;
		default:
			game_state = ST_TITLE;
			break;
		}

		window.display();
		if (++frame == shotFrame && shotPath) {
			sf::Texture cap;
			cap.create(window.getSize().x, window.getSize().y);
			cap.update(window);
			cap.copyToImage().saveToFile(shotPath);
			window.close();
		}
	}
	delete game;
	return 0;
}
