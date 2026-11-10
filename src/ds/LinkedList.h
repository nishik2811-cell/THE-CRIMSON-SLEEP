#pragma once
// ---------------------------------------------------------------------
// Linked List (hand-written, singly linked).
// USED FOR: the player's inventory.  Items are inserted on pickup and
// deleted when consumed (a key is removed from the list when it opens
// its door; the petrol can is removed when used).
// insertFront O(1); find / removeIf O(n).
// ---------------------------------------------------------------------
template <typename T>
class LinkedList {
	struct Node { T value; Node* next; };
	Node* head = nullptr;
	int count = 0;
public:
	LinkedList() {}
	~LinkedList() { clear(); }
	LinkedList(const LinkedList&) = delete;
	LinkedList& operator=(const LinkedList&) = delete;

	void insertFront(const T& v) { head = new Node{ v, head }; ++count; }
	void insertBack(const T& v) {
		Node* n = new Node{ v, nullptr };
		if (!head) { head = n; }
		else { Node* c = head; while (c->next) c = c->next; c->next = n; }
		++count;
	}
	template <typename Pred> T* find(Pred pred) {
		for (Node* n = head; n; n = n->next) if (pred(n->value)) return &n->value;
		return nullptr;
	}
	template <typename Pred> bool removeIf(Pred pred) {
		Node* prev = nullptr;
		for (Node* n = head; n; prev = n, n = n->next) {
			if (pred(n->value)) {
				if (prev) prev->next = n->next; else head = n->next;
				delete n;
				--count;
				return true;
			}
		}
		return false;
	}
	template <typename Fn> void forEach(Fn fn) const { for (Node* n = head; n; n = n->next) fn(n->value); }
	int size() const { return count; }
	bool empty() const { return head == nullptr; }
	void clear() { while (head) { Node* n = head; head = head->next; delete n; } count = 0; }
};
