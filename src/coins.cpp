// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <consensus/consensus.h>
#include <logging.h>
#include <random.h>
#include <util/trace.h>

bool CCoinsView::GetCoin(const COutPoint &outpoint, Coin &coin) const { return false; }
uint256 CCoinsView::GetBestBlock() const { return uint256(); }
std::vector<uint256> CCoinsView::GetHeadBlocks() const { return std::vector<uint256>(); }
bool CCoinsView::BatchWrite(CoinsCachePair *pairs, const uint256 &hashBlock, bool will_erase) { return false; }
std::unique_ptr<CCoinsViewCursor> CCoinsView::Cursor() const { return nullptr; }

bool CCoinsView::HaveCoin(const COutPoint &outpoint) const
{
    Coin coin;
    return GetCoin(outpoint, coin);
}

CCoinsViewBacked::CCoinsViewBacked(CCoinsView *viewIn) : base(viewIn) { }
bool CCoinsViewBacked::GetCoin(const COutPoint &outpoint, Coin &coin) const { return base->GetCoin(outpoint, coin); }
bool CCoinsViewBacked::HaveCoin(const COutPoint &outpoint) const { return base->HaveCoin(outpoint); }
uint256 CCoinsViewBacked::GetBestBlock() const { return base->GetBestBlock(); }
std::vector<uint256> CCoinsViewBacked::GetHeadBlocks() const { return base->GetHeadBlocks(); }
void CCoinsViewBacked::SetBackend(CCoinsView &viewIn) { base = &viewIn; }
bool CCoinsViewBacked::BatchWrite(CoinsCachePair *pairs, const uint256 &hashBlock, bool will_erase) { return base->BatchWrite(pairs, hashBlock, will_erase); }
std::unique_ptr<CCoinsViewCursor> CCoinsViewBacked::Cursor() const { return base->Cursor(); }
size_t CCoinsViewBacked::EstimateSize() const { return base->EstimateSize(); }

CCoinsViewCache::CCoinsViewCache(CCoinsView* baseIn, bool deterministic) :
    CCoinsViewBacked(baseIn), m_deterministic(deterministic),
    cacheCoins(0, SaltedOutpointHasher(/*deterministic=*/deterministic), CCoinsMap::key_equal{}, &m_cache_coins_memory_resource)
{}

size_t CCoinsViewCache::DynamicMemoryUsage() const {
    return memusage::DynamicUsage(cacheCoins) + cachedCoinsUsage;
}

CCoinsMap::iterator CCoinsViewCache::FetchCoin(const COutPoint &outpoint) const {
    CCoinsMap::iterator it = cacheCoins.find(outpoint);
    if (it != cacheCoins.end())
        return it;
    Coin tmp;
    if (!base->GetCoin(outpoint, tmp))
        return cacheCoins.end();
    CCoinsMap::iterator ret = cacheCoins.emplace(std::piecewise_construct, std::forward_as_tuple(outpoint), std::forward_as_tuple(std::move(tmp))).first;
    if (ret->second.coin.IsSpent()) {
        // The parent only has an empty entry for this outpoint; we can consider our
        // version as fresh.
        ret->second.AddFlags(CCoinsCacheEntry::FRESH, *ret, m_flagged_head);
    }
    cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
    return ret;
}

bool CCoinsViewCache::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    CCoinsMap::const_iterator it = FetchCoin(outpoint);
    if (it != cacheCoins.end()) {
        coin = it->second.coin;
        return !coin.IsSpent();
    }
    return false;
}

