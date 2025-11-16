#pragma once
#include <thread>
#include <atomic>
#include <random>
#include <algorithm>
#include <random> 
#include <chrono>
#include "Map.h"
#include "Action.h"
#include "Admin.h" 
#include "Human.h"

class GameAI {
public:
    GameAI(Admin& a, int playerId = 1)
        : admin_(a), playerId(playerId) {}

    void start() {
        running = true;
        play_state = false;
        worker = std::thread([this]{ this->loop(); });
    }
    void pause() {
        play_state = false;
    }
    void resume() {
        play_state = true;
    }
    void stop() {
        running = false;
        if (worker.joinable()) worker.join();
    }
    void init(int level) {
        level_ = level;
        play_state = false;
    }

private:
    Admin& admin_;
    int level_{1};
    int playerId{1};
    std::atomic<bool> play_state{false};
    std::atomic<bool> running{false};
    std::thread worker;

    void loop();
    int handleByAI(Move& m, int game_level=1);
};