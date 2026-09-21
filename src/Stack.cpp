#include "Stack.h"

Stack::Stack() {
    topNode = nullptr;
    count = 0;
}

Stack::~Stack() {
    // Free every node so we don't leak memory.
    while (!isEmpty()) {
        pop();
    }
}

void Stack::push(int roomId) {
    StackNode* newNode = new StackNode();
    newNode->roomId = roomId;
    newNode->next = topNode;
    topNode = newNode;
    count++;
}

int Stack::pop() {
    if (isEmpty()) {
        return -1;
    }
    StackNode* oldTop = topNode;
    int roomId = oldTop->roomId;
    topNode = topNode->next;
    delete oldTop;
    count--;
    return roomId;
}

int Stack::peek() const {
    if (isEmpty()) {
        return -1;
    }
    return topNode->roomId;
}

bool Stack::isEmpty() const {
    return topNode == nullptr;
}

int Stack::size() const {
    return count;
}

void Stack::getContents(int* outArray, int maxSize, int& outCount) const {
    outCount = 0;
    StackNode* current = topNode;
    while (current != nullptr && outCount < maxSize) {
        outArray[outCount] = current->roomId;
        outCount++;
        current = current->next;
    }
}
