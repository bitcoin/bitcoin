// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <iostream>
#include <memory>
#include <set>
#include <stdint.h>
#include <utility>
#include <vector>

#include <consensus/validation.h>
#include <key_io.h>
#include <rpc/server.h>
#include <test/test_dash.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>
#include <univalue.h>

extern UniValue importmulti(const JSONRPCRequest& request);
extern UniValue dumpwallet(const JSONRPCRequest& request);
extern UniValue importwallet(const JSONRPCRequest& request);
extern UniValue getnewaddress(const JSONRPCRequest& request);

BOOST_FIXTURE_TEST_SUITE(wallet_tests, WalletTestingSetup)

static void AddKey(CWallet& wallet, const CKey& key)
{
    LOCK(wallet.cs_wallet);
    wallet.AddKeyPubKey(key, key.GetPubKey());
}

BOOST_FIXTURE_TEST_CASE(rescan, TestChain100Setup)
{
    // Cap last block file size, and mine new block in a new block file.
    CBlockIndex* const nullBlock = nullptr;
    CBlockIndex* oldTip = chainActive.Tip();
    GetBlockFileInfo(oldTip->GetBlockPos().nFile)->nSize = MAX_BLOCKFILE_SIZE;
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    CBlockIndex* newTip = chainActive.Tip();

    LOCK(cs_main);

    // Verify ScanForWalletTransactions picks up transactions in both the old
    // and new block files.
    {
        CWallet wallet(WalletLocation(), WalletDatabase::CreateDummy());
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        BOOST_CHECK_EQUAL(nullBlock, wallet.ScanForWalletTransactions(oldTip, nullptr, reserver));
        BOOST_CHECK_EQUAL(wallet.GetImmatureBalance(), 1000 * COIN);
    }

    // Prune the older block file.
    PruneOneBlockFile(oldTip->GetBlockPos().nFile);
    UnlinkPrunedFiles({oldTip->GetBlockPos().nFile});

    // Verify ScanForWalletTransactions only picks transactions in the new block
    // file.
    {
        CWallet wallet(WalletLocation(), WalletDatabase::CreateDummy());
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        BOOST_CHECK_EQUAL(oldTip, wallet.ScanForWalletTransactions(oldTip, nullptr, reserver));
        BOOST_CHECK_EQUAL(wallet.GetImmatureBalance(), 500 * COIN);
    }

    // Verify importmulti RPC returns failure for a key whose creation time is
    // before the missing block, and success for a key whose creation time is
    // after.
    {
        std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(WalletLocation(), WalletDatabase::CreateDummy());
        AddWallet(wallet);
        UniValue keys;
        keys.setArray();
        UniValue key;
        key.setObject();
        key.pushKV("scriptPubKey", HexStr(GetScriptForRawPubKey(coinbaseKey.GetPubKey())));
        key.pushKV("timestamp", 0);
        key.pushKV("internal", UniValue(true));
        keys.push_back(key);
        key.clear();
        key.setObject();
        CKey futureKey;
        futureKey.MakeNewKey(true);
        key.pushKV("scriptPubKey", HexStr(GetScriptForRawPubKey(futureKey.GetPubKey())));
        key.pushKV("timestamp", newTip->GetBlockTimeMax() + TIMESTAMP_WINDOW + 1);
        key.pushKV("internal", UniValue(true));
        keys.push_back(key);
        JSONRPCRequest request;
        request.params.setArray();
        request.params.push_back(keys);

        UniValue response = importmulti(request);
        BOOST_CHECK_EQUAL(response.write(),
            strprintf("[{\"success\":false,\"error\":{\"code\":-1,\"message\":\"Rescan failed for key with creation "
                      "timestamp %d. There was an error reading a block from time %d, which is after or within %d "
                      "seconds of key creation, and could contain transactions pertaining to the key. As a result, "
                      "transactions and coins using this key may not appear in the wallet. This error could be caused "
                      "by pruning or data corruption (see dashd log for details) and could be dealt with by "
                      "downloading and rescanning the relevant blocks (see -reindex and -rescan "
                      "options).\"}},{\"success\":true}]",
                              0, oldTip->GetBlockTimeMax(), TIMESTAMP_WINDOW));
        RemoveWallet(wallet);
    }
}

