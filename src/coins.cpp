// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"

#include "consensus/consensus.h"
#include "memusage.h"
#include "random.h"
#include "util.h"

#include <assert.h>

bool CCoinsView::GetCoins(const COutPoint &outpoint, Coin &coin) const { return false; }
bool CCoinsView::HaveCoins(const COutPoint &outpoint) const { return false; }
uint256 CCoinsView::GetBestBlock() const { return uint256(); }
bool CCoinsView::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock, size_t &nChildCachedCoinsUsage) { return false; }
<<<<<<< HEAD
CCoinsViewCursor *CCoinsView::Cursor() const { return 0; }
=======
CCoinsViewCursor *CCoinsView::Cursor() const { return nullptr; }
>>>>>>> 239f540... Switch CCoinsView and chainstate db from per-txid to per-txout


CCoinsViewBacked::CCoinsViewBacked(CCoinsView *viewIn) : base(viewIn) { }
bool CCoinsViewBacked::GetCoins(const COutPoint &outpoint, Coin &coin) const { return base->GetCoins(outpoint, coin); }
bool CCoinsViewBacked::HaveCoins(const COutPoint &outpoint) const { return base->HaveCoins(outpoint); }
uint256 CCoinsViewBacked::GetBestBlock() const { return base->GetBestBlock(); }
void CCoinsViewBacked::SetBackend(CCoinsView &viewIn) { base = &viewIn; }
bool CCoinsViewBacked::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock, size_t &nChildCachedCoinsUsage) { return base->BatchWrite(mapCoins, hashBlock, nChildCachedCoinsUsage); }
CCoinsViewCursor *CCoinsViewBacked::Cursor() const { return base->Cursor(); }
size_t CCoinsViewBacked::EstimateSize() const { return base->EstimateSize(); }

SaltedOutpointHasher::SaltedOutpointHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

CCoinsViewCache::CCoinsViewCache(CCoinsView *baseIn) : CCoinsViewBacked(baseIn), cachedCoinsUsage(0) {}

size_t CCoinsViewCache::DynamicMemoryUsage() const {
    LOCK(cs_utxo);
    return memusage::DynamicUsage(cacheCoins) + cachedCoinsUsage;
}

size_t CCoinsViewCache::ResetCachedCoinUsage() const
{

    LOCK(cs_utxo);
    size_t newCachedCoinsUsage = 0;
    for (CCoinsMap::iterator it = cacheCoins.begin(); it != cacheCoins.end(); it++)
        newCachedCoinsUsage += it->second.coins.DynamicMemoryUsage();
    if (cachedCoinsUsage != newCachedCoinsUsage)
    {
        error("Resetting: cachedCoinsUsage has drifted - before %lld after %lld", cachedCoinsUsage,
            newCachedCoinsUsage);
        cachedCoinsUsage = newCachedCoinsUsage;
    }
    return newCachedCoinsUsage;
}

CCoinsMap::iterator CCoinsViewCache::FetchCoins(const COutPoint &outpoint) const {
    // requires cs_utxo
    CCoinsMap::iterator it = cacheCoins.find(outpoint);
    if (it != cacheCoins.end())
        return it;
    Coin tmp;
    if (!base->GetCoins(outpoint, tmp))
        return cacheCoins.end();
    CCoinsMap::iterator ret = cacheCoins.emplace(std::piecewise_construct, std::forward_as_tuple(outpoint), std::forward_as_tuple(std::move(tmp))).first;
    if (ret->second.coins.IsPruned()) {
        // The parent only has an empty entry for this outpoint; we can consider our
        // version as fresh.
        ret->second.flags = CCoinsCacheEntry::FRESH;
    }
    cachedCoinsUsage += ret->second.coins.DynamicMemoryUsage();
    return ret;
}

bool CCoinsViewCache::GetCoins(const COutPoint &outpoint, Coin &coin) const {
    CCoinsMap::const_iterator it = FetchCoins(outpoint);
    if (it != cacheCoins.end()) {
        coin = it->second.coins;
        return true;
    }
    return false;
}

