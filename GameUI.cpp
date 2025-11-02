#include "GameUI.h"
#include "ThreadSafeQueue.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

GameUI::GameUI(ThreadSafeQueue<Move>& human_queue, ThreadSafeQueue<RenderTask*>& render_queue)
    : is_running_(false)
    , current_state_(GameState::INIT)
    , human_action_queue_(human_queue)
    , render_queue_(render_queue)
    , has_selected_tile_(false)
    , drawing_arrow_(false)
    , map_width_(0)
    , map_height_(0) {
}

GameUI::~GameUI() {
    Stop();
}

bool GameUI::Initialize() {
    SetupTerminal();
    return true;
}

void GameUI::Start() {
    if (is_running_) return;
    
    is_running_ = true;
    ui_thread_ = std::thread(&GameUI::UIMainLoop, this);
}

void GameUI::Stop() {
    if (!is_running_) return;
    
    is_running_ = false;
    if (ui_thread_.joinable()) {
        ui_thread_.join();
    }
    RestoreTerminal();
}

void GameUI::SetEventCallback(EventCallback callback) {
    event_callback_ = callback;
}

void GameUI::SetMapInfo(int width, int height) {
    map_width_ = width;
    map_height_ = height;
}

void GameUI::UIMainLoop() {
    while (is_running_) {
        ClearScreen();
        
        switch (current_state_) {
            case GameState::INIT:
            case GameState::WAITING_FOR_PLAYERS:
                SendMenuTask();
                ProcessUserInput();
                break;
            case GameState::PLAYING:
                SendGameBoardTask();
                ProcessUserInput();
                break;
            case GameState::PAUSED:
                SendGameBoardTask();
                ProcessUserInput();
                std::cout << "\n=== GAME PAUSED ===" << std::endl;
                break;
            case GameState::GAME_OVER:
                SendGameBoardTask();
                std::cout << "\n=== GAME OVER ===" << std::endl;
                break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void GameUI::SendMenuTask() {
    struct MainMenuTask : public RenderTask {
        GameState current_state;
        std::vector<PlayerInfo> players;
        int map_width, map_height;
        
        MainMenuTask(GameState state, const std::vector<PlayerInfo>& players_list, 
                    int width, int height)
            : current_state(state), players(players_list), 
              map_width(width), map_height(height) {}
    };
    
    RenderTask* task = new MainMenuTask(current_state_, players_, map_width_, map_height_);
    render_queue_.push(task);
}

void GameUI::SendGameBoardTask() {
    struct GameBoardTask : public RenderTask {
        std::pair<int, int> selected_tile;
        bool has_selected_tile;
        bool drawing_arrow;
        std::pair<int, int> arrow_start;
        GameState current_state;
        std::vector<PlayerInfo> players;
        
        GameBoardTask(const std::pair<int, int>& selected, bool has_selected, 
                     bool drawing, const std::pair<int, int>& arrow, 
                     GameState state, const std::vector<PlayerInfo>& players_list)
            : selected_tile(selected), has_selected_tile(has_selected),
              drawing_arrow(drawing), arrow_start(arrow),
              current_state(state), players(players_list) {}
    };
    
    RenderTask* task = new GameBoardTask(selected_tile_, has_selected_tile_, 
                                        drawing_arrow_, arrow_start_,
                                        current_state_, players_);
    render_queue_.push(task);
}

void GameUI::AddPlayer(const PlayerInfo& player) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    players_.push_back(player);
    
    if (event_callback_) {
        event_callback_(GameEvent::PLAYER_JOINED, player.name);
    }
}

void GameUI::RemovePlayer(int player_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = std::find_if(players_.begin(), players_.end(),
        [player_id](const PlayerInfo& p) { return p.id == player_id; });
    
    if (it != players_.end()) {
        std::string player_name = it->name;
        players_.erase(it);
        
        if (event_callback_) {
            event_callback_(GameEvent::PLAYER_LEFT, player_name);
        }
    }
}

void GameUI::UpdatePlayerReadyStatus(int player_id, bool is_ready) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    for (auto& player : players_) {
        if (player.id == player_id) {
            player.is_ready = is_ready;
            
            if (event_callback_) {
                event_callback_(GameEvent::PLAYER_READY, player.name);
            }
            break;
        }
    }
}

void GameUI::SetGameState(GameState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_state_ = state;
    
    // Reset selection state when entering PLAYING state
    if (state == GameState::PLAYING) {
        has_selected_tile_ = false;
        drawing_arrow_ = false;
    }
    
    // Notify event callback of state change
    if (event_callback_) {
        switch (state) {
            case GameState::PLAYING:
                event_callback_(GameEvent::GAME_STARTED, "");
                break;
            case GameState::PAUSED:
                event_callback_(GameEvent::GAME_PAUSED, "");
                break;
            case GameState::GAME_OVER:
                event_callback_(GameEvent::GAME_ENDED, "");
                break;
            default:
                break;
        }
    }
}

void GameUI::ShowChatMessage(const std::string& player, const std::string& message) {
    if (event_callback_) {
        std::string full_message = player + ": " + message;
        event_callback_(GameEvent::CHAT_MESSAGE, full_message);
    }
}

void GameUI::ProcessUserInput() {
    switch (current_state_) {
        case GameState::INIT:
        case GameState::WAITING_FOR_PLAYERS:
            HandleMenuInput();
            break;
        case GameState::PLAYING:
            HandleGameInput();
            break;
        default:
            break;
    }
}

void GameUI::ClearScreen() {
    static int clear_count = 0;
    clear_count++;
    
    if (clear_count % 10 == 0) {
        // system("clear");
        // std::cout << "\n" << std::string(50, '=') << "\n" << std::endl;
    }
}

void GameUI::HandleMenuInput() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv)) {
        char c = getchar();
        switch (c) {
            case '1':
                SendMessageTask("Starting game...");
                SetGameState(GameState::PLAYING);
                break;
                
            case '2':
                SendMessageTask("Joining game...");
                std::cout << "Generals Game Rules ... " << std::endl;
                break;
                
            case '3':
                SendMessageTask("Opening settings...");
                ShowSettingsMenu();
                break;
                
            case '4':
                SendMessageTask("Exiting game...");
                is_running_ = false;
                break;
                
            case 'r': case 'R':
                ToggleReadyStatus();
                break;
                
            case 'h': case 'H':
                ShowHelp();
                break;
                
            case 27: // ESC key
                HandleEscapeKey();
                break;
                
            case 10: // Enter key
                HandleEnterKey();
                break;
                
            default:
                if (isprint(c)) {
                    std::string msg = "Unknown command: '";
                    msg += c;
                    msg += "'. Press H for help";
                    SendMessageTask(msg);
                }
                break;
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void GameUI::ShowSettingsMenu() {
    // Example settings menu display
    std::cout << "\n=== SETTINGS ===" << std::endl;
    std::cout << "1. Player Name" << std::endl;
    std::cout << "2. Game Difficulty" << std::endl;
    std::cout << "3. Sound Settings" << std::endl;
    std::cout << "4. Back to Main Menu" << std::endl;
    std::cout << "=================" << std::endl;
}

void GameUI::ToggleReadyStatus() {
    bool new_ready_status = !IsPlayerReady(0);
    UpdatePlayerReadyStatus(0, new_ready_status);
    
    if (new_ready_status) {
        SendMessageTask("You are now READY");
    } else {
        SendMessageTask("You are NOT READY");
    }
}

bool GameUI::IsPlayerReady(int player_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Find player by ID and return ready status
    for (const auto& player : players_) {
        if (player.id == player_id) {
            return player.is_ready;
        }
    }
    
    // Player not found
    SendMessageTask("Player " + std::to_string(player_id) + " not found");
    return false;
}

void GameUI::ShowHelp() {
    std::cout << "\n=== HELP ===" << std::endl;
    std::cout << "1 - Start New Game" << std::endl;
    std::cout << "2 - Join Game" << std::endl;
    std::cout << "3 - Settings" << std::endl;
    std::cout << "4 - Exit" << std::endl;
    std::cout << "R - Toggle Ready Status" << std::endl;
    std::cout << "S - Start Game (Host only)" << std::endl;
    std::cout << "H - Show this Help" << std::endl;
    std::cout << "ESC - Cancel/Back" << std::endl;
    std::cout << "=============" << std::endl;
    
    SendMessageTask("Help menu displayed");
}

void GameUI::HandleEscapeKey() {
    // Escape key handling - cancel or go back
    if (current_state_ == GameState::WAITING_FOR_PLAYERS) {
        SendMessageTask("Press 4 to exit game");
    } else {
        SendMessageTask("Operation cancelled");
    }
}

void GameUI::HandleEnterKey() {
    // Enter key handling - confirm selection
    SendMessageTask("Press specific menu number to select option");
}

void GameUI::HandleGameInput() {
    if (current_state_ != GameState::PLAYING) {
        return; 
    }

    // Linux terminal setup for non-blocking input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt); 
    newt = oldt; 
    newt.c_lflag &= ~(ICANON | ECHO); 
    newt.c_cc[VMIN] = 0; 
    newt.c_cc[VTIME] = 0; 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); 

    // Check for input using select
    struct timeval tv = {0, 0}; 
    fd_set fds; 
    FD_ZERO(&fds); 
    FD_SET(STDIN_FILENO, &fds);  

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        int ch = getchar();

        if (ch == 27) {
            if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                ch = getchar();
                if (ch == 91) {  // '[' character
                    ch = getchar();
                    int dx = 0, dy = 0;
                    
                    switch (ch) {
                        case 'A': dy = -1; break; // Up
                        case 'B': dy = 1; break;  // Down
                        case 'D': dx = -1; break; // Left
                        case 'C': dx = 1; break;  // Right
                    }

                    // Handle arrow key movement
                    if (drawing_arrow_) {
                        // Within drawing arrow mode: update arrow end point
                        std::pair<int, int> new_end = {
                            selected_tile_.first + dx,
                            selected_tile_.second + dy
                        };
                        
                        if (IsValidCoordinate(new_end.first, new_end.second)) {
                            // Update arrow display
                            SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
                            selected_tile_ = new_end;
                            SendSelectTileTask(selected_tile_.first, selected_tile_.second, true);
                            SendArrowTask(arrow_start_.first, arrow_start_.second, 
                                        selected_tile_.first, selected_tile_.second, true);
                        }
                    } else if (has_selected_tile_) {
                        // Handle selection box movement
                        std::pair<int, int> new_pos = {
                            selected_tile_.first + dx,
                            selected_tile_.second + dy
                        };
                        
                        if (IsValidCoordinate(new_pos.first, new_pos.second)) {
                            SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
                            selected_tile_ = new_pos;
                            SendSelectTileTask(selected_tile_.first, selected_tile_.second, true);
                        }
                    } else {
                        // Initial selection
                        HandleTileSelection(dx, dy);
                    }
                }
            }
        } 
        // Handle WASD keys for movement
        else if (ch == 'w' || ch == 'W') {
            MoveSelection(0, -1);
        } else if (ch == 's' || ch == 'S') {
            MoveSelection(0, 1);
        } else if (ch == 'a' || ch == 'A') {
            MoveSelection(-1, 0);
        } else if (ch == 'd' || ch == 'D') {
            MoveSelection(1, 0);
        }
        // Handle other keys
        else {
            // Handle normal keys
            switch (ch) {
                case ' ': // Space key
                HandleSelection();
                break;
                
            case 10: // Enter key (Linux uses 10 instead of 13)
                if (!has_selected_tile_) {
                    HandleTileSelection(0, 0);
                } else {
                    HandleSelection();
                }
                break;
                    
            case 27: // ESC key cancel (standalone ESC, not arrow sequence)
                if (drawing_arrow_) {
                    SendArrowTask(arrow_start_.first, arrow_start_.second, selected_tile_.first, selected_tile_.second, false);
                    drawing_arrow_ = false;
                    SendMessageTask("Move cancelled");
                } else if (has_selected_tile_) {
                    SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
                    has_selected_tile_ = false;
                    SendMessageTask("Selection cancelled");
                }
                break;
                    
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                SetArmyCount(ch - '0');
                break;

            case 'q': case 'Q': // Quit game
                is_running_ = false;
                SendMessageTask("Quitting game");
                break;
                    
            case 'p': case 'P': // Pause game
                if (current_state_ == GameState::PLAYING) {
                    current_state_ = GameState::PAUSED;
                    SendMessageTask("Game paused");
                    } else if (current_state_ == GameState::PAUSED) {
                        current_state_ = GameState::PLAYING;
                        SendMessageTask("Game resumed");
                    }
                    break;
            }
        }
    } 
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}


