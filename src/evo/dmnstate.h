// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_DMNSTATE_H
#define BITCOIN_EVO_DMNSTATE_H

#include <bls/bls.h>
#include <crypto/sha256.h>
#include <evo/providertx.h>
#include <netaddress.h>
#include <pubkey.h>
#include <script/script.h>
#include <util/pointer.h>

#include <memory>
#include <utility>

#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>

class CProRegTx;
class UniValue;

class CDeterministicMNState;

namespace llmq
{
    class CFinalCommitment;
} // namespace llmq

class CDeterministicMNState
{
private:
    int nPoSeBanHeight{-1};

    friend class CDeterministicMNStateDiff;
    friend class CDeterministicMNStateDiffLegacy;

public:
    int nVersion{ProTxVersion::LegacyBLS};

    int nRegisteredHeight{-1};
    int nLastPaidHeight{0};
    int nConsecutivePayments{0};
    int nPoSePenalty{0};
    int nPoSeRevivedHeight{-1};
    uint16_t nRevocationReason{CProUpRevTx::REASON_NOT_SPECIFIED};

    // the block hash X blocks after registration, used in quorum calculations
    uint256 confirmedHash;
    // sha256(proTxHash, confirmedHash) to speed up quorum calculations
    // please note that this is NOT a double-sha256 hash
    uint256 confirmedHashWithProRegTxHash;

    CKeyID keyIDOwner;
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    std::shared_ptr<NetInfoInterface> netInfo{nullptr};
    CScript scriptPayout;
    CScript scriptOperatorPayout;

    uint160 platformNodeID{};
    uint16_t platformP2PPort{0};
    uint16_t platformHTTPPort{0};

public:
    CDeterministicMNState() = default;
    explicit CDeterministicMNState(const CProRegTx& proTx) :
        nVersion(proTx.nVersion),
        keyIDOwner(proTx.keyIDOwner),
        pubKeyOperator(proTx.pubKeyOperator),
        keyIDVoting(proTx.keyIDVoting),
        netInfo(proTx.netInfo),
        scriptPayout(proTx.scriptPayout),
        platformNodeID(proTx.platformNodeID),
        platformP2PPort(proTx.platformP2PPort),
        platformHTTPPort(proTx.platformHTTPPort)
    {
    }
    template <typename Stream>
    CDeterministicMNState(deserialize_type, Stream& s) { s >> *this; }

    SERIALIZE_METHODS(CDeterministicMNState, obj)
    {
        READWRITE(
            obj.nVersion,
            obj.nRegisteredHeight,
            obj.nLastPaidHeight,
            obj.nConsecutivePayments,
            obj.nPoSePenalty,
            obj.nPoSeRevivedHeight,
            obj.nPoSeBanHeight,
            obj.nRevocationReason,
            obj.confirmedHash,
            obj.confirmedHashWithProRegTxHash,
            obj.keyIDOwner);
        READWRITE(CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.pubKeyOperator), obj.nVersion == ProTxVersion::LegacyBLS));
        READWRITE(
            obj.keyIDVoting,
            NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.netInfo),
                              obj.nVersion >= ProTxVersion::ExtAddr),
            obj.scriptPayout,
            obj.scriptOperatorPayout,
            obj.platformNodeID,
            obj.platformP2PPort,
            obj.platformHTTPPort);
    }

    void ResetOperatorFields()
    {
        nVersion = ProTxVersion::LegacyBLS;
        pubKeyOperator = CBLSLazyPublicKey();
        netInfo = NetInfoInterface::MakeNetInfo(nVersion);
        scriptOperatorPayout = CScript();
        nRevocationReason = CProUpRevTx::REASON_NOT_SPECIFIED;
        platformNodeID = uint160();
    }
    void BanIfNotBanned(int height)
    {
        if (!IsBanned()) {
            nPoSeBanHeight = height;
        }
    }
    int GetBannedHeight() const
    {
        return nPoSeBanHeight;
    }
    bool IsBanned() const
    {
        return nPoSeBanHeight != -1;
    }
    void Revive(int nRevivedHeight)
    {
        nPoSePenalty = 0;
        nPoSeBanHeight = -1;
        nPoSeRevivedHeight = nRevivedHeight;
    }
    void UpdateConfirmedHash(const uint256& _proTxHash, const uint256& _confirmedHash)
    {
        confirmedHash = _confirmedHash;
        CSHA256 h;
        h.Write(_proTxHash.begin(), _proTxHash.size());
        h.Write(_confirmedHash.begin(), _confirmedHash.size());
        h.Finalize(confirmedHashWithProRegTxHash.begin());
    }

