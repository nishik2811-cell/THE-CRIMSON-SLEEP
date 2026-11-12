// Unit tests for the hand-written data structures (no SFML window needed).
//   make test
#include <cstdio>
#include "ds/Stack.h"
#include "ds/Queue.h"
#include "ds/PriorityQueue.h"
#include "ds/LinkedList.h"
#include "ds/BST.h"
#include "ds/Graph.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

struct T { int priority; int id; };

int main() {
	// Stack: LIFO, used for "Go Back".
	{
		Stack<int> s; int v;
		CHECK(s.empty() && !s.pop(&v));
		for (int i = 1; i <= 5; ++i) s.push(i);
		CHECK(s.size() == 5 && s.top(&v) && v == 5);
		CHECK(s.pop(&v) && v == 5 && s.pop(&v) && v == 4);
		int arr[8]; CHECK(s.toArray(arr, 8) == 3 && arr[0] == 3 && arr[2] == 1);
	}
	// Queue: FIFO, used for haunted events and BFS frontiers.
	{
		Queue<int> q; int v;
		for (int i = 1; i <= 4; ++i) q.enqueue(i);
		CHECK(q.dequeue(&v) && v == 1 && q.dequeue(&v) && v == 2);
		q.enqueue(9);
		CHECK(q.size() == 3 && q.front(&v) && v == 3);
		while (q.dequeue()) {}
		CHECK(q.empty()); q.enqueue(7); CHECK(q.front(&v) && v == 7);
	}
	// Priority queue: max-heap, the most dangerous threat comes out first
	// even if it was inserted after weaker ones.
	{
		PriorityQueue<T> pq; T t;
		pq.insert({ 20, 1 }); pq.insert({ 0, 2 }); pq.insert({ 60, 3 }); pq.insert({ 100, 4 }); pq.insert({ 40, 5 });
		CHECK(pq.peek(&t) && t.id == 4);
		int last = 1000, n = 0;
		while (pq.extractMax(&t)) { CHECK(t.priority <= last); last = t.priority; ++n; }
		CHECK(n == 5 && pq.empty());
	}
	// The raw heap array is only partly sorted: slot 1 need not be the
	// runner-up.  Popping a COPY (what the console does) gives true order
	// and leaves the live queue untouched.
	{
		PriorityQueue<T> pq; T t;
		pq.insert({ 90, 1 }); pq.insert({ 10, 2 }); pq.insert({ 50, 3 });
		CHECK(pq.at(1).priority == 10 && pq.at(2).priority == 50);
		PriorityQueue<T> copy = pq;
		CHECK(copy.extractMax(&t) && t.id == 1 && copy.extractMax(&t) && t.id == 3);
		CHECK(pq.size() == 3);
	}
	// Linked list: inventory insert / find / delete.
	{
		LinkedList<int> l;
		l.insertFront(1); l.insertFront(2); l.insertBack(3);
		CHECK(l.size() == 3 && l.find([](int x) { return x == 3; }));
		CHECK(l.removeIf([](int x) { return x == 2; }) && l.size() == 2);
		CHECK(!l.find([](int x) { return x == 2; }) && !l.removeIf([](int x) { return x == 42; }));
	}
	// BST: item database, in-order traversal is sorted.
	{
		BST<int> b;
		int keys[] = { 6, 3, 9, 1, 4, 7, 11, 2, 5, 8, 10, 12 };
		for (int k : keys) b.insert(k, k * 10);
		CHECK(b.size() == 12 && b.search(7) && *b.search(7) == 70 && !b.search(13));
		b.insert(7, 71); CHECK(*b.search(7) == 71 && b.size() == 12);
		int prev = 0; bool sorted = true;
		b.inOrder([&](int k, int) { if (k < prev) sorted = false; prev = k; });
		CHECK(sorted && b.height() <= 5);
	}
	// Graph + BFS/DFS: shortest route respects locks; DFS finds hidden rooms.
	{
		Graph g;
		g.addEdge(1, 2, 1); g.addEdge(2, 1, 1);
		g.addEdge(2, 3, 2); g.addEdge(3, 2, 1);
		g.addEdge(2, 4, 3, true, 99); g.addEdge(4, 2, 1);
		g.addEdge(3, 4, 2); g.addEdge(4, 3, 2);
		g.addEdge(4, 5, 4, false, 0, true); g.addEdge(5, 4, 1, false, 0, true);
		int path[16];
		auto open = [](const Edge& e) { return !e.locked && !e.hidden; };
		int n = g.bfsPath(1, 4, path, 16, open);
		CHECK(n == 4 && path[0] == 1 && path[1] == 2 && path[2] == 3 && path[3] == 4);  // detours around the lock
		g.unlock(2, 4);
		n = g.bfsPath(1, 4, path, 16, open);
		CHECK(n == 3 && path[1] == 2 && path[2] == 4);                                 // shorter once unlocked
		CHECK(g.bfsPath(1, 5, path, 16, open) == 0);                                   // hidden room unreachable
		int order[16];
		bool seen5 = false; int m = g.dfsExplore(1, order, true);
		for (int i = 0; i < m; ++i) if (order[i] == 5) seen5 = true;
		CHECK(seen5 && m == 5);
		CHECK(g.dfsExplore(1, order, false) == 4);
		g.reveal(4, 5);
		CHECK(g.bfsPath(1, 5, path, 16, open) == 4);
		CHECK(g.edgeByDoor(2, 3) && g.edgeByDoor(2, 3)->to == 4);
	}
	std::printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