// Verify importwallet RPC starts rescan at earliest block with timestamp
// greater or equal than key birthday. Previously there was a bug where
// importwallet RPC would start the scan at the latest block with timestamp less
// than or equal to key birthday.
BOOST_FIXTURE_TEST_CASE(importwallet_rescan, TestChain100Setup)
{
    // Create two blocks with same timestamp to verify that importwallet rescan
    // will pick up both blocks, not just the first.
    const int64_t BLOCK_TIME = chainActive.Tip()->GetBlockTimeMax() + 5;
    SetMockTime(BLOCK_TIME);
    m_coinbase_txns.emplace_back(CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey())).vtx[0]);
    m_coinbase_txns.emplace_back(CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey())).vtx[0]);

    // Set key birthday to block time increased by the timestamp window, so
    // rescan will start at the block time.
    const int64_t KEY_TIME = BLOCK_TIME + 7200;
    SetMockTime(KEY_TIME);
    m_coinbase_txns.emplace_back(CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey())).vtx[0]);

    LOCK(cs_main);

    std::string backup_file = (SetDataDir("importwallet_rescan") / "wallet.backup").string();

    // Import key into wallet and call dumpwallet to create backup file.
    {
        std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(WalletLocation(), WalletDatabase::CreateDummy());
        LOCK(wallet->cs_wallet);
        wallet->mapKeyMetadata[coinbaseKey.GetPubKey().GetID()].nCreateTime = KEY_TIME;
        wallet->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());

        JSONRPCRequest request;
        request.params.setArray();
        request.params.push_back(backup_file);
        AddWallet(wallet);
        ::dumpwallet(request);
        RemoveWallet(wallet);
    }

    // Call importwallet RPC and verify all blocks with timestamps >= BLOCK_TIME
    // were scanned, and no prior blocks were scanned.
    {
        std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(WalletLocation(), WalletDatabase::CreateDummy());

        JSONRPCRequest request;
        request.params.setArray();
        request.params.push_back(backup_file);
        AddWallet(wallet);
        ::importwallet(request);
        RemoveWallet(wallet);

        LOCK(wallet->cs_wallet);
        BOOST_CHECK_EQUAL(wallet->mapWallet.size(), 3U);
        BOOST_CHECK_EQUAL(m_coinbase_txns.size(), 103U);
        for (size_t i = 0; i < m_coinbase_txns.size(); ++i) {
            bool found = wallet->GetWalletTx(m_coinbase_txns[i]->GetHash());
            bool expected = i >= 100;
            BOOST_CHECK_EQUAL(found, expected);
        }
    }

    SetMockTime(0);
}

// Check that GetImmatureCredit() returns a newly calculated value instead of
// the cached value after a MarkDirty() call.
//
// This is a regression test written to verify a bugfix for the immature credit
// function. Similar tests probably should be written for the other credit and
// debit functions.
BOOST_FIXTURE_TEST_CASE(coin_mark_dirty_immature_credit, TestChain100Setup)
{
    CWallet wallet(WalletLocation(), WalletDatabase::CreateDummy());
    CWalletTx wtx(&wallet, m_coinbase_txns.back());
    LOCK2(cs_main, wallet.cs_wallet);
    wtx.hashBlock = chainActive.Tip()->GetBlockHash();
    wtx.nIndex = 0;

    // Call GetImmatureCredit() once before adding the key to the wallet to
    // cache the current immature credit amount, which is 0.
    BOOST_CHECK_EQUAL(wtx.GetImmatureCredit(), 0);

    // Invalidate the cached value, add the key, and make sure a new immature
    // credit amount is calculated.
    wtx.MarkDirty();
    wallet.AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());
    BOOST_CHECK_EQUAL(wtx.GetImmatureCredit(), 500*COIN);
}

static int64_t AddTx(CWallet& wallet, uint32_t lockTime, int64_t mockTime, int64_t blockTime)
{
    CMutableTransaction tx;
    tx.nLockTime = lockTime;
    SetMockTime(mockTime);
    CBlockIndex* block = nullptr;
    if (blockTime > 0) {
        LOCK(cs_main);
        auto inserted = mapBlockIndex.emplace(GetRandHash(), new CBlockIndex);
        assert(inserted.second);
        const uint256& hash = inserted.first->first;
        block = inserted.first->second;
        block->nTime = blockTime;
        block->phashBlock = &hash;
    }

    CWalletTx wtx(&wallet, MakeTransactionRef(tx));
    if (block) {
        wtx.SetMerkleBranch(block, 0);
    }
    {
        LOCK(cs_main);
        wallet.AddToWallet(wtx);
    }
    LOCK(wallet.cs_wallet);
    return wallet.mapWallet.at(wtx.GetHash()).nTimeSmart;
}

// Simple test to verify assignment of CWalletTx::nSmartTime value. Could be
// expanded to cover more corner cases of smart time logic.
BOOST_AUTO_TEST_CASE(ComputeTimeSmart)
{
    // New transaction should use clock time if lower than block time.
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 1, 100, 120), 100);

    // Test that updating existing transaction does not change smart time.
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 1, 200, 220), 100);

    // New transaction should use clock time if there's no block time.
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 2, 300, 0), 300);

    // New transaction should use block time if lower than clock time.
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 3, 420, 400), 400);

    // New transaction should use latest entry time if higher than
    // min(block time, clock time).
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 4, 500, 390), 400);

    // If there are future entries, new transaction should use time of the
    // newest entry that is no more than 300 seconds ahead of the clock time.
    BOOST_CHECK_EQUAL(AddTx(m_wallet, 5, 50, 600), 300);

    // Reset mock time for other tests.
    SetMockTime(0);
}

