// Copyright (c) 2017-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_SIMPLIFIEDMNS_H
#define SYSCOIN_EVO_SIMPLIFIEDMNS_H

#include <merkleblock.h>
#include <pubkey.h>
#include <script/standard.h>
#include <threadsafety.h>
#include <sync.h>
extern RecursiveMutex cs_main;
class UniValue;
class CDeterministicMNList;
class CDeterministicMN;
class ChainstateManager;
namespace llmq
{
    class CFinalCommitment;
} // namespace llmq

class CSimplifiedMNListEntry
{
public:
    uint256 proRegTxHash;
    uint256 confirmedHash;
    CService service;
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    bool isValid;

public:
    CSimplifiedMNListEntry() = default;
    explicit CSimplifiedMNListEntry(const CDeterministicMN& dmn);

    bool operator==(const CSimplifiedMNListEntry& rhs) const
    {
        return proRegTxHash == rhs.proRegTxHash &&
               confirmedHash == rhs.confirmedHash &&
               service == rhs.service &&
               pubKeyOperator == rhs.pubKeyOperator &&
               keyIDVoting == rhs.keyIDVoting &&
               isValid == rhs.isValid;
    }

    bool operator!=(const CSimplifiedMNListEntry& rhs) const
    {
        return !(rhs == *this);
    }

public:
    SERIALIZE_METHODS(CSimplifiedMNListEntry, obj) {
        READWRITE(obj.proRegTxHash, obj.confirmedHash, obj.service, obj.pubKeyOperator,
        obj.keyIDVoting, obj.isValid);
    }

public:
    uint256 CalcHash() const;

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};

class CSimplifiedMNList
{
public:
    std::vector<std::unique_ptr<CSimplifiedMNListEntry>> mnList;

public:
    CSimplifiedMNList() = default;
    explicit CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries);
    explicit CSimplifiedMNList(const CDeterministicMNList& dmnList);

    uint256 CalcMerkleRoot(bool* pmutated = nullptr) const;
};

/// P2P messages

class CGetSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;

public:
    SERIALIZE_METHODS(CGetSimplifiedMNListDiff, obj) {
        READWRITE(obj.baseBlockHash, obj.blockHash);
    }
};

class CSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;
    CPartialMerkleTree cbTxMerkleTree;
    std::vector<uint256> deletedMNs;
    std::vector<CSimplifiedMNListEntry> mnList;

    // we also transfer changes in active quorums
    std::vector<std::pair<uint8_t, uint256>> deletedQuorums; // p<uint8_t, quorumHash>
    std::vector<llmq::CFinalCommitment> newQuorums;
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;

public:
    SERIALIZE_METHODS(CSimplifiedMNListDiff, obj) {
        READWRITE(obj.baseBlockHash, obj.blockHash, obj.cbTxMerkleTree,
        obj.deletedMNs, obj.mnList, obj.deletedQuorums, obj.newQuorums, obj.merkleRootMNList, obj.merkleRootQuorums);
    }

public:
    CSimplifiedMNListDiff();
    ~CSimplifiedMNListDiff();

    bool BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex);

    void ToJson(UniValue& obj) const;
};

bool BuildSimplifiedMNListDiff(ChainstateManager& chainman, const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet);

#endif //SYSCOIN_EVO_SIMPLIFIEDMNS_H
