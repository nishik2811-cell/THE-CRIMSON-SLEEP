#pragma once
#include "ds/Str.h"
#include "ds/Graph.h"
#include "ds/BST.h"
#include "ds/LinkedList.h"

// ---------------------------------------------------------------------
// Static world data for The Crimson Sleep.
//
// ARRAY: ROOMS[] holds static per-room properties (name, map file, danger
// level, fallback spawn, intro line) and ITEMS[] holds baseline item
// metadata.  Both are set once and only read afterwards, so a plain
// fixed-size array (O(1) index by id) is the right structure.
// ---------------------------------------------------------------------

enum RoomId {
	OUTSIDE = 0,          // the night beyond the front door (secret ending)
	ENTRANCE_HALL = 1,
	GRAND_CORRIDOR,
	LIBRARY,
	COLLAPSED_BEDROOM,
	NURSERY,
	CHAPEL,
	ANTECHAMBER,
	CRIMSON_CRYPT,
	HIDDEN_TUNNEL,
	OSSUARY,
	ROOM_COUNT            // 11 nodes (0..10)
};

struct RoomInfo {
	const char* name;
	const char* mapFile;   // "" for OUTSIDE
	int danger;            // 0..10 - number/intensity of haunted events
	int spawnX, spawnY;    // fallback spawn tile
	bool dark;             // without the lantern this room raises fear
	bool wraith;           // the Weeping Bride can manifest here
};
extern const RoomInfo ROOMS[ROOM_COUNT];

enum ItemId {
	ITEM_NONE = 0,
	LANTERN, RUSTY_KEY, NEWSPAPER, BRASS_KEY, CHAPEL_KEY, IRON_KEY,
	SWORD, PETROL, MATTOCK, DIARY, CRIMSON_LOCKET, HYMN_PAGE,
	ITEM_COUNT
};

struct ItemInfo {
	const char* name;
	const char* description;
	bool isClue;           // clues are "read" (discovered) rather than used up
};
extern const ItemInfo ITEMS[ITEM_COUNT];

// Record stored in the BST item database (keyed by ItemId).
struct ItemRecord {
	int id;
	bool collected;        // ever picked up
	bool discovered;       // clue has been read
};

// One entry of the inventory Linked List.
struct InventoryItem {
	int id;
};

// Builds the adjacency-list house graph.
void buildHouse(Graph& g);
// Fills the BST item database from ITEMS[].
void buildItemDatabase(BST<ItemRecord>& db);
