// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINS_H
#define BITCOIN_COINS_H

#include <compressor.h>
#include <core_memusage.h>
#include <memusage.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <support/allocators/pool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/hasher.h>

#include <assert.h>
#include <stdint.h>

#include <functional>
#include <unordered_map>

/**
 * A UTXO entry.
 *
 * Serialized format:
 * - VARINT((coinbase ? 1 : 0) | (height << 1))
 * - the non-spent CTxOut (via TxOutCompression)
 */
class Coin
{
public:
    //! unspent transaction output
    CTxOut out;

    //! whether containing transaction was a coinbase
    unsigned int fCoinBase : 1;

    //! at which height this containing transaction was included in the active block chain
    uint32_t nHeight : 31;

    //! construct a Coin from a CTxOut and height/coinbase information.
    Coin(CTxOut&& outIn, int nHeightIn, bool fCoinBaseIn) : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
    Coin(const CTxOut& outIn, int nHeightIn, bool fCoinBaseIn) : out(outIn), fCoinBase(fCoinBaseIn),nHeight(nHeightIn) {}

    void Clear() {
        out.SetNull();
        fCoinBase = false;
        nHeight = 0;
    }

    //! empty constructor
    Coin() : fCoinBase(false), nHeight(0) { }

    bool IsCoinBase() const {
        return fCoinBase;
    }

    template<typename Stream>
    void Serialize(Stream &s) const {
        assert(!IsSpent());
        uint32_t code = nHeight * uint32_t{2} + fCoinBase;
        ::Serialize(s, VARINT(code));
        ::Serialize(s, Using<TxOutCompression>(out));
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        uint32_t code = 0;
        ::Unserialize(s, VARINT(code));
        nHeight = code >> 1;
        fCoinBase = code & 1;
        ::Unserialize(s, Using<TxOutCompression>(out));
    }

    /** Either this coin never existed (see e.g. coinEmpty in coins.cpp), or it
      * did exist and has been spent.
      */
    bool IsSpent() const {
        return out.IsNull();
    }

    size_t DynamicMemoryUsage() const {
        return memusage::DynamicUsage(out.scriptPubKey);
    }
};

struct CCoinsCacheEntry;
using CoinsCachePair = std::pair<const COutPoint, CCoinsCacheEntry>;

/**
 * A Coin in one level of the coins database caching hierarchy.
 *
 * A coin can either be:
 * - unspent or spent (in which case the Coin object will be nulled out - see Coin.Clear())
 * - DIRTY or not DIRTY
 * - FRESH or not FRESH
 *
 * Out of these 2^3 = 8 states, only some combinations are valid:
 * - unspent, FRESH, DIRTY (e.g. a new coin created in the cache)
 * - unspent, not FRESH, DIRTY (e.g. a coin changed in the cache during a reorg)
 * - unspent, not FRESH, not DIRTY (e.g. an unspent coin fetched from the parent cache)
 * - spent, FRESH, not DIRTY (e.g. a spent coin fetched from the parent cache)
 * - spent, not FRESH, DIRTY (e.g. a coin is spent and spentness needs to be flushed to the parent)
 */
struct CCoinsCacheEntry
{
private:
    /**
     * These are used to create a doubly linked list of flagged entries.
     * They are set in AddFlags and unset in ClearFlags.
     * A flagged entry is any entry that is either DIRTY, FRESH, or both.
     *
     * DIRTY entries are tracked so that only modified entries can be passed to
     * the parent cache for batch writing. This is a performance optimization
     * compared to giving all entries in the cache to the parent and having the
     * parent scan for only modified entries.
     *
     * FRESH-but-not-DIRTY coins can not occur in practice, since that would
     * mean a spent coin exists in the parent CCoinsView and not in the child
     * CCoinsViewCache. Nevertheless, if a spent coin is retrieved from the
     * parent cache, the FRESH-but-not-DIRTY coin will be tracked by the linked
     * list and deleted when Sync or Flush is called on the CCoinsViewCache.
     */
    CoinsCachePair* m_prev{nullptr};
    CoinsCachePair* m_next{nullptr};
    uint8_t m_flags{0};

public:
    Coin coin; // The actual cached data.

    enum Flags {
        /**
         * DIRTY means the CCoinsCacheEntry is potentially different from the
         * version in the parent cache. Failure to mark a coin as DIRTY when
         * it is potentially different from the parent cache will cause a
         * consensus failure, since the coin's state won't get written to the
         * parent when the cache is flushed.
         */
        DIRTY = (1 << 0),
        /**
         * FRESH means the parent cache does not have this coin or that it is a
         * spent coin in the parent cache. If a FRESH coin in the cache is
         * later spent, it can be deleted entirely and doesn't ever need to be
         * flushed to the parent. This is a performance optimization. Marking a
         * coin as FRESH when it exists unspent in the parent cache will cause a
         * consensus failure, since it might not be deleted from the parent
         * when this cache is flushed.
         */
        FRESH = (1 << 1),
    };

