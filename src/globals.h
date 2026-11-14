#pragma once
#include <SFML/Graphics.hpp>
#include "ds/Str.h"
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
	ST_DEMO = 8,        // like NEW_GAME, but the game plays itself
	ST_SAVE_QUIT = 9,   // pause menu: save the run, then back to the title
	ST_CONTINUE = 10,   // title: load the saved run
	ST_CREDITS = 11     // title: the rolling credits
};

enum EndingType { END_NONE, END_ESCAPE, END_TRAPPED, END_SECRET, END_TRUE };

extern TileGrid* current_map;   // map of the room the player is in
extern int game_state;
extern int ending_type;
extern Str dialogue;    // story text box (Space dismisses)

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

// Resolves a path inside the assets/ folder.  Returns a C string that stays
// valid for the next few calls (a small ring of buffers) - enough to pass
// straight into SFML's loadFromFile / openFromFile.
const char* asset(const Str& rel);

// Live data-structure console state, kept outside Game so its layout is unchanged.
extern float g_consoleScroll, g_consoleContentH;   // console window scrolling
extern bool g_consoleClick;            // a left click landed on the console this frame
extern sf::Vector2i g_consoleClickPx;  // ...at this window pixel
extern int g_lastLookup;        // last item searched in the BST (lit as a path)
extern bool g_diag;             // F9: on-screen movement diagnostics

// Screen layout, recomputed every frame from the real window size (it may be
// maximised or fullscreen).  The game always keeps a 4:3 area (g_gameVP, as
// fractions of the window, letterboxed if needed); with the console open it
// gets a strip on the right (g_panelVP).  uiView() is the 1024x768 screen
// view for the game area - use it instead of getDefaultView().
const float CONSOLE_PANEL_W = 560.f;   // width of the console strip, in game pixels at 768 high
extern sf::FloatRect g_gameVP, g_panelVP;
extern float g_gameAreaPx;      // width in pixels of the part left of the console
void layoutScreen(sf::Vector2u window, bool consoleOpen);
sf::View uiView(const sf::RenderTarget& t);
// The same 1024x768 coordinates, but covering the WHOLE window (past the
// letterbox bars): for backgrounds that should fill a wide screen.
sf::View fullView(const sf::RenderTarget& t);
