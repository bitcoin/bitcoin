// Copyright (c) 2020-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <test/util/setup_common.h>
#include <test/util/txmempool.h>

#include <coinjoin/client.h>
#include <coinjoin/coinjoin.h>
#include <coinjoin/walletman.h>
#include <coinjoin/options.h>
#include <coinjoin/util.h>
#include <consensus/amount.h>
#include <interfaces/coinjoin.h>
#include <masternode/sync.h>
#include <node/context.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/time.h>
#include <validation.h>
#include <txmempool.h>
#include <wallet/context.h>
#include <wallet/db.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinjoin_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(coinjoin_options_tests)
{
    gArgs.ForceSetArg("-enablecoinjoin", "0");
    const auto loader{interfaces::MakeCoinJoinLoader(m_node)};

    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetSessions(), DEFAULT_COINJOIN_SESSIONS);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetRounds(), DEFAULT_COINJOIN_ROUNDS);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetRandomRounds(), COINJOIN_RANDOM_ROUNDS);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetAmount(), DEFAULT_COINJOIN_AMOUNT);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetDenomsGoal(), DEFAULT_COINJOIN_DENOMS_GOAL);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetDenomsHardCap(), DEFAULT_COINJOIN_DENOMS_HARDCAP);

    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsEnabled(), false);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsMultiSessionEnabled(), DEFAULT_COINJOIN_MULTISESSION);

    CCoinJoinClientOptions::SetEnabled(true);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsEnabled(), true);
    CCoinJoinClientOptions::SetEnabled(false);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsEnabled(), false);

    CCoinJoinClientOptions::SetMultiSessionEnabled(!DEFAULT_COINJOIN_MULTISESSION);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsMultiSessionEnabled(), !DEFAULT_COINJOIN_MULTISESSION);
    CCoinJoinClientOptions::SetMultiSessionEnabled(DEFAULT_COINJOIN_MULTISESSION);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::IsMultiSessionEnabled(), DEFAULT_COINJOIN_MULTISESSION);

    CCoinJoinClientOptions::SetRounds(DEFAULT_COINJOIN_ROUNDS + 10);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetRounds(), DEFAULT_COINJOIN_ROUNDS + 10);
    CCoinJoinClientOptions::SetAmount(DEFAULT_COINJOIN_AMOUNT + 50);
    BOOST_CHECK_EQUAL(CCoinJoinClientOptions::GetAmount(), DEFAULT_COINJOIN_AMOUNT + 50);
}

BOOST_AUTO_TEST_CASE(coinjoin_collateral_tests)
{
    // Good collateral values
    static_assert(CoinJoin::IsCollateralAmount(0.00010000 * COIN));
    static_assert(CoinJoin::IsCollateralAmount(0.00012345 * COIN));
    static_assert(CoinJoin::IsCollateralAmount(0.00032123 * COIN));
    static_assert(CoinJoin::IsCollateralAmount(0.00019000 * COIN));

    // Bad collateral values
    static_assert(!CoinJoin::IsCollateralAmount(0.00009999 * COIN));
    static_assert(!CoinJoin::IsCollateralAmount(0.00040001 * COIN));
    static_assert(!CoinJoin::IsCollateralAmount(0.00100000 * COIN));
    static_assert(!CoinJoin::IsCollateralAmount(0.00100001 * COIN));
}

BOOST_AUTO_TEST_CASE(coinjoin_pending_dsa_request_tests)
{
    CPendingDsaRequest dsa_request;
    BOOST_CHECK(dsa_request.GetProTxHash() == uint256());
    BOOST_CHECK(dsa_request.GetDSA() == CCoinJoinAccept());
    BOOST_CHECK_EQUAL(dsa_request.IsExpired(), true);
    CPendingDsaRequest dsa_request_2;
    BOOST_CHECK(dsa_request == dsa_request_2);
    CCoinJoinAccept cja;
    cja.nDenom = 4;
    uint256 proTxHash{uint256::ONE};
    CPendingDsaRequest custom_request(proTxHash, cja);
    BOOST_CHECK(custom_request.GetProTxHash() == proTxHash);
    BOOST_CHECK(custom_request.GetDSA() == cja);
    BOOST_CHECK_EQUAL(custom_request.IsExpired(), false);
    SetMockTime(GetTime() + 15);
    BOOST_CHECK_EQUAL(custom_request.IsExpired(), false);
    SetMockTime(GetTime() + 1);
    BOOST_CHECK_EQUAL(custom_request.IsExpired(), true);

    BOOST_CHECK(dsa_request != custom_request);
    BOOST_CHECK(!(dsa_request == custom_request));
    BOOST_CHECK(!dsa_request);
    BOOST_CHECK(custom_request);
}

BOOST_AUTO_TEST_CASE(coinjoin_dstxin_tests)
{
    CTxDSIn txin;
    BOOST_CHECK(txin.prevPubKey == CScript());
    BOOST_CHECK_EQUAL(txin.fHasSig, false);
    BOOST_CHECK_EQUAL(txin.nRounds, -10);
    CTxDSIn custom_txin(txin, CScript(4), -9);
    BOOST_CHECK(custom_txin.prevPubKey == CScript(4));
    BOOST_CHECK_EQUAL(custom_txin.fHasSig, false);
    BOOST_CHECK_EQUAL(custom_txin.nRounds, -9);
}

