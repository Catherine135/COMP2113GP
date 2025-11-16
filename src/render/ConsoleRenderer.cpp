#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>
#include <sstream>
#include <string>

#include "ConsoleRenderer.h"
#include "Logger.h"

#define SHOW_AI_TILES true /* show AI tiles */

#define DFT "\033[0m" /* reset */
#define RED_BG "\033[41m" /* red background */
#define GREEN_BG "\033[42m" /* green background */
#define YELLOW_BG "\033[43m" /* yellow background */
#define CYAN_BG "\033[46m" /* cyan background */
#define WHITE_BG "\033[47m" /* white background */
#define RED "\033[31m" /* red */
#define BLUE "\033[34m" /* blue */
#define BLACK "\033[30m" /* black */
#define CLEAR "\033c"
#define HUMAN_ARROW "\033[31m" /* red */
#define HUMAN_RED "\033[31m" /* red */
#define AI_ARROW "\033[32m" /* green */
#define AI_GREEN "\033[32m" /* green */
#define HUMAN_BG "\033[41m" /* red background */
#define AI_BG "\033[42m" /* green background */
#define HUMAN_CURSOR_SHOW "\033[34m" /* blue */
#define HUMAN_CURSOR_SELECT "\033[31m" /* red */

#define BB_NAME_WIDTH 12
#define BB_MSG_WIDTH 40
#define LAND_WIDTH 5
#define LAND_HEIGHT 2
#define MENU_WIDTH 20
#define SPACE_WIDTH 10
#define LOGO_LOC_X() (3)
#define LOGO_LOC_Y(map_width) (MENU_WIDTH + SPACE_WIDTH + map_width/2 - 38)
#define BULLETIN_BOARD_LOC_X() (11)
#define BULLETIN_BOARD_LOC_Y(map_width) (MENU_WIDTH + SPACE_WIDTH + map_width + SPACE_WIDTH)
#define MAP_LOC_X() (13)
#define MAP_LOC_Y() (MENU_WIDTH + SPACE_WIDTH)
#define MENU_LOC_X() (11)
#define MENU_LOC_Y() (0)
#define WELCOME_LOC_X() (9)
#define WELCOME_LOC_Y(map_width) (MENU_WIDTH + SPACE_WIDTH + map_width/2 - 38)
#define HELP_LOC_X(map_height) (11 + map_height + 3)
#define HELP_LOC_Y() (MENU_WIDTH + SPACE_WIDTH)
#define USER_INPUT_LOC_X(map_height) (11 + map_height + 2)
#define USER_INPUT_LOC_Y() (0)
#define INFO_LOC_X(map_height) (11 + map_height + 2)
#define INFO_LOC_Y() (MENU_WIDTH + SPACE_WIDTH)
#define WIN_MESSAGE_LOC_X() (13)
#define WIN_MESSAGE_LOC_Y(map_width) (MENU_WIDTH + SPACE_WIDTH + map_width/2 - 20)

/// @brief moves the cursor to the assigned position
/// @param row row position for the cursor to move to
/// @param col column position for the cursor to move to
void moveCursor(int row, int col){
    std::cout << "\033[" << row << ";" << col << "H";
}

/// @brief return the type of the target tile (capital, city, mountain, quagmire,none)
/// @param tile the target tile to get the symbol
/// @return a character representing the type of the tile (capital, city, mountain, quagmire,none)
char getTileSymbol(const Tile& tile) {
    if (tile.isMountain()) return '^';
    if (tile.isCapital()) return 'C';
    if (tile.isCity()) return 'c';
    //if (tile.isOriginalCapital) return '~';//if (tile.isQuagmire) return '~';
    return '.';
}

