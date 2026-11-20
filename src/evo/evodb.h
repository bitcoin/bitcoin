// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_EVODB_H
#define BITCOIN_EVO_EVODB_H

#include <dbwrapper.h>
#include <kernel/cs_main.h>
#include <sync.h>

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <thread>

class uint256;
namespace util {
struct DbWrapperParams;
} // namespace util

// "b_b" was used in the initial version of deterministic MN storage
// "b_b2" was used after compact diffs were introduced
// "b_b3" was used after masternode type introduction in evoDB
// "b_b4" was used after storing protx version for each masternode in evoDB
static const std::string EVODB_BEST_BLOCK = "b_b4";
// Released Dash software has no snapshot-chainstate detection, ignores the
// chainstate_snapshot directory, and loads the chainstate directory together
// with this legacy marker. That pair is the background chainstate's own coins
// and marker, so downgrading mid-snapshot safely reverts to background IBD.
static const std::string EVODB_DUAL_CHAINSTATE = "b_dcs";
static const std::string EVODB_SNAPSHOT_MNLIST_HASH = "b_dcs_mn";
static const std::string EVODB_BACKGROUND_MNLIST_HASH = "b_dcs_bg_mn";

enum class EvoDbIdentity {
    NORMAL,
    SNAPSHOT,
};

/** Thrown when an independently derived block payload disagrees with the copy
 *  already recorded in EvoDB. This is local state corruption (or a
 *  cross-chainstate divergence bug), never evidence about the block being
 *  processed, so it must abort the node rather than invalidate the block. */
class EvoDbInconsistencyError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/** Thrown when block-derived state that EvoDB should hold for a block is missing. Like a mismatch it
 *  is never evidence about the block, but it does not abort the node: an RPC may ask about a block
 *  whose state was never stored. */
class EvoDbMissingStateError : public EvoDbInconsistencyError
{
public:
    using EvoDbInconsistencyError::EvoDbInconsistencyError;
};

class CEvoDB;

class CEvoDBScopedCommitter
{
private:
    CEvoDB& evoDB;
    const EvoDbIdentity identity;
    bool didCommitOrRollback{false};

public:
    CEvoDBScopedCommitter(CEvoDB& _evoDB, EvoDbIdentity identity);
    ~CEvoDBScopedCommitter();

    void Commit() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void Rollback() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

class CEvoDB
{
public:
    mutable Mutex cs;
private:
    std::unique_ptr<CDBWrapper> db;

    using RootTransaction = CDBTransaction<CDBWrapper, CDBBatch>;
    using CurTransaction = CDBTransaction<RootTransaction, RootTransaction>;

    struct TransactionContext {
        CDBBatch root_batch;
        RootTransaction root_transaction;
        CurTransaction cur_transaction;

        explicit TransactionContext(CDBWrapper& db) :
            root_batch{db},
            root_transaction{db, root_batch},
            cur_transaction{root_transaction, root_transaction}
        {
        }
    };

    std::map<EvoDbIdentity, std::unique_ptr<TransactionContext>> transaction_contexts GUARDED_BY(cs);
    // The open transaction belongs to the thread that began it: it is the
    // identity of one validation execution context, not of the process. Other
    // threads reading concurrently (RPC, P2P serving, quorum construction after
    // it has released cs_main) must keep resolving against m_default_identity,
    // or a background NORMAL transaction would silently redirect them away from
    // the active snapshot's overlay.
    std::optional<EvoDbIdentity> active_transaction GUARDED_BY(cs);
    std::thread::id active_transaction_thread GUARDED_BY(cs);
    // Identity used by reads and writes outside any transaction. Tracks the
    // active chainstate so transaction-less consumers (RPC, mempool, miner,
    // P2P serving) resolve against active-chain state rather than always
    // falling back to NORMAL.
    EvoDbIdentity m_default_identity GUARDED_BY(cs){EvoDbIdentity::NORMAL};

    TransactionContext& GetContext(EvoDbIdentity identity) EXCLUSIVE_LOCKS_REQUIRED(cs);
    const TransactionContext& GetContext(EvoDbIdentity identity) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    EvoDbIdentity GetCurrentIdentity() const EXCLUSIVE_LOCKS_REQUIRED(cs);

public:
    CEvoDB() = delete;
    CEvoDB(const CEvoDB&) = delete;
    CEvoDB& operator=(const CEvoDB&) = delete;
    explicit CEvoDB(const util::DbWrapperParams& db_params);
    ~CEvoDB();

    std::unique_ptr<CEvoDBScopedCommitter> BeginTransaction(EvoDbIdentity identity = EvoDbIdentity::NORMAL)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs);

