#pragma once

#include <algorithm>
#include <thread>
#include <functional>
#include <chrono>

namespace tp
{

/**
 * @brief The ThreadPoolOptions class provides creation options for ThreadPool.
 */
class ThreadPoolOptions final
{
public:
    /**
    * @brief The BusyWaitOptions class provides worker busy wait behaviour options.
    */
    class BusyWaitOptions final
    {
    public:
        using IterationFunction = std::function<std::chrono::microseconds(size_t)>;

        /**
        * @brief BusyWaitOptions Construct default options for busy wait behaviour.
        */
        BusyWaitOptions(size_t num_iterations = defaultNumIterations()
            , IterationFunction function = defaultIterationFunction());

        /**
        * @brief setNumIterations Set the number of sleeping iterations that take place during
        * a worker's busy wait state. The iteration function will be called on every iteration 
        * with the iteration number.
        * @param count The number of busy wait iterations.
        */
        void setNumIterations(size_t count);

        /**
        * @brief setIterationFunction Set the function to be called upon each sleep iteration.
        * @param function The iteration function to be called.
        */
        void setIterationFunction(IterationFunction function);

        /**
        * @brief numIterations Return the number of busy wait iterations.
        */
        size_t numIterations() const;

        /**
        * @brief iterationFunction Return the busy wait iteration function.
        */
        IterationFunction const& iterationFunction() const;

        /**
        * @brief defaultThreadCount Obtain the default num iterations value.
        */
        static size_t defaultNumIterations();

        /**
        * @brief defaultIterationFunction Obtain the default iteration function value.
        */
        static IterationFunction defaultIterationFunction();

    private:
        size_t m_num_iterations;
        IterationFunction m_iteration_function;
    };

    /**
     * @brief ThreadPoolOptions Construct default options for thread pool.
     */
    ThreadPoolOptions(size_t thread_count = defaultThreadCount()
        , size_t queue_size = defaultQueueSize()
        , size_t failed_wakeup_retry_cap = defaultFailedWakeupRetryCap()
        , BusyWaitOptions busy_wait_options = defaultBusyWaitOptions()
        , std::chrono::microseconds rouse_period = defaultRousePeriod());

    /**
     * @brief setThreadCount Set thread count.
     * @param count Number of threads to be created.
     */
    void setThreadCount(size_t count);

    /**
     * @brief setQueueSize Set single worker queue size.
     * @param count Maximum length of queue of single worker.
     */
    void setQueueSize(size_t size);

    /**
    * @brief setFailedWakeupRetryCap Set retry cap when a worker wakeup fails.
    * @param cap The retry cap.
    */
    void setFailedWakeupRetryCap(size_t cap);

    /**
    * @brief setBusyWaitOptions Set the parameters relating to worker busy waiting behaviour.
    * @param options The busy wait options.
    */
    void setBusyWaitOptions(BusyWaitOptions options);

    /**
    * @brief setRousePeriod Set the period with which workers are aroused from their idling states.
    * @param period The afore-mentioned period.
    */
    void setRousePeriod(std::chrono::microseconds period);


    /**
     * @brief threadCount Return thread count.
     */
    size_t threadCount() const;

    /**
     * @brief queueSize Return single worker queue size.
     */
    size_t queueSize() const;

    /**
    * @brief failedWakeupRetryCap Return the retry cap when a worker wakeup fails.
    */
    size_t failedWakeupRetryCap() const;

    /**
    * @brief busyWaitOptions Return a reference to the busy wait options.
    */
    BusyWaitOptions const& busyWaitOptions() const;

    /**
    * @brief rousePeriod Return the rouse period.
    */
    std::chrono::microseconds const& rousePeriod() const;
    
    /**
    * @brief defaultThreadCount Obtain the default thread count value.
    */
    static size_t defaultThreadCount();

    /**
    * @brief defaultQueueSize Obtain the default queue size value.
    */
    static size_t defaultQueueSize();

    /**
    * @brief defaultFailedWakeupRetryCap Obtain the default retry cap when a worker wakeup fails.
    */
    static size_t defaultFailedWakeupRetryCap();

    /**
    * @brief defaultBusyWaitOptions Obtain the default busy wait options.
    */
    static BusyWaitOptions defaultBusyWaitOptions();

