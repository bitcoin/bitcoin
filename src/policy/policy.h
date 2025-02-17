// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_POLICY_H
#define BITCOIN_POLICY_POLICY_H

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/solver.h>

#include <cstdint>
#include <string>

class CCoinsViewCache;
class CFeeRate;
class CScript;

/** Default for -blockmaxweight, which controls the range of block weights the mining code will create **/
static constexpr unsigned int DEFAULT_BLOCK_MAX_WEIGHT{MAX_BLOCK_WEIGHT};
/** Default for -blockreservedweight **/
static constexpr unsigned int DEFAULT_BLOCK_RESERVED_WEIGHT{8000};
/** This accounts for the block header, var_int encoding of the transaction count and a minimally viable
 * coinbase transaction. It adds an additional safety margin, because even with a thorough understanding
 * of block serialization, it's easy to make a costly mistake when trying to squeeze every last byte.
 * Setting a lower value is prevented at startup. */
static constexpr unsigned int MINIMUM_BLOCK_RESERVED_WEIGHT{2000};
/** Default for -blockmintxfee, which sets the minimum feerate for a transaction in blocks created by mining code **/
static constexpr unsigned int DEFAULT_BLOCK_MIN_TX_FEE{1000};
/** The maximum weight for transactions we're willing to relay/mine */
static constexpr int32_t MAX_STANDARD_TX_WEIGHT{400000};
/** The minimum non-witness size for transactions we're willing to relay/mine: one larger than 64  */
static constexpr unsigned int MIN_STANDARD_TX_NONWITNESS_SIZE{65};
/** Maximum number of signature check operations in an IsStandard() P2SH script */
static constexpr unsigned int MAX_P2SH_SIGOPS{15};
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static constexpr unsigned int MAX_STANDARD_TX_SIGOPS_COST{MAX_BLOCK_SIGOPS_COST/5};
/** Default for -incrementalrelayfee, which sets the minimum feerate increase for mempool limiting or replacement **/
static constexpr unsigned int DEFAULT_INCREMENTAL_RELAY_FEE{1000};
/** Default for -bytespersigop */
static constexpr unsigned int DEFAULT_BYTES_PER_SIGOP{20};
/** Default for -permitbaremultisig */
static constexpr bool DEFAULT_PERMIT_BAREMULTISIG{true};
/** The maximum number of witness stack items in a standard P2WSH script */
static constexpr unsigned int MAX_STANDARD_P2WSH_STACK_ITEMS{100};
/** The maximum size in bytes of each witness stack item in a standard P2WSH script */
static constexpr unsigned int MAX_STANDARD_P2WSH_STACK_ITEM_SIZE{80};
/** The maximum size in bytes of each witness stack item in a standard BIP 342 script (Taproot, leaf version 0xc0) */
static constexpr unsigned int MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE{80};
/** The maximum size in bytes of a standard witnessScript */
static constexpr unsigned int MAX_STANDARD_P2WSH_SCRIPT_SIZE{3600};
/** The maximum size of a standard ScriptSig */
static constexpr unsigned int MAX_STANDARD_SCRIPTSIG_SIZE{1650};
/** Min feerate for defining dust.
 * Changing the dust limit changes which transactions are
 * standard and should be done with care and ideally rarely. It makes sense to
 * only increase the dust limit after prior releases were already not creating
 * outputs below the new threshold */
static constexpr unsigned int DUST_RELAY_TX_FEE{3000};
/** Default for -minrelaytxfee, minimum relay fee for transactions */
static constexpr unsigned int DEFAULT_MIN_RELAY_TX_FEE{1000};
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static constexpr unsigned int DEFAULT_ANCESTOR_LIMIT{25};
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static constexpr unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT_KVB{101};
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static constexpr unsigned int DEFAULT_DESCENDANT_LIMIT{25};
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static constexpr unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT_KVB{101};
/** Default for -datacarrier */
static const bool DEFAULT_ACCEPT_DATACARRIER = true;
/**
 * Default setting for -datacarriersize. 80 bytes of data, +1 for OP_RETURN,
 * +2 for the pushdata opcodes.
 */
