#pragma once
#include <random>
#include <string>
#include <vector>
#include <shared_mutex>

// 0 for normal, 1 for mountain, 2 for city, 3 for capital
enum class LANDTYPE : unsigned char {
    NORMAL = 0,
    MOUNTAIN = 1,
    CITY = 2,
    CAPITAL = 3
};

/// @brief Represents a tile on the game map.
struct Tile {
    int owner{-1}; // -1 indicates no owner， 0 for human, 1 for ai
    int army{0}; 
    LANDTYPE landType{LANDTYPE::NORMAL}; 
    bool isOriginalCapital{false};

    bool isMountain() const { return (landType==LANDTYPE::MOUNTAIN); };
    bool isCity() const { return (landType==LANDTYPE::CITY); };
    bool isCapital() const { return (landType==LANDTYPE::CAPITAL); };
}; 

class Map {
public:
    /// Init a map with given level.
    /// @param level Difficulty level for map generation.
    Map(int level);

    /// Regenerate the map with new level.
    /// @param level Difficulty level for map generation.
    void regenerateMap(int level);

    /// Get the tile at the specified coordinates.
    /// @param x X coordinate.
    /// @param y Y coordinate.
    /// @return Reference to the tile at (x, y).
    Tile getTile(int x, int y) const;

    /// Get the reference snapshot of the entire map.
    /// @return 2D vector representing the map state.
    std::vector<std::vector<Tile>> getSnapshot() const;

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
