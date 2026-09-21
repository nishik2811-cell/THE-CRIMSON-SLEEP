#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "Threat.h"

const int MAX_THREATS = 50;

// ---------------------------------------------------------------------
// Priority Queue (manually implemented as an array-based binary
// MAX-HEAP, keyed on Threat::priority).
//
// USED FOR: Active threats in the house (dark room, trap, ghost
// nearby, ghost attack). No matter what order threats appear in,
// extractMax() always resolves the single most dangerous one first
// (e.g. a Ghost Attack is always handled before a Dark Room, even if
// the Dark Room was added first). This is the classic heap used to
// teach O(log n) insert/remove, so it is implemented with plain
// arrays and index math (parent = (i-1)/2, children = 2i+1, 2i+2)
// rather than hidden inside std::priority_queue.
// ---------------------------------------------------------------------
class PriorityQueue {
private:
    Threat heap[MAX_THREATS];
    int count;

    void heapifyUp(int index);
    void heapifyDown(int index);
    void swapThreats(int i, int j);

public:
    PriorityQueue();

    void insert(const Threat& t);   // O(log n)
    Threat extractMax();            // remove & return highest-priority threat, O(log n)
    Threat peekMax() const;         // look at highest-priority threat without removing
    bool isEmpty() const;
    int size() const;

    // For the debug panel: copy the raw heap array (not fully sorted,
    // but shows exactly what the data structure holds).
    void getContents(Threat* outArray, int maxSize, int& outCount) const;
};

#endif
