// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "coins.h"
#include "consensus/validation.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestChain100Setup)
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;
    CMutableTransaction tx,tx2;
    CScript script;
    std::vector<unsigned char> vchSig;
    uint256 hash;
    std::vector<CMutableTransaction> noTxns;
    CBlock block;

    LOCK(cs_main);
    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    // Just to make sure we can still make simple blocks
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= 1000000;
        hash = tx.GetHash();
        mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
        tx.vin[0].prevout.hash = hash;
    }
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(520);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vout[0].nValue = 5000000000LL;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= 10000000;
        hash = tx.GetHash();
        mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
        tx.vin[0].prevout.hash = hash;
    }
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // orphan in mempool
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // child with higher priority than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = coinbaseTxns[1].GetHash();
    tx.vout[0].nValue = 4900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = 5900000000LL;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // coinbase in mempool
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // invalid (pre-p2sh) txn in mempool
    tx.vin[0].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << (std::vector<unsigned char>)script;
    tx.vout[0].nValue -= 1000000;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // double spend txn pair in mempool
    tx.vin[0].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    chainActive.Tip()->nHeight = 209999;
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    chainActive.Tip()->nHeight = 210000;
    BOOST_CHECK(pblocktemplate = CreateNewBlock(scriptPubKey));
    delete pblocktemplate;
    chainActive.Tip()->nHeight = nHeight;

 
    // non-final txs in mempool
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);

    // height locked
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = coinbaseTxns[0].GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.n = 0;
    tx.vin[0].nSequence = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 4900000000LL;
    tx.vout[0].scriptPubKey = scriptPubKey;
    tx.nLockTime = chainActive.Tip()->nHeight+1;	
    // Sign:
    vchSig.clear();
    hash = SignatureHash(scriptPubKey, tx, 0, SIGHASH_ALL);
    BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    tx.vin[0].scriptSig << vchSig;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(!CheckFinalTx(tx));
    


    // time locked
    tx2.vin.resize(1);
    tx2.vin[0].prevout.hash = hash;
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.n = 0;
    tx2.vin[0].nSequence = 0;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = scriptPubKey;
    tx2.vout[0].nValue = 4900000000LL;
    tx2.nLockTime = chainActive.Tip()->GetMedianTimePast()+1;
    // Sign:
    vchSig.clear();
    hash = SignatureHash(scriptPubKey, tx2, 0, SIGHASH_ALL);
    BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    tx2.vin[0].scriptSig << vchSig;
    hash = tx2.GetHash();
    mempool.addUnchecked(hash, CTxMemPoolEntry(tx2, 11, GetTime(), 111.0, 11));
    BOOST_CHECK(!CheckFinalTx(tx2));
    

    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());

    // Neither tx should have make it into the template.
    BOOST_CHECK_EQUAL(block.vtx.size(), 1);


    // However if we advance height and time by one, both will.
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+2);

    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(chainActive.Tip()->GetBlockHash() == block.GetHash());
    BOOST_CHECK_EQUAL(block.vtx.size(), 3);
    BOOST_CHECK(CheckFinalTx(tx));
    BOOST_CHECK(CheckFinalTx(tx2));

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
