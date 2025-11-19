#pragma once
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
#include "RenderTask.h"
#include "ThreadSafeQueue.h"
#include "ConsoleRenderer.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { stop(); }               // RAII：auto stop
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void start() {
        if (worker.joinable()) return;
        running = true;
        worker = std::thread(&Renderer::loop, this);
    }
    void stop() {
        if (!worker.joinable()) return;
        running = false;
        inQ.stop(); // wake up pop
        worker.join();
    }
    void submit_task(RenderTask* rt) { inQ.push(rt); }

private:
    ThreadSafeQueue<RenderTask*> inQ;
    std::atomic<bool> running{false};
    std::thread worker;
    ConsoleRenderer cr_;

    //void print_snapshot(const std::vector<std::vector<Tile>>& snap);
    void loop();
};
