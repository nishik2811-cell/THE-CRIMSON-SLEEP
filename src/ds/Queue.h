#pragma once
// ---------------------------------------------------------------------
// Queue (hand-written, singly linked with head + tail pointers).
// USED FOR: (1) the haunted-event timeline - doors slamming, lights
// flickering, whispers - which must fire in strict chronological (FIFO)
// order; (2) the frontier of the tile-level BFS that draws the hint trail;
// (3) the boot-screen slide sequence.  enqueue / dequeue are O(1).
// ---------------------------------------------------------------------
template <typename T>
class Queue {
	struct Node { T value; Node* next; };
	Node* head = nullptr;
	Node* tail = nullptr;
	int count = 0;
public:
	Queue() {}
	~Queue() { clear(); }
	Queue(const Queue&) = delete;
	Queue& operator=(const Queue&) = delete;

	void enqueue(const T& v) {
		Node* n = new Node{ v, nullptr };
		if (tail) tail->next = n; else head = n;
		tail = n;
		++count;
	}
	bool dequeue(T* out = nullptr) {
		if (!head) return false;
		Node* n = head;
		if (out) *out = n->value;
		head = n->next;
		if (!head) tail = nullptr;
		delete n;
		--count;
		return true;
	}
	bool front(T* out) const { if (!head) return false; *out = head->value; return true; }
	T* frontPtr() { return head ? &head->value : nullptr; }
	bool empty() const { return head == nullptr; }
	int size() const { return count; }
	void clear() { while (dequeue()) {} }

	int toArray(T* out, int max) const {
		int i = 0;
		for (Node* n = head; n && i < max; n = n->next) out[i++] = n->value;
		return i;
	}
};