void GameUI::MoveSelection(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    
    if (drawing_arrow_) {
        // Update arrow end point
        std::pair<int, int> arrow_end = {
            selected_tile_.first + dx, 
            selected_tile_.second + dy
        };
                    
        if (arrow_end.first >= 0 && arrow_end.first < map_width_ && arrow_end.second >= 0 && arrow_end.second < map_height_) {
            // Clear previous arrow
            SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
            selected_tile_ = arrow_end;
            SendSelectTileTask(selected_tile_.first, selected_tile_.second, true);
            // Send new arrow
            SendArrowTask(arrow_start_.first, arrow_start_.second, arrow_end.first, arrow_end.second, true);
        }
    } else if (has_selected_tile_) {
        // Move selection box to adjacent position
        std::pair<int, int> new_pos = {
            selected_tile_.first + dx,
            selected_tile_.second + dy
        };
        if (new_pos.first >= 0 && new_pos.first < map_width_ && new_pos.second >= 0 && new_pos.second < map_height_) {
            HandleTileSelection(new_pos.first, new_pos.second);
        }
    } else {
        // Initial selection position
        HandleTileSelection(dx, dy);
    }
}

void GameUI::HandleSelection() {
    if (!has_selected_tile_) {
        // No tile selected yet, select the first tile (0,0) as default
        HandleTileSelection(0, 0);
        return;
    }
    
    if (drawing_arrow_) {
         if (arrow_start_ != selected_tile_) {
            // Make move
            int army = has_selected_army_count_ ? selected_army_count_ : 1;
            GenerateMoveAction(arrow_start_, selected_tile_, army);
            
            // Clear arrow
            SendArrowTask(arrow_start_.first, arrow_start_.second, 
                        selected_tile_.first, selected_tile_.second, false);
            drawing_arrow_ = false;
            has_selected_army_count_ = false;

            SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
            has_selected_tile_ = false;
            SendMessageTask("Move command sent");
        } else {
            SendMessageTask("Error: Source and target tiles are the same! ");
        }
    } else {
        // Start drawing move arrow
        arrow_start_ = selected_tile_;
        drawing_arrow_ = true;

        SendArrowTask(arrow_start_.first, arrow_start_.second, selected_tile_.first, selected_tile_.second, true);
        SendMessageTask("Select target position with WASD/arrows, then press Space to confirm");
    }
}

