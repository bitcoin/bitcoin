// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_INTERPRETER_H
#define BITCOIN_SCRIPT_INTERPRETER_H

#include <consensus/amount.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/script_error.h> // IWYU pragma: export
#include <script/verify_flags.h> // IWYU pragma: export
#include <span.h>
#include <uint256.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class CPubKey;
class CScript;
class CScriptNum;
class XOnlyPubKey;
struct CScriptWitness;

/** Signature hash types/flags */
enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,

    SIGHASH_DEFAULT = 0, //!< Taproot only; implied when sighash byte is missing, and equivalent to SIGHASH_ALL
    SIGHASH_OUTPUT_MASK = 3,
    SIGHASH_INPUT_MASK = 0x80,
};

/** Script verification flags.
 *
 *  All flags are intended to be soft forks: the set of acceptable scripts under
 *  flags (A | B) is a subset of the acceptable scripts under flag (A).
 */

static constexpr script_verify_flags SCRIPT_VERIFY_NONE{0};

enum class script_verify_flag_name : uint8_t {
    // Evaluate P2SH subscripts (BIP16).
    SCRIPT_VERIFY_P2SH,

    // Passing a non-strict-DER signature or one with undefined hashtype to a checksig operation causes script failure.
    // Evaluating a pubkey that is not (0x04 + 64 bytes) or (0x02 or 0x03 + 32 bytes) by checksig causes script failure.
    // (not used or intended as a consensus rule).
    SCRIPT_VERIFY_STRICTENC,

    // Passing a non-strict-DER signature to a checksig operation causes script failure (BIP62 rule 1)
    SCRIPT_VERIFY_DERSIG,

    // Passing a non-strict-DER signature or one with S > order/2 to a checksig operation causes script failure
    // (BIP62 rule 5).
    SCRIPT_VERIFY_LOW_S,

    // verify dummy stack item consumed by CHECKMULTISIG is of zero-length (BIP62 rule 7).
    SCRIPT_VERIFY_NULLDUMMY,

    // Using a non-push operator in the scriptSig causes script failure (BIP62 rule 2).
    SCRIPT_VERIFY_SIGPUSHONLY,

    // Require minimal encodings for all push operations (OP_0... OP_16, OP_1NEGATE where possible, direct
    // pushes up to 75 bytes, OP_PUSHDATA up to 255 bytes, OP_PUSHDATA2 for anything larger). Evaluating
    // any other push causes the script to fail (BIP62 rule 3).
    // In addition, whenever a stack element is interpreted as a number, it must be of minimal length (BIP62 rule 4).
    SCRIPT_VERIFY_MINIMALDATA,

    // Discourage use of NOPs reserved for upgrades (NOP1-10)
    //
    // Provided so that nodes can avoid accepting or mining transactions
    // containing executed NOP's whose meaning may change after a soft-fork,
    // thus rendering the script invalid; with this flag set executing
    // discouraged NOPs fails the script. This verification flag will never be
    // a mandatory flag applied to scripts in a block. NOPs that are not
    // executed, e.g.  within an unexecuted IF ENDIF block, are *not* rejected.
    // NOPs that have associated forks to give them new meaning (CLTV, CSV)
    // are not subject to this rule.
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS,

    // Require that only a single stack element remains after evaluation. This changes the success criterion from
    // "At least one stack element must remain, and when interpreted as a boolean, it must be true" to
    // "Exactly one stack element must remain, and when interpreted as a boolean, it must be true".
    // (BIP62 rule 6)
    // Note: CLEANSTACK should never be used without P2SH or WITNESS.
    // Note: WITNESS_V0 and TAPSCRIPT script execution have behavior similar to CLEANSTACK as part of their
    //       consensus rules. It is automatic there and does not need this flag.
    SCRIPT_VERIFY_CLEANSTACK,

    // Verify CHECKLOCKTIMEVERIFY
    //
    // See BIP65 for details.
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY,

