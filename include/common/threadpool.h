#pragma once

#include <deque>
#include <set>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

/** A generic queue for values of type T. */
template <typename T>
class Queue {
public:
    Queue(): m_pop_index(0), m_push_index(0){}

    /** Return whether the queue is empty. */
    bool empty() const {
        return m_push_index == 0 && m_pop_index == 0;
    }

    /** Return the number of items in the queue. */
    size_t size() const {
        return m_push_index + m_pop_index;
    }

    /** Return the front item in the queue. */
    T front() {
        spill();
        return m_pop[m_pop_index - 1];
    }

    /** Add an item to the back of the queue. */
    void push_back(T &&t) {
        m_push[m_push_index++] = std::move(t);
    }

    /** Remove the front item from the queue and return it. */
    T pop_front() {
        spill();
        return m_pop[--m_pop_index];
    }

    /** Return the n-th item in the queue. */
    T& operator[] (const int index) {
        if (index < m_push_index) {
            return m_push[index];
        } else {
            return m_pop[index - m_push_index];
        }
    }
private:
    int m_pop_index;
    int m_push_index;
    T m_pop[256];
    T m_push[256];

    /** Spill the items of m_push into m_pop in reverse order. */
    void spill() {
        if (m_pop_index == 0) {
            while (m_push_index > 0) {
                m_pop[m_pop_index++] = m_push[--m_push_index];
            }
        }
    }
};

/**
 * A generic thread-safe queue for tagged jobs.
 *
 * Items are inserted with tags, providing several guarantees:
 * - Items with the same tag will be popped in insertion order.
 * - Only one item with a given tag can be popped until a call to done().
 * - Only one worker thread can pop items of a given tag.
 */
template <typename Tag, typename Element, int n>
class ThreadPoolQueue {
public:
    ThreadPoolQueue() : stopped(false) {}

    /**
     * Push a new item onto the queue.
     *
     * A worker thread will be assigned to the tag arbitrarily, and all the
     * subsequent pushes of the same tag will be assigned to the same worker.
     *
     * @param tag   The tag of the item that is pushed, will be moved.
     * @param item  The item to be pushed, will be moved.
     */
    void push(const Tag &tag, const Element &item) {
        std::unique_lock<std::mutex> mapLock(mapMutex);
        if (map.count(tag) == 0) {
            lastMapped = (lastMapped + 1) % n;
            map[tag] = lastMapped;
        }
        unsigned int index = map[tag];
        mapLock.unlock();

        auto &subqueue = subqueues[index];
        std::unique_lock<std::mutex> queueLock(subqueue.mutex);
        subqueue.queue.push_back(std::move(std::make_pair(tag, item)));
        queueLock.unlock();
        subqueue.condition.notify_one();
    }

    /**
     * Blocks until an item can be popped by a given thread.
     *
     * @param index The identity of the thread which wants to pop the item.
     * @param tag   A reference to the destination of the popped tag.
     * @param item  A reference to the destination of the popped item.
     * @return      Whether an item was actually popped.
     */
    bool blockUntilPop(unsigned int index, Tag &tag, Element &item) {
        auto &subqueue = subqueues[index];
        std::unique_lock<std::mutex> queueLock(subqueue.mutex);
        subqueue.condition.wait(queueLock, [&]() {
            return !subqueue.queue.empty() || stopped;
        });

        if (!subqueue.queue.empty()) {
            std::tie(tag, item) = subqueue.queue.pop_front();
            return true;
        } else {
            return false;
        }
    }

    /** Stops all the current blockUntilPop operations. */
    void stop() {
        stopped = true;

        for (auto &subqueue : subqueues) {
            subqueue.condition.notify_all();
        }
    }

private:
    using Item = std::pair<Tag, Element>;

    /// The subqueue of a single worker thread.
    struct Subqueue {
        Queue<Item> queue;
        std::mutex mutex;
        std::condition_variable condition;
    };

    /// A subqueue for each worker thread.
    std::array<Subqueue, n> subqueues;

    /// A mapping of tags to their assigned worker threads.
    std::unordered_map<Tag, unsigned int> map;

    /// The last thread that was assigned a tag.
    unsigned int lastMapped;

    /// A mutex to protect map and lastMapped.
    std::mutex mapMutex;

    /// Whether the queue is stopped.
    std::atomic_bool stopped;
};


/**
 * A thread pool for tagged jobs.
 *
 * It provides the following guarantees:
 * - All jobs with a given tag will be executed in insertion order.
 * - All jobs with a given tag will be executed on the same thread.
 * - All scheduled jobs will finish before join() returns.
 */
template <typename Tag, int n>
class ThreadPool {
public:
    ThreadPool(): queue() {
        for (unsigned int i = 0; i < n; ++i) {
            threads.emplace_back(&ThreadPool::worker, this, i);
        }
    }

    /** Schedule a new tagged job onto the pool. */
    void schedule(Tag tag, std::function<void()> job) {
        queue.push(tag, job);
    }

    /** Stop the thread pool and waits for the completion of all jobs. */
    void join() {
        queue.stop();
        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    std::vector<std::thread> threads;
    ThreadPoolQueue<Tag, std::function<void()>, n> queue;

    /**
     * Consume jobs from the queue and execute them.
     *
     * This function is run by every thread in the pool, and will only return
     * when the queue has been stopped and all jobs have finished running.
     *
     * @param index The index of the worker thread.
     */
    void worker(unsigned int index) {
        while (true) {
            Tag tag;
            std::function<void()> job;

            if (queue.blockUntilPop(index, tag, job)) {
                job();
            } else {
                return;
            }
        }
    }
};
