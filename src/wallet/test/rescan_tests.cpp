// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <blockfilter.h>
#include <chain.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/scan.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

using node::MAX_BLOCKFILE_SIZE;

namespace wallet {

BOOST_FIXTURE_TEST_SUITE(rescan_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions, TestChain100Setup)
{
    // Cap last block file size, and mine new block in a new block file.
    CBlockIndex* oldTip = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip());
    WITH_LOCK(::cs_main, m_node.chainman->m_blockman.GetBlockFileInfo(oldTip->GetBlockPos().nFile)->nSize = MAX_BLOCKFILE_SIZE);
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    CBlockIndex* newTip = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip());

    // Verify Scan fails to read an unknown start block.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(/*start_block=*/{}, /*start_height=*/0, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::FAILURE);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK(result.last_scanned_block.IsNull());
        BOOST_CHECK(!result.last_scanned_height);
        BOOST_CHECK_EQUAL(GetBalance(wallet).m_mine_immature, 0);
    }

    // Verify Scan picks up transactions in both the old
    // and new block files.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(newTip->nHeight, newTip->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        std::chrono::steady_clock::time_point fake_time;
        reserver.setNow([&] { fake_time += 60s; return fake_time; });
        reserver.reserve();

        {
            CBlockLocator locator;
            BOOST_CHECK(WalletBatch{wallet.GetDatabase()}.ReadBestBlock(locator));
            BOOST_REQUIRE(!locator.IsNull());
            BOOST_CHECK(locator.vHave.front() == newTip->GetBlockHash());
        }

        ScanResult result = wallet.Scanner().Scan(/*start_block=*/oldTip->GetBlockHash(), /*start_height=*/oldTip->nHeight, /*max_height=*/{}, reserver, /*save_progress=*/true);
        BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, newTip->GetBlockHash());
        BOOST_CHECK_EQUAL(*result.last_scanned_height, newTip->nHeight);
        BOOST_CHECK_EQUAL(GetBalance(wallet).m_mine_immature, 100 * COIN);

        {
            CBlockLocator locator;
            BOOST_CHECK(WalletBatch{wallet.GetDatabase()}.ReadBestBlock(locator));
            BOOST_REQUIRE(!locator.IsNull());
            BOOST_CHECK(locator.vHave.front() == newTip->GetBlockHash());
        }
    }

    // Prune the older block file.
    int file_number;
    {
        LOCK(cs_main);
        file_number = oldTip->GetBlockPos().nFile;
        Assert(m_node.chainman)->m_blockman.PruneOneBlockFile(file_number);
    }
    m_node.chainman->m_blockman.UnlinkPrunedFiles({file_number});

    // Verify Scan only picks transactions in the new block
    // file.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(/*start_block=*/oldTip->GetBlockHash(), /*start_height=*/oldTip->nHeight, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::FAILURE);
        BOOST_CHECK_EQUAL(result.last_failed_block, oldTip->GetBlockHash());
        BOOST_CHECK_EQUAL(result.last_scanned_block, newTip->GetBlockHash());
        BOOST_CHECK_EQUAL(*result.last_scanned_height, newTip->nHeight);
        BOOST_CHECK_EQUAL(GetBalance(wallet).m_mine_immature, 50 * COIN);
    }

    // Prune the remaining block file.
    {
        LOCK(cs_main);
        file_number = newTip->GetBlockPos().nFile;
        Assert(m_node.chainman)->m_blockman.PruneOneBlockFile(file_number);
    }
    m_node.chainman->m_blockman.UnlinkPrunedFiles({file_number});

    // Verify Scan scans no blocks.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(/*start_block=*/oldTip->GetBlockHash(), /*start_height=*/oldTip->nHeight, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::FAILURE);
        BOOST_CHECK_EQUAL(result.last_failed_block, newTip->GetBlockHash());
        BOOST_CHECK(result.last_scanned_block.IsNull());
        BOOST_CHECK(!result.last_scanned_height);
        BOOST_CHECK_EQUAL(GetBalance(wallet).m_mine_immature, 0);
    }
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_reorged_block, TestChain100Setup)
{
    BOOST_REQUIRE(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1_MiB, /*f_memory=*/true));
    BlockFilterIndex& filter_index{*Assert(GetBlockFilterIndex(BlockFilterType::BASIC))};
    BOOST_REQUIRE(filter_index.Init());
    filter_index.Sync();

    // Reorg the tip out of the active chain: invalidate it, then mine a
    // longer replacement branch paying a script unrelated to the wallets
    // below.
    CBlockIndex* stale_block = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip());
    const uint256 stale_hash{stale_block->GetBlockHash()};
    const int stale_height{stale_block->nHeight};
    BlockValidationState state;
    BOOST_REQUIRE(m_node.chainman->ActiveChainstate().InvalidateBlock(state, stale_block));
    const CScript replacement_script{GetScriptForRawPubKey(GenerateRandomKey().GetPubKey())};
    CreateAndProcessBlock({}, replacement_script);
    CreateAndProcessBlock({}, replacement_script);
    BOOST_REQUIRE(filter_index.BlockUntilSyncedToCurrentChain());
    {
        LOCK(Assert(m_node.chainman)->GetMutex());
        BOOST_REQUIRE(!m_node.chainman->ActiveChain().Contains(*stale_block));
        BOOST_REQUIRE_EQUAL(m_node.chainman->ActiveChain().Height(), stale_height + 1);
    }

    {
        BlockFilter filter;
        BOOST_REQUIRE(filter_index.LookupFilter(stale_block, filter));
    }

    // Test wallet whose scripts do not match the stale block's filter.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(stale_hash, stale_height, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, stale_hash);
        BOOST_CHECK_EQUAL(*result.last_scanned_height, stale_height);
    }

    // Test wallet whose scripts do match the stale block's filter.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey); // the stale block's coinbase pays coinbaseKey
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(stale_hash, stale_height, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::FAILURE);
        BOOST_CHECK_EQUAL(result.last_failed_block, stale_hash);
        BOOST_CHECK(result.last_scanned_block.IsNull());
        BOOST_CHECK(!result.last_scanned_height);
        BOOST_CHECK(WITH_LOCK(wallet.cs_wallet, return wallet.mapWallet.empty()));
    }

    // Prune the stale block's file — the block is now not active AND unreadable.
    int file_number;
    {
        LOCK(cs_main);
        file_number = stale_block->GetBlockPos().nFile;
        Assert(m_node.chainman)->m_blockman.PruneOneBlockFile(file_number);
    }
    m_node.chainman->m_blockman.UnlinkPrunedFiles({file_number});

    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(stale_hash, stale_height, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::FAILURE);
        BOOST_CHECK_EQUAL(result.last_failed_block, stale_hash);
        BOOST_CHECK(result.last_scanned_block.IsNull());
        BOOST_CHECK(!result.last_scanned_height);
        BOOST_CHECK(WITH_LOCK(wallet.cs_wallet, return wallet.mapWallet.empty()));
    }

    StopIndex(filter_index, m_node);
    BOOST_REQUIRE(DestroyBlockFilterIndex(BlockFilterType::BASIC));
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_abort, TestChain100Setup)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    uint256 genesis_hash;
    {
        LOCK(wallet.cs_wallet);
        LOCK(Assert(m_node.chainman)->GetMutex());
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        genesis_hash = m_node.chainman->ActiveChain().Genesis()->GetBlockHash();
    }

    // An abort requested while no rescan is held is stale and must
    // not cancel a later scan.
    wallet.Scanner().Abort();
    WalletRescanReserver reserver(wallet);
    BOOST_CHECK(reserver.reserve());
    BOOST_CHECK(!wallet.Scanner().IsAborting());

    // An abort requested after the reservation but before the scan starts
    // (e.g. while importdescriptors is still deriving keys) must cancel the
    // scan.
    wallet.Scanner().Abort();
    ScanResult result = wallet.Scanner().Scan(genesis_hash, /*start_height=*/0, /*max_height=*/{}, reserver, /*save_progress=*/false);
    BOOST_CHECK_EQUAL(result.status, ScanResult::USER_ABORT);
    BOOST_CHECK(result.last_scanned_block.IsNull());
    BOOST_CHECK(!result.last_scanned_height);
    BOOST_CHECK(result.last_failed_block.IsNull());
}

