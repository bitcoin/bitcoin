// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/hdwallet.h"

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

#include "rpc/server.h"
#include "consensus/validation.h"

#include <chrono>
#include <thread>


#include <boost/test/unit_test.hpp>

struct StakeTestingSetup: public TestingSetup {
    StakeTestingSetup(const std::string& chainName = CBaseChainParams::REGTEST):
        TestingSetup(chainName, true) // fParticlMode = true
    {
        //fPrintToConsole = true;
        //fDebug = true;
        
        bitdb.MakeMock();

        bool fFirstRun;
        pwalletMain = (CWallet*) new CHDWallet("wallet_stake_test.dat");
        fParticlWallet = true;
        pwalletMain->LoadWallet(fFirstRun);
        RegisterValidationInterface(pwalletMain);

        RegisterWalletRPCCommands(tableRPC);
        RegisterHDWalletRPCCommands(tableRPC);
    }

    ~StakeTestingSetup()
    {
        UnregisterValidationInterface(pwalletMain);
        delete pwalletMain;
        pwalletMain = NULL;

        bitdb.Flush(true);
        bitdb.Reset();
        
        mapStakeSeen.clear();
        listStakeSeen.clear();
    }
};


extern UniValue CallRPC(std::string args);

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
        
        boost::shared_ptr<CReserveScript> coinbaseScript;
        boost::shared_ptr<CReserveKey> rKey(new CReserveKey(pwallet));
        coinbaseScript = rKey;
        
        int64_t nSearchTime = GetAdjustedTime() & ~Params().GetStakeTimestampMask(nBestHeight+1);
        if (nSearchTime <= pwallet->nLastCoinStakeSearchTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        };
        
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript->reserveScript));
        BOOST_REQUIRE(pblocktemplate.get());
        
        if (pwallet->SignBlock(pblocktemplate.get(), nBestHeight+1, nSearchTime))
        {
            CBlock *pblock = &pblocktemplate->block;
            BOOST_MESSAGE("SignBlock passed " << pblock->GetHash().ToString());
            //LogPrintf("[rm] SignBlock\n");
            
            if (CheckStake(pblock))
            {
                 //LogPrintf("[rm] nTimeLastStake %d\n", nTimeLastStake);
                 nStaked++;
                 BOOST_MESSAGE("nStaked " << nStaked);
            };
        };
        
        if (nStaked >= nBlocks)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    };
    BOOST_REQUIRE(k < nTries);
    BOOST_MESSAGE("nStaked " << nStaked);
};

