#pragma once
// ---------------------------------------------------------------------
// Binary Search Tree (hand-written, unbalanced).
// USED FOR: the item / clue database, keyed by item id.  Puzzle gates
// query it: e.g. the Ossuary door only opens if the "Old Photograph"
// record has been *discovered* (read), and every locked edge in the
// house graph looks up the name of the item it needs here.
// search / insert: O(log n) average, O(n) worst case.
// ---------------------------------------------------------------------
template <typename V>
class BST {
	struct Node { int key; V value; Node* left; Node* right; };
	Node* root = nullptr;
	int count = 0;

	static void destroy(Node* n) { if (!n) return; destroy(n->left); destroy(n->right); delete n; }
	template <typename Fn> static void inorder(Node* n, Fn& fn) {
		if (!n) return;
		inorder(n->left, fn); fn(n->key, n->value); inorder(n->right, fn);
	}
	template <typename Fn> static void walkNode(Node* n, int depth, int parent, Fn& fn) {
		if (!n) return;
		walkNode(n->left, depth + 1, n->key, fn); fn(n->key, n->value, depth, parent); walkNode(n->right, depth + 1, n->key, fn);
	}
	static int heightOf(Node* n) {
		if (!n) return 0;
		int l = heightOf(n->left), r = heightOf(n->right);
		return 1 + (l > r ? l : r);
	}
public:
	BST() {}
	~BST() { destroy(root); }
	BST(const BST&) = delete;
	BST& operator=(const BST&) = delete;

	// Inserts or overwrites.
	void insert(int key, const V& value) {
		Node** cur = &root;
		while (*cur) {
			if (key == (*cur)->key) { (*cur)->value = value; return; }
			cur = key < (*cur)->key ? &(*cur)->left : &(*cur)->right;
		}
		*cur = new Node{ key, value, nullptr, nullptr };
		++count;
	}
	V* search(int key) {
		Node* n = root;
		while (n) {
			if (key == n->key) return &n->value;
			n = key < n->key ? n->left : n->right;
		}
		return nullptr;
	}
	const V* search(int key) const { return const_cast<BST*>(this)->search(key); }
	template <typename Fn> void inOrder(Fn fn) const { inorder(root, fn); }
	// In-order walk that also reports each node's depth and its parent's key
	// (-1 for the root): enough to draw the tree.
	template <typename Fn> void walk(Fn fn) const { walkNode(root, 0, -1, fn); }
	// Keys visited by search(key), root first.  Returns how many were written.
	int pathTo(int key, int* out, int max) const {
		int n = 0;
		for (Node* c = root; c && n < max; c = key < c->key ? c->left : c->right) {
			out[n++] = c->key;
			if (c->key == key) break;
		}
		return n;
	}
	int size() const { return count; }
	int height() const { return heightOf(root); }
	void clear() { destroy(root); root = nullptr; count = 0; }
};
