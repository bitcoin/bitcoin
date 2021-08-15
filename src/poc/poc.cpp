// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poc/poc.h>
#include <chainparams.h>
#include <compat/endian.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <crypto/shabal256.h>
#include <key_io.h>
#include <logging.h>
#include <miner.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <threadinterrupt.h>
#include <timedata.h>
#include <ui_interface.h>
#include <util/time.h>
#include <util/validation.h>
#include <validation.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

#include <cinttypes>
#include <cmath>
#include <exception>
#include <limits>
#include <string>
#include <tuple>
#include <unordered_map>

#include <event2/thread.h>

namespace {

// Generator
struct GeneratorState {
    uint64_t plotterId;
    uint64_t nonce;
    uint64_t best;
    int height;

    CTxDestination dest;
    std::shared_ptr<CKey> privKey;

    GeneratorState() : best(poc::INVALID_DEADLINE) { }
};
typedef std::unordered_map<uint64_t, GeneratorState> Generators; // Generation low 64bits -> GeneratorState
Generators mapGenerators GUARDED_BY(cs_main);

std::shared_ptr<CBlock> CreateBlock(const GeneratorState &generateState)
{
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    try {
        pblocktemplate = BlockAssembler(Params()).CreateNewBlock(GetScriptForDestination(generateState.dest),
            generateState.plotterId, generateState.nonce, generateState.best / ::ChainActive().Tip()->nBaseTarget,
            generateState.privKey);
    } catch (std::exception &e) {
        const char *what = e.what();
        LogPrintf("CreateBlock() fail: %s\n", what ? what : "Catch unknown exception");
    }
    if (!pblocktemplate.get()) 
        return nullptr;

    CBlock *pblock = &pblocktemplate->block;
    return std::make_shared<CBlock>(*pblock);
}

// Mining loop
CThreadInterrupt interruptCheckDeadline;
std::thread threadCheckDeadline;
void CheckDeadlineThread()
{
    util::ThreadRename("bitcoin-checkdeadline");
    while (!interruptCheckDeadline) {
        if (!interruptCheckDeadline.sleep_for(std::chrono::milliseconds(500)))
            break;

        std::shared_ptr<CBlock> pblock;
        CBlockIndex *pTrySnatchTip = nullptr;
        {
            LOCK(cs_main);
            if (!mapGenerators.empty()) {
                if (GetTimeOffset() > MAX_FUTURE_BLOCK_TIME) {
                    LogPrintf("Your computer time maybe abnormal (offset %" PRId64 "). " \
                        "Check your computer time or add -maxtimeadjustment=0 \n", GetTimeOffset());
                }
                int64_t nAdjustedTime = GetAdjustedTime();
                CBlockIndex *pindexTip = ::ChainActive().Tip();
                for (auto it = mapGenerators.cbegin(); it != mapGenerators.cend() && pblock == nullptr; ) {
                    if (pindexTip->GetNextGenerationSignature().GetUint64(0) == it->first) {
                        //! Current round
                        uint64_t deadline = it->second.best / pindexTip->nBaseTarget;
                        if (nAdjustedTime + 1 >= (int64_t)pindexTip->nTime + (int64_t)deadline) {
                            // Forge
                            LogPrint(BCLog::POC, "Generate block: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 ", deadline=%" PRIu64 "\n",
                                it->second.height, it->second.nonce, it->second.plotterId, deadline);
                            pblock = CreateBlock(it->second);
                            if (!pblock) {
                                LogPrintf("Generate block fail: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 ", deadline=%" PRIu64 "\n",
                                    it->second.height, it->second.nonce, it->second.plotterId, deadline);
                            } else {
                                LogPrint(BCLog::POC, "Created block: hash=%s, time=%d\n", pblock->GetHash().ToString(), pblock->nTime);
                            }
                        } else {
                            ++it;
                            continue;
                        }
                    } else if (pindexTip->GetGenerationSignature().GetUint64(0) == it->first) {
                        //! Previous round
                        // Process future post block (MAX_FUTURE_BLOCK_TIME). My deadline is best (highest chainwork).
                        uint64_t mineDeadline = it->second.best / pindexTip->pprev->nBaseTarget;
                        uint64_t tipDeadline = (uint64_t) (pindexTip->GetBlockTime() - pindexTip->pprev->GetBlockTime() - 1);
                        if (mineDeadline <= tipDeadline) {
                            LogPrint(BCLog::POC, "Snatch block: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 ", deadline=%" PRIu64 " <= %" PRIu64 "\n",
                                it->second.height, it->second.nonce, it->second.plotterId, mineDeadline, tipDeadline);

                            //! Try snatch block
                            pTrySnatchTip = pindexTip;
                            break;
                        }
                    }

                    it = mapGenerators.erase(it);
                }

            } else {
                continue;
            }
        }

        //! Try snatch block
        if (pTrySnatchTip != nullptr) {
            assert(pblock == nullptr);
            CValidationState state;
            if (!InvalidateBlock(state, Params(), pTrySnatchTip)) {
                LogPrint(BCLog::POC, "Snatch block fail: invalidate %s got\n\t%s\n", pTrySnatchTip->ToString(), state.GetRejectReason());
            } else {
                {
                    LOCK(cs_main);
                    ResetBlockFailureFlags(pTrySnatchTip);

                    auto itDummyProof = mapGenerators.find(pTrySnatchTip->GetGenerationSignature().GetUint64(0));
                    if (itDummyProof != mapGenerators.end()) {
                        pblock = CreateBlock(itDummyProof->second);
                        if (!pblock) {
                            LogPrintf("Snatch block fail: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 "\n",
                                itDummyProof->second.height, itDummyProof->second.nonce, itDummyProof->second.plotterId);
                        } else if (GetBlockProof(*pblock, Params().GetConsensus()) <= GetBlockProof(*pTrySnatchTip, Params().GetConsensus())) {
                            //! Lowest chainwork, give up
                            LogPrintf("Snatch block give up: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 "\n",
                                itDummyProof->second.height, itDummyProof->second.nonce, itDummyProof->second.plotterId);
                            pblock.reset();
                        } else {
                            LogPrint(BCLog::POC, "Snatch block success: height=%d, hash=%s\n", itDummyProof->second.height, pblock->GetHash().ToString());
                        }
                    }
                    mapGenerators.erase(itDummyProof);
                }

                //! Reset best
                if (!ActivateBestChain(state, Params())) {
                    LogPrintf("Activate best chain fail: %s\n", __func__, FormatStateMessage(state));
                    assert (false);
                }
            }
        }

        if (pblock && !ProcessNewBlock(Params(), pblock, true, nullptr))
            LogPrintf("%s: Process new block fail %s\n", __func__, pblock->ToString());
    }

    LogPrintf("Exit PoC forge thread\n");
}

// Save block signature require private key
typedef std::unordered_map< uint64_t, std::shared_ptr<CKey> > CPrivKeyMap;
CPrivKeyMap mapSignaturePrivKeys;

}