/// @brief assign the rendering task to specific ConsoleRenderer functions
/// @param task the rendering task
void ConsoleRenderer::render(const RenderTask& task) {
    if (const auto* t = dynamic_cast<const SelectTile*>(&task)) {
    LOG_INFO("renderSelectTile: Received a rendering task.");
        renderSelectTile(*t);
    } else if (const auto* t = dynamic_cast<const Arrow*>(&task)) {
    LOG_INFO("renderArrow: Received a rendering task.");
        renderArrow(*t);
    } else if (const auto* t = dynamic_cast<const MoveCursor*>(&task)) {
    LOG_INFO("renderMoveCursor: Received a rendering task.");
        renderMoveCursor(*t);
    } else if (const auto* t = dynamic_cast<const RefreshMapInterface*>(&task)) {
    LOG_INFO("renderRefreshMapInterface: Received a rendering task.");
        renderRefreshMapInterface(*t);
    } else if (const auto* t = dynamic_cast<const UpdateTile*>(&task)) {
    LOG_INFO("renderUpdateTile: Received a rendering task.");
        renderUpdateTile(*t);
    } else if (const auto* t = dynamic_cast<const ShowMsg*>(&task)) {
    LOG_INFO("renderShowMsg: Received a rendering task.");
        renderShowMsg(*t);
    } else if (const auto* t = dynamic_cast<const BulletinBoardInterface*>(&task)) {
    LOG_INFO("renderBulletinBoardInterface: Received a rendering task.");
        renderBulletinBoardInterface(*t);
    } else if (const auto* t = dynamic_cast<const InitGameInterface*>(&task)) {
    LOG_INFO("renderInitGameInterface: Received a rendering task.");
        renderInitGameInterface(*t);
    } else if (const auto* t = dynamic_cast<const StartGameInterface*>(&task)) {
    LOG_INFO("renderStartGameInterface: Received a rendering task.");
        renderStartGameInterface(*t);
    } else if (const auto* t = dynamic_cast<const HelpInterface*>(&task)) {
    LOG_INFO("renderHelpInterface: Received a rendering task.");
        renderHelpInterface(*t);
    } /*else if (const auto* t = dynamic_cast<const MenuInterface*>(&task)){
    LOG_INFO("renderMenuInterface: Received a rendering task.");
        renderMenuInterface(*t);
    } */else if (const auto* t = dynamic_cast<const ConfirmExitInterface*>(&task)){
    LOG_INFO("renderConfirmExitInterface: Received a rendering task.");
        renderConfirmExitInterface(*t);
    } else if (const auto* t = dynamic_cast<const ConfirmResetInterface*>(&task)){
    LOG_INFO("renderConfirmResetInterface: Received a rendering task.");
        renderConfirmResetInterface(*t);
    } /*else if (const auto* t = dynamic_cast<const DifficultyInterface*>(&task)) {
        renderDifficultyInterface(*t);
    }*/ else if (const auto* t = dynamic_cast<const GameClearedInterface*>(&task)){
        renderGameClearedInterface(*t);
    } else if (const auto* t = dynamic_cast<const EndGameInterface*>(&task)){
    LOG_INFO("renderEndGameInterface: Received a rendering task.");
        renderEndGameInterface(*t);
    } else if (const auto* t = dynamic_cast<const UpdateTurnsInterface*>(&task)){
    LOG_INFO("renderUpdateTurnsInterface: Received a rendering task.");
        renderUpdateTurnsInterface(*t);
    }

    // finally, move cursor to user input location
    std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();
}

/// @brief highlight the selected tile into yellow, or turn the tile back to red if it is not selected
///        Given the rules of generals.io, only the player's tiles can be highlighted.
/// @param task the SelectTile rendering task
void ConsoleRenderer::renderSelectTile(const SelectTile& task) {
    std::string cursor_color;
    bSelected_ = task.select;
    if (task.select){cursor_color = HUMAN_CURSOR_SELECT;}
    else {cursor_color = HUMAN_CURSOR_SHOW;} 

    // show new cursor
    int row = LAND_HEIGHT * static_cast<int>(user_cursor_y) + MAP_LOC_X();
    int col = static_cast<int>(user_cursor_x) * LAND_WIDTH + MAP_LOC_Y();
    moveCursor(row-1, col-1); // left top corner
    std::cout << cursor_color << "┌" << DFT;
    moveCursor(row-1, col+LAND_WIDTH-1); // right top corner
    std::cout << cursor_color << "┐" << DFT;
    moveCursor(row+1, col-1); // left bottom corner
    std::cout << cursor_color << "└" << DFT;
    moveCursor(row+1, col+LAND_WIDTH-1); // right bottom corner
    std::cout << cursor_color << "┘" << DFT;
}

