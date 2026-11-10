#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "globals.h"
#include "world.h"
#include "player.h"
#include "npc.h"
#include "enemy_easy.h"
#include "enemy_medium.h"
#include "enemy_boss.h"
#include "horror.h"
#include "audio.h"
#include "map_loader.h"
#include "ds/Graph.h"
#include "ds/BST.h"
#include "ds/Queue.h"
#include "ds/PriorityQueue.h"

// Active danger, ranked in the Priority Queue (higher = more urgent).
enum ThreatKind { TH_CALM, TH_DARK, TH_HAZARD, TH_GHOST, TH_BRIDE_NEAR, TH_CHILD, TH_DEVIL, TH_BRIDE_ATTACK };
struct Threat {
	int priority;
	int kind;
	const char* label;
};

// Haunted event waiting in the FIFO event Queue.
enum EventKind { EV_FLICKER, EV_WHISPER, EV_SLAM, EV_BREATH, EV_BLOODWRITE, EV_MOAN, EV_CRACK, EV_GLASS, EV_KIND_COUNT };
struct HauntEvent {
	float at;      // seconds after entering the room
	int kind;
};

// Bestiary / survival tips shown the first time you meet each horror.
enum TipId { TIP_SPIRITS, TIP_BRIDE, TIP_CHILD, TIP_DEVIL, TIP_HAZARDS, TIP_FEAR, TIP_COUNT };
struct TipInfo { const char* title; const char* body; };
extern const TipInfo TIPS[TIP_COUNT];

struct RunStats {
	float time = 0;
	int deaths = 0;
	int banished = 0;
	int roomsSeen = 0;
	float peakFear = 0;
	int itemsFound = 0;
};

class Game {
public:
	explicit Game(Audio& audio);
	void update(sf::RenderWindow* window, float dt);
	void retryRoom();             // after death: wake up at the room's entrance
	const RunStats& stats() const { return runStats; }
	Track musicTrack() const { return chaseHold > 0 ? TRACK_CHASE : TRACK_GAME; }
	std::string deathReason;
	int deathTip = TIP_FEAR;

	// --- dev / smoke-test hooks ---
	void devStartAt(int room);
	void devForceHint() { forceHint = true; }
	void devSpawnBride();
	void devHold(int dir) { player.forcedDir = dir; }
	sf::Vector2f devPos() const { return pp(); }
	void devPlace(float x, float y) { player.setPosition((int)x, (int)y); }
	void devAlfredTalk(float t) { npc.state = Npc::counting; alfredT = t; }
	void devAlfredDead() { alfredDead = 1; alfredLeft = true; }
	void devNoTips() { for (int i = 0; i < TIP_COUNT; ++i) tipSeen[i] = true; dialogue = ""; }
	void devJumpscare() { jumpscare = 1.3f; }
	void devOpen(int what) { if (what == 1) showDebug = true; if (what == 2) showInventory = true; if (what == 3) openTip(TIP_BRIDE, true); }

private:
	Audio& audio;
	Player player;
	Npc npc;
	Enemy_easy ghostEasy[3];
	Enemy_medium ghostMedium[3];
	Enemy_boss boss;
	Bride bride;
	Crawler child;

	sf::Texture tileset;
	Map_loader maps[ROOM_COUNT];          // ARRAY of room maps, indexed by RoomId
	Graph house;                          // GRAPH (adjacency list) of rooms
	BST<ItemRecord> itemDB;               // BST item / clue database
	Queue<HauntEvent> events;             // QUEUE of upcoming haunted events
	PriorityQueue<Threat> threats;        // PRIORITY QUEUE of active threats

	int room = ENTRANCE_HALL;
	sf::Vector2i entryTile;
	float roomTime = 0;
	bool roomVisited[ROOM_COUNT] = {};
	int easyCount = 0, mediumCount = 0;
	bool bossActive = false;
	float ghostStun = 0, bossStun = 0;

