#pragma once
#include "RenderTask.h"


/// @brief class ConsoleRenderer is responsible for processing rendering tasks
class ConsoleRenderer {
public:
    void render(const RenderTask& task);

private:
    void renderSelectTile(const SelectTile& task);
    void renderArrow(const Arrow& task);
    void renderMoveCursor(const MoveCursor& task);
    void renderRefreshMapInterface(const RefreshMapInterface& task);
    void renderUpdateTile(const UpdateTile& task);
    void renderShowMsg(const ShowMsg& task);

    void renderInitGameInterface(const InitGameInterface& task);
    void renderStartGameInterface(const StartGameInterface& task);
    void renderBulletinBoardInterface(const BulletinBoardInterface& task);
    //void renderMenuInterface(const MenuInterface& task);
    void renderConfirmExitInterface(const ConfirmExitInterface& task);
    void renderConfirmResetInterface(const ConfirmResetInterface& task);
    void renderGameClearedInterface (const GameClearedInterface& task);
    void renderEndGameInterface (const EndGameInterface& task);
    void renderUpdateTurnsInterface (const UpdateTurnsInterface& task);
    void renderHelpInterface (const HelpInterface& task);

    void renderGameLogo (int map_width);
    void renderHelp ();
    void renderMenu ();
    void renderRefreshMap(const std::vector<std::vector<Tile>>& snapshot, bool bInit=false);
    void renderBulletinBoard (int turns,
        int user_land, int user_army, 
        int ai_land, int ai_army,
        const std::string& msg);
    void updateHumanCursor (std::pair<int, int> to, bool bSelected, const std::vector<std::vector<Tile>>& snapshot);

    int level_{1};
    std::string username_;
    int map_width{0};
    int map_height{0};
    int user_cursor_x{0};
    int user_cursor_y{0};
    bool bSelected_{false};
};
