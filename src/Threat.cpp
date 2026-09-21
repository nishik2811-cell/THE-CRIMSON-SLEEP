#include "Threat.h"

Threat makeThreat(ThreatType type, int roomId) {
    Threat t;
    t.type = type;
    t.roomId = roomId;

    switch (type) {
        case THREAT_DARK_ROOM:
            t.description = "This room is pitch dark. Something could be watching.";
            t.priority = 1;
            t.healthDamage = 0;
            t.fearIncrease = 8;
            break;
        case THREAT_TRAP:
            t.description = "A hidden trap springs shut on you!";
            t.priority = 2;
            t.healthDamage = 15;
            t.fearIncrease = 10;
            break;
        case THREAT_GHOST_NEARBY:
            t.description = "A ghostly figure drifts through the wall nearby!";
            t.priority = 3;
            t.healthDamage = 0;
            t.fearIncrease = 18;
            break;
        case THREAT_GHOST_ATTACK:
            t.description = "The ghost lunges and attacks you!";
            t.priority = 4;
            t.healthDamage = 30;
            t.fearIncrease = 25;
            break;
        default:
            t.description = "Something dangerous is here.";
            t.priority = 1;
            t.healthDamage = 5;
            t.fearIncrease = 5;
            break;
    }
    return t;
}

std::string threatTypeName(ThreatType type) {
    switch (type) {
        case THREAT_DARK_ROOM:     return "Dark Room";
        case THREAT_TRAP:          return "Trap";
        case THREAT_GHOST_NEARBY:  return "Ghost Nearby";
        case THREAT_GHOST_ATTACK:  return "Ghost Attack";
        default:                   return "Unknown Threat";
    }
}