BOOST_FIXTURE_TEST_CASE(wallet_rescan_reserver, TestingSetup)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());

    // No scan in progress: accessors report idle state.
    BOOST_CHECK(!wallet.Scanner().IsScanning());
    BOOST_CHECK(wallet.Scanner().ScanningDuration() == SteadyClock::duration{});
    BOOST_CHECK_EQUAL(wallet.Scanner().ScanningProgress(), 0.0);

    {
        WalletRescanReserver first_reserver(wallet);
        BOOST_CHECK(first_reserver.reserve());
        BOOST_CHECK(first_reserver.isReserved());
        BOOST_CHECK(wallet.Scanner().IsScanning());
        BOOST_CHECK(!wallet.Scanner().IsScanningWithPassphrase());
        BOOST_CHECK_EQUAL(wallet.Scanner().ScanningProgress(), 0.0);

        // Only one reservation can be held at a time.
        WalletRescanReserver second_reserver(wallet);
        BOOST_CHECK(!second_reserver.reserve());
        BOOST_CHECK(!second_reserver.isReserved());
    }
    // Destroying the reserver (RAII) clears the scanning state.
    BOOST_CHECK(!wallet.Scanner().IsScanning());

    {
        WalletRescanReserver passphrase_reserver(wallet);
        BOOST_CHECK(passphrase_reserver.reserve(/*with_passphrase=*/true));
        BOOST_CHECK(wallet.Scanner().IsScanningWithPassphrase());
    }
    BOOST_CHECK(!wallet.Scanner().IsScanningWithPassphrase());
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_bounded, TestChain100Setup)
{
    uint256 genesis_hash, max_hash, tip_hash;
    int max_height, tip_height;
    {
        LOCK(Assert(m_node.chainman)->GetMutex());
        genesis_hash = m_node.chainman->ActiveChain().Genesis()->GetBlockHash();
        tip_height = m_node.chainman->ActiveChain().Height();
        tip_hash = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        max_height = tip_height - 2;
        max_hash = m_node.chainman->ActiveChain()[max_height]->GetBlockHash();
    }

    // A scan with max_height set stops exactly at max_height and does not
    // sync any blocks beyond it.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(tip_height, tip_hash);
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(genesis_hash, /*start_height=*/0, max_height, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, max_hash);
        BOOST_CHECK_EQUAL(*result.last_scanned_height, max_height);
        // One coinbase per block from height 1 through max_height.
        BOOST_CHECK_EQUAL(WITH_LOCK(wallet.cs_wallet, return wallet.mapWallet.size()), static_cast<size_t>(max_height));
    }

    // A single-block range (start == max_height == tip) scans exactly that
    // block.
    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet.cs_wallet);
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet.SetLastBlockProcessed(tip_height, tip_hash);
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        ScanResult result = wallet.Scanner().Scan(tip_hash, tip_height, tip_height, reserver, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, tip_hash);
        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip_height);
        BOOST_CHECK_EQUAL(WITH_LOCK(wallet.cs_wallet, return wallet.mapWallet.size()), 1U);
    }
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_tip_extension, TestChain100Setup)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    uint256 genesis_hash;
    int start_tip_height{0};
    {
        LOCK(wallet.cs_wallet);
        LOCK(Assert(m_node.chainman)->GetMutex());
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        start_tip_height = m_node.chainman->ActiveChain().Height();
        wallet.SetLastBlockProcessed(start_tip_height, m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        genesis_hash = m_node.chainman->ActiveChain().Genesis()->GetBlockHash();
    }
    AddKey(wallet, coinbaseKey);

    // Connect a block while the scan is running (the handler fires on the
    // scanning thread as the scan starts) and advance the wallet's tip, as
    // the blockConnected notification would. The scan must pick up the new
    // tip instead of stopping at the height it started with.
    uint256 new_tip_hash;
    bool extended{false};
    auto handler = wallet.ShowProgress.connect([&](const std::string&, int) {
        if (extended) return;
        extended = true;
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK(wallet.cs_wallet);
        LOCK(Assert(m_node.chainman)->GetMutex());
        const CBlockIndex* new_tip = m_node.chainman->ActiveChain().Tip();
        new_tip_hash = new_tip->GetBlockHash();
        wallet.SetLastBlockProcessed(new_tip->nHeight, new_tip_hash);
    });

    WalletRescanReserver reserver(wallet);
    reserver.reserve();
    ScanResult result = wallet.Scanner().Scan(genesis_hash, /*start_height=*/0, /*max_height=*/{}, reserver, /*save_progress=*/false);
    handler.disconnect();
    BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
    BOOST_CHECK_EQUAL(result.last_scanned_block, new_tip_hash);
    BOOST_CHECK_EQUAL(*result.last_scanned_height, start_tip_height + 1);
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_no_progress_saved, TestChain100Setup)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    uint256 genesis_hash, tip_hash;
    int max_height;
    {
        LOCK(wallet.cs_wallet);
        LOCK(Assert(m_node.chainman)->GetMutex());
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        tip_hash = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
        wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), tip_hash);
        genesis_hash = m_node.chainman->ActiveChain().Genesis()->GetBlockHash();
        max_height = m_node.chainman->ActiveChain().Height() - 2;
    }
    AddKey(wallet, coinbaseKey);

    WalletRescanReserver reserver(wallet);
    // Advance the clock on every call so that every scanned block would be
    // eligible for a progress write if save_progress were set.
    std::chrono::steady_clock::time_point fake_time;
    reserver.setNow([&] { fake_time += 60s; return fake_time; });
    reserver.reserve();

    ScanResult result = wallet.Scanner().Scan(genesis_hash, /*start_height=*/0, max_height, reserver, /*save_progress=*/false);
    BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);

    // With save_progress=false the scan must not touch the wallet's best
    // block record: it still points at the tip written when the descriptor
    // was added, not at any block the scan visited.
    CBlockLocator locator;
    BOOST_CHECK(WalletBatch{wallet.GetDatabase()}.ReadBestBlock(locator));
    BOOST_CHECK(!locator.IsNull());
    BOOST_CHECK_EQUAL(locator.vHave.front(), tip_hash);
}

