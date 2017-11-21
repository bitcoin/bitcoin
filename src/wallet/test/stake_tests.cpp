// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdwallet.h"
#include "wallet/coincontrol.h"

#include "wallet/test/hdwallet_test_fixture.h"
#include "base58.h"
#include "chainparams.h"
#include "miner.h"
#include "pos/miner.h"
#include "pos/kernel.h"
#include "timedata.h"
#include "coins.h"
#include "net.h"
#include "validation.h"
#include "blind.h"

#include "rpc/server.h"
#include "consensus/validation.h"

#include <chrono>
#include <thread>


#include <boost/test/unit_test.hpp>

extern CWallet *pwalletMain;

extern UniValue CallRPC(std::string args, std::string wallet="");

extern bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams, bool fJustCheck = false);

struct StakeTestingSetup: public TestingSetup {
    StakeTestingSetup(const std::string& chainName = CBaseChainParams::REGTEST):
        TestingSetup(chainName, true) // fParticlMode = true
    {
        bitdb.MakeMock();

        bool fFirstRun;
        std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, "wallet_test_part.dat"));
        pwalletMain = (CWallet*) new CHDWallet(std::move(dbw));
        vpwallets.push_back(pwalletMain);
        fParticlWallet = true;
        pwalletMain->LoadWallet(fFirstRun);
        RegisterValidationInterface(pwalletMain);

        RegisterWalletRPCCommands(tableRPC);
        RegisterHDWalletRPCCommands(tableRPC);
        ECC_Start_Stealth();
        ECC_Start_Blinding();
        ::pcoinsdbview = pcoinsdbview;
        SetMockTime(0);
    }

    ~StakeTestingSetup()
    {
        ::pcoinsdbview = nullptr;
        UnregisterValidationInterface(pwalletMain);
        delete pwalletMain;
        pwalletMain = nullptr;

        bitdb.Flush(true);
        bitdb.Reset();

        mapStakeSeen.clear();
        listStakeSeen.clear();
        vpwallets.clear();
        ECC_Stop_Stealth();
        ECC_Stop_Blinding();
    }
};

BOOST_FIXTURE_TEST_SUITE(stake_tests, StakeTestingSetup)


void StakeNBlocks(CHDWallet *pwallet, size_t nBlocks)
{
    int nBestHeight;
    size_t nStaked = 0;
    size_t k, nTries = 10000;
    for (k = 0; k < nTries; ++k)
    {
        {
            LOCK(cs_main);
            nBestHeight = chainActive.Height();
        }

        int64_t nSearchTime = GetAdjustedTime() & ~Params().GetStakeTimestampMask(nBestHeight+1);
        if (nSearchTime <= pwallet->nLastCoinStakeSearchTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        };

        CScript coinbaseScript;
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript));
        BOOST_REQUIRE(pblocktemplate.get());

        if (pwallet->SignBlock(pblocktemplate.get(), nBestHeight+1, nSearchTime))
        {
            CBlock *pblock = &pblocktemplate->block;

            if (CheckStake(pblock))
                 nStaked++;
        };

        if (nStaked >= nBlocks)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    };
    BOOST_REQUIRE(k < nTries);
};

static void AddAnonTxn(CHDWallet *pwallet, CBitcoinAddress &address, CAmount amount)
{
    CValidationState state;
    BOOST_REQUIRE(address.IsValid());

    std::vector<CTempRecipient> vecSend;
    std::string sError;
    CTempRecipient r;
    r.nType = OUTPUT_RINGCT;
    r.SetAmount(10);
    r.address = address.Get();
    vecSend.push_back(r);

    CWalletTx wtx;
    CTransactionRecord rtx;
    CAmount nFee;
    CCoinControl coinControl;
    BOOST_CHECK(0 == pwallet->AddStandardInputs(wtx, rtx, vecSend, true, nFee, &coinControl, sError));

    wtx.BindWallet(pwallet);
    BOOST_CHECK(wtx.AcceptToMemoryPool(maxTxFee, state));
}

