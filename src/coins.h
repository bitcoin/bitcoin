// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINS_H
#define BITCOIN_COINS_H

#include <primitives/transaction.h>
#include <compressor.h>
#include <core_memusage.h>
#include <crypto/siphash.h>
#include <memusage.h>
#include <script/standard.h>
#include <serialize.h>
#include <uint256.h>

#include <assert.h>
#include <stdint.h>

#include <functional>
#include <memory>
#include <set>
#include <unordered_map>

/**
 * A UTXO entry.
 *
 * Serialized format:
 * - VARINT((coinbase ? 1 : 0) | (height << 1) | (extraData ? 0x80000000 : 0))
 * - the non-spent CTxOut (via CTxOutCompressor)
 */
class Coin
{
public:
    //! unspent transaction output
    CTxOut out;

    //! whether containing transaction was a coinbase
    unsigned int fCoinBase : 1;

    //! at which height this containing transaction was included in the active block chain
    uint32_t nHeight : 30;

    //! memory only. Ref from out
    CAccountID refOutAccountID;

    //! memory only. Ref from out
    CTxOutPayloadRef payload;

    //! construct a Coin from a CTxOut and height/coinbase information.
    Coin(CTxOut&& outIn, int nHeightIn, bool fCoinBaseIn) : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {
        refOutAccountID = ExtractAccountID(out.scriptPubKey);
        payload = ExtractTxoutPayload(out, nHeight);
    }
    Coin(const CTxOut& outIn, int nHeightIn, bool fCoinBaseIn) : out(outIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {
        refOutAccountID = ExtractAccountID(out.scriptPubKey);
        payload = ExtractTxoutPayload(out, nHeight);
    }
    //! empty constructor
    Coin() : fCoinBase(false), nHeight(0) {}

    void Clear() {
        out.scriptPubKey.clear();
        out.payload.clear();
    }

    bool IsCoinBase() const {
        return fCoinBase;
    }

    template<typename Stream>
    void Serialize(Stream &s) const {
        assert(!IsSpent());
        uint32_t code = nHeight * 2 + fCoinBase;
        ::Serialize(s, VARINT(code));
        ::Serialize(s, CTxOutCompressor(REF(out)));
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        uint32_t code = 0;
        ::Unserialize(s, VARINT(code));
        nHeight = code >> 1;
        fCoinBase = code & 1;
        ::Unserialize(s, CTxOutCompressor(out));

        refOutAccountID = ExtractAccountID(out.scriptPubKey);
        payload = ExtractTxoutPayload(out, nHeight);
    }

    bool IsSpent() const {
        return out.scriptPubKey.empty();
    }

    size_t DynamicMemoryUsage() const {
        return memusage::DynamicUsage(out.scriptPubKey) + memusage::DynamicUsage(out.payload) + CAccountID::WIDTH * 2;
    }

    bool IsBindPlotter() const {
        return payload && payload->type == TXOUT_TYPE_BINDPLOTTER;
    }

    bool IsPoint() const {
        return payload && payload->type == TXOUT_TYPE_POINT;
    }

    bool IsStaking() const {
        return payload && payload->type == TXOUT_TYPE_STAKING;
    }

    TxOutType GetExtraDataType() const {
        return payload ? payload->type : TXOUT_TYPE_UNKNOWN;
    }
};

class SaltedOutpointHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedOutpointHasher();

    /**
     * This *must* return size_t. With Boost 1.46 on 32-bit systems the
     * unordered_map will behave unpredictably if the custom hasher returns a
     * uint64_t, resulting in failures when syncing the chain (#4634).
     *
     * Having the hash noexcept allows libstdc++'s unordered_map to recalculate
     * the hash during rehash, so it does not have to cache the value. This
     * reduces node's memory by sizeof(size_t). The required recalculation has
     * a slight performance penalty (around 1.6%), but this is compensated by
     * memory savings of about 9% which allow for a larger dbcache setting.
     *
     * @see https://gcc.gnu.org/onlinedocs/gcc-9.2.0/libstdc++/manual/manual/unordered_associative.html
     */
    size_t operator()(const COutPoint& id) const noexcept {
        return SipHashUint256Extra(k0, k1, id.hash, id.n);
    }
};

