// ---------------------------------------------------------------------
// main.cpp - SFML presentation layer for The Crimson Sleep.
//
// IMPORTANT: this file contains NO data structure logic of its own.
// Every data structure (Graph, Stack, Queue, PriorityQueue,
// LinkedList, BST) lives in Game/Player and is only *read* here to
// decide what to draw. This keeps the gameplay/data-structure code
// completely testable without a graphics library (see the console
// tests used during development).
// ---------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include "Game.h"
#include "AudioManager.h"
#include "Room.h"
#include <string>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <optional>

static const unsigned int WIN_W = 1200;
static const unsigned int WIN_H = 750;

// --- small text helper -------------------------------------------------
static std::string wrapText(const std::string& text, size_t maxCharsPerLine) {
    std::string result;
    size_t lineLen = 0;
    size_t i = 0;
    while (i < text.size()) {
        size_t nextSpace = text.find(' ', i);
        if (nextSpace == std::string::npos) nextSpace = text.size();
        std::string word = text.substr(i, nextSpace - i);

        if (lineLen + word.size() + 1 > maxCharsPerLine && lineLen > 0) {
            result += '\n';
            lineLen = 0;
        }
        result += word + ' ';
        lineLen += word.size() + 1;
        i = nextSpace + 1;
    }
    return result;
}

static void drawText(sf::RenderWindow& window, sf::Font& font, const std::string& str,
                      float x, float y, unsigned int size, sf::Color color, bool bold = false) {
    sf::Text text(font, str, size);
    text.setFillColor(color);
    text.setPosition({x, y});
    if (bold) text.setStyle(sf::Text::Bold);
    window.draw(text);
}

static void drawBar(sf::RenderWindow& window, sf::Font& font, const std::string& label,
                     float x, float y, float w, float h, int value, int maxValue, sf::Color color) {
    sf::RectangleShape bg({w, h});
    bg.setPosition({x, y});
    bg.setFillColor(sf::Color(35, 35, 38));
    bg.setOutlineThickness(2);
    bg.setOutlineColor(sf::Color(90, 90, 90));
    window.draw(bg);

    float frac = (float)value / (float)maxValue;
    if (frac < 0.f) frac = 0.f;
    if (frac > 1.f) frac = 1.f;
    sf::RectangleShape fill({w * frac, h});
    fill.setPosition({x, y});
    fill.setFillColor(color);
    window.draw(fill);

    drawText(window, font, label + ": " + std::to_string(value) + "/" + std::to_string(maxValue),
              x + 6, y + h / 2 - 9, 14, sf::Color::White);
}

static float w_half(float w) { return w / 2.f; }

// --- room-specific decorative shapes (pixel/silhouette style) ---------
static void drawRoomDecorations(sf::RenderWindow& window, int room, float x, float y, float w, float h) {
    (void)w;
    switch (room) {
        case BASEMENT:
        case HIDDEN_TUNNEL: {
            for (int i = 0; i < 4; i++) {
                sf::RectangleShape chain({4.f, 60.f});
                chain.setPosition({x + 60.f + i * 70.f, y});
                chain.setFillColor(sf::Color(15, 15, 15));
                window.draw(chain);
            }
            break;
        }
        case LIBRARY:
        case STUDY: {
            for (int i = 0; i < 5; i++) {
                sf::RectangleShape shelf({30.f, 90.f});
                shelf.setPosition({x + 30.f + i * 45.f, y + 20.f});
                shelf.setFillColor(sf::Color(45, 30, 20));
                shelf.setOutlineThickness(1);
                shelf.setOutlineColor(sf::Color(20, 15, 10));
                window.draw(shelf);
            }
            break;
        }
        case RITUAL_ROOM: {
            sf::CircleShape circle(70.f);
            circle.setPosition({x + w_half(w) - 70.f, y + 100.f});
            circle.setFillColor(sf::Color::Transparent);
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color(150, 20, 20));
            window.draw(circle);
            for (int i = 0; i < 6; i++) {
                float angle = i * 60.f * 3.14159f / 180.f;
                sf::CircleShape candle(5.f);
                candle.setPosition({x + w_half(w) + std::cos(angle) * 90.f,
                                     y + 170.f + std::sin(angle) * 90.f});
                candle.setFillColor(sf::Color(220, 180, 60));
                window.draw(candle);
            }
            break;
        }
        case GARDEN: {
            for (int i = 0; i < 6; i++) {
                sf::CircleShape fog(40.f + (i % 3) * 10.f);
                fog.setPosition({x + 20.f + i * 110.f, y + 150.f + (i % 2) * 40.f});
                fog.setFillColor(sf::Color(200, 200, 210, 40));
                window.draw(fog);
            }
            break;
        }
        case KITCHEN: {
            sf::RectangleShape stove({100.f, 60.f});
            stove.setPosition({x + 40.f, y + 220.f});
            stove.setFillColor(sf::Color(50, 50, 55));
            window.draw(stove);
            break;
        }
        case ATTIC: {
            for (int i = 0; i < 10; i++) {
                sf::CircleShape dust(2.f);
                dust.setPosition({x + (float)(rand() % (int)w), y + (float)(rand() % 200)});
                dust.setFillColor(sf::Color(180, 180, 160, 120));
                window.draw(dust);
            }
            break;
        }
        default:
            break;
    }
}

