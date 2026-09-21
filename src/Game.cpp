#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

Game::Game() : houseGraph(ROOM_COUNT) {
    initRooms();
    srand((unsigned int)time(nullptr));

    for (int i = 0; i < ROOM_COUNT; i++) {
        roomObjectCount[i] = 0;
    }

    currentScreen = Screen::MAIN_MENU;
    gameOverReason = GameOverReason::NONE;
    endingType = EndingType::NONE;

    pickupToastText = "";
    pickupToastTimer = 0.f;
    clueBannerText = "";
    clueBannerTimer = 0.f;

    storyBeats[0] = "You don't remember walking in. You just remember waking up, and the door behind you was already sealed.";
    storyBeats[1] = "The diary belonged to someone named Elias. His last entry says the house 'asks for one more every generation.'";
    storyBeats[2] = "Something moved beneath the kitchen floor before you even touched the cellar door.";
    storyBeats[3] = "The passage was bricked shut once. Recently. The mortar hasn't even finished curing.";
    storyBeats[4] = "The circle burned into this floor isn't old. Someone has been maintaining it. Feeding it.";
    storyBeats[5] = "The photograph shows six people. One face has been scratched away so hard it tore the paper. You count the others left. There are only five.";
    storyBeats[6] = "The ledger lists every name that has slept in this house since 1961. Yours is already written at the bottom, in ink that hasn't dried.";
    for (int i = 0; i < STORY_BEAT_COUNT; i++) storyShown[i] = false;
    storyText = "";
    storyTimer = 0.f;

    tunnelDiscovered = false;
    secretRouteUnlocked = false;
    usedGardenEscape = false;
    clockInspected = false;
    studyDeskOpened = false;

    hintPathLen = 0;
    dfsResultCount = 0;

    eventTimer = 4.0f;
    threatTimer = 3.0f;
    eventSeq = 0;
    threatSeq = 0;
    lastThreatType = THREAT_DARK_ROOM;

    buildHouse();

    // itemMeta must exist before both the world objects (which name
    // themselves from it) and the BST are built.
    itemMeta[ID_DIARY_PAGE]      = { ID_DIARY_PAGE, "Diary Page", "A torn page: 'The key to the cellar rests where papers pile...'" };
    itemMeta[ID_RUSTY_KEY]       = { ID_RUSTY_KEY, "Rusty Key", "An old rusty key. It might fit a nearby door." };
    itemMeta[ID_BASEMENT_KEY]    = { ID_BASEMENT_KEY, "Basement Key", "A heavy iron key marked with a cellar symbol." };
    itemMeta[ID_TORCH]           = { ID_TORCH, "Torch", "A working torch. Keeps the darkness at bay." };
    itemMeta[ID_OLD_PHOTOGRAPH]  = { ID_OLD_PHOTOGRAPH, "Old Photograph", "A family stands before a ritual circle. One face is scratched out." };
    itemMeta[ID_RITUAL_SYMBOL]   = { ID_RITUAL_SYMBOL, "Ritual Symbol", "A carved stone symbol, still warm to the touch." };
    itemMeta[ID_STRANGE_COIN]    = { ID_STRANGE_COIN, "Strange Coin", "An old coin engraved with symbols matching the Ritual Room." };
    itemMeta[ID_BROKEN_HANDLE]   = { ID_BROKEN_HANDLE, "Broken Handle", "A splintered wooden handle. It might attach to something." };
    itemMeta[ID_METAL_ROD]       = { ID_METAL_ROD, "Metal Rod", "A stiff iron rod, slightly bent." };
    itemMeta[ID_MAKESHIFT_LEVER] = { ID_MAKESHIFT_LEVER, "Makeshift Lever", "A crude lever lashed together from a rod and a handle." };
    itemMeta[ID_TORN_NOTE]       = { ID_TORN_NOTE, "Torn Note", "'...if you find this, I am already gone. Don't trust the Ritual Room.'" };

    buildWorldObjects();
    buildClueDatabase();

    lastMessage = "You wake up in the Entrance Hall. The door behind you will not open... yet.";
    onEnterRoom(ENTRANCE_HALL);
}

