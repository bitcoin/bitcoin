// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_PROVIDERTX_H
#define BITCOIN_EVO_PROVIDERTX_H

#include <bls/bls.h>
#include <evo/specialtx.h>
#include <primitives/transaction.h>

#include <consensus/validation.h>
#include <evo/dmn_types.h>
#include <key_io.h>
#include <netaddress.h>
#include <pubkey.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/underlying.h>

class CBlockIndex;
class CCoinsViewCache;
class TxValidationState;

class CProRegTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_REGISTER;
    static constexpr uint16_t LEGACY_BLS_VERSION = 1;
    static constexpr uint16_t BASIC_BLS_VERSION = 2;

    [[nodiscard]] static constexpr auto GetVersion(const bool is_basic_scheme_active) -> uint16_t
    {
        return is_basic_scheme_active ? BASIC_BLS_VERSION : LEGACY_BLS_VERSION;
    }

    uint16_t nVersion{LEGACY_BLS_VERSION};                 // message version
    MnType nType{MnType::Regular};
    uint16_t nMode{0};                                     // only 0 supported for now
    COutPoint collateralOutpoint{uint256(), (uint32_t)-1}; // if hash is null, we refer to a ProRegTx output
    CService addr;
    uint160 platformNodeID{};
    uint16_t platformP2PPort{0};
    uint16_t platformHTTPPort{0};
    CKeyID keyIDOwner;
    CBLSPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    uint16_t nOperatorReward{0};
    CScript scriptPayout;
    uint256 inputsHash; // replay protection
    std::vector<unsigned char> vchSig;

    SERIALIZE_METHODS(CProRegTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 || obj.nVersion > BASIC_BLS_VERSION) {
            // unknown version, bail out early
            return;
        }
        READWRITE(
                obj.nType,
                obj.nMode,
                obj.collateralOutpoint,
                obj.addr,
                obj.keyIDOwner,
                CBLSPublicKeyVersionWrapper(const_cast<CBLSPublicKey&>(obj.pubKeyOperator), (obj.nVersion == LEGACY_BLS_VERSION)),
                obj.keyIDVoting,
                obj.nOperatorReward,
                obj.scriptPayout,
                obj.inputsHash
        );
        if (obj.nVersion == BASIC_BLS_VERSION && obj.nType == MnType::HighPerformance) {
            READWRITE(
                obj.platformNodeID,
                obj.platformP2PPort,
                obj.platformHTTPPort);
        }
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
    }

    // When signing with the collateral key, we don't sign the hash but a generated message instead
    // This is needed for HW wallet support which can only sign text messages as of now
    std::string MakeSignString() const;

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", nVersion);
        obj.pushKV("type", ToUnderlying(nType));
        obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
        obj.pushKV("collateralIndex", (int)collateralOutpoint.n);
        obj.pushKV("service", addr.ToString(false));
        obj.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
        obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));

        CTxDestination dest;
        if (ExtractDestination(scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
        obj.pushKV("pubKeyOperator", pubKeyOperator.ToString(nVersion == LEGACY_BLS_VERSION));
        obj.pushKV("operatorReward", (double)nOperatorReward / 100);
        if (nType == MnType::HighPerformance) {
            obj.pushKV("platformNodeID", platformNodeID.ToString());
            obj.pushKV("platformP2PPort", platformP2PPort);
            obj.pushKV("platformHTTPPort", platformHTTPPort);
        }
        obj.pushKV("inputsHash", inputsHash.ToString());
    }

    bool IsTriviallyValid(bool is_bls_legacy_scheme, TxValidationState& state) const;
};

class CProUpServTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    static constexpr uint16_t LEGACY_BLS_VERSION = 1;
    static constexpr uint16_t BASIC_BLS_VERSION = 2;

    [[nodiscard]] static constexpr auto GetVersion(const bool is_basic_scheme_active) -> uint16_t
    {
        return is_basic_scheme_active ? BASIC_BLS_VERSION : LEGACY_BLS_VERSION;
    }

    uint16_t nVersion{LEGACY_BLS_VERSION}; // message version
    MnType nType{MnType::Regular};
    uint256 proTxHash;
    CService addr;
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
        if (obj.nVersion == 0 || obj.nVersion > BASIC_BLS_VERSION) {
            // unknown version, bail out early
            return;
        }
        if (obj.nVersion == BASIC_BLS_VERSION) {
            READWRITE(
                obj.nType);
        }
        READWRITE(
                obj.proTxHash,
                obj.addr,
                obj.scriptOperatorPayout,
                obj.inputsHash
        );
        if (obj.nVersion == BASIC_BLS_VERSION && obj.nType == MnType::HighPerformance) {
            READWRITE(
                obj.platformNodeID,
                obj.platformP2PPort,
                obj.platformHTTPPort);
        }
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(
                    CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), (obj.nVersion == LEGACY_BLS_VERSION), true)
            );
        }
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", nVersion);
        obj.pushKV("type", ToUnderlying(nType));
        obj.pushKV("proTxHash", proTxHash.ToString());
        obj.pushKV("service", addr.ToString(false));
        CTxDestination dest;
        if (ExtractDestination(scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
        if (nType == MnType::HighPerformance) {
            obj.pushKV("platformNodeID", platformNodeID.ToString());
            obj.pushKV("platformP2PPort", platformP2PPort);
            obj.pushKV("platformHTTPPort", platformHTTPPort);
        }
        obj.pushKV("inputsHash", inputsHash.ToString());
    }

    bool IsTriviallyValid(bool is_bls_legacy_scheme, TxValidationState& state) const;
};

class CProUpRegTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    static constexpr uint16_t LEGACY_BLS_VERSION = 1;
    static constexpr uint16_t BASIC_BLS_VERSION = 2;

    [[nodiscard]] static constexpr auto GetVersion(const bool is_basic_scheme_active) -> uint16_t
    {
        return is_basic_scheme_active ? BASIC_BLS_VERSION : LEGACY_BLS_VERSION;
    }

    uint16_t nVersion{LEGACY_BLS_VERSION}; // message version
    uint256 proTxHash;
    uint16_t nMode{0}; // only 0 supported for now
    CBLSPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    CScript scriptPayout;
    uint256 inputsHash; // replay protection
    std::vector<unsigned char> vchSig;

    SERIALIZE_METHODS(CProUpRegTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 || obj.nVersion > BASIC_BLS_VERSION) {
            // unknown version, bail out early
            return;
        }
        READWRITE(
                obj.proTxHash,
                obj.nMode,
                CBLSPublicKeyVersionWrapper(const_cast<CBLSPublicKey&>(obj.pubKeyOperator), (obj.nVersion == LEGACY_BLS_VERSION)),
                obj.keyIDVoting,
                obj.scriptPayout,
                obj.inputsHash
        );
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(
                    obj.vchSig
            );
        }
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", nVersion);
        obj.pushKV("proTxHash", proTxHash.ToString());
        obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
        CTxDestination dest;
        if (ExtractDestination(scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
        obj.pushKV("pubKeyOperator", pubKeyOperator.ToString(nVersion == LEGACY_BLS_VERSION));
        obj.pushKV("inputsHash", inputsHash.ToString());
    }

    bool IsTriviallyValid(bool is_bls_legacy_scheme, TxValidationState& state) const;
};

class CProUpRevTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_PROVIDER_UPDATE_REVOKE;
    static constexpr uint16_t LEGACY_BLS_VERSION = 1;
    static constexpr uint16_t BASIC_BLS_VERSION = 2;

    [[nodiscard]] static constexpr auto GetVersion(const bool is_basic_scheme_active) -> uint16_t
    {
        return is_basic_scheme_active ? BASIC_BLS_VERSION : LEGACY_BLS_VERSION;
    }

    // these are just informational and do not have any effect on the revocation
    enum {
        REASON_NOT_SPECIFIED = 0,
        REASON_TERMINATION_OF_SERVICE = 1,
        REASON_COMPROMISED_KEYS = 2,
        REASON_CHANGE_OF_KEYS = 3,
        REASON_LAST = REASON_CHANGE_OF_KEYS
    };

    uint16_t nVersion{LEGACY_BLS_VERSION}; // message version
    uint256 proTxHash;
    uint16_t nReason{REASON_NOT_SPECIFIED};
    uint256 inputsHash; // replay protection
    CBLSSignature sig;

    SERIALIZE_METHODS(CProUpRevTx, obj)
    {
        READWRITE(
                obj.nVersion
        );
        if (obj.nVersion == 0 || obj.nVersion > BASIC_BLS_VERSION) {
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
                    CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), (obj.nVersion == LEGACY_BLS_VERSION), true)
            );
        }
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", nVersion);
        obj.pushKV("proTxHash", proTxHash.ToString());
        obj.pushKV("reason", (int)nReason);
        obj.pushKV("inputsHash", inputsHash.ToString());
    }

    bool IsTriviallyValid(bool is_bls_legacy_scheme, TxValidationState& state) const;
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