public:
    std::string ToString() const;
    [[nodiscard]] UniValue ToJson(MnType nType) const;
};

class CDeterministicMNStateDiff
{
public:
    enum Field : uint32_t {
        Field_nVersion = 0x0001,
        Field_nRegisteredHeight = 0x0002,
        Field_nLastPaidHeight = 0x0004,
        Field_nPoSePenalty = 0x0008,
        Field_nPoSeRevivedHeight = 0x0010,
        Field_nPoSeBanHeight = 0x0020,
        Field_nRevocationReason = 0x0040,
        Field_confirmedHash = 0x0080,
        Field_confirmedHashWithProRegTxHash = 0x0100,
        Field_keyIDOwner = 0x0200,
        Field_pubKeyOperator = 0x0400,
        Field_keyIDVoting = 0x0800,
        Field_netInfo = 0x1000,
        Field_scriptPayout = 0x2000,
        Field_scriptOperatorPayout = 0x4000,
        Field_nConsecutivePayments = 0x8000,
        Field_platformNodeID = 0x10000,
        Field_platformP2PPort = 0x20000,
        Field_platformHTTPPort = 0x40000,
    };

private:
    template <auto CDeterministicMNState::*Field, uint32_t Mask>
    struct Member
    {
        static constexpr uint32_t mask = Mask;
        static auto& get(CDeterministicMNState& state) { return state.*Field; }
        static const auto& get(const CDeterministicMNState& state) { return state.*Field; }
    };

#define DMN_STATE_MEMBER(name) Member<&CDeterministicMNState::name, Field_##name>{}
    static constexpr auto members = boost::hana::make_tuple(
        DMN_STATE_MEMBER(nVersion),
        DMN_STATE_MEMBER(nRegisteredHeight),
        DMN_STATE_MEMBER(nLastPaidHeight),
        DMN_STATE_MEMBER(nPoSePenalty),
        DMN_STATE_MEMBER(nPoSeRevivedHeight),
        DMN_STATE_MEMBER(nPoSeBanHeight),
        DMN_STATE_MEMBER(nRevocationReason),
        DMN_STATE_MEMBER(confirmedHash),
        DMN_STATE_MEMBER(confirmedHashWithProRegTxHash),
        DMN_STATE_MEMBER(keyIDOwner),
        DMN_STATE_MEMBER(pubKeyOperator),
        DMN_STATE_MEMBER(keyIDVoting),
        DMN_STATE_MEMBER(netInfo),
        DMN_STATE_MEMBER(scriptPayout),
        DMN_STATE_MEMBER(scriptOperatorPayout),
        DMN_STATE_MEMBER(nConsecutivePayments),
        DMN_STATE_MEMBER(platformNodeID),
        DMN_STATE_MEMBER(platformP2PPort),
        DMN_STATE_MEMBER(platformHTTPPort)
    );
#undef DMN_STATE_MEMBER

public:
    uint32_t fields{0};
    // we reuse the state class, but only the members as noted by fields are valid
    CDeterministicMNState state;

public:
    CDeterministicMNStateDiff() = default;
    CDeterministicMNStateDiff(const CDeterministicMNState& a, const CDeterministicMNState& b)
    {
        boost::hana::for_each(members, [&](auto&& member) {
            using BaseType = std::decay_t<decltype(member)>;
            if constexpr (BaseType::mask == Field_netInfo) {
                if (util::shared_ptr_not_equal(member.get(a), member.get(b))) {
                    member.get(state) = member.get(b);
                    fields |= member.mask;
                }
            } else {
                if (member.get(a) != member.get(b)) {
                    member.get(state) = member.get(b);
                    fields |= member.mask;
                }
            }
        });
        if (fields & Field_pubKeyOperator) {
            // pubKeyOperator needs nVersion
            state.nVersion = b.nVersion;
            fields |= Field_nVersion;
        }
    }
    template <typename Stream>
    CDeterministicMNStateDiff(deserialize_type, Stream& s) { s >> *this; }

