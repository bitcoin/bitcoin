// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/masternode.h>
#include <test/util/setup_common.h>

#include <chainparams.h>
#include <clientversion.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/assetlocktx.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/providertx.h>
#include <evo/sharedcollateral.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <interfaces/node.h>
#include <llmq/context.h>
#include <node/mempool_args.h>
#include <messagesigner.h>
#include <netbase.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <pow.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <streams.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/std23.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <map>
#include <optional>
#include <set>
#include <vector>

static bool IsV24Active(const ChainstateManager& chainman) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    return DeploymentActiveAfter(chainman.ActiveChain().Tip(), chainman, Consensus::DEPLOYMENT_V24);
}

static SpecialTxRules TipRules(const ChainstateManager& chainman) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    return GetSpecialTxRules(chainman.ActiveChain().Tip(), chainman);
}

static CMutableTransaction CreateSpendTx(const ChainstateManager& chainman, SimpleUTXOMap& utxos, const CScript& scriptPayout, CAmount amount, const CKey& coinbaseKey)
{
    CMutableTransaction tx;
    const auto spent = FundTransaction(chainman, tx, utxos, scriptPayout, amount);
    SignTransaction(tx, spent, coinbaseKey);
    return tx;
}

static COutPoint GetCollateralOutpoint(const CMutableTransaction& tx)
{
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        if (tx.vout[i].nValue == dmn_types::Regular.collat_amount) {
            return COutPoint(tx.GetHash(), i);
        }
    }
    return COutPoint();
}

// ProRegTx that references a pre-existing collateral output instead of funding the collateral inline.
static CMutableTransaction CreateProRegTxExternalCollateral(const ChainstateManager& chainman, SimpleUTXOMap& utxos, int port, const COutPoint& collateralOutpoint, const CScript& scriptPayout, const CKey& ownerKey, const CBLSSecretKey& operatorKey, const CKey& collateralKey, const CKey& coinbaseKey)
{
    CProRegTx proTx;
    proTx.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    proTx.netInfo = NetInfoInterface::MakeNetInfo(proTx.nVersion);
    BOOST_CHECK_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.1:%d", port)),
                      NetInfoStatus::Success);
    proTx.collateralOutpoint = collateralOutpoint;
    proTx.keyIDOwner = ownerKey.GetPubKey().GetID();
    proTx.pubKeyOperator.Set(operatorKey.GetPublicKey(), bls::bls_legacy_scheme.load());
    proTx.keyIDVoting = ownerKey.GetPubKey().GetID();
    proTx.scriptPayout = scriptPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    // The collateral is external (referenced via collateralOutpoint), so this tx only needs to fund a fee.
    const auto spent = FundTransaction(chainman, tx, utxos, scriptPayout, /*amount=*/1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    CMessageSigner::SignMessage(proTx.MakeSignString(), proTx.vchSig, collateralKey);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpServTx(const ChainstateManager& chainman, SimpleUTXOMap& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, int port, const CScript& scriptOperatorPayout, const CKey& coinbaseKey,
                                             uint16_t version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false), CAmount fee = 0)
{
    CProUpServTx proTx;
    proTx.nVersion = version;
    proTx.netInfo = NetInfoInterface::MakeNetInfo(proTx.nVersion);
    proTx.proTxHash = proTxHash;
    BOOST_CHECK_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.1:%d", port)),
                      NetInfoStatus::Success);
    proTx.scriptOperatorPayout = scriptOperatorPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    const auto spent = FundTransaction(chainman, tx, utxos, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 1 * COIN);
    tx.vout.back().nValue -= fee;
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx), bls::bls_legacy_scheme);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRegTx(const ChainstateManager& chainman, SimpleUTXOMap& utxos, const uint256& proTxHash, const CKey& mnKey, const CBLSPublicKey& pubKeyOperator, const CKeyID& keyIDVoting, const CScript& scriptPayout, const CKey& coinbaseKey,
                                            uint16_t version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false), CAmount fee = 0)
{
    CProUpRegTx proTx;
    proTx.nVersion = version;
    proTx.proTxHash = proTxHash;
    proTx.pubKeyOperator.Set(pubKeyOperator, /*specificLegacyScheme=*/version == ProTxVersion::LegacyBLS);
    proTx.keyIDVoting = keyIDVoting;
    proTx.scriptPayout = scriptPayout;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent = FundTransaction(chainman, tx, utxos, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 1 * COIN);
    // FundTransaction returns every input satoshi to the outputs, so the transaction pays no fee and the
    // mempool would refuse it for not meeting the min relay fee. Tests going through CreateAndProcessBlock
    // never notice, because block creation does not check relay fees.
    tx.vout.back().nValue -= fee;
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    CHashSigner::SignHash(::SerializeHash(proTx), mnKey, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, coinbaseKey);

    return tx;
}

static CMutableTransaction CreateProUpRevTx(const ChainstateManager& chainman, SimpleUTXOMap& utxos, const uint256& proTxHash, const CBLSSecretKey& operatorKey, const CKey& coinbaseKey,
                                            CAmount fee = 0)
{
    CProUpRevTx proTx;
    proTx.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    proTx.proTxHash = proTxHash;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;
    const auto spent = FundTransaction(chainman, tx, utxos, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 1 * COIN);
    tx.vout.back().nValue -= fee;
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    proTx.sig = operatorKey.Sign(::SerializeHash(proTx), bls::bls_legacy_scheme);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, coinbaseKey);

    return tx;
}

template<typename ProTx>
static CMutableTransaction MalleateProTxPayout(const CMutableTransaction& tx)
{
    auto opt_protx = GetTxPayload<ProTx>(tx);
    BOOST_REQUIRE(opt_protx.has_value());
    auto& protx = *opt_protx;

    CKey key;
    key.MakeNewKey(false);
    protx.scriptPayout = GetScriptForDestination(PKHash(key.GetPubKey()));

    CMutableTransaction tx2 = tx;
    SetTxPayload(tx2, protx);

    return tx2;
}

static CDeterministicMNCPtr FindPayoutDmn(CDeterministicMNManager& dmnman, const CBlock& block)
{
    auto dmnList = dmnman.GetListAtChainTip();

    for (const auto& txout : block.vtx[0]->vout) {
        CDeterministicMNCPtr found;
        dmnList.ForEachMNShared(/*onlyValid=*/true, [&](const auto& dmn) {
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

static bool CheckTransactionSignature(const CMutableTransaction& tx, const SimpleUTXOMap& coins)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const auto& txin = tx.vin[i];
        const CTxOut& from = coins.at(txin.prevout).out;
        if (!VerifyScript(txin.scriptSig, from.scriptPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&tx, i, from.nValue, MissingDataBehavior::ASSERT_FAIL))) {
            return false;
        }
    }
    return true;
}

void FuncDIP3Activation(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height   = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto tip_hash     = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    CTxDestination payoutDest = DecodeDestination("yRq1Ky1AfFmf597rnotj7QRxsDUKePVWNF");
    auto tx = CreateProRegTx(chainman, utxos, 1, GetScriptForDestination(payoutDest), setup.coinbaseKey, ownerKey, operatorKey);
    std::vector<CMutableTransaction> txns = {tx};

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();

    // We start one block before DIP3 activation, so mining a block with a DIP3 transaction should fail
    auto block = std::make_shared<CBlock>(setup.CreateBlock(txns, coinbase_pk, chainman.ActiveChainstate()));
    chainman.ProcessNewBlock(block, true, nullptr);
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    BOOST_REQUIRE(block->GetHash() != tip_hash());
    BOOST_REQUIRE(!dmnman.GetListAtChainTip().HasMN(tx.GetHash()));

    // This block should activate DIP3
    setup.CreateAndProcessBlock({}, coinbase_pk);
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    // Mining a block with a DIP3 transaction should succeed now
    block = std::make_shared<CBlock>(setup.CreateBlock(txns, coinbase_pk, chainman.ActiveChainstate()));
    // A list looked up while only the header is known must not outlive the block's connection
    BlockValidationState header_state;
    const CBlockIndex* header_index{nullptr};
    BOOST_REQUIRE(chainman.ProcessNewBlockHeaders({block->GetBlockHeader()}, header_state, &header_index));
    BOOST_REQUIRE(!dmnman.GetListForBlock(Assert(header_index)).HasMN(tx.GetHash()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 2);
    BOOST_CHECK_EQUAL(block->GetHash(), tip_hash());
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));
};

void FuncV19Activation(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height   = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));

    // create
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CBLSSecretKey operator_key;
    CKey collateral_key;
    collateral_key.MakeNewKey(false);
    auto collateralScript = GetScriptForDestination(PKHash(collateral_key.GetPubKey()));
    auto tx_reg = CreateProRegTx(chainman, utxos, 1, collateralScript, setup.coinbaseKey, owner_key, operator_key);
    auto tx_reg_hash = tx_reg.GetHash();

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();

    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    ++nHeight;
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    sync_dmn_tip();
    auto tip_list = dmnman.GetListAtChainTip();
    BOOST_REQUIRE(tip_list.HasMN(tx_reg_hash));
    auto pindex_create = tip_index();
    auto base_list = dmnman.GetListForBlock(pindex_create);
    std::vector<CDeterministicMNListDiff> diffs;

    // update
    CBLSSecretKey operator_key_new;
    operator_key_new.MakeNewKey();
    auto tx_upreg = CreateProUpRegTx(chainman, utxos, tx_reg_hash, owner_key, operator_key_new.GetPublicKey(), owner_key.GetPubKey().GetID(), collateralScript, setup.coinbaseKey);

    block = std::make_shared<CBlock>(setup.CreateBlock({tx_upreg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    ++nHeight;
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    sync_dmn_tip();
    tip_list = dmnman.GetListAtChainTip();
    BOOST_REQUIRE(tip_list.HasMN(tx_reg_hash));
    diffs.push_back(base_list.BuildDiff(tip_list));

    // spend
    CMutableTransaction tx_spend;
    COutPoint collateralOutpoint(tx_reg_hash, 0);
    tx_spend.vin.emplace_back(collateralOutpoint);
    tx_spend.vout.emplace_back(999.99 * COIN, collateralScript);

    const auto spend_coins = BuildSimpleUtxoMap({MakeTransactionRef(tx_reg)});
    SignTransaction(tx_spend, spend_coins, collateral_key);
    block = std::make_shared<CBlock>(setup.CreateBlock({tx_spend}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    ++nHeight;
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    sync_dmn_tip();
    diffs.push_back(tip_list.BuildDiff(dmnman.GetListAtChainTip()));
    tip_list = dmnman.GetListAtChainTip();
    BOOST_REQUIRE(!tip_list.HasMN(tx_reg_hash));
    BOOST_REQUIRE(dmnman.GetListForBlock(pindex_create).HasMN(tx_reg_hash));

    // mine another block so that it's not the last one before V19
    setup.CreateAndProcessBlock({}, coinbase_pk);
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    ++nHeight;
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    sync_dmn_tip();
    diffs.push_back(tip_list.BuildDiff(dmnman.GetListAtChainTip()));
    tip_list = dmnman.GetListAtChainTip();
    BOOST_REQUIRE(!tip_list.HasMN(tx_reg_hash));
    BOOST_REQUIRE(dmnman.GetListForBlock(pindex_create).HasMN(tx_reg_hash));

    // this block should activate V19
    setup.CreateAndProcessBlock({}, coinbase_pk);
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    ++nHeight;
    BOOST_CHECK_EQUAL(tip_height(), nHeight);
    sync_dmn_tip();
    diffs.push_back(tip_list.BuildDiff(dmnman.GetListAtChainTip()));
    tip_list = dmnman.GetListAtChainTip();
    BOOST_REQUIRE(!tip_list.HasMN(tx_reg_hash));
    BOOST_REQUIRE(dmnman.GetListForBlock(pindex_create).HasMN(tx_reg_hash));

    // check mn list/diff
    CDeterministicMNListDiff dummy_diff = base_list.BuildDiff(tip_list);
    CDeterministicMNList dummy_list{base_list};
    dummy_list.ApplyDiff(tip_index(), dummy_diff);
    // Lists should match
    BOOST_REQUIRE(dummy_list == tip_list);

    // mine 10 more blocks
    for (int i = 0; i < 10; ++i)
    {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        BOOST_REQUIRE(
            DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
        BOOST_CHECK_EQUAL(tip_height(), nHeight + 1 + i);
        sync_dmn_tip();
        diffs.push_back(tip_list.BuildDiff(dmnman.GetListAtChainTip()));
        tip_list = dmnman.GetListAtChainTip();
        BOOST_REQUIRE(!tip_list.HasMN(tx_reg_hash));
        BOOST_REQUIRE(dmnman.GetListForBlock(pindex_create).HasMN(tx_reg_hash));
    }

    // check mn list/diff
    const CBlockIndex* v19_index = WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetAncestor(Params().GetConsensus().V19Height));
    auto v19_list = dmnman.GetListForBlock(v19_index);
    dummy_diff = v19_list.BuildDiff(tip_list);
    dummy_list = v19_list;
    dummy_list.ApplyDiff(tip_index(), dummy_diff);
    BOOST_REQUIRE(dummy_list == tip_list);

    // NOTE: this fails on v19/v19.1 with errors like:
    // "RemoveMN: Can't delete a masternode ... with a pubKeyOperator=..."
    dummy_diff = base_list.BuildDiff(tip_list);
    dummy_list = base_list;
    dummy_list.ApplyDiff(tip_index(), dummy_diff);
    BOOST_REQUIRE(dummy_list == tip_list);

    dummy_list = base_list;
    for (const auto& diff : diffs) {
        dummy_list.ApplyDiff(tip_index(), diff);
    }
    BOOST_REQUIRE(dummy_list == tip_list);
};

void FuncProUpRegTxVersionHandlingBeforeV24(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(bls::bls_legacy_scheme.load());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 1, GenerateRandomAddress(),
                                 setup.coinbaseKey, owner_key, operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();

    auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    while (!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19)) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    const auto payoutScript = GenerateRandomAddress();
    auto tx_upreg = CreateProUpRegTx(chainman, utxos, proTxHash, owner_key,
                                     operator_key.GetPublicKey(), owner_key.GetPubKey().GetID(), payoutScript,
                                     setup.coinbaseKey);
    const auto opt_upreg = GetTxPayload<CProUpRegTx>(tx_upreg);
    BOOST_REQUIRE(opt_upreg);
    BOOST_CHECK_EQUAL(opt_upreg->nVersion, ProTxVersion::BasicBLS);

    setup.CreateAndProcessBlock({tx_upreg}, coinbase_pk);
    sync_dmn_tip();

    dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK(dmn->pdmnState->scriptPayout == payoutScript);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::LegacyBLS);
    BOOST_REQUIRE(!dmn->pdmnState->IsBanned());

    CBLSSecretKey operator_key_new;
    operator_key_new.MakeNewKey();
    const auto payoutScript2 = GenerateRandomAddress();
    auto tx_upreg2 = CreateProUpRegTx(chainman, utxos, proTxHash, owner_key,
                                      operator_key_new.GetPublicKey(), owner_key.GetPubKey().GetID(), payoutScript2,
                                      setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx_upreg2}, coinbase_pk);
    sync_dmn_tip();

    dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK(dmn->pdmnState->scriptPayout == payoutScript2);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::BasicBLS);
    BOOST_CHECK(!dmn->pdmnState->pubKeyOperator.IsLegacy());
    BOOST_REQUIRE(dmn->pdmnState->IsBanned());

    CBLSSecretKey operator_key_legacy;
    operator_key_legacy.MakeNewKey();
    const auto payoutScript3 = GenerateRandomAddress();
    CProUpRegTx proTxLegacy;
    proTxLegacy.nVersion = ProTxVersion::LegacyBLS;
    proTxLegacy.proTxHash = proTxHash;
    proTxLegacy.pubKeyOperator.Set(operator_key_legacy.GetPublicKey(), /*legacy=*/true);
    proTxLegacy.keyIDVoting = owner_key.GetPubKey().GetID();
    proTxLegacy.scriptPayout = payoutScript3;

    CMutableTransaction tx_upreg3;
    tx_upreg3.nVersion = 3;
    tx_upreg3.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent_upreg3 = FundTransaction(chainman, tx_upreg3, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                              1 * COIN);
    proTxLegacy.inputsHash = CalcTxInputsHash(CTransaction(tx_upreg3));
    CHashSigner::SignHash(::SerializeHash(proTxLegacy), owner_key, proTxLegacy.vchSig);
    SetTxPayload(tx_upreg3, proTxLegacy);
    SignTransaction(tx_upreg3, spent_upreg3, setup.coinbaseKey);

    setup.CreateAndProcessBlock({tx_upreg3}, coinbase_pk);
    sync_dmn_tip();

    dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK(dmn->pdmnState->scriptPayout == payoutScript3);
    // Pre-v24, an operator-changing registrar update adopts the tx version even when that lowers the
    // stored version: the v1 update downgrades the state to LegacyBLS and re-encodes the new operator
    // key with the legacy scheme. This must match already-deployed consensus; version-bump hardening
    // only applies once v24 is active.
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::LegacyBLS);
    BOOST_CHECK(dmn->pdmnState->pubKeyOperator.IsLegacy());
    BOOST_CHECK(dmn->pdmnState->pubKeyOperator.Get() == operator_key_legacy.GetPublicKey());
};

void FuncProUpRegTxV3OnLegacyValid(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CBLSSecretKey operator_key;
    // The update below carries this address into ExtNetInfo, so it must be one ExtNetInfo accepts
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();

    auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    for (int i = 0; i < 2000 && !DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24); ++i) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    CProUpRegTx proTx;
    proTx.nVersion = ProTxVersion::ExtAddr;
    proTx.proTxHash = proTxHash;
    proTx.pubKeyOperator.Set(operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
    proTx.keyIDVoting = owner_key.GetPubKey().GetID();
    proTx.payouts = {{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}};

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent = FundTransaction(chainman, tx, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                       1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    CHashSigner::SignHash(::SerializeHash(proTx), owner_key, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, setup.coinbaseKey);

    TxValidationState val_state;
    {
        LOCK(cs_main);
        BOOST_CHECK(CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                    chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(), TipRules(chainman),
                                    val_state, /*check_sigs=*/true));
    }
    BOOST_CHECK(val_state.IsValid());

    setup.CreateAndProcessBlock({tx}, coinbase_pk);
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::ExtAddr);
};

void FuncProUpRegTxV2CannotBypassV3PayoutCollateralReuse(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    for (int i = 0; i < 2000 && !DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24); ++i) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CKey collateral_key;
    CBLSSecretKey operator_key;
    owner_key.MakeNewKey(true);
    collateral_key.MakeNewKey(true);
    operator_key.MakeNewKey();

    const auto script_collateral = GetScriptForDestination(PKHash(collateral_key.GetPubKey()));
    const auto script_payout = GenerateRandomAddress();

    auto tx_collateral = CreateSpendTx(chainman, utxos, script_collateral, dmn_types::Regular.collat_amount,
                                       setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx_collateral}, coinbase_pk);
    sync_dmn_tip();

    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::ExtAddr;
    pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
    BOOST_CHECK_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);
    pro_reg.keyIDOwner = owner_key.GetPubKey().GetID();
    pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
    pro_reg.keyIDVoting = owner_key.GetPubKey().GetID();
    pro_reg.payouts = {{script_payout, MasternodePayoutShare::MAX_REWARD}};
    pro_reg.collateralOutpoint = GetCollateralOutpoint(tx_collateral);

    CMutableTransaction tx_reg;
    tx_reg.nVersion = 3;
    tx_reg.nType = TRANSACTION_PROVIDER_REGISTER;
    const auto spent_reg = FundTransaction(chainman, tx_reg, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                           1 * COIN);
    pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx_reg));
    CMessageSigner::SignMessage(pro_reg.MakeSignString(), pro_reg.vchSig, collateral_key);
    SetTxPayload(tx_reg, pro_reg);
    SignTransaction(tx_reg, spent_reg, setup.coinbaseKey);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();

    auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::ExtAddr);
    BOOST_CHECK_EQUAL(dmn->pdmnState->payouts.size(), 1U);
    BOOST_CHECK(dmn->pdmnState->payouts.front().scriptPayout == script_payout);

    // The registrar update reuses the collateral script as payout; GetMax() is BasicBLS while v24 (but not
    // multi-payout) is active, which is exactly the version whose payee-reuse handling is under test.
    auto tx_upreg = CreateProUpRegTx(chainman, utxos, proTxHash, owner_key, operator_key.GetPublicKey(),
                                     owner_key.GetPubKey().GetID(), script_collateral, setup.coinbaseKey);

    TxValidationState val_state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!CheckProUpRegTx(CTransaction(tx_upreg), chainman.ActiveChain().Tip(), dmnman,
                                     chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                     TipRules(chainman), val_state, /*check_sigs=*/true));
    }
    BOOST_CHECK_EQUAL(val_state.GetRejectReason(), "bad-protx-payee-reuse");
}

