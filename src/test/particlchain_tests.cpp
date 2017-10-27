// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_particl.h"
#include "net.h"
#include "keystore.h"
#include "script/script.h"
#include "script/ismine.h"
#include "consensus/merkle.h"
#include "key/extkey.h"
#include "pos/kernel.h"

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

    CDataStream ss(SER_DISK, 0);

    ss << blk;
    ss >> blkOut;

    BOOST_CHECK(blk.vtx[0]->nVersion == blkOut.vtx[0]->nVersion);
}

BOOST_AUTO_TEST_CASE(signature_test)
{
    SeedInsecureRand();
    CBasicKeyStore keystore;

    CKey k;
    InsecureNewKey(k, true);
    keystore.AddKey(k);

    CPubKey pk = k.GetPubKey();
    CKeyID id = pk.GetID();

    CAmount nValue = 100;
    SignatureData sigdata;

    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    txn.nLockTime = 0;

    int nBlockHeight = 22;
    OUTPUT_PTR<CTxOutData> out0 = MAKE_OUTPUT<CTxOutData>();
    out0->vData = SetCompressedInt64(out0->vData, nBlockHeight);
    txn.vpout.push_back(out0);

    CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
    OUTPUT_PTR<CTxOutStandard> out1 = MAKE_OUTPUT<CTxOutStandard>();
    out1->nValue = 10000;
    out1->scriptPubKey = script;
    txn.vpout.push_back(out1);

    txn.vin.push_back(CTxIn(txn.GetHash(), 0));

    CMutableTransaction txn2;
    txn2.vin.push_back(CTxIn(txn.GetHash(), 1));

    nValue = out1->nValue;
    std::vector<uint8_t> vchAmount(8);
    memcpy(&vchAmount[0], &nValue, 8);

    CTransaction txToConst(txn2);
    BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keystore, &txToConst, 1, vchAmount, SIGHASH_ALL), script, sigdata));

}

BOOST_AUTO_TEST_CASE(particlchain_test)
{
    SeedInsecureRand();
    CBasicKeyStore keystore;

    CKey k;
    InsecureNewKey(k, true);
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
    txn.nLockTime = 0;
    OUTPUT_PTR<CTxOutStandard> out0 = MAKE_OUTPUT<CTxOutStandard>();
    out0->nValue = 10000;
    out0->scriptPubKey = script;
    txn.vpout.push_back(out0);


    blk.vtx.push_back(MakeTransactionRef(txn));

    bool mutated;
    blk.hashMerkleRoot = BlockMerkleRoot(blk, &mutated);
    blk.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(blk, &mutated);


    CDataStream ss(SER_DISK, 0);
    ss << blk;

    CBlock blkOut;
    ss >> blkOut;

    BOOST_CHECK(blk.hashMerkleRoot == blkOut.hashMerkleRoot);
    BOOST_CHECK(blk.hashWitnessMerkleRoot == blkOut.hashWitnessMerkleRoot);
    BOOST_CHECK(blk.nTime == blkOut.nTime && blkOut.nTime == 1487406900);

    BOOST_CHECK(TXN_COINBASE == blkOut.vtx[0]->GetType());

    CMutableTransaction txnSpend;

    txnSpend.nVersion = PARTICL_BLOCK_VERSION;
}

