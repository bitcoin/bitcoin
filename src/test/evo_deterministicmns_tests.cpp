// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/util/setup_common.h>

#include <script/interpreter.h>
#include <script/script.h>
#include <script/sign.h>
#include <validation.h>
#include <base58.h>
#include <netbase.h>
#include <messagesigner.h>
#include <policy/policy.h>
#include <script/signingprovider.h>
#include <spork.h>
#include <txmempool.h>

#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>
#include <dbwrapper.h>
#include <boost/test/unit_test.hpp>
#include <test/util/txmempool.h>
#include <interfaces/chain.h>

#include <fstream>
using SimpleUTXOVec = std::vector<std::pair<COutPoint, std::pair<int, CAmount>> >;

static SimpleUTXOVec BuildSimpleUTXOVec(const std::vector<CTransactionRef>& txs)
{
    SimpleUTXOVec utxos;
    for (size_t i = 0; i < txs.size(); i++) {
        auto& tx = *txs[i];
        for (size_t j = 0; j < tx.vout.size(); j++) {
            if(tx.vout[j].nValue > 0)
                utxos.emplace_back(COutPoint(tx.GetHash(), j), std::make_pair((int)i + 1, tx.vout[j].nValue));
        }
    }
    return utxos;
}

static std::vector<COutPoint> SelectUTXOs(const node::NodeContext& node, SimpleUTXOVec& utxos, CAmount amount, CAmount& changeRet)
{
    changeRet = 0;
    std::vector<COutPoint> selectedUtxos;
    CAmount selectedAmount = 0;
    auto it = utxos.begin();
    bool bFound = false;
    while (it != utxos.end()) {
        if (*node.chain->getHeight() - it->second.first < 101) {
            it++;
            continue;
        }
        selectedAmount += it->second.second;
        selectedUtxos.emplace_back(it->first);
        it = utxos.erase(it);
        bFound = true;
        if (selectedAmount >= amount) {
            changeRet = selectedAmount - amount;
            break;
        }
    }
    assert(bFound);
    return selectedUtxos;
}

static void FundTransaction(const node::NodeContext& node, CMutableTransaction& tx, SimpleUTXOVec& utoxs, const CScript& scriptPayout, CAmount amount)
{
    CAmount change;
    auto inputs = SelectUTXOs(node, utoxs, amount, change);
    for (size_t i = 0; i < inputs.size(); i++) {
        tx.vin.emplace_back(CTxIn(inputs[i]));
    }
    tx.vout.emplace_back(CTxOut(amount, scriptPayout));
    if (change != 0) {
        tx.vout.emplace_back(CTxOut(change, scriptPayout));
    }
}

static void SignTransaction(const node::NodeContext& node, CMutableTransaction& tx, const CKey& coinbaseKey)
{
    LOCK(cs_main);
    FillableSigningProvider tempKeystore;
    tempKeystore.AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());
    std::map<COutPoint, Coin> coins;
    for (size_t i = 0; i < tx.vin.size(); i++) {
        coins[tx.vin[i].prevout]; 
        node.chain->findCoins(coins);
    }
    std::map<int, bilingual_str> input_errors;
    BOOST_CHECK(SignTransaction(tx, &tempKeystore, coins, SIGHASH_ALL, input_errors));
}

