#pragma once

#include <thread_pool/slotted_bag.hpp>
#include <thread_pool/thread_pool_options.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace tp
{

template <typename Task, template<typename> class Queue>
class Worker;

template <typename Task, template<typename> class Queue>
class ThreadPoolState
{
    using WorkerVector = std::vector<std::unique_ptr<Worker<Task, Queue>>>;

public:

    /**
    * @brief create Construct a shared instance of a thread pool state object.
    * @param options Creation options.
    */
    static std::shared_ptr<ThreadPoolState<Task, Queue>> create(ThreadPoolOptions const& options);

    /**
    * @brief idleWorkers obtain the idle worker queue.
    */
    SlottedBag<Queue>& idleWorkers();

    /**
    * @brief workers Obtain the worker list.
    */
    WorkerVector& workers();

    /**
    * @brief numBusyWaiters Obtain the busy waiter count.
    */
    std::atomic<size_t>& numBusyWaiters();

protected:

    /**
    * @brief ThreadPoolState Construct a thread pool state.
    * @param options Creation options.
    */
    explicit ThreadPoolState(ThreadPoolOptions const& options);

    /**
    * @brief Copy ctor implementation.
    */
    ThreadPoolState(ThreadPoolState const&) = delete;

    /**
    * @brief Copy assignment implementation.
    */
    ThreadPoolState& operator=(ThreadPoolState const& rhs) = delete;

    /**
    * @brief Move ctor implementation.
    */
    ThreadPoolState(ThreadPoolState&& rhs) = delete;

    /**
    * @brief Move assignment implementaion.v
    */
    ThreadPoolState& operator=(ThreadPoolState&& rhs) = delete;

    /**
    * @brief ~ThreadPoolState destructor.
    */
    virtual ~ThreadPoolState() = default;

private:

    SlottedBag<Queue> m_idle_workers;
    WorkerVector m_workers;
    std::atomic<size_t> m_num_busy_waiters;
};


/// Implementation

template <typename Task, template<typename> class Queue>
inline std::shared_ptr<ThreadPoolState<Task, Queue>> ThreadPoolState<Task, Queue>::create(ThreadPoolOptions const& options)
{
    struct Creator : public ThreadPoolState
    {
        Creator(ThreadPoolOptions options)
            : ThreadPoolState(options)
        {
        }
    };

    return std::make_shared<Creator>(options);
}

template <typename Task, template<typename> class Queue>
inline SlottedBag<Queue>& ThreadPoolState<Task, Queue>::idleWorkers()
{
    return m_idle_workers;
}

template <typename Task, template<typename> class Queue>
inline typename ThreadPoolState<Task, Queue>::WorkerVector& ThreadPoolState<Task, Queue>::workers()
{
    return m_workers;
}

template <typename Task, template<typename> class Queue>
inline std::atomic<size_t>& ThreadPoolState<Task, Queue>::numBusyWaiters()
{
    return m_num_busy_waiters;
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolState<Task, Queue>::ThreadPoolState(ThreadPoolOptions const& options)
    : m_idle_workers(options.threadCount())
    , m_workers(options.threadCount())
    , m_num_busy_waiters(0)
{
}

}