// ------------------------------------------------------------------
// Setup helpers
// ------------------------------------------------------------------
void Game::buildHouse() {
    // Building the Graph = wiring every doorway in the house. Some
    // doors start locked and are only opened later through gameplay
    // (see interact()) or through DFS exploration (Cracked Wall).
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

    houseGraph.addEdge(BASEMENT, HIDDEN_TUNNEL, true); // hidden, revealed by the Cracked Wall (DFS)
    houseGraph.addEdge(HIDDEN_TUNNEL, RITUAL_ROOM, true); // needs Ritual Symbol
    houseGraph.addEdge(RITUAL_ROOM, SECRET_ROOM, true);   // needs Strange Coin + Photograph knowledge

    houseGraph.addEdge(SECRET_ROOM, GARDEN, true); // secret escape route, unlocked on discovery
}

void Game::addObject(int room, const WorldObject& obj) {
    if (roomObjectCount[room] >= MAX_OBJECTS_PER_ROOM) return;
    roomObjects[room][roomObjectCount[room]] = obj;
    roomObjectCount[room]++;
}

std::string Game::requiredItemForEdge(int a, int b) const {
    auto isPair = [&](int x, int y) { return (a == x && b == y) || (a == y && b == x); };
    if (isPair(LIBRARY, STUDY)) return "Rusty Key";
    if (isPair(KITCHEN, BASEMENT)) return "Basement Key";
    if (isPair(HIDDEN_TUNNEL, RITUAL_ROOM)) return "Ritual Symbol";
    return ""; // normal open doors, and the two "reveal" edges (handled separately)
}

void Game::setDoorActive(int room, int targetRoom, bool active) {
    for (int i = 0; i < roomObjectCount[room]; i++) {
        if (roomObjects[room][i].type == OBJ_DOOR && roomObjects[room][i].doorTargetRoom == targetRoom) {
            roomObjects[room][i].active = active;
        }
    }
}

