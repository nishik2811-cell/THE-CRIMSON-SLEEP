// ---------------------------------------------------------------------
// Demo mode ("Watch the dream" on the title screen): the game plays itself
// from the Entrance Hall to the true ending, at normal walking speed.
//
// It plays by the real rules - it walks, faces things and uses them exactly
// like a player pressing E - and it finds its way with the project's own
// data structures:
//   * between rooms: BFS over the house GRAPH (Graph::bfsPath)
//   * inside a room: BFS over the tile grid with the QUEUE
// In demo mode the dreamer cannot be hurt, so a ghost can't end the show.
// ---------------------------------------------------------------------
#include "game.h"
#include "ds/Queue.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

static float dist(sf::Vector2f a, sf::Vector2f b) { float x = a.x - b.x, y = a.y - b.y; return std::sqrt(x * x + y * y); }

namespace {
// What finishes a step (checked against the real game state).
enum DemoDone { D_LANTERN, D_NEWSPAPER, D_RUSTY, D_DIARY, D_BRASS, D_LEVER, D_CHAPELKEY, D_HYMN, D_IRON,
	D_LOCKET, D_SWORD, D_NOTE, D_MATTOCK1, D_DIG1, D_DIG2, D_MATTOCK2, D_DIG3, D_DIG4, D_END };
// One step of the walkthrough: use `tile` (an interaction tile id) in `room`.
struct DemoGoal { int room; int tile; int done; const char* what; };

const DemoGoal GOALS[] = {
	{ ENTRANCE_HALL,     11, D_LANTERN,   "take the lantern" },
	{ GRAND_CORRIDOR,     0, D_NEWSPAPER, "read the newspaper in the drawer" },
	{ LIBRARY,            0, D_RUSTY,     "find the rusty key" },
	{ LIBRARY,           10, D_DIARY,     "read the diary" },
	{ COLLAPSED_BEDROOM,  1, D_BRASS,     "open the chest" },
	{ COLLAPSED_BEDROOM, 17, D_LEVER,     "pull the bedroom lever (a shorter way out)" },
	{ NURSERY,           17, D_LEVER,     "pull the nursery lever" },
	{ NURSERY,            1, D_CHAPELKEY, "take the Chapel Key" },
	{ CHAPEL,            17, D_LEVER,     "pull the chapel lever" },
	{ CHAPEL,             3, D_HYMN,      "read the hymn page" },
	{ CHAPEL,            18, D_IRON,      "take the Iron Key" },
	{ OSSUARY,           12, D_LOCKET,    "take the bride's locket" },
	{ ANTECHAMBER,       16, D_SWORD,     "take the Blessed Sword" },
	{ ANTECHAMBER,       17, D_LEVER,     "pull the antechamber lever" },
	{ CRIMSON_CRYPT,     19, D_NOTE,      "read the note (it opens the way)" },
	{ CRIMSON_CRYPT,     17, D_LEVER,     "pull the crypt lever" },
	{ CRIMSON_CRYPT,     -1, D_END,       "slay the Devil with the Blessed Sword" },   // tile -1: hunt the boss
};
const int GOAL_COUNT = (int)(sizeof GOALS / sizeof GOALS[0]);
const float READ_PAUSE = 2.5f;   // seconds to stand and let the story text be read
}

void Game::startDemo() {
	demo = true;
	demoGoal = 0;
	demoWait = 1.5f;                      // a breath before the first step
	for (int i = 0; i < TIP_COUNT; ++i) tipSeen[i] = true;   // no pop-ups mid-show
	player.lockInput = true;              // the keyboard doesn't steer the dreamer
	player.autoDir = -1;
}

