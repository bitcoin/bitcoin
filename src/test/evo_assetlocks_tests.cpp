// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <amount.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <evo/assetlocktx.h>
#include <evo/specialtx.h>
#include <policy/settings.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <util/ranges_set.h>
#include <validation.h> // for ::ChainActive()

#include <boost/test/unit_test.hpp>


//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CMutableTransaction>
SetupDummyInputs(FillableSigningProvider& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    std::array<CKey, 4> key;
    {
        bool flip = true;
        for (auto& k : key) {
            k.MakeNewKey(flip);
            keystoreRet.AddKey(k);
            flip = !flip;
        }
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11*CENT;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    AddCoins(coinsRet, CTransaction(dummyTransactions[0]), 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(PKHash(key[2].GetPubKey()));
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(PKHash(key[3].GetPubKey()));
    AddCoins(coinsRet, CTransaction(dummyTransactions[1]), 0);

    return dummyTransactions;
}

static CMutableTransaction CreateAssetLockTx(FillableSigningProvider& keystore, CCoinsViewCache& coins, CKey& key)
{
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    std::vector<CTxOut> creditOutputs(2);
    creditOutputs[0].nValue = 17 * CENT;
    creditOutputs[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    creditOutputs[1].nValue = 13 * CENT;
    creditOutputs[1].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    CAssetLockPayload assetLockTx(creditOutputs);

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_ASSET_LOCK;
    SetTxPayload(tx, assetLockTx);

    tx.vin.resize(1);
    tx.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    tx.vin[0].prevout.n = 1;
    tx.vin[0].scriptSig << std::vector<unsigned char>(65, 0);

    tx.vout.resize(2);
    tx.vout[0].nValue = 30 * CENT;
    tx.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("");

    tx.vout[1].nValue = 20 * CENT;
    tx.vout[1].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    return tx;
}

static CMutableTransaction CreateAssetUnlockTx(FillableSigningProvider& keystore, CKey& key)
{
    int nVersion = 1;
    // just a big number bigger than uint32_t
    uint64_t index = 0x001122334455667788L;
    // big enough to overflow int32_t
    uint32_t fee = 2000'000'000u;
    // just big enough to overflow uint16_t
    uint32_t requestedHeight = 1000'000;
    uint256 quorumHash;
    CBLSSignature quorumSig;
    CAssetUnlockPayload assetUnlockTx(nVersion, index, fee, requestedHeight, quorumHash, quorumSig);

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_ASSET_UNLOCK;
    SetTxPayload(tx, assetUnlockTx);

    tx.vin.resize(0);

    tx.vout.resize(2);
    tx.vout[0].nValue = 10 * CENT;
    tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    tx.vout[1].nValue = 20 * CENT;
    tx.vout[1].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    return tx;
}

BOOST_FIXTURE_TEST_SUITE(evo_assetlocks_tests, TestChain100Setup)

BOOST_FIXTURE_TEST_CASE(evo_assetlock, TestChain100Setup)
{

    LOCK(cs_main);
    FillableSigningProvider keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);

    CKey key;
    key.MakeNewKey(true);

    const CMutableTransaction tx = CreateAssetLockTx(keystore, coins, key);
    std::string reason;
    BOOST_CHECK(IsStandardTx(CTransaction(tx), reason));

    TxValidationState tx_state;
    std::string strTest;
    BOOST_CHECK_MESSAGE(CheckTransaction(CTransaction(tx), tx_state), strTest);
    BOOST_CHECK(tx_state.IsValid());

    BOOST_CHECK(CheckAssetLockTx(CTransaction(tx), tx_state));

    BOOST_CHECK(AreInputsStandard(CTransaction(tx), coins));

    // Check version
    {
        BOOST_CHECK(tx.nVersion == 3);

        const auto opt_payload = GetTxPayload<CAssetLockPayload>(tx);

        BOOST_CHECK(opt_payload.has_value());
        BOOST_CHECK(opt_payload->getVersion() == 1);
    }

    {
        // Wrong type "Asset Unlock TX" instead "Asset Lock TX"
        CMutableTransaction txWrongType = tx;
        txWrongType.nType = TRANSACTION_ASSET_UNLOCK;
        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txWrongType), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-type");
    }

    {
        CAmount inSum = 0;
        for (const auto& vin : tx.vin) {
            inSum += coins.AccessCoin(vin.prevout).out.nValue;
        }

        auto outSum = CTransaction(tx).GetValueOut();
        BOOST_CHECK(inSum == outSum);

        // Outputs should not be bigger than inputs
        CMutableTransaction txBigOutput = tx;
        txBigOutput.vout[0].nValue += 1;
        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txBigOutput), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-creditamount");

        // Smaller outputs are allown
        CMutableTransaction txSmallOutput = tx;
        txSmallOutput.vout[1].nValue -= 1;
        BOOST_CHECK(CheckAssetLockTx(CTransaction(txSmallOutput), tx_state));
    }

    const auto assetLockPayload = GetTxPayload<CAssetLockPayload>(tx);
    const std::vector<CTxOut> creditOutputs = assetLockPayload->getCreditOutputs();

    {
        // Sum of credit output greater than OP_RETURN
        std::vector<CTxOut> wrongOutput = creditOutputs;
        wrongOutput[0].nValue += CENT;
        CAssetLockPayload greaterCreditsPayload(wrongOutput);

        CMutableTransaction txGreaterCredits = tx;
        SetTxPayload(txGreaterCredits, greaterCreditsPayload);

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txGreaterCredits), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-creditamount");

        // Sum of credit output less than OP_RETURN
        wrongOutput[1].nValue -= 2 * CENT;
        CAssetLockPayload lessCreditsPayload(wrongOutput);

        CMutableTransaction txLessCredits = tx;
        SetTxPayload(txLessCredits, lessCreditsPayload);

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txLessCredits), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-creditamount");
    }

    {
        // Credit output is out-of-range
        std::vector<CTxOut> creditOutputsOutOfRange = creditOutputs;
        creditOutputsOutOfRange[0].nValue = 0;
        CAssetLockPayload invalidOutputsPayload(creditOutputsOutOfRange);

        CMutableTransaction txInvalidOutputs = tx;
        SetTxPayload(txInvalidOutputs, invalidOutputsPayload);

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txInvalidOutputs), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-credit-outofrange");

        // one of output is out of range
        creditOutputsOutOfRange[0].nValue = MAX_MONEY + 1;
        SetTxPayload(txInvalidOutputs, CAssetLockPayload{creditOutputsOutOfRange});
        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txInvalidOutputs), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-credit-outofrange");

        // sum of some of output is out of range
        creditOutputsOutOfRange[0].nValue = MAX_MONEY + 1 - creditOutputsOutOfRange[1].nValue;
        SetTxPayload(txInvalidOutputs, CAssetLockPayload{creditOutputsOutOfRange});
        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txInvalidOutputs), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-credit-outofrange");

    }
    {
        // One credit output keys is not pub key
        std::vector<CTxOut> creditOutputsNotPubkey = creditOutputs;
        creditOutputsNotPubkey[0].scriptPubKey = CScript() << OP_1;
        CAssetLockPayload notPubkeyPayload(creditOutputsNotPubkey);

        CMutableTransaction txNotPubkey = tx;
        SetTxPayload(txNotPubkey, notPubkeyPayload);

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txNotPubkey), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-pubKeyHash");

    }

    {
        // OP_RETURN must be only one, not more
        CMutableTransaction txMultipleReturn = tx;
        txMultipleReturn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("");

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txMultipleReturn), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-multiple-return");

    }

    {
        // zero/negative OP_RETURN
        CMutableTransaction txReturnOutOfRange = tx;
        txReturnOutOfRange.vout[0].nValue = 0;

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txReturnOutOfRange), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-opreturn-outofrange");

        txReturnOutOfRange.vout[0].nValue = MAX_MONEY + 1;

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txReturnOutOfRange), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-opreturn-outofrange");
    }


    {
        // OP_RETURN is missing
        CMutableTransaction txNoReturn = tx;
        txNoReturn.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txNoReturn), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-no-return");
    }

    {
        // OP_RETURN should not have any data
        CMutableTransaction txReturnData = tx;
        txReturnData.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("abc");

        BOOST_CHECK(!CheckAssetLockTx(CTransaction(txReturnData), tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetlocktx-non-empty-return");
    }
}

BOOST_FIXTURE_TEST_CASE(evo_assetunlock, TestChain100Setup)
{
    LOCK(cs_main);
    FillableSigningProvider keystore;

    CKey key;
    key.MakeNewKey(true);

    const CMutableTransaction tx = CreateAssetUnlockTx(keystore, key);
    std::string reason;
    BOOST_CHECK(IsStandardTx(CTransaction(tx), reason));

    TxValidationState tx_state;
    std::string strTest;
    BOOST_CHECK_MESSAGE(CheckTransaction(CTransaction(tx), tx_state), strTest);
    BOOST_CHECK(tx_state.IsValid());

    const CBlockIndex *block_index = ::ChainActive().Tip();
    BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(tx), block_index, std::nullopt, tx_state));
    BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlock-quorum-hash");

    {
        // Any input should be a reason to fail CheckAssetUnlockTx()
        CCoinsView coinsDummy;
        CCoinsViewCache coins(&coinsDummy);
        std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

        CMutableTransaction txNonemptyInput = tx;
        txNonemptyInput.vin.resize(1);
        txNonemptyInput.vin[0].prevout.hash = dummyTransactions[0].GetHash();
        txNonemptyInput.vin[0].prevout.n = 1;
        txNonemptyInput.vin[0].scriptSig << std::vector<unsigned char>(65, 0);

        std::string reason;
        BOOST_CHECK(IsStandardTx(CTransaction(tx), reason));

        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txNonemptyInput), block_index, std::nullopt, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlocktx-have-input");
    }

    {
        const auto unlockPayload = GetTxPayload<CAssetUnlockPayload>(tx);
        BOOST_CHECK(unlockPayload.has_value());
        BOOST_CHECK(unlockPayload->getVersion() == 1);
        BOOST_CHECK(unlockPayload->getRequestedHeight() == 1000'000);
        BOOST_CHECK(unlockPayload->getFee() == 2000'000'000u);
        BOOST_CHECK(unlockPayload->getIndex() == 0x001122334455667788L);

        // Wrong type "Asset Lock TX" instead "Asset Unlock TX"
        CMutableTransaction txWrongType = tx;
        txWrongType.nType = TRANSACTION_ASSET_LOCK;
        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txWrongType), block_index, std::nullopt, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlocktx-type");

        // Check version of tx and payload
        BOOST_CHECK(tx.nVersion == 3);
        for (uint8_t payload_version : {0, 1, 2, 255}) {
            CAssetUnlockPayload unlockPayload_tmp{payload_version,
                unlockPayload->getIndex(),
                unlockPayload->getFee(),
                unlockPayload->getRequestedHeight(),
                unlockPayload->getQuorumHash(),
                unlockPayload->getQuorumSig()};
            CMutableTransaction txWrongVersion = tx;
            SetTxPayload(txWrongVersion, unlockPayload_tmp);
            if (payload_version != 1) {
                BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txWrongVersion), block_index, std::nullopt, tx_state));
                BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlocktx-version");
            } else {
                BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txWrongVersion), block_index, std::nullopt, tx_state));
                BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlock-quorum-hash");
            }
        }
    }

    {
        // Exactly 32 withdrawal is fine
        CMutableTransaction txManyOutputs = tx;
        int outputsLimit = 32;
        txManyOutputs.vout.resize(outputsLimit);
        for (auto& out : txManyOutputs.vout) {
            out.nValue = CENT;
            out.scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        }

        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txManyOutputs), block_index, std::nullopt, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlock-quorum-hash");

        // Basic checks for CRangesSet
        CRangesSet indexes;
        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txManyOutputs), block_index, indexes, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlock-quorum-hash");
        BOOST_CHECK(indexes.Add(0x001122334455667788L));
        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txManyOutputs), block_index, indexes, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlock-duplicated-index");


        // Should not be more than 32 withdrawal in one transaction
        txManyOutputs.vout.resize(outputsLimit + 1);
        txManyOutputs.vout.back().nValue = CENT;
        txManyOutputs.vout.back().scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        BOOST_CHECK(!CheckAssetUnlockTx(CTransaction(txManyOutputs), block_index, std::nullopt, tx_state));
        BOOST_CHECK(tx_state.GetRejectReason() == "bad-assetunlocktx-too-many-outs");
    }

}

BOOST_AUTO_TEST_SUITE_END()