void GameUI::CancelAction() {
    if (drawing_arrow_) {
        SendArrowTask(arrow_start_.first, arrow_start_.second, 
                     selected_tile_.first, selected_tile_.second, false);
        drawing_arrow_ = false;
        SendMessageTask("Move cancelled");
    } else if (has_selected_tile_) {
        SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
        has_selected_tile_ = false;
        SendMessageTask("Selection cancelled");
    } else {
        SendMessageTask("No action to cancel");
    }
}

void GameUI::SetArmyCount(int count) {
    if (count < 1 || count > 9) {
        SendMessageTask("Invalid army count. Please select between 1-9");
        return;
    }
    
    if (has_selected_tile_ && drawing_arrow_) {
        selected_army_count_ = count;
        has_selected_army_count_ = true;
        
        std::string message = "Army count set to: " + std::to_string(count);
        
        // Include move details if arrow is being drawn
        if (drawing_arrow_) {
            message += " for move from (" + 
                      std::to_string(arrow_start_.first) + "," + 
                      std::to_string(arrow_start_.second) + ") to (" +
                      std::to_string(selected_tile_.first) + "," + 
                      std::to_string(selected_tile_.second) + ")";
        }
        
        SendMessageTask(message);
        
    } else if (has_selected_tile_ && !drawing_arrow_) {
        // Set army count for future move
        selected_army_count_ = count;
        has_selected_army_count_ = true;
        SendMessageTask("Army count " + std::to_string(count) + " saved. Select target with arrow keys");
        
    } else {
        SendMessageTask("Select a source tile first, then set army count");
    }
}

