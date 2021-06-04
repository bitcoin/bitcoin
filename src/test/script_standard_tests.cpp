// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(script_standard_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(dest_default_is_no_dest)
{
    CTxDestination dest;
    BOOST_CHECK(!IsValidDestination(dest));
}

BOOST_AUTO_TEST_CASE(script_standard_Solver_success)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript s;
    std::vector<std::vector<unsigned char> > solutions;

    // TxoutType::PUBKEY
    s.clear();
    s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::PUBKEY);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0]));

    // TxoutType::PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::PUBKEYHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0].GetID()));

    // TxoutType::SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::SCRIPTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(CScriptID(redeemScript)));

    // TxoutType::MULTISIG
    s.clear();
    s << OP_1 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::MULTISIG);
    BOOST_CHECK_EQUAL(solutions.size(), 4U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>({1}));
    BOOST_CHECK(solutions[1] == ToByteVector(pubkeys[0]));
    BOOST_CHECK(solutions[2] == ToByteVector(pubkeys[1]));
    BOOST_CHECK(solutions[3] == std::vector<unsigned char>({2}));

    s.clear();
    s << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        ToByteVector(pubkeys[2]) <<
        OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::MULTISIG);
    BOOST_CHECK_EQUAL(solutions.size(), 5U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>({2}));
    BOOST_CHECK(solutions[1] == ToByteVector(pubkeys[0]));
    BOOST_CHECK(solutions[2] == ToByteVector(pubkeys[1]));
    BOOST_CHECK(solutions[3] == ToByteVector(pubkeys[2]));
    BOOST_CHECK(solutions[4] == std::vector<unsigned char>({3}));

    // TxoutType::NULL_DATA
    s.clear();
    s << OP_RETURN <<
        std::vector<unsigned char>({0}) <<
        std::vector<unsigned char>({75}) <<
        std::vector<unsigned char>({255});
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NULL_DATA);
    BOOST_CHECK_EQUAL(solutions.size(), 0U);

    // TxoutType::WITNESS_V0_KEYHASH
    s.clear();
    s << OP_0 << ToByteVector(pubkeys[0].GetID());
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_V0_KEYHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(pubkeys[0].GetID()));

    // TxoutType::WITNESS_V0_SCRIPTHASH
    uint256 scriptHash;
    CSHA256().Write(redeemScript.data(), redeemScript.size())
        .Finalize(scriptHash.begin());

    s.clear();
    s << OP_0 << ToByteVector(scriptHash);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_V0_SCRIPTHASH);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(scriptHash));

    // TxoutType::WITNESS_V1_TAPROOT
    s.clear();
    // SYSCOIN
    s << OP_1 << ToByteVector(uint256::ZEROV);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_V1_TAPROOT);
    BOOST_CHECK_EQUAL(solutions.size(), 1U);
    BOOST_CHECK(solutions[0] == ToByteVector(uint256::ZEROV));

    // TxoutType::WITNESS_UNKNOWN
    s.clear();
    s << OP_16 << ToByteVector(uint256::ONEV);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::WITNESS_UNKNOWN);
    BOOST_CHECK_EQUAL(solutions.size(), 2U);
    BOOST_CHECK(solutions[0] == std::vector<unsigned char>{16});
    BOOST_CHECK(solutions[1] == ToByteVector(uint256::ONEV));

    // TxoutType::NONSTANDARD
    s.clear();
    s << OP_9 << OP_ADD << OP_11 << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);
}

BOOST_AUTO_TEST_CASE(script_standard_Solver_failure)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    std::vector<std::vector<unsigned char> > solutions;

    // TxoutType::PUBKEY with incorrectly sized pubkey
    s.clear();
    s << std::vector<unsigned char>(30, 0x01) << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::PUBKEYHASH with incorrectly sized key hash
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::SCRIPTHASH with incorrectly sized script hash
    s.clear();
    s << OP_HASH160 << std::vector<unsigned char>(21, 0x01) << OP_EQUAL;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG 0/2
    s.clear();
    s << OP_0 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG 2/1
    s.clear();
    s << OP_2 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG n = 2 with 1 pubkey
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::MULTISIG n = 1 with 0 pubkeys
    s.clear();
    s << OP_1 << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::NULL_DATA with other opcodes
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75}) << OP_ADD;
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);

    // TxoutType::WITNESS_UNKNOWN with incorrect program size
    s.clear();
    s << OP_0 << std::vector<unsigned char>(19, 0x01);
    BOOST_CHECK_EQUAL(Solver(s, solutions), TxoutType::NONSTANDARD);
}