    [[nodiscard]] UniValue ToJson(MnType nType) const;

    SERIALIZE_METHODS(CDeterministicMNStateDiff, obj)
    {
        READWRITE(VARINT(obj.fields));

        // NOTE: reading pubKeyOperator requires nVersion
        bool read_pubkey{false};
        boost::hana::for_each(members, [&](auto&& member) {
            using BaseType = std::decay_t<decltype(member)>;
            if constexpr (BaseType::mask == Field_pubKeyOperator) {
                if (obj.fields & member.mask) {
                    SER_READ(obj, read_pubkey = true);
                    READWRITE(CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.state.pubKeyOperator), obj.state.nVersion == ProTxVersion::LegacyBLS));
                }
            } else if constexpr (BaseType::mask == Field_netInfo) {
                if (obj.fields & member.mask) {
                    // As nVersion is stored after netInfo, we use a magic word to determine the underlying implementation
                    // TODO: Implement this
                    READWRITE(NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.state.netInfo),
                                                /*is_extended=*/false));
                }
            } else {
                if (obj.fields & member.mask) {
                    READWRITE(member.get(obj.state));
                }
            }
        });

        if (read_pubkey) {
            SER_READ(obj, obj.fields |= Field_nVersion);
            SER_READ(obj, obj.state.pubKeyOperator.SetLegacy(obj.state.nVersion == ProTxVersion::LegacyBLS));
        }
    }

    void ApplyToState(CDeterministicMNState& target) const
    {
        boost::hana::for_each(members, [&](auto&& member) {
            if (fields & member.mask) {
                member.get(target) = member.get(state);
            }
        });
    }
};

// Legacy deserializer class
class CDeterministicMNStateDiffLegacy
{
private:
    // Legacy field enum with original bit positions
    enum LegacyField : uint32_t {
        LegacyField_nRegisteredHeight = 0x0001,
        LegacyField_nLastPaidHeight = 0x0002,
        LegacyField_nPoSePenalty = 0x0004,
        LegacyField_nPoSeRevivedHeight = 0x0008,
        LegacyField_nPoSeBanHeight = 0x0010,
        LegacyField_nRevocationReason = 0x0020,
        LegacyField_confirmedHash = 0x0040,
        LegacyField_confirmedHashWithProRegTxHash = 0x0080,
        LegacyField_keyIDOwner = 0x0100,
        LegacyField_pubKeyOperator = 0x0200,
        LegacyField_keyIDVoting = 0x0400,
        LegacyField_netInfo = 0x0800,
        LegacyField_scriptPayout = 0x1000,
        LegacyField_scriptOperatorPayout = 0x2000,
        LegacyField_nConsecutivePayments = 0x4000,
        LegacyField_platformNodeID = 0x8000,
        LegacyField_platformP2PPort = 0x10000,
        LegacyField_platformHTTPPort = 0x20000,
        LegacyField_nVersion = 0x40000,
    };

    // Legacy member template with old bit positions
    template <auto CDeterministicMNState::*Field, uint32_t Mask>
    struct LegacyMember {
        static constexpr uint32_t mask = Mask;
        static auto& get(CDeterministicMNState& state) { return state.*Field; }
        static const auto& get(const CDeterministicMNState& state) { return state.*Field; }
    };

#define LEGACY_DMN_STATE_MEMBER(name) \
    LegacyMember<&CDeterministicMNState::name, LegacyField_##name> {}
    static constexpr auto legacy_members = boost::hana::make_tuple(
        LEGACY_DMN_STATE_MEMBER(nRegisteredHeight),
        LEGACY_DMN_STATE_MEMBER(nLastPaidHeight),
        LEGACY_DMN_STATE_MEMBER(nPoSePenalty),
        LEGACY_DMN_STATE_MEMBER(nPoSeRevivedHeight),
        LEGACY_DMN_STATE_MEMBER(nPoSeBanHeight),
        LEGACY_DMN_STATE_MEMBER(nRevocationReason),
        LEGACY_DMN_STATE_MEMBER(confirmedHash),
        LEGACY_DMN_STATE_MEMBER(confirmedHashWithProRegTxHash),
        LEGACY_DMN_STATE_MEMBER(keyIDOwner),
        LEGACY_DMN_STATE_MEMBER(pubKeyOperator),
        LEGACY_DMN_STATE_MEMBER(keyIDVoting),
        LEGACY_DMN_STATE_MEMBER(netInfo),
        LEGACY_DMN_STATE_MEMBER(scriptPayout),
        LEGACY_DMN_STATE_MEMBER(scriptOperatorPayout),
        LEGACY_DMN_STATE_MEMBER(nConsecutivePayments),
        LEGACY_DMN_STATE_MEMBER(platformNodeID),
        LEGACY_DMN_STATE_MEMBER(platformP2PPort),
        LEGACY_DMN_STATE_MEMBER(platformHTTPPort),
        LEGACY_DMN_STATE_MEMBER(nVersion)
    );
#undef LEGACY_DMN_STATE_MEMBER

public:
    uint32_t fields{0};
    CDeterministicMNState state;