BOOST_AUTO_TEST_CASE(coinjoin_status_update_tests)
{
    CCoinJoinStatusUpdate cjsu;
    BOOST_CHECK_EQUAL(cjsu.nSessionID, 0);
    BOOST_CHECK_EQUAL(cjsu.nState, POOL_STATE_IDLE);
    BOOST_CHECK_EQUAL(cjsu.nEntriesCount, 0);
    BOOST_CHECK_EQUAL(cjsu.nStatusUpdate, STATUS_ACCEPTED);
    BOOST_CHECK_EQUAL(cjsu.nMessageID, MSG_NOERR);
    CCoinJoinStatusUpdate custom_cjsu(1, POOL_STATE_QUEUE, 1, STATUS_REJECTED, ERR_QUEUE_FULL);
    BOOST_CHECK_EQUAL(custom_cjsu.nSessionID, 1);
    BOOST_CHECK_EQUAL(custom_cjsu.nState, POOL_STATE_QUEUE);
    BOOST_CHECK_EQUAL(custom_cjsu.nEntriesCount, 1);
    BOOST_CHECK_EQUAL(custom_cjsu.nStatusUpdate, STATUS_REJECTED);
    BOOST_CHECK_EQUAL(custom_cjsu.nMessageID, ERR_QUEUE_FULL);
}

BOOST_AUTO_TEST_CASE(coinjoin_accept_tests)
{
    CCoinJoinAccept cja;
    BOOST_CHECK_EQUAL(cja.nDenom, 0);
    BOOST_CHECK_EQUAL(cja.txCollateral.GetHash(), CMutableTransaction().GetHash());
    // CMutableTransaction custom_cmt()
}

class CTransactionBuilderTestSetup : public TestChain100Setup
{
public:
    CTransactionBuilderTestSetup() :
        wallet{std::make_unique<CWallet>(m_node.chain.get(), m_node.coinjoin_loader.get(), "", m_args, CreateMockWalletDatabase())}
    {
        context.args = &m_args;
        context.chain = m_node.chain.get();
        context.coinjoin_loader = m_node.coinjoin_loader.get();
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        wallet->SetupLegacyScriptPubKeyMan();
        wallet->LoadWallet();
        AddWallet(context, wallet);
        {
            LOCK2(wallet->cs_wallet, ::cs_main);
            wallet->GetLegacyScriptPubKeyMan()->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());
            wallet->SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        }
        WalletRescanReserver reserver(*wallet);
        reserver.reserve();
        CWallet::ScanResult result = wallet->ScanForWalletTransactions(/*start_block=*/wallet->chain().getBlockHash(0),
                                                                       /*start_height=*/0, /*max_height=*/{}, reserver,
                                                                       /*fUpdate=*/true, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
    }

    ~CTransactionBuilderTestSetup()
    {
        RemoveWallet(context, wallet, /*load_on_start=*/std::nullopt);
    }

    WalletContext context;
    const std::shared_ptr<CWallet> wallet;

    CWalletTx& AddTxToChain(uint256 nTxHash)
    {
        decltype(wallet->mapWallet)::iterator it;
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            it = wallet->mapWallet.find(nTxHash);
            BOOST_REQUIRE(it != wallet->mapWallet.end());
            blocktx = CMutableTransaction(*it->second.tx);
        }
        CreateAndProcessBlock({blocktx}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK2(wallet->cs_wallet, ::cs_main);
        wallet->SetLastBlockProcessed(m_node.chainman->ActiveChain().Height(), m_node.chainman->ActiveChain().Tip()->GetBlockHash());
        it->second.m_state = TxStateConfirmed{m_node.chainman->ActiveChain().Tip()->GetBlockHash(), m_node.chainman->ActiveChain().Height(), /*index=*/1};
        return it->second;
    }
    CompactTallyItem GetTallyItem(const std::vector<CAmount>& vecAmounts)
    {
        CompactTallyItem tallyItem;
        ReserveDestination reserveDest(wallet.get());
        int nChangePosRet{RANDOM_CHANGE_POSITION};
        CCoinControl coinControl;
        coinControl.m_feerate = CFeeRate(1000);
        {
            LOCK(wallet->cs_wallet);
            auto dest_opt = reserveDest.GetReservedDestination(false);
            BOOST_REQUIRE(dest_opt);
            tallyItem.txdest = *dest_opt;
        }
        for (CAmount nAmount : vecAmounts) {
            CTransactionRef tx;
            {
                auto res = CreateTransaction(*wallet, {{GetScriptForDestination(tallyItem.txdest), nAmount, false}}, nChangePosRet, coinControl);
                // Report why it failed, a bare "critical check res has failed" says nothing
                BOOST_REQUIRE_MESSAGE(res, strprintf("CreateTransaction(%d) failed: %s", nAmount,
                                                     util::ErrorString(res).original));
                tx = res->tx;
                nChangePosRet = res->change_pos;
            }
            {
                LOCK2(wallet->cs_wallet, ::cs_main);
                wallet->CommitTransaction(tx, {}, {});
            }
            AddTxToChain(tx->GetHash());
            for (uint32_t n = 0; n < tx->vout.size(); ++n) {
                if (nChangePosRet != RANDOM_CHANGE_POSITION && int(n) == nChangePosRet) {
                    // Skip the change output to only return the requested coins
                    continue;
                }
                tallyItem.outpoints.emplace_back(tx->GetHash(), n);
                tallyItem.nAmount += tx->vout[n].nValue;
            }
        }
        BOOST_REQUIRE_EQUAL(tallyItem.outpoints.size(), vecAmounts.size());
        reserveDest.KeepDestination();
        return tallyItem;
    }
};

