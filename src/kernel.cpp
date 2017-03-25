// Copyright (c) 2012-2017 The Peercoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "kernel.h"
#include "db.h"
#include "txdb.h"

using namespace std;

// Protocol switch time of v0.3 kernel protocol
unsigned int nProtocolV03SwitchTime     = 1363800000;
unsigned int nProtocolV03TestSwitchTime = 1359781000;
// Protocol switch time of v0.4 kernel protocol
unsigned int nProtocolV04SwitchTime     = 1399300000;
unsigned int nProtocolV04TestSwitchTime = 1395700000;
// Protocol switch time of v0.5 kernel protocol
unsigned int nProtocolV05SwitchTime     = 1461700000;
unsigned int nProtocolV05TestSwitchTime = 1447700000;


// Modifier interval: time to elapse before new modifier is computed
// Set to 6-hour for production network and 20-minute for test network
unsigned int nModifierInterval = MODIFIER_INTERVAL;

// Hard checkpoints of stake modifiers to ensure they are deterministic
static std::map<int, unsigned int> mapStakeModifierCheckpoints =
    boost::assign::map_list_of
    ( 0, 0x0e00670bu )
    ( 19080, 0xad4e4d29u )
    ( 30583, 0xdc7bf136u )
    ( 99999, 0xf555cfd2u )
    (219999, 0x91b7444du )
    ;

// Whether the given coinstake is subject to new v0.3 protocol
bool IsProtocolV03(unsigned int nTimeCoinStake)
{
    return (nTimeCoinStake >= (fTestNet? nProtocolV03TestSwitchTime : nProtocolV03SwitchTime));
}

// Whether the given block is subject to new v0.4 protocol
bool IsProtocolV04(unsigned int nTimeBlock)
{
    return (nTimeBlock >= (fTestNet? nProtocolV04TestSwitchTime : nProtocolV04SwitchTime));
}

// Whether the given transaction is subject to new v0.5 protocol
bool IsProtocolV05(unsigned int nTimeTx)
{
    return (nTimeTx >= (fTestNet? nProtocolV05TestSwitchTime : nProtocolV05SwitchTime));
}

// Whether a given block is subject to new v0.6 protocol
bool IsProtocolV06(unsigned int nTimeBlock)
{
    // TODO decide on upgrade timing
    return false;
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64& nStakeModifier, int64& nModifierTime)
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

