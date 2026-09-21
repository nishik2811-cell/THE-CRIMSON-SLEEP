#ifndef EVENT_H
#define EVENT_H

#include <string>

// Types of harmless-but-creepy environmental events.
// These are queued and processed in the order they happen (FIFO).
enum EventType {
    LIGHTS_FLICKER,
    DOOR_SLAM,
    WHISPER_HEARD,
    OBJECT_FALLS,
    FOOTSTEPS_HEARD,
    ROOM_GOES_DARK
};

// A single haunted event. Plain data (POD-style struct) held inside
// the Queue's linked nodes.
struct Event {
    EventType type;
    std::string description;
    int roomId;
    int fearIncrease; // how much fear this event adds when processed
};

// Utility: builds a filled-in Event for a given type/room.
Event makeEvent(EventType type, int roomId);

// Utility: human readable name of an event type (for UI text).
std::string eventTypeName(EventType type);

#endif
