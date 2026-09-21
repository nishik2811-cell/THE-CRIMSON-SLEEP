#include "Game.h"
#include <cstdlib>
#include <ctime>

Game::Game() : houseGraph(ROOM_COUNT) {
    initRooms();
    srand((unsigned int)time(nullptr));

    for (int i = 0; i < ROOM_COUNT; i++) {
        roomItemId[i] = 0;
    }

    selectedExitIndex = 0;
    currentScreen = Screen::MAIN_MENU;
    gameOverReason = GameOverReason::NONE;
    endingType = EndingType::NONE;

    tunnelDiscovered = false;
    secretRouteUnlocked = false;
    usedGardenEscape = false;

    hintPathLen = 0;
    dfsResultCount = 0;

    eventTimer = 4.0f;
    threatTimer = 3.0f;
    eventSeq = 0;
    threatSeq = 0;
    lastThreatType = THREAT_DARK_ROOM;

    buildHouse();
    placeItems();
    buildClueDatabase();

    lastMessage = "You wake up in the Entrance Hall. The door behind you will not open... yet.";

    // Roll for the starting room too, so the debug panel has
    // something to show immediately.
    rollEventsAndThreatsForRoom(ENTRANCE_HALL);
}

// ------------------------------------------------------------------
// Setup helpers
// ------------------------------------------------------------------
void Game::buildHouse() {
    // Building the Graph = wiring every doorway in the house. Some
    // doors start locked and are only opened later through gameplay
    // (see interact()) or through DFS exploration (see
    // inspectOrSearch()).
    houseGraph.addEdge(ENTRANCE_HALL, LIBRARY, false);
    houseGraph.addEdge(ENTRANCE_HALL, LIVING_ROOM, false);
    houseGraph.addEdge(ENTRANCE_HALL, GARDEN, false);

    houseGraph.addEdge(LIBRARY, STUDY, true);      // needs Rusty Key

    houseGraph.addEdge(LIVING_ROOM, KITCHEN, false);
    houseGraph.addEdge(LIVING_ROOM, BEDROOM, false);

    houseGraph.addEdge(BEDROOM, BATHROOM, false);
    houseGraph.addEdge(BEDROOM, ATTIC, false);

    houseGraph.addEdge(STUDY, ATTIC, false);

    houseGraph.addEdge(KITCHEN, BASEMENT, true);   // needs Basement Key

    houseGraph.addEdge(BASEMENT, HIDDEN_TUNNEL, true); // hidden, revealed by DFS search
    houseGraph.addEdge(HIDDEN_TUNNEL, RITUAL_ROOM, true); // needs Ritual Symbol
    houseGraph.addEdge(RITUAL_ROOM, SECRET_ROOM, true);   // needs Strange Coin + Photograph knowledge

    houseGraph.addEdge(SECRET_ROOM, GARDEN, true); // secret escape route, unlocked on discovery
}

void Game::placeItems() {
    itemMeta[ID_DIARY_PAGE]     = { ID_DIARY_PAGE, "Diary Page", "A torn page: 'The key to the cellar rests where papers pile...'" };
    itemMeta[ID_RUSTY_KEY]      = { ID_RUSTY_KEY, "Rusty Key", "An old rusty key. It might fit a nearby door." };
    itemMeta[ID_BASEMENT_KEY]   = { ID_BASEMENT_KEY, "Basement Key", "A heavy iron key marked with a cellar symbol." };
    itemMeta[ID_TORCH]          = { ID_TORCH, "Torch", "A working torch. Keeps the darkness at bay." };
    itemMeta[ID_OLD_PHOTOGRAPH] = { ID_OLD_PHOTOGRAPH, "Old Photograph", "A family stands before a ritual circle. One face is scratched out." };
    itemMeta[ID_RITUAL_SYMBOL]  = { ID_RITUAL_SYMBOL, "Ritual Symbol", "A carved stone symbol, still warm to the touch." };
    itemMeta[ID_STRANGE_COIN]   = { ID_STRANGE_COIN, "Strange Coin", "An old coin engraved with symbols matching the Ritual Room." };

    roomItemId[LIBRARY]    = ID_DIARY_PAGE;
    roomItemId[LIVING_ROOM] = ID_RUSTY_KEY;
    roomItemId[STUDY]      = ID_BASEMENT_KEY;
    roomItemId[KITCHEN]    = ID_TORCH;
    roomItemId[ATTIC]      = ID_OLD_PHOTOGRAPH;
    roomItemId[BASEMENT]   = ID_RITUAL_SYMBOL;
    roomItemId[GARDEN]     = ID_STRANGE_COIN;
}