BOOST_AUTO_TEST_CASE(LoadReceiveRequests)
{
    CTxDestination dest = CKeyID();
    LOCK(m_wallet.cs_wallet);
    m_wallet.AddDestData(dest, "misc", "val_misc");
    m_wallet.AddDestData(dest, "rr0", "val_rr0");
    m_wallet.AddDestData(dest, "rr1", "val_rr1");

    auto values = m_wallet.GetDestValues("rr");
    BOOST_CHECK_EQUAL(values.size(), 2U);
    BOOST_CHECK_EQUAL(values[0], "val_rr0");
    BOOST_CHECK_EQUAL(values[1], "val_rr1");
}

class ListCoinsTestingSetup : public TestChain100Setup
{
public:
    ListCoinsTestingSetup()
    {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        wallet = MakeUnique<CWallet>(WalletLocation(), WalletDatabase::CreateMock());
        bool firstRun;
        wallet->LoadWallet(firstRun);
        AddKey(*wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet.get());
        reserver.reserve();
        wallet->ScanForWalletTransactions(chainActive.Genesis(), nullptr, reserver);
    }

    ~ListCoinsTestingSetup()
    {
        wallet.reset();
    }

    CWalletTx& AddTx(CRecipient recipient)
    {
        CTransactionRef tx;
        CReserveKey reservekey(wallet.get());
        CAmount fee;
        int changePos = -1;
        std::string error;
        CCoinControl dummy;
        BOOST_CHECK(wallet->CreateTransaction({recipient}, tx, reservekey, fee, changePos, error, dummy));
        CValidationState state;
        BOOST_CHECK(wallet->CommitTransaction(tx, {}, {}, {}, reservekey, nullptr, state));
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            blocktx = CMutableTransaction(*wallet->mapWallet.at(tx->GetHash()).tx);
        }
        CreateAndProcessBlock({CMutableTransaction(blocktx)}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK(cs_main);
        LOCK(wallet->cs_wallet);
        auto it = wallet->mapWallet.find(tx->GetHash());
        BOOST_CHECK(it != wallet->mapWallet.end());
        it->second.SetMerkleBranch(chainActive.Tip(), 1);
        return it->second;
    }

    std::unique_ptr<CWallet> wallet;
};

BOOST_FIXTURE_TEST_CASE(ListCoins, ListCoinsTestingSetup)
{
    std::string coinbaseAddress = coinbaseKey.GetPubKey().GetID().ToString();

    // Confirm ListCoins initially returns 1 coin grouped under coinbaseKey
    // address.
    auto list = wallet->ListCoins();
    BOOST_CHECK_EQUAL(list.size(), 1U);
    BOOST_CHECK_EQUAL(boost::get<CKeyID>(list.begin()->first).ToString(), coinbaseAddress);
    BOOST_CHECK_EQUAL(list.begin()->second.size(), 1U);

    // Check initial balance from one mature coinbase transaction.
    BOOST_CHECK_EQUAL(500 * COIN, wallet->GetAvailableBalance());

    // Add a transaction creating a change address, and confirm ListCoins still
    // returns the coin associated with the change address underneath the
    // coinbaseKey pubkey, even though the change address has a different
    // pubkey.
    AddTx(CRecipient{GetScriptForRawPubKey({}), 1 * COIN, false /* subtract fee */});
    list = wallet->ListCoins();
    BOOST_CHECK_EQUAL(list.size(), 1U);
    BOOST_CHECK_EQUAL(boost::get<CKeyID>(list.begin()->first).ToString(), coinbaseAddress);
    BOOST_CHECK_EQUAL(list.begin()->second.size(), 2U);

    // Lock both coins. Confirm number of available coins drops to 0.
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::vector<COutput> available;
        wallet->AvailableCoins(available);
        BOOST_CHECK_EQUAL(available.size(), 2U);
    }
    for (const auto& group : list) {
        for (const auto& coin : group.second) {
            LOCK(wallet->cs_wallet);
            wallet->LockCoin(COutPoint(coin.tx->GetHash(), coin.i));
        }
    }
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::vector<COutput> available;
        wallet->AvailableCoins(available);
        BOOST_CHECK_EQUAL(available.size(), 0U);
    }
    // Confirm ListCoins still returns same result as before, despite coins
    // being locked.
    list = wallet->ListCoins();
    BOOST_CHECK_EQUAL(list.size(), 1U);
    BOOST_CHECK_EQUAL(boost::get<CKeyID>(list.begin()->first).ToString(), coinbaseAddress);
    BOOST_CHECK_EQUAL(list.begin()->second.size(), 2U);
}

