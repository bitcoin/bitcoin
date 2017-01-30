// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h" // For CBitcoinSecret
#include "chain.h"
#include "chainparams.h"
#include "consensus/header_verify.h"
#include "consensus/validation.h"
#include "keystore.h"
#include "primitives/block.h"
#include "script/generic.hpp"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

static bool fSignBlocksGlobalRestore = fSignBlocksGlobal;

static void start_blocksing_test()
{
    ECC_Start();
    fSignBlocksGlobalRestore = fSignBlocksGlobal; // TODO signblocks: This is ugly
    fSignBlocksGlobal = true; // Set global from primitives/block to true for these tests
}

static void stop_blocksing_test()
{
    fSignBlocksGlobal = fSignBlocksGlobalRestore; // Restore global
    ECC_Stop();
}

static std::string SingleSignerScriptStrFromKey(const CKey& key)
{
    std::string strPubkey = HexStr(key.GetPubKey());
    CScript scriptPubKey = CScript() << ParseHex(strPubkey) << OP_CHECKSIG;
    std::string strScript = HexStr(scriptPubKey);
    // TOOLING: Uncomment to create params for a private chain with a single signer
    // BOOST_CHECK_EQUAL("-con_signblockscript", strScript);
    // std::unique_ptr<CChainParams> chainparams = CChainParams::Factory(CBaseChainParams::CUSTOM);
    // BOOST_CHECK_EQUAL("importprivkey", CBitcoinSecret(key, *chainparams).ToString());
    // BOOST_CHECK_EQUAL("pubkey_for_multisig", strPubkey);

    return strScript;
}

static std::string MultiSignerScriptStrFromPubKeys(const CPubKey* pubkeys, uint32_t min, uint32_t max)
{
    BOOST_CHECK(min <= max);
    CScript multisigScriptPubKey = CScript() << OP_2; // TODO Replace 2 with min

    for (unsigned i = 0; i < max; ++i) {
        multisigScriptPubKey << ParseHex(HexStr(pubkeys[i]));
    }
    multisigScriptPubKey << OP_3 << OP_CHECKMULTISIG; // TODO Replace 3 with max
    return HexStr(multisigScriptPubKey);
}

BOOST_AUTO_TEST_CASE(GenericSignWithRegularBlocks)
{
    ECC_Start();

    CBlockHeader block;
    CScript scriptSig;
    CScript scriptPubKey;
    unsigned int flags = SCRIPT_VERIFY_NONE;
    CBasicKeyStore keystore;
    SignatureData scriptSigData;
    SignatureData scriptSig1;
    SignatureData scriptSig2;

    // Make sure that the generic templates compile for CBlockHeader
    SignatureData scriptExpected = GenericCombineSignatures(scriptPubKey, block, scriptSig1, scriptSig2);
    BOOST_CHECK(!GenericSignScript(keystore, block, scriptPubKey, scriptSigData));
    BOOST_CHECK(!GenericVerifyScript(scriptSig, scriptPubKey, flags, block));

    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    keystore.AddKeyPubKey(key, pubkey);
    const CScript scriptCode;
    SigVersion sigversion = SIGVERSION_WITNESS_V0;
    const std::vector<unsigned char> vchPubKey = ToByteVector(pubkey);
    std::vector<unsigned char> vchScriptSig;
    SimpleSignatureCreator simpleSignatureCreator(&keystore, SerializeHash(block));

    BOOST_CHECK(simpleSignatureCreator.CreateSig(vchScriptSig, pubkey.GetID(), scriptCode, sigversion));
    BOOST_CHECK(simpleSignatureCreator.Checker().CheckSig(vchScriptSig, vchPubKey, scriptCode, sigversion));

    scriptPubKey << ParseHex(HexStr(pubkey)) << OP_CHECKSIG;
    BOOST_CHECK(ProduceSignature(simpleSignatureCreator, scriptPubKey, scriptSigData));
    BOOST_CHECK(VerifyScript(scriptSigData.scriptSig, scriptPubKey, NULL, flags, simpleSignatureCreator.Checker()));

    BOOST_CHECK(GenericSignScript(keystore, block, scriptPubKey, scriptSigData));
    BOOST_CHECK(GenericVerifyScript(scriptSigData.scriptSig, scriptPubKey, flags, block));

    ECC_Stop();
}

