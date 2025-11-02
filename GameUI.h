#ifndef GAME_UI_H
#define GAME_UI_H

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <termios.h>
#include "Action.h"
#include "RenderTask.h"
#include "ThreadSafeQueue.h"

// Game Status Enumeration
enum class GameState {
    INIT,
    WAITING_FOR_PLAYERS,
    PLAYING,
    PAUSED,
    GAME_OVER
};

// Player Information Structure
struct PlayerInfo {
    int id;
    std::string name;
    int color;
    bool is_ready;
    bool is_connected;
};

// Game Event Enumeration
enum class GameEvent {
    PLAYER_JOINED,
    PLAYER_LEFT,
    PLAYER_READY,
    GAME_STARTED,
    GAME_PAUSED,
    GAME_RESUMED,
    GAME_ENDED,
    CHAT_MESSAGE,
    MOVE_MADE,
    TILE_SELECTED
};

// Event Callback Type
using EventCallback = std::function<void(GameEvent, const std::string&)>;

class GameUI {
public:
    // Constructor and Destructor
    GameUI(ThreadSafeQueue<Move>& human_queue, ThreadSafeQueue<RenderTask*>& render_queue);
    ~GameUI();

    // Disable copy semantics
    GameUI(const GameUI&) = delete;
    GameUI& operator=(const GameUI&) = delete;

    // Initialize game UI system
    bool Initialize();
    
    // Start game UI main thread
    void Start();
    
    // Stop game UI main thread
    void Stop();
    
    // Set event callback
    void SetEventCallback(EventCallback callback);
    
    // Game UI main loop
    void UIMainLoop();

    // Player management
    void AddPlayer(const PlayerInfo& player);
    void RemovePlayer(int player_id);
    void UpdatePlayerReadyStatus(int player_id, bool is_ready);
    
    // Game state management
    void SetGameState(GameState state);
    void ShowGameBoard();
    void ShowChatMessage(const std::string& player, const std::string& message);
    void ShowSystemMessage(const std::string& message);
    GameState GetGameState() const { return current_state_; }
    
    // User input processing
    void ProcessUserInput();
    
    // Set map information for input validation
    void SetMapInfo(int width, int height);

    // Handle different input modes
    void HandleMenuInput();
    void HandleGameInput();
    void HandleChatInput();
    void MoveSelection(int dx, int dy);
    void HandleSelection();
    void CancelAction();
    void SetArmyCount(int count);
    void TogglePause();
    
    // Action generation
    void GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army);
    void HandleTileSelection(int x, int y);

    // Render task sending
    void SendMenuTask();
    void SendGameBoardTask();
    void SendSelectTileTask(int x, int y, bool select);
    void SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw);
    void SendMessageTask(const std::string& message);
    void ClearScreen();

    // Menu handling
    void ShowSettingsMenu();
    void ToggleReadyStatus();
    void StartGameIfHost();
    void ShowHelp();
    void HandleEscapeKey();
    void HandleEnterKey();
    bool IsPlayerReady(int player_id);
    
    // Thread management
    std::atomic<bool> is_running_;
    std::thread ui_thread_;
    
    // State management
    std::mutex state_mutex_;
    std::condition_variable state_cv_;
    
    // Current game state and players
    GameState current_state_;
    std::vector<PlayerInfo> players_;
    EventCallback event_callback_;
    
    // Queues for actions
    ThreadSafeQueue<Move>& human_action_queue_;
    ThreadSafeQueue<RenderTask*>& render_queue_;
    
    // Input state
    std::pair<int, int> selected_tile_;
    bool has_selected_tile_;
    std::pair<int, int> arrow_start_;
    bool drawing_arrow_;
    
    // Map information
    int map_width_;
    int map_height_;

    // Selecting army count
    int selected_army_count_;
    bool has_selected_army_count_; 
    std::pair<int, int> move_start_; 
    std::pair<int, int> move_target_;
    
    // Terminal setup and restoration
    struct termios original_termios_;
    void SetupTerminal();
    void RestoreTerminal();
    
    // Coordinate validation
    bool IsValidCoordinate(int x, int y) const;
};

#endif