    CCoinsCacheEntry() noexcept = default;
    explicit CCoinsCacheEntry(Coin&& coin_) noexcept : coin(std::move(coin_)) {}
    ~CCoinsCacheEntry()
    {
        ClearFlags();
    }

    //! Adding a flag also requires a self reference to the pair that contains
    //! this entry in the CCoinsCache map and a reference to the sentinel of the
    //! flagged pair linked list.
    inline void AddFlags(uint8_t flags, CoinsCachePair& self, CoinsCachePair& sentinel) noexcept
    {
        Assume(&self.second == this);
        if (!m_flags && flags) {
            m_prev = sentinel.second.m_prev;
            m_next = &sentinel;
            sentinel.second.m_prev = &self;
            m_prev->second.m_next = &self;
        }
        m_flags |= flags;
    }
    inline void ClearFlags() noexcept
    {
        if (!m_flags) return;
        m_next->second.m_prev = m_prev;
        m_prev->second.m_next = m_next;
        m_flags = 0;
    }
    inline uint8_t GetFlags() const noexcept { return m_flags; }
    inline bool IsDirty() const noexcept { return m_flags & DIRTY; }
    inline bool IsFresh() const noexcept { return m_flags & FRESH; }

    //! Only call Next when this entry is DIRTY, FRESH, or both
    inline CoinsCachePair* Next() const noexcept {
        Assume(m_flags);
        return m_next;
    }

    //! Only call Prev when this entry is DIRTY, FRESH, or both
    inline CoinsCachePair* Prev() const noexcept {
        Assume(m_flags);
        return m_prev;
    }

    //! Only use this for initializing the linked list sentinel
    inline void SelfRef(CoinsCachePair& self) noexcept
    {
        Assume(&self.second == this);
        m_prev = &self;
        m_next = &self;
        // Set sentinel to DIRTY so we can call Next on it
        m_flags = DIRTY;
    }
};

/**
 * PoolAllocator's MAX_BLOCK_SIZE_BYTES parameter here uses sizeof the data, and adds the size
 * of 4 pointers. We do not know the exact node size used in the std::unordered_node implementation
 * because it is implementation defined. Most implementations have an overhead of 1 or 2 pointers,
 * so nodes can be connected in a linked list, and in some cases the hash value is stored as well.
 * Using an additional sizeof(void*)*4 for MAX_BLOCK_SIZE_BYTES should thus be sufficient so that
 * all implementations can allocate the nodes from the PoolAllocator.
 */
using CCoinsMap = std::unordered_map<COutPoint,
                                     CCoinsCacheEntry,
                                     SaltedOutpointHasher,
                                     std::equal_to<COutPoint>,
                                     PoolAllocator<CoinsCachePair,
                                                   sizeof(CoinsCachePair) + sizeof(void*) * 4>>;

using CCoinsMapMemoryResource = CCoinsMap::allocator_type::ResourceType;

/** Cursor for iterating over CoinsView state */
class CCoinsViewCursor
{
public:
    CCoinsViewCursor(const uint256 &hashBlockIn): hashBlock(hashBlockIn) {}
    virtual ~CCoinsViewCursor() = default;

    virtual bool GetKey(COutPoint &key) const = 0;
    virtual bool GetValue(Coin &coin) const = 0;

    virtual bool Valid() const = 0;
    virtual void Next() = 0;

    //! Get best block at the time this cursor was created
    const uint256 &GetBestBlock() const { return hashBlock; }
private:
    uint256 hashBlock;
};

/**
 * Cursor for iterating over the linked list of flagged entries in CCoinsViewCache.
 *
 * This is a helper struct to encapsulate the diverging logic between a non-erasing
 * CCoinsViewCache::Sync and an erasing CCoinsViewCache::Flush. This allows the receiver
 * of CCoinsView::BatchWrite to iterate through the flagged entries without knowing
 * the caller's intent.
 *
 * However, the receiver can still call CoinsViewCacheCursor::WillErase to see if the
 * caller will erase the entry after BatchWrite returns. If so, the receiver can
 * perform optimizations such as moving the coin out of the CCoinsCachEntry instead
 * of copying it.
 */
