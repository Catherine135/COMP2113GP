#pragma once
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
#include "Admin.h"
#include "Renderer.h"
#include "GameAI.h"
#include "Human.h"

#ifndef MAX_GAME_LEVEL
#define MAX_GAME_LEVEL 6
#endif

// Game Status Enumeration
enum class GameState {
    INIT,
    PLAYING,
    PAUSED,
    RESETTING,
    QUITTING,
    GAME_OVER,
    GAME_CLEARED,
    GAME_MAX
};

// Player Information Structure
struct PlayerInfo {
    int id;
    std::string name;
    int color;
    int level;
};
/*
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
*/
class GameCtrl {
public:
    // Constructor and Destructor
    GameCtrl(Admin& a, Renderer& r, GameAI& ai, CHuman& human);
    // Convenience constructor: if caller doesn't provide a CHuman, GameCtrl will create and own one.
    //GameCtrl(Admin& a, Renderer& r, GameAI& ai);
    ~GameCtrl();

    // Accessor to get the current GameCtrl instance (or nullptr if none)
    static GameCtrl* getInstance();
     
    void Init(); 

    void StartNextLevelGame(bool isLevelUp, int round);

    // User input processing
    // -2 - error
    // -1 exit game
    // 0 - process success
    // 1 - human input
    int ProcessUserInput(int c);

    // Player management
    void AddPlayer(const PlayerInfo& player);
    void RemovePlayer(int player_id);

private:
    void StartGame();
    void RestartGame();
    void PauseGame();
    void ResumeGame();
    void ConfirmReset();
    void ConfirmExit();
    void QuitGame();
    //void ShowHelp();

    bool IsPlayGameKey(int c);
    bool IsMenuKey(int c);

    int ProcessMenuInput(char c);

    // Game state management
    void SetGameState(GameState state);
    //GameState GetGameState() const { return current_state_; }
    //void ShowGameBoard();
    //void ConfirmExitMessage();
    //void ShowSystemMessage(const std::string& message);

    // Handle different input modes
    //void HandleMenuInput();
    //void HandleGameInput();
    //void HandleChatInput();
    //void MoveSelection(int dx, int dy);
    //void HandleSelection();
    //void CancelAction();
    //void SetArmyCount(int count);
    //void TogglePause();
    
    // Action generation
    //void GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army);
    //void HandleTileSelection(int x, int y);

    // Render task sending
    //void SendMenuTask();
    //void SendGameBoardTask();
    //void SendSelectTileTask(int x, int y, bool select);
    //void SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw);
    void SendMessageTask(const std::string& message);
    //void ClearScreen();

    // Menu handling
    //void ShowSettingsMenu();
    //void ToggleReadyStatus();
    //void StartGameIfHost();
    //void ShowHelp();
    //void HandleEscapeKey();
    //void HandleEnterKey();
    //bool IsPlayerReady(int player_id);
    
    // Make the single instance accessible to static wrappers
    static GameCtrl* instance_;

    // Current game state and players
    GameState current_state_;
    std::vector<PlayerInfo> players_;

    // If GameCtrl created its own CHuman (via 3-arg ctor), we own it here.
    //std::unique_ptr<CHuman> owned_human_;

    // Input state
    //std::pair<int, int> selected_tile_;
    //bool has_selected_tile_;
    //std::pair<int, int> arrow_start_;
    //bool drawing_arrow_;

    Admin& admin_;
    Renderer& renderer_;
    GameAI& ai_;
    CHuman& human_;
        
    // Coordinate validation
    //bool IsValidCoordinate(int x, int y) const;
};