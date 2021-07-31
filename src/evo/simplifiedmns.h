// Copyright (c) 2017-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SIMPLIFIEDMNS_H
#define BITCOIN_EVO_SIMPLIFIEDMNS_H

#include <bls/bls.h>

#include <merkleblock.h>
#include <pubkey.h>

class UniValue;
class CDeterministicMNList;
class CDeterministicMN;

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

    SERIALIZE_METHODS(CSimplifiedMNListEntry, obj)
    {
        READWRITE(
                obj.proRegTxHash,
                obj.confirmedHash,
                obj.service,
                obj.pubKeyOperator,
                obj.keyIDVoting,
                obj.isValid
                );
    }

    uint256 CalcHash() const;

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};

class CSimplifiedMNList
{
public:
    std::vector<std::unique_ptr<CSimplifiedMNListEntry>> mnList;

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

    SERIALIZE_METHODS(CGetSimplifiedMNListDiff, obj)
    {
        READWRITE(obj.baseBlockHash, obj.blockHash);
    }
};

class CSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;
    CPartialMerkleTree cbTxMerkleTree;
    CTransactionRef cbTx;
    std::vector<uint256> deletedMNs;
    std::vector<CSimplifiedMNListEntry> mnList;

    // starting with proto version LLMQS_PROTO_VERSION, we also transfer changes in active quorums
    std::vector<std::pair<uint8_t, uint256>> deletedQuorums; // p<LLMQType, quorumHash>
    std::vector<llmq::CFinalCommitment> newQuorums;

    SERIALIZE_METHODS(CSimplifiedMNListDiff, obj)
    {
        READWRITE(obj.baseBlockHash, obj.blockHash, obj.cbTxMerkleTree, obj.cbTx, obj.deletedMNs, obj.mnList);

        if (s.GetVersion() >= LLMQS_PROTO_VERSION) {
            READWRITE(obj.deletedQuorums, obj.newQuorums);
        }
    }

    CSimplifiedMNListDiff();
    ~CSimplifiedMNListDiff();

    bool BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex);

    void ToJson(UniValue& obj) const;
};

bool BuildSimplifiedMNListDiff(const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet);

#endif // BITCOIN_EVO_SIMPLIFIEDMNS_H