namespace poc {

static constexpr int HASH_SIZE = 32;
static constexpr int HASHES_PER_SCOOP = 2;
static constexpr int SCOOP_SIZE = HASHES_PER_SCOOP * HASH_SIZE; // 2 hashes per scoop
static constexpr int SCOOPS_PER_PLOT = 4096;
static constexpr int PLOT_SIZE = SCOOPS_PER_PLOT * SCOOP_SIZE; // 256KB
static std::unique_ptr<unsigned char> calcDLDataCache(new unsigned char[PLOT_SIZE + 16]); // Global calc cache

//! Thread safe
static uint64_t CalcDL(int nHeight, const uint256& generationSignature, const uint64_t& nPlotterId, const uint64_t& nNonce, const Consensus::Params& params) {
    CShabal256 shabal256;
    uint256 temp;

    // Row data
    const uint64_t plotterId_be = htobe64(nPlotterId);
    const uint64_t nonce_be = htobe64(nNonce);
    unsigned char *const data = calcDLDataCache.get();
    memcpy(data + PLOT_SIZE, (const unsigned char*)&plotterId_be, 8);
    memcpy(data + PLOT_SIZE + 8, (const unsigned char*)&nonce_be, 8);
    for (int i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
        int len = PLOT_SIZE + 16 - i;
        if (len > SCOOPS_PER_PLOT) {
            len = SCOOPS_PER_PLOT;
        }

        shabal256
            .Write(data + i, len)
            .Finalize(data + i - HASH_SIZE);
    }
    // Final
    shabal256
        .Write(data, PLOT_SIZE + 16)
        .Finalize(temp.begin());
    for (int i = 0; i < PLOT_SIZE; i++) {
        data[i] = (unsigned char) (data[i] ^ (temp.begin()[i % HASH_SIZE]));
    }

    // Scoop
    const uint64_t height_be = htobe64(static_cast<uint64_t>(nHeight));
    shabal256
        .Write(generationSignature.begin(), generationSignature.size())
        .Write((const unsigned char*)&height_be, 8)
        .Finalize((unsigned char*)temp.begin());
    const uint32_t scoop = (uint32_t) (temp.begin()[31] + 256 * temp.begin()[30]) % 4096;

    // PoC2 Rearrangement. Swap high hash
    //
    // [0] [1] [2] [3] ... [N-1]
    // [1] <-> [N-1]
    // [2] <-> [N-2]
    // [3] <-> [N-3]
    //
    // Only care hash data of scoop index
    memcpy(data + scoop * SCOOP_SIZE + HASH_SIZE, data + (SCOOPS_PER_PLOT - scoop) * SCOOP_SIZE - HASH_SIZE, HASH_SIZE);

    // Result
    shabal256
        .Write(generationSignature.begin(), generationSignature.size())
        .Write(data + scoop * SCOOP_SIZE, SCOOP_SIZE)
        .Finalize(temp.begin());
    return temp.GetUint64(0);
}

//! Thread unsafe
static uint64_t CalculateUnformattedDeadline(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params)
{
    // Pre-mining
    if (prevBlockIndex.nHeight <= 1)
        return 0;

    // Regtest use nonce as deadline
    if (params.fAllowMinDifficultyBlocks)
        return block.nNonce * prevBlockIndex.nBaseTarget;

    return CalcDL(prevBlockIndex.nHeight + 1, prevBlockIndex.GetNextGenerationSignature(), block.nPlotterId, block.nNonce, params);
}

// Require hold cs_main
uint64_t CalculateDeadline(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params)
{
    return CalculateUnformattedDeadline(prevBlockIndex, block, params) / prevBlockIndex.nBaseTarget;
}

uint64_t CalculateBaseTarget(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params)
{
    const int N = 80; // About 4 hours
    int nHeight = prevBlockIndex.nHeight + 1;
    if (nHeight <= 1 + N) {
        // genesis block & pre-mining block & const block
        return INITIAL_BASE_TARGET;
    } else {
        // Algorithm:
        //   B(0) = prevBlock, B(1) = B(0).prev, ..., B(n) = B(n-1).prev
        //   Y(0) = B(0).nBaseTarget
        //   Y(n) = (Y(n-1) * (n-1) + B(n).nBaseTarget) / (n + 1); n > 0
        const CBlockIndex *pLastindex = &prevBlockIndex;
        uint64_t avgBaseTarget = pLastindex->nBaseTarget;
        for (int n = 1; n < N; n++) {
            pLastindex = pLastindex->pprev;
            avgBaseTarget = (avgBaseTarget * n + pLastindex->nBaseTarget) / (n + 1);
        }
        int64_t diffTime = block.GetBlockTime() - pLastindex->GetBlockTime() - static_cast<int64_t>(N) * 1;
        int64_t targetTimespan = params.nPowTargetSpacing * N;
        if (diffTime < targetTimespan / 2) {
            diffTime = targetTimespan / 2;
        }
        if (diffTime > targetTimespan * 2) {
            diffTime = targetTimespan * 2;
        }
        uint64_t curBaseTarget = prevBlockIndex.nBaseTarget;
        uint64_t newBaseTarget = avgBaseTarget * diffTime / targetTimespan;
        if (newBaseTarget > INITIAL_BASE_TARGET) {
            newBaseTarget = INITIAL_BASE_TARGET;
        }
        if (newBaseTarget == 0) {
            newBaseTarget = 1;
        }
        if (newBaseTarget < curBaseTarget * 8 / 10) {
            newBaseTarget = curBaseTarget * 8 / 10;
        }
        if (newBaseTarget > curBaseTarget * 12 / 10) {
            newBaseTarget = curBaseTarget * 12 / 10;
        }

        return newBaseTarget;
    }
}

uint64_t AddNonce(uint64_t& bestDeadline, const CBlockIndex& miningBlockIndex,
    const uint64_t& nNonce, const uint64_t& nPlotterId, const std::string& generateTo,
    bool fCheckBind, const Consensus::Params& params)
{
    AssertLockHeld(cs_main);

    if (interruptCheckDeadline)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Not run in mining mode, restart by -server");

    CBlockHeader block;
    block.nPlotterId = nPlotterId;
    block.nNonce     = nNonce;
    const uint64_t calcUnformattedDeadline = CalculateUnformattedDeadline(miningBlockIndex, block, params);
    if (calcUnformattedDeadline == INVALID_DEADLINE)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid deadline");

    const uint64_t calcDeadline = calcUnformattedDeadline / miningBlockIndex.nBaseTarget;
    LogPrint(BCLog::POC, "Add nonce: height=%d, nonce=%" PRIu64 ", plotterId=%" PRIu64 ", deadline=%" PRIu64 "\n",
        miningBlockIndex.nHeight + 1, nNonce, nPlotterId, calcDeadline);
    bestDeadline = calcDeadline;
    bool fNewBest = false;
    if (miningBlockIndex.nHeight >= ::ChainActive().Height() - 1) {
        // Only tip and previous block
        auto it = mapGenerators.find(miningBlockIndex.GetNextGenerationSignature().GetUint64(0));
        if (it != mapGenerators.end()) {
            if (it->second.best > calcUnformattedDeadline) {
                fNewBest = true;
            } else {
                fNewBest = false;
                bestDeadline = it->second.best / miningBlockIndex.nBaseTarget;
            }
        } else {
            fNewBest = true;
        }
    }

    if (fNewBest) {
        CTxDestination dest;
        std::shared_ptr<CKey> privKey;
        if (generateTo.empty()) {
            // Update generate address from wallet
        #ifdef ENABLE_WALLET
            auto pwallet = HasWallets() ? GetWallets()[0] : nullptr;
            if (!pwallet)
                throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Require generate destination address or private key");
            dest = pwallet->GetPrimaryDestination();
        #else
            throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Require generate destination address or private key");
        #endif
        } else {
            dest = DecodeDestination(generateTo);
            if (!boost::get<ScriptHash>(&dest)) {
                // Maybe privkey
                CKey key = DecodeSecret(generateTo);
                if (!key.IsValid()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid generate destination address or private key");
                } else {
                    privKey = std::make_shared<CKey>(key);
                    // P2SH-Segwit
                    CKeyID keyid = privKey->GetPubKey().GetID();
                    CTxDestination segwit = WitnessV0KeyHash(keyid);
                    dest = ScriptHash(GetScriptForDestination(segwit));
                }
            }
        }
        if (!boost::get<ScriptHash>(&dest))
            throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Qitcoin address");

        // Check bind
        if (miningBlockIndex.nHeight + 1 >= params.nBindPlotterCheckHeight) {
            const CAccountID accountID = ExtractAccountID(dest);
            if (accountID.IsNull())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qitcoin address");
            if (!::ChainstateActive().CoinsTip().AccountHaveActiveBindPlotter(accountID, nPlotterId))
                throw JSONRPCError(RPC_INVALID_REQUEST,
                    strprintf("%" PRIu64 " with %s not active bind", nPlotterId, EncodeDestination(dest)));
        }

        // Update private key for signature. Pre-set
        if (miningBlockIndex.nHeight > 1) {
            uint64_t destId = boost::get<ScriptHash>(&dest)->GetUint64(0);

            // From cache
            if (!privKey && mapSignaturePrivKeys.count(destId))
                privKey = mapSignaturePrivKeys[destId];

            // From wallets
        #ifdef ENABLE_WALLET
            if (!privKey) {
                for (auto pwallet : GetWallets()) {
                    CKeyID keyid = GetKeyForDestination(*pwallet, dest);
                    if (!keyid.IsNull()) {
                        CKey key;
                        if (pwallet->GetKey(keyid, key)) {
                            privKey = std::make_shared<CKey>(key);
                            break;
                        }
                    }
                }
            }
        #endif

            if (!privKey)
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                    strprintf("Please pre-set %s private key for mining-sign.", EncodeDestination(dest)));

            if (!mapSignaturePrivKeys.count(destId))
                mapSignaturePrivKeys[destId] = privKey;
        }

        // Update best
        GeneratorState &generatorState = mapGenerators[miningBlockIndex.GetNextGenerationSignature().GetUint64(0)];
        generatorState.plotterId = nPlotterId;
        generatorState.nonce     = nNonce;
        generatorState.best      = calcUnformattedDeadline;
        generatorState.height    = miningBlockIndex.nHeight + 1;
        generatorState.dest      = dest;
        generatorState.privKey   = privKey;

        LogPrint(BCLog::POC, "New best deadline %" PRIu64 ".\n", calcDeadline);

        uiInterface.NotifyBestDeadlineChanged(generatorState.height, generatorState.plotterId, generatorState.nonce, calcDeadline);
    }

