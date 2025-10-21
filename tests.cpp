#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include "Map.h"
#include "Action.h"
#include "ThreadSafeQueue.h"
#include "Admin.h"
#include "RenderTask.h"

// A simple null renderer that just drains tasks so queues don't grow.
class NullRenderer {
public:
    explicit NullRenderer(ThreadSafeQueue<RenderTask*>& q) : q(q) {}
    void start() { running = true; worker = std::thread([this]{ loop(); }); }
    void stop() { running = false; q.stop(); if (worker.joinable()) worker.join(); }
private:
    void loop() {
        using namespace std::chrono_literals;
        while (running) {
            RenderTask* t = q.wait_and_pop();
            if (!t) continue;
            delete t;
        }
    }
    ThreadSafeQueue<RenderTask*>& q;
    std::atomic<bool> running{false};
    std::thread worker;
};

struct TestResult { std::string name; bool pass; std::string info; };

static void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// Utility: pick a walkable neighbor of (x,y)
static bool pick_neighbor_walkable(Map& map, int x, int y, std::pair<int,int>& out) {
    std::vector<std::pair<int,int>> ns{{x+1,y},{x-1,y},{x,y+1},{x,y-1}};
    for (auto p: ns) {
        Tile t = map.getTile(p.first, p.second);
        if (p.first>=0 && p.first<map.getWidth() && p.second>=0 && p.second<map.getHeight() && !t.isMountain) {
            out = p; return true;
        }
    }
    return false;
}

TestResult test_neutral_capture() {
    TestResult r{"neutral_capture", false, ""};
    Map map(8,6,123);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq;
    NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq); 

    // Pre-buff capital army before admin thread starts to avoid race
    Tile cap = map.getTile(1,1); cap.army = 5; map.setTile(1,1, cap);

    admin.start();

    std::pair<int,int> dst;
    bool ok = pick_neighbor_walkable(map, 1,1, dst);
    if (!ok) { nr.stop(); admin.stop(); r.info = "no walkable neighbor"; return r; }

    Tile beforeDst = map.getTile(dst.first, dst.second);
    Move m; m.from={1,1}; m.to=dst; m.army=3; hq.push(m);
    sleep_ms(300);
    Tile afterDst = map.getTile(dst.first, dst.second);
    r.pass = (afterDst.owner == 0 && afterDst.army >= std::max(1, beforeDst.army));
    r.info = "dst owner=" + std::to_string(afterDst.owner) + ", army=" + std::to_string(afterDst.army);

    admin.stop(); nr.stop();
    return r;
}

TestResult test_merge_own() {
    TestResult r{"merge_own", false, ""};
    Map map(8,6,124);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    Tile cap = map.getTile(1,1); cap.army = 5; map.setTile(1,1, cap);
    admin.start();

    std::pair<int,int> dst; pick_neighbor_walkable(map,1,1,dst);
    Move m1; m1.from={1,1}; m1.to=dst; m1.army=2; hq.push(m1);
    sleep_ms(200);
    Tile d1 = map.getTile(dst.first, dst.second);
    Move m2; m2.from={1,1}; m2.to=dst; m2.army=2; hq.push(m2);
    sleep_ms(300);
    Tile d2 = map.getTile(dst.first, dst.second);

    r.pass = (d2.owner==0 && d2.army >= d1.army + 1);
    r.info = "before="+std::to_string(d1.army)+", after="+std::to_string(d2.army);

    admin.stop(); nr.stop();
    return r;
}

TestResult test_invalid_non_adjacent() {
    TestResult r{"invalid_non_adjacent", false, ""};
    Map map(8,6,125);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);
    Tile cap = map.getTile(1,1); cap.army=5; map.setTile(1,1,cap);
    admin.start();

    Tile destBefore = map.getTile(4,4);
    Move m; m.from={1,1}; m.to={4,4}; m.army=3; hq.push(m); // not adjacent
    sleep_ms(300);
    Tile destAfter = map.getTile(4,4);

    // 非法移动不应改变目标格
    r.pass = (destAfter.owner == destBefore.owner && destAfter.army == destBefore.army && destAfter.isMountain==destBefore.isMountain);
    r.info = "dest owner="+std::to_string(destAfter.owner)+", army="+std::to_string(destAfter.army);

    admin.stop(); nr.stop();
    return r;
}

TestResult test_attack_win() {
    TestResult r{"attack_win", false, ""};
    Map map(8,6,126);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    // 在 Admin 开始之前布置一个敌方相邻格 (2,1)
    Tile cap = map.getTile(1,1); cap.army=5; map.setTile(1,1,cap);
    Tile enemy = map.getTile(2,1); enemy.isMountain=false; enemy.owner=1; enemy.army=2; enemy.isCity=false; map.setTile(2,1,enemy);

    admin.start();
    Move m; m.from={1,1}; m.to={2,1}; m.army=4; hq.push(m);
    sleep_ms(300);
    Tile after = map.getTile(2,1);
    r.pass = (after.owner==0);
    r.info = "owner="+std::to_string(after.owner)+", army="+std::to_string(after.army);

    admin.stop(); nr.stop();
    return r;
}