    /**
    * @brief defaultRousePeriod Obtain the default rouse period.
    */
    static std::chrono::microseconds defaultRousePeriod();


private:
    size_t m_thread_count;
    size_t m_queue_size;
    size_t m_failed_wakeup_retry_cap;
    BusyWaitOptions m_busy_wait_options;
    std::chrono::microseconds m_rouse_period;
};

/// Implementation

inline ThreadPoolOptions::BusyWaitOptions::BusyWaitOptions(size_t num_iterations, ThreadPoolOptions::BusyWaitOptions::IterationFunction function)
    : m_num_iterations(num_iterations)
    , m_iteration_function(std::move(function))
{
}

inline void ThreadPoolOptions::BusyWaitOptions::setNumIterations(size_t count)
{
    m_num_iterations = std::max<size_t>(0u, count);
}

inline void ThreadPoolOptions::BusyWaitOptions::setIterationFunction(ThreadPoolOptions::BusyWaitOptions::IterationFunction function)
{
    m_iteration_function = std::move(function);
}

inline size_t ThreadPoolOptions::BusyWaitOptions::numIterations() const
{
    return m_num_iterations;
}

inline ThreadPoolOptions::BusyWaitOptions::IterationFunction const& ThreadPoolOptions::BusyWaitOptions::iterationFunction() const
{
    return m_iteration_function;
}


inline size_t ThreadPoolOptions::BusyWaitOptions::defaultNumIterations()
{
    static const size_t instance = 3;
    return instance;
}

inline ThreadPoolOptions::BusyWaitOptions::IterationFunction ThreadPoolOptions::BusyWaitOptions::defaultIterationFunction()
{
    return [](size_t i) { return std::chrono::microseconds(static_cast<size_t>(pow(2, i))*1000); };
}

inline ThreadPoolOptions::ThreadPoolOptions(size_t thread_count, size_t queue_size, size_t failed_wakeup_retry_cap, BusyWaitOptions busy_wait_options, std::chrono::microseconds rouse_period)
    : m_thread_count(thread_count)
    , m_queue_size(queue_size)
    , m_failed_wakeup_retry_cap(failed_wakeup_retry_cap)
    , m_busy_wait_options(std::move(busy_wait_options))
    , m_rouse_period(std::move(rouse_period))
{
}

inline void ThreadPoolOptions::setThreadCount(size_t count)
{
    m_thread_count = std::max<size_t>(1u, count);
}

inline void ThreadPoolOptions::setQueueSize(size_t size)
{
    m_queue_size = std::max<size_t>(1u, size);
}

inline void ThreadPoolOptions::setFailedWakeupRetryCap(size_t cap)
{
    m_failed_wakeup_retry_cap = std::max<size_t>(1u, cap);
}

inline void ThreadPoolOptions::setBusyWaitOptions(BusyWaitOptions busy_wait_options)
{
    m_busy_wait_options = std::move(busy_wait_options);
}

inline void ThreadPoolOptions::setRousePeriod(std::chrono::microseconds period)
{
    m_rouse_period = std::move(period);
}

inline size_t ThreadPoolOptions::threadCount() const
{
    return m_thread_count;
}

inline size_t ThreadPoolOptions::queueSize() const
{
    return m_queue_size;
}

inline size_t ThreadPoolOptions::failedWakeupRetryCap() const
{
    return m_failed_wakeup_retry_cap;
}

inline ThreadPoolOptions::BusyWaitOptions const& ThreadPoolOptions::busyWaitOptions() const
{
    return m_busy_wait_options;
}

inline std::chrono::microseconds const& ThreadPoolOptions::rousePeriod() const
{
    return m_rouse_period;
}

inline size_t ThreadPoolOptions::defaultThreadCount()
{
    static const size_t instance = std::max<size_t>(1u, std::thread::hardware_concurrency());
    return instance;
}

inline size_t ThreadPoolOptions::defaultQueueSize()
{
    return 1024;
}

inline size_t ThreadPoolOptions::defaultFailedWakeupRetryCap()
{
    return 5;
}

inline ThreadPoolOptions::BusyWaitOptions ThreadPoolOptions::defaultBusyWaitOptions()
{
    return ThreadPoolOptions::BusyWaitOptions();
}

inline std::chrono::microseconds ThreadPoolOptions::defaultRousePeriod()
{
    return std::chrono::microseconds(10000);
}


}
