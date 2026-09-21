#include "BST.h"

BST::BST() {
    root = nullptr;
    count = 0;
}

BST::~BST() {
    destroyTree(root);
    root = nullptr;
}

void BST::destroyTree(TreeNode* node) {
    if (node == nullptr) {
        return;
    }
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

BST::TreeNode* BST::insertNode(TreeNode* node, const ClueInfo& info) {
    if (node == nullptr) {
        TreeNode* newNode = new TreeNode();
        newNode->data = info;
        newNode->left = nullptr;
        newNode->right = nullptr;
        count++;
        return newNode;
    }

    if (info.id < node->data.id) {
        node->left = insertNode(node->left, info);
    } else if (info.id > node->data.id) {
        node->right = insertNode(node->right, info);
    }
    // if id already exists, ignore (no duplicate ids expected)
    return node;
}

void BST::insert(const ClueInfo& info) {
    root = insertNode(root, info);
}

BST::TreeNode* BST::searchNode(TreeNode* node, int id) const {
    if (node == nullptr) {
        return nullptr;
    }
    if (id == node->data.id) {
        return node;
    }
    if (id < node->data.id) {
        return searchNode(node->left, id);
    }
    return searchNode(node->right, id);
}

BST::TreeNode* BST::searchNodeByName(TreeNode* node, const std::string& name) const {
    // Names are not the sort key, so a name lookup still has to visit
    // every node in the worst case (O(n)) - unlike the id search above.
    if (node == nullptr) {
        return nullptr;
    }
    if (node->data.name == name) {
        return node;
    }
    TreeNode* found = searchNodeByName(node->left, name);
    if (found != nullptr) {
        return found;
    }
    return searchNodeByName(node->right, name);
}

ClueInfo* BST::search(int id) {
    TreeNode* node = searchNode(root, id);
    if (node == nullptr) {
        return nullptr;
    }
    return &(node->data);
}

ClueInfo* BST::searchByName(const std::string& name) {
    TreeNode* node = searchNodeByName(root, name);
    if (node == nullptr) {
        return nullptr;
    }
    return &(node->data);
}

void BST::markDiscovered(int id) {
    TreeNode* node = searchNode(root, id);
    if (node != nullptr) {
        node->data.discovered = true;
    }
}

int BST::size() const {
    return count;
}

void BST::inorderCollect(TreeNode* node, ClueInfo* outArray, int maxSize, int& outCount) const {
    if (node == nullptr || outCount >= maxSize) {
        return;
    }
    inorderCollect(node->left, outArray, maxSize, outCount);
    if (outCount < maxSize) {
        outArray[outCount] = node->data;
        outCount++;
    }
    inorderCollect(node->right, outArray, maxSize, outCount);
}

void BST::getContentsInOrder(ClueInfo* outArray, int maxSize, int& outCount) const {
    outCount = 0;
    inorderCollect(root, outArray, maxSize, outCount);
}
