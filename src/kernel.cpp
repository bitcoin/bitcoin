// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "kernel.h"
#include "txdb.h"

// Get time weight
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{
    // Kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low

    return nIntervalEnd - nIntervalBeginning - nStakeMinAge;
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifierThin(const CBlockThinIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifierThin: null pindex");
    
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifierThin: no generation at genesis block");
    
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert (nSection >= 0 && nSection < 64);
    return (nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection=0; nSection<64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp, std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    BOOST_FOREACH(const PAIRTYPE(int64_t, uint256)& item, vSortedByTimestamp)
    {
        if (!mapBlockIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString().c_str());
        
        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;
        
        // compute the selection hash by hashing its proof-hash and the
        // previous proof-of-stake modifier
        CDataStream ss(SER_GETHASH, 0);
        ss << pindex->hashProof << nStakeModifierPrev;
        uint256 hashSelection = Hash(ss.begin(), ss.end());
        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        } else
        if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        };
    };
    
    if (fDebugPoS)
        LogPrintf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString().c_str());
    
    return fSelected;
}

static bool SelectBlockFromCandidatesThin(std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp, std::map<uint256, const CBlockThinIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockThinIndex** pindexSelected)
{
    bool fSelected = false;
    uint256 hashBest = 0;
    *pindexSelected = (const CBlockThinIndex*) 0;
    BOOST_FOREACH(const PAIRTYPE(int64_t, uint256)& item, vSortedByTimestamp)
    {
        if (!mapBlockThinIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString().c_str());
        
        
        const CBlockThinIndex* pindex = mapBlockThinIndex[item.second];
        
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;
        
        // compute the selection hash by hashing its proof-hash and the
        // previous proof-of-stake modifier
        CDataStream ss(SER_GETHASH, 0);
        ss << pindex->hashProof << nStakeModifierPrev;
        uint256 hashSelection = Hash(ss.begin(), ss.end());
        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;
        
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockThinIndex*) pindex;
        } else
        if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockThinIndex*) pindex;
        };
    };
    
    if (fDebugPoS)
        LogPrintf("SelectBlockFromCandidatesThin: selection hash=%s\n", hashBest.ToString().c_str());
    
    return fSelected;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every 
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    };
    
    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    
    if (fDebug)
    {
        LogPrintf("ComputeNextStakeModifier: prev modifier=0x%016x time=%s\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str());
    };
    
    if (nModifierTime / nModifierInterval >= pindexPrev->GetBlockTime() / nModifierInterval)
        return true;

    // Sort candidate blocks by timestamp
    std::vector<std::pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / GetTargetSpacing(pindexPrev->nHeight));
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(std::make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    std::map<uint256, const CBlockIndex*> mapSelectedBlocks;
    
    for (int nRound=0; nRound<std::min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(std::make_pair(pindex->GetBlockHash(), pindex));
        if (fDebugPoS)
            LogPrintf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    };

    // Print selection map for visualization of the selected blocks
    if (fDebugPoS && LogAcceptCategory("stakemodifier"))
    {
        std::string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        };
        
        BOOST_FOREACH(const PAIRTYPE(uint256, const CBlockIndex*)& item, mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        };
        
        LogPrintf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    } else
    if (fDebugPoS)
    {
        LogPrintf("ComputeNextStakeModifier: new modifier=0x%016x time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    };

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

bool ComputeNextStakeModifierThin(const CBlockThinIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    };
    
    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifierThin(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    
    if (fDebug)
    {
        LogPrintf("ComputeNextStakeModifierThin: prev modifier=0x%016x time=%s\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str());
    };
    
    if (nModifierTime / nModifierInterval >= pindexPrev->GetBlockTime() / nModifierInterval)
        return true;

    // Sort candidate blocks by timestamp
    std::vector<std::pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / GetTargetSpacing(pindexPrev->nHeight));
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
    const CBlockThinIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(std::make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    };
    
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    std::map<uint256, const CBlockThinIndex*> mapSelectedBlocks;
    
    for (int nRound=0; nRound<std::min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidatesThin(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifierThin: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(std::make_pair(pindex->GetBlockHash(), pindex));
        if (fDebugPoS)
            LogPrintf("ComputeNextStakeModifierThin: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    };

    // Print selection map for visualization of the selected blocks
    if (fDebugPoS && fThinFullIndex && LogAcceptCategory("stakemodifier"))
    {
        std::string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        };
        
        BOOST_FOREACH(const PAIRTYPE(uint256, const CBlockThinIndex*)& item, mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        };
        
        LogPrintf("ComputeNextStakeModifierThin: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    } else
    if (fDebugPoS)
    {
        LogPrintf("ComputeNextStakeModifierThin: new modifier=0x%016x time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    };

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
};

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlockFrom);
    if (mi == mapBlockIndex.end())
        return error("GetKernelStakeModifier() : block not indexed");
    
    const CBlockIndex* pindexFrom = mi->second;
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
    const CBlockIndex* pindex = pindexFrom;
    
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        if (!pindex->pnext)
        {
            // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (pindex->GetBlockTime() + nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
            {
                return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight, hashBlockFrom.ToString().c_str());
            } else
            {
                return false;
            };
        };
        
        pindex = pindex->pnext;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        };
    };
    
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

static bool GetKernelStakeModifierThinIt(CBlockThinIndex* pindex, int64_t nFoundTime, int64_t nStakeModifierSelectionInterval, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < nFoundTime + nStakeModifierSelectionInterval)
    {
        if (!pindex->pnext)
        {
            // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (pindex->GetBlockTime() + nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
            {
                //return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                //    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight, hashBlockFrom.ToString().c_str());
                return error("GetKernelStakeModifier() : reached best block %s at height %d",
                    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight);
            } else
            {
                return false;
            };
        };
        
        pindex = pindex->pnext;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        };
    };
    
    nStakeModifier = pindex->nStakeModifier;
    return true;
};

static bool GetKernelStakeModifierThin(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    
    int64_t nFoundTime;
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
    
    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(hashBlockFrom);
    if (mi == mapBlockThinIndex.end())
    {
        if (fThinFullIndex
            || !pindexRear)
            return error("GetKernelStakeModifierThin() : block not indexed");
        
        CTxDB txdb("r");
        CDiskBlockThinIndex diskindex;
        if (!txdb.ReadBlockThinIndex(hashBlockFrom, diskindex)
            || diskindex.hashNext == 0)
            return error("GetKernelStakeModifierThin() : block not in db %s", hashBlockFrom.ToString().c_str());
        
        nStakeModifierHeight = diskindex.nHeight;
        nStakeModifierTime = (int64_t)diskindex.nTime;
        nFoundTime = (int64_t)diskindex.nTime;
        
        // TODO, check mapBlockThinIndex
        while (nStakeModifierTime < nFoundTime + nStakeModifierSelectionInterval)
        {
            if (diskindex.hashNext == 0)
            {
                // reached best block; may happen if node is behind on block chain
                if (fPrintProofOfStake || ((int64_t)diskindex.nTime + nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
                {
                    uint256 hash = diskindex.GetBlockHash();
                    return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                        hash.ToString().c_str(), diskindex.nHeight, hashBlockFrom.ToString().c_str());
                } else
                {
                    return false;
                };
            };
            
            
            if ((int64_t)diskindex.nTime > pindexRear->GetBlockTime())
            {
                // -- back into the index window
                mi = mapBlockThinIndex.find(diskindex.hashNext);
                
                if (mi != mapBlockThinIndex.end())
                {
                    CBlockThinIndex* pindex = mi->second;
                    return GetKernelStakeModifierThinIt(pindex, nFoundTime, nStakeModifierSelectionInterval, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake);
                };
            };
            
            if (!txdb.ReadBlockThinIndex(diskindex.hashNext, diskindex)
                || diskindex.hashNext == 0)
                return error("GetKernelStakeModifierThin() : block not indexed %s", diskindex.hashNext.ToString().c_str());
            
            //pindex = pindex->pnext;
            if (diskindex.GeneratedStakeModifier())
            {
                nStakeModifierHeight = diskindex.nHeight;
                nStakeModifierTime = (int64_t)diskindex.nTime;
            };
        };
        
        nStakeModifier = diskindex.nStakeModifier;
        return true;
    };
    
    CBlockThinIndex* pindexFrom = mi->second;
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    nFoundTime = pindexFrom->GetBlockTime();
    
    CBlockThinIndex* pindex = pindexFrom;
    return GetKernelStakeModifierThinIt(pindex, nFoundTime, nStakeModifierSelectionInterval, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake);
}





// ppcoin kernel protocol
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula
//     hash(nStakeModifier + txPrev.block.nTime + txPrev.offset + txPrev.nTime + txPrev.vout.n + nTime) < bnTarget * nCoinDayWeight
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coin age one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier: scrambles computation to make it very difficult to precompute
//                  future proof-of-stake at the time of the coin's confirmation
//   txPrev.block.nTime: prevent nodes from guessing a good timestamp to
//                       generate transaction for future advantage
//   txPrev.offset: offset of txPrev inside block, to reduce the chance of 
//                  nodes generating coinstake at the same time
//   txPrev.nTime: reduce the chance of nodes generating coinstake at the same
//                 time
//   txPrev.vout.n: output number of txPrev, to reduce the chance of nodes
//                  generating coinstake at the same time
//   block/tx hash should not be used here as they can be generated in vast
//   quantities so as to generate blocks faster, degrading the system back into
//   a proof-of-work situation.
//
static inline bool CheckStakeKernelHashV1(unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev.nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");
    
    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();
    
    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");
    
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;

    uint256 hashBlockFrom = blockFrom.GetHash();

    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * GetWeight((int64_t)txPrev.nTime, (int64_t)nTimeTx) / COIN / (24 * 60 * 60);
    targetProofOfStake = (bnCoinDayWeight * bnTargetPerCoinDay).getuint256();

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;
    
    if (nNodeMode == NT_FULL)
    {
        if (!GetKernelStakeModifier(hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
            return false;
    } else
    {
        if (!GetKernelStakeModifierThin(hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
            return false;
    };
    
    ss << nStakeModifier;
    
    ss << nTimeBlockFrom << nTxPrevOffset << txPrev.nTime << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    
    if (fPrintProofOfStake)
    {
        int nHeight = 0;
        if (nNodeMode == NT_FULL)
            nHeight = mapBlockIndex[hashBlockFrom]->nHeight;
        else
        if (fThinFullIndex) // otherwise index may not be in the window
            nHeight = mapBlockThinIndex[hashBlockFrom]->nHeight;
        
        LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
            nStakeModifier, nStakeModifierHeight,
            DateTimeStrFormat(nStakeModifierTime).c_str(),
            nHeight,
            DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        LogPrintf("CheckStakeKernelHash() : check modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            nStakeModifier,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString().c_str());
        
        CBigNum nTry = CBigNum(hashProofOfStake);
        CBigNum nTar = bnCoinDayWeight * bnTargetPerCoinDay;
        LogPrintf("try    %s\n                    target %s\n", nTry.ToString().c_str(), nTar.ToString().c_str());
    };
    
    
    // Now check if proof-of-stake hash meets target protocol
    if (CBigNum(hashProofOfStake) > bnCoinDayWeight * bnTargetPerCoinDay)
        return false;
    
    if (fDebug && !fPrintProofOfStake)
    {
        //int nHeight = nNodeMode == NT_FULL ? mapBlockIndex[hashBlockFrom]->nHeight :  mapBlockThinIndex[hashBlockFrom]->nHeight;
        int nHeight = 0;
        if (nNodeMode == NT_FULL)
            nHeight = mapBlockIndex[hashBlockFrom]->nHeight;
        else
        if (fThinFullIndex) // otherwise index may not be in the window
            nHeight = mapBlockThinIndex[hashBlockFrom]->nHeight;
        
        LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
            nStakeModifier, nStakeModifierHeight, 
            DateTimeStrFormat(nStakeModifierTime).c_str(),
            nHeight,
            DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        LogPrintf("CheckStakeKernelHash() : pass modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            nStakeModifier,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString().c_str());
    };
    
    return true;
}


// BlackCoin kernel protocol
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula
//     hash(nStakeModifier + txPrev.block.nTime + txPrev.nTime + txPrev.vout.hash + txPrev.vout.n + nTime) < bnTarget * nWeight
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coins one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier: scrambles computation to make it very difficult to precompute
//                   future proof-of-stake
//   txPrev.block.nTime: prevent nodes from guessing a good timestamp to
//                       generate transaction for future advantage
//   txPrev.nTime: slightly scrambles computation
//   txPrev.vout.hash: hash of txPrev, to reduce the chance of nodes
//                     generating coinstake at the same time
//   txPrev.vout.n: output number of txPrev, to reduce the chance of nodes
//                  generating coinstake at the same time
//   nTime: current timestamp
//   block/tx hash should not be used here as they can be generated in vast
//   quantities so as to generate blocks faster, degrading the system back into
//   a proof-of-work situation.
//
static inline bool CheckStakeKernelHashV2(CStakeModifier* pStakeMod, unsigned int nBits, unsigned int nTimeBlockFrom, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev.nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    // Base target
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    // Weighted target
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;
    CBigNum bnWeight = CBigNum(nValueIn);
    bnTarget *= bnWeight;

    targetProofOfStake = bnTarget.getuint256();

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    ss << pStakeMod->nModifier << nTimeBlockFrom << txPrev.nTime << prevout.hash << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());

    if (fPrintProofOfStake)
    {
        LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from timestamp=%s\n",
            pStakeMod->nModifier, pStakeMod->nHeight,
            DateTimeStrFormat(pStakeMod->nTime),
            DateTimeStrFormat(nTimeBlockFrom));
        LogPrintf("CheckStakeKernelHash() : check modifier=0x%016x nTimeBlockFrom=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            pStakeMod->nModifier,
            nTimeBlockFrom, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString());
    };
    
    // Now check if proof-of-stake hash meets target protocol
    if (CBigNum(hashProofOfStake) > bnTarget)
        return false;
    
    if (fDebug && !fPrintProofOfStake)
    {
        LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from timestamp=%s\n",
            pStakeMod->nModifier, pStakeMod->nHeight,
            DateTimeStrFormat(pStakeMod->nTime),
            DateTimeStrFormat(nTimeBlockFrom));
        LogPrintf("CheckStakeKernelHash() : pass modifier=0x%016x nTimeBlockFrom=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            pStakeMod->nModifier,
            nTimeBlockFrom, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString());
    };
    
    return true;
}


bool CheckStakeKernelHash(int nPrevHeight, CStakeModifier* pStakeMod, unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake)
{
    if (Params().IsProtocolV2(nPrevHeight+1))
        return CheckStakeKernelHashV2(pStakeMod, nBits, blockFrom.GetBlockTime(), txPrev, prevout, nTimeTx, hashProofOfStake, targetProofOfStake, fPrintProofOfStake);
    return CheckStakeKernelHashV1(nBits, blockFrom, nTxPrevOffset, txPrev, prevout, nTimeTx, hashProofOfStake, targetProofOfStake, fPrintProofOfStake);
}


// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake)
{
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];

    // First try finding the previous transaction in database
    CTxDB txdb("r");
    CTransaction txPrev;
    CTxIndex txindex;
    if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: read txPrev failed"));  // previous transaction not in main chain, may occur during initial download

    // Verify signature
    if (!VerifySignature(txPrev, tx, 0, SCRIPT_VERIFY_NONE, 0))
        return tx.DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return fDebug? error("CheckProofOfStake() : read block failed") : false; // unable to read block of previous transaction
    
    CStakeModifier stakeMod(pindexPrev->nStakeModifier, pindexPrev->nHeight, pindexPrev->nTime);
    if (!CheckStakeKernelHash(pindexPrev->nHeight, &stakeMod, nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, txin.prevout, tx.nTime, hashProofOfStake, targetProofOfStake, fDebugPoS))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str())); // may occur during initial download or if behind on block chain sync

    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock, int64_t nTimeTx)
{
    if (Params().IsProtocolV2(nHeight))
        return (nTimeBlock == nTimeTx) && ((nTimeTx & STAKE_TIMESTAMP_MASK) == 0);
    else
        return (nTimeBlock == nTimeTx);
}

bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime)
{
    uint256 hashProofOfStake, targetProofOfStake;
    
    CTxDB txdb("r");
    CTransaction txPrev;
    CTxIndex txindex;
    if (!txPrev.ReadFromDisk(txdb, prevout, txindex))
        return false;
    
    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return false;
    
    if (block.GetBlockTime() + nStakeMinAge > nTime)
        return false; // only count coins meeting min age requirement
    
    if (pBlockTime)
        *pBlockTime = block.GetBlockTime();
    
    // - workaround for thin mode
    CStakeModifier stakeMod(pindexPrev->nStakeModifier, pindexPrev->nHeight, pindexPrev->nTime);
    return CheckStakeKernelHash(pindexPrev->nHeight, &stakeMod, nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, prevout, nTime, hashProofOfStake, targetProofOfStake, fDebugPoS);
}
