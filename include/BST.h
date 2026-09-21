#ifndef BST_H
#define BST_H

#include <string>

// A searchable clue/item record. "discovered" flips to true the
// moment the player actually reads/inspects it in-game, and puzzles
// later check that flag through the BST — this is what makes the
// tree a real gameplay system instead of a static lookup table.
struct ClueInfo {
    int id;
    std::string name;
    std::string description;
    bool discovered;
};

// ---------------------------------------------------------------------
// Binary Search Tree (manually implemented, keyed by integer id).
//
// USED FOR: Looking up clue/item information by id or name (the 'F'
// inspect/search action) and, more importantly, for PUZZLE GATES:
// e.g. the Study puzzle checks search(DIARY_PAGE_ID)->discovered
// before it allows the player to make sense of the Basement Key.
// ---------------------------------------------------------------------
class BST {
private:
    struct TreeNode {
        ClueInfo data;
        TreeNode* left;
        TreeNode* right;
    };

    TreeNode* root;
    int count;

    TreeNode* insertNode(TreeNode* node, const ClueInfo& info);
    TreeNode* searchNode(TreeNode* node, int id) const;
    TreeNode* searchNodeByName(TreeNode* node, const std::string& name) const;
    void destroyTree(TreeNode* node);
    void inorderCollect(TreeNode* node, ClueInfo* outArray, int maxSize, int& outCount) const;

public:
    BST();
    ~BST();

    // Owns its nodes via raw pointers - copying is unsafe, so it is
    // disallowed (Rule of 3).
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

    void insert(const ClueInfo& info);         // O(log n) average
    ClueInfo* search(int id);                   // O(log n) average, nullptr if not found
    ClueInfo* searchByName(const std::string& name);
    void markDiscovered(int id);                 // mark a clue as read/found
    int size() const;

    // For the debug panel: in-order traversal (sorted by id).
    void getContentsInOrder(ClueInfo* outArray, int maxSize, int& outCount) const;
};

#endif
