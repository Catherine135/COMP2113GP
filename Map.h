#pragma once
#include <random>
#include <string>
#include <vector>
#include <shared_mutex>

/// @brief Represents a tile on the game map.
struct Tile {
    int owner{-1}; // -1 indicates no owner
    int army{0}; 
    bool isCapital{false};
    bool isOriginalCapital{false};
    bool isMountain{false};
    bool isCity{false};
}; 

class Map {
public:
    /// Init a map with given width, height, and random seed.
    /// @param width Width of the map.
    /// @param height Height of the map.
    /// @param seed Random seed for map generation.
    Map(int width, int height, int seed);

    /// Get the tile at the specified coordinates.
    /// @param x X coordinate.
    /// @param y Y coordinate.
    /// @return Reference to the tile at (x, y).
    Tile getTile(int x, int y);

    /// Get the reference snapshot of the entire map.
    /// @return 2D vector representing the map state.
    std::vector<std::vector<Tile>> getSnapshot();

    /// Change the Tile at the specified coordinates.
    /// @param x X coordinate.
    /// @param y Y coordinate.
    /// @param newTile New tile to set at (x, y).
    void setTile(int x, int y, const Tile newTile);

    int getWidth() const noexcept { return width; }
    int getHeight() const noexcept { return height; }

private:
    int width;
    int height;
    int seed;
    std::vector<std::vector<Tile>> tiles;
    mutable std::shared_mutex mapMutex;

    /// Generate the map using the specified seed and DFS algorithm.
    /// @param seed Random seed for map generation.
    void generateMap(int seed);
};
