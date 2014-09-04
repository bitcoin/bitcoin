// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "miner.h"
#include "uint256.h"
#include "util.h"

#include <boost/test/unit_test.hpp>

// Tests the majority rule which states that after 1000 v2 blocks no v1 block can go
BOOST_AUTO_TEST_SUITE(blockv2_tests)

static CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

static void SetEmptyBlock(CBlock * pblock)
{
        pblock->nVersion = 2;
        pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1;
        pblock->nNonce = 0;
}

static void SetBlockDefaultAttributesAndHeight(CBlock * pblock,bool addHeight,int difValue)
{
        SetEmptyBlock(pblock);

        // Add the coinbase
        CMutableTransaction txCoinbase(pblock->vtx[0]);

        if (addHeight)
            txCoinbase.vin[0].scriptSig = (CScript() << (chainActive.Height()+1+difValue) << 0);
            else
            txCoinbase.vin[0].scriptSig = (CScript() << difValue << 0); // At least size 2, this is a protocol spec

        txCoinbase.vout[0].scriptPubKey = CScript();
        pblock->vtx[0] = CTransaction(txCoinbase);
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

void CheckSubsidyHalving(CBlockTemplate * &pblocktemplate, CBlock * &pblock)
{
    if ((chainActive.Height()+1) % Params().SubsidyHalvingInterval() == 0)
        {
            // The RegTest network has a low subsidy halving interval (150) so
            // we must recompute the coinbase subsidy if we reach the boundary.

            // preserve parent hash
            uint256 prevParent = pblock->hashPrevBlock;
            delete pblocktemplate;
            pblocktemplate = CreateNewBlock(scriptPubKey);
            pblock = &pblocktemplate->block; // pointer for convenience
            pblock->hashPrevBlock = prevParent;
        }
}

void Blockv2test()
{
    assert(Params().NetworkID() == CBaseChainParams::UNITTEST);
    ModifiableParams()->setSkipProofOfWorkCheck(true);

    // We don't know the state of the block-chain here: it depends on which other tests are run before this test.
    // See https://github.com/bitcoin/bitcoin/pull/4688 for a patch that allows the re-creation of the block-chain
    // for each testcase that requires it.

    // If miner_tests.cpp is run before, the chain will be 100 blocks long, and all of them will be v1


    LogPrintf("Blockv2test testcase starts\n");

    CBlockTemplate *pblocktemplate;
    CScript script;
    uint256 hash;
    int PreviousHeight;


    LOCK(cs_main);


    // Simple block creation, nothing special yet.
    pblocktemplate = CreateNewBlock(scriptPubKey);
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    LogPrintf("Blockv2test block v1 add begin\n");
    // First create a block v1, check that it is accepted. The block has an invalid height
    SetBlockDefaultAttributesAndHeight(pblock,false,5000);
    pblock->nVersion = 1;
    CValidationState state1;
    PreviousHeight = chainActive.Height();
    BOOST_CHECK(ProcessBlock(state1, NULL, pblock));
    BOOST_CHECK(state1.IsValid());
    BOOST_CHECK((PreviousHeight+1) == chainActive.Height()); // to differentiate from orphan blocks, which also get accepted in ProcessBlock()
    pblock->hashPrevBlock = pblock->GetHash(); // update parent


    // Now create exactly 1000 blocks v2

    // First check that the supermajority threshold is exactly 1000 blocks
    BOOST_CHECK(Params().ToCheckBlockUpgradeMajority()==1000);  //
    BOOST_CHECK(Params().EnforceBlockUpgradeMajority()==750);
    BOOST_CHECK(Params().RejectBlockOutdatedMajority()==950);

    // Over the last 1000 blocks, 750 blocks must be v2 to switch to v2-only mode.
    // Here we're testing only the last 750, not any subset.

    LogPrintf("Blockv2test BIP30 repetition begin\n");

    // First, if we try to add a block v2 with the same coinbase tx, we should get
    // "bad-txns-BIP30" because the coinbase tx has the same hash as the previous.
    // Unluckily, even if ConnectBlock returns a "bad-txns-BIP30", ActivateBestChainStep clears
    // the state, so we get true here and the "bad-txns-BIP30" reason is lost.
    // We verify instead that the chain height has not been incremented.

    CValidationState state7;
    PreviousHeight = chainActive.Height();
    CheckSubsidyHalving(pblocktemplate,pblock);
    SetBlockDefaultAttributesAndHeight(pblock,false,5000); //
    pblock->nVersion = 2;
    BOOST_CHECK(ProcessBlock(state7, NULL, pblock)); // should we care about the return value?
    BOOST_CHECK(state7.IsValid());
    BOOST_CHECK(PreviousHeight == chainActive.Height()); // we check the block has not been added.

    LogPrintf("Blockv2test 750 v2 blocks  begin\n");
    for (int i=0;i<750;i++)
    {

        LogPrintf("Blockv2test block %d begin\n",i);

        CheckSubsidyHalving(pblocktemplate,pblock);

        // We add a value to the height to make is NOT equal to the actual height.
        SetBlockDefaultAttributesAndHeight(pblock,true,1000); // blocks version 2 without height are allowed! for only 750 blocks
        pblock->nVersion = 2;
        CValidationState state;

        PreviousHeight = chainActive.Height();
        BOOST_CHECK(ProcessBlock(state, NULL, pblock));
        BOOST_CHECK(state.IsValid());
        BOOST_CHECK((PreviousHeight+1) == chainActive.Height()); // to differentiate from orphan blocks, which also get accepted in ProcessBlock()
        pblock->hashPrevBlock = pblock->GetHash(); // update parent
    }

    LogPrintf("Blockv2test v2 without height rejected begin\n");

    // Now we try to add a block v2, with an invalid height and it should be rejected. We use 2000 because is not in the range [1000..1750].
    CheckSubsidyHalving(pblocktemplate,pblock);
    SetBlockDefaultAttributesAndHeight(pblock,true,2000); //
    pblock->nVersion = 2;
    CValidationState state0;
    BOOST_CHECK(ProcessBlock(state0, NULL, pblock)==false);
    BOOST_CHECK(!state0.IsValid());
    BOOST_CHECK(state0.GetRejectReason()=="bad-cb-height");
    // Do not update parent since block has failed

    LogPrintf("Blockv2test v2 with height accepted begin\n");


    // Now we add a block with height, must be ok.
    for (int i=0;i<200;i++)
    {

        LogPrintf("Blockv2test v2block %d begin\n",i);
        CheckSubsidyHalving(pblocktemplate,pblock);
        SetBlockDefaultAttributesAndHeight(pblock,true,0);
        pblock->nVersion = 2;
        CValidationState state;
        PreviousHeight = chainActive.Height();
        BOOST_CHECK(ProcessBlock(state, NULL, pblock));
        BOOST_CHECK(state.IsValid());
        BOOST_CHECK((PreviousHeight+1) == chainActive.Height()); // to differentiate from orphan blocks, which also get accepted in ProcessBlock()

        pblock->hashPrevBlock = pblock->GetHash(); // update parent
    }


    LogPrintf("Blockv2test block v1 rejected\n");
    // Now we add 200 additional blocks, until we get 950 (the threshold were v1 blocks are not accepted anymore)
    // Now we try to add a block v1, it should be rejected, even if it hash the height field
    CheckSubsidyHalving(pblocktemplate,pblock);
    SetBlockDefaultAttributesAndHeight(pblock,true,0);
    pblock->nVersion = 1;
    CValidationState state2;
    BOOST_CHECK(ProcessBlock(state2, NULL, pblock)==false);
    BOOST_CHECK(!state2.IsValid());
    BOOST_CHECK(state2.GetRejectReason()=="bad-version");
    // Do not update parent since block has failed



    // Some other missing tests, added here as bonus...

    // Block time too old check
    CheckSubsidyHalving(pblocktemplate,pblock);
    SetBlockDefaultAttributesAndHeight(pblock,true,0);
    pblock->nVersion = 2;
    pblock->nTime = chainActive.Tip()->GetMedianTimePast()-1;
    CValidationState state4;
    BOOST_CHECK(ProcessBlock(state4, NULL, pblock)==false);
    BOOST_CHECK(!state4.IsValid());
    BOOST_CHECK(state4.GetRejectReason()=="time-too-old");
    // Do not update parent since block has failed

    // Adding a non-final coinbase, must modify coinbase
    CheckSubsidyHalving(pblocktemplate,pblock);
    SetEmptyBlock(pblock);
    // Use a mutable coinbase to change nLockTime and  nSequence
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << chainActive.Height() << 0);
    txCoinbase.nLockTime = LOCKTIME_THRESHOLD-1; // refers to height
    txCoinbase.vin[0].nSequence = 1; // non-zero sequence
    pblock->vtx[0] = CTransaction(txCoinbase);
    pblock->nVersion = 2;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
    CValidationState state5;
    BOOST_CHECK(ProcessBlock(state5, NULL, pblock)==false);
    BOOST_CHECK(!state5.IsValid());
    BOOST_CHECK(state5.GetRejectReason()=="bad-txns-nonfinal");
    // Do not update parent since block has failed


    delete pblocktemplate;

    ModifiableParams()->setSkipProofOfWorkCheck(false);
    LogPrintf("Blockv2test testcase ends\n");
}

BOOST_AUTO_TEST_CASE(Blockv2testcase)
{
    Blockv2test();
}

BOOST_AUTO_TEST_SUITE_END()
