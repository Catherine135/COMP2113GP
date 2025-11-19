#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <random>

#include "Admin.h"
#include "Map.h"
#include "Action.h"
#include "RenderTask.h"
#include "ThreadSafeQueue.h"
#include "Logger.h"
#include "GameCtrl.h"


bool Admin::in_bounds(int x, int y) const {
    return x >= 0 && x < map.getWidth() && y >= 0 && y < map.getHeight();
}

bool Admin::is_adjacent(const std::pair<int,int>& a, const std::pair<int,int>& b) const {
    int dx = std::abs(a.first - b.first);
    int dy = std::abs(a.second - b.second);
    return (dx + dy == 1);
}

void Admin::init(int level) {
    play_state = false;
    winGame = false;
    roundCount = 0;
    curWinner = -1;

    // re-generate map
    map.regenerateMap(level);
    LOG_INFOF("Map regenerated at level %d", level);
}

void Admin::apply_move(const Move& m, int player) {
    LOG_INFOF("Applying move for player %d: from (%d,%d) to (%d,%d) with army %d", 
        player, m.from.first, m.from.second, m.to.first, m.to.second, m.army);
    if (!in_bounds(m.from.first, m.from.second) || !in_bounds(m.to.first, m.to.second)) {
        LOG_ERRORF("Move out of bounds: from (%d,%d) to (%d,%d)", 
            m.from.first, m.from.second, m.to.first, m.to.second);
        return;
    }
    if (!is_adjacent(m.from, m.to)) {
        LOG_ERRORF("Move not adjacent: from (%d,%d) to (%d,%d)", 
            m.from.first, m.from.second, m.to.first, m.to.second);
        return;
    }

    if (player == 0) { // anyway to clear arrow for human player
        LOG_INFOF("Only clear arrow for human, Ignoring move from (%d,%d) to (%d,%d)", 
            m.from.first, m.from.second, m.to.first, m.to.second);
        auto *a1 = new Arrow(m.from, m.to, false, player == 0, map.getSnapshot());
        r_.submit_task(static_cast<RenderTask*>(a1));
    }

    Tile src = map.getTile(m.from.first, m.from.second);
    Tile dst = map.getTile(m.to.first, m.to.second);
    if (src.owner != player) {
        LOG_INFOF("Source tile not owned by player %d: from (%d,%d) owner %d", 
            player, m.from.first, m.from.second, src.owner);
        return;
    }
    if (dst.isMountain()) {
        LOG_INFOF("Destination tile is mountain: to (%d,%d)", 
            m.to.first, m.to.second);
        return;
    }
    //int movable = std::min(m.army, src.army <= 1 ? 0 : src.army - 1);
    int movable = (src.army <= 1 ? 0 : src.army - 1);
    if (movable <= 0) {
        LOG_INFOF("No movable armies for move from (%d,%d) to (%d,%d)", 
            m.from.first, m.from.second, m.to.first, m.to.second);
        return;
    }

    // Combat resolution: attacker moves movable into dst
    Tile newSrc = src;
    newSrc.army -= movable;
    map.setTile(m.from.first, m.from.second, newSrc);

    if (dst.owner == player) {
        // Move into own tile: merge armies
        Tile newDst = dst;
        newDst.owner = player;
        newDst.army += movable;
        map.setTile(m.to.first, m.to.second, newDst);
        LOG_INFOF("Merged into own tile at (%d,%d), new army %d", m.to.first, m.to.second, newDst.army);
    } else if (dst.owner == -1) {
        // Neutral tile
        if (dst.isCity()) {
            // Correct rule: battle with neutral city garrison (simple subtraction)
            LOG_INFOF("Attacking neutral city at (%d,%d): atk %d vs def %d", 
                m.to.first, m.to.second, movable, dst.army);
            if (movable > dst.army) {
                int survivors = movable - dst.army; // no +1 penalty for neutral city
                Tile newDst = dst;
                newDst.owner = player;
                newDst.army = survivors;
                map.setTile(m.to.first, m.to.second, newDst);
                LOG_INFOF("Captured neutral city at (%d,%d) with %d survivors", m.to.first, m.to.second, survivors);
            } else {
                // Defender (neutral) holds or tie -> defender wins; remaining defenders decrease
                Tile newDst = dst;
                newDst.army = dst.army - movable; // tie -> 0
                // owner stays -1 (neutral)
                map.setTile(m.to.first, m.to.second, newDst);
                LOG_INFOF("Neutral city holds at (%d,%d), remaining defenders %d", m.to.first, m.to.second, newDst.army);
            }
        } else {
            // Neutral non-city ground: claim and add armies
            Tile newDst = dst;
            newDst.owner = player;
            newDst.army += movable;
            map.setTile(m.to.first, m.to.second, newDst);
            LOG_INFOF("Claimed neutral tile at (%d,%d), army %d", m.to.first, m.to.second, newDst.army);
        }
    } else {
        // enemy encounter rule: winner loses (loser + 1)
        if (movable > dst.army) {
            // attacker wins and takes tile; cost = defenders + 1
            int survivors = movable - (dst.army + 1);
            if (survivors < 0) survivors = 0;
            Tile newDst = dst;
            bool capturedCapital = newDst.isCapital() && newDst.owner != player;
            newDst.owner = player;
            newDst.army = survivors;
            map.setTile(m.to.first, m.to.second, newDst);
            if (capturedCapital && dst.isOriginalCapital) {
                // Transfer all territory from old owner to the winner
                int defeated = dst.owner; // previous owner before overwrite
                auto snapAll = map.getSnapshot();
                for (int yy = 0; yy < map.getHeight(); ++yy) {
                    for (int xx = 0; xx < map.getWidth(); ++xx) {
                        Tile t2 = snapAll[yy][xx];
                        if (t2.owner == defeated) {
                            t2.owner = player;
                            map.setTile(xx, yy, t2);
                        }
                    }
                }
                winGame = true;
                curWinner = player;
                auto *msg = new ShowMsg(std::string("Player ") + std::to_string(player) + " captured a capital and wins! Territory transferred.");
                r_.submit_task(static_cast<RenderTask*>(msg));
                LOG_INFOF("Player %d captured a capital and wins! Territory transferred.", player);
            }
        } else {
            // defender wins/retains tile; defender loses (attacker + 1) when strictly stronger
            Tile newDst = dst;
            if (movable < dst.army) {
                int remain = dst.army - (movable + 1);
                newDst.army = remain < 0 ? 0 : remain;
            } else { // tie -> defender wins with normal subtraction
                newDst.army = dst.army - movable; // typically zero
            }
            map.setTile(m.to.first, m.to.second, newDst);
        }
    }

    // Emit a couple of render tasks by value wrapped pointers
    bool bIsHuman = (player == 0);
    if (bIsHuman) {
        auto *a1 = new Arrow(m.from, m.to, false, bIsHuman, map.getSnapshot());
        r_.submit_task(static_cast<RenderTask*>(a1));

        auto snap = map.getSnapshot();
        auto *u1 = new UpdateTile(m.from, map.getTile(m.from.first, m.from.second), snap);
        auto *u2 = new UpdateTile(m.to, map.getTile(m.to.first, m.to.second), snap);
        r_.submit_task(static_cast<RenderTask*>(u1));
        r_.submit_task(static_cast<RenderTask*>(u2));
    }
}

