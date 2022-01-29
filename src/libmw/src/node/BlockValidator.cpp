#include <mw/node/BlockValidator.h>
#include <mw/exceptions/ValidationException.h>
#include <set>
#include <unordered_map>

bool BlockValidator::ValidateBlock(
    const mw::Block::CPtr& pBlock,
    const std::vector<PegInCoin>& pegInCoins,
    const std::vector<PegOutCoin>& pegOutCoins) noexcept
{
    assert(pBlock != nullptr);

    try {
        pBlock->Validate();

        ValidatePegInCoins(pBlock, pegInCoins);
        ValidatePegOutCoins(pBlock, pegOutCoins);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to validate {}. Error: {}", *pBlock, e);
    }

    return false;
}

void BlockValidator::ValidatePegInCoins(
    const mw::Block::CPtr& pBlock,
    const std::vector<PegInCoin>& pegInCoins)
{
    std::unordered_map<mw::Hash, CAmount> pegInAmounts;
    std::for_each(
        pegInCoins.cbegin(), pegInCoins.cend(),
        [&pegInAmounts](const PegInCoin& coin) {
            pegInAmounts.insert({coin.GetKernelID(), coin.GetAmount()});
        }
    );

    auto pegin_coins = pBlock->GetPegIns();
    if (pegin_coins.size() != pegInAmounts.size()) {
        ThrowValidation(EConsensusError::PEGIN_MISMATCH);
    }

    for (const auto& pegin : pegin_coins) {
        auto pIter = pegInAmounts.find(pegin.GetKernelID());
        if (pIter == pegInAmounts.end() || pegin.GetAmount() != pIter->second) {
            ThrowValidation(EConsensusError::PEGIN_MISMATCH);
        }
    }
}

void BlockValidator::ValidatePegOutCoins(
    const mw::Block::CPtr& pBlock,
    const std::vector<PegOutCoin>& pegOutCoins)
{
    std::vector<PegOutCoin> mweb_pegouts = pBlock->GetPegOuts();
    if (mweb_pegouts.size() != pegOutCoins.size()) {
        ThrowValidation(EConsensusError::PEGOUT_MISMATCH);
    }

    // We use a multiset since there can be multiple pegouts with the same scriptPubKey and amount.
    std::multiset<PegOutCoin> hogex_pegouts(pegOutCoins.begin(), pegOutCoins.end());
    for (const auto& pegout : mweb_pegouts) {
        auto iter = hogex_pegouts.find(pegout);
        if (iter == hogex_pegouts.end()) {
            ThrowValidation(EConsensusError::PEGOUT_MISMATCH);
        }
        
        hogex_pegouts.erase(iter);
    }
}