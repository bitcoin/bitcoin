// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_STANDARD_H
#define BITCOIN_SCRIPT_STANDARD_H

#include <script/interpreter.h>
#include <uint256.h>

#include <boost/variant.hpp>

#include <stdint.h>
#include <memory>
#include <set>

static const bool DEFAULT_ACCEPT_DATACARRIER = true;

class CKeyID;
class CScript;

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public uint160
{
public:
    CScriptID() : uint160() {}
    explicit CScriptID(const CScript& in);
    CScriptID(const uint160& in) : uint160(in) {}
};

/**
 * Default setting for nMaxDatacarrierBytes. 8000 bytes of data, +1 for OP_RETURN,
 * +2 for the pushdata opcodes.
 */
static const unsigned int MAX_OP_RETURN_RELAY = 8003;

/**
 * A data carrying output is an unspendable output containing data. The script
 * type is designated as TX_NULL_DATA.
 */
extern bool fAcceptDatacarrier;

/** Maximum size of TX_NULL_DATA scripts that this node considers standard. */
extern unsigned nMaxDatacarrierBytes;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added, such as a soft-fork to enforce
 * strict DER encoding.
 *
 * Failing one of these tests may trigger a DoS ban - see CheckInputs() for
 * details.
 */
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

enum txnouttype
{
    TX_NONSTANDARD,
    // 'standard' transaction types:
    TX_PUBKEY,
    TX_PUBKEYHASH,
    TX_SCRIPTHASH,
    TX_MULTISIG,
    TX_NULL_DATA, //!< unspendable OP_RETURN script that carries data
    TX_WITNESS_V0_SCRIPTHASH,
    TX_WITNESS_V0_KEYHASH,
    TX_WITNESS_UNKNOWN, //!< Only for Witness versions not already defined above
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

struct PKHash : public uint160
{
    PKHash() : uint160() {}
    explicit PKHash(const uint160& hash) : uint160(hash) {}
    explicit PKHash(const CPubKey& pubkey);
    using uint160::uint160;
};

struct WitnessV0KeyHash;
struct ScriptHash : public uint160
{
    ScriptHash() : uint160() {}
    // These don't do what you'd expect.
    // Use ScriptHash(GetScriptForDestination(...)) instead.
    explicit ScriptHash(const WitnessV0KeyHash& hash) = delete;
    explicit ScriptHash(const PKHash& hash) = delete;
    explicit ScriptHash(const uint160& hash) : uint160(hash) {}
    explicit ScriptHash(const CScript& script);
    using uint160::uint160;
};

struct WitnessV0ScriptHash : public uint256
{
    WitnessV0ScriptHash() : uint256() {}
    explicit WitnessV0ScriptHash(const uint256& hash) : uint256(hash) {}
    explicit WitnessV0ScriptHash(const CScript& script);
    using uint256::uint256;
};

struct WitnessV0KeyHash : public uint160
{
    WitnessV0KeyHash() : uint160() {}
    explicit WitnessV0KeyHash(const uint160& hash) : uint160(hash) {}
    using uint160::uint160;
};

//! CTxDestination subtype to encode any future Witness version
struct WitnessUnknown
{
    unsigned int version;
    unsigned int length;
    unsigned char program[40];

    friend bool operator==(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version != w2.version) return false;
        if (w1.length != w2.length) return false;
        return std::equal(w1.program, w1.program + w1.length, w2.program);
    }

    friend bool operator<(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version < w2.version) return true;
        if (w1.version > w2.version) return false;
        if (w1.length < w2.length) return true;
        if (w1.length > w2.length) return false;
        return std::lexicographical_compare(w1.program, w1.program + w1.length, w2.program, w2.program + w2.length);
    }
};

/**
 * A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * PKHash: TX_PUBKEYHASH destination (P2PKH)
 *  * ScriptHash: TX_SCRIPTHASH destination (P2SH)
 *  * WitnessV0ScriptHash: TX_WITNESS_V0_SCRIPTHASH destination (P2WSH)
 *  * WitnessV0KeyHash: TX_WITNESS_V0_KEYHASH destination (P2WPKH)
 *  * WitnessUnknown: TX_WITNESS_UNKNOWN destination (P2W???)
 *  A CTxDestination is the internal data type encoded in a bitcoin address
 */