static CMutableTransaction CreateProRegTx(const node::NodeContext& node, SimpleUTXOVec& utxos, int port, const CScript& scriptPayout, const CKey& coinbaseKey, CKey& ownerKeyRet, CBLSSecretKey& operatorKeyRet)
{
    ownerKeyRet.MakeNewKey(true);
    operatorKeyRet.MakeNewKey();

    CProRegTx proTx;
    proTx.nVersion = CProRegTx::GetVersion(!bls::bls_legacy_scheme);
    proTx.collateralOutpoint.n = 0;
    proTx.addr = LookupNumeric("1.1.1.1", port);
    proTx.keyIDOwner = ownerKeyRet.GetPubKey().GetID();
    proTx.pubKeyOperator.Set(operatorKeyRet.GetPublicKey(), bls::bls_legacy_scheme.load());
    proTx.keyIDVoting = ownerKeyRet.GetPubKey().GetID();
    proTx.scriptPayout = scriptPayout;
    proTx.nOperatorReward = 5000;

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;
    FundTransaction(node, tx, utxos, scriptPayout, 100 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    SetTxPayload(tx, proTx);
    SignTransaction(node, tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpServTx(const node::NodeContext& node, SimpleUTXOVec& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, int port, const CKey& coinbaseKey)
{
    CProUpServTx proTx;
    proTx.nVersion = CProUpRevTx::GetVersion(!bls::bls_legacy_scheme);
    proTx.proTxHash = proTxHash;
    proTx.addr = LookupNumeric("1.1.1.1", port);
    proTx.scriptOperatorPayout = GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()));
    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE;
    FundTransaction(node, tx, utxos, GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey())), 1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx), bls::bls_legacy_scheme);
    SetTxPayload(tx, proTx);
    SignTransaction(node, tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRegTx(const node::NodeContext& node, SimpleUTXOVec& utxos, const uint256& proTxHash, const CKey& mnKey, const CBLSPublicKey& pubKeyOperator, const CKeyID& keyIDVoting, const CScript& scriptPayout, const CKey& coinbaseKey)
{

    CProUpRegTx proTx;
    proTx.nVersion = CProUpRegTx::GetVersion(!bls::bls_legacy_scheme);
    proTx.proTxHash = proTxHash;
    proTx.pubKeyOperator.Set(pubKeyOperator, bls::bls_legacy_scheme.load());
    proTx.keyIDVoting = keyIDVoting;
    proTx.scriptPayout = scriptPayout;

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR;
    FundTransaction(node, tx, utxos, GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey())), 1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    CHashSigner::SignHash(::SerializeHash(proTx), mnKey, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(node, tx, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRevTx(const node::NodeContext& node, SimpleUTXOVec& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, const CKey& coinbaseKey)
{
    CProUpRevTx proTx;
    proTx.nVersion = CProUpRevTx::GetVersion(!bls::bls_legacy_scheme);
    proTx.proTxHash = proTxHash;

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE;
    FundTransaction(node, tx, utxos, GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey())), 1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx), bls::bls_legacy_scheme);
    SetTxPayload(tx, proTx);
    SignTransaction(node, tx, coinbaseKey);

    return tx;
}

template<typename ProTx>
static CMutableTransaction MalleateProTxPayout(const CMutableTransaction& tx)
{
    ProTx proTx;
    GetTxPayload(tx, proTx);

    CKey key;
    key.MakeNewKey(true);
    proTx.scriptPayout = GetScriptForDestination(WitnessV0KeyHash(key.GetPubKey()));

    CMutableTransaction tx2 = tx;
    SetTxPayload(tx2, proTx);

    return tx2;
}

static CScript GenerateRandomAddress()
{
    CKey key;
    key.MakeNewKey(true);
    return GetScriptForDestination(WitnessV0KeyHash(key.GetPubKey()));
}

static CDeterministicMNCPtr FindPayoutDmn(const CBlock& block)
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    for (const auto& txout : block.vtx[0]->vout) {
        CDeterministicMNCPtr found;
        mnList.ForEachMNShared(true, [&](const CDeterministicMNCPtr& dmn) {
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

static bool CheckTransactionSignature(const node::NodeContext& node, const CMutableTransaction& tx)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const auto& txin = tx.vin[i];
        std::map<COutPoint, Coin> coins;
        coins[txin.prevout]; 
        node.chain->findCoins(coins);
        const Coin& coin = coins.at(txin.prevout);
        if (!VerifyScript(txin.scriptSig, coin.out.scriptPubKey, nullptr, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&tx, i, coin.out.nValue, MissingDataBehavior::ASSERT_FAIL))) {
            return false;
        }
    }
    return true;
}

static uint256 MakeSnapshotKey(int height)
{
    return ArithToUint256(arith_uint256(height + 1));
}

static CDeterministicMNList MakeSnapshot(int height)
{
    const uint256 block_hash = MakeSnapshotKey(height);
    return CDeterministicMNList(block_hash, height, 0);
}

static void WriteSnapshotRange(CDeterministicMNManager& manager, int start_height, int count)
{
    for (int i = 0; i < count; ++i) {
        const int height = start_height + i;
        manager.m_evoDb->WriteCache(MakeSnapshotKey(height), MakeSnapshot(height));
    }
}

