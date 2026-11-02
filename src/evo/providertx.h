// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_PROVIDERTX_H
#define BITCOIN_EVO_PROVIDERTX_H

#include <bls/bls.h>
#include <evo/dmn_types.h>
#include <evo/netinfo.h>
#include <evo/specialtx.h>
#include <evo/types.h>
#include <primitives/transaction.h>
#include <util/std23.h>

#include <consensus/validation.h>
#include <key_io.h>
#include <netaddress.h>
#include <pubkey.h>

#include <gsl/pointers.h>
#include <univalue.h>

#include <array>
#include <limits>
#include <vector>

class TxValidationState;
struct RPCResult;

class MasternodePayoutShare
{
public:
    static constexpr uint16_t MIN_REWARD = 100;
    static constexpr uint16_t MAX_REWARD = 10000;

    CScript scriptPayout;
    uint16_t reward{MAX_REWARD};

    SERIALIZE_METHODS(MasternodePayoutShare, obj)
    {
        READWRITE(obj.scriptPayout, obj.reward);
    }

    bool operator==(const MasternodePayoutShare& b) const = default;
    bool operator!=(const MasternodePayoutShare& b) const = default;
};

using MasternodePayoutShares = std::vector<MasternodePayoutShare>;

/** A 65-byte compact recoverable ECDSA signature; serializes as raw bytes without a length prefix */
using CompactSignature = std::array<unsigned char, CPubKey::COMPACT_SIGNATURE_SIZE>;

/** One participant's contribution to a shared masternode collateral */
class CCollateralShare
{
public:
    static constexpr CAmount MIN_AMOUNT{100 * COIN};

    CAmount amount{0};
    CScript scriptRefund;
    CScript scriptReward; //!< empty means "use scriptRefund"
    CKeyID keyIDOwner;

    CCollateralShare() = default;
    CCollateralShare(CAmount amount, CScript script_refund, CScript script_reward, CKeyID key_id_owner) :
        amount(amount),
        scriptRefund(std::move(script_refund)),
        scriptReward(std::move(script_reward)),
        keyIDOwner(key_id_owner)
    {
    }

    SERIALIZE_METHODS(CCollateralShare, obj)
    {
        READWRITE(obj.amount, obj.scriptRefund, obj.scriptReward, obj.keyIDOwner);
    }

    /** The script this share's portion of owner rewards is paid to */
    const CScript& RewardScript() const { return scriptReward.empty() ? scriptRefund : scriptReward; }

    friend bool operator==(const CCollateralShare& a, const CCollateralShare& b)
    {
        return a.amount == b.amount && a.scriptRefund == b.scriptRefund && a.scriptReward == b.scriptReward &&
               a.keyIDOwner == b.keyIDOwner;
    }
    friend bool operator!=(const CCollateralShare& a, const CCollateralShare& b) { return !(a == b); }
};

using CollateralShares = std::vector<CCollateralShare>;

[[nodiscard]] MasternodePayoutShares LegacyPayoutAsList(const CScript& script_payout);
template<class T>
[[nodiscard]] MasternodePayoutShares GetOwnerPayouts(const T& protx)
{
    return protx.nVersion >= ProTxVersion::ExtAddr ? protx.payouts : LegacyPayoutAsList(protx.scriptPayout);
}

[[nodiscard]] bool IsPayoutListTriviallyValid(const MasternodePayoutShares& payouts, const CKeyID& keyIDOwner,
                                              const CKeyID& keyIDVoting, TxValidationState& state);
[[nodiscard]] bool IsPayoutListKeySafe(const MasternodePayoutShares& payouts, const CTxDestination& collateral_dest,
                                       const CKeyID& keyIDOwner, const CKeyID& keyIDVoting,
                                       bool check_payout_collateral_reuse, TxValidationState& state);
[[nodiscard]] std::string PayoutListToString(const MasternodePayoutShares& payouts);
[[nodiscard]] UniValue PayoutListToJson(const MasternodePayoutShares& payouts);

/** Validate all provider network fields using the same rules as special transaction validation.
 *  Pass nullptr for platform_node_id when validating endpoint input separately from the rest of a payload. */
[[nodiscard]] bool CheckProviderNetworkFields(const std::shared_ptr<NetInfoInterface>& net_info, MnType type,
                                              uint16_t version, const uint160* platform_node_id, uint16_t platform_p2p_port,
                                              uint16_t platform_http_port, bool allow_empty, TxValidationState& state);

/** The count, per-share bound and sum rules of IsShareListTriviallyValid, usable before any share address exists */
[[nodiscard]] bool AreShareAmountsValid(const std::vector<CAmount>& amounts, CAmount required_collateral,
                                        TxValidationState& state);