typedef boost::variant<CNoDestination, PKHash, ScriptHash, WitnessV0ScriptHash, WitnessV0KeyHash, WitnessUnknown> CTxDestination;

/** Check whether a CTxDestination is a CNoDestination. */
bool IsValidDestination(const CTxDestination& dest);

/** Get the name of a txnouttype as a C string, or nullptr if unknown. */
const char* GetTxnOutputType(txnouttype t);

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @return                     The script type. TX_NONSTANDARD represents a failed solve.
 */
txnouttype Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet);

/**
 * Parse a standard scriptPubKey for the destination address. Assigns result to
 * the addressRet parameter and returns true if successful. For multisig
 * scripts, instead use ExtractDestinations. Currently only works for P2PK,
 * P2PKH, P2SH, P2WPKH, and P2WSH scripts.
 */
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);
CTxDestination ExtractDestination(const CScript& scriptPubKey);

/**
 * Parse a standard scriptPubKey with one or more destination addresses. For
 * multisig scripts, this populates the addressRet vector with the pubkey IDs
 * and nRequiredRet with the n required to spend. For other destinations,
 * addressRet is populated with a single value and nRequiredRet is set to 1.
 * Returns true if successful.
 *
 * Note: this function confuses destinations (a subset of CScripts that are
 * encodable as an address) with key identifiers (of keys involved in a
 * CScript), and its use should be phased out.
 */
bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet);

/**
 * Generate a Bitcoin scriptPubKey for the given CTxDestination. Returns a P2PKH
 * script for a CKeyID destination, a P2SH script for a CScriptID, and an empty
 * script for CNoDestination.
 */
CScript GetScriptForDestination(const CTxDestination& dest);

/** Generate a P2PK script for the given pubkey. */
CScript GetScriptForRawPubKey(const CPubKey& pubkey);

/** Generate a multisig script. */
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);

/**
 * Generate a pay-to-witness script for the given redeem script. If the redeem
 * script is P2PK or P2PKH, this returns a P2WPKH script, otherwise it returns a
 * P2WSH script.
 *
 * TODO: replace calls to GetScriptForWitness with GetScriptForDestination using
 * the various witness-specific CTxDestination subtypes.
 */
CScript GetScriptForWitness(const CScript& redeemscript);

/** Generate a P2WPKH script for the given pubkey. */
CScript GetScriptForPubKey(const CPubKey& pubkey);

/** Generate a P2WPKH script for the given account ID. */
CScript GetScriptForAccountID(const CAccountID& accountID);

/** Utility function to get account ID. */
CAccountID ExtractAccountID(const CPubKey& pubkey);
CAccountID ExtractAccountID(const CScript& scriptPubKey);
CAccountID ExtractAccountID(const CTxDestination& dest);

/**
 * Parse a account ID for the destination address. 
 * Currently only works for P2SH, P2WPKH, and P2WSH scripts.
 */
CTxDestination ExtractDestination(const CAccountID& accountID);

/** opreturn type. See https://qitchain.org/wiki/outtype */
enum TxOutType : unsigned int {
    // Range
    TXOUT_TYPE_MIN = 0x0000000f,
    TXOUT_TYPE_MAX = 0x10000000,

    // Alias for unknown
    TXOUT_TYPE_UNKNOWN = TXOUT_TYPE_MIN,

    // Type of consensus relevant
    TXOUT_TYPE_BINDPLOTTER = 0x00000010,
    TXOUT_TYPE_POINT       = 0x00000011,
    TXOUT_TYPE_STAKING     = 0x00000012,
};

/** Datacarrier payload */
struct TxOutPayload
{
    const TxOutType type;

    explicit TxOutPayload(TxOutType typeIn) : type(typeIn) {}
    virtual ~TxOutPayload() {}
};
typedef std::shared_ptr<TxOutPayload> CTxOutPayloadRef;

/** For bind plotter */
struct BindPlotterPayload : public TxOutPayload
{
    uint64_t id;

    BindPlotterPayload() : TxOutPayload(TXOUT_TYPE_BINDPLOTTER), id(0) {}

    const uint64_t& GetId() const { return id; }

    // Checkable cast for CTxOutPayloadRef
    static BindPlotterPayload * As(CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_BINDPLOTTER);
        return (BindPlotterPayload*) ref.get();
    }
    static const BindPlotterPayload * As(const CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_BINDPLOTTER);
        return (const BindPlotterPayload*) ref.get();
    }
};