bool Game::demoGoalDone(int g) {
	int c, r;
	switch (GOALS[g].done) {
	case D_LANTERN: return player.has(LANTERN);
	case D_NEWSPAPER: return collected(NEWSPAPER);
	case D_RUSTY: return collected(RUSTY_KEY);
	case D_DIARY: return collected(DIARY);
	case D_BRASS: return collected(BRASS_KEY);
	case D_LEVER: return !maps[GOALS[g].room].map.find(17, &c, &r);   // a pulled lever is a different tile
	case D_NOTE: return !maps[GOALS[g].room].map.find(19, &c, &r);            // a read note crumbles away
	case D_CHAPELKEY: return collected(CHAPEL_KEY);
	case D_HYMN: { const Edge* e = house.edgeTo(CHAPEL, HIDDEN_TUNNEL); return collected(HYMN_PAGE) && e && !e->hidden; }
	case D_IRON: return collected(IRON_KEY);
	case D_LOCKET: return player.has(CRIMSON_LOCKET);
	case D_SWORD: return player.has(SWORD);
	case D_MATTOCK1: return player.is_mattock || digCount >= 1;
	case D_DIG1: return digCount >= 1;
	case D_DIG2: return digCount >= 2;
	case D_MATTOCK2: return player.is_mattock || digCount >= 3;
	case D_DIG3: return digCount >= 3;
	case D_DIG4: return digCount >= 4;
	default: return false;   // the last step ends the game
	}
}

// Tiles at or next to anything that can hurt you right now (ghosts, Carrie,
// Samara, the Devil).  The demo plans around them and waits for them to pass.
bool Game::demoDanger(int c, int r) {
	sf::Vector2f pts[12]; int n = 0;
	for (int i = 0; i < easyCount; ++i) pts[n++] = ghostEasy[i].characterImage.getPosition() + sf::Vector2f(12, 12);
	for (int i = 0; i < mediumCount; ++i) pts[n++] = ghostMedium[i].characterImage.getPosition() + sf::Vector2f(12, 12);
	if (child.active && child.stun <= 0 && childBanishT <= 0) pts[n++] = child.pos;
	if (bride.active && bride.visible() && !bride.pacified) pts[n++] = bride.pos;
	if (bossActive && bossStun <= 0) pts[n++] = boss.characterImage.getPosition() + sf::Vector2f(12, 12);
	// (Spiders aren't avoided: they only bite and run, so the demo flares them instead of waiting.)
	for (int i = 0; i < n; ++i) {
		int gc = (int)(pts[i].x / 32), gr = (int)(pts[i].y / 32);
		if (std::abs(gc - c) <= 1 && std::abs(gr - r) <= 1) return true;
	}
	return false;
}

// Meeting the dead, each by its own rule from the survival notes.  Returns
// true while an encounter needs the dreamer to stop and deal with it.
bool Game::demoEncounter(float dt) {
	sf::Vector2f me = pp() + sf::Vector2f(12, 12);
	// A spider at the dreamer's feet: STOMP it.
	for (int i = 0; i < spiderCount; ++i)
		if (spiders[i].active && spiders[i].state == Spider::ATTACK && dist(spiders[i].pos, me) < STOMP_REACH) { stomp(); break; }
	bool canFlare = flareCooldown <= 0 && player.has(LANTERN);
	int was = demoEnc;
	demoEnc = 0;

	// Carrie: turn and LOOK at her - she freezes while watched - then flare her away.
	if (child.active && childBanishT <= 0 && child.stun <= 0 && child.distanceTo(pp()) < 180) {
		demoEnc = 1;
		sf::Vector2f d = child.pos - me;
		player.source.y = std::fabs(d.x) > std::fabs(d.y) ? (d.x > 0 ? Character::Right : Character::Left)
		                                                  : (d.y > 0 ? Character::Down : Character::Up);
		if (was != 1) { demoEncT = 0; showToast("Carrie! Face her - she can't move while you watch her."); }
		demoEncT += dt;
		float d2 = child.distanceTo(pp());
		if (demoEncT > 1.5f && d2 < 120 && canFlare) flareLantern();
		else if (demoEncT > 1.5f && d2 >= 110 && !player.col.isCollisionMap(pp(), player.source.y))
			player.forcedDir = player.source.y;      // step toward her, eyes locked - she stays frozen
		if (demoEncT > 12.f) { childBanishT = 0.01f; child.active = false; }   // never stall the show
		return true;
	}
	// Samara: she hunts by sound - stand perfectly still until she loses the trail.
	if (bride.active && bride.state == Bride::HUNT && !bride.pacified && bride.distanceTo(pp()) < 210) {
		demoEnc = 2;
		if (was != 2) { demoEncT = 0; showToast("Samara... stand still. She hunts by sound."); }
		demoEncT += dt;
		if (demoEncT > 6.f && canFlare && bride.distanceTo(pp()) < 125) flareLantern();   // she won't leave: drive her off
		return true;
	}
	// Wandering spirits and the Devil: a flare stuns them when they get close.
	if (canFlare) {
		bool near = bossActive && bossStun <= 0 && dist(boss.characterImage.getPosition(), pp()) < 120;
		for (int i = 0; i < easyCount && !near; ++i) near = ghostStun <= 0 && dist(ghostEasy[i].characterImage.getPosition(), pp()) < 75;
		for (int i = 0; i < mediumCount && !near; ++i) near = ghostStun <= 0 && dist(ghostMedium[i].characterImage.getPosition(), pp()) < 75;
		if (near) { flareLantern(); showToast("A flare - the dead recoil from the light."); }
	}
	return false;
}