BOOST_FIXTURE_TEST_CASE(coinjoin_pending_observation_tests, CTransactionBuilderTestSetup)
{
    // 0.100001 DASH, a valid CoinJoin denomination
    constexpr CAmount nDenomAmount{10000100};
    BOOST_REQUIRE(CoinJoin::IsDenominatedAmount(nDenomAmount));
    CompactTallyItem tallyItem = GetTallyItem({nDenomAmount, nDenomAmount, nDenomAmount, nDenomAmount});
    const COutPoint outpointUserLocked = tallyItem.outpoints[0];
    const COutPoint outpointPending = tallyItem.outpoints[1];
    const COutPoint outpointTimeout = tallyItem.outpoints[2];
    const COutPoint outpointInMempool = tallyItem.outpoints[3];

    // A denominated coin the user locked themselves, e.g. via `lockunspent`
    WITH_LOCK(wallet->cs_wallet, wallet->LockCoin(outpointUserLocked));

    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        // A user-created lock is never adopted as a pending observation: it has no
        // CoinJoin record backing it, so it is left strictly alone
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointUserLocked));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 0);

        const int64_t nStart{GetTime()};
        SetMockTime(nStart);
        cj_man.AddPendingObservation({outpointPending});
        BOOST_CHECK(cj_man.IsPendingObservation(outpointPending));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 1);
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPending)));

        // Pending inputs are excluded from coin selection while unrelated inputs are not
        {
            LOCK(wallet->cs_wallet);
            bool fFoundPending{false};
            bool fFoundFree{false};
            for (const auto& out : AvailableCoinsListUnspent(*wallet).All()) {
                fFoundPending |= out.outpoint == outpointPending;
                fFoundFree |= out.outpoint == outpointTimeout;
            }
            BOOST_CHECK(!fFoundPending);
            BOOST_CHECK(fFoundFree);
        }

        // The pending set is mirrored to the wallet database so it survives a restart
        {
            std::map<COutPoint, int64_t> persisted;
            WalletBatch batch(wallet->GetDatabase());
            BOOST_REQUIRE(batch.ReadCoinJoinPendingObs(persisted));
            BOOST_CHECK_EQUAL(persisted.size(), 1);
            BOOST_CHECK(persisted.count(outpointPending) > 0);
            BOOST_CHECK_EQUAL(persisted.at(outpointPending), nStart);
        }

        // Nothing is released while the inputs remain unspent and the timeout has not passed
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 1);

        // Once the wallet observes a transaction spending a pending input its lock is dropped
        CMutableTransaction mtxSpend;
        mtxSpend.vin.emplace_back(outpointPending);
        mtxSpend.vout.emplace_back(nDenomAmount - 1000, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxSpend), TxStateInMempool{}));
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointPending));
        BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPending)));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 0);

        // An input whose spending transaction sits in the mempool but has not reached the
        // wallet yet must NOT be released: findCoins() reports it as unspent (it only
        // knows outputs mempool transactions create, not the ones they spend), so the
        // mempool has to be consulted separately
        CMutableTransaction mtxMempool;
        mtxMempool.vin.emplace_back(outpointInMempool);
        mtxMempool.vout.emplace_back(nDenomAmount - 1000, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        {
            LOCK2(::cs_main, m_node.mempool->cs);
            m_node.mempool->addUnchecked(TestMemPoolEntryHelper().FromTx(MakeTransactionRef(mtxMempool)));
        }
        BOOST_REQUIRE(m_node.mempool->isSpent(outpointInMempool));
        BOOST_REQUIRE(WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(mtxMempool.GetHash())) == nullptr);

        cj_man.AddPendingObservation({outpointTimeout, outpointInMempool});
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 2);
        SetMockTime(nStart + COINJOIN_PENDING_OBSERVATION_TIMEOUT + 1);

        // The timeout never fires while the chain is still catching up: the spending
        // transaction could be sitting in a block we have not downloaded yet
        BOOST_REQUIRE(!m_node.mn_sync->IsBlockchainSynced());
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 2);
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointTimeout)));

        m_node.mn_sync->SwitchToNextAsset();
        BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());
        cj_man.CheckPendingObservations(*m_node.mempool);

        // Unspent everywhere - released (with a warning) after the terminal timeout
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointTimeout));
        BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointTimeout)));
        // Spent by an unconfirmed transaction the wallet has not recorded - kept locked
        BOOST_CHECK(cj_man.IsPendingObservation(outpointInMempool));
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointInMempool)));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 1);

        // A manual unlock (e.g. via lockunspent) purges the pending entry
        WITH_LOCK(wallet->cs_wallet, wallet->UnlockCoin(outpointInMempool));
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointInMempool));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 0);

        // The user's own lock was never touched throughout
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointUserLocked)));
        SetMockTime(0);
    }));
}

