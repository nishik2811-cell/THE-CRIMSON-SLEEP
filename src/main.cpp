// The Crimson Sleep - a 2D pixelated horror escape (SFML 3).
// The Crimson Sleep (SFML 2D Horror RPG)
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "globals.h"
#include "game.h"
#include "screens.h"
#include "audio.h"

#define GAME_WIDTH 1024u
#define GAME_HEIGHT 768u

// Small speaker icon in the bottom-right corner: click it (or press M) to mute.
// The corner buttons slide left with the console panel so they stay visible.
static float g_uiShift = 0;
static sf::FloatRect muteButton() { return sf::FloatRect({ 1024.f - 46.f - g_uiShift, 768.f - 46.f }, { 34.f, 34.f }); }
static void drawMuteButton(sf::RenderWindow& w, bool muted) {
	sf::View keep = w.getView();
	w.setView(w.getDefaultView());
	sf::FloatRect r = muteButton();
	sf::Vector2f mouse = w.mapPixelToCoords(sf::Mouse::getPosition(w));
	bool hover = r.contains(mouse);
	sf::RectangleShape bg(r.size); bg.setPosition(r.position);
	bg.setFillColor(sf::Color(10, 2, 4, hover ? 220 : 150)); bg.setOutlineColor(sf::Color(120, 6, 18)); bg.setOutlineThickness(1);
	w.draw(bg);
	sf::Color ink = muted ? sf::Color(150, 140, 136) : sf::Color(236, 224, 204);
	float x = r.position.x, y = r.position.y;
	sf::RectangleShape box({ 6.f, 10.f }); box.setPosition({ x + 7, y + 12 }); box.setFillColor(ink); w.draw(box);
	sf::ConvexShape cone(4);
	cone.setPoint(0, { x + 13, y + 12 }); cone.setPoint(1, { x + 20, y + 6 }); cone.setPoint(2, { x + 20, y + 28 }); cone.setPoint(3, { x + 13, y + 22 });
	cone.setFillColor(ink); w.draw(cone);
	if (muted) {
		sf::RectangleShape a({ 11.f, 2.f }), b({ 11.f, 2.f });
		a.setOrigin({ 5.5f, 1.f }); b.setOrigin({ 5.5f, 1.f });
		a.setPosition({ x + 26, y + 17 }); b.setPosition({ x + 26, y + 17 });
		a.setRotation(sf::degrees(45)); b.setRotation(sf::degrees(-45));
		a.setFillColor(sf::Color(200, 40, 60)); b.setFillColor(sf::Color(200, 40, 60));
		w.draw(a); w.draw(b);
	}
	else {
		for (int i = 0; i < 2; ++i) {                 // two sound waves
			sf::CircleShape wave(4.f + i * 4.f, 24);
			wave.setOrigin({ wave.getRadius(), wave.getRadius() });
			wave.setPosition({ x + 20, y + 17 });
			wave.setFillColor(sf::Color::Transparent); wave.setOutlineColor(ink); wave.setOutlineThickness(1.5f);
			sf::RectangleShape mask({ 12.f, 34.f }); mask.setPosition({ x + 8, y }); mask.setFillColor(sf::Color(10, 2, 4, 255));
			w.draw(wave);
			w.draw(mask);                              // hide the left half: only the arcs show
			w.draw(cone); w.draw(box);
		}
	}
	w.setView(keep);
}

// Console button: a little "graph" icon left of the mute button, only during play.
static sf::FloatRect consoleButton() { return sf::FloatRect({ 1024.f - 86.f - g_uiShift, 768.f - 46.f }, { 34.f, 34.f }); }
static void drawConsoleButton(sf::RenderWindow& w, bool open) {
	sf::View keep = w.getView();
	w.setView(w.getDefaultView());
	sf::FloatRect r = consoleButton();
	bool hover = r.contains(w.mapPixelToCoords(sf::Mouse::getPosition(w)));
	sf::RectangleShape bg(r.size); bg.setPosition(r.position);
	bg.setFillColor(open ? sf::Color(70, 10, 22, 230) : sf::Color(10, 2, 4, hover ? 220 : 150));
	bg.setOutlineColor(sf::Color(120, 6, 18)); bg.setOutlineThickness(1);
	w.draw(bg);
	sf::Color ink = open ? sf::Color(255, 210, 200) : sf::Color(236, 224, 204);
	float x = r.position.x, y = r.position.y;
	sf::Vector2f n[4] = { { x + 9, y + 10 }, { x + 25, y + 10 }, { x + 9, y + 25 }, { x + 25, y + 25 } };   // four nodes
	sf::Vertex edges[8] = { { n[0], ink }, { n[1], ink }, { n[0], ink }, { n[2], ink }, { n[1], ink }, { n[3], ink }, { n[2], ink }, { n[1], ink } };
	w.draw(edges, 8, sf::PrimitiveType::Lines);
	for (auto& p : n) { sf::CircleShape c(3.f); c.setOrigin({ 3.f, 3.f }); c.setPosition(p); c.setFillColor(ink); w.draw(c); }
	w.setView(keep);
}

static const char* env(const char* k) { const char* v = std::getenv(k); return v && *v ? v : nullptr; }

int main() {
	std::srand((unsigned)std::time(nullptr));
	sf::RenderWindow window(sf::VideoMode({ GAME_WIDTH, GAME_HEIGHT }), "The Crimson Sleep", sf::Style::Default);
	window.setFramerateLimit(120);
	window.setKeyRepeatEnabled(false);

	Audio audio;
	Screens screens;
	Game* game = nullptr;
	int controlsReturn = ST_TITLE;

	sf::Texture floorTex;
	(void)floorTex.loadFromFile(asset("graphics/bg.png"));
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
		if (env("CS_NOTIPS")) game->devNoTips();
		if (env("CS_ALFRED")) game->devAlfredDead();
		if (env("CS_PX")) game->devPlace((float)std::atof(env("CS_PX")), (float)std::atof(env("CS_PY")));
		if (env("CS_FLARECD")) game->devFlareCooldown((float)std::atof(env("CS_FLARECD")));
		if (env("CS_WRITE")) game->devWrite(env("CS_WRITE"));
		if (env("CS_HOLD")) game->devHold(std::atoi(env("CS_HOLD")));
		if (env("CS_TALK")) game->devAlfredTalk((float)std::atof(env("CS_TALK")));
		if (env("CS_OPEN")) game->devOpen(std::atoi(env("CS_OPEN")));
	}
	if (const char* st = env("CS_STATE")) {
		if (!std::strcmp(st, "title")) game_state = ST_TITLE;
		if (!std::strcmp(st, "controls")) game_state = ST_CONTROLS;
		if (!std::strcmp(st, "pause")) { if (!game) { game = new Game(audio); } game_state = ST_PAUSED; }
		if (env("CS_MUTE")) audio.toggleMute();
		if (!std::strncmp(st, "ending", 6)) { ending_type = std::atoi(st + 7); game_state = ST_ENDING; if (!game) game = new Game(audio); }
		if (!std::strcmp(st, "retry")) { if (!game) game = new Game(audio); game->deathReason = "The Devil took you."; game->deathTip = TIP_DEVIL; game_state = ST_RETRY; }
	}
	float simDt = env("CS_DT") ? (float)std::atof(env("CS_DT")) : 0.f;

	// Live data-structure console: a panel that slides in over the right of the
	// game window (TAB or the console button).  The camera and HUD shift left
	// so the hero stays in view while it is open.
	const float CONSOLE_W = 430.f;
	bool consoleOpen = env("CS_CONSOLE") != nullptr;
	float devScroll = env("CS_SCROLL") ? (float)std::atof(env("CS_SCROLL")) : 0.f;
	float consoleSlide = consoleOpen ? 1.f : 0.f;          // 0 = hidden, 1 = fully out
	auto toggleConsole = [&]() { consoleOpen = !consoleOpen; };

	sf::Clock frameClock;
	int frame = 0;
	int prevState = env("CS_STATE") && !std::strcmp(env("CS_STATE"), "retry") ? -1 : game_state;
	while (window.isOpen()) {
		int stateAtFrameStart = game_state;
		key_reset();
		while (const std::optional<sf::Event> event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) window.close();
			else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
				key_record(key->code);
				if (key->code == sf::Keyboard::Key::Tab) toggleConsole();
				if (game && consoleOpen && key->code == sf::Keyboard::Key::PageDown) game->scrollConsole(300.f);
				if (game && consoleOpen && key->code == sf::Keyboard::Key::PageUp) game->scrollConsole(-300.f);
				if (key->code == sf::Keyboard::Key::M) audio.toggleMute();
				if (key->code == sf::Keyboard::Key::Escape && game_state == ST_PLAYING) { game_state = ST_PAUSED; key_reset(); }
			}
			else if (const auto* ws = event->getIf<sf::Event::MouseWheelScrolled>()) {
				sf::Vector2f p = window.mapPixelToCoords(ws->position, window.getDefaultView());
				if (game && consoleSlide > 0.5f && p.x > 1024.f - CONSOLE_W * consoleSlide) game->scrollConsole(-ws->delta * 48.f);
			}
			else if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
				sf::Vector2f p = window.mapPixelToCoords(mb->position, window.getDefaultView());
				if (mb->button == sf::Mouse::Button::Left && muteButton().contains(p)) audio.toggleMute();
				bool inGame = game_state == ST_PLAYING || game_state == ST_PAUSED;
				if (mb->button == sf::Mouse::Button::Left && inGame && consoleButton().contains(p)) toggleConsole();
			}
			else if (event->is<sf::Event::FocusLost>() && game_state == ST_PLAYING && shotFrame < 0) game_state = ST_PAUSED;
		}
		if (!window.isOpen()) break;
		float dt = frameClock.restart().asSeconds();
		if (dt > 0.1f) dt = 0.1f;
		if (simDt > 0) dt = simDt;
		// Slide the console panel (only while actually playing).
		{
			bool show = consoleOpen && game && game_state == ST_PLAYING;
			float target = show ? 1.f : 0.f;
			consoleSlide += (target - consoleSlide) * std::min(1.f, dt * 12.f);
			if (std::abs(target - consoleSlide) < 0.002f) consoleSlide = target;
		}
		float consolePx = CONSOLE_W * consoleSlide;
		g_uiShift = consolePx;
		if (game) game->setConsoleWidth(consolePx);
		if (game && devScroll > 0 && frame > 2) { game->scrollConsole(devScroll); devScroll = 0; }
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
			if (controlsReturn == ST_PAUSED) audio.setPaused(true);   // keep the pause music when opened from pause
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
			audio.setPaused(false);
			audio.music(game->musicTrack());
			audio.pauseAll(false);
			window.draw(floor);
			game->update(&window, dt);
			if (game_state == ST_ENDING) screens.resetEnding();
			break;
		case ST_PAUSED:
			audio.setPaused(true);         // the pause menu has its own music; the game's resumes where it was
			game_state = screens.pause(&window, dt);
			if (game_state == ST_CONTROLS) controlsReturn = ST_PAUSED;
			break;
		case ST_RETRY:
			if (prevState != ST_RETRY) { screens.startDeath(); audio.play(SFX_JUMPSCARE, 100); }
			audio.music(TRACK_DEATH);
			game_state = screens.retry(&window, dt, *game);
			if (game_state == ST_PLAYING) game->retryRoom();
			break;
		case ST_ENDING:
			audio.music(ending_type == END_TRUE ? TRACK_END_TRUE : ending_type == END_TRAPPED ? TRACK_END_TRAPPED : TRACK_END_OTHER);
			game_state = screens.ending(&window, dt, game ? game->stats() : RunStats());
			break;
		default:
			game_state = ST_TITLE;
			break;
		}
		prevState = stateAtFrameStart;   // the state this frame ran, so "just died" is detected once
		if (!window.isOpen()) break;
		if (consolePx > 0.5f && game) game->drawConsole(&window, 1024.f - consolePx, CONSOLE_W);
		window.setView(window.getDefaultView());
		drawMuteButton(window, audio.isMuted());
		if (game_state == ST_PLAYING || game_state == ST_PAUSED) drawConsoleButton(window, consoleOpen);
		window.display();
		if (++frame == shotFrame && shotPath) {
			sf::Texture cap(window.getSize());
			cap.update(window);
			(void)cap.copyToImage().saveToFile(shotPath);
			if (game && env("CS_PRINTPOS")) std::fprintf(stderr, "POS %.1f %.1f HP %.0f\n", game->devPos().x, game->devPos().y, game->devHealth());
			window.close();
		}
	}
	delete game;
	return 0;
}
