#pragma once
#include <vector>
#include <utility>

/**
 * @Michael-wzl
 * @brief Base structure for game actions.
 */
struct Action {}; 

/**
 * @Michael-wzl
 * @brief Structure representing a move action in the game.
 */
struct Move : public Action {
    std::pair<int, int> from; // (x, y) coordinates of the source tile
    std::pair<int, int> to;   // (x, y) coordinates of the destination tile
    int army;                 // Number of armies to move
}; 