BOOST_FIXTURE_TEST_CASE(coinjoin_pending_observation_reload_tests, CTransactionBuilderTestSetup)
{
    // 0.100001 DASH, a valid CoinJoin denomination
    constexpr CAmount nDenomAmount{10000100};
    CompactTallyItem tallyItem = GetTallyItem({nDenomAmount});
    const COutPoint outpointPending = tallyItem.outpoints[0];

    const int64_t nStart{GetTime()};
    SetMockTime(nStart);
    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        cj_man.AddPendingObservation({outpointPending});
        BOOST_CHECK(cj_man.IsPendingObservation(outpointPending));
    }));
    BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPending)));

    // Drop the client manager and create a fresh one for the same wallet: it has to pick
    // the pending observation back up from the wallet database, the way it would after a
    // restart, otherwise the persisted lock would be left with nothing to release it
    m_node.cj_walletman->removeWallet(wallet->GetName());
    m_node.cj_walletman->addWallet(wallet);

    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        // The record is read lazily, on the first check
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 0);
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(cj_man.IsPendingObservation(outpointPending));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 1);
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPending)));

        m_node.mn_sync->SwitchToNextAsset();
        BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());

        // The grace period was restored along with the entry rather than restarted: the
        // terminal timeout is measured from when the session completed, not from reload
        SetMockTime(nStart + COINJOIN_PENDING_OBSERVATION_TIMEOUT - 1);
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(cj_man.IsPendingObservation(outpointPending));
        SetMockTime(nStart + COINJOIN_PENDING_OBSERVATION_TIMEOUT + 1);
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointPending));
        BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPending)));
    }));
    SetMockTime(0);
}

BOOST_FIXTURE_TEST_CASE(coinjoin_pending_observation_unreadable_tests, CTransactionBuilderTestSetup)
{
    // 0.100001 DASH, a valid CoinJoin denomination
    constexpr CAmount nDenomAmount{10000100};
    CompactTallyItem tallyItem = GetTallyItem({nDenomAmount, nDenomAmount});
    const COutPoint outpointPersisted = tallyItem.outpoints[0];
    const COutPoint outpointInMemory = tallyItem.outpoints[1];

    const int64_t nStart{GetTime()};
    SetMockTime(nStart);
    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        cj_man.AddPendingObservation({outpointPersisted});
    }));
    BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPersisted)));

    // Corrupt the record behind the manager's back: it exists but cannot be deserialized
    // into the pending map anymore. This must not read as "nothing was ever pending", the
    // locks it tracks would be left with nothing to ever release them.
    BOOST_REQUIRE(wallet->GetDatabase().MakeBatch()->Write(std::string(DBKeys::COINJOIN_PENDING_OBS),
                                                           std::string("not a pending observation map")));

    // A fresh manager for the same wallet, the way a restart would build one
    m_node.cj_walletman->removeWallet(wallet->GetName());
    m_node.cj_walletman->addWallet(wallet);

    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        // The read fails, so nothing is loaded and the persisted lock is left in place
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 0);
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPersisted)));

        // Adding a new observation while the record is unreadable must not overwrite it,
        // that would orphan the locks it still tracks for good. The new entry is kept in
        // memory (and its coin locked) but deliberately not persisted.
        cj_man.AddPendingObservation({outpointInMemory});
        BOOST_CHECK(cj_man.IsPendingObservation(outpointInMemory));
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointInMemory)));
        {
            std::map<COutPoint, int64_t> persisted;
            WalletBatch batch(wallet->GetDatabase());
            BOOST_CHECK(!batch.ReadCoinJoinPendingObs(persisted));
            BOOST_CHECK(persisted.empty());
        }

        // Same for the release path: a pass which changes the pending set must not write
        // the record out either while its contents are still unknown
        WITH_LOCK(wallet->cs_wallet, wallet->UnlockCoin(outpointInMemory));
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointInMemory));
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPersisted)));
        {
            std::map<COutPoint, int64_t> persisted;
            WalletBatch batch(wallet->GetDatabase());
            BOOST_CHECK(!batch.ReadCoinJoinPendingObs(persisted));
        }

        // Once the record is readable again the entry it tracks is picked back up and the
        // lock behind it can finally be released. An entry added in the meantime is only
        // locked in memory until then, its lock is persisted along with the record.
        cj_man.AddPendingObservation({outpointInMemory});
        {
            WalletBatch batch(wallet->GetDatabase());
            BOOST_REQUIRE(batch.WriteCoinJoinPendingObs({{outpointPersisted, nStart}}));
        }
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(cj_man.IsPendingObservation(outpointPersisted));
        BOOST_CHECK_EQUAL(cj_man.GetPendingObservationCount(), 2);
        BOOST_CHECK(wallet->GetDatabase().MakeBatch()->Exists(
            std::make_pair(DBKeys::LOCKED_UTXO, std::make_pair(outpointInMemory.hash, outpointInMemory.n))));

        m_node.mn_sync->SwitchToNextAsset();
        BOOST_REQUIRE(m_node.mn_sync->IsBlockchainSynced());
        SetMockTime(nStart + COINJOIN_PENDING_OBSERVATION_TIMEOUT + 1);
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(!cj_man.IsPendingObservation(outpointPersisted));
        BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpointPersisted)));
    }));
    SetMockTime(0);
}

