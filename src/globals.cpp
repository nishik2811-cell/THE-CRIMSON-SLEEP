#include "globals.h"
#include "ds/MinMax.h"

TileGrid* current_map = nullptr;
int game_state = ST_BOOT;
int ending_type = END_NONE;
Str dialogue = "";

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

const char* asset(const Str& rel) {
	static Str ring[8];
	static int next = 0;
	Str& slot = ring[next];
	next = (next + 1) % 8;
	slot = "assets/" + rel;
	return slot.c_str();
}

float g_consoleScroll = 0, g_consoleContentH = 0;
bool g_consoleClick = false;
sf::Vector2i g_consoleClickPx;
int g_lastLookup = -1;
bool g_diag = false;

sf::FloatRect g_gameVP({ 0.f, 0.f }, { 1.f, 1.f }), g_panelVP({ 1.f, 0.f }, { 0.f, 1.f });
float g_gameAreaPx = 1024.f;
void layoutScreen(sf::Vector2u win, bool consoleOpen) {
	float W = (float)maxOf(1u, win.x), H = (float)maxOf(1u, win.y);
	float panel = consoleOpen ? minOf(CONSOLE_PANEL_W * H / 768.f, W * 0.5f) : 0.f;   // undistorted strip
	float gameW = W - panel;
	float vw = minOf(gameW, H * 4.f / 3.f), vh = vw * 3.f / 4.f;                      // 4:3, letterboxed
	g_gameVP = sf::FloatRect({ (gameW - vw) / 2 / W, (H - vh) / 2 / H }, { vw / W, vh / H });
	g_panelVP = sf::FloatRect({ gameW / W, 0.f }, { panel / W, 1.f });
	g_gameAreaPx = gameW;
}
sf::View uiView(const sf::RenderTarget&) {
	sf::View v(sf::FloatRect({ 0.f, 0.f }, { 1024.f, 768.f }));
	v.setViewport(g_gameVP);
	return v;
}
