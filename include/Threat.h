#ifndef THREAT_H
#define THREAT_H

#include <string>

// Types of active dangers in the house. The numeric priority (below)
// is what the PriorityQueue heap actually sorts on.
enum ThreatType {
    THREAT_DARK_ROOM,
    THREAT_TRAP,
    THREAT_GHOST_NEARBY,
    THREAT_GHOST_ATTACK
};

struct Threat {
    ThreatType type;
    std::string description;
    int roomId;
    int priority;      // higher number = more dangerous = handled first
    int healthDamage;  // applied to player when this threat is resolved
    int fearIncrease;  // applied to player when this threat is resolved
};

// Builds a fully filled-in Threat for the given type/room.
Threat makeThreat(ThreatType type, int roomId);

std::string threatTypeName(ThreatType type);

#endif