// The failed write is provoked with a transaction left open on the database, which only blocks
// further transactions on SQLite: BerkeleyBatch::TxnBegin() starts an independent one.
#ifdef USE_SQLITE
BOOST_FIXTURE_TEST_CASE(coinjoin_pending_observation_persist_retry_tests, CTransactionBuilderTestSetup)
{
    // 0.100001 DASH, a valid CoinJoin denomination
    constexpr CAmount nDenomAmount{10000100};
    const COutPoint outpoint = GetTallyItem({nDenomAmount}).outpoints[0];

    BOOST_CHECK(m_node.cj_walletman->doForClient("", [&](CCoinJoinClientManager& cj_man) {
        // Load the (missing) record first so that only the write itself fails below
        cj_man.CheckPendingObservations(*m_node.mempool);
        {
            // A transaction left open on the database keeps the manager from starting its own
            WalletBatch blocker(wallet->GetDatabase());
            BOOST_REQUIRE(blocker.TxnBegin());
            cj_man.AddPendingObservation({outpoint});
            BOOST_REQUIRE(blocker.TxnAbort());
        }
        BOOST_CHECK(cj_man.IsPendingObservation(outpoint));
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsLockedCoin(outpoint)));

        // The initial write really failed: neither the record nor the persistent lock exist
        BOOST_CHECK(!wallet->GetDatabase().MakeBatch()->Exists(std::string(DBKeys::COINJOIN_PENDING_OBS)));
        BOOST_CHECK(!wallet->GetDatabase().MakeBatch()->Exists(
            std::make_pair(DBKeys::LOCKED_UTXO, std::make_pair(outpoint.hash, outpoint.n))));

        // Nothing else changes, the next check has to retry the failed write on its own
        cj_man.CheckPendingObservations(*m_node.mempool);
        BOOST_CHECK(cj_man.IsPendingObservation(outpoint));
    }));

    // Load a copy of the wallet database, the way a restart would: the input must still
    // be locked and tracked
    DatabaseOptions options;
    const auto reloaded = std::make_shared<CWallet>(m_node.chain.get(), m_node.coinjoin_loader.get(), "", m_args,
                                                    DuplicateMockDatabase(wallet->GetDatabase(), options));
    BOOST_REQUIRE_EQUAL(reloaded->LoadWallet(), DBErrors::LOAD_OK);
    BOOST_CHECK(WITH_LOCK(reloaded->cs_wallet, return reloaded->IsLockedCoin(outpoint)));
    std::map<COutPoint, int64_t> persisted;
    BOOST_CHECK(WalletBatch(reloaded->GetDatabase()).ReadCoinJoinPendingObs(persisted));
    BOOST_CHECK(persisted.count(outpoint) > 0);
}
#endif // USE_SQLITE

BOOST_FIXTURE_TEST_CASE(coinjoin_manager_start_stop_tests, CTransactionBuilderTestSetup)
{
    BOOST_CHECK(m_node.cj_walletman->doForClient("", [](auto& cj_man) {
        BOOST_CHECK_EQUAL(cj_man.isMixing(), false);
        BOOST_CHECK_EQUAL(cj_man.startMixing(), true);
        BOOST_CHECK_EQUAL(cj_man.isMixing(), true);
        BOOST_CHECK_EQUAL(cj_man.startMixing(), false);
        cj_man.stopMixing();
        BOOST_CHECK_EQUAL(cj_man.isMixing(), false);
    }));
}

// End-to-end check that NewKeyPool() stops mixing
BOOST_FIXTURE_TEST_CASE(coinjoin_newkeypool_stops_mixing_tests, CTransactionBuilderTestSetup)
{
    BOOST_CHECK(m_node.cj_walletman->doForClient("", [](auto& cj_man) {
        BOOST_REQUIRE(cj_man.startMixing());
        BOOST_CHECK_EQUAL(cj_man.isMixing(), true);
    }));
    {
        LOCK(wallet->cs_wallet);
        BOOST_REQUIRE(wallet->GetLegacyScriptPubKeyMan()->NewKeyPool());
    }
    BOOST_CHECK(m_node.cj_walletman->doForClient("", [](auto& cj_man) {
        BOOST_CHECK_EQUAL(cj_man.isMixing(), false);
    }));
}

