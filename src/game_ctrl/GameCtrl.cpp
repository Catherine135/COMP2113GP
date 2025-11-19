#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "Logger.h"
#include "GameCtrl.h"
#include <fstream>
#include <memory>

// static instance pointer definition
GameCtrl* GameCtrl::instance_ = nullptr;

// Constructor (caller provides CHuman)
GameCtrl::GameCtrl(Admin& a, Renderer& r, GameAI& ai, CHuman& human)
    : current_state_(GameState::INIT)
    , admin_(a)
    , renderer_(r)
    , ai_(ai)
    , human_(human)
    /*, owned_human_(nullptr)*/
{
    instance_ = this;
}
/*
// Constructor (GameCtrl creates owned CHuman)
GameCtrl::GameCtrl(Admin& a, Renderer& r, GameAI& ai)
    : current_state_(GameState::INIT)
    , admin_(a)
    , renderer_(r)
    , ai_(ai)
    , owned_human_(std::make_unique<CHuman>(a, r))
    , human_(*owned_human_)
{
    instance_ = this;
}*/

GameCtrl::~GameCtrl() {
    if (instance_ == this) instance_ = nullptr;
}

void GameCtrl::StartNextLevelGame(bool isLevelUp, int round) {
    if (isLevelUp && !players_.empty() && players_[0].level >= MAX_GAME_LEVEL) {
        LOG_INFOF("Player %s has reached max level %d, cannot level up further.", players_[0].name.c_str(), MAX_GAME_LEVEL);
        //isLevelUp = false; // prevent level up beyond max

        // send msessage to renderer
        RenderTask* task = new GameClearedInterface();
        renderer_.submit_task(task);

        SetGameState(GameState::GAME_CLEARED);
        ai_.pause(); 
        admin_.pause();
        /* renderer don't need to pause here, let it work */
        /* human don't need to do anything here, because it is controlled by user input */

        LOG_INFOF("Current game cleared. Waiting for exit.");
        return;
    }

    if (isLevelUp) {
        RenderTask *endTask = new EndGameInterface(true, round);
        renderer_.submit_task(endTask);
    } else {
        RenderTask *endTask = new EndGameInterface(false, round);
        renderer_.submit_task(endTask);
    }

    // increment player level and re-init the game
    if (isLevelUp && !players_.empty()) {
        players_[0].level += 1;
        LOG_INFOF("Player %s leveled up to level %d", players_[0].name.c_str(), players_[0].level);
    
        // update new level to user-data.txt for user persistence (update only current user, preserve others)
        // Load existing users
        std::vector<PlayerInfo> fileUsers;
        std::ifstream ifs("user-data.txt");
        if (ifs.is_open()) {
            std::string line;
            while (std::getline(ifs, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line);
                PlayerInfo p;
                std::string token;
                if (std::getline(ss, token, ',')) p.id = std::stoi(token);
                if (std::getline(ss, token, ',')) p.name = token;
                if (std::getline(ss, token, ',')) p.color = std::stoi(token);
                if (std::getline(ss, token, ',')) p.level = std::stoi(token);
                fileUsers.push_back(p);
            }
            ifs.close();
        }

        // For each player in players_, update matching record in fileUsers (by id), or append if missing
        for (const auto& pl : players_) {
            bool updated = false;
            for (auto& fu : fileUsers) {
                if (fu.id == pl.id) {
                    fu.level = pl.level;
                    fu.name = pl.name;
                    fu.color = pl.color;
                    updated = true;
                    break;
                }
            }
            if (!updated) {
                fileUsers.push_back(pl);
            }
        }

        // Write back all users
        std::ofstream ofs("user-data.txt");
        if (ofs.is_open()) {
            for (const auto& fu : fileUsers) {
                ofs << fu.id << "," << fu.name << "," << fu.color << "," << fu.level << "\n";
            }
            ofs.close();
        }
    }

    // set game state and wait for next level game or restart game
    SetGameState(GameState::GAME_OVER);
    ai_.pause(); 
    admin_.pause();
    /* renderer don't need to pause here, let it work */
    /* human don't need to do anything here, because it is controlled by user input */

    LOG_INFOF("Current game over. Waiting for next level game or restart.");
}

