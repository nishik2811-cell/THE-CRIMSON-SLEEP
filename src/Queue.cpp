#include "Queue.h"

Queue::Queue() {
    frontNode = nullptr;
    rearNode = nullptr;
    count = 0;
}

Queue::~Queue() {
    while (!isEmpty()) {
        dequeue();
    }
}

void Queue::enqueue(const Event& e) {
    QueueNode* newNode = new QueueNode();
    newNode->data = e;
    newNode->next = nullptr;

    if (rearNode == nullptr) {
        // Queue was empty; new node is both front and rear.
        frontNode = newNode;
        rearNode = newNode;
    } else {
        rearNode->next = newNode;
        rearNode = newNode;
    }
    count++;
}

Event Queue::dequeue() {
    Event empty; // returned if queue is empty; caller should check isEmpty() first
    empty.type = LIGHTS_FLICKER;
    empty.description = "";
    empty.roomId = -1;
    empty.fearIncrease = 0;

    if (isEmpty()) {
        return empty;
    }

    QueueNode* oldFront = frontNode;
    Event data = oldFront->data;
    frontNode = frontNode->next;
    if (frontNode == nullptr) {
        rearNode = nullptr; // queue is now empty
    }
    delete oldFront;
    count--;
    return data;
}

Event Queue::peekFront() const {
    return frontNode->data;
}

bool Queue::isEmpty() const {
    return frontNode == nullptr;
}

int Queue::size() const {
    return count;
}

void Queue::getContents(Event* outArray, int maxSize, int& outCount) const {
    outCount = 0;
    QueueNode* current = frontNode;
    while (current != nullptr && outCount < maxSize) {
        outArray[outCount] = current->data;
        outCount++;
        current = current->next;
    }
}