BOOST_FIXTURE_TEST_CASE(coinjoin_rebalance_rounds_reset_tests, CTransactionBuilderTestSetup)
{
    // 0.100001 DASH and its larger adjacent denomination 1.00001 DASH; the standard
    // denominations are constructed so PROMOTION_RATIO smaller coins equal one larger coin
    constexpr CAmount nSmallerAmount{10000100};
    constexpr CAmount nLargerAmount{nSmallerAmount * CoinJoin::PROMOTION_RATIO};
    BOOST_REQUIRE(CoinJoin::IsDenominatedAmount(nSmallerAmount));
    BOOST_REQUIRE(CoinJoin::IsDenominatedAmount(nLargerAmount));

    // PROMOTION_RATIO coins for the promotion plus one to demonstrate standard mixing
    CompactTallyItem tallyItem = GetTallyItem(std::vector<CAmount>(CoinJoin::PROMOTION_RATIO + 1, nSmallerAmount));
    const CScript scriptOurs = GetScriptForRawPubKey(coinbaseKey.GetPubKey());

    // The funding transactions pay fees, so their denominated outputs start at 0 rounds
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(tallyItem.outpoints[0]), 0);

    // Standard mixing shape (1 in -> 1 out, same denomination): rounds advance by one
    CMutableTransaction mtxMix;
    mtxMix.vin.emplace_back(tallyItem.outpoints[0]);
    mtxMix.vout.emplace_back(nSmallerAmount, scriptOurs);
    BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxMix), TxStateInMempool{}));
    const COutPoint outpointMixed{mtxMix.GetHash(), 0};
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(outpointMixed), 1);

    // Promotion shape (PROMOTION_RATIO inputs -> 1 output at the larger adjacent
    // denomination): the conversion's public 10:1 shape clusters the participant's coins,
    // so the promoted output starts mixing over instead of inheriting its inputs' rounds
    CMutableTransaction mtxPromo;
    for (int i = 1; i <= CoinJoin::PROMOTION_RATIO; ++i) {
        mtxPromo.vin.emplace_back(tallyItem.outpoints[i]);
    }
    mtxPromo.vout.emplace_back(nLargerAmount, scriptOurs);
    BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxPromo), TxStateInMempool{}));
    const COutPoint outpointPromoted{mtxPromo.GetHash(), 0};
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(outpointPromoted), 0);

    // A promoted coin re-enters mixing normally: one standard round at the new
    // denomination advances it to 1
    CMutableTransaction mtxRemix;
    mtxRemix.vin.emplace_back(outpointPromoted);
    mtxRemix.vout.emplace_back(nLargerAmount, scriptOurs);
    BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxRemix), TxStateInMempool{}));
    const COutPoint outpointRemixed{mtxRemix.GetHash(), 0};
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(outpointRemixed), 1);

    // Demotion shape (1 input -> PROMOTION_RATIO outputs at the smaller adjacent
    // denomination): the mirror image, every demoted output starts mixing over
    CMutableTransaction mtxDemo;
    mtxDemo.vin.emplace_back(outpointRemixed);
    for (int i = 0; i < CoinJoin::PROMOTION_RATIO; ++i) {
        mtxDemo.vout.emplace_back(nSmallerAmount, scriptOurs);
    }
    BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxDemo), TxStateInMempool{}));
    for (uint32_t n = 0; n < uint32_t(CoinJoin::PROMOTION_RATIO); ++n) {
        BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(COutPoint{mtxDemo.GetHash(), n}), 0);
    }
}