TestResult test_attack_lose() {
    TestResult r{"attack_lose", false, ""};
    Map map(8,6,127);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    Tile cap = map.getTile(1,1); cap.army=4; map.setTile(1,1,cap);
    Tile enemy = map.getTile(2,1); enemy.isMountain=false; enemy.owner=1; enemy.army=5; enemy.isCity=false; map.setTile(2,1,enemy);

    admin.start();
    Move m; m.from={1,1}; m.to={2,1}; m.army=3; hq.push(m);
    sleep_ms(300);
    Tile after = map.getTile(2,1);
    r.pass = (after.owner==1 && after.army>=1);
    r.info = "owner="+std::to_string(after.owner)+", army="+std::to_string(after.army);

    admin.stop(); nr.stop();
    return r;
}

TestResult test_growth_per_round() {
    TestResult r{"growth_per_round", false, ""};
    Map map(8,6,300);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    // prepare: own a city near capital
    Tile cap = map.getTile(1,1); cap.army = 5; map.setTile(1,1,cap);
    Tile city = map.getTile(2,1); city.isMountain=false; city.isCity=true; city.owner=0; city.army=2; map.setTile(2,1,city);

    admin.start();
    sleep_ms(350); // wait > one round
    Tile cap2 = map.getTile(1,1);
    Tile city2 = map.getTile(2,1);
    r.pass = (cap2.army >= 6 && city2.army >= 3);
    r.info = "cap:"+std::to_string(cap2.army)+", city:"+std::to_string(city2.army);
    admin.stop(); nr.stop();
    return r;
}

TestResult test_growth_every_25_rounds() {
    TestResult r{"growth_every_25_rounds", false, ""};
    Map map(8,6,301);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    // Setup: capital, a city, and a plain owned tile
    Tile cap = map.getTile(1,1); cap.army = 1; map.setTile(1,1,cap);
    Tile city = map.getTile(2,1); city.isMountain=false; city.isCity=true; city.owner=0; city.army=1; map.setTile(2,1,city);
    Tile land = map.getTile(1,2); land.isMountain=false; land.owner=0; land.isCity=false; land.isCapital=false; land.army=1; map.setTile(1,2,land);

    admin.start();
    // Wait for a little over 25 rounds (25 * 300ms = 7500ms)
    sleep_ms(7800);
    Tile cap2 = map.getTile(1,1);
    Tile city2 = map.getTile(2,1);
    Tile land2 = map.getTile(1,2);
    // Expected deltas (given our implementation):
    // capital: +1 per round (25) + extra +2 at 25th -> >= 1 + 27 = 28
    // city: +1 per round (25) +1 at 25th -> >= 1 + 26 = 27
    // land: only +1 at 25th -> >= 1 + 1 = 2
    r.pass = (cap2.army >= 28 && city2.army >= 27 && land2.army >= 2);
    r.info = "cap:"+std::to_string(cap2.army)+", city:"+std::to_string(city2.army)+", land:"+std::to_string(land2.army);
    admin.stop(); nr.stop();
    return r;
}
TestResult test_capital_transfer() {
    TestResult r{"capital_transfer", false, ""};
    Map map(8,6,200);
    ThreadSafeQueue<Move> hq, aiq; ThreadSafeQueue<RenderTask*> rq; NullRenderer nr(rq); nr.start();
    Admin admin(map, hq, aiq, rq);

    // Place enemy capital at (2,1) for deterministic test and enemy territory nearby
    // Override generator outcome around (2,1)
    Tile cap0 = map.getTile(1,1); cap0.army = 10; map.setTile(1,1,cap0);
    Tile cap1 = map.getTile(2,1); cap1.isMountain=false; cap1.isCity=true; cap1.isCapital=true; cap1.isOriginalCapital=true; cap1.owner=1; cap1.army=3; map.setTile(2,1,cap1);
    Tile enemyLand = map.getTile(3,1); enemyLand.isMountain=false; enemyLand.owner=1; enemyLand.army=2; map.setTile(3,1,enemyLand);

    admin.start();
    Move m; m.from={1,1}; m.to={2,1}; m.army=8; hq.push(m);
    sleep_ms(400);

    Tile cAfter = map.getTile(2,1);
    Tile transferred = map.getTile(3,1);
    r.pass = (cAfter.owner==0 && transferred.owner==0);
    r.info = "cap owner="+std::to_string(cAfter.owner)+", land owner="+std::to_string(transferred.owner);
    admin.stop(); nr.stop();
    return r;
}

int main(){
    std::vector<TestResult> results;
    results.push_back(test_neutral_capture());
    results.push_back(test_merge_own());
    results.push_back(test_invalid_non_adjacent());
    results.push_back(test_attack_win());
    results.push_back(test_attack_lose());
    results.push_back(test_capital_transfer());
    results.push_back(test_growth_per_round());
    results.push_back(test_growth_every_25_rounds());

    int pass=0; for (auto& t: results) { if (t.pass) ++pass; }
    std::cout << "\n==== TEST RESULTS ====\n";
    for (auto& t: results) {
        std::cout << (t.pass?"[PASS] ":"[FAIL] ") << t.name << " -- " << t.info << "\n";
    }
    std::cout << "Summary: " << pass << "/" << results.size() << " passed\n";
    return pass == (int)results.size() ? 0 : 1;
}
