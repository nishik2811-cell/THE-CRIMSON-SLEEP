#include "LinkedList.h"

LinkedList::LinkedList() {
    head = nullptr;
    count = 0;
}

LinkedList::~LinkedList() {
    ListNode* current = head;
    while (current != nullptr) {
        ListNode* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
}

void LinkedList::addItem(const Item& item) {
    ListNode* newNode = new ListNode();
    newNode->data = item;
    newNode->next = head; // insert at front, O(1)
    head = newNode;
    count++;
}

bool LinkedList::removeItem(const std::string& name) {
    ListNode* current = head;
    ListNode* previous = nullptr;

    while (current != nullptr) {
        if (current->data.name == name) {
            if (previous == nullptr) {
                head = current->next; // removing the head
            } else {
                previous->next = current->next;
            }
            delete current;
            count--;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false; // item not found
}

bool LinkedList::hasItem(const std::string& name) const {
    return findItem(name) != nullptr;
}

Item* LinkedList::findItem(const std::string& name) const {
    ListNode* current = head;
    while (current != nullptr) {
        if (current->data.name == name) {
            return &(current->data);
        }
        current = current->next;
    }
    return nullptr;
}

int LinkedList::size() const {
    return count;
}

bool LinkedList::isEmpty() const {
    return head == nullptr;
}

void LinkedList::getContents(Item* outArray, int maxSize, int& outCount) const {
    outCount = 0;
    ListNode* current = head;
    while (current != nullptr && outCount < maxSize) {
        outArray[outCount] = current->data;
        outCount++;
        current = current->next;
    }
}
