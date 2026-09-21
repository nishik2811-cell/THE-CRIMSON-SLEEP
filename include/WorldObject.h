#ifndef WORLD_OBJECT_H
#define WORLD_OBJECT_H

#include <string>

// Local coordinate space every room's objects and the player live in.
// main.cpp just offsets these by the room box's screen position when
// drawing - the game logic never needs to know about pixels-on-screen.
const float ROOM_AREA_W = 1140.f;
const float ROOM_AREA_H = 340.f;

// What an interactable object visually/behaviourally is. This drives
// both rendering (main.cpp picks a shape per type) and the verb shown
// in the contextual "[E] ..." prompt.
enum ObjectType {
    OBJ_ITEM,        // a pickup (key, diary, photo, torch, symbol, coin...)
    OBJ_DOOR,        // a doorway to another room (backed by the Graph)
    OBJ_DRAWER,      // openable furniture, may hide something
    OBJ_CABINET,
    OBJ_WARDROBE,
    OBJ_CLOCK,
    OBJ_MIRROR,
    OBJ_PAINTING,
    OBJ_BOOKSHELF,
    OBJ_BOX,         // a chest/box that may need a crafted item to open
    OBJ_CRACKED_WALL,// triggers the Basement DFS search
    OBJ_CANDLE,
    OBJ_STATUE,
    OBJ_SYMBOL
};

// One interactable (or purely decorative) thing placed in a room.
// Rooms hold these in a fixed-size ARRAY (Game::roomObjects) - the
// array itself is mutable at runtime (active/opened flags flip as the
// player plays), same as roomItemId was before this rework.
struct WorldObject {
    ObjectType type;
    float x, y;             // position in room-local coordinates
    float radius;           // how close the player must be to interact
    std::string name;       // "Rusty Key", "Old Cabinet", "Grandfather Clock"
    std::string verb;       // "Pick up", "Open", "Examine", "Search"
    bool active;            // false = invisible/non-interactable (used for hidden doors/pickups already taken)
    bool opened;            // for drawers/cabinets/wardrobes/boxes
    int linkedItemId;       // >0 if interacting grants this item id
    int doorTargetRoom;     // >=0 if this object is a door to that room
    std::string requiredItemName; // non-empty = door needs this item to unlock
    std::string inspectText;      // shown when examined (clock/mirror/painting/etc.)
    bool isDecorative;      // true = no prompt/glow, pure background flavor
};

#endif
