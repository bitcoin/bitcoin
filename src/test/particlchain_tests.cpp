// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_particl.h"
#include "net.h"
#include "keystore.h"
#include "script/script.h"
#include "consensus/merkle.h"
#include "key/extkey.h"

#include "script/sign.h"
#include "policy/policy.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(particlchain_tests, BasicTestingSetup)


BOOST_AUTO_TEST_CASE(oldversion_test)
{
    CBlock blk, blkOut;
    blk.nTime = 1487406900;
    
    CMutableTransaction txn;
    blk.vtx.push_back(MakeTransactionRef(txn));
    //BOOST_MESSAGE("blk.vtx[0]->nVersion "  << blk.vtx[0]->nVersion);
    
    CDataStream ss(SER_DISK, 0);
    
    ss << blk;
    ss >> blkOut;
    
    //BOOST_MESSAGE("blkOut.vtx[0]->nVersion "  << blkOut.vtx[0]->nVersion);
    
    BOOST_CHECK(blk.vtx[0]->nVersion == blkOut.vtx[0]->nVersion);
}

BOOST_AUTO_TEST_CASE(signature_test)
{
    CBasicKeyStore keystore;
    
    CKey k;
    k.MakeNewKey(true);
    keystore.AddKey(k);
    
    CPubKey pk = k.GetPubKey();
    CKeyID id = pk.GetID();
    
    CAmount nValue = 100; // used for???
    SignatureData sigdata;
    
    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    //txn.SetType(TXN_COINBASE);
    //txn.nTime = 1487406824;
    txn.nLockTime = 0;
    
    int nBlockHeight = 22;
    OUTPUT_PTR<CTxOutData> out0 = MAKE_OUTPUT<CTxOutData>();
    out0->vData = SetCompressedInt64(out0->vData, nBlockHeight);
    txn.vpout.push_back(out0);
    
    CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
    //script << OP_RETURN << nBlockHeight;
    OUTPUT_PTR<CTxOutStandard> out1 = MAKE_OUTPUT<CTxOutStandard>();
    out1->nValue = 10000;
    out1->scriptPubKey = script;
    txn.vpout.push_back(out1);
    
    txn.vin.push_back(CTxIn(txn.GetHash(), 0)); // needed for SignatureHash, why?
    
    
    
    CMutableTransaction txn2;
    txn2.vin.push_back(CTxIn(txn.GetHash(), 1));
    
    nValue = out1->nValue;
    std::vector<uint8_t> vchAmount(8);
    memcpy(&vchAmount[0], &nValue, 8);
    
    CTransaction txToConst(txn2);
    BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keystore, &txToConst, 1, vchAmount, SIGHASH_ALL), script, sigdata));
    
    //BOOST_MESSAGE("sigdata.scriptSig.size() " << sigdata.scriptSig.size());
    //BOOST_MESSAGE("sigdata.scriptWitness.stack.size() " << sigdata.scriptWitness.stack.size());
    
    //BOOST_MESSAGE("scriptSig " << HexStr(sigdata.scriptSig.begin(), sigdata.scriptSig.end()));
    
    /*
    
    if (nIn == 0) // append serialised block height
    {
        sigdata.scriptSig << OP_RETURN << nBlockHeight;
    };
    
    UpdateTransaction(txNew, nIn, sigdata);
    */
    
}

BOOST_AUTO_TEST_CASE(particlchain_test)
{
    CBasicKeyStore keystore;
    
    CKey k;
    k.MakeNewKey(true);
    keystore.AddKey(k);
    
    CPubKey pk = k.GetPubKey();
    CKeyID id = pk.GetID();
    
    CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
    
    CBlock blk;
    blk.nVersion = PARTICL_BLOCK_VERSION;
    blk.nTime = 1487406900;
    
    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    txn.SetType(TXN_COINBASE);
    //txn.nTime = 1487406824;
    txn.nLockTime = 0;
    OUTPUT_PTR<CTxOutStandard> out0 = MAKE_OUTPUT<CTxOutStandard>();
    out0->nValue = 10000;
    out0->scriptPubKey = script;
    txn.vpout.push_back(out0);
    
    
    blk.vtx.push_back(MakeTransactionRef(txn));
    
    BOOST_MESSAGE("blk.vtx.size() " << blk.vtx.size());
    
    bool mutated;
    blk.hashMerkleRoot = BlockMerkleRoot(blk, &mutated);
    //BOOST_CHECK(mutated == false);
    blk.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(blk, &mutated);
    //BOOST_CHECK(mutated == false);
    
    BOOST_MESSAGE("blk.hashMerkleRoot " << blk.hashMerkleRoot.ToString());
    BOOST_MESSAGE("blk.hashWitnessMerkleRoot " << blk.hashWitnessMerkleRoot.ToString());
    
    //BOOST_CHECK(blk.hashMerkleRoot == blk.hashWitnessMerkleRoot); // no inputs, no witness data, hashes should match
    
    
    CDataStream ss(SER_DISK, 0);
    
    ss << blk;
    
    BOOST_MESSAGE("blk " << HexStr(ss.begin(), ss.end()));
    
    CBlock blkOut;
    
    ss >> blkOut;
    
    BOOST_CHECK(blk.hashMerkleRoot == blkOut.hashMerkleRoot);
    BOOST_CHECK(blk.hashWitnessMerkleRoot == blkOut.hashWitnessMerkleRoot);
    BOOST_CHECK(blk.nTime == blkOut.nTime && blkOut.nTime == 1487406900);
    
    //BOOST_MESSAGE("blkOut.vtx.size() " << blkOut.vtx.size());
    //BOOST_MESSAGE("blkOut.vtx[0]->vpout.size() " << blkOut.vtx[0]->vpout.size());
    
    //BOOST_MESSAGE("blkOut.vtx[0].nVersion " << blkOut.vtx[0]->nVersion);
    
    
    //BOOST_MESSAGE("blkOut.vtx[0]->vpout[0]->nVersion " << blkOut.vtx[0]->vpout[0]->nVersion);
    
    BOOST_CHECK(TXN_COINBASE == blkOut.vtx[0]->GetType());
    
    
    
    //blkOut.vtx[0]->vpout[0]->scriptPubKey
    //HexStr
    
    
    CMutableTransaction txnSpend;
    
    txnSpend.nVersion = PARTICL_BLOCK_VERSION;
    
}


BOOST_AUTO_TEST_SUITE_END()