    CDeterministicMNStateDiffLegacy() = default;

    template <typename Stream>
    CDeterministicMNStateDiffLegacy(deserialize_type, Stream& s) { s >> *this; }

    // Deserialize using legacy format
    SERIALIZE_METHODS(CDeterministicMNStateDiffLegacy, obj)
    {
        READWRITE(VARINT(obj.fields));

        bool read_pubkey{false};
        boost::hana::for_each(legacy_members, [&](auto&& member) {
            using BaseType = std::decay_t<decltype(member)>;
            if constexpr (BaseType::mask == LegacyField_pubKeyOperator) {
                if (obj.fields & member.mask) {
                    SER_READ(obj, read_pubkey = true);
                    READWRITE(CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.state.pubKeyOperator),
                                                              obj.state.nVersion == ProTxVersion::LegacyBLS));
                }
            } else if constexpr (BaseType::mask == LegacyField_netInfo) {
                if (obj.fields & member.mask) {
                    // Legacy format supports non-extended addresses only
                    READWRITE(NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.state.netInfo), false));
                }
            } else {
                if (obj.fields & member.mask) {
                    READWRITE(member.get(obj.state));
                }
            }
        });

        if (read_pubkey) {
            SER_READ(obj, obj.fields |= LegacyField_nVersion);
            SER_READ(obj, obj.state.pubKeyOperator.SetLegacy(obj.state.nVersion == ProTxVersion::LegacyBLS));
        }
    }

    // Convert to new format
    CDeterministicMNStateDiff ToNewFormat() const
    {
        CDeterministicMNStateDiff newDiff;
        newDiff.state = state;

        // Convert field bits to new positions
#define LEGACY_DMN_STATE_BITS(name) \
    if (fields & LegacyField_##name) newDiff.fields |= CDeterministicMNStateDiff::Field_##name;
        LEGACY_DMN_STATE_BITS(nVersion)
        LEGACY_DMN_STATE_BITS(nRegisteredHeight)
        LEGACY_DMN_STATE_BITS(nLastPaidHeight)
        LEGACY_DMN_STATE_BITS(nPoSePenalty)
        LEGACY_DMN_STATE_BITS(nPoSeRevivedHeight)
        LEGACY_DMN_STATE_BITS(nPoSeBanHeight)
        LEGACY_DMN_STATE_BITS(nRevocationReason)
        LEGACY_DMN_STATE_BITS(confirmedHash)
        LEGACY_DMN_STATE_BITS(confirmedHashWithProRegTxHash)
        LEGACY_DMN_STATE_BITS(keyIDOwner)
        LEGACY_DMN_STATE_BITS(pubKeyOperator)
        LEGACY_DMN_STATE_BITS(keyIDVoting)
        LEGACY_DMN_STATE_BITS(netInfo)
        LEGACY_DMN_STATE_BITS(scriptPayout)
        LEGACY_DMN_STATE_BITS(scriptOperatorPayout)
        LEGACY_DMN_STATE_BITS(nConsecutivePayments)
        LEGACY_DMN_STATE_BITS(platformNodeID)
        LEGACY_DMN_STATE_BITS(platformP2PPort)
        LEGACY_DMN_STATE_BITS(platformHTTPPort)
#undef LEGACY_DMN_STATE_BITS

        return newDiff;
    }
};

#endif // BITCOIN_EVO_DMNSTATE_H