void GameUI::TogglePause() {
    if (current_state_ == GameState::PLAYING) {
        current_state_ = GameState::PAUSED;
        SendMessageTask("Game paused");
    } else if (current_state_ == GameState::PAUSED) {
        current_state_ = GameState::PLAYING;
        SendMessageTask("Game resumed");
    }
}

void GameUI::GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army) {
    Move move;
    move.from = from;
    move.to = to;
    move.army = army;
    
    // Send move action to human action queue
    human_action_queue_.push(move);
    
    std::stringstream ss;
    ss << "Move order: (" << from.first << "," << from.second << ") -> (" 
       << to.first << "," << to.second << ") with " << army << " armies";
    SendMessageTask(ss.str());
}

void GameUI::HandleTileSelection(int x, int y) {
    if (!IsValidCoordinate(x, y)) {
        SendMessageTask("Invalid coordinates!");
        return;
    }
    
    // Deselect previous tile if any
    if (has_selected_tile_) {
        SendSelectTileTask(selected_tile_.first, selected_tile_.second, false);
    }

    selected_tile_ = {x, y};
    has_selected_tile_ = true;
    SendSelectTileTask(x, y, true);

    std::stringstream ss;
    ss << "Tile selected: (" << x << ", " << y << ")";
    SendMessageTask(ss.str());
    
    if (event_callback_) {
        event_callback_(GameEvent::TILE_SELECTED, ss.str());
    }
}

// Send render tasks to the render queue
void GameUI::SendSelectTileTask(int x, int y, bool select) {
    RenderTask* task = new SelectTile({x, y}, select);
    render_queue_.push(task);
}

void GameUI::SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw) {
    RenderTask* task = new Arrow({from_x, from_y}, {to_x, to_y}, draw);
    render_queue_.push(task);
}

void GameUI::SendMessageTask(const std::string& message) {
    RenderTask* task = new ShowMsg(message);
    render_queue_.push(task);
}

bool GameUI::IsValidCoordinate(int x, int y) const {
    return x >= 0 && x < map_width_ && y >= 0 && y < map_height_;
}

void GameUI::SetupTerminal() {
    tcgetattr(STDIN_FILENO, &original_termios_);
    
    struct termios new_termios = original_termios_;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void GameUI::RestoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios_);
}