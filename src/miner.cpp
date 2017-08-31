// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/tx_verify.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "hash.h"
#include "validation.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/policy.h"
#include "pow.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"
#include "validationinterface.h"

#include <algorithm>
#include <queue>
#include <utility>

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest fee rate of a transaction combined with all
// its ancestors.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
uint64_t nLastBlockWeight = 0;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
    nBlockMaxSize = DEFAULT_BLOCK_MAX_SIZE;
    nRecentTxWindow = (DEFAULT_RECENT_TX_WINDOW + 999) / 1000;
    dRecentTxStaleRate = DEFAULT_RECENT_TX_STALE_RATE;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
    // Limit size to between 1K and MAX_BLOCK_SERIALIZED_SIZE-1K for sanity:
    nBlockMaxSize = std::max<size_t>(1000, std::min<size_t>(MAX_BLOCK_SERIALIZED_SIZE - 1000, options.nBlockMaxSize));
    nRecentTxWindow = options.nRecentTxWindow;
    // 1 - stale rate is the income threshold we want to use for our comparison
    // in CreateNewBlock. Bound it between 0 and 1.
    dIncomeThreshold = std::max<double>(0.0, std::min<double>(1.0, 1 - options.dRecentTxStaleRate));

    // Whether we need to account for byte usage (in addition to weight usage)
    fNeedSizeAccounting = (nBlockMaxSize < MAX_BLOCK_SERIALIZED_SIZE - 1000);
}

static BlockAssembler::Options DefaultOptions(const CChainParams& params)
{
    // Block resource limits
    // If neither -blockmaxsize or -blockmaxweight is given, limit to DEFAULT_BLOCK_MAX_*
    // If only one is given, only restrict the specified resource.
    // If both are given, restrict both.
    BlockAssembler::Options options;
    options.nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
    options.nBlockMaxSize = DEFAULT_BLOCK_MAX_SIZE;
    bool fWeightSet = false;
    if (gArgs.IsArgSet("-blockmaxweight")) {
        options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
        options.nBlockMaxSize = MAX_BLOCK_SERIALIZED_SIZE;
        fWeightSet = true;
    }
    if (gArgs.IsArgSet("-blockmaxsize")) {
        options.nBlockMaxSize = gArgs.GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
        if (!fWeightSet) {
            options.nBlockMaxWeight = options.nBlockMaxSize * WITNESS_SCALE_FACTOR;
        }
    }
    if (gArgs.IsArgSet("-blockmintxfee")) {
        CAmount n = 0;
        ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n);
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    // Convert -recenttxwindow to seconds, since that's what the mempool stores (rounding up).
    options.nRecentTxWindow = (gArgs.GetArg("-recenttxwindow", DEFAULT_RECENT_TX_WINDOW) + 999)/1000;
    // If the user specified a parseable stale rate to use, use it.
    if (!gArgs.IsArgSet("-recenttxstalerate") ||
            !ParseDouble(gArgs.GetArg("-recenttxstalerate", ""), &options.dRecentTxStaleRate)) {
        options.dRecentTxStaleRate = DEFAULT_RECENT_TX_STALE_RATE;
    }

    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions(params)) {}

