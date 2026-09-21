#ifndef PLAYER_H
#define PLAYER_H

#include "Stack.h"
#include "LinkedList.h"
#include "Room.h"
#include <string>

// ---------------------------------------------------------------------
// Player
//
// Owns two of the required data structures directly:
//   - Stack   history   -> movement history / "Go Back" (B key)
//   - LinkedList inventory -> carried items (I key to view)
// Also keeps a plain bool ARRAY of which rooms have been visited,
// used both by the DFS exploration feature and by ending logic.
// ---------------------------------------------------------------------
class Player {
private:
    int currentRoomId;
    int health;
    int fear;
    Stack history;         // linked-list based stack of room ids
    LinkedList inventory;  // linked list of carried Items
    bool visited[ROOM_COUNT];

public:
    Player();

    // --- Movement ---
    void enterRoom(int roomId);   // push new room onto history stack
    bool goBack(int& outRoomId);  // pop stack -> return true and set outRoomId if possible
    int getCurrentRoom() const;
    bool hasVisited(int roomId) const;

    // --- Health / Fear ---
    int getHealth() const;
    int getFear() const;
    void takeDamage(int amount);
    void addFear(int amount);
    void heal(int amount);
    void reduceFear(int amount);
    bool isDead() const;
    bool isInsane() const; // fear reached 100

    // --- Inventory ---
    void addItem(const Item& item);
    bool useItem(const std::string& name); // removes item, true if it was carried
    bool hasItem(const std::string& name) const;
    const LinkedList& getInventory() const;

    // --- Debug panel access ---
    const Stack& getHistory() const;
};

#endif