BOOST_FIXTURE_TEST_CASE(rescan_from_time, TestChain100Setup)
{
    // Cap last block file size, and mine new block in a new block file.
    CBlockIndex* old_tip = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip());
    WITH_LOCK(::cs_main, m_node.chainman->m_blockman.GetBlockFileInfo(old_tip->GetBlockPos().nFile)->nSize = MAX_BLOCKFILE_SIZE);
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    CBlockIndex* new_tip = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Tip());

    // Prune the older block file.
    int file_number;
    {
        LOCK(cs_main);
        file_number = old_tip->GetBlockPos().nFile;
        Assert(m_node.chainman)->m_blockman.PruneOneBlockFile(file_number);
    }
    m_node.chainman->m_blockman.UnlinkPrunedFiles({file_number});

    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    {
        LOCK(wallet.cs_wallet);
        LOCK(Assert(m_node.chainman)->GetMutex());
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
    }
    AddKey(wallet, coinbaseKey);
    WalletRescanReserver reserver(wallet);
    reserver.reserve();

    // Blocks before the prune point cannot be read: the returned timestamp
    // is moved past the last unreadable block, telling the caller from when
    // the rescan is actually complete.
    const int64_t genesis_time{WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Genesis()->GetBlockTime())};
    BOOST_CHECK_EQUAL(wallet.Scanner().ScanFromTime(genesis_time, reserver),
                      WITH_LOCK(::cs_main, return old_tip->GetBlockTimeMax()) + TIMESTAMP_WINDOW + 1);

    bool scan_logged{false};
    DebugLogHelper scan_check{"Rescan started from block", [&](const std::string* s) {
        if (s) scan_logged = true;
        return false;
    }};
    // A timestamp past the tip requires no scanning and is returned unchanged.
    const int64_t future_time{WITH_LOCK(::cs_main, return new_tip->GetBlockTimeMax()) + TIMESTAMP_WINDOW + 1};
    BOOST_CHECK(!scan_logged);
    BOOST_CHECK_EQUAL(wallet.Scanner().ScanFromTime(future_time, reserver), future_time);
}

BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_missing_filter, TestChain100Setup)
{
    // Enable the block filter index but do not sync it: no filters are
    // available, so the scan must inspect every block rather than treat
    // the missing filters as misses and skip blocks.
    BOOST_REQUIRE(InitBlockFilterIndex([&]{ return interfaces::MakeChain(m_node); }, BlockFilterType::BASIC, 1_MiB, /*f_memory=*/true));
    BlockFilterIndex& filter_index{*Assert(GetBlockFilterIndex(BlockFilterType::BASIC))};
    BOOST_REQUIRE(filter_index.Init());

    {
        CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
        uint256 genesis_hash, tip_hash;
        int tip_height;
        {
            LOCK(wallet.cs_wallet);
            LOCK(Assert(m_node.chainman)->GetMutex());
            wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            genesis_hash = m_node.chainman->ActiveChain().Genesis()->GetBlockHash();
            tip_height = m_node.chainman->ActiveChain().Height();
            auto tip{m_node.chainman->ActiveChain().Tip()};
            tip_hash = tip->GetBlockHash();
            wallet.SetLastBlockProcessed(tip_height, tip_hash);
            BlockFilter filter;
            BOOST_REQUIRE(!filter_index.LookupFilter(tip, filter));
        }
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet);
        reserver.reserve();
        bool fast_scan_logged{false};
        DebugLogHelper scan_check{"fast variant using block filters", [&](const std::string* s) {
            if (s) fast_scan_logged = true;
            return false;
        }};
        ScanResult result = wallet.Scanner().Scan(genesis_hash, /*start_height=*/0, /*max_height=*/{}, reserver, /*save_progress=*/false);
        BOOST_REQUIRE(fast_scan_logged);
        BOOST_CHECK_EQUAL(result.status, ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, tip_hash);
        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip_height);
        // One coinbase per block from height 1 through the tip.
        BOOST_CHECK_EQUAL(WITH_LOCK(wallet.cs_wallet, return wallet.mapWallet.size()), static_cast<size_t>(tip_height));
    }

    StopIndex(filter_index, m_node);
    BOOST_REQUIRE(DestroyBlockFilterIndex(BlockFilterType::BASIC));
}

