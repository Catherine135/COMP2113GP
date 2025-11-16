#include "GameAI.h"
#include "Logger.h"
#include <utility>
#include <limits>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <queue>
#include <tuple>


namespace {
constexpr std::array<std::pair<int,int>,4> kDirs{{{1,0},{-1,0},{0,1},{0,-1}}};
}


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

// ExpanderAI: replicate Python ExpanderAgent heuristic
Move ExpanderAI::pickMove(const std::vector<std::vector<Tile>>& snap)
{
    Move m{};
    if (snap.empty() || snap[0].empty()) return m;
    int W = static_cast<int>(snap[0].size());
    int H = static_cast<int>(snap.size());
    static thread_local std::mt19937 rng(std::random_device{}());

    struct Candidate { std::pair<int,int> from; std::pair<int,int> to; bool opponent; bool neutral; int srcArmy; int dstArmy; bool srcCapital; bool srcCity; };
    std::vector<Candidate> opponentCaps;
    std::vector<Candidate> neutralCaps;
    std::vector<Candidate> others;

    const int dx[4] = {1,-1,0,0};
    const int dy[4] = {0,0,1,-1};

    for (int y=0; y<H; ++y){
        for (int x=0; x<W; ++x){
            const Tile& src = snap[y][x];
            if (src.owner != playerId) continue;
            if (src.army <= 1) continue; // nothing movable
            int reserve = 1;
            if (src.isCapital()) reserve = std::max(reserve, minCapitalGarrison());
            else if (src.isCity()) reserve = std::max(reserve, 1);
            int movable = src.army - reserve;
            if (movable <= 0) continue;
            for (int k=0;k<4;++k){
                int nx = x + dx[k];
                int ny = y + dy[k];
                if (!in_bounds(nx,ny,W,H)) continue;
                const Tile& dst = snap[ny][nx];
                if (dst.isMountain()) continue;
                if (dst.owner == playerId) continue; // skip own tile (we expand only)
                bool isNeutral = (dst.owner == -1);
                bool isOpponent = (!isNeutral && dst.owner != playerId);
                bool enoughToCapture = movable > dst.army; // need > dst.army to leave reserve already considered
                if (!enoughToCapture) continue;
                Candidate c{{x,y},{nx,ny},isOpponent,isNeutral,src.army,dst.army,src.isCapital(),src.isCity()};
                if (isOpponent) opponentCaps.push_back(c);
                else if (isNeutral) neutralCaps.push_back(c);
                others.push_back(c);
            }
        }
    }

    auto pick = [&](std::vector<Candidate>& vec)->Move{
        if (vec.empty()) return Move{};
        std::uniform_int_distribution<size_t> dist(0, vec.size()-1);
        Candidate c = vec[dist(rng)];
        Move mv{};
        mv.from = c.from;
        mv.to = c.to;
        // Decide army to send: for enemy send just enough (dstArmy+1), for neutral send all movable
        const Tile& src = snap[c.from.second][c.from.first];
        int reserve = 1;
        if (src.isCapital()) reserve = std::max(reserve, minCapitalGarrison());
        else if (src.isCity()) reserve = std::max(reserve, 1);
        int movable = std::max(0, src.army - reserve);
        if (c.opponent) mv.army = std::min(movable, c.dstArmy + 1);
        else mv.army = movable;
        if (mv.army <= 0) mv.army = 1; // safety
        return mv;
    };

    Move picked = pick(opponentCaps);
    if (picked.from != picked.to) return picked;
    picked = pick(neutralCaps);
    if (picked.from != picked.to) return picked;
    picked = pick(others);
    return picked;
}

int GreedyFrontierAI::drawAssaultSpacing() {
    std::uniform_int_distribution<int> dist(100, 150);
    return dist(assaultRng);
}

void GreedyFrontierAI::scheduleNextAssault() {
    nextAssaultTurn = turnCounter + drawAssaultSpacing();
}

