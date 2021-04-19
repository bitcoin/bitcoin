// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_dash.h>

#include <script/interpreter.h>
#include <script/standard.h>
#include <script/sign.h>
#include <validation.h>
#include <base58.h>
#include <netbase.h>
#include <messagesigner.h>
#include <policy/policy.h>
#include <keystore.h>
#include <spork.h>

#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>

#include <boost/test/unit_test.hpp>

typedef std::map<COutPoint, std::pair<int, CAmount>> SimpleUTXOMap;

static SimpleUTXOMap BuildSimpleUtxoMap(const std::vector<CTransactionRef>& txs)
{
    SimpleUTXOMap utxos;
    for (size_t i = 0; i < txs.size(); i++) {
        auto& tx = txs[i];
        for (size_t j = 0; j < tx->vout.size(); j++) {
            utxos.emplace(COutPoint(tx->GetHash(), j), std::make_pair((int)i + 1, tx->vout[j].nValue));
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
        BOOST_ASSERT(SignSignature(tempKeystore, *txFrom, tx, i, SIGHASH_ALL));
    }
}

static CMutableTransaction CreateProRegTx(SimpleUTXOMap& utxos, int port, const CScript& scriptPayout, const CKey& coinbaseKey, CKey& ownerKeyRet, CBLSSecretKey& operatorKeyRet)
{
    ownerKeyRet.MakeNewKey(true);
    operatorKeyRet.MakeNewKey();

    CProRegTx proTx;
    proTx.collateralOutpoint.n = 0;
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
    SetTxPayload(tx, proTx);
    SignTransaction(tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpServTx(SimpleUTXOMap& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, int port, const CScript& scriptOperatorPayout, const CKey& coinbaseKey)
{
    CProUpServTx proTx;
    proTx.proTxHash = proTxHash;
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

template<typename ProTx>
static CMutableTransaction MalleateProTxPayout(const CMutableTransaction& tx)
{
    ProTx proTx;
    GetTxPayload(tx, proTx);

    CKey key;
    key.MakeNewKey(false);
    proTx.scriptPayout = GetScriptForDestination(key.GetPubKey().GetID());

    CMutableTransaction tx2 = tx;
    SetTxPayload(tx2, proTx);

    return tx2;
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

static bool CheckTransactionSignature(const CMutableTransaction& tx)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const auto& txin = tx.vin[i];
        CTransactionRef txFrom;
        uint256 hashBlock;
        BOOST_ASSERT(GetTransaction(txin.prevout.hash, txFrom, Params().GetConsensus(), hashBlock));

        CAmount amount = txFrom->vout[txin.prevout.n].nValue;
        if (!VerifyScript(txin.scriptSig, txFrom->vout[txin.prevout.n].scriptPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&tx, i, amount))) {
            return false;
        }
    }
    return true;
}

BOOST_AUTO_TEST_SUITE(evo_dip3_activation_tests)

BOOST_FIXTURE_TEST_CASE(dip3_activation, TestChainDIP3BeforeActivationSetup)
{
    auto utxos = BuildSimpleUtxoMap(m_coinbase_txns);
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    CTxDestination payoutDest = DecodeDestination("yRq1Ky1AfFmf597rnotj7QRxsDUKePVWNF");
    auto tx = CreateProRegTx(utxos, 1, GetScriptForDestination(payoutDest), coinbaseKey, ownerKey, operatorKey);
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
    sporkManager.SetSporkAddress(EncodeDestination(sporkKey.GetPubKey().GetID()));
    sporkManager.SetPrivKey(EncodeSecret(sporkKey));

    auto utxos = BuildSimpleUtxoMap(m_coinbase_txns);

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

        // also verify that payloads are not malleable after they have been signed
        // the form of ProRegTx we use here is one with a collateral included, so there is no signature inside the
        // payload itself. This means, we need to rely on script verification, which takes the hash of the extra payload
        // into account
        auto tx2 = MalleateProTxPayout<CProRegTx>(tx);
        CValidationState dummyState;
        // Technically, the payload is still valid...
        BOOST_ASSERT(CheckProRegTx(tx, chainActive.Tip(), dummyState, *pcoinsTip.get()));
        BOOST_ASSERT(CheckProRegTx(tx2, chainActive.Tip(), dummyState, *pcoinsTip.get()));
        // But the signature should not verify anymore
        BOOST_ASSERT(CheckTransactionSignature(tx));
        BOOST_ASSERT(!CheckTransactionSignature(tx2));

        CreateAndProcessBlock({tx}, coinbaseKey);
        deterministicMNManager->UpdatedBlockTip(chainActive.Tip());

        BOOST_ASSERT(chainActive.Height() == nHeight + 1);
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));

        nHeight++;
    }

    int DIP0003EnforcementHeightBackup = Params().GetConsensus().DIP0003EnforcementHeight;
    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = chainActive.Height() + 1;
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
    BOOST_ASSERT(dmn != nullptr && dmn->pdmnState->GetBannedHeight() == nHeight);

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
    // check malleability protection again, but this time by also relying on the signature inside the ProUpRegTx
    auto tx2 = MalleateProTxPayout<CProUpRegTx>(tx);
    CValidationState dummyState;
    BOOST_ASSERT(CheckProUpRegTx(tx, chainActive.Tip(), dummyState, *pcoinsTip.get()));
    BOOST_ASSERT(!CheckProUpRegTx(tx2, chainActive.Tip(), dummyState, *pcoinsTip.get()));
    BOOST_ASSERT(CheckTransactionSignature(tx));
    BOOST_ASSERT(!CheckTransactionSignature(tx2));
    // now process the block
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
    BOOST_ASSERT(dmn != nullptr && !dmn->pdmnState->IsBanned());

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

    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = DIP0003EnforcementHeightBackup;
}