struct CCoinsCacheEntry
{
    Coin coin; // The actual cached data.
    unsigned char flags;

    enum Flags {
        DIRTY = (1 << 0), // This cache entry is potentially different from the version in the parent view.
        FRESH = (1 << 1), // The parent view does not have this entry (or it is pruned).
        /* Note that FRESH is a performance optimization with which we can
         * erase coins that are fully spent if we know we do not need to
         * flush the changes to the parent cache.  It is always safe to
         * not mark FRESH if that condition is not guaranteed.
         */
    };

    CCoinsCacheEntry() : flags(0) {}
    explicit CCoinsCacheEntry(Coin&& coin_) : coin(std::move(coin_)), flags(0) {}
};

typedef std::unordered_map<COutPoint, CCoinsCacheEntry, SaltedOutpointHasher> CCoinsMap;

/** Bind plotter coin information */
struct CBindPlotterCoinInfo
{
    int nHeight;
    CAccountID accountID;
    uint64_t plotterId;

    CBindPlotterCoinInfo() : nHeight(-1), accountID(), plotterId(0) {}
    explicit CBindPlotterCoinInfo(const Coin& coin) : nHeight((int)coin.nHeight),
        accountID(coin.refOutAccountID),
        plotterId(BindPlotterPayload::As(coin.payload)->GetId()) {}
};

typedef std::map<COutPoint, CBindPlotterCoinInfo> CBindPlotterCoinsMap;
typedef std::pair<CBindPlotterCoinsMap::key_type, CBindPlotterCoinsMap::mapped_type> CBindPlotterCoinPair;

/** Bind plotter information */
class CBindPlotterInfo
{
public:
    COutPoint outpoint;
    int nHeight;
    CAccountID accountID;
    uint64_t plotterId;

    CBindPlotterInfo() : outpoint(), nHeight(-1), accountID(), plotterId(0) {}
    explicit CBindPlotterInfo(const CBindPlotterCoinPair &pair) : outpoint(pair.first),
        nHeight(pair.second.nHeight),
        accountID(pair.second.accountID),
        plotterId(pair.second.plotterId) {}
    CBindPlotterInfo(const COutPoint &o, const Coin &coin) : outpoint(o),
        nHeight((int)coin.nHeight),
        accountID(coin.refOutAccountID),
        plotterId(BindPlotterPayload::As(coin.payload)->GetId()) {}
};

/** Cursor template for iterating over CoinsData state */
template <typename K, typename V>
class CCoinsDataCursor
{
public:
    CCoinsDataCursor(const uint256 &hashBlockIn): hashBlock(hashBlockIn) {}
    virtual ~CCoinsDataCursor() {}

    virtual bool GetKey(K &key) const = 0;
    virtual bool GetValue(V &coin) const = 0;
    virtual unsigned int GetValueSize() const = 0;

    virtual bool Valid() const = 0;
    virtual void Next() = 0;

    //! Get best block at the time this cursor was created
    const uint256 &GetBestBlock() const { return hashBlock; }
private:
    uint256 hashBlock;
};

/** Cursor for iterating over CoinsView state */
typedef CCoinsDataCursor<COutPoint,Coin> CCoinsViewCursor;
typedef std::shared_ptr<CCoinsViewCursor> CCoinsViewCursorRef;

/** List of <Account, Amount> */
typedef std::pair<CAccountID, CAmount> CAccountBalance;
typedef std::vector<CAccountBalance> CAccountBalanceList;

