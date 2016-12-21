// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"
#include "script/standard.h"
#include "uint256.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"
#include "test/test_random.h"
#include "validation.h"
#include "consensus/validation.h"

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

namespace
{
class CCoinsViewTest : public CCoinsView
{
    uint256 hashBestBlock_;
    std::map<uint256, CCoins> map_;

public:
    bool GetCoins(const uint256& txid, CCoins& coins) const
    {
        std::map<uint256, CCoins>::const_iterator it = map_.find(txid);
        if (it == map_.end()) {
            return false;
        }
        coins = it->second;
        if (coins.IsPruned() && insecure_rand() % 2 == 0) {
            // Randomly return false in case of an empty entry.
            return false;
        }
        return true;
    }

    bool HaveCoins(const uint256& txid) const
    {
        CCoins coins;
        return GetCoins(txid, coins);
    }

    uint256 GetBestBlock() const { return hashBestBlock_; }

    bool BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
    {
        for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end(); ) {
            if (it->second.flags & CCoinsCacheEntry::DIRTY) {
                // Same optimization used in CCoinsViewDB is to only write dirty entries.
                map_[it->first] = it->second.coins;
                if (it->second.coins.IsPruned() && insecure_rand() % 3 == 0) {
                    // Randomly delete empty entries on write.
                    map_.erase(it->first);
                }
            }
            mapCoins.erase(it++);
        }
        if (!hashBlock.IsNull())
            hashBestBlock_ = hashBlock;
        return true;
    }
};

class CCoinsViewCacheTest : public CCoinsViewCache
{
public:
    CCoinsViewCacheTest(CCoinsView* base) : CCoinsViewCache(base) {}

    void SelfTest() const
    {
        // Manually recompute the dynamic usage of the whole data, and compare it.
        size_t ret = memusage::DynamicUsage(cacheCoins);
        for (CCoinsMap::iterator it = cacheCoins.begin(); it != cacheCoins.end(); it++) {
            ret += it->second.coins.DynamicMemoryUsage();
        }
        BOOST_CHECK_EQUAL(DynamicMemoryUsage(), ret);
    }

    CCoinsMap& map() { return cacheCoins; }
    size_t& usage() { return cachedCoinsUsage; }
};

}

BOOST_FIXTURE_TEST_SUITE(coins_tests, BasicTestingSetup)

static const unsigned int NUM_SIMULATION_ITERATIONS = 40000;

// This is a large randomized insert/remove simulation test on a variable-size
// stack of caches on top of CCoinsViewTest.
//
// It will randomly create/update/delete CCoins entries to a tip of caches, with
// txids picked from a limited list of random 256-bit hashes. Occasionally, a
// new tip is added to the stack of caches, or the tip is flushed and removed.
//
// During the process, booleans are kept to make sure that the randomized
// operation hits all branches.
BOOST_AUTO_TEST_CASE(coins_cache_simulation_test)
{
    // Various coverage trackers.
    bool removed_all_caches = false;
    bool reached_4_caches = false;
    bool added_an_entry = false;
    bool removed_an_entry = false;
    bool updated_an_entry = false;
    bool found_an_entry = false;
    bool missed_an_entry = false;

    // A simple map to track what we expect the cache stack to represent.
    std::map<uint256, CCoins> result;

    // The cache stack.
    CCoinsViewTest base; // A CCoinsViewTest at the bottom.
    std::vector<CCoinsViewCacheTest*> stack; // A stack of CCoinsViewCaches on top.
    stack.push_back(new CCoinsViewCacheTest(&base)); // Start with one cache.

    // Use a limited set of random transaction ids, so we do test overwriting entries.
    std::vector<uint256> txids;
    txids.resize(NUM_SIMULATION_ITERATIONS / 8);
    for (unsigned int i = 0; i < txids.size(); i++) {
        txids[i] = GetRandHash();
    }

    for (unsigned int i = 0; i < NUM_SIMULATION_ITERATIONS; i++) {
        // Do a random modification.
        {
            uint256 txid = txids[insecure_rand() % txids.size()]; // txid we're going to modify in this iteration.
            CCoins& coins = result[txid];
            CCoinsModifier entry = stack.back()->ModifyCoins(txid);
            BOOST_CHECK(coins == *entry);
            if (insecure_rand() % 5 == 0 || coins.IsPruned()) {
                if (coins.IsPruned()) {
                    added_an_entry = true;
                } else {
                    updated_an_entry = true;
                }
                coins.nVersion = insecure_rand();
                coins.vout.resize(1);
                coins.vout[0].nValue = insecure_rand();
                *entry = coins;
            } else {
                coins.Clear();
                entry->Clear();
                removed_an_entry = true;
            }
        }

        // Once every 1000 iterations and at the end, verify the full cache.
        if (insecure_rand() % 1000 == 1 || i == NUM_SIMULATION_ITERATIONS - 1) {
            for (std::map<uint256, CCoins>::iterator it = result.begin(); it != result.end(); it++) {
                const CCoins* coins = stack.back()->AccessCoins(it->first);
                if (coins) {
                    BOOST_CHECK(*coins == it->second);
                    found_an_entry = true;
                } else {
                    BOOST_CHECK(it->second.IsPruned());
                    missed_an_entry = true;
                }
            }
            BOOST_FOREACH(const CCoinsViewCacheTest *test, stack) {
                test->SelfTest();
            }
        }

        if (insecure_rand() % 100 == 0) {
            // Every 100 iterations, flush an intermediate cache
            if (stack.size() > 1 && insecure_rand() % 2 == 0) {
                unsigned int flushIndex = insecure_rand() % (stack.size() - 1);
                stack[flushIndex]->Flush();
            }
        }
        if (insecure_rand() % 100 == 0) {
            // Every 100 iterations, change the cache stack.
            if (stack.size() > 0 && insecure_rand() % 2 == 0) {
                //Remove the top cache
                stack.back()->Flush();
                delete stack.back();
                stack.pop_back();
            }
            if (stack.size() == 0 || (stack.size() < 4 && insecure_rand() % 2)) {
                //Add a new cache
                CCoinsView* tip = &base;
                if (stack.size() > 0) {
                    tip = stack.back();
                } else {
                    removed_all_caches = true;
                }
                stack.push_back(new CCoinsViewCacheTest(tip));
                if (stack.size() == 4) {
                    reached_4_caches = true;
                }
            }
        }
    }

    // Clean up the stack.
    while (stack.size() > 0) {
        delete stack.back();
        stack.pop_back();
    }

    // Verify coverage.
    BOOST_CHECK(removed_all_caches);
    BOOST_CHECK(reached_4_caches);
    BOOST_CHECK(added_an_entry);
    BOOST_CHECK(removed_an_entry);
    BOOST_CHECK(updated_an_entry);
    BOOST_CHECK(found_an_entry);
    BOOST_CHECK(missed_an_entry);
}

