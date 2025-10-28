#pragma once
#include "RenderTask.h"

/// @brief class ConsoleRenderer is responsible for processing rendering tasks
class ConsoleRenderer {
public:
void render(const RenderTask& task);

private:
    
    void renderSelectTile(const SelectTile& task);
    void renderArrow(const Arrow& task);
    void renderRefreshMap(const RefreshMap& task);
    void renderUpdateTile(const UpdateTile& task);
    void renderShowMsg(const ShowMsg& task);

    void renderGameInterface(const GameInterface& task);
    void renderStartInterface(const StartInterface& task);
    void renderDifficultyInterface(const DifficultyInterface& task);
    void renderPauseInterface (const PauseInterface& task);
    void renderEndGameInterface (const EndGameInterface& task);
    void renderLeaderboardInterface (const LeaderboardInterface& task);
    
};