BOOST_FIXTURE_TEST_CASE(coinjoin_promotion_prefers_coins_from_distinct_txs_tests, CTransactionBuilderTestSetup)
{
    constexpr CAmount nSmallerAmount{10000100};
    constexpr CAmount nLargerAmount{nSmallerAmount * CoinJoin::PROMOTION_RATIO};
    const int nSmallerDenom{CoinJoin::AmountToDenomination(nSmallerAmount)};
    const int nLargerDenom{CoinJoin::AmountToDenomination(nLargerAmount)};
    const size_t idxSmaller = CoinJoin::GetDenominationIndex(nSmallerDenom);
    const CScript scriptOurs = GetScriptForRawPubKey(coinbaseKey.GetPubKey());

    const bool fEnabledPrev{CCoinJoinClientOptions::IsEnabled()};
    const int nRoundsPrev{CCoinJoinClientOptions::GetRounds()};
    const int nGoalPrev{CCoinJoinClientOptions::GetDenomsGoal()};
    CCoinJoinClientOptions::SetEnabled(true);
    CCoinJoinClientOptions::SetRounds(MIN_COINJOIN_ROUNDS);
    CCoinJoinClientOptions::SetDenomsGoal(MIN_COINJOIN_DENOMS_GOAL);

    // Mines zero-fee self-mixing txs of nInputsPerTx inputs and as many outputs each, until
    // vecIn is fully mixed whatever the salt decides
    const auto mix_fully = [&](std::vector<COutPoint> vecIn, size_t nInputsPerTx) {
        BOOST_REQUIRE(nInputsPerTx > 0 && vecIn.size() % nInputsPerTx == 0);
        for (int i = 0; i < CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds(); ++i) {
            std::vector<COutPoint> vecOut;
            for (auto it = vecIn.begin(); it != vecIn.end(); it += nInputsPerTx) {
                CMutableTransaction mtx;
                for (auto in = it; in != it + nInputsPerTx; ++in) {
                    mtx.vin.emplace_back(*in);
                    mtx.vout.emplace_back(nSmallerAmount, scriptOurs);
                }
                BOOST_REQUIRE(WITH_LOCK(wallet->cs_wallet, return wallet->SignTransaction(mtx)));
                BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{}));
                AddTxToChain(mtx.GetHash());
                for (uint32_t n = 0; n < mtx.vout.size(); ++n) {
                    vecOut.emplace_back(mtx.GetHash(), n);
                }
            }
            vecIn = vecOut;
        }
        for (const auto& outpoint : vecIn) {
            BOOST_REQUIRE(wallet->IsFullyMixed(outpoint));
        }
    };
    const auto count_parents = [](const std::vector<COutPoint>& vecOutpoints) {
        std::set<uint256> setParents;
        for (const auto& outpoint : vecOutpoints) {
            setParents.insert(outpoint.hash);
        }
        return setParents.size();
    };

    // A full mixing entry's worth of fully-mixed coins sharing one parent tx, as standard mixing
    // clusters them, plus one coin of its own: the promotion is reported, and selection takes
    // the lone coin and fills the rest with siblings
    mix_fully(GetTallyItem(std::vector<CAmount>(COINJOIN_ENTRY_MAX_SIZE, nSmallerAmount)).outpoints,
              COINJOIN_ENTRY_MAX_SIZE);
    mix_fully(GetTallyItem({nSmallerAmount}).outpoints, 1);
    const auto counts = wallet->GetDenominationCounts();
    BOOST_CHECK_EQUAL(counts.fully_mixed[idxSmaller], CoinJoin::PROMOTION_RATIO);
    BOOST_CHECK(CCoinJoinClientManager::ShouldPromote(nSmallerDenom, nLargerDenom, counts));
    auto vecSelected = wallet->SelectFullyMixedForPromotion(nSmallerDenom, CoinJoin::PROMOTION_RATIO);
    BOOST_CHECK_EQUAL(vecSelected.size(), static_cast<size_t>(CoinJoin::PROMOTION_RATIO));
    BOOST_CHECK_EQUAL(count_parents(vecSelected), 2U);

    // Enough coins of their own for a whole promotion: selection takes every distinct parent
    // before any sibling, so only one coin of the cluster is used
    mix_fully(GetTallyItem(std::vector<CAmount>(CoinJoin::PROMOTION_RATIO - 2, nSmallerAmount)).outpoints, 1);
    vecSelected = wallet->SelectFullyMixedForPromotion(nSmallerDenom, CoinJoin::PROMOTION_RATIO);
    BOOST_CHECK_EQUAL(vecSelected.size(), static_cast<size_t>(CoinJoin::PROMOTION_RATIO));
    BOOST_CHECK_EQUAL(count_parents(vecSelected), static_cast<size_t>(CoinJoin::PROMOTION_RATIO));

    CCoinJoinClientOptions::SetDenomsGoal(nGoalPrev);
    CCoinJoinClientOptions::SetRounds(nRoundsPrev);
    CCoinJoinClientOptions::SetEnabled(fEnabledPrev);
}

BOOST_FIXTURE_TEST_CASE(coinjoin_rounds_cache_unknown_tx_tests, CTransactionBuilderTestSetup)
{
    constexpr CAmount nDenomAmount{10000100}; // 0.100001 DASH
    BOOST_REQUIRE(CoinJoin::IsDenominatedAmount(nDenomAmount));

    CompactTallyItem tallyItem = GetTallyItem({nDenomAmount});
    const CScript scriptOurs = GetScriptForRawPubKey(coinbaseKey.GetPubKey());

    CMutableTransaction mtxMix;
    mtxMix.vin.emplace_back(tallyItem.outpoints[0]);
    mtxMix.vout.emplace_back(nDenomAmount, scriptOurs);
    const COutPoint outpointMixed{mtxMix.GetHash(), 0};

    // Queried before the wallet knows the tx, the way RPCs do for user-supplied
    // preset inputs: unknown, reported as -1
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(outpointMixed), -1);

    // Once the wallet learns the tx its rounds are computed from it; the miss
    // above must not stick in mapOutpointRoundsCache
    BOOST_REQUIRE(wallet->AddToWallet(MakeTransactionRef(mtxMix), TxStateInMempool{}));
    BOOST_CHECK_EQUAL(wallet->GetRealOutpointCoinJoinRounds(outpointMixed), 1);
}

