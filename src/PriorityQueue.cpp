#include "PriorityQueue.h"

PriorityQueue::PriorityQueue() {
    count = 0;
}

void PriorityQueue::swapThreats(int i, int j) {
    Threat temp = heap[i];
    heap[i] = heap[j];
    heap[j] = temp;
}

void PriorityQueue::heapifyUp(int index) {
    // Keep swapping the new element up while it is more dangerous
    // than its parent (max-heap property).
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (heap[index].priority > heap[parent].priority) {
            swapThreats(index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

void PriorityQueue::heapifyDown(int index) {
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        if (left < count && heap[left].priority > heap[largest].priority) {
            largest = left;
        }
        if (right < count && heap[right].priority > heap[largest].priority) {
            largest = right;
        }
        if (largest == index) {
            break; // heap property restored
        }
        swapThreats(index, largest);
        index = largest;
    }
}

void PriorityQueue::insert(const Threat& t) {
    if (count >= MAX_THREATS) {
        return; // heap is full; ignore (should not happen in normal play)
    }
    heap[count] = t;
    heapifyUp(count);
    count++;
}

Threat PriorityQueue::extractMax() {
    // Caller should check isEmpty() first. If empty, return a harmless
    // placeholder threat so the game does not crash.
    if (isEmpty()) {
        Threat empty;
        empty.type = THREAT_DARK_ROOM;
        empty.description = "";
        empty.roomId = -1;
        empty.priority = 0;
        empty.healthDamage = 0;
        empty.fearIncrease = 0;
        return empty;
    }

    Threat top = heap[0];
    heap[0] = heap[count - 1]; // move last element to root
    count--;
    heapifyDown(0);
    return top;
}

Threat PriorityQueue::peekMax() const {
    return heap[0];
}

bool PriorityQueue::isEmpty() const {
    return count == 0;
}

int PriorityQueue::size() const {
    return count;
}

void PriorityQueue::getContents(Threat* outArray, int maxSize, int& outCount) const {
    outCount = 0;
    for (int i = 0; i < count && i < maxSize; i++) {
        outArray[outCount] = heap[i];
        outCount++;
    }
}
