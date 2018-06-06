// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mempool_layer.h>
#include <validation.h>

void TransactionSubmissionRequest::operator()()
{
    LogPrint(BCLog::ASYNC, "%s: validating request=%s\n", __func__, GetId());
    auto res = m_mempool_layer.ValidateInternal(m_transaction, m_bypass_limits, m_absurd_fee, m_test_only);
    LogPrint(BCLog::ASYNC, "%s: validation result request=%s accepted=%d\n",
        __func__, GetId(), res.accepted);

    m_promise.set_value(res);
    if (m_on_ready) {
        m_on_ready();
    }
}

std::string TransactionSubmissionRequest::GetId() const
{
    return strprintf("TransactionRequest[%s]", m_transaction->GetWitnessHash().ToString());
}

std::future<TransactionSubmissionResponse> MempoolLayer::SubmitForValidation(
    const CTransactionRef& transaction,
    const bool bypass_limits,
    const CAmount absurd_fee,
    const bool test_only,
    std::function<void()> on_ready)
{
    TransactionSubmissionRequest* req = new TransactionSubmissionRequest(*this, transaction,
        bypass_limits, absurd_fee, test_only,
        on_ready);
    return AddToQueue<TransactionSubmissionRequest, TransactionSubmissionResponse>(req);
}

TransactionSubmissionResponse MempoolLayer::Validate(
    const CTransactionRef& transaction,
    const bool bypass_limits,
    const CAmount absurd_fee,
    const bool test_only)
{
    return SubmitForValidation(transaction, bypass_limits, absurd_fee, test_only).get();
}

TransactionSubmissionResponse MempoolLayer::ValidateInternal(const CTransactionRef& tx, bool bypass_limits, CAmount absurd_fee, bool test_only) const
{
    bool missing;
    CValidationState state{};
    std::list<CTransactionRef> removed;

    LOCK(cs_main);
    bool accepted = AcceptToMemoryPool(m_mempool, state, tx, &missing, &removed, bypass_limits, absurd_fee, test_only);

    return TransactionSubmissionResponse{accepted, missing, std::move(removed), state};
};