/// @brief draw an arrow between adjacent tiles, or erase the arrow between adjacent tiles
///        Given the rules of generals.io, only arrows on the player's side is generated.
/// @param task the Arrow rendering task
void ConsoleRenderer::renderArrow(const Arrow& task) {
    int x0 = task.from.first;
    int y0 = task.from.second;
    int x1 = task.to.first;
    int y1 = task.to.second;

    LOG_INFOF("renderArrow: from (%d,%d) to (%d,%d), draw=%d, isHuman=%d", 
        x0, y0, x1, y1, task.draw, task.isHuman);

    //Tile tile = task.snapshot[y0][x0];
    std::string arrow_color;
    if (task.isHuman){arrow_color = HUMAN_ARROW;}
    else {arrow_color = AI_ARROW;} 

    if ((x0 == x1) && (y0 - y1 == 1)){
        int row = LAND_HEIGHT * y0 + MAP_LOC_X() - 1;
        int col = x0 * LAND_WIDTH + MAP_LOC_Y() + 1;
        moveCursor(row,col);
        if (task.draw){std::cout << arrow_color << "^";} 
        else {std::cout << " ";}
    }
    else if ((x0 == x1) && (y0 - y1 == -1)){
        int row = LAND_HEIGHT * y0 + MAP_LOC_X() + 1;
        int col = x0 * LAND_WIDTH + MAP_LOC_Y() + 1;
        moveCursor(row,col);
        if (task.draw){std::cout << arrow_color << "v";} 
        else {std::cout << " ";}
    }
    else if ((x0 - x1 == 1) && (y0 == y1)){
        int row = LAND_HEIGHT * y0 + MAP_LOC_X();
        int col = x0 * LAND_WIDTH + MAP_LOC_Y() - 1;
        moveCursor(row,col);
        if (task.draw){std::cout << arrow_color << "<";} 
        else {std::cout << " ";}
    }
    else if ((x0 - x1 == -1) && (y0 == y1)){
        int row = LAND_HEIGHT * y0 + MAP_LOC_X();
        int col = x0 * LAND_WIDTH + MAP_LOC_Y() + LAND_WIDTH - 2;//3;
        moveCursor(row,col);
        if (task.draw){std::cout << arrow_color << ">";} 
        else {std::cout << " ";}
    }

    // update user cursor position
    if (task.isHuman && task.draw) { // only human drawing
        updateHumanCursor(task.to, true, task.snapshot);
    }

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief MoveCursor the cursor to a new position
/// @param task the MoveCursor rendering task
void ConsoleRenderer::renderMoveCursor(const MoveCursor& task) {
    LOG_INFOF("renderMoveCursor: to (%d,%d)", task.to.first, task.to.second);

    // update user cursor position
    updateHumanCursor(task.to, false, task.snapshot);

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief update the whole game map
/// @param task the RefreshMap rendering task
void ConsoleRenderer::renderRefreshMapInterface(const RefreshMapInterface& task) {    
    renderRefreshMap(task.snapshot);

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

void ConsoleRenderer::renderRefreshMap(const std::vector<std::vector<Tile>>& snapshot, bool bInit) {  
    moveCursor(MAP_LOC_X()-2,MAP_LOC_Y()-2);

    // dynamic MAP header: left and right '=' counts symmetric
    int left_eq = 0;
    int right_eq = 0;
    if (map_width > 1) {
        left_eq = (map_width-1) / 2; // left side count as requested
        right_eq = (map_width-1) - left_eq;
    }
    std::string header = std::string(left_eq, '=') + " MAP " + std::string(right_eq, '=');
    std::cout << header;

    for (size_t y = 0; y < snapshot.size(); y++) {
        for (size_t x = 0; x < snapshot[y].size(); x++) {
            int row = LAND_HEIGHT * static_cast<int>(y) + MAP_LOC_X();
            int col = static_cast<int>(x) * LAND_WIDTH + MAP_LOC_Y();
            Tile tile = snapshot[y][x];
            
            // Decide whether to render this tile: always show mountains; otherwise show only if owner == 0 (human)
            // or adjacent (up/down/left/right) to a human-owned tile
            bool show = false;
            int h = static_cast<int>(snapshot.size());
            int w = h > 0 ? static_cast<int>(snapshot[0].size()) : 0;
            if (tile.isMountain()) show = true;
            else if (tile.owner == 0) show = true;
            else if (tile.owner == 1 && SHOW_AI_TILES) show = true;
            else {
                const int dx[4] = {1, -1, 0, 0};
                const int dy[4] = {0, 0, 1, -1};
                for (int i = 0; i < 4 && !show; ++i) {
                    int nx = static_cast<int>(x) + dx[i];
                    int ny = static_cast<int>(y) + dy[i];
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        if (snapshot[ny][nx].owner == 0) show = true;
                    }
                }
            }

            moveCursor(row, col);
            if (!show) {
                std::cout << WHITE_BG << ' ' << std::string(LAND_WIDTH-2, ' ');
            } else {
                if (tile.isMountain()) {
                    std::cout << WHITE_BG << BLACK << getTileSymbol(tile) << std::string(LAND_WIDTH-2, ' ');
                } 
                else if (bInit) {
                    std::cout << WHITE_BG << BLACK << std::string(LAND_WIDTH-1, ' ');
                }
                else if (tile.army){
                    // format army count: cap display at "99+" when >= 1000, right-align into LAND_WIDTH-2
                    std::string armyStr = (tile.army >= 1000) ? std::string("99+") : std::to_string(tile.army);
                    if ((int)armyStr.size() < LAND_WIDTH-2) armyStr = std::string(LAND_WIDTH-2 - armyStr.size(), ' ') + armyStr;
                    if (tile.owner == 0) { // human
                        std::cout << HUMAN_BG << BLACK << getTileSymbol(tile) << armyStr;
                    }
                    else if (tile.owner == 1){ // ai
                        std::cout << AI_BG << BLACK << getTileSymbol(tile) << armyStr; 
                    }
                    else { // neutral
                        std::cout << WHITE_BG << BLACK << getTileSymbol(tile) << armyStr; 
                    }
                } else {
                    std::cout << WHITE_BG << BLACK << getTileSymbol(tile) << std::string(LAND_WIDTH-2, ' ');
                }
            }

            std::cout << DFT;
        }
    }

    // render human cursor
    updateHumanCursor({user_cursor_x, user_cursor_y}, bSelected_, snapshot);
}

/// @brief update a certain file
/// @param task the UpdateTile rendering task
void ConsoleRenderer::renderUpdateTile(const UpdateTile& task) {
    int x = task.coords.first;
    int y = task.coords.second;
    int row = LAND_HEIGHT * y + MAP_LOC_X();
    int col = x * LAND_WIDTH + MAP_LOC_Y();
    // Decide whether to render this tile: always show mountains; otherwise show only if owner == 0 (human)
    // or adjacent (up/down/left/right) to a human-owned tile
    bool show = false;
    auto& snap = task.snapshot;
    int h = static_cast<int>(snap.size());
    int w = h > 0 ? static_cast<int>(snap[0].size()) : 0;
    if (task.tile.isMountain()) show = true;
    else if (task.tile.owner == 0) show = true;
    else {
        // check neighbors
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4 && !show; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                if (snap[ny][nx].owner == 0) show = true;
            }
        }
    }

    moveCursor(row, col);
    if (!show) {
        // render empty cell (preserve layout)
        std::cout << WHITE_BG << ' ' << std::string(LAND_WIDTH-2, ' ');
    } else {
    if (task.tile.isMountain()){std::cout << WHITE_BG << BLACK << getTileSymbol(task.tile) << std::string(LAND_WIDTH-2, ' ');} 
        else if (task.tile.army){
            // format army count: cap display at "99+" when >= 1000, right-align into LAND_WIDTH-2
            std::string armyStr = (task.tile.army >= 1000) ? std::string("99+") : std::to_string(task.tile.army);
            if ((int)armyStr.size() < LAND_WIDTH-2) armyStr = std::string(LAND_WIDTH-2 - armyStr.size(), ' ') + armyStr;
            if (task.tile.owner == 1) {std::cout << RED_BG << BLACK << getTileSymbol(task.tile) << armyStr;}
            else if (task.tile.owner == 0){std::cout << GREEN_BG << BLACK << getTileSymbol(task.tile) << armyStr; }
            else {std::cout << WHITE_BG << BLACK << getTileSymbol(task.tile) << armyStr; }
        } else {
            std::cout << WHITE_BG << BLACK << getTileSymbol(task.tile) << std::string(LAND_WIDTH-2, ' ');
        }
    }

    /*std::cout <<DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

void ConsoleRenderer::renderGameLogo (int map_width) {
    moveCursor(LOGO_LOC_X(),LOGO_LOC_Y(map_width));
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗    █████╗ ██╗";
    moveCursor(LOGO_LOC_X()+1,LOGO_LOC_Y(map_width));
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██╔══██╗██║";
    moveCursor(LOGO_LOC_X()+2,LOGO_LOC_Y(map_width));
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ███████║██║";
    moveCursor(LOGO_LOC_X()+3,LOGO_LOC_Y(map_width));
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██╔══██║██║";
    moveCursor(LOGO_LOC_X()+4,LOGO_LOC_Y(map_width));
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║  ██║██║";
    moveCursor(LOGO_LOC_X()+5,LOGO_LOC_Y(map_width));
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝  ╚═╝╚═╝";
    std::cout << DFT;
}

/// @brief shows the init game page (first page)
/// @param task the InitGameInterface rendering task
void ConsoleRenderer::renderInitGameInterface (const InitGameInterface& task){
    bSelected_ = false;
    level_ = task.level;
    username_ = task.username;
    map_width = static_cast<int>(task.snapshot[0].size()) * LAND_WIDTH;
    map_height = static_cast<int>(task.snapshot.size()) * LAND_HEIGHT;
    user_cursor_x = task.user_cursor_x;
    user_cursor_y = task.user_cursor_y;
    std::cout << CLEAR;
    
    renderGameLogo(map_width);
    moveCursor(WELCOME_LOC_X(),WELCOME_LOC_Y(map_width));
    std::cout << BLUE << " *********** Welcome to GENERALS.AI, a human vs AI board game ************** " << DFT;
    //std::vector<std::vector<Tile>> snapToRender;
    //int tiles_w = task.snapshot[0].size();
    //int tiles_h = task.snapshot.size();
    //snapToRender.assign(tiles_h, std::vector<Tile>(tiles_w));
    renderRefreshMap(task.snapshot, true);
    std::string s = "";
    renderBulletinBoard(0, 0, 0, 0, 0, s);
    renderMenu();
    renderHelp();

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief shows the start game page (first page)
/// @param task the StartGameInterface rendering task
void ConsoleRenderer::renderStartGameInterface (const StartGameInterface& task){
    bSelected_ = false;
    level_ = task.level;
    username_ = task.username;
    map_width = static_cast<int>(task.snapshot[0].size()) * LAND_WIDTH;
    map_height = static_cast<int>(task.snapshot.size()) * LAND_HEIGHT;
    user_cursor_x = task.user_cursor_x;
    user_cursor_y = task.user_cursor_y;
    std::cout << CLEAR;

    renderGameLogo(map_width);
    moveCursor(WELCOME_LOC_X(),WELCOME_LOC_Y(map_width));
    std::cout << BLUE << " *********** Welcome to GENERALS.AI, a human vs AI board game ************** " << DFT;
    renderRefreshMap(task.snapshot);
    std::string s = "";
    renderBulletinBoard(0, 0, 0, 0, 0, s);
    renderMenu();
    renderHelp();
    
    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief update the human cursor on the map
/// @param snapshot the current game map snapshot
void ConsoleRenderer::updateHumanCursor (std::pair<int, int> to, bool bSelected, const std::vector<std::vector<Tile>>& snapshot){
    Tile tile = snapshot[user_cursor_y][user_cursor_x];
    int x1 = static_cast<int>(to.first);
    int y1 = static_cast<int>(to.second);
    LOG_DEBUGF("user_cursor_x-%d,user_cursor_y-%d, row-%d, col-%d, tile.owner-%d, army-%d, landType-%d, to.x-%d, to.y-%d",
        user_cursor_x, user_cursor_y, tile.owner, tile.army, static_cast<int>(tile.landType), x1, y1);

    // erase cur cursor
    int row = LAND_HEIGHT * static_cast<int>(user_cursor_y) + MAP_LOC_X();
    int col = static_cast<int>(user_cursor_x) * LAND_WIDTH + MAP_LOC_Y();
    moveCursor(row-1, col-1); // left top corner
    std::cout << " " ;
    moveCursor(row-1, col+LAND_WIDTH-1); // right top corner
    std::cout << " " ;
    moveCursor(row+1, col-1); // left bottom corner
    std::cout << " " ;
    moveCursor(row+1, col+LAND_WIDTH-1); // right bottom corner
    std::cout << " " ;

    // update user cursor
    user_cursor_x = x1;
    user_cursor_y = y1;

    // show new cursor
    std::string cursor_color;
    if (bSelected){cursor_color = HUMAN_CURSOR_SELECT;}
    else {cursor_color = HUMAN_CURSOR_SHOW;} 
    row = LAND_HEIGHT * static_cast<int>(user_cursor_y) + MAP_LOC_X();
    col = static_cast<int>(user_cursor_x) * LAND_WIDTH + MAP_LOC_Y();
    moveCursor(row-1, col-1); // left top corner
    std::cout << cursor_color << "┌" << DFT;
    moveCursor(row-1, col+LAND_WIDTH-1); // right top corner
    std::cout << cursor_color << "┐" << DFT;
    moveCursor(row+1, col-1); // left bottom corner
    std::cout << cursor_color << "└" << DFT;
    moveCursor(row+1, col+LAND_WIDTH-1); // right bottom corner
    std::cout << cursor_color << "┘" << DFT;
 
    std::cout << DFT;
}

/// @brief shows the help page (first page)
/// @param task the HelpInterface rendering task
void ConsoleRenderer::renderHelpInterface (const HelpInterface& task){
    (void)task;
    //std::cout << CLEAR;

    //renderGameLogo ();
    renderHelp ();

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

void ConsoleRenderer::renderHelp (){
    moveCursor(HELP_LOC_X(map_height),HELP_LOC_Y());
    std::cout << /*WHITE_BG <<*/ DFT << "A/a - Left, D/d - Right, W/w - Up, S/s - Down";
    moveCursor(HELP_LOC_X(map_height)+1,HELP_LOC_Y());
    std::cout << /*WHITE_BG <<*/ DFT << "Space - Toggle Army Movement: On / Off";
    moveCursor(HELP_LOC_X(map_height)+2,HELP_LOC_Y());
    std::cout << /*WHITE_BG <<*/ DFT << "The level in this game is 1~6" << DFT;
}