    return calcDeadline;
}

CBlockList GetEvalBlocks(int nHeight, bool fAscent, const Consensus::Params& params)
{
    AssertLockHeld(cs_main);
    assert(nHeight >= 0 && nHeight <= ::ChainActive().Height());

    CBlockList vBlocks;
    int nBeginHeight = std::max(nHeight - params.nCapacityEvalWindow + 1, 2);
    if (nHeight >= nBeginHeight) {
        vBlocks.reserve(nHeight - nBeginHeight + 1);
        if (fAscent) {
            for (int height = nBeginHeight; height <= nHeight; height++) {
                vBlocks.push_back(std::cref(*(::ChainActive()[height])));
            }
        } else {
            for (int height = nHeight; height >= nBeginHeight; height--) {
                vBlocks.push_back(std::cref(*(::ChainActive()[height])));
            }
        }
    }
    return vBlocks;
}

int64_t GetNetCapacity(int nHeight, const Consensus::Params& params)
{
    uint64_t nBaseTarget = 0;
    int nBlockCount = 0;
    for (const CBlockIndex& block : GetEvalBlocks(nHeight, true, params)) {
        nBaseTarget += block.nBaseTarget;
        nBlockCount++;
    }
    if (nBlockCount != 0) {
        nBaseTarget /= nBlockCount;
        if (nBaseTarget != 0) {
            return std::max(static_cast<int64_t>(INITIAL_BASE_TARGET / nBaseTarget), (int64_t) 1);
        }
    }

    return (int64_t) 1;
}

