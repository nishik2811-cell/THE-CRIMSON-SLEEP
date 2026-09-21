#include "Player.h"

Player::Player() {
    currentRoomId = ENTRANCE_HALL;
    health = 100;
    fear = 0;
    for (int i = 0; i < ROOM_COUNT; i++) {
        visited[i] = false;
    }
    posX = 570.f;
    posY = 290.f;
    enterRoom(ENTRANCE_HALL); // pushes onto the history stack, marks visited
}

void Player::enterRoom(int roomId) {
    currentRoomId = roomId;
    history.push(roomId); // Stack usage: record every room we step into
    visited[roomId] = true;
    posX = 570.f; // spawn near the bottom-center of the new room
    posY = 290.f;
}

float Player::getX() const { return posX; }
float Player::getY() const { return posY; }

void Player::moveBy(float dx, float dy, float maxW, float maxH) {
    posX += dx;
    posY += dy;
    if (posX < 20.f) posX = 20.f;
    if (posY < 20.f) posY = 20.f;
    if (posX > maxW - 20.f) posX = maxW - 20.f;
    if (posY > maxH - 20.f) posY = maxH - 20.f;
}

void Player::setPosition(float x, float y) {
    posX = x;
    posY = y;
}

bool Player::goBack(int& outRoomId) {
    // The stack always has the starting room at the bottom, so if only
    // one entry remains there is nowhere left to go back to.
    if (history.size() <= 1) {
        return false;
    }
    history.pop();               // discard current room
    int previousRoom = history.peek(); // new top = room we came from
    currentRoomId = previousRoom;
    posX = 570.f; // re-spawn near the bottom-center of the room we returned to
    posY = 290.f;
    outRoomId = previousRoom;
    return true;
}

int Player::getCurrentRoom() const {
    return currentRoomId;
}

bool Player::hasVisited(int roomId) const {
    return visited[roomId];
}

int Player::getHealth() const {
    return health;
}

int Player::getFear() const {
    return fear;
}

void Player::takeDamage(int amount) {
    health -= amount;
    if (health < 0) health = 0;
}

void Player::addFear(int amount) {
    fear += amount;
    if (fear > 100) fear = 100;
}

void Player::heal(int amount) {
    health += amount;
    if (health > 100) health = 100;
}

void Player::reduceFear(int amount) {
    fear -= amount;
    if (fear < 0) fear = 0;
}

bool Player::isDead() const {
    return health <= 0;
}

bool Player::isInsane() const {
    return fear >= 100;
}

void Player::addItem(const Item& item) {
    inventory.addItem(item);
}

bool Player::useItem(const std::string& name) {
    return inventory.removeItem(name);
}

bool Player::hasItem(const std::string& name) const {
    return inventory.hasItem(name);
}

const LinkedList& Player::getInventory() const {
    return inventory;
}

const Stack& Player::getHistory() const {
    return history;
}
