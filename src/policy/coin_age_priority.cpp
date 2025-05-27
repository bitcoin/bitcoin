// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/coin_age_priority.h>

#include <coins.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <txmempool.h>
#include <util/check.h>
#include <validation.h>

using node::BlockAssembler;

unsigned int CalculateModifiedSize(const CTransaction& tx, unsigned int nTxSize)
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    Assert(nTxSize > 0);
    for (std::vector<CTxIn>::const_iterator it(tx.vin.begin()); it != tx.vin.end(); ++it)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)it->scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    return nTxSize;
}

double ComputePriority2(double inputs_coin_age, unsigned int mod_vsize)
{
    if (mod_vsize == 0) return 0.0;

    return inputs_coin_age / mod_vsize;
}

double ReversePriority2(const double coin_age_priority, const unsigned int mod_vsize)
{
    return coin_age_priority * mod_vsize;
}

CoinAgeCache GetCoinAge(const CTransaction &tx, const CCoinsViewCache& view, int nHeight)
{
    CoinAgeCache r{COIN_AGE_CACHE_ZERO};
    if (tx.IsCoinBase()) {
        return r;
    }
    for (const CTxIn& txin : tx.vin)
    {
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            continue;
        }
        if (coin.nHeight <= nHeight) {
            r.inputs_coin_age += (double)(coin.out.nValue) * (nHeight - coin.nHeight);
            r.in_chain_input_value += coin.out.nValue;
        }
    }
    return r;
}

void CTxMemPoolEntry::UpdateCachedPriority(unsigned int currentHeight, CAmount valueInCurrentBlock)
{
    int heightDiff = int(currentHeight) - int(cachedHeight);
    double deltaPriority = ((double)heightDiff*inChainInputValue)/nModSize;
    cachedPriority += deltaPriority;
    cachedHeight = currentHeight;
    inChainInputValue += valueInCurrentBlock;
    assert(MoneyRange(inChainInputValue));
}

struct update_priority
{
    update_priority(unsigned int _height, CAmount _value) :
        height(_height), value(_value)
    {}

    void operator() (CTxMemPoolEntry &e)
    { e.UpdateCachedPriority(height, value); }

    private:
        unsigned int height;
        CAmount value;
};

void CTxMemPool::UpdateDependentPriorities(const CTransaction &tx, unsigned int nBlockHeight, bool addToChain)
{
    LOCK(cs);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        auto it = mapNextTx.find(COutPoint(tx.GetHash(), i));
        if (it == mapNextTx.end())
            continue;
        uint256 hash = it->second->GetHash();
        txiter iter = mapTx.find(hash);
        mapTx.modify(iter, update_priority(nBlockHeight, addToChain ? tx.vout[i].nValue : -tx.vout[i].nValue));
    }
}

double
CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const
{
    // This will only return accurate results when currentHeight >= the heights
    // at which all the in-chain inputs of the tx were included in blocks.
    // Typical usage of GetPriority with chainActive.Height() will ensure this.
    int heightDiff = currentHeight - cachedHeight;
    double deltaPriority = ((double)heightDiff*inChainInputValue)/nModSize;
    double dResult = cachedPriority + deltaPriority;
    if (dResult < 0) // This should only happen if it was called with an invalid height
        dResult = 0;
    return dResult;
}

#ifndef BUILDING_FOR_LIBBITCOINKERNEL
// We want to sort transactions by coin age priority
typedef std::pair<double, CTxMemPool::txiter> TxCoinAgePriority;

struct TxCoinAgePriorityCompare
{
    bool operator()(const TxCoinAgePriority& a, const TxCoinAgePriority& b)
    {
        if (a.first == b.first)
            return CompareTxMemPoolEntryByScore()(*(b.second), *(a.second)); //Reverse order to make sort less than
        return a.first < b.first;
    }
};

bool BlockAssembler::isStillDependent(const CTxMemPool& mempool, CTxMemPool::txiter iter)
{
    assert(iter != mempool.mapTx.end());
    for (const auto& parent : iter->GetMemPoolParentsConst()) {
        auto parent_it = mempool.mapTx.iterator_to(parent);
        if (!inBlock.count(parent_it)) {
            return true;
        }
    }
    return false;
}

