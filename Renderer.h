#pragma once
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
#include "RenderTask.h"
#include "ThreadSafeQueue.h"

class Renderer {
public:
    Renderer(ThreadSafeQueue<RenderTask*>& inQ) : inQ(inQ) {}
    void start() { running = true; worker = std::thread([this]{ loop(); }); }
    void stop() { running = false; if (worker.joinable()) worker.join(); }

private:
    ThreadSafeQueue<RenderTask*>& inQ;
    std::atomic<bool> running{false};
    std::thread worker;

    void print_snapshot(const std::vector<std::vector<Tile>>& snap) {
        std::cout << "\nMap:\n";
        for (size_t y = 0; y < snap.size(); ++y) {
            for (size_t x = 0; x < snap[y].size(); ++x) {
                const Tile& t = snap[y][x];
                char c = '.';
                if (t.isMountain) c = '#';
                else if (t.isCapital) c = (t.owner==0? 'H':'A');
                else if (t.isCity) c = 'C';
                else if (t.owner==0) c = 'h';
                else if (t.owner==1) c = 'a';
                std::cout << c;
            }
            std::cout << "\n";
        }
        std::cout.flush();
    }

    void loop() {
        while (running) {
            RenderTask* base = inQ.wait_and_pop();
            if (!running && base==nullptr) break;
            if (auto msg = dynamic_cast<ShowMsg*>(base)) {
                std::cout << "[MSG] " << msg->msg << "\n";
            } else if (auto upd = dynamic_cast<UpdateTile*>(base)) {
                // could print minimal info; ignore to keep output readable
            } else if (auto ref = dynamic_cast<RefreshMap*>(base)) {
                print_snapshot(ref->snapshot);
            } else if (auto arr = dynamic_cast<Arrow*>(base)) {
                std::cout << "[Arrow] (" << arr->from.first << "," << arr->from.second << ") -> ("
                          << arr->to.first << "," << arr->to.second << ")\n";
            }
            delete base; // free
        }
    }
};
