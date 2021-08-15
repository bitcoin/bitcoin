// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POC_POC_H
#define BITCOIN_POC_POC_H

#include <amount.h>
#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <uint256.h>

#include <stdlib.h>
#include <stdint.h>

#include <functional>
#include <vector>

class CBlockHeader;
class CBlock;
class CBlockIndex;
class CCoinsViewCache;
class CKey;

namespace Consensus { struct Params; }

namespace poc {

/** 2^64, 0x10000000000000000*/
static const arith_uint256 TWO64 = arith_uint256(std::numeric_limits<uint64_t>::max()) + 1;

/**
 * QTC base target when target spacing is 1 seconds
 * 
 * See https://qitchain.org/wiki/The_Proof_of_Capacity#Base_Target
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
 *
 * 4398046511104 / 180 = 24433591728
 */
static const uint64_t INITIAL_BASE_TARGET = 24433591728ull;

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
 * Get capacity required balance
 *
 * @param nCapacityTB       Miner capacity
 * @param nMiningHeight     The height of mining
 * @param params            Consensus params
 *
 * @return Required balance
 */
CAmount GetCapacityRequireBalance(int64_t nCapacityTB, int nMiningHeight, const Consensus::Params& params);

/**
 * Get mining pledge ratio
 *
 * @param nMiningHeight             The height of mining
 * @param params                    Consensus params
 *
 * @return Required pledge ratio
 */
CAmount GetMiningPledgeRatio(int nMiningHeight, const Consensus::Params& params);

/**
 * Get mining required balance
 *
 * @param generatorAccountID        Block generator
 * @param nPlotterId                Proof of capacity ID
 * @param nMiningHeight             The height of mining
 * @param view                      The coin view
 * @param pMinerCapacityTB          Miner capacity by estimate
 * @param params                    Consensus params
 *
 * @return Required balance
 */
CAmount GetMiningRequireBalance(const CAccountID& generatorAccountID, const uint64_t& nPlotterId, int nMiningHeight,
    const CCoinsViewCache& view, int64_t* pMinerCapacityTB,
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
 * @param key               Private key for signing
 *
 * @return Return P2WSH address
 */
CTxDestination AddMiningSignaturePrivkey(const CKey& key);

/**
 * Get mining signature addresses
 *
 * @return Imported signature key related P2WSH addresses
 */
std::vector<CTxDestination> GetMiningSignatureAddresses();

}

bool StartPOC();
void InterruptPOC();
void StopPOC();

#endif