// Coinbase validation matches each expected masternode payment against a DISTINCT coinbase output
// only after v24. This exercises the exact activation boundary with a SINGLE masternode whose
// owner payout and operator payout resolve to the same script AND amount (an even 50% operator
// reward), so the coinbase carries two identical outputs. The "merge cheat" folds those two
// identical outputs into one, redirecting the freed value to a miner output so the total coinbase
// value is unchanged; this underpays the masternode.
//
//   Pre-v24:  existence-only matching ACCEPTS the merge cheat.
//   Post-v24: strict multiplicity matching REJECTS it (and the faithful block connects).
//
// The duplicate is NOT a multi-payout (v4) phenomenon: multi-payout owner shares are forced to
// distinct scripts (bad-protx-payee-dup) and can never collide. It is a plain owner-payout ==
// operator-payout script collision, which every ProTx version supports -- here a v2 (BasicBLS)
// MN, the max version eligible while v24 is still pending, kept across the boundary.
void FuncMNPaymentMultiplicityV24Boundary(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_hash     = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    const auto& consensus = chainman.GetConsensus();

    // Start in the pre-v24 window: v19 active (basic BLS scheme) so we can register a v2 MN, but
    // v24 -- and thus strict multiplicity matching -- not yet active.
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), consensus, Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), consensus, Consensus::DEPLOYMENT_MN_RR));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);

    // A single script that BOTH an owner payout and the operator payout will point at.
    CKey payout_key;
    payout_key.MakeNewKey(true);
    const CScript shared_script = GetScriptForDestination(PKHash(payout_key.GetPubKey()));

    CKey owner_key;
    CKey collateral_key;
    CBLSSecretKey operator_key;
    owner_key.MakeNewKey(true);
    collateral_key.MakeNewKey(true);
    operator_key.MakeNewKey();
    const auto script_collateral = GetScriptForDestination(PKHash(collateral_key.GetPubKey()));

    // Fund the collateral.
    auto tx_collateral = CreateSpendTx(chainman, utxos, script_collateral, dmn_types::Regular.collat_amount,
                                       setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx_collateral}, coinbase_pk);
    sync_dmn_tip();

    // Register a plain v2 (BasicBLS) MN -- deliberately NOT multi-payout, and the max version
    // eligible while v24 is pending -- with a single legacy owner payout -> shared_script and a
    // 50% operator reward, so the owner share and the operator share are equal whenever the
    // masternode reward is even.
    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::BasicBLS;
    pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
    // Legacy MnNetInfo (v2) on regtest must not use the mainnet port; a small port works.
    BOOST_CHECK_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:1"), NetInfoStatus::Success);
    pro_reg.keyIDOwner = owner_key.GetPubKey().GetID();
    pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
    pro_reg.keyIDVoting = owner_key.GetPubKey().GetID();
    pro_reg.nOperatorReward = 5000; // 50%
    pro_reg.scriptPayout = shared_script;
    for (size_t i = 0; i < tx_collateral.vout.size(); ++i) {
        if (tx_collateral.vout[i].nValue == dmn_types::Regular.collat_amount) {
            pro_reg.collateralOutpoint = COutPoint(tx_collateral.GetHash(), i);
            break;
        }
    }

    CMutableTransaction tx_reg;
    tx_reg.nVersion = 3;
    tx_reg.nType = TRANSACTION_PROVIDER_REGISTER;
    const auto spent_reg = FundTransaction(chainman, tx_reg, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                           1 * COIN);
    pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx_reg));
    CMessageSigner::SignMessage(pro_reg.MakeSignString(), pro_reg.vchSig, collateral_key);
    SetTxPayload(tx_reg, pro_reg);
    SignTransaction(tx_reg, spent_reg, setup.coinbaseKey);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();

    auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::BasicBLS);

    // Point the operator payout at the same script as the owner payout. Nothing checks the
    // operator payout script against the owner payout script, in any version, so this collision
    // is accepted.
    CProUpServTx pro_ups;
    pro_ups.nVersion = ProTxVersion::BasicBLS;
    pro_ups.proTxHash = proTxHash;
    pro_ups.netInfo = NetInfoInterface::MakeNetInfo(pro_ups.nVersion);
    BOOST_CHECK_EQUAL(pro_ups.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:1"), NetInfoStatus::Success);
    pro_ups.scriptOperatorPayout = shared_script;

    CMutableTransaction tx_ups;
    tx_ups.nVersion = 3;
    tx_ups.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    const auto spent_ups = FundTransaction(chainman, tx_ups, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                           1 * COIN);
    pro_ups.inputsHash = CalcTxInputsHash(CTransaction(tx_ups));
    pro_ups.sig = operator_key.Sign(::SerializeHash(pro_ups), bls::bls_legacy_scheme);
    SetTxPayload(tx_ups, pro_ups);
    SignTransaction(tx_ups, spent_ups, setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx_ups}, coinbase_pk);
    sync_dmn_tip();
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->scriptOperatorPayout == shared_script);

    // Two identical coinbase outputs paying shared_script (the owner/operator collision).
    auto find_duplicate = [&](const CBlock& b) -> std::optional<CTxOut> {
        const auto& vout = b.vtx[0]->vout;
        for (size_t i = 0; i < vout.size(); ++i) {
            if (vout[i].scriptPubKey != shared_script) continue;
            for (size_t j = i + 1; j < vout.size(); ++j) {
                if (vout[j] == vout[i]) return vout[i];
            }
        }
        return std::nullopt;
    };

    // Mine until a block's (pre-operator) masternode reward is even: only then does the 50%
    // operator share equal the single owner share, so the owner and operator outputs -- both
    // paying shared_script -- collide into two identical coinbase outputs. The reward is constant
    // within a subsidy/reallocation epoch, so this can take up to a full epoch; intermediate
    // blocks are processed to advance the chain.
    auto mine_until_duplicate = [&]() -> std::pair<CBlock, CTxOut> {
        for (int i = 0; i < 2000; ++i) {
            CBlock good = setup.CreateBlock({}, coinbase_pk, chainman.ActiveChainstate());
            if (auto dup = find_duplicate(good)) return {good, *dup};
            BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<CBlock>(good), /*force_processing=*/true, nullptr));
            sync_dmn_tip();
        }
        BOOST_REQUIRE_MESSAGE(false, "expected owner/operator collision to yield a duplicate coinbase output");
        return {}; // unreachable, BOOST_REQUIRE above aborts the test
    };

    // Build the "merge cheat" sibling of `good`: drop one of the two identical outputs and fold
    // its value into a non-masternode (miner) output. Total coinbase value is unchanged (so
    // IsBlockValueValid still passes), but only ONE distinct output now pays shared_script.
    auto build_merge_cheat = [&](const CBlock& good, const CTxOut& dup) -> CBlock {
        CBlock merged = good;
        CMutableTransaction cb(*merged.vtx[0]);
        bool kept_first = false;
        for (auto it = cb.vout.begin(); it != cb.vout.end();) {
            if (*it == dup && kept_first) {
                it = cb.vout.erase(it);
            } else {
                if (*it == dup) kept_first = true;
                ++it;
            }
        }
        bool redirected = false;
        for (auto& o : cb.vout) {
            if (o.scriptPubKey != shared_script && !o.scriptPubKey.IsUnspendable()) {
                o.nValue += dup.nValue;
                redirected = true;
                break;
            }
        }
        BOOST_REQUIRE(redirected);
        merged.vtx[0] = MakeTransactionRef(std::move(cb));
        merged.hashMerkleRoot = BlockMerkleRoot(merged);
        merged.nNonce = 0;
        while (!CheckProofOfWork(merged.GetHash(), merged.nBits, consensus)) ++merged.nNonce;
        return merged;
    };

    // ---- Pre-v24: legacy existence-only matching ACCEPTS the merge cheat (masternode underpaid).
    {
        const auto [good, dup] = mine_until_duplicate();
        // The collision must be reached while v24 is still pending, otherwise this phase would be
        // testing post-v24 behaviour by accident.
        BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
        const CBlock merged = build_merge_cheat(good, dup);
        BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<CBlock>(merged), /*force_processing=*/true, nullptr));
        BOOST_CHECK_EQUAL(tip_hash(), merged.GetHash());
        sync_dmn_tip();
    }

    // ---- Mine across v24 activation (the same v2 MN is kept; its collision is version-independent).
    for (int i = 0; i < 2000 && !DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24); ++i) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), consensus, Consensus::DEPLOYMENT_MN_RR));

    // ---- Post-v24: strict multiplicity matching REJECTS the same merge (two expected outputs, one
    // distinct actual). Rejection is observed as the tip not advancing.
    {
        const auto [good, dup] = mine_until_duplicate();
        const CBlock merged = build_merge_cheat(good, dup);
        const uint256 tip_before = tip_hash();
        chainman.ProcessNewBlock(std::make_shared<CBlock>(merged), /*force_processing=*/true, nullptr);
        BOOST_CHECK(tip_hash() == tip_before);
        BOOST_CHECK(tip_hash() != merged.GetHash());

        // The faithful block (both identical outputs present) connects.
        BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<CBlock>(good), /*force_processing=*/true, nullptr));
        BOOST_CHECK_EQUAL(tip_hash(), good.GetHash());
    }
}

static std::shared_ptr<NetInfoInterface> DeserializeCoreP2PExtNetInfo(const std::vector<NetInfoEntry>& entries)
{
    // Hand-roll ExtNetInfo's serialization (version byte followed by a map of purpose to entries)
    // to construct states that AddEntry() would refuse to produce
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << uint8_t{1};                      // m_version (ExtNetInfo::CURRENT_VERSION)
    WriteCompactSize(ds, 1);               // m_data map size (one purpose)
    ds << NetInfoPurpose::CORE_P2P;        // map key (purpose)
    WriteCompactSize(ds, entries.size());  // entry count for this purpose
    for (const auto& entry : entries) {
        ds << entry;
    }

    auto net_info{std::make_shared<ExtNetInfo>()};
    ds >> *net_info;
    return net_info;
}

static CTransaction BuildExtNetInfoProRegTx(std::shared_ptr<NetInfoInterface> net_info)
{
    CKey owner_key;
    owner_key.MakeNewKey(true);
    CBLSSecretKey operator_key;
    operator_key.MakeNewKey();

    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::ExtAddr;
    pro_reg.netInfo = std::move(net_info);
    pro_reg.keyIDOwner = owner_key.GetPubKey().GetID();
    pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
    pro_reg.keyIDVoting = owner_key.GetPubKey().GetID();
    pro_reg.payouts = {{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}};

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx));
    SetTxPayload(tx, pro_reg);
    return CTransaction(tx);
}

void FuncProRegTxRejectsInvalidDeserializedExtNetInfo(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    for (int i = 0; i < 2000 && !DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24); ++i) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    auto check_reject_reason = [&](std::shared_ptr<NetInfoInterface> net_info, const std::string& reject_reason) {
        TxValidationState state;
        {
            LOCK(cs_main);
            BOOST_CHECK(!CheckProRegTx(BuildExtNetInfoProRegTx(std::move(net_info)), chainman.ActiveChain().Tip(),
                                       dmnman, chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                       TipRules(chainman), state,
                                       /*check_sigs=*/false));
        }
        BOOST_CHECK_EQUAL(state.GetResult(), TxValidationResult::TX_BAD_SPECIAL);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), reject_reason);
    };

    const uint16_t port{9998};
    const NetInfoEntry p2p_entry{LookupNumeric("1.1.1.1", port)};
    BOOST_REQUIRE(p2p_entry.IsTriviallyValid());
    check_reject_reason(DeserializeCoreP2PExtNetInfo({p2p_entry, p2p_entry}), "bad-protx-dup-netinfo-entry");

    std::vector<NetInfoEntry> too_many_entries;
    for (size_t i = 1; i <= MAX_ENTRIES_EXTNETINFO + 1; ++i) {
        const NetInfoEntry entry{LookupNumeric(strprintf("1.1.1.%d", i), port)};
        BOOST_REQUIRE(entry.IsTriviallyValid());
        too_many_entries.emplace_back(entry);
    }
    check_reject_reason(DeserializeCoreP2PExtNetInfo(too_many_entries), "bad-protx-netinfo-maxlimit");

    DomainPort domain;
    BOOST_REQUIRE_EQUAL(domain.Set("example.com", 443), DomainPort::Status::Success);
    const NetInfoEntry domain_entry{domain};
    BOOST_REQUIRE(domain_entry.IsTriviallyValid());
    check_reject_reason(DeserializeCoreP2PExtNetInfo({domain_entry}), "bad-protx-netinfo-entry");
}

void FuncDIP3Protx(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height   = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    // Snapshot kept for signature checks below, since FundTransaction() consumes utxos.
    const SimpleUTXOMap coins{utxos};

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();
    int port = 1;

    std::vector<uint256> dmnHashes;
    std::map<uint256, CKey> ownerKeys;
    std::map<uint256, CBLSSecretKey> operatorKeys;

    // register one MN per block
    for (size_t i = 0; i < 6; i++) {
        CKey ownerKey;
        CBLSSecretKey operatorKey;
        auto tx = CreateProRegTx(chainman, utxos, port++, GenerateRandomAddress(), setup.coinbaseKey, ownerKey, operatorKey);
        dmnHashes.emplace_back(tx.GetHash());
        ownerKeys.emplace(tx.GetHash(), ownerKey);
        operatorKeys.emplace(tx.GetHash(), operatorKey);

        // also verify that payloads are not malleable after they have been signed
        // the form of ProRegTx we use here is one with a collateral included, so there is no signature inside the
        // payload itself. This means, we need to rely on script verification, which takes the hash of the extra payload
        // into account
        auto tx2 = MalleateProTxPayout<CProRegTx>(tx);
        TxValidationState dummy_state;
        // Technically, the payload is still valid...
        {
            LOCK(cs_main);
            BOOST_REQUIRE(CheckProRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                        chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                        TipRules(chainman), dummy_state, true));
            BOOST_REQUIRE(CheckProRegTx(CTransaction(tx2), chainman.ActiveChain().Tip(), dmnman,
                                        chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                        TipRules(chainman), dummy_state, true));
        }
        // But the signature should not verify anymore
        BOOST_REQUIRE(CheckTransactionSignature(tx, coins));
        BOOST_REQUIRE(!CheckTransactionSignature(tx2, coins));

        setup.CreateAndProcessBlock({tx}, coinbase_pk);
        sync_dmn_tip();

        BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
        BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));

        nHeight++;
    }

    int DIP0003EnforcementHeightBackup = Params().GetConsensus().DIP0003EnforcementHeight;
    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = tip_height() + 1;
    setup.CreateAndProcessBlock({}, coinbase_pk);
    sync_dmn_tip();
    nHeight++;

    // check MN reward payments
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = dmnman.GetListAtChainTip().GetMNPayee(tip_index());
        BOOST_ASSERT(dmnExpectedPayee);

        CBlock block = setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
        BOOST_REQUIRE(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(dmnman, block);
        BOOST_REQUIRE(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // register multiple MNs per block
    for (size_t i = 0; i < 3; i++) {
        std::vector<CMutableTransaction> txns;
        for (size_t j = 0; j < 3; j++) {
            CKey ownerKey;
            CBLSSecretKey operatorKey;
            auto tx = CreateProRegTx(chainman, utxos, port++, GenerateRandomAddress(), setup.coinbaseKey, ownerKey, operatorKey);
            dmnHashes.emplace_back(tx.GetHash());
            ownerKeys.emplace(tx.GetHash(), ownerKey);
            operatorKeys.emplace(tx.GetHash(), operatorKey);
            txns.emplace_back(tx);
        }
        setup.CreateAndProcessBlock(txns, coinbase_pk);
        sync_dmn_tip();
        BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);

        for (size_t j = 0; j < 3; j++) {
            BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(txns[j].GetHash()));
        }

        nHeight++;
    }

    // test ProUpServTx
    auto tx = CreateProUpServTx(chainman, utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], 1000, CScript(), setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, coinbase_pk);
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    nHeight++;

    auto dmn = dmnman.GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_REQUIRE(dmn != nullptr && dmn->pdmnState->netInfo->GetPrimary().GetPort() == 1000);

    // test ProUpRevTx
    tx = CreateProUpRevTx(chainman, utxos, dmnHashes[0], operatorKeys[dmnHashes[0]], setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, coinbase_pk);
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    nHeight++;

    dmn = dmnman.GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_REQUIRE(dmn != nullptr && dmn->pdmnState->GetBannedHeight() == nHeight);

    // test that the revoked MN does not get paid anymore
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = dmnman.GetListAtChainTip().GetMNPayee(tip_index());
        BOOST_REQUIRE(dmnExpectedPayee && dmnExpectedPayee->proTxHash != dmnHashes[0]);

        CBlock block = setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
        BOOST_REQUIRE(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(dmnman, block);
        BOOST_REQUIRE(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }

    // test reviving the MN
    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    dmn = dmnman.GetListAtChainTip().GetMN(dmnHashes[0]);
    tx = CreateProUpRegTx(chainman, utxos, dmnHashes[0], ownerKeys[dmnHashes[0]], newOperatorKey.GetPublicKey(), ownerKeys[dmnHashes[0]].GetPubKey().GetID(), dmn->pdmnState->scriptPayout, setup.coinbaseKey);
    // check malleability protection again, but this time by also relying on the signature inside the ProUpRegTx
    auto tx2 = MalleateProTxPayout<CProUpRegTx>(tx);
    TxValidationState dummy_state;
    {
        LOCK(cs_main);
        BOOST_REQUIRE(CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                      chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                      TipRules(chainman), dummy_state, true));
        BOOST_REQUIRE(!CheckProUpRegTx(CTransaction(tx2), chainman.ActiveChain().Tip(), dmnman,
                                       chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                       TipRules(chainman), dummy_state, true));
    }
    BOOST_REQUIRE(CheckTransactionSignature(tx, coins));
    BOOST_REQUIRE(!CheckTransactionSignature(tx2, coins));
    // now process the block
    setup.CreateAndProcessBlock({tx}, coinbase_pk);
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    nHeight++;

    tx = CreateProUpServTx(chainman, utxos, dmnHashes[0], newOperatorKey, 100, CScript(), setup.coinbaseKey);
    setup.CreateAndProcessBlock({tx}, coinbase_pk);
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    nHeight++;

    dmn = dmnman.GetListAtChainTip().GetMN(dmnHashes[0]);
    BOOST_REQUIRE(dmn != nullptr && dmn->pdmnState->netInfo->GetPrimary().GetPort() == 100);
    BOOST_REQUIRE(dmn != nullptr && !dmn->pdmnState->IsBanned());

    // test that the revived MN gets payments again
    bool foundRevived = false;
    for (size_t i = 0; i < 20; i++) {
        auto dmnExpectedPayee = dmnman.GetListAtChainTip().GetMNPayee(tip_index());
        BOOST_ASSERT(dmnExpectedPayee);
        if (dmnExpectedPayee->proTxHash == dmnHashes[0]) {
            foundRevived = true;
        }

        CBlock block = setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
        BOOST_REQUIRE(!block.vtx.empty());

        auto dmnPayout = FindPayoutDmn(dmnman, block);
        BOOST_REQUIRE(dmnPayout != nullptr);
        BOOST_CHECK_EQUAL(dmnPayout->proTxHash.ToString(), dmnExpectedPayee->proTxHash.ToString());

        nHeight++;
    }
    BOOST_REQUIRE(foundRevived);

    const_cast<Consensus::Params&>(Params().GetConsensus()).DIP0003EnforcementHeight = DIP0003EnforcementHeightBackup;
}

// ProUpServTx with an out-of-range or mismatched nType must be rejected before
// mempool acceptance. Historically IsTriviallyValid / CheckProUpServTx ignored nType while
// BuildNewListFromBlock rejected it, so a single relayed tx stalled CreateNewBlock/getblocktemplate.
void FuncProUpServInvalidNType(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, GenerateRandomAddress(), setup.coinbaseKey, ownerKey,
                                 operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(proTxHash));
    BOOST_REQUIRE_EQUAL(std23::to_underlying(dmnman.GetListAtChainTip().GetMN(proTxHash)->nType),
                        std23::to_underlying(MnType::Regular));

    auto make_proupserv = [&](MnType nType) {
        CProUpServTx proTx;
        proTx.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
        proTx.nType = nType;
        proTx.netInfo = NetInfoInterface::MakeNetInfo(proTx.nVersion);
        proTx.proTxHash = proTxHash;
        BOOST_REQUIRE_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:42"), NetInfoStatus::Success);
        if (nType == MnType::Evo) {
            // Platform fields required by CheckProUpServTx when nType claims Evo.
            proTx.platformNodeID.SetHex("00112233445566778899aabbccddeeff00112233");
            proTx.platformP2PPort = 20001;
            proTx.platformHTTPPort = 20002;
        }

        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
        const auto spent =
            FundTransaction(chainman, tx, utxos, GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
        // FundTransaction constructs a zero-fee tx; leave a dust-sized fee so ProcessTransaction
        // reaches special-tx validation instead of failing on min-relay fee first.
        BOOST_REQUIRE(!tx.vout.empty());
        BOOST_REQUIRE(tx.vout[0].nValue > 10000);
        tx.vout[0].nValue -= 10000;
        proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
        proTx.sig = operatorKey.Sign(::SerializeHash(proTx), bls::bls_legacy_scheme);
        SetTxPayload(tx, proTx);
        SignTransaction(tx, spent, setup.coinbaseKey);
        return tx;
    };

    // Case 1: out-of-range nType. Must fail trivial validation and never enter the mempool.
    {
        auto tx = make_proupserv(static_cast<MnType>(42));
        auto opt_payload = GetTxPayload<CProUpServTx>(tx, /*assert_type=*/false);
        BOOST_REQUIRE(opt_payload.has_value());

        TxValidationState trivial_state;
        BOOST_CHECK(!opt_payload->IsTriviallyValid(trivial_state));
        BOOST_CHECK_EQUAL(trivial_state.GetRejectReason(), "bad-protx-type");

        TxValidationState check_state;
        {
            LOCK(cs_main);
            BOOST_CHECK(!CheckProUpServTx(CTransaction(tx), tip_index(), dmnman, chainman.GetConsensus(),
                                          TipRules(chainman), check_state, /*check_sigs=*/true));
        }
        BOOST_CHECK_EQUAL(check_state.GetRejectReason(), "bad-protx-type");

        {
            LOCK(cs_main);
            const MempoolAcceptResult result = chainman.ProcessTransaction(MakeTransactionRef(tx));
            BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
            BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "bad-protx-type");
        }
    }

    // Case 2: valid-range but mismatched nType (claim Evo for a Regular MN). Must fail contextual checks.
    {
        auto tx = make_proupserv(MnType::Evo);
        auto opt_payload = GetTxPayload<CProUpServTx>(tx, /*assert_type=*/false);
        BOOST_REQUIRE(opt_payload.has_value());

        // Trivial validation accepts Evo as a valid type; contextual validation must still reject.
        TxValidationState trivial_state;
        BOOST_CHECK(opt_payload->IsTriviallyValid(trivial_state));

        TxValidationState check_state;
        {
            LOCK(cs_main);
            BOOST_CHECK(!CheckProUpServTx(CTransaction(tx), tip_index(), dmnman, chainman.GetConsensus(),
                                          TipRules(chainman), check_state, /*check_sigs=*/true));
        }
        BOOST_CHECK_EQUAL(check_state.GetRejectReason(), "bad-protx-type-mismatch");
        BOOST_CHECK(check_state.GetResult() == TxValidationResult::TX_CONSENSUS);

        {
            LOCK(cs_main);
            const MempoolAcceptResult result = chainman.ProcessTransaction(MakeTransactionRef(tx));
            BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
            BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "bad-protx-type-mismatch");
            BOOST_CHECK(result.m_state.GetResult() == TxValidationResult::TX_CONSENSUS);
        }
    }

    // Case 3: the consensus path rejects a hand-built block for both reasons (defense in depth).
    // Out-of-range nType is caught by the range check, a valid-but-wrong nType by the mismatch check.
    for (const auto& [nType, expected_reason] : {std::pair{static_cast<MnType>(42), "bad-protx-type"},
                                                 std::pair{MnType::Evo, "bad-protx-type-mismatch"}}) {
        auto tx = make_proupserv(nType);
        CMutableTransaction coinbase;
        coinbase.vin.emplace_back(COutPoint(), CScript() << OP_0 << OP_0);
        coinbase.vout.emplace_back(50 * COIN, coinbase_pk);

        CBlock block;
        block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
        block.vtx.emplace_back(MakeTransactionRef(tx));
        BlockValidationState state;
        CDeterministicMNList mn_list;
        LOCK(cs_main);
        BOOST_CHECK(!chainman.ActiveChainstate().ChainHelper().special_tx->BuildNewListFromBlock(
            block, tip_index(), IsV24Active(chainman), chainman.ActiveChainstate().CoinsTip(), /*debugLogs=*/false,
            state, mn_list));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), expected_reason);
    }

    // Case 4: the matching nType is still accepted, i.e. the new checks reject nothing legitimate.
    {
        auto tx = make_proupserv(MnType::Regular);
        TxValidationState check_state;
        {
            LOCK(cs_main);
            BOOST_CHECK_MESSAGE(CheckProUpServTx(CTransaction(tx), tip_index(), dmnman, chainman.GetConsensus(),
                                                 TipRules(chainman), check_state,
                                                 /*check_sigs=*/true),
                                "unexpected rejection: " << check_state.GetRejectReason());
        }

        LOCK(cs_main);
        const MempoolAcceptResult result = chainman.ProcessTransaction(MakeTransactionRef(tx));
        BOOST_CHECK_MESSAGE(result.m_result_type == MempoolAcceptResult::ResultType::VALID,
                            "unexpected rejection: " << result.m_state.GetRejectReason());
    }
}

void FuncTestMempoolReorg(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto tip_index  = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto tip_hash   = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    // Snapshot kept to sign the double-spend below, since FundTransaction() consumes utxos.
    const SimpleUTXOMap coins{utxos};

    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(PKHash(payoutKey.GetPubKey()));
    auto scriptCollateral = GetScriptForDestination(PKHash(collateralKey.GetPubKey()));

    // Create a MN with an external collateral
    auto tx_collateral = CreateSpendTx(chainman, utxos, scriptCollateral, dmn_types::Regular.collat_amount, setup.coinbaseKey);

    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_collateral}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    setup.m_node.dmnman->UpdatedBlockTip(tip_index());
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    BOOST_CHECK_EQUAL(block->GetHash(), tip_hash());

    const auto collateralOutpoint = GetCollateralOutpoint(tx_collateral);
    auto tx_reg = CreateProRegTxExternalCollateral(chainman, utxos, 1, collateralOutpoint, scriptPayout, ownerKey, operatorKey, collateralKey, setup.coinbaseKey);

    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;
    LOCK2(cs_main, testPool.cs);

    // Create ProUpServ and test block reorg which double-spend ProRegTx
    auto tx_up_serv = CreateProUpServTx(chainman, utxos, tx_reg.GetHash(), operatorKey, 2, CScript(), setup.coinbaseKey);
    testPool.addUnchecked(entry.FromTx(tx_up_serv));
    // A disconnected block would insert ProRegTx back into mempool
    testPool.addUnchecked(entry.FromTx(tx_reg));
    BOOST_CHECK_EQUAL(testPool.size(), 2U);

    // Create a tx that will double-spend ProRegTx
    CMutableTransaction tx_reg_ds;
    tx_reg_ds.vin = tx_reg.vin;
    tx_reg_ds.vout.emplace_back(0, CScript() << OP_RETURN);
    SignTransaction(tx_reg_ds, coins, setup.coinbaseKey);

    // Check mempool as if a new block with tx_reg_ds was connected instead of the old one with tx_reg
    std::vector<CTransactionRef> block_reorg;
    block_reorg.emplace_back(std::make_shared<CTransaction>(tx_reg_ds));
    testPool.removeForBlock(block_reorg, nHeight + 2);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
}