bool BlockAssembler::TestForBlock(CTxMemPool::txiter iter)
{
    uint64_t packageSize = iter->GetSizeWithAncestors();
    int64_t packageSigOps = iter->GetSigOpCostWithAncestors();
    if (!TestPackage(packageSize, packageSigOps)) {
        // If the block is so close to full that no more txs will fit
        // or if we've tried more than 50 times to fill remaining space
        // then flag that the block is finished
        if (nBlockWeight > m_options.nBlockMaxWeight - 400 || nBlockSigOpsCost > MAX_BLOCK_SIGOPS_COST - 8 || lastFewTxs > 50) {
             blockFinished = true;
             return false;
        }
        // Once we're within 4000 weight of a full block, only look at 50 more txs
        // to try to fill the remaining space.
        if (nBlockWeight > m_options.nBlockMaxWeight - 4000) {
            ++lastFewTxs;
        }
        return false;
    }

    CTxMemPool::setEntries package;
    package.insert(iter);
    if (!TestPackageTransactions(package)) {
        if (nBlockSize > m_options.nBlockMaxSize - 100 || lastFewTxs > 50) {
            blockFinished = true;
            return false;
        }
        if (nBlockSize > m_options.nBlockMaxSize - 1000) {
            ++lastFewTxs;
        }
        return false;
    }

    return true;
}

void BlockAssembler::addPriorityTxs(const CTxMemPool& mempool, int &nPackagesSelected)
{
    AssertLockHeld(mempool.cs);

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    uint64_t nBlockPrioritySize = gArgs.GetIntArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    if (m_options.nBlockMaxSize < nBlockPrioritySize) {
        nBlockPrioritySize = m_options.nBlockMaxSize;
    }

    if (nBlockPrioritySize <= 0) {
        return;
    }

    bool fSizeAccounting = fNeedSizeAccounting;
    fNeedSizeAccounting = true;

    // This vector will be sorted into a priority queue:
    std::vector<TxCoinAgePriority> vecPriority;
    TxCoinAgePriorityCompare pricomparer;
    std::map<CTxMemPool::txiter, double, CompareIteratorByHash> waitPriMap;
    typedef std::map<CTxMemPool::txiter, double, CompareIteratorByHash>::iterator waitPriIter;
    double actualPriority = -1;

    vecPriority.reserve(mempool.mapTx.size());
    for (auto mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi) {
        double dPriority = mi->GetPriority(nHeight);
        CAmount dummy;
        mempool.ApplyDeltas(mi->GetTx().GetHash(), dPriority, dummy);
        vecPriority.emplace_back(dPriority, mi);
    }
    std::make_heap(vecPriority.begin(), vecPriority.end(), pricomparer);

    CTxMemPool::txiter iter;
    while (!vecPriority.empty() && !blockFinished) { // add a tx from priority queue to fill the blockprioritysize
        iter = vecPriority.front().second;
        actualPriority = vecPriority.front().first;
        std::pop_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
        vecPriority.pop_back();

        // If tx already in block, skip
        if (inBlock.count(iter)) {
            assert(false); // shouldn't happen for priority txs
            continue;
        }

        // If tx is dependent on other mempool txs which haven't yet been included
        // then put it in the waitSet
        if (isStillDependent(mempool, iter)) {
            waitPriMap.insert(std::make_pair(iter, actualPriority));
            continue;
        }

        // If this tx fits in the block add it, otherwise keep looping
        if (TestForBlock(iter)) {
            AddToBlock(mempool, iter);

            ++nPackagesSelected;

            // If now that this txs is added we've surpassed our desired priority size
            // or have dropped below the minimum priority threshold, then we're done adding priority txs
            if (nBlockSize >= nBlockPrioritySize || actualPriority <= MINIMUM_TX_PRIORITY) {
                break;
            }

            // This tx was successfully added, so
            // add transactions that depend on this one to the priority queue to try again
            for (const auto& child : iter->GetMemPoolChildrenConst())
            {
                auto child_it = mempool.mapTx.iterator_to(child);
                waitPriIter wpiter = waitPriMap.find(child_it);
                if (wpiter != waitPriMap.end()) {
                    vecPriority.emplace_back(wpiter->second, child_it);
                    std::push_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
                    waitPriMap.erase(wpiter);
                }
            }
        }
    }
    fNeedSizeAccounting = fSizeAccounting;
}
#endif
