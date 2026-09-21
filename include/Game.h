#ifndef GAME_H
#define GAME_H

#include "Graph.h"
#include "Player.h"
#include "Queue.h"
#include "PriorityQueue.h"
#include "BST.h"
#include "Room.h"
#include "WorldObject.h"
#include <string>

// Item / clue ids. These are shared between the BST (ClueInfo.id),
// the picked-up Item.id, and the world-object placement table, so
// that "does the player know about X" (BST) and "does the player
// carry X" (LinkedList inventory) always agree on what X means.
const int ID_DIARY_PAGE     = 1;
const int ID_RUSTY_KEY      = 2;
const int ID_BASEMENT_KEY   = 3;
const int ID_TORCH          = 4;
const int ID_OLD_PHOTOGRAPH = 5;
const int ID_RITUAL_SYMBOL  = 6;
const int ID_STRANGE_COIN   = 7;
const int ID_BROKEN_HANDLE  = 8;  // flavor item, combines into a Makeshift Lever
const int ID_METAL_ROD      = 9;  // flavor item, combines into a Makeshift Lever
const int ID_MAKESHIFT_LEVER= 10; // crafted automatically once both parts are held
const int ID_TORN_NOTE      = 11; // bonus lore reward from the Stuck Chest
const int ITEM_ID_COUNT     = 12; // ids 1..11 used, index 0 unused

const int MAX_OBJECTS_PER_ROOM = 8;

enum class Screen {
    MAIN_MENU,
    CONTROLS,
    PLAYING,
    PAUSED,
    GAME_OVER,
    ENDING
};

enum class GameOverReason { NONE, HEALTH, FEAR };
enum class EndingType { NONE, ESCAPE, TRAPPED, SECRET, TRUE_END };

// Simple linear objective state used to point the BFS hint system at
// something useful. Re-evaluated from current flags every time a hint
// is requested (see Game::getObjective).
enum class Objective {
    GET_RUSTY_KEY, UNLOCK_STUDY, GET_DIARY, GET_BASEMENT_KEY,
    UNLOCK_BASEMENT, GET_TORCH, GET_RITUAL_SYMBOL, SEARCH_BASEMENT,
    UNLOCK_RITUAL, ENTER_RITUAL, GET_PHOTOGRAPH, GET_COIN,
    UNLOCK_SECRET, ENTER_SECRET, GO_TO_EXIT
};

class Game {
private:
    Graph houseGraph;
    Player player;
    Queue eventQueue;
    PriorityQueue threatQueue;
    BST clueDB;

    // Every interactable (and purely decorative) object placed in
    // every room. Fixed-size ARRAY of ARRAYs - mutable at runtime
    // (active/opened flags flip as the player plays).
    WorldObject roomObjects[ROOM_COUNT][MAX_OBJECTS_PER_ROOM];
    int roomObjectCount[ROOM_COUNT];

    // Simple metadata table (ARRAY) describing every item/clue, used
    // to build both the Item granted by a world object and the
    // ClueInfo in the BST from a single source of truth.
    struct ItemMeta { int id; std::string name; std::string description; };
    ItemMeta itemMeta[ITEM_ID_COUNT];

    Screen currentScreen;
    GameOverReason gameOverReason;
    EndingType endingType;

    std::string lastMessage;      // general interaction feedback
    std::string lastEventMessage; // most recently processed haunted event
    std::string lastThreatMessage;// most recently resolved threat
    ThreatType lastThreatType;    // type of the most recently resolved threat
    std::string hintMessage;

    std::string pickupToastText;  // "+ Rusty Key added to inventory"
    float pickupToastTimer;
    std::string clueBannerText;   // "NEW CLUE: ..."
    float clueBannerTimer;

    static const int STORY_BEAT_COUNT = 7;
    std::string storyBeats[STORY_BEAT_COUNT]; // the house's backstory, revealed as you progress
    bool storyShown[STORY_BEAT_COUNT];
    std::string storyText;
    float storyTimer;

