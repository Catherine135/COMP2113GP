#pragma once
#include "Map.h"
#include <vector>
#include <string>
#include <utility>

struct RenderTask{ virtual ~RenderTask() = default; };
/// @brief Selected a tile on the map.
struct SelectTile : public RenderTask {
    std::pair<int, int> coords; // (x, y) coordinates of the selected tile
    bool select{true}; // true for select, false for deselect
    SelectTile(std::pair<int,int> c, bool s=true) : coords(c), select(s) {}
};

/// @brief Draw an arrow from one point to another on the map.
struct Arrow : public RenderTask {
    std::pair<int, int> from; // (x, y) coordinates of the start point
    std::pair<int, int> to;   // (x, y) coordinates of the end point
    bool draw{true}; // Whether to draw or clear the arrow
    Arrow(std::pair<int,int> f, std::pair<int,int> t, bool d=true) : from(f), to(t), draw(d) {}
}; 

/// @brief Refresh the entire map display.
struct RefreshMap : public RenderTask {
    std::vector<std::vector<Tile>> snapshot; // Snapshot of the current map state
    explicit RefreshMap(std::vector<std::vector<Tile>> s) : snapshot(std::move(s)) {}
}; 

struct UpdateTile : public RenderTask {
    std::pair<int, int> coords; // (x, y) coordinates of the tile to update
    Tile tile; // New state of the tile
    UpdateTile(std::pair<int,int> c, Tile t) : coords(c), tile(t) {}
};

struct ShowMsg : public RenderTask {
    std::string msg; // Message to display
    explicit ShowMsg(std::string m) : msg(std::move(m)) {}
};