    // support CHECKSEQUENCEVERIFY opcode
    //
    // See BIP112 for details
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY,

    // Support segregated witness
    //
    SCRIPT_VERIFY_WITNESS,

    // Making v1-v16 witness program non-standard
    //
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM,

    // Segwit script only: Require the argument of OP_IF/NOTIF to be exactly 0x01 or empty vector
    //
    // Note: TAPSCRIPT script execution has behavior similar to MINIMALIF as part of its consensus
    //       rules. It is automatic there and does not depend on this flag.
    SCRIPT_VERIFY_MINIMALIF,

    // Signature(s) must be empty vector if a CHECK(MULTI)SIG operation failed
    //
    SCRIPT_VERIFY_NULLFAIL,

    // Public keys in segregated witness scripts must be compressed
    //
    SCRIPT_VERIFY_WITNESS_PUBKEYTYPE,

    // Making OP_CODESEPARATOR and FindAndDelete fail any non-segwit scripts
    //
    SCRIPT_VERIFY_CONST_SCRIPTCODE,

    // Taproot/Tapscript validation (BIPs 341 & 342)
    //
    SCRIPT_VERIFY_TAPROOT,

    // Making unknown Taproot leaf versions non-standard
    //
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION,

    // Making unknown OP_SUCCESS non-standard
    SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS,

    // Making unknown public key versions (in BIP 342 scripts) non-standard
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE,

    // Constants to point to the highest flag in use. Add new flags above this line.
    //
    SCRIPT_VERIFY_END_MARKER
};
using enum script_verify_flag_name;

static constexpr int MAX_SCRIPT_VERIFY_FLAGS_BITS = static_cast<int>(SCRIPT_VERIFY_END_MARKER);

// assert there is still a spare bit
static_assert(0 < MAX_SCRIPT_VERIFY_FLAGS_BITS && MAX_SCRIPT_VERIFY_FLAGS_BITS <= 63);

static constexpr script_verify_flags::value_type MAX_SCRIPT_VERIFY_FLAGS = ((script_verify_flags::value_type{1} << MAX_SCRIPT_VERIFY_FLAGS_BITS) - 1);

bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, script_verify_flags flags, ScriptError* serror);

struct PrecomputedTransactionData
{
    // BIP341 precomputed data.
    // These are single-SHA256, see https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki#cite_note-16.
    uint256 m_prevouts_single_hash;
    uint256 m_sequences_single_hash;
    uint256 m_outputs_single_hash;
    uint256 m_spent_amounts_single_hash;
    uint256 m_spent_scripts_single_hash;
    //! Whether the 5 fields above are initialized.
    bool m_bip341_taproot_ready = false;

    // BIP143 precomputed data (double-SHA256).
    uint256 hashPrevouts, hashSequence, hashOutputs;
    //! Whether the 3 fields above are initialized.
    bool m_bip143_segwit_ready = false;

    std::vector<CTxOut> m_spent_outputs;
    //! Whether m_spent_outputs is initialized.
    bool m_spent_outputs_ready = false;

    PrecomputedTransactionData() = default;

    /** Initialize this PrecomputedTransactionData with transaction data.
     *
     * @param[in]   tx             The transaction for which data is being precomputed.
     * @param[in]   spent_outputs  The CTxOuts being spent, one for each tx.vin, in order.
     * @param[in]   force          Whether to precompute data for all optional features,
     *                             regardless of what is in the inputs (used at signing
     *                             time, when the inputs aren't filled in yet). */
    template <class T>
    void Init(const T& tx, std::vector<CTxOut>&& spent_outputs, bool force = false);

    template <class T>
    explicit PrecomputedTransactionData(const T& tx);
};

enum class SigVersion
{
    BASE = 0,        //!< Bare scripts and BIP16 P2SH-wrapped redeemscripts
    WITNESS_V0 = 1,  //!< Witness v0 (P2WPKH and P2WSH); see BIP 141
    TAPROOT = 2,     //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, key path spending; see BIP 341
    TAPSCRIPT = 3,   //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, script path spending, leaf version 0xc0; see BIP 342
};

