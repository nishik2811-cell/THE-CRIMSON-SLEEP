#ifndef ITEM_H
#define ITEM_H

#include <string>

// Plain description of a pickable item. The "id" matches the id used
// as the BST search key in ClueDatabase (see BST.h).
struct Item {
    int id;
    std::string name;
    std::string description;
};

#endif
