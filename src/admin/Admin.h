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

/**
 * @Catherine135
 * @brief The Admin owns write access to Map. It pulls Actions from queues, validates them, applies to Map under lock, and emits RenderTasks.
 */
class Admin {
public:
    /**
     * @Catherine135
     * @brief Constructor for Admin class.
     * @param r Reference to the Renderer instance for submitting render tasks.
     * @param level Initial difficulty level for map generation.
     */
    Admin(Renderer& r, int level) 
        : map(level), r_(r) {}
    ~Admin() { stop(); } // auto stop

    /**
     * @Catherine135
     * @brief Start the admin loop in a separate thread.
     */
    void start() { // start thread
        if (worker.joinable()) return;
        running = true;
        play_state = false;
        worker = std::thread(&Admin::loop, this);
    }

    /**
     * @Catherine135
     * @brief Initialize the game state and regenerate the map at the specified level.
     * @param level Difficulty level for map generation.
     */
    void init(int level=1);

    /**
     * @Catherine135
     * @brief Pause the game processing.
     */
    void pause() {
        play_state = false;
    }

    /**
     * @Catherine135
     * @brief Resume the game processing.
     */
    void resume() {
        play_state = true;
    }

    /**
     * @Catherine135
     * @brief Stop the admin loop and join the thread.
     */
    void stop() {
        if (!worker.joinable()) return;
        running = false;
        humanQ.stop(); 
        aiQ.stop();
        worker.join();
    }

    /**
     * @Catherine135
     * @brief Submit a move action for the human player.
     * @param m Move action to submit.
     */
    void submit_human_action(Move& m) { humanQ.push(m); };

    /**
     * @Catherine135
     * @brief Submit a move action for the AI player.
     * @param m Move action to submit.
     */
    void submit_ai_action(Move& m) { aiQ.push(m); };

    /**
     * @Catherine135
     * @brief Get a reference to the map.
     * @return Reference to the Map instance.
     */
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