static int64_t EvalNetCapacity(int nHeight, const Consensus::Params& params, std::function<void(const CBlockIndex&)> associateBlock)
{
    uint64_t nBaseTarget = 0;
    int nBlockCount = 0;
    for (const CBlockIndex& block : GetEvalBlocks(nHeight, true, params)) {
        // All blocks
        associateBlock(block);

        nBaseTarget += block.nBaseTarget;
        nBlockCount++;
    }

    if (nBlockCount != 0) {
        nBaseTarget /= nBlockCount;
        if (nBaseTarget != 0) {
            return std::max(static_cast<int64_t>(INITIAL_BASE_TARGET / nBaseTarget), (int64_t) 1);
        }
    }

    return (int64_t) 1;
}

int64_t GetNetCapacity(int nHeight, const Consensus::Params& params, std::function<void(const CBlockIndex&)> associateBlock)
{
    return EvalNetCapacity(nHeight, params, associateBlock);
}

CAmount GetMiningPledgeRatio(int nMiningHeight, const Consensus::Params& params)
{
    AssertLockHeld(cs_main);
    CAmount nSubsidy = GetBlockSubsidy(nMiningHeight, params);
    if (nSubsidy == 0) {
        return 0;
    }
    int half = (75 * COIN) / nSubsidy;
    return params.nPledgeRatio / half;
}

