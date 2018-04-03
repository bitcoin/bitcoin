// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_CONSUMERTHREAD_H
#define BITCOIN_CORE_CONSUMERTHREAD_H

#include <future>
#include <thread>

#include <core/producerconsumerqueue.h>
#include <util.h>

template <WorkerMode MODE>
class ConsumerThread;

//! A WorkItem() encapsulates a task that can be processed by a ConsumerThread()
//! @see ConsumerThread()
template <WorkerMode MODE>
class WorkItem
{
    friend ConsumerThread<MODE>; //<! only a consumer thread can execute a WorkItem

protected:
    WorkItem(){};
    virtual void operator()(){};
};

template <WorkerMode MODE>
class GenericWorkItem : public WorkItem<MODE>
{
    friend ConsumerThread<MODE>;

public:
    GenericWorkItem(std::function<void()> f) : m_f(f) {}

protected:
    void operator()() override
    {
        m_f();
    }

    std::function<void()> m_f;
};

//! A special WorkItem() that is used to interrupt a blocked ConsumerThread() so that it can terminate
template <WorkerMode MODE>
class ShutdownPill : public WorkItem<MODE>
{
    friend ConsumerThread<MODE>;

private:
    ShutdownPill(ConsumerThread<MODE>& consumer) : m_consumer(consumer){};
    void operator()()
    {
        std::thread::id id = m_consumer.m_thread.get_id();
        if (std::this_thread::get_id() != id) {
            // this ShutdownPill was intended for another thread

            // we haven't seen this pill before
            if (!m_threads_observed.count(id)) {
                m_threads_observed.insert(std::this_thread::get_id());

                // resubmit it so that it gets a chance to get to the right thread
                // when resubmitting, do not block and do not care about failures
                // theres a potential deadlock where we try to push this to a queue thats
                // full and there are no other threads still consuming
                // since the only purpose of reinjecting this is to terminate threads that
                // may be blocking on an empty queue when the queue is full we do not need to do this
                m_consumer.m_queue->Push(MakeUnique<ShutdownPill<MODE>>(std::move(*this)), WorkerMode::NONBLOCKING);
            }

            // if the same pill has been seen by the same thread previously then it can safely be discarded
            // the intended thread has either terminated or is currently processing a work item and will terminate
            // after completing that item and before blocking on the queue
        }
    };

    ConsumerThread<MODE>& m_consumer;
    std::set<std::thread::id> m_threads_observed;
};

template <WorkerMode PRODUCER_MODE>
class WorkQueue : public BlockingConsumerQueue<std::unique_ptr<WorkItem<PRODUCER_MODE>>, PRODUCER_MODE>
{
public:
    WorkQueue(int capacity) :BlockingConsumerQueue<std::unique_ptr<WorkItem<PRODUCER_MODE>>, PRODUCER_MODE>(capacity) {}

    //! Blocks until everything pushed to the queue prior to this call has been dequeued by a worker
    void Sync()
    {
        std::promise<void> barrier;
        this->Push(MakeUnique<GenericWorkItem<PRODUCER_MODE>>([&barrier](){ barrier.set_value(); }), WorkerMode::BLOCKING);
        barrier.get_future().wait();
    }
};

/**
 * A worker thread that interoperates with a BlockingConsumerQueue
 *
 * Blocks on the queue, pulls WorkItem() tasks and executes them
 * No assumptions are made about number of threads operating on this queue
 *
 * @see WorkItem
 * @see WorkQueue
 * @see BlockingConsumerQueue
 * @see ProducerConsumerQueue
 */
template <WorkerMode PRODUCER_POLICY>
class ConsumerThread
{
    friend ShutdownPill<PRODUCER_POLICY>; //<! needs to introspect in order to cleanly terminate this thread

public:
    //! Default constructor: not a valid thread
    ConsumerThread() : m_active(false){};

    //! Constructs a ConsumerThread: RAII
    //! @param queue the queue from which this thread will pull work
    ConsumerThread(std::shared_ptr<WorkQueue<PRODUCER_POLICY>> queue, const std::string id = "worker")
        : m_id(id), m_queue(queue), m_active(true)
    {
        m_thread = std::thread(&TraceThread<std::function<void()>>, id.c_str(), std::function<void()>(std::bind(&ConsumerThread<PRODUCER_POLICY>::Loop, this)));
    };

    //! Terminates a running consumer thread
    //! Blocks until the thread joins
    //! Repeated calls are no-ops
    void Terminate()
    {
        RequestTermination();
        Join();
    }

    //! Requests termination of a running consumer thread
    //! Does not wait for the thread to terminate
    //! Repeated calls are no-ops
    void RequestTermination()
    {
        // locked only so that repeated calls do not push extra ShutdownPills
        std::unique_lock<CWaitableCriticalSection> l(m_cs_shutdown);
        if (m_active) {
            m_active = false;

            // push an empty WorkItem so that we wake the thread up if it is blocking on an empty queue
            // there is no easy way to determine if this consumer is blocked on the queue without introducing
            // additional synchronization, but there is little downside to pushing this unnecessarily:
            // either this is the last active thread on the queue in which case this will be destroyed if/when
            // the queue (and any other work that may remain is destroyed)
            // or there are other threads on the queue - in which case this pill will be discarded after any
            // of the other threads observe it more than once
            m_queue->Push(std::unique_ptr<ShutdownPill<PRODUCER_POLICY>>(new ShutdownPill<PRODUCER_POLICY>(*this)), WorkerMode::NONBLOCKING);
        }
    }

    //! Waits until this thread terminates
    //! RequestTerminate() must have been previously called or be called by a different thread
    void Join()
    {
        m_thread.join();
    }

    bool IsActive() const { std::unique_lock<CWaitableCriticalSection> l(m_cs_shutdown); return m_active; }

    const std::string m_id;

private:
    //! the queue of work that this thread should consume from
    const std::shared_ptr<WorkQueue<PRODUCER_POLICY>> m_queue;

    //! the thread that this class wraps
    std::thread m_thread;

    //! whether this thread should continue running: behaves like a latch
    //! initialized to true in the constructor
    //! can be set to false by calling Terminate()
    volatile bool m_active;

    //! protects Terminate()
    mutable CWaitableCriticalSection m_cs_shutdown;

    //! the body of this thread
    //! Receives WorkItem() elements from m_queue and executes them until Terminate() is called
    void Loop()
    {
        while (m_active) {
            Process(m_queue->Pop());
        }
    }

    void Process(const std::unique_ptr<WorkItem<PRODUCER_POLICY>> work) const
    {
        (*work)();
    }
};

#endif // BITCOIN_CORE_CONSUMERTHREAD_H
