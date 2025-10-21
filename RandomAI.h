#pragma once
#include <thread>
#include <atomic>
#include <random>
#include "Map.h"
#include "Action.h"
#include "ThreadSafeQueue.h"

class RandomAI {
public:
    RandomAI(Map& map, ThreadSafeQueue<Move>& outQ, int playerId = 1)
        : map(map), outQ(outQ), playerId(playerId) {}

    void start() {
        running = true;
        worker = std::thread([this]{ this->loop(); });
    }
    void stop() {
        running = false;
        if (worker.joinable()) worker.join();
    }

private:
    Map& map;
    ThreadSafeQueue<Move>& outQ;
    int playerId{1};
    std::atomic<bool> running{false};
    std::thread worker;

    void loop() {
        std::mt19937 rng(std::random_device{}());
        using namespace std::chrono_literals;
        while (running) {
            auto snap = map.getSnapshot();
            std::vector<std::pair<int,int>> owned;
            for (int y=0; y<map.getHeight(); ++y) {
                for (int x=0; x<map.getWidth(); ++x) {
                    Tile t = snap[y][x];
                    if (t.owner == playerId && t.army > 1) owned.emplace_back(x,y);
                }
            }
            if (!owned.empty()) {
                std::uniform_int_distribution<size_t> pick(0, owned.size()-1);
                auto from = owned[pick(rng)];
                // random neighbor
                std::vector<std::pair<int,int>> nbrs{
                    {from.first+1, from.second}, {from.first-1, from.second},
                    {from.first, from.second+1}, {from.first, from.second-1}
                };
                std::shuffle(nbrs.begin(), nbrs.end(), rng);
                for (auto to : nbrs) {
                    if (to.first>=0 && to.first<map.getWidth() && to.second>=0 && to.second<map.getHeight()) {
                        Move m; m.from = from; m.to = to; m.army = 1 + (snap[from.second][from.first].army/2);
                        outQ.push(m);
                        break;
                    }
                }
            }
            std::this_thread::sleep_for(150ms);
        }
    }
};
