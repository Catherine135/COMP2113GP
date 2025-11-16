#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ncurses.h>

#include "Human.h"
#include "Logger.h"

void CHuman::Init() {
    has_selected_tile_ = false;

    std::vector<std::vector<Tile>> snap = admin_.get_map().getSnapshot();
    // guard against empty snapshot
    if (!snap.empty() && !snap[0].empty()) {
        // snap is indexed as snap[row][col] -> snap[y][x]
        // iterate rows (y) then columns (x)
        for (int y = 0; y < static_cast<int>(snap.size()); ++y) {
            for (int x = 0; x < static_cast<int>(snap[y].size()); ++x) {
                Tile t = snap[y][x];
                if (t.owner == 0 && t.isCapital()) {
                    user_cursor_x = x;
                    user_cursor_y = y;
                    break;
                }
            }
        }
    }

    LOG_INFOF("CHuman::Init completed with user_cursor_x=%d, user_cursor_y=%d", user_cursor_x, user_cursor_y);
}

int CHuman::ProcessUserInput(char c) {
    switch (c)
    {
    case 'w':
    case 'W':
    case KEY_UP:
        moveCursor(0, -1);
        break;
    
    case 's':
    case 'S':
    case KEY_DOWN:
        moveCursor(0, 1);
        break;
    
    case 'a':
    case 'A':
    case KEY_LEFT:
        moveCursor(-1, 0);
        break;
    
    case 'd':
    case 'D':
    case KEY_RIGHT:
        moveCursor(1, 0);
        break;
    
    case ' ': // Space key
        HandleTileSelection(user_cursor_x, user_cursor_y);
        break;

    default:
        std::string msg = "Unknown command: '";
        if (isprint(c)) {
            msg += c;
            msg += "'. Please check help.";
        } else {
            msg += "0x" + std::to_string(static_cast<int>(c)) + "'. Please check help.";
        }
        SendMessageTask(msg);
        break;
    } 

    return 0;
}

/*
void CHuman::SendMenuTask() {
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
    r_.submit_task(task);
}

void CHuman::SendGameBoardTask() {
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
    r_.submit_task(task);
}

void CHuman::ShowChatMessage(const std::string& player, const std::string& message) {
    if (event_callback_) {
        std::string full_message = player + ": " + message;
        event_callback_(GameEvent::CHAT_MESSAGE, full_message);
    }
}

void CHuman::ToggleReadyStatus() {
    bool new_ready_status = !IsPlayerReady(0);
    UpdatePlayerReadyStatus(0, new_ready_status);
    
    if (new_ready_status) {
        SendMessageTask("You are now READY");
    } else {
        SendMessageTask("You are NOT READY");
    }
}

bool CHuman::IsPlayerReady(int player_id) {
    //std::lock_guard<std::mutex> lock(state_mutex_);
    
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

void CHuman::ShowHelp() {
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

void CHuman::HandleEscapeKey() {
    // Escape key handling - cancel or go back
    if (current_state_ == GameState::WAITING_FOR_PLAYERS) {
        SendMessageTask("Press 4 to exit game");
    } else {
        SendMessageTask("Operation cancelled");
    }
}

void CHuman::HandleEnterKey() {
    // Enter key handling - confirm selection
    SendMessageTask("Press specific menu number to select option");
}
*/

