// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "random.h"
#include "uint256.h"
#include "util.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

// These must match parameters in chainparams.cpp
static const uint64_t EARLIEST_FORK_TIME = 1452470400; // 11 Jan 2016
static const uint32_t MAXSIZE_PREFORK = 1000*1000;
static const uint32_t MAXSIZE_POSTFORK = 8*1000*1000;
static const uint64_t SIZE_DOUBLE_EPOCH = 60*60*24*365*2; // two years

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
    
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_11;
    tx.vin[0].prevout.hash = block.vtx[0].GetHash(); // passes CheckBlock, would fail if we checked inputs.
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 1LL;
    tx.vout[0].scriptPubKey = block.vtx[0].vout[0].scriptPubKey;

    unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
    block.vtx.reserve(1+nSize/nTxSize);

    // ... add copies of tx to the block to get close to nSize:
    while (nBlockSize+nTxSize < nSize) {
        block.vtx.push_back(tx);
        nBlockSize += nTxSize;
        tx.vin[0].prevout.hash = GetRandHash(); // Just to make each tx unique
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
BOOST_AUTO_TEST_CASE(BigBlockFork_Time1)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;

    uint64_t t = EARLIEST_FORK_TIME;
    uint64_t preforkSize = MAXSIZE_PREFORK;
    uint64_t postforkSize = MAXSIZE_POSTFORK;
    uint64_t tActivate = EARLIEST_FORK_TIME;

    sizeForkTime.store(tActivate);

    LOCK(cs_main);

    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    CBlock *pblock = &pblocktemplate->block;

    // Before fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t-1LL, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t-1LL, preforkSize+1)); // >1MB : invalid
    BOOST_CHECK(!TestCheckBlock(*pblock, t-1LL, postforkSize)); // big : invalid

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, t, postforkSize)); // big : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t,  postforkSize+1)); // big+1 : invalid

    // Halfway to first doubling...
    uint64_t tHalf = t+SIZE_DOUBLE_EPOCH/2;
    BOOST_CHECK(!TestCheckBlock(*pblock, tHalf-1, (3*postforkSize)/2));
    BOOST_CHECK(TestCheckBlock(*pblock, tHalf, (3*postforkSize)/2));
    BOOST_CHECK(!TestCheckBlock(*pblock, tHalf, (3*postforkSize)/2)+1);

    // Sanity check: April 1 2017 is more than halfway to first
    // doubling:
    uint64_t tApril_2017 = 1491004800;
    BOOST_CHECK(TestCheckBlock(*pblock, tApril_2017, (3*postforkSize)/2)+1);

    // After one doubling...
    uint64_t yearsAfter = t+SIZE_DOUBLE_EPOCH;
    BOOST_CHECK(TestCheckBlock(*pblock, yearsAfter, 2*postforkSize)); // 2 * big : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, yearsAfter, 2*postforkSize+1)); // > 2 * big : invalid

#if 0
    // These tests use gigabytes of memory and take a long time to run--
    // don't enable by default until computers have petabytes of memory
    // and are 100 times faster than in 2015.
    // Network protocol will have to be updated before we get there...
    uint64_t maxDoublings = 8;
    uint64_t postDoubleTime = t + SIZE_DOUBLE_EPOCH * maxDoublings + 1;
    uint64_t farFuture = t + SIZE_DOUBLE_EPOCH * 100;
    BOOST_CHECK(TestCheckBlock(*pblock, postDoubleTime, postforkSize<<maxDoublings));
    BOOST_CHECK(TestCheckBlock(*pblock, farFuture, postforkSize<<maxDoublings));
    BOOST_CHECK(!TestCheckBlock(*pblock, postDoubleTime, (postforkSize<<maxDoublings)+1));
    BOOST_CHECK(!TestCheckBlock(*pblock, farFuture, (postforkSize<<maxDoublings)+1));
#endif

    sizeForkTime.store(std::numeric_limits<uint64_t>::max());
}

// Test activation time 30 days after earliest possible:
BOOST_AUTO_TEST_CASE(BigBlockFork_Time2)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;

    uint64_t t = EARLIEST_FORK_TIME;
    uint64_t preforkSize = MAXSIZE_PREFORK;
    uint64_t postforkSize = MAXSIZE_POSTFORK;

    uint64_t tActivate = EARLIEST_FORK_TIME+60*60*24*30;
    sizeForkTime.store(tActivate);

    LOCK(cs_main);

    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    CBlock *pblock = &pblocktemplate->block;

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t, postforkSize)); // big : invalid

    // Exactly at activation time....
    BOOST_CHECK(TestCheckBlock(*pblock, tActivate, preforkSize)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, tActivate, postforkSize)); // big : valid
 
    // Halfway to first doubling IS after the activation time:
    uint64_t tHalf = t+SIZE_DOUBLE_EPOCH/2;
    BOOST_CHECK(TestCheckBlock(*pblock, tHalf, (3*postforkSize)/2));

    sizeForkTime.store(std::numeric_limits<uint64_t>::max());
}

// Test: no miner consensus, no big blocks:
BOOST_AUTO_TEST_CASE(BigBlockFork_NoActivation)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;

    uint64_t t = EARLIEST_FORK_TIME;
    uint64_t preforkSize = MAXSIZE_PREFORK;
    uint64_t postforkSize = MAXSIZE_POSTFORK;

    LOCK(cs_main);

    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    CBlock *pblock = &pblocktemplate->block;

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t, postforkSize)); // big : invalid

    uint64_t tHalf = t+SIZE_DOUBLE_EPOCH/2;
    BOOST_CHECK(!TestCheckBlock(*pblock, tHalf, (3*postforkSize)/2));
}

BOOST_AUTO_TEST_SUITE_END()
