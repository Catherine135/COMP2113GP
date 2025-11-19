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


/**
 * @Amelia-Wang-Hanyu
 * @brief Game Event Enumeration
 */
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

/**
 * @Amelia-Wang-Hanyu
 * @brief Class representing the human player.
 */
class CHuman {
public:
    /**
     * @Amelia-Wang-Hanyu
     * @brief Constructor for CHuman class.
     */
    CHuman(Admin& a, Renderer& r)
        : admin_(a)
        , renderer_(r)
        , has_selected_tile_(false)
        /*, drawing_arrow_(false)*/ {}
    ~CHuman() {}

    /**
     * @Amelia-Wang-Hanyu
     * @brief Initialize the human player state.
     */
    void Init(); 
    
    /**
     * @Amelia-Wang-Hanyu
     * @brief Get the current cursor position.
     * @return Pair of (x, y) coordinates of the cursor.
     */
    std::pair<int, int> GetCursorPosition() const {
        return {user_cursor_x, user_cursor_y};
    }
    
    /**
     * @Amelia-Wang-Hanyu
     * @brief Process user input character.
     * @param c Input character.
     * @return Status code (0 for success, -1 for invalid input).
     */
    int ProcessUserInput(int c);

private:
    //void HandleChatInput();
    void moveCursor(int dx, int dy);
    //void HandleSelection();
    //void CancelAction();
    //void SetArmyCount(int count);
    //void TogglePause();
    
    // Action generation
    void GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army);
    void HandleTileSelection(int x, int y);

    // Render task sending
    //void SendMenuTask();
    //void SendGameBoardTask();
    void SendSelectTileTask(/*int x, int y, */bool select, std::vector<std::vector<Tile>> snap);
    void SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw);
    void SendMessageTask(const std::string& message);
    //void ClearScreen();
    
    Admin& admin_;
    Renderer& renderer_;

    // Input state
    int user_cursor_x{0};
    int user_cursor_y{0};
    //std::pair<int, int> selected_tile_;
    bool has_selected_tile_{false};
    //std::pair<int, int> arrow_start_;
    //bool drawing_arrow_{false};
    
    // Selecting army count
    //int selected_army_count_;
    //bool has_selected_army_count_; 
    //std::pair<int, int> move_start_; 
    //std::pair<int, int> move_target_;

    // Coordinate validation
    //bool IsValidCoordinate(int x, int y) const;
    bool IsSelectableTile(int x, int y, const Map& map) const;
    bool IsMovableLand(int x, int y, const std::vector<std::vector<Tile>>& snap) const;
};