CAmount GetCapacityRequireBalance(int64_t nCapacityTB, int nMiningHeight, const Consensus::Params& params)
{
    CAmount ratio = GetMiningPledgeRatio(nMiningHeight, params);
    return ((ratio * nCapacityTB + COIN/2) / COIN) * COIN;
}

CAmount GetMiningRequireBalance(const CAccountID& generatorAccountID, const uint64_t& nPlotterId, int nMiningHeight,
    const CCoinsViewCache& view, int64_t* pMinerCapacity,
    const Consensus::Params& params)
{
    AssertLockHeld(cs_main);
    assert(GetSpendHeight(view) == nMiningHeight);

    if (pMinerCapacity != nullptr) *pMinerCapacity = 0;

    int64_t nNetCapacityTB = 0;
    int nBlockCount = 0, nMinedCount = 0;
    const std::set<uint64_t> plotters = view.GetAccountBindPlotters(generatorAccountID);
    nNetCapacityTB = EvalNetCapacity(nMiningHeight - 1, params,
        [&nBlockCount, &nMinedCount, &plotters] (const CBlockIndex &block) {
            nBlockCount++;

            if (plotters.count(block.nPlotterId))
                nMinedCount++;
        }
    );
    // Remove sugar
    if (nMinedCount < nBlockCount) nMinedCount++;
    if (nMinedCount == 0 || nBlockCount == 0)
        return 0;

    int64_t nMinerCapacityTB = std::max((nNetCapacityTB * nMinedCount) / nBlockCount, (int64_t) 1);
    if (pMinerCapacity != nullptr) *pMinerCapacity = nMinerCapacityTB;
    return GetCapacityRequireBalance(nMinerCapacityTB, nMiningHeight, params);
}

