#include "ConsoleRenderer.h"
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <fstream>
#include <sstream>
#include <string>

#define DFT "\033[0m"
#define RED_BG "\033[41m"
#define LIGHT_REG_BG "\033[101m"
#define GREEN_BG "\033[42m"
#define YELLOW_BG "\033[43m"
#define WHITE_BG "\033[47m"

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
    if (tile.isMountain) return '^';
    if (tile.isCity) return 'C';
    if (tile.isCapital) return '*';
    if (tile.isQuagmire) return '~';
    return '.';
}

/// @brief assign the rendering task to specific ConsoleRenderer functions
/// @param task the rendering task
void ConsoleRenderer::render(const RenderTask& task) {
    if (const auto* t = dynamic_cast<const SelectTile*>(&task)) {
        renderSelectTile(*t);
    } else if (const auto* t = dynamic_cast<const Arrow*>(&task)) {
        renderArrow(*t);
    } else if (const auto* t = dynamic_cast<const RefreshMap*>(&task)) {
        renderRefreshMap(*t);
    } else if (const auto* t = dynamic_cast<const UpdateTile*>(&task)) {
        renderUpdateTile(*t);
    } else if (const auto* t = dynamic_cast<const ShowMsg*>(&task)) {
        renderShowMsg(*t);
    } else if (const auto* t = dynamic_cast<const GameInterface*>(&task)) {
        renderGameInterface(*t);
    } else if (const auto* t = dynamic_cast<const StartInterface*>(&task)) {
        renderStartInterface(*t);
    } else if (const auto* t = dynamic_cast<const DifficultyInterface*>(&task)) {
        renderDifficultyInterface(*t);
    } else if (const auto* t = dynamic_cast<const PauseInterface*>(&task)){
        renderPauseInterface(*t);
    } else if (const auto* t = dynamic_cast<const EndGameInterface*>(&task)){
        renderEndGameInterface(*t);
    } else if (const auto* t = dynamic_cast<const LeaderboardInterface*>(&task)){
        renderLeaderboardInterface(*t);
    }
}

