#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& value) {
        if (discard) return;          // Discard mode, do not accept new tasks
        {
            std::lock_guard<std::mutex> lk(mtx);
            q.push(value);
        }
        cv.notify_one();
    }

    // Wait and pop; returns value by copy
    T wait_and_pop() {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [&]{ return !q.empty() || stopped; });
        if (q.empty() || discard) return T{}; // if stopped and empty
        T v = q.front();
        q.pop();
        return v;
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lk(mtx);
        if (q.empty() || discard) return false;
        out = q.front();
        q.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            stopped = true;
        }
        cv.notify_all();
    }

    void stop_now() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            discard = true; 
            std::queue<T> empty;
            std::swap(q, empty);
        }
        cv.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx);
        return q.empty();
    }

private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<T> q;
    bool stopped{false};
    bool discard{false};  
};