// Regression test: a ProUpRev/ProUpReg invalidates every pending ProTx of the same masternode, and
// removeProTxKeyChangedConflicts() collects those into a std::set<uint256> before removing any of
// them. When one conflicting TX is an in-mempool descendant of another, removeRecursive() on the
// ancestor also erases the descendant, so the descendant's txid in the snapshot no longer resolves.
// Dereferencing that stale iterator aborted the node; it must be skipped instead.
//
// Reachable from CTxMemPool::removeForBlock() (any block carrying such a ProTx).
void FuncTestMempoolProTxKeyChangedConflictChain(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey()));

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    // Only the resulting proTxHash matters here; the registration never has to be mined because
    // a pending ProUpServ of a masternode missing from the list is evicted by any operator key change.
    auto tx_reg = CreateProRegTx(chainman, utxos, 1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();

    // Parent ProUpServ for that masternode.
    auto tx_parent = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, 2, CScript(), setup.coinbaseKey);
    BOOST_REQUIRE(!tx_parent.vout.empty());

    // Child ProUpServ for the same masternode, spending the parent's first output.
    FillableSigningProvider keystore;
    BOOST_REQUIRE(keystore.AddKeyPubKey(setup.coinbaseKey, setup.coinbaseKey.GetPubKey()));

    CMutableTransaction tx_child;
    tx_child.nVersion = 3;
    tx_child.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    tx_child.vin.emplace_back(COutPoint(tx_parent.GetHash(), 0));
    tx_child.vout.emplace_back(0, scriptPayout); // value assigned by the grind loop below

    CProUpServTx payload;
    payload.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    payload.netInfo = NetInfoInterface::MakeNetInfo(payload.nVersion);
    payload.proTxHash = proTxHash;
    BOOST_REQUIRE_EQUAL(payload.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:3"), NetInfoStatus::Success);
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx_child));
    payload.sig = operatorKey.Sign(::SerializeHash(payload), bls::bls_legacy_scheme);
    SetTxPayload(tx_child, payload);

    // Grind the child's output value until the parent's txid sorts before the child's: the
    // snapshot is an ordered set, so this makes the parent get removed first, taking the child
    // with it, and only then is the child's now-stale txid revisited. Only the output value has to
    // change -- inputsHash covers the inputs alone, so the payload and its BLS signature stay put,
    // and re-signing replaces the scriptSig outright.
    bool parent_sorts_first{false};
    for (CAmount fee = 1000; fee < 2000 && !parent_sorts_first; ++fee) {
        tx_child.vout[0].nValue = tx_parent.vout[0].nValue - fee;
        BOOST_REQUIRE(SignSignature(keystore, CTransaction(tx_parent), tx_child, 0, SIGHASH_ALL));
        parent_sorts_first = tx_parent.GetHash() < tx_child.GetHash();
    }
    BOOST_REQUIRE(parent_sorts_first);

    // The revocation that invalidates both pending ProUpServ transactions.
    auto tx_revoke = CreateProUpRevTx(chainman, utxos, proTxHash, operatorKey, setup.coinbaseKey);

    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;
    LOCK2(cs_main, testPool.cs);

    testPool.addUnchecked(entry.FromTx(tx_parent));
    testPool.addUnchecked(entry.FromTx(tx_child));
    BOOST_CHECK_EQUAL(testPool.size(), 2U);

    // Pre-fix this aborted the process instead of returning.
    std::vector<CTransactionRef> block_txs{std::make_shared<CTransaction>(tx_revoke)};
    testPool.removeForBlock(block_txs, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
}

// Re-types a registration as an EvoNode with internal collateral claiming the given Platform node ID.
// Only the mempool conflict maps are consulted by the callers, so the payload does not have to be valid.
static CTransaction AsEvoWithPlatformNodeID(CMutableTransaction tx, const uint160& platform_node_id)
{
    auto pro_reg{*Assert(GetTxPayload<CProRegTx>(tx))};
    pro_reg.nVersion = ProTxVersion::BasicBLS;
    pro_reg.nType = MnType::Evo;
    pro_reg.collateralOutpoint = COutPoint{};
    pro_reg.platformNodeID = platform_node_id;
    SetTxPayload(tx, pro_reg);
    return CTransaction(tx);
}

// An ExtAddr registration with internal collateral, shared when shares are given. Its funding input is a
// placeholder: CheckProRegTx() does not look the funding inputs up.
static CTransaction CreateExtAddrProRegTx(MnType type, const MasternodePayoutShares& payouts, const CollateralShares& shares)
{
    CBLSSecretKey operator_key;
    operator_key.MakeNewKey();

    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::ExtAddr;
    pro_reg.nType = type;
    pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
    pro_reg.collateralOutpoint.n = 0;
    pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg.keyIDVoting = GenerateRandomKey().GetPubKey().GetID();
    if (type == MnType::Evo) {
        pro_reg.platformNodeID = uint160{Span{GetRandHash()}.first(uint160::size())};
    }
    if (shares.empty()) {
        pro_reg.keyIDOwner = GenerateRandomKey().GetPubKey().GetID();
        pro_reg.payouts = payouts;
    } else {
        pro_reg.shares = shares;
        pro_reg.vchJoinSigs.resize(shares.size());
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    tx.vin.emplace_back(COutPoint(GetRandHash(), 0));
    tx.vout.emplace_back(GetMnType(type).collat_amount,
                         shares.empty() ? GenerateRandomAddress() : SharedCollateralScript());
    pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx));
    SetTxPayload(tx, pro_reg);
    return CTransaction(tx);
}

void FuncTestMempoolDualProregtx(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);

    // Create a MN
    CKey ownerKey1;
    CBLSSecretKey operatorKey1;
    auto tx_reg1 = CreateProRegTx(chainman, utxos, 1, GenerateRandomAddress(), setup.coinbaseKey, ownerKey1, operatorKey1);

    // Create a MN with an external collateral that references tx_reg1
    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(PKHash(payoutKey.GetPubKey()));

    const auto collateralOutpoint = GetCollateralOutpoint(tx_reg1);
    auto tx_reg2 = CreateProRegTxExternalCollateral(chainman, utxos, 2, collateralOutpoint, scriptPayout, ownerKey, operatorKey, collateralKey, setup.coinbaseKey);

    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;
    LOCK2(cs_main, testPool.cs);

    testPool.addUnchecked(entry.FromTx(tx_reg1));
    BOOST_CHECK_EQUAL(testPool.size(), 1U);
    BOOST_CHECK(testPool.existsProviderTxConflict(CTransaction(tx_reg2)));

    // The masternode list keeps owner key IDs and Platform node IDs in one uniqueness namespace, so a
    // pending owner key and a pending Platform node ID with the same 160 bits cannot be mined in one block.
    const uint160 owner1_id{ownerKey1.GetPubKey().GetID()};
    // tx_reg2 has its own owner key, operator key and address; only its collateral reuse would clash with tx_reg1
    const auto tx_platform_reg = AsEvoWithPlatformNodeID(tx_reg2, owner1_id);
    const auto tx_unrelated_reg = AsEvoWithPlatformNodeID(tx_reg2, uint160{Span{GetRandHash()}.first(uint160::size())});
    const auto tx_platform_up_serv = [&] {
        auto tx = CreateProUpServTx(chainman, utxos, tx_reg1.GetHash(), operatorKey1, 5, CScript(), setup.coinbaseKey,
                                    ProTxVersion::BasicBLS);
        auto payload{*Assert(GetTxPayload<CProUpServTx>(tx))};
        payload.nType = MnType::Evo;
        payload.platformNodeID = owner1_id;
        SetTxPayload(tx, payload);
        return CTransaction(tx);
    }();

    // A pending owner key blocks a pending Platform node ID of the same value, at registration and at update
    BOOST_CHECK(testPool.existsProviderTxConflict(tx_platform_reg));
    BOOST_CHECK(testPool.existsProviderTxConflict(tx_platform_up_serv));
    BOOST_CHECK(!testPool.existsProviderTxConflict(tx_unrelated_reg));

    // ... and a pending Platform node ID blocks a pending owner key of the same value
    testPool.removeRecursive(CTransaction(tx_reg1), MemPoolRemovalReason::MANUAL);
    testPool.addUnchecked(entry.FromTx(MakeTransactionRef(tx_platform_reg)));
    BOOST_CHECK(testPool.existsProviderTxConflict(CTransaction(tx_reg1)));

    // A confirmed claim evicts the pending claim of the same value in the other field
    testPool.removeForBlock({MakeTransactionRef(tx_reg1)}, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK(!testPool.exists(tx_platform_reg.GetHash()));

    testPool.addUnchecked(entry.FromTx(tx_reg1));
    testPool.removeForBlock({MakeTransactionRef(tx_platform_reg)}, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK(!testPool.exists(tx_reg1.GetHash()));
    BOOST_CHECK_EQUAL(testPool.size(), 0U);

    // Share owner keys take part in the same namespace. The registrations below have distinct operator keys,
    // no addresses and internal collateral, so only the owner key / Platform node ID collision can conflict.
    const auto make_evo = [] { return CreateExtAddrProRegTx(MnType::Evo, {{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}}, {}); };
    const auto make_shared = [](const uint160& colliding_owner_id) {
        const CAmount collateral{GetMnType(MnType::Regular).collat_amount};
        return CreateExtAddrProRegTx(
            MnType::Regular, {},
            {{collateral - 100 * COIN, GenerateRandomAddress(), CScript{}, GenerateRandomKey().GetPubKey().GetID()},
             {100 * COIN, GenerateRandomAddress(), CScript{}, CKeyID{colliding_owner_id}}});
    };
    const auto platform_node_id = [](const CTransaction& tx) { return Assert(GetTxPayload<CProRegTx>(tx))->platformNodeID; };

    // A pending Platform node ID blocks a share owner key of the same value, and a confirmed share evicts it
    const auto tx_evo = make_evo();
    const auto tx_shared_on_evo = make_shared(platform_node_id(tx_evo));
    testPool.addUnchecked(entry.FromTx(MakeTransactionRef(tx_evo)));
    BOOST_CHECK(testPool.existsProviderTxConflict(tx_shared_on_evo));
    BOOST_CHECK(!testPool.existsProviderTxConflict(make_shared(uint160{Span{GetRandHash()}.first(uint160::size())})));
    testPool.removeForBlock({MakeTransactionRef(tx_shared_on_evo)}, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK(!testPool.exists(tx_evo.GetHash()));
    BOOST_CHECK_EQUAL(testPool.size(), 0U);

    // A pending share owner key blocks a Platform node ID of the same value, and a confirmed Evo registration evicts it
    const auto tx_shared = make_shared(uint160{Span{GetRandHash()}.first(uint160::size())});
    const uint160 share_owner_id{Assert(GetTxPayload<CProRegTx>(tx_shared))->shares.back().keyIDOwner};
    const auto with_platform_node_id = [](const CTransaction& tx, const uint160& id) {
        auto mtx = CMutableTransaction(tx);
        auto payload{*Assert(GetTxPayload<CProRegTx>(tx))};
        payload.platformNodeID = id;
        SetTxPayload(mtx, payload);
        return CTransaction(mtx);
    };
    const auto tx_evo_on_share = with_platform_node_id(make_evo(), share_owner_id);
    testPool.addUnchecked(entry.FromTx(MakeTransactionRef(tx_shared)));
    BOOST_CHECK(testPool.existsProviderTxConflict(tx_evo_on_share));
    BOOST_CHECK(!testPool.existsProviderTxConflict(make_evo()));
    testPool.removeForBlock({MakeTransactionRef(tx_evo_on_share)}, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK(!testPool.exists(tx_shared.GetHash()));
    BOOST_CHECK_EQUAL(testPool.size(), 0U);

    // An Evo registration may use the same value for its own owner key and Platform node ID, which is not a
    // conflict with itself or with unrelated pending registrations
    const auto tx_pending = make_evo();
    testPool.addUnchecked(entry.FromTx(MakeTransactionRef(tx_pending)));
    const auto tx_evo_base = make_evo();
    const auto tx_same_value = with_platform_node_id(tx_evo_base, uint160{Assert(GetTxPayload<CProRegTx>(tx_evo_base))->keyIDOwner});
    BOOST_CHECK(!testPool.existsProviderTxConflict(tx_same_value));
    testPool.addUnchecked(entry.FromTx(MakeTransactionRef(tx_same_value)));
    testPool.removeForBlock({MakeTransactionRef(tx_same_value)}, chainman.ActiveChain().Height() + 1);
    BOOST_CHECK(testPool.exists(tx_pending.GetHash()));
}

// A ProRegTx that reuses a confirmed external collateral replaces the live MN at block
// connect time. An update (ProUpServ/ProUpReg/ProUpRev) for the replaced proTxHash is still
// mempool-valid against the pre-block tip list, so fee-ordered packaging can put the replacement
// ProRegTx before the update and make BuildNewListFromBlock return bad-protx-hash, aborting
// CreateNewBlock/getblocktemplate. The mempool must treat them as conflicts.
void FuncTestMempoolProRegReplacementUpdateConflict(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);

    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;
    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(PKHash(payoutKey.GetPubKey()));
    auto scriptCollateral = GetScriptForDestination(PKHash(collateralKey.GetPubKey()));

    // Fund and mine an external collateral, then register MN X against it.
    auto tx_collateral = CreateSpendTx(chainman, utxos, scriptCollateral, dmn_types::Regular.collat_amount, setup.coinbaseKey);
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_collateral}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);

    const auto collateralOutpoint = GetCollateralOutpoint(tx_collateral);
    auto tx_reg = CreateProRegTxExternalCollateral(chainman, utxos, /*port=*/1, collateralOutpoint, scriptPayout,
                                                    ownerKey, operatorKey, collateralKey, setup.coinbaseKey);
    const uint256 proTxHash = tx_reg.GetHash();
    block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 2);
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(proTxHash));
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMNByCollateral(collateralOutpoint) != nullptr);
    BOOST_CHECK_EQUAL(dmnman.GetListAtChainTip().GetMNByCollateral(collateralOutpoint)->proTxHash.ToString(),
                      proTxHash.ToString());

    // Replacement ProRegTx reusing the same external collateral with fresh keys/address.
    CKey ownerKey2;
    CKey payoutKey2;
    CBLSSecretKey operatorKey2;
    ownerKey2.MakeNewKey(true);
    payoutKey2.MakeNewKey(true);
    operatorKey2.MakeNewKey();
    auto scriptPayout2 = GetScriptForDestination(PKHash(payoutKey2.GetPubKey()));
    auto tx_reg_replace = CreateProRegTxExternalCollateral(chainman, utxos, /*port=*/3, collateralOutpoint, scriptPayout2,
                                                            ownerKey2, operatorKey2, collateralKey, setup.coinbaseKey);

    // Same-block-style update for the MN that the replacement would delete.
    auto tx_up_serv = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/2, CScript(), setup.coinbaseKey);

    // Prove the in-block hazard itself: replacement first, then update of the deleted proTxHash.
    {
        LOCK(cs_main);
        CBlock hazard_block;
        hazard_block.vtx.emplace_back(MakeTransactionRef(CMutableTransaction())); // dummy coinbase slot
        hazard_block.vtx.emplace_back(MakeTransactionRef(tx_reg_replace));
        hazard_block.vtx.emplace_back(MakeTransactionRef(tx_up_serv));

        BlockValidationState state;
        CDeterministicMNList mn_list;
        BOOST_CHECK(!chainman.ActiveChainstate().ChainHelper().special_tx->BuildNewListFromBlock(
            hazard_block, tip_index(), IsV24Active(chainman), chainman.ActiveChainstate().CoinsTip(),
            /*debugLogs=*/false, state, mn_list));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-protx-hash");
    }

    // Mempool acceptance must refuse the second of these once one is present.
    CTxMemPool testPool{MemPoolOptionsForTest(setup.m_node)};
    TestMemPoolEntryHelper entry;

    {
        LOCK2(cs_main, testPool.cs);
        // Replacement already in mempool => update for the MN it replaces is a conflict.
        testPool.addUnchecked(entry.FromTx(tx_reg_replace));
        BOOST_CHECK_EQUAL(testPool.size(), 1U);
        BOOST_CHECK(testPool.existsProviderTxConflict(CTransaction(tx_up_serv)));
        testPool.removeRecursive(CTransaction(tx_reg_replace), MemPoolRemovalReason::MANUAL);
        BOOST_CHECK_EQUAL(testPool.size(), 0U);

        // Update already in mempool => replacement ProRegTx reusing that MN's collateral conflicts.
        testPool.addUnchecked(entry.FromTx(tx_up_serv));
        BOOST_CHECK_EQUAL(testPool.size(), 1U);
        BOOST_CHECK(testPool.existsProviderTxConflict(CTransaction(tx_reg_replace)));

        // existsProviderTxConflict only gates our own acceptance; it cannot stop a miner from
        // confirming the replacement. Once that block arrives, removeForBlock must evict the
        // now-unmineable update, otherwise it lingers and stalls our own block assembly.
        std::vector<CTransactionRef> connected{MakeTransactionRef(CMutableTransaction()),
                                               MakeTransactionRef(tx_reg_replace)};
        testPool.removeForBlock(connected, tip_height() + 1);
        BOOST_CHECK_EQUAL(testPool.size(), 0U);
    }

    // The replacement is not the only confirmed transaction that can leave updates of the MN
    // unmineable: so does a ProUpReg changing the operator key, and a spend of the collateral.
    auto tx_up_reg = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, operatorKey.GetPublicKey(),
                                      ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey);
    auto tx_up_reg_new_key = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, operatorKey2.GetPublicKey(),
                                              ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey);
    auto tx_up_rev = CreateProUpRevTx(chainman, utxos, proTxHash, operatorKey, setup.coinbaseKey);

    CMutableTransaction tx_spend_other;
    tx_spend_other.vin.emplace_back(COutPoint(tx_collateral.GetHash(), collateralOutpoint.n + 1));
    tx_spend_other.vout.emplace_back(0, CScript() << OP_RETURN);

    CMutableTransaction tx_spend_collateral;
    tx_spend_collateral.vin.emplace_back(collateralOutpoint);
    tx_spend_collateral.vout.emplace_back(0, CScript() << OP_RETURN);

    {
        LOCK2(cs_main, testPool.cs);
        // ProUpServ and ProUpRev are signed by the operator key, so they stay valid across a
        // confirmed ProUpReg that keeps it, and across a spend of anything but the collateral.
        testPool.addUnchecked(entry.FromTx(tx_up_serv));
        testPool.addUnchecked(entry.FromTx(tx_up_rev));
        testPool.removeForBlock({MakeTransactionRef(tx_up_reg), MakeTransactionRef(tx_spend_other)}, tip_height() + 1);
        BOOST_CHECK_EQUAL(testPool.size(), 2U);

        testPool.removeForBlock({MakeTransactionRef(tx_up_reg_new_key)}, tip_height() + 1);
        BOOST_CHECK_EQUAL(testPool.size(), 0U);

        // Spending the collateral removes the MN, and no update of any kind can be mined after that.
        testPool.addUnchecked(entry.FromTx(tx_up_serv));
        testPool.addUnchecked(entry.FromTx(tx_up_reg));
        testPool.addUnchecked(entry.FromTx(tx_up_rev));
        BOOST_CHECK_EQUAL(testPool.size(), 3U);
        testPool.removeForBlock({MakeTransactionRef(tx_spend_collateral)}, tip_height() + 1);
        BOOST_CHECK_EQUAL(testPool.size(), 0U);
    }
}

// Builds a block template from the given mempool transactions and returns the position of each
// transaction in it.
static std::map<uint256, size_t> TemplatePositions(TestChainSetup& setup,
                                                   const std::vector<std::pair<CMutableTransaction, CAmount>>& txs,
                                                   size_t block_max_size = DEFAULT_BLOCK_MAX_SIZE)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& mempool = *Assert(setup.m_node.mempool.get());
    TestMemPoolEntryHelper entry;
    {
        LOCK2(cs_main, mempool.cs);
        for (const auto& [tx, fee] : txs) {
            mempool.addUnchecked(entry.Fee(fee).FromTx(tx));
        }
    }
    node::BlockAssembler::Options options;
    options.nBlockMaxSize = block_max_size;
    const auto block_template = node::BlockAssembler{chainman.ActiveChainstate(), setup.m_node, &mempool, options}.CreateNewBlock(
        GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey()));
    BOOST_REQUIRE(block_template);
    std::map<uint256, size_t> positions;
    for (size_t i = 0; i < block_template->block.vtx.size(); ++i) {
        positions.emplace(block_template->block.vtx[i]->GetHash(), i);
    }
    {
        LOCK2(cs_main, mempool.cs);
        for (const auto& [tx, fee] : txs) {
            mempool.removeRecursive(CTransaction(tx), MemPoolRemovalReason::MANUAL);
        }
    }
    return positions;
}

// A pending ProUpServTx is signed with the operator key a key change replaces. Mined after the key
// change in the same block, it would restore the previous operator's service fields, so block
// assembly mines it first whatever the fees say.
void FuncTestMinerServiceUpdateBeforeKeyChange(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CKey otherOwnerKey;
    CBLSSecretKey operatorKey;
    CBLSSecretKey otherOperatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    auto tx_other_reg = CreateProRegTx(chainman, utxos, /*port=*/2, scriptPayout, setup.coinbaseKey, otherOwnerKey,
                                       otherOperatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(
        setup.CreateBlock({tx_reg, tx_other_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(proTxHash));

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount low_fee{1000};
    const CAmount high_fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_serv = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/3, CScript(), setup.coinbaseKey,
                                     version, low_fee);
    // Takes the address of the other masternode, so it cannot be mined at all
    auto tx_serv_unminable = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/2, CScript(),
                                               setup.coinbaseKey, version, low_fee);
    // The revocation spends what the key change spends; the two are never in the pool together
    auto revoke_utxos{utxos};
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          high_fee);
    auto tx_revoke = CreateProUpRevTx(chainman, revoke_utxos, proTxHash, operatorKey, setup.coinbaseKey, high_fee);
    auto tx_same_key = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, operatorKey.GetPublicKey(),
                                        ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version, high_fee);

    // The key change or revocation pays more, but the service update goes in first
    for (const auto& tx_change : {tx_key_change, tx_revoke}) {
        const auto positions = TemplatePositions(setup, {{tx_serv, low_fee}, {tx_change, high_fee}});
        BOOST_REQUIRE(positions.count(tx_serv.GetHash()) && positions.count(tx_change.GetHash()));
        BOOST_CHECK_LT(positions.at(tx_serv.GetHash()), positions.at(tx_change.GetHash()));
    }
    std::map<uint256, size_t> positions;

    // A registrar update that keeps the operator key is ordered by fee as usual
    positions = TemplatePositions(setup, {{tx_serv, low_fee}, {tx_same_key, high_fee}});
    BOOST_REQUIRE(positions.count(tx_serv.GetHash()) && positions.count(tx_same_key.GetHash()));
    BOOST_CHECK_LT(positions.at(tx_same_key.GetHash()), positions.at(tx_serv.GetHash()));

    // A service update that cannot be mined does not hold the key change back
    positions = TemplatePositions(setup, {{tx_serv_unminable, low_fee}, {tx_key_change, high_fee}});
    BOOST_CHECK(!positions.count(tx_serv_unminable.GetHash()));
    BOOST_CHECK(positions.count(tx_key_change.GetHash()));
}

