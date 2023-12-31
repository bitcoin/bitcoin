// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <amount.h>
#include <coinjoin/client.h>
#include <coinjoin/coinjoin.h>
#include <coinjoin/context.h>
#include <coinjoin/options.h>
#include <coinjoin/util.h>
#include <node/context.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinjoin_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(coinjoin_options_tests)
{
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
    BOOST_CHECK(dsa_request.GetAddr() == CService());
    BOOST_CHECK(dsa_request.GetDSA() == CCoinJoinAccept());
    BOOST_CHECK_EQUAL(dsa_request.IsExpired(), true);
    CPendingDsaRequest dsa_request_2;
    BOOST_CHECK(dsa_request == dsa_request_2);
    CCoinJoinAccept cja;
    cja.nDenom = 4;
    CService cserv(CNetAddr(), 1111);
    CPendingDsaRequest custom_request(cserv, cja);
    BOOST_CHECK(custom_request.GetAddr() == cserv);
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
    CTransactionBuilderTestSetup()
    {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        wallet = std::make_unique<CWallet>(m_node.chain.get(), m_node.coinjoin_loader.get(), "", CreateMockWalletDatabase());
        wallet->SetupLegacyScriptPubKeyMan();
        bool firstRun;
        wallet->LoadWallet(firstRun);
        AddWallet(wallet);
        {
            LOCK2(wallet->cs_wallet, cs_main);
            wallet->GetLegacyScriptPubKeyMan()->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());
            wallet->SetLastBlockProcessed(::ChainActive().Height(), ::ChainActive().Tip()->GetBlockHash());
            WalletRescanReserver reserver(*wallet);
            reserver.reserve();
            CWallet::ScanResult result = wallet->ScanForWalletTransactions(::ChainActive().Genesis()->GetBlockHash(),  0 /* start_height */, {} /* max_height */, reserver, true /* fUpdate */);
            BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
        }
    }

    ~CTransactionBuilderTestSetup()
    {
        RemoveWallet(wallet, std::nullopt);
    }

    std::shared_ptr<CWallet> wallet;

    CWalletTx& AddTxToChain(uint256 nTxHash)
    {
        std::map<uint256, CWalletTx>::iterator it;
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            it = wallet->mapWallet.find(nTxHash);
            assert(it != wallet->mapWallet.end());
            blocktx = CMutableTransaction(*it->second.tx);
        }
        CreateAndProcessBlock({blocktx}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK2(wallet->cs_wallet, cs_main);
        wallet->SetLastBlockProcessed(::ChainActive().Height(), ::ChainActive().Tip()->GetBlockHash());
        CWalletTx::Confirmation confirm(CWalletTx::Status::CONFIRMED, ::ChainActive().Height(), ::ChainActive().Tip()->GetBlockHash(), 1);
        it->second.m_confirm = confirm;
        return it->second;
    }
    CompactTallyItem GetTallyItem(const std::vector<CAmount>& vecAmounts)
    {
        CompactTallyItem tallyItem;
        CTransactionRef tx;
        ReserveDestination reserveDest(wallet.get());
        CAmount nFeeRet;
        int nChangePosRet = -1;
        bilingual_str strError;
        CCoinControl coinControl;
        coinControl.m_feerate = CFeeRate(1000);
        {
            LOCK(wallet->cs_wallet);
            BOOST_CHECK(reserveDest.GetReservedDestination(tallyItem.txdest, false));
        }
        for (CAmount nAmount : vecAmounts) {
            BOOST_CHECK(wallet->CreateTransaction({{GetScriptForDestination(tallyItem.txdest), nAmount, false}}, tx, nFeeRet, nChangePosRet, strError, coinControl));
            {
                LOCK2(wallet->cs_wallet, cs_main);
                wallet->CommitTransaction(tx, {}, {});
            }
            AddTxToChain(tx->GetHash());
            for (size_t n = 0; n < tx->vout.size(); ++n) {
                if (nChangePosRet != -1 && int(n) == nChangePosRet) {
                    // Skip the change output to only return the requested coins
                    continue;
                }
                tallyItem.vecInputCoins.emplace_back(tx, n);
                tallyItem.nAmount += tx->vout[n].nValue;
            }
        }
        assert(tallyItem.vecInputCoins.size() == vecAmounts.size());
        reserveDest.KeepDestination();
        return tallyItem;
    }
};

BOOST_FIXTURE_TEST_CASE(coinjoin_manager_start_stop_tests, CTransactionBuilderTestSetup)
{
    BOOST_CHECK_EQUAL(m_node.cj_ctx->walletman->raw().size(), 1);
    auto& cj_man = m_node.cj_ctx->walletman->raw().begin()->second;
    BOOST_CHECK_EQUAL(cj_man->IsMixing(), false);
    BOOST_CHECK_EQUAL(cj_man->StartMixing(), true);
    BOOST_CHECK_EQUAL(cj_man->IsMixing(), true);
    BOOST_CHECK_EQUAL(cj_man->StartMixing(), false);
    cj_man->StopMixing();
    BOOST_CHECK_EQUAL(cj_man->IsMixing(), false);
}


BOOST_FIXTURE_TEST_CASE(CTransactionBuilderTest, CTransactionBuilderTestSetup)
{
    // NOTE: Mock wallet version is FEATURE_BASE which means that it uses uncompressed pubkeys
    // (65 bytes instead of 33 bytes) and we use Low R signatures, so CTxIn size is 179 bytes.
    // Each output is 34 bytes, vin and vout compact sizes are 1 byte each.
    // Therefore base size (i.e. for a tx with 1 input, 0 outputs) is expected to be
    // 4(n32bitVersion) + 1(vin size) + 179(vin[0]) + 1(vout size) + 4(nLockTime) = 189 bytes.

    minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);
    // Tests with single outpoint tallyItem
    {
        CompactTallyItem tallyItem = GetTallyItem({4999});
        CTransactionBuilder txBuilder(wallet, tallyItem, *m_node.fee_estimator);

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
        BOOST_CHECK(txBuilder.Commit(strResult));
        CWalletTx& wtx = AddTxToChain(uint256S(strResult.original));
        BOOST_CHECK_EQUAL(wtx.tx->vout.size(), txBuilder.CountOutputs()); // should have no change output
        BOOST_CHECK_EQUAL(wtx.tx->vout[0].nValue, output->GetAmount());
        BOOST_CHECK(wtx.tx->vout[0].scriptPubKey == output->GetScript());
    }
    // Tests with multiple outpoint tallyItem
    {
        CompactTallyItem tallyItem = GetTallyItem({10000, 20000, 30000, 40000, 50000});
        CTransactionBuilder txBuilder(wallet, tallyItem, *m_node.fee_estimator);
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
        BOOST_CHECK(txBuilder.Commit(strResult));
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
