// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"
#include "random.h"
#include "uint256.h"
#include "test/test_bitcoin.h"
#include "main.h"
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

    bool GetStats(CCoinsStats& stats) const { return false; }
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

            CValidationState dummy;
            UpdateCoins(tx, dummy, *(stack.back()), height);
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

BOOST_AUTO_TEST_SUITE_END()
