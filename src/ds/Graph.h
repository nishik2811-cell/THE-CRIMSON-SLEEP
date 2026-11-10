#pragma once
#include "Queue.h"
#include "Stack.h"
// ---------------------------------------------------------------------
// Graph (hand-written ADJACENCY LIST).
// USED FOR: the house layout.  Rooms are nodes; each door/passage is a
// directed edge stored in a linked list hanging off adjacency[room].
// An edge remembers which door tile (doorNo) it belongs to, whether it is
// locked, which item unlocks it, and whether it is still hidden (secret
// passages only appear once the DFS "sense the house" exploration finds
// them).
//
// BFS  -> shortest room route for the hint system.          O(V + E)
// DFS  -> explores every reachable room incl. hidden ones;
//         used to reveal the Hidden Tunnel.                  O(V + E)
// ---------------------------------------------------------------------
const int MAX_ROOMS = 16;

struct Edge {
	int to;
	int doorNo;        // door tile id on the map: tile x = 3 + doorNo
	bool locked;
	bool hidden;
	int requiredItem;  // item id that unlocks it (0 = none / special)
	Edge* next;
};

class Graph {
	Edge* adjacency[MAX_ROOMS];
	int nodeCount;

	void dfsVisit(int room, bool* visited, int* order, int& n, bool includeHidden) const {
		visited[room] = true;
		order[n++] = room;
		for (Edge* e = adjacency[room]; e; e = e->next)
			if (!visited[e->to] && (includeHidden || !e->hidden))
				dfsVisit(e->to, visited, order, n, includeHidden);
	}
public:
	explicit Graph(int n = MAX_ROOMS) : nodeCount(n) { for (int i = 0; i < MAX_ROOMS; ++i) adjacency[i] = nullptr; }
	~Graph() { clear(); }
	Graph(const Graph&) = delete;
	Graph& operator=(const Graph&) = delete;

	void clear() {
		for (int i = 0; i < MAX_ROOMS; ++i) {
			while (adjacency[i]) { Edge* e = adjacency[i]; adjacency[i] = e->next; delete e; }
		}
	}
	int size() const { return nodeCount; }

	void addEdge(int from, int to, int doorNo, bool locked = false, int requiredItem = 0, bool hidden = false) {
		adjacency[from] = new Edge{ to, doorNo, locked, hidden, requiredItem, adjacency[from] };
	}
	Edge* edgeByDoor(int from, int doorNo) const {
		for (Edge* e = adjacency[from]; e; e = e->next) if (e->doorNo == doorNo) return e;
		return nullptr;
	}
	Edge* edgeTo(int from, int to) const {
		for (Edge* e = adjacency[from]; e; e = e->next) if (e->to == to) return e;
		return nullptr;
	}
	const Edge* neighbours(int room) const { return adjacency[room]; }

	// Unlocks the passage in both directions (if a reverse edge exists).
	void unlock(int a, int b) {
		if (Edge* e = edgeTo(a, b)) e->locked = false;
		if (Edge* e = edgeTo(b, a)) e->locked = false;
	}
	void reveal(int a, int b) {
		if (Edge* e = edgeTo(a, b)) e->hidden = false;
		if (Edge* e = edgeTo(b, a)) e->hidden = false;
	}

	// BFS over edges accepted by canUse(edge). Writes the room sequence
	// start..target into path and returns its length (0 = unreachable).
	template <typename CanUse>
	int bfsPath(int start, int target, int* path, int maxLen, CanUse canUse) const {
		bool visited[MAX_ROOMS] = {};
		int parent[MAX_ROOMS];
		for (int i = 0; i < MAX_ROOMS; ++i) parent[i] = -1;
		Queue<int> frontier;
		frontier.enqueue(start);
		visited[start] = true;
		int room;
		while (frontier.dequeue(&room)) {
			if (room == target) break;
			for (Edge* e = adjacency[room]; e; e = e->next) {
				if (visited[e->to] || !canUse(*e)) continue;
				visited[e->to] = true;
				parent[e->to] = room;
				frontier.enqueue(e->to);
			}
		}
		if (!visited[target]) return 0;
		// Walk parents back from the target using a Stack to reverse the order.
		Stack<int> reversed;
		for (int r = target; r != -1; r = parent[r]) reversed.push(r);
		int n = 0;
		while (n < maxLen && reversed.pop(&room)) path[n++] = room;
		return n;
	}

	// Recursive DFS from start. Returns the number of rooms visited.
	int dfsExplore(int start, int* order, bool includeHidden) const {
		bool visited[MAX_ROOMS] = {};
		int n = 0;
		dfsVisit(start, visited, order, n, includeHidden);
		return n;
	}
};