Move GreedyFrontierAI::planAssault(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> enemyCapital)
{
    Move assault{};
    if (enemyCapital.first == -1) return assault;
    int W = static_cast<int>(snap[0].size());
    int H = static_cast<int>(snap.size());

    int bestMovable = 10;
    std::pair<int,int> bestSrc{-1,-1};

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Tile& tile = snap[y][x];
            if (tile.owner != playerId) continue;
            int movable = movableArmy(tile);
            if (movable >= bestMovable) {
                bestMovable = movable;
                bestSrc = {x,y};
            }
        }
    }

    if (bestSrc.first == -1) return assault;

    int srcDist = std::abs(bestSrc.first - enemyCapital.first) + std::abs(bestSrc.second - enemyCapital.second);
    int bestScore = std::numeric_limits<int>::min();
    std::pair<int,int> bestDst{-1,-1};

    for (const auto& d : kDirs) {
        int nx = bestSrc.first + d.first;
        int ny = bestSrc.second + d.second;
        if (!in_bounds(nx, ny, W, H)) continue;
        const Tile& dst = snap[ny][nx];
        if (dst.isMountain()) continue;

        bool isFriendly = (dst.owner == playerId);
        bool isNeutral = (dst.owner == -1);
        bool isEnemy = (!isFriendly && !isNeutral);

        if (isEnemy && bestMovable <= dst.army) continue; // cannot punch through yet

        int distNext = std::abs(nx - enemyCapital.first) + std::abs(ny - enemyCapital.second);
        bool reduces = distNext < srcDist;

        int score = (srcDist - distNext) * 80;
        if (isEnemy) score += 400;
        else if (isNeutral) score += 120;
        else score += 40;

        if (!reduces) score -= 60; // discourage sidesteps unless necessary
        if (dst.isCapital() && dst.owner != playerId) score += 2000;

        if (score > bestScore) {
            bestScore = score;
            bestDst = {nx, ny};
        }
    }

    if (bestDst.first == -1) return assault;

    assault.from = bestSrc;
    assault.to = bestDst;
    assault.army = bestMovable;
    return assault;
}

void GreedyFrontierAI::onMoveSubmitted(const Move& m) {
    // Mark destination as 'moved this turn' so we can detect stagnation later
    lastMovedTurnByPos[m.to] = turnCounter;
}

int GreedyFrontierAI::bigArmyThreshold() const {
    // Base 10, +1 per 60 turns, capped at 35 (slow growth)
    int base = 10;
    int inc = turnCounter / 60;
    int thr = base + inc;
    if (thr > 35) thr = 35;
    return thr;
}

int GreedyFrontierAI::reserveFor(const Tile& tile) const {
    int reserve = 1;
    if (tile.isCapital()) reserve = std::max(reserve, minCapitalGarrison());
    else if (tile.isCity()) reserve = std::max(reserve, 2);
    return reserve;
}

int GreedyFrontierAI::movableArmy(const Tile& tile) const {
    return tile.army - reserveFor(tile);
}

std::pair<int,int> GreedyFrontierAI::locateEnemyCapital(const std::vector<std::vector<Tile>>& snap) const {
    if (snap.empty() || snap[0].empty()) return {-1,-1};
    int H = static_cast<int>(snap.size());
    int W = static_cast<int>(snap[0].size());
    for (int y=0; y<H; ++y){
        for (int x=0; x<W; ++x){
            const Tile& t = snap[y][x];
            if (t.isCapital() && t.isOriginalCapital && t.owner != playerId) {
                return {x,y};
            }
        }
    }
    return {-1,-1};
}

std::pair<int,int> GreedyFrontierAI::pickRallyTarget(const std::vector<std::vector<Tile>>& snap) const {
    if (snap.empty() || snap[0].empty()) return {-1,-1};
    int W = (int)snap[0].size();
    int H = (int)snap.size();
    // Prefer enemy original capital, then enemy cities, then any enemy tile
    for (int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            const Tile& t = snap[y][x];
            if (t.isCapital() && t.isOriginalCapital && t.owner!=playerId) return {x,y};
        }
    }
    for (int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            const Tile& t = snap[y][x];
            if (t.isCity() && t.owner!=playerId && !t.isCapital()) return {x,y};
        }
    }
    for (int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            const Tile& t = snap[y][x];
            if (t.owner>=0 && t.owner!=playerId) return {x,y};
        }
    }
    return {-1,-1};
}

