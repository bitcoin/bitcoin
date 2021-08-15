// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <miner.h>

#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <net.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <pos.h>
#include <primitives/transaction.h>
#include <timedata.h>
#include <util/moneystr.h>
#include <util/system.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

#include <algorithm>
#include <utility>

unsigned int nMinerSleep = STAKER_POLLING_PERIOD;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams, pblock->IsProofOfStake());

    return nNewTime - nOldTime;
}

void RegenerateCommitments(CBlock& block)
{
    CMutableTransaction tx{*block.vtx.at(0)};
    tx.vout.erase(tx.vout.begin() + GetWitnessCommitmentIndex(block));
    block.vtx.at(0) = MakeTransactionRef(tx);

    GenerateCoinbaseCommitment(block, WITH_LOCK(cs_main, return LookupBlockIndex(block.hashPrevBlock)), Params().GetConsensus());

    block.hashMerkleRoot = BlockMerkleRoot(block);
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CTxMemPool& mempool, const CChainParams& params, const Options& options)
    : chainparams(params),
      m_mempool(mempool)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
}

static BlockAssembler::Options DefaultOptions()
{
    // Block resource limits
    // If -blockmaxweight is not given, limit to DEFAULT_BLOCK_MAX_WEIGHT
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    CAmount n = 0;
    if (gArgs.IsArgSet("-blockmintxfee") && ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n)) {
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CTxMemPool& mempool, const CChainParams& params)
    : BlockAssembler(mempool, params, DefaultOptions()) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}