class CreateTransactionTestSetup : public TestChain100Setup
{
public:
    enum ChangeTest {
        Skip,
        NoChangeExpected,
        ChangeExpected,
    };

    // Result strings to test
    const std::string strInsufficientFunds = "Insufficient funds.";
    const std::string strAmountNotNegative = "Transaction amounts must not be negative";
    const std::string strAtLeastOneRecipient = "Transaction must have at least one recipient";
    const std::string strTooSmallToPayFee = "The transaction amount is too small to pay the fee";
    const std::string strTooSmallAfterFee = "The transaction amount is too small to send after the fee has been deducted";
    const std::string strTooSmall = "Transaction amount too small";
    const std::string strUnableToLocateCoinJoin1 = "Unable to locate enough non-denominated funds for this transaction.";
    const std::string strUnableToLocateCoinJoin2 = "Unable to locate enough mixed funds for this transaction. CoinJoin uses exact denominated amounts to send funds, you might simply need to mix some more coins.";
    const std::string strTransactionTooLarge = "Transaction too large";
    const std::string strTransactionTooLargeForFeePolicy = "Transaction too large for fee policy";
    const std::string strChangeIndexOutOfRange = "Change index out of range";
    const std::string strExceededMaxTries = "Exceeded max tries.";

    CreateTransactionTestSetup()
    {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        wallet = MakeUnique<CWallet>(WalletLocation(), WalletDatabase::CreateMock());
        bool firstRun;
        wallet->LoadWallet(firstRun);
        AddWallet(wallet);
        AddKey(*wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet.get());
        reserver.reserve();
        wallet->ScanForWalletTransactions(chainActive.Genesis(), nullptr, reserver);
    }

    ~CreateTransactionTestSetup()
    {
        RemoveWallet(wallet);
    }

    std::shared_ptr<CWallet> wallet;
    CCoinControl coinControl;

    template <typename T>
    bool CheckEqual(const T expected, const T actual)
    {
        BOOST_CHECK_EQUAL(expected, actual);
        return expected == actual;
    }

    bool CreateTransaction(const std::vector<std::pair<CAmount, bool>>& vecEntries, bool fCreateShouldSucceed = true, ChangeTest changeTest = ChangeTest::Skip)
    {
        return CreateTransaction(vecEntries, {}, -1, fCreateShouldSucceed, changeTest);
    }
    bool CreateTransaction(const std::vector<std::pair<CAmount, bool>>& vecEntries, std::string strErrorExpected, bool fCreateShouldSucceed = true, ChangeTest changeTest = ChangeTest::Skip)
    {
        return CreateTransaction(vecEntries, strErrorExpected, -1, fCreateShouldSucceed, changeTest);
    }

    bool CreateTransaction(const std::vector<std::pair<CAmount, bool>>& vecEntries, std::string strErrorExpected, int nChangePosRequest = -1, bool fCreateShouldSucceed = true, ChangeTest changeTest = ChangeTest::Skip)
    {
        CTransactionRef tx;
        CReserveKey reservekey(wallet.get());
        CAmount nFeeRet;
        int nChangePos = nChangePosRequest;
        std::string strError;

        bool fCreationSucceeded = wallet->CreateTransaction(GetRecipients(vecEntries), tx, reservekey, nFeeRet, nChangePos, strError, coinControl);
        bool fHitMaxTries = strError == strExceededMaxTries;
        // This should never happen.
        if (fHitMaxTries) {
            BOOST_CHECK(!fHitMaxTries);
            return false;
        }
        // Verify the creation succeeds if expected and fails if not.
        if (!CheckEqual(fCreateShouldSucceed, fCreationSucceeded)) {
            return false;
        }
        //  Verify the expected error string if there is one provided
        if (strErrorExpected.size() && !CheckEqual(strErrorExpected, strError)) {
            return false;
        }
        if (!fCreateShouldSucceed) {
            // No need to evaluate the following if the creation should have failed.
            return true;
        }
        // Verify there is no change output if there wasn't any expected
        bool fChangeTestPassed = changeTest == ChangeTest::Skip ||
                                 (changeTest == ChangeTest::ChangeExpected && nChangePos != -1) ||
                                 (changeTest == ChangeTest::NoChangeExpected && nChangePos == -1);
        BOOST_CHECK(fChangeTestPassed);
        if (!fChangeTestPassed) {
            return false;
        }
        // Verify the change is at the requested position if there was a request
        if (nChangePosRequest != -1 && !CheckEqual(nChangePosRequest, nChangePos)) {
            return false;
        }
        // Verify the number of requested outputs does match the resulting outputs
        return CheckEqual(vecEntries.size(), tx->vout.size() - (nChangePos != -1 ? 1 : 0));
    }