// --- player silhouette --------------------------------------------------
static void drawPlayerSilhouette(sf::RenderWindow& window, float centerX, float bottomY) {
    sf::CircleShape head(13.f);
    head.setPosition({centerX - 13.f, bottomY - 60.f});
    head.setFillColor(sf::Color(12, 12, 12));
    window.draw(head);

    sf::RectangleShape body({26.f, 42.f});
    body.setPosition({centerX - 13.f, bottomY - 44.f});
    body.setFillColor(sf::Color(12, 12, 12));
    window.draw(body);
}

// --- inventory overlay (Linked List) ------------------------------------
static void drawInventoryOverlay(sf::RenderWindow& window, sf::Font& font, Game& game) {
    sf::RectangleShape box({520.f, 420.f});
    box.setPosition({340.f, 150.f});
    box.setFillColor(sf::Color(15, 15, 18, 235));
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color(150, 30, 30));
    window.draw(box);

    drawText(window, font, "INVENTORY", 360.f, 165.f, 22, sf::Color(220, 180, 100), true);
    drawText(window, font, "(Linked List - press I to close)", 360.f, 195.f, 13, sf::Color(140, 140, 140));

    Item items[20];
    int count = 0;
    game.getPlayer().getInventory().getContents(items, 20, count);

    if (count == 0) {
        drawText(window, font, "Empty. Explore the house to find items.", 360.f, 230.f, 15, sf::Color(180, 180, 180));
    } else {
        for (int i = 0; i < count; i++) {
            drawText(window, font, "- " + items[i].name, 360.f, 230.f + i * 50.f, 17, sf::Color::White, true);
            drawText(window, font, wrapText(items[i].description, 70), 375.f, 250.f + i * 50.f, 12, sf::Color(170, 170, 170));
        }
    }
}

