// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <consensus/consensus.h>
#include <logging.h>
#include <random.h>
#include <util/trace.h>
#include <version.h>

#include <optional>

bool CCoinsView::GetCoin(const COutPoint &outpoint, Coin &coin) const { return false; }
uint256 CCoinsView::GetBestBlock() const { return uint256(); }
std::vector<uint256> CCoinsView::GetHeadBlocks() const { return std::vector<uint256>(); }
bool CCoinsView::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) { return false; }
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
bool CCoinsViewBacked::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) { return base->BatchWrite(mapCoins, hashBlock); }
std::unique_ptr<CCoinsViewCursor> CCoinsViewBacked::Cursor() const { return base->Cursor(); }
size_t CCoinsViewBacked::EstimateSize() const { return base->EstimateSize(); }

CCoinsViewCache::CCoinsViewCache(CCoinsView *baseIn) : CCoinsViewBacked(baseIn), cachedCoinsUsage(0) {}

size_t CCoinsViewCache::DynamicMemoryUsage() const {
    return memusage::DynamicUsage(cacheCoins) + cachedCoinsUsage;
}

class CCoinsViewCache::Modifier
{
public:
    Modifier(const CCoinsViewCache& cache, const COutPoint& outpoint);
    Modifier(const CCoinsViewCache& cache, const COutPoint& outpoint, CCoinsCacheEntry&& new_entry);
    ~Modifier() { Flush(); }
    Coin& Modify();
    CCoinsMap::iterator Flush();

private:
    const CCoinsViewCache& m_cache;
    const COutPoint& m_outpoint;
    CCoinsMap::iterator m_cur_entry = m_cache.cacheCoins.find(m_outpoint);
    std::optional<CCoinsCacheEntry> m_new_entry;
};

CCoinsMap::iterator CCoinsViewCache::FetchCoin(const COutPoint &outpoint) const {
    return Modifier(*this, outpoint).Flush();
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
    CCoinsCacheEntry entry;
    // If we are not possibly overwriting, any coin in the base view below the
    // cache will be spent, so the cache entry can be marked fresh.
    // If we are possibly overwriting, we can't make any assumption about the
    // coin in the the base view below the cache, so the new cache entry which
    // will replace it must be marked dirty.
    entry.flags |= possible_overwrite ? CCoinsCacheEntry::DIRTY : CCoinsCacheEntry::FRESH;
    Modifier(*this, outpoint, std::move(entry)).Modify() = std::move(coin);
    TRACE5(utxocache, add,
           outpoint.hash.data(),
           (uint32_t)outpoint.n,
           (uint32_t)coin.nHeight,
           (int64_t)coin.out.nValue,
           (bool)coin.IsCoinBase());
}

void CCoinsViewCache::EmplaceCoinInternalDANGER(COutPoint&& outpoint, Coin&& coin) {
    cachedCoinsUsage += coin.DynamicMemoryUsage();
    cacheCoins.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(outpoint)),
        std::forward_as_tuple(std::move(coin), CCoinsCacheEntry::DIRTY));
}

void AddCoins(CCoinsViewCache& cache, const CTransaction &tx, int nHeight, bool check) {
    bool fCoinbase = tx.IsCoinBase();
    const uint256& txid = tx.GetHash();
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        bool overwrite = check ? cache.HaveCoin(COutPoint(txid, i)) : fCoinbase;
        // Always set the possible_overwrite flag to AddCoin for coinbase txn, in order to correctly
        // deal with the pre-BIP30 occurrences of duplicate coinbase transactions.
        cache.AddCoin(COutPoint(txid, i), Coin(tx.vout[i], nHeight, fCoinbase), overwrite);
    }
}

bool CCoinsViewCache::SpendCoin(const COutPoint &outpoint, Coin* moveout) {
    Modifier modifier(*this, outpoint);
    Coin& coin = modifier.Modify();
    TRACE5(utxocache, spent,
           outpoint.hash.data(),
           (uint32_t)outpoint.n,
           (uint32_t)it->second.coin.nHeight,
           (int64_t)it->second.coin.out.nValue,
           (bool)it->second.coin.IsCoinBase());
    bool already_spent = coin.IsSpent();
    if (moveout) {
        *moveout = std::move(coin);
    }
    coin.Clear();
    return !already_spent;
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

bool CCoinsViewCache::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlockIn) {
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end(); it = mapCoins.erase(it)) {
        // Ignore non-dirty entries (optimization).
        if (!(it->second.flags & CCoinsCacheEntry::DIRTY)) {
            continue;
        }
        Modifier(*this, it->first, std::move(it->second));
    }
    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, hashBlock);
    cacheCoins.clear();
    cachedCoinsUsage = 0;
    return fOk;
}