// This test is similar to the previous test
// except the emphasis is on testing the functionality of UpdateCoins
// random txs are created and UpdateCoins is used to update the cache stack
// In particular it is tested that spending a duplicate coinbase tx
// has the expected effect (the other duplicate is overwitten at all cache levels)
BOOST_AUTO_TEST_CASE(updatecoins_simulation_test)
{
    bool spent_a_duplicate_coinbase = false;
    // A simple map to track what we expect the cache stack to represent.
    std::map<uint256, CCoins> result;

    // The cache stack.
    CCoinsViewTest base; // A CCoinsViewTest at the bottom.
    std::vector<CCoinsViewCacheTest*> stack; // A stack of CCoinsViewCaches on top.
    stack.push_back(new CCoinsViewCacheTest(&base)); // Start with one cache.

    // Track the txids we've used and whether they have been spent or not
    std::map<uint256, CAmount> coinbaseids;
    std::set<uint256> alltxids;
    std::set<uint256> duplicateids;

    for (unsigned int i = 0; i < NUM_SIMULATION_ITERATIONS; i++) {
        {
            CMutableTransaction tx;
            tx.vin.resize(1);
            tx.vout.resize(1);
            tx.vout[0].nValue = i; //Keep txs unique unless intended to duplicate
            unsigned int height = insecure_rand();

            // 1/10 times create a coinbase
            if (insecure_rand() % 10 == 0 || coinbaseids.size() < 10) {
                // 1/100 times create a duplicate coinbase
                if (insecure_rand() % 10 == 0 && coinbaseids.size()) {
                    std::map<uint256, CAmount>::iterator coinbaseIt = coinbaseids.lower_bound(GetRandHash());
                    if (coinbaseIt == coinbaseids.end()) {
                        coinbaseIt = coinbaseids.begin();
                    }
                    //Use same random value to have same hash and be a true duplicate
                    tx.vout[0].nValue = coinbaseIt->second;
                    assert(tx.GetHash() == coinbaseIt->first);
                    duplicateids.insert(coinbaseIt->first);
                }
                else {
                    coinbaseids[tx.GetHash()] = tx.vout[0].nValue;
                }
                assert(CTransaction(tx).IsCoinBase());
            }
            // 9/10 times create a regular tx
            else {
                uint256 prevouthash;
                // equally likely to spend coinbase or non coinbase
                std::set<uint256>::iterator txIt = alltxids.lower_bound(GetRandHash());
                if (txIt == alltxids.end()) {
                    txIt = alltxids.begin();
                }
                prevouthash = *txIt;

                // Construct the tx to spend the coins of prevouthash
                tx.vin[0].prevout.hash = prevouthash;
                tx.vin[0].prevout.n = 0;

                // Update the expected result of prevouthash to know these coins are spent
                CCoins& oldcoins = result[prevouthash];
                oldcoins.Clear();

                // It is of particular importance here that once we spend a coinbase tx hash
                // it is no longer available to be duplicated (or spent again)
                // BIP 34 in conjunction with enforcing BIP 30 (at least until BIP 34 was active)
                // results in the fact that no coinbases were duplicated after they were already spent
                alltxids.erase(prevouthash);
                coinbaseids.erase(prevouthash);

                // The test is designed to ensure spending a duplicate coinbase will work properly
                // if that ever happens and not resurrect the previously overwritten coinbase
                if (duplicateids.count(prevouthash))
                    spent_a_duplicate_coinbase = true;

                assert(!CTransaction(tx).IsCoinBase());
            }
            // Track this tx to possibly spend later
            alltxids.insert(tx.GetHash());

            // Update the expected result to know about the new output coins
            CCoins &coins = result[tx.GetHash()];
            coins.FromTx(tx, height);

            UpdateCoins(tx, *(stack.back()), height);
        }

        // Once every 1000 iterations and at the end, verify the full cache.
        if (insecure_rand() % 1000 == 1 || i == NUM_SIMULATION_ITERATIONS - 1) {
            for (std::map<uint256, CCoins>::iterator it = result.begin(); it != result.end(); it++) {
                const CCoins* coins = stack.back()->AccessCoins(it->first);
                if (coins) {
                    BOOST_CHECK(*coins == it->second);
                 } else {
                    BOOST_CHECK(it->second.IsPruned());
                 }
            }
        }

        if (insecure_rand() % 100 == 0) {
            // Every 100 iterations, flush an intermediate cache
            if (stack.size() > 1 && insecure_rand() % 2 == 0) {
                unsigned int flushIndex = insecure_rand() % (stack.size() - 1);
                stack[flushIndex]->Flush();
            }
        }
        if (insecure_rand() % 100 == 0) {
            // Every 100 iterations, change the cache stack.
            if (stack.size() > 0 && insecure_rand() % 2 == 0) {
                stack.back()->Flush();
                delete stack.back();
                stack.pop_back();
            }
            if (stack.size() == 0 || (stack.size() < 4 && insecure_rand() % 2)) {
                CCoinsView* tip = &base;
                if (stack.size() > 0) {
                    tip = stack.back();
                }
                stack.push_back(new CCoinsViewCacheTest(tip));
           }
        }
    }

    // Clean up the stack.
    while (stack.size() > 0) {
        delete stack.back();
        stack.pop_back();
    }

    // Verify coverage.
    BOOST_CHECK(spent_a_duplicate_coinbase);
}