Optional<int64_t> BlockAssembler::m_last_block_num_txs{nullopt};
Optional<int64_t> BlockAssembler::m_last_block_weight{nullopt};

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fProofOfStake, int64_t* pTotalFees, int32_t nTime, bool fAddTxs)
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    CBlock* const pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    // Add dummy coinstake tx as second transaction
    if(fProofOfStake)
        pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, m_mempool.cs);
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    uint32_t txProofTime = nTime == 0 ? GetAdjustedTime() : nTime;
    if(fProofOfStake)
        txProofTime &= ~STAKE_TIMESTAMP_MASK;
    pblock->nTime = txProofTime;
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization).
    // Note that the mempool would accept transactions with witness data before
    // IsWitnessEnabled, but we would only ever mine blocks after IsWitnessEnabled
    // unless there is a massive block reorganization with the witness softfork
    // not activated.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus());

    if (fAddTxs) {
        int nPackagesSelected = 0;
        int nDescendantsUpdated = 0;
        addPackageTxs(nPackagesSelected, nDescendantsUpdated);
    }

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    if (fProofOfStake)
    {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
        coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    }
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

    // Create coinstake transaction.
    if(fProofOfStake)
    {
        CMutableTransaction coinstakeTx;
        coinstakeTx.vout.resize(2);
        coinstakeTx.vout[0].SetEmpty();
        coinstakeTx.vout[1].scriptPubKey = scriptPubKeyIn;
        pblock->vtx[1] = MakeTransactionRef(std::move(coinstakeTx));

        //this just makes CBlock::IsProofOfStake to return true
        //real prevoutstake info is filled in later in SignBlock
        pblock->prevoutStake.n = 0;

    }

    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus(), fProofOfStake);
    pblocktemplate->vTxFees[0] = -nFees;

    if (pTotalFees)
        *pTotalFees = nFees;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(), fProofOfStake);
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    BlockValidationState state;
    if (!fProofOfStake && !TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, state.ToString()));
    }

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblocktemplate->block.vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        m_mempool.CalculateDescendants(it, descendants);
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
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert(it != m_mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
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
void BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = m_mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != m_mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty()) {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != m_mempool.mapTx.get<ancestor_score>().end() &&
            SkipMapTxEntry(m_mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == m_mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = m_mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
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

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

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

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        m_mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
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
        SortForBlock(ancestors, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
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
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce));
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Proof of Stake miner
//

//
// Looking for suitable coins for creating new block.
//

bool CheckStake(const std::shared_ptr<const CBlock> pblock, CWallet& wallet, ChainstateManager* chainman)
{
    uint256 proofHash, hashTarget;
    uint256 hashBlock = pblock->GetHash();

    if(!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());

    // verify hash target and signature of coinstake tx
    BlockValidationState state;
    if (!CheckProofOfStake(::BlockIndex()[pblock->hashPrevBlock], state, *pblock->vtx[1], pblock->nBits, pblock->nTime, proofHash, hashTarget, ::ChainstateActive().CoinsTip()))
        return error("CheckStake() : proof-of-stake checking failed");

    //// debug print
    LogPrint(BCLog::COINSTAKE, "CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
    LogPrint(BCLog::COINSTAKE, "%s\n", pblock->ToString());
    LogPrint(BCLog::COINSTAKE, "out %s\n", FormatMoney(pblock->vtx[1]->GetValueOut()));

    // Found a solution
    {
        if (pblock->hashPrevBlock != ::ChainActive().Tip()->GetBlockHash())
            return error("CheckStake() : generated block is stale");

        for(const CTxIn& vin : pblock->vtx[1]->vin) {
            if (wallet.IsSpent(vin.prevout.hash, vin.prevout.n)) {
                return error("CheckStake() : generated block became invalid due to stake UTXO being spent");
            }
        }
    }

    // Process this block the same as if we had received it from another node
    bool fNewBlock = false;
    if (!chainman->ProcessNewBlock(Params(), pblock, true, &fNewBlock))
        return error("CheckStake() : ProcessBlock, block not accepted");

    return true;
}

void ThreadStakeMiner(CWallet *pwallet, CConnman* connman, ChainstateManager* chainman, CTxMemPool* mempool)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Make this thread recognisable as the mining thread
    std::string threadName = "bpsstake";
    if (pwallet && pwallet->GetName() != "")
    {
        threadName = threadName + "-" + pwallet->GetName();
    }
    RenameThread(threadName.c_str());

    bool fTryToSync = true;
    bool regtestMode = Params().MineBlocksOnDemand();
    if (regtestMode) {
        nMinerSleep = 30000; //limit regtest to 30s, otherwise it'll create 2 blocks per second
    }

    std::set<std::pair<const CWalletTx*,unsigned int> > setCoins;
    uint256 chainTipForCoins;

    while (true)
    {
        while (pwallet->IsLocked() || !pwallet->m_enabled_staking)
        {
            pwallet->m_last_coin_stake_search_interval = 0;
            UninterruptibleSleep(std::chrono::milliseconds{10000});
        }
        // Check if the last PoW block has been mined yet
        if (::ChainActive().Tip()->nHeight < Params().GetConsensus().nLastPOWBlock) {
            UninterruptibleSleep(std::chrono::milliseconds{Params().GetConsensus().nPowTargetSpacing * 1000});// sleep one block
            continue;
        }
        // Don't disable PoS mining for no connections if in regtest mode
        if (!regtestMode && !gArgs.GetBoolArg("-emergencystaking", false)) {
            while (connman->GetNodeCount(CConnman::CONNECTIONS_ALL) < 4 || ::ChainstateActive().IsInitialBlockDownload()) {
                pwallet->m_last_coin_stake_search_interval = 0;
                fTryToSync = true;
                UninterruptibleSleep(std::chrono::milliseconds{1000});
            }
            if (fTryToSync) {
                fTryToSync = false;
                if (connman->GetNodeCount(CConnman::CONNECTIONS_ALL) < 4 ||
                    ::ChainActive().Tip()->GetBlockTime() < GetTime() - Params().GetConsensus().nPowTargetSpacing ||
                    !::ChainActive().Tip()->HaveTxsDownloaded() ||
                    !::ChainActive().Tip()->IsValid(BLOCK_VALID_TRANSACTIONS)) {
                    UninterruptibleSleep(std::chrono::milliseconds{Params().GetConsensus().nPowTargetSpacing / 10 * 1000});
                    continue;
                }
            }
        }
        //
        // Select the suitable coins
        //
        if (chainTipForCoins != ::ChainActive().Tip()->GetBlockHash()) {
            int64_t start_time = GetTimeMillis();
            LogPrint(BCLog::COINSTAKE, "Chain tip changed since previous coin selection, selecting new coins for staking...\n");
            LOCK(pwallet->cs_wallet);
            setCoins.clear();
            chainTipForCoins = ::ChainActive().Tip()->GetBlockHash();
            pwallet->SelectCoinsForStaking(setCoins);
            LogPrint(BCLog::COINSTAKE, "Selecting coins for staking completed in %15dms\n", GetTimeMillis() - start_time);
        } else {
            LogPrint(BCLog::COINSTAKE, "Chain tip unchanged since previous coin selection, using previously selected coins...\n");
        }

        //
        // Create new block
        //
        if (setCoins.size() > 0)
        {
            int64_t nTotalFees = 0;
            // First just create an empty block. No need to process transactions until we know we can create a block
            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(*mempool, Params()).CreateNewBlock(CScript(), true, &nTotalFees, 0, false));
            if (!pblocktemplate.get()) {
                LogPrintf("ThreadStakeMiner(): Failed to create block template; thread exiting...\n");
                return;
            }

            CBlockIndex* pindexPrev = ::ChainActive().Tip();

            uint32_t beginningTime=GetAdjustedTime();
            beginningTime &= ~STAKE_TIMESTAMP_MASK;
            for (uint32_t i=beginningTime;i<beginningTime + MAX_STAKE_LOOKAHEAD;i+=STAKE_TIMESTAMP_MASK+1) {
                // The information is needed for status bar to determine if the staker is trying to create block and when it will be created approximately,
                if (pwallet->m_last_coin_stake_search_time == 0) pwallet->m_last_coin_stake_search_time = GetAdjustedTime(); // startup timestamp
                // nLastCoinStakeSearchInterval > 0 mean that the staker is running
                pwallet->m_last_coin_stake_search_interval = i - pwallet->m_last_coin_stake_search_time;

                // Try to sign a block (this also checks for a PoS stake)
                pblocktemplate->block.nTime = i;
                std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>(pblocktemplate->block);
                if (SignBlock(pblock, *pwallet, nTotalFees, i, setCoins)) {
                    // increase priority so we can build the full PoS block ASAP to ensure the timestamp doesn't expire
                    SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);

                    if (::ChainActive().Tip()->GetBlockHash() != pblock->hashPrevBlock) {
                        //another block was received while building ours, scrap progress
                        LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
                        break;
                    }
                    // Create a block that's properly populated with transactions
                    std::unique_ptr<CBlockTemplate> pblocktemplatefilled(
                            BlockAssembler(*mempool, Params()).CreateNewBlock(pblock->vtx[1]->vout[1].scriptPubKey, true, &nTotalFees, i));
                    if (!pblocktemplatefilled.get()) {
                        LogPrintf("ThreadStakeMiner(): Failed to create block template; thread exiting...\n");
                        return;
                    }
                    if (::ChainActive().Tip()->GetBlockHash() != pblock->hashPrevBlock) {
                        //another block was received while building ours, scrap progress
                        LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
                        break;
                    }
                    // Sign the full block and use the timestamp from earlier for a valid stake
                    std::shared_ptr<CBlock> pblockfilled = std::make_shared<CBlock>(pblocktemplatefilled->block);
                    if (SignBlock(pblockfilled, *pwallet, nTotalFees, i, setCoins)) {
                        // Should always reach here unless we spent too much time processing transactions and the timestamp is now invalid
                        // CheckStake also does CheckBlock and AcceptBlock to propogate it to the network
                        bool validBlock = false;
                        while (!validBlock) {
                            if (::ChainActive().Tip()->GetBlockHash() != pblockfilled->hashPrevBlock) {
                                //another block was received while building ours, scrap progress
                                LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
                                break;
                            }
                            //check timestamps
                            if (pblockfilled->GetBlockTime() <= pindexPrev->GetBlockTime() ||
                                FutureDrift(pblockfilled->GetBlockTime()) < pindexPrev->GetBlockTime()) {
                                LogPrintf("ThreadStakeMiner(): Valid PoS block took too long to create and has expired\n");
                                break; //timestamp too late, so ignore
                            }
                            if (pblockfilled->GetBlockTime() > FutureDrift(GetAdjustedTime())) {
                                if (gArgs.IsArgSet("-aggressivestaking")) {
                                    //if being agressive, then check more often to publish immediately when valid. This might allow you to find more blocks, 
                                    //but also increases the chance of broadcasting invalid blocks and getting DoS banned by nodes,
                                    //or receiving more stale/orphan blocks than normal. Use at your own risk.
                                    UninterruptibleSleep(std::chrono::milliseconds{100});
                                } else {
                                    //too early, so wait 3 seconds and try again
                                    UninterruptibleSleep(std::chrono::milliseconds{3000});
                                }
                                continue;
                            }
                            validBlock=true;
                        }
                        if (validBlock) {
                            CheckStake(pblockfilled, *pwallet, chainman);
                            // Update the search time when new valid block is created, needed for status bar icon
                            pwallet->m_last_coin_stake_search_time = pblockfilled->GetBlockTime();
                        }
                        break;
                    }
                    //return back to low priority
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);
                }
            }
        }
        UninterruptibleSleep(std::chrono::milliseconds{nMinerSleep});
    }
}

void StakeBTCs(bool fStake, CWallet *pwallet, CConnman* connman, ChainstateManager* chainman, CTxMemPool* mempool, boost::thread_group*& stakeThread)
{
    if (stakeThread != nullptr)
    {
        stakeThread->interrupt_all();
        delete stakeThread;
        stakeThread = nullptr;
    }

    if(fStake)
    {
        stakeThread = new boost::thread_group();
        stakeThread->create_thread(boost::bind(&ThreadStakeMiner, pwallet, connman, chainman, mempool));
    }
}
#endif
