// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_dash.h"

#include "script/interpreter.h"
#include "script/standard.h"
#include "script/sign.h"
#include "validation.h"
#include "base58.h"
#include "netbase.h"
#include "messagesigner.h"
#include "keystore.h"
#include "spork.h"

#include "evo/specialtx.h"
#include "evo/providertx.h"
#include "evo/deterministicmns.h"

#include <boost/test/unit_test.hpp>

static const CBitcoinAddress payoutAddress  ("yRq1Ky1AfFmf597rnotj7QRxsDUKePVWNF");
static const std::string payoutKey          ("cV3qrPWzDcnhzRMV4MqtTH4LhNPqPo26ZntGvfJhc8nqCi8Ae5xR");

typedef std::map<COutPoint, std::pair<int, CAmount>> SimpleUTXOMap;

static SimpleUTXOMap BuildSimpleUtxoMap(const std::vector<CTransaction>& txs)
{
    SimpleUTXOMap utxos;
    for (size_t i = 0; i < txs.size(); i++) {
        auto& tx = txs[i];
        for (size_t j = 0; j < tx.vout.size(); j++) {
            utxos.emplace(COutPoint(tx.GetHash(), j), std::make_pair((int)i + 1, tx.vout[j].nValue));
        }
    }
    return utxos;
}

static std::vector<COutPoint> SelectUTXOs(SimpleUTXOMap& utoxs, CAmount amount, CAmount& changeRet)
{
    changeRet = 0;

    std::vector<COutPoint> selectedUtxos;
    CAmount selectedAmount = 0;
    while (!utoxs.empty()) {
        bool found = false;
        for (auto it = utoxs.begin(); it != utoxs.end(); ++it) {
            if (chainActive.Height() - it->second.first < 101) {
                continue;
            }

            found = true;
            selectedAmount += it->second.second;
            selectedUtxos.emplace_back(it->first);
            utoxs.erase(it);
            break;
        }
        BOOST_ASSERT(found);
        if (selectedAmount >= amount) {
            changeRet = selectedAmount - amount;
            break;
        }
    }

    return selectedUtxos;
}

static void FundTransaction(CMutableTransaction& tx, SimpleUTXOMap& utoxs, const CScript& scriptPayout, CAmount amount, const CKey& coinbaseKey)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    CAmount change;
    auto inputs = SelectUTXOs(utoxs, amount, change);
    for (size_t i = 0; i < inputs.size(); i++) {
        tx.vin.emplace_back(CTxIn(inputs[i]));
    }
    tx.vout.emplace_back(CTxOut(amount, scriptPayout));
    if (change != 0) {
        tx.vout.emplace_back(CTxOut(change, scriptPayout));
    }
}

static void SignTransaction(CMutableTransaction& tx, const CKey& coinbaseKey)
{
    CBasicKeyStore tempKeystore;
    tempKeystore.AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());

    for (size_t i = 0; i < tx.vin.size(); i++) {
        CTransactionRef txFrom;
        uint256 hashBlock;
        BOOST_ASSERT(GetTransaction(tx.vin[i].prevout.hash, txFrom, Params().GetConsensus(), hashBlock));
        BOOST_ASSERT(SignSignature(tempKeystore, *txFrom, tx, i));
    }
}