void CCoinsViewCache::AddCoin(const COutPoint &outpoint, Coin&& coin, bool possible_overwrite) {
    assert(!coin.IsSpent());
    if (coin.out.scriptPubKey.IsUnspendable()) return;
    CCoinsMap::iterator it;
    bool inserted;
    std::tie(it, inserted) = cacheCoins.emplace(std::piecewise_construct, std::forward_as_tuple(outpoint), std::tuple<>());
    bool fresh = false;
    if (!inserted) {
        cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();
    }
    if (!possible_overwrite) {
        if (!it->second.coin.IsSpent()) {
            throw std::logic_error("Attempted to overwrite an unspent coin (when possible_overwrite is false)");
        }
        // If the coin exists in this cache as a spent coin and is DIRTY, then
        // its spentness hasn't been flushed to the parent cache. We're
        // re-adding the coin to this cache now but we can't mark it as FRESH.
        // If we mark it FRESH and then spend it before the cache is flushed
        // we would remove it from this cache and would never flush spentness
        // to the parent cache.
        //
        // Re-adding a spent coin can happen in the case of a re-org (the coin
        // is 'spent' when the block adding it is disconnected and then
        // re-added when it is also added in a newly connected block).
        //
        // If the coin doesn't exist in the current cache, or is spent but not
        // DIRTY, then it can be marked FRESH.
        fresh = !(it->second.GetFlags() & CCoinsCacheEntry::DIRTY);
    }
    it->second.coin = std::move(coin);
    it->second.AddFlags(CCoinsCacheEntry::DIRTY | (fresh ? CCoinsCacheEntry::FRESH : 0), *it, m_flagged_head);
    cachedCoinsUsage += it->second.coin.DynamicMemoryUsage();
    TRACE5(utxocache, add,
           outpoint.hash.data(),
           (uint32_t)outpoint.n,
           (uint32_t)it->second.coin.nHeight,
           (int64_t)it->second.coin.out.nValue,
           (bool)it->second.coin.IsCoinBase());
}

void CCoinsViewCache::EmplaceCoinInternalDANGER(COutPoint&& outpoint, Coin&& coin) {
    cachedCoinsUsage += coin.DynamicMemoryUsage();
    auto [it, inserted] = cacheCoins.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(outpoint)),
        std::forward_as_tuple(std::move(coin)));
    if (inserted) {
        it->second.AddFlags(CCoinsCacheEntry::DIRTY, *it, m_flagged_head);
    }
}

void AddCoins(CCoinsViewCache& cache, const CTransaction &tx, int nHeight, bool check_for_overwrite) {
    bool fCoinbase = tx.IsCoinBase();
    const Txid& txid = tx.GetHash();
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        bool overwrite = check_for_overwrite ? cache.HaveCoin(COutPoint(txid, i)) : fCoinbase;
        // Coinbase transactions can always be overwritten, in order to correctly
        // deal with the pre-BIP30 occurrences of duplicate coinbase transactions.
        cache.AddCoin(COutPoint(txid, i), Coin(tx.vout[i], nHeight, fCoinbase), overwrite);
    }
}

bool CCoinsViewCache::SpendCoin(const COutPoint &outpoint, Coin* moveout) {
    CCoinsMap::iterator it = FetchCoin(outpoint);
    if (it == cacheCoins.end()) return false;
    cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();
    TRACE5(utxocache, spent,
           outpoint.hash.data(),
           (uint32_t)outpoint.n,
           (uint32_t)it->second.coin.nHeight,
           (int64_t)it->second.coin.out.nValue,
           (bool)it->second.coin.IsCoinBase());
    if (moveout) {
        *moveout = std::move(it->second.coin);
    }
    if (it->second.GetFlags() & CCoinsCacheEntry::FRESH) {
        cacheCoins.erase(it);
    } else {
        it->second.AddFlags(CCoinsCacheEntry::DIRTY, *it, m_flagged_head);
        it->second.coin.Clear();
    }
    return true;
}

static const Coin coinEmpty;

const Coin& CCoinsViewCache::AccessCoin(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = FetchCoin(outpoint);
    if (it == cacheCoins.end()) {
        return coinEmpty;
    } else {
        return it->second.coin;
    }
}

bool CCoinsViewCache::HaveCoin(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = FetchCoin(outpoint);
    return (it != cacheCoins.end() && !it->second.coin.IsSpent());
}

bool CCoinsViewCache::HaveCoinInCache(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = cacheCoins.find(outpoint);
    return (it != cacheCoins.end() && !it->second.coin.IsSpent());
}

uint256 CCoinsViewCache::GetBestBlock() const {
    if (hashBlock.IsNull())
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

void CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
}