/** Abstract view on the open txout dataset. */
class CCoinsView
{
public:
    /** Retrieve the Coin (unspent transaction output) for a given outpoint.
     *  Returns true only when an unspent coin was found, which is returned in coin.
     *  When false is returned, coin's value is unspecified.
     */
    virtual bool GetCoin(const COutPoint &outpoint, Coin &coin) const;

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
    //! The passed mapCoins can be modified.
    virtual bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock);

    //! Get a cursor to iterate over the whole spendable state
    virtual CCoinsViewCursorRef Cursor() const;
    virtual CCoinsViewCursorRef Cursor(const CAccountID &accountID) const;

    //! Get a cursor to iterate over the whole point send and receive state
    virtual CCoinsViewCursorRef PointSendCursor(const CAccountID &accountID) const;
    virtual CCoinsViewCursorRef PointReceiveCursor(const CAccountID &accountID) const;

    //! Get a cursor to iterate over the whole point send and receive state
    virtual CCoinsViewCursorRef StakingSendCursor(const CAccountID &accountID) const;
    virtual CCoinsViewCursorRef StakingReceiveCursor(const CAccountID &accountID) const;

    //! As we use CCoinsViews polymorphically, have a virtual destructor
    virtual ~CCoinsView() {}

    //! Estimate database size (0 if not implemented)
    virtual size_t EstimateSize() const { return 0; }

    //! Get balance. Return amount of account. [0] is sent, -1 disabled; [1] is received, -1 disabled;
    virtual CAmount GetAccountBalance(const CAccountID &accountID, CAmount *balanceBindPlotter, CAmount balancePoint[2], CAmount balanceStaking[2], const CCoinsMap &mapModifiedCoins = {}) const;

    //! Get account bind plotter all coin entries. if plotterId is 0 then return all coin entries for account.
    virtual CBindPlotterCoinsMap GetAccountBindPlotterEntries(const CAccountID &accountID, const uint64_t &plotterId = 0) const;

    //! Get plotter bind all coin entries.
    virtual CBindPlotterCoinsMap GetBindPlotterEntries(const uint64_t &plotterId) const;

    //! Get top staking accounts
    virtual CAccountBalanceList GetTopStakingAccounts(int n, const CCoinsMap &mapModifiedCoins = {}) const;
};


/** CCoinsView backed by another CCoinsView */
class CCoinsViewBacked : public CCoinsView
{
protected:
    CCoinsView *base;

public:
    CCoinsViewBacked(CCoinsView *viewIn);
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void SetBackend(CCoinsView &viewIn);
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    CCoinsViewCursorRef Cursor() const override;
    CCoinsViewCursorRef Cursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef PointSendCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef PointReceiveCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef StakingSendCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef StakingReceiveCursor(const CAccountID &accountID) const override;
    size_t EstimateSize() const override;
    CAmount GetAccountBalance(const CAccountID &accountID, CAmount *balanceBindPlotter, CAmount balancePoint[2], CAmount balanceStaking[2], const CCoinsMap &mapModifiedCoins = {}) const override;
    CBindPlotterCoinsMap GetAccountBindPlotterEntries(const CAccountID &accountID, const uint64_t &plotterId = 0) const override;
    CBindPlotterCoinsMap GetBindPlotterEntries(const uint64_t &plotterId) const override;
    CAccountBalanceList GetTopStakingAccounts(int n, const CCoinsMap &mapModifiedCoins = {}) const override;
};


/** CCoinsView that adds a memory cache for transactions to another CCoinsView */
class CCoinsViewCache : public CCoinsViewBacked
{
protected:
    /**
     * Make mutable so that we can "fill the cache" even from Get-methods
     * declared as "const".
     */
    mutable uint256 hashBlock;
    mutable CCoinsMap cacheCoins;

    /* Cached dynamic memory usage for the inner Coin objects. */
    mutable size_t cachedCoinsUsage;

public:
    CCoinsViewCache(CCoinsView *baseIn);

    /**
     * By deleting the copy constructor, we prevent accidentally using it when one intends to create a cache on top of a base cache.
     */
    CCoinsViewCache(const CCoinsViewCache &) = delete;

