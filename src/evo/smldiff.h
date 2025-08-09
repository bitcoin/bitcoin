// Copyright (c) 2017-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SMLDIFF_H
#define BITCOIN_EVO_SMLDIFF_H

#include <evo/simplifiedmns.h>

#include <bls/bls.h>
#include <evo/dmn_types.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <merkleblock.h>
#include <netaddress.h>
#include <pubkey.h>
#include <sync.h>
#include <threadsafety.h>

class CBlockIndex;
class CDeterministicMNManager;
class UniValue;
class ChainstateManager;

namespace llmq {
class CFinalCommitment;
class CQuorumBlockProcessor;
class CQuorumManager;
} // namespace llmq

extern RecursiveMutex cs_main;

/// P2P messages

class CGetSimplifiedMNListDiff
{
public:
    uint256 baseBlockHash;
    uint256 blockHash;

    SERIALIZE_METHODS(CGetSimplifiedMNListDiff, obj) { READWRITE(obj.baseBlockHash, obj.blockHash); }
};

class CSimplifiedMNListDiff
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint256 baseBlockHash;
    uint256 blockHash;
    CPartialMerkleTree cbTxMerkleTree;
    CMutableTransaction cbTx;
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
        if ((s.GetType() & SER_NETWORK) && s.GetVersion() >= BLS_SCHEME_PROTO_VERSION &&
            s.GetVersion() < MNLISTDIFF_VERSION_ORDER) {
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

#endif // BITCOIN_EVO_SMLDIFF_H
