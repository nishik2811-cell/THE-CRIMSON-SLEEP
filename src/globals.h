#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "ds/TileGrid.h"

// Screen / flow states (explicit, so new screens never fall through).
enum GameState {
	ST_BOOT = -2,       // studio splash + title card (boot page)
	ST_WARNING = -1,    // content warning + controls
	ST_RETRY = 0,       // "you died" - wake up / title
	ST_PLAYING = 1,
	ST_PAUSED = 2,
	ST_TITLE = 3,
	ST_QUIT = 4,
	ST_ENDING = 5,
	ST_CONTROLS = 6,
	ST_NEW_GAME = 7     // request: rebuild the Game object then play
};

enum EndingType { END_NONE, END_ESCAPE, END_TRAPPED, END_SECRET, END_TRUE };

extern TileGrid* current_map;   // map of the room the player is in
extern int game_state;
extern int ending_type;
extern std::string dialogue;    // story text box (Space dismisses)

// Edge-triggered key presses collected from the event loop this frame.
bool key_hit(sf::Keyboard::Key k);
bool key_any();
using Key = sf::Keyboard::Key;
void key_reset();
void key_record(sf::Keyboard::Key k);

// Resolves a path inside the assets/ folder.
std::string asset(const std::string& rel);