void Game::buildClueDatabase() {
    // The BST is keyed by item id so that "does the player know about
    // item X" can be looked up in O(log n) time during puzzle checks.
    for (int id = 1; id < ITEM_ID_COUNT; id++) {
        ClueInfo info;
        info.id = id;
        info.name = itemMeta[id].name;
        info.description = itemMeta[id].description;
        info.discovered = false;
        clueDB.insert(info);
    }
}

bool Game::isDarkRoom(int roomId) const {
    return roomId == BASEMENT || roomId == HIDDEN_TUNNEL ||
           roomId == RITUAL_ROOM || roomId == ATTIC;
}

void Game::rollEventsAndThreatsForRoom(int roomId) {
    int danger = getRoomData(roomId).baseDangerLevel;

    // --- Queue usage: chronological environmental events ---
    if (rand() % 10 < danger) {
        EventType types[] = { LIGHTS_FLICKER, DOOR_SLAM, WHISPER_HEARD, OBJECT_FALLS, FOOTSTEPS_HEARD };
        EventType chosen = types[rand() % 5];
        eventQueue.enqueue(makeEvent(chosen, roomId));
    }

    // --- Priority Queue usage: active threats, resolved by danger level ---
    if (isDarkRoom(roomId) && !player.hasItem("Torch")) {
        threatQueue.insert(makeThreat(THREAT_DARK_ROOM, roomId));
    }
    if (danger >= 5 && rand() % 10 < (danger - 3)) {
        threatQueue.insert(makeThreat(THREAT_TRAP, roomId));
    }
    if (danger >= 6 && rand() % 10 < (danger - 4)) {
        threatQueue.insert(makeThreat(THREAT_GHOST_NEARBY, roomId));
    }
    if (danger >= 8 && rand() % 10 < (danger - 6)) {
        threatQueue.insert(makeThreat(THREAT_GHOST_ATTACK, roomId));
    }
}

// ------------------------------------------------------------------
// Per-frame update
// ------------------------------------------------------------------
void Game::update(float dt) {
    if (currentScreen != Screen::PLAYING) {
        return;
    }

    // Process the oldest queued haunted event every few seconds (FIFO).
    eventTimer -= dt;
    if (eventTimer <= 0.0f) {
        eventTimer = 4.0f;
        if (!eventQueue.isEmpty()) {
            Event e = eventQueue.dequeue();
            player.addFear(e.fearIncrease);
            lastEventMessage = e.description;
            eventSeq++;
        }
    }

    // Resolve the single most dangerous queued threat every few
    // seconds (max-heap extraction order, not arrival order).
    threatTimer -= dt;
    if (threatTimer <= 0.0f) {
        threatTimer = 3.0f;
        if (!threatQueue.isEmpty()) {
            Threat t = threatQueue.extractMax();
            player.takeDamage(t.healthDamage);
            player.addFear(t.fearIncrease);
            lastThreatMessage = t.description;
            lastThreatType = t.type;
            threatSeq++;
        }
    }

    if (player.isDead()) {
        gameOverReason = GameOverReason::HEALTH;
        currentScreen = Screen::GAME_OVER;
    } else if (player.isInsane()) {
        gameOverReason = GameOverReason::FEAR;
        currentScreen = Screen::GAME_OVER;
    }
}

// ------------------------------------------------------------------
// Screen / flow control
// ------------------------------------------------------------------
Screen Game::getScreen() const { return currentScreen; }

void Game::startNewGame() {
    currentScreen = Screen::PLAYING;
}

void Game::goToMainMenu() {
    currentScreen = Screen::MAIN_MENU;
}

void Game::togglePause() {
    if (currentScreen == Screen::PLAYING) {
        currentScreen = Screen::PAUSED;
    } else if (currentScreen == Screen::PAUSED) {
        currentScreen = Screen::PLAYING;
    }
}

void Game::showControls() {
    currentScreen = Screen::CONTROLS;
}

// ------------------------------------------------------------------
// Movement (Graph + Stack)
// ------------------------------------------------------------------
void Game::getCurrentExits(int* outNeighbors, bool* outPassable, int maxSize, int& outCount) const {
    houseGraph.getNeighbors(player.getCurrentRoom(), outNeighbors, maxSize, outCount);
    for (int i = 0; i < outCount; i++) {
        outPassable[i] = houseGraph.isPassable(player.getCurrentRoom(), outNeighbors[i]);
    }
}

int Game::getSelectedExitIndex() const {
    return selectedExitIndex;
}