/** For point */
struct PointPayload : public TxOutPayload
{
    CAccountID receiverID;
    uint32_t lockBlocks;
    CAmount amount;

    PointPayload() : TxOutPayload(TXOUT_TYPE_POINT), lockBlocks(0), amount(0) {}

    const CAccountID& GetReceiverID() const { return receiverID; }
    const uint32_t& GetLockBlocks() const { return lockBlocks; }
    const CAmount& GetAmount() const { return amount; }

    // Checkable cast for CTxOutPayloadRef
    static PointPayload * As(CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_POINT);
        return (PointPayload*) ref.get();
    }
    static const PointPayload * As(const CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_POINT);
        return (const PointPayload*) ref.get();
    }
};

/** For staking */
struct StakingPayload : public TxOutPayload
{
    CAccountID receiverID;
    uint32_t lockBlocks;
    CAmount amount;

    StakingPayload() : TxOutPayload(TXOUT_TYPE_STAKING), lockBlocks(0), amount(0) {}

    const CAccountID& GetReceiverID() const { return receiverID; }
    const uint32_t& GetLockBlocks() const { return lockBlocks; }
    const CAmount& GetAmount() const { return amount; }

    // Checkable cast for CTxOutPayloadRef
    static StakingPayload * As(CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_STAKING);
        return (StakingPayload*) ref.get();
    }
    static const StakingPayload * As(const CTxOutPayloadRef &ref) {
        assert(ref->type == TXOUT_TYPE_STAKING);
        return (const StakingPayload*) ref.get();
    }
};

/** The bind plotter lock amount */
static const CAmount PROTOCOL_BINDPLOTTER_LOCKAMOUNT = COIN / 10;

/** The height for bind plotter default maximum relative tip height */
static const int PROTOCOL_BINDPLOTTER_DEFAULTMAXALIVE = 24;

/** The height for bind plotter maximum relative tip height */
static const int PROTOCOL_BINDPLOTTER_MAXALIVE = 288 * 7;

/** The bind plotter script size */
static const int PROTOCOL_BINDPLOTTER_SCRIPTSIZE = 108;

/** Check whether a string is a valid passphrase. */
bool IsValidPassphrase(const std::string& passphrase);

/** Check whether a string is a valid plotter ID. */
bool IsValidPlotterID(const std::string& strPlotterId, uint64_t *id = nullptr);

/** Generate a bind plotter script. */
CScript GetBindPlotterScriptForDestination(const CTxDestination& dest, const std::string& passphrase, int lastActiveHeight);

/** Check bind plotter script. */
bool IsBindPlotterScript(const CScript &script);

/** Signature bind plotter script. */
class CKey;
CScript SignBindPlotterScript(const CScript &script, const CKey &key);

/** Decode bind plotter script. */
bool DecodeBindPlotterScript(const CScript &script, uint64_t& plotterId, std::string& pubkeyHex, std::string& signatureHex, int& lastActiveHeight);
uint64_t GetBindPlotterIdFromScript(const CScript &script);

/** The minimal point amount */
static const CAmount PROTOCOL_POINT_AMOUNT_MIN = 10 * COIN;

/** The point script size */
static const int PROTOCOL_POINT_SCRIPTSIZE = 31;

/** Generate a point script. */
CScript GetPointScriptForDestination(const CTxDestination& dest, int lockBlocks);
/** Get effective point amount. */
CAmount GetPointAmount(CAmount amount, int lockBlocks);

/** The minimal staking amount */
static const CAmount PROTOCOL_STAKING_AMOUNT_MIN = 100 * COIN;

/** The staking script size */
static const int PROTOCOL_STAKING_SCRIPTSIZE = 31;

/** Generate staking script. */
CScript GetStakingScriptForDestination(const CTxDestination& dest, int lockBlocks);
/** Get effective staking amount. */
CAmount GetStakingAmount(CAmount amount, int lockBlocks);

/** Parse transaction output payload. */
CTxOutPayloadRef ExtractTxoutPayload(const CTxOut& txout, int nHeight = 0, const std::set<TxOutType>& filters = {}, bool for_test = false);

#endif // BITCOIN_SCRIPT_STANDARD_H