    std::vector<CRecipient> GetRecipients(const std::vector<std::pair<CAmount, bool>>& vecEntries)
    {
        std::vector<CRecipient> vecRecipients;
        for (auto entry : vecEntries) {
            vecRecipients.push_back({GetScriptForDestination(DecodeDestination(getnewaddress(JSONRPCRequest()).get_str())), entry.first, entry.second});
        }
        return vecRecipients;
    }

    std::vector<COutPoint> GetCoins(const std::vector<std::pair<CAmount, bool>>& vecEntries)
    {
        CTransactionRef tx;
        CReserveKey reserveKey(wallet.get());
        CAmount nFeeRet;
        int nChangePosRet = -1;
        std::string strError;
        CCoinControl coinControl;
        BOOST_CHECK(wallet->CreateTransaction(GetRecipients(vecEntries), tx, reserveKey, nFeeRet, nChangePosRet, strError, coinControl));
        CValidationState state;
        BOOST_CHECK(wallet->CommitTransaction(tx, {}, {}, {}, reserveKey, nullptr, state));
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            blocktx = CMutableTransaction(*tx);
        }
        CreateAndProcessBlock({CMutableTransaction(blocktx)}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK2(cs_main, wallet->cs_wallet);
        auto it = wallet->mapWallet.find(tx->GetHash());
        BOOST_CHECK(it != wallet->mapWallet.end());
        it->second.SetMerkleBranch(chainActive.Tip(), 1);

        std::vector<COutPoint> vecOutpoints;
        size_t n;
        for (n = 0; n < tx->vout.size(); ++n) {
            if (nChangePosRet != -1 && n == nChangePosRet) {
                // Skip the change output to only return the requested coins
                continue;
            }
            vecOutpoints.push_back(COutPoint(tx->GetHash(), n));
        }
        assert(vecOutpoints.size() == vecEntries.size());
        return vecOutpoints;
    }
};

