#ifndef GRAPH_H
#define GRAPH_H

const int MAX_ROOMS = 16;

// ---------------------------------------------------------------------
// Graph (manually implemented as an ADJACENCY LIST).
//
// USED FOR: The haunted house layout itself. Every room is a node
// (0..MAX_ROOMS-1). Every doorway is an edge stored as a small linked
// list hanging off adjacencyHead[roomId] -- this array-of-linked-lists
// IS the adjacency list. Some edges start LOCKED (e.g. the Basement
// door) and only become passable once the player unlocks them.
// Player movement (Game::movePlayer) walks this structure directly;
// it never teleports between rooms that aren't connected & unlocked.
//
// BFS and DFS (below) also operate directly on this adjacency list.
// ---------------------------------------------------------------------
class Graph {
private:
    struct EdgeNode {
        int destRoomId;
        bool locked;
        EdgeNode* next;
    };

    EdgeNode* adjacencyHead[MAX_ROOMS]; // array of linked-list heads
    int roomCount;

    // Small fixed-size circular queue used only inside bfs() so the
    // Graph module has zero STL dependency. Not exposed publicly.
    struct IntCircularQueue {
        int data[MAX_ROOMS];
        int front, rear, count;
        IntCircularQueue() { front = 0; rear = -1; count = 0; }
        void push(int v) { rear = (rear + 1) % MAX_ROOMS; data[rear] = v; count++; }
        int pop() { int v = data[front]; front = (front + 1) % MAX_ROOMS; count--; return v; }
        bool empty() const { return count == 0; }
    };

    void addDirectedEdge(int from, int to, bool locked);
    void dfsVisit(int roomId, bool* visited, int* outOrder, int& outCount, int maxSize) const;

public:
    Graph(int numRooms);
    ~Graph();

    // Owns its edge nodes via raw pointers - copying is unsafe, so it
    // is disallowed (Rule of 3).
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // Adds a two-way passage between two rooms. Locked passages exist
    // in the graph but cannot be traversed until unlocked.
    void addEdge(int roomA, int roomB, bool locked);

    void unlockEdge(int roomA, int roomB); // removes the lock in both directions
    bool isConnected(int from, int to) const;   // edge exists, ignoring lock state
    bool isPassable(int from, int to) const;    // edge exists AND is not locked
    bool isLocked(int from, int to) const;

    // Fills outNeighbors with every room directly reachable (edge
    // exists) from roomId, and reports how many were written.
    void getNeighbors(int roomId, int* outNeighbors, int maxSize, int& outCount) const;

    // BREADTH-FIRST SEARCH: shortest number of rooms from start to
    // target using only PASSABLE (unlocked) edges. Fills outPath with
    // the room sequence [start, ..., target]. outPathLen == 0 means
    // no route currently exists. Used for the in-game 'H' hint system.
    void bfsShortestPath(int start, int target, int* outPath, int& outPathLen) const;

    // DEPTH-FIRST SEARCH: explores as deep as possible down each
    // branch from startRoom using ALL edges (locked or not, since this
    // represents the player's eyes/knowledge of the house layout, not
    // physical movement). Used to reveal/explore a hidden section of
    // the house and to check whether a target room is reachable at
    // all. Returns the order rooms were visited in.
    void dfsExplore(int startRoom, int* outOrder, int& outCount) const;
    bool dfsCanReach(int startRoom, int targetRoom) const;
};

#endif
