// Copyright (c) 2021-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <messagesigner.h>
#include <miner.h>
#include <netbase.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <spork.h>
#include <validation.h>

#include <evo/deterministicmns.h>
#include <evo/mnhftx.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <governance/governance.h>
#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/context.h>
#include <llmq/instantsend.h>
#include <masternode/payments.h>
#include <util/enumerate.h>
#include <util/irange.h>

#include <boost/test/unit_test.hpp>

#include <map>
#include <vector>

using SimpleUTXOMap = std::map<COutPoint, std::pair<int, CAmount>>;

struct TestChainBRRBeforeActivationSetup : public TestChainSetup
{
    // Force fast DIP3 activation
    TestChainBRRBeforeActivationSetup() : TestChainSetup(497, {"-dip3params=30:50", "-vbparams=mn_rr:0:999999999999:20:16:12:5:1"}) {}
};

static SimpleUTXOMap BuildSimpleUtxoMap(const std::vector<CTransactionRef>& txs)
{
    SimpleUTXOMap utxos;
    for (auto [i, tx] : enumerate(txs)) {
        for (auto [j, output] : enumerate(tx->vout)) {
            utxos.try_emplace(COutPoint(tx->GetHash(), j), std::make_pair((int)i + 1, output.nValue));
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
            if (::ChainActive().Height() - it->second.first < 101) {
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

static void FundTransaction(CMutableTransaction& tx, SimpleUTXOMap& utoxs, const CScript& scriptPayout, CAmount amount)
{
    CAmount change;
    auto inputs = SelectUTXOs(utoxs, amount, change);
    for (const auto& input : inputs) {
        tx.vin.emplace_back(input);
    }
    tx.vout.emplace_back(amount, scriptPayout);
    if (change != 0) {
        tx.vout.emplace_back(change, scriptPayout);
    }
}

static void SignTransaction(const CTxMemPool& mempool, CMutableTransaction& tx, const CKey& coinbaseKey)
{
    FillableSigningProvider tempKeystore;
    tempKeystore.AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());

    for (auto [i, input] : enumerate(tx.vin)) {
        uint256 hashBlock;
        CTransactionRef txFrom = GetTransaction(/* block_index */ nullptr, &mempool, input.prevout.hash, Params().GetConsensus(), hashBlock);
        BOOST_ASSERT(txFrom);
        BOOST_ASSERT(SignSignature(tempKeystore, *txFrom, tx, i, SIGHASH_ALL));
    }
}

static CMutableTransaction CreateProRegTx(const CTxMemPool& mempool, SimpleUTXOMap& utxos, int port, const CScript& scriptPayout, const CKey& coinbaseKey, CKey& ownerKeyRet, CBLSSecretKey& operatorKeyRet)
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

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    FundTransaction(tx, utxos, scriptPayout, dmn_types::Regular.collat_amount);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
    SetTxPayload(tx, proTx);
    SignTransaction(mempool, tx, coinbaseKey);

    return tx;
}

static CScript GenerateRandomAddress()
{
    CKey key;
    key.MakeNewKey(false);
    return GetScriptForDestination(PKHash(key.GetPubKey()));
}

BOOST_AUTO_TEST_SUITE(block_reward_reallocation_tests)

BOOST_FIXTURE_TEST_CASE(block_reward_reallocation, TestChainBRRBeforeActivationSetup)
{
    auto& dmnman = *Assert(m_node.dmnman);
    const auto& consensus_params = Params().GetConsensus();

    CScript coinbasePubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    BOOST_ASSERT(DeploymentDIP0003Enforced(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Height()), consensus_params));

    // Register one MN
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto utxos = BuildSimpleUtxoMap(m_coinbase_txns);
    auto tx = CreateProRegTx(*m_node.mempool, utxos, 1, GenerateRandomAddress(), coinbaseKey, ownerKey, operatorKey);

    CreateAndProcessBlock({tx}, coinbaseKey);

    {
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        dmnman.UpdatedBlockTip(tip);

        BOOST_ASSERT(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));

        BOOST_CHECK_EQUAL(tip->nHeight, 498);
        BOOST_CHECK(tip->nHeight < Params().GetConsensus().BRRHeight);
    }

    CreateAndProcessBlock({}, coinbaseKey);

    {
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        BOOST_CHECK_EQUAL(tip->nHeight, 499);
        dmnman.UpdatedBlockTip(tip);
        BOOST_ASSERT(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK(tip->nHeight < Params().GetConsensus().BRRHeight);
        // Creating blocks by different ways
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
    }
    for ([[maybe_unused]] auto _ : irange::range(1999)) {
        CreateAndProcessBlock({}, coinbaseKey);
        LOCK(cs_main);
        dmnman.UpdatedBlockTip(m_node.chainman->ActiveChain().Tip());
    }
    BOOST_CHECK(m_node.chainman->ActiveChain().Height() < Params().GetConsensus().BRRHeight);
    CreateAndProcessBlock({}, coinbaseKey);

    {
        // Advance to ACTIVE at height = (BRRHeight - 1)
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        BOOST_CHECK_EQUAL(tip->nHeight, Params().GetConsensus().BRRHeight - 1);
        dmnman.UpdatedBlockTip(tip);
        BOOST_ASSERT(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));
    }

    {
        // Reward split should stay ~50/50 before the first superblock after activation.
        // This applies even if reallocation was activated right at superblock height like it does here.
        // next block should be signaling by default
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
        dmnman.UpdatedBlockTip(tip);
        BOOST_ASSERT(dmnman.GetListAtChainTip().HasMN(tx.GetHash()));
        const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        const CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
    }

    for ([[maybe_unused]] auto _ : irange::range(consensus_params.nSuperblockCycle - 1)) {
        CreateAndProcessBlock({}, coinbaseKey);
    }

    {
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
        const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        const CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), 122209530);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, 61104762); // 0.4999999755
    }

    // Reallocation should kick-in with the superblock mined at height = 2010,
    // there will be 19 adjustments, 3 superblocks long each
    for ([[maybe_unused]] auto i : irange::range(19)) {
        for ([[maybe_unused]] auto j : irange::range(3)) {
            for ([[maybe_unused]] auto k : irange::range(consensus_params.nSuperblockCycle)) {
                CreateAndProcessBlock({}, coinbaseKey);
            }
            LOCK(cs_main);
            const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
            const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
            const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
            const CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
            const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
            BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        }
    }
    BOOST_CHECK(DeploymentActiveAfter(m_node.chainman->ActiveChain().Tip(), consensus_params, Consensus::DEPLOYMENT_V20));
    // Allocation of block subsidy is 60% MN, 20% miners and 20% treasury
    {
        // Reward split should reach ~75/25 after reallocation is done
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
        const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        const CAmount block_subsidy_sb = GetSuperblockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        CAmount block_subsidy_potential = block_subsidy + block_subsidy_sb;
        BOOST_CHECK_EQUAL(block_subsidy_potential, 84437941);
        CAmount expected_block_reward = block_subsidy_potential - block_subsidy_potential / 5;

        const CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), expected_block_reward);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), 67550353);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, 50662764); // 0.75
    }
    BOOST_CHECK(!DeploymentActiveAfter(m_node.chainman->ActiveChain().Tip(), consensus_params, Consensus::DEPLOYMENT_MN_RR));

    // Activate EHF "MN_RR"
    {
        LOCK(cs_main);
        m_node.mnhf_manager->AddSignal(m_node.chainman->ActiveChain().Tip(), 10);
    }

    // Reward split should stay ~75/25 after reallocation is done,
    // check 10 next superblocks
    for ([[maybe_unused]] auto i : irange::range(10)) {
        for ([[maybe_unused]] auto k : irange::range(consensus_params.nSuperblockCycle)) {
            CreateAndProcessBlock({}, coinbaseKey);
        }
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
        const bool isMNRewardReallocated{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_MN_RR)};
        const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);

        if (isMNRewardReallocated) {
            const CAmount platform_payment = MasternodePayments::PlatformShare(masternode_payment);
            masternode_payment -= platform_payment;
        }
        size_t payment_index = isMNRewardReallocated ? 1 : 0;

        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[payment_index].nValue, masternode_payment);
    }

    BOOST_CHECK(DeploymentActiveAfter(m_node.chainman->ActiveChain().Tip(), consensus_params, Consensus::DEPLOYMENT_MN_RR));
    { // At this moment Masternode reward should be reallocated to platform
        // Allocation of block subsidy is 60% MN, 20% miners and 20% treasury
        LOCK(cs_main);
        const CBlockIndex* const tip{m_node.chainman->ActiveChain().Tip()};
        const bool isV20Active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V20)};
        const CAmount block_subsidy = GetBlockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        const CAmount block_subsidy_sb = GetSuperblockSubsidyInner(tip->nBits, tip->nHeight, consensus_params, isV20Active);
        CAmount masternode_payment = GetMasternodePayment(tip->nHeight, block_subsidy, isV20Active);
        const CAmount platform_payment = MasternodePayments::PlatformShare(masternode_payment);
        masternode_payment -= platform_payment;
        const auto pblocktemplate = BlockAssembler(*m_node.sporkman, *m_node.govman, *m_node.llmq_ctx, *m_node.evodb, m_node.chainman->ActiveChainstate(), *m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);

        CAmount block_subsidy_potential = block_subsidy + block_subsidy_sb;
        BOOST_CHECK_EQUAL(tip->nHeight, 3858);
        BOOST_CHECK_EQUAL(block_subsidy_potential, 78406660);
        // Treasury is 20% since MNRewardReallocation
        CAmount expected_block_reward = block_subsidy_potential - block_subsidy_potential / 5;
        // Since MNRewardReallocation, MN reward share is 75% of the block reward
        CAmount expected_masternode_reward = expected_block_reward * 3 / 4;
        CAmount expected_mn_platform_payment = MasternodePayments::PlatformShare(expected_masternode_reward);
        CAmount expected_mn_core_payment = expected_masternode_reward - expected_mn_platform_payment;

        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), expected_block_reward);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[1].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[1].nValue, expected_mn_core_payment);
    }
}

BOOST_AUTO_TEST_SUITE_END()