static const unsigned int MAX_OP_RETURN_RELAY = 83;
/**
 * An extra transaction can be added to a package, as long as it only has one
 * ancestor and is no larger than this. Not really any reason to make this
 * configurable as it doesn't materially change DoS parameters.
 */
static constexpr unsigned int EXTRA_DESCENDANT_TX_SIZE_LIMIT{10000};

/**
 * Maximum number of ephemeral dust outputs allowed.
 */
static constexpr unsigned int MAX_DUST_OUTPUTS_PER_TX{1};

/**
 * Mandatory script verification flags that all new transactions must comply with for
 * them to be valid. Failing one of these tests may trigger a DoS ban;
 * see CheckInputScripts() for details.
 *
 * Note that this does not affect consensus validity; see GetBlockScriptFlags()
 * for that.
 */
static constexpr unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS{SCRIPT_VERIFY_P2SH |
                                                             SCRIPT_VERIFY_DERSIG |
                                                             SCRIPT_VERIFY_NULLDUMMY |
                                                             SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
                                                             SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
                                                             SCRIPT_VERIFY_WITNESS |
                                                             SCRIPT_VERIFY_TAPROOT};

/**
 * Standard script verification flags that standard transactions will comply
 * with. However we do not ban/disconnect nodes that forward txs violating
 * the additional (non-mandatory) rules here, to improve forwards and
 * backwards compatibility.
 */
static constexpr unsigned int STANDARD_SCRIPT_VERIFY_FLAGS{MANDATORY_SCRIPT_VERIFY_FLAGS |
                                                             SCRIPT_VERIFY_STRICTENC |
                                                             SCRIPT_VERIFY_MINIMALDATA |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
                                                             SCRIPT_VERIFY_CLEANSTACK |
                                                             SCRIPT_VERIFY_MINIMALIF |
                                                             SCRIPT_VERIFY_NULLFAIL |
                                                             SCRIPT_VERIFY_LOW_S |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM |
                                                             SCRIPT_VERIFY_WITNESS_PUBKEYTYPE |
                                                             SCRIPT_VERIFY_CONST_SCRIPTCODE |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION |
                                                             SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE};

/** For convenience, standard but not mandatory verify flags. */
static constexpr unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS{STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS};

/** Used as the flags parameter to sequence and nLocktime checks in non-consensus code. */
static constexpr unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS{LOCKTIME_VERIFY_SEQUENCE};

CAmount GetDustThreshold(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsStandard(const CScript& scriptPubKey, const std::optional<unsigned>& max_datacarrier_bytes, TxoutType& whichType);

/** Get the vout index numbers of all dust outputs */
std::vector<uint32_t> GetDust(const CTransaction& tx, CFeeRate dust_relay_rate);

// Changing the default transaction version requires a two step process: first
// adapting relay policy by bumping TX_MAX_STANDARD_VERSION, and then later
// allowing the new transaction version in the wallet/RPC.
static constexpr decltype(CTransaction::version) TX_MAX_STANDARD_VERSION{3};

/**
* Check for standard transaction types
* @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(const CTransaction& tx, const std::optional<unsigned>& max_datacarrier_bytes, bool permit_bare_multisig, const CFeeRate& dust_relay_fee, std::string& reason);
/**
* Check for standard transaction types
* @param[in] mapInputs       Map of previous transactions that have outputs we're spending
* @return True if all inputs (scriptSigs) use only standard transaction forms
*/
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);
/**
* Check if the transaction is over standard P2WSH resources limit:
* 3600bytes witnessScript size, 80bytes per witness stack element, 100 witness stack elements
* These limits are adequate for multisignatures up to n-of-100 using OP_CHECKSIG, OP_ADD, and OP_EQUAL.
*
* Also enforce a maximum stack item size limit and no annexes for tapscript spends.
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
