// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2013-2015 The Novacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "kernel.h"
#include "kernel_worker.h"
#include "txdb.h"

extern unsigned int nStakeMaxAge;
extern unsigned int nStakeTargetSpacing;

using namespace std;


// Protocol switch time for fixed kernel modifier interval
unsigned int nModifierSwitchTime  = 1413763200;    // Mon, 20 Oct 2014 00:00:00 GMT
unsigned int nModifierTestSwitchTime = 1397520000; // Tue, 15 Apr 2014 00:00:00 GMT

// Note: user must upgrade before the protocol switch deadline, otherwise it's required to
//   re-download the blockchain. The timestamp of upgrade is recorded in the blockchain 
//   database.
unsigned int nModifierUpgradeTime = 0;

typedef std::map<int, unsigned int> MapModifierCheckpoints;

// Hard checkpoints of stake modifiers to ensure they are deterministic
static std::map<int, unsigned int> mapStakeModifierCheckpoints =
    boost::assign::map_list_of
        ( 0, 0x0e00670bu )
        ( 12661, 0x5d84115du )
        (143990, 0x9c592c78u )
        (149000, 0x48f2bdc4u )
        (160000, 0x789df0f0u )
        (200000, 0x01ec1503u )
        (221047, 0x0b39ef50u )
        (243100, 0xe928d83au )
    ;

// Hard checkpoints of stake modifiers to ensure they are deterministic (testNet)
static std::map<int, unsigned int> mapStakeModifierCheckpointsTestNet =
    boost::assign::map_list_of
        ( 0, 0x0e00670bu )
    ;

// Pregenerated entropy bits table (from genesis to #9689)
//
// Bits are packed into array of 256 bit integers:
//
// * array index calculated as nHeight / 256
// * position of bit is calculated as nHeight & 0xFF.
//
const uint256 entropyStore[] = {
    uint256("0x4555b4dcc1d690ddd9b810c90c66e82b18bf4f43cc887246c418383ec120a5ab"),
    uint256("0xaa6d1198412fa77608addf6549c9198a22155e8afd7a9ded6179f6b7cfc66b0c"),
    uint256("0x9442fabfa4116fb14a9769c2eea003845a1f5c3a0260f36b497d68f3a3cd4078"),
    uint256("0x0e769042a9a98e42388195d699574b822d06515f7053ad884c53d7ee059f05b1"),
    uint256("0x7005aac20baf70251aebfe3f1b95987d83ef1e3e6963de8fed601d4dd07bf7cf"),
    uint256("0x58952c5c3de188f2e33c38d3f53d7bf44f9bc545a4289d266696273fa821be66"),
    uint256("0x50b6c2ed780c08aaec3f7665b1b6004206243e3866456fc910b83b52d07eeb63"),
    uint256("0x563841eefca85ba3384986c58100408ae3f1ba2ac727e1ac910ce154a06c702f"),
    uint256("0x79275b03938b3e27a9b01a7f7953c6c487c58355f5d4169accfbb800213ffd13"),
    uint256("0xd783f2538b3ed18f135af90adc687c5646d93aeaeaabc6667be94f7aa0a2d366"),
    uint256("0xb441d0c175c40c8e88b09d88ea008af79cbed2d28219427d2e72fda682974db8"),
    uint256("0x3204c43bd41f2e19628af3b0c9aca3db15bca4c8705d51056e7b17a319c04715"),
    uint256("0x7e80e6ab7857d8f2f261a0a49c783bd800b365b8c9b85cc0e13f73904b0dcaa9"),
    uint256("0xefaaee60ed82d2ad145c0e347941fdb131eb8fd289a45eef07121a93f283c5f1"),
    uint256("0x3efc86e4334da332c1fd4c12513c40cff689f3efdc7f9913230822adacdda4f9"),
    uint256("0xf0d6b8f38599a017fa35d1fbbf9ef51eca5ebc5b286aadba40c4c3e1d9bace0c"),
    uint256("0x286a67f27323486036a0a92d35382fc8963c0c00bad331723318b4b9fdb2b56e"),
    uint256("0xecbfaaa6567c54f08c4d5bd0118a2d7b58740f42cbfc73aa1536c1f5f76de87c"),
    uint256("0xf9a4de1c5c46520de5aaf10d3796cf0e27ddce98b3398357f5726a949664e308"),
    uint256("0xd75e6c4dc4be08401e3478d2467d9ab96a62af4f255c04a82c41af0de0a487bb"),
    uint256("0x1a82c3bc6ad6047294c16571b5e2b7316c97bf8813e7da15798b9820d67e39f2"),
    uint256("0xb49be0080de564e01829ded7e7971979565a741c5975dc9978dcc020192d396c"),
    uint256("0x0d8eed113be67663b5a15a0625a9b49792b5ea59c005c4f405914877acab7000"),
    uint256("0x8f9d46e2bc05a218ffa942965b747056197d393b097085523640cd59e07fe7c7"),
    uint256("0x7a63ab40bc7f40ac2ebe9ede438d97b45fa6ed6f8419016da8d5f7a670111dda"),
    uint256("0x63fbcc080448c43d6cf915c958314feff7a95a52ba43a68c05fc281d3a522d25"),
    uint256("0xf834cf824c326d3ea861ea1e85dc3289265e37045981e28208e7344a7f8081d7"),
    uint256("0xb4edc22ec98cc49b2f5af5bae3f52f5e6058280f74f2c432c2dd89ae49acceb8"),
    uint256("0x0fe596037dcf81bf5c64f39755261c404ed088af5c8c31dd7549b6657ee92365"),
    uint256("0xbbad51a0aeba254b01d18c328de9e932b9b859b61e622c325d64e2211b5e413d"),
    uint256("0xabf0194cc787be938bc51c7fdf1cae4ec79e65ebab8fa8b8f40541c44ef384b0"),
    uint256("0x83bc12d6fdbd3e854cb91c4ca7dfba3c38e8714121af88c8a8abdb33e5002438"),
    uint256("0x71a2513026cabaedcbe55aeb6dc8049e5b763a3f54f10c33dd333624f764b38c"),
    uint256("0xee6725632ff5c025dff6a18cd059875dcae20f399b03bccba13d9d5fcf6d9d9a"),
    uint256("0xa168a2741d1e7e50cc74b79f695c25ffd3576e6bd61353c2a20e569fd63b2dac"),
    uint256("0x6e462d2a87bfde9398b6747f94a8ed6a01e4d96c5b4372df5c910c106c48bd13"),
    uint256("0x8eeb696181957c4b22434028990f49cb30006827c73860e77e2eecf5c38be99d"),
    uint256("0x3188aaa65877b166f05cdc48f55b1f77a7d6fb221c395596d990ae5647e9ba96")
};

