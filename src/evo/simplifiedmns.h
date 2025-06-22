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

class UniValue;
class CDeterministicMN;
class CDeterministicMNList;
class ChainstateManager;

namespace llmq {
class CFinalCommitment;
class CQuorumBlockProcessor;
class CQuorumManager;
} // namespace llmq

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
    explicit CSimplifiedMNListEntry(const CDeterministicMN& dmn);

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
    explicit CSimplifiedMNList(const CDeterministicMNList& dmnList);

    // This constructor from std::vector is used in unit-tests
    explicit CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries);

    uint256 CalcMerkleRoot(bool* pmutated = nullptr) const;
    bool operator==(const CSimplifiedMNList& rhs) const;
};

bool CalcCbTxMerkleRootMNList(uint256& merkleRootRet, const CDeterministicMNList& sml, BlockValidationState& state);

#endif // BITCOIN_EVO_SIMPLIFIEDMNS_H