void FuncDIP3Activation(TestChain100Setup& setup)
{
    auto utxos = BuildSimpleUTXOVec(setup.m_coinbase_txns);
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    CScript addr = GenerateRandomAddress();
    auto tx = CreateProRegTx(setup.m_node, utxos, 1, addr, setup.coinbaseKey, ownerKey, operatorKey);
    std::vector<CMutableTransaction> txns = std::vector<CMutableTransaction>{tx};

    int nHeight = *setup.m_node.chain->getHeight();

    // We start one block before DIP3 activation, so mining a block with a DIP3 transaction should be no-op
    auto block = std::make_shared<CBlock>(setup.CreateAndProcessBlock(txns, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey())));
 
    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
    BOOST_CHECK_EQUAL(block->GetHash() , setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));
    
    assert(!deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));

    // re-create reg tx prev one got mined as no-op
    tx = CreateProRegTx(setup.m_node, utxos, 1, addr, setup.coinbaseKey, ownerKey, operatorKey);
    txns = std::vector<CMutableTransaction>{tx};
    // Mining a block with a DIP3 transaction should succeed now
    block = std::make_shared<CBlock>(setup.CreateAndProcessBlock(txns, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey())));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 2);
    BOOST_CHECK_EQUAL(block->GetHash() , setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));
    
    assert(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
}

