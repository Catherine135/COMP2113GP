#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <random>
#include "Map.h"
#include "Action.h"
#include "RenderTask.h"
#include "ThreadSafeQueue.h"

// The Admin owns write access to Map. It pulls Actions from queues,
// validates them, applies to Map under lock, and emits RenderTasks.
class Admin {
public:
    Admin(Map& map,
          ThreadSafeQueue<Move>& humanQ,
          ThreadSafeQueue<Move>& aiQ,
          ThreadSafeQueue<RenderTask*>& renderQ)
        : map(map), humanQ(humanQ), aiQ(aiQ), renderQ(renderQ) {}

    void start() {
        running = true;
        worker = std::thread([this]{ this->loop(); });
    }

    void stop() {
        running = false;
        // poke queues to unblock
        Move dummy{};
        humanQ.push(dummy);
        aiQ.push(dummy);
        if (worker.joinable()) worker.join();
    }

private:
    Map& map;
    ThreadSafeQueue<Move>& humanQ;
    ThreadSafeQueue<Move>& aiQ;
    ThreadSafeQueue<RenderTask*>& renderQ;
    std::atomic<bool> running{false};
    std::thread worker;
    int roundCount{0};
    bool gameOver{false};

    bool in_bounds(int x, int y) const {
        return x >= 0 && x < map.getWidth() && y >= 0 && y < map.getHeight();
    }

    bool is_adjacent(const std::pair<int,int>& a, const std::pair<int,int>& b) const {
        int dx = std::abs(a.first - b.first);
        int dy = std::abs(a.second - b.second);
        return (dx + dy == 1);
    }

    void apply_move(const Move& m, int player) {
        if (!in_bounds(m.from.first, m.from.second) || !in_bounds(m.to.first, m.to.second)) return;
        if (!is_adjacent(m.from, m.to)) return;

        Tile src = map.getTile(m.from.first, m.from.second);
        Tile dst = map.getTile(m.to.first, m.to.second);
        if (src.owner != player) return;
        int movable = std::min(m.army, src.army <= 1 ? 0 : src.army - 1);
        if (movable <= 0) return;
        if (dst.isMountain) return;

        // Combat resolution: attacker moves movable into dst
        Tile newSrc = src;
        newSrc.army -= movable;
        map.setTile(m.from.first, m.from.second, newSrc);

        if (dst.owner == -1 || dst.owner == player) {
            // move into neutral or own tile: add armies
            Tile newDst = dst;
            newDst.owner = player;
            newDst.army += movable;
            map.setTile(m.to.first, m.to.second, newDst);
        } else {
            // enemy encounter rule: winner loses (loser + 1)
            if (movable > dst.army) {
                // attacker wins and takes tile; cost = defenders + 1
                int survivors = movable - (dst.army + 1);
                if (survivors < 0) survivors = 0;
                Tile newDst = dst;
                bool capturedCapital = newDst.isCapital && newDst.owner != player;
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
                                // armies保持不变（可选：也可清空重置）
                                map.setTile(xx, yy, t2);
                            }
                        }
                    }
                    gameOver = true;
                    auto *msg = new ShowMsg(std::string("Player ") + std::to_string(player) + " captured a capital and wins! Territory transferred.");
                    renderQ.push(static_cast<RenderTask*>(msg));
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
    auto *a1 = new Arrow(m.from, m.to, true);
        renderQ.push(static_cast<RenderTask*>(a1));
    auto *u1 = new UpdateTile(m.from, map.getTile(m.from.first, m.from.second));
    auto *u2 = new UpdateTile(m.to, map.getTile(m.to.first, m.to.second));
        renderQ.push(static_cast<RenderTask*>(u1));
        renderQ.push(static_cast<RenderTask*>(u2));
    }

    void grow_phase() {
        // Growth cadence:
        // - Every round (0.3s): each owned capital +1
        // - Every 25 rounds: all owned tiles grow (+1), and capitals get extra +2
        auto snap = map.getSnapshot();
        // per-round: each owned capital or city +1
        for (int y = 0; y < map.getHeight(); ++y) {
            for (int x = 0; x < map.getWidth(); ++x) {
                Tile t = snap[y][x];
                if (t.owner >= 0 && (t.isCapital || t.isCity) && !t.isMountain) {
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
                    if (t.owner >= 0 && !t.isMountain) {
                        if (t.isCapital) t.army += 2; else t.army += 1;
                        map.setTile(x, y, t);
                    }
                }
            }
        }

        // notify renderer whole refresh
        auto *rt = new RefreshMap(map.getSnapshot());
        renderQ.push(static_cast<RenderTask*>(rt));
    }

    void loop() {
        using namespace std::chrono_literals;
        while (running && !gameOver) {
            // consume one action from each queue if available, prioritize human
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
            std::this_thread::sleep_for(300ms);
        }
        // final flush
        auto *msg = new ShowMsg(std::string("Admin stopped"));
        renderQ.push(static_cast<RenderTask*>(msg));
    }
};