BOOST_AUTO_TEST_CASE(script_standard_ExtractDestination)
{
    CKey key;
    CPubKey pubkey;
    key.MakeNewKey(true);
    pubkey = key.GetPubKey();

    CScript s;
    CTxDestination address;

    // TxoutType::PUBKEY
    s.clear();
    s << ToByteVector(pubkey) << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<PKHash>(address) == PKHash(pubkey));

    // TxoutType::PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<PKHash>(address) == PKHash(pubkey));

    // TxoutType::SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<ScriptHash>(address) == ScriptHash(redeemScript));

    // TxoutType::MULTISIG
    s.clear();
    s << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG;
    BOOST_CHECK(!ExtractDestination(s, address));

    // TxoutType::NULL_DATA
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75});
    BOOST_CHECK(!ExtractDestination(s, address));

    // TxoutType::WITNESS_V0_KEYHASH
    s.clear();
    s << OP_0 << ToByteVector(pubkey.GetID());
    BOOST_CHECK(ExtractDestination(s, address));
    WitnessV0KeyHash keyhash;
    CHash160().Write(pubkey).Finalize(keyhash);
    BOOST_CHECK(std::get<WitnessV0KeyHash>(address) == keyhash);

    // TxoutType::WITNESS_V0_SCRIPTHASH
    s.clear();
    WitnessV0ScriptHash scripthash;
    CSHA256().Write(redeemScript.data(), redeemScript.size()).Finalize(scripthash.begin());
    s << OP_0 << ToByteVector(scripthash);
    BOOST_CHECK(ExtractDestination(s, address));
    BOOST_CHECK(std::get<WitnessV0ScriptHash>(address) == scripthash);

    // TxoutType::WITNESS_UNKNOWN with unknown version
    s.clear();
    s << OP_1 << ToByteVector(pubkey);
    BOOST_CHECK(ExtractDestination(s, address));
    WitnessUnknown unk;
    unk.length = 33;
    unk.version = 1;
    std::copy(pubkey.begin(), pubkey.end(), unk.program);
    BOOST_CHECK(std::get<WitnessUnknown>(address) == unk);
}

BOOST_AUTO_TEST_CASE(script_standard_ExtractDestinations)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript s;
    TxoutType whichType;
    std::vector<CTxDestination> addresses;
    int nRequired;

    // TxoutType::PUBKEY
    s.clear();
    s << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TxoutType::PUBKEY);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(std::get<PKHash>(addresses[0]) == PKHash(pubkeys[0]));

    // TxoutType::PUBKEYHASH
    s.clear();
    s << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TxoutType::PUBKEYHASH);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(std::get<PKHash>(addresses[0]) == PKHash(pubkeys[0]));

    // TxoutType::SCRIPTHASH
    CScript redeemScript(s); // initialize with leftover P2PKH script
    s.clear();
    s << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TxoutType::SCRIPTHASH);
    BOOST_CHECK_EQUAL(addresses.size(), 1U);
    BOOST_CHECK_EQUAL(nRequired, 1);
    BOOST_CHECK(std::get<ScriptHash>(addresses[0]) == ScriptHash(redeemScript));

    // TxoutType::MULTISIG
    s.clear();
    s << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK(ExtractDestinations(s, whichType, addresses, nRequired));
    BOOST_CHECK_EQUAL(whichType, TxoutType::MULTISIG);
    BOOST_CHECK_EQUAL(addresses.size(), 2U);
    BOOST_CHECK_EQUAL(nRequired, 2);
    BOOST_CHECK(std::get<PKHash>(addresses[0]) == PKHash(pubkeys[0]));
    BOOST_CHECK(std::get<PKHash>(addresses[1]) == PKHash(pubkeys[1]));

    // TxoutType::NULL_DATA
    s.clear();
    s << OP_RETURN << std::vector<unsigned char>({75});
    BOOST_CHECK(!ExtractDestinations(s, whichType, addresses, nRequired));
}