// --- data structure debug/demo panel -------------------------------------
static void drawDebugPanel(sf::RenderWindow& window, sf::Font& font, Game& game) {
    sf::RectangleShape box({1140.f, 640.f});
    box.setPosition({30.f, 55.f});
    box.setFillColor(sf::Color(8, 8, 10, 240));
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color(80, 150, 80));
    window.draw(box);

    drawText(window, font, "DATA STRUCTURE DEBUG PANEL (TAB to close)", 45.f, 65.f, 18, sf::Color(120, 220, 120), true);

    float leftX = 45.f, rightX = 610.f;
    float y = 100.f;
    const Player& player = game.getPlayer();

    // GRAPH
    drawText(window, font, "GRAPH - neighbors of current room:", leftX, y, 14, sf::Color(200, 200, 255), true);
    int neighbors[MAX_ROOMS]; bool passable[MAX_ROOMS]; int ncount;
    game.getCurrentExits(neighbors, passable, MAX_ROOMS, ncount);
    std::string neighborLine;
    for (int i = 0; i < ncount; i++) {
        neighborLine += getRoomData(neighbors[i]).name + (passable[i] ? "(open) " : "(locked) ");
    }
    if (neighborLine.empty()) neighborLine = "(none)";
    drawText(window, font, wrapText(neighborLine, 60), leftX, y + 18, 12, sf::Color(220, 220, 220));
    y += 60.f;

    // STACK
    drawText(window, font, "STACK - movement history (top -> bottom):", leftX, y, 14, sf::Color(200, 200, 255), true);
    int hist[100]; int histCount;
    player.getHistory().getContents(hist, 100, histCount);
    std::string histLine;
    for (int i = 0; i < histCount && i < 12; i++) histLine += getRoomData(hist[i]).name + " -> ";
    drawText(window, font, wrapText(histLine, 60), leftX, y + 18, 12, sf::Color(220, 220, 220));
    y += 60.f;

    // QUEUE
    drawText(window, font, "QUEUE - pending haunted events (FIFO):", leftX, y, 14, sf::Color(200, 200, 255), true);
    Event events[50]; int eventCount;
    game.getEventQueue().getContents(events, 50, eventCount);
    std::string eventLine;
    for (int i = 0; i < eventCount; i++) eventLine += eventTypeName(events[i].type) + ", ";
    if (eventLine.empty()) eventLine = "(empty)";
    drawText(window, font, wrapText(eventLine, 60), leftX, y + 18, 12, sf::Color(220, 220, 220));
    y += 60.f;

    // PRIORITY QUEUE
    drawText(window, font, "PRIORITY QUEUE - active threats (max-heap):", leftX, y, 14, sf::Color(200, 200, 255), true);
    Threat threats[50]; int threatCount;
    game.getThreatQueue().getContents(threats, 50, threatCount);
    std::string threatLine;
    for (int i = 0; i < threatCount; i++) {
        threatLine += threatTypeName(threats[i].type) + "(p" + std::to_string(threats[i].priority) + ") ";
    }
    if (threatLine.empty()) threatLine = "(empty)";
    drawText(window, font, wrapText(threatLine, 60), leftX, y + 18, 12, sf::Color(220, 220, 220));

    // RIGHT COLUMN
    float ry = 100.f;
    drawText(window, font, "LINKED LIST - inventory:", rightX, ry, 14, sf::Color(255, 220, 180), true);
    Item items[20]; int itemCount;
    player.getInventory().getContents(items, 20, itemCount);
    std::string itemLine;
    for (int i = 0; i < itemCount; i++) itemLine += items[i].name + ", ";
    if (itemLine.empty()) itemLine = "(empty)";
    drawText(window, font, wrapText(itemLine, 60), rightX, ry + 18, 12, sf::Color(220, 220, 220));
    ry += 60.f;

    drawText(window, font, "BST - clue database (in-order by id):", rightX, ry, 14, sf::Color(255, 220, 180), true);
    ClueInfo clues[20]; int clueCount;
    game.getClueDB().getContentsInOrder(clues, 20, clueCount);
    std::string clueText;
    for (int i = 0; i < clueCount; i++) {
        clueText += "#" + std::to_string(clues[i].id) + " " + clues[i].name +
                    (clues[i].discovered ? "[known] " : "[unread] ");
    }
    drawText(window, font, wrapText(clueText, 55), rightX, ry + 18, 12, sf::Color(220, 220, 220));
    ry += 90.f;

    drawText(window, font, "BFS - last computed hint path:", rightX, ry, 14, sf::Color(255, 220, 180), true);
    std::string bfsLine;
    for (int i = 0; i < game.getHintPathLen(); i++) bfsLine += getRoomData(game.getHintPath()[i]).name + " -> ";
    if (bfsLine.empty()) bfsLine = "(press H to compute a hint)";
    drawText(window, font, wrapText(bfsLine, 55), rightX, ry + 18, 12, sf::Color(220, 220, 220));
    ry += 60.f;

    drawText(window, font, "DFS - last exploration result:", rightX, ry, 14, sf::Color(255, 220, 180), true);
    std::string dfsLine;
    for (int i = 0; i < game.getDFSResultCount(); i++) dfsLine += getRoomData(game.getDFSResult()[i]).name + " ";
    if (dfsLine.empty()) dfsLine = "(press F in the Basement to search)";
    drawText(window, font, wrapText(dfsLine, 55), rightX, ry + 18, 12, sf::Color(220, 220, 220));
}