    bool tunnelDiscovered;     // DFS reveal flag for Basement -> Hidden Tunnel
    bool secretRouteUnlocked;  // Secret Room -> Garden shortcut (secret ending path)
    bool usedGardenEscape;     // player chose the Garden tunnel over the front door
    bool clockInspected;       // one-time flavor clue flag (Entrance Hall clock)
    bool studyDeskOpened;      // one-time flavor clue flag (Study desk drawer)

    int hintPath[MAX_ROOMS];
    int hintPathLen;

    int dfsResult[MAX_ROOMS];
    int dfsResultCount;

    float eventTimer;  // seconds until the next queued event auto-processes
    float threatTimer; // seconds until the next queued threat auto-resolves
    int eventSeq;       // increments every time an event is processed (lets UI detect new ones)
    int threatSeq;       // increments every time a threat is resolved

    void buildHouse();          // wires up the Graph edges (locked/unlocked)
    void buildWorldObjects();   // fills roomObjects[] (items, doors, furniture, puzzle objects)
    void buildClueDatabase();   // fills the BST with all item/clue metadata

    void addObject(int room, const WorldObject& obj);
    std::string requiredItemForEdge(int roomA, int roomB) const;
    void setDoorActive(int room, int targetRoom, bool active); // flips visibility on both sides
    void craftIfPossible(); // auto-combines Broken Handle + Metal Rod

    bool isDarkRoom(int roomId) const;
    void rollEventsAndThreatsForRoom(int roomId);
    void onEnterRoom(int roomId); // shared logic for confirmMove/goBack landing in a room
    Objective getObjective() const;
    std::string objectiveLabel(Objective obj) const;
    void evaluateEndingAndFinish(); // decides EndingType and switches to Screen::ENDING
    void announceClue(const std::string& text);
    void triggerStory(int index); // shows storyBeats[index] once, ever
    void grantItem(int id);

public:
    Game();

    void update(float dt); // called once per frame by the SFML layer

    // --- Screen / flow control ---
    Screen getScreen() const;
    void startNewGame();
    void goToMainMenu();
    void togglePause();
    void showControls();

    // --- Movement (Graph + Stack + spatial position) ---
    void movePlayer(float dx, float dy); // continuous WASD/arrow movement within the room
    bool goBack();                        // Stack-based "Go Back"

    // --- Interaction (proximity-based) ---
    int findNearestInteractable() const;  // index into current room's objects, or -1
    std::string getInteractPrompt() const;// "[E] Pick up Rusty Key" etc, empty if nothing nearby
    void interact();                      // 'E' - act on the nearest interactable object
    void inspectOrSearch();               // 'F' - read a held-but-unread clue item (BST)

    // --- BFS hint ---
    void requestHint();       // 'H' - fills hintPath using BFS toward the current objective
    const int* getHintPath() const;
    int getHintPathLen() const;
    std::string getHintMessage() const;
    std::string getObjectiveLabel() const; // persistent short objective text

    // --- DFS exploration result (for UI/debug panel) ---
    const int* getDFSResult() const;
    int getDFSResultCount() const;

    // --- World objects (for rendering) ---
    const WorldObject* getRoomObjects(int room, int& outCount) const;

    // --- Accessors for rendering / UI ---
    const Player& getPlayer() const;
    const Graph& getGraph() const;
    const Queue& getEventQueue() const;
    const PriorityQueue& getThreatQueue() const;
    const BST& getClueDB() const;
    std::string getLastMessage() const;
    std::string getLastEventMessage() const;
    std::string getLastThreatMessage() const;
    ThreatType getLastThreatType() const;
    int getEventSeq() const;
    int getThreatSeq() const;
    GameOverReason getGameOverReason() const;
    EndingType getEndingType() const;
    std::string getItemName(int id) const;
    std::string getItemDescription(int id) const;
    std::string getPickupToast(float& outTimer) const;
    std::string getClueBanner(float& outTimer) const;
    std::string getStoryBanner(float& outTimer) const;
};

#endif
