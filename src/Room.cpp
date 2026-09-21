
#include "Room.h"

RoomData ROOMS[ROOM_COUNT];

void initRooms() {
    ROOMS[ENTRANCE_HALL] = { ENTRANCE_HALL, "Entrance Hall",
        "A dusty foyer lit by a single flickering bulb. The front door looms behind you.", 1 };

    ROOMS[LIBRARY] = { LIBRARY, "Library",
        "Shelves of rotting books line the walls. Something rustles in the corner.", 3 };

    ROOMS[LIVING_ROOM] = { LIVING_ROOM, "Living Room",
        "An overturned armchair faces a cracked fireplace. Portraits watch you.", 2 };

    ROOMS[KITCHEN] = { KITCHEN, "Kitchen",
        "Rusted pots hang above a cold stove. The smell of decay lingers.", 3 };

    ROOMS[BEDROOM] = { BEDROOM, "Bedroom",
        "A moth-eaten bed sits beneath a cracked mirror.", 2 };

    ROOMS[BATHROOM] = { BATHROOM, "Bathroom",
        "Water drips steadily from a rusted faucet into a cracked tub.", 2 };

    ROOMS[STUDY] = { STUDY, "Study",
        "Papers are scattered across an old desk. A locked drawer catches your eye.", 3 };

    ROOMS[BASEMENT] = { BASEMENT, "Basement",
        "Cold, damp, and pitch black without a light source. Chains hang from the ceiling.", 6 };

    ROOMS[ATTIC] = { ATTIC, "Attic",
        "Dust and cobwebs coat forgotten furniture. Something scratches above.", 5 };

    ROOMS[GARDEN] = { GARDEN, "Garden",
        "Overgrown thorns choke a stone path leading into the fog.", 2 };

    ROOMS[HIDDEN_TUNNEL] = { HIDDEN_TUNNEL, "Hidden Tunnel",
        "A narrow passage carved into the earth, reeking of old candle wax.", 7 };

    ROOMS[RITUAL_ROOM] = { RITUAL_ROOM, "Ritual Room",
        "Candles surround a symbol burned into the floor. The air feels heavy.", 9 };

    ROOMS[SECRET_ROOM] = { SECRET_ROOM, "Secret Room",
        "A hidden chamber untouched by time, holding the house's darkest truth.", 8 };
}

const RoomData& getRoomData(int roomId) {
    return ROOMS[roomId];
}
