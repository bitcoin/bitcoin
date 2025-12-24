// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SNAPSHOT_H
#define BITCOIN_LLMQ_SNAPSHOT_H

#include <evo/smldiff.h>
#include <llmq/commitment.h>
#include <llmq/params.h>
#include <saltedhasher.h>
#include <serialize.h>
#include <sync.h>
#include <threadsafety.h>
#include <unordered_lru_cache.h>
#include <util/irange.h>

#include <optional>

class CBlockIndex;
class CEvoDB;
struct RPCResult;
namespace llmq {
class CQuorumBlockProcessor;
class CQuorumManager;
} // namespace llmq

class UniValue;

enum class SnapshotSkipMode : int {
    MODE_NO_SKIPPING = 0,
    MODE_SKIPPING_ENTRIES = 1,
    MODE_NO_SKIPPING_ENTRIES = 2,
    MODE_ALL_SKIPPED = 3
};
template<> struct is_serializable_enum<SnapshotSkipMode> : std::true_type {};

namespace llmq {
constexpr int WORK_DIFF_DEPTH{8};

class CQuorumSnapshot
{
public:
    std::vector<bool> activeQuorumMembers;
    SnapshotSkipMode mnSkipListMode{SnapshotSkipMode::MODE_NO_SKIPPING};
    std::vector<int> mnSkipList;

public:
    CQuorumSnapshot();
    CQuorumSnapshot(std::vector<bool> active_quorum_members, SnapshotSkipMode skip_mode, std::vector<int> skip_list);
    ~CQuorumSnapshot();

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

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
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

struct CycleBase {
    CQuorumSnapshot m_snap;
    const CBlockIndex* m_cycle_index{nullptr};
};

struct CycleData : public CycleBase {
    CSimplifiedMNListDiff m_diff;
    const CBlockIndex* m_work_index{nullptr};
};

class CQuorumRotationInfo
{
public:
    bool extraShare{false};
    CycleData cycleHMinusC;
    CycleData cycleHMinus2C;
    CycleData cycleHMinus3C;
    std::optional<CycleData> cycleHMinus4C;
    CSimplifiedMNListDiff mnListDiffTip;
    CSimplifiedMNListDiff mnListDiffH;
    std::vector<llmq::CFinalCommitment> lastCommitmentPerIndex;
    std::vector<CQuorumSnapshot> quorumSnapshotList;
    std::vector<CSimplifiedMNListDiff> mnListDiffList;

public:
    CQuorumRotationInfo();
    CQuorumRotationInfo(const CQuorumRotationInfo& dmn) = delete;
    ~CQuorumRotationInfo();

    template <typename Stream, typename Operation>
    inline void SerializationOpBase(Stream& s, Operation ser_action)
    {
        READWRITE(cycleHMinusC.m_snap,
                  cycleHMinus2C.m_snap,
                  cycleHMinus3C.m_snap,
                  mnListDiffTip,
                  mnListDiffH,
                  cycleHMinusC.m_diff,
                  cycleHMinus2C.m_diff,
                  cycleHMinus3C.m_diff,
                  extraShare);
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        const_cast<CQuorumRotationInfo*>(this)->SerializationOpBase(s, CSerActionSerialize());

        if (extraShare) {
            // Needed to maintain compatibility with existing on-disk format
            ::Serialize(s, cycleHMinus4C.value_or(CycleData{}).m_snap);
            ::Serialize(s, cycleHMinus4C.value_or(CycleData{}).m_diff);
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

        if (extraShare) {
            CycleData val{};
            ::Unserialize(s, val.m_snap);
            ::Unserialize(s, val.m_diff);
            cycleHMinus4C = val;
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

    std::vector<CycleData*> GetCycles();
    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;
};

bool BuildQuorumRotationInfo(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                             const ChainstateManager& chainman, const CQuorumManager& qman,
                             const CQuorumBlockProcessor& qblockman, const CGetQuorumRotationInfo& request,
                             bool use_legacy_construction, CQuorumRotationInfo& response, std::string& errorRet)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
uint256 GetLastBaseBlockHash(Span<const CBlockIndex*> baseBlockIndexes, const CBlockIndex* blockIndex,
                             bool use_legacy_construction);

class CQuorumSnapshotManager
{
private:
    mutable RecursiveMutex snapshotCacheCs;

    CEvoDB& m_evoDb;

    Uint256LruHashMap<CQuorumSnapshot> quorumSnapshotCache GUARDED_BY(snapshotCacheCs);

public:
    CQuorumSnapshotManager() = delete;
    CQuorumSnapshotManager(const CQuorumSnapshotManager&) = delete;
    CQuorumSnapshotManager& operator=(const CQuorumSnapshotManager&) = delete;
    explicit CQuorumSnapshotManager(CEvoDB& evoDb);
    ~CQuorumSnapshotManager();

    std::optional<CQuorumSnapshot> GetSnapshotForBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    void StoreSnapshotForBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, const CQuorumSnapshot& snapshot);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_SNAPSHOT_H
