// Copyright (c) 2017-2018 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POC_POC_H
#define BITCOIN_POC_POC_H

#include <script/script.h>

#include <amount.h>
#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <stdlib.h>
#include <stdint.h>

#include <functional>
#include <vector>

class CBlockHeader;
class CBlock;
class CBlockIndex;
class CCoinsViewCache;

namespace Consensus { struct Params; }

namespace poc {

/** 2^64, 0x10000000000000000*/
static const arith_uint256 TWO64 = arith_uint256(std::numeric_limits<uint64_t>::max()) + 1;

/**
 * BHD base target when target spacing is 1 seconds
 * 
 * See https://btchd.org/wiki/The_Proof_of_Capacity#Base_Target
 *
 * net capacity(t) = 4398046511104 / t / baseTarget(t)
 *
 * Each nonce provides a deadline.
 * The size of 4 nonces is 1 MiB. Each deadline is an 64bit unsigned integer, i.e.
 * has a value between 0 and 2^64-1. Deadlines are uniform and independent identically
 * distributed (i.i.d). Scanning deadlines means looking for the minimum (best deadline).
 * The expected minimum deadline of such a distribution is* E(X)=(b + a * n) / (n + 1),
 * where (a,b) is the range of possible values for a deadline (0..2^64-1), n is the number
 * of deadlines being scanned.
 */
static const uint64_t BHD_BASE_TARGET_1 = 4398046511104ULL;

/**
 * Get basetarget for give target spacing
 */
inline uint64_t GetBaseTarget(int targetSpacing) {
    return BHD_BASE_TARGET_1 / targetSpacing;
}

/**
 * Get basetarget for give height
 */
uint64_t GetBaseTarget(int nHeight, const Consensus::Params& params);

// Max target deadline
static const int64_t MAX_TARGET_DEADLINE = std::numeric_limits<uint32_t>::max();

// Invalid deadline
static const uint64_t INVALID_DEADLINE = std::numeric_limits<uint64_t>::max();

/**
 * Calculate deadline
 *
 * @param prevBlockIndex    Previous block
 * @param block             Block header
 * @param params            Consensus params
 *
 * @return Return deadline
 */
uint64_t CalculateDeadline(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params);

/**
 * Calculate base target
 *
 * @param prevBlockIndex    Previous block
 * @param block             Block header
 * @param params            Consensus params
 *
 * @return Return new base target for current block
 */
uint64_t CalculateBaseTarget(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params);

/**
 * Add new nonce
 *
 * @param bestDeadline      Output current best deadline
 * @param miningBlockIndex  Mining block
 * @param nPlotterId        Plot Id
 * @param nNonce            Found nonce
 * @param generateTo        Destination address or private key for block signing
 * @param fCheckBind        Check address and plot bind relation
 * @param params            Consensus params
 *
 * @return Return deadline calc result
 */
uint64_t AddNonce(uint64_t& bestDeadline, const CBlockIndex& miningBlockIndex,
    const uint64_t& nPlotterId, const uint64_t& nNonce, const std::string& generateTo,
    bool fCheckBind, const Consensus::Params& params);

/**
 * Block collection
 */
typedef std::vector< std::reference_wrapper<const CBlockIndex> > CBlockList;

/**
 * Get blocks by eval window
 *
 * @param nHeight           The height of net capacity
 * @param fAscent           Ascent or Descent sort blocks
 * @param params            Consensus params
 */
CBlockList GetEvalBlocks(int nHeight, bool fAscent, const Consensus::Params& params);

/**
 * Eval mining ratio by capacity
 *
 * @param nMiningHeight     The height for mining
 * @param nNetCapacityTB    Network capacity of TB
 * @param params            Consensus params
 * @param pRatioStage       The stage of current ratio
 */
CAmount EvalMiningRatio(int nMiningHeight, int64_t nNetCapacityTB, const Consensus::Params& params, int* pRatioStage = nullptr);

/**
 * Get net capacity for ratio
 *
 * @param nHeight               The height of net capacity
 * @param nNetCapacityTB        Network capacity of TB
 * @param nPrevNetCapacityTB    Previous eval net capacity for ratio
 * @param params                Consensus params
 *
 * @return Return adjuested net capacity of TB for ratio
 */
int64_t GetRatioNetCapacity(int64_t nNetCapacityTB, int64_t nPrevNetCapacityTB, const Consensus::Params& params);

/**
 * Get net capacity
 *
 * @param nHeight           The height of net capacity
 * @param params            Consensus params
 *
 * @return Return net capacity of TB
 */
int64_t GetNetCapacity(int nHeight, const Consensus::Params& params);

/**
 * Get net capacity
 *
 * @param nHeight           The height of net capacity
 * @param params            Consensus params
 * @param associateBlock    Associate block callback
 *
 * @return Return net capacity of TB
 */
int64_t GetNetCapacity(int nHeight, const Consensus::Params& params, std::function<void(const CBlockIndex &block)> associateBlock);

/**
 * Get mining ratio
 *
 * @param nHeight           The height for mining
 * @param params            Consensus params
 * @param pRatioStage       The stage of current ratio
 * @param pRatioCapacityTB  The net capacity of current stage
 * @param pRatioBeginHeight The begin block height of current stage
 *
 * @return Return mining ratio
 */
CAmount GetMiningRatio(int nMiningHeight, const Consensus::Params& params, int* pRatioStage = nullptr,
    int64_t* pRatioCapacityTB = nullptr, int *pRatioBeginHeight = nullptr);

/**
 * Get capacity required balance
 *
 * @param nCapacityTB       Miner capacity
 * @param miningRatio       The mining ratio
 *
 * @return Required balance
 */
CAmount GetCapacityRequireBalance(int64_t nCapacityTB, CAmount miningRatio);

/**
 * Get mining required balance
 *
 * @param generatorAccountID        Block generator
 * @param nPlotterId                Proof of capacity ID
 * @param nMiningHeight             The height of mining
 * @param view                      The coin view
 * @param pMinerCapacityTB          Miner capacity by estimate
 * @param pOldMiningRequireBalance  Only in BHDIP004. See https://btchd.org/wiki/BHDIP/004#getminingrequire
 * @param params                    Consensus params
 *
 * @return Required balance
 */
CAmount GetMiningRequireBalance(const CAccountID& generatorAccountID, const uint64_t& nPlotterId, int nMiningHeight,
    const CCoinsViewCache& view, int64_t* pMinerCapacityTB, CAmount* pOldMiningRequireBalance,
    const Consensus::Params& params);

/**
 * Check block work
 *
 * @param prevBlockIndex    Previous block
 * @param block             Block header
 * @param params            Consensus params
 *
 * @return Return true is poc valid
 */
bool CheckProofOfCapacity(const CBlockIndex& prevBlockIndex, const CBlockHeader& block, const Consensus::Params& params);

/**
 * Add private key for mining signature
 *
 * @param privkey       Private key for signing
 * @param newAddress    Private key related P2WSH address
 *
 * @return Return false on private key invalid
 */
bool AddMiningSignaturePrivkey(const std::string& privkey, std::string* newAddress = nullptr);

/**
 * Get mining signature addresses
 *
 * @return Imported signature key related P2WSH addresses
 */
std::vector<std::string> GetMiningSignatureAddresses();

}

bool StartPOC();
void InterruptPOC();
void StopPOC();

#endif
