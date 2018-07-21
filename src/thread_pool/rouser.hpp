#pragma once

#include "slotted_bag.hpp"
#include "thread_pool_options.hpp"
#include "thread_pool_state.hpp"
#include "worker.hpp"

#include <atomic>
#include <thread>
#include <limits>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace tp
{

/**
* @brief Rouser is a specialized worker that periodically wakes other workers. 
* @detail This serves two purposes:
* The first is that it emplaces an upper bound on the processing time of tasks in the thread pool, since
* it is otherwise possible for the thread pool to enter a state where all threads are asleep, and tasks exist
* in worker queues. The second is that it increases the likelihood of at least one worker busy-waiting at any
* point in time, which speeds up task processing response time.
*/
class Rouser final
{
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
    */
    Rouser(std::chrono::microseconds rouse_period);

    /**
    * @brief Copy ctor implementation.
    */
    Rouser(Rouser const&) = delete;

    /**
    * @brief Copy assignment implementation.
    */
    Rouser& operator=(Rouser const& rhs) = delete;

    /**
    * @brief Move ctor implementation.
    */
    Rouser(Rouser&& rhs) = delete;

    /**
    * @brief Move assignment implementaion.
    */
    Rouser& operator=(Rouser&& rhs) = delete;

    /**
    * @brief Destructor implementation.
    */
    ~Rouser();

    /**
    * @brief start Create the executing thread and start tasks execution.
    * @param state A pointer to thread pool's shared state.
    * @note The parameters passed into this function generally relate to the global thread pool state.
    */
    template <typename Task, template<typename> class Queue>
    void start(std::shared_ptr<ThreadPoolState<Task, Queue>> state);

    /**
    * @brief stop Stop all worker's thread and stealing activity.
    * Waits until the executing thread becomes finished.
    * @note Stop may only be called once start() has been invoked. 
    * Repeated successful calls to stop() will be no-ops after the first.
    */
    void stop();

private:

    /**
    * @brief threadFunc Executing thread function.
    * @param state A pointer to thread pool's shared state.
    */
    template <typename Task, template<typename> class Queue>
    void threadFunc(std::shared_ptr<ThreadPoolState<Task, Queue>> shared_state);

    std::atomic<State> m_state;
    std::thread m_thread;
    std::chrono::microseconds m_rouse_period;
};

inline Rouser::Rouser(std::chrono::microseconds rouse_period)
    : m_state(State::Initialized)
    , m_rouse_period(std::move(rouse_period))
{
}

inline Rouser::~Rouser()
{
    stop();
}

template <typename Task, template<typename> class Queue>
inline void Rouser::start(std::shared_ptr<ThreadPoolState<Task, Queue>> state)
{
    auto expectedState = State::Initialized;
    if (!m_state.compare_exchange_strong(expectedState, State::Running, std::memory_order_acq_rel, std::memory_order_seq_cst))
        throw std::runtime_error("Cannot start Rouser: it has previously been started or stopped.");

    m_thread = std::thread(&Rouser::threadFunc<Task, Queue>, this, state);
}

inline void Rouser::stop()
{
    auto expectedState = State::Running;
    if (m_state.compare_exchange_strong(expectedState, State::Stopped, std::memory_order_acq_rel, std::memory_order_seq_cst))
        m_thread.join();
    else if (expectedState == State::Initialized)
        throw std::runtime_error("Cannot stop Rouser: stop may only be calld after the Rouser has been started.");
}

template <typename Task, template<typename> class Queue>
inline void Rouser::threadFunc(std::shared_ptr<ThreadPoolState<Task, Queue>> shared_state)
{
    while (m_state.load(std::memory_order_acquire) == State::Running)
    {
        // Try to wake up a thread if there are no current busy waiters.
        if (shared_state->numBusyWaiters().load(std::memory_order_acquire) == 0)
        {
            auto result = shared_state->idleWorkers().tryEmptyAny();
            if (result.first)
                shared_state->workers()[result.second]->wake();
        }

        // Sleep.
        std::this_thread::sleep_for(m_rouse_period);
    }
}

}