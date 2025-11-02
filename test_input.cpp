#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "GameUI.h"
#include "ThreadSafeQueue.h"
#include "Map.h"

std::atomic<bool> test_running{true};

void CompleteCallback(GameEvent event, const std::string& data) {
    std::cout << data << std::endl;
}

void TestMenuFunctions() {
    std::cout << "\n=== Testing Menu Functions ===" << std::endl;
    std::cout << "Now in menu state, please test:" << std::endl;
    std::cout << "1 (Start Game), 2 (Join Game), 3 (Settings), 4 (Exit), R (Toggle Ready), H (Help) and other menu keys" << std::endl;
    std::cout << "Will automatically enter game state after 10 seconds..." << std::endl;
    
    ThreadSafeQueue<Move> humanQ;
    ThreadSafeQueue<RenderTask*> renderQ;
    GameUI game_ui(humanQ, renderQ);
    
    Map map(8, 6, 123);
    game_ui.SetMapInfo(map.getWidth(), map.getHeight());
    game_ui.SetEventCallback(CompleteCallback);
    
    if (!game_ui.Initialize()) return;
    
    PlayerInfo player{1, "Player", 1, false, true};
    game_ui.AddPlayer(player);
    
    game_ui.Start();
    game_ui.SetGameState(GameState::WAITING_FOR_PLAYERS); // Menu state
    
    // Monitor thread
    std::thread monitor([&]() {
        while (test_running) {
            RenderTask* task;
            if (renderQ.try_pop(task)) delete task;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Improvement: Immediate response to key presses
    bool user_entered_game = false;
    for (int i = 10; i > 0 && test_running && !user_entered_game; --i) {
        // Check if '1' was pressed to enter game
        if (game_ui.GetGameState() == GameState::PLAYING) {
            std::cout << "\nUser pressed '1', entering game test immediately..." << std::endl;
            user_entered_game = true;
            break;
        }
        
        // Check if exited
        if (!game_ui.is_running_) {
            std::cout << "Detected user pressed '4', exiting test" << std::endl;
            test_running = false;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // If user didn't take any action, automatically enter game
    if (test_running && game_ui.is_running_ && !user_entered_game) {
        std::cout << "\nAutomatically entering game test..." << std::endl;
        game_ui.SetGameState(GameState::PLAYING);
    }
    
    // Brief wait to ensure state transition completes
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    game_ui.Stop();
    test_running = false;
    monitor.join();
}

void TestGameFunctions() {
    std::cout << "\n=== Testing Game Functions ===" << std::endl;
    std::cout << "Now in game state, please test:" << std::endl;
    
    ThreadSafeQueue<Move> humanQ;
    ThreadSafeQueue<RenderTask*> renderQ;
    GameUI game_ui(humanQ, renderQ);
    
    Map map(8, 6, 123);
    game_ui.SetMapInfo(map.getWidth(), map.getHeight());
    game_ui.SetEventCallback(CompleteCallback);
    
    if (!game_ui.Initialize()) return;
    
    PlayerInfo player{1, "Player", 1, false, true};
    game_ui.AddPlayer(player);
    
    // Monitor thread
    std::thread monitor([&]() {
        int move_count = 0;
        while (test_running) {
            Move move;
            if (humanQ.try_pop(move)) {
                move_count++;
                std::cout << "Action" << move_count << ": (" 
                          << move.from.first << "," << move.from.second << ")->("
                          << move.to.first << "," << move.to.second << ") Army:" 
                          << move.army << std::endl;
            }
            
            RenderTask* task;
            if (renderQ.try_pop(task)) delete task;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    game_ui.Start();
    game_ui.SetGameState(GameState::PLAYING); // Game state
    renderQ.push(new RefreshMap(map.getSnapshot()));
    
    std::cout << "\nTest steps:" << std::endl;
    std::cout << "1. Movement test:" << std::endl;
    std::cout << "   - Use arrow keys to select start point (e.g., from 0,0 to 1,1)" << std::endl;
    std::cout << "   - Press Space to start movement" << std::endl;
    std::cout << "   - Use arrow keys to select end point (e.g., from 1,1 to 2,2)" << std::endl;
    std::cout << "   - Press Space to confirm movement" << std::endl;
    std::cout << std::endl;
    std::cout << "2. Army count test:" << std::endl;
    std::cout << "   - After starting movement (when arrow appears) press 5" << std::endl;
    std::cout << "   - Should see army count set to 5" << std::endl;
    std::cout << std::endl;
    std::cout << "3. Cancel test:" << std::endl;
    std::cout << "   - After starting movement press R to cancel" << std::endl;
    std::cout << std::endl;
    std::cout << "4. Pause test:" << std::endl;
    std::cout << "   - Press P to pause, press P again to resume" << std::endl;
    std::cout << std::endl;
    std::cout << "Testing for 60 seconds..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    for (int i = 60; i > 0 && test_running; --i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    game_ui.Stop();
    test_running = false;
    monitor.join();
}

int main() {
    // First test menu functions
    TestMenuFunctions();
    
    // Then test game functions  
    test_running = true;
    TestGameFunctions();
    
    std::cout << "\nTest completed" << std::endl;
    return 0;
}