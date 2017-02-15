// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/generic.hpp"
#include "script/sign.h"

#include <boost/test/unit_test.hpp>

// Make sure the templates compile and run by at least running a templated test
template<typename T>
void SingleSignerTemplateTest(unsigned int flags, const T& data)
{
    CBasicKeyStore keystore;
    SimpleSignatureCreator simpleSignatureCreator(&keystore, SerializeHash(data));
    std::vector<unsigned char> vchScriptSig;
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    keystore.AddKeyPubKey(key, pubkey);
    const CScript scriptCode;
    SigVersion sigversion = SIGVERSION_WITNESS_V0;
    const std::vector<unsigned char> vchPubKey = ToByteVector(pubkey);   

    BOOST_CHECK(simpleSignatureCreator.CreateSig(vchScriptSig, pubkey.GetID(), scriptCode, sigversion));
    BOOST_CHECK(simpleSignatureCreator.Checker().CheckSig(vchScriptSig, vchPubKey, scriptCode, sigversion));
}

BOOST_AUTO_TEST_CASE(GenericSignTests)
{
    ECC_Start();

    unsigned int flags = SCRIPT_VERIFY_NONE;

    uint256 hash;
    COutPoint outPoint;
    CTxIn txIn;
    CTxOut txOut;
    CTransaction transaction;
    CBlockHeader header;
    CBlock block;

    SingleSignerTemplateTest(flags, hash);
    SingleSignerTemplateTest(flags, outPoint);
    SingleSignerTemplateTest(flags, txIn);
    SingleSignerTemplateTest(flags, txOut);
    SingleSignerTemplateTest(flags, transaction);
    SingleSignerTemplateTest(flags, header);
    SingleSignerTemplateTest(flags, block);

    ECC_Stop();
}
