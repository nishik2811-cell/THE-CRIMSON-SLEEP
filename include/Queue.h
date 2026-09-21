#ifndef QUEUE_H
#define QUEUE_H

#include "Event.h"

// ---------------------------------------------------------------------
// Queue (manually implemented using a singly linked list with
// front/rear pointers).
//
// USED FOR: Chronological haunted-house events (lights flicker, door
// slams, whispers, etc). Events are ENQUEUEd as they happen and
// DEQUEUEd in the exact order they occurred (FIFO) so the oldest
// unresolved spooky event is always handled first.
// ---------------------------------------------------------------------
class Queue {
private:
    struct QueueNode {
        Event data;
        QueueNode* next;
    };

    QueueNode* frontNode;
    QueueNode* rearNode;
    int count;

public:
    Queue();
    ~Queue();

    // Owns its nodes via raw pointers - copying is unsafe, so it is
    // disallowed (Rule of 3).
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const Event& e); // add event to the back of the line
    Event dequeue();              // remove & return event from the front
    Event peekFront() const;      // look at the next event without removing it
    bool isEmpty() const;
    int size() const;

    // For the debug panel: copy up to maxSize pending events (front to back).
    void getContents(Event* outArray, int maxSize, int& outCount) const;
};

#endif