// Accessor implementation
GameCtrl* GameCtrl::getInstance() {
    if (instance_ == nullptr) {
        LOG_ERROR("GameCtrl instance is not initialized!");
    }
    return instance_;
}

void GameCtrl::Init() {
    admin_.init(players_[0].level);
    human_.Init();
    ai_.init(players_[0].level);

    RenderInitInterface();
}

void GameCtrl::StartGame() {
    // set game state
    SetGameState(GameState::PLAYING);
    
    // resume admin && AI when game start
    admin_.resume();
    ai_.resume(); 

    // render
    int user_cursor_x = human_.GetCursorPosition().first;
    int user_cursor_y = human_.GetCursorPosition().second;
    RenderTask* task = new StartGameInterface(players_[0].level, players_[0].name, user_cursor_x, user_cursor_y, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);
    SendMessageTask("Starting game...");
}

void GameCtrl::ConfirmReset() {
    // set game state
    SetGameState(GameState::RESETTING);
    SendMessageTask("Restart game...");
    
    RenderTask* task = new ConfirmResetInterface();
    renderer_.submit_task(task);
}

void GameCtrl::RestartGame() {
    // re-init admin, human, ai
    admin_.init(players_[0].level);
    human_.Init();
    ai_.init(players_[0].level);

    // render
    int user_cursor_x = human_.GetCursorPosition().first;
    int user_cursor_y = human_.GetCursorPosition().second;
    RenderTask* task = new StartGameInterface(players_[0].level, players_[0].name, user_cursor_x, user_cursor_y, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);

    // resume admin && AI when game start
    admin_.resume();
    ai_.resume(); 

    // set game state
    SetGameState(GameState::PLAYING);
    LOG_INFOF("Game restarted at level %d", players_[0].level);
}

void GameCtrl::PauseGame() {
    SetGameState(GameState::PAUSED);
    // Pause both AI and Admin so ticks and army growth stop during pause
    ai_.pause(); 
    admin_.pause();
    SendMessageTask("Game paused");
}

void GameCtrl::ResumeGame() {
    // set game state
    SetGameState(GameState::PLAYING);
    
    // render
    int user_cursor_x = human_.GetCursorPosition().first;
    int user_cursor_y = human_.GetCursorPosition().second;
    RenderTask* task = new StartGameInterface(players_[0].level, players_[0].name, user_cursor_x, user_cursor_y, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);

    // Resume both Admin and AI so the game continues ticking
    admin_.resume();
    ai_.resume(); 

    SendMessageTask("Resuming game...");
}

void GameCtrl::ConfirmExit() {
    GameState previous_state = current_state_;
    SetGameState(GameState::QUITTING);
    ai_.pause();
    admin_.pause();

    // Keep the existing interface visible (especially before game start) by
    // re-rendering the current screen before overlaying the confirmation text.
    if (!players_.empty()) {
        if (previous_state == GameState::INIT) {
            RenderInitInterface();
        } else if (previous_state == GameState::PLAYING || previous_state == GameState::PAUSED) {
            int user_cursor_x = human_.GetCursorPosition().first;
            int user_cursor_y = human_.GetCursorPosition().second;
            RenderTask* redraw = new StartGameInterface(
                players_[0].level,
                players_[0].name,
                user_cursor_x,
                user_cursor_y,
                admin_.get_map().getSnapshot());
            renderer_.submit_task(redraw);
        }
    }

    RenderTask* task = new ConfirmExitInterface();
    renderer_.submit_task(task);
}

void GameCtrl::QuitGame() {
    ai_.pause();
    SendMessageTask("Exiting game...");
}
/*
void GameCtrl::ShowHelp() {
    RenderTask* task = new HelpInterface();
    renderer_.submit_task(task);
}*/

bool GameCtrl::IsPlayGameKey(int c) {
    return (c == 'A' || c == 'a'|| c == 'S' || c == 's'|| c == 'D' || c == 'd'|| c == 'W' || c == 'w' ||
            c == ' ');
}