struct ScriptExecutionData
{
    //! Whether m_tapleaf_hash is initialized.
    bool m_tapleaf_hash_init = false;
    //! The tapleaf hash.
    uint256 m_tapleaf_hash;

    //! Whether m_codeseparator_pos is initialized.
    bool m_codeseparator_pos_init = false;
    //! Opcode position of the last executed OP_CODESEPARATOR (or 0xFFFFFFFF if none executed).
    uint32_t m_codeseparator_pos;

    //! Whether m_annex_present and (when needed) m_annex_hash are initialized.
    bool m_annex_init = false;
    //! Whether an annex is present.
    bool m_annex_present;
    //! Hash of the annex data.
    uint256 m_annex_hash;

    //! Whether m_validation_weight_left is initialized.
    bool m_validation_weight_left_init = false;
    //! How much validation weight is left (decremented for every successful non-empty signature check).
    int64_t m_validation_weight_left;

    //! The hash of the corresponding output
    std::optional<uint256> m_output_hash;
};

/** Signature hash sizes */
static constexpr size_t WITNESS_V0_SCRIPTHASH_SIZE = 32;
static constexpr size_t WITNESS_V0_KEYHASH_SIZE = 20;
static constexpr size_t WITNESS_V1_TAPROOT_SIZE = 32;

static constexpr uint8_t TAPROOT_LEAF_MASK = 0xfe;
static constexpr uint8_t TAPROOT_LEAF_TAPSCRIPT = 0xc0;
static constexpr size_t TAPROOT_CONTROL_BASE_SIZE = 33;
static constexpr size_t TAPROOT_CONTROL_NODE_SIZE = 32;
static constexpr size_t TAPROOT_CONTROL_MAX_NODE_COUNT = 128;
static constexpr size_t TAPROOT_CONTROL_MAX_SIZE = TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * TAPROOT_CONTROL_MAX_NODE_COUNT;

extern const HashWriter HASHER_TAPSIGHASH; //!< Hasher with tag "TapSighash" pre-fed to it.
extern const HashWriter HASHER_TAPLEAF;    //!< Hasher with tag "TapLeaf" pre-fed to it.
extern const HashWriter HASHER_TAPBRANCH;  //!< Hasher with tag "TapBranch" pre-fed to it.

/** Data structure to cache SHA256 midstates for the ECDSA sighash calculations
 *  (bare, P2SH, P2WPKH, P2WSH). */
class SigHashCache
{
    /** For each sighash mode (ALL, SINGLE, NONE, ALL|ANYONE, SINGLE|ANYONE, NONE|ANYONE),
     *  optionally store a scriptCode which the hash is for, plus a midstate for the SHA256
     *  computation just before adding the hash_type itself. */
    std::optional<std::pair<CScript, HashWriter>> m_cache_entries[6];

    /** Given a hash_type, find which of the 6 cache entries is to be used. */
    int CacheIndex(int32_t hash_type) const noexcept;

public:
    /** Load into writer the SHA256 midstate if found in this cache. */
    [[nodiscard]] bool Load(int32_t hash_type, const CScript& script_code, HashWriter& writer) const noexcept;
    /** Store into this cache object the provided SHA256 midstate. */
    void Store(int32_t hash_type, const CScript& script_code, const HashWriter& writer) noexcept;
};

template <class T>
uint256 SignatureHash(const CScript& scriptCode, const T& txTo, unsigned int nIn, int32_t nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache = nullptr, SigHashCache* sighash_cache = nullptr);

class BaseSignatureChecker
{
public:
    virtual bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        return false;
    }

    virtual bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const
    {
        return false;
    }

    virtual bool CheckLockTime(const CScriptNum& nLockTime) const
    {
         return false;
    }

    virtual bool CheckSequence(const CScriptNum& nSequence) const
    {
         return false;
    }

    virtual ~BaseSignatureChecker() = default;
};