// --- main gameplay screen -------------------------------------------------
static void drawPlaying(sf::RenderWindow& window, sf::Font& font, Game& game,
                         bool showInventory, bool showDebugPanel, float hintTimer) {
    const Player& player = game.getPlayer();
    int room = player.getCurrentRoom();
    const RoomData& rd = getRoomData(room);

    drawText(window, font, rd.name, 30.f, 12.f, 30, sf::Color(210, 40, 40), true);
    drawText(window, font, wrapText(rd.description, 95), 30.f, 50.f, 14, sf::Color(170, 170, 170));

    // Room visualization box
    float boxX = 30.f, boxY = 105.f, boxW = 740.f, boxH = 330.f;
    sf::RectangleShape roomBox({boxW, boxH});
    roomBox.setPosition({boxX, boxY});
    int danger = rd.baseDangerLevel;
    int shade = 34 - danger * 2; if (shade < 8) shade = 8;
    roomBox.setFillColor(sf::Color((std::uint8_t)(shade + danger), (std::uint8_t)shade, (std::uint8_t)shade));
    roomBox.setOutlineThickness(3);
    roomBox.setOutlineColor(sf::Color(100, 25, 25));
    window.draw(roomBox);

    drawRoomDecorations(window, room, boxX, boxY, boxW, boxH);
    drawPlayerSilhouette(window, boxX + boxW / 2.f, boxY + boxH - 30.f);

    // Exits panel
    float exX = 790.f, exY = 105.f, exW = 380.f, exH = 330.f;
    sf::RectangleShape exPanel({exW, exH});
    exPanel.setPosition({exX, exY});
    exPanel.setFillColor(sf::Color(20, 20, 22));
    exPanel.setOutlineThickness(2);
    exPanel.setOutlineColor(sf::Color(80, 80, 80));
    window.draw(exPanel);
    drawText(window, font, "DOORS", exX + 10.f, exY + 6.f, 16, sf::Color(180, 180, 180), true);
    drawText(window, font, "Up/Down select, Enter to move", exX + 10.f, exY + 28.f, 11, sf::Color(120, 120, 120));

    int neighbors[MAX_ROOMS]; bool passable[MAX_ROOMS]; int count;
    game.getCurrentExits(neighbors, passable, MAX_ROOMS, count);
    int selected = game.getSelectedExitIndex();

    if (count == 0) {
        drawText(window, font, "No visible doors.", exX + 15.f, exY + 55.f, 14, sf::Color(150, 150, 150));
    }
    for (int i = 0; i < count && i < 8; i++) {
        float rowY = exY + 48.f + i * 34.f;
        sf::RectangleShape rowBg({exW - 20.f, 28.f});
        rowBg.setPosition({exX + 10.f, rowY});
        bool isSel = (i == selected);
        rowBg.setFillColor(isSel ? sf::Color(110, 25, 25) : sf::Color(32, 32, 34));
        window.draw(rowBg);

        std::string label = (passable[i] ? "-> " : "[locked] ") + getRoomData(neighbors[i]).name;
        drawText(window, font, label, exX + 18.f, rowY + 4.f, 15, passable[i] ? sf::Color::White : sf::Color(130, 130, 130));
    }

    // Health / Fear bars
    drawBar(window, font, "HEALTH", 30.f, 455.f, 350.f, 26.f, player.getHealth(), 100, sf::Color(190, 25, 25));
    drawBar(window, font, "FEAR", 400.f, 455.f, 350.f, 26.f, player.getFear(), 100, sf::Color(130, 30, 170));

    // Message log
    drawText(window, font, "You: " + game.getLastMessage(), 30.f, 500.f, 14, sf::Color(225, 215, 170));
    std::string evMsg = game.getLastEventMessage();
    drawText(window, font, "Event: " + (evMsg.empty() ? "(quiet so far)" : evMsg), 30.f, 522.f, 14, sf::Color(160, 185, 225));
    std::string thMsg = game.getLastThreatMessage();
    drawText(window, font, "Threat: " + (thMsg.empty() ? "(nothing yet)" : thMsg), 30.f, 544.f, 14, sf::Color(225, 140, 140));

    // Hint banner
    if (hintTimer > 0.f) {
        sf::RectangleShape hintBox({1140.f, 40.f});
        hintBox.setPosition({30.f, 575.f});
        hintBox.setFillColor(sf::Color(45, 40, 10, 230));
        hintBox.setOutlineThickness(1);
        hintBox.setOutlineColor(sf::Color(180, 150, 40));
        window.draw(hintBox);
        drawText(window, font, "HINT (BFS): " + game.getHintMessage(), 40.f, 585.f, 15, sf::Color(255, 225, 120), true);
    }

    // Controls strip
    sf::RectangleShape ctrlStrip({(float)WIN_W, 40.f});
    ctrlStrip.setPosition({0.f, 710.f});
    ctrlStrip.setFillColor(sf::Color(14, 14, 16));
    window.draw(ctrlStrip);
    drawText(window, font,
        "Move: Arrows/WASD  Enter: Go  E: Interact  F: Inspect/Search  H: Hint  B: Go Back  I: Inventory  TAB: DS Panel  ESC: Pause",
        10.f, 720.f, 12, sf::Color(150, 150, 150));

    if (showInventory) drawInventoryOverlay(window, font, game);
    if (showDebugPanel) drawDebugPanel(window, font, game);
}

