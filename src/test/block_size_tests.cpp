// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "uint256.h"
#include "util.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(block_size_tests, TestingSetup)

// Fill block with dummy transactions until it's serialized size is exactly nSize
static void
FillBlock(CBlock& block, unsigned int nSize)
{
    assert(block.vtx.size() > 0); // Start with at least a coinbase

    unsigned int nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
    if (nBlockSize > nSize) {
        block.vtx.resize(1); // passed in block is too big, start with just coinbase
        nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
    }

    // Create a block that is exactly 1,000,000 bytes, serialized:
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_11;
    tx.vin[0].prevout.hash = block.vtx[0].GetHash(); // passes CheckBlock, would fail if we checked inputs.
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 1LL;
    tx.vout[0].scriptPubKey = block.vtx[0].vout[0].scriptPubKey;

    unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
    uint256 txhash = tx.GetHash();

    // ... add copies of tx to the block to get close to 1MB:
    while (nBlockSize+nTxSize < nSize) {
        block.vtx.push_back(tx);
        nBlockSize += nTxSize;
        tx.vin[0].prevout.hash = txhash; // ... just to make each transaction unique
        txhash = tx.GetHash();
    }
    // Make the last transaction exactly the right size by making the scriptSig bigger.
    block.vtx.pop_back();
    nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
    unsigned int nFill = nSize - nBlockSize - nTxSize;
    for (unsigned int i = 0; i < nFill; i++)
        tx.vin[0].scriptSig << OP_11;
    block.vtx.push_back(tx);
    nBlockSize = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
    assert(nBlockSize == nSize);
}

static bool TestCheckBlock(CBlock& block, uint64_t nTime, unsigned int nSize)
{
    SetMockTime(nTime);
    block.nTime = nTime;
    FillBlock(block, nSize);
    CValidationState validationState;
    bool fResult = CheckBlock(block, validationState, false, false) && validationState.IsValid();
    SetMockTime(0);
    return fResult;
}

//
// Unit test CheckBlock() for conditions around the block size hard fork
//
BOOST_AUTO_TEST_CASE(TwoMegFork)
{
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;

    LOCK(cs_main);

    BOOST_CHECK(pblocktemplate = CreateNewBlock(chainparams, scriptPubKey));
    CBlock *pblock = &pblocktemplate->block;

    // Before fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, BIP202_FORK_TIME-1, 1000*1000)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, BIP202_FORK_TIME-1, 1000*1000+1)); // >1MB : invalid
    BOOST_CHECK(!TestCheckBlock(*pblock, BIP202_FORK_TIME-1, 2*1000*1000)); // 2MB : invalid

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, BIP202_FORK_TIME, 1000*1000)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, BIP202_FORK_TIME, 2*1000*1000)); // 2MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, BIP202_FORK_TIME, 2*1000*1000+1)); // >2MB : invalid

    // Fork height + 10 min...
    BOOST_CHECK(TestCheckBlock(*pblock, BIP202_FORK_TIME+600, 2*1000*1000+20)); // 2MB+20 : valid

    // A year after fork time:
    unsigned int yearAfter = BIP202_FORK_TIME + (365 * 24 * 60 * 60);
    BOOST_CHECK(TestCheckBlock(*pblock, yearAfter, 1000*1000)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, yearAfter, 2*1000*1000)); // 2MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, yearAfter, 3*1000*1000)); // 3MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, yearAfter, 4*1000*1000)); // 4MB : invalid
}

BOOST_AUTO_TEST_SUITE_END()
