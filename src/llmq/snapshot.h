// Copyright (c) 2021-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SNAPSHOT_H
#define BITCOIN_LLMQ_SNAPSHOT_H

#include <evo/evodb.h>
#include <evo/simplifiedmns.h>
#include <llmq/commitment.h>
#include <llmq/params.h>
#include <saltedhasher.h>
#include <serialize.h>
#include <univalue.h>
#include <unordered_lru_cache.h>
#include <util/irange.h>

#include <optional>

class CBlockIndex;
class CDeterministicMN;
class CDeterministicMNList;

namespace llmq {
class CQuorumBlockProcessor;
class CQuorumManager;

//TODO use enum class (probably)
enum SnapshotSkipMode : int {
    MODE_NO_SKIPPING = 0,
    MODE_SKIPPING_ENTRIES = 1,
    MODE_NO_SKIPPING_ENTRIES = 2,
    MODE_ALL_SKIPPED = 3
};

class CQuorumSnapshot
{
public:
    std::vector<bool> activeQuorumMembers;
    int mnSkipListMode = 0;
    std::vector<int> mnSkipList;

    CQuorumSnapshot() = default;
    CQuorumSnapshot(std::vector<bool> _activeQuorumMembers, int _mnSkipListMode, std::vector<int> _mnSkipList) :
        activeQuorumMembers(std::move(_activeQuorumMembers)),
        mnSkipListMode(_mnSkipListMode),
        mnSkipList(std::move(_mnSkipList))
    {
    }

    template <typename Stream, typename Operation>
    inline void SerializationOpBase(Stream& s, Operation ser_action)
    {
        READWRITE(mnSkipListMode);
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        const_cast<CQuorumSnapshot*>(this)->SerializationOpBase(s, CSerActionSerialize());

        WriteCompactSize(s, activeQuorumMembers.size());
        WriteFixedBitSet(s, activeQuorumMembers, activeQuorumMembers.size());
        WriteCompactSize(s, mnSkipList.size());
        for (const auto& obj : mnSkipList) {
            s << obj;
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        SerializationOpBase(s, CSerActionUnserialize());

        size_t cnt = ReadCompactSize(s);
        ReadFixedBitSet(s, activeQuorumMembers, cnt);
        cnt = ReadCompactSize(s);
        for ([[maybe_unused]] const auto _ : irange::range(cnt)) {
            int obj;
            s >> obj;
            mnSkipList.push_back(obj);
        }
    }

    [[nodiscard]] UniValue ToJson() const;
};

class CGetQuorumRotationInfo
{
public:
    std::vector<uint256> baseBlockHashes;
    uint256 blockRequestHash;
    bool extraShare;

    SERIALIZE_METHODS(CGetQuorumRotationInfo, obj)
    {
        READWRITE(obj.baseBlockHashes, obj.blockRequestHash, obj.extraShare);
    }
};

//TODO Maybe we should split the following class:
// CQuorumSnaphot should include {creationHeight, activeQuorumMembers H_C H_2C H_3C, and skipLists H_C H_2C H3_C}
// Maybe we need to include also blockHash for heights H_C H_2C H_3C
// CSnapshotInfo should include CQuorumSnaphot + mnListDiff Tip H H_C H_2C H3_C
class CQuorumRotationInfo
{
public:
    CQuorumSnapshot quorumSnapshotAtHMinusC;
    CQuorumSnapshot quorumSnapshotAtHMinus2C;
    CQuorumSnapshot quorumSnapshotAtHMinus3C;

    CSimplifiedMNListDiff mnListDiffTip;
    CSimplifiedMNListDiff mnListDiffH;
    CSimplifiedMNListDiff mnListDiffAtHMinusC;
    CSimplifiedMNListDiff mnListDiffAtHMinus2C;
    CSimplifiedMNListDiff mnListDiffAtHMinus3C;

    bool extraShare{false};
    std::optional<CQuorumSnapshot> quorumSnapshotAtHMinus4C;
    std::optional<CSimplifiedMNListDiff> mnListDiffAtHMinus4C;

