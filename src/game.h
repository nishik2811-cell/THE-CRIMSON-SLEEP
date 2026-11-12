#pragma once
#include <SFML/Graphics.hpp>
#include "ds/Str.h"
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
enum ThreatKind { TH_CALM, TH_DARK, TH_HAZARD, TH_GHOST, TH_BRIDE_NEAR, TH_CHILD, TH_DEVIL, TH_BRIDE_ATTACK, TH_SPIDER };
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
enum TipId { TIP_SPIRITS, TIP_BRIDE, TIP_CHILD, TIP_DEVIL, TIP_HAZARDS, TIP_FEAR, TIP_SPIDERS, TIP_COUNT };
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
	Str deathReason;
	int deathTip = TIP_FEAR;

	// --- dev / smoke-test hooks ---
	void devStartAt(int room);
	void devForceHint() { forceHint = true; }
	void devSpawnBride();
	void devSwing() { swingSword(); }
	void devBack() { goBack(); }
	void devHide() { hidden = true; player.hiddenAway = true; }
	void devSpawnSpider() { if (spiderCount < 1) spiderCount = 1; spiders[0].reset(true, pp() + sf::Vector2f(-150.f, 12.f)); spiders[0].dir = 2; spiders[0].wait = 30; }
	void devHold(int dir) { player.forcedDir = dir; }
	sf::Vector2f devPos() const { return pp(); }
	float devHealth() const { return player.health; }
	void devPlace(float x, float y) { player.setPosition((int)x, (int)y); }
	void devAlfredTalk(float t) { npc.state = Npc::counting; alfredT = t; alfredWalkT = 0; player.lockInput = true; }
	void devAlfredDead() { alfredDead = 1; alfredLeft = true; }
	void devNoTips() { for (int i = 0; i < TIP_COUNT; ++i) tipSeen[i] = true; dialogue = ""; }
	void devJumpscare() { jumpscare = 1.3f; }
	void devFlareCooldown(float s) { flareCooldown = s; }
	void devWrite(const char* w) { Str s = w; if (!placeWallWriting(s)) { s = "GET OUT"; if (!placeWallWriting(s)) return; } wallText = s; wallT = 5.f; }
	void drawConsole(sf::RenderWindow* w);
	// Demo mode: the game plays itself to the true ending (title screen option).
	// Save & continue (save.cpp): the whole run, written out of the data structures.
	bool saveGame(const char* path);
	bool loadGame(const char* path);
	static bool saveExists(const char* path);
	void startDemo();
	void devDemoFrom(int g) { startDemo(); demoGoal = g; }
	bool demo = false;   // live data structures, in the separate console window
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
	Spider spiders[3];                    // the Weavers (creepy crawlers)
	int spiderCount = 0;
	float spiderFollowT = -1;             // >0: a spider that was chasing you comes through the door
	sf::Vector2f corpses[ROOM_COUNT][4];  // squashed spiders stay where they died
	int corpseCount[ROOM_COUNT] = {};
	sf::Texture deadTex; sf::Sprite deadSprite{ deadTex };
	bool hidden = false;                  // inside a wardrobe (stealth)
	bool lanternOn = true;                // L toggles; off = harder to see, harder to be seen
	bool searched[ROOM_COUNT][GRID_W * GRID_H] = {};   // drawers already opened (each holds one thing)
	void squash(Spider& s);
	void stomp();
	// Door transition: the door swings open, the screen fades, you arrive as the
	// door behind you swings shut.  Locked doors rattle.
	float doorT = -1, arriveT = -1, rattleT = 0;
	int doorTo = -1; bool doorPush = true; bool doorBack = false;
	sf::Vector2i doorTile{ -1, -1 }, arriveTile{ -1, -1 }, rattleTile{ -1, -1 };
	static constexpr float DOOR_OPEN = 0.45f, DOOR_FADE = 0.3f, DOOR_IN = 0.5f, WALK_OUT = 0.5f;
	sf::Vector2f walkFrom, walkTo;          // the hero walks through the doorway
	void beginDoor(int to, bool push, bool back);
	void updateDoor(float dt);
	void drawDoorTile(sf::RenderWindow* w, sf::Vector2i t, float open, float jitter, bool away = true);
	void stompLand();
	float stompT = 0, stompHitT = 0, stompFxT = 0;   // cooldown, time to the stamp frame, dust ring

	sf::Texture tileset;
	Map_loader maps[ROOM_COUNT];          // ARRAY of room maps, indexed by RoomId
	Graph house;                          // GRAPH (adjacency list) of rooms
	BST<ItemRecord> itemDB;               // BST item / clue database
	Queue<HauntEvent> events;             // QUEUE of upcoming haunted events
	PriorityQueue<Threat> threats;        // PRIORITY QUEUE of active threats

	// Demo-mode state (see demo.cpp).
	static const int DEMO_MAX_PATH = 256;
	int demoGoal = 0, demoTries = 0, demoPath[DEMO_MAX_PATH] = {}, demoPathLen = 0, demoStep = 0, demoFace = 0;
	int demoRoomPlanned = -1, demoTilePlanned = -1, demoTargetIdx = -1;
	float demoWait = 0, demoTalkT = 0, demoReplanT = 0, demoBlockedT = 0;
	void demoUpdate(float dt);
	bool demoPlan(int tile, bool avoid);
	bool demoDanger(int c, int r);
	bool demoEncounter(float dt);
	int demoEnc = 0;          // 0 none, 1 Carrie, 2 Samara
	float demoEncT = 0;
	bool demoGoalDone(int g);

	int room = ENTRANCE_HALL;
	sf::Vector2i entryTile;
	float roomTime = 0;
	bool roomVisited[ROOM_COUNT] = {};
	int easyCount = 0, mediumCount = 0;
	bool bossActive = false;
	static const int BOSS_MAX_HP = 6;
	int bossHP = BOSS_MAX_HP;             // the Devil takes six blows of the Blessed Sword
	float bossHitT = 0, bossDeathT = -1;  // red flash on a hit; >= 0 while he collapses
	float swingT = 0, swingHitT = 0;      // sword cooldown, time until the STRIKE frame lands
	void swingSword();
	void swordLand();
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
	float alfredWalkT = 0;    // how long we've been walking over to him (gives up after 3 s)
	void alfredGiveLantern();
	bool alfredLeft = false;
	float alfredScreamT = -1; // countdown to his scream after he leaves
	int alfredDead = 0;       // 0 not yet, 1 waiting in the Library, 2 turning, 3 gone
	float alfredDeadT = 0;
	sf::Texture alfredDeadTex; sf::Sprite alfredDeadSprite{ alfredDeadTex };
	bool updateAlfred(float dt, sf::RenderWindow* window, bool overlay);
	void drawDeadAlfred(sf::RenderWindow* window, float dt, bool overlay);
	// events / scares
	Str eventCaption; float eventCaptionT = 0;
	Str wallText; sf::Vector2f wallPos; float wallT = 0;
	static const unsigned WALL_TEXT_SIZE = 15;   // fits inside one 32px wall tile
	float scrawlWidth(const Str& word) const;
	bool placeWallWriting(const Str& word);
	float shake = 0, jumpscare = 0, flare = 0, flareCooldown = 0, hurtFlash = 0;
	static constexpr float FLARE_COOLDOWN = 10.f;   // seconds between lantern flares
	static constexpr float STOMP_COOLDOWN = 15.f;   // seconds before you can stomp again
	static constexpr float STOMP_REACH = 30.f;      // px from your feet - about one tile
	// digging (crypt)
	int digCount = 0; float digT = 0;
	// hints
	int objectiveKey = -1;
	float objectiveTime = 0;
	bool forceHint = false;
	Str hintText, objectiveText;
	int targetRoom = 0, targetTile = -1;
	int trail[GRID_W * GRID_H]; int trailLen = 0; float trailT = 0;
	int lastRoute[MAX_ROOMS]; int lastRouteLen = 0;
	int lastDfs[MAX_ROOMS]; int lastDfsLen = 0;
	// UI
	bool showDebug = false, showInventory = false;
	int tipOpen = -1; bool tipSeen[TIP_COUNT] = {};
	int tipCard = -1; float tipCardT = 0;            // survival note shown at the side (non-blocking)
	static constexpr float TIP_CARD_TIME = 10.f;
	void drawTipCard(sf::RenderWindow* w);
	sf::Texture bloodTex; sf::Sprite bloodSprite{ bloodTex };   // blood dripping down the note card
	int bloodTip[128] = {};                                       // per 3-px strip: where its run ends (px)
	int journalPage = -1;
	Str toast; float toastT = 0;
	sf::Font fontTitle, fontSerif, fontItalic, fontMono, fontBlood;
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
	void pickUp(int item, const Str& msg, bool clearTile);
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
	void hurt(float health, float fear, const Str& why, int tip, bool respawn);
	void knockBack(sf::Vector2f from);
	void die(const Str& why, int tip);
	void openTip(int tip, bool force = false);
	void say(const Str& s) { dialogue = s; }
	void showToast(const Str& s) { toast = s; toastT = 3.f; }
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
