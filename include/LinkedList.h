#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "Item.h"
#include <string>

// ---------------------------------------------------------------------
// Linked List (manually implemented singly linked list).
//
// USED FOR: The player's inventory. Items are ADDED to the front of
// the list when picked up and REMOVED (by name) when used/consumed.
// Because the position/node is known during removal we walk the list
// exactly once, matching the O(n) search / O(1) unlink-once-found
// behaviour described in the project's complexity table.
// ---------------------------------------------------------------------
class LinkedList {
private:
    struct ListNode {
        Item data;
        ListNode* next;
    };

    ListNode* head;
    int count;

public:
    LinkedList();
    ~LinkedList();

    // Owns its nodes via raw pointers - copying is unsafe, so it is
    // disallowed (Rule of 3).
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void addItem(const Item& item);          // pick up an item
    bool removeItem(const std::string& name); // use/consume an item; true if it was found
    bool hasItem(const std::string& name) const;
    Item* findItem(const std::string& name) const; // nullptr if not carried
    int size() const;
    bool isEmpty() const;

    // For inventory UI / debug panel: copy up to maxSize items in order.
    void getContents(Item* outArray, int maxSize, int& outCount) const;
};

#endif
