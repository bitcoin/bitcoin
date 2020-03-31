// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(simplesigner_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(simplesigner_base_tests)
{
    uint256 message = uint256S("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    CKey privkey;
    privkey.MakeNewKey(true);
    CPubKey pubkey = privkey.GetPubKey();

    // Sign `message` with `privkey`, associated with `pubkey`

    CKeyID keyid = pubkey.GetID();
    CScript script = GetScriptForDestination(PKHash(keyid));

    FillableSigningProvider signing_provider;
    signing_provider.AddKeyPubKey(privkey, pubkey);

    SimpleSignatureCreator creator(message);

    std::vector<unsigned char> sig;
    bool res = creator.CreateSig(signing_provider, sig, keyid, script, SigVersion::BASE);
    BOOST_CHECK(res);
    BOOST_CHECK(sig.size() > 0);

    // Verify `sig` for `message` and `pubkey`

    SimpleSignatureChecker checker(message);
    res = checker.CheckSig(sig, ToByteVector(pubkey), script, SigVersion::BASE);
    BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(simplesigner_witv0_tests)
{
    uint256 message = uint256S("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    CKey privkey;
    privkey.MakeNewKey(true);
    CPubKey pubkey = privkey.GetPubKey();

    // Sign `message` with `privkey`, associated with `pubkey`

    CKeyID keyid = pubkey.GetID();
    CScript script = GetScriptForDestination(WitnessV0KeyHash(keyid));

    FillableSigningProvider signing_provider;
    signing_provider.AddKeyPubKey(privkey, pubkey);

    SimpleSignatureCreator creator(message);

    std::vector<unsigned char> sig;
    bool res = creator.CreateSig(signing_provider, sig, keyid, script, SigVersion::WITNESS_V0);
    BOOST_CHECK(res);
    BOOST_CHECK(sig.size() > 0);

    // Verify `sig` for `message` and `pubkey`

    SimpleSignatureChecker checker(message);
    res = checker.CheckSig(sig, ToByteVector(pubkey), script, SigVersion::WITNESS_V0);
    BOOST_CHECK(res);
}

/* Based off of multisig_verify in multisig_tests.cpp */
BOOST_AUTO_TEST_CASE(simplesigner_multisig_tests)
{
    unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

    uint256 message = uint256S("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    FillableSigningProvider signing_provider;
    ScriptError err;
    CKey key[4];
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(true);
        signing_provider.AddKey(key[i]);
    }

    CScript a_and_b;
    a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript a_or_b;
    a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    CScript escrow;
    escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

    // Test a AND b:
    std::vector<unsigned char> sig1, sig2;
    SimpleSignatureCreator creator(message);
    SimpleSignatureChecker checker(message);
    bool res;
    CScript multisig;
    {
        res = creator.CreateSig(signing_provider, sig1, key[0].GetPubKey().GetID(), a_and_b, SigVersion::BASE);
        BOOST_CHECK(res);
        res = creator.CreateSig(signing_provider, sig2, key[1].GetPubKey().GetID(), a_and_b, SigVersion::BASE);
        BOOST_CHECK(res);
        multisig = CScript() << OP_0 << sig1 << sig2;
        res = VerifyScript(multisig, a_and_b, nullptr, flags, checker, &err);
        BOOST_CHECK_MESSAGE(res, ScriptErrorString(err));
    }

    for (int i = 0; i < 4; i++) {
        res = creator.CreateSig(signing_provider, sig1, key[i].GetPubKey().GetID(), a_and_b, SigVersion::BASE);
        BOOST_CHECK(res);
        multisig = CScript() << OP_0 << sig1;
        BOOST_CHECK_MESSAGE(!VerifyScript(multisig, a_and_b, nullptr, flags, checker, &err), strprintf("a&b 1: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));

        res = creator.CreateSig(signing_provider, sig1, key[1].GetPubKey().GetID(), a_and_b, SigVersion::BASE);
        BOOST_CHECK(res);
        res = creator.CreateSig(signing_provider, sig2, key[i].GetPubKey().GetID(), a_and_b, SigVersion::BASE);
        BOOST_CHECK(res);
        multisig = CScript() << OP_0 << sig1 << sig2;
        BOOST_CHECK_MESSAGE(!VerifyScript(multisig, a_and_b, nullptr, flags, checker, &err), strprintf("a&b 2: %d", i));
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
    }

    // Test a OR b:
    for (int i = 0; i < 4; i++) {
        res = creator.CreateSig(signing_provider, sig1, key[i].GetPubKey().GetID(), a_or_b, SigVersion::BASE);
        multisig = CScript() << OP_0 << sig1;
        if (i == 0 || i == 1) {
            BOOST_CHECK_MESSAGE(VerifyScript(multisig, a_or_b, nullptr, flags, checker, &err), strprintf("a|b: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
        } else {
            BOOST_CHECK_MESSAGE(!VerifyScript(multisig, a_or_b, nullptr, flags, checker, &err), strprintf("a|b: %d", i));
            BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
        }
    }
    multisig = CScript() << OP_0 << OP_1;
    BOOST_CHECK(!VerifyScript(multisig, a_or_b, nullptr, flags, checker, &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_SIG_DER, ScriptErrorString(err));

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            res = creator.CreateSig(signing_provider, sig1, key[i].GetPubKey().GetID(), escrow, SigVersion::BASE);
            BOOST_CHECK(res);
            res = creator.CreateSig(signing_provider, sig2, key[j].GetPubKey().GetID(), escrow, SigVersion::BASE);
            BOOST_CHECK(res);
            multisig = CScript() << OP_0 << sig1 << sig2;
            if (i < j && i < 3 && j < 3) {
                BOOST_CHECK_MESSAGE(VerifyScript(multisig, escrow, nullptr, flags, checker, &err), strprintf("escrow 1: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
            } else {
                BOOST_CHECK_MESSAGE(!VerifyScript(multisig, escrow, nullptr, flags, checker, &err), strprintf("escrow 2: %d %d", i, j));
                BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
