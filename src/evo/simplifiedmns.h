// Copyright (c) 2017-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SIMPLIFIEDMNS_H
#define BITCOIN_EVO_SIMPLIFIEDMNS_H

#include <bls/bls.h>
#include <evo/dmn_types.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <merkleblock.h>
#include <netaddress.h>
#include <pubkey.h>
#include <util/pointer.h>

#include <memory>
#include <vector>

class UniValue;

class CSimplifiedMNListEntry
{
public:
    uint256 proRegTxHash;
    uint256 confirmedHash;
    std::shared_ptr<NetInfoInterface> netInfo{nullptr};
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    bool isValid{false};
    uint16_t platformHTTPPort{0};
    uint160 platformNodeID{};
    CScript scriptPayout; // mem-only
    CScript scriptOperatorPayout; // mem-only
    uint16_t nVersion{ProTxVersion::LegacyBLS};
    MnType nType{MnType::Regular};

    CSimplifiedMNListEntry() = default;
    CSimplifiedMNListEntry(const uint256& proreg_tx_hash, const uint256& confirmed_hash,
                           const std::shared_ptr<NetInfoInterface>& net_info, const CBLSLazyPublicKey& pubkey_operator,
                           const CKeyID& keyid_voting, bool is_valid, uint16_t platform_http_port,
                           const uint160& platform_node_id, const CScript& script_payout,
                           const CScript& script_operator_payout, uint16_t version, MnType type);

    bool operator==(const CSimplifiedMNListEntry& rhs) const
    {
        return proRegTxHash == rhs.proRegTxHash &&
               confirmedHash == rhs.confirmedHash &&
               util::shared_ptr_equal(netInfo, rhs.netInfo) &&
               pubKeyOperator == rhs.pubKeyOperator &&
               keyIDVoting == rhs.keyIDVoting &&
               isValid == rhs.isValid &&
               nVersion == rhs.nVersion &&
               nType == rhs.nType &&
               platformHTTPPort == rhs.platformHTTPPort &&
               platformNodeID == rhs.platformNodeID;
    }

    bool operator!=(const CSimplifiedMNListEntry& rhs) const
    {
        return !(rhs == *this);
    }

    SERIALIZE_METHODS(CSimplifiedMNListEntry, obj)
    {
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() >= SMNLE_VERSIONED_PROTO_VERSION) {
            READWRITE(obj.nVersion);
        }
        READWRITE(
                obj.proRegTxHash,
                obj.confirmedHash,
                NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.netInfo),
                                  obj.nVersion >= ProTxVersion::ExtAddr),
                CBLSLazyPublicKeyVersionWrapper(const_cast<CBLSLazyPublicKey&>(obj.pubKeyOperator), (obj.nVersion == ProTxVersion::LegacyBLS)),
                obj.keyIDVoting,
                obj.isValid
                );
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() < DMN_TYPE_PROTO_VERSION) {
            return;
        }
        if (obj.nVersion >= ProTxVersion::BasicBLS) {
            READWRITE(obj.nType);
            if (obj.nType == MnType::Evo) {
                READWRITE(obj.platformHTTPPort);
                READWRITE(obj.platformNodeID);
            }
        }
    }

    uint256 CalcHash() const;

    std::string ToString() const;
    [[nodiscard]] UniValue ToJson(bool extended = false) const;
};

class CSimplifiedMNList
{
public:
    std::vector<std::unique_ptr<CSimplifiedMNListEntry>> mnList;

    CSimplifiedMNList() = default;

    // This constructor from std::vector is used in unit-tests
    explicit CSimplifiedMNList(std::vector<std::unique_ptr<CSimplifiedMNListEntry>>&& smlEntries);

    uint256 CalcMerkleRoot(bool* pmutated = nullptr) const;
    bool operator==(const CSimplifiedMNList& rhs) const;
};

bool CalcCbTxMerkleRootMNList(uint256& merkleRootRet, std::shared_ptr<const CSimplifiedMNList> sml,
                              BlockValidationState& state);

#endif // BITCOIN_EVO_SIMPLIFIEDMNS_H
