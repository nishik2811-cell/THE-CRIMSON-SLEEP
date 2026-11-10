#include "globals.h"

TileGrid* current_map = nullptr;
int game_state = ST_BOOT;
int ending_type = END_NONE;
std::string dialogue = "";

static bool g_hits[sf::Keyboard::KeyCount];

bool key_hit(sf::Keyboard::Key k) { return k >= 0 && k < sf::Keyboard::KeyCount && g_hits[k]; }
void key_reset() { for (int i = 0; i < sf::Keyboard::KeyCount; ++i) g_hits[i] = false; }
void key_record(sf::Keyboard::Key k) { if (k >= 0 && k < sf::Keyboard::KeyCount) g_hits[k] = true; }

std::string asset(const std::string& rel) { return "assets/" + rel; }