BOOST_FIXTURE_TEST_CASE(dip3_verify_db, TestChainDIP3Setup)
{
    int nHeight = chainActive.Height();
    auto utxos = BuildSimpleUtxoMap(m_coinbase_txns);

    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(payoutKey.GetPubKey().GetID());
    auto scriptCollateral = GetScriptForDestination(collateralKey.GetPubKey().GetID());

    // Create a MN with an external collateral
    CMutableTransaction tx_collateral;
    FundTransaction(tx_collateral, utxos, scriptCollateral, 1000 * COIN, coinbaseKey);
    SignTransaction(tx_collateral, coinbaseKey);

    auto block = std::make_shared<CBlock>(CreateBlock({tx_collateral}, coinbaseKey));
    BOOST_ASSERT(ProcessNewBlock(Params(), block, true, nullptr));
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 1);
    BOOST_ASSERT(block->GetHash() == chainActive.Tip()->GetBlockHash());

    CProRegTx payload;
    payload.addr = LookupNumeric("1.1.1.1", 1);
    payload.keyIDOwner = ownerKey.GetPubKey().GetID();
    payload.pubKeyOperator = operatorKey.GetPublicKey();
    payload.keyIDVoting = ownerKey.GetPubKey().GetID();
    payload.scriptPayout = scriptPayout;

    for (size_t i = 0; i < tx_collateral.vout.size(); ++i) {
        if (tx_collateral.vout[i].nValue == 1000 * COIN) {
            payload.collateralOutpoint = COutPoint(tx_collateral.GetHash(), i);
            break;
        }
    }

    CMutableTransaction tx_reg;
    tx_reg.nVersion = 3;
    tx_reg.nType = TRANSACTION_PROVIDER_REGISTER;
    FundTransaction(tx_reg, utxos, scriptPayout, 1000 * COIN, coinbaseKey);
    payload.inputsHash = CalcTxInputsHash(tx_reg);
    CMessageSigner::SignMessage(payload.MakeSignString(), payload.vchSig, collateralKey);
    SetTxPayload(tx_reg, payload);
    SignTransaction(tx_reg, coinbaseKey);

    auto tx_reg_hash = tx_reg.GetHash();

    block = std::make_shared<CBlock>(CreateBlock({tx_reg}, coinbaseKey));
    BOOST_ASSERT(ProcessNewBlock(Params(), block, true, nullptr));
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 2);
    BOOST_ASSERT(block->GetHash() == chainActive.Tip()->GetBlockHash());
    BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx_reg_hash));

    // Now spend the collateral while updating the same MN
    SimpleUTXOMap collateral_utxos;
    collateral_utxos.emplace(payload.collateralOutpoint, std::make_pair(1, 1000));
    auto proUpRevTx = CreateProUpRevTx(collateral_utxos, tx_reg_hash, operatorKey, collateralKey);

    block = std::make_shared<CBlock>(CreateBlock({proUpRevTx}, coinbaseKey));
    BOOST_ASSERT(ProcessNewBlock(Params(), block, true, nullptr));
    deterministicMNManager->UpdatedBlockTip(chainActive.Tip());
    BOOST_ASSERT(chainActive.Height() == nHeight + 3);
    BOOST_ASSERT(block->GetHash() == chainActive.Tip()->GetBlockHash());
    BOOST_ASSERT(!deterministicMNManager->GetListAtChainTip().HasMN(tx_reg_hash));

    // Verify db consistency
    BOOST_ASSERT(CVerifyDB().VerifyDB(Params(), pcoinsTip.get(), 4, 2));
}

BOOST_AUTO_TEST_SUITE_END()