BOOST_FIXTURE_TEST_CASE(CTransactionBuilderTest, CTransactionBuilderTestSetup)
{
    // NOTE: Mock wallet version is FEATURE_BASE which means that it uses uncompressed pubkeys
    // (65 bytes instead of 33 bytes) and we use Low R signatures, so CTxIn size is 179 bytes.
    // Each output is 34 bytes, vin and vout compact sizes are 1 byte each.
    // Therefore base size (i.e. for a tx with 1 input, 0 outputs) is expected to be
    // 4(n32bitVersion) + 1(vin size) + 179(vin[0]) + 1(vout size) + 4(nLockTime) = 189 bytes.

    // Tests with single outpoint tallyItem
    {
        CompactTallyItem tallyItem = GetTallyItem({4999});
        CTransactionBuilder txBuilder(*wallet, tallyItem);

        BOOST_CHECK_EQUAL(txBuilder.CountOutputs(), 0);
        BOOST_CHECK_EQUAL(txBuilder.GetAmountInitial(), tallyItem.nAmount);
        BOOST_CHECK_EQUAL(txBuilder.GetAmountLeft(), 4810);         // 4999 - 189

        BOOST_CHECK(txBuilder.CouldAddOutput(4776));                // 4810 - 34
        BOOST_CHECK(!txBuilder.CouldAddOutput(4777));

        BOOST_CHECK(txBuilder.CouldAddOutput(0));
        BOOST_CHECK(!txBuilder.CouldAddOutput(-1));

        BOOST_CHECK(txBuilder.CouldAddOutputs({1000, 1000, 2708})); // (4810 - 34 * 3) split in 3 outputs
        BOOST_CHECK(!txBuilder.CouldAddOutputs({1000, 1000, 2709}));

        BOOST_CHECK_EQUAL(txBuilder.AddOutput(4999), nullptr);
        BOOST_CHECK_EQUAL(txBuilder.AddOutput(-1), nullptr);

        CTransactionBuilderOutput* output = txBuilder.AddOutput();
        BOOST_CHECK(output->UpdateAmount(txBuilder.GetAmountLeft()));
        BOOST_CHECK(output->UpdateAmount(1));
        BOOST_CHECK(output->UpdateAmount(output->GetAmount() + txBuilder.GetAmountLeft()));
        BOOST_CHECK(!output->UpdateAmount(output->GetAmount() + 1));
        BOOST_CHECK(!output->UpdateAmount(0));
        BOOST_CHECK(!output->UpdateAmount(-1));
        BOOST_CHECK_EQUAL(txBuilder.CountOutputs(), 1);

        bilingual_str strResult;
        BOOST_REQUIRE(txBuilder.Commit(strResult));
        CWalletTx& wtx = AddTxToChain(uint256S(strResult.original));
        BOOST_CHECK_EQUAL(wtx.tx->vout.size(), txBuilder.CountOutputs()); // should have no change output
        BOOST_CHECK_EQUAL(wtx.tx->vout[0].nValue, output->GetAmount());
        BOOST_CHECK(wtx.tx->vout[0].scriptPubKey == output->GetScript());
    }
    // Tests with multiple outpoint tallyItem
    {
        CompactTallyItem tallyItem = GetTallyItem({10000, 20000, 30000, 40000, 50000});
        CTransactionBuilder txBuilder(*wallet, tallyItem);
        std::vector<CTransactionBuilderOutput*> vecOutputs;
        bilingual_str strResult;

        auto output = txBuilder.AddOutput(100);
        BOOST_CHECK(output != nullptr);
        BOOST_CHECK(!txBuilder.Commit(strResult));

        if (output != nullptr) {
            output->UpdateAmount(1000);
            vecOutputs.push_back(output);
        }
        while (vecOutputs.size() < 100) {
            output = txBuilder.AddOutput(1000 + vecOutputs.size());
            if (output == nullptr) {
                break;
            }
            vecOutputs.push_back(output);
        }
        BOOST_CHECK_EQUAL(vecOutputs.size(), 100);
        BOOST_CHECK_EQUAL(txBuilder.CountOutputs(), vecOutputs.size());
        BOOST_REQUIRE(txBuilder.Commit(strResult));
        CWalletTx& wtx = AddTxToChain(uint256S(strResult.original));
        BOOST_CHECK_EQUAL(wtx.tx->vout.size(), txBuilder.CountOutputs() + 1); // should have change output
        for (const auto& out : wtx.tx->vout) {
            auto it = std::find_if(vecOutputs.begin(), vecOutputs.end(), [&](CTransactionBuilderOutput* output) -> bool {
                return output->GetAmount() == out.nValue && output->GetScript() == out.scriptPubKey;
            });
            if (it != vecOutputs.end()) {
                vecOutputs.erase(it);
            } else {
                // change output
                BOOST_CHECK_EQUAL(txBuilder.GetAmountLeft() - 34, out.nValue);
            }
        }
        BOOST_CHECK(vecOutputs.size() == 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