bool CheckProofOfCapacity(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params)
{
    uint64_t deadline = CalculateDeadline(prevBlockIndex, block, params);

    // Maybe overflow on arithmetic operation
    if (deadline > poc::MAX_TARGET_DEADLINE)
        return false;

    if (prevBlockIndex.nHeight == 1) {
        return params.nBeginMiningTime == 0 || params.nBeginMiningTime == block.GetBlockTime();
    }

    return block.GetBlockTime() == prevBlockIndex.GetBlockTime() + static_cast<int64_t>(deadline) + 1;
}

CTxDestination AddMiningSignaturePrivkey(const CKey& key)
{
    LOCK(cs_main);

    std::shared_ptr<CKey> privKeyPtr = std::make_shared<CKey>(key);
    CKeyID keyid = privKeyPtr->GetPubKey().GetID();
    CTxDestination segwit = WitnessV0KeyHash(keyid);
    CTxDestination dest = ScriptHash(GetScriptForDestination(segwit));
    mapSignaturePrivKeys[boost::get<ScriptHash>(&dest)->GetUint64(0)] = privKeyPtr;
    return dest;
}

std::vector<CTxDestination> GetMiningSignatureAddresses()
{
    LOCK(cs_main);

    std::vector<CTxDestination> addresses;
    addresses.reserve(mapSignaturePrivKeys.size());
    for (auto it = mapSignaturePrivKeys.cbegin(); it != mapSignaturePrivKeys.cend(); it++) {
        CKeyID keyid = it->second->GetPubKey().GetID();
        CTxDestination segwit = WitnessV0KeyHash(keyid);
        CTxDestination dest = ScriptHash(GetScriptForDestination(segwit));
        addresses.push_back(dest);
    }

    return addresses;
}

}

bool StartPOC()
{
    LogPrintf("Starting PoC module\n");
    interruptCheckDeadline.reset();
    if (gArgs.GetBoolArg("-server", false)) {
        LogPrintf("Starting PoC forge thread\n");
        threadCheckDeadline = std::thread(CheckDeadlineThread);

        // import private key
        if (gArgs.IsArgSet("-signprivkey")) {
            for (const std::string &privkey : gArgs.GetArgs("-signprivkey")) {
                std::string strkeyLog = (privkey.size() > 2 ? privkey.substr(0, 2) : privkey) + "**************************************************";
                CTxDestination dest = poc::AddMiningSignaturePrivkey(DecodeSecret(privkey));
                if (IsValidDestination(dest)) {
                    LogPrintf("  Success import mining sign key for %s from `-signprivkey` \"%s\"\n", EncodeDestination(dest), strkeyLog);
                } else {
                    LogPrintf("  Fail import mining sign private key from `-signprivkey` \"%s\"\n", strkeyLog);
                }
            }
            gArgs.ForceSetArg("-signprivkey", "");
        }

    #ifdef ENABLE_WALLET
        // From current wallet
        for (auto pwallet : GetWallets()) {
            CTxDestination dest = pwallet->GetPrimaryDestination();
            CKeyID keyid = GetKeyForDestination(*pwallet, dest);
            if (!keyid.IsNull()) {
                std::shared_ptr<CKey> privKey = std::make_shared<CKey>();
                if (pwallet->GetKey(keyid, *privKey)) {
                    LOCK(cs_main);
                    mapSignaturePrivKeys[boost::get<ScriptHash>(&dest)->GetUint64(0)] = privKey;

                    LogPrintf("Import mining-sign private key from wallet primary address %s\n", EncodeDestination(dest));
                }
            }
        }
    #endif
    } else {
        LogPrintf("Skip PoC forge thread\n");
        interruptCheckDeadline();
    }

    return true;
}

void InterruptPOC()
{
    LogPrintf("Interrupting PoC module\n");
    interruptCheckDeadline();
}

void StopPOC()
{
    if (threadCheckDeadline.joinable())
        threadCheckDeadline.join();

    mapSignaturePrivKeys.clear();

    LogPrintf("Stopped PoC module\n");
}