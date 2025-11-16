#include <algorithm>
#include <queue>
#include <random>
#include <mutex>
#include <chrono>

#include "Map.h"
#include "Logger.h"

#define MIN_MAP_WIDTH 10
#define MIN_MAP_HEIGHT 5
#define MAP_LEVEL_GAP 3

Map::Map(int level) {
    // gen map params based on level
    width = MIN_MAP_WIDTH + level * MAP_LEVEL_GAP;
    height = MIN_MAP_HEIGHT + level * MAP_LEVEL_GAP;
    seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count()) % 10000;

    tiles.assign(height, std::vector<Tile>(width));
    generateMap(seed);
}

void Map::regenerateMap(int level) {
    if (level < 1) {
        LOG_ERRORF("Invalid level %d for map regeneration, setting to 1", level);
        level = 1;
    }
    // gen map params based on level
    width = MIN_MAP_WIDTH + (level - 1) * MAP_LEVEL_GAP;
    height = MIN_MAP_HEIGHT + (level - 1) * MAP_LEVEL_GAP;
    seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count()) % 10000;

    std::unique_lock lock(mapMutex);
    tiles.assign(height, std::vector<Tile>(width));
    generateMap(seed);
}

Tile Map::getTile(int x, int y) const {
    std::shared_lock lock(mapMutex);
    if (y < 0 || y >= height || x < 0 || x >= width) return Tile{};
    return tiles[y][x]; // pass-by-value snapshot
}

std::vector<std::vector<Tile>> Map::getSnapshot() const {
    std::shared_lock lock(mapMutex);
    return tiles; // copy entire grid by value
}

void Map::setTile(int x, int y, const Tile newTile) {
    std::unique_lock lock(mapMutex);
    if (y < 0 || y >= height || x < 0 || x >= width) return;
    tiles[y][x] = newTile; // store by value
}

void Map::generateMap(int s) {
    // Very simple generator: random mountains, a few cities, one capital for player 0 and 1
    std::mt19937 rng(s);
    std::uniform_int_distribution<int> percent(0, 99);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Tile t;
            int r = percent(rng);
            if(r<10){t.landType = LANDTYPE::MOUNTAIN;} //t.isMountain = (r < 10);         // 10% mountains
            else if(r >= 10 && r < 15) {t.landType = LANDTYPE::CITY;} // 5% cities
            else {t.landType = LANDTYPE::NORMAL;}
            tiles[y][x] = t;
        }
    }

    auto place_capital = [&](int owner, int px, int py) {
        // ensure it's not mountain
        /*for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = std::clamp(px + dx, 0, width - 1);
                int ny = std::clamp(py + dy, 0, height - 1);
                tiles[ny][nx].isMountain = false; // clear small area
            }
        }*/
        Tile cap = tiles[py][px];
        cap.owner = owner;
        cap.landType = LANDTYPE::CAPITAL;
        //cap.isCapital = true;
        //cap.isCity = true; // capital behaves like a city for growth
        cap.isOriginalCapital = true;
        cap.army = 1;
        tiles[py][px] = cap;
    };

    place_capital(0, 1, 1);
    place_capital(1, std::max(0, width - 2), std::max(0, height - 2));
}
