// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSTANTSEND_DB_H
#define BITCOIN_INSTANTSEND_DB_H

#include <saltedhasher.h>
#include <sync.h>
#include <uint256.h>
#include <util/hasher.h>

#include <instantsend/lock.h>
#include <unordered_lru_cache.h>

#include <gsl/pointers.h>

#include <cstdint>
#include <memory>
#include <unordered_map>

class CBlock;
class CBlockIndex;
class CDBBatch;
class CDBWrapper;
class COutPoint;

namespace instantsend {
class CInstantSendDb
{
private:
    mutable Mutex cs_db;

    static constexpr int CURRENT_VERSION{1};

    int best_confirmed_height GUARDED_BY(cs_db){0};

    std::unique_ptr<CDBWrapper> db GUARDED_BY(cs_db){nullptr};
    mutable Uint256LruHashMap<InstantSendLockPtr, 10000> islockCache GUARDED_BY(cs_db);
    mutable Uint256LruHashMap<uint256, 10000> txidCache GUARDED_BY(cs_db);

    mutable unordered_lru_cache<COutPoint, uint256, SaltedOutpointHasher, 10000> outpointCache GUARDED_BY(cs_db);
    void WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    void RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    /**
     * This method removes a InstantSend Lock from the database and is called when a tx with an IS lock is confirmed and Chainlocked
     * @param batch Object used to batch many calls together
     * @param hash The hash of the InstantSend Lock
     * @param islock The InstantSend Lock object itself
     * @param keep_cache Should we still keep corresponding entries in the cache or not
     */
    void RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, const InstantSendLock& islock,
                               bool keep_cache = true) EXCLUSIVE_LOCKS_REQUIRED(cs_db);
    /**
     * Marks an InstantSend Lock as archived.
     * @param batch Object used to batch many calls together
     * @param hash The hash of the InstantSend Lock
     * @param nHeight The height that the transaction was included at
     */
    void WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs_db);
    /**
     * Gets a vector of IS Lock hashes of the IS Locks which rely on or are children of the parent IS Lock
     * @param parent The hash of the parent IS Lock
     * @return Returns a vector of IS Lock hashes
     */
    std::vector<uint256> GetInstantSendLocksByParent(const uint256& parent) const EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    /**
     * See GetInstantSendLockByHash
     */
    InstantSendLockPtr GetInstantSendLockByHashInternal(const uint256& hash, bool use_cache = true) const EXCLUSIVE_LOCKS_REQUIRED(cs_db);

    /**
     * See GetInstantSendLockHashByTxid
     */
    uint256 GetInstantSendLockHashByTxidInternal(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs_db);


    void Upgrade(bool unitTests) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);

public:
    explicit CInstantSendDb(bool unitTests, bool fWipe);
    ~CInstantSendDb();

    /**
     * This method is called when an InstantSend Lock is processed and adds the lock to the database
     * @param hash The hash of the InstantSend Lock
     * @param islock The InstantSend Lock object itself
     */
    void WriteNewInstantSendLock(const uint256& hash, const InstantSendLockPtr& islock) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * This method updates a DB entry for an InstantSend Lock from being not included in a block to being included in a block
     * @param hash The hash of the InstantSend Lock
     * @param nHeight The height that the transaction was included at
     */
    void WriteInstantSendLockMined(const uint256& hash, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Archives and deletes all IS Locks which were mined into a block before nUntilHeight
     * @param nUntilHeight Removes all IS Locks confirmed up until nUntilHeight
     * @return returns an unordered_map of the hash of the IS Locks and a pointer object to the IS Locks for all IS Locks which were removed
     */
    Uint256HashMap<InstantSendLockPtr> RemoveConfirmedInstantSendLocks(int nUntilHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Removes IS Locks from the archive if the tx was confirmed 100 blocks before nUntilHeight
     * @param nUntilHeight the height from which to base the remove of archive IS Locks
     */
    void RemoveArchivedInstantSendLocks(int nUntilHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    void WriteBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock, gsl::not_null<const CBlockIndex*> pindexConnected) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    void RemoveBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    bool KnownInstantSendLock(const uint256& islockHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Gets the number of IS Locks which have not been confirmed by a block
     * @return size_t value of the number of IS Locks not confirmed by a block
     */
    size_t GetInstantSendLockCount() const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Gets a pointer to the IS Lock based on the hash
     * @param hash The hash of the IS Lock
     * @param use_cache Should we try using the cache first or not
     * @return A Pointer object to the IS Lock, returns nullptr if it doesn't exist
     */
    InstantSendLockPtr GetInstantSendLockByHash(const uint256& hash, bool use_cache = true) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db)
    {
        LOCK(cs_db);
        return GetInstantSendLockByHashInternal(hash, use_cache);
    };
    /**
     * Gets an IS Lock hash based on the txid the IS Lock is for
     * @param txid The txid which is being searched for
     * @return Returns the hash the IS Lock of the specified txid, returns uint256() if it doesn't exist
     */
    uint256 GetInstantSendLockHashByTxid(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db)
    {
        LOCK(cs_db);
        return GetInstantSendLockHashByTxidInternal(txid);
    };
    /**
     * Gets an IS Lock pointer from the txid given
     * @param txid The txid for which the IS Lock Pointer is being returned
     * @return Returns the IS Lock Pointer associated with the txid, returns nullptr if it doesn't exist
     */
    InstantSendLockPtr GetInstantSendLockByTxid(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Gets an IS Lock pointer from an input given
     * @param outpoint Since all inputs are really just outpoints that are being spent
     * @return IS Lock Pointer associated with that input.
     */
    InstantSendLockPtr GetInstantSendLockByInput(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    /**
     * Called when a ChainLock invalidated a IS Lock, removes any chained/children IS Locks and the invalidated IS Lock
     * @param islockHash IS Lock hash which has been invalidated
     * @param txid Transaction id associated with the islockHash
     * @param nHeight height of the block which received a chainlock and invalidated the IS Lock
     * @return A vector of IS Lock hashes of all IS Locks removed
     */
    std::vector<uint256> RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
};
} // namespace instantsend

#endif // BITCOIN_INSTANTSEND_DB_H
