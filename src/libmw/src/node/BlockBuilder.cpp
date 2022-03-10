#include <mw/node/BlockBuilder.h>
#include <mw/consensus/Aggregation.h>
#include <mw/consensus/KernelSumValidator.h>
#include <mw/consensus/Params.h>
#include <mw/consensus/Weight.h>

#include <unordered_set>
#include <numeric>

MW_NAMESPACE

bool BlockBuilder::AddTransaction(const Transaction::CPtr& pTransaction, const std::vector<PegInCoin>& pegins)
{
    // Check weight
    uint64_t weight = Weight::Calculate(pTransaction->GetBody());
    if ((weight + m_weight) > mw::MAX_BLOCK_WEIGHT) {
        LOG_ERROR("Exceeds max block weight");
        return false;
    }
    
    // Verify pegin amount matches
    const uint64_t actual_amount = pTransaction->GetPegInAmount();
    const uint64_t expected_amount = std::accumulate(pegins.cbegin(), pegins.cend(), (uint64_t)0,
        [](const uint64_t sum, const PegInCoin& pegin) { return sum + pegin.GetAmount(); }
    );
    if (actual_amount != expected_amount) {
        LOG_ERROR("Mismatched pegin amount");
        return false;
    }

    // Verify pegin kernels are unique
    std::unordered_set<mw::Hash> pegin_ids;
    for (const PegInCoin& pegin : pegins) {
        if (pegin_ids.find(pegin.GetKernelID()) != pegin_ids.end()) {
            LOG_ERROR("Duplicate pegin kernels");
            return false;
        }

        pegin_ids.insert(pegin.GetKernelID());
    }

    // Verify pegin outputs are included
    std::vector<PegInCoin> pegin_coins = pTransaction->GetPegIns();
    if (pegin_coins.size() != pegins.size()) {
        LOG_ERROR("Mismatched pegin count");
        return false;
    }

    for (const PegInCoin& pegin : pegin_coins) {
        if (pegin_ids.find(pegin.GetKernelID()) == pegin_ids.end()) {
            LOG_ERROR_F("Pegin kernel {} not found", pegin.GetKernelID());
            return false;
        }
    }

    // Validate transaction
    try {
        pTransaction->Validate();
    } catch (std::exception& e) {
        LOG_DEBUG_F("Failed to add transaction {}. Error: {}", pTransaction, e.what());
    }

    // Make sure all inputs are available.
    for (const Input& input : pTransaction->GetInputs()) {
        if (!m_pCoinsView->HasCoin(input.GetOutputID())) {
            LOG_ERROR_F("Input {} not found on chain", input.GetOutputID());
            return false;
        }
    }

    // Make sure no duplicate outputs already on chain.
    for (const Output& output : pTransaction->GetOutputs()) {
        if (m_pCoinsView->HasCoin(output.GetOutputID())) {
            LOG_ERROR_F("Output {} already on chain", output.GetOutputID());
            return false;
        }
    }

    // Aggregate transactions
    auto pAggregated = pTransaction;
    if (m_pAggregated != nullptr) {
        pAggregated = Aggregation::Aggregate({ m_pAggregated, pTransaction });
    }

    if (pAggregated == nullptr) {
        LOG_ERROR("Failed to aggregate transaction");
        return false;
    }

    m_pAggregated = pAggregated;
    m_weight = Weight::Calculate(pAggregated->GetBody());

    return true;
}

mw::Block::Ptr BlockBuilder::BuildBlock() const
{
    mw::CoinsViewCache cache(m_pCoinsView);

    std::vector<mw::Transaction::CPtr> txs;
    if (m_pAggregated != nullptr) {
        txs.push_back(m_pAggregated);
    }

    return cache.BuildNextBlock(m_height, txs);
}

END_NAMESPACE