    std::vector<llmq::CFinalCommitment> lastCommitmentPerIndex;
    std::vector<CQuorumSnapshot> quorumSnapshotList;
    std::vector<CSimplifiedMNListDiff> mnListDiffList;

    template <typename Stream, typename Operation>
    inline void SerializationOpBase(Stream& s, Operation ser_action)
    {
        READWRITE(quorumSnapshotAtHMinusC,
                  quorumSnapshotAtHMinus2C,
                  quorumSnapshotAtHMinus3C,
                  mnListDiffTip,
                  mnListDiffH,
                  mnListDiffAtHMinusC,
                  mnListDiffAtHMinus2C,
                  mnListDiffAtHMinus3C,
                  extraShare);
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        const_cast<CQuorumRotationInfo*>(this)->SerializationOpBase(s, CSerActionSerialize());

        if (extraShare && quorumSnapshotAtHMinus4C.has_value()) {
            ::Serialize(s, quorumSnapshotAtHMinus4C.value());
        }

        if (extraShare && mnListDiffAtHMinus4C.has_value()) {
            ::Serialize(s, mnListDiffAtHMinus4C.value());
        }

        WriteCompactSize(s, lastCommitmentPerIndex.size());
        for (const auto& obj : lastCommitmentPerIndex) {
            ::Serialize(s, obj);
        }

        WriteCompactSize(s, quorumSnapshotList.size());
        for (const auto& obj : quorumSnapshotList) {
            ::Serialize(s, obj);
        }

        WriteCompactSize(s, mnListDiffList.size());
        for (const auto& obj : mnListDiffList) {
            ::Serialize(s, obj);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        SerializationOpBase(s, CSerActionUnserialize());

        if (extraShare && quorumSnapshotAtHMinus4C.has_value()) {
            ::Unserialize(s, quorumSnapshotAtHMinus4C.value());
        }

        if (extraShare && mnListDiffAtHMinus4C.has_value()) {
            ::Unserialize(s, mnListDiffAtHMinus4C.value());
        }

        size_t cnt = ReadCompactSize(s);
        for ([[maybe_unused]] const auto _ : irange::range(cnt)) {
            CFinalCommitment qc;
            ::Unserialize(s, qc);
            lastCommitmentPerIndex.push_back(std::move(qc));
        }

        cnt = ReadCompactSize(s);
        for ([[maybe_unused]] const auto _ : irange::range(cnt)) {
            CQuorumSnapshot snap;
            ::Unserialize(s, snap);
            quorumSnapshotList.push_back(std::move(snap));
        }

        cnt = ReadCompactSize(s);
        for ([[maybe_unused]] const auto _ : irange::range(cnt)) {
            CSimplifiedMNListDiff mnlist;
            ::Unserialize(s, mnlist);
            mnListDiffList.push_back(std::move(mnlist));
        }
    }

    CQuorumRotationInfo() = default;
    CQuorumRotationInfo(const CQuorumRotationInfo& dmn) {}

    [[nodiscard]] UniValue ToJson() const;
};

bool BuildQuorumRotationInfo(const CGetQuorumRotationInfo& request, CQuorumRotationInfo& response,
                             const CQuorumManager& qman, const CQuorumBlockProcessor& quorumBlockProcessor, std::string& errorRet);
uint256 GetLastBaseBlockHash(Span<const CBlockIndex*> baseBlockIndexes, const CBlockIndex* blockIndex);

class CQuorumSnapshotManager
{
private:
    mutable RecursiveMutex snapshotCacheCs;

    CEvoDB& m_evoDb;

    unordered_lru_cache<uint256, CQuorumSnapshot, StaticSaltedHasher> quorumSnapshotCache GUARDED_BY(snapshotCacheCs);

public:
    explicit CQuorumSnapshotManager(CEvoDB& evoDb) :
        m_evoDb(evoDb), quorumSnapshotCache(32) {}

    std::optional<CQuorumSnapshot> GetSnapshotForBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    void StoreSnapshotForBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, const CQuorumSnapshot& snapshot);
};

extern std::unique_ptr<CQuorumSnapshotManager> quorumSnapshotManager;

} // namespace llmq

#endif //BITCOIN_LLMQ_SNAPSHOT_H