void CCoinsViewCache::AddCoin(const COutPoint &outpoint, Coin&& coin, bool possible_overwrite) {
    assert(!coin.IsPruned());
    if (coin.out.scriptPubKey.IsUnspendable()) return;
    CCoinsMap::iterator it;
    bool inserted;
    std::tie(it, inserted) = cacheCoins.emplace(std::piecewise_construct, std::forward_as_tuple(outpoint), std::tuple<>());
    bool fresh = false;
    if (!inserted) {
        cachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();
    }
    if (!possible_overwrite) {
        if (!it->second.coins.IsPruned()) {
            throw std::logic_error("Adding new coin that replaces non-pruned entry");
        }
        fresh = !(it->second.flags & CCoinsCacheEntry::DIRTY);
    }
    it->second.coins = std::move(coin);
    it->second.flags |= CCoinsCacheEntry::DIRTY | (fresh ? CCoinsCacheEntry::FRESH : 0);
    cachedCoinsUsage += it->second.coins.DynamicMemoryUsage();
}

void AddCoins(CCoinsViewCache& cache, const CTransaction &tx, int nHeight) {
    bool fCoinbase = tx.IsCoinBase();
    const uint256& txid = tx.GetHash();
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        // Pass fCoinbase as the possible_overwrite flag to AddCoin, in order to correctly
        // deal with the pre-BIP30 occurrances of duplicate coinbase transactions.
        cache.AddCoin(COutPoint(txid, i), Coin(tx.vout[i], nHeight, fCoinbase), fCoinbase);
    }
}

void CCoinsViewCache::SpendCoin(const COutPoint &outpoint, Coin* moveout) {
    CCoinsMap::iterator it = FetchCoins(outpoint);
    if (it == cacheCoins.end()) return;
    cachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();
    if (moveout) {
        *moveout = std::move(it->second.coins);
    }
    if (it->second.flags & CCoinsCacheEntry::FRESH) {
        cacheCoins.erase(it);
    } else {
        it->second.flags |= CCoinsCacheEntry::DIRTY;
        it->second.coins.Clear();
    }
}

static const Coin coinEmpty;

const Coin& CCoinsViewCache::AccessCoin(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = FetchCoins(outpoint);
    if (it == cacheCoins.end()) {
        return coinEmpty;
    } else {
        return it->second.coins;
    }
}

bool CCoinsViewCache::HaveCoins(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = FetchCoins(outpoint);
    return (it != cacheCoins.end() && !it->second.coins.IsPruned());
}

bool CCoinsViewCache::HaveCoinsInCache(const COutPoint &outpoint) const {
    CCoinsMap::const_iterator it = cacheCoins.find(outpoint);
    return it != cacheCoins.end();
}

uint256 CCoinsViewCache::GetBestBlock() const {
    LOCK(cs_utxo);
    if (hashBlock.IsNull())
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

void CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    LOCK(cs_utxo);
    hashBlock = hashBlockIn;
}

bool CCoinsViewCache::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlockIn, size_t &nChildCachedCoinsUsage) {
    LOCK(cs_utxo);
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        
        if (it->second.flags & CCoinsCacheEntry::DIRTY) { // Ignore non-dirty entries (optimization).
            // Update usage of the chile cache before we do any swapping and deleting
            nChildCachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();

            CCoinsMap::iterator itUs = cacheCoins.find(it->first);
            if (itUs == cacheCoins.end()) {
                // The parent cache does not have an entry, while the child does
                // We can ignore it if it's both FRESH and pruned in the child
                if (!(it->second.flags & CCoinsCacheEntry::FRESH && it->second.coins.IsPruned())) {
                    // Otherwise we will need to create it in the parent
                    // and move the data up and mark it as dirty
                    CCoinsCacheEntry& entry = cacheCoins[it->first];
                    entry.coins = std::move(it->second.coins);
                    cachedCoinsUsage += entry.coins.DynamicMemoryUsage();
                    entry.flags = CCoinsCacheEntry::DIRTY;
                    // We can mark it FRESH in the parent if it was FRESH in the child
                    // Otherwise it might have just been flushed from the parent's cache
                    // and already exist in the grandparent
                    if (it->second.flags & CCoinsCacheEntry::FRESH)
                        entry.flags |= CCoinsCacheEntry::FRESH;
                }
            } else {
                // Found the entry in the parent cache
                if ((itUs->second.flags & CCoinsCacheEntry::FRESH) && it->second.coins.IsPruned()) {
                    // The grandparent does not have an entry, and the child is
                    // modified and being pruned. This means we can just delete
                    // it from the parent.
                    cachedCoinsUsage -= itUs->second.coins.DynamicMemoryUsage();
                    cacheCoins.erase(itUs);
                } else {
                    // A normal modification.
                    cachedCoinsUsage -= itUs->second.coins.DynamicMemoryUsage();
                    itUs->second.coins = std::move(it->second.coins);
                    cachedCoinsUsage += itUs->second.coins.DynamicMemoryUsage();
                    itUs->second.flags |= CCoinsCacheEntry::DIRTY;
                }
            }

            CCoinsMap::iterator itOld = it++;
            mapCoins.erase(itOld);
        }
        else
            it++;
    }
    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::Flush() {
    LOCK(cs_utxo);
    bool fOk = base->BatchWrite(cacheCoins, hashBlock, cachedCoinsUsage);
    return fOk;
}

