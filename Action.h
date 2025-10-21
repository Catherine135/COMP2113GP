#pragma once
#include <vector>
#include <utility>

struct Action {}; 
struct Move : public Action {
    std::pair<int, int> from; // (x, y) coordinates of the source tile
    std::pair<int, int> to;   // (x, y) coordinates of the destination tile
    int army;                 // Number of armies to move
}; 