static void drawPauseOverlay(sf::RenderWindow& window, sf::Font& font) {
    sf::RectangleShape dim({(float)WIN_W, (float)WIN_H});
    dim.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(dim);
    drawText(window, font, "PAUSED", WIN_W / 2.f - 90.f, WIN_H / 2.f - 60.f, 40, sf::Color::White, true);
    drawText(window, font, "ESC to resume   |   M for Main Menu", WIN_W / 2.f - 160.f, WIN_H / 2.f + 5.f, 16, sf::Color(200, 200, 200));
}

static void drawMainMenu(sf::RenderWindow& window, sf::Font& font, float flicker) {
    window.clear(sf::Color(6, 6, 8));
    std::uint8_t alpha = (std::uint8_t)(200 + 55 * std::sin(flicker * 3.0f));
    drawText(window, font, "THE CRIMSON SLEEP", WIN_W / 2.f - 300.f, 190.f, 52, sf::Color(190, 20, 20, alpha), true);
    drawText(window, font, "A Data Structures Laboratory Project - Horror Escape", WIN_W / 2.f - 230.f, 260.f, 15, sf::Color(150, 150, 150));

    drawText(window, font, "ENTER - Start Game", WIN_W / 2.f - 110.f, 400.f, 22, sf::Color::White);
    drawText(window, font, "C - Controls", WIN_W / 2.f - 110.f, 440.f, 22, sf::Color(200, 200, 200));
    drawText(window, font, "ESC - Quit", WIN_W / 2.f - 110.f, 480.f, 22, sf::Color(200, 200, 200));

    drawText(window, font, "Explore 13 rooms. Manage Health and Fear. Escape... or don't.", WIN_W / 2.f - 260.f, 560.f, 14, sf::Color(120, 120, 120));
}

static void drawControls(sf::RenderWindow& window, sf::Font& font) {
    window.clear(sf::Color(6, 6, 8));
    drawText(window, font, "CONTROLS", 60.f, 50.f, 34, sf::Color(200, 40, 40), true);

    struct Line { const char* key; const char* desc; };
    Line lines[] = {
        {"Arrow Keys / W A S D", "Select a door (Up/Down or Left/Right)"},
        {"Enter / Space", "Move through the selected door (Graph movement)"},
        {"B", "Go Back to the previous room (Stack)"},
        {"E", "Interact - pick up items, unlock doors, use exits"},
        {"F", "Inspect a held clue / search the room for secrets (BST + DFS)"},
        {"H", "Request a hint toward your next goal (BFS)"},
        {"I", "Toggle Inventory panel (Linked List)"},
        {"TAB", "Toggle the Data Structures debug panel"},
        {"ESC", "Pause / Resume"},
    };
    float y = 130.f;
    for (auto& l : lines) {
        drawText(window, font, l.key, 80.f, y, 18, sf::Color(255, 210, 120), true);
        drawText(window, font, l.desc, 420.f, y, 16, sf::Color(210, 210, 210));
        y += 45.f;
    }
    drawText(window, font, "Press any key to return to the Main Menu", 60.f, y + 40.f, 16, sf::Color(150, 150, 150));
}

