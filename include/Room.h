#ifndef ROOM_H
#define ROOM_H

#include <string>

const int ROOM_COUNT = 13;

// Room ids -- also used as indices into the ROOMS array below and as
// node ids in the Graph.
enum RoomId {
    ENTRANCE_HALL = 0,
    LIBRARY,
    LIVING_ROOM,
    KITCHEN,
    BEDROOM,
    BATHROOM,
    STUDY,
    BASEMENT,
    ATTIC,
    GARDEN,
    HIDDEN_TUNNEL,
    RITUAL_ROOM,
    SECRET_ROOM
};

// Static per-room metadata. This is intentionally a plain fixed-size
// ARRAY (the "Array" data structure required by the project) rather
// than a linked structure, because this data never grows/shrinks at
// runtime -- it is set once at startup and only read afterwards.
struct RoomData {
    int id;
    std::string name;
    std::string description;
    int baseDangerLevel; // 0 (safe) .. 10 (very dangerous) - influences threat chance
};

extern RoomData ROOMS[ROOM_COUNT];

void initRooms();                 // fills the ROOMS array (call once at startup)
const RoomData& getRoomData(int roomId);

#endif