void Game::selectNextExit() {
    int neighbors[MAX_ROOMS];
    bool passable[MAX_ROOMS];
    int count;
    getCurrentExits(neighbors, passable, MAX_ROOMS, count);
    if (count == 0) return;
    selectedExitIndex = (selectedExitIndex + 1) % count;
}

void Game::selectPrevExit() {
    int neighbors[MAX_ROOMS];
    bool passable[MAX_ROOMS];
    int count;
    getCurrentExits(neighbors, passable, MAX_ROOMS, count);
    if (count == 0) return;
    selectedExitIndex = (selectedExitIndex - 1 + count) % count;
}

bool Game::confirmMove() {
    int neighbors[MAX_ROOMS];
    bool passable[MAX_ROOMS];
    int count;
    getCurrentExits(neighbors, passable, MAX_ROOMS, count);

    if (count == 0 || selectedExitIndex >= count) {
        return false;
    }

    int target = neighbors[selectedExitIndex];
    if (!houseGraph.isPassable(player.getCurrentRoom(), target)) {
        lastMessage = "That door is locked.";
        return false;
    }

    player.enterRoom(target); // Stack push happens inside Player::enterRoom
    selectedExitIndex = 0;
    lastMessage = "You enter the " + getRoomData(target).name + ".";
    rollEventsAndThreatsForRoom(target);

    // Reaching the Secret Room automatically reveals the house's
    // hidden truth and opens the Garden shortcut (the "final route").
    if (target == SECRET_ROOM && !secretRouteUnlocked) {
        secretRouteUnlocked = true;
        houseGraph.unlockEdge(SECRET_ROOM, GARDEN);
        lastMessage = "Hidden documents reveal the house's darkest truth. A passage beyond the wall now leads to the Garden.";
    }

    return true;
}

bool Game::goBack() {
    int previousRoom;
    if (player.goBack(previousRoom)) {
        selectedExitIndex = 0;
        lastMessage = "You go back to the " + getRoomData(previousRoom).name + ".";
        return true;
    }
    lastMessage = "There is nowhere to go back to.";
    return false;
}

// ------------------------------------------------------------------
// Interaction ('E')
// ------------------------------------------------------------------
void Game::interact() {
    int room = player.getCurrentRoom();

    // 1) Pick up an item sitting in this room, if any.
    if (roomItemId[room] != 0) {
        int id = roomItemId[room];

        if (id == ID_BASEMENT_KEY) {
            ClueInfo* diary = clueDB.search(ID_DIARY_PAGE);
            if (diary == nullptr || !diary->discovered) {
                lastMessage = "A heavy iron key sits here, but you have no idea what it opens. (Try reading the Diary Page first)";
                return;
            }
        }

        Item item;
        item.id = id;
        item.name = itemMeta[id].name;
        item.description = itemMeta[id].description;
        player.addItem(item);
        roomItemId[room] = 0;

        // Diary Page and Old Photograph must be actively read (F) to
        // count as "discovered" - picking them up is not enough.
        if (id != ID_DIARY_PAGE && id != ID_OLD_PHOTOGRAPH) {
            clueDB.markDiscovered(id);
        }

        lastMessage = "You picked up: " + itemMeta[id].name;
        return;
    }

    // 2) Room-specific door unlocks / special actions.
    if (room == LIBRARY && houseGraph.isLocked(LIBRARY, STUDY)) {
        if (player.hasItem("Rusty Key")) {
            houseGraph.unlockEdge(LIBRARY, STUDY);
            player.useItem("Rusty Key");
            lastMessage = "You unlock the Study door with the Rusty Key.";
        } else {
            lastMessage = "The door to the Study is locked. You need a key.";
        }
        return;
    }

    if (room == KITCHEN && houseGraph.isLocked(KITCHEN, BASEMENT)) {
        if (player.hasItem("Basement Key")) {
            houseGraph.unlockEdge(KITCHEN, BASEMENT);
            player.useItem("Basement Key");
            lastMessage = "You unlock the basement door with the Basement Key.";
        } else {
            lastMessage = "The cellar door is locked tight.";
        }
        return;
    }

    if (room == HIDDEN_TUNNEL && houseGraph.isLocked(HIDDEN_TUNNEL, RITUAL_ROOM)) {
        if (player.hasItem("Ritual Symbol")) {
            houseGraph.unlockEdge(HIDDEN_TUNNEL, RITUAL_ROOM);
            player.useItem("Ritual Symbol");
            lastMessage = "You fit the Ritual Symbol into the wall. A passage grinds open.";
        } else {
            lastMessage = "A symbol-shaped groove is carved into the wall here.";
        }
        return;
    }

    if (room == RITUAL_ROOM && houseGraph.isLocked(RITUAL_ROOM, SECRET_ROOM)) {
        ClueInfo* photo = clueDB.search(ID_OLD_PHOTOGRAPH);
        bool knowsPhoto = (photo != nullptr && photo->discovered);
        if (player.hasItem("Strange Coin") && knowsPhoto) {
            houseGraph.unlockEdge(RITUAL_ROOM, SECRET_ROOM);
            player.useItem("Strange Coin");
            lastMessage = "The photograph shows exactly where the coin fits. The wall grinds open.";
        } else if (!player.hasItem("Strange Coin")) {
            lastMessage = "There is a coin-sized slot in the wall here.";
        } else {
            lastMessage = "You have a coin, but the pattern makes no sense yet. (Study a photograph first)";
        }
        return;
    }

    if (room == ENTRANCE_HALL) {
        usedGardenEscape = false;
        evaluateEndingAndFinish();
        return;
    }

    if (room == GARDEN && secretRouteUnlocked) {
        usedGardenEscape = true;
        evaluateEndingAndFinish();
        return;
    }

    lastMessage = "There is nothing to interact with here.";
}