/// @brief shows the Bulletin Board page
/// @param task the BulletinBoardInterface rendering task
void ConsoleRenderer::renderBulletinBoardInterface (const BulletinBoardInterface& task){
    //std::cout << CLEAR;

    //renderGameLogo();
    renderBulletinBoard(task.turns, task.user_land, task.user_army, task.ai_land, task.ai_army, task.msg);

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

void ConsoleRenderer::renderBulletinBoard (int turns,
    int user_land, int user_army, 
    int ai_land, int ai_army,
    const std::string& msg) {
    moveCursor(BULLETIN_BOARD_LOC_X(), BULLETIN_BOARD_LOC_Y(map_width));
    std::cout << "======== BULLETIN BOARD ========";
    moveCursor(BULLETIN_BOARD_LOC_X()+1, BULLETIN_BOARD_LOC_Y(map_width));
    std::cout << DFT << "TURNS: " << RED << turns << DFT;
    moveCursor(BULLETIN_BOARD_LOC_X()+2, BULLETIN_BOARD_LOC_Y(map_width));
    std::cout << DFT << "LEVEL: " << RED << level_ << DFT;

    moveCursor(BULLETIN_BOARD_LOC_X()+4, BULLETIN_BOARD_LOC_Y(map_width));
    std::cout <<"PLAYER" << std::setw(12) << "LAND" << std::setw(12) << "ARMY" << DFT;
    // print player name padded/truncated to BB_NAME_WIDTH
    {
        moveCursor(BULLETIN_BOARD_LOC_X()+5, BULLETIN_BOARD_LOC_Y(map_width));
        std::string out = username_.substr(0, BB_NAME_WIDTH);
        if ((int)out.size() < BB_NAME_WIDTH) out += std::string(BB_NAME_WIDTH - out.size(), ' ');
        std::cout << HUMAN_RED << out;
    }
    moveCursor(BULLETIN_BOARD_LOC_X()+5, BULLETIN_BOARD_LOC_Y(map_width)+14);
    std::cout << HUMAN_RED << user_land;
    moveCursor(BULLETIN_BOARD_LOC_X()+5, BULLETIN_BOARD_LOC_Y(map_width)+26);
    std::cout << HUMAN_RED << user_army << DFT;
    moveCursor(BULLETIN_BOARD_LOC_X()+6, BULLETIN_BOARD_LOC_Y(map_width));
    std::cout << AI_GREEN << "AI";
    moveCursor(BULLETIN_BOARD_LOC_X()+6, BULLETIN_BOARD_LOC_Y(map_width)+14);
    std::cout << AI_GREEN << ai_land;
    moveCursor(BULLETIN_BOARD_LOC_X()+6, BULLETIN_BOARD_LOC_Y(map_width)+26);
    std::cout << AI_GREEN << ai_army << DFT;
    if(msg!=""){
        // print a fixed-width message area: label on the left, message content padded/truncated
        moveCursor(BULLETIN_BOARD_LOC_X()+8, BULLETIN_BOARD_LOC_Y(map_width));
        std::cout << "Messages: " << DFT;
        moveCursor(BULLETIN_BOARD_LOC_X()+9, BULLETIN_BOARD_LOC_Y(map_width));
        std::string out = msg.substr(0, BB_MSG_WIDTH);
        if ((int)out.size() < BB_MSG_WIDTH) out += std::string(BB_MSG_WIDTH - out.size(), ' ');
        std::cout << out << DFT;
    }
}

/// @brief shows the message of the game
/// @param task the ShowMsg rendering task
void ConsoleRenderer::renderShowMsg(const ShowMsg& task) {
    moveCursor(BULLETIN_BOARD_LOC_X()+8, BULLETIN_BOARD_LOC_Y(map_width));
    std::cout << "Messages: " << DFT;
    {
        moveCursor(BULLETIN_BOARD_LOC_X()+9, BULLETIN_BOARD_LOC_Y(map_width));
        std::string out = task.msg.substr(0, BB_MSG_WIDTH);
        if ((int)out.size() < BB_MSG_WIDTH) out += std::string(BB_MSG_WIDTH - out.size(), ' ');
        std::cout << out << DFT;
    }

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}
/*
/// @brief shows the Menu page
/// @param task the MenuInterface task
void ConsoleRenderer::renderMenuInterface(const MenuInterface& task){
    (void)task;
    std::string name;
    std::cout << CLEAR;

    renderGameLogo ();
    renderMenu();

    std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();
}*/

void ConsoleRenderer::renderMenu(){
    moveCursor(MENU_LOC_X(),MENU_LOC_Y());
    std::cout << "======= MENU =======";
    moveCursor(MENU_LOC_X()+1,MENU_LOC_Y());
    std::cout << " G/g: Start";
    moveCursor(MENU_LOC_X()+2,MENU_LOC_Y());
    std::cout << " R/r: Restart";
    moveCursor(MENU_LOC_X()+3,MENU_LOC_Y());
    std::cout << " P/p: Pause/Resume";
    moveCursor(MENU_LOC_X()+4,MENU_LOC_Y());
    //std::cout << " H/h: Show/Hide Help";
    //moveCursor(MENU_LOC_X()+5,MENU_LOC_Y());
    std::cout << " Q/q: Quit" << DFT;
}

/// @brief shows the ConfirmExit page
/// @param task the ConfirmExitInterface rendering task
void ConsoleRenderer::renderConfirmExitInterface (const ConfirmExitInterface& task){
    (void)task;

    moveCursor(INFO_LOC_X(map_height), INFO_LOC_Y());
    std::cout << WHITE_BG << RED << " Are you sure you want to quit? (Y/N) " << DFT;

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief shows the ConfirmReset page
/// @param task the ConfirmResetInterface rendering task
void ConsoleRenderer::renderConfirmResetInterface (const ConfirmResetInterface& task){
    (void)task;

    moveCursor(INFO_LOC_X(map_height), INFO_LOC_Y());
    std::cout << WHITE_BG << RED << " Are you sure you want to restart? (Y/N) " << DFT;

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief shows the GameCleared page
/// @param task the GameClearedInterface rendering task
void ConsoleRenderer::renderGameClearedInterface (const GameClearedInterface& task){
    (void)task;
    moveCursor(WIN_MESSAGE_LOC_X(), WIN_MESSAGE_LOC_Y(map_width));
    std::cout << YELLOW_BG << " ----------------------------------------- ";
    moveCursor(WIN_MESSAGE_LOC_X()+1, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |               Good job!               | "; 
    moveCursor(WIN_MESSAGE_LOC_X()+2, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |Congratulations on completing the game!| ";
    moveCursor(WIN_MESSAGE_LOC_X()+3, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << YELLOW_BG << " ----------------------------------------- ";
    moveCursor(WIN_MESSAGE_LOC_X()+4, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |Press Q/q to exit!                     | ";
    moveCursor(WIN_MESSAGE_LOC_X()+5, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " ----------------------------------------- ";
}

/// @brief shows the ending message when a round of game ends
/// @param task the EndGameInterface rendering task
void ConsoleRenderer::renderEndGameInterface (const EndGameInterface& task){
    moveCursor(WIN_MESSAGE_LOC_X(), WIN_MESSAGE_LOC_Y(map_width));
    std::cout << YELLOW_BG << " ---------------------------------------- ";
    moveCursor(WIN_MESSAGE_LOC_X()+1, WIN_MESSAGE_LOC_Y(map_width));
    std::cout <<     " |              Good job!               | "; 
    moveCursor(WIN_MESSAGE_LOC_X()+2, WIN_MESSAGE_LOC_Y(map_width));
    if (task.isWinning){
        std::cout << " |              You win!                | ";
    } else{
        std::cout << " |              You lose...             | ";
    }
    moveCursor(WIN_MESSAGE_LOC_X()+3, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << YELLOW_BG << " ---------------------------------------- ";
    moveCursor(WIN_MESSAGE_LOC_X()+4, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |Round count:" << std::setw(5) << task.roundCount << "                     | ";
    moveCursor(WIN_MESSAGE_LOC_X()+5, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |Press Q/q to exit!                    | ";
    moveCursor(WIN_MESSAGE_LOC_X()+6, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " |Press R/r to start next level!        | ";
    moveCursor(WIN_MESSAGE_LOC_X()+7, WIN_MESSAGE_LOC_Y(map_width));
    std::cout << " ---------------------------------------- ";

    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}

/// @brief shows the UpdateTurns page
/// @param task the UpdateTurnsInterface rendering task
void ConsoleRenderer::renderUpdateTurnsInterface(const UpdateTurnsInterface& task){
    renderRefreshMap(task.snapshot);
    renderBulletinBoard(task.turns, task.user_land, task.user_army, task.ai_land, task.ai_army, task.msg);
    
    /*std::cout << DFT;
    moveCursor(USER_INPUT_LOC_X(map_height), USER_INPUT_LOC_Y());
    std::cout.flush();*/
}