void Admin::grow_phase() {
    // Growth cadence:
    // - Every round (0.3s): each owned capital +1
    // - Every 25 rounds: all owned tiles grow (+1), and capitals get extra +2
    auto snap = map.getSnapshot();
    // per-round: each owned capital or city +1
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            Tile t = snap[y][x];
            if (t.owner >= 0 && (t.isCapital() || t.isCity())) {
                t.army += 1;
                map.setTile(x, y, t);
            }
        }
    }

    // every 25 rounds global growth
    if (roundCount > 0 && (roundCount % 25 == 0)) {
        auto snap2 = map.getSnapshot();
        for (int y = 0; y < map.getHeight(); ++y) {
            for (int x = 0; x < map.getWidth(); ++x) {
                Tile t = snap2[y][x];
                if (t.owner >= 0 && !t.isMountain()) {
                    //if (t.isCapital()) t.army += 2; else t.army += 1;
                    t.army += 1;
                    map.setTile(x, y, t);
                }
            }
        }
    }

    // count army && land for human and ai
    int human_army = 0;
    int ai_army = 0;
    int human_land = 0;
    int ai_land = 0;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            Tile t = snap[y][x];
            if (t.owner == 0) {
                human_army += t.army;
                human_land += 1;
            } else if (t.owner == 1) {
                ai_army += t.army;
                ai_land += 1;
            }
        }
    }

    // notify renderer refresh turns info
    auto *rt = new UpdateTurnsInterface(roundCount, human_land, human_army, ai_land, ai_army, "", map.getSnapshot());
    r_.submit_task(static_cast<RenderTask*>(rt));
}

void Admin::loop() {
    using namespace std::chrono_literals;
    while (running) {
        // consume one action from each queue if available, prioritize human
        if(play_state) {
            Move act{};
            bool haveHuman = humanQ.try_pop(act);
            if (haveHuman) {
                apply_move(act, 0);
            }

            Move aact{};
            bool haveAI = aiQ.try_pop(aact);
            if (haveAI) {
                apply_move(aact, 1);
            }

            ++roundCount;
            grow_phase();

            if (winGame) {
                LOG_INFOF("Game over! Player %d wins after %d rounds.", curWinner, roundCount);
                auto *msg = new ShowMsg(std::string("Game over! Player ") + std::to_string(curWinner) + " wins after " + std::to_string(roundCount) + " rounds.");
                r_.submit_task(static_cast<RenderTask*>(msg));

                /*if (curWinner == 0) {
                    auto *endTask = new EndGameInterface(true, roundCount);
                    r_.submit_task(static_cast<RenderTask*>(endTask));
                } else {
                    auto *endTask = new EndGameInterface(false, roundCount);
                    r_.submit_task(static_cast<RenderTask*>(endTask));
                }*/

                // restart or next level handled by GameCtrl
                GameCtrl::getInstance()->StartNextLevelGame((curWinner == 0), roundCount);

                play_state = false;
                winGame = false;
                LOG_INFOF("Admin loop wait a new start after game end.");
            }
        }
        std::this_thread::sleep_for(1000ms);
    }

    // final flush
    //auto *msg = new ShowMsg(std::string("Admin stopped"));
    //r_.submit_task(static_cast<RenderTask*>(msg));
    LOG_INFO("Admin loop exited");
}