static CMutableTransaction CreateProRegTx(SimpleUTXOMap& utxos, int port, const CScript& scriptPayout, const CKey& coinbaseKey, CKey& ownerKeyRet, CBLSSecretKey& operatorKeyRet)
{
    ownerKeyRet.MakeNewKey(true);
    operatorKeyRet.MakeNewKey();

    CAmount change;
    auto inputs = SelectUTXOs(utxos, 1000 * COIN, change);

    CProRegTx proTx;
    proTx.nProtocolVersion = PROTOCOL_VERSION;
    proTx.nCollateralIndex = 0;
    proTx.addr = LookupNumeric("1.1.1.1", port);
    proTx.keyIDOwner = ownerKeyRet.GetPubKey().GetID();
    proTx.pubKeyOperator = operatorKeyRet.GetPublicKey();
    proTx.keyIDVoting = ownerKeyRet.GetPubKey().GetID();
    proTx.scriptPayout = scriptPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    FundTransaction(tx, utxos, scriptPayout, 1000 * COIN, coinbaseKey);
    proTx.inputsHash = CalcTxInputsHash(tx);
    CHashSigner::SignHash(::SerializeHash(proTx), ownerKeyRet, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpServTx(SimpleUTXOMap& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, int port, const CScript& scriptOperatorPayout, const CKey& coinbaseKey)
{
    CAmount change;
    auto inputs = SelectUTXOs(utxos, 1 * COIN, change);

    CProUpServTx proTx;
    proTx.proTxHash = proTxHash;
    proTx.nProtocolVersion = PROTOCOL_VERSION;
    proTx.addr = LookupNumeric("1.1.1.1", port);
    proTx.scriptOperatorPayout = scriptOperatorPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    FundTransaction(tx, utxos, GetScriptForDestination(coinbaseKey.GetPubKey().GetID()), 1 * COIN, coinbaseKey);
    proTx.inputsHash = CalcTxInputsHash(tx);
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx));
    SetTxPayload(tx, proTx);
    SignTransaction(tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRegTx(SimpleUTXOMap& utxos, const uint256& proTxHash, const CKey& mnKey, const CBLSPublicKey& pubKeyOperator, const CKeyID& keyIDVoting, const CScript& scriptPayout, const CKey& coinbaseKey)
{
    CAmount change;
    auto inputs = SelectUTXOs(utxos, 1 * COIN, change);

    CProUpRegTx proTx;
    proTx.proTxHash = proTxHash;
    proTx.pubKeyOperator = pubKeyOperator;
    proTx.keyIDVoting = keyIDVoting;
    proTx.scriptPayout = scriptPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    FundTransaction(tx, utxos, GetScriptForDestination(coinbaseKey.GetPubKey().GetID()), 1 * COIN, coinbaseKey);
    proTx.inputsHash = CalcTxInputsHash(tx);
    CHashSigner::SignHash(::SerializeHash(proTx), mnKey, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRevTx(SimpleUTXOMap& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, const CKey& coinbaseKey)
{
    CAmount change;
    auto inputs = SelectUTXOs(utxos, 1 * COIN, change);

    CProUpRevTx proTx;
    proTx.proTxHash = proTxHash;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;
    FundTransaction(tx, utxos, GetScriptForDestination(coinbaseKey.GetPubKey().GetID()), 1 * COIN, coinbaseKey);
    proTx.inputsHash = CalcTxInputsHash(tx);
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx));
    SetTxPayload(tx, proTx);
    SignTransaction(tx, coinbaseKey);

    return tx;
}

static CScript GenerateRandomAddress()
{
    CKey key;
    key.MakeNewKey(false);
    return GetScriptForDestination(key.GetPubKey().GetID());
}

static CDeterministicMNCPtr FindPayoutDmn(const CBlock& block)
{
    auto dmnList = deterministicMNManager->GetListAtChainTip();

    for (const auto& txout : block.vtx[0]->vout) {
        CDeterministicMNCPtr found;
        dmnList.ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
            if (found == nullptr && txout.scriptPubKey == dmn->pdmnState->scriptPayout) {
                found = dmn;
            }
        });
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

BOOST_AUTO_TEST_SUITE(evo_dip3_activation_tests)

BOOST_FIXTURE_TEST_CASE(dip3_activation, TestChainDIP3BeforeActivationSetup)
{
    auto utxos = BuildSimpleUtxoMap(coinbaseTxns);
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx = CreateProRegTx(utxos, 1, GetScriptForDestination(payoutAddress.Get()), coinbaseKey, ownerKey, operatorKey);
    std::vector<CMutableTransaction> txns = {tx};

    int nHeight = chainActive.Height();

    // We start one block before DIP3 activation, so mining a block with a DIP3 transaction should fail
    auto block = std::make_shared<CBlock>(CreateBlock(txns, coinbaseKey));
    ProcessNewBlock(Params(), block, true, nullptr);
    BOOST_ASSERT(chainActive.Height() == nHeight);
    BOOST_ASSERT(block->GetHash() != chainActive.Tip()->GetBlockHash());
    BOOST_ASSERT(!deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));

    // This block should activate DIP3
    CreateAndProcessBlock({}, coinbaseKey);
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);

    // Mining a block with a DIP3 transaction should succeed now
    block = std::make_shared<CBlock>(CreateBlock(txns, coinbaseKey));
    ProcessNewBlock(Params(), block, true, nullptr);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 2);
    BOOST_ASSERT(block->GetHash() == chainActive.Tip()->GetBlockHash());
    BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
}

