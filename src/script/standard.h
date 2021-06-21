// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_STANDARD_H
#define BITCOIN_SCRIPT_STANDARD_H

#include <pubkey.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <util/hash_type.h>

#include <map>
#include <string>
#include <variant>

static const bool DEFAULT_ACCEPT_DATACARRIER = true;

class CKeyID;
class CScript;
struct ScriptHash;

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public BaseHash<uint160>
{
public:
    CScriptID() : BaseHash() {}
    explicit CScriptID(const CScript& in);
    explicit CScriptID(const uint160& in) : BaseHash(in) {}
    explicit CScriptID(const ScriptHash& in);
};

/**
 * Default setting for nMaxDatacarrierBytes. 80 bytes of data, +1 for OP_RETURN,
 * +2 for the pushdata opcodes.
 */
static const unsigned int MAX_OP_RETURN_RELAY = 83;

/**
 * A data carrying output is an unspendable output containing data. The script
 * type is designated as TxoutType::NULL_DATA.
 */
extern bool fAcceptDatacarrier;

/** Maximum size of TxoutType::NULL_DATA scripts that this node considers standard. */
extern unsigned nMaxDatacarrierBytes;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added.
 *
 * Failing one of these tests may trigger a DoS ban - see CheckInputScripts() for
 * details.
 */
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH;

enum class TxoutType {
    NONSTANDARD,
    // 'standard' transaction types:
    PUBKEY,
    PUBKEYHASH,
    SCRIPTHASH,
    MULTISIG,
    NULL_DATA, //!< unspendable OP_RETURN script that carries data
    WITNESS_V0_SCRIPTHASH,
    WITNESS_V0_KEYHASH,
    WITNESS_V1_TAPROOT,
    WITNESS_UNKNOWN, //!< Only for Witness versions not already defined above
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

struct PKHash : public BaseHash<uint160>
{
    PKHash() : BaseHash() {}
    explicit PKHash(const uint160& hash) : BaseHash(hash) {}
    explicit PKHash(const CPubKey& pubkey);
    explicit PKHash(const CKeyID& pubkey_id);
};
CKeyID ToKeyID(const PKHash& key_hash);

struct WitnessV0KeyHash;
struct ScriptHash : public BaseHash<uint160>
{
    ScriptHash() : BaseHash() {}
    // These don't do what you'd expect.
    // Use ScriptHash(GetScriptForDestination(...)) instead.
    explicit ScriptHash(const WitnessV0KeyHash& hash) = delete;
    explicit ScriptHash(const PKHash& hash) = delete;

    explicit ScriptHash(const uint160& hash) : BaseHash(hash) {}
    explicit ScriptHash(const CScript& script);
    explicit ScriptHash(const CScriptID& script);
};

struct WitnessV0ScriptHash : public BaseHash<uint256>
{
    WitnessV0ScriptHash() : BaseHash() {}
    explicit WitnessV0ScriptHash(const uint256& hash) : BaseHash(hash) {}
    explicit WitnessV0ScriptHash(const CScript& script);
};

struct WitnessV0KeyHash : public BaseHash<uint160>
{
    WitnessV0KeyHash() : BaseHash() {}
    explicit WitnessV0KeyHash(const uint160& hash) : BaseHash(hash) {}
    explicit WitnessV0KeyHash(const CPubKey& pubkey);
    explicit WitnessV0KeyHash(const PKHash& pubkey_hash);
};
CKeyID ToKeyID(const WitnessV0KeyHash& key_hash);

struct WitnessV1Taproot : public XOnlyPubKey
{
    WitnessV1Taproot() : XOnlyPubKey() {}
    explicit WitnessV1Taproot(const XOnlyPubKey& xpk) : XOnlyPubKey(xpk) {}
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
 *  * PKHash: TxoutType::PUBKEYHASH destination (P2PKH)
 *  * ScriptHash: TxoutType::SCRIPTHASH destination (P2SH)
 *  * WitnessV0ScriptHash: TxoutType::WITNESS_V0_SCRIPTHASH destination (P2WSH)
 *  * WitnessV0KeyHash: TxoutType::WITNESS_V0_KEYHASH destination (P2WPKH)
 *  * WitnessV1Taproot: TxoutType::WITNESS_V1_TAPROOT destination (P2TR)
 *  * WitnessUnknown: TxoutType::WITNESS_UNKNOWN destination (P2W???)
 *  A CTxDestination is the internal data type encoded in a bitcoin address
 */
using CTxDestination = std::variant<CNoDestination, PKHash, ScriptHash, WitnessV0ScriptHash, WitnessV0KeyHash, WitnessV1Taproot, WitnessUnknown>;

/** Check whether a CTxDestination is a CNoDestination. */
bool IsValidDestination(const CTxDestination& dest);

/** Get the name of a TxoutType as a string */
std::string GetTxnOutputType(TxoutType t);

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @return                     The script type. TxoutType::NONSTANDARD represents a failed solve.
 */
TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet);

/**
 * Parse a standard scriptPubKey for the destination address. Assigns result to
 * the addressRet parameter and returns true if successful. For multisig
 * scripts, instead use ExtractDestinations. Currently only works for P2PK,
 * P2PKH, P2SH, P2WPKH, and P2WSH scripts.
 */
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);

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
 *
 * TODO: from v23 ("addresses" and "reqSigs" deprecated) "ExtractDestinations" should be removed
 */