BOOST_AUTO_TEST_CASE(BasicSignBlock)
{
    start_blocksing_test();
    fSignBlocksGlobal = true;
    BOOST_CHECK(fSignBlocksGlobal);

    // Generate and check proofs on custom blocksigned chains
    CBasicKeyStore keystore;
    CBlockHeader block;
    CBlockIndex indexPrev;
    CValidationState state;
    ArgsManager testArgs;
    testArgs.ForceSetArg("-con_signblockscript", "1");
    BOOST_CHECK(testArgs.IsArgSet("-con_signblockscript"));
    BOOST_CHECK(testArgs.GetBoolArg("-con_signblockscript", false));
    std::unique_ptr<CChainParams> chainparams = CreateChainParams(CBaseChainParams::CUSTOM, testArgs);

    // TODO signblocks: Make sure BasicSignBlock is independent from GenericSignWithRegularBlocks

    // Default -con_signblockscript with -con_fsignblockchain=1 is OP_TRUE

    // BOOST_CHECK(CheckChallenge(chainparams->GetConsensus(), state, &block, &indexPrev)); // Should pass always for signed blocks
    ResetChallenge(chainparams->GetConsensus(), &block, &indexPrev); // Shouldn't do anything
    BOOST_CHECK(CheckChallenge(chainparams->GetConsensus(), state, &block, &indexPrev)); // Should pass always for signed blocks

    ResetProof(chainparams->GetConsensus(), &block);
    // BOOST_CHECK(CheckProof(chainparams->GetConsensus(), block)); // Should pass with an empty script for scriptpubKey=OP_TRUE
    BOOST_CHECK(!GenerateProof(chainparams->GetConsensus(), &block, &keystore)); // But OP_TRUE is not a standard output type

    // Also OP_TRUE-chain from custom -con_signblockscript

    testArgs.ForceSetArg("-con_signblockscript", HexStr(CScript(OP_TRUE)));
    // chainparams = CreateChainParams(CBaseChainParams::CUSTOM, testArgs);
    ResetProof(chainparams->GetConsensus(), &block);
    // BOOST_CHECK(CheckProof(chainparams->GetConsensus(), block)); // Should pass with an empty script for scriptpubKey=OP_TRUE
    BOOST_CHECK(!GenerateProof(chainparams->GetConsensus(), &block, &keystore)); // But OP_TRUE is not a standard output type

    // Also Choose one signing key

    CKey key;
    key.MakeNewKey(true);
    testArgs.ForceSetArg("-con_signblockscript", SingleSignerScriptStrFromKey(key));
    // chainparams = CreateChainParams(CBaseChainParams::CUSTOM, testArgs);

    ResetProof(chainparams->GetConsensus(), &block);
    // BOOST_CHECK(!CheckProof(chainparams->GetConsensus(), block)); // Should not pass without generating the proof
    BOOST_CHECK(!GenerateProof(chainparams->GetConsensus(), &block, &keystore)); // Should not be able to generate the proof without the key
    keystore.AddKey(key); // Should pass after adding the key
    // TODO signblocks: These 2 seems to be dependent on GenericSignWithRegularBlocks
    // BOOST_CHECK(GenerateProof(chainparams->GetConsensus(), &block, &keystore));
    // BOOST_CHECK(CheckProof(chainparams->GetConsensus(), block));

    // Also test multisig scripts

    const unsigned MAX_MULTISIG_AGENTS = 5;
    CKey keys[MAX_MULTISIG_AGENTS];
    CPubKey pubkeys[MAX_MULTISIG_AGENTS];
    for (unsigned i = 0; i < MAX_MULTISIG_AGENTS; ++i) {
        keys[i].MakeNewKey(true);
        pubkeys[i] = keys[i].GetPubKey();
        keystore.AddKey(keys[i]);
    }
    testArgs.ForceSetArg("-con_signblockscript", MultiSignerScriptStrFromPubKeys(pubkeys, 2, 3));
    // chainparams = CreateChainParams(CBaseChainParams::CUSTOM, testArgs);

    // TODO signblocks: tests for multisig
    
    stop_blocksing_test();
}
