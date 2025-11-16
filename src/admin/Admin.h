#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <random>

#include "Map.h"
#include "Action.h"
#include "RenderTask.h"
#include "Renderer.h"
#include "ThreadSafeQueue.h"

// The Admin owns write access to Map. It pulls Actions from queues,
// validates them, applies to Map under lock, and emits RenderTasks.
class Admin {
public:
    Admin(Renderer& r, int level) 
        : map(level), r_(r) {}
    ~Admin() { stop(); } // RAII：auto stop

    void start() { // start thread
        if (worker.joinable()) return;
        running = true;
        play_state = false;
        worker = std::thread(&Admin::loop, this);
    }
    void init(int level=1);
    void pause() {
        play_state = false;
    }
    void resume() {
        play_state = true;
    }
    void stop() {
        if (!worker.joinable()) return;
        running = false;
        humanQ.stop(); 
        aiQ.stop();
        worker.join();
    }
    void submit_human_action(Move& m) { humanQ.push(m); };
    void submit_ai_action(Move& m) { aiQ.push(m); };
    Map& get_map() { return map;}


private:
    Map map;
    ThreadSafeQueue<Move> humanQ;
    ThreadSafeQueue<Move> aiQ;
    Renderer& r_;
    
    std::atomic<bool> running{false};
    std::thread worker;
    int roundCount{0};
    bool winGame{false};
    int curWinner{-1};
    bool play_state{false};

    // Selecting army count
    //int selected_army_count_;
    //bool has_selected_army_count_; 
    //std::pair<int, int> move_start_; 
    //std::pair<int, int> move_target_;

    bool in_bounds(int x, int y) const;
    bool is_adjacent(const std::pair<int,int>& a, const std::pair<int,int>& b) const;
    void apply_move(const Move& m, int player);
    void grow_phase();
    void loop();
};