void FuncDIP3Protx(TestChain100Setup& setup)
{
    CKey sporkKey;
    sporkKey.MakeNewKey(true);
    sporkManager->SetSporkAddress(EncodeDestination(PKHash(sporkKey.GetPubKey())));
    sporkManager->SetPrivKey(EncodeSecret(sporkKey));

    auto utxos = BuildSimpleUTXOVec(setup.m_coinbase_txns);

    int nHeight = *setup.m_node.chain->getHeight();
    int port = 1;

    std::vector<uint256> dmnHashes;
    std::map<uint256, CKey> ownerKeys;
    std::map<uint256, CBLSSecretKey> operatorKeys;

    // register one MN per block
    for (size_t i = 0; i < 6; i++) {
        CKey ownerKey;
        CBLSSecretKey operatorKey;
        auto tx = CreateProRegTx(setup.m_node, utxos, port++, GenerateRandomAddress(), setup.coinbaseKey, ownerKey, operatorKey);
        dmnHashes.emplace_back(tx.GetHash());
        ownerKeys.emplace(tx.GetHash(), ownerKey);
        operatorKeys.emplace(tx.GetHash(), operatorKey);
        {
            LOCK(cs_main);
            // also verify that payloads are not malleable after they have been signed
            // the form of ProRegTx we use here is one with a collateral included, so there is no signature inside the
            // payload itself. This means, we need to rely on script verification, which takes the hash of the extra payload
            // into account
            auto tx2 = MalleateProTxPayout<CProRegTx>(tx);
            TxValidationState dummyState;
            // Technically, the payload is still valid...
            assert(CheckProRegTx(CTransaction(tx), setup.m_node.chainman->ActiveTip(), dummyState, setup.m_node.chainman->ActiveChainstate().CoinsTip(), false, true));
            assert(CheckProRegTx(CTransaction(tx2), setup.m_node.chainman->ActiveTip(), dummyState, setup.m_node.chainman->ActiveChainstate().CoinsTip(), false, true));
            // But the signature should not verify anymore
            assert(CheckTransactionSignature(setup.m_node, tx));
            assert(!CheckTransactionSignature(setup.m_node, tx2));
        }

        setup.CreateAndProcessBlock({tx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

        BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
        
        auto mnList = deterministicMNManager->GetListAtChainTip();
        assert(mnList.HasMN(tx.GetHash()));

        nHeight++;
    }
    int DIP0003EnforcementHeightBackup = Params().GetConsensus().DIP0003EnforcementHeight;
    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = *setup.m_node.chain->getHeight() + 1;
    
    setup.CreateAndProcessBlock({}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    nHeight++;

    // check MN reward payments
    for (size_t i = 0; i < 20; i++) {
        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmnExpectedPayee = mnList.GetMNPayee();

        CBlock block = setup.CreateAndProcessBlock({}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

        assert(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        assert(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // register multiple MNs per block
    for (size_t i = 0; i < 3; i++) {
        std::vector<CMutableTransaction> txns;
        for (size_t j = 0; j < 3; j++) {
            CKey ownerKey;
            CBLSSecretKey operatorKey;
            auto tx = CreateProRegTx(setup.m_node, utxos, port++, GenerateRandomAddress(), setup.coinbaseKey, ownerKey, operatorKey);
            dmnHashes.emplace_back(tx.GetHash());
            ownerKeys.emplace(tx.GetHash(), ownerKey);
            operatorKeys.emplace(tx.GetHash(), operatorKey);
            txns.emplace_back(tx);
        }
        setup.CreateAndProcessBlock(txns, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));


        for (size_t j = 0; j < 3; j++) {
            assert(deterministicMNManager->GetListAtChainTip().HasMN(txns[j].GetHash()));
        }

        nHeight++;
    }

    // test ProUpServTx
    auto tx = CreateProUpServTx(setup.m_node, utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], 1000, setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
    nHeight++;
    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    assert(dmn != nullptr && dmn->pdmnState->addr.GetPort() == 1000);

    // test ProUpRevTx
    tx = CreateProUpRevTx(setup.m_node, utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
    
    nHeight++;
    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    assert(dmn != nullptr && dmn->pdmnState->GetBannedHeight() == nHeight);

    // test that the revoked MN does not get paid anymore
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = deterministicMNManager->GetListAtChainTip().GetMNPayee();
        assert(dmnExpectedPayee->proTxHash != dmnHashes[0]);

        CBlock block = setup.CreateAndProcessBlock({}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

        assert(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        assert(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // test reviving the MN
    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    tx = CreateProUpRegTx(setup.m_node, utxos, dmnHashes[0], ownerKeys[dmnHashes[0]], newOperatorKey.GetPublicKey(), ownerKeys[dmnHashes[0]].GetPubKey().GetID(), dmn->pdmnState->scriptPayout, setup.coinbaseKey);
    {
        LOCK(cs_main);
        // check malleability protection again, but this time by also relying on the signature inside the ProUpRegTx
        auto tx2 = MalleateProTxPayout<CProUpRegTx>(tx);
        TxValidationState dummyState;
        assert(CheckProUpRegTx(CTransaction(tx), setup.m_node.chainman->ActiveTip(), dummyState, setup.m_node.chainman->ActiveChainstate().CoinsTip(), false, true));
        assert(!CheckProUpRegTx(CTransaction(tx2), setup.m_node.chainman->ActiveTip(), dummyState, setup.m_node.chainman->ActiveChainstate().CoinsTip(), false, true));
        assert(CheckTransactionSignature(setup.m_node, tx));
        assert(!CheckTransactionSignature(setup.m_node, tx2));
    }
    // now process the block
    setup.CreateAndProcessBlock({tx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
    nHeight++;

    tx = CreateProUpServTx(setup.m_node, utxos, dmnHashes[0], newOperatorKey, 100, setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 1);
    nHeight++;
    dmn = deterministicMNManager->GetListAtChainTip().GetMN(dmnHashes[0]);
    assert(dmn != nullptr && dmn->pdmnState->addr.GetPort() == 100);
    assert(dmn != nullptr && !dmn->pdmnState->IsBanned());

    // test that the revived MN gets payments again
    bool foundRevived = false;
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = deterministicMNManager->GetListAtChainTip().GetMNPayee();
        if (dmnExpectedPayee->proTxHash == dmnHashes[0]) {
            foundRevived = true;
        }

        CBlock block = setup.CreateAndProcessBlock({}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

        assert(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(block);
        assert(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }
    assert(foundRevived);

    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = DIP0003EnforcementHeightBackup;
}

void FuncTestMempoolReorg(TestChain100Setup& setup)
{
    int nHeight = *setup.m_node.chain->getHeight();
    auto utxos = BuildSimpleUTXOVec(setup.m_coinbase_txns);
    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(WitnessV0KeyHash(payoutKey.GetPubKey()));
    auto scriptCollateral = GetScriptForDestination(WitnessV0KeyHash(collateralKey.GetPubKey()));

    // Create a MN with an external collateral
    CMutableTransaction tx_collateral;
    FundTransaction(setup.m_node, tx_collateral, utxos, scriptCollateral, 100 * COIN);
    SignTransaction(setup.m_node, tx_collateral, setup.coinbaseKey);

    auto block = setup.CreateAndProcessBlock({tx_collateral}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*(setup.m_node.chain->getHeight()) , nHeight + 1);
    BOOST_CHECK_EQUAL(block.GetHash() , setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));

    CProRegTx payload;
    payload.addr = LookupNumeric("1.1.1.1", 1);
    payload.keyIDOwner = ownerKey.GetPubKey().GetID();
    payload.pubKeyOperator.Set(operatorKey.GetPublicKey(), bls::bls_legacy_scheme.load());
    payload.keyIDVoting = ownerKey.GetPubKey().GetID();
    payload.scriptPayout = scriptPayout;

    for (size_t i = 0; i < tx_collateral.vout.size(); ++i) {
        if (tx_collateral.vout[i].nValue == 100 * COIN) {
            payload.collateralOutpoint = COutPoint(tx_collateral.GetHash(), i);
            break;
        }
    }

    CMutableTransaction tx_reg;
    tx_reg.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;
    FundTransaction(setup.m_node, tx_reg, utxos, scriptPayout, 100 * COIN);
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx_reg));
    CMessageSigner::SignMessage(payload.MakeSignString(), payload.vchSig, collateralKey);
    SetTxPayload(tx_reg, payload);
    SignTransaction(setup.m_node, tx_reg, setup.coinbaseKey);

    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;
    LOCK2(cs_main, testPool.cs);
    // Create ProUpServ and test block reorg which double-spend ProRegTx
    auto tx_up_serv = CreateProUpServTx(setup.m_node, utxos, tx_reg.GetHash(), operatorKey, 2, setup.coinbaseKey);
    testPool.addUnchecked(entry.FromTx(tx_up_serv));
    // A disconnected block would insert ProRegTx back into mempool
    testPool.addUnchecked(entry.FromTx(tx_reg));
    BOOST_CHECK_EQUAL(testPool.size(), 2U);

    // Create a tx that will double-spend ProRegTx
    CMutableTransaction tx_reg_ds;
    tx_reg_ds.vin = tx_reg.vin;
    tx_reg_ds.vout.emplace_back(0, CScript() << OP_RETURN);
    SignTransaction(setup.m_node, tx_reg_ds, setup.coinbaseKey);

    // Check mempool as if a new block with tx_reg_ds was connected instead of the old one with tx_reg
    std::vector<CTransactionRef> block_reorg;
    block_reorg.emplace_back(std::make_shared<CTransaction>(tx_reg_ds));
    testPool.removeForBlock(block_reorg, nHeight + 2);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
}

void FuncTestMempoolDualProregtx(TestChain100Setup& setup)
{
    auto utxos = BuildSimpleUTXOVec(setup.m_coinbase_txns);

    // Create a MN
    CKey ownerKey1;
    CBLSSecretKey operatorKey1;
    auto tx_reg1 = CreateProRegTx(setup.m_node, utxos, 1, GenerateRandomAddress(), setup.coinbaseKey, ownerKey1, operatorKey1);

    // Create a MN with an external collateral that references tx_reg1
    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(WitnessV0KeyHash(payoutKey.GetPubKey()));

    CProRegTx payload;
    payload.addr = LookupNumeric("1.1.1.1", 2);
    payload.keyIDOwner = ownerKey.GetPubKey().GetID();
    payload.pubKeyOperator.Set(operatorKey.GetPublicKey(), bls::bls_legacy_scheme.load());
    payload.keyIDVoting = ownerKey.GetPubKey().GetID();
    payload.scriptPayout = scriptPayout;

    for (size_t i = 0; i < tx_reg1.vout.size(); ++i) {
        if (tx_reg1.vout[i].nValue == 100 * COIN) {
            payload.collateralOutpoint = COutPoint(tx_reg1.GetHash(), i);
            break;
        }
    }

    CMutableTransaction tx_reg2;
    tx_reg2.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;
    FundTransaction(setup.m_node, tx_reg2, utxos, scriptPayout, 100 * COIN);
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx_reg2));
    CMessageSigner::SignMessage(payload.MakeSignString(), payload.vchSig, collateralKey);
    SetTxPayload(tx_reg2, payload);
    SignTransaction(setup.m_node, tx_reg2, setup.coinbaseKey);

    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;
    LOCK2(cs_main, testPool.cs);

    testPool.addUnchecked(entry.FromTx(tx_reg1));
    BOOST_CHECK_EQUAL(testPool.size(), 1U);
    BOOST_CHECK(testPool.existsProviderTxConflict(CTransaction(tx_reg2)));
}

void FuncVerifyDB(TestChain100Setup& setup)
{
    int nHeight = *setup.m_node.chain->getHeight();
    auto utxos = BuildSimpleUTXOVec(setup.m_coinbase_txns);

    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(WitnessV0KeyHash(payoutKey.GetPubKey()));
    auto scriptCollateral = GetScriptForDestination(WitnessV0KeyHash(collateralKey.GetPubKey()));

    // Create a MN with an external collateral
    CMutableTransaction tx_collateral;
    FundTransaction(setup.m_node, tx_collateral, utxos, scriptCollateral, 100 * COIN);
    SignTransaction(setup.m_node, tx_collateral, setup.coinbaseKey);


    auto block = setup.CreateAndProcessBlock({tx_collateral}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    assert(*setup.m_node.chain->getHeight() == nHeight + 1);
    assert(block.GetHash() == setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));

    CProRegTx payload;
    payload.addr = LookupNumeric("1.1.1.1", 1);
    payload.keyIDOwner = ownerKey.GetPubKey().GetID();
    payload.pubKeyOperator.Set(operatorKey.GetPublicKey(), bls::bls_legacy_scheme.load());
    payload.keyIDVoting = ownerKey.GetPubKey().GetID();
    payload.scriptPayout = scriptPayout;

    for (size_t i = 0; i < tx_collateral.vout.size(); ++i) {
        if (tx_collateral.vout[i].nValue == 100 * COIN) {
            payload.collateralOutpoint = COutPoint(tx_collateral.GetHash(), i);
            break;
        }
    }

    CMutableTransaction tx_reg;
    tx_reg.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;
    FundTransaction(setup.m_node, tx_reg, utxos, scriptPayout, 100 * COIN);
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx_reg));
    CMessageSigner::SignMessage(payload.MakeSignString(), payload.vchSig, collateralKey);
    SetTxPayload(tx_reg, payload);
    SignTransaction(setup.m_node, tx_reg, setup.coinbaseKey);

    auto tx_reg_hash = tx_reg.GetHash();

    block = setup.CreateAndProcessBlock({tx_reg}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 2);
    BOOST_CHECK_EQUAL(block.GetHash() , setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));
    assert(deterministicMNManager->GetListAtChainTip().HasMN(tx_reg_hash));

    // Now spend the collateral while updating the same MN
    SimpleUTXOVec collateral_utxos;
    collateral_utxos.emplace_back(payload.collateralOutpoint, std::make_pair(1, 100 * COIN));
    auto proUpRevTx = CreateProUpRevTx(setup.m_node, collateral_utxos, tx_reg_hash, operatorKey, collateralKey);

    block = setup.CreateAndProcessBlock({proUpRevTx}, GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));

    BOOST_CHECK_EQUAL(*setup.m_node.chain->getHeight() , nHeight + 3);
    BOOST_CHECK_EQUAL(block.GetHash() , setup.m_node.chain->getBlockHash(*setup.m_node.chain->getHeight()));
    assert(!deterministicMNManager->GetListAtChainTip().HasMN(tx_reg_hash));
    LOCK(cs_main);
    Chainstate& active_chainstate = setup.m_node.chainman->ActiveChainstate();
    // Verify db consistency
    assert(CVerifyDB(setup.m_node.chainman->GetNotifications()).VerifyDB(active_chainstate, Params().GetConsensus(), active_chainstate.CoinsTip(), 4, 2) == VerifyDBResult::SUCCESS);
}
BOOST_AUTO_TEST_SUITE(evo_dip3_activation_tests)

// DIP3 can only be activated with legacy scheme (v19 is activated later)
BOOST_AUTO_TEST_CASE(dip3_activation_legacy)
{
    TestChainDIP3BeforeActivationSetup setup;
    FuncDIP3Activation(setup);
}

BOOST_AUTO_TEST_CASE(dip3_protx_legacy)
{
    TestChainDIP3Setup setup;
    FuncDIP3Protx(setup);
}

BOOST_AUTO_TEST_CASE(dip3_protx_basic)
{
    TestChainDIP3V19Setup setup;
    FuncDIP3Protx(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_reorg_legacy)
{
    TestChainDIP3Setup setup;
    FuncTestMempoolReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_reorg_basic)
{
    TestChainDIP3V19Setup setup;
    FuncTestMempoolReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_dual_proregtx_legacy)
{
    TestChainDIP3Setup setup;
    FuncTestMempoolDualProregtx(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_dual_proregtx_basic)
{
    TestChainDIP3V19Setup setup;
    FuncTestMempoolDualProregtx(setup);
}

//This one can be started only with legacy scheme, since inside undo block will switch it back to legacy resulting into an inconsistency
BOOST_AUTO_TEST_CASE(verify_db_legacy)
{
    TestChainDIP3Setup setup;
    FuncVerifyDB(setup);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(evo_dmn_db_maintenance_tests)

BOOST_AUTO_TEST_CASE(forced_flush_compacts_persisted_snapshots_window)
{
    const int cache_limit = CDeterministicMNManager::LIST_CACHE_SIZE;
    BOOST_REQUIRE_EQUAL(cache_limit % 3, 0);
    const int sub_flush_batch = cache_limit / 3;

    auto db_params = DBParams{
        .path = "testdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true,
    };
    CDeterministicMNManager manager(db_params);

    for (int batch = 0; batch < 3; ++batch) {
        WriteSnapshotRange(manager, batch * sub_flush_batch, sub_flush_batch);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));
        BOOST_CHECK_EQUAL(manager.m_evoDb->CountPersistedEntries(), (batch + 1) * sub_flush_batch);
    }

    // Reproduce the shutdown-growth bug: cache is below 1728, but disk already
    // holds a full 3-day window. Forced flush must compact instead of appending.
    WriteSnapshotRange(manager, cache_limit, 1);
    BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));

    BOOST_CHECK_EQUAL(manager.m_evoDb->CountPersistedEntries(), cache_limit);
    BOOST_CHECK_EQUAL(manager.m_evoDb->GetReadWriteCacheSize(), 0U);
    BOOST_CHECK_EQUAL(manager.m_evoDb->GetEraseCacheSize(), 0U);

    CDeterministicMNList snapshot;
    BOOST_CHECK(!manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(1), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 1);
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit);
}

BOOST_AUTO_TEST_CASE(forced_flush_rewrite_failure_preserves_existing_snapshots)
{
    const int cache_limit = CDeterministicMNManager::LIST_CACHE_SIZE;
    BOOST_REQUIRE_EQUAL(cache_limit % 3, 0);
    const int sub_flush_batch = cache_limit / 3;

    auto db_params = DBParams{
        .path = "testdb_dmn_rewrite_failure",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = true,
    };
    CDeterministicMNManager manager(db_params);

    for (int batch = 0; batch < 3; ++batch) {
        WriteSnapshotRange(manager, batch * sub_flush_batch, sub_flush_batch);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));
    }

    WriteSnapshotRange(manager, cache_limit, 1);

    int batch_calls{0};
    manager.m_evoDb->SetWriteBatchHookForTesting([&](CDBBatch&) {
        ++batch_calls;
        return false;
    });

    BOOST_CHECK(!manager.FlushCacheToDisk(/*bForceFlush=*/true));
    BOOST_CHECK_EQUAL(batch_calls, 1);

    CDeterministicMNList snapshot;
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 0);
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(cache_limit - 1), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit - 1);
    BOOST_CHECK(!manager.m_evoDb->Read(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK(manager.m_evoDb->ReadCache(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit);

    manager.m_evoDb->SetWriteBatchHookForTesting({});
    BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));
    BOOST_CHECK(!manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(1), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 1);
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit);
}

