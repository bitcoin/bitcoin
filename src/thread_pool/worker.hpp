#pragma once

#include <thread_pool/slotted_bag.hpp>
#include <thread_pool/thread_pool_options.hpp>

#include <atomic>
#include <thread>
#include <limits>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace tp
{

/**
* @brief The Worker class owns task queue and executing thread.
* In thread it tries to pop task from queue. If queue is empty then it tries
* to steal task from the sibling worker. If steal was unsuccessful then spins
* with one millisecond delay.
* @details State Machine:
*
*            +----------------+---------------+
*            |                |               |
*            v                |               |
*       +--------+       +----------+      +------+
*    +--| Active | ----> | BusyWait | ---> | Idle |
*    |  +--------+       +----------+      +------+
*    |       ^
*    |       |
*    +-------+
*
*/
template <typename Task, template<typename> class Queue>
class Worker final
{
    using WorkerVector = std::vector<std::unique_ptr<Worker<Task, Queue>>>;
    
    class WorkerStoppedException final : public std::exception
    {
    };

    /**
    * @brief State An Enum representing the Rouser thread state.
    */
    enum class State
    {
        Initialized,
        Running,
        Stopped
    };

public:
    /**
    * @brief Worker Constructor.
    * @param busy_wait_options The busy wait behaviour options.
    * @param queue_size Length of underlying task queue.
    */
    explicit Worker(ThreadPoolOptions::BusyWaitOptions const& busy_wait_options, size_t queue_size);
    
    /**
    * @brief Copy ctor implementation.
    */
    Worker(Worker const&) = delete;

    /**
    * @brief Copy assignment implementation.
    */
    Worker& operator=(Worker const& rhs) = delete;

    /**
    * @brief Move ctor implementation.
    * @note Be very careful when invoking this while the thread pool is 
    * active, or in an otherwise undefined state.
    */
    Worker(Worker&& rhs) noexcept;

    /**
    * @brief Move assignment implementaion.
    * @note Be very careful when invoking this while the thread pool is 
    * active, or in an otherwise undefined state.
    */
    Worker& operator=(Worker&& rhs) noexcept;

    /**
    * @brief Destructor implementation.
    */
    ~Worker();

    /**
    * @brief start Create the executing thread and start tasks execution.
    * @param id Worker ID.
    * @param workers A pointer to the vector containing sibling workers for performing round robin work stealing.
    * @param idle_workers A pointer to the slotted bag containing all idle workers.
    * @param num_busy_waiters A pointer to the atomic busy waiter counter.
    * @note The parameters passed into this function generally relate to the global thread pool state.
    */
    void start(const size_t id, WorkerVector& workers, SlottedBag<Queue>& idle_workers, std::atomic<size_t>& num_busy_waiters);

    /**
    * @brief stop Stop all worker's thread and stealing activity.
    * Waits until the executing thread becomes finished.
    * @note Stop may only be called once start() has been invoked. 
    * Repeated successful calls to stop() will be no-ops after the first.
    */
    void stop();

    /**
    * @brief tryPost Post task to queue.
    * @param handler Handler to be executed in executing thread.
    * @return true on success.
    */
    template <typename Handler>
    bool tryPost(Handler&& handler);

    /**
    * @brief tryGetLocalTask Get one task from this worker queue.
    * @param task Place for the obtained task to be stored.
    * @return true on success.
    */
    bool tryGetLocalTask(Task& task);

    /**
    * @brief getWorkerIdForCurrentThread Return worker ID associated with
    * current thread if exists.
    * @return Worker ID.
    */
    static size_t getWorkerIdForCurrentThread();

    /**
    * @brief wake Awake the worker if it was previously asleep.
    */
    void wake();

private:

    /**
    * @brief tryRoundRobinSteal Try stealing a thread from sibling workers in a round-robin fashion.
    * @param task Place for the obtained task to be stored.
    * @param workers Sibling workers for performing round robin work stealing.
    * @return true upon success, false otherwise.
    */
    bool tryRoundRobinSteal(Task& task, WorkerVector& workers);

    /**
    * @brief HandleTask Process a specific task.
    * @param task The task to be processed.
    */
    void handleTask(Task& task);

    /**
    * @brief tryGetTask Try to obtain a work item.
    * @details This entails attempting to pop an item from the local queue, and if not successful,
    * the worker will attempt to perform a round robin steal.
    * @param task Place for the obtained task to be stored.
    * @param workers Sibling workers for performing round robin work stealing.
    * @return true upon success, false otherwise.
    */
    bool tryGetTask(Task& task, WorkerVector& workers);

    /**
    * @brief threadFunc Executing thread function.
    * @param id Worker ID.
    * @param workers A pointer to the vector containing sibling workers for performing round robin work stealing.
    * @param idle_workers A pointer to the slotted bag containing all idle workers.
    * @param num_busy_waiters A pointer to the atomic busy waiter counter.
    * @note The parameters passed into this function generally relate to the global thread pool state.
    */
    void threadFunc(const size_t id, WorkerVector& workers, SlottedBag<Queue>& idle_workers, std::atomic<size_t>& num_busy_waiters);

    Queue<Task> m_queue;
    std::atomic<State> m_state;
    std::thread m_thread;
    size_t m_next_donor;
    ThreadPoolOptions::BusyWaitOptions m_busy_wait_options;
    
    std::mutex m_idle_mutex;
    std::condition_variable m_idle_cv;

    bool m_is_idle;
    bool m_abort_idle;
    
};


/// Implementation

namespace detail
{
    inline size_t& thread_id()
    {
        static thread_local size_t tss_id = std::numeric_limits<size_t>::max();
        return tss_id;
    }
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(ThreadPoolOptions::BusyWaitOptions const& busy_wait_options, size_t queue_size)
    : m_queue(queue_size)
    , m_state(State::Initialized)
    , m_next_donor(0) // Initialized in threadFunc.
    , m_busy_wait_options(busy_wait_options)
    , m_is_idle(false)
    , m_abort_idle(false)
{
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(Worker&& rhs) noexcept
{
    *this = std::move(rhs);
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>& Worker<Task, Queue>::operator=(Worker&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_queue = std::move(rhs.m_queue);
        m_state = rhs.m_state.load();
        m_thread = std::move(rhs.m_thread);
        m_next_donor = rhs.m_next_donor;
        m_busy_wait_options = std::move(rhs.m_busy_wait_options);

        m_idle_mutex = std::move(rhs.m_idle_mutex);
        m_idle_cv = std::move(rhs.m_idle_cv);

        m_is_idle = rhs.m_is_idle;
        m_abort_idle = rhs.m_abort_idle;
    }

    return *this;
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::~Worker()
{
    stop();
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::stop()
{
    auto expectedState = State::Running;
    if (m_state.compare_exchange_strong(expectedState, State::Stopped, std::memory_order_acq_rel))
    {
        wake();
        m_thread.join();
    }
    else if (expectedState == State::Initialized)
        throw std::runtime_error("Cannot stop Worker: stop may only be calld after the Worker has been started.");
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::start(const size_t id, WorkerVector& workers, SlottedBag<Queue>& idle_workers, std::atomic<size_t>& num_busy_waiters)
{
    auto expectedState = State::Initialized;
    if (!m_state.compare_exchange_strong(expectedState, State::Running, std::memory_order_acq_rel))
        throw std::runtime_error("Cannot start Worker: it has previously been started or stopped.");

    m_thread = std::thread(&Worker<Task, Queue>::threadFunc, this, id, std::ref(workers), std::ref(idle_workers), std::ref(num_busy_waiters));
}

template <typename Task, template<typename> class Queue>
inline size_t Worker<Task, Queue>::getWorkerIdForCurrentThread()
{
    return detail::thread_id();
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::wake()
{
    bool notify;
    {
        std::unique_lock<std::mutex> lock(m_idle_mutex);

        m_abort_idle = true;
        notify = m_is_idle;
    }

    // Notify outside of lock.
    if (notify)
        m_idle_cv.notify_one();
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool Worker<Task, Queue>::tryPost(Handler&& handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::tryGetLocalTask(Task& task)
{
    return m_queue.pop(task);
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::tryRoundRobinSteal(Task& task, WorkerVector& workers)
{
    auto starting_index = m_next_donor;

    // Iterate once through the worker ring, checking for queued work items on each thread.
    do
    {
        // Don't steal from local queue.
        if (m_next_donor != detail::thread_id() && workers[m_next_donor]->tryGetLocalTask(task))
        {
            // Increment before returning so that m_next_donor always points to the worker that has gone the longest
            // without a steal attempt. This helps enforce fairness in the stealing.
            ++m_next_donor %= workers.size();
            return true;
        }

        ++m_next_donor %= workers.size();
    } while (m_next_donor != starting_index);

    return false;
}

template <typename Task, template <typename> class Queue>
void Worker<Task, Queue>::handleTask(Task& task)
{
    try
    {
        task();
    }
    catch (...)
    {
        // Suppress all exceptions.
    }
}

template <typename Task, template <typename> class Queue>
bool Worker<Task, Queue>::tryGetTask(Task& task, WorkerVector& workers)
{
    if (m_state.load(std::memory_order_acquire) != State::Running)
        throw WorkerStoppedException();

    // Prioritize local queue, then try stealing from sibling workers.
    return tryGetLocalTask(task) || tryRoundRobinSteal(task, workers);
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::threadFunc(const size_t id, WorkerVector& workers, SlottedBag<Queue>& idle_workers, std::atomic<size_t>& num_busy_waiters)
{
    detail::thread_id() = id;
    m_next_donor = (id + 1) % workers.size();
    Task handler;
    bool task_found = false;

    try
    {
        while (true)
        {
            // By default, this loop operates in the active state. 
            // We poll for items from our local task queue and try to steal from others.
            if (tryGetTask(handler, workers))
            {
                handleTask(handler);
                continue;
            };

            // We were unable to obtain a task. 
            // We now transition into the busy wait state.
            task_found = false;
            num_busy_waiters.fetch_add(1, std::memory_order_acq_rel);

            for (auto i = 0u; i < m_busy_wait_options.numIterations() && !task_found; i++)
            {
                std::this_thread::sleep_for(m_busy_wait_options.iterationFunction()(i));
                task_found = tryGetTask(handler, workers);
            }

            // If we found a task during our busy wait sequence, we abort it and transition back into the active loop.
            if (task_found)
            {
                num_busy_waiters.fetch_sub(1, std::memory_order_acq_rel);
                handleTask(handler); // Handle the task body only once we decrement the busy waiting loop.
                continue;
            }

            // No tasks were found during the busy wait sequence. 
            // We now transition into the idle state.
            {
                std::unique_lock<std::mutex> lock(m_idle_mutex);
                m_is_idle = false;
                m_abort_idle = false;
            }

            // We put this worker up for grabs as a recipient to new posts in the thread pool.
            idle_workers.fill(id);

            // We need to transition out of the busy wait state after we have submitted ourselves to the idle 
            // worker queue in order to avoid a race.
            num_busy_waiters.fetch_sub(1, std::memory_order_acq_rel);

            // While we were adding this worker to the idle worker bag, a job may have been posted into this 
            // worker's queue. We need to check for work again before initiating the deep sleep sequence, otherwise
            // the given task may be lost. 
            // Any further posts will flip the m_abort_idle flag to true, and we will catch them later.
            if (tryGetTask(handler, workers))
            {
                handleTask(handler);
                // A task was indeed posted in the time it took this worker to enter the bag.
                // We remove the worker from the bag. If the internal state of the bag was not changed,
                // this means a different thread has already removed this worker from the idle queue, 
                // and this case will be caught below. Looping early in this case will cause a loss of synchronization.
                if (idle_workers.empty(id))
                    continue;
            }

            {
                std::unique_lock<std::mutex> lock(m_idle_mutex);

                // A post has occurred during the initialization of the sleep sequence! Abort the sleep sequence.
                if (m_abort_idle)
                    continue;

                m_is_idle = true;
                m_idle_cv.wait(lock, [this]() { return m_abort_idle; });
            }
        }
    }
    catch (WorkerStoppedException)
    {
        // Allow thread function to complete.
    }
}

}