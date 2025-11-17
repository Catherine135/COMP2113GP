#include <algorithm>
#include <array>
#include <deque>
#include <queue>
#include <random>
#include <mutex>
#include <chrono>
#include <stack>
#include <numeric>
#include <limits>

#include "Map.h"
#include "Logger.h"

#define MIN_MAP_WIDTH 10
#define MIN_MAP_HEIGHT 5
#define MAP_LEVEL_GAP 3

Map::Map(int level) {
    // gen map params based on level
    width = MIN_MAP_WIDTH + level * MAP_LEVEL_GAP;
    height = MIN_MAP_HEIGHT + level * MAP_LEVEL_GAP;
    // initial seed (can be overridden by setDeterministic + regenerateMap)
    int s = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    seed = s;

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
    // choose seed by reproducibility setting
    if (deterministic) {
        seed = fixedSeed;
    } else {
        seed = static_cast<int>(
            std::chrono::system_clock::now().time_since_epoch().count());
    }

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
    // Maze-based generator with optional reproducibility
    std::mt19937 rng(s);

    const std::array<std::pair<int,int>, 4> dirs{{{ -1,0 }, { 1,0 }, { 0,-1 }, { 0,1 }}};

    // Common helpers
    auto inb = [&](int x, int y){ return x>=0 && x<width && y>=0 && y<height; };
    auto isMountain = [&](int x, int y){ return tiles[y][x].landType == LANDTYPE::MOUNTAIN; };
    auto isPassable = [&](int x, int y){ return tiles[y][x].landType != LANDTYPE::MOUNTAIN; };
    auto setNormalEmpty = [&](int x, int y){ tiles[y][x].landType = LANDTYPE::NORMAL; tiles[y][x].owner = -1; tiles[y][x].army = 0; };
    auto hasPassNeighbor = [&](int x, int y){
        for (auto [dx, dy] : dirs) {
            int nx = x + dx, ny = y + dy;
            if (inb(nx, ny) && isPassable(nx, ny)) return true;
        }
        return false;
    };
    auto ensure_capital_path = [&](){
        std::vector<std::pair<int,int>> capitals;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (tiles[y][x].landType == LANDTYPE::CAPITAL)
                    capitals.emplace_back(x,y);
        if (capitals.size() != 2) return;

        const int total = width * height;
        auto idx = [&](int x, int y){ return y * width + x; };
        const int INF = std::numeric_limits<int>::max();
        std::vector<int> dist(total, INF);
        std::vector<int> prev(total, -1);
        std::deque<int> dq;
        int startIdx = idx(capitals[0].first, capitals[0].second);
        int targetIdx = idx(capitals[1].first, capitals[1].second);
        dist[startIdx] = 0;
        dq.push_back(startIdx);

        while (!dq.empty()) {
            int cur = dq.front(); dq.pop_front();
            if (cur == targetIdx) break;
            int cx = cur % width;
            int cy = cur / width;
            for (auto [dx, dy] : dirs) {
                int nx = cx + dx, ny = cy + dy;
                if (!inb(nx, ny)) continue;
                int nidx = idx(nx, ny);
                int cost = tiles[ny][nx].landType == LANDTYPE::MOUNTAIN ? 1 : 0;
                if (dist[cur] != INF && dist[cur] + cost < dist[nidx]) {
                    dist[nidx] = dist[cur] + cost;
                    prev[nidx] = cur;
                    if (cost == 0) dq.push_front(nidx);
                    else dq.push_back(nidx);
                }
            }
        }

        if (dist[targetIdx] == INF || dist[targetIdx] == 0) return;

        int cur = targetIdx;
        while (cur != -1) {
            int px = cur % width;
            int py = cur / width;
            if (tiles[py][px].landType == LANDTYPE::MOUNTAIN) {
                setNormalEmpty(px, py);
            }
            cur = prev[cur];
        }
    };

    bool capitalsPlaced = false;

    auto place_capital = [&](int owner, int px, int py) {
        Tile cap = tiles[py][px];
        cap.owner = owner; cap.landType = LANDTYPE::CAPITAL;
        cap.isOriginalCapital = true; cap.army = 10;
        tiles[py][px] = cap;
    };

    auto ensure_capital_has_empty_neighbor = [&](int px, int py) {
        for (auto [dx, dy] : dirs) {
            int nx = px + dx, ny = py + dy;
            if (inb(nx, ny) && tiles[ny][nx].landType == LANDTYPE::NORMAL) return; // already ok
        }
        for (auto [dx, dy] : dirs) { // open mountain first
            int nx = px + dx, ny = py + dy;
            if (inb(nx, ny) && isMountain(nx, ny)) { setNormalEmpty(nx, ny); return; }
        }
        for (auto [dx, dy] : dirs) { // else convert a city
            int nx = px + dx, ny = py + dy;
            if (inb(nx, ny) && tiles[ny][nx].landType == LANDTYPE::CITY) { setNormalEmpty(nx, ny); return; }
        }
    };

    auto try_place_capitals_from = [&](const std::vector<std::pair<int,int>>& pool) -> bool {
        if (pool.size() < 2) return false;
        std::vector<std::pair<int,int>> candidates;
        candidates.reserve(pool.size());
        for (auto [x, y] : pool) {
            if (tiles[y][x].landType == LANDTYPE::NORMAL && hasPassNeighbor(x, y)) {
                candidates.emplace_back(x, y);
            }
        }
        if (candidates.size() < 2) return false;

        int required = std::min(15, (width - 1) + (height - 1));
        std::vector<size_t> order(candidates.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), rng);
        for (size_t i = 0; i < order.size(); ++i) {
            auto [x1, y1] = candidates[order[i]];
            for (size_t j = i + 1; j < order.size(); ++j) {
                auto [x2, y2] = candidates[order[j]];
                if (std::abs(x1 - x2) + std::abs(y1 - y2) >= required) {
                    place_capital(0, x1, y1); ensure_capital_has_empty_neighbor(x1, y1);
                    place_capital(1, x2, y2); ensure_capital_has_empty_neighbor(x2, y2);
                    return true;
                }
            }
        }
        return false;
    };

    std::vector<std::pair<int,int>> carvedCells;
    carvedCells.reserve(width * height);

    // 1) Initialize all tiles as mountains
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            tiles[y][x] = Tile{ -1, 0, LANDTYPE::MOUNTAIN, false };

    // 2) Carve a DFS maze over odd coordinates
    auto in_bounds_cell = [&](int cx, int cy) {
        return cx >= 1 && cx < width - 1 && cy >= 1 && cy < height - 1 &&
               (cx % 2 == 1) && (cy % 2 == 1);
    };

    if (width >= 3 && height >= 3) {
        int maxCellX = (width - 2) / 2;
        int maxCellY = (height - 2) / 2;
        if (maxCellX <= 0 || maxCellY <= 0) {
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    tiles[y][x].landType = LANDTYPE::NORMAL;
        } else {
            std::uniform_int_distribution<int> distCellX(0, maxCellX - 1);
            std::uniform_int_distribution<int> distCellY(0, maxCellY - 1);
            int sx = 1 + 2 * distCellX(rng);
            int sy = 1 + 2 * distCellY(rng);

            std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
            std::stack<std::pair<int, int>> st;
            visited[sy][sx] = true;
            tiles[sy][sx].landType = LANDTYPE::NORMAL;
            carvedCells.emplace_back(sx, sy);
            if (!capitalsPlaced) capitalsPlaced = try_place_capitals_from(carvedCells);
            st.push({sx, sy});

            auto step = [&](int x, int y){
                std::array<std::pair<int, int>, 4> dirs{{{2, 0}, {-2, 0}, {0, 2}, {0, -2}}};
                std::shuffle(dirs.begin(), dirs.end(), rng);
                for (auto [dx, dy] : dirs) {
                    int nx = x + dx, ny = y + dy;
                    if (in_bounds_cell(nx, ny) && !visited[ny][nx]) {
                        int wx = x + dx/2, wy = y + dy/2;
                        tiles[wy][wx].landType = LANDTYPE::NORMAL;
                        carvedCells.emplace_back(wx, wy);
                        tiles[ny][nx].landType = LANDTYPE::NORMAL;
                        carvedCells.emplace_back(nx, ny);
                        visited[ny][nx] = true;
                        st.push({nx, ny});
                        if (!capitalsPlaced) capitalsPlaced = try_place_capitals_from(carvedCells);
                        return true;
                    }
                }
                return false;
            };

            while (!st.empty()) {
                auto [cx, cy] = st.top();
                if (!step(cx, cy)) st.pop();
            }
        }
    } else {
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                tiles[y][x].landType = LANDTYPE::NORMAL;
    }

    // 3) Remove 50% walls; half become NORMAL, half become CITY (30-60),
    //    but only create CITY if it has a passable neighbor.
    std::vector<std::pair<int, int>> walls;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (isMountain(x,y)) walls.emplace_back(x,y);

    if (!walls.empty()) {
        std::shuffle(walls.begin(), walls.end(), rng);
        size_t take = walls.size() / 2;
        std::uniform_int_distribution<int> coin(0, 1);
        std::uniform_int_distribution<int> armyDist(30, 60);
        for (size_t i = 0; i < take; ++i) {
            auto [wx, wy] = walls[i];
            Tile t = tiles[wy][wx];
            if (coin(rng) == 0) {
                setNormalEmpty(wx, wy);
            } else {
                if (hasPassNeighbor(wx, wy)) {
                    t.landType = LANDTYPE::CITY; t.owner = -1; t.army = armyDist(rng);
                    tiles[wy][wx] = t;
                } else {
                    setNormalEmpty(wx, wy);
                }
            }
        }
    }

    // 4) Capitals placement with distance and mobility guarantees
    if (!capitalsPlaced) {
        std::vector<std::pair<int, int>> passable;
        passable.reserve(width * height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (isPassable(x,y) && hasPassNeighbor(x,y)) passable.emplace_back(x,y);

        if (!try_place_capitals_from(passable)) {
            int x1 = std::min(1, std::max(0, width - 2));
            int y1 = std::min(1, std::max(0, height - 2));
            int x2 = std::max(0, width - 2);
            int y2 = std::max(0, height - 2);
            setNormalEmpty(x1, y1);
            if (x1+1 < width) setNormalEmpty(x1+1, y1);
            if (y1+1 < height) setNormalEmpty(x1, y1+1);
            setNormalEmpty(x2, y2);
            if (x2 > 0) setNormalEmpty(x2-1, y2);
            if (y2 > 0) setNormalEmpty(x2, y2-1);
            place_capital(0, x1, y1); ensure_capital_has_empty_neighbor(x1, y1);
            place_capital(1, x2, y2); ensure_capital_has_empty_neighbor(x2, y2);
        }
    }

    ensure_capital_path();

    // 5) Normalize neutral city count to ~5% of map size
    {
        int total = width * height;
        int targetCities = (total * 5) / 100;

        std::vector<std::pair<int,int>> capitals, cities;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (tiles[y][x].landType == LANDTYPE::CAPITAL) capitals.emplace_back(x,y);
                else if (tiles[y][x].landType == LANDTYPE::CITY) cities.emplace_back(x,y);

        std::vector<std::vector<bool>> nearCapital(height, std::vector<bool>(width,false));
        const int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (auto [cx, cy] : capitals) {
            for (auto &d : dirs) {
                int nx = cx + d[0], ny = cy + d[1];
                if (inb(nx, ny)) nearCapital[ny][nx] = true;
            }
        }

        int current = static_cast<int>(cities.size());
        if (current > targetCities) {
            std::shuffle(cities.begin(), cities.end(), rng);
            int toRemove = current - targetCities;
            for (int i = 0; i < toRemove && i < static_cast<int>(cities.size()); ++i)
                setNormalEmpty(cities[i].first, cities[i].second);
        } else if (current < targetCities) {
            std::vector<std::pair<int,int>> normals;
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    if (tiles[y][x].landType == LANDTYPE::NORMAL && !nearCapital[y][x] && hasPassNeighbor(x,y))
                        normals.emplace_back(x,y);
            std::shuffle(normals.begin(), normals.end(), rng);
            int need = targetCities - current; std::uniform_int_distribution<int> armyDist2(30,60);
            for (int i = 0; i < need && i < static_cast<int>(normals.size()); ++i) {
                int x = normals[i].first, y = normals[i].second;
                tiles[y][x].landType = LANDTYPE::CITY;
                tiles[y][x].owner = -1;
                tiles[y][x].army = armyDist2(rng);
            }
        }
    }
}