BOOST_FIXTURE_TEST_CASE(dip3_protx, TestChainDIP3Setup)
{
    CKey sporkKey;
    sporkKey.MakeNewKey(false);
    CBitcoinSecret sporkSecret(sporkKey);
    CBitcoinAddress sporkAddress;
    sporkAddress.Set(sporkKey.GetPubKey().GetID());
    sporkManager.SetSporkAddress(sporkAddress.ToString());
    sporkManager.SetPrivKey(sporkSecret.ToString());

    auto utxos = BuildSimpleUtxoMap(coinbaseTxns);

    int nHeight = chainActive.Height();
    int port = 1;

    std::vector<uint256> dmnHashes;
    std::map<uint256, CKey> ownerKeys;
    std::map<uint256, CBLSSecretKey> operatorKeys;

    // register one MN per block
    for (size_t i = 0; i < 6; i++) {
        CKey ownerKey;
        CBLSSecretKey operatorKey;
        auto tx = CreateProRegTx(utxos, port++, GenerateRandomAddress(), coinbaseKey, ownerKey, operatorKey);
        dmnHashes.emplace_back(tx.GetHash());
        ownerKeys.emplace(tx.GetHash(), ownerKey);
        operatorKeys.emplace(tx.GetHash(), operatorKey);
        CreateAndProcessBlock({tx}, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());

        BOOST_ASSERT(chainActive.Height() == nHeight + 1);
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));

        nHeight++;
    }

    // activate spork15
    sporkManager.UpdateSpork(SPORK_15_DETERMINISTIC_MNS_ENABLED, chainActive.Height() + 1, *g_connman);
    CreateAndProcessBlock({}, coinbaseKey);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    nHeight++;

    // check MN reward payments
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = deterministicMNManager->GetListAtChainTip().GetMNPayee();

        CBlock block = CreateAndProcessBlock({}, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
        BOOST_ASSERT(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        BOOST_ASSERT(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // register multiple MNs per block
    for (size_t i = 0; i < 3; i++) {
        std::vector<CMutableTransaction> txns;
        for (size_t j = 0; j < 3; j++) {
            CKey ownerKey;
            CBLSSecretKey operatorKey;
            auto tx = CreateProRegTx(utxos, port++, GenerateRandomAddress(), coinbaseKey, ownerKey, operatorKey);
            dmnHashes.emplace_back(tx.GetHash());
            ownerKeys.emplace(tx.GetHash(), ownerKey);
            operatorKeys.emplace(tx.GetHash(), operatorKey);
            txns.emplace_back(tx);
        }
        CreateAndProcessBlock(txns, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
        BOOST_ASSERT(chainActive.Height() == nHeight + 1);

        for (size_t j = 0; j < 3; j++) {
            BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(txns[j].GetHash()));
        }

        nHeight++;
    }

    // test ProUpServTx
    auto tx = CreateProUpServTx(utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], 1000, CScript(), coinbaseKey);
    CreateAndProcessBlock({tx}, coinbaseKey);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);
    nHeight++;

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_ASSERT(dmn != nullptr && dmn->pdmnState->addr.GetPort() == 1000);

    // test ProUpRevTx
    tx = CreateProUpRevTx(utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], coinbaseKey);
    CreateAndProcessBlock({tx}, coinbaseKey);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);
    nHeight++;

    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_ASSERT(dmn != nullptr && dmn->pdmnState->nPoSeBanHeight == nHeight);

    // test that the revoked MN does not get paid anymore
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = deterministicMNManager->GetListAtChainTip().GetMNPayee();
        BOOST_ASSERT(dmnExpectedPayee->proTxHash != dmnHashes[0]);

        CBlock block = CreateAndProcessBlock({}, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
        BOOST_ASSERT(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        BOOST_ASSERT(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // test reviving the MN
    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    tx = CreateProUpRegTx(utxos, dmnHashes[0], ownerKeys[dmnHashes[0]], newOperatorKey.GetPublicKey(), ownerKeys[dmnHashes[0]].GetPubKey().GetID(), dmn->pdmnState->scriptPayout, coinbaseKey);
    CreateAndProcessBlock({tx}, coinbaseKey);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);
    nHeight++;

    tx = CreateProUpServTx(utxos, dmnHashes[0], newOperatorKey, 100, CScript(), coinbaseKey);
    CreateAndProcessBlock({tx}, coinbaseKey);
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);
    nHeight++;

    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_ASSERT(dmn != nullptr && dmn->pdmnState->addr.GetPort() == 100);
    BOOST_ASSERT(dmn != nullptr && dmn->pdmnState->nPoSeBanHeight == -1);

    // test that the revived MN gets payments again
    bool foundRevived = false;
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = deterministicMNManager->GetListAtChainTip().GetMNPayee();
        if (dmnExpectedPayee->proTxHash == dmnHashes[0]) {
            foundRevived = true;
        }

        CBlock block = CreateAndProcessBlock({}, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
        BOOST_ASSERT(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        BOOST_ASSERT(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }
    BOOST_ASSERT(foundRevived);
}
BOOST_AUTO_TEST_SUITE_END()
