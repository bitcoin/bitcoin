#pragma once

#include <atomic>
#include <thread>

namespace tp
{

/**
 * @brief The Worker class owns task queue and executing thread.
 * In thread it tries to pop task from queue. If queue is empty then it tries
 * to steal task from the sibling worker. If steal was unsuccessful then spins
 * with one millisecond delay.
 */
template <typename Task, template<typename> class Queue>
class Worker
{
public:
    /**
     * @brief Worker Constructor.
     * @param queue_size Length of undelaying task queue.
     */
    explicit Worker(size_t queue_size);

    /**
     * @brief Move ctor implementation.
     */
    Worker(Worker&& rhs) noexcept;

    /**
     * @brief Move assignment implementaion.
     */
    Worker& operator=(Worker&& rhs) noexcept;

    /**
     * @brief start Create the executing thread and start tasks execution.
     * @param id Worker ID.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void start(size_t id, Worker* steal_donor);

    /**
     * @brief stop Stop all worker's thread and stealing activity.
     * Waits until the executing thread became finished.
     */
    void stop();

    /**
     * @brief post Post task to queue.
     * @param handler Handler to be executed in executing thread.
     * @return true on success.
     */
    template <typename Handler>
    bool post(Handler&& handler);

    /**
     * @brief steal Steal one task from this worker queue.
     * @param task Place for stealed task to be stored.
     * @return true on success.
     */
    bool steal(Task& task);

    /**
     * @brief getWorkerIdForCurrentThread Return worker ID associated with
     * current thread if exists.
     * @return Worker ID.
     */
    static size_t getWorkerIdForCurrentThread();

private:
    /**
     * @brief threadFunc Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void threadFunc(size_t id, Worker* steal_donor);

    Queue<Task> m_queue;
    std::atomic<bool> m_running_flag;
    std::thread m_thread;
};


/// Implementation

namespace detail
{
    inline size_t* thread_id()
    {
        static thread_local size_t tss_id = -1u;
        return &tss_id;
    }
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(size_t queue_size)
    : m_queue(queue_size)
    , m_running_flag(true)
{
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(Worker&& rhs) noexcept
{
    *this = rhs;
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>& Worker<Task, Queue>::operator=(Worker&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_queue = std::move(rhs.m_queue);
        m_running_flag = rhs.m_running_flag.load();
        m_thread = std::move(rhs.m_thread);
    }
    return *this;
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::stop()
{
    m_running_flag.store(false, std::memory_order_relaxed);
    m_thread.join();
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::start(size_t id, Worker* steal_donor)
{
    m_thread = std::thread(&Worker<Task, Queue>::threadFunc, this, id, steal_donor);
}

template <typename Task, template<typename> class Queue>
inline size_t Worker<Task, Queue>::getWorkerIdForCurrentThread()
{
    return *detail::thread_id();
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool Worker<Task, Queue>::post(Handler&& handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::steal(Task& task)
{
    return m_queue.pop(task);
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::threadFunc(size_t id, Worker* steal_donor)
{
    *detail::thread_id() = id;

    Task handler;

    while (m_running_flag.load(std::memory_order_relaxed))
    {
        if (m_queue.pop(handler) || steal_donor->steal(handler))
        {
            try
            {
                handler();
            }
            catch(...)
            {
                // suppress all exceptions
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

}
