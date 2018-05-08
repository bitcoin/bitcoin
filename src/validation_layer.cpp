// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation_layer.h>

/**
 * Process an incoming block. This only returns after the best known valid
 * block is made active. Note that it does not, however, guarantee that the
 * specific block passed to it has been checked for validity!
 *
 * If you want to *possibly* get feedback on whether pblock is valid, you must
 * install a CValidationInterface (see validationinterface.h) - this will have
 * its BlockChecked method called whenever *any* block completes validation.
 *
 * Note that we guarantee that either the proof-of-work is valid on pblock, or
 * (and possibly also) BlockChecked will have been called.
 *
 * May not be called in a
 * validationinterface callback.
 *
 * @param[in]   pblock  The block we want to process.
 * @param[in]   fForceProcessing Process this block even if unrequested; used for non-network block sources and whitelisted peers.
 * @param[out]  fNewBlock A boolean which is set to indicate if the block was first received via this call
 * @return True if state.IsValid()
 */
bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock);

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