	// threat / fear
	Threat topThreat{ 0, TH_CALM, "" };
	float heartbeatT = 0;
	float stepT = 0, brideVoiceT = 3, childVoiceT = 2, devilVoiceT = 4;
	int lastBrideState = 0;
	void updateVoices(float dt);
	float hazardPushT = 0, lastDt = 0;   // forgiving hazards: how long you've pushed into one
	float childBanishT = 0;   // a flare banishes the crawler for this long
	bool childRoom = false;
	float chaseHold = 0;      // chase music keeps playing this long after the threat
	// Alfred the butler: talks, hands over the lantern, leaves... and comes back wrong.
	float alfredT = -1;       // seconds into his talk (-1 = not talking)
	bool alfredLeft = false;
	float alfredScreamT = -1; // countdown to his scream after he leaves
	int alfredDead = 0;       // 0 not yet, 1 waiting in the Library, 2 turning, 3 gone
	float alfredDeadT = 0;
	sf::Texture alfredDeadTex; sf::Sprite alfredDeadSprite{ alfredDeadTex };
	bool updateAlfred(float dt, sf::RenderWindow* window, bool overlay);
	void drawDeadAlfred(sf::RenderWindow* window, float dt, bool overlay);
	// events / scares
	std::string eventCaption; float eventCaptionT = 0;
	std::string wallText; sf::Vector2f wallPos; float wallT = 0;
	float shake = 0, jumpscare = 0, flare = 0, flareCooldown = 0, hurtFlash = 0;
	// digging (crypt)
	int digCount = 0; float digT = 0;
	// hints
	int objectiveKey = -1;
	float objectiveTime = 0;
	bool forceHint = false;
	std::string hintText, objectiveText;
	int targetRoom = 0, targetTile = -1;
	int trail[GRID_W * GRID_H]; int trailLen = 0; float trailT = 0;
	int lastRoute[MAX_ROOMS]; int lastRouteLen = 0;
	int lastDfs[MAX_ROOMS]; int lastDfsLen = 0;
	// UI
	bool showDebug = false, showInventory = false;
	int tipOpen = -1; bool tipSeen[TIP_COUNT] = {};
	int journalPage = -1;
	std::string toast; float toastT = 0;
	sf::Font fontTitle, fontSerif, fontItalic, fontMono;
	sf::Texture jumpTex; sf::Sprite jumpSprite{ jumpTex };
	sf::Texture decalTex; sf::Sprite decalSprite{ decalTex };   // blood decals (hint trail)
	RunStats runStats;

	// flow
	void enterRoom(int to, int from, bool pushHistory);
	void spawnEnemies();
	sf::Vector2i spawnNearDoor(int inRoom, int fromRoom);
	void placePlayer(sf::Vector2i tile);
	// interaction
	void interact();
	void tryDoor(int doorNo);
	void goBack();
	void pickUp(int item, const std::string& msg, bool clearTile);
	void changeFacingTile(sf::Vector2i v);
	bool edgeUsable(const Edge& e);
	void flareLantern();
	void finish(int ending);
	// systems
	void updateThreats(float dt);
	void updateEvents(float dt);
	void updateHazards();
	void updateDig(float dt);
	void updateHints(float dt);
	void computeTrail();
	int trailDistance() const;
	void hurt(float health, float fear, const std::string& why, int tip, bool respawn);
	void die(const std::string& why, int tip);
	void openTip(int tip, bool force = false);
	void say(const std::string& s) { dialogue = s; }
	void showToast(const std::string& s) { toast = s; toastT = 3.f; }
	// drawing
	void drawWorldOverlays(sf::RenderWindow* w);
	void drawHud(sf::RenderWindow* w);
	void drawDebug(sf::RenderWindow* w);
	void drawInventory(sf::RenderWindow* w);
	void drawTip(sf::RenderWindow* w, int tip);
	void drawJournal(sf::RenderWindow* w);
	void drawMinimap(sf::RenderWindow* w);
	sf::Vector2f pp() const { return player.characterImage.getPosition(); }
	bool collected(int item) const { const ItemRecord* r = itemDB.search(item); return r && r->collected; }
	bool discovered(int item) const { const ItemRecord* r = itemDB.search(item); return r && r->discovered; }
};