BOOST_AUTO_TEST_CASE(opiscoinstake_test)
{
    SeedInsecureRand();
    CBasicKeyStore keystoreA;
    CBasicKeyStore keystoreB;

    CKey kA, kB;
    InsecureNewKey(kA, true);
    keystoreA.AddKey(kA);

    CPubKey pkA = kA.GetPubKey();
    CKeyID idA = pkA.GetID();

    InsecureNewKey(kB, true);
    keystoreB.AddKey(kB);

    CPubKey pkB = kB.GetPubKey();
    CKeyID256 idB = pkB.GetID256();

    CScript scriptSignA = CScript() << OP_DUP << OP_HASH160 << ToByteVector(idA) << OP_EQUALVERIFY << OP_CHECKSIG;
    //CScript scriptSignB = CScript() << OP_DUP << OP_HASH160 << ToByteVector(idB) << OP_EQUALVERIFY << OP_CHECKSIG;
    CScript scriptSignB = CScript() << OP_DUP << OP_SHA256 << ToByteVector(idB) << OP_EQUALVERIFY << OP_CHECKSIG;

    CScript script = CScript()
        << OP_ISCOINSTAKE << OP_IF
        << OP_DUP << OP_HASH160 << ToByteVector(idA) << OP_EQUALVERIFY << OP_CHECKSIG
        << OP_ELSE
        << OP_DUP << OP_SHA256 << ToByteVector(idB) << OP_EQUALVERIFY << OP_CHECKSIG
        << OP_ENDIF;


    BOOST_CHECK(HasIsCoinstakeOp(script));

    BOOST_CHECK(!IsSpendScriptP2PKH(script));


    CScript scriptFail1 = CScript()
        << OP_ISCOINSTAKE << OP_IF
        << OP_DUP << OP_HASH160 << ToByteVector(idA) << OP_EQUALVERIFY << OP_CHECKSIG
        << OP_ELSE
        << OP_DUP << OP_HASH160 << ToByteVector(idA) << OP_EQUALVERIFY << OP_CHECKSIG
        << OP_ENDIF;
    BOOST_CHECK(IsSpendScriptP2PKH(scriptFail1));


    CScript scriptTest, scriptTestB;
    BOOST_CHECK(GetCoinstakeScriptPath(script, scriptTest));
    BOOST_CHECK(scriptTest == scriptSignA);


    BOOST_CHECK(GetNonCoinstakeScriptPath(script, scriptTest));
    BOOST_CHECK(scriptTest == scriptSignB);


    BOOST_CHECK(SplitConditionalCoinstakeScript(script, scriptTest, scriptTestB));
    BOOST_CHECK(scriptTest == scriptSignA);
    BOOST_CHECK(scriptTestB == scriptSignB);


    txnouttype whichType;
    // IsStandard should fail until chain time is >= OpIsCoinstakeTime
    BOOST_CHECK(!IsStandard(script, whichType, true));


    BOOST_CHECK(IsMine(keystoreB, script));
    BOOST_CHECK(IsMine(keystoreA, script));


    CAmount nValue = 100000;
    SignatureData sigdataA, sigdataB, sigdataC;

    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    txn.SetType(TXN_COINSTAKE);
    txn.nLockTime = 0;

    int nBlockHeight = 1;
    std::shared_ptr<CTxOutData> outData = MAKE_OUTPUT<CTxOutData>();
    outData->vData.resize(4);
    memcpy(&outData->vData[0], &nBlockHeight, 4);
    txn.vpout.push_back(outData);


    OUTPUT_PTR<CTxOutStandard> out0 = MAKE_OUTPUT<CTxOutStandard>();
    out0->nValue = nValue;
    out0->scriptPubKey = script;
    txn.vpout.push_back(out0);
    txn.vin.push_back(CTxIn(COutPoint(uint256S("d496208ea84193e0c5ed05ac708aec84dfd2474b529a7608b836e282958dc72b"), 0)));
    CTransaction txnConst(txn);
    BOOST_CHECK(txnConst.IsCoinStake());

    std::vector<uint8_t> vchAmount(8);
    memcpy(&vchAmount[0], &nValue, 8);



    BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keystoreA, &txnConst, 0, vchAmount, SIGHASH_ALL), script, sigdataA));
    BOOST_CHECK(!ProduceSignature(TransactionSignatureCreator(&keystoreB, &txnConst, 0, vchAmount, SIGHASH_ALL), script, sigdataB));


    ScriptError serror = SCRIPT_ERR_OK;
    int nFlags = STANDARD_SCRIPT_VERIFY_FLAGS;
    CScript scriptSig;
    BOOST_CHECK(VerifyScript(scriptSig, script, &sigdataA.scriptWitness, nFlags, TransactionSignatureChecker(&txnConst, 0, vchAmount), &serror));


    txn.nVersion = PARTICL_TXN_VERSION;
    txn.SetType(TXN_STANDARD);
    CTransaction txnConst2(txn);
    BOOST_CHECK(!txnConst2.IsCoinStake());

    // This should fail anyway as the txn changed
    BOOST_CHECK(!VerifyScript(scriptSig, script, &sigdataA.scriptWitness, nFlags, TransactionSignatureChecker(&txnConst2, 0, vchAmount), &serror));

    BOOST_CHECK(!ProduceSignature(TransactionSignatureCreator(&keystoreA, &txnConst2, 0, vchAmount, SIGHASH_ALL), script, sigdataC));
    BOOST_CHECK(ProduceSignature(TransactionSignatureCreator(&keystoreB, &txnConst2, 0, vchAmount, SIGHASH_ALL), script, sigdataB));

    BOOST_CHECK(VerifyScript(scriptSig, script, &sigdataB.scriptWitness, nFlags, TransactionSignatureChecker(&txnConst2, 0, vchAmount), &serror));

}


BOOST_AUTO_TEST_SUITE_END()