    /** Whether a block-scoped transaction is open. Writes performed outside one are never
     *  committed and trip the clean-transaction assertion at the next root commit, so callers
     *  reachable from transaction-less contexts (mempool acceptance, mining, RPC) must skip
     *  optional persistence when this is false. */
    bool HasActiveTransaction() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return active_transaction.has_value() && active_transaction_thread == std::this_thread::get_id();
    }

    CurTransaction& GetCurTransaction() EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs); // lock must be held from outside as long as the DB transaction is used
        return GetContext(GetCurrentIdentity()).cur_transaction;
    }

    template <typename K, typename V>
    bool Read(const K& key, V& value) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return GetContext(GetCurrentIdentity()).cur_transaction.Read(key, value);
    }

    template <typename K, typename V>
    void Write(const K& key, const V& value) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        GetContext(GetCurrentIdentity()).cur_transaction.Write(key, value);
    }

    /**
     * Write immutable block-derived data, accepting an identical existing value.
     * V must have canonical serialization: its bytes must be a pure function of
     * its logical content. Audited call-site types are CDeterministicMNListDiff,
     * CDeterministicMNList, AbstractEHFManager::Signals, the mined-commitment
     * pair, and CCreditPool.
     * TODO(assumeutxo): WriteDerived spot-checks are not the holistic base-state
     * comparison required at snapshot completion.
     */
    template <typename K, typename V>
    bool WriteDerived(const K& key, const V& value) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        const EvoDbIdentity identity = GetCurrentIdentity();
        auto& transaction = GetContext(identity).cur_transaction;
        V existing;
        bool write{true};
        if (transaction.Read(key, existing)) {
            CDataStream existing_stream{SER_DISK, CLIENT_VERSION};
            CDataStream value_stream{SER_DISK, CLIENT_VERSION};
            existing_stream << existing;
            value_stream << value;
            const bool matches = existing_stream.size() == value_stream.size() &&
                                 std::equal(existing_stream.begin(), existing_stream.end(), value_stream.begin());
            if (!matches) {
                LogPrintf("ERROR: CEvoDB::WriteDerived: block-derived payload mismatch in EvoDB\n");
                return false;
            }
            write = false;
        }

        // Cross-identity writes of the same key are disjoint by construction
        // (background validates blocks <= snapshot base; the snapshot chainstate
        // validates blocks > base; seeded data is flushed at activation). This check is
        // verification-only insurance for that invariant and must never suppress the
        // caller's own write.
        for (const auto& [other_identity, context] : transaction_contexts) {
            if (other_identity == identity || !context) continue;
            V pending;
            if (!context->root_transaction.ReadPending(key, pending)) continue;

            CDataStream pending_stream{SER_DISK, CLIENT_VERSION};
            CDataStream value_stream{SER_DISK, CLIENT_VERSION};
            pending_stream << pending;
            value_stream << value;
            const bool matches = pending_stream.size() == value_stream.size() &&
                                 std::equal(pending_stream.begin(), pending_stream.end(), value_stream.begin());
            if (!matches) {
                LogPrintf("ERROR: CEvoDB::WriteDerived: cross-identity block-derived payload mismatch in EvoDB\n");
                return false;
            }
        }

        if (write) transaction.Write(key, value);
        return true;
    }

    template <typename K>
    bool Exists(const K& key) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return GetContext(GetCurrentIdentity()).cur_transaction.Exists(key);
    }

    template <typename K>
    void Erase(const K& key) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        GetContext(GetCurrentIdentity()).cur_transaction.Erase(key);
    }

    CDBWrapper& GetRawDB()
    {
        return *db;
    }

    [[nodiscard]] size_t GetMemoryUsage() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        size_t result{0};
        for (const auto& [_, context] : transaction_contexts) {
            result += context->root_transaction.GetMemoryUsage();
        }
        return result;
    }

    bool CommitRootTransaction(EvoDbIdentity identity = EvoDbIdentity::NORMAL, bool sync = false)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs);

    bool IsEmpty() { return db->IsEmpty(); }

    //! Set the identity used by reads/writes outside any transaction. Must
    //! track the active chainstate: snapshot activation sets SNAPSHOT;
    //! PromoteSnapshotMarkers/DiscardSnapshotMarkers reset it to NORMAL.
    void SetDefaultIdentity(EvoDbIdentity identity) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        m_default_identity = identity;
    }

    bool ReadBestBlock(EvoDbIdentity identity, uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool VerifyBestBlock(EvoDbIdentity identity, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void WriteBestBlock(EvoDbIdentity identity, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void WriteDualChainstateMarker() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool HasDualChainstateMarker() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    //! Undo every snapshot lifecycle marker (SNAPSHOT best block, base and
    //! background MN-list hashes, dual-chainstate marker). Needed when snapshot
    //! activation is abandoned after those markers were already committed: a
    //! stale dual-chainstate marker turns supported legacy bootstrapping into
    //! "unavailable history", and a stale SNAPSHOT marker would let a later
    //! snapshot directory pass ActivateExistingSnapshot().
    void EraseSnapshotMarkers() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void WriteSnapshotBaseMNListHash(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ReadSnapshotBaseMNListHash(uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void WriteBackgroundMNListHash(const uint256& block_hash, const uint256& mn_list_hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ReadBackgroundMNListHash(uint256& block_hash, uint256& mn_list_hash) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /**
     * Atomically promote the surviving snapshot marker to the legacy NORMAL key
     * and remove all dual-chainstate metadata. Both identity transaction trees
     * must already be fully committed by the caller.
     */
    bool PromoteSnapshotMarkers(const uint256& expected_snapshot_tip) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /** Remove snapshot metadata after rejecting a snapshot, preserving NORMAL. */
    bool DiscardSnapshotMarkers() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool VerifyBestBlock(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs) { return VerifyBestBlock(EvoDbIdentity::NORMAL, hash); }
    void WriteBestBlock(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs) { WriteBestBlock(EvoDbIdentity::NORMAL, hash); }

private:
    // only CEvoDBScopedCommitter is allowed to invoke these
    friend class CEvoDBScopedCommitter;
    void CommitCurTransaction(EvoDbIdentity identity) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs);
    void RollbackCurTransaction(EvoDbIdentity identity) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs);
};

#endif // BITCOIN_EVO_EVODB_H