BOOST_AUTO_TEST_CASE(forced_flush_rewrite_exception_preserves_existing_snapshots)
{
    const int cache_limit = CDeterministicMNManager::LIST_CACHE_SIZE;
    BOOST_REQUIRE_EQUAL(cache_limit % 3, 0);
    const int sub_flush_batch = cache_limit / 3;

    auto db_params = DBParams{
        .path = "testdb_dmn_rewrite_exception",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = true,
    };
    CDeterministicMNManager manager(db_params);

    for (int batch = 0; batch < 3; ++batch) {
        WriteSnapshotRange(manager, batch * sub_flush_batch, sub_flush_batch);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));
    }

    WriteSnapshotRange(manager, cache_limit, 1);

    manager.m_evoDb->SetWriteBatchHookForTesting([&](CDBBatch&) -> bool {
        throw dbwrapper_error("injected rewrite failure");
    });

    BOOST_CHECK(!manager.FlushCacheToDisk(/*bForceFlush=*/true));

    CDeterministicMNList snapshot;
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 0);
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(cache_limit - 1), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit - 1);
    BOOST_CHECK(!manager.m_evoDb->Read(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK(manager.m_evoDb->ReadCache(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit);

    manager.m_evoDb->SetWriteBatchHookForTesting({});
    BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));
    BOOST_CHECK(!manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(1), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 1);
    BOOST_CHECK(manager.m_evoDb->Read(MakeSnapshotKey(cache_limit), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), cache_limit);
}

