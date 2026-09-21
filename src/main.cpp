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
#include "WorldObject.h"
#include <string>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <optional>

static const unsigned int WIN_W = 1200;
static const unsigned int WIN_H = 750;

static const float ROOM_BOX_X = 30.f;
static const float ROOM_BOX_Y = 95.f;
static const float ROOM_BOX_W = ROOM_AREA_W; // shared with Game so world coords line up 1:1
static const float ROOM_BOX_H = ROOM_AREA_H;

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

// --- ambient (non-interactive) background decoration per room ----------
static void drawRoomDecorations(sf::RenderWindow& window, int room, float x, float y, float w, float h) {
    (void)h;
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
        case RITUAL_ROOM: {
            sf::CircleShape circle(70.f);
            circle.setPosition({x + w_half(w) - 70.f, y + 90.f});
            circle.setFillColor(sf::Color::Transparent);
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color(150, 20, 20));
            window.draw(circle);
            break;
        }
        case GARDEN: {
            for (int i = 0; i < 6; i++) {
                sf::CircleShape fog(40.f + (i % 3) * 10.f);
                fog.setPosition({x + 20.f + i * 180.f, y + 150.f + (i % 2) * 40.f});
                fog.setFillColor(sf::Color(200, 200, 210, 35));
                window.draw(fog);
            }
            break;
        }
        case KITCHEN: {
            sf::RectangleShape stove({100.f, 60.f});
            stove.setPosition({x + 500.f, y + 260.f});
            stove.setFillColor(sf::Color(50, 50, 55));
            window.draw(stove);
            break;
        }
        case ATTIC: {
            for (int i = 0; i < 10; i++) {
                sf::CircleShape dust(2.f);
                dust.setPosition({x + (float)(rand() % (int)w), y + (float)(rand() % 200)});
                dust.setFillColor(sf::Color(180, 180, 160, 100));
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
    head.setOrigin({13.f, 13.f});
    head.setPosition({centerX, bottomY - 47.f});
    head.setFillColor(sf::Color(12, 12, 12));
    head.setOutlineThickness(1.f);
    head.setOutlineColor(sf::Color(60, 60, 60));
    window.draw(head);

    sf::RectangleShape body({26.f, 42.f});
    body.setOrigin({13.f, 0.f});
    body.setPosition({centerX, bottomY - 34.f});
    body.setFillColor(sf::Color(12, 12, 12));
    window.draw(body);
}

// --- shared item icon drawing (used both in-world and in inventory) ----
static void drawItemIcon(sf::RenderWindow& window, const std::string& name, float cx, float cy, float scale) {
    if (name.find("Key") != std::string::npos) {
        sf::CircleShape bow(7.f * scale);
        bow.setOrigin({7.f * scale, 7.f * scale});
        bow.setPosition({cx - 8.f * scale, cy});
        bow.setFillColor(sf::Color::Transparent);
        bow.setOutlineThickness(2.5f * scale);
        bow.setOutlineColor(sf::Color(210, 175, 70));
        window.draw(bow);

        sf::RectangleShape shaft({16.f * scale, 3.f * scale});
        shaft.setPosition({cx - 2.f * scale, cy - 1.5f * scale});
        shaft.setFillColor(sf::Color(210, 175, 70));
        window.draw(shaft);

        sf::RectangleShape tooth({4.f * scale, 6.f * scale});
        tooth.setPosition({cx + 10.f * scale, cy - 1.5f * scale});
        tooth.setFillColor(sf::Color(210, 175, 70));
        window.draw(tooth);
    } else if (name.find("Diary") != std::string::npos || name.find("Note") != std::string::npos) {
        sf::RectangleShape page({22.f * scale, 28.f * scale});
        page.setOrigin({11.f * scale, 14.f * scale});
        page.setPosition({cx, cy});
        page.setFillColor(sf::Color(200, 185, 140));
        page.setOutlineThickness(1.f);
        page.setOutlineColor(sf::Color(120, 105, 70));
        window.draw(page);
        for (int i = 0; i < 3; i++) {
            sf::RectangleShape line({14.f * scale, 2.f});
            line.setPosition({cx - 7.f * scale, cy - 6.f * scale + i * 6.f * scale});
            line.setFillColor(sf::Color(120, 105, 70, 180));
            window.draw(line);
        }
    } else if (name.find("Photograph") != std::string::npos) {
        sf::RectangleShape frame({24.f * scale, 20.f * scale});
        frame.setOrigin({12.f * scale, 10.f * scale});
        frame.setPosition({cx, cy});
        frame.setFillColor(sf::Color(230, 228, 218));
        frame.setOutlineThickness(2.f);
        frame.setOutlineColor(sf::Color(60, 60, 60));
        window.draw(frame);
        sf::RectangleShape inner({16.f * scale, 12.f * scale});
        inner.setOrigin({8.f * scale, 6.f * scale});
        inner.setPosition({cx, cy});
        inner.setFillColor(sf::Color(90, 90, 100));
        window.draw(inner);
    } else if (name.find("Torch") != std::string::npos) {
        sf::RectangleShape handle({6.f * scale, 22.f * scale});
        handle.setOrigin({3.f * scale, 0.f});
        handle.setPosition({cx, cy - 8.f * scale});
        handle.setFillColor(sf::Color(90, 60, 30));
        window.draw(handle);
        sf::CircleShape flame(7.f * scale);
        flame.setOrigin({7.f * scale, 7.f * scale});
        flame.setPosition({cx, cy - 12.f * scale});
        flame.setFillColor(sf::Color(255, 180, 60, 200));
        window.draw(flame);
    } else if (name.find("Symbol") != std::string::npos) {
        sf::CircleShape disc(11.f * scale);
        disc.setOrigin({11.f * scale, 11.f * scale});
        disc.setPosition({cx, cy});
        disc.setFillColor(sf::Color(80, 30, 30));
        disc.setOutlineThickness(2.f);
        disc.setOutlineColor(sf::Color(150, 40, 40));
        window.draw(disc);
        sf::RectangleShape barA({16.f * scale, 3.f}), barB({16.f * scale, 3.f});
        barA.setOrigin({8.f * scale, 1.5f});
        barB.setOrigin({8.f * scale, 1.5f});
        barA.setPosition({cx, cy});
        barB.setPosition({cx, cy});
        barA.setRotation(sf::degrees(45));
        barB.setRotation(sf::degrees(-45));
        barA.setFillColor(sf::Color(220, 200, 180));
        barB.setFillColor(sf::Color(220, 200, 180));
        window.draw(barA);
        window.draw(barB);
    } else if (name.find("Coin") != std::string::npos) {
        sf::CircleShape coin(9.f * scale);
        coin.setOrigin({9.f * scale, 9.f * scale});
        coin.setPosition({cx, cy});
        coin.setFillColor(sf::Color(210, 180, 70));
        coin.setOutlineThickness(2.f);
        coin.setOutlineColor(sf::Color(140, 110, 30));
        window.draw(coin);
    } else if (name.find("Rod") != std::string::npos) {
        sf::RectangleShape rod({26.f * scale, 5.f * scale});
        rod.setOrigin({13.f * scale, 2.5f * scale});
        rod.setPosition({cx, cy});
        rod.setFillColor(sf::Color(110, 110, 115));
        window.draw(rod);
    } else if (name.find("Handle") != std::string::npos || name.find("Lever") != std::string::npos) {
        sf::RectangleShape handle({20.f * scale, 6.f * scale});
        handle.setOrigin({10.f * scale, 3.f * scale});
        handle.setPosition({cx, cy});
        handle.setFillColor(sf::Color(120, 80, 45));
        window.draw(handle);
    } else {
        sf::CircleShape dot(8.f * scale);
        dot.setOrigin({8.f * scale, 8.f * scale});
        dot.setPosition({cx, cy});
        dot.setFillColor(sf::Color(180, 180, 180));
        window.draw(dot);
    }
}

// --- draw one interactable/decorative world object ----------------------
static void drawWorldObject(sf::RenderWindow& window, sf::Font& font, const WorldObject& obj,
                             float screenX, float screenY, bool isNearest, float pulse, bool locked) {
    (void)font;

    // Subtle glow behind anything the player can actually act on -
    // uniform for both "important" and "useless" objects so the
    // glow itself never gives away which is which.
    if (obj.active && !obj.isDecorative) {
        sf::CircleShape glow(obj.radius * 0.85f + pulse * 3.f);
        glow.setOrigin({glow.getRadius(), glow.getRadius()});
        glow.setPosition({screenX, screenY});
        glow.setFillColor(sf::Color(255, 235, 180, (std::uint8_t)(30 + 18 * pulse)));
        window.draw(glow);
    }

    switch (obj.type) {
        case OBJ_ITEM:
            drawItemIcon(window, obj.name, screenX, screenY, 1.4f);
            break;

        case OBJ_DOOR: {
            sf::RectangleShape doorShape({46.f, 66.f});
            doorShape.setOrigin({23.f, 66.f});
            doorShape.setPosition({screenX, screenY + 33.f});
            bool isEnding = (obj.doorTargetRoom == -1);
            if (locked) {
                doorShape.setFillColor(sf::Color(45, 40, 40));
                doorShape.setOutlineColor(sf::Color(90, 30, 30));
            } else if (isEnding) {
                doorShape.setFillColor(sf::Color(70, 45, 30));
                doorShape.setOutlineColor(sf::Color(200, 120, 40));
            } else {
                doorShape.setFillColor(sf::Color(80, 55, 35));
                doorShape.setOutlineColor(sf::Color(40, 25, 15));
            }
            doorShape.setOutlineThickness(2.f);
            window.draw(doorShape);

            if (locked) {
                sf::RectangleShape lockBody({12.f, 10.f});
                lockBody.setOrigin({6.f, 5.f});
                lockBody.setPosition({screenX, screenY + 10.f});
                lockBody.setFillColor(sf::Color(200, 180, 60));
                window.draw(lockBody);
                sf::CircleShape lockLoop(5.f);
                lockLoop.setOrigin({5.f, 5.f});
                lockLoop.setPosition({screenX, screenY + 2.f});
                lockLoop.setFillColor(sf::Color::Transparent);
                lockLoop.setOutlineThickness(2.f);
                lockLoop.setOutlineColor(sf::Color(200, 180, 60));
                window.draw(lockLoop);
            }
            break;
        }

        case OBJ_DRAWER:
        case OBJ_CABINET:
        case OBJ_WARDROBE: {
            float w = (obj.type == OBJ_WARDROBE) ? 46.f : 56.f;
            float h = (obj.type == OBJ_WARDROBE) ? 80.f : 46.f;
            sf::RectangleShape body({w, h});
            body.setOrigin({w / 2.f, h});
            body.setPosition({screenX, screenY + h / 2.f});
            body.setFillColor(sf::Color(70, 50, 35));
            body.setOutlineThickness(2.f);
            body.setOutlineColor(sf::Color(35, 22, 14));
            window.draw(body);

            sf::RectangleShape front({w - 12.f, h - 14.f});
            front.setOrigin({(w - 12.f) / 2.f, (h - 14.f)});
            front.setPosition({screenX, screenY + h / 2.f - 4.f});
            front.setFillColor(obj.opened ? sf::Color(15, 15, 15) : sf::Color(95, 68, 46));
            window.draw(front);
            break;
        }

        case OBJ_CLOCK: {
            sf::CircleShape face(26.f);
            face.setOrigin({26.f, 26.f});
            face.setPosition({screenX, screenY});
            face.setFillColor(sf::Color(225, 220, 200));
            face.setOutlineThickness(3.f);
            face.setOutlineColor(sf::Color(60, 45, 25));
            window.draw(face);
            sf::RectangleShape hourHand({2.f, 12.f});
            hourHand.setOrigin({1.f, 12.f});
            hourHand.setPosition({screenX, screenY});
            hourHand.setRotation(sf::degrees(-100.f));
            hourHand.setFillColor(sf::Color(20, 20, 20));
            window.draw(hourHand);
            sf::RectangleShape minHand({2.f, 17.f});
            minHand.setOrigin({1.f, 17.f});
            minHand.setPosition({screenX, screenY});
            minHand.setRotation(sf::degrees(80.f));
            minHand.setFillColor(sf::Color(20, 20, 20));
            window.draw(minHand);
            break;
        }

        case OBJ_MIRROR: {
            sf::RectangleShape frame({40.f, 62.f});
            frame.setOrigin({20.f, 62.f});
            frame.setPosition({screenX, screenY + 31.f});
            frame.setFillColor(sf::Color(60, 65, 75));
            frame.setOutlineThickness(2.f);
            frame.setOutlineColor(sf::Color(30, 30, 35));
            window.draw(frame);
            sf::RectangleShape highlight({6.f, 50.f});
            highlight.setOrigin({3.f, 50.f});
            highlight.setPosition({screenX - 8.f, screenY + 55.f});
            highlight.setFillColor(sf::Color(255, 255, 255, 30));
            window.draw(highlight);
            break;
        }

        case OBJ_PAINTING: {
            sf::RectangleShape frame({50.f, 40.f});
            frame.setOrigin({25.f, 40.f});
            frame.setPosition({screenX, screenY + 20.f});
            frame.setFillColor(sf::Color(90, 55, 35));
            frame.setOutlineThickness(3.f);
            frame.setOutlineColor(sf::Color(40, 25, 15));
            window.draw(frame);
            sf::RectangleShape canvas({40.f, 30.f});
            canvas.setOrigin({20.f, 30.f});
            canvas.setPosition({screenX, screenY + 18.f});
            canvas.setFillColor(sf::Color(55, 30, 30));
            window.draw(canvas);
            break;
        }

        case OBJ_BOOKSHELF: {
            for (int i = 0; i < 3; i++) {
                sf::RectangleShape book({10.f, 34.f});
                book.setPosition({screenX - 18.f + i * 13.f, screenY - 17.f});
                sf::Color cols[3] = { sf::Color(90, 40, 30), sf::Color(60, 70, 40), sf::Color(45, 30, 60) };
                book.setFillColor(cols[i % 3]);
                book.setOutlineThickness(1.f);
                book.setOutlineColor(sf::Color(20, 15, 10));
                window.draw(book);
            }
            break;
        }

        case OBJ_BOX: {
            sf::RectangleShape chest({54.f, 34.f});
            chest.setOrigin({27.f, 34.f});
            chest.setPosition({screenX, screenY + 17.f});
            chest.setFillColor(obj.opened ? sf::Color(35, 28, 20) : sf::Color(95, 65, 35));
            chest.setOutlineThickness(2.f);
            chest.setOutlineColor(sf::Color(40, 28, 15));
            window.draw(chest);
            sf::RectangleShape clasp({8.f, 8.f});
            clasp.setOrigin({4.f, 4.f});
            clasp.setPosition({screenX, screenY - 1.f});
            clasp.setFillColor(sf::Color(180, 160, 70));
            window.draw(clasp);
            break;
        }

        case OBJ_CRACKED_WALL: {
            sf::RectangleShape patch({60.f, 60.f});
            patch.setOrigin({30.f, 30.f});
            patch.setPosition({screenX, screenY});
            patch.setFillColor(sf::Color(40, 38, 36));
            window.draw(patch);
            for (int i = 0; i < 3; i++) {
                sf::RectangleShape crack({30.f, 2.f});
                crack.setOrigin({15.f, 1.f});
                crack.setPosition({screenX - 8.f + i * 6.f, screenY - 15.f + i * 12.f});
                crack.setRotation(sf::degrees(20.f * (i - 1)));
                crack.setFillColor(sf::Color(15, 15, 15));
                window.draw(crack);
            }
            break;
        }

        case OBJ_CANDLE: {
            sf::RectangleShape wax({8.f, 26.f});
            wax.setOrigin({4.f, 26.f});
            wax.setPosition({screenX, screenY + 13.f});
            wax.setFillColor(sf::Color(210, 200, 170));
            window.draw(wax);
            sf::CircleShape flame(5.f);
            flame.setOrigin({5.f, 5.f});
            flame.setPosition({screenX, screenY - 14.f});
            flame.setFillColor(sf::Color(255, 190, 70, 210));
            window.draw(flame);
            break;
        }

        case OBJ_STATUE: {
            sf::CircleShape head(11.f);
            head.setOrigin({11.f, 11.f});
            head.setPosition({screenX, screenY - 30.f});
            head.setFillColor(sf::Color(140, 140, 135));
            window.draw(head);
            sf::RectangleShape body({28.f, 45.f});
            body.setOrigin({14.f, 0.f});
            body.setPosition({screenX, screenY - 20.f});
            body.setFillColor(sf::Color(130, 130, 125));
            window.draw(body);
            break;
        }

        case OBJ_SYMBOL: {
            sf::CircleShape ring(24.f);
            ring.setOrigin({24.f, 24.f});
            ring.setPosition({screenX, screenY});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f);
            ring.setOutlineColor(sf::Color(150, 30, 30));
            window.draw(ring);
            sf::RectangleShape barA({34.f, 3.f}), barB({34.f, 3.f});
            barA.setOrigin({17.f, 1.5f});
            barB.setOrigin({17.f, 1.5f});
            barA.setPosition({screenX, screenY});
            barB.setPosition({screenX, screenY});
            barA.setRotation(sf::degrees(45));
            barB.setRotation(sf::degrees(-45));
            barA.setFillColor(sf::Color(150, 30, 30));
            barB.setFillColor(sf::Color(150, 30, 30));
            window.draw(barA);
            window.draw(barB);
            break;
        }
    }

    // Selection ring around whichever object is currently in range.
    if (isNearest) {
        sf::CircleShape ring(obj.radius + 4.f);
        ring.setOrigin({ring.getRadius(), ring.getRadius()});
        ring.setPosition({screenX, screenY});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(sf::Color(255, 230, 140, 200));
        window.draw(ring);
    }
}