void Game::evaluateEndingAndFinish() {
    if (usedGardenEscape && secretRouteUnlocked) {
        endingType = EndingType::SECRET;
    } else if (!player.hasVisited(RITUAL_ROOM)) {
        endingType = EndingType::TRAPPED;
    } else if (secretRouteUnlocked && player.getHealth() >= 50 && player.getFear() <= 40) {
        endingType = EndingType::TRUE_END;
    } else {
        endingType = EndingType::ESCAPE;
    }
    currentScreen = Screen::ENDING;
}

// ------------------------------------------------------------------
// Inspect / Search ('F') - BST clue reading + DFS hidden search
// ------------------------------------------------------------------
void Game::inspectOrSearch() {
    ClueInfo* diary = clueDB.search(ID_DIARY_PAGE);
    if (player.hasItem("Diary Page") && diary != nullptr && !diary->discovered) {
        clueDB.markDiscovered(ID_DIARY_PAGE);
        lastMessage = "Diary Page: \"" + diary->description + "\"";
        return;
    }

    ClueInfo* photo = clueDB.search(ID_OLD_PHOTOGRAPH);
    if (player.hasItem("Old Photograph") && photo != nullptr && !photo->discovered) {
        clueDB.markDiscovered(ID_OLD_PHOTOGRAPH);
        lastMessage = "Old Photograph: \"" + photo->description + "\"";
        return;
    }

    int room = player.getCurrentRoom();
    if (room == BASEMENT && !tunnelDiscovered) {
        // DFS gameplay feature: explore every room reachable from the
        // Basement (ignoring locks) to "discover" the Hidden Tunnel,
        // then physically unlock the passage.
        houseGraph.dfsExplore(BASEMENT, dfsResult, dfsResultCount);
        tunnelDiscovered = true;
        houseGraph.unlockEdge(BASEMENT, HIDDEN_TUNNEL);
        lastMessage = "You search the damp walls and find a hidden lever! A passage to a Hidden Tunnel creaks open.";
        return;
    }

    // No special find: still run DFS from here so the debug panel has
    // a fresh exploration result to show.
    houseGraph.dfsExplore(room, dfsResult, dfsResultCount);
    lastMessage = "You search the room but find nothing new.";
}

// ------------------------------------------------------------------
// BFS hint system ('H')
// ------------------------------------------------------------------
Objective Game::getObjective() const {
    if (!player.hasItem("Diary Page") && roomItemId[LIBRARY] == ID_DIARY_PAGE) return Objective::GET_DIARY;
    if (!player.hasItem("Rusty Key") && roomItemId[LIVING_ROOM] == ID_RUSTY_KEY) return Objective::GET_RUSTY_KEY;
    if (houseGraph.isLocked(LIBRARY, STUDY)) return Objective::UNLOCK_STUDY;
    if (!player.hasItem("Basement Key") && roomItemId[STUDY] == ID_BASEMENT_KEY) return Objective::GET_BASEMENT_KEY;
    if (!player.hasItem("Torch") && roomItemId[KITCHEN] == ID_TORCH) return Objective::GET_TORCH;
    if (houseGraph.isLocked(KITCHEN, BASEMENT)) return Objective::UNLOCK_BASEMENT;
    if (!player.hasItem("Ritual Symbol") && roomItemId[BASEMENT] == ID_RITUAL_SYMBOL) return Objective::GET_RITUAL_SYMBOL;
    if (!tunnelDiscovered) return Objective::SEARCH_BASEMENT;
    if (houseGraph.isLocked(HIDDEN_TUNNEL, RITUAL_ROOM)) return Objective::UNLOCK_RITUAL;
    if (!player.hasVisited(RITUAL_ROOM)) return Objective::ENTER_RITUAL;
    if (!player.hasItem("Old Photograph") && roomItemId[ATTIC] == ID_OLD_PHOTOGRAPH) return Objective::GET_PHOTOGRAPH;
    if (!player.hasItem("Strange Coin") && roomItemId[GARDEN] == ID_STRANGE_COIN) return Objective::GET_COIN;
    if (houseGraph.isLocked(RITUAL_ROOM, SECRET_ROOM)) return Objective::UNLOCK_SECRET;
    if (!player.hasVisited(SECRET_ROOM)) return Objective::ENTER_SECRET;
    return Objective::GO_TO_EXIT;
}

