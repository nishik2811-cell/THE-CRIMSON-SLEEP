#ifndef STACK_H
#define STACK_H

// ---------------------------------------------------------------------
// Stack (manually implemented using a singly linked list)
//
// USED FOR: Player's room movement history ("Go Back" feature).
// Every time the player enters a new room, its id is PUSHED here.
// Pressing 'B' (Go Back) POPs the current room and sends the player
// back to the room underneath it. This is a real LIFO stack, not an
// array pretending to be one, and it is not the same node type used
// by Queue/LinkedList even though all three are linked structures.
// ---------------------------------------------------------------------
class Stack {
private:
    struct StackNode {
        int roomId;
        StackNode* next;
    };

    StackNode* topNode; // points to the top of the stack
    int count;          // how many rooms are currently on the stack

public:
    Stack();
    ~Stack();

    // The stack owns its nodes via raw pointers, so copying it would
    // double-free memory. Disallow copying entirely (Rule of 3).
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(int roomId);   // enter a room -> push it
    int pop();                // go back -> pop and return previous room (-1 if empty)
    int peek() const;         // look at current top without removing it (-1 if empty)
    bool isEmpty() const;
    int size() const;

    // For the on-screen Data Structure debug panel: prints the whole
    // stack from top to bottom into the given char buffer.
    void getContents(int* outArray, int maxSize, int& outCount) const;
};

#endif