/// @brief highlight the selected tile into yellow, or turn the tile back to red if it is not selected
///        Given the rules of generals.io, only the player's tiles can be highlighted.
/// @param task the SelectTile rendering task
void ConsoleRenderer::renderSelectTile(const SelectTile& task) {
    int x = task.coords.first;
    int y = task.coords.second;
    Tile tile = task.snapshot[y][x];
    int row = 2 * y + 13;
    int col = x * 4 + 24 + 1;
    if (task.select){
        if (!tile.army){
            moveCursor(row,col);
            std::cout << YELLOW_BG << "  " << DFT;
        } else{
            moveCursor(row,col);
            std::cout << YELLOW_BG << std::setw(2) << tile.army << DFT << std::flush;
        }
    } else {
        if (!tile.army){
            moveCursor(row,col);
            std::cout << RED_BG << "  " << DFT;
        } else{
            moveCursor(row,col);
            std::cout << RED_BG << std::setw(2) << tile.army << DFT << std::flush;
        }
    }
    

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief draw an arrow between adjacent tiles, or erase the arrow between adjacent tiles
///        Given the rules of generals.io, only arrows on the player's side is generated.
/// @param task the Arrow rendering task
void ConsoleRenderer::renderArrow(const Arrow& task) {
    int x0 = task.from.first;
    int y0 = task.from.second;
    int x1 = task.to.first;
    int y1 = task.to.second;

    Tile tile = task.snapshot[y0][x0];

    if ((x0 == x1) && (y0 - y1 == 1)){
        int row = 2 * y0 + 13 -1;
        int col = x0 * 4 + 24 + 1;
        moveCursor(row,col);
        if (task.draw){std::cout << "^";} 
        else {std::cout << " ";}
    }
    else if ((x0 == x1) && (y0 - y1 == -1)){
        int row = 2 * y0 + 13 + 1;
        int col = x0 * 4 + 24 + 1;
        moveCursor(row,col);
        if (task.draw){std::cout << "v";} 
        else {std::cout << " ";}
    }
    else if ((x0 - x1 == 1) && (y0 == y1)){
        int row = 2 * y0 + 13;
        int col = x0 * 4 + 24 - 1;
        moveCursor(row,col);
        if (task.draw){std::cout  << "<";} 
        else {std::cout << " ";}
    }
    else if ((x0 - x1 == -1) && (y0 == y1)){
        int row = 2 * y0 + 13;
        int col = x0 * 4 + 24 +3;
        moveCursor(row,col);
        if (task.draw){std::cout << ">";} 
        else {std::cout << " ";}
    }

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief update the whole game board (map)
/// @param task the RefreshMap rendering task
void ConsoleRenderer::renderRefreshMap(const RefreshMap& task) {    
    for (int y = 0; y < task.snapshot.size(); y++) {
        for (int x = 0; x < task.snapshot[y].size(); x++) {
            int row = 2 * y + 13;
            int col = x * 4 + 24;
            Tile tile = task.snapshot[y][x];
            
            moveCursor(row, col);
            if (tile.isMountain){std::cout << WHITE_BG << getTileSymbol(tile) << "  ";}
            else if (tile.army){
                if (tile.owner == 1) {std::cout << RED_BG << getTileSymbol(tile) << std::setw(2) << tile.army;}
                else if (tile.owner == 0){std::cout << GREEN_BG << getTileSymbol(tile) << std::setw(2) << tile.army; }
                else {std::cout << WHITE_BG << getTileSymbol(tile) << std::setw(2) << tile.army; }
            } else {
                std::cout << WHITE_BG << getTileSymbol(tile) << "  ";
            }

            std::cout << DFT;
        }
    }

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief update a certain file
/// @param task the UpdateTile rendering task
void ConsoleRenderer::renderUpdateTile(const UpdateTile& task) {
    int x = task.coords.first;
    int y = task.coords.second;
    int row = 2 * y + 13;
    int col = x * 4 + 24;

    moveCursor(row, col);
    if (task.tile.isMountain){std::cout << WHITE_BG << getTileSymbol(task.tile) << "  ";}
    else if (task.tile.army){
        if (task.tile.owner == 1) {std::cout << RED_BG << getTileSymbol(task.tile) << std::setw(2) << task.tile.army;}
        else if (task.tile.owner == 0){std::cout << GREEN_BG << getTileSymbol(task.tile) << std::setw(2) << task.tile.army; }
        else {std::cout << WHITE_BG << getTileSymbol(task.tile) << std::setw(2) << task.tile.army; }
    } else {
        std::cout << WHITE_BG << getTileSymbol(task.tile) << "  ";
    }
    std::cout <<DFT;
    
    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the message of the game (turns, players, land, army, or some other messages)
/// @param task the ShowMsg rendering task
void ConsoleRenderer::renderShowMsg(const ShowMsg& task) {
    moveCursor(12,72);
    std::cout << WHITE_BG << "\033[34m" << "TURNS: ";
    moveCursor(14,72);
    std::cout <<"PLAYER" << std::setw(12) << "LAND" << std::setw(12) << "ARMY";
    std::cout << DFT;
    moveCursor(15,72);
    std::cout << "YOU";
    moveCursor(16,72);
    std::cout << "AI";
    moveCursor(17,72);
    std::cout << WHITE_BG << "\033[34m" << "Messages" << DFT;
    moveCursor(18,72);
    std::cout << task.msg;

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the gaming page
/// @param task the GameInterface rendering task
void ConsoleRenderer::renderGameInterface (const GameInterface& task){
    std::cout << "\033c";

    moveCursor(3,10);
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ ";
    moveCursor(4,10);
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗";
    moveCursor(5,10);
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║";
    moveCursor(6,10);
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║";
    moveCursor(7,10);
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝";
    moveCursor(8,10);
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ ";

    moveCursor(11,7);
    std::cout << "=========================== MAP ===========================";

    moveCursor(11,72);
    std::cout << "======== NOTICE BOARD ========";
    
   
    moveCursor(21,72);
    std::cout << WHITE_BG << "Press \"P\" to pause game.  ";
    moveCursor(22,72);
    std::cout << WHITE_BG << "Press \"R\" to resume game. ";
    std::cout << DFT;
    moveCursor(23,72);
    std::cout << "==============================";

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the starting page
/// @param task the StartInterface rendering task
void ConsoleRenderer::renderStartInterface (const StartInterface& task){
    std::cout << "\033c";
    moveCursor(3,10);
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ ";
    moveCursor(4,10);
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗";
    moveCursor(5,10);
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║";
    moveCursor(6,10);
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║";
    moveCursor(7,10);
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝";
    moveCursor(8,10);
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ ";

    moveCursor(13,23);
    std::cout << WHITE_BG << "\033[31m" << " Welcome to generAIs.io, a human vs AI board game!! ";
    moveCursor(17,37);
    std::cout << WHITE_BG << "\033[34m" << " START        (PRESS 1) ";
    moveCursor(19,37);
    std::cout << WHITE_BG << "\033[34m" << " LEADERBOARD  (PRESS 2) ";
    moveCursor(21,37);
    std::cout << WHITE_BG << "\033[34m" << " EXIT         (PRESS 3) ";

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the difficulty selection page
/// @param task the DifficultyInterface rendering task
void ConsoleRenderer::renderDifficultyInterface (const DifficultyInterface& task){
     std::cout << "\033c";
    moveCursor(3,10);
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ ";
    moveCursor(4,10);
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗";
    moveCursor(5,10);
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║";
    moveCursor(6,10);
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║";
    moveCursor(7,10);
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝";
    moveCursor(8,10);
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ ";

    moveCursor(13,28);
    std::cout << WHITE_BG << "\033[31m" << " Please choose the difficulty level: ";
    moveCursor(17,37);
    std::cout << WHITE_BG << "\033[34m" << " EASY    (PRESS 1) ";
    moveCursor(19,37);
    std::cout << WHITE_BG << "\033[34m" << " MEDIUM  (PRESS 2) ";
    moveCursor(21,37);
    std::cout << WHITE_BG << "\033[34m" << " HARD    (PRESS 3) ";

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the pause page
/// @param task the PauseInterface rendering task
void ConsoleRenderer::renderPauseInterface (const PauseInterface& task){
    std::cout << "\033c";

    moveCursor(3,10);
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ ";
    moveCursor(4,10);
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗";
    moveCursor(5,10);
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║";
    moveCursor(6,10);
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║";
    moveCursor(7,10);
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝";
    moveCursor(8,10);
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ ";

    moveCursor(14,30);
    std::cout << "------------------------------";
    moveCursor(15,30);
    std::cout << "|                            |";
    moveCursor(16,30);
    std::cout << "| Press \"P\" to resume game |";
    moveCursor(17,30);
    std::cout << "|                            |";
    moveCursor(18,30);
    std::cout << "| Press \"Q\" to exit game   |";
    moveCursor(19,30);
    std::cout << "|                            |";
    moveCursor(20,30);
    std::cout << "------------------------------";

    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the ending message when a round of game ends
/// @param task the EndGameInterface rendering task
void ConsoleRenderer::renderEndGameInterface (const EndGameInterface& task){
    moveCursor(15, 30);
    std::cout << WHITE_BG << "\033[46m" << "------------------------------";
    moveCursor(16,30);
    std::cout << "|                            |";
    moveCursor(17,30);
    if (task.isWinning){
        std::cout << "|          You win!          |";
    } else{
        std::cout << "|         You lose...        |";
    }
    moveCursor(18,30);
    std::cout << "| Time elapsed:" << std::setw(5) << task.timeElapsed << " seconds |";
    moveCursor(19,30);
    std::cout << "|     Press \"E\" to exit.     |";
    moveCursor(20,30);
    std::cout << "|                            |";
    moveCursor(21,30);
    std::cout << "------------------------------";

    std::cout << DFT;
    moveCursor(40,0);
    std::cout.flush();
}

/// @brief shows the leaderboard
/// @param task the LeaderboardInterface rendering task
void ConsoleRenderer::renderLeaderboardInterface(const LeaderboardInterface& task){
    std::cout << "\033c";

    moveCursor(3,10);
    std::cout << " ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ ";
    moveCursor(4,10);
    std::cout << "██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗";
    moveCursor(5,10);
    std::cout << "██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║";
    moveCursor(6,10);
    std::cout << "██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║";
    moveCursor(7,10);
    std::cout << "╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝";
    moveCursor(8,10);
    std::cout << " ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ ";

    moveCursor(12,6);
    std::cout << WHITE_BG << "\033[34m" << "EASY" << DFT;
    moveCursor(13,6);
    std::cout << "-----------------------------------------------------------------------------";
    std::ifstream fin1(task.fileNameEasy);
    std::string line,name,time;
    for (int i = 0; i < 3; i++){
        std::getline(fin1,line);
        std::istringstream iss(line);
        iss >> name;iss >> time;
        moveCursor(14+i,6);
        std::cout << name; 
        moveCursor(14+i,45);
        std::cout << time << " seconds";
    }
    fin1.close();

    moveCursor(18,6);
    std::cout << WHITE_BG << "\033[34m" << "MEDIUM" << DFT;
    moveCursor(19,6);
    std::cout << "-----------------------------------------------------------------------------";
    std::ifstream fin2(task.fileNameMedium);
    for (int i = 0; i < 3; i++){
        std::getline(fin2,line);
        std::istringstream iss(line);
        iss >> name;iss >> time;
        moveCursor(20+i,6);
        std::cout << name;
        moveCursor(20+i,45);
        std::cout << time << " seconds";
    }
    fin2.close();

    moveCursor(24,6);
    std::cout << WHITE_BG << "\033[34m" << "MEDIUM" << DFT;
    moveCursor(25,6);
    std::cout << "-----------------------------------------------------------------------------";
    std::ifstream fin3(task.fileNameMedium);
    for (int i = 0; i < 3; i++){
        std::getline(fin3,line);
        std::istringstream iss(line);
        iss >> name;iss >> time;
        moveCursor(26+i,6);
        std::cout << name;
        moveCursor(26+i,45);
        std::cout << time << " seconds";
    }
    fin3.close();

    moveCursor(30,6);
    std::cout << WHITE_BG << "\033[31m" << "Press \"E\" to exit";

    moveCursor(40,0);
    std::cout.flush();
}