bool ExtractDestinations(const CScript& scriptPubKey, TxoutType& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet);

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

struct ShortestVectorFirstComparator
{
    bool operator()(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) const
    {
        if (a.size() < b.size()) return true;
        if (a.size() > b.size()) return false;
        return a < b;
    }
};

struct TaprootSpendData
{
    /** The BIP341 internal key. */
    XOnlyPubKey internal_key;
    /** The Merkle root of the script tree (0 if no scripts). */
    uint256 merkle_root;
    /** Map from (script, leaf_version) to (sets of) control blocks.
     *  The control blocks are sorted by size, so that the signing logic can
     *  easily prefer the cheapest one. */
    std::map<std::pair<CScript, int>, std::set<std::vector<unsigned char>, ShortestVectorFirstComparator>> scripts;
    /** Merge other TaprootSpendData (for the same scriptPubKey) into this. */
    void Merge(TaprootSpendData other);
};

/** Utility class to construct Taproot outputs from internal key and script tree. */
class TaprootBuilder
{
private:
    /** Information about a tracked leaf in the Merkle tree. */
    struct LeafInfo
    {
        CScript script;                      //!< The script.
        int leaf_version;                    //!< The leaf version for that script.
        std::vector<uint256> merkle_branch;  //!< The hashing partners above this leaf.
    };

    /** Information associated with a node in the Merkle tree. */
    struct NodeInfo
    {
        /** Merkle hash of this node. */
        uint256 hash;
        /** Tracked leaves underneath this node (either from the node itself, or its children).
         *  The merkle_branch field for each is the partners to get to *this* node. */
        std::vector<LeafInfo> leaves;
    };
    /** Whether the builder is in a valid state so far. */
    bool m_valid = true;

    /** The current state of the builder.
     *
     * For each level in the tree, one NodeInfo object may be present. m_branch[0]
     * is information about the root; further values are for deeper subtrees being
     * explored.
     *
     * For every right branch taken to reach the position we're currently
     * working in, there will be a (non-nullopt) entry in m_branch corresponding
     * to the left branch at that level.
     *
     * For example, imagine this tree:     - N0 -
     *                                    /      \
     *                                   N1      N2
     *                                  /  \    /  \
     *                                 A    B  C   N3
     *                                            /  \
     *                                           D    E
     *
     * Initially, m_branch is empty. After processing leaf A, it would become
     * {nullopt, nullopt, A}. When processing leaf B, an entry at level 2 already
     * exists, and it would thus be combined with it to produce a level 1 one,
     * resulting in {nullopt, N1}. Adding C and D takes us to {nullopt, N1, C}
     * and {nullopt, N1, C, D} respectively. When E is processed, it is combined
     * with D, and then C, and then N1, to produce the root, resulting in {N0}.
     *
     * This structure allows processing with just O(log n) overhead if the leaves
     * are computed on the fly.
     *
     * As an invariant, there can never be nullopt entries at the end. There can
     * also not be more than 128 entries (as that would mean more than 128 levels
     * in the tree). The depth of newly added entries will always be at least
     * equal to the current size of m_branch (otherwise it does not correspond
     * to a depth-first traversal of a tree). m_branch is only empty if no entries
     * have ever be processed. m_branch having length 1 corresponds to being done.
     */
    std::vector<std::optional<NodeInfo>> m_branch;

    XOnlyPubKey m_internal_key;  //!< The internal key, set when finalizing.
    XOnlyPubKey m_output_key;    //!< The output key, computed when finalizing.
    bool m_parity;               //!< The tweak parity, computed when finalizing.

    /** Combine information about a parent Merkle tree node from its child nodes. */
    static NodeInfo Combine(NodeInfo&& a, NodeInfo&& b);
    /** Insert information about a node at a certain depth, and propagate information up. */
    void Insert(NodeInfo&& node, int depth);

public:
    /** Add a new script at a certain depth in the tree. Add() operations must be called
     *  in depth-first traversal order of binary tree. If track is true, it will be included in
     *  the GetSpendData() output. */
    TaprootBuilder& Add(int depth, const CScript& script, int leaf_version, bool track = true);
    /** Like Add(), but for a Merkle node with a given hash to the tree. */
    TaprootBuilder& AddOmitted(int depth, const uint256& hash);
    /** Finalize the construction. Can only be called when IsComplete() is true.
        internal_key.IsFullyValid() must be true. */
    TaprootBuilder& Finalize(const XOnlyPubKey& internal_key);

    /** Return true if so far all input was valid. */
    bool IsValid() const { return m_valid; }
    /** Return whether there were either no leaves, or the leaves form a Huffman tree. */
    bool IsComplete() const { return m_valid && (m_branch.size() == 0 || (m_branch.size() == 1 && m_branch[0].has_value())); }
    /** Compute scriptPubKey (after Finalize()). */
    WitnessV1Taproot GetOutput();
    /** Check if a list of depths is legal (will lead to IsComplete()). */
    static bool ValidDepths(const std::vector<int>& depths);
    /** Compute spending data (after Finalize()). */
    TaprootSpendData GetSpendData() const;
};

#endif // BITCOIN_SCRIPT_STANDARD_H
