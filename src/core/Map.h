#pragma once
#include <random>
#include <string>
#include <vector>
#include <shared_mutex>

/**
 * @Michael-wzl
 * @brief 0 for normal, 1 for mountain, 2 for city, 3 for capital
 */
enum class LANDTYPE : unsigned char {
    NORMAL = 0,
    MOUNTAIN = 1,
    CITY = 2,
    CAPITAL = 3
};

/**
 * @Michael-wzl
 * @brief Tile structure representing each cell on the map.
 */
struct Tile {
    int owner{-1}; // -1 indicates no owner， 0 for human, 1 for ai
    int army{0}; 
    LANDTYPE landType{LANDTYPE::NORMAL}; 
    bool isOriginalCapital{false};

    bool isMountain() const { return (landType==LANDTYPE::MOUNTAIN); };
    bool isCity() const { return (landType==LANDTYPE::CITY); };
    bool isCapital() const { return (landType==LANDTYPE::CAPITAL); };
}; 

/**
 * @Michael-wzl
 * @brief Class representing the game map.
 */
class Map {
public:
    /**
     * @Michael-wzl
     * @brief Constructor for Map class.
     * @param level Initial difficulty level for map generation.
     */
    Map(int level);

    /**
     * @Michael-wzl
     * @brief Regenerate the map at the specified difficulty level.
     * @param level New difficulty level for map generation.
     */
    void regenerateMap(int level);

    /**
     * @Michael-wzl
     * @brief Control whether map generation is reproducible with a fixed seed. If enabled, all subsequent generations will use the provided seed. Call this before regenerateMap to take effect.
     * @param enable True to enable deterministic generation, false to disable.
     * @param fixed Fixed seed to use when deterministic generation is enabled.
     */
    void setDeterministic(bool enable, int fixed = 0) {
        deterministic = enable;
        fixedSeed = fixed;
    }

    /**
     * @Michael-wzl
     * @brief Get current random seed used for the last generation.
     */
    int getSeed() const noexcept { return seed; }

    /**
     * @Michael-wzl
     * Get the tile at the specified coordinates.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Reference to the tile at (x, y).
     */
    Tile getTile(int x, int y) const;

    /**
     * @Michael-wzl
     * @brief Get a snapshot of the current map state.
     * @return 2D vector representing the map tiles.
     */
    std::vector<std::vector<Tile>> getSnapshot() const;

    /**
     * @Michael-wzl
     * @brief Set the tile at the specified coordinates.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param newTile New tile to set at (x, y).
     */
    void setTile(int x, int y, const Tile newTile);

    /**
     * @Michael-wzl
     * @brief Get the width of the map.
     * @return Width of the map.
     */
    int getWidth() const noexcept { return width; }

    /**
     * @Michael-wzl
     * @brief Get the height of the map.
     * @return Height of the map.
     */
    int getHeight() const noexcept { return height; }

private:
    int width;
    int height;
    int seed;
    std::vector<std::vector<Tile>> tiles;
    mutable std::shared_mutex mapMutex;

    // Reproducibility control
    bool deterministic{false};
    int fixedSeed{0};

    /// Generate the map using the specified seed and DFS algorithm.
    /// @param seed Random seed for map generation.
    void generateMap(int seed);
};