/*
 * CreateNewBlock:
 *
 * This selects transactions for a new candidate block, and verifies that the
 * resulting block is valid (aside from proof-of-work). If the resulting block
 * is not valid, then an error is thrown.  Relies on mempool consistency (no
 * invalid transactions in the mempool given our current tip).
 *
 * Transaction selection is initially done by selecting highest feerate
 * transactions, taking into account ancestors that are not yet in the
 * candidate block, and adding those transactions with their ancestors to the
 * block.
 *
 * Then, we model that block propagation can be affected by the presence of
 * recently received transactions that may not have propagated across the
 * network (see BIP 152): transactions that have been recently received are
 * removed (and we re-fill the block with non-recent transactions) unless the
 * block income from doing so drops by some percentage.  Miners can view this
 * as a statement: blocks that contain transactions received less than X
 * seconds ago are orphaned with probability Y.  By setting the recently
 * received threshold window to X and the income reduction threshold to Y,
 * CreateNewBlock ought to perform more optimally (and generally result in
 * blocks that propagate quicker).
 */

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx)
{
    int64_t nTimeStart = GetTimeMicros();

    WorkingState workState;

    // Add dummy coinbase tx as first transaction
    workState.pblock->vtx.emplace_back();
    workState.pblocktemplate->vTxFees.push_back(-1); // updated at end
    workState.pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;

    workState.pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        workState.pblock->nVersion = gArgs.GetArg("-blockversion", workState.pblock->nVersion);

    workState.pblock->nTime = GetAdjustedTime();
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : workState.pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus()) && fMineWitnessTx;

    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;

    // First, we add transactions without regard to received time of
    // transactions, based on ancestor feerate score.
    addPackageTxs(workState, nPackagesSelected, nDescendantsUpdated, std::numeric_limits<int64_t>::max(), mapModifiedTx);

    // Duplicate the working state, and remove the recently received
    // transactions from the block.
    WorkingState noRecentWorkState(workState);

    int64_t nTimeCutoff = GetTime() - nRecentTxWindow;
    RemoveRecentTransactionsFromBlockAndUpdatePackages(noRecentWorkState, nTimeCutoff, mapModifiedTx);

    // Re-add to this block, skipping recent transactions from transaction selection.
    addPackageTxs(noRecentWorkState, nPackagesSelected, nDescendantsUpdated, nTimeCutoff, mapModifiedTx);

    int64_t nTime1 = GetTimeMicros();

    // Now compare and decide which block to use
    WorkingState *winner = &workState;
    CAmount blockSubsidy = GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    if (blockSubsidy + noRecentWorkState.nModifiedFees >= dIncomeThreshold * (workState.nModifiedFees + blockSubsidy)) {
        winner = &noRecentWorkState;
    }

    nLastBlockTx = winner->nBlockTx;
    nLastBlockSize = winner->nBlockSize;
    nLastBlockWeight = winner->nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    coinbaseTx.vout[0].nValue = winner->nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    winner->pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    winner->pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*winner->pblock, pindexPrev, chainparams.GetConsensus());
    winner->pblocktemplate->vTxFees[0] = -winner->nFees;

    uint64_t nSerializeSize = GetSerializeSize(*winner->pblock, SER_NETWORK, PROTOCOL_VERSION);
    LogPrintf("CreateNewBlock(): total size: %u block weight: %u txs: %u fees: %ld sigops %d\n", nSerializeSize, GetBlockWeight(*winner->pblock), winner->nBlockTx, winner->nFees, winner->nBlockSigOpsCost);

    // Fill in header
    winner->pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    UpdateTime(winner->pblock, chainparams.GetConsensus(), pindexPrev);
    winner->pblock->nBits          = GetNextWorkRequired(pindexPrev, winner->pblock, chainparams.GetConsensus());
    winner->pblock->nNonce         = 0;
    winner->pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*winner->pblock->vtx[0]);

    CValidationState state;
    if (!TestBlockValidity(state, chainparams, *winner->pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    return std::move(winner->pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(WorkingState &workState, CTxMemPool::setEntries& testSet) const
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (workState.inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(const WorkingState &workState, uint64_t packageSize, int64_t packageSigOpsCost) const
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (workState.nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (workState.nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
// - serialized size (in case -blockmaxsize is in use)
bool BlockAssembler::TestPackageTransactions(WorkingState &workState, const CTxMemPool::setEntries& package) const
{
    uint64_t nPotentialBlockSize = workState.nBlockSize; // only used with fNeedSizeAccounting
    for (const CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
        if (fNeedSizeAccounting) {
            uint64_t nTxSize = ::GetSerializeSize(it->GetTx(), SER_NETWORK, PROTOCOL_VERSION);
            if (nPotentialBlockSize + nTxSize >= nBlockMaxSize) {
                return false;
            }
            nPotentialBlockSize += nTxSize;
        }
    }
    return true;
}

void BlockAssembler::AddToBlock(WorkingState &workState, CTxMemPool::txiter iter) const
{
    workState.pblock->vtx.emplace_back(iter->GetSharedTx());
    workState.pblocktemplate->vTxFees.push_back(iter->GetFee());
    workState.pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    if (fNeedSizeAccounting) {
        workState.nBlockSize += ::GetSerializeSize(iter->GetTx(), SER_NETWORK, PROTOCOL_VERSION);
    }
    workState.nBlockWeight += iter->GetTxWeight();
    ++workState.nBlockTx;
    workState.nBlockSigOpsCost += iter->GetSigOpCost();
    workState.nFees += iter->GetFee();
    workState.nModifiedFees += iter->GetModifiedFee();
    workState.inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

void BlockAssembler::RemoveRecentTransactionsFromBlockAndUpdatePackages(WorkingState &workState, int64_t timeCutoff, indexed_modified_transaction_set &mapModifiedTx)
{
    std::vector<CTransactionRef> vtxCopy;
    vtxCopy.reserve(workState.pblocktemplate->block.vtx.size());
    std::vector<CAmount> vTxFeesCopy;
    vTxFeesCopy.reserve(vtxCopy.size());
    std::vector<int64_t> vTxSigOpsCostCopy;
    vTxSigOpsCostCopy.reserve(vtxCopy.size());

    CTxMemPool::setEntries descendantTransactions;
    for (const CTransactionRef &ptx : workState.pblocktemplate->block.vtx) {
        if (vtxCopy.empty()) {
            // This is the coinbase placeholder; just copy its
            // information over.
            vtxCopy.push_back(ptx);
            vTxFeesCopy.push_back(workState.pblocktemplate->vTxFees[0]);
            vTxSigOpsCostCopy.push_back(workState.pblocktemplate->vTxSigOpsCost[0]);
            continue;
        }
        // TODO: Store the mempool entry iterators in another vector to save this
        // lookup.
        CTxMemPool::txiter it = mempool.mapTx.find(ptx->GetHash());
        assert(it != mempool.mapTx.end());
        // Keep any transactions that are sufficiently old, but skip transactions
        // that depend on removed transactions.  (Note that it's technically
        // possible for a child tx to have a newer arrival time than its
        // parent, eg after a reorg.)
        if (it->GetTime() <= timeCutoff && descendantTransactions.count(it) == 0) {
            vtxCopy.push_back(ptx);
            vTxFeesCopy.push_back(it->GetFee());
            vTxSigOpsCostCopy.push_back(it->GetSigOpCost());
        } else {
            // Update block statistics for transactions being skipped.
            if (fNeedSizeAccounting) {
                workState.nBlockSize -= ::GetSerializeSize(*ptx, SER_NETWORK, PROTOCOL_VERSION);
            }
            workState.nBlockWeight -= it->GetTxWeight();
            --workState.nBlockTx;
            workState.nBlockSigOpsCost -= it->GetSigOpCost();
            workState.nFees -= it->GetFee();
            workState.nModifiedFees -= it->GetModifiedFee();
            workState.inBlock.erase(it);

            // Update the packages to account for this transaction's removal
            // from the block.
            CTxMemPool::setEntries tx_descendants;
            mempool.CalculateDescendants(it, tx_descendants);
            for (auto desc : tx_descendants) {
                modtxiter mit = mapModifiedTx.find(desc);
                // Only descendants that are not in the block should exist in
                // mapModifiedTx
                if (mit != mapModifiedTx.end()) {
                    mapModifiedTx.modify(mit, update_for_parent_removal(it));
                }
            }

            // Add all of this transaction's descendants into the master list
            // of transactions that must be removed (missing inputs).
            descendantTransactions.insert(tx_descendants.begin(), tx_descendants.end());
        }
    }

    // Now we have a new version of the block.  Swap out with what is stored.
    std::swap(workState.pblocktemplate->block.vtx, vtxCopy);
    std::swap(workState.pblocktemplate->vTxFees, vTxFeesCopy);
    std::swap(workState.pblocktemplate->vTxSigOpsCost, vTxSigOpsCostCopy);
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx) const
{
    int nDescendantsUpdated = 0;
    for (const CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}



// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(WorkingState &workState, CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx) const
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || workState.inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries) const
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(WorkingState &workState, int &nPackagesSelected, int &nDescendantsUpdated, int64_t nTimeCutoff, indexed_modified_transaction_set &mapModifiedTx) const
{
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                (mi->GetTime() > nTimeCutoff ||
                 SkipMapTxEntry(workState, mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx))) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();

        // Remove too-recent entries from mapModifiedTx.
        if (modit != mapModifiedTx.get<ancestor_score>().end() && modit->iter->GetTime() > nTimeCutoff) {
            mapModifiedTx.get<ancestor_score>().erase(modit);
            continue;
        }

        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are workState.inBlock, and mapModifiedTx shouldn't
        // contain anything that is workState.inBlock.
        assert(!workState.inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(workState, packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && workState.nBlockWeight >
                    nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(workState, ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(workState, ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, iter, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(workState, sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}
