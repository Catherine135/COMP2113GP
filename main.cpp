#include <iostream>
#include <thread>
#include <chrono>
#include "Map.h"
#include "Action.h"
#include "ThreadSafeQueue.h"
#include "Admin.h"
#include "RandomAI.h"
#include "Renderer.h"

int main() {
    Map map(12, 8, 42);

    ThreadSafeQueue<Move> humanQ;
    ThreadSafeQueue<Move> aiQ;
    ThreadSafeQueue<RenderTask*> renderQ;

    // Preload some human actions: try to move from (1,1) capital outward
    for (int i=0;i<5;++i) {
        Move m; m.from={1,1}; m.to={2,1}; m.army=1; humanQ.push(m);
        Move m2; m2.from={1,1}; m2.to={1,2}; m2.army=1; humanQ.push(m2);
    }

    Renderer renderer(renderQ);
    renderer.start();

    Admin admin(map, humanQ, aiQ, renderQ);
    admin.start();

    RandomAI ai(map, aiQ, 1);
    ai.start();

    // Initial full render
    renderQ.push(new RefreshMap(map.getSnapshot()));

    std::this_thread::sleep_for(std::chrono::seconds(5));

    ai.stop();
    admin.stop();
    renderer.stop();

    std::cout << "\nDone.\n" << std::endl;
    return 0;
}