bool GameCtrl::IsMenuKey(int c) {
    return (c == 'g' || c == 'G' || c == 'r' || c == 'R' || c == 'p' || c == 'P' ||
            /*c == 'h' || c == 'H' || */c == 'q' || c == 'Q'/* || c == 27 || c == 10*/);
}

int GameCtrl::ProcessUserInput(int c) {
    LOG_INFOF("Processing user input - %c, current_state - %d", c, current_state_);
    if(c == 'q' || c == 'Q') { // response to quit game in any state
        if (current_state_ == GameState::GAME_CLEARED) { // quit game now
            QuitGame();
            return -1;
        }
        ConfirmExit();
        return 0;
    } /*else if (c == 'h' || c == 'H') // response to help in any state
    {
        ShowHelp();
        return 0;
    }*/
    
    switch (current_state_)
    {
    case GameState::INIT:
        if(IsMenuKey(c)) {
            return ProcessMenuInput(c);
        }
        RenderInitInterface();
        break;
    
    case GameState::PLAYING:
        if (IsPlayGameKey(c)) { // 方向键盘、空格键，为游戏操作键
            return 1; // Let human handle game input
        } else if (IsMenuKey(c)) {
            return ProcessMenuInput(c);
        }
        break;
    
    case GameState::PAUSED:
        if(c == 'p' || c == 'P') {
            return ProcessMenuInput(c);
        }
        break; 
    case GameState::QUITTING:
        if(c == 'y' || c == 'Y' || c == 'n' || c == 'N') {
            return ProcessMenuInput(c);
        }
        break;
    case GameState::RESETTING:
        if(c == 'y' || c == 'Y' || c == 'n' || c == 'N') {
            return ProcessMenuInput(c);
        }
        break;
    
    case GameState::GAME_OVER:
        if(c == 'r' || c == 'R') {
            return ProcessMenuInput(c);
        }
        break;

    case GameState::GAME_CLEARED:
        if(c == 'q' || c == 'Q') {
            return ProcessMenuInput(c);
        }
        break;
    
    default:
        break;
    }

    SendMessageTask("Invalid key input in current state ...");
    return 0;
}

int GameCtrl::ProcessMenuInput(char c) {
    switch (c) {
        case 'g':
        case 'G':
            if (current_state_ == GameState::INIT) {
                StartGame();
                return 0; 
            }
            break;
            
        case 'r':
        case 'R':
            if (current_state_ == GameState::PLAYING) {
                ConfirmReset();
                return 0;
            }
            else if (current_state_ == GameState::GAME_OVER) {
                RestartGame();
                return 0;
            }
            break;
            
        case 'p': 
        case 'P': // Pause / Resume game
            if (current_state_ == GameState::PLAYING) {
                PauseGame();
                return 0;
            } 
            else if (current_state_ == GameState::PAUSED) {
                ResumeGame();
                return 0;
            }
            break;
            
        /*case 'h': 
        case 'H':
            ShowHelp();
            return 0;*/

        case 'q': 
        case 'Q': 
            if (current_state_ == GameState::GAME_CLEARED) { // quit game now
                QuitGame();
                return -1;
            }

            // Want to quit game
            ConfirmExit();
            return 0;

        case 'y':
        case 'Y': 
            if (current_state_ == GameState::QUITTING) { // Confirm quit
                QuitGame();
                return -1;
            } else if (current_state_ == GameState::RESETTING) { // Confirm restart
                RestartGame();
                return 0;
            }
            break;

        case 'n':
        case 'N': // Cancel quit or restart
            if (current_state_ == GameState::QUITTING ||
                current_state_ == GameState::RESETTING) {
                ResumeGame();
                return 0;
            }
            break;
            
        /*case 27: // ESC key
            //HandleEscapeKey();
            break;
            
        case 10: // Enter key
            //HandleEnterKey();
            break;*/

        default:
            break;
    }

    if (current_state_ == GameState::INIT) {
        RenderInitInterface();
    }
    SendMessageTask("Invalid key input in current state ...");
    return 0;
}
/*
void GameCtrl::SendMenuTask() {
    struct MainMenuTask : public RenderTask {
        GameState current_state;
        std::vector<PlayerInfo> players;
        int map_width, map_height;
        
        MainMenuTask(GameState state, const std::vector<PlayerInfo>& players_list, 
                    int width, int height)
            : current_state(state), players(players_list), 
              map_width(width), map_height(height) {}
    };
    
    RenderTask* task = new MainMenuTask(current_state_, players_, admin_.get_map().getWidth(), admin_.get_map().getHeight());
    renderer_.submit_task(task);
}
*/
/*
void GameCtrl::SendGameBoardTask() {
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
    renderer_.submit_task(task);
}
*/
void GameCtrl::AddPlayer(const PlayerInfo& player) {
    players_.push_back(player);
}

