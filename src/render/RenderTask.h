#pragma once
#include "Map.h"
#include <vector>

struct RenderTask{virtual ~RenderTask() = default; // safe polymorphic base
};

/// @brief Selected a tile on the map.
///         New parameter added! (snapshot of map, because need to get the tile info)
struct SelectTile : public RenderTask {
    //std::pair<int, int> coords; // (x, y) coordinates of the selected tile
    bool select{true}; // true for select, false for deselect
    std::vector<std::vector<Tile>> snapshot;
    SelectTile(/*std::pair<int, int> c,*/ bool s, std::vector<std::vector<Tile>> snap) : /*coords(c),*/ select(s), snapshot(snap) {}
};

/// @brief Draw an arrow from one point to another on the map.
struct Arrow : public RenderTask {
    std::pair<int, int> from; // (x, y) coordinates of the start point
    std::pair<int, int> to;   // (x, y) coordinates of the end point
    bool draw{true}; // Whether to draw or clear the arrow
    bool isHuman{true}; // true for human arrow, false for AI arrow
    std::vector<std::vector<Tile>> snapshot;
    
    Arrow(std::pair<int, int> f, std::pair<int, int> t, bool d, bool isH, std::vector<std::vector<Tile>> snap) 
        : from(f), to(t), draw(d), isHuman(isH), snapshot(snap) {}
}; 

/// @brief MoveCursor to another on the map.
struct MoveCursor : public RenderTask {
    //std::pair<int, int> from; // (x, y) coordinates of the start point
    std::pair<int, int> to;   // (x, y) coordinates of the end point
    std::vector<std::vector<Tile>> snapshot;
    
    MoveCursor(/*std::pair<int, int> f, */std::pair<int, int> t, std::vector<std::vector<Tile>> snap) 
        : /*from(f), */to(t), snapshot(snap) {}
}; 

/// @brief Refresh the entire map display.
struct RefreshMapInterface : public RenderTask {
    std::vector<std::vector<Tile>> snapshot; // Snapshot of the current map state
    
    RefreshMapInterface(std::vector<std::vector<Tile>> snap) : snapshot(snap) {}
}; 

/// @brief Update specific tiles
struct UpdateTile : public RenderTask {
    std::pair<int, int> coords; // (x, y) coordinates of the tile to update
    Tile tile; // New state of the tile
    std::vector<std::vector<Tile>> snapshot;
    UpdateTile(std::pair<int, int> c, Tile t, std::vector<std::vector<Tile>> snap) : coords(c), tile(t), snapshot(snap) {}
};

/// @brief Display messgae
//          What message to be shown? If game status, need to get the map snapshot parameter and the turns parameter.
//          I have added an area for displaying game status info, but specific info (turns, land, army) are yet added.
struct ShowMsg : public RenderTask {
    std::string msg; // Message to display
    
    ShowMsg(std::string m) : msg(m) {}
};

//prints out the interface for Bulletin Board
struct BulletinBoardInterface : public RenderTask{
    int turns;
    int user_land;
    int user_army; 
    int ai_land;
    int ai_army;
    std::string msg;

    BulletinBoardInterface(int ts, int ul, int ua, int al, int aa, std::string m):
        turns(ts), user_land(ul), user_army(ua), ai_land(al), ai_army(aa), msg(m) {}
};

//prints out the init game interface
struct InitGameInterface : public RenderTask{
    int level;
    std::string username;
    int user_cursor_x;
    int user_cursor_y;
    std::vector<std::vector<Tile>> snapshot;

    InitGameInterface(int lvl, std::string user, int cursor_x, int cursor_y, std::vector<std::vector<Tile>> snap) 
    : level(lvl), username(user), user_cursor_x(cursor_x), user_cursor_y(cursor_y), snapshot(snap) {}
};

//prints out the start game interface
struct StartGameInterface : public RenderTask{
    int level;
    std::string username;
    int user_cursor_x;
    int user_cursor_y;
    std::vector<std::vector<Tile>> snapshot;

    StartGameInterface(int lvl, std::string user, int cursor_x, int cursor_y, std::vector<std::vector<Tile>> snap) 
    : level(lvl), username(user), user_cursor_x(cursor_x), user_cursor_y(cursor_y), snapshot(snap) {}
};

// prints out the help interface
struct HelpInterface : public RenderTask{
    HelpInterface() = default;
};

//prints out the Menu interface
/*struct MenuInterface : public RenderTask{
    MenuInterface() = default;
};*/

//prints out the ConfirmExit interface
struct ConfirmExitInterface : public RenderTask{
    ConfirmExitInterface() = default;
};

//prints out the ConfirmReset interface
struct ConfirmResetInterface : public RenderTask{
    ConfirmResetInterface() = default;
};

//prints out the GameCleared interface
struct GameClearedInterface : public RenderTask{
    GameClearedInterface() = default;
};

/// prints out the interface when the game ends (pops out from the gaming interface)
//          Time elapsed info need to be changed! (currently: int timeElapsed)
struct EndGameInterface : public RenderTask{
    bool isWinning;
    int roundCount;

    EndGameInterface(bool i, int r): isWinning(i), roundCount(r) {}
};

/// @brief Display UpdateTurns info.
struct UpdateTurnsInterface : public RenderTask{
    int turns;
    int user_land;
    int user_army; 
    int ai_land;
    int ai_army;
    std::string msg;
    std::vector<std::vector<Tile>> snapshot;

    UpdateTurnsInterface(int ts, int ul, int ua, int al, int aa, std::string m, std::vector<std::vector<Tile>> snap):
        turns(ts), user_land(ul), user_army(ua), ai_land(al), ai_army(aa), msg(m), snapshot(snap) {}
};