void Game::buildWorldObjects() {
    // --- Doors: one visible object per Graph edge, spread along the
    // top of the room. This is a pure presentation detail - the
    // Graph itself remains the single source of truth for whether a
    // door is passable.
    for (int room = 0; room < ROOM_COUNT; room++) {
        int neighbors[MAX_ROOMS];
        int count;
        houseGraph.getNeighbors(room, neighbors, MAX_ROOMS, count);
        for (int i = 0; i < count; i++) {
            int target = neighbors[i];
            float x = (count == 1) ? ROOM_AREA_W / 2.f : 90.f + i * ((ROOM_AREA_W - 180.f) / (float)(count - 1));

            WorldObject door;
            door.type = OBJ_DOOR;
            door.x = x; door.y = 45.f; door.radius = 42.f;
            door.name = getRoomData(target).name;
            door.verb = "Enter";
            door.active = true;
            door.opened = false;
            door.linkedItemId = 0;
            door.doorTargetRoom = target;
            door.requiredItemName = requiredItemForEdge(room, target);
            door.inspectText = "";
            door.isDecorative = false;

            bool startsHidden =
                (room == BASEMENT && target == HIDDEN_TUNNEL) || (room == HIDDEN_TUNNEL && target == BASEMENT) ||
                (room == SECRET_ROOM && target == GARDEN) || (room == GARDEN && target == SECRET_ROOM);
            if (startsHidden) door.active = false;

            addObject(room, door);
        }
    }

    // --- Ending-trigger "virtual doors" (not real Graph edges) ---
    addObject(ENTRANCE_HALL, { OBJ_DOOR, 570.f, 320.f, 40.f, "Front Door", "Open", true, false, 0, -1, "", "", false });
    addObject(GARDEN, { OBJ_DOOR, 570.f, 320.f, 40.f, "Foggy Path", "Vanish into", false, false, 0, -1, "", "", false });

    // --- Entrance Hall ---
    addObject(ENTRANCE_HALL, { OBJ_PAINTING, 200.f, 180.f, 30.f, "Faded Painting", "Examine", true, false, 0, -1, "",
        "A painting of the family that once lived here. Their eyes seem to follow you.", false });
    addObject(ENTRANCE_HALL, { OBJ_CLOCK, 950.f, 180.f, 30.f, "Grandfather Clock", "Examine", true, false, 0, -1, "",
        "The clock is frozen at exactly 11:17.", false });

    // --- Library ---
    addObject(LIBRARY, { OBJ_ITEM, 300.f, 220.f, 32.f, itemMeta[ID_DIARY_PAGE].name, "Pick up", true, false, ID_DIARY_PAGE, -1, "", "", false });
    addObject(LIBRARY, { OBJ_BOOKSHELF, 150.f, 150.f, 32.f, "Bookshelf", "Search", true, false, 0, -1, "",
        "Rows of rotting books. Nothing stands out... yet.", false });
    addObject(LIBRARY, { OBJ_DRAWER, 900.f, 200.f, 32.f, "Writing Desk", "Open", true, false, 0, -1, "", "", false });

    // --- Living Room ---
    addObject(LIVING_ROOM, { OBJ_ITEM, 250.f, 200.f, 32.f, itemMeta[ID_RUSTY_KEY].name, "Pick up", true, false, ID_RUSTY_KEY, -1, "", "", false });
    addObject(LIVING_ROOM, { OBJ_PAINTING, 850.f, 150.f, 30.f, "Cracked Portrait", "Examine", true, false, 0, -1, "",
        "A portrait with a slashed canvas. Someone was very angry.", false });
    addObject(LIVING_ROOM, { OBJ_CABINET, 550.f, 250.f, 32.f, "Old Cabinet", "Open", true, false, 0, -1, "", "", false });

    // --- Kitchen ---
    addObject(KITCHEN, { OBJ_ITEM, 300.f, 220.f, 32.f, itemMeta[ID_TORCH].name, "Pick up", true, false, ID_TORCH, -1, "", "", false });
    addObject(KITCHEN, { OBJ_DRAWER, 750.f, 220.f, 32.f, "Kitchen Drawer", "Open", true, false, 0, -1, "", "", false });

    // --- Bedroom ---
    addObject(BEDROOM, { OBJ_MIRROR, 850.f, 180.f, 32.f, "Cracked Mirror", "Examine", true, false, 0, -1, "",
        "Your reflection looks tired.", false });
    addObject(BEDROOM, { OBJ_ITEM, 300.f, 250.f, 30.f, itemMeta[ID_BROKEN_HANDLE].name, "Pick up", true, false, ID_BROKEN_HANDLE, -1, "", "", false });

    // --- Bathroom ---
    addObject(BATHROOM, { OBJ_CABINET, 600.f, 200.f, 32.f, "Medicine Cabinet", "Open", true, false, 0, -1, "", "", false });

    // --- Study ---
    addObject(STUDY, { OBJ_ITEM, 300.f, 200.f, 32.f, itemMeta[ID_BASEMENT_KEY].name, "Pick up", true, false, ID_BASEMENT_KEY, -1, "", "", false });
    addObject(STUDY, { OBJ_DRAWER, 850.f, 220.f, 32.f, "Desk Drawer", "Open", true, false, 0, -1, "", "", false });

    // --- Basement ---
    addObject(BASEMENT, { OBJ_ITEM, 300.f, 250.f, 32.f, itemMeta[ID_RITUAL_SYMBOL].name, "Pick up", true, false, ID_RITUAL_SYMBOL, -1, "", "", false });
    addObject(BASEMENT, { OBJ_CRACKED_WALL, 850.f, 220.f, 34.f, "Cracked Wall", "Search", true, false, 0, -1, "", "", false });

    // --- Attic ---
    addObject(ATTIC, { OBJ_ITEM, 300.f, 200.f, 32.f, itemMeta[ID_OLD_PHOTOGRAPH].name, "Pick up", true, false, ID_OLD_PHOTOGRAPH, -1, "", "", false });
    addObject(ATTIC, { OBJ_ITEM, 800.f, 250.f, 30.f, itemMeta[ID_METAL_ROD].name, "Pick up", true, false, ID_METAL_ROD, -1, "", "", false });
    addObject(ATTIC, { OBJ_BOX, 550.f, 150.f, 32.f, "Stuck Chest", "Pry open", true, false, 0, -1, "", "", false });

    // --- Garden ---
    addObject(GARDEN, { OBJ_ITEM, 300.f, 220.f, 32.f, itemMeta[ID_STRANGE_COIN].name, "Pick up", true, false, ID_STRANGE_COIN, -1, "", "", false });
    addObject(GARDEN, { OBJ_STATUE, 900.f, 200.f, 30.f, "Weathered Statue", "Examine", true, false, 0, -1, "",
        "A statue worn smooth by rain. It might have been an angel once.", false });

    // --- Hidden Tunnel ---
    addObject(HIDDEN_TUNNEL, { OBJ_CANDLE, 570.f, 200.f, 30.f, "Old Candle", "Examine", true, false, 0, -1, "",
        "Wax pooled long ago. Strange - the wax is still soft.", false });

    // --- Ritual Room ---
    addObject(RITUAL_ROOM, { OBJ_SYMBOL, 570.f, 220.f, 32.f, "Burned Symbol", "Examine", true, false, 0, -1, "",
        "The same symbol as your Ritual Symbol. This place was built around it.", false });

    // --- Secret Room ---
    addObject(SECRET_ROOM, { OBJ_BOOKSHELF, 570.f, 200.f, 32.f, "Old Ledger", "Examine", true, false, 0, -1, "",
        "A ledger of names and dates, with one word repeated over and over: SLEEP.", false });
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

void Game::onEnterRoom(int roomId) {
    rollEventsAndThreatsForRoom(roomId);

    if (roomId == SECRET_ROOM && !secretRouteUnlocked) {
        secretRouteUnlocked = true;
        houseGraph.unlockEdge(SECRET_ROOM, GARDEN);
        setDoorActive(SECRET_ROOM, GARDEN, true);
        setDoorActive(GARDEN, SECRET_ROOM, true);
        setDoorActive(GARDEN, -1, true); // reveal the Foggy Path exit
        lastMessage = "Hidden documents reveal the house's darkest truth. A passage beyond the wall now leads to the Garden.";
        announceClue("The Secret Room reveals everything. A passage now leads back to the Garden.");
        triggerStory(6);
    }
}

void Game::announceClue(const std::string& text) {
    clueBannerText = "NEW CLUE: " + text;
    clueBannerTimer = 5.0f;
}

void Game::triggerStory(int index) {
    if (index < 0 || index >= STORY_BEAT_COUNT || storyShown[index]) return;
    storyShown[index] = true;
    storyText = storyBeats[index];
    storyTimer = 7.0f;
}

void Game::grantItem(int id) {
    Item item;
    item.id = id;
    item.name = itemMeta[id].name;
    item.description = itemMeta[id].description;
    player.addItem(item);

    // Diary Page and Old Photograph must be actively read (F) to
    // count as "discovered" - picking them up is not enough.
    if (id != ID_DIARY_PAGE && id != ID_OLD_PHOTOGRAPH) {
        clueDB.markDiscovered(id);
    }
}

void Game::craftIfPossible() {
    if (player.hasItem("Broken Handle") && player.hasItem("Metal Rod")) {
        player.useItem("Broken Handle");
        player.useItem("Metal Rod");
        grantItem(ID_MAKESHIFT_LEVER);
        pickupToastText = "+ Makeshift Lever crafted";
        pickupToastTimer = 2.5f;
        announceClue("You lash the rod and handle together into a Makeshift Lever.");
    }
}

// ------------------------------------------------------------------
// Per-frame update
// ------------------------------------------------------------------
void Game::update(float dt) {
    if (currentScreen != Screen::PLAYING) {
        return;
    }

    if (pickupToastTimer > 0.f) pickupToastTimer -= dt;
    if (clueBannerTimer > 0.f) clueBannerTimer -= dt;
    if (storyTimer > 0.f) storyTimer -= dt;

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
    triggerStory(0);
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
// Movement (spatial position + Graph + Stack)
// ------------------------------------------------------------------
void Game::movePlayer(float dx, float dy) {
    if (currentScreen != Screen::PLAYING) return;
    player.moveBy(dx, dy, ROOM_AREA_W, ROOM_AREA_H);
}

bool Game::goBack() {
    int previousRoom;
    if (player.goBack(previousRoom)) {
        lastMessage = "You go back to the " + getRoomData(previousRoom).name + ".";
        return true;
    }
    lastMessage = "There is nowhere to go back to.";
    return false;
}

// ------------------------------------------------------------------
// Interaction ('E') - proximity based
// ------------------------------------------------------------------
int Game::findNearestInteractable() const {
    int room = player.getCurrentRoom();
    float px = player.getX();
    float py = player.getY();

    int bestIdx = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < roomObjectCount[room]; i++) {
        const WorldObject& obj = roomObjects[room][i];
        if (!obj.active || obj.isDecorative) continue;
        float dx = obj.x - px;
        float dy = obj.y - py;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= obj.radius && dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

std::string Game::getInteractPrompt() const {
    int idx = findNearestInteractable();
    if (idx < 0) return "";
    int room = player.getCurrentRoom();
    const WorldObject& obj = roomObjects[room][idx];

    if (obj.type == OBJ_DOOR && obj.doorTargetRoom >= 0) {
        bool locked = !houseGraph.isPassable(room, obj.doorTargetRoom);
        if (locked && !obj.requiredItemName.empty() && player.hasItem(obj.requiredItemName)) {
            return "[E] Unlock " + obj.name + " with " + obj.requiredItemName;
        }
        return "[E] " + obj.verb + " " + obj.name + (locked ? " (locked)" : "");
    }
    if ((obj.type == OBJ_DRAWER || obj.type == OBJ_CABINET || obj.type == OBJ_WARDROBE) && obj.opened) {
        return "[E] Close " + obj.name;
    }
    return "[E] " + obj.verb + " " + obj.name;
}

void Game::interact() {
    int room = player.getCurrentRoom();
    int idx = findNearestInteractable();
    if (idx < 0) {
        lastMessage = "There is nothing to interact with here.";
        return;
    }
    WorldObject& obj = roomObjects[room][idx];

    switch (obj.type) {
        case OBJ_ITEM: {
            int id = obj.linkedItemId;
            if (id == ID_BASEMENT_KEY) {
                ClueInfo* diary = clueDB.search(ID_DIARY_PAGE);
                if (diary == nullptr || !diary->discovered) {
                    lastMessage = "A heavy iron key sits here, but you have no idea what it opens. (Try reading the Diary Page first)";
                    return;
                }
            }
            grantItem(id);
            obj.active = false;
            pickupToastText = "+ " + itemMeta[id].name + " added to inventory";
            pickupToastTimer = 2.5f;
            lastMessage = "You picked up: " + itemMeta[id].name;
            craftIfPossible();
            break;
        }

        case OBJ_DOOR: {
            int target = obj.doorTargetRoom;
            if (target == -1) {
                usedGardenEscape = (room == GARDEN);
                evaluateEndingAndFinish();
                break;
            }
            if (houseGraph.isPassable(room, target)) {
                bool firstRitualVisit = (target == RITUAL_ROOM && !player.hasVisited(RITUAL_ROOM));
                player.enterRoom(target);
                onEnterRoom(target);
                lastMessage = "You enter the " + getRoomData(target).name + ".";
                if (firstRitualVisit) triggerStory(4);
            } else if (room == RITUAL_ROOM && target == SECRET_ROOM) {
                ClueInfo* photo = clueDB.search(ID_OLD_PHOTOGRAPH);
                bool knowsPhoto = (photo != nullptr && photo->discovered);
                if (player.hasItem("Strange Coin") && knowsPhoto) {
                    houseGraph.unlockEdge(room, target);
                    player.useItem("Strange Coin");
                    player.enterRoom(target);
                    onEnterRoom(target);
                    lastMessage = "The photograph shows exactly where the coin fits. The wall grinds open, revealing the Secret Room.";
                } else if (!player.hasItem("Strange Coin")) {
                    lastMessage = "There is a coin-sized slot in this door.";
                } else {
                    lastMessage = "You have a coin, but the pattern makes no sense yet. (Study a photograph first)";
                }
            } else if (!obj.requiredItemName.empty() && player.hasItem(obj.requiredItemName)) {
                houseGraph.unlockEdge(room, target);
                player.useItem(obj.requiredItemName);
                std::string usedItem = obj.requiredItemName;
                bool firstRitualVisit = (target == RITUAL_ROOM && !player.hasVisited(RITUAL_ROOM));
                player.enterRoom(target);
                onEnterRoom(target);
                lastMessage = "You unlock the door with the " + usedItem + " and step through into the " +
                              getRoomData(target).name + ".";
                if (target == BASEMENT) triggerStory(2);
                if (firstRitualVisit) triggerStory(4);
            } else {
                lastMessage = "This door is locked.";
            }
            break;
        }

        case OBJ_DRAWER:
        case OBJ_CABINET:
        case OBJ_WARDROBE: {
            obj.opened = !obj.opened;
            if (obj.opened) {
                if (room == STUDY && obj.name == "Desk Drawer" && !studyDeskOpened) {
                    studyDeskOpened = true;
                    lastMessage = "You find a torn note tucked inside.";
                    announceClue("A note: 'Forgive me for what I did in the Ritual Room.'");
                } else if (room == KITCHEN && obj.name == "Kitchen Drawer") {
                    lastMessage = "Something skitters away as the drawer slides open!";
                    eventQueue.enqueue(makeEvent(WHISPER_HEARD, room));
                } else {
                    lastMessage = "Empty, except for dust.";
                }
            } else {
                if (room == LIBRARY && obj.name == "Writing Desk") {
                    lastMessage = "You close the drawer.";
                    eventQueue.enqueue(makeEvent(OBJECT_FALLS, room));
                } else {
                    lastMessage = "You close it.";
                }
            }
            break;
        }

        case OBJ_BOOKSHELF: {
            if (room == LIBRARY) {
                ClueInfo* diary = clueDB.search(ID_DIARY_PAGE);
                bool diaryRead = (diary != nullptr && diary->discovered);
                if (diaryRead && !obj.opened) {
                    obj.opened = true;
                    grantItem(ID_BROKEN_HANDLE);
                    pickupToastText = "+ Broken Handle added to inventory";
                    pickupToastTimer = 2.5f;
                    lastMessage = "The diary mentioned a loose book. You pull it - a Broken Handle falls out from behind the shelf!";
                    craftIfPossible();
                } else if (!diaryRead) {
                    lastMessage = "Rows of rotting books. Nothing stands out... yet.";
                } else {
                    lastMessage = "The loose book is already gone.";
                }
            } else {
                lastMessage = obj.inspectText.empty() ? "Just old books." : obj.inspectText;
            }
            break;
        }

        case OBJ_BOX: {
            if (obj.opened) {
                lastMessage = "The chest is already open and empty.";
            } else if (player.hasItem("Makeshift Lever")) {
                obj.opened = true;
                player.useItem("Makeshift Lever");
                grantItem(ID_TORN_NOTE);
                pickupToastText = "+ Torn Note added to inventory";
                pickupToastTimer = 2.5f;
                lastMessage = "You pry the chest open with the Makeshift Lever.";
                announceClue("Torn Note: '...if you find this, I am already gone. Don't trust the Ritual Room.'");
            } else {
                lastMessage = "The chest is jammed shut. You'd need something to pry it open.";
            }
            break;
        }

        case OBJ_CRACKED_WALL: {
            if (room == BASEMENT && !tunnelDiscovered) {
                houseGraph.dfsExplore(BASEMENT, dfsResult, dfsResultCount);
                tunnelDiscovered = true;
                houseGraph.unlockEdge(BASEMENT, HIDDEN_TUNNEL);
                setDoorActive(BASEMENT, HIDDEN_TUNNEL, true);
                setDoorActive(HIDDEN_TUNNEL, BASEMENT, true);
                lastMessage = "You search the damp walls and find a hidden lever! A passage to a Hidden Tunnel creaks open.";
                triggerStory(3);
            } else {
                houseGraph.dfsExplore(room, dfsResult, dfsResultCount);
                lastMessage = "You search the wall but find nothing new.";
            }
            break;
        }

        case OBJ_CLOCK: {
            if (room == ENTRANCE_HALL && !clockInspected) {
                clockInspected = true;
                announceClue("The clock is frozen at exactly 11:17. What happened at that time?");
            }
            lastMessage = obj.inspectText;
            break;
        }

        case OBJ_MIRROR: {
            lastMessage = (player.getFear() >= 40)
                ? "Your reflection doesn't blink when you do."
                : obj.inspectText;
            break;
        }

        case OBJ_PAINTING:
        case OBJ_STATUE:
        case OBJ_CANDLE:
        case OBJ_SYMBOL:
        default:
            lastMessage = obj.inspectText.empty() ? "Nothing more to see here." : obj.inspectText;
            break;
    }
}

// ------------------------------------------------------------------
// Inspect ('F') - reads a held-but-unread clue item (BST)
// ------------------------------------------------------------------
void Game::inspectOrSearch() {
    ClueInfo* diary = clueDB.search(ID_DIARY_PAGE);
    if (player.hasItem("Diary Page") && diary != nullptr && !diary->discovered) {
        clueDB.markDiscovered(ID_DIARY_PAGE);
        announceClue(diary->description);
        lastMessage = "Diary Page: \"" + diary->description + "\"";
        triggerStory(1);
        return;
    }

    ClueInfo* photo = clueDB.search(ID_OLD_PHOTOGRAPH);
    if (player.hasItem("Old Photograph") && photo != nullptr && !photo->discovered) {
        clueDB.markDiscovered(ID_OLD_PHOTOGRAPH);
        announceClue(photo->description);
        lastMessage = "Old Photograph: \"" + photo->description + "\"";
        triggerStory(5);
        return;
    }

    lastMessage = "Nothing new to read right now.";
}

// ------------------------------------------------------------------
// Ending resolution
// ------------------------------------------------------------------
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
// BFS hint system ('H') + persistent objective label
// ------------------------------------------------------------------
Objective Game::getObjective() const {
    if (!player.hasItem("Diary Page")) return Objective::GET_DIARY;
    if (!player.hasItem("Rusty Key") && houseGraph.isLocked(LIBRARY, STUDY)) return Objective::GET_RUSTY_KEY;
    if (houseGraph.isLocked(LIBRARY, STUDY)) return Objective::UNLOCK_STUDY;
    if (!player.hasItem("Basement Key") && houseGraph.isLocked(KITCHEN, BASEMENT)) return Objective::GET_BASEMENT_KEY;
    if (!player.hasItem("Torch")) return Objective::GET_TORCH;
    if (houseGraph.isLocked(KITCHEN, BASEMENT)) return Objective::UNLOCK_BASEMENT;
    if (!player.hasItem("Ritual Symbol") && houseGraph.isLocked(HIDDEN_TUNNEL, RITUAL_ROOM)) return Objective::GET_RITUAL_SYMBOL;
    if (!tunnelDiscovered) return Objective::SEARCH_BASEMENT;
    if (houseGraph.isLocked(HIDDEN_TUNNEL, RITUAL_ROOM)) return Objective::UNLOCK_RITUAL;
    if (!player.hasVisited(RITUAL_ROOM)) return Objective::ENTER_RITUAL;
    if (!player.hasItem("Old Photograph")) return Objective::GET_PHOTOGRAPH;
    if (!player.hasItem("Strange Coin")) return Objective::GET_COIN;
    if (houseGraph.isLocked(RITUAL_ROOM, SECRET_ROOM)) return Objective::UNLOCK_SECRET;
    if (!player.hasVisited(SECRET_ROOM)) return Objective::ENTER_SECRET;
    return Objective::GO_TO_EXIT;
}

std::string Game::objectiveLabel(Objective obj) const {
    switch (obj) {
        case Objective::GET_DIARY:        return "Find the Diary Page (Library)";
        case Objective::GET_RUSTY_KEY:    return "Find the Rusty Key (Living Room)";
        case Objective::UNLOCK_STUDY:     return "Unlock the Study door";
        case Objective::GET_BASEMENT_KEY: return "Find the Basement Key (Study)";
        case Objective::GET_TORCH:        return "Find the Torch (Kitchen)";
        case Objective::UNLOCK_BASEMENT:  return "Unlock the Basement door";
        case Objective::GET_RITUAL_SYMBOL:return "Find the Ritual Symbol (Basement)";
        case Objective::SEARCH_BASEMENT:  return "Search the Basement for hidden passages";
        case Objective::UNLOCK_RITUAL:    return "Unlock the way to the Ritual Room";
        case Objective::ENTER_RITUAL:     return "Enter the Ritual Room";
        case Objective::GET_PHOTOGRAPH:   return "Find the Old Photograph (Attic)";
        case Objective::GET_COIN:         return "Find the Strange Coin (Garden)";
        case Objective::UNLOCK_SECRET:    return "Unlock the Secret Room";
        case Objective::ENTER_SECRET:     return "Enter the Secret Room";
        case Objective::GO_TO_EXIT:       return "Find a way out";
    }
    return "";
}

std::string Game::getObjectiveLabel() const {
    return objectiveLabel(getObjective());
}

void Game::requestHint() {
    Objective obj = getObjective();
    int target = ENTRANCE_HALL;

    switch (obj) {
        case Objective::GET_DIARY:        target = LIBRARY; break;
        case Objective::GET_RUSTY_KEY:    target = LIVING_ROOM; break;
        case Objective::UNLOCK_STUDY:     target = LIBRARY; break;
        case Objective::GET_BASEMENT_KEY: target = STUDY; break;
        case Objective::GET_TORCH:        target = KITCHEN; break;
        case Objective::UNLOCK_BASEMENT:  target = KITCHEN; break;
        case Objective::GET_RITUAL_SYMBOL:target = BASEMENT; break;
        case Objective::SEARCH_BASEMENT:  target = BASEMENT; break;
        case Objective::UNLOCK_RITUAL:    target = HIDDEN_TUNNEL; break;
        case Objective::ENTER_RITUAL:     target = RITUAL_ROOM; break;
        case Objective::GET_PHOTOGRAPH:   target = ATTIC; break;
        case Objective::GET_COIN:         target = GARDEN; break;
        case Objective::UNLOCK_SECRET:    target = RITUAL_ROOM; break;
        case Objective::ENTER_SECRET:     target = SECRET_ROOM; break;
        case Objective::GO_TO_EXIT:       target = ENTRANCE_HALL; break;
    }

    std::string label = objectiveLabel(obj);
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
// World objects (for rendering)
// ------------------------------------------------------------------
const WorldObject* Game::getRoomObjects(int room, int& outCount) const {
    outCount = roomObjectCount[room];
    return roomObjects[room];
}

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
std::string Game::getItemName(int id) const { return itemMeta[id].name; }
std::string Game::getItemDescription(int id) const { return itemMeta[id].description; }

std::string Game::getPickupToast(float& outTimer) const {
    outTimer = pickupToastTimer;
    return pickupToastText;
}

std::string Game::getClueBanner(float& outTimer) const {
    outTimer = clueBannerTimer;
    return clueBannerText;
}

std::string Game::getStoryBanner(float& outTimer) const {
    outTimer = storyTimer;
    return storyText;
}
