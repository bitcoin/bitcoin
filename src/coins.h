// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINS_H
#define BITCOIN_COINS_H

#include <attributes.h>
#include <compressor.h>
#include <core_memusage.h>
#include <memusage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <random.h>
#include <serialize.h>
#include <support/allocators/pool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/hasher.h>

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <atomic>
#include <functional>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

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
 * - spent, not FRESH, DIRTY (e.g. a coin is spent and spentness needs to be flushed to the parent)
 */
struct CCoinsCacheEntry
{
private:
    /**
     * These are used to create a doubly linked list of flagged entries.
     * They are set in SetDirty, SetFresh, and unset in SetClean.
     * A flagged entry is any entry that is either DIRTY, FRESH, or both.
     *
     * DIRTY entries are tracked so that only modified entries can be passed to
     * the parent cache for batch writing. This is a performance optimization
     * compared to giving all entries in the cache to the parent and having the
     * parent scan for only modified entries.
     */
    CoinsCachePair* m_prev{nullptr};
    CoinsCachePair* m_next{nullptr};
    uint8_t m_flags{0};

    //! Adding a flag requires a reference to the sentinel of the flagged pair linked list.
    static void AddFlags(uint8_t flags, CoinsCachePair& pair, CoinsCachePair& sentinel) noexcept
    {
        Assume(flags & (DIRTY | FRESH));
        if (!pair.second.m_flags) {
            Assume(!pair.second.m_prev && !pair.second.m_next);
            pair.second.m_prev = sentinel.second.m_prev;
            pair.second.m_next = &sentinel;
            sentinel.second.m_prev = &pair;
            pair.second.m_prev->second.m_next = &pair;
        }
        Assume(pair.second.m_prev && pair.second.m_next);
        pair.second.m_flags |= flags;
    }

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
        SetClean();
    }

    static void SetDirty(CoinsCachePair& pair, CoinsCachePair& sentinel) noexcept { AddFlags(DIRTY, pair, sentinel); }
    static void SetFresh(CoinsCachePair& pair, CoinsCachePair& sentinel) noexcept { AddFlags(FRESH, pair, sentinel); }

    void SetClean() noexcept
    {
        if (!m_flags) return;
        m_next->second.m_prev = m_prev;
        m_prev->second.m_next = m_next;
        m_flags = 0;
        m_prev = m_next = nullptr;
    }
    bool IsDirty() const noexcept { return m_flags & DIRTY; }
    bool IsFresh() const noexcept { return m_flags & FRESH; }

    //! Only call Next when this entry is DIRTY, FRESH, or both
    CoinsCachePair* Next() const noexcept
    {
        Assume(m_flags);
        return m_next;
    }

    //! Only call Prev when this entry is DIRTY, FRESH, or both
    CoinsCachePair* Prev() const noexcept
    {
        Assume(m_flags);
        return m_prev;
    }

    //! Only use this for initializing the linked list sentinel
    void SelfRef(CoinsCachePair& pair) noexcept
    {
        Assume(&pair.second == this);
        m_prev = &pair;
        m_next = &pair;
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
    CoinsViewCacheCursor(size_t& dirty_count LIFETIMEBOUND,
                         CoinsCachePair& sentinel LIFETIMEBOUND,
                         CCoinsMap& map LIFETIMEBOUND,
                         bool will_erase) noexcept
        : m_dirty_count(dirty_count), m_sentinel(sentinel), m_map(map), m_will_erase(will_erase) {}

    inline CoinsCachePair* Begin() const noexcept { return m_sentinel.second.Next(); }
    inline CoinsCachePair* End() const noexcept { return &m_sentinel; }

    //! Return the next entry after current, possibly erasing current
    inline CoinsCachePair* NextAndMaybeErase(CoinsCachePair& current) noexcept
    {
        const auto next_entry{current.second.Next()};
        Assume(TrySub(m_dirty_count, current.second.IsDirty()));
        // If we are not going to erase the cache, we must still erase spent entries.
        // Otherwise, clear the state of the entry.
        if (!m_will_erase) {
            if (current.second.coin.IsSpent()) {
                assert(current.second.coin.DynamicMemoryUsage() == 0); // scriptPubKey was already cleared in SpendCoin
                m_map.erase(current.first);
            } else {
                current.second.SetClean();
            }
        }
        return next_entry;
    }

    inline bool WillErase(CoinsCachePair& current) const noexcept { return m_will_erase || current.second.coin.IsSpent(); }
    size_t GetDirtyCount() const noexcept { return m_dirty_count; }
    size_t GetTotalCount() const noexcept { return m_map.size(); }
private:
    size_t& m_dirty_count;
    CoinsCachePair& m_sentinel;
    CCoinsMap& m_map;
    bool m_will_erase;
};

