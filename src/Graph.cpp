#include "Graph.h"

Graph::Graph(int numRooms) {
    roomCount = numRooms;
    for (int i = 0; i < MAX_ROOMS; i++) {
        adjacencyHead[i] = nullptr;
    }
}

Graph::~Graph() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        EdgeNode* current = adjacencyHead[i];
        while (current != nullptr) {
            EdgeNode* next = current->next;
            delete current;
            current = next;
        }
        adjacencyHead[i] = nullptr;
    }
}

void Graph::addDirectedEdge(int from, int to, bool locked) {
    EdgeNode* newEdge = new EdgeNode();
    newEdge->destRoomId = to;
    newEdge->locked = locked;
    newEdge->next = adjacencyHead[from];
    adjacencyHead[from] = newEdge;
}

void Graph::addEdge(int roomA, int roomB, bool locked) {
    // The house's doorways work both ways, so we store the edge once
    // in each room's adjacency list.
    addDirectedEdge(roomA, roomB, locked);
    addDirectedEdge(roomB, roomA, locked);
}

void Graph::unlockEdge(int roomA, int roomB) {
    EdgeNode* current = adjacencyHead[roomA];
    while (current != nullptr) {
        if (current->destRoomId == roomB) {
            current->locked = false;
        }
        current = current->next;
    }
    current = adjacencyHead[roomB];
    while (current != nullptr) {
        if (current->destRoomId == roomA) {
            current->locked = false;
        }
        current = current->next;
    }
}

bool Graph::isConnected(int from, int to) const {
    EdgeNode* current = adjacencyHead[from];
    while (current != nullptr) {
        if (current->destRoomId == to) {
            return true;
        }
        current = current->next;
    }
    return false;
}

bool Graph::isPassable(int from, int to) const {
    EdgeNode* current = adjacencyHead[from];
    while (current != nullptr) {
        if (current->destRoomId == to) {
            return !current->locked;
        }
        current = current->next;
    }
    return false;
}

bool Graph::isLocked(int from, int to) const {
    EdgeNode* current = adjacencyHead[from];
    while (current != nullptr) {
        if (current->destRoomId == to) {
            return current->locked;
        }
        current = current->next;
    }
    return false; // no such edge at all
}

void Graph::getNeighbors(int roomId, int* outNeighbors, int maxSize, int& outCount) const {
    outCount = 0;
    EdgeNode* current = adjacencyHead[roomId];
    while (current != nullptr && outCount < maxSize) {
        outNeighbors[outCount] = current->destRoomId;
        outCount++;
        current = current->next;
    }
}

void Graph::bfsShortestPath(int start, int target, int* outPath, int& outPathLen) const {
    bool visited[MAX_ROOMS];
    int parent[MAX_ROOMS];
    for (int i = 0; i < MAX_ROOMS; i++) {
        visited[i] = false;
        parent[i] = -1;
    }

    outPathLen = 0;

    IntCircularQueue queue; // manual queue, local to this algorithm
    queue.push(start);
    visited[start] = true;

    bool found = (start == target);

    while (!queue.empty() && !found) {
        int room = queue.pop();
        EdgeNode* edge = adjacencyHead[room];
        while (edge != nullptr) {
            // BFS only travels through doors the player can actually
            // walk through right now.
            if (!edge->locked && !visited[edge->destRoomId]) {
                visited[edge->destRoomId] = true;
                parent[edge->destRoomId] = room;
                if (edge->destRoomId == target) {
                    found = true;
                    break;
                }
                queue.push(edge->destRoomId);
            }
            edge = edge->next;
        }
    }

    if (!found) {
        outPathLen = 0;
        return;
    }

    // Walk parent[] backwards from target to start, then reverse it.
    int reversePath[MAX_ROOMS];
    int reverseLen = 0;
    int node = target;
    while (node != -1) {
        reversePath[reverseLen] = node;
        reverseLen++;
        node = parent[node];
    }

    for (int i = 0; i < reverseLen; i++) {
        outPath[i] = reversePath[reverseLen - 1 - i];
    }
    outPathLen = reverseLen;
}

void Graph::dfsVisit(int roomId, bool* visited, int* outOrder, int& outCount, int maxSize) const {
    if (visited[roomId] || outCount >= maxSize) {
        return;
    }
    visited[roomId] = true;
    outOrder[outCount] = roomId;
    outCount++;

    EdgeNode* edge = adjacencyHead[roomId];
    while (edge != nullptr) {
        if (!visited[edge->destRoomId]) {
            dfsVisit(edge->destRoomId, visited, outOrder, outCount, maxSize);
        }
        edge = edge->next;
    }
}

void Graph::dfsExplore(int startRoom, int* outOrder, int& outCount) const {
    bool visited[MAX_ROOMS];
    for (int i = 0; i < MAX_ROOMS; i++) {
        visited[i] = false;
    }
    outCount = 0;
    dfsVisit(startRoom, visited, outOrder, outCount, MAX_ROOMS);
}

bool Graph::dfsCanReach(int startRoom, int targetRoom) const {
    int order[MAX_ROOMS];
    int orderCount = 0;
    dfsExplore(startRoom, order, orderCount);
    for (int i = 0; i < orderCount; i++) {
        if (order[i] == targetRoom) {
            return true;
        }
    }
    return false;
}