//! Test the rescan that loading a wallet performs when the wallet is behind
//! the chain tip: it scans from the wallet's recorded best block - a
//! mid-chain start - with cs_wallet held.
BOOST_FIXTURE_TEST_CASE(scan_for_wallet_transactions_attach_chain, TestChain100Setup)
{
    // Do not wait for sqlite to flush data to disk to improve performance
    m_args.ForceSetArg("-unsafesqlitesync", "1");

    // Create a wallet owning the coinbases, and unload it at the current tip.
    WalletContext context;
    context.args = &m_args;
    context.chain = m_node.chain.get();
    auto wallet = TestCreateWallet(context);
    AddKey(*wallet, coinbaseKey);
    TestUnloadWallet(std::move(wallet));

    // Extend the chain while the wallet is not loaded.
    constexpr int NEW_BLOCKS{5};
    for (int i = 0; i < NEW_BLOCKS; ++i) {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }

    int tip_height;
    uint256 tip_hash;
    {
        LOCK(Assert(m_node.chainman)->GetMutex());
        tip_height = m_node.chainman->ActiveChain().Height();
        tip_hash = m_node.chainman->ActiveChain().Tip()->GetBlockHash();
    }

    // Loading the wallet must rescan the extension from the recorded best
    // block and find its coinbases.
    wallet = TestLoadWallet(context);
    {
        LOCK(wallet->cs_wallet);
        BOOST_CHECK_EQUAL(wallet->GetLastBlockHeight(), tip_height);
        BOOST_CHECK_EQUAL(wallet->GetLastBlockHash(), tip_hash);
        // The extension's coinbases plus the one of the recorded best block:
        // the load rescan starts mid-chain, at that block inclusive.
        BOOST_CHECK_EQUAL(wallet->mapWallet.size(), static_cast<size_t>(NEW_BLOCKS + 1));
    }
    TestUnloadWallet(std::move(wallet));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet
