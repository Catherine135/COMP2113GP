#pragma once
#include <thread>
#include <utility>
#include <atomic>
#include <random>
#include <algorithm>
#include <random> 
#include <chrono>
#include <map>
#include "Map.h"
#include "Action.h"
#include "Admin.h" 
#include "Human.h"

class GameAI {
public:
    GameAI(Admin& a, int playerId = 1)
        : admin_(a), playerId(playerId) {}
    virtual ~GameAI() = default;

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

protected:
    Admin& admin_;
    int level_{1};
    int playerId{1};
    // helpers for subclasses
    inline bool in_bounds(int x, int y, int w, int h) const { return x>=0 && y>=0 && x<w && y<h; }
    // Keep capital minimally safe; lower threshold to allow earlier expansion
    int minCapitalGarrison() const { return std::max(2, level_); }

    // Strategy hook to be implemented by subclasses
    virtual Move pickMove(const std::vector<std::vector<Tile>>& snap) = 0;
    // Optional callback for subclasses to update internal state when a move is submitted
    virtual void onMoveSubmitted(const Move&) {}

private:
    std::atomic<bool> play_state{false};
    std::atomic<bool> running{false};
    std::thread worker;

    void loop();
    int handleByAI(Move& m, int game_level=1);
};

// Random baseline AI: simple expansion with filtered neighbors and light preferences
class RandomAI : public GameAI {
public:
    using GameAI::GameAI;

protected:
    Move pickMove(const std::vector<std::vector<Tile>>& snap) override;
};

// ExpanderAI: prioritize capturing opponent tiles, then neutral expansion; otherwise random valid move
class ExpanderAI : public GameAI {
public:
    using GameAI::GameAI;

protected:
    Move pickMove(const std::vector<std::vector<Tile>>& snap) override;
};

class GreedyFrontierAI : public GameAI {
public:
    using GameAI::GameAI;
    void onMoveSubmitted(const Move& m) override; // track movement history

protected:
    Move pickMove(const std::vector<std::vector<Tile>>& snap) override;

private:
    Move planAssault(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> enemyCapital);
    void scheduleNextAssault();
    int drawAssaultSpacing();
    int bigArmyThreshold() const; // slowly increases with turns
    int reserveFor(const Tile& tile) const;
    int movableArmy(const Tile& tile) const;
    std::pair<int,int> locateEnemyCapital(const std::vector<std::vector<Tile>>& snap) const;
    std::pair<int,int> pickRallyTarget(const std::vector<std::vector<Tile>>& snap) const;
    Move planStackingMove(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> rally);
    Move planNudgeFrom(const std::vector<std::vector<Tile>>& snap, std::pair<int,int> src, std::pair<int,int> rally);

    int turnCounter{0};
    int nextAssaultTurn{0};
    std::mt19937 assaultRng{std::random_device{}()};
    // Track last turn a tile's occupying force moved (by destination)
    std::map<std::pair<int,int>, int> lastMovedTurnByPos;
};

// Alias for existing usage in main.cpp (can extend with new heuristics later)
using HeuristicAI = GreedyFrontierAI;