// Get selection interval section (in seconds)
static int64 GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert (nSection >= 0 && nSection < 64);
    return (nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64 GetStakeModifierSelectionInterval()
{
    int64 nSelectionInterval = 0;
    for (int nSection=0; nSection<64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(
    vector<pair<int64, uint256> >& vSortedByTimestamp,
    map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64 nSelectionIntervalStop, uint64 nStakeModifierPrev,
    const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    BOOST_FOREACH(const PAIRTYPE(int64, uint256)& item, vSortedByTimestamp)
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
        uint256 hashProof = pindex->IsProofOfStake()? pindex->hashProofOfStake : pindex->GetBlockHash();
        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
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
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
    }
    if (fDebug && GetBoolArg("-printstakemodifier"))
        printf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString().c_str());
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
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64& nStakeModifier, bool& fGeneratedStakeModifier)
{
    const CBlockIndex* pindexPrev = pindexCurrent->pprev;
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    }
    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64 nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (fDebug)
    {
        printf("ComputeNextStakeModifier: prev modifier=0x%016"PRI64x" time=%s epoch=%u\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str(), (unsigned int)nModifierTime);
    }
    if (nModifierTime / nModifierInterval >= pindexPrev->GetBlockTime() / nModifierInterval)
    {
        if (fDebug)
        {
            printf("ComputeNextStakeModifier: no new interval keep current modifier: pindexPrev nHeight=%d nTime=%u\n", pindexPrev->nHeight, (unsigned int)pindexPrev->GetBlockTime());
        }
        return true;
    }
    if (nModifierTime / nModifierInterval >= pindexCurrent->GetBlockTime() / nModifierInterval)
    {
        // v0.4+ requires current block timestamp also be in a different modifier interval
        if (IsProtocolV04(pindexCurrent->nTime))
        {
            if (fDebug)
            {
                printf("ComputeNextStakeModifier: (v0.4+) no new interval keep current modifier: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
            }
            return true;
        }
        else
        {
            if (fDebug)
            {
                printf("ComputeNextStakeModifier: v0.3 modifier at block %s not meeting v0.4+ protocol: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->GetBlockHash().ToString().c_str(), pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
            }
        }
    }

    // Sort candidate blocks by timestamp
    vector<pair<int64, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / STAKE_TARGET_SPACING);
    int64 nSelectionInterval = GetStakeModifierSelectionInterval();
    int64 nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64 nStakeModifierNew = 0;
    int64 nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound<min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n",
                nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (fDebug && GetBoolArg("-printstakemodifier"))
    {
        string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        BOOST_FOREACH(const PAIRTYPE(uint256, const CBlockIndex*)& item, mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        }
        printf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    if (fDebug)
    {
        printf("ComputeNextStakeModifier: new modifier=0x%016"PRI64x" time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// V0.5: Stake modifier used to hash for a stake kernel is chosen as the stake
// modifier that is (nStakeMinAge minus a selection interval) earlier than the
// stake, thus at least a selection interval later than the coin generating the // kernel, as the generating coin is from at least nStakeMinAge ago.
static bool GetKernelStakeModifierV05(unsigned int nTimeTx, uint64& nStakeModifier, int& nStakeModifierHeight, int64& nStakeModifierTime, bool fPrintProofOfStake)
{
    const CBlockIndex* pindex = pindexBest;
    nStakeModifierHeight = pindex->nHeight;
    nStakeModifierTime = pindex->GetBlockTime();
    int64 nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();

    if (nStakeModifierTime + nStakeMinAge - nStakeModifierSelectionInterval <= (int64) nTimeTx)
    {
        // Best block is still more than
        // (nStakeMinAge minus a selection interval) older than kernel timestamp
        if (fPrintProofOfStake)
            return error("GetKernelStakeModifier() : best block %s at height %d too old for stake",
                pindex->GetBlockHash().ToString().c_str(), pindex->nHeight);
        else
            return false;
    }
    // loop to find the stake modifier earlier by 
    // (nStakeMinAge minus a selection interval)
    while (nStakeModifierTime + nStakeMinAge - nStakeModifierSelectionInterval >(int64) nTimeTx)
    {
        if (!pindex->pprev)
        {   // reached genesis block; should not happen
            return error("GetKernelStakeModifier() : reached genesis block");
        }
        pindex = pindex->pprev;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

// V0.3: Stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifierV03(uint256 hashBlockFrom, uint64& nStakeModifier, int& nStakeModifierHeight, int64& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64 nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
    const CBlockIndex* pindex = pindexFrom;
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        if (!pindex->pnext)
        {   // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (pindex->GetBlockTime() + nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
                return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight, hashBlockFrom.ToString().c_str());
            else
                return false;
        }
        pindex = pindex->pnext;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

// Get the stake modifier specified by the protocol to hash for a stake kernel
static bool GetKernelStakeModifier(uint256 hashBlockFrom, unsigned int nTimeTx, uint64& nStakeModifier, int& nStakeModifierHeight, int64& nStakeModifierTime, bool fPrintProofOfStake)
{
    if (IsProtocolV05(nTimeTx))
        return GetKernelStakeModifierV05(nTimeTx, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake);
    else
        return GetKernelStakeModifierV03(hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake);
}

// ppcoin kernel protocol
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula
//     hash(nStakeModifier + txPrev.block.nTime + txPrev.offset + txPrev.nTime + txPrev.vout.n + nTime) < bnTarget * nCoinDayWeight
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coin age one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier: 
//       (v0.5) uses dynamic stake modifier around 21 days before the kernel,
//              versus static stake modifier about 9 days after the staked
//              coin (txPrev) used in v0.3
//       (v0.3) scrambles computation to make it very difficult to precompute
//              future proof-of-stake at the time of the coin's confirmation
//       (v0.2) nBits (deprecated): encodes all past block timestamps
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
bool CheckStakeKernelHash(unsigned int nBits, const CBlockHeader& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev.nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();
    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64 nValueIn = txPrev.vout[prevout.n].nValue;
    // v0.3 protocol kernel hash weight starts from 0 at the 30-day min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low
    int64 nTimeWeight = min((int64)nTimeTx - txPrev.nTime, (int64)STAKE_MAX_AGE) - (IsProtocolV03(nTimeTx)? nStakeMinAge : 0);
    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);
    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64 nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64 nStakeModifierTime = 0;
    if (IsProtocolV03(nTimeTx))  // v0.3 protocol
    {
        if (!GetKernelStakeModifier(blockFrom.GetHash(), nTimeTx, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
            return false;
        ss << nStakeModifier;
    }
    else // v0.2 protocol
    {
        ss << nBits;
    }

    ss << nTimeBlockFrom << nTxPrevOffset << txPrev.nTime << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    if (fPrintProofOfStake)
    {
        if (IsProtocolV03(nTimeTx))
            printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
                nStakeModifier, nStakeModifierHeight,
                DateTimeStrFormat(nStakeModifierTime).c_str(),
                mapBlockIndex[blockFrom.GetHash()]->nHeight,
                DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : check protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            IsProtocolV05(nTimeTx)? "0.5" : (IsProtocolV03(nTimeTx)? "0.3" : "0.2"),
            IsProtocolV03(nTimeTx)? nStakeModifier : (uint64) nBits,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString().c_str());
    }

    // Now check if proof-of-stake hash meets target protocol
    if (CBigNum(hashProofOfStake) > bnCoinDayWeight * bnTargetPerCoinDay)
        return false;
    if (fDebug && !fPrintProofOfStake)
    {
        if (IsProtocolV03(nTimeTx))
            printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
                nStakeModifier, nStakeModifierHeight, 
                DateTimeStrFormat(nStakeModifierTime).c_str(),
                mapBlockIndex[blockFrom.GetHash()]->nHeight,
                DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : pass protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            IsProtocolV03(nTimeTx)? "0.3" : "0.2",
            IsProtocolV03(nTimeTx)? nStakeModifier : (uint64) nBits,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            hashProofOfStake.ToString().c_str());
    }
    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CValidationState &state, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake)
{
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];

    // Transaction index is required to get to block header
    if (!fTxIndex)
        return error("CheckProofOfStake() : transaction index not available");

    // First try finding the previous transaction in database
    CCoinsViewCache view(*pcoinsTip, true);
    CCoins coins;
    if (!view.GetCoins(txin.prevout.hash, coins))
        return state.DoS(1, error("CheckProofOfStake() : txPrev not found")); // previous transaction not in main chain, may occur during initial download

    // Verify signature
    if (!VerifySignature(coins, tx, 0, true, 0))
        return state.DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(txin.prevout.hash, postx))
        return error("CheckProofOfStake() : tx index not found");  // tx index not found

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransaction txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file, postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception &e) {
            return error("%s() : deserialize or I/O error in CheckProofOfStake()", __PRETTY_FUNCTION__);
        }
        if (txPrev.GetHash() != txin.prevout.hash)
            return error("%s() : txid mismatch in CheckProofOfStake()", __PRETTY_FUNCTION__);
    }

    if (!CheckStakeKernelHash(nBits, header, postx.nTxOffset + sizeof(CBlockHeader), txPrev, txin.prevout, tx.nTime, hashProofOfStake, fDebug))
        return state.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str())); // may occur during initial download or if behind on block chain sync

    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64 nTimeBlock, int64 nTimeTx)
{
    if (IsProtocolV03(nTimeTx))  // v0.3 protocol
        return (nTimeBlock == nTimeTx);
    else // v0.2 protocol
        return ((nTimeTx <= nTimeBlock) && (nTimeBlock <= nTimeTx + nMaxClockDrift));
}

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert (pindex->pprev || pindex->GetBlockHash() == hashGenesisBlock);
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    uint256 hashChecksum = Hash(ss.begin(), ss.end());
    hashChecksum >>= (256 - 32);
    return hashChecksum.Get64();
}

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    if (fTestNet) return true; // Testnet has no checkpoints
    if (mapStakeModifierCheckpoints.count(nHeight))
        return nStakeModifierChecksum == mapStakeModifierCheckpoints[nHeight];
    return true;
}