/** The early-exit penalty rules of IsShareListTriviallyValid, usable before the whole share list exists */
[[nodiscard]] bool IsEarlyPenaltyValid(uint32_t early_period_blocks, CAmount early_penalty, CAmount min_share_amount,
                                       TxValidationState& state);
[[nodiscard]] bool IsShareListTriviallyValid(const CollateralShares& shares, uint32_t early_period_blocks,
                                             CAmount early_penalty, CAmount required_collateral,
                                             const CKeyID& keyIDVoting, TxValidationState& state);
/** Whether no share refund or effective reward script pays the voting key's P2PKH destination,
 *  the registration-time separation rule that voting-key and reward-script updates must preserve */
[[nodiscard]] bool IsShareListVotingKeySafe(const CollateralShares& shares, const CKeyID& keyIDVoting);
[[nodiscard]] std::string PayoutListToString(const CollateralShares& shares, uint32_t early_period_blocks,
                                            CAmount early_penalty);
[[nodiscard]] UniValue ShareListToJson(const CollateralShares& shares);
/** floor(total * weight / weight_total) without 64-bit overflow: the pro-rata portion DIP-0026's
 *  rounding convention gives every entry but the last */
[[nodiscard]] CAmount ProRataFloor(CAmount total, CAmount weight, CAmount weight_total);
/** Split an amount across shares proportionally to their collateral amounts: sequential floor,
 *  remainder to the last entry. The result always sums to total exactly. */
[[nodiscard]] std::vector<CAmount> SplitAmountByShares(CAmount total, const CollateralShares& shares);

class CProRegTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_REGISTER;

    static constexpr uint8_t MIN_SHARES{2};
    static constexpr uint8_t MAX_SHARES{8};
    static_assert(MIN_SHARES < MAX_SHARES);
    // The share and signature counts travel in one byte; a wider limit would need a new layout
    static_assert(MAX_SHARES <= std::numeric_limits<uint8_t>::max());
    static constexpr uint32_t MAX_EARLY_PERIOD_BLOCKS{420480}; // approx. two years at 2.5-minute blocks

    uint16_t nVersion{ProTxVersion::LegacyBLS}; // message version
    MnType nType{MnType::Regular};
    uint16_t nMode{0};                                     // only 0 supported for now
    COutPoint collateralOutpoint{uint256(), static_cast<uint32_t>(-1)}; // if hash is null, we refer to a ProRegTx output
    std::shared_ptr<NetInfoInterface> netInfo{nullptr};
    uint160 platformNodeID{};
    uint16_t platformP2PPort{0};
    uint16_t platformHTTPPort{0};
    CKeyID keyIDOwner;
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    uint16_t nOperatorReward{0};
    CScript scriptPayout;
    MasternodePayoutShares payouts;
    CollateralShares shares;                            // non-empty = shared masternode registration
    std::vector<CompactSignature> vchJoinSigs;          // one consent signature per share, in share order
    uint32_t nEarlyPeriodBlocks{0};
    CAmount nEarlyPenalty{0};
    uint256 inputsHash; // replay protection
    std::vector<unsigned char> vchSig;

    SERIALIZE_METHODS(CProRegTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 ||
            obj.nVersion > ProTxVersion::GetMax(/*is_basic_scheme_active=*/true, /*is_extended_addr=*/true)) {
            // unknown version, bail out early
            return;
        }

        READWRITE(
                obj.nType,
                obj.nMode,
                obj.collateralOutpoint,
                NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.netInfo),
                                  obj.nVersion >= ProTxVersion::ExtAddr),
                obj.keyIDOwner,
                CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.pubKeyOperator), (obj.nVersion == ProTxVersion::LegacyBLS)),
                obj.keyIDVoting,
                obj.nOperatorReward
        );
        if (obj.nVersion >= ProTxVersion::ExtAddr) {
            uint8_t payouts_count{0};
            SER_WRITE(obj, payouts_count = static_cast<uint8_t>(obj.payouts.size()));
            READWRITE(payouts_count);
            SER_READ(obj, obj.payouts.resize(payouts_count));
            for (auto& payout : obj.payouts) {
                READWRITE(payout);
            }
            uint8_t shares_count{0};
            // One join signature per share: a mismatched in-memory object cannot round-trip, so
            // fail the write up front
            SER_WRITE(obj, if (obj.vchJoinSigs.size() != obj.shares.size()) {
                throw std::ios_base::failure("join signature count mismatch");
            });
            SER_WRITE(obj, shares_count = static_cast<uint8_t>(obj.shares.size()));
            READWRITE(shares_count);
            SER_READ(obj, obj.shares.resize(shares_count));
            for (auto& share : obj.shares) {
                READWRITE(share);
            }
            SER_READ(obj, obj.vchJoinSigs.resize(shares_count));
            for (auto& sig : obj.vchJoinSigs) {
                READWRITE(sig);
            }
            READWRITE(obj.nEarlyPeriodBlocks, obj.nEarlyPenalty);
        } else {
            READWRITE(obj.scriptPayout);
        }
        READWRITE(
                obj.inputsHash
        );
        if (obj.nType == MnType::Evo) {
            READWRITE(
                obj.platformNodeID);
            if (obj.nVersion < ProTxVersion::ExtAddr) {
                READWRITE(
                obj.platformP2PPort,
                obj.platformHTTPPort);
            }
        }
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    /** Whether this is a shared masternode registration (DIP: decentralized masternode shares) */
    [[nodiscard]] bool IsShared() const { return !shares.empty(); }

    /** The digest each share owner signs (joinSigs) to consent to a shared registration. It binds
     *  every participant to the exact funding inputs, all outputs, the full share table, the
     *  penalty terms and the registrar configuration of the containing transaction. */
    [[nodiscard]] uint256 MakeSharedRegConsentHash(const CTransaction& tx) const;

    // When signing with the collateral key, we don't sign the hash but a generated message instead
    // This is needed for HW wallet support which can only sign text messages as of now
    std::string MakeSignString() const;

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `CheckProRegTx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProUpServTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    uint16_t nVersion{ProTxVersion::LegacyBLS}; // message version
    MnType nType{MnType::Regular};
    uint256 proTxHash;
    std::shared_ptr<NetInfoInterface> netInfo{nullptr};
    uint160 platformNodeID{};
    uint16_t platformP2PPort{0};
    uint16_t platformHTTPPort{0};
    CScript scriptOperatorPayout;
    uint256 inputsHash; // replay protection
    CBLSSignature sig;

    SERIALIZE_METHODS(CProUpServTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 ||
            obj.nVersion > ProTxVersion::GetMax(/*is_basic_scheme_active=*/true, /*is_extended_addr=*/true)) {
            // unknown version, bail out early
            return;
        }
        if (obj.nVersion >= ProTxVersion::BasicBLS) {
            READWRITE(
                obj.nType);
        }
        READWRITE(
                obj.proTxHash,
                NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.netInfo),
                                  obj.nVersion >= ProTxVersion::ExtAddr),
                obj.scriptOperatorPayout,
                obj.inputsHash
        );
        if (obj.nType == MnType::Evo) {
            READWRITE(
                obj.platformNodeID);
            if (obj.nVersion < ProTxVersion::ExtAddr) {
                READWRITE(
                obj.platformP2PPort,
                obj.platformHTTPPort);
            }
        }
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(
                    CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), (obj.nVersion == ProTxVersion::LegacyBLS))
            );
        }
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `CheckProUpServTx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProUpRegTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    uint16_t nVersion{ProTxVersion::LegacyBLS}; // message version
    uint256 proTxHash;
    uint16_t nMode{0}; // only 0 supported for now
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    CScript scriptPayout;
    MasternodePayoutShares payouts;
    uint256 inputsHash; // replay protection
    std::vector<unsigned char> vchSig;

    SERIALIZE_METHODS(CProUpRegTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 ||
            obj.nVersion > ProTxVersion::GetMax(/*is_basic_scheme_active=*/true, /*is_extended_addr=*/true)) {
            // unknown version, bail out early
            return;
        }
        READWRITE(
                obj.proTxHash,
                obj.nMode,
                CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.pubKeyOperator), (obj.nVersion == ProTxVersion::LegacyBLS)),
                obj.keyIDVoting
        );
        if (obj.nVersion >= ProTxVersion::ExtAddr) {
            uint8_t payouts_count{0};
            SER_WRITE(obj, payouts_count = static_cast<uint8_t>(obj.payouts.size()));
            READWRITE(payouts_count);
            SER_READ(obj, obj.payouts.resize(payouts_count));
            for (auto& payout : obj.payouts) {
                READWRITE(payout);
            }
        } else {
            READWRITE(obj.scriptPayout);
        }
        READWRITE(
                obj.inputsHash
        );
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(
                    obj.vchSig
            );
        }
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `CheckProUpRegTx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProUpRevTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_REVOKE;

    // these are just informational and do not have any effect on the revocation
    enum {
        REASON_NOT_SPECIFIED = 0,
        REASON_TERMINATION_OF_SERVICE = 1,
        REASON_COMPROMISED_KEYS = 2,
        REASON_CHANGE_OF_KEYS = 3,
        REASON_LAST = REASON_CHANGE_OF_KEYS
    };

    uint16_t nVersion{ProTxVersion::LegacyBLS}; // message version
    uint256 proTxHash;
    uint16_t nReason{REASON_NOT_SPECIFIED};
    uint256 inputsHash; // replay protection
    CBLSSignature sig;

    SERIALIZE_METHODS(CProUpRevTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 ||
            obj.nVersion > ProTxVersion::GetMax(/*is_basic_scheme_active=*/true, /*is_extended_addr=*/true)) {
            // unknown version, bail out early
            return;
        }
        READWRITE(
                obj.proTxHash,
                obj.nReason,
                obj.inputsHash
        );
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(
                    CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), (obj.nVersion == ProTxVersion::LegacyBLS))
            );
        }
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `CheckProUpRevTx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProDisTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_DISSOLVE;
    static constexpr uint16_t CURRENT_VERSION = 1;
    //! Consensus ceiling on the transaction fee. Together with the unilateral bonus ceiling
    //! (earlyPenalty) it bounds what a stolen share owner key can drain from the actor's share.
    static constexpr CAmount MAX_FEE{1000000};

    uint16_t nVersion{CURRENT_VERSION};
    uint256 proTxHash;
    uint16_t actorIndex{0};
    // Exactly one signature (unilateral, by shares[actorIndex]) or one per share in share order
    // (unanimous); the signature count defines the mode
    std::vector<CompactSignature> vchSigs;

    SERIALIZE_METHODS(CProDisTx, obj)
    {
        READWRITE(obj.nVersion, obj.proTxHash, obj.actorIndex);
        uint8_t sig_count{0};
        SER_WRITE(obj, sig_count = static_cast<uint8_t>(obj.vchSigs.size()));
        READWRITE(sig_count);
        SER_READ(obj, obj.vchSigs.resize(sig_count));
        for (auto& sig : obj.vchSigs) {
            READWRITE(sig);
        }
    }

    /** The digest every dissolution signature commits to. It covers the transaction's actual
     *  input(s) and outputs directly, plus the signature count, which selects the mode
     *  (1 = unilateral, sharesCount = unanimous). Committing the count is what stops a third party
     *  from reinterpreting a penalty-free unanimous dissolution as a unilateral one (or vice
     *  versa) by dropping/adding signatures, which would change the txid. Callers pass the count
     *  the transaction will carry; verification passes the actual vchSigs.size(). */
    [[nodiscard]] uint256 MakeSignHash(const CTransaction& tx, uint8_t sig_count) const;

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `Check*Tx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProUpShareTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_SHARE;
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint16_t nVersion{CURRENT_VERSION};
    uint256 proTxHash;
    uint16_t shareIndex{0};
    CScript scriptReward; //!< explicit reward script; use the refund script to reset rewards
    uint256 inputsHash;   // replay protection
    std::vector<unsigned char> vchSig;

    SERIALIZE_METHODS(CProUpShareTx, obj)
    {
        READWRITE(obj.nVersion, obj.proTxHash, obj.shareIndex, obj.scriptReward, obj.inputsHash);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `Check*Tx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

class CProUpSharedRegTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR;
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint16_t nVersion{CURRENT_VERSION};
    uint256 proTxHash;
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    uint256 inputsHash; // replay protection
    // One signature per share, in share order; a shared registrar update requires unanimity
    std::vector<CompactSignature> vchSigs;

    SERIALIZE_METHODS(CProUpSharedRegTx, obj)
    {
        READWRITE(obj.nVersion, obj.proTxHash,
                  CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.pubKeyOperator), /*legacy=*/false),
                  obj.keyIDVoting, obj.inputsHash);
        if (!(s.GetType() & SER_GETHASH)) {
            uint8_t sig_count{0};
            SER_WRITE(obj, sig_count = static_cast<uint8_t>(obj.vchSigs.size()));
            READWRITE(sig_count);
            SER_READ(obj, obj.vchSigs.resize(sig_count));
            for (auto& sig : obj.vchSigs) {
                READWRITE(sig);
            }
        }
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    /**
     * Note: this check validates only some trivial consensus rules
     * Use `Check*Tx` or GetValidatedPayload<T> helper for full validation
     */
    bool IsTriviallyValid(TxValidationState& state) const;
};

template <typename ProTx>
static bool CheckInputsHash(const CTransaction& tx, const ProTx& proTx, TxValidationState& state)
{
    if (uint256 inputsHash = CalcTxInputsHash(tx); inputsHash != proTx.inputsHash) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-inputs-hash");
    }
    return true;
}

#endif // BITCOIN_EVO_PROVIDERTX_H