void Game::requestHint() {
    Objective obj = getObjective();
    int target = ENTRANCE_HALL;
    std::string label;

    switch (obj) {
        case Objective::GET_DIARY:       target = LIBRARY;      label = "find the Diary Page"; break;
        case Objective::GET_RUSTY_KEY:   target = LIVING_ROOM;  label = "find the Rusty Key"; break;
        case Objective::UNLOCK_STUDY:    target = LIBRARY;      label = "unlock the Study door"; break;
        case Objective::GET_BASEMENT_KEY:target = STUDY;        label = "find the Basement Key"; break;
        case Objective::GET_TORCH:       target = KITCHEN;      label = "find the Torch"; break;
        case Objective::UNLOCK_BASEMENT: target = KITCHEN;      label = "unlock the Basement door"; break;
        case Objective::GET_RITUAL_SYMBOL:target = BASEMENT;    label = "find the Ritual Symbol"; break;
        case Objective::SEARCH_BASEMENT: target = BASEMENT;     label = "search the Basement for hidden passages"; break;
        case Objective::UNLOCK_RITUAL:   target = HIDDEN_TUNNEL;label = "unlock the way to the Ritual Room"; break;
        case Objective::ENTER_RITUAL:    target = RITUAL_ROOM;  label = "enter the Ritual Room"; break;
        case Objective::GET_PHOTOGRAPH:  target = ATTIC;        label = "find the Old Photograph"; break;
        case Objective::GET_COIN:        target = GARDEN;       label = "find the Strange Coin"; break;
        case Objective::UNLOCK_SECRET:   target = RITUAL_ROOM;  label = "unlock the Secret Room"; break;
        case Objective::ENTER_SECRET:    target = SECRET_ROOM;  label = "enter the Secret Room"; break;
        case Objective::GO_TO_EXIT:      target = ENTRANCE_HALL;label = "head for the exit"; break;
    }

    houseGraph.bfsShortestPath(player.getCurrentRoom(), target, hintPath, hintPathLen);

    if (hintPathLen <= 1) {
        hintMessage = "You are already here. Goal: " + label + ".";
    } else if (hintPathLen == 0) {
        hintMessage = "No known route right now - a door may still be locked.";
    } else {
        hintMessage = "Goal: " + label + ". Next room: " + getRoomData(hintPath[1]).name + ".";
    }
}

const int* Game::getHintPath() const { return hintPath; }
int Game::getHintPathLen() const { return hintPathLen; }
std::string Game::getHintMessage() const { return hintMessage; }

const int* Game::getDFSResult() const { return dfsResult; }
int Game::getDFSResultCount() const { return dfsResultCount; }

// ------------------------------------------------------------------
// Accessors
// ------------------------------------------------------------------
const Player& Game::getPlayer() const { return player; }
const Graph& Game::getGraph() const { return houseGraph; }
const Queue& Game::getEventQueue() const { return eventQueue; }
const PriorityQueue& Game::getThreatQueue() const { return threatQueue; }
const BST& Game::getClueDB() const { return clueDB; }
std::string Game::getLastMessage() const { return lastMessage; }
std::string Game::getLastEventMessage() const { return lastEventMessage; }
std::string Game::getLastThreatMessage() const { return lastThreatMessage; }
ThreatType Game::getLastThreatType() const { return lastThreatType; }
int Game::getEventSeq() const { return eventSeq; }
int Game::getThreatSeq() const { return threatSeq; }
GameOverReason Game::getGameOverReason() const { return gameOverReason; }
EndingType Game::getEndingType() const { return endingType; }
int Game::getRoomItemId(int roomId) const { return roomItemId[roomId]; }
std::string Game::getItemName(int id) const { return itemMeta[id].name; }
std::string Game::getItemDescription(int id) const { return itemMeta[id].description; }
