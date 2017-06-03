// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "keystore.h"
#include "keystore.h"
#include "pow.h"
#include "primitives/block.h"
#include "script/generic.hpp"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

static bool SignMaybeGenerateProof(const Consensus::Params& params, CBlockHeader& block, const CKeyStore& keystore)
{
    uint64_t nTries = 1;
    return MaybeGenerateProof(params, &block, &keystore, nTries);
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

void SingleBlockSignerTest(const Consensus::Params& params, unsigned int flags, CBlockHeader& block)
{
    ResetProof(params, &block);
    // BOOST_CHECK(GenerateProof(params, &block));
    CValidationState state;
    // const uint256 block_hash = block.GetHash();
    // BOOST_CHECK(CheckProof(params, block, block_hash, state));
}

BOOST_AUTO_TEST_CASE(BlockSignBasicTests)
{
    ECC_Start();

    CBlockHeader header;
    CBlockHeader block;
    uint256 block_hash;
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
    BOOST_CHECK(key.IsCompressed());
    CPubKey pubkey = key.GetPubKey();
    keystore.AddKeyPubKey(key, pubkey);
    SigVersion sigversion = SIGVERSION_WITNESS_V0;
    const std::vector<unsigned char> vchPubKey = ToByteVector(pubkey);
    std::vector<unsigned char> vchScriptSig;
    SimpleSignatureCreator simpleSignatureCreator(&keystore, SerializeHash(block));

    CScript scriptCode;
    BOOST_CHECK(simpleSignatureCreator.CreateSig(vchScriptSig, pubkey.GetID(), scriptCode, sigversion));
    BOOST_CHECK(simpleSignatureCreator.Checker().CheckSig(vchScriptSig, vchPubKey, scriptCode, sigversion));

    scriptPubKey = CScript() << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    // BOOST_CHECK(ProduceSignature(simpleSignatureCreator, scriptPubKey, scriptSigData));
    // BOOST_CHECK(VerifyScript(scriptSigData.scriptSig, scriptPubKey, NULL, flags, simpleSignatureCreator.Checker()));

    // BOOST_CHECK(GenericSignScript(keystore, block, scriptPubKey, scriptSigData));
    // BOOST_CHECK(GenericVerifyScript(scriptSigData.scriptSig, scriptPubKey, flags, block));

    // --------------

    ArgsManager test_args;
    CValidationState state;
    std::unique_ptr<CChainParams> test_params;

    // Create a key to sign
    key.MakeNewKey(true);

    test_args.ForceSetArg("-con_blocksignscript", SingleSignerScriptStrFromKey(key));
    test_params = CreateChainParams(CBaseChainParams::CUSTOM, test_args);

    ResetProof(test_params->GetConsensus(), &block);
    // Things should fail with an empty unsigned block
    block_hash = block.GetHash();
    BOOST_CHECK(!CheckProof(block, block_hash, state, test_params->GetConsensus()));
    BOOST_CHECK(!SignMaybeGenerateProof(test_params->GetConsensus(), block, keystore));

    keystore.AddKey(key);
    // BOOST_CHECK(SignMaybeGenerateProof(test_params->GetConsensus(), block, keystore));

    SingleBlockSignerTest(test_params->GetConsensus(), flags, header);
    SingleBlockSignerTest(test_params->GetConsensus(), flags, block);

    ECC_Stop();
}