// A ProUpServTx with a pending operator key change among its ancestors can only be mined after that
// change. For its own masternode it would then restore the previous operator's service fields; for
// another one it would hold back a key change of its own masternode in block assembly, as would a
// pending asset lock or unlock that block assembly turns down. The mempool refuses it whether it
// spends such a transaction directly or through another one, and accepts an independent ProUpServTx.
void FuncTestMempoolRejectsUnorderableServiceUpdate(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto& mempool = *Assert(setup.m_node.mempool.get());
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto submit = [&](const CMutableTransaction& tx) {
        return WITH_LOCK(::cs_main, return chainman.ProcessTransaction(MakeTransactionRef(tx)));
    };
    auto output_coins = [](const CMutableTransaction& tx) {
        return SimpleUTXOMap{{COutPoint(tx.GetHash(), 0), Coin(tx.vout[0], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    const CScript coinbase_pkh = GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey()));
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CKey otherOwnerKey;
    CBLSSecretKey operatorKey;
    CBLSSecretKey otherOperatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    auto tx_other_reg = CreateProRegTx(chainman, utxos, /*port=*/2, scriptPayout, setup.coinbaseKey, otherOwnerKey,
                                       otherOperatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(
        setup.CreateBlock({tx_reg, tx_other_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    CBLSSecretKey otherNewOperatorKey;
    newOperatorKey.MakeNewKey();
    otherNewOperatorKey.MakeNewKey();
    const CAmount fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version, fee);
    auto tx_other_key_change = CreateProUpRegTx(chainman, utxos, tx_other_reg.GetHash(), otherOwnerKey,
                                                otherNewOperatorKey.GetPublicKey(), otherOwnerKey.GetPubKey().GetID(),
                                                scriptPayout, setup.coinbaseKey, version, fee);
    BOOST_REQUIRE(submit(tx_key_change).m_result_type == MempoolAcceptResult::ResultType::VALID);
    BOOST_REQUIRE(submit(tx_other_key_change).m_result_type == MempoolAcceptResult::ResultType::VALID);

    // An ordinary transaction spending the change of the key change, for a service update to spend in turn
    BOOST_REQUIRE_EQUAL(tx_key_change.vout.size(), 2U);
    const COutPoint key_change_change{tx_key_change.GetHash(), 1};
    CMutableTransaction tx_spend_key_change;
    tx_spend_key_change.vin.emplace_back(key_change_change);
    tx_spend_key_change.vout.emplace_back(tx_key_change.vout[1].nValue - fee, coinbase_pkh);
    const SimpleUTXOMap spend_coins{
        {key_change_change, Coin(tx_key_change.vout[1], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    SignTransaction(tx_spend_key_change, spend_coins, setup.coinbaseKey);
    BOOST_REQUIRE(submit(tx_spend_key_change).m_result_type == MempoolAcceptResult::ResultType::VALID);

    // Parked in the mempool without validation, only to be an ancestor
    CMutableTransaction tx_asset_lock;
    tx_asset_lock.nVersion = 3;
    tx_asset_lock.nType = TRANSACTION_ASSET_LOCK;
    const auto lock_coins = FundTransaction(chainman, tx_asset_lock, utxos, coinbase_pkh, 1 * COIN);
    SetTxPayload(tx_asset_lock, CAssetLockPayload{});
    SignTransaction(tx_asset_lock, lock_coins, setup.coinbaseKey);
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(TestMemPoolEntryHelper{}.Fee(fee).Time(Now<NodeSeconds>()).FromTx(tx_asset_lock));
    }

    for (const auto& [funding, port] : std::vector<std::pair<CMutableTransaction, int>>{
             {tx_key_change, 3}, {tx_spend_key_change, 4}, {tx_other_key_change, 5}, {tx_asset_lock, 7}}) {
        auto coins = output_coins(funding);
        auto tx_serv = CreateProUpServTx(chainman, coins, proTxHash, operatorKey, port, CScript(), setup.coinbaseKey,
                                         version, fee);
        const auto result = submit(tx_serv);
        BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
        BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "protx-dup");
    }

    auto tx_serv = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/6, CScript(), setup.coinbaseKey,
                                     version, fee);
    BOOST_CHECK(submit(tx_serv).m_result_type == MempoolAcceptResult::ResultType::VALID);
}

// A reorg returns a mined key change to the mempool before the block tip notifications move the
// masternode list. Recorded against the list of the disconnected block, which already has the new
// key, it would not count as a pending key change, and the mempool would admit a second one for the
// masternode. A service update still goes in before it.
void FuncTestMinerServiceUpdateBeforeReorgedKeyChange(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto& mempool = *Assert(setup.m_node.mempool.get());
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount low_fee{1000};
    const CAmount high_fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          high_fee);
    block = std::make_shared<CBlock>(setup.CreateBlock({tx_key_change}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, tip_index()));
    BOOST_REQUIRE(WITH_LOCK(mempool.cs, return mempool.exists(tx_key_change.GetHash())));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey otherOperatorKey;
    otherOperatorKey.MakeNewKey();
    auto tx_other_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, otherOperatorKey.GetPublicKey(),
                                                ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                                high_fee);
    const auto result{WITH_LOCK(::cs_main, return chainman.ProcessTransaction(MakeTransactionRef(tx_other_key_change)))};
    BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
    BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "protx-dup");

    auto tx_serv = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/2, CScript(), setup.coinbaseKey,
                                     version, low_fee);
    const auto positions = TemplatePositions(setup, {{tx_serv, low_fee}});
    BOOST_REQUIRE(positions.count(tx_serv.GetHash()) && positions.count(tx_key_change.GetHash()));
    BOOST_CHECK_LT(positions.at(tx_serv.GetHash()), positions.at(tx_key_change.GetHash()));
}

// A registrar update that kept the operator key when it entered the mempool becomes a key change
// once a reorg disconnects the block that set that key. A service update signed with the key
// restored by the reorg still goes in before it.
void FuncTestMinerServiceUpdateBeforeKeyChangeSurvivingReorg(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto& mempool = *Assert(setup.m_node.mempool.get());
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount low_fee{1000};
    const CAmount high_fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          high_fee);
    block = std::make_shared<CBlock>(setup.CreateBlock({tx_key_change}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    // Keeps the key the mined change set, so it enters the mempool as no key change
    auto tx_same_key = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                        ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version, high_fee);
    BOOST_REQUIRE(WITH_LOCK(::cs_main, return chainman.ProcessTransaction(MakeTransactionRef(tx_same_key)))
                      .m_result_type == MempoolAcceptResult::ResultType::VALID);

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, tip_index()));
    dmnman.UpdatedBlockTip(tip_index());
    BOOST_REQUIRE(WITH_LOCK(mempool.cs, return mempool.exists(tx_same_key.GetHash())));

    auto tx_serv = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/2, CScript(), setup.coinbaseKey,
                                     version, low_fee);
    const auto positions = TemplatePositions(setup, {{tx_serv, low_fee}});
    BOOST_REQUIRE(positions.count(tx_serv.GetHash()) && positions.count(tx_same_key.GetHash()));
    BOOST_CHECK_LT(positions.at(tx_serv.GetHash()), positions.at(tx_same_key.GetHash()));
}

// However a ProUpServTx comes to follow a key change of its masternode in the pool (it spends the key
// change here), block assembly never mines it after the key change, but still mines one following a
// registrar update that keeps the operator key.
void FuncTestMinerServiceUpdateNeverAfterKeyChange(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto output_coins = [](const CMutableTransaction& tx) {
        return SimpleUTXOMap{{COutPoint(tx.GetHash(), 0), Coin(tx.vout[0], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version, fee);
    auto tx_same_key = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, operatorKey.GetPublicKey(),
                                        ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version, fee);

    auto key_change_coins = output_coins(tx_key_change);
    auto tx_serv = CreateProUpServTx(chainman, key_change_coins, proTxHash, operatorKey, /*port=*/2, CScript(),
                                     setup.coinbaseKey, version, fee);
    auto positions = TemplatePositions(setup, {{tx_key_change, fee}, {tx_serv, fee}});
    BOOST_CHECK(positions.count(tx_key_change.GetHash()));
    BOOST_CHECK(!positions.count(tx_serv.GetHash()));

    auto same_key_coins = output_coins(tx_same_key);
    auto tx_serv_after_same_key = CreateProUpServTx(chainman, same_key_coins, proTxHash, operatorKey, /*port=*/3,
                                                    CScript(), setup.coinbaseKey, version, fee);
    positions = TemplatePositions(setup, {{tx_same_key, fee}, {tx_serv_after_same_key, fee}});
    BOOST_REQUIRE(positions.count(tx_same_key.GetHash()) && positions.count(tx_serv_after_same_key.GetHash()));
    BOOST_CHECK_LT(positions.at(tx_same_key.GetHash()), positions.at(tx_serv_after_same_key.GetHash()));
}

// A reorg can return, below a pending ProUpServTx, an ancestor the mempool would have refused it
// for: here a key change of another masternode that pays nothing, so nothing else gets it mined.
// Block assembly leaves such a ProUpServTx out instead of holding back a key change of its
// masternode, and does not mine it after that key change either.
void FuncTestMinerKeyChangeNotHeldBackAfterReorg(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto& mempool = *Assert(setup.m_node.mempool.get());
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto submit = [&](const CMutableTransaction& tx) {
        return WITH_LOCK(::cs_main, return chainman.ProcessTransaction(MakeTransactionRef(tx)));
    };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CKey otherOwnerKey;
    CBLSSecretKey operatorKey;
    CBLSSecretKey otherOperatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    auto tx_other_reg = CreateProRegTx(chainman, utxos, /*port=*/2, scriptPayout, setup.coinbaseKey, otherOwnerKey,
                                       otherOperatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(
        setup.CreateBlock({tx_reg, tx_other_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    CBLSSecretKey otherNewOperatorKey;
    newOperatorKey.MakeNewKey();
    otherNewOperatorKey.MakeNewKey();
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_other_key_change = CreateProUpRegTx(chainman, utxos, tx_other_reg.GetHash(), otherOwnerKey,
                                                otherNewOperatorKey.GetPublicKey(), otherOwnerKey.GetPubKey().GetID(),
                                                scriptPayout, setup.coinbaseKey, version, /*fee=*/0);
    block = std::make_shared<CBlock>(setup.CreateBlock({tx_other_key_change}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    // Spends the confirmed key change, so the mempool admits it
    SimpleUTXOMap other_key_change_coins{{COutPoint(tx_other_key_change.GetHash(), 0),
                                          Coin(tx_other_key_change.vout[0], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    auto tx_serv = CreateProUpServTx(chainman, other_key_change_coins, proTxHash, operatorKey, /*port=*/3, CScript(),
                                     setup.coinbaseKey, version, /*fee=*/500);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          /*fee=*/100000);
    BOOST_REQUIRE(submit(tx_serv).m_result_type == MempoolAcceptResult::ResultType::VALID);
    BOOST_REQUIRE(submit(tx_key_change).m_result_type == MempoolAcceptResult::ResultType::VALID);

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, tip_index()));
    dmnman.UpdatedBlockTip(tip_index());
    {
        LOCK(mempool.cs);
        BOOST_REQUIRE(mempool.exists(tx_other_key_change.GetHash()));
        BOOST_REQUIRE(mempool.exists(tx_serv.GetHash()));
        BOOST_REQUIRE(mempool.exists(tx_key_change.GetHash()));
    }

    const auto positions = TemplatePositions(setup, {});
    BOOST_CHECK(positions.count(tx_key_change.GetHash()));
    BOOST_CHECK(!positions.count(tx_serv.GetHash()));
}

// A ProUpServTx funded by an ordinary pending transaction is pulled in ahead of a key change of its
// masternode together with that parent. With room for all of them they go in as parent, service
// update, key change. With room for either the parent and the update or the key change, but not for
// all three, the key change goes in without the update, and the template stays within its size limit.
void FuncTestMinerServiceUpdateWithParentBeforeKeyChange(TestChainSetup& setup, bool limit_block_size)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    const CScript coinbase_pkh = GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey()));
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount low_fee{1000};
    const CAmount high_fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    CMutableTransaction tx_parent;
    const auto parent_coins = FundTransaction(chainman, tx_parent, utxos, coinbase_pkh, 1 * COIN);
    tx_parent.vout.back().nValue -= low_fee;
    SignTransaction(tx_parent, parent_coins, setup.coinbaseKey);
    SimpleUTXOMap service_coins{
        {COutPoint(tx_parent.GetHash(), 0), Coin(tx_parent.vout[0], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    auto tx_serv = CreateProUpServTx(chainman, service_coins, proTxHash, operatorKey, /*port=*/2, CScript(),
                                     setup.coinbaseKey, version, low_fee);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          high_fee);
    const std::vector<std::pair<CMutableTransaction, CAmount>> txs{{tx_parent, low_fee},
                                                                   {tx_serv, low_fee},
                                                                   {tx_key_change, high_fee}};

    if (!limit_block_size) {
        const auto positions = TemplatePositions(setup, txs);
        BOOST_REQUIRE(positions.count(tx_parent.GetHash()) && positions.count(tx_serv.GetHash()) &&
                      positions.count(tx_key_change.GetHash()));
        BOOST_CHECK_LT(positions.at(tx_parent.GetHash()), positions.at(tx_serv.GetHash()));
        BOOST_CHECK_LT(positions.at(tx_serv.GetHash()), positions.at(tx_key_change.GetHash()));
        return;
    }

    TemplatePositions(setup, {});
    const auto base_size{static_cast<size_t>(*Assert(node::BlockAssembler::m_last_block_size))};
    const size_t service_size{::GetSerializeSize(CTransaction(tx_parent), PROTOCOL_VERSION) +
                              ::GetSerializeSize(CTransaction(tx_serv), PROTOCOL_VERSION)};
    const size_t key_change_size{::GetSerializeSize(CTransaction(tx_key_change), PROTOCOL_VERSION)};
    const size_t block_max_size{base_size + std::max(service_size, key_change_size) + 1};
    const auto positions = TemplatePositions(setup, txs, block_max_size);
    BOOST_CHECK(positions.count(tx_key_change.GetHash()));
    BOOST_CHECK(!positions.count(tx_serv.GetHash()));
    BOOST_CHECK_LT(static_cast<size_t>(*Assert(node::BlockAssembler::m_last_block_size)), block_max_size);
}

// The operator being replaced can keep several independently funded service updates of its masternode
// pending, together too large to go in ahead of a key change. Block assembly then mines the key change
// without them instead of holding it back, and does not mine them after it.
void FuncTestMinerKeyChangeNotHeldBackByServiceUpdates(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto tx_reg = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKey, operatorKey);
    const uint256 proTxHash = tx_reg.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKey;
    newOperatorKey.MakeNewKey();
    const CAmount low_fee{1000};
    const CAmount high_fee{100000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    auto tx_serv1 = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/2, CScript(), setup.coinbaseKey,
                                      version, low_fee);
    auto tx_serv2 = CreateProUpServTx(chainman, utxos, proTxHash, operatorKey, /*port=*/3, CScript(), setup.coinbaseKey,
                                      version, low_fee);
    auto tx_key_change = CreateProUpRegTx(chainman, utxos, proTxHash, ownerKey, newOperatorKey.GetPublicKey(),
                                          ownerKey.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                          high_fee);

    // Room for the key change and either update, but not both
    TemplatePositions(setup, {});
    const auto base_size{static_cast<size_t>(*Assert(node::BlockAssembler::m_last_block_size))};
    const size_t service_size{std::max(::GetSerializeSize(CTransaction(tx_serv1), PROTOCOL_VERSION),
                                       ::GetSerializeSize(CTransaction(tx_serv2), PROTOCOL_VERSION))};
    const size_t key_change_size{::GetSerializeSize(CTransaction(tx_key_change), PROTOCOL_VERSION)};
    const size_t block_max_size{base_size + key_change_size + service_size + 1};
    const auto positions = TemplatePositions(setup, {{tx_serv1, low_fee}, {tx_serv2, low_fee}, {tx_key_change, high_fee}},
                                             block_max_size);
    BOOST_CHECK(positions.count(tx_key_change.GetHash()));
    BOOST_CHECK(!positions.count(tx_serv1.GetHash()) && !positions.count(tx_serv2.GetHash()));
    BOOST_CHECK_LT(static_cast<size_t>(*Assert(node::BlockAssembler::m_last_block_size)), block_max_size);
}

// A key change goes in without the service updates of its masternode A when they do not fit. A
// pending service update of another masternode B, funded by one of A's, cannot go in after that
// either, and must not hold back B's key change.
void FuncTestMinerKeyChangeNotHeldBackByOtherKeyChange(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    const CScript scriptPayout = GenerateRandomAddress();

    CKey ownerKeyA;
    CKey ownerKeyB;
    CBLSSecretKey operatorKeyA;
    CBLSSecretKey operatorKeyB;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, /*port=*/1, scriptPayout, setup.coinbaseKey, ownerKeyA, operatorKeyA);
    auto tx_reg_b = CreateProRegTx(chainman, utxos, /*port=*/2, scriptPayout, setup.coinbaseKey, ownerKeyB, operatorKeyB);
    const uint256 proTxHashA = tx_reg_a.GetHash();
    const uint256 proTxHashB = tx_reg_b.GetHash();
    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg_a, tx_reg_b}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    // Matures enough coinbase outputs to fund the transactions below
    for (int i = 0; i < 5; ++i) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
    }
    dmnman.UpdatedBlockTip(tip_index());

    CBLSSecretKey newOperatorKeyA;
    CBLSSecretKey newOperatorKeyB;
    newOperatorKeyA.MakeNewKey();
    newOperatorKeyB.MakeNewKey();
    const CAmount low_fee{1000};
    const auto version = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    std::vector<CMutableTransaction> serv_a;
    for (uint16_t port = 3; port < 7; ++port) {
        serv_a.push_back(CreateProUpServTx(chainman, utxos, proTxHashA, operatorKeyA, port, CScript(),
                                           setup.coinbaseKey, version, low_fee));
    }
    SimpleUTXOMap serv_a_coins{
        {COutPoint(serv_a[0].GetHash(), 0), Coin(serv_a[0].vout[0], /*nHeightIn=*/0, /*fCoinBaseIn=*/false)}};
    auto tx_serv_b = CreateProUpServTx(chainman, serv_a_coins, proTxHashB, operatorKeyB, /*port=*/7, CScript(),
                                       setup.coinbaseKey, version, low_fee);
    auto tx_key_change_a = CreateProUpRegTx(chainman, utxos, proTxHashA, ownerKeyA, newOperatorKeyA.GetPublicKey(),
                                            ownerKeyA.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                            /*fee=*/200000);
    auto tx_key_change_b = CreateProUpRegTx(chainman, utxos, proTxHashB, ownerKeyB, newOperatorKeyB.GetPublicKey(),
                                            ownerKeyB.GetPubKey().GetID(), scriptPayout, setup.coinbaseKey, version,
                                            /*fee=*/100000);

    // Room for both key changes with B's update and its parent, but not for A's key change with all
    // of A's updates
    auto size = [](const CMutableTransaction& tx) { return ::GetSerializeSize(CTransaction(tx), PROTOCOL_VERSION); };
    TemplatePositions(setup, {});
    const auto base_size{static_cast<size_t>(*Assert(node::BlockAssembler::m_last_block_size))};
    const size_t block_max_size{base_size + size(tx_key_change_a) + size(tx_key_change_b) + size(serv_a[0]) +
                                size(tx_serv_b) + 1};
    BOOST_REQUIRE_GT(size(serv_a[1]) + size(serv_a[2]) + size(serv_a[3]), size(tx_key_change_b) + size(tx_serv_b) + 1);
    std::vector<std::pair<CMutableTransaction, CAmount>> txs;
    for (const auto& tx : serv_a) {
        txs.emplace_back(tx, low_fee);
    }
    txs.insert(txs.end(), {{tx_serv_b, low_fee}, {tx_key_change_a, 200000}, {tx_key_change_b, 100000}});
    const auto positions = TemplatePositions(setup, txs, block_max_size);
    BOOST_CHECK(positions.count(tx_key_change_a.GetHash()) && positions.count(tx_key_change_b.GetHash()));
    BOOST_CHECK(!positions.count(tx_serv_b.GetHash()));
    for (const auto& tx : serv_a) {
        BOOST_CHECK(!positions.count(tx.GetHash()));
    }
}

void FuncVerifyDB(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto tip_height   = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()); };
    auto tip_hash     = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->GetBlockHash()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };

    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    int nHeight = tip_height();
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);

    CKey ownerKey;
    CKey payoutKey;
    CKey collateralKey;
    CBLSSecretKey operatorKey;

    ownerKey.MakeNewKey(true);
    payoutKey.MakeNewKey(true);
    collateralKey.MakeNewKey(true);
    operatorKey.MakeNewKey();

    auto scriptPayout = GetScriptForDestination(PKHash(payoutKey.GetPubKey()));
    auto scriptCollateral = GetScriptForDestination(PKHash(collateralKey.GetPubKey()));

    // Create a MN with an external collateral
    auto tx_collateral = CreateSpendTx(chainman, utxos, scriptCollateral, dmn_types::Regular.collat_amount, setup.coinbaseKey);

    auto block = std::make_shared<CBlock>(setup.CreateBlock({tx_collateral}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 1);
    BOOST_CHECK_EQUAL(block->GetHash(), tip_hash());

    const auto collateralOutpoint = GetCollateralOutpoint(tx_collateral);
    auto tx_reg = CreateProRegTxExternalCollateral(chainman, utxos, 1, collateralOutpoint, scriptPayout, ownerKey, operatorKey, collateralKey, setup.coinbaseKey);

    auto tx_reg_hash = tx_reg.GetHash();

    block = std::make_shared<CBlock>(setup.CreateBlock({tx_reg}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 2);
    BOOST_CHECK_EQUAL(block->GetHash(), tip_hash());
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(tx_reg_hash));

    // Now spend the collateral while updating the same MN
    SimpleUTXOMap collateral_utxos;
    collateral_utxos.emplace(collateralOutpoint, Coin(tx_collateral.vout[collateralOutpoint.n], /*nHeightIn=*/1, /*fCoinBaseIn=*/false));
    auto proUpRevTx = CreateProUpRevTx(chainman, collateral_utxos, tx_reg_hash, operatorKey, collateralKey);

    block = std::make_shared<CBlock>(setup.CreateBlock({proUpRevTx}, coinbase_pk, chainman.ActiveChainstate()));
    BOOST_REQUIRE(chainman.ProcessNewBlock(block, true, nullptr));
    sync_dmn_tip();
    BOOST_CHECK_EQUAL(tip_height(), nHeight + 3);
    BOOST_CHECK_EQUAL(block->GetHash(), tip_hash());
    BOOST_REQUIRE(!dmnman.GetListAtChainTip().HasMN(tx_reg_hash));

    // Verify db consistency
    LOCK(cs_main);
    BOOST_REQUIRE(CVerifyDB().VerifyDB(chainman.ActiveChainstate(), Params().GetConsensus(),
                                       chainman.ActiveChainstate().CoinsTip(), *(setup.m_node.evodb), 4, 2) == VerifyDBResult::SUCCESS);
}

static CDeterministicMNCPtr create_mock_mn(uint64_t internal_id)
{
    // Create a mock MN
    CKey ownerKey;
    ownerKey.MakeNewKey(true);
    CBLSSecretKey operatorKey;
    operatorKey.MakeNewKey();

    auto dmnState = std::make_shared<CDeterministicMNState>();
    dmnState->confirmedHash = GetRandHash();
    dmnState->keyIDOwner = ownerKey.GetPubKey().GetID();
    dmnState->pubKeyOperator.Set(operatorKey.GetPublicKey(), bls::bls_legacy_scheme.load());
    dmnState->keyIDVoting = ownerKey.GetPubKey().GetID();
    dmnState->netInfo = NetInfoInterface::MakeNetInfo(
        ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false));
    BOOST_CHECK_EQUAL(dmnState->netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:1"), NetInfoStatus::Success);

    auto dmn = std::make_shared<CDeterministicMN>(internal_id, MnType::Regular);
    dmn->proTxHash = GetRandHash();
    dmn->collateralOutpoint = COutPoint(GetRandHash(), 0);
    dmn->nOperatorReward = 0;
    dmn->pdmnState = dmnState;

    return dmn;
}

static void SmlCache(TestChainSetup& setup)
{
    BOOST_CHECK(setup.m_node.dmnman != nullptr);

    // Create empty list and verify SML cache
    CDeterministicMNList emptyList(uint256(), 0, 0);
    auto sml_empty = emptyList.to_sml();

    // Should return the same cached object
    BOOST_CHECK(sml_empty == emptyList.to_sml());

    // Should contain empty list
    BOOST_CHECK_EQUAL(sml_empty->mnList.size(), 0);

    // Copy list should return the same cached object
    CDeterministicMNList mn_list_1(emptyList);
    BOOST_CHECK(sml_empty == mn_list_1.to_sml());

    CDeterministicMNList mn_list_2;
    // Assigning list should return the same cached object
    mn_list_2 = emptyList;
    BOOST_CHECK(sml_empty == mn_list_2.to_sml());

    auto dmn = create_mock_mn(1);

    // Add MN - should invalidate cache
    mn_list_1.AddMN(dmn, true);
    auto sml_add = mn_list_1.to_sml();

    // Cache should be invalidated, so different pointer but equal content after regeneration
    BOOST_CHECK(sml_empty != sml_add); // Different pointer (cache invalidated)

    BOOST_CHECK_EQUAL(sml_add->mnList.size(), 1); // Should contain the added MN

    {
        // Remove MN - should invalidate cache
        CDeterministicMNList mn_list(mn_list_1);
        BOOST_CHECK(mn_list_1.to_sml() == mn_list.to_sml());

        mn_list.RemoveMN(dmn->proTxHash);
        auto sml_remove = mn_list.to_sml();

        // Cache should be invalidated
        BOOST_CHECK(sml_remove != sml_add);
        BOOST_CHECK(sml_remove != sml_empty);
        BOOST_CHECK_EQUAL(sml_remove->mnList.size(), 0); // Should be empty after removal
    }

    // Start with a list containing one MN mn_list_1
    // Test 1: Update with same SML entry data - cache should NOT be invalidated
    auto unchangedState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
    unchangedState->nPoSePenalty += 10;
    mn_list_1.UpdateMN(*dmn, unchangedState);

    // Cache should NOT be invalidated since SML entry didn't change
    BOOST_CHECK(sml_add == mn_list_1.to_sml()); // Same pointer (cache preserved)

    // Test 2: Update with different SML entry data - cache SHOULD be invalidated
    auto changedState = std::make_shared<CDeterministicMNState>(*unchangedState);
    changedState->pubKeyOperator.Set(CBLSPublicKey{}, bls::bls_legacy_scheme.load());
    mn_list_1.UpdateMN(*dmn, changedState);

    // Cache should be invalidated since SML entry changed
    BOOST_CHECK(sml_add != mn_list_1.to_sml());
    BOOST_CHECK_EQUAL(mn_list_1.to_sml()->mnList.size(), 1); // Still one MN but with updated data
}

BOOST_AUTO_TEST_SUITE(evo_dip3_activation_tests)

// FuncDIP3Protx registers six masternodes in successive blocks. Height 109 is the lowest boundary
// that keeps two mature coinbases available for every 1000 DASH collateral; height 108 runs out on
// the sixth registration.
constexpr int DIP3_ACTIVATION_HEIGHT{109};