BOOST_AUTO_TEST_CASE(rewrite_backup_is_recovered_on_restart)
{
    auto db_params = DBParams{
        .path = "testdb_dmn_rewrite_recovery",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = true,
    };

    {
        CDeterministicMNManager manager(db_params);
        WriteSnapshotRange(manager, 0, 3);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));

        fs::path backup_path = db_params.path;
        backup_path += ".rewrite-backup";
        fs::path marker_path = db_params.path;
        marker_path += ".rewrite-in-progress";
        std::error_code ec;
        fs::remove_all(backup_path, ec);
        fs::remove(marker_path, ec);

        manager.m_evoDb->CloseDB();
        fs::rename(db_params.path, backup_path, ec);
        BOOST_REQUIRE(!ec);
        std::ofstream marker_file(fs::PathToString(marker_path));
        BOOST_REQUIRE(marker_file.good());
        marker_file << "prepared";
        marker_file.close();

        manager.m_evoDb->OpenDB(/*create_new=*/true);
        BOOST_REQUIRE(manager.m_evoDb->Write(MakeSnapshotKey(999), MakeSnapshot(999), /*fSync=*/true));
        manager.m_evoDb->CloseDB();
    }

    DBParams reopen_params = db_params;
    reopen_params.wipe_data = false;
    CDeterministicMNManager recovered_manager(reopen_params);

    CDeterministicMNList snapshot;
    BOOST_CHECK(recovered_manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 0);
    BOOST_CHECK(recovered_manager.m_evoDb->Read(MakeSnapshotKey(2), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 2);
    BOOST_CHECK(!recovered_manager.m_evoDb->Read(MakeSnapshotKey(999), snapshot));
}

