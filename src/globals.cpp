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

std::string asset(const std::string& rel) { return "assets/" + rel; }
