#pragma once
// ---------------------------------------------------------------------
// Stack (hand-written, singly linked).
// USED FOR: the player's room-visit history. Every room entered is
// pushed; the "Go Back" key (B) pops the current room and walks back to
// the room underneath it.  push / pop / top are all O(1).
// ---------------------------------------------------------------------
template <typename T>
class Stack {
	struct Node { T value; Node* next; };
	Node* head = nullptr;
	int count = 0;
public:
	Stack() {}
	~Stack() { clear(); }
	Stack(const Stack&) = delete;
	Stack& operator=(const Stack&) = delete;

	void push(const T& v) { head = new Node{ v, head }; ++count; }
	bool pop(T* out = nullptr) {
		if (!head) return false;
		Node* n = head;
		if (out) *out = n->value;
		head = n->next;
		delete n;
		--count;
		return true;
	}
	bool top(T* out) const { if (!head) return false; *out = head->value; return true; }
	bool empty() const { return head == nullptr; }
	int size() const { return count; }
	void clear() { while (pop()) {} }

	// Copies up to max items, top first (used by the debug panel).
	int toArray(T* out, int max) const {
		int i = 0;
		for (Node* n = head; n && i < max; n = n->next) out[i++] = n->value;
		return i;
	}
};
