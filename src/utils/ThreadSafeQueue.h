#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ThreadSafeQueue {
public:
    /**
     * @Michael-wzl
     * @brief Push a new element into the queue.
     * @param value The element to push into the queue.
     */
    void push(const T& value) {
        if (discard) return;          // Discard mode, do not accept new tasks
        {
            std::lock_guard<std::mutex> lk(mtx);
            q.push(value);
        }
        cv.notify_one();
    }

    /**
     * @Michael-wzl
     * @brief Wait and pop an element from the queue.
     * @return The popped element.
     */
    T wait_and_pop() {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [&]{ return !q.empty() || stopped; });
        if (q.empty() || discard) return T{}; // if stopped and empty
        T v = q.front();
        q.pop();
        return v;
    }

    /**
     * @Michael-wzl
     * @brief Try to pop an element from the queue without waiting.
     * @param out Reference to store the popped element.
     * @return True if an element was popped, false otherwise.
     */
    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lk(mtx);
        if (q.empty() || discard) return false;
        out = q.front();
        q.pop();
        return true;
    }

    /**
     * @Michael-wzl
     * @brief Stop the queue gracefully, allowing existing tasks to be processed.
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            stopped = true;
        }
        cv.notify_all();
    }

    /**
     * @Michael-wzl
     * @brief Stop the queue immediately, discarding all pending tasks.
     */
    void stop_now() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            discard = true; 
            std::queue<T> empty;
            std::swap(q, empty);
        }
        cv.notify_all();
    }

    /**
     * @Michael-wzl
     * @brief Check if the queue is empty.
     * @return True if the queue is empty, false otherwise.
     */
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
