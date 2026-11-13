// ---------------------------------------------------------------------
// Demo mode ("Watch the dream" on the title screen): the game plays itself
// from the Entrance Hall to the true ending, at normal walking speed.
//
// It plays by the real rules - it walks, faces things and uses them exactly
// like a player pressing E - and it finds its way with the project's own
// data structures:
//   * between rooms: BFS over the house GRAPH (Graph::bfsPath)
//   * inside a room: BFS over the tile grid with the QUEUE
// The dreamer plays by the same rules as you: it takes real damage, heals
// with the health kits hidden in drawers, and if it does die it wakes again
// and carries on, like a player choosing "Dream again".
// ---------------------------------------------------------------------
#include "game.h"
#include "ds/Queue.h"
#include "ai.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

static float dist(sf::Vector2f a, sf::Vector2f b) { float x = a.x - b.x, y = a.y - b.y; return std::sqrt(x * x + y * y); }

namespace {
// What finishes a step (checked against the real game state).
enum DemoDone { D_LANTERN, D_NEWSPAPER, D_RUSTY, D_DIARY, D_BRASS, D_LEVER, D_CHAPELKEY, D_HYMN, D_IRON,
	D_LOCKET, D_RETURN, D_SWORD, D_DEVIL, D_NOTE, D_MATTOCK1, D_DIG1, D_DIG2, D_MATTOCK2, D_DIG3, D_DIG4, D_END };
// One step of the walkthrough: use `tile` (an interaction tile id) in `room`.
struct DemoGoal { int room; int tile; int done; const char* what; };

// One walkthrough per ending ("Watch the dream" offers all four).
const DemoGoal GOALS_TRUE[] = {
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
	{ OSSUARY,           -2, D_RETURN,    "give Samara back her locket" },          // tile -2: walk up to Samara
	{ ANTECHAMBER,       16, D_SWORD,     "take the Blessed Sword" },
	{ ANTECHAMBER,       17, D_LEVER,     "pull the antechamber lever" },
	{ CRIMSON_CRYPT,     19, D_NOTE,      "read the note (it opens the way)" },
	{ CRIMSON_CRYPT,     17, D_LEVER,     "pull the crypt lever" },
	{ CRIMSON_CRYPT,     -1, D_END,       "slay the Devil with the Blessed Sword" },   // tile -1: hunt the boss
};
// Escape: the Devil dies, but Samara never gets her locket back.
const DemoGoal GOALS_ESCAPE[] = {
	{ ENTRANCE_HALL,     11, D_LANTERN,   "take the lantern" },
	{ GRAND_CORRIDOR,     0, D_NEWSPAPER, "read the newspaper in the drawer" },
	{ LIBRARY,            0, D_RUSTY,     "find the rusty key" },
	{ COLLAPSED_BEDROOM,  1, D_BRASS,     "open the chest" },
	{ COLLAPSED_BEDROOM, 17, D_LEVER,     "pull the bedroom lever (a shorter way out)" },
	{ NURSERY,           17, D_LEVER,     "pull the nursery lever" },
	{ NURSERY,            1, D_CHAPELKEY, "take the Chapel Key" },
	{ CHAPEL,            17, D_LEVER,     "pull the chapel lever" },
	{ CHAPEL,            18, D_IRON,      "take the Iron Key" },
	{ ANTECHAMBER,       16, D_SWORD,     "take the Blessed Sword" },
	{ ANTECHAMBER,       17, D_LEVER,     "pull the antechamber lever" },
	{ CRIMSON_CRYPT,     19, D_NOTE,      "read the note (it opens the way)" },
	{ CRIMSON_CRYPT,     17, D_LEVER,     "pull the crypt lever" },
	{ CRIMSON_CRYPT,     -1, D_DEVIL,     "slay the Devil (the locket never returned)" },
	{ OUTSIDE,            0, D_END,       "walk out of the front door" },
};
// Secret: find the locket, keep it, and walk out of the front door.
const DemoGoal GOALS_SECRET[] = {
	{ ENTRANCE_HALL,     11, D_LANTERN,   "take the lantern" },
	{ GRAND_CORRIDOR,     0, D_NEWSPAPER, "read the newspaper in the drawer" },
	{ LIBRARY,            0, D_RUSTY,     "find the rusty key" },
	{ COLLAPSED_BEDROOM,  1, D_BRASS,     "open the chest" },
	{ COLLAPSED_BEDROOM, 17, D_LEVER,     "pull the bedroom lever (a shorter way out)" },
	{ NURSERY,           17, D_LEVER,     "pull the nursery lever" },
	{ NURSERY,            1, D_CHAPELKEY, "take the Chapel Key" },
	{ CHAPEL,            17, D_LEVER,     "pull the chapel lever" },
	{ CHAPEL,             3, D_HYMN,      "read the hymn page" },
	{ OSSUARY,           12, D_LOCKET,    "take the bride's locket - and keep it" },
	{ OUTSIDE,            0, D_END,       "walk out of the front door with the locket" },   // the route ends at the front door
};
// Trapped: take the petrol in the Antechamber instead of the sword.
const DemoGoal GOALS_TRAPPED[] = {
	{ ENTRANCE_HALL,     11, D_LANTERN,   "take the lantern" },
	{ GRAND_CORRIDOR,     0, D_NEWSPAPER, "read the newspaper in the drawer" },
	{ LIBRARY,            0, D_RUSTY,     "find the rusty key" },
	{ COLLAPSED_BEDROOM,  1, D_BRASS,     "open the chest" },
	{ COLLAPSED_BEDROOM, 17, D_LEVER,     "pull the bedroom lever (a shorter way out)" },
	{ NURSERY,           17, D_LEVER,     "pull the nursery lever" },
	{ NURSERY,            1, D_CHAPELKEY, "take the Chapel Key" },
	{ CHAPEL,            17, D_LEVER,     "pull the chapel lever" },
	{ CHAPEL,            18, D_IRON,      "take the Iron Key" },
	{ ANTECHAMBER,       20, D_END,       "pour the petrol and strike a match" },
};
const DemoGoal* const ALL_GOALS[] = { GOALS_TRUE, GOALS_ESCAPE, GOALS_SECRET, GOALS_TRAPPED };
const int ALL_COUNTS[] = { (int)(sizeof GOALS_TRUE / sizeof GOALS_TRUE[0]), (int)(sizeof GOALS_ESCAPE / sizeof GOALS_ESCAPE[0]),
                           (int)(sizeof GOALS_SECRET / sizeof GOALS_SECRET[0]), (int)(sizeof GOALS_TRAPPED / sizeof GOALS_TRAPPED[0]) };
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
	switch (ALL_GOALS[demoRoute][g].done) {
	case D_LANTERN: return player.has(LANTERN);
	case D_NEWSPAPER: return collected(NEWSPAPER);
	case D_RUSTY: return collected(RUSTY_KEY);
	case D_DIARY: return collected(DIARY);
	case D_BRASS: return collected(BRASS_KEY);
	case D_LEVER: return !maps[ALL_GOALS[demoRoute][g].room].map.find(17, &c, &r);   // a pulled lever is a different tile
	case D_NOTE: return !maps[ALL_GOALS[demoRoute][g].room].map.find(19, &c, &r);            // a read note crumbles away
	case D_CHAPELKEY: return collected(CHAPEL_KEY);
	case D_HYMN: { const Edge* e = house.edgeTo(CHAPEL, HIDDEN_TUNNEL); return collected(HYMN_PAGE) && e && !e->hidden; }
	case D_IRON: return collected(IRON_KEY);
	case D_LOCKET: return player.has(CRIMSON_LOCKET) || locketReturned;
	case D_RETURN: return locketReturned;
	case D_DEVIL: return bossHP <= 0 && bossDeathT < 0;
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
	// A spirit frozen by a flare can't touch you: walk straight past it.
	for (int i = 0; i < easyCount && ghostStun <= 0; ++i) pts[n++] = ghostEasy[i].characterImage.getPosition() + sf::Vector2f(12, 12);
	for (int i = 0; i < mediumCount && ghostStun <= 0; ++i) pts[n++] = ghostMedium[i].characterImage.getPosition() + sf::Vector2f(12, 12);
	if (child.active && child.stun <= 0 && childBanishT <= 0) pts[n++] = child.pos;
	if (bride.active && bride.visible() && !bride.pacified) pts[n++] = bride.pos;
	if (bossActive && bossStun <= 0) pts[n++] = bossCentre();
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
	// A spider charging while the stomp is still recharging: the flare drives it off.
	if (canFlare && stompT > 0)
		for (int i = 0; i < spiderCount; ++i)
			if (spiders[i].active && (spiders[i].state == Spider::CHASE || spiders[i].state == Spider::ATTACK) && dist(spiders[i].pos, me) < 100) {
				flareLantern(); canFlare = false; break;
			}
	// Fear kills too: when its nerve is going, a flare steadies it (fear -20).
	if (canFlare && player.fear > 70) { flareLantern(); showToast("A flare to steady the nerves."); canFlare = false; }

	// A wandering spirit about to walk into you: flare it (it freezes and you
	// walk on past), or if the flare is still recharging, step out of its way.
	// Mid-dodge: finish the sidestep, then wait there for the spirit to pass
	// (no turning back and forth).
	if (demoDodging) {
		demoDodgeT += dt;
		if (demoDodgeT < 0.55f) { player.forcedDir = demoDodgeDir; return true; }
		float gd = 1e9f;
		for (int i = 0; i < easyCount; ++i) { float q = dist(ghostEasy[i].characterImage.getPosition() + sf::Vector2f(12, 12), me); if (q < gd) gd = q; }
		for (int i = 0; i < mediumCount; ++i) { float q = dist(ghostMedium[i].characterImage.getPosition() + sf::Vector2f(12, 12), me); if (q < gd) gd = q; }
		if (ghostStun <= 0 && gd < 70 && gd > 44 && demoDodgeT < 3.f) return true;   // let it go by (closer than that: dodge again below)
		demoDodging = false;
	}
	if (ghostStun <= 0) {
		float gd = 1e9f; sf::Vector2f gp;
		for (int i = 0; i < easyCount; ++i) { sf::Vector2f q = ghostEasy[i].characterImage.getPosition() + sf::Vector2f(12, 12); if (dist(q, me) < gd) { gd = dist(q, me); gp = q; } }
		for (int i = 0; i < mediumCount; ++i) { sf::Vector2f q = ghostMedium[i].characterImage.getPosition() + sf::Vector2f(12, 12); if (dist(q, me) < gd) { gd = dist(q, me); gp = q; } }
		if (gd < 80) {
			if (canFlare) { flareLantern(); showToast("A flare - the spirit freezes in the light."); return false; }
			if (gd < 64 && demoDodge(gp)) return true;
		}
	}

	// Carrie: turn and LOOK at her - she freezes while watched - then flare her
	// away.  Only when she can actually be seen (no staring at a wall), and
	// only briefly: if she's out of flare reach it walks on, glancing back.
	if (demoCarrieCD > 0) demoCarrieCD -= dt;
	float cd = child.distanceTo(pp());
	bool seeCarrie = child.active && childBanishT <= 0 && child.stun <= 0 && cd < 170 &&
	                 (ai::lineOfSight(maps[room].map, me, child.pos) || cd < 100);   // in view, or close enough to see her coming round the corner
	if (seeCarrie && cd < 60 && canFlare) { flareLantern(); demoCarrieCD = 4.f; return false; }   // she's almost on you: flare at once
	if (seeCarrie && demoCarrieCD <= 0) {
		demoEnc = 1;
		sf::Vector2f d = child.pos - me;
		player.source.y = std::fabs(d.x) > std::fabs(d.y) ? (d.x > 0 ? Character::Right : Character::Left)
		                                                  : (d.y > 0 ? Character::Down : Character::Up);
		if (was != 1) { demoEncT = 0; showToast("Carrie! Face her - she can't move while you watch her."); }
		demoEncT += dt;
		if (demoEncT > 1.0f && cd < 125 && canFlare) { flareLantern(); demoCarrieCD = 4.f; return false; }
		if (cd < 110 && flareCooldown < 8.5f) return true;   // she's close: keep her frozen with your eyes till the lantern is ready
		if (demoEncT > 2.0f) { demoCarrieCD = 5.f; return false; }   // no flare for a while and she's further off: carry on
		return true;
	}
	// Samara: she hunts by sound.  The flare drives her off the moment she's
	// in reach; otherwise the dreamer keeps walking, and only freezes (so she
	// loses the trail) if she gets right on top of it - one smooth stop, not
	// a stop-start crawl.  Standing in the dark too long also feeds the fear.
	float sd = bride.distanceTo(pp());
	if (bride.active && bride.state == Bride::HUNT && !bride.pacified && sd < 210) {
		if (was != 2) { demoEncT = 0; demoSamaraHold = 0; showToast("Samara... she hunts by sound."); }
		demoEnc = 2;
		demoEncT += dt;
		if (canFlare && sd < 125) { flareLantern(); return false; }       // drive her off
		if (sd > 85 && sd < 160 && demoSamaraHold < Bride::LOSE_AFTER + 0.5f) { demoSamaraHold += dt; return true; }   // freeze till she loses the trail
		return false;                                                    // keep going
	}
	return false;
}

// Step one tile away from a threat at `from` (the free side that gains the
// most distance).  False if every side is a wall.
bool Game::demoDodge(sf::Vector2f from) {
	const TileGrid& m = maps[room].map;
	sf::Vector2f me = pp() + sf::Vector2f(12, 12);
	int c = (int)(me.x / 32), r = (int)(me.y / 32);
	const int dc[4] = { 0, -1, 1, 0 }, dr[4] = { 1, 0, 0, -1 };
	// Spirits walk straight lines, so stepping ACROSS their line is what saves you.
	sf::Vector2f away = me - from;
	bool acrossVertical = std::fabs(away.x) >= std::fabs(away.y);   // it comes along a row: dodge up/down
	int best = -1; float bestD = -1e9f;
	for (int d = 0; d < 4; ++d) {
		int nc = c + dc[d], nr = r + dr[d];
		if (!m.inside(nc, nr) || m.solid(nc, nr)) continue;
		sf::Vector2i t = m.at(nc, nr);
		if (t.y == 1 && (t.x == 14 || t.x == 21 || t.x == 22)) continue;   // never dodge into a pit, spikes or fire
		if (player.col.isCollisionMap(pp(), d)) continue;
		float dd = dist(sf::Vector2f(nc * 32.f + 16, nr * 32.f + 16), from);
		bool across = acrossVertical ? (d == 0 || d == 3) : (d == 1 || d == 2);
		float score = dd + (across ? 40.f : 0.f);
		if (bossActive) {                               // in the crypt, dodge round the grave, not off into a corner
			sf::Vector2f ac = arenaCentre();
			score += (dist(me, ac) - dist(sf::Vector2f(nc * 32.f + 16, nr * 32.f + 16), ac)) * 1.5f;
		}
		if (dd > dist(me, from) - 2 && score > bestD) { bestD = score; best = d; }
	}
	if (best < 0) return false;
	player.forcedDir = best;
	demoDodgeDir = best; demoDodgeT = 0; demoDodging = true;   // commit: step aside for ~half a second, then hold
	demoPathLen = 0;                                // re-plan from wherever the dodge ends
	return true;
}

// The nearest unopened drawer in this room (every first search outside the
// Library and Corridor holds a health kit).  Tile index, or -1.
int Game::demoFindKit() {
	if (room == LIBRARY || room == GRAND_CORRIDOR) return -1;
	const TileGrid& m = maps[room].map;
	sf::Vector2f me = pp() + sf::Vector2f(12, 12);
	int best = -1; float bestD = 1e9f;
	for (int r = 0; r < GRID_H; ++r) for (int c = 0; c < GRID_W; ++c) {
		sf::Vector2i t = m.at(c, r);
		if (t.y != 1 || t.x != 0 || searched[room][r * GRID_W + c]) continue;
		float d = dist(me, sf::Vector2f(c * 32.f + 16, r * 32.f + 16));
		if (d < bestD) { bestD = d; best = r * GRID_W + c; }
	}
	return best;
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
	player.blocking = false;                  // the guard only stays up while the dreamer holds it (below)
	{ static int tk = 0; if (getenv("CS_TRACE") && ++tk % 10 == 0) std::fprintf(stderr, "D enc=%d gstun=%.1f wait=%.1f goal=%d path=%d/%d hp=%.0f pos=%.0f,%.0f\n", demoEnc, ghostStun, demoWait, demoGoal, demoStep, demoPathLen, player.health, pp().x, pp().y); }
	if (hidden) { hidden = false; player.hiddenAway = false; }   // the dreamer never hides (it could never step out)
	if (tipOpen >= 0) tipOpen = -1;
	if (journalPage >= 0) journalPage = -1;
	if (!dialogue.empty()) { demoTalkT += dt; if (demoTalkT > 4.5f) { dialogue = ""; demoTalkT = 0; } }
	else demoTalkT = 0;
	if (demoEncounter(dt)) return;            // a horror is close: deal with it first

	if (demoWait > 0) { demoWait -= dt; return; }
	if (demoGoal >= ALL_COUNTS[demoRoute]) return;
	if (demoGoalDone(demoGoal)) {
		if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO done %d: %s\n", demoGoal, ALL_GOALS[demoRoute][demoGoal].what);
		++demoGoal; demoPathLen = 0; demoTries = 0;
		demoWait = READ_PAUSE;
		return;
	}
	const DemoGoal& g = ALL_GOALS[demoRoute][demoGoal];

	// Hurt: search an unopened drawer in this room for a health kit first.
	int tile = g.tile;
	bool healing = false;
	if (player.health <= 55 && g.tile >= 0) {
		int kit = demoFindKit();
		if (kit >= 0) { healing = true; tile = -100 - kit; if (demoTargetIdx != kit) { demoTargetIdx = kit; demoPathLen = 0; } }
	}
	// Wrong room: take the next door on the shortest usable route (graph BFS).
	if (!healing && room != g.room) {
		int route[MAX_ROOMS];
		int n = house.bfsPath(room, g.room, route, MAX_ROOMS, [&](const Edge& e) { return edgeUsable(e); });
		if (n < 2) { if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO no route %d -> %d\n", room, g.room); demoWait = 1.f; return; }
		const Edge* e = house.edgeTo(room, route[1]);
		tile = 3 + e->doorNo;
	}
	demoReplanT += dt;
	// Samara: walk up to her and give the locket back (E), like a player would.
	if (g.tile == -2 && room == g.room) {
		if (locketFlyT >= 0 || samaraGoneT >= 0) return;   // the locket is on its way / she is rising: watch
		if (nearSamara()) {
			player.source.y = samaraPos.y > pp().y + 12 ? Character::Down : Character::Up;
			if (demoTalkT < 3.0f && !dialogue.empty()) return;        // let her words be read first
			interact(); demoWait = 1.4f; return;
		}
		if (samaraSpoke && demoTalkT < 3.0f && !dialogue.empty()) return;
		if (samaraT < SAMARA_RISEN) return;         // wait for her: the silence, the whisper, her rising
		int st = (int)(samaraPos.y / 32) * GRID_W + (int)(samaraPos.x / 32);
		if (st != demoTargetIdx) { demoTargetIdx = st; demoPathLen = 0; }
	}
	// The Devil: close in and strike with the Blessed Sword (SHIFT) until he falls.
	if (g.tile == -1) {
		if (!bossActive && bossDormant && room == g.room) {   // step into the middle of the crypt: he rises
			sf::Vector2f ac = arenaCentre() + sf::Vector2f(0, 64);
			int at = (int)(ac.y / 32) * GRID_W + (int)(ac.x / 32);
			if (at != demoTargetIdx) { demoTargetIdx = at; demoPathLen = 0; }
		}
		else if (!bossActive) return;               // he's collapsing - the ending follows
		else {
		if (bossRiseT >= 0) return;                 // let him rise
		sf::Vector2f bc = bossCentre();
		sf::Vector2f me = pp() + sf::Vector2f(12, 12), dv = bc - me;
		float bd = std::sqrt(dv.x * dv.x + dv.y * dv.y);
		// He's winding up the scythe: flare him if the lantern is ready,
		// otherwise get out of the red arc - then step back in and strike.
		if (bossAtkT >= 0 && bossAtkT < BOSS_SWING + 0.05f) {
			// He winds up: turn to face him and raise the guard (C) - the clash
			// staggers him, and that's the opening for the sword.
			if (bossAtkT < 0.05f) demoWillBlock = std::rand() % 100 < (bossAtkKind == 1 ? 60 : 80);   // quick jabs are harder to read   // like a player, it doesn't always get the guard up
			if (bd < BOSS_REACH + 14 && demoWillBlock) {
				player.source.y = std::fabs(dv.x) > std::fabs(dv.y) ? (dv.x > 0 ? Character::Right : Character::Left)
				                                                     : (dv.y > 0 ? Character::Down : Character::Up);
				player.blocking = true;
				return;
			}
			if (bd < BOSS_REACH + 14) return;            // too slow to raise the guard: it takes the cut (no backing off)
			player.source.y = std::fabs(dv.x) > std::fabs(dv.y) ? (dv.x > 0 ? Character::Right : Character::Left)
			                                                     : (dv.y > 0 ? Character::Down : Character::Up);
			return;                                 // wait at the edge of his reach for the blade to pass
		}
		if (bd < 46) {
			player.source.y = std::fabs(dv.x) > std::fabs(dv.y) ? (dv.x > 0 ? Character::Right : Character::Left)
			                                                     : (dv.y > 0 ? Character::Down : Character::Up);
			swingSword();
			demoPathLen = 0;
			return;
		}
		int bt = (int)(bc.y / 32) * GRID_W + (int)(bc.x / 32);
		if (bt != demoTargetIdx) { demoTargetIdx = bt; demoPathLen = 0; }   // he moved: chase his new tile
		}
	}

	// Re-plan around moving horrors - but only while standing exactly on a tile,
	// so a new plan never starts from the tile behind (no stepping back and forth).
	sf::Vector2f cur = pp();
	bool onTile = std::fabs(std::fmod(cur.x - 4.f, 32.f)) < 0.5f && std::fabs(std::fmod(cur.y - 4.f, 32.f)) < 0.5f;
	if (demoRoomPlanned != room || demoTilePlanned != tile || demoPathLen == 0 || (demoReplanT > 0.35f && onTile)) {
		demoRoomPlanned = room; demoTilePlanned = tile; demoReplanT = 0;
		// Plan around the dead first; if they block every way, plan straight
		// and wait for them to move (below).
		int planTile = tile < -1 ? -1 : tile;      // -1: plan to demoTargetIdx (the Devil, or a drawer)
		if ((g.tile < 0 || !demoPlan(planTile, true)) && !demoPlan(planTile, false)) {
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
		if (!startTile && (g.tile >= 0 || healing) && demoDanger(t % GRID_W, t / GRID_W) && demoBlockedT < 2.5f) {
			demoBlockedT += dt;
			if (demoBlockedT > 0.6f && flareCooldown <= 0 && player.has(LANTERN)) flareLantern();
			if (demoBlockedT > 2.0f && onTile) { demoPathLen = 0; demoBlockedT = 0; return; }   // still blocked: try a way around
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
	if (player.col.isInteractionMap(pp(), demoFace) == 23) { demoPathLen = 0; demoWait = 0.3f; return; }   // a wardrobe: not this
	if (g.tile < 0 && !healing) { demoPathLen = 0; return; }   // next to the Devil: the strike happens above
	interact();
	demoPathLen = 0;
	demoWait = 0.6f;
	if (++demoTries > 6) { if (getenv("CS_DEMO_LOG")) std::fprintf(stderr, "DEMO giving up on %d\n", demoGoal); ++demoGoal; demoTries = 0; }
}