bool CCoinsViewCache::BatchWrite(CoinsCachePair *pairs, const uint256 &hashBlockIn, bool will_erase) {
    for (auto it{pairs};
            it != nullptr;
            it = it->second.Next(/*clear_flags=*/will_erase)) {
        // Ignore non-dirty entries (optimization).
        if (!(it->second.GetFlags() & CCoinsCacheEntry::DIRTY)) {
            continue;
        }
        CCoinsMap::iterator itUs = cacheCoins.find(it->first);
        if (itUs == cacheCoins.end()) {
            // The parent cache does not have an entry, while the child cache does.
            // We can ignore it if it's both spent and FRESH in the child
            if (!(it->second.GetFlags() & CCoinsCacheEntry::FRESH && it->second.coin.IsSpent())) {
                // Create the coin in the parent cache, move the data up
                // and mark it as dirty.
                itUs = cacheCoins.try_emplace(it->first).first;
                CCoinsCacheEntry& entry{itUs->second};
                if (will_erase) {
                    // Since the child will erase the coins after this batch write,
                    // we can move the coin into the parent instead of copying it
                    entry.coin = std::move(it->second.coin);
                } else {
                    entry.coin = it->second.coin;
                }
                cachedCoinsUsage += entry.coin.DynamicMemoryUsage();
                entry.AddFlags(CCoinsCacheEntry::DIRTY, *itUs, m_flagged_head);
                // We can mark it FRESH in the parent if it was FRESH in the child
                // Otherwise it might have just been flushed from the parent's cache
                // and already exist in the grandparent
                if (it->second.GetFlags() & CCoinsCacheEntry::FRESH) {
                    entry.AddFlags(CCoinsCacheEntry::FRESH, *itUs, m_flagged_head);
                }
            }
        } else {
            // Found the entry in the parent cache
            if ((it->second.GetFlags() & CCoinsCacheEntry::FRESH) && !itUs->second.coin.IsSpent()) {
                // The coin was marked FRESH in the child cache, but the coin
                // exists in the parent cache. If this ever happens, it means
                // the FRESH flag was misapplied and there is a logic error in
                // the calling code.
                throw std::logic_error("FRESH flag misapplied to coin that exists in parent cache");
            }

            if ((itUs->second.GetFlags() & CCoinsCacheEntry::FRESH) && it->second.coin.IsSpent()) {
                // The grandparent cache does not have an entry, and the coin
                // has been spent. We can just delete it from the parent cache.
                cachedCoinsUsage -= itUs->second.coin.DynamicMemoryUsage();
                cacheCoins.erase(itUs);
            } else {
                // A normal modification.
                cachedCoinsUsage -= itUs->second.coin.DynamicMemoryUsage();
                if (will_erase) {
                    // Since the child will erase the coins after this batch write,
                    // we can move the coin into the parent instead of copying it
                    itUs->second.coin = std::move(it->second.coin);
                } else {
                    itUs->second.coin = it->second.coin;
                }
                cachedCoinsUsage += itUs->second.coin.DynamicMemoryUsage();
                itUs->second.AddFlags(CCoinsCacheEntry::DIRTY, *itUs, m_flagged_head);
                // NOTE: It isn't safe to mark the coin as FRESH in the parent
                // cache. If it already existed and was spent in the parent
                // cache then marking it FRESH would prevent that spentness
                // from being flushed to the grandparent.
            }
        }
    }
    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::Flush() {
    const auto fOk{base->BatchWrite(m_flagged_head.second.Next(), hashBlock, /*will_erase=*/true)};
    if (fOk) {
        if (m_flagged_head.second.Next() != nullptr) {
            /* BatchWrite must unset all flagged entries when will_erase=true. */
            throw std::logic_error("Not all flagged cached coins were unset");
        }
        ReallocateCache();
    }
    cachedCoinsUsage = 0;
    return fOk;
}

bool CCoinsViewCache::Sync()
{
    const auto fOk{base->BatchWrite(m_flagged_head.second.Next(), hashBlock, /*will_erase=*/false)};
    // Instead of clearing `cacheCoins` as we would in Flush(), just erase the
    // spent coins and clear the FRESH/DIRTY flags of any coin that isn't spent.
    for (auto it{m_flagged_head.second.Next()}; it != nullptr; ) {
        const auto next_entry{it->second.Next(/*clear_flags=*/true)};
        if (it->second.coin.IsSpent()) {
            cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();
            cacheCoins.erase(it->first);
        }
        it = next_entry;
    }
    return fOk;
}

void CCoinsViewCache::Uncache(const COutPoint& hash)
{
    CCoinsMap::iterator it = cacheCoins.find(hash);
    if (it != cacheCoins.end() && it->second.GetFlags() == 0) {
        cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();
        TRACE5(utxocache, uncache,
               hash.hash.data(),
               (uint32_t)hash.n,
               (uint32_t)it->second.coin.nHeight,
               (int64_t)it->second.coin.out.nValue,
               (bool)it->second.coin.IsCoinBase());
        cacheCoins.erase(it);
    }
}

unsigned int CCoinsViewCache::GetCacheSize() const {
    return cacheCoins.size();
}

bool CCoinsViewCache::HaveInputs(const CTransaction& tx) const
{
    if (!tx.IsCoinBase()) {
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            if (!HaveCoin(tx.vin[i].prevout)) {
                return false;
            }
        }
    }
    return true;
}