void CCoinsViewCache::Trim(size_t nTrimSize) const
{
    uint64_t nTrimmed = 0;

    LOCK(cs_utxo);
    CCoinsMap::iterator iter = cacheCoins.begin();
    while (DynamicMemoryUsage() > nTrimSize)
    {
        if (iter == cacheCoins.end())
            break;

        // Only erase entries that have not been modified
        if (iter->second.flags == 0)
        {
            cachedCoinsUsage -= iter->second.coins.DynamicMemoryUsage();

            CCoinsMap::iterator itOld = iter++;
            cacheCoins.erase(itOld);
            nTrimmed++;
        }
        else
            iter++;
    }

    if (nTrimmed > 0)
        LogPrint("coindb", "Trimmed %ld from the CoinsViewCache, current size after trim: %ld and usage %ld bytes\n", nTrimmed, cacheCoins.size(), cachedCoinsUsage);
}

void CCoinsViewCache::Uncache(const COutPoint& hash)
{
    LOCK(cs_utxo);
    CCoinsMap::iterator it = cacheCoins.find(hash);
    if (it != cacheCoins.end() && it->second.flags == 0) {
        cachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();
        cacheCoins.erase(it);
    }
}

unsigned int CCoinsViewCache::GetCacheSize() const {
    LOCK(cs_utxo);
    return cacheCoins.size();
}

CAmount CCoinsViewCache::GetValueIn(const CTransaction& tx) const
{
    LOCK(cs_utxo);
    if (tx.IsCoinBase())
        return 0;

    CAmount nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += AccessCoin(tx.vin[i].prevout).out.nValue;

    return nResult;
}

bool CCoinsViewCache::HaveInputs(const CTransaction& tx) const
{
    LOCK(cs_utxo);
    if (!tx.IsCoinBase()) {
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            if (!HaveCoins(tx.vin[i].prevout)) {
                return false;
            }
        }
    }
    return true;
}

double CCoinsViewCache::GetPriority(const CTransaction &tx, int nHeight, CAmount &inChainInputValue) const
{
    LOCK(cs_utxo);
    inChainInputValue = 0;
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        const Coin &coin = AccessCoin(txin.prevout);
        if (coin.IsPruned()) continue;
        if (coin.nHeight <= nHeight) {
            dResult += coin.out.nValue * (nHeight-coin.nHeight);
            inChainInputValue += coin.out.nValue;
        }
    }
    return tx.ComputePriority(dResult);
}


CCoinsViewCursor::~CCoinsViewCursor()
{
}

static const size_t MAX_OUTPUTS_PER_BLOCK = MAX_BLOCK_BASE_SIZE /  ::GetSerializeSize(CTxOut(), SER_NETWORK, PROTOCOL_VERSION); // TODO: 
const Coin& AccessByTxid(const CCoinsViewCache& view, const uint256& txid)
{
    COutPoint iter(txid, 0);
    while (iter.n < MAX_OUTPUTS_PER_BLOCK) {
        const Coin& alternate = view.AccessCoin(iter);
        if (!alternate.IsPruned()) return alternate;
        ++iter.n;
    }
    return coinEmpty;
}