BOOST_AUTO_TEST_CASE(deterministic_mn_list_diff_serialization_is_canonical)
{
    CDeterministicMNListDiff forward;
    forward.updatedMNs.emplace(1, CDeterministicMNStateDiff{});
    forward.updatedMNs.emplace(2, CDeterministicMNStateDiff{});

    CDeterministicMNListDiff reverse;
    reverse.updatedMNs.emplace(2, CDeterministicMNStateDiff{});
    reverse.updatedMNs.emplace(1, CDeterministicMNStateDiff{});

    CDataStream forward_stream{SER_DISK, CLIENT_VERSION};
    CDataStream reverse_stream{SER_DISK, CLIENT_VERSION};
    forward_stream << forward;
    reverse_stream << reverse;

    BOOST_CHECK_EQUAL_COLLECTIONS(forward_stream.begin(), forward_stream.end(),
                                  reverse_stream.begin(), reverse_stream.end());
}

struct TestChainDIP3BeforeActivationSetup : public TestChainSetup {
    TestChainDIP3BeforeActivationSetup() :
        TestChainSetup(DIP3_ACTIVATION_HEIGHT - 2, CBaseChainParams::REGTEST, {"-dip3params=109:500"},
                       /*coins_db_in_memory=*/true, /*block_tree_db_in_memory=*/true)
    {
    }
};

struct TestChainDIP3Setup : public TestChainDIP3BeforeActivationSetup {
    TestChainDIP3Setup()
    {
        // Activate DIP3 here
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }
};

struct TestMNChainSetup : public TestChainSetup {
    TestMNChainSetup(int num_blocks, const std::vector<const char*>& extra_args) :
        TestChainSetup(num_blocks, CBaseChainParams::REGTEST, extra_args,
                       /*coins_db_in_memory=*/true, /*block_tree_db_in_memory=*/true),
        chainman(*Assert(m_node.chainman)),
        dmnman(*Assert(m_node.dmnman)),
        utxos(BuildSimpleUtxoMap(m_coinbase_txns)),
        coinbase_pk(GetScriptForRawPubKey(coinbaseKey.GetPubKey()))
    {
    }

    const CBlockIndex* Tip() const
    {
        return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip());
    }

    bool IsV19Active() const
    {
        return DeploymentActiveAfter(Tip(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19);
    }

    bool IsV24Active() const
    {
        return DeploymentActiveAfter(Tip(), chainman, Consensus::DEPLOYMENT_V24);
    }

    void ProcessBlock(const std::vector<CMutableTransaction>& txs = {})
    {
        CreateAndProcessBlock(txs, coinbase_pk);
        dmnman.UpdatedBlockTip(Tip());
    }

    void MineToV19()
    {
        while (!IsV19Active()) {
            ProcessBlock();
        }
    }

    void MineToV24()
    {
        for (int i = 0; i < 2000 && !IsV24Active(); ++i) {
            ProcessBlock();
        }
        BOOST_REQUIRE(IsV24Active());
    }

    ChainstateManager& chainman;
    CDeterministicMNManager& dmnman;
    SimpleUTXOMap utxos;
    const CScript coinbase_pk;
};

struct TestChainV24SignalBeforeV19Setup : public TestMNChainSetup {
    TestChainV24SignalBeforeV19Setup() :
        TestMNChainSetup(494,
                         {"-testactivationheight=v19@500", "-testactivationheight=v20@500",
                          "-testactivationheight=mn_rr@511", "-vbparams=v24:0:9999999999:510:1:1:1:5:0"})
    {
        assert(!IsV19Active());
        assert(!IsV24Active());
    }
};

// Advance the shared v24 chain just far enough that v19/v20 are active while v24 remains held in
// LOCKED_IN by its minimum activation height. Delaying mn_rr until the block after v24 keeps the
// subsidy-only masternode reward even on both sides of the boundary, so the duplicate-payment
// helper does not need to mine to the next subsidy epoch.
struct TestChainV24PendingSetup : public TestChainV24SignalBeforeV19Setup {
    TestChainV24PendingSetup()
    {
        // Mine just enough to activate v19/v20 (height 500) while keeping v24 and mn_rr pending.
        for (int i = 0; i < 20 && !IsV19Active(); ++i) {
            ProcessBlock();
        }
        assert(IsV19Active());
        assert(!IsV24Active());
        assert(!bls::bls_legacy_scheme.load());
    }
};

// DIP3 can only be activated with legacy scheme (v19 is activated later)
BOOST_AUTO_TEST_CASE(dip3_activation_legacy)
{
    TestChainDIP3BeforeActivationSetup setup;
    FuncDIP3Activation(setup);
}

// V19 can only be activated with legacy scheme
BOOST_AUTO_TEST_CASE(v19_activation_legacy)
{
    TestChainV19BeforeActivationSetup setup;
    FuncV19Activation(setup);
}

// The invariant this whole change rests on: a stored operator key never advertises a scheme its own
// state version contradicts, so the live list and the same list reloaded from disk agree — including
// mnUniquePropertyMap, which IsEqual() compares directly.
static void CheckListRoundTrips(CDeterministicMNManager& dmnman, const std::string& what)
{
    const auto live = dmnman.GetListAtChainTip();
    live.ForEachMN(false, [&](const CDeterministicMN& dmn) {
        BOOST_CHECK_MESSAGE(dmn.pdmnState->pubKeyOperator == CBLSLazyPublicKey() ||
                            dmn.pdmnState->pubKeyOperator.IsLegacy() ==
                                (dmn.pdmnState->nVersion == ProTxVersion::LegacyBLS),
                            what << ": key scheme contradicts state version for " << dmn.proTxHash.ToString());
    });
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << live;
    CDeterministicMNList reloaded;
    ds >> reloaded;
    BOOST_CHECK_MESSAGE(live.IsEqual(reloaded), what << ": live list diverges from its serialized form");
}

void FuncMigrationRejectedWhenKeySquatted(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;

    // Victim A: legacy, holding operator key K.
    CKey owner_key_a;
    CBLSSecretKey operator_key;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key);
    const auto proTxHashA = tx_reg_a.GetHash();
    setup.ProcessBlock({tx_reg_a});

    // Reach v19, register squatter B holding K basic-encoded (accepted pre-v24), then activate v24.
    setup.MineToV19();
    BOOST_REQUIRE(!DeploymentActiveAfter(setup.Tip(), chainman, Consensus::DEPLOYMENT_V24));

    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20003"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_b;
    tx_reg_b.nVersion = 3;
    tx_reg_b.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_b, utxos, pro_reg_b.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_b));
        SetTxPayload(tx_reg_b, pro_reg_b);
        SignTransaction(tx_reg_b, spent, setup.coinbaseKey);
    }
    setup.ProcessBlock({tx_reg_b});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(tx_reg_b.GetHash()));

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHashA)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    // (a) A's service-update migration is rejected cleanly.
    {
        CProUpServTx ups;
        ups.nVersion = ProTxVersion::BasicBLS;
        ups.netInfo = NetInfoInterface::MakeNetInfo(ups.nVersion);
        ups.proTxHash = proTxHashA;
        BOOST_REQUIRE_EQUAL(ups.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:19999"), NetInfoStatus::Success);
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
        const auto spent = FundTransaction(chainman, tx, utxos,
                                           GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
        ups.inputsHash = CalcTxInputsHash(CTransaction(tx));
        ups.sig = operator_key.Sign(::SerializeHash(ups), bls::bls_legacy_scheme);
        SetTxPayload(tx, ups);
        SignTransaction(tx, spent, setup.coinbaseKey);
        TxValidationState st;
        LOCK(cs_main);
        BOOST_CHECK(!CheckProUpServTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman, chainman.GetConsensus(),
                                      TipRules(chainman), st, true));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-dup-key");
    }

    // (b) A's same-key registrar migration is rejected cleanly.
    {
        CProUpRegTx upreg;
        upreg.nVersion = ProTxVersion::BasicBLS;
        upreg.proTxHash = proTxHashA;
        upreg.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
        upreg.keyIDVoting = owner_key_a.GetPubKey().GetID();
        upreg.scriptPayout = GenerateRandomAddress();
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
        const auto spent = FundTransaction(chainman, tx, utxos,
                                           GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
        upreg.inputsHash = CalcTxInputsHash(CTransaction(tx));
        CHashSigner::SignHash(::SerializeHash(upreg), owner_key_a, upreg.vchSig);
        SetTxPayload(tx, upreg);
        SignTransaction(tx, spent, setup.coinbaseKey);
        TxValidationState st;
        LOCK(cs_main);
        BOOST_CHECK(!CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                     chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                     TipRules(chainman), st, true));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-dup-key");
    }
};

void FuncProUpServTxMigratesLegacy(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.ProcessBlock({tx_reg});
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    setup.MineToV24();

    // A BasicBLS service update migrates a legacy masternode in place: it keeps the same operator
    // key (re-encoded to the basic scheme), raises the version, and is NOT PoSe-banned -- no key
    // rotation is forced.
    auto tx = CreateProUpServTx(chainman, utxos, proTxHash, operator_key, 19998, CScript(), setup.coinbaseKey,
                                ProTxVersion::BasicBLS);
    {
        TxValidationState val_state;
        LOCK(cs_main);
        BOOST_REQUIRE_MESSAGE(CheckProUpServTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                               chainman.GetConsensus(), TipRules(chainman), val_state,
                                               /*check_sigs=*/true),
                              "migration ProUpServTx rejected: " << val_state.GetRejectReason());
    }
    setup.ProcessBlock({tx});

    const auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::BasicBLS);
    BOOST_CHECK(!dmn->pdmnState->pubKeyOperator.IsLegacy());
    BOOST_CHECK(!dmn->pdmnState->IsBanned());
    // Same underlying key, and the list round-trips (key re-encoded, so a reloaded node decodes the
    // same operator key an online node built).
    BOOST_CHECK(dmn->pdmnState->pubKeyOperator.Get() == operator_key.GetPublicKey());
    CheckListRoundTrips(dmnman, "after legacy->basic migration ProUpServTx");
    {
        const auto live = dmnman.GetListAtChainTip();
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << live;
        CDeterministicMNList reloaded;
        ds >> reloaded;
        BOOST_CHECK(reloaded.GetMN(proTxHash)->pdmnState->pubKeyOperator.Get() == operator_key.GetPublicKey());
    }
};

BOOST_AUTO_TEST_CASE(migration_rejected_when_key_squatted)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncMigrationRejectedWhenKeySquatted(setup);
}

BOOST_AUTO_TEST_CASE(proupserv_migrates_legacy)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProUpServTxMigratesLegacy(setup);
}

// The Platform GUI consumes DAPI gateway endpoints through the node interface
// (MnEntry::getPlatformHTTPSAddrs). Both EvoNode generations must export a gateway: a pre-ExtAddr
// registration carries the Platform HTTPS port as a scalar paired with the primary core address,
// while an ExtAddr registration carries explicit PLATFORM_HTTPS netInfo entries where domain
// entries are skipped (unresolvable without DNS lookups the interface must not perform) and the
// leading CService entry keeps the EvoNode reachable. Regular masternodes export nothing.
void FuncPlatformHTTPSAddrsViaNodeInterface(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;
    auto& utxos = setup.utxos;

    setup.MineToV19();
    BOOST_REQUIRE(!setup.IsV24Active());
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    // Regular masternode: no Platform endpoints to export.
    CKey regular_owner_key;
    CBLSSecretKey regular_operator_key;
    auto tx_reg_regular = CreateProRegTx(chainman, utxos, 20000, GenerateRandomAddress(), setup.coinbaseKey,
                                         regular_owner_key, regular_operator_key);
    const auto regular_protx_hash = tx_reg_regular.GetHash();
    setup.ProcessBlock({tx_reg_regular});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(regular_protx_hash));

    // Legacy (pre-ExtAddr) EvoNode: the Platform HTTPS port is a scalar field, the gateway address
    // is the primary core address paired with it.
    CKey evo_owner_key;
    evo_owner_key.MakeNewKey(true);
    CBLSSecretKey evo_operator_key;
    evo_operator_key.MakeNewKey();
    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::BasicBLS;
    pro_reg.nType = MnType::Evo;
    pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
    pro_reg.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20100"), NetInfoStatus::Success);
    pro_reg.platformNodeID.SetHex("00112233445566778899aabbccddeeff00112233");
    pro_reg.platformP2PPort = 20101;
    pro_reg.platformHTTPPort = 20102;
    pro_reg.keyIDOwner = evo_owner_key.GetPubKey().GetID();
    pro_reg.pubKeyOperator.Set(evo_operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
    pro_reg.keyIDVoting = evo_owner_key.GetPubKey().GetID();
    pro_reg.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_evo;
    tx_reg_evo.nVersion = 3;
    tx_reg_evo.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_evo, utxos, pro_reg.scriptPayout,
                                           dmn_types::Evo.collat_amount);
        pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_evo));
        SetTxPayload(tx_reg_evo, pro_reg);
        SignTransaction(tx_reg_evo, spent, setup.coinbaseKey);
    }
    const auto evo_protx_hash = tx_reg_evo.GetHash();
    setup.ProcessBlock({tx_reg_evo});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(evo_protx_hash));

    setup.MineToV24();
    // Not upgraded to ExtAddr by activation alone: the state keeps the scalar-port layout.
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(evo_protx_hash)->pdmnState->nVersion,
                        ProTxVersion::BasicBLS);

    // ExtAddr EvoNode: explicit PLATFORM_HTTPS entries, a CService first and a domain second.
    CKey evo2_owner_key;
    evo2_owner_key.MakeNewKey(true);
    CBLSSecretKey evo2_operator_key;
    evo2_operator_key.MakeNewKey();
    CProRegTx pro_reg2;
    pro_reg2.nVersion = ProTxVersion::ExtAddr;
    pro_reg2.nType = MnType::Evo;
    pro_reg2.netInfo = NetInfoInterface::MakeNetInfo(pro_reg2.nVersion);
    pro_reg2.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg2.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.3:20200"), NetInfoStatus::Success);
    BOOST_REQUIRE_EQUAL(pro_reg2.netInfo->AddEntry(NetInfoPurpose::PLATFORM_P2P, "1.1.1.3:20201"),
                        NetInfoStatus::Success);
    BOOST_REQUIRE_EQUAL(pro_reg2.netInfo->AddEntry(NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.3:20202"),
                        NetInfoStatus::Success);
    BOOST_REQUIRE_EQUAL(pro_reg2.netInfo->AddEntry(NetInfoPurpose::PLATFORM_HTTPS, "platform.example.com:443"),
                        NetInfoStatus::Success);
    pro_reg2.platformNodeID.SetHex("445566778899aabbccddeeff0011223344556677");
    pro_reg2.keyIDOwner = evo2_owner_key.GetPubKey().GetID();
    pro_reg2.pubKeyOperator.Set(evo2_operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg2.keyIDVoting = evo2_owner_key.GetPubKey().GetID();
    pro_reg2.payouts = {{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}};
    CMutableTransaction tx_reg_evo2;
    tx_reg_evo2.nVersion = 3;
    tx_reg_evo2.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        // The collateral must not pay to a payout script from ExtAddr onwards, so fund it from and
        // change back to the coinbase key's script.
        const auto spent = FundTransaction(chainman, tx_reg_evo2, utxos,
                                           GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())),
                                           dmn_types::Evo.collat_amount);
        pro_reg2.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_evo2));
        SetTxPayload(tx_reg_evo2, pro_reg2);
        SignTransaction(tx_reg_evo2, spent, setup.coinbaseKey);
    }
    const auto evo2_protx_hash = tx_reg_evo2.GetHash();
    setup.ProcessBlock({tx_reg_evo2});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().HasMN(evo2_protx_hash));

    auto node{interfaces::MakeNode(setup.m_node)};
    auto [mn_list, tip] = node->evo().getListAtChainTip();
    BOOST_REQUIRE(mn_list != nullptr);
    BOOST_REQUIRE(tip != nullptr);

    std::map<uint256, interfaces::MnEntryCPtr> entries;
    mn_list->forEachMN(/*only_valid=*/false,
                       [&entries](const interfaces::MnEntryCPtr& mn) { entries.emplace(mn->getProTxHash(), mn); });
    BOOST_REQUIRE_EQUAL(entries.size(), 3U);

    // Regular masternode contributes nothing.
    BOOST_CHECK(entries.at(regular_protx_hash)->getType() == MnType::Regular);
    BOOST_CHECK(entries.at(regular_protx_hash)->getPlatformHTTPSAddrs().empty());

    // Legacy EvoNode: primary core address paired with the scalar HTTPS port.
    {
        const auto& entry = entries.at(evo_protx_hash);
        BOOST_CHECK(entry->getType() == MnType::Evo);
        const auto addrs = entry->getPlatformHTTPSAddrs();
        BOOST_REQUIRE_EQUAL(addrs.size(), 1U);
        BOOST_CHECK(addrs[0] == LookupNumeric("1.1.1.2", 20102));
        BOOST_CHECK(static_cast<CNetAddr>(addrs[0]) == static_cast<CNetAddr>(entry->getNetInfoPrimary()));
    }

    // ExtAddr EvoNode: the CService entry is exported, the domain entry is skipped.
    {
        const auto& entry = entries.at(evo2_protx_hash);
        BOOST_CHECK(entry->getType() == MnType::Evo);
        const auto addrs = entry->getPlatformHTTPSAddrs();
        BOOST_REQUIRE_EQUAL(addrs.size(), 1U);
        BOOST_CHECK(addrs[0] == LookupNumeric("1.1.1.3", 20202));

        // Pin the shape the interface relies on: the first stored PLATFORM_HTTPS entry is a
        // CService, so an EvoNode never contributes an empty gateway set even though domain
        // entries are skipped.
        const auto dmn = dmnman.GetListAtChainTip().GetMN(evo2_protx_hash);
        BOOST_REQUIRE(dmn != nullptr);
        const auto https_entries = dmn->pdmnState->netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS);
        BOOST_REQUIRE_EQUAL(https_entries.size(), 2U);
        BOOST_REQUIRE(https_entries[0].GetAddrPort().has_value());
        BOOST_CHECK(addrs[0] == *https_entries[0].GetAddrPort());
        BOOST_CHECK(https_entries[1].GetDomainPort().has_value());
    }
}

BOOST_AUTO_TEST_CASE(platform_https_addrs_via_node_interface)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncPlatformHTTPSAddrsViaNodeInterface(setup);
}

static uint256 RegisterBasicEvoNode(TestChainV24SignalBeforeV19Setup& setup, const std::string& addr,
                                    uint16_t platform_http_port, const CKey& owner_key, const CBLSSecretKey& operator_key)
{
    CProRegTx pro_reg;
    pro_reg.nVersion = ProTxVersion::BasicBLS;
    pro_reg.nType = MnType::Evo;
    pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
    pro_reg.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("%s:20300", addr)),
                        NetInfoStatus::Success);
    pro_reg.platformNodeID.SetHex("8899aabbccddeeff00112233445566778899aabb");
    pro_reg.platformP2PPort = 20301;
    pro_reg.platformHTTPPort = platform_http_port;
    pro_reg.keyIDOwner = owner_key.GetPubKey().GetID();
    pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg.keyIDVoting = owner_key.GetPubKey().GetID();
    pro_reg.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    const auto spent = FundTransaction(setup.chainman, tx, setup.utxos, pro_reg.scriptPayout, dmn_types::Evo.collat_amount);
    pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx));
    SetTxPayload(tx, pro_reg);
    SignTransaction(tx, spent, setup.coinbaseKey);
    setup.ProcessBlock({tx});
    BOOST_REQUIRE(setup.dmnman.GetListAtChainTip().HasMN(tx.GetHash()));
    return tx.GetHash();
}

static CMutableTransaction CreateExtAddrProUpRegTx(TestChainV24SignalBeforeV19Setup& setup, const uint256& proTxHash,
                                                   const CKey& owner_key, const CBLSSecretKey& operator_key,
                                                   MasternodePayoutShares payouts = {
                                                       {GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}})
{
    CProUpRegTx proTx;
    proTx.nVersion = ProTxVersion::ExtAddr;
    proTx.proTxHash = proTxHash;
    proTx.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    proTx.keyIDVoting = owner_key.GetPubKey().GetID();
    proTx.payouts = std::move(payouts);
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent = FundTransaction(setup.chainman, tx, setup.utxos,
                                       GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    CHashSigner::SignHash(::SerializeHash(proTx), owner_key, proTx.vchSig);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, setup.coinbaseKey);
    return tx;
}

static CMutableTransaction CreateEvoProUpServTx(TestChainV24SignalBeforeV19Setup& setup, const uint256& proTxHash,
                                                uint16_t version, const std::string& addr, const CBLSSecretKey& operator_key)
{
    CProUpServTx proTx;
    proTx.nVersion = version;
    proTx.nType = MnType::Evo;
    proTx.netInfo = NetInfoInterface::MakeNetInfo(proTx.nVersion);
    proTx.proTxHash = proTxHash;
    BOOST_REQUIRE_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("%s:20300", addr)),
                        NetInfoStatus::Success);
    proTx.platformNodeID.SetHex("8899aabbccddeeff00112233445566778899aabb");
    if (version >= ProTxVersion::ExtAddr) {
        BOOST_REQUIRE_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::PLATFORM_P2P, strprintf("%s:20301", addr)),
                            NetInfoStatus::Success);
        BOOST_REQUIRE_EQUAL(proTx.netInfo->AddEntry(NetInfoPurpose::PLATFORM_HTTPS, strprintf("%s:443", addr)),
                            NetInfoStatus::Success);
    } else {
        proTx.platformP2PPort = 20301;
        proTx.platformHTTPPort = 443;
    }
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    const auto spent = FundTransaction(setup.chainman, tx, setup.utxos,
                                       GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    proTx.sig = operator_key.Sign(::SerializeHash(proTx), /*specificLegacyScheme=*/false);
    SetTxPayload(tx, proTx);
    SignTransaction(tx, spent, setup.coinbaseKey);
    return tx;
}

// From ExtAddr on an EvoNode's Platform ports live in netInfo and are not derived from a BasicBLS
// state's scalar ports: the derived IP:port entries could be held by another masternode, and the
// re-key in UpdateMN() would then throw out of block assembly. Only a service update that spells out
// the Platform entries raises a BasicBLS EvoNode to ExtAddr.
void FuncEvoNodeRaisedToExtAddrByServiceUpdateOnly(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    setup.MineToV19();
    CKey owner_key;
    owner_key.MakeNewKey(true);
    CBLSSecretKey operator_key;
    operator_key.MakeNewKey();
    const auto proTxHash = RegisterBasicEvoNode(setup, "1.1.1.4", 443, owner_key, operator_key);
    setup.MineToV24();

    auto check_proupreg = [&](const CMutableTransaction& tx) {
        TxValidationState val_state;
        LOCK(cs_main);
        CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman, chainman.ActiveChainstate().CoinsTip(),
                        chainman.GetConsensus(), TipRules(chainman), val_state, /*check_sigs=*/true);
        return val_state.GetRejectReason();
    };
    auto check_proupserv = [&](const CMutableTransaction& tx) {
        TxValidationState val_state;
        LOCK(cs_main);
        CheckProUpServTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman, chainman.GetConsensus(),
                         TipRules(chainman), val_state, /*check_sigs=*/true);
        return val_state.GetRejectReason();
    };
    auto check_state = [&](uint16_t version, const std::string& what) {
        const auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
        BOOST_REQUIRE(dmn);
        BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, version);
        if (version >= ProTxVersion::ExtAddr) {
            const auto entries = dmn->pdmnState->netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS);
            BOOST_REQUIRE_EQUAL(entries.size(), 1U);
            BOOST_CHECK(entries[0].GetAddrPort() == LookupNumeric("1.1.1.4", 443));
            BOOST_CHECK_EQUAL(dmn->pdmnState->platformHTTPPort, 0);
        }
        CheckListRoundTrips(dmnman, what);
    };

    BOOST_CHECK_EQUAL(check_proupreg(CreateExtAddrProUpRegTx(setup, proTxHash, owner_key, operator_key)),
                      "bad-protx-netinfo-version");
    BOOST_CHECK_EQUAL(check_proupserv(CreateEvoProUpServTx(setup, proTxHash, ProTxVersion::BasicBLS, "1.1.1.4", operator_key)),
                      "bad-protx-version-disallowed");
    check_state(ProTxVersion::BasicBLS, "after rejected raises");

    {
        const auto tx = CreateEvoProUpServTx(setup, proTxHash, ProTxVersion::ExtAddr, "1.1.1.4", operator_key);
        BOOST_CHECK_EQUAL(check_proupserv(tx), "");
        setup.ProcessBlock({tx});
        check_state(ProTxVersion::ExtAddr, "after ExtAddr ProUpServTx");
    }
    {
        const auto tx = CreateExtAddrProUpRegTx(setup, proTxHash, owner_key, operator_key);
        BOOST_CHECK_EQUAL(check_proupreg(tx), "");
        setup.ProcessBlock({tx});
        check_state(ProTxVersion::ExtAddr, "after ExtAddr ProUpRegTx");
    }
    BOOST_CHECK_EQUAL(check_proupserv(CreateEvoProUpServTx(setup, proTxHash, ProTxVersion::BasicBLS, "1.1.1.4", operator_key)),
                      "bad-protx-version-disallowed");
}