BOOST_AUTO_TEST_CASE(completed_rewrite_backup_is_ignored_on_restart)
{
    auto db_params = DBParams{
        .path = "testdb_dmn_rewrite_stale_backup",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = true,
    };

    {
        CDeterministicMNManager manager(db_params);
        WriteSnapshotRange(manager, 0, 3);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));

        fs::path backup_path = db_params.path;
        backup_path += ".rewrite-backup";
        fs::path marker_path = db_params.path;
        marker_path += ".rewrite-in-progress";
        std::error_code ec;
        fs::remove_all(backup_path, ec);
        fs::remove(marker_path, ec);

        manager.m_evoDb->CloseDB();
        fs::rename(db_params.path, backup_path, ec);
        BOOST_REQUIRE(!ec);

        manager.m_evoDb->OpenDB(/*create_new=*/true);
        BOOST_REQUIRE(manager.m_evoDb->Write(MakeSnapshotKey(999), MakeSnapshot(999), /*fSync=*/true));
        manager.m_evoDb->CloseDB();

        std::ofstream marker_file(fs::PathToString(marker_path));
        BOOST_REQUIRE(marker_file.good());
        marker_file << "complete";
        marker_file.close();
    }

    DBParams reopen_params = db_params;
    reopen_params.wipe_data = false;
    CDeterministicMNManager recovered_manager(reopen_params);

    CDeterministicMNList snapshot;
    BOOST_CHECK(recovered_manager.m_evoDb->Read(MakeSnapshotKey(999), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 999);

    fs::path backup_path = db_params.path;
    backup_path += ".rewrite-backup";
    BOOST_CHECK(!fs::exists(backup_path));
}

BOOST_AUTO_TEST_CASE(rewrite_marker_without_backup_is_ignored_on_restart)
{
    auto db_params = DBParams{
        .path = "testdb_dmn_rewrite_marker_only",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = true,
    };

    {
        CDeterministicMNManager manager(db_params);
        WriteSnapshotRange(manager, 0, 3);
        BOOST_REQUIRE(manager.FlushCacheToDisk(/*bForceFlush=*/true));

        fs::path marker_path = db_params.path;
        marker_path += ".rewrite-in-progress";
        std::error_code ec;
        fs::remove(marker_path, ec);

        std::ofstream marker_file(fs::PathToString(marker_path));
        BOOST_REQUIRE(marker_file.good());
        marker_file << "prepared";
        marker_file.close();
    }

    DBParams reopen_params = db_params;
    reopen_params.wipe_data = false;
    CDeterministicMNManager recovered_manager(reopen_params);

    CDeterministicMNList snapshot;
    BOOST_CHECK(recovered_manager.m_evoDb->Read(MakeSnapshotKey(0), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 0);
    BOOST_CHECK(recovered_manager.m_evoDb->Read(MakeSnapshotKey(2), snapshot));
    BOOST_CHECK_EQUAL(snapshot.GetHeight(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