void CCoinsViewCache::Uncache(const COutPoint& hash)
{
    CCoinsMap::iterator it = cacheCoins.find(hash);
    if (it != cacheCoins.end() && it->second.flags == 0) {
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
    // Cache should be empty when we're calling this.
    assert(cacheCoins.size() == 0);
    cacheCoins.~CCoinsMap();
    ::new (&cacheCoins) CCoinsMap();
}

static const size_t MIN_TRANSACTION_OUTPUT_WEIGHT = WITNESS_SCALE_FACTOR * ::GetSerializeSize(CTxOut(), PROTOCOL_VERSION);
static const size_t MAX_OUTPUTS_PER_BLOCK = MAX_BLOCK_WEIGHT / MIN_TRANSACTION_OUTPUT_WEIGHT;

const Coin& AccessByTxid(const CCoinsViewCache& view, const uint256& txid)
{
    COutPoint iter(txid, 0);
    while (iter.n < MAX_OUTPUTS_PER_BLOCK) {
        const Coin& alternate = view.AccessCoin(iter);
        if (!alternate.IsSpent()) return alternate;
        ++iter.n;
    }
    return coinEmpty;
}

bool CCoinsViewErrorCatcher::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    try {
        return CCoinsViewBacked::GetCoin(outpoint, coin);
    } catch(const std::runtime_error& e) {
        for (auto f : m_err_callbacks) {
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

CCoinsViewCache::Modifier::Modifier(const CCoinsViewCache& cache, const COutPoint& outpoint)
    : m_cache(cache), m_outpoint(outpoint)
{
    if (m_cur_entry == m_cache.cacheCoins.end()) {
        m_new_entry.emplace();
        m_cache.base->GetCoin(m_outpoint, m_new_entry->coin);
        if (m_new_entry->coin.IsSpent()) {
            m_new_entry->flags |= CCoinsCacheEntry::FRESH;
        }
    }
}

CCoinsViewCache::Modifier::Modifier(const CCoinsViewCache& cache,
    const COutPoint& outpoint,
    CCoinsCacheEntry&& new_entry)
    : m_cache(cache), m_outpoint(outpoint)
{
    const bool cur_entry = m_cur_entry != m_cache.cacheCoins.end();
    const bool cur_spent = cur_entry && m_cur_entry->second.coin.IsSpent();
    const bool cur_dirty = cur_entry && m_cur_entry->second.flags & CCoinsCacheEntry::DIRTY;
    const bool cur_fresh = cur_entry && m_cur_entry->second.flags & CCoinsCacheEntry::FRESH;
    const bool new_spent = new_entry.coin.IsSpent();
    const bool new_dirty = new_entry.flags & CCoinsCacheEntry::DIRTY;
    const bool new_fresh = new_entry.flags & CCoinsCacheEntry::FRESH;

    // If the new value is marked FRESH, assert any existing cache entry is
    // spent, otherwise it means the FRESH flag was misapplied.
    if (new_fresh && cur_entry && !cur_spent) {
        throw std::logic_error("FRESH flag misapplied to cache of unspent coin");
    }

    // If a cache entry is spent but not dirty, it should be marked fresh.
    if (new_spent && !new_fresh && !new_dirty) {
        throw std::logic_error("Missing FRESH or DIRTY flags for spent cache entry.");
    }

    // Create new cache entry that can be merged into the cache in Flush().
    m_new_entry.emplace();
    m_new_entry->coin = std::move(new_entry.coin);

    // If `cur_fresh` is true it means the `m_cache.base` coin is spent, so
    // keep the FRESH flag. If `new_fresh` is true, it means that the `m_cache`
    // coin is spent, which implies that the `m_cache.base` coin is also spent
    // as long as the cache is not dirty, so keep the FRESH flag in this case as
    // well.
    if (cur_fresh || (new_fresh && !cur_dirty)) {
        m_new_entry->flags |= CCoinsCacheEntry::FRESH;
    }

    if (cur_dirty || new_dirty) {
        m_new_entry->flags |= CCoinsCacheEntry::DIRTY;
    }
}

// Add DIRTY flag to m_new_entry and return mutable coin reference. Populate
// m_new_entry from existing cache entry if necessary.
Coin& CCoinsViewCache::Modifier::Modify()
{
    if (!m_new_entry) {
        assert(m_cur_entry != m_cache.cacheCoins.end());
        m_new_entry.emplace(m_cur_entry->second);
    }
    m_new_entry->flags |= CCoinsCacheEntry::DIRTY;
    return m_new_entry->coin;
}

// Update m_cache.cacheCoins with the contents of m_new_entry, if present.
CCoinsMap::iterator CCoinsViewCache::Modifier::Flush()
{
    if (m_new_entry) {
        bool erase = (m_new_entry->flags & CCoinsCacheEntry::FRESH) && m_new_entry->coin.IsSpent();
        if (m_cur_entry != m_cache.cacheCoins.end()) {
            m_cache.cachedCoinsUsage -= m_cur_entry->second.coin.DynamicMemoryUsage();
            if (erase) {
                m_cache.cacheCoins.erase(m_cur_entry);
                m_cur_entry = m_cache.cacheCoins.end();
            } else {
                m_cur_entry->second = std::move(*m_new_entry);
            }
        } else if (!erase) {
            m_cur_entry = m_cache.cacheCoins.emplace(m_outpoint, std::move(*m_new_entry)).first;
        }
        if (!erase) {
            m_cache.cachedCoinsUsage += m_cur_entry->second.coin.DynamicMemoryUsage();
        }
    }
    m_new_entry.reset();
    return m_cur_entry;
}