BOOST_AUTO_TEST_CASE(ccoins_serialization)
{
    // Good example
    CDataStream ss1(ParseHex("0104835800816115944e077fe7c803cfa57f29b36bf87c1d358bb85e"), SER_DISK, CLIENT_VERSION);
    CCoins cc1;
    ss1 >> cc1;
    BOOST_CHECK_EQUAL(cc1.nVersion, 1);
    BOOST_CHECK_EQUAL(cc1.fCoinBase, false);
    BOOST_CHECK_EQUAL(cc1.nHeight, 203998);
    BOOST_CHECK_EQUAL(cc1.vout.size(), 2);
    BOOST_CHECK_EQUAL(cc1.IsAvailable(0), false);
    BOOST_CHECK_EQUAL(cc1.IsAvailable(1), true);
    BOOST_CHECK_EQUAL(cc1.vout[1].nValue, 60000000000ULL);
    BOOST_CHECK_EQUAL(HexStr(cc1.vout[1].scriptPubKey), HexStr(GetScriptForDestination(CKeyID(uint160(ParseHex("816115944e077fe7c803cfa57f29b36bf87c1d35"))))));

    // Good example
    CDataStream ss2(ParseHex("0109044086ef97d5790061b01caab50f1b8e9c50a5057eb43c2d9563a4eebbd123008c988f1a4a4de2161e0f50aac7f17e7f9555caa486af3b"), SER_DISK, CLIENT_VERSION);
    CCoins cc2;
    ss2 >> cc2;
    BOOST_CHECK_EQUAL(cc2.nVersion, 1);
    BOOST_CHECK_EQUAL(cc2.fCoinBase, true);
    BOOST_CHECK_EQUAL(cc2.nHeight, 120891);
    BOOST_CHECK_EQUAL(cc2.vout.size(), 17);
    for (int i = 0; i < 17; i++) {
        BOOST_CHECK_EQUAL(cc2.IsAvailable(i), i == 4 || i == 16);
    }
    BOOST_CHECK_EQUAL(cc2.vout[4].nValue, 234925952);
    BOOST_CHECK_EQUAL(HexStr(cc2.vout[4].scriptPubKey), HexStr(GetScriptForDestination(CKeyID(uint160(ParseHex("61b01caab50f1b8e9c50a5057eb43c2d9563a4ee"))))));
    BOOST_CHECK_EQUAL(cc2.vout[16].nValue, 110397);
    BOOST_CHECK_EQUAL(HexStr(cc2.vout[16].scriptPubKey), HexStr(GetScriptForDestination(CKeyID(uint160(ParseHex("8c988f1a4a4de2161e0f50aac7f17e7f9555caa4"))))));

    // Smallest possible example
    CDataStream ssx(SER_DISK, CLIENT_VERSION);
    BOOST_CHECK_EQUAL(HexStr(ssx.begin(), ssx.end()), "");

    CDataStream ss3(ParseHex("0002000600"), SER_DISK, CLIENT_VERSION);
    CCoins cc3;
    ss3 >> cc3;
    BOOST_CHECK_EQUAL(cc3.nVersion, 0);
    BOOST_CHECK_EQUAL(cc3.fCoinBase, false);
    BOOST_CHECK_EQUAL(cc3.nHeight, 0);
    BOOST_CHECK_EQUAL(cc3.vout.size(), 1);
    BOOST_CHECK_EQUAL(cc3.IsAvailable(0), true);
    BOOST_CHECK_EQUAL(cc3.vout[0].nValue, 0);
    BOOST_CHECK_EQUAL(cc3.vout[0].scriptPubKey.size(), 0);

    // scriptPubKey that ends beyond the end of the stream
    CDataStream ss4(ParseHex("0002000800"), SER_DISK, CLIENT_VERSION);
    try {
        CCoins cc4;
        ss4 >> cc4;
        BOOST_CHECK_MESSAGE(false, "We should have thrown");
    } catch (const std::ios_base::failure& e) {
    }

    // Very large scriptPubKey (3*10^9 bytes) past the end of the stream
    CDataStream tmp(SER_DISK, CLIENT_VERSION);
    uint64_t x = 3000000000ULL;
    tmp << VARINT(x);
    BOOST_CHECK_EQUAL(HexStr(tmp.begin(), tmp.end()), "8a95c0bb00");
    CDataStream ss5(ParseHex("0002008a95c0bb0000"), SER_DISK, CLIENT_VERSION);
    try {
        CCoins cc5;
        ss5 >> cc5;
        BOOST_CHECK_MESSAGE(false, "We should have thrown");
    } catch (const std::ios_base::failure& e) {
    }
}

