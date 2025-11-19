#pragma once
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
#include "RenderTask.h"
#include "ThreadSafeQueue.h"
#include "ConsoleRenderer.h"

/**
 * @GHAccC
 * @brief The Renderer class manages rendering tasks and delegates them to the ConsoleRenderer.
 */
class Renderer {
public:
    /**
     * @GHAccC
     * @brief Constructor and Destructor for Renderer.
     */
    Renderer() = default;
    ~Renderer() { stop(); }              
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * @GHAccC
     * @brief Start the renderer processing loop in a separate thread.
     */
    void start() {
        if (worker.joinable()) return;
        running = true;
        worker = std::thread(&Renderer::loop, this);
    }

    /**
     * @GHAccC
     * @brief Stop the renderer processing and join the thread.
     */
    void stop() {
        if (!worker.joinable()) return;
        running = false;
        inQ.stop(); // wake up pop
        worker.join();
    }

    /**
     * @GHAccC
     * @brief Submit a rendering task to the renderer.
     * @param rt Pointer to the RenderTask to submit.
     */
    void submit_task(RenderTask* rt) { inQ.push(rt); }

private:
    ThreadSafeQueue<RenderTask*> inQ;
    std::atomic<bool> running{false};
    std::thread worker;
    ConsoleRenderer cr_;

    //void print_snapshot(const std::vector<std::vector<Tile>>& snap);
    void loop();
};