static void drawGameOver(sf::RenderWindow& window, sf::Font& font, Game& game) {
    bool byFear = (game.getGameOverReason() == GameOverReason::FEAR);
    window.clear(byFear ? sf::Color(20, 5, 25) : sf::Color(25, 4, 4));

    std::string title = byFear ? "LOST TO MADNESS" : "YOU DIED";
    drawText(window, font, title, WIN_W / 2.f - (float)title.size() * 15.f, 260.f, 46, sf::Color(230, 40, 40), true);

    std::string flavor = byFear
        ? "The fear consumed you completely. The house keeps your screams forever."
        : "Your body falls still in the dark. The house has claimed another guest.";
    drawText(window, font, wrapText(flavor, 60), WIN_W / 2.f - 320.f, 340.f, 16, sf::Color(190, 190, 190));

    drawText(window, font, "Press ENTER to return to the Main Menu", WIN_W / 2.f - 200.f, 450.f, 16, sf::Color(150, 150, 150));
}

static void drawEnding(sf::RenderWindow& window, sf::Font& font, Game& game) {
    EndingType e = game.getEndingType();
    sf::Color bg, titleColor;
    std::string title, body;

    switch (e) {
        case EndingType::ESCAPE:
            bg = sf::Color(10, 12, 20); titleColor = sf::Color(120, 160, 220);
            title = "ESCAPE ENDING";
            body = "You stumble out the front door into the cold night air. You survived the Crimson Sleep - but the house, and everything in it, will haunt your dreams for years to come.";
            break;
        case EndingType::TRAPPED:
            bg = sf::Color(15, 4, 4); titleColor = sf::Color(220, 40, 40);
            title = "TRAPPED ENDING";
            body = "The front door will not open. You never faced what waited in the Ritual Room, and the house is not finished with you. The Crimson Sleep pulls you back into the dark, forever.";
            break;
        case EndingType::SECRET:
            bg = sf::Color(6, 16, 12); titleColor = sf::Color(90, 210, 150);
            title = "SECRET ENDING";
            body = "You slip through the fog behind the Secret Room and vanish into the garden mist. No one will ever know what happened to you - but you are free.";
            break;
        case EndingType::TRUE_END:
            bg = sf::Color(18, 16, 4); titleColor = sf::Color(240, 200, 90);
            title = "TRUE ENDING";
            body = "You uncover the house's darkest secret, keep your mind intact, and walk out the front door with proof of everything. The Crimson Sleep finally, truly, ends.";
            break;
        default:
            bg = sf::Color::Black; titleColor = sf::Color::White; title = "THE END"; body = "";
            break;
    }

    window.clear(bg);
    drawText(window, font, title, WIN_W / 2.f - (float)title.size() * 13.f, 240.f, 40, titleColor, true);
    drawText(window, font, wrapText(body, 60), WIN_W / 2.f - 320.f, 320.f, 16, sf::Color(210, 210, 210));
    drawText(window, font, "Press ENTER to return to the Main Menu", WIN_W / 2.f - 200.f, 460.f, 16, sf::Color(150, 150, 150));
}

