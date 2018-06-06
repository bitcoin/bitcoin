// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_ASYNC_LAYER_H
#define BITCOIN_CORE_ASYNC_LAYER_H

#include <future>

#include <chainparams.h>
#include <core/async_layer.h>
#include <core/consumerthread.h>
#include <core/producerconsumerqueue.h>
#include <util.h>

/**
 * Encapsulates a request to validate an object
 *
 * @see AsyncLayer
 */
template <typename RESPONSE>
class AsyncRequest : public WorkItem<WorkerMode::BLOCKING>
{
    friend class AsyncLayer;

public:
    //! Returns a string identifier (for logging)
    virtual std::string GetId() const = 0;

protected:
    //! Guts of the validation
    virtual void operator()() = 0;

    //! Promise that will deliver the validation result to the caller who generated this request
    std::promise<RESPONSE> m_promise;
};

class AsyncLayer
{
    typedef WorkQueue<WorkerMode::BLOCKING> RequestQueue;
    typedef ConsumerThread<WorkerMode::BLOCKING> WorkerThread;

public:
    AsyncLayer(unsigned int capacity) : m_request_queue(std::make_shared<RequestQueue>(capacity)) {}

    void Start();
    void Stop();

protected:
    //! Internal utility method that wraps a request in a unique pointer and deposits it on the validation queue
    template <typename REQUEST, typename RESPONSE>
    std::future<RESPONSE> AddToQueue(REQUEST* request)
    {
        LogPrint(BCLog::ASYNC, "%s<%s>: submitting request=%s\n", __func__, typeid(RESPONSE).name(), request->GetId());

        auto ret = request->m_promise.get_future();
        m_request_queue->Push(std::unique_ptr<REQUEST>(request));
        return ret;
    };

private:
    //! a queue that holds validation requests that are sequentially processed by m_thread
    const std::shared_ptr<RequestQueue> m_request_queue;

    //! the validation thread - sequentially processes validation requests from m_validation_queue
    std::unique_ptr<WorkerThread> m_thread;
};

#endif // BITCOIN_CORE_ASYNC_LAYER_H