BOOST_AUTO_TEST_CASE(stake_test)
{
    CHDWallet *pwallet = (CHDWallet*) pwalletMain;
    UniValue rv;
    
    const CChainParams &chainparams = Params(CBaseChainParams::REGTEST);
    
    
    BOOST_MESSAGE("Tip hash " << chainActive.Tip()->GetBlockHash().ToString());
    BOOST_MESSAGE("genesis hash " << chainparams.GenesisBlock().GetHash().ToString());
    BOOST_REQUIRE(chainparams.GenesisBlock().GetHash() == chainActive.Tip()->GetBlockHash());
    
    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster tprv8ZgxMBicQKsPeK5mCpvMsd1cwyT1JZsrBN82XkoYuZY1EVK7EwDaiL9sDfqUU5SntTfbRfnRedFWjg5xkDG5i3iwd3yP7neX5F2dtdCojk4"));
    
    
    // Import the key to the last 5 outputs in the regtest genesis coinbase
    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster tprv8ZgxMBicQKsPe3x7bUzkHAJZzCuGqN6y28zFFyg5i7Yqxqm897VCnmMJz6QScsftHDqsyWW5djx6FzrbkF9HSD3ET163z1SzRhfcWxvwL4G"));
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewextaddress lblHDKey")); 
    
    
    //BOOST_CHECK_NO_THROW(rv = CallRPC("extkey import xparFntbLBU6CS8cRXkPWRUcQfQa47aFN8d5VptFZ6scD14zPWsmMutnEcb6wonnk86zxAn6PumC64JnSf6k51kYjtEWEvfngDdgZCd9ES4rLaU"));
    
    //BOOST_CHECK_NO_THROW(rv = CallRPC("scanchain"));
    
    //BOOST_CHECK_NO_THROW(rv = CallRPC("extkey importAccount xparFntbLBU6CS8cRXkPWRUcQfQa47aFN8d5VptFZ6scD14zPWsmMutnEcb6wonnk86zxAn6PumC64JnSf6k51kYjtEWEvfngDdgZCd9ES4rLaU"));
    
    
    size_t nStaked = 0;
    {
        //LOCK(pwallet->cs_wallet);
        LOCK2(cs_main, pwallet->cs_wallet);
        BOOST_MESSAGE("pwallet->GetBalance() " << pwallet->GetBalance());
        BOOST_REQUIRE(pwallet->GetBalance() == 12500000000000);
    }

    nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp
    
    StakeNBlocks(pwallet, 2);
    
    BOOST_MESSAGE("chain height " << chainActive.Height());
    CBlockIndex *pindexDelete = chainActive.Tip();
    BOOST_REQUIRE(pindexDelete);
    BOOST_MESSAGE("Tip hash " << pindexDelete->GetBlockHash().ToString());
    
    CBlock block;
    BOOST_REQUIRE(ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus()));
    
    const CTxIn &txin = block.vtx[0]->vin[0];
    BOOST_MESSAGE("Staked kernel " << txin.prevout.ToString());
    
    CCoinsViewCache view(pcoinsTip, fParticlMode);
    const CCoins *coins = view.AccessCoins(txin.prevout.hash);
    BOOST_REQUIRE(coins);
    BOOST_MESSAGE("coins->IsAvailable(txin.prevout.n) " << coins->IsAvailable(txin.prevout.n));
    BOOST_CHECK(!coins->IsAvailable(txin.prevout.n));
    
    CValidationState state;
    BOOST_REQUIRE(DisconnectBlock(block, state, pindexDelete, view));
    BOOST_MESSAGE("view.GetBestBlock() " << view.GetBestBlock().ToString());
    BOOST_REQUIRE(view.Flush());
    BOOST_REQUIRE(FlushStateToDisk(state, FLUSH_STATE_IF_NEEDED));
    UpdateTip(pindexDelete->pprev, chainparams);
    
    
    
    BOOST_REQUIRE(pindexDelete->pprev->GetBlockHash() == chainActive.Tip()->GetBlockHash());
    
    BOOST_MESSAGE("state.IsInvalid() " << state.IsInvalid());
    
    
    const CCoins *coins2 = view.AccessCoins(txin.prevout.hash);
    BOOST_REQUIRE(coins2);
    BOOST_MESSAGE("Staked kernel " << txin.prevout.ToString());
    BOOST_MESSAGE("coins2->IsAvailable(txin.prevout.n) " << coins2->IsAvailable(txin.prevout.n));
    BOOST_CHECK(coins2->IsAvailable(txin.prevout.n));
    
    BOOST_CHECK(chainActive.Height() == pindexDelete->nHeight - 1);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == pindexDelete->pprev->GetBlockHash());
    BOOST_MESSAGE("chain height " << chainActive.Height());
    BOOST_MESSAGE("Tip hash " << chainActive.Tip()->GetBlockHash().ToString());
    
    
    // reconnect block
    {
        
        std::shared_ptr<const CBlock> pblock = std::make_shared<const CBlock>(block);
        BOOST_REQUIRE(ActivateBestChain(state, chainparams, pblock));
    }
    
    BOOST_MESSAGE("chain height " << chainActive.Height());
    BOOST_MESSAGE("Tip hash " << chainActive.Tip()->GetBlockHash().ToString());
    
    BOOST_MESSAGE("Staked kernel " << txin.prevout.ToString());
    BOOST_MESSAGE("coins2->IsAvailable(txin.prevout.n) " << coins2->IsAvailable(txin.prevout.n));
    BOOST_CHECK(coins2->IsAvailable(txin.prevout.n));
    
    // need to get a new view for coins to update
    {
        CCoinsViewCache view(pcoinsTip, fParticlMode);
        const CCoins *coins = view.AccessCoins(txin.prevout.hash);
        BOOST_REQUIRE(coins);
        
        BOOST_MESSAGE("coins->IsAvailable(txin.prevout.n) " << coins->IsAvailable(txin.prevout.n));
        BOOST_CHECK(!coins->IsAvailable(txin.prevout.n));
    }
    
    
    BOOST_MESSAGE("pwallet->GetBalance() " << pwallet->GetBalance());
    
    CKey kRecv;
    kRecv.MakeNewKey(true);
    
    CKeyID idRecv = kRecv.GetPubKey().GetID();
    
    bool fSubtractFeeFromAmount = false;
    CAmount nAmount = 1000;
    CWalletTx wtx;
    //SendMoney(idRecv, nAmount, fSubtractFeeFromAmount, wtx);
    
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
    BOOST_CHECK(pwallet->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError));
    BOOST_MESSAGE("strError " << strError);
    
    {
        LOCK(mempool.cs);
        BOOST_MESSAGE("mempool.size() " << mempool.size());
    }
    
    {
        g_connman = std::unique_ptr<CConnman>(new CConnman(GetRand(std::numeric_limits<uint64_t>::max()), GetRand(std::numeric_limits<uint64_t>::max())));
        CValidationState state;
        pwallet->SetBroadcastTransactions(true);
        BOOST_CHECK(pwallet->CommitTransaction(wtx, reservekey, g_connman.get(), state));
        BOOST_MESSAGE("state.GetRejectReason() " << state.GetRejectReason());
    }
    
    {
        LOCK(mempool.cs);
        BOOST_MESSAGE("mempool.size() " << mempool.size());
    }
    
    
    BOOST_MESSAGE("wtx.GetHash() " << wtx.GetHash().ToString());
    
    
    StakeNBlocks(pwallet, 1);
    
    BOOST_MESSAGE("chainActive.Tip()->GetBlockHash() " << chainActive.Tip()->GetBlockHash().ToString());
    
    CBlock blockLast;
    BOOST_REQUIRE(ReadBlockFromDisk(blockLast, chainActive.Tip(), chainparams.GetConsensus()));
    
    BOOST_MESSAGE("blockLast.vtx.size() " << blockLast.vtx.size());
    BOOST_REQUIRE(blockLast.vtx.size() == 2);
    BOOST_REQUIRE(blockLast.vtx[1]->GetHash() == wtx.GetHash());
    
    
    {
        uint256 tipHash = chainActive.Tip()->GetBlockHash();
        uint256 prevTipHash = chainActive.Tip()->pprev->GetBlockHash();
        
        // Disconnect last block
        CBlockIndex *pindexDelete = chainActive.Tip();
        BOOST_REQUIRE(pindexDelete);
        BOOST_MESSAGE("Tip hash " << pindexDelete->GetBlockHash().ToString());
        
        CBlock block;
        BOOST_REQUIRE(ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus()));
        
        CCoinsViewCache view(pcoinsTip, fParticlMode);
        CValidationState state;
        BOOST_REQUIRE(DisconnectBlock(block, state, pindexDelete, view));
        BOOST_MESSAGE("view.GetBestBlock() " << view.GetBestBlock().ToString());
        BOOST_REQUIRE(view.Flush());
        BOOST_REQUIRE(FlushStateToDisk(state, FLUSH_STATE_IF_NEEDED));
        UpdateTip(pindexDelete->pprev, chainparams);
        
        
        BOOST_MESSAGE("chainActive.Tip()->GetBlockHash() " << chainActive.Tip()->GetBlockHash().ToString());
        BOOST_CHECK(prevTipHash == chainActive.Tip()->GetBlockHash());
        
        
        // reduce the reward 
        RegtestParams().SetCoinYearReward(1 * CENT);
        
        {
            LOCK(cs_main);
            
            CValidationState state;
            CCoinsViewCache view(pcoinsTip, fParticlMode);
            BOOST_MESSAGE("view.GetBestBlock() " << view.GetBestBlock().ToString());
            BOOST_REQUIRE(false == ConnectBlock(block, state, pindexDelete, view, chainparams, false));
            
            BOOST_CHECK(state.IsInvalid());
            BOOST_CHECK(state.GetRejectReason() == "bad-cs-amount");
            BOOST_CHECK(prevTipHash == chainActive.Tip()->GetBlockHash());
            
            // restore the reward 
            RegtestParams().SetCoinYearReward(2 * CENT);
            
            // block should connect now
            CValidationState clearstate;
            CCoinsViewCache clearview(pcoinsTip, fParticlMode);
            BOOST_REQUIRE(ConnectBlock(block, clearstate, pindexDelete, clearview, chainparams, false));
            
            BOOST_MESSAGE("clearstate.IsInvalid() " << clearstate.IsInvalid());
            BOOST_CHECK(!clearstate.IsInvalid());
            BOOST_REQUIRE(clearview.Flush());
            BOOST_REQUIRE(FlushStateToDisk(clearstate, FLUSH_STATE_IF_NEEDED));
            
            UpdateTip(pindexDelete, chainparams);
            
            BOOST_MESSAGE("Tip hash " << chainActive.Tip()->GetBlockHash().ToString());
            
            BOOST_CHECK(tipHash == chainActive.Tip()->GetBlockHash());
        }
        
    }
    
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewextaddress lblTestKey"));
    std::string extaddr = StripQuotes(rv.write());
    BOOST_MESSAGE("extaddr " << extaddr);
    
    BOOST_CHECK(pwallet->GetBalance() + pwallet->GetStaked() == 12500000117911);
    
    
    
    
}

BOOST_AUTO_TEST_SUITE_END()
