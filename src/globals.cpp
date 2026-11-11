#include "globals.h"

TileGrid* current_map = nullptr;
int game_state = ST_BOOT;
int ending_type = END_NONE;
std::string dialogue = "";

static bool g_hits[sf::Keyboard::KeyCount];

static int idx(sf::Keyboard::Key k) { return static_cast<int>(k); }
bool key_hit(sf::Keyboard::Key k) { int i = idx(k); return i >= 0 && i < (int)sf::Keyboard::KeyCount && g_hits[i]; }
bool key_any() { for (int i = 0; i < (int)sf::Keyboard::KeyCount; ++i) if (g_hits[i]) return true; return false; }
void key_reset() { for (int i = 0; i < (int)sf::Keyboard::KeyCount; ++i) g_hits[i] = false; }
void key_record(sf::Keyboard::Key k) { int i = idx(k); if (i >= 0 && i < (int)sf::Keyboard::KeyCount) g_hits[i] = true; }
static bool g_held[sf::Keyboard::KeyCount] = {};
bool key_held(sf::Keyboard::Key k) { int i = idx(k); return i >= 0 && i < (int)sf::Keyboard::KeyCount && g_held[i]; }
void key_set_held(sf::Keyboard::Key k, bool down) { int i = idx(k); if (i >= 0 && i < (int)sf::Keyboard::KeyCount) g_held[i] = down; }
void key_release_all() { for (int i = 0; i < (int)sf::Keyboard::KeyCount; ++i) g_held[i] = false; }

std::string asset(const std::string& rel) { return "assets/" + rel; }

float g_consoleScroll = 0, g_consoleContentH = 0;
int g_lastLookup = -1;
bool g_diag = false;

float g_gameFrac = 1.f;
sf::View uiView(const sf::RenderTarget&) {
	sf::View v(sf::FloatRect({ 0.f, 0.f }, { 1024.f, 768.f }));
	v.setViewport(sf::FloatRect({ 0.f, 0.f }, { g_gameFrac, 1.f }));
	return v;
}
