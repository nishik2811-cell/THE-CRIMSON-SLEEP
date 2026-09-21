#include "Event.h"

Event makeEvent(EventType type, int roomId) {
    Event e;
    e.type = type;
    e.roomId = roomId;

    switch (type) {
        case LIGHTS_FLICKER:
            e.description = "The lights flicker for a moment...";
            e.fearIncrease = 3;
            break;
        case DOOR_SLAM:
            e.description = "A door slams shut somewhere in the house!";
            e.fearIncrease = 5;
            break;
        case WHISPER_HEARD:
            e.description = "You hear a faint whisper behind you...";
            e.fearIncrease = 6;
            break;
        case OBJECT_FALLS:
            e.description = "Something falls and shatters in another room.";
            e.fearIncrease = 4;
            break;
        case FOOTSTEPS_HEARD:
            e.description = "Slow footsteps echo down the hallway...";
            e.fearIncrease = 7;
            break;
        case ROOM_GOES_DARK:
            e.description = "The room grows unnaturally dark for a moment.";
            e.fearIncrease = 5;
            break;
        default:
            e.description = "Something feels wrong.";
            e.fearIncrease = 2;
            break;
    }
    return e;
}

std::string eventTypeName(EventType type) {
    switch (type) {
        case LIGHTS_FLICKER:   return "Lights Flicker";
        case DOOR_SLAM:        return "Door Slam";
        case WHISPER_HEARD:    return "Whisper Heard";
        case OBJECT_FALLS:     return "Object Falls";
        case FOOTSTEPS_HEARD:  return "Footsteps";
        case ROOM_GOES_DARK:   return "Darkness";
        default:               return "Unknown Event";
    }
}