BOOST_FIXTURE_TEST_CASE(CreateTransactionTest, CreateTransactionTestSetup)
{
    minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

    auto runTest = [&](const int nTestId, const CAmount nFeeRate, const std::map<int, std::pair<bool, ChangeTest>>& mapExpected) {
        coinControl.m_feerate = CFeeRate(nFeeRate);
        const std::map<int, std::vector<std::pair<CAmount, bool>>> mapTestCases{
            {0, {{1000, false}}},
            {1, {{1000, true}}},
            {2, {{10000, false}}},
            {3, {{10000, true}}},
            {4, {{34000, false}, {40000, false}}},
            {5, {{37000, false}, {40000, false}}},
            {6, {{50000, false}, {50000, false}}},
            {7, {{50000, true}, {50000, false}}},
            {8, {{50000, false}, {50001, false}}},
            {9, {{50000, true}, {50001, true}}},
            {10, {{100000, false}}},
            {11, {{100000, true}}},
            {12, {{100001, false}}},
            {13, {{100001, true}}}
        };
        assert(mapTestCases.size() == mapExpected.size());
        for (int i = 0; i < mapTestCases.size(); ++i) {
            if (!CreateTransaction(mapTestCases.at(i), mapExpected.at(i).first, mapExpected.at(i).second)) {
                std::cout << strprintf("CreateTransactionTest failed at: %d - %d\n", nTestId, i) << std::endl;
            }
        }
    };

    // First run the tests with only one input containing 100k duffs
    {
        coinControl.SetNull();
        coinControl.Select(GetCoins({{100000, false}})[0]);

        // Start with fallback feerate
        runTest(1, DEFAULT_FALLBACK_FEE, {
            {0, {true, ChangeTest::ChangeExpected}},
            {1, {true, ChangeTest::ChangeExpected}},
            {2, {true, ChangeTest::ChangeExpected}},
            {3, {true, ChangeTest::ChangeExpected}},
            {4, {true, ChangeTest::ChangeExpected}},
            {5, {true, ChangeTest::ChangeExpected}},
            {6, {false, ChangeTest::Skip}},
            {7, {true, ChangeTest::NoChangeExpected}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {true, ChangeTest::NoChangeExpected}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
        // Now with 100x fallback feerate
        runTest(2, DEFAULT_FALLBACK_FEE * 100, {
            {0, {true, ChangeTest::ChangeExpected}},
            {1, {false, ChangeTest::Skip}},
            {2, {true, ChangeTest::ChangeExpected}},
            {3, {false, ChangeTest::Skip}},
            {4, {true, ChangeTest::NoChangeExpected}},
            {5, {false, ChangeTest::Skip}},
            {6, {false, ChangeTest::Skip}},
            {7, {true, ChangeTest::NoChangeExpected}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {true, ChangeTest::NoChangeExpected}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
    }
    // Now use 4 different inputs with a total of 100k duff
    {
        coinControl.SetNull();
        auto setCoins = GetCoins({{1000, false}, {5000, false}, {10000, false}, {84000, false}});
        for (auto coin : setCoins) {
            coinControl.Select(coin);
        }

        // Start with fallback feerate
        runTest(3, DEFAULT_FALLBACK_FEE, {
            {0, {true, ChangeTest::ChangeExpected}},
            {1, {false, ChangeTest::Skip}},
            {2, {true, ChangeTest::ChangeExpected}},
            {3, {true, ChangeTest::ChangeExpected}},
            {4, {true, ChangeTest::ChangeExpected}},
            {5, {true, ChangeTest::ChangeExpected}},
            {6, {false, ChangeTest::Skip}},
            {7, {true, ChangeTest::NoChangeExpected}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {true, ChangeTest::NoChangeExpected}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
        // Now with 100x fallback feerate
        runTest(4, DEFAULT_FALLBACK_FEE * 100, {
            {0, {true, ChangeTest::ChangeExpected}},
            {1, {false, ChangeTest::Skip}},
            {2, {true, ChangeTest::ChangeExpected}},
            {3, {false, ChangeTest::Skip}},
            {4, {false, ChangeTest::Skip}},
            {5, {false, ChangeTest::Skip}},
            {6, {false, ChangeTest::Skip}},
            {7, {false, ChangeTest::Skip}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {true, ChangeTest::NoChangeExpected}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
    }

    // Last use 10 equal inputs with a total of 100k duff
    {
        coinControl.SetNull();
        auto setCoins = GetCoins({{10000, false}, {10000, false}, {10000, false}, {10000, false}, {10000, false},
                                  {10000, false}, {10000, false}, {10000, false}, {10000, false}, {10000, false}});

        for (auto coin : setCoins) {
            coinControl.Select(coin);
        }

        // Start with fallback feerate
        runTest(5, DEFAULT_FALLBACK_FEE, {
            {0, {true, ChangeTest::ChangeExpected}},
            {1, {false, ChangeTest::Skip}},
            {2, {true, ChangeTest::ChangeExpected}},
            {3, {true, ChangeTest::ChangeExpected}},
            {4, {true, ChangeTest::ChangeExpected}},
            {5, {true, ChangeTest::ChangeExpected}},
            {6, {false, ChangeTest::Skip}},
            {7, {true, ChangeTest::NoChangeExpected}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {true, ChangeTest::NoChangeExpected}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
        // Now with 100x fallback feerate
        runTest(6, DEFAULT_FALLBACK_FEE * 100, {
            {0, {false, ChangeTest::Skip}},
            {1, {false, ChangeTest::Skip}},
            {2, {false, ChangeTest::Skip}},
            {3, {false, ChangeTest::Skip}},
            {4, {false, ChangeTest::Skip}},
            {5, {false, ChangeTest::Skip}},
            {6, {false, ChangeTest::Skip}},
            {7, {false, ChangeTest::Skip}},
            {8, {false, ChangeTest::Skip}},
            {9, {false, ChangeTest::Skip}},
            {10, {false, ChangeTest::Skip}},
            {11, {false, ChangeTest::Skip}},
            {12, {false, ChangeTest::Skip}},
            {13, {false, ChangeTest::Skip}}
        });
    }
    // Some tests without selected coins in coinControl, let the wallet decide
    // which inputs to use
    {
        coinControl.SetNull();
        auto setCoins = GetCoins({{1000, false}, {1000, false}, {1000, false}, {1000, false}, {1000, false},
                                  {1100, false}, {1200, false}, {1300, false}, {1400, false}, {1500, false},
                                  {3000, false}, {3000, false}, {2000, false}, {2000, false}, {1000, false}});
        // Lock all other coins which were already in the wallet
        std::vector<COutput> vecAvailable;
        {
            LOCK2(cs_main, wallet->cs_wallet);
            wallet->AvailableCoins(vecAvailable);
            for (auto coin : vecAvailable) {
                auto out = COutPoint(coin.tx->GetHash(), coin.i);
                if (std::find(setCoins.begin(), setCoins.end(), out) == setCoins.end()) {
                    wallet->LockCoin(out);
                }
            }
        }

        BOOST_CHECK(CreateTransaction({{100, false}}, false));
        BOOST_CHECK(CreateTransaction({{1000, true}}, true));
        BOOST_CHECK(CreateTransaction({{1100, false}}, true));
        BOOST_CHECK(CreateTransaction({{1100, true}}, true));
        BOOST_CHECK(CreateTransaction({{2200, false}}, true));
        BOOST_CHECK(CreateTransaction({{3300, false}}, true));
        BOOST_CHECK(CreateTransaction({{4400, false}}, true));
        BOOST_CHECK(CreateTransaction({{5500, false}}, true));
        BOOST_CHECK(CreateTransaction({{5500, true}}, true));
        BOOST_CHECK(CreateTransaction({{6600, false}}, true));
        BOOST_CHECK(CreateTransaction({{7700, false}}, true));
        BOOST_CHECK(CreateTransaction({{8800, false}}, true));
        BOOST_CHECK(CreateTransaction({{9900, false}}, true));
        BOOST_CHECK(CreateTransaction({{9900, true}}, true));
        BOOST_CHECK(CreateTransaction({{10000, false}}, true));
        BOOST_CHECK(CreateTransaction({{10000, false}, {10000, false}}, false));
        BOOST_CHECK(CreateTransaction({{10000, false}, {12500, true}}, true));
        BOOST_CHECK(CreateTransaction({{10000, true}, {10000, true}}, true));
        BOOST_CHECK(CreateTransaction({{1000, false}, {2000, false}, {3000, false}, {4000, false}}, true));
        BOOST_CHECK(CreateTransaction({{1234, false}}, true));
        BOOST_CHECK(CreateTransaction({{1234, false}, {4321, false}}, true));
        BOOST_CHECK(CreateTransaction({{1234, false}, {4321, false}, {5678, false}}, true));
        BOOST_CHECK(CreateTransaction({{1234, false}, {4321, false}, {5678, false}, {8765, false}}, false));
        BOOST_CHECK(CreateTransaction({{1234, false}, {4321, false}, {5678, false}, {8765, true}}, true));
        BOOST_CHECK(CreateTransaction({{1000000, false}}, false));

        LOCK(wallet->cs_wallet);
        wallet->UnlockAllCoins();
    }
    // Test if the change output ends up at the requested position
    {
        coinControl.SetNull();
        coinControl.Select(GetCoins({{100000, false}})[0]);

        BOOST_CHECK(CreateTransaction({{25000, false}, {25000, false}, {25000, false}}, {}, 0, true, ChangeTest::ChangeExpected));
        BOOST_CHECK(CreateTransaction({{25000, false}, {25000, false}, {25000, false}}, {}, 1, true, ChangeTest::ChangeExpected));
        BOOST_CHECK(CreateTransaction({{25000, false}, {25000, false}, {25000, false}}, {}, 2, true, ChangeTest::ChangeExpected));
        BOOST_CHECK(CreateTransaction({{25000, false}, {25000, false}, {25000, false}}, {}, 3, true, ChangeTest::ChangeExpected));
    }
    // Test error cases
    {
        coinControl.SetNull();
        // First try to send something without any coins available
        {
            // Lock all other coins
            std::vector<COutput> vecAvailable;
            {
                LOCK2(cs_main, wallet->cs_wallet);
                wallet->AvailableCoins(vecAvailable);
                for (auto coin : vecAvailable) {
                    wallet->LockCoin(COutPoint(coin.tx->GetHash(), coin.i));
                }
            }

            BOOST_CHECK(CreateTransaction({{1000, false}}, strInsufficientFunds, false));
            BOOST_CHECK(CreateTransaction({{1000, true}}, strInsufficientFunds, false));
            coinControl.nCoinType = CoinType::ONLY_NONDENOMINATED;
            BOOST_CHECK(CreateTransaction({{1000, true}}, strUnableToLocateCoinJoin1, false));
            coinControl.nCoinType = CoinType::ONLY_FULLY_MIXED;
            BOOST_CHECK(CreateTransaction({{1000, true}}, strUnableToLocateCoinJoin2, false));

            LOCK(wallet->cs_wallet);
            wallet->UnlockAllCoins();
        }

        // Just to create nCount output recipes to use in tests below
        std::vector<std::pair<CAmount, bool>> vecOutputEntries{{5000, false}};
        auto createOutputEntries = [&](int nCount) {
            while (vecOutputEntries.size() <= nCount) {
                vecOutputEntries.push_back(vecOutputEntries.back());
            }
            if (vecOutputEntries.size() > nCount) {
                int nDiff = vecOutputEntries.size() - nCount;
                vecOutputEntries.erase(vecOutputEntries.begin(), vecOutputEntries.begin() + nDiff);
            }
        };

        coinControl.SetNull();
        coinControl.Select(GetCoins({{100 * COIN, false}})[0]);

        BOOST_CHECK(CreateTransaction({{-5000, false}}, strAmountNotNegative, false));
        BOOST_CHECK(CreateTransaction({}, strAtLeastOneRecipient, false));
        BOOST_CHECK(CreateTransaction({{545, false}}, strTooSmall, false));
        BOOST_CHECK(CreateTransaction({{545, true}}, strTooSmall, false));
        BOOST_CHECK(CreateTransaction({{546, true}}, strTooSmallAfterFee, false));

        createOutputEntries(100);
        vecOutputEntries.push_back({600, true});
        BOOST_CHECK(CreateTransaction(vecOutputEntries, strTooSmallToPayFee, false));
        vecOutputEntries.pop_back();

        createOutputEntries(2934);
        BOOST_CHECK(CreateTransaction(vecOutputEntries, {}, true));
        createOutputEntries(2935);
        BOOST_CHECK(CreateTransaction(vecOutputEntries, strTransactionTooLarge, false));

        auto prevRate = minRelayTxFee;
        coinControl.m_feerate = prevRate;
        coinControl.fOverrideFeeRate = true;
        minRelayTxFee = CFeeRate(prevRate.GetFeePerK() * 10);
        BOOST_CHECK(CreateTransaction({{5000, false}}, strTransactionTooLargeForFeePolicy, false));
        coinControl.m_feerate.reset();
        minRelayTxFee = prevRate;

        BOOST_CHECK(CreateTransaction({{5000, false}, {5000, false}, {5000, false}}, strChangeIndexOutOfRange, 4, false));
    }
}

// Check SelectCoinsGroupedByAddresses() behaviour
BOOST_FIXTURE_TEST_CASE(select_coins_grouped_by_addresses, ListCoinsTestingSetup)
{
    // Check initial balance from one mature coinbase transaction.
    BOOST_CHECK_EQUAL(wallet->GetAvailableBalance(), 500 * COIN);

    std::vector<CompactTallyItem> vecTally;
    BOOST_CHECK(wallet->SelectCoinsGroupedByAddresses(vecTally, false /*fSkipDenominated*/, false /*fAnonymizable*/,
                                                                false /*fSkipUnconfirmed*/, 100/*nMaxOupointsPerAddress*/));
    BOOST_CHECK_EQUAL(vecTally.size(), 1);
    BOOST_CHECK_EQUAL(vecTally.at(0).nAmount, 500 * COIN);
    BOOST_CHECK_EQUAL(vecTally.at(0).vecInputCoins.size(), 1);
    vecTally.clear();

    // Create two conflicting transactions, add one to the wallet and mine the other one.
    CTransactionRef tx1;
    CTransactionRef tx2;
    CReserveKey reservekey1(wallet.get());
    CReserveKey reservekey2(wallet.get());
    CAmount fee;
    int changePos = -1;
    std::string error;
    CCoinControl dummy;
    BOOST_CHECK(wallet->CreateTransaction({CRecipient{GetScriptForRawPubKey({}), 2 * COIN, true /* subtract fee */}},
                                        tx1, reservekey1, fee, changePos, error, dummy));
    BOOST_CHECK(wallet->CreateTransaction({CRecipient{GetScriptForRawPubKey({}), 1 * COIN, true /* subtract fee */}},
                                        tx2, reservekey2, fee, changePos, error, dummy));
    CValidationState state;
    BOOST_CHECK(wallet->CommitTransaction(tx1, {}, {}, {}, reservekey1, nullptr, state));
    reservekey2.KeepKey();
    BOOST_CHECK_EQUAL(wallet->GetAvailableBalance(), 0);
    CreateAndProcessBlock({CMutableTransaction(*tx2)}, GetScriptForRawPubKey({}));

    // Reveal the mined tx, it should conflict with the one we have in the wallet already.
    WalletRescanReserver reserver(wallet.get());
    reserver.reserve();
    BOOST_CHECK_EQUAL(wallet->ScanForWalletTransactions(chainActive.Genesis(), nullptr, reserver), nullptr);
    {
        LOCK(wallet->cs_wallet);
        const auto& conflicts = wallet->GetConflicts(tx2->GetHash());
        BOOST_CHECK_EQUAL(conflicts.size(), 2);
        BOOST_CHECK_EQUAL(conflicts.count(tx1->GetHash()), 1);
        BOOST_CHECK_EQUAL(conflicts.count(tx2->GetHash()), 1);
    }

    // Committed tx is the one that should be marked as "conflicting".
    // Make sure that available balance and SelectCoinsGroupedByAddresses results match.
    BOOST_CHECK(wallet->SelectCoinsGroupedByAddresses(vecTally, false /*fSkipDenominated*/, false /*fAnonymizable*/,
                                                                false /*fSkipUnconfirmed*/, 100/*nMaxOupointsPerAddress*/));
    BOOST_CHECK_EQUAL(vecTally.size(), 2);
    BOOST_CHECK_EQUAL(vecTally.at(0).vecInputCoins.size(), 1);
    BOOST_CHECK_EQUAL(vecTally.at(1).vecInputCoins.size(), 1);
    BOOST_CHECK_EQUAL(vecTally.at(0).nAmount + vecTally.at(1).nAmount, (500 + 499) * COIN);
    BOOST_CHECK_EQUAL(wallet->GetAvailableBalance(), (500 + 499) * COIN);
    vecTally.clear();
}

BOOST_AUTO_TEST_SUITE_END()