static bool walkable(const TileGrid& m, int c, int r) {
	if (!m.inside(c, r)) return false;
	sf::Vector2i t = m.at(c, r);
	return !m.solid(c, r) && !(t.x == 0 && t.y == 0);
}

// BFS over the room's floor tiles (QUEUE) from the player's tile to any floor
// tile next to a tile with interaction id `tile`.  Fills demoPath (next tile
// first) and demoFace (the direction to face at the end).  False = no way.
bool Game::demoPlan(int tile, bool avoid) {
	const TileGrid& m = maps[room].map;
	sf::Vector2f p = pp();
	int sc = (int)((p.x + 12) / 32), sr = (int)((p.y + 12) / 32);
	static int parent[GRID_H * GRID_W];
	for (int i = 0; i < GRID_H * GRID_W; ++i) parent[i] = -2;
	const int dc[4] = { 0, -1, 1, 0 }, dr[4] = { 1, 0, 0, -1 };   // Down, Left, Right, Up (Character order)
	Queue<int> q;
	q.enqueue(sr * GRID_W + sc);
	parent[sr * GRID_W + sc] = -1;
	int goal = -1, face = 0, cur;
	while (q.dequeue(&cur)) {
		int c = cur % GRID_W, r = cur / GRID_W;
		for (int d = 0; d < 4 && goal < 0; ++d) {
			sf::Vector2i t = m.at(c + dc[d], r + dr[d]);
			bool hitTarget = tile >= 0 ? (t.y == 1 && t.x == tile) : (r + dr[d]) * GRID_W + (c + dc[d]) == demoTargetIdx;
			if (m.inside(c + dc[d], r + dr[d]) && hitTarget) { goal = cur; face = d; }
		}
		if (goal >= 0) break;
		for (int d = 0; d < 4; ++d) {
			int nc = c + dc[d], nr = r + dr[d];
			if (!walkable(m, nc, nr) || parent[nr * GRID_W + nc] != -2) continue;
			if (avoid && demoDanger(nc, nr)) continue;
			parent[nr * GRID_W + nc] = cur;
			q.enqueue(nr * GRID_W + nc);
		}
	}
	if (goal < 0) return false;
	demoPathLen = 0;
	int rev[GRID_H * GRID_W]; int n = 0;
	for (int i = goal; i != -1 && n < GRID_H * GRID_W; i = parent[i]) rev[n++] = i;
	for (int i = n - 1; i >= 0 && demoPathLen < DEMO_MAX_PATH; --i) demoPath[demoPathLen++] = rev[i];
	demoStep = 0;
	demoFace = face;
	return true;
}

