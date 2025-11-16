#include "GameAI.h"
#include "Logger.h"
#include <utility>
#include <limits>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <queue>


void GameAI::loop() {
    int sleep_time = 2000 - level_ * 150; // seconds
    while (running) {
        if (play_state) {
            Move m; 
            int ret = handleByAI(m, level_);
            if (ret == 0) { // submit move to admin
                admin_.submit_ai_action(m);
                LOG_INFOF("AI submitted move: from (%d,%d) to (%d,%d) with army %d", 
                    m.from.first, m.from.second, m.to.first, m.to.second, m.army);
                // allow subclass to update its own state
                onMoveSubmitted(m);
            } else {
                LOG_INFOF("AI decided not to move this turn.");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
}

// snap can be geted by admin_.get_map().getSnapshot()
int GameAI::handleByAI(Move& m, int game_level)
{
    // keep signature for external compatibility; strategy decided by subclass
    (void)game_level;
    using namespace std::chrono_literals;
    std::vector<std::vector<Tile>> snap = admin_.get_map().getSnapshot();
    if (snap.empty() || snap[0].empty()) return -1;

    Move move = pickMove(snap);
    if (move.from != move.to) {
        m.from = move.from;
        m.to = move.to;
        m.army = move.army;
        return 0;
    }
    return -1;
}
// ===== Strategy implementations =====

// RandomAI
Move RandomAI::pickMove(const std::vector<std::vector<Tile>>& snap)
{
    if (snap.empty() || snap[0].empty()) return Move{};
    int W = static_cast<int>(snap[0].size());
    int H = static_cast<int>(snap.size());

    std::mt19937 rng(std::random_device{}());

    auto expandable = [&](int x,int y){
        static const int dx[4]={1,-1,0,0};
        static const int dy[4]={0,0,1,-1};
        for(int k=0;k<4;++k){
            int nx=x+dx[k], ny=y+dy[k];
            if(!in_bounds(nx,ny,W,H)) continue;
            const Tile& t = snap[ny][nx];
            if(!t.isMountain() && t.owner!=playerId && t.army < snap[y][x].army) return true;
        }
        return false;
    };

    // Find frontline tiles
    std::vector<std::pair<int,int>> candidates;
    std::vector<std::pair<int,int>> moveable;
    for (int y=0;y<H;++y){
        for (int x=0;x<W;++x){
            const Tile& t = snap[y][x];
            if (t.owner!=playerId || t.army<=1) continue;
            // Find expandable neighbors
            if (expandable(x,y)){
                candidates.emplace_back(x,y);
            }
            if (t.army > 1) {
                moveable.emplace_back(x,y);
            }
        }
    }

    if (candidates.empty()) {
        candidates = moveable; // fallback to any movable tile
    }

    // Randomly pick one candidate
    std::shuffle(candidates.begin(), candidates.end(), rng);
    for (auto [x,y] : candidates){
        static const int dx[4]={1,-1,0,0};
        static const int dy[4]={0,0,1,-1};
        std::vector<int> dirOrder = {0,1,2,3};
        std::shuffle(dirOrder.begin(), dirOrder.end(), rng);
        for (int idx : dirOrder){
            int nx=x+dx[idx], ny=y+dy[idx];
            if(!in_bounds(nx,ny,W,H)) continue;
            const Tile& t = snap[ny][nx];
            if (t.isMountain()) continue;
            if (t.owner!=playerId && t.army < snap[y][x].army){
                Move m;
                m.from = {x,y};
                m.to = {nx,ny};
                m.army = snap[y][x].army - 1; // send all but one
                return m;
            }
        }
    }

    return Move{}; // No valid move found
}

// GreedyFrontierAI
Move GreedyFrontierAI::pickMove(const std::vector<std::vector<Tile>>& snap)
{
    if (snap.empty() || snap[0].empty()) return Move{};
    int W = static_cast<int>(snap[0].size());
    int H = static_cast<int>(snap.size());
    const std::array<std::pair<int,int>,4> dirs{{{1,0},{-1,0},{0,1},{0,-1}}};

    auto enemyCapital = std::make_pair(-1, -1);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Tile& t = snap[y][x];
            if (t.isCapital() && t.isOriginalCapital && t.owner != playerId) {
                enemyCapital = {x, y};
                y = H; // break outer
                break;
            }
        }
    }

    auto scoreTarget = [&](const Tile& target, bool isNeutral) {
        int value = 0;
        if (target.isCapital()) value += 900;
        else if (target.isCity()) value += 300;
        else value += 80;

        if (isNeutral) value += 40;
        else value += 100; // enemy territory
        return value;
    };

    auto frontlineTile = [&](int x, int y) {
        for (const auto& d : dirs) {
            int nx = x + d.first;
            int ny = y + d.second;
            if (!in_bounds(nx, ny, W, H)) continue;
            const Tile& t = snap[ny][nx];
            if (!t.isMountain() && t.owner != playerId) return true;
        }
        return false;
    };

    Move bestMove{};
    int bestScore = std::numeric_limits<int>::min();

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Tile& src = snap[y][x];
            if (src.owner != playerId || src.army <= 1) continue;
            if (!frontlineTile(x, y)) continue;

            int reserve = 1;
            if (src.isCapital()) reserve = std::max(reserve, minCapitalGarrison());
            else if (src.isCity()) reserve = std::max(reserve, 2);

            int movable = src.army - reserve;
            if (movable <= 0) continue;

            for (const auto& d : dirs) {
                int nx = x + d.first;
                int ny = y + d.second;
                if (!in_bounds(nx, ny, W, H)) continue;
                const Tile& dst = snap[ny][nx];
                if (dst.isMountain()) continue;
                if (dst.owner == playerId) continue; // don't shuffle internally for this strat

                bool isNeutral = (dst.owner == -1);
                if (!isNeutral && movable <= dst.army) continue; // cannot win enemy tile

                int sendArmy = std::max(1, movable);
                if (!isNeutral) {
                    // ensure we send enough to capture with battle tax
                    int need = dst.army + 1;
                    if (sendArmy < need) sendArmy = need;
                }

                Move candidate;
                candidate.from = {x, y};
                candidate.to = {nx, ny};
                candidate.army = sendArmy;

                int score = scoreTarget(dst, isNeutral);

                if (isNeutral) {
                    score += std::min(sendArmy, 5) * 5; // faster land grab
                } else {
                    int surplus = sendArmy - (dst.army + 1);
                    score += surplus * 12; // prefer overwhelming attacks
                }

                // prefer pushing towards enemy capital if known
                if (enemyCapital.first != -1) {
                    int dist = std::abs(nx - enemyCapital.first) + std::abs(ny - enemyCapital.second);
                    score += std::max(0, 80 - dist * 5);
                }

                // penalize exposing freshly captured tile to many enemies when weak
                int enemyAdj = 0;
                for (const auto& d2 : dirs) {
                    int ax = nx + d2.first;
                    int ay = ny + d2.second;
                    if (!in_bounds(ax, ay, W, H)) continue;
                    const Tile& around = snap[ay][ax];
                    if (around.owner >= 0 && around.owner != playerId) enemyAdj++;
                }
                score -= enemyAdj * 10;

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = candidate;
                }
            }
        }
    }

    if (bestScore != std::numeric_limits<int>::min()) return bestMove;

    // --- Fallback: logistics flow from backline to frontline ---
    // Build multi-source BFS distance field from frontline over owned tiles
    const int INF = 1e9;
    std::vector<std::vector<int>> dist(H, std::vector<int>(W, INF));
    std::queue<std::pair<int,int>> q;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Tile& t = snap[y][x];
            if (t.owner == playerId && frontlineTile(x,y)) {
                dist[y][x] = 0;
                q.emplace(x,y);
            }
        }
    }
    while (!q.empty()) {
        auto [x,y] = q.front(); q.pop();
        for (const auto& d : dirs) {
            int nx = x + d.first, ny = y + d.second;
            if (!in_bounds(nx,ny,W,H)) continue;
            const Tile& nt = snap[ny][nx];
            if (nt.isMountain()) continue;
            if (nt.owner != playerId) continue; // only route within owned land
            if (dist[ny][nx] > dist[y][x] + 1) {
                dist[ny][nx] = dist[y][x] + 1;
                q.emplace(nx,ny);
            }
        }
    }

    // pick a far back tile with movable army and step towards smaller dist
    Move bestFlow{};
    int bestFlowScore = std::numeric_limits<int>::min();
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Tile& src = snap[y][x];
            if (src.owner != playerId || src.army <= 1) continue;
            if (dist[y][x] == INF || dist[y][x] == 0) continue; // not routable or already frontline

            int reserve = 1;
            if (src.isCapital()) reserve = std::max(reserve, minCapitalGarrison());
            else if (src.isCity()) reserve = std::max(reserve, 2);
            int movable = src.army - reserve;
            if (movable <= 0) continue;

            // choose neighbor that reduces distance
            int targetNx = -1, targetNy = -1;
            for (const auto& d : dirs) {
                int nx = x + d.first, ny = y + d.second;
                if (!in_bounds(nx,ny,W,H)) continue;
                if (dist[ny][nx] + 1 == dist[y][x]) {
                    const Tile& mid = snap[ny][nx];
                    if (mid.isMountain() || mid.owner != playerId) continue;
                    targetNx = nx; targetNy = ny; break; // any downhill is fine
                }
            }
            if (targetNx == -1) continue;

            int score = dist[y][x] * 10 + std::min(movable, 10); // farther and more movable preferred
            if (score > bestFlowScore) {
                bestFlowScore = score;
                bestFlow.from = {x,y};
                bestFlow.to = {targetNx,targetNy};
                bestFlow.army = movable; // push all spare to accelerate flow
            }
        }
    }

    if (bestFlowScore != std::numeric_limits<int>::min()) return bestFlow;
    return Move{};
}