BOOST_AUTO_TEST_CASE(script_standard_GetScriptFor_)
{
    CKey keys[3];
    CPubKey pubkeys[3];
    for (int i = 0; i < 3; i++) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
    }

    CScript expected, result;

    // PKHash
    expected.clear();
    expected << OP_DUP << OP_HASH160 << ToByteVector(pubkeys[0].GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    result = GetScriptForDestination(PKHash(pubkeys[0]));
    BOOST_CHECK(result == expected);

    // CScriptID
    CScript redeemScript(result);
    expected.clear();
    expected << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;
    result = GetScriptForDestination(ScriptHash(redeemScript));
    BOOST_CHECK(result == expected);

    // CNoDestination
    expected.clear();
    result = GetScriptForDestination(CNoDestination());
    BOOST_CHECK(result == expected);

    // GetScriptForRawPubKey
    expected.clear();
    expected << ToByteVector(pubkeys[0]) << OP_CHECKSIG;
    result = GetScriptForRawPubKey(pubkeys[0]);
    BOOST_CHECK(result == expected);

    // GetScriptForMultisig
    expected.clear();
    expected << OP_2 <<
        ToByteVector(pubkeys[0]) <<
        ToByteVector(pubkeys[1]) <<
        ToByteVector(pubkeys[2]) <<
        OP_3 << OP_CHECKMULTISIG;
    result = GetScriptForMultisig(2, std::vector<CPubKey>(pubkeys, pubkeys + 3));
    BOOST_CHECK(result == expected);

    // WitnessV0KeyHash
    expected.clear();
    expected << OP_0 << ToByteVector(pubkeys[0].GetID());
    result = GetScriptForDestination(WitnessV0KeyHash(Hash160(ToByteVector(pubkeys[0]))));
    BOOST_CHECK(result == expected);
    result = GetScriptForDestination(WitnessV0KeyHash(pubkeys[0].GetID()));
    BOOST_CHECK(result == expected);

    // WitnessV0ScriptHash (multisig)
    CScript witnessScript;
    witnessScript << OP_1 << ToByteVector(pubkeys[0]) << OP_1 << OP_CHECKMULTISIG;

    uint256 scriptHash;
    CSHA256().Write(witnessScript.data(), witnessScript.size())
        .Finalize(scriptHash.begin());

    expected.clear();
    expected << OP_0 << ToByteVector(scriptHash);
    result = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
    BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_CASE(script_standard_taproot_builder)
{
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,1}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,0,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,0,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,0,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,1,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,1,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,1,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,2,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,2,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({0,2,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,0,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,0,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,0,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,1,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,1,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,1,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,2,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,2,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({1,2,2}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,0,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,0,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,0,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,1,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,1,1}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,1,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,2,0}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,2,1}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,2,2}), false);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({2,2,2,3,4,5,6,7,8,9,10,11,12,14,14,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,31,31,31,31,31,31,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,128}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({128,128,127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1}), true);
    BOOST_CHECK_EQUAL(TaprootBuilder::ValidDepths({129,129,128,127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1}), false);

    XOnlyPubKey key_inner{ParseHex("79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")};
    XOnlyPubKey key_1{ParseHex("c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5")};
    XOnlyPubKey key_2{ParseHex("f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9")};
    CScript script_1 = CScript() << ToByteVector(key_1) << OP_CHECKSIG;
    CScript script_2 = CScript() << ToByteVector(key_2) << OP_CHECKSIG;
    uint256 hash_3 = uint256S("31fe7061656bea2a36aa60a2f7ef940578049273746935d296426dc0afd86b68");

    TaprootBuilder builder;
    BOOST_CHECK(builder.IsValid() && builder.IsComplete());
    builder.Add(2, script_2, 0xc0);
    BOOST_CHECK(builder.IsValid() && !builder.IsComplete());
    builder.AddOmitted(2, hash_3);
    BOOST_CHECK(builder.IsValid() && !builder.IsComplete());
    builder.Add(1, script_1, 0xc0);
    BOOST_CHECK(builder.IsValid() && builder.IsComplete());
    builder.Finalize(key_inner);
    BOOST_CHECK(builder.IsValid() && builder.IsComplete());
    // SYSCOIN
    BOOST_CHECK_EQUAL(EncodeDestination(builder.GetOutput()), "sys1pj6gaw944fy0xpmzzu45ugqde4rz7mqj5kj0tg8kmr5f0pjq8vnaqjvkpnk");
}

BOOST_AUTO_TEST_SUITE_END()