/** Abstract view on the open txout dataset. */
class CCoinsView
{
public:
    //! Retrieve the Coin (unspent transaction output) for a given outpoint.
    //! May populate the cache. Use PeekCoin() to perform a non-caching lookup.
    virtual std::optional<Coin> GetCoin(const COutPoint& outpoint) const;

    //! Retrieve the Coin (unspent transaction output) for a given outpoint, without caching results.
    //! Does not populate the cache. Use GetCoin() to cache the result.
    virtual std::optional<Coin> PeekCoin(const COutPoint& outpoint) const;

    //! Just check whether a given outpoint is unspent.
    //! May populate the cache. Use PeekCoin() to perform a non-caching lookup.
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
    virtual void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& hashBlock);

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
    std::optional<Coin> PeekCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void SetBackend(CCoinsView &viewIn);
    void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& hashBlock) override;
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
    /* Running count of dirty Coin cache entries. */
    mutable size_t m_dirty_count{0};

    /**
     * Discard all modifications made to this cache without flushing to the base view.
     * This can be used to efficiently reuse a cache instance across multiple operations.
     */
    virtual void Reset() noexcept;

    /* Fetch the coin from base. Used for cache misses in FetchCoin. */
    virtual std::optional<Coin> FetchCoinFromBase(const COutPoint& outpoint) const;

public:
    CCoinsViewCache(CCoinsView *baseIn, bool deterministic = false);

    /**
     * By deleting the copy constructor, we prevent accidentally using it when one intends to create a cache on top of a base cache.
     */
    CCoinsViewCache(const CCoinsViewCache &) = delete;

    // Standard CCoinsView methods
    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override;
    std::optional<Coin> PeekCoin(const COutPoint& outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    void SetBestBlock(const uint256 &hashBlock);
    void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& hashBlock) override;
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
     * If reallocate_cache is false, the cache will retain the same memory footprint
     * after flushing and should be destroyed to deallocate.
     */
    void Flush(bool reallocate_cache = true);

    /**
     * Push the modifications applied to this cache to its base while retaining
     * the contents of this cache (except for spent coins, which we erase).
     * Failure to call this method or Flush() before destruction will cause the changes
     * to be forgotten.
     */
    void Sync();

    /**
     * Removes the UTXO with the given outpoint from the cache, if it is
     * not modified.
     */
    void Uncache(const COutPoint &outpoint);

    //! Size of the cache (in number of transaction outputs)
    unsigned int GetCacheSize() const;

    //! Number of dirty cache entries (transaction outputs)
    size_t GetDirtyCount() const noexcept { return m_dirty_count; }

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

    class ResetGuard
    {
    private:
        friend CCoinsViewCache;
        CCoinsViewCache& m_cache;
        explicit ResetGuard(CCoinsViewCache& cache LIFETIMEBOUND) noexcept : m_cache{cache} {}

    public:
        ResetGuard(const ResetGuard&) = delete;
        ResetGuard& operator=(const ResetGuard&) = delete;
        ResetGuard(ResetGuard&&) = delete;
        ResetGuard& operator=(ResetGuard&&) = delete;

        ~ResetGuard() { m_cache.Reset(); }
    };

    //! Create a scoped guard that will call `Reset()` on this cache when it goes out of scope.
    [[nodiscard]] ResetGuard CreateResetGuard() noexcept { return ResetGuard{*this}; }

private:
    /**
     * @note this is marked const, but may actually append to `cacheCoins`, increasing
     * memory usage.
     */
    CCoinsMap::iterator FetchCoin(const COutPoint &outpoint) const;
};

/**
 * CCoinsViewCache overlay that avoids populating/mutating parent cache layers on cache misses.
 *
 * This is achieved by fetching coins from the base view using PeekCoin() instead of GetCoin(),
 * so intermediate CCoinsViewCache layers are not filled.
 *
 * Used during ConnectBlock() as an ephemeral, resettable top-level view that is flushed only
 * on success, so invalid blocks don't pollute the underlying cache.
 */
class CoinsViewOverlay : public CCoinsViewCache
{
private:
    //! The latest input not yet being fetched. Workers atomically increment this when fetching.
    mutable std::atomic_uint32_t m_input_head{0};
    //! The latest input not yet accessed by a consumer. Only the main thread increments this.
    mutable uint32_t m_input_tail{0};