Move GreedyFrontierAI::planStackingMove(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> rally)
{
    Move m{};
    if (snap.empty() || snap[0].empty()) return m;
    int W = (int)snap[0].size();
    int H = (int)snap.size();

    struct Node { int x,y,movable,distToRally; bool isCapital,isCity; };
    std::vector<Node> bigs;
    int thr = bigArmyThreshold();

    // Collect big armies
    for (int y=0;y<H;++y){
        for(int x=0;x<W;++x){
            const Tile& t = snap[y][x];
            if (t.owner!=playerId) continue;
            int movable = movableArmy(t);
            if (movable < thr) continue;
            int dist = (rally.first==-1) ? 0 : (std::abs(x - rally.first) + std::abs(y - rally.second));
            bigs.push_back({x,y,movable,dist,t.isCapital(),t.isCity()});
        }
    }
    if (bigs.empty()) return m;

    // Choose leader: closest to rally (if known), tie-break by movable
    auto leaderIt = std::min_element(bigs.begin(), bigs.end(), [](const Node&a, const Node&b){
        if (a.distToRally != b.distToRally) return a.distToRally < b.distToRally;
        return a.movable > b.movable;
    });
    Node leader = *leaderIt;

    // Try move a follower toward leader (prefer farthest from leader)
    int bestFollowerScore = std::numeric_limits<int>::min();
    std::pair<int,int> followerFrom{-1,-1}, followerTo{-1,-1};
    for (const auto& n : bigs){
        if (n.x==leader.x && n.y==leader.y) continue;
        int dist = std::abs(n.x - leader.x) + std::abs(n.y - leader.y);
        // pick neighbor that reduces distance and stays in friendly tile if possible
        for (const auto& d : kDirs){
            int nx = n.x + d.first, ny = n.y + d.second;
            if (!in_bounds(nx,ny,W,H)) continue;
            const Tile& t = snap[ny][nx];
            if (t.isMountain()) continue;
            int nd = std::abs(nx - leader.x) + std::abs(ny - leader.y);
            if (nd >= dist) continue; // must get closer
            // Prefer moving into our land to stack，避免消耗
            int score = 0;
            if (t.owner == playerId) score += 200;
            else if (t.owner == -1) score += 40; // neutral is acceptable但次优
            else score -= 200; // 不在堆叠阶段打架
            score += dist * 3; // 更远的跟随者优先
            score += std::min(n.movable, 30); // 更大的部队更优
            if (score > bestFollowerScore){
                bestFollowerScore = score;
                followerFrom = {n.x,n.y};
                followerTo = {nx,ny};
            }
        }
    }

    // Evaluate leader pushing toward rally (can step into own/neutral or winning enemy)
    int bestLeaderScore = std::numeric_limits<int>::min();
    std::pair<int,int> leaderTo{-1,-1};
    int leaderMovable = 0;
    if (rally.first != -1){
        int dist = std::abs(leader.x - rally.first) + std::abs(leader.y - rally.second);
        const Tile& src = snap[leader.y][leader.x];
        leaderMovable = movableArmy(src);
        for (const auto& d : kDirs){
            int nx = leader.x + d.first, ny = leader.y + d.second;
            if (!in_bounds(nx,ny,W,H)) continue;
            const Tile& t = snap[ny][nx];
            if (t.isMountain()) continue;
            int nd = std::abs(nx - rally.first) + std::abs(ny - rally.second);
            if (nd >= dist) continue; // must get closer to rally
            bool isOwn = (t.owner == playerId);
            bool isNeutral = (t.owner == -1);
            bool isEnemy = (!isOwn && !isNeutral);
            if (isEnemy && leaderMovable <= t.army) continue; // do not suicide
            int score = 500; // strong urge to move the ball forward
            if (isOwn) score += 180;
            else if (isNeutral) score += 120;
            else score += 400; // capturing step
            score += std::min(leaderMovable, 40); // bigger stack even more incentive
            // prefer heading straight line by smaller Manhattan next step implicit
            if (score > bestLeaderScore){
                bestLeaderScore = score;
                leaderTo = {nx,ny};
            }
        }
    }

    // Decision: if leader is very large or leader step is better, push the leader
    bool shouldPushLeader = (bestLeaderScore != std::numeric_limits<int>::min()) &&
        (leaderMovable >= thr + 5 || bestLeaderScore >= bestFollowerScore + 50 || followerFrom.first == -1);

    if (shouldPushLeader && leaderTo.first != -1){
        m.from = {leader.x, leader.y};
        m.to = leaderTo;
        const Tile& src = snap[leader.y][leader.x];
        m.army = std::max(1, movableArmy(src));
        return m;
    }

    // Otherwise execute best follower merge
    if (followerFrom.first != -1){
        m.from = followerFrom;
        m.to = followerTo;
        const Tile& src = snap[followerFrom.second][followerFrom.first];
        m.army = std::max(1, movableArmy(src));
        return m;
    }


    return m;
}