BOOST_AUTO_TEST_CASE(evonode_raised_to_extaddr_by_service_update_only)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncEvoNodeRaisedToExtAddrByServiceUpdateOnly(setup);
}

// A registrar update that would raise a BasicBLS EvoNode to ExtAddr is refused up front: once in the
// mempool it would otherwise be selected into every block template and make block assembly fail.
void FuncEvoNodeRegistrarRaiseNotMined(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    setup.MineToV19();
    CKey owner_key;
    owner_key.MakeNewKey(true);
    CBLSSecretKey operator_key;
    operator_key.MakeNewKey();
    const auto proTxHash = RegisterBasicEvoNode(setup, "1.1.1.5", 443, owner_key, operator_key);
    setup.MineToV24();

    const auto tx = CreateExtAddrProUpRegTx(setup, proTxHash, owner_key, operator_key);
    {
        TxValidationState val_state;
        LOCK(cs_main);
        BOOST_CHECK(!CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                     chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                     TipRules(chainman), val_state, /*check_sigs=*/true));
        BOOST_CHECK_EQUAL(val_state.GetRejectReason(), "bad-protx-netinfo-version");
    }

    auto& mempool = *Assert(setup.m_node.mempool.get());
    TestMemPoolEntryHelper entry;
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(entry.Fee(50000).Time(Now<NodeSeconds>()).FromTx(tx));
        BOOST_REQUIRE(mempool.exists(tx.GetHash()));
    }
    const auto block_template = node::BlockAssembler{chainman.ActiveChainstate(), setup.m_node, &mempool}.CreateNewBlock(
        setup.coinbase_pk);
    BOOST_REQUIRE(block_template != nullptr);
    for (const auto& block_tx : block_template->block.vtx) {
        BOOST_CHECK(block_tx->GetHash() != tx.GetHash());
    }
    BOOST_CHECK_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::BasicBLS);
}

BOOST_AUTO_TEST_CASE(evonode_registrar_raise_not_mined)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncEvoNodeRegistrarRaiseNotMined(setup);
}

// Until evo_shares activates, an EvoNode registers with a single owner payout and without shared
// collateral. Regular masternodes are not restricted.
void FuncEvoNodeRegistrationNeedsEvoShares(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    setup.MineToV19();
    setup.MineToV24();

    const MasternodePayoutShares one_payout{{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}};
    const MasternodePayoutShares two_payouts{{GenerateRandomAddress(), 6000}, {GenerateRandomAddress(), 4000}};
    const auto two_shares = [](MnType type) {
        const CAmount collateral{GetMnType(type).collat_amount};
        return CollateralShares{{collateral - 100 * COIN, GenerateRandomAddress(), CScript{},
                                 GenerateRandomKey().GetPubKey().GetID()},
                                {100 * COIN, GenerateRandomAddress(), CScript{}, GenerateRandomKey().GetPubKey().GetID()}};
    };

    struct Case {
        std::string name;
        MnType type;
        MasternodePayoutShares payouts;
        CollateralShares shares;
        bool evo_shares;
        std::string expected;
    };
    const std::vector<Case> cases{
        {"evo, one payout", MnType::Evo, one_payout, {}, false, ""},
        {"evo, two payouts", MnType::Evo, two_payouts, {}, false, "bad-protx-payouts-evo"},
        {"evo, two payouts, evo_shares", MnType::Evo, two_payouts, {}, true, ""},
        {"regular, two payouts", MnType::Regular, two_payouts, {}, false, ""},
        {"evo, shared", MnType::Evo, {}, two_shares(MnType::Evo), false, "bad-protx-shares-evo"},
        {"evo, shared, evo_shares", MnType::Evo, {}, two_shares(MnType::Evo), true, ""},
        {"regular, shared", MnType::Regular, {}, two_shares(MnType::Regular), false, ""},
    };
    for (const auto& c : cases) {
        const auto tx{CreateExtAddrProRegTx(c.type, c.payouts, c.shares)};
        TxValidationState state;
        LOCK(cs_main);
        SpecialTxRules rules{TipRules(chainman)};
        BOOST_REQUIRE(rules.v24 && !rules.evo_shares);
        rules.evo_shares = c.evo_shares;
        CheckProRegTx(tx, chainman.ActiveChain().Tip(), dmnman, chainman.ActiveChainstate().CoinsTip(),
                      chainman.GetConsensus(), rules, state, /*check_sigs=*/false);
        BOOST_CHECK_MESSAGE(state.GetRejectReason() == c.expected,
                            strprintf("%s: got '%s'", c.name, state.GetRejectReason()));
    }
}

BOOST_AUTO_TEST_CASE(evonode_registration_needs_evo_shares)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncEvoNodeRegistrationNeedsEvoShares(setup);
}

// Until evo_shares activates, a registrar update keeps an EvoNode at a single owner payout, also when it
// rotates the operator key
void FuncEvoNodePayoutUpdateNeedsEvoShares(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    setup.MineToV19();
    CKey owner_key;
    owner_key.MakeNewKey(true);
    CBLSSecretKey operator_key;
    operator_key.MakeNewKey();
    CBLSSecretKey new_operator_key;
    new_operator_key.MakeNewKey();
    const auto proTxHash = RegisterBasicEvoNode(setup, "1.1.1.6", 443, owner_key, operator_key);
    setup.MineToV24();
    setup.ProcessBlock({CreateEvoProUpServTx(setup, proTxHash, ProTxVersion::ExtAddr, "1.1.1.6", operator_key)});

    const MasternodePayoutShares two_payouts{{GenerateRandomAddress(), 6000}, {GenerateRandomAddress(), 4000}};
    struct Case {
        std::string name;
        const CBLSSecretKey& operator_key;
        MasternodePayoutShares payouts;
        bool evo_shares;
        std::string expected;
    };
    const std::vector<Case> cases{
        {"one payout", operator_key, {{GenerateRandomAddress(), MasternodePayoutShare::MAX_REWARD}}, false, ""},
        {"two payouts", operator_key, two_payouts, false, "bad-protx-payouts-evo"},
        {"two payouts, operator key rotation", new_operator_key, two_payouts, false, "bad-protx-payouts-evo"},
        {"two payouts, evo_shares", operator_key, two_payouts, true, ""},
        {"two payouts, operator key rotation, evo_shares", new_operator_key, two_payouts, true, ""},
    };
    for (const auto& c : cases) {
        const auto tx{CreateExtAddrProUpRegTx(setup, proTxHash, owner_key, c.operator_key, c.payouts)};
        TxValidationState state;
        LOCK(cs_main);
        SpecialTxRules rules{TipRules(chainman)};
        BOOST_REQUIRE(rules.v24 && !rules.evo_shares);
        rules.evo_shares = c.evo_shares;
        CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman, chainman.ActiveChainstate().CoinsTip(),
                        chainman.GetConsensus(), rules, state, /*check_sigs=*/true);
        BOOST_CHECK_MESSAGE(state.GetRejectReason() == c.expected,
                            strprintf("%s: got '%s'", c.name, state.GetRejectReason()));
    }
}

BOOST_AUTO_TEST_CASE(evonode_payout_update_needs_evo_shares)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncEvoNodePayoutUpdateNeedsEvoShares(setup);
}

// The SAME masternode, two registrar updates in one block, version-crossing. tx1 rotates a
// legacy MN to a new key at v2 (making it BasicBLS); tx2 then rotates it to another new key at v1.
// tx2 passes CheckProUpRegTx against pindexPrev (the MN was legacy there), but in the rebuild the MN
// is already BasicBLS, so the round-3 guard (which keys off old_version == LegacyBLS) does not fire,
// and the v1-payload key is placed into a v2 state. Assert the final key's scheme matches its version
// and the list round-trips.
void FuncSameMnSameBlockVersionCrossingKeyRotation(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key;
    CBLSSecretKey operator_key_old;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key_old);
    const auto proTxHash = tx_reg.GetHash();
    setup.ProcessBlock({tx_reg});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    CBLSSecretKey key1, key2;
    key1.MakeNewKey();
    key2.MakeNewKey();
    auto build_upreg = [&](const CBLSSecretKey& new_op, uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, proTxHash, owner_key, new_op.GetPublicKey(),
                                owner_key.GetPubKey().GetID(), GenerateRandomAddress(), setup.coinbaseKey, version);
    };
    auto tx1 = build_upreg(key1, ProTxVersion::BasicBLS); // rotate to K1, bump to v2
    auto tx2 = build_upreg(key2, ProTxVersion::LegacyBLS); // rotate to K2 at v1

    // Both pass their own consensus check against pindexPrev (MN is still legacy there).
    {
        LOCK(cs_main);
        TxValidationState s1, s2;
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx1), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s1, true),
                              "tx1 rejected standalone: " << s1.GetRejectReason());
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx2), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s2, true),
                              "tx2 rejected standalone: " << s2.GetRejectReason());
    }

    // Rebuild the list with both in one block, tx1 then tx2.
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{})); // vtx[0] skipped as coinbase
    block.vtx.push_back(MakeTransactionRef(tx1));
    block.vtx.push_back(MakeTransactionRef(tx2));
    BlockValidationState block_state;
    CDeterministicMNList mn_list_ret;
    bool rebuilt{false};
    std::string thrown;
    {
        LOCK(cs_main);
        auto& chain_helper = *Assert(setup.m_node.chain_helper.get());
        try {
            rebuilt = chain_helper.special_tx->RebuildListFromBlock(
                block, chainman.ActiveChain().Tip(), IsV24Active(chainman), dmnman.GetListAtChainTip(),
                chainman.ActiveChainstate().CoinsTip(), /*debugLogs=*/false, block_state, mn_list_ret);
        } catch (const std::exception& e) {
            thrown = e.what();
        }
    }
    BOOST_TEST_MESSAGE("rebuilt=" << rebuilt << " reject=" << block_state.GetRejectReason() << " threw=" << thrown);
    // A throw escaping RebuildListFromBlock is the block-assembly stall this PR must prevent, so it has
    // to fail the test rather than silently skip the state checks below.
    BOOST_REQUIRE_MESSAGE(thrown.empty(), "RebuildListFromBlock threw: " << thrown);

    // If the block is accepted, the resulting state must be consistent (key scheme matches version,
    // and the live list equals its serialized form). A mismatch here is the desync this PR must
    // eliminate.
    if (rebuilt) {
        const auto dmn = mn_list_ret.GetMN(proTxHash);
        BOOST_REQUIRE(dmn);
        BOOST_CHECK_MESSAGE(dmn->pdmnState->pubKeyOperator == CBLSLazyPublicKey() ||
                            dmn->pdmnState->pubKeyOperator.IsLegacy() ==
                                (dmn->pdmnState->nVersion == ProTxVersion::LegacyBLS),
                            "DESYNC: nVersion=" << dmn->pdmnState->nVersion << " but key IsLegacy()="
                                                << dmn->pdmnState->pubKeyOperator.IsLegacy());
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << mn_list_ret;
        CDeterministicMNList reloaded;
        ds >> reloaded;
        BOOST_CHECK_MESSAGE(mn_list_ret.IsEqual(reloaded), "DESYNC: live list != serialized-and-reloaded list");
        // The stored key must still represent the point tx2 rotated to. If SetLegacy() left legacy
        // bytes under a basic flag, Get() decodes them under the wrong scheme and yields a different
        // point -- an operator key nobody can sign for (the MN is bricked), even if flag/version and
        // the serialized bytes are self-consistent.
        BOOST_CHECK_MESSAGE(dmn->pdmnState->pubKeyOperator.Get() == key2.GetPublicKey(),
                            "CORRUPT KEY (online): stored operator key decodes to a different point than tx2 set");
        // The decisive check: a node that reloaded this list from disk decodes the stored bytes fresh.
        // If online (cached object) and reloaded (decoded bytes) yield different points, two honest
        // nodes disagree on this operator key -- a reconstruction-history split, exactly the bug class
        // this PR exists to eliminate.
        const auto dmn_reloaded = reloaded.GetMN(proTxHash);
        BOOST_REQUIRE(dmn_reloaded);
        BOOST_CHECK_MESSAGE(dmn_reloaded->pdmnState->pubKeyOperator.Get() == key2.GetPublicKey(),
                            "RECONSTRUCTION SPLIT: reloaded operator key decodes to a different point "
                            "than the online list");
        BOOST_CHECK_MESSAGE(dmn_reloaded->pdmnState->pubKeyOperator.Get() ==
                                dmn->pdmnState->pubKeyOperator.Get(),
                            "RECONSTRUCTION SPLIT: online vs reloaded operator key differ");
    }
};

// The same masternode, twice in one block. Each transaction is checked against pindexPrev, where the
// key is still the original, so both look like genuine rotations and pass. But the rebuild computes
// operator_changed against the list as rebuilt so far, where the first transaction has already
// stored the new key -- and operator== ignores the encoding, so the second looks like a no-op, skips
// SetLegacy(), and yet still raises the version. That is bug A recreated from inside a single block.
void FuncSameMnSameBlockMigrationConsistent(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key;
    CBLSSecretKey operator_key_old;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key_old);
    const auto proTxHash = tx_reg.GetHash();
    setup.ProcessBlock({tx_reg});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    // Both rotate to the SAME new key, one legacy-encoded at v1, one basic-encoded at v2.
    CBLSSecretKey operator_key_new;
    operator_key_new.MakeNewKey();
    auto build_upreg = [&](uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, proTxHash, owner_key, operator_key_new.GetPublicKey(),
                                owner_key.GetPubKey().GetID(), GenerateRandomAddress(), setup.coinbaseKey, version);
    };
    auto tx1 = build_upreg(ProTxVersion::LegacyBLS);
    auto tx2 = build_upreg(ProTxVersion::BasicBLS);

    // Each is a genuine rotation against pindexPrev, so both pass their own consensus check.
    {
        LOCK(cs_main);
        TxValidationState s1, s2;
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx1), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s1, true),
                              "tx1 rejected standalone: " << s1.GetRejectReason());
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx2), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s2, true),
                              "tx2 rejected standalone: " << s2.GetRejectReason());
    }

    // Both rotate to the same new key -- tx1 at v1 (legacy-encoded), tx2 migrating to v2
    // (basic-encoded). The rebuild must accept this and leave a consistent state: SetStateVersion
    // re-encodes the key on the migration, so the second transaction does not desync the scheme.
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{})); // vtx[0] is skipped as the coinbase
    block.vtx.push_back(MakeTransactionRef(tx1));
    block.vtx.push_back(MakeTransactionRef(tx2));
    BlockValidationState block_state;
    CDeterministicMNList mn_list_ret;
    bool rebuilt{false};
    {
        LOCK(cs_main);
        auto& chain_helper = *Assert(setup.m_node.chain_helper.get());
        BOOST_CHECK_NO_THROW(
            rebuilt = chain_helper.special_tx->RebuildListFromBlock(
                block, chainman.ActiveChain().Tip(), IsV24Active(chainman), dmnman.GetListAtChainTip(),
                chainman.ActiveChainstate().CoinsTip(), /*debugLogs=*/false, block_state, mn_list_ret));
    }
    BOOST_REQUIRE_MESSAGE(rebuilt, "same-key migration across one block was rejected: " << block_state.GetRejectReason());
    const auto dmn = mn_list_ret.GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::BasicBLS);
    BOOST_CHECK(!dmn->pdmnState->pubKeyOperator.IsLegacy());
    // Online and reloaded decode the same operator key -- no reconstruction-history divergence.
    BOOST_CHECK(dmn->pdmnState->pubKeyOperator.Get() == operator_key_new.GetPublicKey());
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << mn_list_ret;
    CDeterministicMNList reloaded;
    ds >> reloaded;
    BOOST_CHECK(mn_list_ret.IsEqual(reloaded));
    BOOST_CHECK(reloaded.GetMN(proTxHash)->pdmnState->pubKeyOperator.Get() == operator_key_new.GetPublicKey());
};

// The activation-boundary case, closed at the root. Nothing evicts mempool entries when v24 activates,
// and block assembly rechecks each candidate independently against the tip rather than cumulatively,
// so a pair admitted beforehand would still be selected together afterwards and the rebuild would
// then reject the whole block -- an honest miner unable to build one at all. The mempool probe is
// therefore ungated policy: the pair can never become resident in the first place. This asserts that,
// and that a template still builds afterwards.
void FuncPreV24CrossSchemePairCannotBecomeResident(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;
    const auto& coinbase_pk = setup.coinbase_pk;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;

    CKey owner_key_a;
    CBLSSecretKey operator_key_a;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key_a);
    const auto proTxHashA = tx_reg_a.GetHash();
    setup.ProcessBlock({tx_reg_a});

    // Reach v19 but stop short of v24: this is where the pair gets admitted.
    setup.MineToV19();
    BOOST_REQUIRE(!DeploymentActiveAfter(setup.Tip(), chainman, Consensus::DEPLOYMENT_V24));

    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CBLSSecretKey operator_key_b;
    operator_key_b.MakeNewKey();
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20301"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key_b.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_b;
    tx_reg_b.nVersion = 3;
    tx_reg_b.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_b, utxos, pro_reg_b.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_b));
        SetTxPayload(tx_reg_b, pro_reg_b);
        SignTransaction(tx_reg_b, spent, setup.coinbaseKey);
    }
    const auto proTxHashB = tx_reg_b.GetHash();
    setup.ProcessBlock({tx_reg_b});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(proTxHashB));

    CBLSSecretKey contested;
    contested.MakeNewKey();
    auto build_upreg = [&](const uint256& protx_hash, const CKey& owner_key, uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, protx_hash, owner_key, contested.GetPublicKey(),
                                owner_key.GetPubKey().GetID(), GenerateRandomAddress(), setup.coinbaseKey, version,
                                /*fee=*/100000);
    };

    // The first claim is admitted pre-v24; the ungated mempool policy rejects the conflicting one.
    auto tx_a = build_upreg(proTxHashA, owner_key_a, ProTxVersion::LegacyBLS);
    auto tx_b = build_upreg(proTxHashB, owner_key_b, ProTxVersion::BasicBLS);
    {
        LOCK(cs_main);
        const auto ra = chainman.ProcessTransaction(MakeTransactionRef(tx_a));
        BOOST_REQUIRE_MESSAGE(ra.m_result_type == MempoolAcceptResult::ResultType::VALID,
                              "pre-v24 tx_a rejected: " << ra.m_state.GetRejectReason());
        const auto rb = chainman.ProcessTransaction(MakeTransactionRef(tx_b));
        BOOST_CHECK_MESSAGE(rb.m_result_type != MempoolAcceptResult::ResultType::VALID,
                            "the cross-scheme pair was admitted and will survive activation");
        BOOST_CHECK_EQUAL(rb.m_state.GetRejectReason(), "protx-dup");
    }

    // Activate v24 with the surviving transaction still resident.
    setup.MineToV24();

    // tx_a is the valid, non-conflicting claim: it must still be resident, not evicted at activation.
    auto& mempool = *Assert(setup.m_node.mempool.get());
    BOOST_REQUIRE(WITH_LOCK(mempool.cs, return mempool.exists(tx_a.GetHash())));

    // A template must still build and must actually select tx_a: proving the honest miner is not left
    // unable to produce a block, and that this passes because the valid transaction survives rather
    // than because every special transaction was dropped.
    std::unique_ptr<node::CBlockTemplate> tmpl;
    auto make_template = [&] {
        tmpl = node::BlockAssembler{chainman.ActiveChainstate(), setup.m_node, &mempool}.CreateNewBlock(coinbase_pk);
    };
    BOOST_CHECK_NO_THROW(make_template());
    BOOST_REQUIRE_MESSAGE(tmpl != nullptr, "no template could be built with a pre-v24 cross-scheme pair resident");
    bool selected_tx_a{false};
    for (const auto& tx : tmpl->block.vtx) {
        selected_tx_a |= tx->GetHash() == tx_a.GetHash();
    }
    BOOST_CHECK_MESSAGE(selected_tx_a, "the valid surviving special transaction was not selected into the template");
};

// The same-block case. Per-transaction checks run against pindexPrev, so two registrar updates in one
// block are invisible to each other and could claim one operator key under different encodings. The
// mempool keeps an honest miner from assembling that pair; this covers a hand-crafted block, which
// must be rejected cleanly rather than by AddMN/UpdateMN throwing out of block assembly.
void FuncSameBlockCrossSchemeKeyPairRejected(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;

    CKey owner_key_a;
    CBLSSecretKey operator_key_a;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key_a);
    const auto proTxHashA = tx_reg_a.GetHash();
    setup.ProcessBlock({tx_reg_a});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHashA)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CBLSSecretKey operator_key_b;
    operator_key_b.MakeNewKey();
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20201"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key_b.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_b;
    tx_reg_b.nVersion = 3;
    tx_reg_b.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_b, utxos, pro_reg_b.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_b));
        SetTxPayload(tx_reg_b, pro_reg_b);
        SignTransaction(tx_reg_b, spent, setup.coinbaseKey);
    }
    const auto proTxHashB = tx_reg_b.GetHash();
    setup.ProcessBlock({tx_reg_b});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(proTxHashB));

    CBLSSecretKey contested;
    contested.MakeNewKey();
    auto build_upreg = [&](const uint256& protx_hash, const CKey& owner_key, uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, protx_hash, owner_key, contested.GetPublicKey(),
                                owner_key.GetPubKey().GetID(), GenerateRandomAddress(), setup.coinbaseKey, version);
    };

    // MN-B claims the key basic-encoded, MN-A claims the same key legacy-encoded. Each passes its own
    // consensus check against the previous block's list.
    auto tx1 = build_upreg(proTxHashB, owner_key_b, ProTxVersion::BasicBLS);
    auto tx2 = build_upreg(proTxHashA, owner_key_a, ProTxVersion::LegacyBLS);
    {
        LOCK(cs_main);
        TxValidationState s1, s2;
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx1), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s1, true),
                              "tx1 rejected standalone: " << s1.GetRejectReason());
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx2), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), s2, true),
                              "tx2 rejected standalone: " << s2.GetRejectReason());
    }

    // Together in one block the rebuild must refuse them -- cleanly, with a reject reason, rather than
    // by AddMN/UpdateMN throwing. The rebuild is driven directly here because the test fixture's
    // CreateAndProcessBlock asserts rather than reporting a rebuild failure, so it cannot express a
    // block that is meant to be rejected.
    CBlock block;
    // The rebuild skips vtx[0] as the coinbase, so a placeholder must occupy it or the first real
    // transaction is silently never processed.
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    block.vtx.push_back(MakeTransactionRef(tx1));
    block.vtx.push_back(MakeTransactionRef(tx2));
    BlockValidationState block_state;
    CDeterministicMNList mn_list_ret;
    bool rebuilt{true};
    {
        LOCK(cs_main);
        auto& chain_helper = *Assert(setup.m_node.chain_helper.get());
        BOOST_CHECK_NO_THROW(
            rebuilt = chain_helper.special_tx->RebuildListFromBlock(
                block, chainman.ActiveChain().Tip(), IsV24Active(chainman), dmnman.GetListAtChainTip(),
                chainman.ActiveChainstate().CoinsTip(), /*debugLogs=*/false, block_state, mn_list_ret));
    }
    BOOST_CHECK_MESSAGE(!rebuilt, "a block claiming one operator key under two encodings was accepted");
    BOOST_CHECK_EQUAL(block_state.GetRejectReason(), "bad-protx-dup-key");
};