static void drawJumpscare(sf::RenderWindow& window, float timer) {
    float t = timer / 0.45f;
    if (t > 1.f) t = 1.f;

    sf::RectangleShape flash({(float)WIN_W, (float)WIN_H});
    bool white = (((int)(timer * 20)) % 2 == 0);
    flash.setFillColor(white ? sf::Color(255, 255, 255, (std::uint8_t)(180 * t))
                              : sf::Color(120, 0, 0, (std::uint8_t)(200 * t)));
    window.draw(flash);

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    sf::CircleShape head(120.f * t + 40.f);
    head.setOrigin({head.getRadius(), head.getRadius()});
    head.setPosition({cx, cy});
    head.setFillColor(sf::Color(5, 5, 5, (std::uint8_t)(230 * t)));
    window.draw(head);

    sf::CircleShape eyeL(14.f * t + 6.f), eyeR(14.f * t + 6.f);
    eyeL.setFillColor(sf::Color(255, 255, 255, (std::uint8_t)(255 * t)));
    eyeR.setFillColor(sf::Color(255, 255, 255, (std::uint8_t)(255 * t)));
    eyeL.setOrigin({eyeL.getRadius(), eyeL.getRadius()});
    eyeR.setOrigin({eyeR.getRadius(), eyeR.getRadius()});
    eyeL.setPosition({cx - 35.f, cy - 20.f});
    eyeR.setPosition({cx + 35.f, cy - 20.f});
    window.draw(eyeL);
    window.draw(eyeR);

    sf::ConvexShape mouth(4);
    mouth.setPoint(0, {cx - 40.f, cy + 30.f});
    mouth.setPoint(1, {cx + 40.f, cy + 30.f});
    mouth.setPoint(2, {cx + 25.f, cy + 60.f});
    mouth.setPoint(3, {cx - 25.f, cy + 60.f});
    mouth.setFillColor(sf::Color(150, 0, 0, (std::uint8_t)(255 * t)));
    window.draw(mouth);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WIN_W, WIN_H}), "The Crimson Sleep");
    window.setFramerateLimit(60);

    sf::Font font;
    bool loaded = font.openFromFile("assets/fonts/DejaVuSans.ttf");
    if (!loaded) loaded = font.openFromFile("../assets/fonts/DejaVuSans.ttf");
    if (!loaded) {
        // Still run; SFML simply won't render text glyphs without a font.
    }

    AudioManager audio;
    Game* game = new Game();

    bool showInventory = false;
    bool showDebugPanel = false;
    float hintTimer = 0.f;
    float menuFlicker = 0.f;

    int lastEventSeq = 0;
    int lastThreatSeq = 0;
    Screen lastScreen = game->getScreen();

    float jumpscareTimer = 0.f;
    float shakeTimer = 0.f;
    float dimTimer = 0.f;

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        menuFlicker += dt;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                sf::Keyboard::Key code = key->code;
                Screen scr = game->getScreen();

                if (scr == Screen::MAIN_MENU) {
                    if (code == sf::Keyboard::Key::Enter) {
                        delete game;
                        game = new Game();
                        game->startNewGame();
                        showInventory = false;
                        showDebugPanel = false;
                        hintTimer = 0.f;
                        audio.playAmbientDrone();
                    } else if (code == sf::Keyboard::Key::C) {
                        game->showControls();
                    } else if (code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                } else if (scr == Screen::CONTROLS) {
                    game->goToMainMenu();
                } else if (scr == Screen::PLAYING) {
                    if (code == sf::Keyboard::Key::Up || code == sf::Keyboard::Key::W ||
                        code == sf::Keyboard::Key::Left || code == sf::Keyboard::Key::A) {
                        game->selectPrevExit();
                    } else if (code == sf::Keyboard::Key::Down || code == sf::Keyboard::Key::S ||
                               code == sf::Keyboard::Key::Right || code == sf::Keyboard::Key::D) {
                        game->selectNextExit();
                    } else if (code == sf::Keyboard::Key::Enter || code == sf::Keyboard::Key::Space) {
                        if (game->confirmMove()) audio.playDoor();
                    } else if (code == sf::Keyboard::Key::E) {
                        game->interact();
                        audio.playClick();
                    } else if (code == sf::Keyboard::Key::F) {
                        game->inspectOrSearch();
                        audio.playClick();
                    } else if (code == sf::Keyboard::Key::H) {
                        game->requestHint();
                        hintTimer = 6.f;
                    } else if (code == sf::Keyboard::Key::B) {
                        if (game->goBack()) audio.playDoor();
                    } else if (code == sf::Keyboard::Key::I) {
                        showInventory = !showInventory;
                    } else if (code == sf::Keyboard::Key::Tab) {
                        showDebugPanel = !showDebugPanel;
                    } else if (code == sf::Keyboard::Key::Escape) {
                        game->togglePause();
                    }
                } else if (scr == Screen::PAUSED) {
                    if (code == sf::Keyboard::Key::Escape) {
                        game->togglePause();
                    } else if (code == sf::Keyboard::Key::M) {
                        game->goToMainMenu();
                        audio.stopAmbient();
                    }
                } else if (scr == Screen::GAME_OVER || scr == Screen::ENDING) {
                    if (code == sf::Keyboard::Key::Enter) {
                        game->goToMainMenu();
                        audio.stopAmbient();
                    }
                }
            }
        }

        game->update(dt);

        // --- reactive audio / jumpscares based on data-structure activity ---
        if (game->getScreen() == Screen::PLAYING) {
            if (game->getEventSeq() != lastEventSeq) {
                lastEventSeq = game->getEventSeq();
                dimTimer = 0.25f;
            }
            if (game->getThreatSeq() != lastThreatSeq) {
                lastThreatSeq = game->getThreatSeq();
                ThreatType tt = game->getLastThreatType();
                if (tt == THREAT_GHOST_ATTACK) {
                    jumpscareTimer = 0.45f;
                    shakeTimer = 0.45f;
                    audio.playJumpscareStinger();
                } else if (tt == THREAT_GHOST_NEARBY) {
                    shakeTimer = 0.2f;
                    audio.playWhisper();
                } else if (tt == THREAT_TRAP) {
                    shakeTimer = 0.25f;
                    audio.playDoor();
                } else {
                    dimTimer = 0.3f;
                    audio.playWhisper();
                }
            }
        }

        if (lastScreen != Screen::GAME_OVER && game->getScreen() == Screen::GAME_OVER) {
            jumpscareTimer = 0.5f;
            audio.playJumpscareStinger();
            audio.stopAmbient();
        }
        if (lastScreen != Screen::ENDING && game->getScreen() == Screen::ENDING) {
            audio.stopAmbient();
        }
        lastScreen = game->getScreen();

        if (hintTimer > 0.f) hintTimer -= dt;
        if (jumpscareTimer > 0.f) jumpscareTimer -= dt;
        if (shakeTimer > 0.f) shakeTimer -= dt;
        if (dimTimer > 0.f) dimTimer -= dt;

        // --- render ---
        sf::View view = window.getDefaultView();
        if (shakeTimer > 0.f) {
            float mag = 6.f;
            float ox = ((rand() % 200) / 100.f - 1.f) * mag;
            float oy = ((rand() % 200) / 100.f - 1.f) * mag;
            view.move({ox, oy});
        }
        window.setView(view);

        switch (game->getScreen()) {
            case Screen::MAIN_MENU:
                drawMainMenu(window, font, menuFlicker);
                break;
            case Screen::CONTROLS:
                drawControls(window, font);
                break;
            case Screen::PLAYING:
            case Screen::PAUSED:
                window.clear(sf::Color(10, 10, 12));
                drawPlaying(window, font, *game, showInventory, showDebugPanel, hintTimer);
                if (game->getScreen() == Screen::PAUSED) drawPauseOverlay(window, font);
                break;
            case Screen::GAME_OVER:
                drawGameOver(window, font, *game);
                break;
            case Screen::ENDING:
                drawEnding(window, font, *game);
                break;
        }

        if (dimTimer > 0.f && game->getScreen() == Screen::PLAYING) {
            sf::RectangleShape dim({(float)WIN_W, (float)WIN_H});
            dim.setFillColor(sf::Color(0, 0, 0, (std::uint8_t)(120 * (dimTimer / 0.3f))));
            window.draw(dim);
        }

        if (jumpscareTimer > 0.f) {
            drawJumpscare(window, jumpscareTimer);
        }

        window.setView(window.getDefaultView());
        window.display();
    }

    delete game;
    return 0;
}
