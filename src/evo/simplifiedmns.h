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
#include <sync.h>
#include <threadsafety.h>
#include <util/pointer.h>

class UniValue;
class CBlockIndex;
class CDeterministicMN;
class CDeterministicMNList;
class CDeterministicMNManager;
class ChainstateManager;

extern RecursiveMutex cs_main;

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
                NetInfoSerWrapper(const_cast<std::shared_ptr<NetInfoInterface>&>(obj.netInfo)),
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

/// P2P messages

class CGetSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;

    SERIALIZE_METHODS(CGetSimplifiedMNListDiff, obj)
    {
        READWRITE(obj.baseBlockHash, obj.blockHash);
    }
};

class CSimplifiedMNListDiff
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint256 baseBlockHash;
    uint256 blockHash;
    CPartialMerkleTree cbTxMerkleTree;
    CTransactionRef cbTx;
    std::vector<uint256> deletedMNs;
    std::vector<CSimplifiedMNListEntry> mnList;
    uint16_t nVersion{CURRENT_VERSION};

    std::vector<std::pair<uint8_t, uint256>> deletedQuorums; // p<LLMQType, quorumHash>
    std::vector<llmq::CFinalCommitment> newQuorums;

    // Map of Chainlock Signature used for shuffling per set of quorums
    // The set of quorums is the set of indexes corresponding to entries in newQuorums
    std::map<CBLSSignature, std::set<uint16_t>> quorumsCLSigs;

    SERIALIZE_METHODS(CSimplifiedMNListDiff, obj)
    {
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() >= MNLISTDIFF_VERSION_ORDER) {
            READWRITE(obj.nVersion);
        }
        READWRITE(obj.baseBlockHash, obj.blockHash, obj.cbTxMerkleTree, obj.cbTx);
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() >= BLS_SCHEME_PROTO_VERSION && s.GetVersion() < MNLISTDIFF_VERSION_ORDER) {
            READWRITE(obj.nVersion);
        }
        READWRITE(obj.deletedMNs, obj.mnList);
        READWRITE(obj.deletedQuorums, obj.newQuorums);
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() >= MNLISTDIFF_CHAINLOCKS_PROTO_VERSION) {
            READWRITE(obj.quorumsCLSigs);
        }
    }

    CSimplifiedMNListDiff();
    ~CSimplifiedMNListDiff();

    bool BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex,
                          const llmq::CQuorumBlockProcessor& quorum_block_processor);
    bool BuildQuorumChainlockInfo(const llmq::CQuorumManager& qman, const CBlockIndex* blockIndex);

    [[nodiscard]] UniValue ToJson(bool extended = false) const;
};

bool BuildSimplifiedMNListDiff(CDeterministicMNManager& dmnman, const ChainstateManager& chainman,
                               const llmq::CQuorumBlockProcessor& qblockman, const llmq::CQuorumManager& qman,
                               const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet,
                               std::string& errorRet, bool extended = false) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif // BITCOIN_EVO_SIMPLIFIEDMNS_H