const static uint256 TXID;
const static CAmount PRUNED = -1;
const static CAmount ABSENT = -2;
const static CAmount VALUE1 = 100;
const static CAmount VALUE2 = 200;
const static CAmount VALUE3 = 300;
const static char DIRTY = CCoinsCacheEntry::DIRTY;
const static char FRESH = CCoinsCacheEntry::FRESH;
const static char NO_ENTRY = -1;

const static auto FLAGS = {char(0), FRESH, DIRTY, char(DIRTY | FRESH)};
const static auto CLEAN_FLAGS = {char(0), FRESH};
const static auto DIRTY_FLAGS = {DIRTY, char(DIRTY | FRESH)};
const static auto ABSENT_FLAGS = {NO_ENTRY};

void SetCoinsValue(CAmount value, CCoins& coins)
{
    assert(value != ABSENT);
    coins.Clear();
    assert(coins.IsPruned());
    if (value != PRUNED) {
        coins.vout.emplace_back();
        coins.vout.back().nValue = value;
        assert(!coins.IsPruned());
    }
}

size_t InsertCoinsMapEntry(CCoinsMap& map, CAmount value, char flags)
{
    if (value == ABSENT) {
        assert(flags == NO_ENTRY);
        return 0;
    }
    assert(flags != NO_ENTRY);
    CCoinsCacheEntry entry;
    entry.flags = flags;
    SetCoinsValue(value, entry.coins);
    auto inserted = map.emplace(TXID, std::move(entry));
    assert(inserted.second);
    return inserted.first->second.coins.DynamicMemoryUsage();
}

void GetCoinsMapEntry(const CCoinsMap& map, CAmount& value, char& flags)
{
    auto it = map.find(TXID);
    if (it == map.end()) {
        value = ABSENT;
        flags = NO_ENTRY;
    } else {
        if (it->second.coins.IsPruned()) {
            assert(it->second.coins.vout.size() == 0);
            value = PRUNED;
        } else {
            assert(it->second.coins.vout.size() == 1);
            value = it->second.coins.vout[0].nValue;
        }
        flags = it->second.flags;
        assert(flags != NO_ENTRY);
    }
}

