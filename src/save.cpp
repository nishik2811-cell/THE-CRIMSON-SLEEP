// ---------------------------------------------------------------------
// Save & continue.  A plain-text save file holding everything that changes
// as you play - and it is written straight out of the data structures:
//   GRAPH  every door's locked / hidden flags      STACK  the rooms behind you
//   LINKED LIST  the inventory, in order           BST    each item's flags
//   ARRAY  every room's 32x24 tile grid (picked-up items, pulled levers, digs)
// ---------------------------------------------------------------------
#include "game.h"
#include <cstdio>
#include <cstring>

static const char* const SAVE_TAG = "CRIMSON_SLEEP_SAVE 1";

bool Game::saveExists(const char* path) {
	FILE* f = std::fopen(path, "r");
	if (!f) return false;
	char tag[64] = {};
	bool ok = std::fgets(tag, sizeof tag, f) && std::strncmp(tag, SAVE_TAG, std::strlen(SAVE_TAG)) == 0;
	std::fclose(f);
	return ok;
}

bool Game::saveGame(const char* path) {
	FILE* f = std::fopen(path, "w");
	if (!f) return false;
	sf::Vector2f p = pp();
	std::fprintf(f, "%s\n", SAVE_TAG);
	std::fprintf(f, "room %d %.1f %.1f\n", room, p.x, p.y);
	std::fprintf(f, "player %.2f %.2f %d %d %d %.2f\n", player.health, player.fear, (int)player.is_mattock, (int)lanternOn, digCount, flareCooldown);
	std::fprintf(f, "alfred %d %d %d %.1f %.1f\n", npc.state, (int)alfredLeft, alfredDead,
		npc.characterImage.getPosition().x, npc.characterImage.getPosition().y);
	std::fprintf(f, "boss %d\n", bossHP);
	std::fprintf(f, "stats %.2f %d %d %d %.2f %d\n", runStats.time, runStats.deaths, runStats.banished, runStats.roomsSeen, runStats.peakFear, runStats.itemsFound);
	// LINKED LIST: inventory front to back
	int inv[32]; int n = 0;
	player.inventory.forEach([&](const InventoryItem& it) { if (n < 32) inv[n++] = it.id; });
	std::fprintf(f, "inventory %d", n);
	for (int i = 0; i < n; ++i) std::fprintf(f, " %d", inv[i]);
	// STACK: top first
	int hist[32]; int hn = player.history.toArray(hist, 32);
	std::fprintf(f, "\nhistory %d", hn);
	for (int i = 0; i < hn; ++i) std::fprintf(f, " %d", hist[i]);
	// BST: every item's flags, in order
	std::fprintf(f, "\nitems");
	itemDB.inOrder([&](int k, const ItemRecord& r) { std::fprintf(f, " %d:%d:%d", k, (int)r.collected, (int)r.discovered); });
	// GRAPH: every edge's locked / hidden flags
	std::fprintf(f, "\nedges");
	for (int a = 1; a < ROOM_COUNT; ++a)
		for (const Edge* e = house.neighbours(a); e; e = e->next)
			std::fprintf(f, " %d:%d:%d:%d", a, e->to, (int)e->locked, (int)e->hidden);
	std::fprintf(f, "\ntips");
	for (int i = 0; i < TIP_COUNT; ++i) std::fprintf(f, " %d", (int)tipSeen[i]);
	std::fprintf(f, "\nvisited");
	for (int r = 0; r < ROOM_COUNT; ++r) std::fprintf(f, " %d", (int)roomVisited[r]);
	std::fprintf(f, "\ncorpses");
	for (int r = 0; r < ROOM_COUNT; ++r)
		for (int i = 0; i < corpseCount[r]; ++i) std::fprintf(f, " %d:%.0f:%.0f", r, corpses[r][i].x, corpses[r][i].y);
	std::fprintf(f, "\nsearched");
	for (int r = 0; r < ROOM_COUNT; ++r)
		for (int i = 0; i < GRID_W * GRID_H; ++i) if (searched[r][i]) std::fprintf(f, " %d:%d", r, i);
	// ARRAY: each room's tile grid
	for (int r = 1; r < ROOM_COUNT; ++r) {
		std::fprintf(f, "\nmap %d", r);
		for (int i = 0; i < GRID_H; ++i)
			for (int j = 0; j < GRID_W; ++j) std::fprintf(f, " %d,%d", maps[r].map.cell[i][j].x, maps[r].map.cell[i][j].y);
	}
	std::fprintf(f, "\nend\n");
	std::fclose(f);
	return true;
}

// Reads "a:b:c..." integers separated by ':' from the next token.
static int readInts(FILE* f, int* out, int max) {
	char tok[64];
	if (std::fscanf(f, "%63s", tok) != 1) return 0;
	int n = 0; const char* s = tok;
	while (n < max && *s) {
		int v = 0, sign = 1;
		if (*s == '-') { sign = -1; ++s; }
		while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
		out[n++] = v * sign;
		if (*s == ':' || *s == ',' || *s == '.') ++s; else break;
	}
	return n;
}