void CCoinsViewCache::ReallocateCache()
{
    cacheCoins.~CCoinsMap();
    m_cache_coins_memory_resource.~CCoinsMapMemoryResource();
    ::new (&m_cache_coins_memory_resource) CCoinsMapMemoryResource{};
    ::new (&cacheCoins) CCoinsMap{0, SaltedOutpointHasher{/*deterministic=*/m_deterministic}, CCoinsMap::key_equal{}, &m_cache_coins_memory_resource};
}

void CCoinsViewCache::SanityCheck() const
{
    size_t recomputed_usage = 0;
    for (const auto& [_, entry] : cacheCoins) {
        unsigned attr = 0;
        if (entry.GetFlags() & CCoinsCacheEntry::DIRTY) attr |= 1;
        if (entry.GetFlags() & CCoinsCacheEntry::FRESH) attr |= 2;
        if (entry.coin.IsSpent()) attr |= 4;
        // Only 5 combinations are possible.
        assert(attr != 2 && attr != 4 && attr != 7);

        // Recompute cachedCoinsUsage.
        recomputed_usage += entry.coin.DynamicMemoryUsage();
    }
    assert(recomputed_usage == cachedCoinsUsage);
}

static const size_t MIN_TRANSACTION_OUTPUT_WEIGHT = WITNESS_SCALE_FACTOR * ::GetSerializeSize(CTxOut());
static const size_t MAX_OUTPUTS_PER_BLOCK = MAX_BLOCK_WEIGHT / MIN_TRANSACTION_OUTPUT_WEIGHT;

const Coin& AccessByTxid(const CCoinsViewCache& view, const Txid& txid)
{
    COutPoint iter(txid, 0);
    while (iter.n < MAX_OUTPUTS_PER_BLOCK) {
        const Coin& alternate = view.AccessCoin(iter);
        if (!alternate.IsSpent()) return alternate;
        ++iter.n;
    }
    return coinEmpty;
}

template <typename Func>
static bool ExecuteBackedWrapper(Func func, const std::vector<std::function<void()>>& err_callbacks)
{
    try {
        return func();
    } catch(const std::runtime_error& e) {
        for (const auto& f : err_callbacks) {
            f();
        }
        LogPrintf("Error reading from database: %s\n", e.what());
        // Starting the shutdown sequence and returning false to the caller would be
        // interpreted as 'entry not found' (as opposed to unable to read data), and
        // could lead to invalid interpretation. Just exit immediately, as we can't
        // continue anyway, and all writes should be atomic.
        std::abort();
    }
}

bool CCoinsViewErrorCatcher::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    return ExecuteBackedWrapper([&]() { return CCoinsViewBacked::GetCoin(outpoint, coin); }, m_err_callbacks);
}

bool CCoinsViewErrorCatcher::HaveCoin(const COutPoint &outpoint) const {
    return ExecuteBackedWrapper([&]() { return CCoinsViewBacked::HaveCoin(outpoint); }, m_err_callbacks);
}