/** Enum to specify what *TransactionSignatureChecker's behavior should be
 *  when dealing with missing transaction data.
 */
enum class MissingDataBehavior
{
    ASSERT_FAIL,  //!< Abort execution through assertion failure (for consensus code)
    FAIL,         //!< Just act as if the signature was invalid
};

template<typename T>
bool SignatureHashSchnorr(uint256& hash_out, ScriptExecutionData& execdata, const T& tx_to, uint32_t in_pos, uint8_t hash_type, SigVersion sigversion, const PrecomputedTransactionData& cache, MissingDataBehavior mdb);

template <class T>
class GenericTransactionSignatureChecker : public BaseSignatureChecker
{
private:
    const T* txTo;
    const MissingDataBehavior m_mdb;
    unsigned int nIn;
    const CAmount amount;
    const PrecomputedTransactionData* txdata;
    mutable SigHashCache m_sighash_cache;

protected:
    virtual bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;
    virtual bool VerifySchnorrSignature(std::span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const;

public:
    GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn, MissingDataBehavior mdb) : txTo(txToIn), m_mdb(mdb), nIn(nInIn), amount(amountIn), txdata(nullptr) {}
    GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn, const PrecomputedTransactionData& txdataIn, MissingDataBehavior mdb) : txTo(txToIn), m_mdb(mdb), nIn(nInIn), amount(amountIn), txdata(&txdataIn) {}
    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override;
    bool CheckLockTime(const CScriptNum& nLockTime) const override;
    bool CheckSequence(const CScriptNum& nSequence) const override;
};

using TransactionSignatureChecker = GenericTransactionSignatureChecker<CTransaction>;
using MutableTransactionSignatureChecker = GenericTransactionSignatureChecker<CMutableTransaction>;

class DeferringSignatureChecker : public BaseSignatureChecker
{
protected:
    const BaseSignatureChecker& m_checker;

public:
    DeferringSignatureChecker(const BaseSignatureChecker& checker) : m_checker(checker) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        return m_checker.CheckECDSASignature(scriptSig, vchPubKey, scriptCode, sigversion);
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override
    {
        return m_checker.CheckSchnorrSignature(sig, pubkey, sigversion, execdata, serror);
    }

    bool CheckLockTime(const CScriptNum& nLockTime) const override
    {
        return m_checker.CheckLockTime(nLockTime);
    }
    bool CheckSequence(const CScriptNum& nSequence) const override
    {
        return m_checker.CheckSequence(nSequence);
    }
};

/** Compute the BIP341 tapleaf hash from leaf version & script. */
uint256 ComputeTapleafHash(uint8_t leaf_version, std::span<const unsigned char> script);
/** Compute the BIP341 tapbranch hash from two branches.
  * Spans must be 32 bytes each. */
uint256 ComputeTapbranchHash(std::span<const unsigned char> a, std::span<const unsigned char> b);
/** Compute the BIP341 taproot script tree Merkle root from control block and leaf hash.
 *  Requires control block to have valid length (33 + k*32, with k in {0,1,..,128}). */
uint256 ComputeTaprootMerkleRoot(std::span<const unsigned char> control, const uint256& tapleaf_hash);

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, script_verify_flags flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* error = nullptr);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, script_verify_flags flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* error = nullptr);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, script_verify_flags flags, const BaseSignatureChecker& checker, ScriptError* serror = nullptr);

size_t CountWitnessSigOps(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, script_verify_flags flags);

int FindAndDelete(CScript& script, const CScript& b);

extern const std::map<std::string, script_verify_flag_name> g_verify_flag_names;

std::vector<std::string> GetScriptFlagNames(script_verify_flags flags);

#endif // BITCOIN_SCRIPT_INTERPRETER_H
