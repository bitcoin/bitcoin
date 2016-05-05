// Copyright (c) 2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "consensus/consensus.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "random.h"
#include "uint256.h"
#include "util.h"

#include "test/test_bitcoin.h"

#include <boost/atomic.hpp>
#include <boost/test/unit_test.hpp>

extern boost::atomic<uint32_t> sizeForkTime;

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
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);
    CBlockTemplate *pblocktemplate;

    uint64_t t = GetTime();
    uint64_t preforkSize = OLD_MAX_BLOCK_SIZE;
    uint64_t postforkSize = MAX_BLOCK_SIZE;
    uint64_t tActivate = t;

    sizeForkTime.store(tActivate);

    LOCK(cs_main);

    Mining mining;
    mining.SetCoinbase(scriptPubKey);
    BOOST_CHECK(pblocktemplate = mining.CreateNewBlock(chainparams));
    CBlock *pblock = &pblocktemplate->block;

    // Before fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t-1LL, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t-1LL, preforkSize+1)); // >1MB : invalid
    BOOST_CHECK(!TestCheckBlock(*pblock, t-1LL, postforkSize)); // big : invalid

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, t, postforkSize)); // big : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t,  postforkSize+1)); // big+1 : invalid

    // After fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t+11000, preforkSize)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, t+11000, postforkSize)); // big : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t+11000,  postforkSize+1)); // big+1 : invalid

    sizeForkTime.store(std::numeric_limits<uint32_t>::max());
}

// Test activation time 30 days after earliest possible:
BOOST_AUTO_TEST_CASE(BigBlockFork_Time2)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);

    uint64_t t = GetTime();
    uint64_t preforkSize = OLD_MAX_BLOCK_SIZE;
    uint64_t postforkSize = MAX_BLOCK_SIZE;

    uint64_t tActivate = t+60*60*24*30;
    sizeForkTime.store(tActivate);

    LOCK(cs_main);

    Mining mining;
    mining.SetCoinbase(scriptPubKey);
    BOOST_CHECK(pblocktemplate = mining.CreateNewBlock(chainparams));
    CBlock *pblock = &pblocktemplate->block;

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t, postforkSize)); // big : invalid

    // Exactly at activation time....
    BOOST_CHECK(TestCheckBlock(*pblock, tActivate, preforkSize)); // 1MB : valid
    BOOST_CHECK(TestCheckBlock(*pblock, tActivate, postforkSize)); // big : valid
 
    sizeForkTime.store(std::numeric_limits<uint32_t>::max());
}

// Test: no miner consensus, no big blocks:
BOOST_AUTO_TEST_CASE(BigBlockFork_NoActivation)
{
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);

    uint64_t t = GetTime();
    uint64_t preforkSize = OLD_MAX_BLOCK_SIZE;
    uint64_t postforkSize = MAX_BLOCK_SIZE;

    LOCK(cs_main);

    Mining mining;
    mining.SetCoinbase(scriptPubKey);
    BOOST_CHECK(pblocktemplate = mining.CreateNewBlock(chainparams));
    CBlock *pblock = &pblocktemplate->block;

    // Exactly at fork time...
    BOOST_CHECK(TestCheckBlock(*pblock, t, preforkSize)); // 1MB : valid
    BOOST_CHECK(!TestCheckBlock(*pblock, t, postforkSize)); // big : invalid

    uint64_t tAfter = t+11000;
    BOOST_CHECK(!TestCheckBlock(*pblock, tAfter, postforkSize));
}

BOOST_AUTO_TEST_SUITE_END()
