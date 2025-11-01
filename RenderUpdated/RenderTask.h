#pragma once
#include "Map.h"
#include <vector>

struct RenderTask{virtual ~RenderTask() = default; // safe polymorphic base
};

/// @brief Selected a tile on the map.
///         New parameter added! (snapshot of map, because need to get the tile info)
struct SelectTile : public RenderTask {
    std::pair<int, int> coords; // (x, y) coordinates of the selected tile
    bool select{true}; // true for select, false for deselect
    std::vector<std::vector<Tile>> snapshot;
    SelectTile(std::pair<int, int> c, bool s, std::vector<std::vector<Tile>> snap) : coords(c), select(s), snapshot(snap) {}
};

/// @brief Draw an arrow from one point to another on the map.
///         New parameter added! (snapshot of map, because need to get the tile info)
struct Arrow : public RenderTask {
    std::pair<int, int> from; // (x, y) coordinates of the start point
    std::pair<int, int> to;   // (x, y) coordinates of the end point
    bool draw{true}; // Whether to draw or clear the arrow
    std::vector<std::vector<Tile>> snapshot;
    
    Arrow(std::pair<int, int> f, std::pair<int, int> t, bool d, std::vector<std::vector<Tile>> snap) 
        : from(f), to(t), draw(d), snapshot(snap) {}
}; 

/// @brief Refresh the entire map display.
struct RefreshMap : public RenderTask {
    std::vector<std::vector<Tile>> snapshot; // Snapshot of the current map state
    
    RefreshMap(std::vector<std::vector<Tile>> snap) : snapshot(snap) {}
}; 

/// @brief Update specific tiles
struct UpdateTile : public RenderTask {
    std::pair<int, int> coords; // (x, y) coordinates of the tile to update
    Tile tile; // New state of the tile

    UpdateTile(std::pair<int, int> c, Tile t) : coords(c), tile(t) {}
};

/// @brief Display messgae
//          What message to be shown? If game status, need to get the map snapshot parameter and the turns parameter.
//          I have added an area for displaying game status info, but specific info (turns, land, army) are yet added.
struct ShowMsg : public RenderTask {
    std::string msg; // Message to display
    
    ShowMsg(std::string m) : msg(m) {}
};

//prints out the interface for gaming
struct GameInterface : public RenderTask{
    GameInterface() = default;
};

//prints out the starting interface
struct StartInterface : public RenderTask{
    StartInterface() = default;
};

//allow the user to set their name
struct NameInterface : public RenderTask{
    NameInterface() = default;
};

//prints out the difficulty selection interface
struct DifficultyInterface : public RenderTask{
    DifficultyInterface() = default;
};

//prints out the pausing interface
struct PauseInterface : public RenderTask{
    PauseInterface() = default;
};

/// prints out the interface when the game ends (pops out from the gaming interface)
//          Time elapsed info need to be changed! (currently: int timeElapsed)
struct EndGameInterface : public RenderTask{
    bool isWinning;
    int timeElapsed;

    EndGameInterface(bool i, int t): isWinning(i), timeElapsed(t) {}
};

/// @brief Display leaderboard info. NEED FILE I/O!!
struct LeaderboardInterface : public RenderTask{
    std::string fileNameEasy;
    std::string fileNameMedium;
    std::string fileNameHard;

    LeaderboardInterface(std::string fne, std::string fnm, std::string fnh): 
        fileNameEasy(fne), fileNameMedium(fnm),fileNameHard(fnh) {}
};