    //! The inputs of the block which is being fetched.
    struct InputToFetch {
        //! The outpoint of the input to fetch.
        const COutPoint& outpoint;
        //! The coin that workers will fetch and main thread will insert into cache.
        std::optional<Coin> coin{std::nullopt};

        explicit InputToFetch(const COutPoint& o LIFETIMEBOUND) noexcept : outpoint{o} {}
    };
    mutable std::vector<InputToFetch> m_inputs{};

    class QuickHashHasher
    {
        uint64_t m_key[4];

    public:
        explicit QuickHashHasher(bool deterministic) noexcept
        {
            FastRandomContext rng;
            for (auto& k : m_key) k = deterministic ? 0 : rng.rand64();
        }

#if defined(__clang__)
        __attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
        uint64_t operator()(const Txid& txid) const noexcept
        {
            const auto& hash_input{txid.ToUint256()};
            uint64_t out{0};
            for (const auto i : std::views::iota(0, 4)) out += hash_input.GetUint64(i) ^ m_key[i];
            return out;
        }
    };
    QuickHashHasher m_hasher;

    /**
     * The sorted quick hash of txids of all txs in the block being fetched. This is used to filter out inputs that
     * are created earlier in the same block, since they will not be in the db or the cache.
     * Using an 8 byte quick hash is a performance improvement, versus storing the entire 32 bytes. In case of a
     * collision of an input being spent having the same quick hash as a txid of a tx elsewhere in the block,
     * the input will not be fetched in the background. The input will still be fetched later on the main thread.
     * Using a sorted vector and binary search lookups is a performance improvement. It is faster than
     * using std::unordered_set with salted hash or std::set.
     */
    std::vector<uint64_t> m_txids{};

    /**
     * Claim and fetch the next input in the queue.
     *
     * @return true if there are more inputs in the queue to fetch
     * @return false if there are no more inputs in the queue to fetch
     */
    bool ProcessInput() const noexcept
    {
        const auto i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
        if (i >= m_inputs.size()) [[unlikely]] return false;

        auto& input{m_inputs[i]};
        // Inputs spending a coin from a tx earlier in the block won't be in the cache or db
        if (std::ranges::binary_search(m_txids, m_hasher(input.outpoint.hash))) {
            return true;
        }

        if (auto coin{base->PeekCoin(input.outpoint)}) [[likely]] input.coin.emplace(std::move(*coin));
        return true;
    }

    //! Clear fetching data.
    void StopFetching() noexcept
    {
        m_inputs.clear();
        m_input_head.store(0, std::memory_order_relaxed);
        m_input_tail = 0;
        m_txids.clear();
    }

    std::optional<Coin> FetchCoinFromBase(const COutPoint& outpoint) const override
    {
        // This assumes ConnectBlock accesses all inputs in the same order as they are added to m_inputs
        // in StartFetching. Some outpoints are not accessed because they are created by the block, so we scan until we
        // come across the requested input.
        for (const auto i : std::views::iota(m_input_tail, m_inputs.size())) [[likely]] {
            auto& input{m_inputs[i]};
            if (input.outpoint != outpoint) continue;
            // We advance the tail since the input is cached and not accessed through this method again.
            m_input_tail = i + 1;
            // We can move the coin since we won't access this input again.
            if (input.coin) [[likely]] return std::move(*input.coin);
            // If we get here, then this block has missing or spent inputs or there is a txid quick hash collision.
            break;
        }

        // We will only get here for BIP30 checks, txid quick hash collisions or a block with missing or spent inputs.
        return base->PeekCoin(outpoint);
    }

protected:
    void Reset() noexcept override
    {
        StopFetching();
        CCoinsViewCache::Reset();
    }

public:
    explicit CoinsViewOverlay(CCoinsView* in_base, bool deterministic = false) noexcept
        : CCoinsViewCache{in_base, deterministic}, m_hasher{deterministic} {}

    //! Start fetching inputs from block.
    [[nodiscard]] ResetGuard StartFetching(const CBlock& block LIFETIMEBOUND) noexcept
    {
        // Loop through the inputs of the block and set them in the queue. Also construct the set of txids to filter.
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] m_inputs.emplace_back(input.prevout);
            m_txids.emplace_back(m_hasher(tx->GetHash()));
        }
        // Sort txids so we can do binary search lookups.
        std::ranges::sort(m_txids);
        while (ProcessInput()) [[likely]] {}
        return CreateResetGuard();
    }
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
    std::optional<Coin> PeekCoin(const COutPoint& outpoint) const override;

private:
    /** A list of callbacks to execute upon leveldb read error. */
    std::vector<std::function<void()>> m_err_callbacks;

};

#endif // BITCOIN_COINS_H