Move GreedyFrontierAI::planNudgeFrom(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> srcPos, std::pair<int,int> rally)
{
    Move m{};
    if (snap.empty() || snap[0].empty()) return m;
    int W = (int)snap[0].size();
    int H = (int)snap.size();
    int x = srcPos.first, y = srcPos.second;
    if (!in_bounds(x,y,W,H)) return m;
    const Tile& src = snap[y][x];
    if (src.owner != playerId) return m;

    // Ensure we have a rally; if not, find one
    if (rally.first == -1) rally = pickRallyTarget(snap);
    int movable = movableArmy(src);
    if (movable <= 0) return m;

    int bestScore = std::numeric_limits<int>::min();
    std::pair<int,int> bestTo{-1,-1};
    for (const auto& d : kDirs){
        int nx = x + d.first, ny = y + d.second;
        if (!in_bounds(nx,ny,W,H)) continue;
        const Tile& t = snap[ny][nx];
        if (t.isMountain()) continue;
        int score = 0;
        // prefer moving closer to rally when known
        if (rally.first != -1){
            int dist = std::abs(x - rally.first) + std::abs(y - rally.second);
            int nd = std::abs(nx - rally.first) + std::abs(ny - rally.second);
            if (nd >= dist) score -= 60; else score += 120; // must reduce distance ideally
        }

        bool isOwn = (t.owner == playerId);
        bool isNeutral = (t.owner == -1);
        bool isEnemy = (!isOwn && !isNeutral); (void)isEnemy; // used only for readability in comments
        if (isOwn) score += 150; // safe stacking
        else if (isNeutral) score += 80; // acceptable stepping
        else {
            if (movable > t.army) score += 400; // can capture
            else score -= 500; // avoid suicide
        }

        // Slight preference to align with straight pathing
        if (score > bestScore){
            bestScore = score;
            bestTo = {nx,ny};
        }
    }
    if (bestTo.first == -1) return m;

    m.from = srcPos;
    m.to = bestTo;
    if (snap[bestTo.second][bestTo.first].owner == playerId || snap[bestTo.second][bestTo.first].owner == -1){
        m.army = movable;
    } else {
        // enemy: send enough to win
        int need = snap[bestTo.second][bestTo.first].army + 1;
        m.army = std::max(need, std::min(movable, src.army - 1));
    }
    return m;
}
// GreedyFrontierAI
Move GreedyFrontierAI::pickMove(const std::vector<std::vector<Tile>>& snap)
{
    if (snap.empty() || snap[0].empty()) return Move{};
    int W = static_cast<int>(snap[0].size());
    int H = static_cast<int>(snap.size());
    auto enemyCapital = locateEnemyCapital(snap);

    ++turnCounter;
    // Priority 0: If top-2 strongest stacks haven't moved for 5 turns, nudge them forward first
    {
        // find top-2 owned tiles by army size
        std::tuple<int,int,int> top1{-1,-1,-1}; // army,x,y
        std::tuple<int,int,int> top2{-1,-1,-1};
        for (int y=0;y<H;++y){
            for (int x=0;x<W;++x){
                const Tile& t = snap[y][x];
                if (t.owner != playerId) continue;
                int army = t.army;
                if (army > std::get<0>(top1)) { top2 = top1; top1 = {army,x,y}; }
                else if (army > std::get<0>(top2)) { top2 = {army,x,y}; }
            }
        }
        auto tryNudge = [&](const std::tuple<int,int,int>& cand){
            int army = std::get<0>(cand);
            if (army <= 0) return Move{};
            int x = std::get<1>(cand), y = std::get<2>(cand);
            auto it = lastMovedTurnByPos.find({x,y});
            int last = (it==lastMovedTurnByPos.end()) ? turnCounter : it->second; // default: treated as just moved
            if (turnCounter - last >= 5){
                auto rally = (enemyCapital.first!=-1) ? enemyCapital : pickRallyTarget(snap);
                Move n = planNudgeFrom(snap, {x,y}, rally);
                if (n.from != n.to) return n;
            }
            return Move{};
        };
        Move n1 = tryNudge(top1);
        if (n1.from != n1.to) return n1;
        Move n2 = tryNudge(top2);
        if (n2.from != n2.to) return n2;
    }
    // High-level behavior for advanced AI
    if (level_ > 3) {
        // 1) Periodic assault if capital known
        if (enemyCapital.first != -1) {
            if (nextAssaultTurn == 0) scheduleNextAssault();
            if (turnCounter >= nextAssaultTurn) {
                Move assault = planAssault(snap, enemyCapital);
                if (assault.from != assault.to) {
                    scheduleNextAssault();
                    return assault;
                }
            }
        }

        // 2) Every turn: stack big armies and converge toward rally
        auto rally = (enemyCapital.first != -1) ? enemyCapital : pickRallyTarget(snap);
        Move stack = planStackingMove(snap, rally);
        if (stack.from != stack.to) {
            return stack;
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
        for (const auto& d : kDirs) {
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
            int movable = movableArmy(src);
            if (movable <= 0) continue;

            for (const auto& d : kDirs) {
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
                for (const auto& d2 : kDirs) {
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
        for (const auto& d : kDirs) {
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

            int movable = movableArmy(src);
            if (movable <= 0) continue;

            // choose neighbor that reduces distance
            int targetNx = -1, targetNy = -1;
            for (const auto& d : kDirs) {
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