// Two in-flight registrar updates must not be able to claim the same operator key under different
// encodings. Each is valid against the confirmed list -- neither masternode holds the key yet, so
// each is invisible to the other's consensus check. Block assembly revalidates each candidate only
// against the confirmed tip rather than cumulatively, so admitting both would hand an honest miner
// a template whose block is invalid.
void FuncMempoolRejectsCrossSchemeKeyRace(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;

    // MN-A stays legacy; MN-B is registered basic after v24. Both keep their own keys for now.
    CKey owner_key_a;
    CBLSSecretKey operator_key_a;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key_a);
    const auto proTxHashA = tx_reg_a.GetHash();
    setup.ProcessBlock({tx_reg_a});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHashA)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CBLSSecretKey operator_key_b;
    operator_key_b.MakeNewKey();
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20101"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key_b.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_b;
    tx_reg_b.nVersion = 3;
    tx_reg_b.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_b, utxos, pro_reg_b.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_b));
        SetTxPayload(tx_reg_b, pro_reg_b);
        SignTransaction(tx_reg_b, spent, setup.coinbaseKey);
    }
    const auto proTxHashB = tx_reg_b.GetHash();
    setup.ProcessBlock({tx_reg_b});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(proTxHashB));

    // The contested key: held by nobody yet, so both updates below pass their consensus checks.
    CBLSSecretKey contested;
    contested.MakeNewKey();

    auto build_upreg = [&](const uint256& protx_hash, const CKey& owner_key, uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, protx_hash, owner_key, contested.GetPublicKey(),
                                owner_key.GetPubKey().GetID(), GenerateRandomAddress(), setup.coinbaseKey, version,
                                /*fee=*/100000);
    };

    // MN-B claims it basic-encoded. Accepted: nobody holds it.
    auto tx_first = build_upreg(proTxHashB, owner_key_b, ProTxVersion::BasicBLS);
    {
        LOCK(cs_main);
        const auto res = chainman.ProcessTransaction(MakeTransactionRef(tx_first));
        BOOST_REQUIRE_MESSAGE(res.m_result_type == MempoolAcceptResult::ResultType::VALID,
                              "first cross-scheme claim rejected: " << res.m_state.GetRejectReason());
    }

    // MN-A claims the SAME key legacy-encoded. Its consensus check still passes -- the confirmed list
    // does not contain the key -- so only the mempool can catch this.
    auto tx_second = build_upreg(proTxHashA, owner_key_a, ProTxVersion::LegacyBLS);
    {
        TxValidationState st;
        LOCK(cs_main);
        BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx_second), chainman.ActiveChain().Tip(), dmnman,
                                              chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                              TipRules(chainman), st,
                                              /*check_sigs=*/true),
                              "second claim should still pass its consensus check: " << st.GetRejectReason());
    }
    {
        LOCK(cs_main);
        const auto res = chainman.ProcessTransaction(MakeTransactionRef(tx_second));
        BOOST_CHECK_MESSAGE(res.m_result_type != MempoolAcceptResult::ResultType::VALID,
                            "second in-flight claim of the same key under the other encoding was accepted");
        BOOST_CHECK_EQUAL(res.m_state.GetRejectReason(), "protx-dup");
    }
};

// A registrar update must not be able to claim an operator key another masternode already holds,
// under either encoding. Consensus probes when the key changes or a same-key migration re-encodes it.
// It skips non-migrating same-key updates, which cannot create a duplicate; probing those would let
// a cross-scheme pair formed before activation permanently block the affected masternode's routine
// registrar updates, making an old squat more harmful rather than less.
void FuncProUpRegTxRejectsCrossSchemeKeyReuse(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;

    // MN-A registers pre-v19: legacy scheme, and stays legacy.
    CKey owner_key_a;
    CBLSSecretKey operator_key_a;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key_a);
    const auto proTxHashA = tx_reg_a.GetHash();
    setup.ProcessBlock({tx_reg_a});

    setup.MineToV24();
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHashA)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    // MN-B registers post-v24 with its own fresh basic key.
    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CBLSSecretKey operator_key_b;
    operator_key_b.MakeNewKey();
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20001"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key_b.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_reg_b;
    tx_reg_b.nVersion = 3;
    tx_reg_b.nType = TRANSACTION_PROVIDER_REGISTER;
    {
        const auto spent = FundTransaction(chainman, tx_reg_b, utxos, pro_reg_b.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_reg_b));
        SetTxPayload(tx_reg_b, pro_reg_b);
        SignTransaction(tx_reg_b, spent, setup.coinbaseKey);
    }
    const auto proTxHashB = tx_reg_b.GetHash();
    setup.ProcessBlock({tx_reg_b});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(proTxHashB));

    auto build_upreg = [&](const uint256& protx_hash, const CKey& owner_key, const CBLSPublicKey& op_pubkey,
                           uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, protx_hash, owner_key, op_pubkey, owner_key.GetPubKey().GetID(),
                                GenerateRandomAddress(), setup.coinbaseKey, version);
    };
    auto check_upreg = [&](const CMutableTransaction& tx, TxValidationState& st) {
        LOCK(cs_main);
        return CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                               chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(), TipRules(chainman), st,
                               /*check_sigs=*/true);
    };

    // MN-A tries to take MN-B's key under the OTHER encoding, via a v1 payload. This is the reverse
    // direction's only route that survives post-v24, since registration cannot carry a legacy key.
    {
        auto tx = build_upreg(proTxHashA, owner_key_a, operator_key_b.GetPublicKey(), ProTxVersion::LegacyBLS);
        TxValidationState st;
        BOOST_CHECK(!check_upreg(tx, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-dup-key");
    }

    // MN-B tries to take MN-A's key, basic-encoded (MN-A holds it legacy-encoded).
    {
        auto tx = build_upreg(proTxHashB, owner_key_b, operator_key_a.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState st;
        BOOST_CHECK(!check_upreg(tx, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-dup-key");
    }

    // No false positive: MN-B re-submitting its OWN key unchanged is not a duplicate. This is the
    // key-change scoping, and without it a masternode could never update its registrar again.
    {
        auto tx = build_upreg(proTxHashB, owner_key_b, operator_key_b.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState st;
        BOOST_CHECK_MESSAGE(check_upreg(tx, st), "self-key registrar update wrongly rejected: "
                                                     << st.GetRejectReason());
    }
};

// A ProRegTx must not be able to claim an operator key another masternode already holds, whichever
// encoding either side uses. ProRegTx never proves ownership of the operator key, so without this a
// masternode's key can be squatted for the price of a collateral.
void FuncProRegTxRejectsCrossSchemeKeyReuse(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key_a;
    CBLSSecretKey operator_key;
    auto tx_reg_a = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key_a,
                                   operator_key);
    setup.ProcessBlock({tx_reg_a});
    BOOST_REQUIRE(dmnman.GetListAtChainTip().GetMN(tx_reg_a.GetHash())->pdmnState->pubKeyOperator.IsLegacy());

    int port{20000};
    auto build_proreg = [&](const CBLSPublicKey& op_pubkey, uint16_t version) {
        CKey owner_key_b;
        owner_key_b.MakeNewKey(true);
        CProRegTx pro_reg;
        pro_reg.nVersion = version;
        pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
        pro_reg.collateralOutpoint.n = 0;
        BOOST_REQUIRE_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.%d:%d", port % 250 + 2, port)),
                            NetInfoStatus::Success);
        ++port;
        pro_reg.keyIDOwner = owner_key_b.GetPubKey().GetID();
        pro_reg.pubKeyOperator.Set(op_pubkey, /*specificLegacyScheme=*/version == ProTxVersion::LegacyBLS);
        pro_reg.keyIDVoting = owner_key_b.GetPubKey().GetID();
        pro_reg.scriptPayout = GenerateRandomAddress();
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_REGISTER;
        const auto spent = FundTransaction(chainman, tx, utxos, pro_reg.scriptPayout,
                                           dmn_types::Regular.collat_amount);
        pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx));
        SetTxPayload(tx, pro_reg);
        SignTransaction(tx, spent, setup.coinbaseKey);
        return tx;
    };
    auto check_proreg = [&](const CMutableTransaction& tx, TxValidationState& st) {
        LOCK(cs_main);
        return CheckProRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                             chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(), TipRules(chainman), st,
                             /*check_sigs=*/true);
    };

    setup.MineToV19();
    BOOST_REQUIRE(!DeploymentActiveAfter(setup.Tip(), chainman, Consensus::DEPLOYMENT_V24));

    // Non-retroactivity: before v24 the squat is still accepted, exactly as on develop today.
    {
        auto tx = build_proreg(operator_key.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState st;
        BOOST_CHECK_MESSAGE(check_proreg(tx, st), "pre-v24 cross-scheme registration wrongly rejected: "
                                                      << st.GetRejectReason());
    }

    setup.MineToV24();

    // The main post-v24 vector: reuse the legacy masternode's key, basic-encoded.
    {
        auto tx = build_proreg(operator_key.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState st;
        BOOST_CHECK(!check_proreg(tx, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-dup-key");
    }

    // The reverse direction is unreachable via registration post-v24 for an unrelated reason: a
    // legacy-encoded payload needs nVersion=1, which registration refuses outright. Pinned so it
    // cannot start silently passing.
    {
        CBLSSecretKey fresh;
        fresh.MakeNewKey();
        auto tx = build_proreg(fresh.GetPublicKey(), ProTxVersion::LegacyBLS);
        TxValidationState st;
        BOOST_CHECK(!check_proreg(tx, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-protx-version-disallowed");
    }

    // No false positives: a fresh key still registers.
    {
        CBLSSecretKey fresh;
        fresh.MakeNewKey();
        auto tx = build_proreg(fresh.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState st;
        BOOST_CHECK_MESSAGE(check_proreg(tx, st), "fresh-key registration wrongly rejected: "
                                                      << st.GetRejectReason());
    }
};

// The unique property map is keyed by the scheme-dependent serialization of a BLS key, so one public
// key occupies a different slot depending on its encoding. The helper must find it under either, and
// must exclude only the masternode asked about.
void FuncHasOperatorKeyUnderAnyScheme(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    dmnman.UpdatedBlockTip(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

    const auto list = dmnman.GetListAtChainTip();
    const auto dmn = list.GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    // Stored legacy-encoded, so a plain lookup under the basic encoding misses it...
    BOOST_REQUIRE(dmn->pdmnState->pubKeyOperator.IsLegacy());
    CBLSLazyPublicKey basic_wrapped;
    basic_wrapped.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    BOOST_REQUIRE(!list.HasUniqueProperty(basic_wrapped));
    // ...which is exactly the gap the helper closes.
    BOOST_CHECK(list.HasOperatorKeyUnderAnyScheme(operator_key.GetPublicKey(), uint256()));
    // The holder itself is excluded when asked.
    BOOST_CHECK(!list.HasOperatorKeyUnderAnyScheme(operator_key.GetPublicKey(), proTxHash));
    // A key nobody holds is absent.
    CBLSSecretKey other;
    other.MakeNewKey();
    BOOST_CHECK(!list.HasOperatorKeyUnderAnyScheme(other.GetPublicKey(), uint256()));
};

// Non-retroactivity. Every consensus rule added here is gated on v24, which is NEVER_ACTIVE on
// mainnet and testnet, so before activation both transactions the new rules reject must still be
// accepted exactly as they are on develop today. If this test ever fails, a rule has leaked past its
// gate and is changing a live chain — that is the one failure mode here that reaches real users, so
// fix the gating rather than this test.
void FuncPreV24BehaviourUnchanged(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto tip_index    = [&] { return WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()); };
    auto sync_dmn_tip = [&] { dmnman.UpdatedBlockTip(tip_index()); };
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
    BOOST_REQUIRE(bls::bls_legacy_scheme.load());

    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.CreateAndProcessBlock({tx_reg}, coinbase_pk);
    sync_dmn_tip();

    while (!DeploymentActiveAfter(tip_index(), chainman.GetConsensus(), Consensus::DEPLOYMENT_V19)) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
        sync_dmn_tip();
    }
    // v19 active, v24 inactive: this is mainnet's configuration today.
    BOOST_REQUIRE(!DeploymentActiveAfter(tip_index(), chainman, Consensus::DEPLOYMENT_V24));
    BOOST_REQUIRE(!bls::bls_legacy_scheme.load());

    // A BasicBLS service update on a legacy masternode: rejected post-v24 by Component 1.
    auto tx_ups = CreateProUpServTx(chainman, utxos, proTxHash, operator_key, 19998, CScript(), setup.coinbaseKey);
    {
        TxValidationState val_state;
        LOCK(cs_main);
        BOOST_CHECK_MESSAGE(CheckProUpServTx(CTransaction(tx_ups), chainman.ActiveChain().Tip(), dmnman,
                                             chainman.GetConsensus(), TipRules(chainman), val_state,
                                             /*check_sigs=*/true),
                            "pre-v24 v2 ProUpServTx wrongly rejected: " << val_state.GetRejectReason());
    }
    setup.CreateAndProcessBlock({tx_ups}, coinbase_pk);
    sync_dmn_tip();
    CheckListRoundTrips(dmnman, "pre-v24 after v2 ProUpServTx");

    // A same-key BasicBLS registrar update on a legacy masternode: rejected post-v24 by Component 2.
    CProUpRegTx pro_upreg;
    pro_upreg.nVersion = ProTxVersion::BasicBLS;
    pro_upreg.proTxHash = proTxHash;
    pro_upreg.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_upreg.keyIDVoting = owner_key.GetPubKey().GetID();
    pro_upreg.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_upreg;
    tx_upreg.nVersion = 3;
    tx_upreg.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent = FundTransaction(chainman, tx_upreg, utxos,
                                       GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey())), 1 * COIN);
    pro_upreg.inputsHash = CalcTxInputsHash(CTransaction(tx_upreg));
    CHashSigner::SignHash(::SerializeHash(pro_upreg), owner_key, pro_upreg.vchSig);
    SetTxPayload(tx_upreg, pro_upreg);
    SignTransaction(tx_upreg, spent, setup.coinbaseKey);
    {
        TxValidationState val_state;
        LOCK(cs_main);
        BOOST_CHECK_MESSAGE(CheckProUpRegTx(CTransaction(tx_upreg), chainman.ActiveChain().Tip(), dmnman,
                                            chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                            TipRules(chainman), val_state,
                                            /*check_sigs=*/true),
                            "pre-v24 same-key ProUpRegTx wrongly rejected: " << val_state.GetRejectReason());
    }
    setup.CreateAndProcessBlock({tx_upreg}, coinbase_pk);
    sync_dmn_tip();

    // Pre-v24 the version does not move and the key keeps its scheme, so nothing desyncs.
    const auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
    BOOST_REQUIRE(dmn);
    BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::LegacyBLS);
    BOOST_CHECK(dmn->pdmnState->pubKeyOperator.IsLegacy());
    CheckListRoundTrips(dmnman, "pre-v24 after same-key ProUpRegTx");
};

// A special transaction accepted into the mempool while it was valid is not evicted when a fork
// activates a rule that invalidates it, and BlockAssembler otherwise trusts mempool validity. It
// would then select the stale transaction into every template, and every one of those blocks would
// be rejected by peers, stalling an honest miner.
void FuncStaleSpecialTxDoesNotPoisonTemplate(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;
    const auto& coinbase_pk = setup.coinbase_pk;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.ProcessBlock({tx_reg});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    // A cross-scheme squatter ProRegTx: it reuses the legacy masternode's operator key basic-encoded.
    // Such a registration is valid before v24 and rejected after it (bad-protx-dup-key), which is the
    // shape of transaction that can sit in the mempool when the fork activates. addUnchecked()
    // reproduces that resident-but-now-invalid state directly.
    CKey owner_key_b;
    owner_key_b.MakeNewKey(true);
    CProRegTx pro_reg_b;
    pro_reg_b.nVersion = ProTxVersion::BasicBLS;
    pro_reg_b.netInfo = NetInfoInterface::MakeNetInfo(pro_reg_b.nVersion);
    pro_reg_b.collateralOutpoint.n = 0;
    BOOST_REQUIRE_EQUAL(pro_reg_b.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:20002"), NetInfoStatus::Success);
    pro_reg_b.keyIDOwner = owner_key_b.GetPubKey().GetID();
    pro_reg_b.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/false);
    pro_reg_b.keyIDVoting = owner_key_b.GetPubKey().GetID();
    pro_reg_b.scriptPayout = GenerateRandomAddress();
    CMutableTransaction tx_stale;
    tx_stale.nVersion = 3;
    tx_stale.nType = TRANSACTION_PROVIDER_REGISTER;
    const auto spent = FundTransaction(chainman, tx_stale, utxos, pro_reg_b.scriptPayout,
                                       dmn_types::Regular.collat_amount);
    pro_reg_b.inputsHash = CalcTxInputsHash(CTransaction(tx_stale));
    SetTxPayload(tx_stale, pro_reg_b);
    SignTransaction(tx_stale, spent, setup.coinbaseKey);
    const auto stale_hash = tx_stale.GetHash();

    // Sanity: consensus really does reject it now, so this test is about a genuinely invalid entry.
    {
        TxValidationState val_state;
        LOCK(cs_main);
        BOOST_REQUIRE(!CheckProRegTx(CTransaction(tx_stale), chainman.ActiveChain().Tip(), dmnman,
                                     chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                     TipRules(chainman), val_state,
                                     /*check_sigs=*/true));
        BOOST_REQUIRE_EQUAL(val_state.GetRejectReason(), "bad-protx-dup-key");
    }

    // Make it fee-eligible so selection would genuinely pick it, then park it in the mempool.
    auto& mempool = *Assert(setup.m_node.mempool.get());
    TestMemPoolEntryHelper entry;
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(entry.Fee(50000).Time(Now<NodeSeconds>()).FromTx(tx_stale));
        BOOST_REQUIRE(mempool.exists(stale_hash));
    }

    auto block_template = node::BlockAssembler{chainman.ActiveChainstate(), setup.m_node, &mempool}
                              .CreateNewBlock(coinbase_pk);
    BOOST_REQUIRE_MESSAGE(block_template != nullptr, "no template built while a stale special tx was resident");
    for (const auto& tx : block_template->block.vtx) {
        BOOST_CHECK_MESSAGE(tx->GetHash() != stale_hash,
                            "a stale special tx was selected into the block template");
    }
};

// Provider transactions are validated against the tip, so two that are each valid alone can
// still conflict when mined together. Block assembly must keep the first one selected, leave out
// the other and its descendants, and still include unrelated transactions, instead of producing a
// template whose masternode list cannot be built. This holds whether the list rejects a
// transaction through a check or by throwing (as AddMN does for a duplicate platformNodeID). A
// package that is left out must leave no trace in the list: a later registration reusing the owner
// key of its (valid) parent is still selected. addUnchecked() puts registrations with the same
// owner key, or the same platformNodeID, into the mempool, which relay would not do.
void FuncTemplateSkipsConflictingProviderPackage(TestChainSetup& setup)
{
    auto& chainman = *Assert(setup.m_node.chainman.get());
    auto& mempool = *Assert(setup.m_node.mempool.get());
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());
    const CScript coinbase_script = GetScriptForDestination(PKHash(setup.coinbaseKey.GetPubKey()));
    // Mature enough coinbase outputs to fund EvoNode collateral too.
    setup.mineBlocks(60);
    auto utxos = BuildSimpleUtxoMap(setup.m_coinbase_txns);

    CKey owner_key;
    owner_key.MakeNewKey(true);
    // Funded from the coinbase outputs, or from a collateral-sized output paid to the coinbase key.
    // A platformNodeID makes it an EvoNode registration.
    auto make_reg = [&](int port, const CKey& owner, const std::optional<COutPoint>& funding = std::nullopt,
                        const std::optional<uint160>& platform_node_id = std::nullopt) {
        CBLSSecretKey operator_key;
        operator_key.MakeNewKey();
        CProRegTx pro_reg;
        pro_reg.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
        pro_reg.netInfo = NetInfoInterface::MakeNetInfo(pro_reg.nVersion);
        pro_reg.collateralOutpoint.n = 0;
        BOOST_REQUIRE_EQUAL(pro_reg.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.1:%d", port)),
                            NetInfoStatus::Success);
        if (platform_node_id) {
            pro_reg.nType = MnType::Evo;
            pro_reg.platformNodeID = *platform_node_id;
            pro_reg.platformP2PPort = port + 1000;
            pro_reg.platformHTTPPort = port + 2000;
        }
        pro_reg.keyIDOwner = owner.GetPubKey().GetID();
        pro_reg.pubKeyOperator.Set(operator_key.GetPublicKey(), bls::bls_legacy_scheme.load());
        pro_reg.keyIDVoting = owner.GetPubKey().GetID();
        pro_reg.scriptPayout = coinbase_script;
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_REGISTER;
        SimpleUTXOMap spent;
        if (funding) {
            const CTxOut funding_out{dmn_types::Regular.collat_amount, coinbase_script};
            tx.vin.emplace_back(*funding);
            tx.vout.push_back(funding_out);
            spent.emplace(*funding, Coin(funding_out, /*nHeightIn=*/1, /*fCoinBaseIn=*/false));
        } else {
            spent = FundTransaction(chainman, tx, utxos, coinbase_script, GetMnType(pro_reg.nType).collat_amount);
        }
        pro_reg.inputsHash = CalcTxInputsHash(CTransaction(tx));
        SetTxPayload(tx, pro_reg);
        SignTransaction(tx, spent, setup.coinbaseKey);
        return tx;
    };
    const auto tx_first = make_reg(21001, owner_key);
    const auto tx_second = make_reg(21002, owner_key);

    CKey other_owner_key;
    CBLSSecretKey other_operator_key;
    const auto tx_other = CreateProRegTx(chainman, utxos, 21003, GenerateRandomAddress(), setup.coinbaseKey,
                                         other_owner_key, other_operator_key);

    // A package whose parent applies and whose child (spending the parent's collateral output)
    // conflicts with tx_first. It ranks right after tx_first and is tried as a whole and left out.
    // tx_reuse, ranked next, claims the parent's owner key: it must apply because the rejected
    // package left no trace, after which the parent, retried on its own, conflicts with it.
    CKey parent_owner_key;
    parent_owner_key.MakeNewKey(true);
    const auto tx_parent = make_reg(21004, parent_owner_key);
    const auto tx_conflicting_child = make_reg(21005, owner_key, COutPoint{tx_parent.GetHash(), 0});
    const auto tx_reuse = make_reg(21006, parent_owner_key);

    // Two EvoNode registrations with distinct owner keys and the same platformNodeID: the list
    // rejects the second one by throwing from AddMN, so it must be left out like a failing one.
    uint160 platform_node_id;
    platform_node_id.SetHex("00112233445566778899aabbccddeeff00112233");
    CKey evo_owner_keys[2];
    for (auto& key : evo_owner_keys) key.MakeNewKey(true);
    const auto tx_evo_first = make_reg(21007, evo_owner_keys[0], std::nullopt, platform_node_id);
    const auto tx_evo_second = make_reg(21008, evo_owner_keys[1], std::nullopt, platform_node_id);

    // A child of the transaction selected second must be left out with it.
    const COutPoint second_out{tx_second.GetHash(), 0};
    CMutableTransaction tx_child;
    tx_child.vin.emplace_back(second_out);
    tx_child.vout.emplace_back(tx_second.vout[0].nValue - 10000, coinbase_script);
    SignTransaction(tx_child, {{second_out, Coin(tx_second.vout[0], /*nHeightIn=*/1, /*fCoinBaseIn=*/false)}},
                    setup.coinbaseKey);

    // Entries carry their real fees (the coinbase claims them); selection order comes from
    // prioritisation.
    TestMemPoolEntryHelper entry;
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_first));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_second));
        mempool.addUnchecked(entry.Fee(10000).FromTx(tx_child));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_other));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_parent));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_conflicting_child));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_reuse));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_evo_first));
        mempool.addUnchecked(entry.Fee(0).FromTx(tx_evo_second));
        mempool.PrioritiseTransaction(tx_first.GetHash(), 1000000);
        mempool.PrioritiseTransaction(tx_conflicting_child.GetHash(), 900000);
        mempool.PrioritiseTransaction(tx_reuse.GetHash(), 300000);
        mempool.PrioritiseTransaction(tx_parent.GetHash(), 20000);
        mempool.PrioritiseTransaction(tx_evo_first.GetHash(), 400000);
        mempool.PrioritiseTransaction(tx_evo_second.GetHash(), 200000);
        mempool.PrioritiseTransaction(tx_second.GetHash(), 100000);
        mempool.PrioritiseTransaction(tx_other.GetHash(), 50000);
        // The rollback check relies on this order: tx_first, then the parent/child package (the
        // child's entry ranks by its ancestor feerate), then tx_reuse, then the parent alone.
        const CompareTxMemPoolEntryByAncestorFee ranks_before;
        const auto first_entry = *Assert(mempool.GetIter(tx_first.GetHash()));
        const auto package_entry = *Assert(mempool.GetIter(tx_conflicting_child.GetHash()));
        const auto reuse_entry = *Assert(mempool.GetIter(tx_reuse.GetHash()));
        const auto parent_entry = *Assert(mempool.GetIter(tx_parent.GetHash()));
        BOOST_REQUIRE(ranks_before(*first_entry, *package_entry));
        BOOST_REQUIRE(ranks_before(*package_entry, *reuse_entry));
        BOOST_REQUIRE(ranks_before(*reuse_entry, *parent_entry));
    }

    node::BlockAssembler assembler{chainman.ActiveChainstate(), setup.m_node, &mempool};
    std::unique_ptr<node::CBlockTemplate> block_template;
    try {
        block_template = assembler.CreateNewBlock(coinbase_pk);
    } catch (const std::exception& e) {
        BOOST_ERROR("CreateNewBlock threw: " << e.what());
    }
    BOOST_REQUIRE_MESSAGE(block_template != nullptr, "no template built while conflicting ProTxs were resident");

    std::set<uint256> selected;
    for (const auto& tx : block_template->block.vtx) {
        selected.insert(tx->GetHash());
    }
    BOOST_CHECK(selected.count(tx_first.GetHash()));
    BOOST_CHECK(!selected.count(tx_second.GetHash()));
    BOOST_CHECK(!selected.count(tx_child.GetHash()));
    BOOST_CHECK(selected.count(tx_other.GetHash()));
    BOOST_CHECK(!selected.count(tx_conflicting_child.GetHash()));
    BOOST_CHECK(selected.count(tx_reuse.GetHash()));
    BOOST_CHECK(!selected.count(tx_parent.GetHash()));
    BOOST_CHECK(selected.count(tx_evo_first.GetHash()));
    BOOST_CHECK(!selected.count(tx_evo_second.GetHash()));
}

