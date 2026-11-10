#pragma once
// ---------------------------------------------------------------------
// Priority Queue (hand-written, array-based binary MAX-heap).
// USED FOR: active threats.  Every frame the Threat Director inserts
// whatever is endangering the player (wraith attack, devil nearby, ghost
// nearby, spikes/fire underfoot, darkness) with a danger priority; the
// single most dangerous one (extractMax) drives the HUD warning and the
// fear rate.  insert / extractMax are O(log n), peek is O(1).
// T must expose an int member `priority`.
// ---------------------------------------------------------------------
template <typename T, int CAPACITY = 64>
class PriorityQueue {
	T heap[CAPACITY];
	int count = 0;

	static void swap(T& a, T& b) { T t = a; a = b; b = t; }
	void siftUp(int i) {
		while (i > 0) {
			int p = (i - 1) / 2;
			if (heap[p].priority >= heap[i].priority) break;
			swap(heap[p], heap[i]);
			i = p;
		}
	}
	void siftDown(int i) {
		for (;;) {
			int l = 2 * i + 1, r = l + 1, best = i;
			if (l < count && heap[l].priority > heap[best].priority) best = l;
			if (r < count && heap[r].priority > heap[best].priority) best = r;
			if (best == i) return;
			swap(heap[i], heap[best]);
			i = best;
		}
	}
public:
	bool insert(const T& v) {
		if (count >= CAPACITY) return false;
		heap[count] = v;
		siftUp(count++);
		return true;
	}
	bool extractMax(T* out) {
		if (count == 0) return false;
		if (out) *out = heap[0];
		heap[0] = heap[--count];
		siftDown(0);
		return true;
	}
	bool peek(T* out) const { if (count == 0) return false; *out = heap[0]; return true; }
	bool empty() const { return count == 0; }
	int size() const { return count; }
	void clear() { count = 0; }
	const T& at(int i) const { return heap[i]; } // raw heap order (debug panel)
};
