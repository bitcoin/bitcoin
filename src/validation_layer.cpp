// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation_layer.h>
#include <validation.h>

void BlockValidationRequest::operator()()
{
    LogPrint(BCLog::VALIDATION, "%s: validating request=%s\n", __func__, GetId());
    auto res = m_validation_layer.ValidateInternal(m_block, m_force_processing);
    LogPrint(BCLog::VALIDATION, "%s: validation result request=%s block_valid=%d is_new=%d\n",
        __func__, GetId(), res.block_valid, res.is_new);

    m_promise.set_value(res);
    if (m_on_ready) {
        m_on_ready();
    }
}

std::string BlockValidationRequest::GetId() const
{
    return strprintf("BlockValidationRequest[%s]", m_block->GetHash().ToString());
}

void ValidationLayer::Start()
{
    assert(!m_thread || !m_thread->IsActive());
    m_thread = std::unique_ptr<ValidationThread>(new ValidationThread(m_validation_queue));
}

void ValidationLayer::Stop()
{
    assert(m_thread && m_thread->IsActive());
    m_thread->Terminate();
}

std::future<BlockValidationResponse> ValidationLayer::SubmitForValidation(const std::shared_ptr<const CBlock> block, bool force_processing, std::function<void()> on_ready)
{
    BlockValidationRequest* req = new BlockValidationRequest(*this, block, force_processing, on_ready);
    return SubmitForValidation<BlockValidationResponse>(req);
}

BlockValidationResponse ValidationLayer::Validate(const std::shared_ptr<const CBlock> block, bool force_processing)
{
    return SubmitForValidation(block, force_processing).get();
}

BlockValidationResponse ValidationLayer::ValidateInternal(const std::shared_ptr<const CBlock> block, bool force_processing) const
{
    bool is_new = false;
    bool block_valid = ProcessNewBlock(m_chainparams, block, force_processing, &is_new);
    return BlockValidationResponse(block_valid, is_new);
};