void GameCtrl::RemovePlayer(int player_id) {
    auto it = std::find_if(players_.begin(), players_.end(),
        [player_id](const PlayerInfo& p) { return p.id == player_id; });
    
    if (it != players_.end()) {
        std::string player_name = it->name;
        players_.erase(it);
    }
}

void GameCtrl::SetGameState(GameState state) {
    switch (state)
    {
    case GameState::INIT:
        break;

    case GameState::PLAYING:
        // Reset selection state when entering PLAYING state
        //has_selected_tile_ = false;
        //drawing_arrow_ = false;
        break;

    case GameState::PAUSED:
        /* code */
        break;

    case GameState::QUITTING:
        /* code */
        break;

    case GameState::GAME_OVER: 
        /* code */
        break;
    
    default:
        break;
    }

    current_state_ = state;
}

/*
void GameCtrl::HandleEscapeKey() {
    // Escape key handling - cancel or go back
    if (current_state_ == GameState::WAITING_FOR_PLAYERS) {
        SendMessageTask("Press 4 to exit game");
    } else {
        SendMessageTask("Operation cancelled");
    }
}
*/
/*
void GameCtrl::HandleEnterKey() {
    // Enter key handling - confirm selection
    SendMessageTask("Press specific menu number to select option");
}*/
/*
void GameCtrl::TogglePause() {
    if (current_state_ == GameState::PLAYING) {
        current_state_ = GameState::PAUSED;
        ai_.pause();
        SendMessageTask("Game paused");
    } else if (current_state_ == GameState::PAUSED) {
        current_state_ = GameState::PLAYING;
        ai_.resume();
        SendMessageTask("Game resumed");
    }
}

void GameCtrl::GenerateMoveAction(const std::pair<int, int>& from, const std::pair<int, int>& to, int army) {
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

void GameCtrl::HandleTileSelection(int x, int y) {
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
}

// Send render tasks to the render queue
void GameCtrl::SendSelectTileTask(int x, int y, bool select) {
    RenderTask* task = new SelectTile({x, y}, select, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);
}*/

/*void GameCtrl::SendArrowTask(int from_x, int from_y, int to_x, int to_y, bool draw) {
    RenderTask* task = new Arrow({from_x, from_y}, {to_x, to_y}, draw, admin_.get_map().getSnapshot());
    renderer_.submit_task(task);
}*/

void GameCtrl::SendMessageTask(const std::string& message) {
    RenderTask* task = new ShowMsg(message);
    renderer_.submit_task(task);
}

void GameCtrl::RenderInitInterface() {
    if (players_.empty()) {
        LOG_ERROR("RenderInitInterface called with no players available.");
        return;
    }

    int user_cursor_x = human_.GetCursorPosition().first;
    int user_cursor_y = human_.GetCursorPosition().second;
    RenderTask* task = new InitGameInterface(
        players_[0].level,
        players_[0].name,
        user_cursor_x,
        user_cursor_y,
        admin_.get_map().getSnapshot());
    renderer_.submit_task(task);
}
/*
bool GameCtrl::IsValidCoordinate(int x, int y) const {
    return (x >= 0 && x < admin_.get_map().getWidth() && y >= 0 && y < admin_.get_map().getHeight());
}*/