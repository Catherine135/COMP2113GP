#include <iostream>
#include <queue>
#include <vector>

#include "core/Map.h"
/*
g++ -std=c++17 -Wall -Wextra -O2 -I./src -I./src/core -I./src/utils src/tests/MapConnectivity.cpp src/core/Map.cpp src/utils/Logger.cpp -o build/map_connectivity && ./build/map_connectivity
*/

namespace {

char tileChar(const Tile &tile) {
    switch (tile.landType) {
        case LANDTYPE::MOUNTAIN: return '#';
        case LANDTYPE::CITY: return 'S';
        case LANDTYPE::CAPITAL: return tile.owner == 0 ? 'H' : 'A';
        case LANDTYPE::NORMAL:
        default: return '.';
    }
}

bool isPassable(const Tile &tile) {
    return tile.landType != LANDTYPE::MOUNTAIN;
}

void dumpMap(const Map &map) {
    auto grid = map.getSnapshot();
    std::cout << "Map " << map.getWidth() << "x" << map.getHeight()
              << " (seed=" << map.getSeed() << ")" << std::endl;
    for (const auto &row : grid) {
        for (const auto &tile : row) {
            std::cout << tileChar(tile);
        }
        std::cout << '\n';
    }
}

bool capitalsConnected(const Map &map) {
    auto grid = map.getSnapshot();
    if (grid.empty() || grid.front().empty()) return true;

    const int height = static_cast<int>(grid.size());
    const int width = static_cast<int>(grid.front().size());

    std::vector<std::pair<int, int>> capitals;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y][x].landType == LANDTYPE::CAPITAL) {
                capitals.emplace_back(x, y);
            }
        }
    }

    if (capitals.size() != 2) {
        std::cerr << "Expected 2 capitals but found " << capitals.size() << "\n";
        return false;
    }

    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::queue<std::pair<int, int>> q;
    q.push(capitals[0]);
    visited[capitals[0].second][capitals[0].first] = true;

    auto enqueue = [&](int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        if (visited[y][x]) return;
        if (!isPassable(grid[y][x])) return;
        visited[y][x] = true;
        q.emplace(x, y);
    };

    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        if (x == capitals[1].first && y == capitals[1].second) {
            return true;
        }
        enqueue(x - 1, y);
        enqueue(x + 1, y);
        enqueue(x, y - 1);
        enqueue(x, y + 1);
    }

    return false;
}

} // namespace

int main() {
    constexpr int iterations = 20000;
    const std::vector<int> levels = {1, 2, 3};

    bool disconnectedFound = false;
    for (int level : levels) {
        Map map(level);
        int failures = 0;
        for (int seed = 0; seed < iterations; ++seed) {
            map.setDeterministic(true, seed);
            map.regenerateMap(level);
            if (!capitalsConnected(map)) {
                ++failures;
                std::cout << "[LEVEL " << level << "] disconnected map detected at seed "
                          << map.getSeed() << " (" << seed << ")" << std::endl;
                if (failures == 1) {
                    dumpMap(map);
                }
                disconnectedFound = true;
                if (failures >= 5) {
                    break;
                }
            }
        }
        if (failures == 0) {
            std::cout << "[LEVEL " << level
                      << "] no disconnected capitals observed in " << iterations
                      << " iterations." << std::endl;
        }
    }

    if (!disconnectedFound) {
        std::cout << "All tested maps had a connecting path between capitals." << std::endl;
    }

    return disconnectedFound ? 1 : 0;
}