    // Standard CCoinsView methods
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    void SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    CCoinsViewCursorRef Cursor() const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CCoinsViewCursorRef Cursor(const CAccountID &accountID) const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CCoinsViewCursorRef PointSendCursor(const CAccountID &accountID) const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CCoinsViewCursorRef PointReceiveCursor(const CAccountID &accountID) const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CCoinsViewCursorRef StakingSendCursor(const CAccountID &accountID) const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CCoinsViewCursorRef StakingReceiveCursor(const CAccountID &accountID) const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    CAmount GetAccountBalance(const CAccountID &accountID, CAmount *balanceBindPlotter, CAmount balancePoint[2], CAmount balanceStaking[2], const CCoinsMap &mapModifiedCoins = {}) const override;
    CBindPlotterCoinsMap GetAccountBindPlotterEntries(const CAccountID &accountID, const uint64_t &plotterId = 0) const override;
    CBindPlotterCoinsMap GetBindPlotterEntries(const uint64_t &plotterId) const override;
    CAccountBalanceList GetTopStakingAccounts(int n, const CCoinsMap &mapModifiedCoins = {}) const override;

    /**
     * Check if we have the given utxo already loaded in this cache.
     * The semantics are the same as HaveCoin(), but no calls to
     * the backing CCoinsView are made.
     */
    bool HaveCoinInCache(const COutPoint &outpoint) const;

    /**
     * Return a reference to Coin in the cache, or a pruned one if not found. This is
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
     * Add a coin. Set potential_overwrite to true if a non-pruned version may
     * already exist.
     */
    void AddCoin(const COutPoint& outpoint, Coin&& coin, bool potential_overwrite);

    /**
     * Spend a coin. Pass moveto in order to get the deleted data.
     * If no unspent output exists for the passed outpoint, this call
     * has no effect.
     */
    bool SpendCoin(const COutPoint &outpoint, Coin* moveto = nullptr);

    /**
     * Push the modifications applied to this cache to its base.
     * Failure to call this method before destruction will cause the changes to be forgotten.
     * If false is returned, the state of this cache (and its backing view) will be undefined.
     */
    bool Flush();

    /**
     * Removes the UTXO with the given outpoint from the cache, if it is
     * not modified.
     */
    void Uncache(const COutPoint &outpoint);

    //! Calculate the size of the cache (in number of transaction outputs)
    unsigned int GetCacheSize() const;

    //! Calculate the size of the cache (in bytes)
    size_t DynamicMemoryUsage() const;

    /**
     * Amount of bitcoins coming in to a transaction
     * Note that lightweight clients may not know anything besides the hash of previous transactions,
     * so may not be able to calculate this.
     *
     * @param[in] tx    transaction for which we are checking input total
     * @return  Sum of value of all inputs (scriptSigs)
     */
    CAmount GetValueIn(const CTransaction& tx) const;

    //! Check whether all prevouts of the transaction are present in the UTXO set represented by this view
    bool HaveInputs(const CTransaction& tx) const;

    /** Return a reference to lastest bind plotter information, or a pruned one if not found. */
    CBindPlotterInfo GetChangeBindPlotterInfo(const CBindPlotterInfo &sourceBindInfo) const;
    CBindPlotterInfo GetLastBindPlotterInfo(const uint64_t &plotterId) const;
    const Coin& GetLastBindPlotterCoin(const uint64_t &plotterId, COutPoint *outpoint = nullptr) const;

    /** Just check whether a given <accountID,plotterId> exist and lastest binded of plotterId. */
    bool AccountHaveActiveBindPlotter(const CAccountID &accountID, const uint64_t &plotterId) const;

    /** Find accont revelate plotters */
    std::set<uint64_t> GetAccountBindPlotters(const CAccountID &accountID) const;

    /** Find accont received balance */
    CAmount GetAccountPointReceivedBalance(const CAccountID &accountID) const;

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
const Coin& AccessByTxid(const CCoinsViewCache& cache, const uint256& txid);

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

    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;

private:
    /** A list of callbacks to execute upon leveldb read error. */
    std::vector<std::function<void()>> m_err_callbacks;

};

#endif // BITCOIN_COINS_H