// --- inventory overlay (Linked List) ------------------------------------
static void drawInventoryOverlay(sf::RenderWindow& window, sf::Font& font, Game& game) {
    sf::RectangleShape box({560.f, 440.f});
    box.setPosition({320.f, 140.f});
    box.setFillColor(sf::Color(15, 15, 18, 235));
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color(150, 30, 30));
    window.draw(box);

    drawText(window, font, "INVENTORY", 340.f, 155.f, 22, sf::Color(220, 180, 100), true);
    drawText(window, font, "(Linked List - press I to close)", 340.f, 185.f, 13, sf::Color(140, 140, 140));

    Item items[20];
    int count = 0;
    game.getPlayer().getInventory().getContents(items, 20, count);

    if (count == 0) {
        drawText(window, font, "Empty. Explore the house to find items.", 340.f, 230.f, 15, sf::Color(180, 180, 180));
    } else {
        for (int i = 0; i < count; i++) {
            float rowY = 225.f + i * 52.f;
            sf::RectangleShape iconBg({40.f, 40.f});
            iconBg.setPosition({340.f, rowY});
            iconBg.setFillColor(sf::Color(30, 30, 33));
            iconBg.setOutlineThickness(1.f);
            iconBg.setOutlineColor(sf::Color(70, 70, 70));
            window.draw(iconBg);
            drawItemIcon(window, items[i].name, 360.f, rowY + 20.f, 1.f);

            drawText(window, font, items[i].name, 392.f, rowY, 17, sf::Color::White, true);
            drawText(window, font, wrapText(items[i].description, 60), 392.f, rowY + 22.f, 12, sf::Color(170, 170, 170));
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
    int neighbors[MAX_ROOMS]; int ncount;
    game.getGraph().getNeighbors(player.getCurrentRoom(), neighbors, MAX_ROOMS, ncount);
    std::string neighborLine;
    for (int i = 0; i < ncount; i++) {
        bool passable = game.getGraph().isPassable(player.getCurrentRoom(), neighbors[i]);
        neighborLine += getRoomData(neighbors[i]).name + (passable ? "(open) " : "(locked) ");
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
    ry += 100.f;

    drawText(window, font, "BFS - last computed hint path:", rightX, ry, 14, sf::Color(255, 220, 180), true);
    std::string bfsLine;
    for (int i = 0; i < game.getHintPathLen(); i++) bfsLine += getRoomData(game.getHintPath()[i]).name + " -> ";
    if (bfsLine.empty()) bfsLine = "(press H to compute a hint)";
    drawText(window, font, wrapText(bfsLine, 55), rightX, ry + 18, 12, sf::Color(220, 220, 220));
    ry += 60.f;

    drawText(window, font, "DFS - last exploration result:", rightX, ry, 14, sf::Color(255, 220, 180), true);
    std::string dfsLine;
    for (int i = 0; i < game.getDFSResultCount(); i++) dfsLine += getRoomData(game.getDFSResult()[i]).name + " ";
    if (dfsLine.empty()) dfsLine = "(search the Cracked Wall in the Basement)";
    drawText(window, font, wrapText(dfsLine, 55), rightX, ry + 18, 12, sf::Color(220, 220, 220));
}

// --- main gameplay screen -------------------------------------------------
static void drawPlaying(sf::RenderWindow& window, sf::Font& font, Game& game,
                         bool showInventory, bool showDebugPanel, float hintTimer, float globalTime) {
    const Player& player = game.getPlayer();
    int room = player.getCurrentRoom();
    const RoomData& rd = getRoomData(room);

    drawText(window, font, rd.name, 30.f, 8.f, 26, sf::Color(210, 40, 40), true);
    drawText(window, font, wrapText(rd.description, 110), 30.f, 40.f, 13, sf::Color(160, 160, 160));
    drawText(window, font, "Objective: " + game.getObjectiveLabel(), 30.f, 74.f, 13, sf::Color(150, 165, 120));

    // Room visualization box (now spans the full width - objects live inside it)
    sf::RectangleShape roomBox({ROOM_BOX_W, ROOM_BOX_H});
    roomBox.setPosition({ROOM_BOX_X, ROOM_BOX_Y});
    int danger = rd.baseDangerLevel;
    int shade = 34 - danger * 2; if (shade < 8) shade = 8;
    roomBox.setFillColor(sf::Color((std::uint8_t)(shade + danger), (std::uint8_t)shade, (std::uint8_t)shade));
    roomBox.setOutlineThickness(3);
    roomBox.setOutlineColor(sf::Color(100, 25, 25));
    window.draw(roomBox);

    drawRoomDecorations(window, room, ROOM_BOX_X, ROOM_BOX_Y, ROOM_BOX_W, ROOM_BOX_H);

    // World objects (doors, items, furniture, puzzle pieces)
    int objCount = 0;
    const WorldObject* objects = game.getRoomObjects(room, objCount);
    int nearestIdx = game.findNearestInteractable();
    for (int i = 0; i < objCount; i++) {
        const WorldObject& obj = objects[i];
        if (!obj.active) continue;
        float screenX = ROOM_BOX_X + obj.x;
        float screenY = ROOM_BOX_Y + obj.y;
        float pulse = 0.5f + 0.5f * std::sin(globalTime * 2.2f + obj.x * 0.02f + obj.y * 0.03f);
        bool locked = (obj.type == OBJ_DOOR && obj.doorTargetRoom >= 0 &&
                       !game.getGraph().isPassable(room, obj.doorTargetRoom));
        drawWorldObject(window, font, obj, screenX, screenY, i == nearestIdx, pulse, locked);
    }

    drawPlayerSilhouette(window, ROOM_BOX_X + player.getX(), ROOM_BOX_Y + player.getY());

    // Contextual "[E] ..." prompt floats above the nearest interactable.
    std::string prompt = game.getInteractPrompt();
    if (!prompt.empty() && nearestIdx >= 0) {
        const WorldObject& obj = objects[nearestIdx];
        float px = ROOM_BOX_X + obj.x;
        float py = ROOM_BOX_Y + obj.y - obj.radius - 26.f;
        float boxW = 16.f + prompt.size() * 7.5f;
        sf::RectangleShape promptBg({boxW, 22.f});
        promptBg.setOrigin({boxW / 2.f, 22.f});
        promptBg.setPosition({px, py + 22.f});
        promptBg.setFillColor(sf::Color(0, 0, 0, 190));
        window.draw(promptBg);
        drawText(window, font, prompt, px - boxW / 2.f + 8.f, py, 14, sf::Color(255, 235, 150), true);
    }

    // Health / Fear bars
    drawBar(window, font, "HEALTH", 30.f, 450.f, 350.f, 26.f, player.getHealth(), 100, sf::Color(190, 25, 25));
    drawBar(window, font, "FEAR", 400.f, 450.f, 350.f, 26.f, player.getFear(), 100, sf::Color(130, 30, 170));

    // Message log
    drawText(window, font, "You: " + game.getLastMessage(), 30.f, 495.f, 14, sf::Color(225, 215, 170));
    std::string evMsg = game.getLastEventMessage();
    drawText(window, font, "Event: " + (evMsg.empty() ? "(quiet so far)" : evMsg), 30.f, 517.f, 14, sf::Color(160, 185, 225));
    std::string thMsg = game.getLastThreatMessage();
    drawText(window, font, "Threat: " + (thMsg.empty() ? "(nothing yet)" : thMsg), 30.f, 539.f, 14, sf::Color(225, 140, 140));

    // Hint banner
    if (hintTimer > 0.f) {
        sf::RectangleShape hintBox({1140.f, 38.f});
        hintBox.setPosition({30.f, 568.f});
        hintBox.setFillColor(sf::Color(45, 40, 10, 230));
        hintBox.setOutlineThickness(1);
        hintBox.setOutlineColor(sf::Color(180, 150, 40));
        window.draw(hintBox);
        drawText(window, font, "HINT (BFS): " + game.getHintMessage(), 40.f, 577.f, 15, sf::Color(255, 225, 120), true);
    }

    // Controls strip
    sf::RectangleShape ctrlStrip({(float)WIN_W, 40.f});
    ctrlStrip.setPosition({0.f, 710.f});
    ctrlStrip.setFillColor(sf::Color(14, 14, 16));
    window.draw(ctrlStrip);
    drawText(window, font,
        "Move: Arrows/WASD  E: Interact  F: Inspect held clue  H: Hint  B: Go Back  I: Inventory  TAB: DS Panel  ESC: Pause",
        10.f, 720.f, 12, sf::Color(150, 150, 150));

    if (showInventory) drawInventoryOverlay(window, font, game);
    if (showDebugPanel) drawDebugPanel(window, font, game);
}

static void drawPickupToast(sf::RenderWindow& window, sf::Font& font, const std::string& text, float timer) {
    float alpha = timer > 2.0f ? 1.f : timer / 2.0f;
    sf::RectangleShape box({330.f, 44.f});
    box.setPosition({(float)WIN_W - 350.f, 100.f});
    box.setFillColor(sf::Color(20, 60, 25, (std::uint8_t)(220 * alpha)));
    box.setOutlineThickness(1.f);
    box.setOutlineColor(sf::Color(80, 200, 100, (std::uint8_t)(255 * alpha)));
    window.draw(box);
    drawText(window, font, text, (float)WIN_W - 335.f, 113.f, 14, sf::Color(220, 255, 220, (std::uint8_t)(255 * alpha)), true);
}

static void drawClueBanner(sf::RenderWindow& window, sf::Font& font, const std::string& text, float timer) {
    float alpha = timer > 4.0f ? 1.f : timer / 4.0f;
    sf::RectangleShape box({900.f, 46.f});
    box.setPosition({WIN_W / 2.f - 450.f, 100.f});
    box.setFillColor(sf::Color(50, 40, 10, (std::uint8_t)(230 * alpha)));
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(220, 180, 60, (std::uint8_t)(255 * alpha)));
    window.draw(box);
    drawText(window, font, wrapText(text, 100), WIN_W / 2.f - 430.f, 110.f, 14,
              sf::Color(255, 235, 170, (std::uint8_t)(255 * alpha)), true);
}

// A "memory" flashback fragment - the house's backstory, surfaced at
// key progression milestones rather than all at once.
static void drawStoryBanner(sf::RenderWindow& window, sf::Font& font, const std::string& text, float timer) {
    float alpha = timer > 5.5f ? 1.f : timer / 5.5f;
    float boxW = 760.f, boxH = 100.f;
    float boxX = ROOM_BOX_X + ROOM_BOX_W / 2.f - boxW / 2.f;
    float boxY = ROOM_BOX_Y + ROOM_BOX_H / 2.f - boxH / 2.f;

    sf::RectangleShape dim({(float)WIN_W, (float)WIN_H});
    dim.setFillColor(sf::Color(0, 0, 0, (std::uint8_t)(90 * alpha)));
    window.draw(dim);

    sf::RectangleShape box({boxW, boxH});
    box.setPosition({boxX, boxY});
    box.setFillColor(sf::Color(15, 10, 20, (std::uint8_t)(225 * alpha)));
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(120, 80, 160, (std::uint8_t)(255 * alpha)));
    window.draw(box);

    drawText(window, font, "MEMORY", boxX + 20.f, boxY + 10.f, 13, sf::Color(160, 130, 200, (std::uint8_t)(255 * alpha)), true);

    sf::Text body(font, wrapText(text, 85), 15);
    body.setStyle(sf::Text::Italic);
    body.setFillColor(sf::Color(220, 210, 230, (std::uint8_t)(255 * alpha)));
    body.setPosition({boxX + 20.f, boxY + 34.f});
    window.draw(body);
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

    drawText(window, font, "Explore 13 rooms. Notice what doesn't belong. Escape... or don't.", WIN_W / 2.f - 270.f, 560.f, 14, sf::Color(120, 120, 120));
}

