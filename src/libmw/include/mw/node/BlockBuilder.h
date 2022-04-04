#pragma once

#include <mw/common/Macros.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/tx/PegInCoin.h>
#include <mw/node/CoinsView.h>
#include <memory>

MW_NAMESPACE

class BlockBuilder
{
public:
    using Ptr = std::shared_ptr<BlockBuilder>;

    /// <summary>
    /// Constructs a new BlockBuilder for assembling an MW ext block incrementally (tx by tx).
    /// </summary>
    /// <param name="height">The height of the block being built.</param>
    /// <param name="view">The CoinsView representing the latest state of the active chain. Must not be null.</param>
    /// <returns>A non-null BlockBuilder</returns>
    BlockBuilder(const uint64_t height, const mw::ICoinsView::Ptr& pCoinsView)
        : m_height(height), m_weight(0), m_pCoinsView(std::make_shared<mw::CoinsViewCache>(pCoinsView)) { }

    bool AddTransaction(const Transaction::CPtr& pTransaction, const std::vector<PegInCoin>& pegins);

    mw::Block::Ptr BuildBlock() const;

private:
    uint64_t m_height;
    uint64_t m_weight;
    mw::CoinsViewCache::Ptr m_pCoinsView;

    std::vector<Transaction::CPtr> m_stagedTxs;
    std::set<Hash> m_stagedOutputs;
};

END_NAMESPACE // mw