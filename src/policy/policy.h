// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_POLICY_H
#define BITCOIN_POLICY_POLICY_H

#include <consensus/consensus.h>
#include <policy/feerate.h>
#include <script/interpreter.h>
#include <script/standard.h>

#include <string>

class CCoinsViewCache;
class CTxOut;

/** Default for -blockmaxweight, which controls the range of block weights the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_WEIGHT = MAX_BLOCK_WEIGHT - 4000;
/** Default for -blockmintxfee, which sets the minimum feerate for a transaction in blocks created by mining code **/
static const unsigned int DEFAULT_BLOCK_MIN_TX_FEE = 1000;
/** The maximum weight for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_WEIGHT = 400000;
/** The minimum non-witness size for transactions we're willing to relay/mine (1 segwit input + 1 P2WPKH output = 82 bytes) */
static const unsigned int MIN_STANDARD_TX_NONWITNESS_SIZE = 82;
/** Maximum number of signature check operations in an IsStandard() P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_STANDARD_TX_SIGOPS_COST = MAX_BLOCK_SIGOPS_COST/5;
/** Default for -maxmempool, maximum megabytes of mempool memory usage */
static const unsigned int DEFAULT_MAX_MEMPOOL_SIZE = 300;
/** Default for -incrementalrelayfee, which sets the minimum feerate increase for mempool limiting or BIP 125 replacement **/
static const unsigned int DEFAULT_INCREMENTAL_RELAY_FEE = 1000;
/** Default for -bytespersigop */
static const unsigned int DEFAULT_BYTES_PER_SIGOP = 20;
/** Default for -permitbaremultisig */
static const bool DEFAULT_PERMIT_BAREMULTISIG = true;
/** The maximum number of witness stack items in a standard P2WSH script */
static const unsigned int MAX_STANDARD_P2WSH_STACK_ITEMS = 100;
/** The maximum size of each witness stack item in a standard P2WSH script */
static const unsigned int MAX_STANDARD_P2WSH_STACK_ITEM_SIZE = 80;
/** The maximum size of each witness stack item in a standard BIP 342 script (Taproot, leaf version 0xc0) */
static const unsigned int MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE = 80;
/** The maximum size of a standard witnessScript */
static const unsigned int MAX_STANDARD_P2WSH_SCRIPT_SIZE = 3600;
/** Min feerate for defining dust. Historically this has been based on the
 * minRelayTxFee, however changing the dust limit changes which transactions are
 * standard and should be done with care and ideally rarely. It makes sense to
 * only increase the dust limit after prior releases were already not creating
 * outputs below the new threshold */
static const unsigned int DUST_RELAY_TX_FEE = 3000;
/**
 * Standard script verification flags that standard transactions will comply
 * with. However scripts violating these flags may still be present in valid
 * blocks and we must accept those blocks.
 */
static constexpr unsigned int STANDARD_SCRIPT_VERIFY_FLAGS = MANDATORY_SCRIPT_VERIFY_FLAGS |
                                                             SCRIPT_VERIFY_DERSIG |
                                                             SCRIPT_VERIFY_STRICTENC |
                                                             SCRIPT_VERIFY_MINIMALDATA |
                                                             SCRIPT_VERIFY_NULLDUMMY |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
                                                             SCRIPT_VERIFY_CLEANSTACK |
                                                             SCRIPT_VERIFY_MINIMALIF |
                                                             SCRIPT_VERIFY_NULLFAIL |
                                                             SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
                                                             SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
                                                             SCRIPT_VERIFY_LOW_S |
                                                             SCRIPT_VERIFY_WITNESS |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM |
                                                             SCRIPT_VERIFY_WITNESS_PUBKEYTYPE |
                                                             SCRIPT_VERIFY_CONST_SCRIPTCODE |
                                                             SCRIPT_VERIFY_TAPROOT |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION |
                                                             SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE;

/** For convenience, standard but not mandatory verify flags. */
static constexpr unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS;

/** Used as the flags parameter to sequence and nLocktime checks in non-consensus code. */
static constexpr unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS = LOCKTIME_VERIFY_SEQUENCE |
                                                               LOCKTIME_MEDIAN_TIME_PAST;

CAmount GetDustThreshold(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsStandard(const CScript& scriptPubKey, TxoutType& whichType);
    /**
     * Check for standard transaction types
     * @return True if all outputs (scriptPubKeys) use only standard transaction forms
     */
bool IsStandardTx(const CTransaction& tx, bool permit_bare_multisig, const CFeeRate& dust_relay_fee, std::string& reason);
    /**
     * Check for standard transaction types
     * @param[in] mapInputs       Map of previous transactions that have outputs we're spending
     * @param[in] taproot_active  Whether or taproot consensus rules are active (used to decide whether spends of them are permitted)
     * @return True if all inputs (scriptSigs) use only standard transaction forms
     */
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs, bool taproot_active);
    /**
     * Check if the transaction is over standard P2WSH resources limit:
     * 3600bytes witnessScript size, 80bytes per witness stack element, 100 witness stack elements
     * These limits are adequate for multi-signature up to n-of-100 using OP_CHECKSIG, OP_ADD, and OP_EQUAL,
     */
bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);

/** Compute the virtual transaction size (weight reinterpreted as bytes). */
int64_t GetVirtualTransactionSize(int64_t nWeight, int64_t nSigOpCost, unsigned int bytes_per_sigop);
int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t nSigOpCost, unsigned int bytes_per_sigop);
int64_t GetVirtualTransactionInputSize(const CTxIn& tx, int64_t nSigOpCost, unsigned int bytes_per_sigop);

static inline int64_t GetVirtualTransactionSize(const CTransaction& tx)
{
    return GetVirtualTransactionSize(tx, 0, 0);
}

static inline int64_t GetVirtualTransactionInputSize(const CTxIn& tx)
{
    return GetVirtualTransactionInputSize(tx, 0, 0);
}

#endif // BITCOIN_POLICY_POLICY_H