static void drawControls(sf::RenderWindow& window, sf::Font& font) {
    window.clear(sf::Color(6, 6, 8));
    drawText(window, font, "CONTROLS", 60.f, 50.f, 34, sf::Color(200, 40, 40), true);

    struct Line { const char* key; const char* desc; };
    Line lines[] = {
        {"Arrow Keys / W A S D", "Walk around the room"},
        {"E", "Interact with whatever is nearest to you (pick up, open, examine, unlock, enter)"},
        {"B", "Go Back to the previous room (Stack)"},
        {"F", "Read a clue item you are already carrying (BST)"},
        {"H", "Request a hint toward your next goal (BFS)"},
        {"I", "Toggle Inventory panel (Linked List)"},
        {"TAB", "Toggle the Data Structures debug panel"},
        {"ESC", "Pause / Resume"},
    };
    float y = 130.f;
    for (auto& l : lines) {
        drawText(window, font, l.key, 80.f, y, 18, sf::Color(255, 210, 120), true);
        drawText(window, font, l.desc, 420.f, y, 15, sf::Color(210, 210, 210));
        y += 45.f;
    }
    drawText(window, font, "Walk close to an object to see a prompt like \"[E] Pick up Rusty Key\".", 60.f, y + 20.f, 14, sf::Color(160, 160, 160));
    drawText(window, font, "Press any key to return to the Main Menu", 60.f, y + 60.f, 16, sf::Color(150, 150, 150));
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
    float t = timer / 0.5f;
    if (t > 1.f) t = 1.f;

    // Fast strobing flash - cycles white / deep red / black for a harsher,
    // more disorienting hit than a simple fade.
    sf::RectangleShape flash({(float)WIN_W, (float)WIN_H});
    int phase = ((int)(timer * 30)) % 3;
    sf::Color flashColor = (phase == 0) ? sf::Color(255, 255, 255, (std::uint8_t)(200 * t))
                          : (phase == 1) ? sf::Color(140, 0, 0, (std::uint8_t)(220 * t))
                                         : sf::Color(0, 0, 0, (std::uint8_t)(180 * t));
    flash.setFillColor(flashColor);
    window.draw(flash);

    // Jittery "glitch" offset so the face never sits perfectly still.
    float jx = (float)(rand() % 14 - 7);
    float jy = (float)(rand() % 14 - 7);
    float cx = WIN_W / 2.f + jx;
    float cy = WIN_H / 2.f + jy;
    float scale = 0.85f + 0.35f * t;

    // Jagged, irregular skull-like head instead of a clean circle -
    // reads as far more "wrong" than a smooth shape.
    const int spikes = 14;
    sf::ConvexShape head(spikes);
    for (int i = 0; i < spikes; i++) {
        float angle = (float)i / spikes * 2.f * 3.14159f;
        float wobble = 0.75f + 0.35f * std::sin(angle * 5.f + timer * 40.f);
        float r = (95.f + 40.f * t) * wobble * scale;
        head.setPoint(i, {cx + std::cos(angle) * r, cy + std::sin(angle) * r * 0.9f});
    }
    head.setFillColor(sf::Color(8, 4, 4, (std::uint8_t)(240 * t)));
    head.setOutlineThickness(2.f);
    head.setOutlineColor(sf::Color(120, 0, 0, (std::uint8_t)(200 * t)));
    window.draw(head);

    // Cracks/veins radiating out from the face across the screen.
    for (int i = 0; i < 8; i++) {
        float angle = (float)i / 8.f * 2.f * 3.14159f + timer * 3.f;
        sf::RectangleShape crack({140.f * scale, 2.5f});
        crack.setOrigin({0.f, 1.25f});
        crack.setPosition({cx, cy});
        crack.setRotation(sf::radians(angle));
        crack.setFillColor(sf::Color(200, 0, 0, (std::uint8_t)(150 * t)));
        window.draw(crack);
    }

    // Multiple asymmetric glowing red eyes - unsettling rather than a
    // normal symmetric face.
    struct EyeSpec { float ox, oy, r; };
    EyeSpec eyes[5] = {
        { -38.f, -18.f, 15.f }, { 34.f, -22.f, 12.f },
        { -60.f, 10.f, 7.f },   { 55.f, 15.f, 8.f },
        { 5.f, -40.f, 6.f }
    };
    for (auto& e : eyes) {
        sf::CircleShape glow((e.r + 6.f) * scale);
        glow.setOrigin({glow.getRadius(), glow.getRadius()});
        glow.setPosition({cx + e.ox * scale, cy + e.oy * scale});
        glow.setFillColor(sf::Color(255, 30, 30, (std::uint8_t)(90 * t)));
        window.draw(glow);

        sf::CircleShape eye(e.r * scale);
        eye.setOrigin({eye.getRadius(), eye.getRadius()});
        eye.setPosition({cx + e.ox * scale, cy + e.oy * scale});
        eye.setFillColor(sf::Color(255, 240, 240, (std::uint8_t)(255 * t)));
        window.draw(eye);

        sf::CircleShape pupil(e.r * 0.45f * scale);
        pupil.setOrigin({pupil.getRadius(), pupil.getRadius()});
        pupil.setPosition({cx + e.ox * scale, cy + e.oy * scale});
        pupil.setFillColor(sf::Color(30, 0, 0, (std::uint8_t)(255 * t)));
        window.draw(pupil);
    }

    // Wide, jagged mouth full of zigzag teeth.
    const int teeth = 9;
    float mouthW = 110.f * scale, mouthTop = cy + 32.f * scale, mouthBot = cy + 78.f * scale;
    sf::ConvexShape mouthBg(4);
    mouthBg.setPoint(0, {cx - mouthW / 2.f, mouthTop});
    mouthBg.setPoint(1, {cx + mouthW / 2.f, mouthTop});
    mouthBg.setPoint(2, {cx + mouthW / 2.f - 10.f, mouthBot});
    mouthBg.setPoint(3, {cx - mouthW / 2.f + 10.f, mouthBot});
    mouthBg.setFillColor(sf::Color(40, 0, 0, (std::uint8_t)(255 * t)));
    window.draw(mouthBg);

    for (int i = 0; i < teeth; i++) {
        float tx = cx - mouthW / 2.f + (mouthW / (teeth - 1)) * i;
        bool fromTop = (i % 2 == 0);
        sf::ConvexShape tooth(3);
        float toothH = 16.f * scale;
        if (fromTop) {
            tooth.setPoint(0, {tx - 8.f, mouthTop});
            tooth.setPoint(1, {tx + 8.f, mouthTop});
            tooth.setPoint(2, {tx, mouthTop + toothH});
        } else {
            tooth.setPoint(0, {tx - 8.f, mouthBot});
            tooth.setPoint(1, {tx + 8.f, mouthBot});
            tooth.setPoint(2, {tx, mouthBot - toothH});
        }
        tooth.setFillColor(sf::Color(230, 225, 210, (std::uint8_t)(255 * t)));
        window.draw(tooth);
    }
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
    float globalTime = 0.f;

    int lastEventSeq = 0;
    int lastThreatSeq = 0;
    Screen lastScreen = game->getScreen();

    float jumpscareTimer = 0.f;
    float shakeTimer = 0.f;
    float dimTimer = 0.f;

    const float PLAYER_SPEED = 230.f;

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        menuFlicker += dt;
        globalTime += dt;

        if (game->getScreen() == Screen::MAIN_MENU || game->getScreen() == Screen::CONTROLS) {
            audio.playMenuMusic();
        }

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
                        audio.stopMenuMusic();
                        audio.playAmbientDrone();
                    } else if (code == sf::Keyboard::Key::C) {
                        game->showControls();
                    } else if (code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                } else if (scr == Screen::CONTROLS) {
                    game->goToMainMenu();
                } else if (scr == Screen::PLAYING) {
                    if (code == sf::Keyboard::Key::E) {
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

        // --- continuous movement (polled every frame, not event-based) ---
        if (game->getScreen() == Screen::PLAYING) {
            float dx = 0.f, dy = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) dy -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) dy += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) dx -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dx += 1.f;
            if (dx != 0.f && dy != 0.f) {
                dx *= 0.7071f;
                dy *= 0.7071f;
            }
            if (dx != 0.f || dy != 0.f) {
                game->movePlayer(dx * PLAYER_SPEED * dt, dy * PLAYER_SPEED * dt);
            }
        }

        game->update(dt);

        // --- reactive audio / jumpscares based on data-structure activity ---
        if (game->getScreen() == Screen::PLAYING) {
            if (game->getEventSeq() != lastEventSeq) {
                lastEventSeq = game->getEventSeq();
                dimTimer = 0.25f;
                audio.playEventStinger();
            }
            if (game->getThreatSeq() != lastThreatSeq) {
                lastThreatSeq = game->getThreatSeq();
                ThreatType tt = game->getLastThreatType();
                if (tt == THREAT_GHOST_ATTACK) {
                    jumpscareTimer = 0.6f;
                    shakeTimer = 0.5f;
                    audio.playJumpscareStinger();
                } else if (tt == THREAT_GHOST_NEARBY) {
                    shakeTimer = 0.2f;
                    audio.playScream();
                } else if (tt == THREAT_TRAP) {
                    shakeTimer = 0.25f;
                    audio.playTrapHit();
                } else {
                    dimTimer = 0.3f;
                    audio.playWhisper();
                }
            }
        }

        if (lastScreen != Screen::GAME_OVER && game->getScreen() == Screen::GAME_OVER) {
            jumpscareTimer = 0.7f;
            audio.playJumpscareStinger();
            audio.stopAmbient();
        }
        if (lastScreen != Screen::ENDING && game->getScreen() == Screen::ENDING) {
            audio.stopAmbient();
            audio.playEndingMusic();
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
                drawPlaying(window, font, *game, showInventory, showDebugPanel, hintTimer, globalTime);
                {
                    float toastTimer = 0.f;
                    std::string toastText = game->getPickupToast(toastTimer);
                    if (toastTimer > 0.f) drawPickupToast(window, font, toastText, toastTimer);
                    float banTimer = 0.f;
                    std::string banText = game->getClueBanner(banTimer);
                    if (banTimer > 0.f) drawClueBanner(window, font, banText, banTimer);
                    float storyTimer2 = 0.f;
                    std::string storyText2 = game->getStoryBanner(storyTimer2);
                    if (storyTimer2 > 0.f) drawStoryBanner(window, font, storyText2, storyTimer2);
                }
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