// A legacy masternode migrates to the basic scheme keeping its operator key: SetStateVersion
// re-encodes the key and UpdateMN re-keys the unique-property map, so no key rotation is forced.
void FuncProUpRegTxMigratesLegacySameKey(TestChainV24SignalBeforeV19Setup& setup)
{
    auto& chainman = setup.chainman;
    auto& dmnman = setup.dmnman;

    BOOST_REQUIRE(bls::bls_legacy_scheme.load());
    auto& utxos = setup.utxos;
    CKey owner_key;
    CBLSSecretKey operator_key;
    auto tx_reg = CreateProRegTx(chainman, utxos, 19999, GenerateRandomAddress(), setup.coinbaseKey, owner_key,
                                 operator_key);
    const auto proTxHash = tx_reg.GetHash();
    setup.ProcessBlock({tx_reg});

    setup.MineToV24();
    BOOST_REQUIRE_EQUAL(dmnman.GetListAtChainTip().GetMN(proTxHash)->pdmnState->nVersion, ProTxVersion::LegacyBLS);

    auto build_upreg = [&](const CBLSPublicKey& op_pubkey, uint16_t version) {
        return CreateProUpRegTx(chainman, utxos, proTxHash, owner_key, op_pubkey, owner_key.GetPubKey().GetID(),
                                GenerateRandomAddress(), setup.coinbaseKey, version);
    };

    // Same key, re-encoded basic: migrates in place. Accepted, version rises, key preserved and
    // re-encoded, NOT PoSe-banned (no rotation forced), and the list round-trips.
    {
        auto tx = build_upreg(operator_key.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState val_state;
        {
            LOCK(cs_main);
            BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                                  chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                                  TipRules(chainman), val_state,
                                                  /*check_sigs=*/true),
                                  "same-key migration rejected: " << val_state.GetRejectReason());
        }
        setup.ProcessBlock({tx});
        const auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
        BOOST_REQUIRE(dmn);
        BOOST_CHECK_EQUAL(dmn->pdmnState->nVersion, ProTxVersion::BasicBLS);
        BOOST_CHECK(!dmn->pdmnState->pubKeyOperator.IsLegacy());
        BOOST_CHECK(!dmn->pdmnState->IsBanned());
        BOOST_CHECK(dmn->pdmnState->pubKeyOperator.Get() == operator_key.GetPublicKey());
        CheckListRoundTrips(dmnman, "after same-key legacy->basic migration");
        const auto live = dmnman.GetListAtChainTip();
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << live;
        CDeterministicMNList reloaded;
        ds >> reloaded;
        BOOST_CHECK(reloaded.GetMN(proTxHash)->pdmnState->pubKeyOperator.Get() == operator_key.GetPublicKey());
    }

    // A genuinely new key still rotates and PoSe-bans, as before.
    {
        CBLSSecretKey new_operator_key;
        new_operator_key.MakeNewKey();
        auto tx = build_upreg(new_operator_key.GetPublicKey(), ProTxVersion::BasicBLS);
        TxValidationState val_state;
        {
            LOCK(cs_main);
            BOOST_REQUIRE_MESSAGE(CheckProUpRegTx(CTransaction(tx), chainman.ActiveChain().Tip(), dmnman,
                                                  chainman.ActiveChainstate().CoinsTip(), chainman.GetConsensus(),
                                                  TipRules(chainman), val_state,
                                                  /*check_sigs=*/true),
                                  "new-key rotation rejected: " << val_state.GetRejectReason());
        }
        setup.ProcessBlock({tx});
        const auto dmn = dmnman.GetListAtChainTip().GetMN(proTxHash);
        BOOST_REQUIRE(dmn);
        BOOST_CHECK(!dmn->pdmnState->pubKeyOperator.IsLegacy());
        BOOST_CHECK(dmn->pdmnState->IsBanned());
        CheckListRoundTrips(dmnman, "after new-key rotation");
    }
};

BOOST_AUTO_TEST_CASE(same_mn_same_block_version_crossing_key_rotation)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncSameMnSameBlockVersionCrossingKeyRotation(setup);
}

BOOST_AUTO_TEST_CASE(same_mn_same_block_migration_consistent)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncSameMnSameBlockMigrationConsistent(setup);
}

BOOST_AUTO_TEST_CASE(pre_v24_cross_scheme_pair_cannot_become_resident)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncPreV24CrossSchemePairCannotBecomeResident(setup);
}

BOOST_AUTO_TEST_CASE(same_block_cross_scheme_key_pair_rejected)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncSameBlockCrossSchemeKeyPairRejected(setup);
}

BOOST_AUTO_TEST_CASE(mempool_rejects_cross_scheme_key_race)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncMempoolRejectsCrossSchemeKeyRace(setup);
}

BOOST_AUTO_TEST_CASE(proupreg_rejects_cross_scheme_key_reuse)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProUpRegTxRejectsCrossSchemeKeyReuse(setup);
}

BOOST_AUTO_TEST_CASE(proregtx_rejects_cross_scheme_key_reuse)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProRegTxRejectsCrossSchemeKeyReuse(setup);
}

BOOST_AUTO_TEST_CASE(statediff_captures_operator_key_reencoding)
{
    // A legacy->basic migration re-encodes the operator key. CDeterministicMNStateDiff must capture that,
    // or the evoDB diff path reconstructs the masternode with the old encoding while an online-built list
    // has the new one -- a reconstruction split that full-snapshot serialization (which re-derives the
    // encoding from nVersion) would not reveal.
    CBLSSecretKey sk;
    sk.MakeNewKey();

    // Mainnet block 2005287 migrated a masternode with a key whose bytes match its legacy key. The bytes
    // decode to different keys under each scheme, so the encoded bytes (and GetHash()) do not change.
    const auto same_bytes{ParseHex("991ffe06346a3652cb5f26e614ad9c57b2651405c64712beb45c0b8c870e86fab7fa47a77bb66d1d6d79ce95c40150fc")};
    CBLSPublicKey same_bytes_legacy;
    same_bytes_legacy.SetBytes(same_bytes, /*specificLegacyScheme=*/true);
    CBLSPublicKey same_bytes_basic;
    same_bytes_basic.SetBytes(same_bytes, /*specificLegacyScheme=*/false);
    BOOST_REQUIRE(same_bytes_legacy.IsValid() && same_bytes_basic.IsValid());
    BOOST_REQUIRE(same_bytes_legacy != same_bytes_basic);

    const struct {
        std::string name;
        CBLSPublicKey legacy_key;
        CBLSPublicKey basic_key;
    } cases[]{
        {"same key, new encoding", sk.GetPublicKey(), sk.GetPublicKey()},
        {"same bytes, different key", same_bytes_legacy, same_bytes_basic},
    };

    for (const auto& c : cases) {
        BOOST_TEST_CONTEXT(c.name)
        {
            CDeterministicMNState a;
            a.nVersion = ProTxVersion::LegacyBLS;
            a.pubKeyOperator.Set(c.legacy_key, /*specificLegacyScheme=*/true);
            a.netInfo = NetInfoInterface::MakeNetInfo(a.nVersion);

            CDeterministicMNState b{a};
            b.nVersion = ProTxVersion::BasicBLS;
            b.pubKeyOperator.Set(c.basic_key, /*specificLegacyScheme=*/false);

            CDeterministicMNStateDiff diff{a, b};
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << diff;
            CDeterministicMNStateDiff diff2;
            ds >> diff2;

            CDeterministicMNState reconstructed{a};
            diff2.ApplyToState(reconstructed);

            BOOST_CHECK_EQUAL(reconstructed.nVersion, ProTxVersion::BasicBLS);
            BOOST_CHECK(!reconstructed.pubKeyOperator.IsLegacy());
            BOOST_CHECK(reconstructed.pubKeyOperator.Get() == c.basic_key);
            BOOST_CHECK(::SerializeHash(reconstructed.pubKeyOperator) == ::SerializeHash(b.pubKeyOperator));
        }
    }
}

BOOST_AUTO_TEST_CASE(has_operator_key_under_any_scheme)
{
    TestChainV19BeforeActivationSetup setup;
    FuncHasOperatorKeyUnderAnyScheme(setup);
}

BOOST_AUTO_TEST_CASE(pre_v24_behaviour_unchanged)
{
    TestChainV19BeforeActivationSetup setup;
    FuncPreV24BehaviourUnchanged(setup);
}

BOOST_AUTO_TEST_CASE(stale_special_tx_does_not_poison_template)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncStaleSpecialTxDoesNotPoisonTemplate(setup);
}

BOOST_AUTO_TEST_CASE(template_skips_conflicting_provider_package)
{
    TestChainV19Setup setup;
    FuncTemplateSkipsConflictingProviderPackage(setup);
}

BOOST_AUTO_TEST_CASE(proupreg_migrates_legacy_same_key)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProUpRegTxMigratesLegacySameKey(setup);
}

BOOST_AUTO_TEST_CASE(proupreg_version_handling_before_v24)
{
    TestChainV19BeforeActivationSetup setup;
    FuncProUpRegTxVersionHandlingBeforeV24(setup);
}

BOOST_AUTO_TEST_CASE(proupreg_v3_on_legacy_valid)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProUpRegTxV3OnLegacyValid(setup);
}

BOOST_AUTO_TEST_CASE(proupreg_v2_cannot_bypass_v3_payout_collateral_reuse)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProUpRegTxV2CannotBypassV3PayoutCollateralReuse(setup);
}

BOOST_AUTO_TEST_CASE(proregtx_rejects_invalid_deserialized_extnetinfo)
{
    TestChainV24SignalBeforeV19Setup setup;
    FuncProRegTxRejectsInvalidDeserializedExtNetInfo(setup);
}

BOOST_AUTO_TEST_CASE(mn_payment_multiplicity_v24_boundary)
{
    TestChainV24PendingSetup setup;
    FuncMNPaymentMultiplicityV24Boundary(setup);
}

BOOST_AUTO_TEST_CASE(dip3_protx_legacy)
{
    TestChainDIP3Setup setup;
    FuncDIP3Protx(setup);
}

BOOST_AUTO_TEST_CASE(dip3_protx_basic)
{
    TestChainV19Setup setup;
    FuncDIP3Protx(setup);
}

// Requires BasicBLS so ProUpServTx serialises nType. Pre-fix this test fails because
// IsTriviallyValid / CheckProUpServTx accept out-of-range nType.
BOOST_AUTO_TEST_CASE(proupserv_invalid_ntype_basic)
{
    TestChainV19Setup setup;
    FuncProUpServInvalidNType(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_reorg_legacy)
{
    TestChainDIP3Setup setup;
    FuncTestMempoolReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_reorg_basic)
{
    TestChainV19Setup setup;
    FuncTestMempoolReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_protx_key_changed_conflict_chain_legacy)
{
    TestChainDIP3Setup setup;
    FuncTestMempoolProTxKeyChangedConflictChain(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_protx_key_changed_conflict_chain_basic)
{
    TestChainV19Setup setup;
    FuncTestMempoolProTxKeyChangedConflictChain(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_dual_proregtx_legacy)
{
    TestChainDIP3Setup setup;
    FuncTestMempoolDualProregtx(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_dual_proregtx_basic)
{
    TestChainV19Setup setup;
    FuncTestMempoolDualProregtx(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_proreg_replacement_update_conflict)
{
    TestChainV19Setup setup;
    FuncTestMempoolProRegReplacementUpdateConflict(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_service_update_before_key_change)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateBeforeKeyChange(setup);
}

BOOST_AUTO_TEST_CASE(test_mempool_rejects_unorderable_service_update)
{
    TestChainV19Setup setup;
    FuncTestMempoolRejectsUnorderableServiceUpdate(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_service_update_before_reorged_key_change)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateBeforeReorgedKeyChange(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_service_update_before_key_change_surviving_reorg)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateBeforeKeyChangeSurvivingReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_service_update_never_after_key_change)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateNeverAfterKeyChange(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_key_change_not_held_back_after_reorg)
{
    TestChainV19Setup setup;
    FuncTestMinerKeyChangeNotHeldBackAfterReorg(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_service_update_with_parent_before_key_change)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateWithParentBeforeKeyChange(setup, /*limit_block_size=*/false);
}

BOOST_AUTO_TEST_CASE(test_miner_key_change_without_room_for_service_update)
{
    TestChainV19Setup setup;
    FuncTestMinerServiceUpdateWithParentBeforeKeyChange(setup, /*limit_block_size=*/true);
}

BOOST_AUTO_TEST_CASE(test_miner_key_change_not_held_back_by_service_updates)
{
    TestChainV19Setup setup;
    FuncTestMinerKeyChangeNotHeldBackByServiceUpdates(setup);
}

BOOST_AUTO_TEST_CASE(test_miner_key_change_not_held_back_by_other_key_change)
{
    TestChainV19Setup setup;
    FuncTestMinerKeyChangeNotHeldBackByOtherKeyChange(setup);
}

//This one can be started only with legacy scheme, since inside undo block will switch it back to legacy resulting into an inconsistency
BOOST_AUTO_TEST_CASE(verify_db_legacy)
{
    TestChainDIP3Setup setup;
    FuncVerifyDB(setup);
}

BOOST_AUTO_TEST_CASE(test_sml_cache_legacy)
{
    TestChainDIP3Setup setup;
    SmlCache(setup);
}

BOOST_AUTO_TEST_CASE(test_sml_cache_basic)
{
    TestChainV19Setup setup;
    SmlCache(setup);
}

// Startup repair marks evodb as repaired unless repair_errors is set, so a snapshot missing from disk
// must land there in repair mode, while "evodb verify" keeps reporting it as a verification error.
BOOST_AUTO_TEST_CASE(evodb_repair_missing_snapshot_is_repair_error)
{
    TestChainDIP3Setup setup;
    auto& chainman = *Assert(setup.m_node.chainman);
    auto& dmnman = *Assert(setup.m_node.dmnman);
    auto& evodb = *Assert(setup.m_node.evodb);
    const CScript coinbase_pk = GetScriptForRawPubKey(setup.coinbaseKey.GetPubKey());

    // CDeterministicMNManager::DISK_SNAPSHOT_PERIOD
    constexpr int snapshot_period{576};
    while (WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()) < 2 * snapshot_period) {
        setup.CreateAndProcessBlock({}, coinbase_pk);
    }
    const CBlockIndex* dip3_index;
    const CBlockIndex* first_index;
    const CBlockIndex* second_index;
    {
        LOCK(::cs_main);
        dip3_index = chainman.ActiveChain()[Params().GetConsensus().DIP0003Height];
        first_index = chainman.ActiveChain()[snapshot_period];
        second_index = chainman.ActiveChain()[2 * snapshot_period];
    }

    // Diffs between intact snapshots verify, so the rebuild callback is never needed here
    const auto build_list_func = [](const CBlock&, gsl::not_null<const CBlockIndex*>, const CDeterministicMNList&,
                                    const CCoinsViewCache&, bool, BlockValidationState&,
                                    CDeterministicMNList&) { return false; };

    auto result = dmnman.RecalculateAndRepairDiffs(dip3_index, second_index, build_list_func, /*repair=*/true);
    BOOST_CHECK(result.verification_errors.empty());
    BOOST_CHECK(result.repair_errors.empty());
    BOOST_CHECK_EQUAL(result.snapshots_verified, 2);

    {
        LOCK(::cs_main);
        auto db_tx = evodb.BeginTransaction();
        // DB_LIST_SNAPSHOT
        evodb.Erase(std::make_pair(std::string{"dmn_S3"}, first_index->GetBlockHash()));
        db_tx->Commit();
    }

    const std::string missing_msg{strprintf("Snapshot missing at height %d", snapshot_period)};

    // Snapshot missing as the end of a pair
    result = dmnman.RecalculateAndRepairDiffs(dip3_index, second_index, build_list_func, /*repair=*/true);
    BOOST_CHECK(result.verification_errors.empty());
    BOOST_REQUIRE_EQUAL(result.repair_errors.size(), 1U);
    BOOST_CHECK(result.repair_errors[0].find(missing_msg) != std::string::npos);

    // Snapshot missing as the start of a pair
    result = dmnman.RecalculateAndRepairDiffs(first_index, second_index, build_list_func, /*repair=*/true);
    BOOST_CHECK(result.verification_errors.empty());
    BOOST_REQUIRE_EQUAL(result.repair_errors.size(), 1U);
    BOOST_CHECK(result.repair_errors[0].find(missing_msg) != std::string::npos);

    // Verify-only mode has no repair errors to report
    result = dmnman.RecalculateAndRepairDiffs(dip3_index, second_index, build_list_func, /*repair=*/false);
    BOOST_CHECK(result.repair_errors.empty());
    BOOST_REQUIRE_EQUAL(result.verification_errors.size(), 1U);
    BOOST_CHECK(result.verification_errors[0].find(missing_msg) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(field_bit_migration_validation)
{
    // Test individual field mappings for ALL 19 fields
    struct FieldMapping {
        uint32_t legacyBit;
        uint32_t newBit;
        std::string name;
    };

    std::vector<FieldMapping> mappings = {
        {0x0001, CDeterministicMNStateDiff::Field_nRegisteredHeight, "nRegisteredHeight"},
        {0x0002, CDeterministicMNStateDiff::Field_nLastPaidHeight, "nLastPaidHeight"},
        {0x0004, CDeterministicMNStateDiff::Field_nPoSePenalty, "nPoSePenalty"},
        {0x0008, CDeterministicMNStateDiff::Field_nPoSeRevivedHeight, "nPoSeRevivedHeight"},
        {0x0010, CDeterministicMNStateDiff::Field_nPoSeBanHeight, "nPoSeBanHeight"},
        {0x0020, CDeterministicMNStateDiff::Field_nRevocationReason, "nRevocationReason"},
        {0x0040, CDeterministicMNStateDiff::Field_confirmedHash, "confirmedHash"},
        {0x0080, CDeterministicMNStateDiff::Field_confirmedHashWithProRegTxHash, "confirmedHashWithProRegTxHash"},
        {0x0100, CDeterministicMNStateDiff::Field_keyIDOwner, "keyIDOwner"},
        {0x0200, CDeterministicMNStateDiff::Field_pubKeyOperator, "pubKeyOperator"},
        {0x0400, CDeterministicMNStateDiff::Field_keyIDVoting, "keyIDVoting"},
        {0x0800, CDeterministicMNStateDiff::Field_netInfo, "netInfo"},
        {0x1000, CDeterministicMNStateDiff::Field_scriptPayout, "scriptPayout"},
        {0x2000, CDeterministicMNStateDiff::Field_scriptOperatorPayout, "scriptOperatorPayout"},
        {0x4000, CDeterministicMNStateDiff::Field_nConsecutivePayments, "nConsecutivePayments"},
        {0x8000, CDeterministicMNStateDiff::Field_platformNodeID, "platformNodeID"},
        {0x10000, CDeterministicMNStateDiff::Field_platformP2PPort, "platformP2PPort"},
        {0x20000, CDeterministicMNStateDiff::Field_platformHTTPPort, "platformHTTPPort"},
        {0x40000, CDeterministicMNStateDiff::Field_nVersion, "nVersion"},
    };

    // Verify each field mapping is correct
    for (const auto& mapping : mappings) {
        // Test individual field conversion
        CDeterministicMNStateDiffLegacy legacyDiff;
        legacyDiff.fields |= mapping.legacyBit;
        // Convert to new format
        auto newDiff = legacyDiff.ToNewFormat();
        BOOST_CHECK_MESSAGE(newDiff.fields == mapping.newBit, strprintf("Field %s: legacy 0x%x should convert to 0x%x",
                                                                        mapping.name, mapping.legacyBit, mapping.newBit));
    }

    // Test complex multi-field scenarios
    uint32_t complexLegacyFields = 0x0200 | // Legacy Field_pubKeyOperator
                                   0x0800 | // Legacy Field_netInfo
                                   0x1000 | // Legacy Field_scriptPayout
                                   0x40000; // Legacy Field_nVersion

    uint32_t expectedNewFields = CDeterministicMNStateDiff::Field_nVersion |       // 0x0001
                                 CDeterministicMNStateDiff::Field_pubKeyOperator | // 0x0400 (was 0x0200)
                                 CDeterministicMNStateDiff::Field_netInfo |        // 0x1000 (was 0x0800)
                                 CDeterministicMNStateDiff::Field_scriptPayout;    // 0x2000 (was 0x1000)

    CDeterministicMNStateDiffLegacy legacyDiff;
    legacyDiff.fields |= complexLegacyFields;
    // Convert to new format
    auto newDiff = legacyDiff.ToNewFormat();
    BOOST_CHECK_EQUAL(newDiff.fields, expectedNewFields);

    // Verify no bit conflicts exist in new field layout
    std::set<uint32_t> usedBits;
    for (const auto& mapping : mappings) {
        BOOST_CHECK_MESSAGE(usedBits.find(mapping.newBit) == usedBits.end(),
                            strprintf("Duplicate bit 0x%x found for field %s", mapping.newBit, mapping.name));
        usedBits.insert(mapping.newBit);
    }

    // Verify all 19 fields have unique bit assignments
    BOOST_CHECK_EQUAL(usedBits.size(), 19);
}

BOOST_AUTO_TEST_CASE(migration_logic_validation)
{
    // Test the database migration logic for nVersion-first format conversion.
    // Migration logic is handled at CDeterministicMNListDiff level
    // using CDeterministicMNStateDiffLegacy for legacy format deserialization.

    // Create sample legacy format state diff
    CDeterministicMNStateDiffLegacy legacyDiff;
    legacyDiff.fields = 0x40000 | 0x0200 | 0x0800 | 0x0010; // Legacy: nVersion, pubKeyOperator, netInfo, nPoSeBanHeight
    legacyDiff.state.nVersion = ProTxVersion::BasicBLS;
    CBLSSecretKey sk;
    sk.MakeNewKey();
    legacyDiff.state.pubKeyOperator.Set(sk.GetPublicKey(), false);
    BOOST_CHECK(!legacyDiff.state.pubKeyOperator.IsLegacy());
    legacyDiff.state.netInfo = NetInfoInterface::MakeNetInfo(ProTxVersion::BasicBLS);
    BOOST_CHECK(!legacyDiff.state.IsBanned());
    legacyDiff.state.BanIfNotBanned(2367316);
    BOOST_CHECK(legacyDiff.state.IsBanned());
    BOOST_CHECK_EQUAL(legacyDiff.state.GetBannedHeight(), 2367316);


    // Test legacy class conversion (this would normally be done by CDeterministicMNListDiff)
    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << legacyDiff;

    CDeterministicMNStateDiffLegacy legacyDeserializer(deserialize, ss);
    CDeterministicMNStateDiff convertedDiff = legacyDeserializer.ToNewFormat();
    BOOST_CHECK(!legacyDiff.state.pubKeyOperator.IsLegacy());
    BOOST_CHECK(convertedDiff.state.pubKeyOperator.IsLegacy());
    convertedDiff.state.pubKeyOperator.SetLegacy(false);
    BOOST_CHECK(!convertedDiff.state.pubKeyOperator.IsLegacy());

    // Verify conversion worked correctly
    uint32_t expectedNewFields = CDeterministicMNStateDiff::Field_nVersion |       // 0x0001
                                 CDeterministicMNStateDiff::Field_nPoSeBanHeight | // 0x0020
                                 CDeterministicMNStateDiff::Field_pubKeyOperator | // 0x0400
                                 CDeterministicMNStateDiff::Field_netInfo;         // 0x1000

    BOOST_CHECK_EQUAL(convertedDiff.fields, expectedNewFields);
    BOOST_CHECK_EQUAL(convertedDiff.state.nVersion, legacyDiff.state.nVersion);
    BOOST_CHECK_EQUAL(convertedDiff.state.GetBannedHeight(), legacyDiff.state.GetBannedHeight());
    BOOST_CHECK(convertedDiff.state.pubKeyOperator.Get() == legacyDiff.state.pubKeyOperator.Get());
    BOOST_CHECK_EQUAL(convertedDiff.state.pubKeyOperator.ToString(), legacyDiff.state.pubKeyOperator.ToString());
}

BOOST_AUTO_TEST_SUITE_END()
