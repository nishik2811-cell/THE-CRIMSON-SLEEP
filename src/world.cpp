#include "world.h"

const RoomInfo ROOMS[ROOM_COUNT] = {
	//  name                    map            danger spawn    dark  wraith
	{ "The Night Outside",      "",            0,  0,  0,  false, false },
	{ "Entrance Hall",          "map1.txt",    1, 15, 19,  false, false },
	{ "Grand Corridor",         "map2.txt",    3, 11,  8,  true,  true  },
	{ "Library",                "map3.txt",    4, 25,  6,  true,  true  },
	{ "Collapsed Bedroom",      "map4.txt",    5, 15,  4,  true,  false },
	{ "Nursery of Whispers",    "map5.txt",    7, 16, 17,  true,  false },
	{ "Chapel of Ash",          "map6.txt",    7, 16,  4,  true,  true  },
	{ "Ritual Antechamber",     "map7.txt",    6, 16, 14,  true,  true  },
	{ "The Crimson Crypt",      "map8.txt",   10, 28, 11,  true,  false },
	{ "Hidden Tunnel",          "map9.txt",    8, 15,  4,  true,  true  },
	{ "The Ossuary",            "map10.txt",   9, 15,  5,  true,  true  },
};

const ItemInfo ITEMS[ITEM_COUNT] = {
	{ "",                    "",                                                   false },
	{ "Lantern",             "Its flame burns a sickly red. Keeps the dark at bay.", false },
	{ "Rusty Key",           "Opens the Collapsed Bedroom (from the Library).",     false },
	{ "Newspaper, 1953",     "June 26 1953 - ten murdered, then the killer's suicide.", true },
	{ "Brass Key",           "Opens the Nursery of Whispers (from the Library).",   false },
	{ "Chapel Key",          "Opens the Chapel of Ash (from the Grand Corridor).",  false },
	{ "Iron Key",            "Opens the Ritual Antechamber (Grand Corridor).",      false },
	{ "Blessed Sword",       "Lets you enter the Crimson Crypt and face the Devil.", false },
	{ "Petrol Can",          "Burn it all down... and hope.",                        false },
	{ "Mattock",             "Stand still on the cracked ground to dig.",            false },
	{ "Diary",               "Dead souls wander the mansion. Something holds them.", true },
	{ "Crimson Locket",      "A bride's locket. It is warm, like a heartbeat.",      false },
	{ "Hymn Page",           "'Beneath the altar the dead still walk.'",             true },
};

void buildHouse(Graph& g) {
	g.clear();
	// addEdge(from, to, doorNo, locked, requiredItem, hidden)
	g.addEdge(ENTRANCE_HALL, GRAND_CORRIDOR, 6, true, LANTERN);
	g.addEdge(ENTRANCE_HALL, OUTSIDE, 5, true, CRIMSON_LOCKET);   // front door: secret ending

	g.addEdge(GRAND_CORRIDOR, ENTRANCE_HALL, 5);
	g.addEdge(GRAND_CORRIDOR, LIBRARY, 3);
	g.addEdge(GRAND_CORRIDOR, CHAPEL, 1, true, CHAPEL_KEY);
	g.addEdge(GRAND_CORRIDOR, ANTECHAMBER, 2, true, IRON_KEY);

	g.addEdge(LIBRARY, GRAND_CORRIDOR, 3);
	g.addEdge(LIBRARY, COLLAPSED_BEDROOM, 1, true, RUSTY_KEY);
	g.addEdge(LIBRARY, NURSERY, 2, true, BRASS_KEY);

	g.addEdge(COLLAPSED_BEDROOM, LIBRARY, 1);
	g.addEdge(NURSERY, LIBRARY, 1);

	g.addEdge(CHAPEL, GRAND_CORRIDOR, 1);
	g.addEdge(CHAPEL, HIDDEN_TUNNEL, 4, false, ITEM_NONE, true); // secret passage, revealed by DFS

	g.addEdge(ANTECHAMBER, GRAND_CORRIDOR, 1);
	g.addEdge(ANTECHAMBER, CRIMSON_CRYPT, 3, true, SWORD);

	g.addEdge(HIDDEN_TUNNEL, CHAPEL, 1, false, ITEM_NONE, true);
	g.addEdge(HIDDEN_TUNNEL, OSSUARY, 2, true, NEWSPAPER);       // needs the clipping *read* (BST)

	g.addEdge(OSSUARY, HIDDEN_TUNNEL, 1);
}

void buildItemDatabase(BST<ItemRecord>& db) {
	db.clear();
	// Insert in a "middle-first" order so the unbalanced BST stays shallow.
	const int order[] = { 6, 3, 9, 1, 4, 7, 11, 2, 5, 8, 10, 12 };
	for (int id : order) db.insert(id, ItemRecord{ id, false, false });
}
