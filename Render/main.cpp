#include "ConsoleRenderer.h"
#include "RenderTask.h"
#include "Map.h" 
 #include <iostream>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    
    std::vector<std::unique_ptr<RenderTask>> renderQueue;


    renderQueue.emplace_back(std::make_unique<StartInterface>());
    renderQueue.emplace_back(std::make_unique<DifficultyInterface>());


   
    renderQueue.emplace_back(std::make_unique<GameInterface>());

    renderQueue.emplace_back(std::make_unique<ShowMsg>("Here is the message display."));

    std::vector<std::vector<Tile>> map(5, std::vector<Tile>(5));
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            Tile& tile = map[y][x];

            tile.isMountain = (x == 1 && y == 1) || (x == 3 && y == 3);
            tile.isCity = (x == 2 && y == 2) || (x == 0 && y == 3);
            tile.isCapital = (x == 0 && y == 0) || (x == 4 && y == 4);
            tile.isQuagmire = (x == 4 && y == 0) || (x == 0 && y == 4);

            if (tile.isMountain) {
                tile.owner = -1;
                tile.army = 0;
            } else if (x + y < 3) {
                tile.owner = 1; 
                tile.army = 2 + (x + y);
            } else if (x + y > 6) {
                tile.owner = 0; 
                tile.army = 1 + (x * y) % 4;
            } else {
                tile.owner = -1;
                tile.army = tile.isCity ? 2 : 0;
            }
        }
    }
    renderQueue.emplace_back(std::make_unique<RefreshMap>(map));

    renderQueue.emplace_back(std::make_unique<PauseInterface>());
    renderQueue.emplace_back(std::make_unique<GameInterface>());
    renderQueue.emplace_back(std::make_unique<ShowMsg>("Here is the message display."));
    renderQueue.emplace_back(std::make_unique<RefreshMap>(map));


    renderQueue.emplace_back(std::make_unique<Arrow>(std::array<int,2>{0,0}, std::array<int,2>{1,0}, true, map));
    renderQueue.emplace_back(std::make_unique<Arrow>(std::array<int,2>{0,0}, std::array<int,2>{1,0}, false, map));

    Tile tile{0, 5};
    renderQueue.emplace_back(std::make_unique<UpdateTile>(std::array<int,2>{2,0}, tile));

    renderQueue.emplace_back(std::make_unique<SelectTile>(std::array<int,2>{0,1}, true, map));
    renderQueue.emplace_back(std::make_unique<SelectTile>(std::array<int,2>{0,1}, false, map));

    renderQueue.emplace_back(std::make_unique<EndGameInterface>(true, 200));
    renderQueue.emplace_back(std::make_unique<LeaderboardInterface>("easy.txt","medium.txt","hard.txt"));

    //hide cursor
    std::cout << "\033[?25l";
    
    ConsoleRenderer renderer;

    for (auto& task : renderQueue) {
    renderer.render(*task);
    std::this_thread::sleep_for(std::chrono::seconds(1));  // 模拟帧间隔
}
   
    //show cursor
    std::cout << "\033[?25h";
    return 0;
}

/*
 ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ██╗███████╗   ██╗ ██████╗ 
██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗██║██╔════╝   ██║██╔═══██╗
██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║██║███████╗   ██║██║   ██║
██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║██║╚════██║   ██║██║   ██║
╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║██║███████║██╗██║╚██████╔╝
 ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝╚═╝╚═╝ ╚═════╝ 
 */