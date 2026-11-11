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
	ST_NEW_GAME = 7,    // request: rebuild the Game object then play
	ST_DEMO = 8         // like NEW_GAME, but the game plays itself
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
// Held keys, tracked from KeyPressed/KeyReleased events.  Used instead of
// sf::Keyboard::isKeyPressed, which on some Linux desktops stops reporting
// keys after the window is resized (the console opening).
bool key_held(sf::Keyboard::Key k);
void key_set_held(sf::Keyboard::Key k, bool down);
void key_release_all();

// Resolves a path inside the assets/ folder.
std::string asset(const std::string& rel);

// Live data-structure console state, kept outside Game so its layout is unchanged.
extern float g_consoleScroll, g_consoleContentH;   // console window scrolling
extern int g_lastLookup;        // last item searched in the BST (lit as a path)
extern bool g_diag;             // F9: on-screen movement diagnostics

// When the console is open the window is widened and the game keeps the left
// g_gameFrac of it, drawn exactly as at 1024x768.  uiView() is the 1024x768
// screen-space view for that area (use it instead of getDefaultView()).
extern float g_gameFrac;
const float CONSOLE_PANEL_W = 560.f;   // width of the console strip, in game pixels
sf::View uiView(const sf::RenderTarget& t);