BOOST_AUTO_TEST_CASE(stake_test)
{
    SeedInsecureRand();
    CHDWallet *pwallet = (CHDWallet*) pwalletMain;
    UniValue rv;

    std::unique_ptr<CChainParams> regtestChainParams = CreateChainParams(CBaseChainParams::REGTEST);
    const CChainParams &chainparams = *regtestChainParams;

    BOOST_REQUIRE(chainparams.GenesisBlock().GetHash() == chainActive.Tip()->GetBlockHash());
    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster tprv8ZgxMBicQKsPeK5mCpvMsd1cwyT1JZsrBN82XkoYuZY1EVK7EwDaiL9sDfqUU5SntTfbRfnRedFWjg5xkDG5i3iwd3yP7neX5F2dtdCojk4"));

    // Import the key to the last 5 outputs in the regtest genesis coinbase
    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster tprv8ZgxMBicQKsPe3x7bUzkHAJZzCuGqN6y28zFFyg5i7Yqxqm897VCnmMJz6QScsftHDqsyWW5djx6FzrbkF9HSD3ET163z1SzRhfcWxvwL4G"));
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewextaddress lblHDKey"));

    {
        LOCK2(cs_main, pwallet->cs_wallet);
        BOOST_REQUIRE(pwallet->GetBalance() == 12500000000000);
    }

    StakeNBlocks(pwallet, 2);

    CBlockIndex *pindexDelete = chainActive.Tip();
    BOOST_REQUIRE(pindexDelete);

    CBlock block;
    BOOST_REQUIRE(ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus()));

    const CTxIn &txin = block.vtx[0]->vin[0];

    CCoinsViewCache view(pcoinsTip);
    const Coin &coin = view.AccessCoin(txin.prevout);
    BOOST_REQUIRE(coin.IsSpent());

    CValidationState state;
    BOOST_REQUIRE(DISCONNECT_OK == DisconnectBlock(block, pindexDelete, view));
    BOOST_REQUIRE(FlushView(&view, state, true));
    BOOST_REQUIRE(FlushStateToDisk(chainparams, state, FLUSH_STATE_IF_NEEDED));
    UpdateTip(pindexDelete->pprev, chainparams);


    BOOST_REQUIRE(pindexDelete->pprev->GetBlockHash() == chainActive.Tip()->GetBlockHash());

    const Coin &coin2 = view.AccessCoin(txin.prevout);
    BOOST_REQUIRE(!coin2.IsSpent());

    BOOST_CHECK(chainActive.Height() == pindexDelete->nHeight - 1);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == pindexDelete->pprev->GetBlockHash());


    // reconnect block
    {
        std::shared_ptr<const CBlock> pblock = std::make_shared<const CBlock>(block);
        BOOST_REQUIRE(ActivateBestChain(state, chainparams, pblock));
    }

    {
        CCoinsViewCache view(pcoinsTip);
        const Coin &coin = view.AccessCoin(txin.prevout);
        BOOST_REQUIRE(coin.IsSpent());
    }

    CKey kRecv;
    InsecureNewKey(kRecv, true);
    CKeyID idRecv = kRecv.GetPubKey().GetID();

    bool fSubtractFeeFromAmount = false;
    CAmount nAmount = 10000;
    CWalletTx wtx;

    // Parse Bitcoin address
    CScript scriptPubKey = GetScriptForDestination(idRecv);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient(scriptPubKey, nAmount, fSubtractFeeFromAmount);
    vecSend.push_back(recipient);

    CCoinControl coinControl;
    BOOST_CHECK(pwallet->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, coinControl));

    {
        g_connman = std::unique_ptr<CConnman>(new CConnman(GetRand(std::numeric_limits<uint64_t>::max()), GetRand(std::numeric_limits<uint64_t>::max())));
        CValidationState state;
        pwallet->SetBroadcastTransactions(true);
        BOOST_CHECK(pwallet->CommitTransaction(wtx, reservekey, g_connman.get(), state));
    }

    StakeNBlocks(pwallet, 1);

    CBlock blockLast;
    BOOST_REQUIRE(ReadBlockFromDisk(blockLast, chainActive.Tip(), chainparams.GetConsensus()));

    BOOST_REQUIRE(blockLast.vtx.size() == 2);
    BOOST_REQUIRE(blockLast.vtx[1]->GetHash() == wtx.GetHash());

    {
        uint256 tipHash = chainActive.Tip()->GetBlockHash();
        uint256 prevTipHash = chainActive.Tip()->pprev->GetBlockHash();

        // Disconnect last block
        CBlockIndex *pindexDelete = chainActive.Tip();
        BOOST_REQUIRE(pindexDelete);

        CBlock block;
        BOOST_REQUIRE(ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus()));

        CCoinsViewCache view(pcoinsTip);
        CValidationState state;
        BOOST_REQUIRE(DISCONNECT_OK == DisconnectBlock(block, pindexDelete, view));
        BOOST_REQUIRE(FlushView(&view, state, true));
        BOOST_REQUIRE(FlushStateToDisk(chainparams, state, FLUSH_STATE_IF_NEEDED));
        UpdateTip(pindexDelete->pprev, chainparams);


        BOOST_CHECK(prevTipHash == chainActive.Tip()->GetBlockHash());


        // Reduce the reward
        RegtestParams().SetCoinYearReward(1 * CENT);

        {
            LOCK(cs_main);

            CValidationState state;
            CCoinsViewCache view(pcoinsTip);
            BOOST_REQUIRE(false == ConnectBlock(block, state, pindexDelete, view, chainparams, false));

            BOOST_CHECK(state.IsInvalid());
            BOOST_CHECK(state.GetRejectReason() == "bad-cs-amount");
            BOOST_CHECK(prevTipHash == chainActive.Tip()->GetBlockHash());

            // restore the reward
            RegtestParams().SetCoinYearReward(2 * CENT);

            // block should connect now
            CValidationState clearstate;
            CCoinsViewCache clearview(pcoinsTip);
            //CCoinsViewCache &clearview = *pcoinsTip;
            BOOST_REQUIRE(ConnectBlock(block, clearstate, pindexDelete, clearview, chainparams, false));

            BOOST_CHECK(!clearstate.IsInvalid());
            BOOST_REQUIRE(FlushView(&clearview, state, false));
            BOOST_REQUIRE(FlushStateToDisk(chainparams, clearstate, FLUSH_STATE_IF_NEEDED));

            UpdateTip(pindexDelete, chainparams);

            BOOST_CHECK(tipHash == chainActive.Tip()->GetBlockHash());
        }
    }

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewextaddress lblTestKey"));
    std::string extaddr = StripQuotes(rv.write());

    BOOST_CHECK(pwallet->GetBalance() + pwallet->GetStaked() == 12500000108911);

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress"));
        std::string sSxAddr = StripQuotes(rv.write());

        CBitcoinAddress address(sSxAddr);


        AddAnonTxn(pwallet, address, 10);

        AddAnonTxn(pwallet, address, 20);

        StakeNBlocks(pwallet, 1);


        int64_t nLastRCTOutIndex = 0;
        pblocktree->ReadLastRCTOutput(nLastRCTOutIndex);
        BOOST_CHECK(nLastRCTOutIndex == 4);


        {
            // Disconnect last block
            uint256 prevTipHash = chainActive.Tip()->pprev->GetBlockHash();
            CBlockIndex *pindexDelete = chainActive.Tip();
            BOOST_REQUIRE(pindexDelete);

            CBlock block;
            BOOST_REQUIRE(ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus()));

            CCoinsViewCache view(pcoinsTip);
            CValidationState state;
            BOOST_REQUIRE(DISCONNECT_OK == DisconnectBlock(block, pindexDelete, view));
            BOOST_REQUIRE(FlushView(&view, state, true));
            BOOST_REQUIRE(FlushStateToDisk(chainparams, state, FLUSH_STATE_IF_NEEDED));
            UpdateTip(pindexDelete->pprev, chainparams);

            BOOST_CHECK(prevTipHash == chainActive.Tip()->GetBlockHash());
        }

        pblocktree->ReadLastRCTOutput(nLastRCTOutIndex);
        BOOST_CHECK(nLastRCTOutIndex == 0);
    }

}

BOOST_AUTO_TEST_SUITE_END()
