// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMPOOL_LAYER_H
#define BITCOIN_MEMPOOL_LAYER_H

#include <consensus/validation.h>
#include <core/async_layer.h>
#include <txmempool.h>

class MempoolLayer;
extern std::unique_ptr<MempoolLayer> g_mempool_layer;

struct TransactionSubmissionResponse {
    //! was the transaction accepted
    const bool accepted;

    //! if rejected, was it rejected because it was missing inputs
    const bool missing_inputs;

    //! if accepted, a list of transactions evicted
    const std::list<CTransactionRef> removed_transactions;

    //! validation state, contains reject reason
    CValidationState status;
};

class TransactionSubmissionRequest : public AsyncRequest<TransactionSubmissionResponse>
{
    friend MempoolLayer;

public:
    //! Returns a transaction witness hash
    std::string GetId() const override;

protected:
    //! Adds this transaction to the mempool
    void operator()() override;

private:
    TransactionSubmissionRequest(MempoolLayer& mempool_layer, const CTransactionRef transaction, const bool bypass_limits, const CAmount absurd_fee, const bool test_only, const std::function<void()> on_ready)
        : m_mempool_layer(mempool_layer), m_transaction(transaction),
          m_bypass_limits(bypass_limits), m_absurd_fee(absurd_fee), m_test_only(test_only),
          m_on_ready(on_ready){};

    //! The mempool
    const MempoolLayer& m_mempool_layer;

    //! Parameters for AcceptToMemoryPool()
    const CTransactionRef m_transaction;
    const bool m_bypass_limits;
    const CAmount m_absurd_fee;
    const bool m_test_only;

    //! A callback to invoke when ready
    //! This is a workaround because c++11 does not support multiplexed waiting on futures
    //! In a move to subsequent standards when this behavior is supported this can probably be removed
    const std::function<void()> m_on_ready;
};

class MempoolLayer : public AsyncLayer
{
    friend TransactionSubmissionRequest;

public:
    MempoolLayer(CTxMemPool& mempool)
        : AsyncLayer(1000), m_mempool(mempool) {}

    //! Submit a transaction for asynchronous validation
    std::future<TransactionSubmissionResponse> SubmitForValidation(
        const CTransactionRef& transaction,
        const bool bypass_limits,
        const CAmount absurd_fee,
        const bool test_only,
        std::function<void()> on_ready = []() {});

    //! Submit a transaction for synchronous validation
    TransactionSubmissionResponse Validate(
        const CTransactionRef& transaction,
        const bool bypass_limits,
        const CAmount absurd_fee,
        const bool test_only);

private:
    //! Internal utility method - sets up and calls AcceptToMemoryPool
    TransactionSubmissionResponse ValidateInternal(
        const CTransactionRef& transaction,
        const bool bypass_limits,
        const CAmount absurd_fee,
        const bool test_only) const;

    CTxMemPool& m_mempool;
};

#endif
