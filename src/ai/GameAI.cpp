#include "GameAI.h"
#include "Logger.h"


void GameAI::loop() {
    int sleep_time = 2000 - level_ * 150; // seconds
    while (running) {
        if (play_state) {
            Move m; 
            int ret = handleByAI(m, level_);
            if (ret == 0) { // submit move to admin
                admin_.submit_ai_action(m);
                LOG_INFOF("AI submitted move: from (%d,%d) to (%d,%d) with army %d", 
                    m.from.first, m.from.second, m.to.first, m.to.second, m.army);
            } else {
                LOG_INFOF("AI decided not to move this turn.");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
}

// snap can be geted by admin_.get_map().getSnapshot()
int GameAI::handleByAI(Move& m, int game_level)
{
    (void)game_level; // currently unused
    std::mt19937 rng(std::random_device{}());
    using namespace std::chrono_literals;
    std::vector<std::vector<Tile>> snap = admin_.get_map().getSnapshot();
    int map_width = snap[0].size();
    int map_height = snap.size();

    std::vector<std::pair<int,int>> owned;
    for (int y=0; y<map_height; ++y) {
        for (int x=0; x<map_width; ++x) {
            Tile t = snap[y][x];
            if (t.owner == playerId && t.army > 1) 
                owned.emplace_back(x,y);
        }
    }
    
    if (!owned.empty()) {
        std::uniform_int_distribution<size_t> pick(0, owned.size()-1);
        auto from = owned[pick(rng)];
        // random neighbor
        std::vector<std::pair<int,int>> nbrs{
            {from.first+1, from.second}, {from.first-1, from.second},
            {from.first, from.second+1}, {from.first, from.second-1}
        };
        std::shuffle(nbrs.begin(), nbrs.end(), rng);
        for (auto to : nbrs) {
            if (to.first>=0 && to.first<map_width && to.second>=0 && to.second<map_height) {
                m.from = from; 
                m.to = to; 
                //m.army = 1 + (snap[from.second][from.first].army/2); // half army
                m.army = snap[m.from.second][m.from.first].army - 1; // all army but leave one behind
                return 0;
            }
        }
    }

    return -1;
}