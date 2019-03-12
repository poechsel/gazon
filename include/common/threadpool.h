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

/* Thread Safe queue. Elements inserted in it are tagged, and
 only one element of each tag can be extracted from it at a time.*/
template <typename Tag, typename Element>
class ThreadQueueGroup {
public:
    ThreadQueueGroup(): m_exists(true) {
    }

    ~ThreadQueueGroup() {
        destroy();
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void done(const Tag &tag) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tags_being_used.erase(tag);
        m_condition.notify_one();
    }

    void push(const Tag &tag, const Element &element) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push_back({tag, element, false});
        m_condition.notify_one();
    }

    bool waitPop(Tag &tag, Element &element) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this]() {
                                   return unsafeCanSchedule() || !m_exists;
                                       });
        if (!m_exists)
            return false;

        bool status = false;
        for (unsigned int i = 0; i < m_queue.size(); ++i) {
            if (!m_queue[i].wasUsed
                && m_tags_being_used.find(m_queue[i].tag) == m_tags_being_used.end()) {
                status = true;
                m_queue[i].wasUsed = true;
                m_tags_being_used.insert(m_queue[i].tag);
                element = std::move(m_queue[i].element);
                tag = m_queue[i].tag;
                break;
            }
        }

        while(!m_queue.empty() && m_queue.front().wasUsed) {
            m_queue.pop_front();
        }
        return status;
    }

    void destroy() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exists = false;
        m_condition.notify_all();
    }

private:
    bool unsafeCanSchedule() {
        if (m_queue.empty()) return false;
        for (unsigned int i = 0; i < m_queue.size(); ++i) {
            if (!m_queue[i].wasUsed
                && m_tags_being_used.find(m_queue[i].tag) == m_tags_being_used.end()) {
                return true;
            }
        }
        return false;
    }
    struct TaggedElement {
        Tag tag;
        Element element;
        bool wasUsed;
    };
    std::set<Tag> m_tags_being_used;
    std::deque<TaggedElement> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_exists;
};


/* Thread pool.
   When `join` is called every thread will finish their current work and
   quit, even though jobs are remaining in the job queue.
*/
template <typename Tag>
class ThreadPool {
public:
    ThreadPool(unsigned int n): m_pool_done(false) {
        for (unsigned int i = 0; i < n; ++i) {
            m_threads.emplace_back(&ThreadPool::worker, this);
        }
    }

    void schedule(Tag tag, std::function<void()> task) {
        m_queue.push(tag, task);
    }

    void join() {
        m_pool_done = true;
        m_queue.destroy();
        for (auto &thread : m_threads) {
            if (thread.joinable())
                thread.join();
        }
    }

private:
    void worker() {
        while (!m_pool_done) {
            Tag tag;
            std::function<void()> element;
            if (m_queue.waitPop(tag, element)) {
                element();
                m_queue.done(tag);
            }
        }
    }

    ThreadQueueGroup<Tag, std::function<void()>> m_queue;
    std::atomic_bool m_pool_done;
    std::vector<std::thread> m_threads;
};