struct CoinsViewCacheCursor
{
    //! If will_erase is not set, iterating through the cursor will erase spent coins from the map,
    //! and other coins will be unflagged (removing them from the linked list).
    //! If will_erase is set, the underlying map and linked list will not be modified,
    //! as the caller is expected to wipe the entire map anyway.
    //! This is an optimization compared to erasing all entries as the cursor iterates them when will_erase is set.
    //! Calling CCoinsMap::clear() afterwards is faster because a CoinsCachePair cannot be coerced back into a
    //! CCoinsMap::iterator to be erased, and must therefore be looked up again by key in the CCoinsMap before being erased.
    CoinsViewCacheCursor(size_t& usage LIFETIMEBOUND,
                        CoinsCachePair& sentinel LIFETIMEBOUND,
                        CCoinsMap& map LIFETIMEBOUND,
                        bool will_erase) noexcept
        : m_usage(usage), m_sentinel(sentinel), m_map(map), m_will_erase(will_erase) {}

    inline CoinsCachePair* Begin() const noexcept { return m_sentinel.second.Next(); }
    inline CoinsCachePair* End() const noexcept { return &m_sentinel; }

    //! Return the next entry after current, possibly erasing current
    inline CoinsCachePair* NextAndMaybeErase(CoinsCachePair& current) noexcept
    {
        const auto next_entry{current.second.Next()};
        // If we are not going to erase the cache, we must still erase spent entries.
        // Otherwise clear the flags on the entry.
        if (!m_will_erase) {
            if (current.second.coin.IsSpent()) {
                m_usage -= current.second.coin.DynamicMemoryUsage();
                m_map.erase(current.first);
            } else {
                current.second.ClearFlags();
            }
        }
        return next_entry;
    }

    inline bool WillErase(CoinsCachePair& current) const noexcept { return m_will_erase || current.second.coin.IsSpent(); }
private:
    size_t& m_usage;
    CoinsCachePair& m_sentinel;
    CCoinsMap& m_map;
    bool m_will_erase;
};

/** Abstract view on the open txout dataset. */
class CCoinsView
{
public:
    //! Retrieve the Coin (unspent transaction output) for a given outpoint.
    virtual std::optional<Coin> GetCoin(const COutPoint& outpoint) const;

    //! Just check whether a given outpoint is unspent.
    virtual bool HaveCoin(const COutPoint &outpoint) const;

    //! Retrieve the block hash whose state this CCoinsView currently represents
    virtual uint256 GetBestBlock() const;

    //! Retrieve the range of blocks that may have been only partially written.
    //! If the database is in a consistent state, the result is the empty vector.
    //! Otherwise, a two-element vector is returned consisting of the new and
    //! the old block hash, in that order.
    virtual std::vector<uint256> GetHeadBlocks() const;

    //! Do a bulk modification (multiple Coin changes + BestBlock change).
    //! The passed cursor is used to iterate through the coins.
    virtual bool BatchWrite(CoinsViewCacheCursor& cursor, const uint256& hashBlock);

    //! Get a cursor to iterate over the whole state
    virtual std::unique_ptr<CCoinsViewCursor> Cursor() const;

    //! As we use CCoinsViews polymorphically, have a virtual destructor
    virtual ~CCoinsView() = default;

    //! Estimate database size (0 if not implemented)
    virtual size_t EstimateSize() const { return 0; }
};


/** CCoinsView backed by another CCoinsView */
class CCoinsViewBacked : public CCoinsView
{
protected:
    CCoinsView *base;

public:
    CCoinsViewBacked(CCoinsView *viewIn);
    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void SetBackend(CCoinsView &viewIn);
    bool BatchWrite(CoinsViewCacheCursor& cursor, const uint256 &hashBlock) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override;
    size_t EstimateSize() const override;
};


/** CCoinsView that adds a memory cache for transactions to another CCoinsView */
class CCoinsViewCache : public CCoinsViewBacked
{
private:
    const bool m_deterministic;

protected:
    /**
     * Make mutable so that we can "fill the cache" even from Get-methods
     * declared as "const".
     */
    mutable uint256 hashBlock;
    mutable CCoinsMapMemoryResource m_cache_coins_memory_resource{};
    /* The starting sentinel of the flagged entry circular doubly linked list. */
    mutable CoinsCachePair m_sentinel;
    mutable CCoinsMap cacheCoins;

    /* Cached dynamic memory usage for the inner Coin objects. */
    mutable size_t cachedCoinsUsage{0};

public:
    CCoinsViewCache(CCoinsView *baseIn, bool deterministic = false);

    /**
     * By deleting the copy constructor, we prevent accidentally using it when one intends to create a cache on top of a base cache.
     */
    CCoinsViewCache(const CCoinsViewCache &) = delete;