bool Game::loadGame(const char* path) {
	if (!saveExists(path)) return false;
	FILE* f = std::fopen(path, "r");
	char line[64];
	if (!std::fgets(line, sizeof line, f)) { std::fclose(f); return false; }
	int savedRoom = ENTRANCE_HALL; float px = 0, py = 0;
	char key[32];
	while (std::fscanf(f, "%31s", key) == 1) {
		if (!std::strcmp(key, "end")) break;
		if (!std::strcmp(key, "room")) { if (std::fscanf(f, "%d %f %f", &savedRoom, &px, &py) != 3) break; }
		else if (!std::strcmp(key, "player")) {
			int mat = 0, lit = 1;
			if (std::fscanf(f, "%f %f %d %d %d %f", &player.health, &player.fear, &mat, &lit, &digCount, &flareCooldown) != 6) break;
			player.is_mattock = mat; lanternOn = lit;
		}
		else if (!std::strcmp(key, "alfred")) {
			int left = 0; float ax = 0, ay = 0;
			if (std::fscanf(f, "%d %d %d %f %f", &npc.state, &left, &alfredDead, &ax, &ay) != 5) break;
			alfredLeft = left; npc.setPosition({ ax, ay });
		}
		else if (!std::strcmp(key, "boss")) { if (std::fscanf(f, "%d", &bossHP) != 1) break; }
		else if (!std::strcmp(key, "stats")) {
			if (std::fscanf(f, "%f %d %d %d %f %d", &runStats.time, &runStats.deaths, &runStats.banished, &runStats.roomsSeen, &runStats.peakFear, &runStats.itemsFound) != 6) break;
		}
		else if (!std::strcmp(key, "inventory")) {
			int n = 0, ids[32];
			if (std::fscanf(f, "%d", &n) != 1) break;
			for (int i = 0; i < n && i < 32; ++i) if (std::fscanf(f, "%d", &ids[i]) != 1) n = i;
			while (!player.inventory.empty()) player.inventory.removeIf([](const InventoryItem&) { return true; });
			for (int i = (n < 32 ? n : 32) - 1; i >= 0; --i) player.give(ids[i]);   // give() inserts at the front
		}
		else if (!std::strcmp(key, "history")) {
			int n = 0, ids[32];
			if (std::fscanf(f, "%d", &n) != 1) break;
			for (int i = 0; i < n && i < 32; ++i) if (std::fscanf(f, "%d", &ids[i]) != 1) n = i;
			player.history.clear();
			for (int i = (n < 32 ? n : 32) - 1; i >= 0; --i) player.history.push(ids[i]);   // bottom first
		}
		else if (!std::strcmp(key, "items")) {
			for (int i = 0; i < ITEM_COUNT - 1; ++i) {
				int v[3];
				if (readInts(f, v, 3) == 3) if (ItemRecord* r = itemDB.search(v[0])) { r->collected = v[1]; r->discovered = v[2]; }
			}
		}
		else if (!std::strcmp(key, "edges")) {
			int count = 0;
			for (int a = 1; a < ROOM_COUNT; ++a) for (const Edge* e = house.neighbours(a); e; e = e->next) ++count;
			for (int i = 0; i < count; ++i) {
				int v[4];
				if (readInts(f, v, 4) == 4) if (Edge* e = house.edgeTo(v[0], v[1])) { e->locked = v[2]; e->hidden = v[3]; }
			}
		}
		else if (!std::strcmp(key, "tips")) { for (int i = 0; i < TIP_COUNT; ++i) { int v = 0; if (std::fscanf(f, "%d", &v) == 1) tipSeen[i] = v; } }
		else if (!std::strcmp(key, "visited")) { for (int r = 0; r < ROOM_COUNT; ++r) { int v = 0; if (std::fscanf(f, "%d", &v) == 1) roomVisited[r] = v; } }
		else if (!std::strcmp(key, "corpses") || !std::strcmp(key, "searched")) {
			bool corpse = key[0] == 'c';
			int c;
			while ((c = std::fgetc(f)) == ' ') {
				int v[3]; int got = readInts(f, v, 3);
				if (corpse && got == 3 && v[0] >= 0 && v[0] < ROOM_COUNT && corpseCount[v[0]] < 4)
					corpses[v[0]][corpseCount[v[0]]++] = sf::Vector2f((float)v[1], (float)v[2]);
				if (!corpse && got == 2 && v[0] >= 0 && v[0] < ROOM_COUNT && v[1] >= 0 && v[1] < GRID_W * GRID_H) searched[v[0]][v[1]] = true;
			}
			if (c != EOF) std::ungetc(c, f);
		}
		else if (!std::strcmp(key, "map")) {
			int r = 0;
			if (std::fscanf(f, "%d", &r) != 1 || r < 1 || r >= ROOM_COUNT) break;
			for (int i = 0; i < GRID_H; ++i)
				for (int j = 0; j < GRID_W; ++j) { int v[2]; if (readInts(f, v, 2) == 2) maps[r].map.cell[i][j] = sf::Vector2i(v[0], v[1]); }
		}
	}
	std::fclose(f);
	if (savedRoom < 1 || savedRoom >= ROOM_COUNT) savedRoom = ENTRANCE_HALL;
	enterRoom(savedRoom, -1, false);
	player.setPosition({ px, py });
	dialogue = "";
	say("You wake again where you left the dream...");
	return true;
}