// Whether the given block is subject to new modifier protocol
bool IsFixedModifierInterval(unsigned int nTimeBlock)
{
    return (nTimeBlock >= (fTestNet? nModifierTestSwitchTime : nModifierSwitchTime));
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
static bool SelectBlockFromCandidates(vector<pair<int64_t, uint256> >& vSortedByTimestamp, map<uint256, const CBlockIndex*>& mapSelectedBlocks,
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
            *pindexSelected = pindex;
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = pindex;
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
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    const CBlockIndex* pindexPrev = pindexCurrent->pprev;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    }

    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (fDebug)
    {
        printf("ComputeNextStakeModifier: prev modifier=0x%016" PRIx64 " time=%s epoch=%u\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str(), (unsigned int)nModifierTime);
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
        // fixed interval protocol requires current block timestamp also be in a different modifier interval
        if (IsFixedModifierInterval(pindexCurrent->nTime))
        {
            if (fDebug)
            {
                printf("ComputeNextStakeModifier: no new interval keep current modifier: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
            }
            return true;
        }
        else
        {
            if (fDebug)
            {
                printf("ComputeNextStakeModifier: old modifier at block %s not meeting fixed modifier interval: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->GetBlockHash().ToString().c_str(), pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
            }
        }
    }

    // Sort candidate blocks by timestamp
    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / nStakeTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
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
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound<min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
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
        printf("ComputeNextStakeModifier: new modifier=0x%016" PRIx64 " time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
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

bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier)
{
    int nStakeModifierHeight;
    int64_t nStakeModifierTime;

    return GetKernelStakeModifier(hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, false);
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
bool CheckStakeKernelHash(uint32_t nBits, const CBlock& blockFrom, uint32_t nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, uint32_t nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev.nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    uint32_t nTimeBlockFrom = blockFrom.GetBlockTime();
    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;

    uint256 hashBlockFrom = blockFrom.GetHash();

    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * GetWeight((int64_t)txPrev.nTime, (int64_t)nTimeTx) / COIN / nOneDay;
    targetProofOfStake = (bnCoinDayWeight * bnTargetPerCoinDay).getuint256();

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;

    if (!GetKernelStakeModifier(hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
        return false;
    ss << nStakeModifier;

    ss << nTimeBlockFrom << nTxPrevOffset << txPrev.nTime << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    if (fPrintProofOfStake)
    {
        printf("CheckStakeKernelHash() : using modifier 0x%016" PRIx64 " at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
            nStakeModifier, nStakeModifierHeight,
            DateTimeStrFormat(nStakeModifierTime).c_str(),
            mapBlockIndex[hashBlockFrom]->nHeight,
            DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : check modifier=0x%016" PRIx64 " nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashTarget=%s hashProof=%s\n",
            nStakeModifier,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            targetProofOfStake.ToString().c_str(), hashProofOfStake.ToString().c_str());
    }

    // Now check if proof-of-stake hash meets target protocol
    if (CBigNum(hashProofOfStake) > bnCoinDayWeight * bnTargetPerCoinDay)
        return false;
    if (fDebug && !fPrintProofOfStake)
    {
        printf("CheckStakeKernelHash() : using modifier 0x%016" PRIx64 " at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
            nStakeModifier, nStakeModifierHeight, 
            DateTimeStrFormat(nStakeModifierTime).c_str(),
            mapBlockIndex[hashBlockFrom]->nHeight,
            DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : pass modifier=0x%016" PRIx64 " nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashTarget=%s hashProof=%s\n",
            nStakeModifier,
            nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
            targetProofOfStake.ToString().c_str(), hashProofOfStake.ToString().c_str());
    }
    return true;
}

// Scan given kernel for solution
bool ScanKernelForward(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::vector<std::pair<uint256, uint32_t> > &solutions)
{
    // TODO: custom threads amount

    uint32_t nThreads = boost::thread::hardware_concurrency();
    if (nThreads == 0)
    {
       nThreads = 1;
       printf("Warning: hardware_concurrency() failed in %s:%d\n", __FILE__, __LINE__);
    }
    uint32_t nPart = (SearchInterval.second - SearchInterval.first) / nThreads;

    KernelWorker *workers = new KernelWorker[nThreads];

    boost::thread_group group;
    for(size_t i = 0; i < nThreads; i++)
    {
        uint32_t nBegin = SearchInterval.first + nPart * i;
        uint32_t nEnd = SearchInterval.first + nPart * (i + 1);
        workers[i] = KernelWorker(kernel, nBits, nInputTxTime, nValueIn, nBegin, nEnd);
        boost::function<void()> workerFnc = boost::bind(&KernelWorker::Do, &workers[i]);
        group.create_thread(workerFnc);
    }

    group.join_all();
    solutions.clear();

    for(size_t i = 0; i < nThreads; i++)
    {
        std::vector<std::pair<uint256, uint32_t> > ws = workers[i].GetSolutions();
        solutions.insert(solutions.end(), ws.begin(), ws.end());
    }

    delete [] workers;

    if (solutions.size() == 0)
    {
        // no solutions
        return false;
    }

    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake)
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

#ifndef USE_LEVELDB
    txdb.Close();
#endif

    // Verify signature
    if (!VerifySignature(txPrev, tx, 0, MANDATORY_SCRIPT_VERIFY_FLAGS, 0))
        return tx.DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return fDebug? error("CheckProofOfStake() : read block failed") : false; // unable to read block of previous transaction

    if (!CheckStakeKernelHash(nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, txin.prevout, tx.nTime, hashProofOfStake, targetProofOfStake, fDebug))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str())); // may occur during initial download or if behind on block chain sync

    return true;
}

// Get stake modifier checksum
uint32_t GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert (pindex->pprev || pindex->GetBlockHash() == (!fTestNet ? hashGenesisBlock : hashGenesisBlockTestNet));
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    uint256 hashChecksum = Hash(ss.begin(), ss.end());
    hashChecksum >>= (256 - 32);
    return static_cast<uint32_t>(hashChecksum.Get64());
}

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, uint32_t nStakeModifierChecksum)
{
    MapModifierCheckpoints& checkpoints = (fTestNet ? mapStakeModifierCheckpointsTestNet : mapStakeModifierCheckpoints);

    if (checkpoints.count(nHeight))
        return nStakeModifierChecksum == checkpoints[nHeight];
    return true;
}