    // Standard CCoinsView methods
    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    void SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(CoinsViewCacheCursor& cursor, const uint256 &hashBlock) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }

    /**
     * Check if we have the given utxo already loaded in this cache.
     * The semantics are the same as HaveCoin(), but no calls to
     * the backing CCoinsView are made.
     */
    bool HaveCoinInCache(const COutPoint &outpoint) const;

    /**
     * Return a reference to Coin in the cache, or coinEmpty if not found. This is
     * more efficient than GetCoin.
     *
     * Generally, do not hold the reference returned for more than a short scope.
     * While the current implementation allows for modifications to the contents
     * of the cache while holding the reference, this behavior should not be relied
     * on! To be safe, best to not hold the returned reference through any other
     * calls to this cache.
     */
    const Coin& AccessCoin(const COutPoint &output) const;

    /**
     * Add a coin. Set possible_overwrite to true if an unspent version may
     * already exist in the cache.
     */
    void AddCoin(const COutPoint& outpoint, Coin&& coin, bool possible_overwrite);

    /**
     * Emplace a coin into cacheCoins without performing any checks, marking
     * the emplaced coin as dirty.
     *
     * NOT FOR GENERAL USE. Used only when loading coins from a UTXO snapshot.
     * @sa ChainstateManager::PopulateAndValidateSnapshot()
     */
    void EmplaceCoinInternalDANGER(COutPoint&& outpoint, Coin&& coin);

    /**
     * Spend a coin. Pass moveto in order to get the deleted data.
     * If no unspent output exists for the passed outpoint, this call
     * has no effect.
     */
    bool SpendCoin(const COutPoint &outpoint, Coin* moveto = nullptr);

    /**
     * Push the modifications applied to this cache to its base and wipe local state.
     * Failure to call this method or Sync() before destruction will cause the changes
     * to be forgotten.
     * If false is returned, the state of this cache (and its backing view) will be undefined.
     */
    bool Flush();

    /**
     * Push the modifications applied to this cache to its base while retaining
     * the contents of this cache (except for spent coins, which we erase).
     * Failure to call this method or Flush() before destruction will cause the changes
     * to be forgotten.
     * If false is returned, the state of this cache (and its backing view) will be undefined.
     */
    bool Sync();

    /**
     * Removes the UTXO with the given outpoint from the cache, if it is
     * not modified.
     */
    void Uncache(const COutPoint &outpoint);

    //! Calculate the size of the cache (in number of transaction outputs)
    unsigned int GetCacheSize() const;

    //! Calculate the size of the cache (in bytes)
    size_t DynamicMemoryUsage() const;

    //! Check whether all prevouts of the transaction are present in the UTXO set represented by this view
    bool HaveInputs(const CTransaction& tx) const;

    //! Force a reallocation of the cache map. This is required when downsizing
    //! the cache because the map's allocator may be hanging onto a lot of
    //! memory despite having called .clear().
    //!
    //! See: https://stackoverflow.com/questions/42114044/how-to-release-unordered-map-memory
    void ReallocateCache();

    //! Run an internal sanity check on the cache data structure. */
    void SanityCheck() const;

private:
    /**
     * @note this is marked const, but may actually append to `cacheCoins`, increasing
     * memory usage.
     */
    CCoinsMap::iterator FetchCoin(const COutPoint &outpoint) const;
};

//! Utility function to add all of a transaction's outputs to a cache.
//! When check is false, this assumes that overwrites are only possible for coinbase transactions.
//! When check is true, the underlying view may be queried to determine whether an addition is
//! an overwrite.
// TODO: pass in a boolean to limit these possible overwrites to known
// (pre-BIP34) cases.
void AddCoins(CCoinsViewCache& cache, const CTransaction& tx, int nHeight, bool check = false);

//! Utility function to find any unspent output with a given txid.
//! This function can be quite expensive because in the event of a transaction
//! which is not found in the cache, it can cause up to MAX_OUTPUTS_PER_BLOCK
//! lookups to database, so it should be used with care.
const Coin& AccessByTxid(const CCoinsViewCache& cache, const Txid& txid);

/**
 * This is a minimally invasive approach to shutdown on LevelDB read errors from the
 * chainstate, while keeping user interface out of the common library, which is shared
 * between bitcoind, and bitcoin-qt and non-server tools.
 *
 * Writes do not need similar protection, as failure to write is handled by the caller.
*/
class CCoinsViewErrorCatcher final : public CCoinsViewBacked
{
public:
    explicit CCoinsViewErrorCatcher(CCoinsView* view) : CCoinsViewBacked(view) {}

    void AddReadErrCallback(std::function<void()> f) {
        m_err_callbacks.emplace_back(std::move(f));
    }

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;

private:
    /** A list of callbacks to execute upon leveldb read error. */
    std::vector<std::function<void()>> m_err_callbacks;

};

#endif // BITCOIN_COINS_H