void CHuman::moveCursor(int dx, int dy) {
    LOG_DEBUGF("CHuman::moveCursor called with dx=%d, dy=%d, cur-x-%d, cur-y-%d", dx, dy, user_cursor_x, user_cursor_y);
    if (dx == 0 && dy == 0) return;

    std::vector<std::vector<Tile>> snap = admin_.get_map().getSnapshot();
    int target_x = user_cursor_x + dx;
    int target_y = user_cursor_y + dy;
    if (!IsMovableLand(target_x, target_y, snap)) {
        LOG_INFOF("Non-IsMovableLand - %d,%d,%d,%d", target_x, target_y, user_cursor_x, user_cursor_y);
        SendMessageTask("Can't move to " + std::to_string(target_x) + "," + std::to_string(target_y));
        return;
    }

    if (has_selected_tile_) {
        Move m;
        m.from.first = user_cursor_x; 
        m.from.second = user_cursor_y;
        m.to.first = target_x; 
        m.to.second = target_y;
        //m.army = 1 + (snap[m.from.second][m.from.first].army/2); // half army
        m.army = snap[m.from.second][m.from.first].army - 1; // all army but leave one behind
        admin_.submit_human_action(m);

        LOG_INFOF("Submitted Move Action - from (%d,%d) to (%d,%d) with army %d", 
            m.from.first, m.from.second, m.to.first, m.to.second, m.army);
        
        SendArrowTask(user_cursor_x, user_cursor_y, target_x, target_y, true);
    } else {
        RenderTask* task = new MoveCursor({target_x, target_y}, snap);
        renderer_.submit_task(task);

        LOG_INFOF("Cursor move without selection from (%d,%d) to (%d,%d) with army %d", 
            user_cursor_x, user_cursor_y, target_x, target_y, 0);
    }
    
    // update current cursor position
    user_cursor_x += dx;
    user_cursor_y += dy;
    return;
}

/*void CHuman::HandleSelection() {
    if (!has_selected_tile_) {
        HandleTileSelection(user_cursor_x, user_cursor_y);
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
}*/
/*
void CHuman::CancelAction() {
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
}*/
/*
void CHuman::SetArmyCount(int count) {
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
}*/
/*
void CHuman::TogglePause() {
    if (current_state_ == GameState::PLAYING) {
        current_state_ = GameState::PAUSED;
        SendMessageTask("Game paused");
    } else if (current_state_ == GameState::PAUSED) {
        current_state_ = GameState::PLAYING;
        SendMessageTask("Game resumed");
    }
}
*/
void CHuman::GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army) {
    Move move;
    move.from = from;
    move.to = to;
    move.army = army;
    
    // Send move action to human action queue
    admin_.submit_human_action(move);
    
    std::stringstream ss;
    ss << "Move order: (" << from.first << "," << from.second << ") -> (" 
       << to.first << "," << to.second << ") with " << army << " armies";
    SendMessageTask(ss.str());
}

void CHuman::HandleTileSelection(int x, int y) {
    Map & map = admin_.get_map();

    // Deselect previous tile if any
    if (has_selected_tile_) {
        SendSelectTileTask(false, map.getSnapshot());
        has_selected_tile_ = false;
        SendMessageTask("Previous selection cleared");
        return;
    }

    if (!IsSelectableTile(x, y, map)) {
        SendMessageTask("Non-Selectable tile!");
        return;
    }
    
    has_selected_tile_ = true;
    SendSelectTileTask(true, map.getSnapshot());

    std::stringstream ss;
    ss << "Tile selected: (" << x << ", " << y << ")";
    SendMessageTask(ss.str());
}

// Send render tasks to the render queue
void CHuman::SendSelectTileTask(/*int x, int y, */bool select, std::vector<std::vector<Tile>> snap) {
    RenderTask* task = new SelectTile(/*{x, y}, */select, snap);
    renderer_.submit_task(task);
}

void CHuman::SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw) {
    RenderTask* task = new Arrow({from_x, from_y}, {to_x, to_y}, draw, true, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);
}

void CHuman::SendMessageTask(const std::string& message) {
    RenderTask* task = new ShowMsg(message);
    renderer_.submit_task(task);
}

/*bool CHuman::IsValidCoordinate(int x, int y) const {
    return (x >= 0 && x < admin_.get_map().getWidth() && y >= 0 && y < admin_.get_map().getHeight());
}*/

bool CHuman::IsSelectableTile(int x, int y, const Map& map) const {
    Tile tile = map.getTile(x, y);
    return (x >= 0 && x < map.getWidth() && y >= 0 && y < map.getHeight() && tile.owner == 0 && tile.army > 0);
}

bool CHuman::IsMovableLand(int x, int y, const std::vector<std::vector<Tile>>& snap) const {
    if (x < 0 || x >= (int)snap[0].size() || y < 0 || y >= (int)snap.size()) {
        return false;
    }

    Tile tile = snap[y][x];
    return (!tile.isMountain());
}