// Called every frame in demo mode, before the player moves.
void Game::demoUpdate(float dt) {
	player.forcedDir = -1;
	// The dreamer takes hits like a player (the flash, the health bar) but can't die:
	// the show always reaches the end.
	if (player.health < 20) player.health = 20;
	if (tipOpen >= 0) tipOpen = -1;
	if (journalPage >= 0) journalPage = -1;
	if (!dialogue.empty()) { demoTalkT += dt; if (demoTalkT > 4.5f) { dialogue = ""; demoTalkT = 0; } }
	else demoTalkT = 0;
	if (demoEncounter(dt)) return;            // a horror is close: deal with it first

	if (demoWait > 0) { demoWait -= dt; return; }
	if (demoGoal >= GOAL_COUNT) return;
	if (demoGoalDone(demoGoal)) {
		if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO done %d: %s\n", demoGoal, GOALS[demoGoal].what);
		++demoGoal; demoPathLen = 0; demoTries = 0;
		demoWait = READ_PAUSE;
		return;
	}
	const DemoGoal& g = GOALS[demoGoal];

	// Wrong room: take the next door on the shortest usable route (graph BFS).
	int tile = g.tile;
	if (room != g.room) {
		int route[MAX_ROOMS];
		int n = house.bfsPath(room, g.room, route, MAX_ROOMS, [&](const Edge& e) { return edgeUsable(e); });
		if (n < 2) { if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO no route %d -> %d\n", room, g.room); demoWait = 1.f; return; }
		const Edge* e = house.edgeTo(room, route[1]);
		tile = 3 + e->doorNo;
	}
	demoReplanT += dt;
	// The Devil: close in and strike with the Blessed Sword (SHIFT) until he falls.
	if (g.tile < 0) {
		if (!bossActive) return;                    // he's collapsing - the ending follows
		sf::Vector2f bc = boss.characterImage.getPosition() + sf::Vector2f(12, 0);
		sf::Vector2f me = pp() + sf::Vector2f(12, 12), dv = bc - me;
		if (std::sqrt(dv.x * dv.x + dv.y * dv.y) < 46) {
			player.source.y = std::fabs(dv.x) > std::fabs(dv.y) ? (dv.x > 0 ? Character::Right : Character::Left)
			                                                     : (dv.y > 0 ? Character::Down : Character::Up);
			swingSword();
			demoPathLen = 0;
			return;
		}
		int bt = (int)(bc.y / 32) * GRID_W + (int)(bc.x / 32);
		if (bt != demoTargetIdx) { demoTargetIdx = bt; demoPathLen = 0; }   // he moved: chase his new tile
	}

	// Re-plan around moving horrors - but only while standing exactly on a tile,
	// so a new plan never starts from the tile behind (no stepping back and forth).
	sf::Vector2f cur = pp();
	bool onTile = std::fabs(std::fmod(cur.x - 4.f, 32.f)) < 0.5f && std::fabs(std::fmod(cur.y - 4.f, 32.f)) < 0.5f;
	if (demoRoomPlanned != room || demoTilePlanned != tile || demoPathLen == 0 || (demoReplanT > 0.35f && onTile)) {
		demoRoomPlanned = room; demoTilePlanned = tile; demoReplanT = 0;
		// Plan around the dead first; if they block every way, plan straight
		// and wait for them to move (below).
		if ((g.tile < 0 || !demoPlan(tile, true)) && !demoPlan(tile, false)) {
			if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO can't reach tile %d in room %d\n", tile, room);
			demoWait = 1.f; demoPathLen = 0;
			if (++demoTries > 5) { ++demoGoal; demoTries = 0; }   // never get stuck forever
			return;
		}
	}

	// Walk the path one tile at a time, lining up exactly on each tile.
	if (demoStep < demoPathLen) {
		int t = demoPath[demoStep];
		sf::Vector2f target((t % GRID_W) * 32.f + 4, (t / GRID_W) * 32.f + 4);
		sf::Vector2f p = pp();
		float dx = target.x - p.x, dy = target.y - p.y;
		if (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f) { player.setPosition(target); ++demoStep; return; }
		// Something deadly on the next tile: hold still and let it pass; if it
		// lingers, flare the lantern at it; after a while just go.
		bool startTile = demoStep == 0;
		if (!startTile && g.tile >= 0 && demoDanger(t % GRID_W, t / GRID_W) && demoBlockedT < 5.f) {
			demoBlockedT += dt;
			if (demoBlockedT > 2.f && flareCooldown <= 0 && player.has(LANTERN)) flareLantern();
			return;
		}
		demoBlockedT = 0;
		if (std::fabs(dx) > 8 * 32 || std::fabs(dy) > 8 * 32) { demoPathLen = 0; return; }   // pushed away: re-plan
		if (std::fabs(dx) >= 0.5f) player.forcedDir = dx > 0 ? Character::Right : Character::Left;
		else player.forcedDir = dy > 0 ? Character::Down : Character::Up;
		return;
	}

	// Arrived: face the thing and use it (or, on cracked ground, stand still and dig).
	player.source.y = demoFace;
	if (g.tile < 0) { demoPathLen = 0; return; }   // next to the Devil: the strike happens above
	interact();
	demoPathLen = 0;
	demoWait = 0.6f;
	if (++demoTries > 6) { if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO giving up on %d\n", demoGoal); ++demoGoal; demoTries = 0; }
}
