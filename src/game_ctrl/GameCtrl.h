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
#define MAX_GAME_LEVEL 4
#endif

/**
 * @Amelia-Wang-Hanyu
 * @brief Enumeration for different game states.
 */
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

/**
 * @Amelia-Wang-Hanyu
 * @brief Structure to hold player information.
 */
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
    /**
     * @Amelia-Wang-Hanyu
     * @brief Constructor for GameCtrl.
     * @param a Reference to the Admin instance.
     * @param r Reference to the Renderer instance.
     * @param ai Reference to the GameAI instance.
     * @param human Reference to the CHuman instance.
     */
    GameCtrl(Admin& a, Renderer& r, GameAI& ai, CHuman& human);
    ~GameCtrl();

    /**
     * @brief Get the singleton instance of GameCtrl.
     * @return Pointer to the GameCtrl instance.
     */
    static GameCtrl* getInstance();
    
    /**
     * @Amelia-Wang-Hanyu
     * @brief Initialize the game controller and its components.
     */
    void Init(); 

    /**
     * @Amelia-Wang-Hanyu
     * @brief Start a new game session.
     * @param isLevelUp Indicates if the game is a level-up scenario.
     * @param round The round number at which the game is starting.
     */
    void StartNextLevelGame(bool isLevelUp, int round);

    /**
     * @Amelia-Wang-Hanyu
     * @brief Process user input based on the current game state.
     * @param c Character input from the user.
     * @return -1 to exit game, 0 for successful processing, 1 for human input, -2 for error.
     */
    int ProcessUserInput(int c);

    /**
     * @Amelia-Wang-Hanyu
     * @brief Add a player to the game.
     * @param player PlayerInfo structure containing player details.
     */
    void AddPlayer(const PlayerInfo& player);

    /**
     * @Amelia-Wang-Hanyu
     * @brief Remove a player from the game by their ID.
     * @param player_id ID of the player to remove.
     */
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
    void RenderInitInterface();

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