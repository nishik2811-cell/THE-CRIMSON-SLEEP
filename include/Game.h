#ifndef GAME_H
#define GAME_H

#include "Graph.h"
#include "Player.h"
#include "Queue.h"
#include "PriorityQueue.h"
#include "BST.h"
#include "Room.h"
#include <string>

// Item / clue ids. These are shared between the BST (ClueInfo.id),
// the picked-up Item.id, and the room-item placement table below, so
// that "does the player know about X" (BST) and "does the player
// carry X" (LinkedList inventory) always agree on what X means.
const int ID_DIARY_PAGE     = 1;
const int ID_RUSTY_KEY      = 2;
const int ID_BASEMENT_KEY   = 3;
const int ID_TORCH          = 4;
const int ID_OLD_PHOTOGRAPH = 5;
const int ID_RITUAL_SYMBOL  = 6;
const int ID_STRANGE_COIN   = 7;
const int ITEM_ID_COUNT     = 8; // ids 1..7 used, index 0 unused

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
// is requested (see Game::getObjectiveRoom).
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

    // Which item (by id, 0 = none) is currently sitting in each room,
    // waiting to be picked up. Parallel ARRAY indexed by RoomId.
    int roomItemId[ROOM_COUNT];

    // Simple metadata table (ARRAY) describing every item/clue, used
    // to build both the Item placed in a room and the ClueInfo in the
    // BST from a single source of truth.
    struct ItemMeta { int id; std::string name; std::string description; };
    ItemMeta itemMeta[ITEM_ID_COUNT];

    int selectedExitIndex;
    Screen currentScreen;
    GameOverReason gameOverReason;
    EndingType endingType;

    std::string lastMessage;      // general interaction feedback
    std::string lastEventMessage; // most recently processed haunted event
    std::string lastThreatMessage;// most recently resolved threat
    ThreatType lastThreatType;    // type of the most recently resolved threat
    std::string hintMessage;

    bool tunnelDiscovered;     // DFS reveal flag for Basement -> Hidden Tunnel
    bool secretRouteUnlocked;  // Secret Room -> Garden shortcut (secret ending path)
    bool usedGardenEscape;     // player chose the Garden tunnel over the front door

    int hintPath[MAX_ROOMS];
    int hintPathLen;

    int dfsResult[MAX_ROOMS];
    int dfsResultCount;

    float eventTimer;  // seconds until the next queued event auto-processes
    float threatTimer; // seconds until the next queued threat auto-resolves
    int eventSeq;       // increments every time an event is processed (lets UI detect new ones)
    int threatSeq;       // increments every time a threat is resolved

    void buildHouse();          // wires up the Graph edges (locked/unlocked)
    void placeItems();          // fills roomItemId[]
    void buildClueDatabase();   // fills the BST with all item/clue metadata

    bool isDarkRoom(int roomId) const;
    void rollEventsAndThreatsForRoom(int roomId);
    Objective getObjective() const;
    void evaluateEndingAndFinish(); // decides EndingType and switches to Screen::ENDING

public:
    Game();

    void update(float dt); // called once per frame by the SFML layer

    // --- Screen / flow control ---
    Screen getScreen() const;
    void startNewGame();
    void goToMainMenu();
    void togglePause();
    void showControls();

    // --- Movement (Graph + Stack) ---
    void selectNextExit();
    void selectPrevExit();
    bool confirmMove();  // move into currently selected exit if passable
    bool goBack();        // Stack-based "Go Back"
    void getCurrentExits(int* outNeighbors, bool* outPassable, int maxSize, int& outCount) const;
    int getSelectedExitIndex() const;

    // --- Interaction ---
    void interact();          // 'E' - pick up items / unlock doors / trigger exits
    void inspectOrSearch();   // 'F' - BST clue read, or DFS hidden-area search

    // --- BFS hint ---
    void requestHint();       // 'H' - fills hintPath using BFS toward the current objective
    const int* getHintPath() const;
    int getHintPathLen() const;
    std::string getHintMessage() const;

    // --- DFS exploration result (for UI/debug panel) ---
    const int* getDFSResult() const;
    int getDFSResultCount() const;

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
    int getRoomItemId(int roomId) const;
    std::string getItemName(int id) const;
    std::string getItemDescription(int id) const;
};

#endif