void WriteCoinsViewEntry(CCoinsView& view, CAmount value, char flags)
{
    CCoinsMap map;
    InsertCoinsMapEntry(map, value, flags);
    view.BatchWrite(map, {});
}

class SingleEntryCacheTest
{
public:
    SingleEntryCacheTest(CAmount base_value, CAmount cache_value, char cache_flags)
    {
        WriteCoinsViewEntry(base, base_value, base_value == ABSENT ? NO_ENTRY : DIRTY);
        cache.usage() += InsertCoinsMapEntry(cache.map(), cache_value, cache_flags);
    }

    CCoinsView root;
    CCoinsViewCacheTest base{&root};
    CCoinsViewCacheTest cache{&base};
};

void CheckAccessCoins(CAmount base_value, CAmount cache_value, CAmount expected_value, char cache_flags, char expected_flags)
{
    SingleEntryCacheTest test(base_value, cache_value, cache_flags);
    test.cache.AccessCoins(TXID);
    test.cache.SelfTest();

    CAmount result_value;
    char result_flags;
    GetCoinsMapEntry(test.cache.map(), result_value, result_flags);
    BOOST_CHECK_EQUAL(result_value, expected_value);
    BOOST_CHECK_EQUAL(result_flags, expected_flags);
}

BOOST_AUTO_TEST_CASE(ccoins_access)
{
    /* Check AccessCoin behavior, requesting a coin from a cache view layered on
     * top of a base view, and checking the resulting entry in the cache after
     * the access.
     *
     *               Base    Cache   Result  Cache        Result
     *               Value   Value   Value   Flags        Flags
     */
    CheckAccessCoins(ABSENT, ABSENT, ABSENT, NO_ENTRY   , NO_ENTRY   );
    CheckAccessCoins(ABSENT, PRUNED, PRUNED, 0          , 0          );
    CheckAccessCoins(ABSENT, PRUNED, PRUNED, FRESH      , FRESH      );
    CheckAccessCoins(ABSENT, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckAccessCoins(ABSENT, PRUNED, PRUNED, DIRTY|FRESH, DIRTY|FRESH);
    CheckAccessCoins(ABSENT, VALUE2, VALUE2, 0          , 0          );
    CheckAccessCoins(ABSENT, VALUE2, VALUE2, FRESH      , FRESH      );
    CheckAccessCoins(ABSENT, VALUE2, VALUE2, DIRTY      , DIRTY      );
    CheckAccessCoins(ABSENT, VALUE2, VALUE2, DIRTY|FRESH, DIRTY|FRESH);
    CheckAccessCoins(PRUNED, ABSENT, PRUNED, NO_ENTRY   , FRESH      );
    CheckAccessCoins(PRUNED, PRUNED, PRUNED, 0          , 0          );
    CheckAccessCoins(PRUNED, PRUNED, PRUNED, FRESH      , FRESH      );
    CheckAccessCoins(PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckAccessCoins(PRUNED, PRUNED, PRUNED, DIRTY|FRESH, DIRTY|FRESH);
    CheckAccessCoins(PRUNED, VALUE2, VALUE2, 0          , 0          );
    CheckAccessCoins(PRUNED, VALUE2, VALUE2, FRESH      , FRESH      );
    CheckAccessCoins(PRUNED, VALUE2, VALUE2, DIRTY      , DIRTY      );
    CheckAccessCoins(PRUNED, VALUE2, VALUE2, DIRTY|FRESH, DIRTY|FRESH);
    CheckAccessCoins(VALUE1, ABSENT, VALUE1, NO_ENTRY   , 0          );
    CheckAccessCoins(VALUE1, PRUNED, PRUNED, 0          , 0          );
    CheckAccessCoins(VALUE1, PRUNED, PRUNED, FRESH      , FRESH      );
    CheckAccessCoins(VALUE1, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckAccessCoins(VALUE1, PRUNED, PRUNED, DIRTY|FRESH, DIRTY|FRESH);
    CheckAccessCoins(VALUE1, VALUE2, VALUE2, 0          , 0          );
    CheckAccessCoins(VALUE1, VALUE2, VALUE2, FRESH      , FRESH      );
    CheckAccessCoins(VALUE1, VALUE2, VALUE2, DIRTY      , DIRTY      );
    CheckAccessCoins(VALUE1, VALUE2, VALUE2, DIRTY|FRESH, DIRTY|FRESH);
}

void CheckModifyCoins(CAmount base_value, CAmount cache_value, CAmount modify_value, CAmount expected_value, char cache_flags, char expected_flags)
{
    SingleEntryCacheTest test(base_value, cache_value, cache_flags);
    SetCoinsValue(modify_value, *test.cache.ModifyCoins(TXID));
    test.cache.SelfTest();

    CAmount result_value;
    char result_flags;
    GetCoinsMapEntry(test.cache.map(), result_value, result_flags);
    BOOST_CHECK_EQUAL(result_value, expected_value);
    BOOST_CHECK_EQUAL(result_flags, expected_flags);
};

BOOST_AUTO_TEST_CASE(ccoins_modify)
{
    /* Check ModifyCoin behavior, requesting a coin from a cache view layered on
     * top of a base view, writing a modification to the coin, and then checking
     * the resulting entry in the cache after the modification.
     *
     *               Base    Cache   Write   Result  Cache        Result
     *               Value   Value   Value   Value   Flags        Flags
     */
    CheckModifyCoins(ABSENT, ABSENT, PRUNED, ABSENT, NO_ENTRY   , NO_ENTRY   );
    CheckModifyCoins(ABSENT, ABSENT, VALUE3, VALUE3, NO_ENTRY   , DIRTY|FRESH);
    CheckModifyCoins(ABSENT, PRUNED, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(ABSENT, PRUNED, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(ABSENT, PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(ABSENT, PRUNED, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(ABSENT, PRUNED, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(ABSENT, PRUNED, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(ABSENT, PRUNED, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(ABSENT, PRUNED, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
    CheckModifyCoins(ABSENT, VALUE2, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(ABSENT, VALUE2, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(ABSENT, VALUE2, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(ABSENT, VALUE2, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(ABSENT, VALUE2, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(ABSENT, VALUE2, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(ABSENT, VALUE2, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(ABSENT, VALUE2, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
    CheckModifyCoins(PRUNED, ABSENT, PRUNED, ABSENT, NO_ENTRY   , NO_ENTRY   );
    CheckModifyCoins(PRUNED, ABSENT, VALUE3, VALUE3, NO_ENTRY   , DIRTY|FRESH);
    CheckModifyCoins(PRUNED, PRUNED, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(PRUNED, PRUNED, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(PRUNED, PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(PRUNED, PRUNED, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(PRUNED, PRUNED, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(PRUNED, PRUNED, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(PRUNED, PRUNED, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(PRUNED, PRUNED, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
    CheckModifyCoins(PRUNED, VALUE2, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(PRUNED, VALUE2, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(PRUNED, VALUE2, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(PRUNED, VALUE2, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(PRUNED, VALUE2, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(PRUNED, VALUE2, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(PRUNED, VALUE2, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(PRUNED, VALUE2, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
    CheckModifyCoins(VALUE1, ABSENT, PRUNED, PRUNED, NO_ENTRY   , DIRTY      );
    CheckModifyCoins(VALUE1, ABSENT, VALUE3, VALUE3, NO_ENTRY   , DIRTY      );
    CheckModifyCoins(VALUE1, PRUNED, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(VALUE1, PRUNED, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(VALUE1, PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(VALUE1, PRUNED, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(VALUE1, PRUNED, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(VALUE1, PRUNED, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(VALUE1, PRUNED, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(VALUE1, PRUNED, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
    CheckModifyCoins(VALUE1, VALUE2, PRUNED, PRUNED, 0          , DIRTY      );
    CheckModifyCoins(VALUE1, VALUE2, PRUNED, ABSENT, FRESH      , NO_ENTRY   );
    CheckModifyCoins(VALUE1, VALUE2, PRUNED, PRUNED, DIRTY      , DIRTY      );
    CheckModifyCoins(VALUE1, VALUE2, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   );
    CheckModifyCoins(VALUE1, VALUE2, VALUE3, VALUE3, 0          , DIRTY      );
    CheckModifyCoins(VALUE1, VALUE2, VALUE3, VALUE3, FRESH      , DIRTY|FRESH);
    CheckModifyCoins(VALUE1, VALUE2, VALUE3, VALUE3, DIRTY      , DIRTY      );
    CheckModifyCoins(VALUE1, VALUE2, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH);
}

void CheckModifyNewCoinsBase(CAmount base_value, CAmount cache_value, CAmount modify_value, CAmount expected_value, char cache_flags, char expected_flags, bool coinbase)
{
    SingleEntryCacheTest test(base_value, cache_value, cache_flags);
    SetCoinsValue(modify_value, *test.cache.ModifyNewCoins(TXID, coinbase));

    CAmount result_value;
    char result_flags;
    GetCoinsMapEntry(test.cache.map(), result_value, result_flags);
    BOOST_CHECK_EQUAL(result_value, expected_value);
    BOOST_CHECK_EQUAL(result_flags, expected_flags);
}

// Simple wrapper for CheckModifyNewCoinsBase function above that loops through
// different possible base_values, making sure each one gives the same results.
// This wrapper lets the modify_new test below be shorter and less repetitive,
// while still verifying that the CoinsViewCache::ModifyNewCoins implementation
// ignores base values.
template <typename... Args>
void CheckModifyNewCoins(Args&&... args)
{
    for (CAmount base_value : {ABSENT, PRUNED, VALUE1})
        CheckModifyNewCoinsBase(base_value, std::forward<Args>(args)...);
}

BOOST_AUTO_TEST_CASE(ccoins_modify_new)
{
    /* Check ModifyNewCoin behavior, requesting a new coin from a cache view,
     * writing a modification to the coin, and then checking the resulting
     * entry in the cache after the modification. Verify behavior with the
     * with the ModifyNewCoin coinbase argument set to false, and to true.
     *
     *                  Cache   Write   Result  Cache        Result     Coinbase
     *                  Value   Value   Value   Flags        Flags
     */
    CheckModifyNewCoins(ABSENT, PRUNED, ABSENT, NO_ENTRY   , NO_ENTRY   , false);
    CheckModifyNewCoins(ABSENT, PRUNED, PRUNED, NO_ENTRY   , DIRTY      , true );
    CheckModifyNewCoins(ABSENT, VALUE3, VALUE3, NO_ENTRY   , DIRTY|FRESH, false);
    CheckModifyNewCoins(ABSENT, VALUE3, VALUE3, NO_ENTRY   , DIRTY      , true );
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, 0          , NO_ENTRY   , false);
    CheckModifyNewCoins(PRUNED, PRUNED, PRUNED, 0          , DIRTY      , true );
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, FRESH      , NO_ENTRY   , false);
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, FRESH      , NO_ENTRY   , true );
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, DIRTY      , NO_ENTRY   , false);
    CheckModifyNewCoins(PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      , true );
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   , false);
    CheckModifyNewCoins(PRUNED, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   , true );
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, 0          , DIRTY|FRESH, false);
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, 0          , DIRTY      , true );
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, FRESH      , DIRTY|FRESH, false);
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, FRESH      , DIRTY|FRESH, true );
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, DIRTY      , DIRTY|FRESH, false);
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, DIRTY      , DIRTY      , true );
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH, false);
    CheckModifyNewCoins(PRUNED, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH, true );
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, 0          , NO_ENTRY   , false);
    CheckModifyNewCoins(VALUE2, PRUNED, PRUNED, 0          , DIRTY      , true );
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, FRESH      , NO_ENTRY   , false);
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, FRESH      , NO_ENTRY   , true );
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, DIRTY      , NO_ENTRY   , false);
    CheckModifyNewCoins(VALUE2, PRUNED, PRUNED, DIRTY      , DIRTY      , true );
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   , false);
    CheckModifyNewCoins(VALUE2, PRUNED, ABSENT, DIRTY|FRESH, NO_ENTRY   , true );
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, 0          , DIRTY|FRESH, false);
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, 0          , DIRTY      , true );
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, FRESH      , DIRTY|FRESH, false);
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, FRESH      , DIRTY|FRESH, true );
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, DIRTY      , DIRTY|FRESH, false);
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, DIRTY      , DIRTY      , true );
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH, false);
    CheckModifyNewCoins(VALUE2, VALUE3, VALUE3, DIRTY|FRESH, DIRTY|FRESH, true );
}

void CheckWriteCoins(CAmount parent_value, CAmount child_value, CAmount expected_value, char parent_flags, char child_flags, char expected_flags)
{
    SingleEntryCacheTest test(ABSENT, parent_value, parent_flags);
    WriteCoinsViewEntry(test.cache, child_value, child_flags);
    test.cache.SelfTest();

    CAmount result_value;
    char result_flags;
    GetCoinsMapEntry(test.cache.map(), result_value, result_flags);
    BOOST_CHECK_EQUAL(result_value, expected_value);
    BOOST_CHECK_EQUAL(result_flags, expected_flags);
}

BOOST_AUTO_TEST_CASE(ccoins_write)
{
    /* Check BatchWrite behavior, flushing one entry from a child cache to a
     * parent cache, and checking the resulting entry in the parent cache
     * after the write.
     *
     *              Parent  Child   Result  Parent       Child        Result
     *              Value   Value   Value   Flags        Flags        Flags
     */
    CheckWriteCoins(ABSENT, ABSENT, ABSENT, NO_ENTRY   , NO_ENTRY   , NO_ENTRY   );
    CheckWriteCoins(ABSENT, PRUNED, PRUNED, NO_ENTRY   , DIRTY      , DIRTY      );
    CheckWriteCoins(ABSENT, PRUNED, ABSENT, NO_ENTRY   , DIRTY|FRESH, NO_ENTRY   );
    CheckWriteCoins(ABSENT, VALUE2, VALUE2, NO_ENTRY   , DIRTY      , DIRTY      );
    CheckWriteCoins(ABSENT, VALUE2, VALUE2, NO_ENTRY   , DIRTY|FRESH, DIRTY|FRESH);
    CheckWriteCoins(PRUNED, ABSENT, PRUNED, 0          , NO_ENTRY   , 0          );
    CheckWriteCoins(PRUNED, ABSENT, PRUNED, FRESH      , NO_ENTRY   , FRESH      );
    CheckWriteCoins(PRUNED, ABSENT, PRUNED, DIRTY      , NO_ENTRY   , DIRTY      );
    CheckWriteCoins(PRUNED, ABSENT, PRUNED, DIRTY|FRESH, NO_ENTRY   , DIRTY|FRESH);
    CheckWriteCoins(PRUNED, PRUNED, PRUNED, 0          , DIRTY      , DIRTY      );
    CheckWriteCoins(PRUNED, PRUNED, PRUNED, 0          , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(PRUNED, PRUNED, ABSENT, FRESH      , DIRTY      , NO_ENTRY   );
    CheckWriteCoins(PRUNED, PRUNED, ABSENT, FRESH      , DIRTY|FRESH, NO_ENTRY   );
    CheckWriteCoins(PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY      , DIRTY      );
    CheckWriteCoins(PRUNED, PRUNED, PRUNED, DIRTY      , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(PRUNED, PRUNED, ABSENT, DIRTY|FRESH, DIRTY      , NO_ENTRY   );
    CheckWriteCoins(PRUNED, PRUNED, ABSENT, DIRTY|FRESH, DIRTY|FRESH, NO_ENTRY   );
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, 0          , DIRTY      , DIRTY      );
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, 0          , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, FRESH      , DIRTY      , DIRTY|FRESH);
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, FRESH      , DIRTY|FRESH, DIRTY|FRESH);
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, DIRTY      , DIRTY      , DIRTY      );
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, DIRTY      , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, DIRTY|FRESH, DIRTY      , DIRTY|FRESH);
    CheckWriteCoins(PRUNED, VALUE2, VALUE2, DIRTY|FRESH, DIRTY|FRESH, DIRTY|FRESH);
    CheckWriteCoins(VALUE1, ABSENT, VALUE1, 0          , NO_ENTRY   , 0          );
    CheckWriteCoins(VALUE1, ABSENT, VALUE1, FRESH      , NO_ENTRY   , FRESH      );
    CheckWriteCoins(VALUE1, ABSENT, VALUE1, DIRTY      , NO_ENTRY   , DIRTY      );
    CheckWriteCoins(VALUE1, ABSENT, VALUE1, DIRTY|FRESH, NO_ENTRY   , DIRTY|FRESH);
    CheckWriteCoins(VALUE1, PRUNED, PRUNED, 0          , DIRTY      , DIRTY      );
    CheckWriteCoins(VALUE1, PRUNED, PRUNED, 0          , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(VALUE1, PRUNED, ABSENT, FRESH      , DIRTY      , NO_ENTRY   );
    CheckWriteCoins(VALUE1, PRUNED, ABSENT, FRESH      , DIRTY|FRESH, NO_ENTRY   );
    CheckWriteCoins(VALUE1, PRUNED, PRUNED, DIRTY      , DIRTY      , DIRTY      );
    CheckWriteCoins(VALUE1, PRUNED, PRUNED, DIRTY      , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(VALUE1, PRUNED, ABSENT, DIRTY|FRESH, DIRTY      , NO_ENTRY   );
    CheckWriteCoins(VALUE1, PRUNED, ABSENT, DIRTY|FRESH, DIRTY|FRESH, NO_ENTRY   );
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, 0          , DIRTY      , DIRTY      );
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, 0          , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, FRESH      , DIRTY      , DIRTY|FRESH);
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, FRESH      , DIRTY|FRESH, DIRTY|FRESH);
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, DIRTY      , DIRTY      , DIRTY      );
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, DIRTY      , DIRTY|FRESH, DIRTY      );
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, DIRTY|FRESH, DIRTY      , DIRTY|FRESH);
    CheckWriteCoins(VALUE1, VALUE2, VALUE2, DIRTY|FRESH, DIRTY|FRESH, DIRTY|FRESH);

    // The checks above omit cases where the child flags are not DIRTY, since
    // they would be too repetitive (the parent cache is never updated in these
    // cases). The loop below covers these cases and makes sure the parent cache
    // is always left unchanged.
    for (CAmount parent_value : {ABSENT, PRUNED, VALUE1})
        for (CAmount child_value : {ABSENT, PRUNED, VALUE2})
            for (char parent_flags : parent_value == ABSENT ? ABSENT_FLAGS : FLAGS)
                for (char child_flags : child_value == ABSENT ? ABSENT_FLAGS : CLEAN_FLAGS)
                    CheckWriteCoins(parent_value, child_value, parent_value, parent_flags, child_flags, parent_flags);
}

BOOST_AUTO_TEST_SUITE_END()
