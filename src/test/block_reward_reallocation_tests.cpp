// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <keystore.h>
#include <messagesigner.h>
#include <miner.h>
#include <netbase.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <script/standard.h>
#include <validation.h>

#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>

#include <boost/test/unit_test.hpp>

#include <map>
#include <vector>

using SimpleUTXOMap = std::map<COutPoint, std::pair<int, CAmount>>;

const auto deployment_id = Consensus::DEPLOYMENT_REALLOC;
const int window{500}, th_start{400}, th_end{300};

struct TestChainBRRBeforeActivationSetup : public TestChainSetup
{
    TestChainBRRBeforeActivationSetup() : TestChainSetup(497)
    {
        // Force fast DIP3 activation
        gArgs.ForceSetArg("-dip3params", "30:50");
        SelectParams(CBaseChainParams::REGTEST);
        gArgs.ForceRemoveArg("-dip3params");
    }
};

static SimpleUTXOMap BuildSimpleUtxoMap(const std::vector<CTransactionRef>& txs)
{
    SimpleUTXOMap utxos;
    for (size_t i = 0; i < txs.size(); i++) {
        auto& tx = txs[i];
        for (size_t j = 0; j < tx->vout.size(); j++) {
            utxos.try_emplace(COutPoint(tx->GetHash(), j), std::make_pair((int)i + 1, tx->vout[j].nValue));
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
    FundTransaction(tx, utxos, scriptPayout, 1000 * COIN);
    proTx.inputsHash = CalcTxInputsHash(CTransaction(tx));
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

static constexpr int threshold(int attempt)
{
    // An implementation of VersionBitsConditionChecker::Threshold()
    int threshold_calc = th_start - attempt * attempt * window / 100 / 5;
    if (threshold_calc < th_end) {
        return th_end;
    }
    return threshold_calc;
}

BOOST_AUTO_TEST_SUITE(block_reward_reallocation_tests)

BOOST_FIXTURE_TEST_CASE(block_reward_reallocation, TestChainBRRBeforeActivationSetup)
{
    const auto& consensus_params = Params().GetConsensus();
    CScript coinbasePubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    auto signal = [&](int num_blocks, bool expected_lockin)
    {
        // Mine non-signalling blocks
        gArgs.ForceSetArg("-blockversion", "536870912");
        for (int i = 0; i < window - num_blocks; ++i) {
            CreateAndProcessBlock({}, coinbaseKey);
            LOCK(cs_main);
            deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        }
        gArgs.ForceRemoveArg("-blockversion");
        if (num_blocks > 0) {
            // Mine signalling blocks
            for (int i = 0; i < num_blocks; ++i) {
                CreateAndProcessBlock({}, coinbaseKey);
                LOCK(cs_main);
                deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
            }
        }
        LOCK(cs_main);
        if (expected_lockin) {
            BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::LOCKED_IN);
        } else {
            BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::STARTED);
        }
    };

    BOOST_ASSERT(deterministicMNManager->IsDIP3Enforced(WITH_LOCK(cs_main, return ::ChainActive().Height())));

    // Register one MN
    CKey ownerKey;
    CBLSSecretKey operatorKey;
    auto utxos = BuildSimpleUtxoMap(m_coinbase_txns);
    auto tx = CreateProRegTx(utxos, 1, GenerateRandomAddress(), coinbaseKey, ownerKey, operatorKey);

    CreateAndProcessBlock({tx}, coinbaseKey);

    {
        LOCK(cs_main);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());

        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));

        BOOST_CHECK_EQUAL(::ChainActive().Height(), 498);
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::DEFINED);
    }

    CreateAndProcessBlock({}, coinbaseKey);

    {
        LOCK(cs_main);
        // Advance from DEFINED to STARTED at height = 499
        BOOST_CHECK_EQUAL(::ChainActive().Height(), 499);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::STARTED);
        BOOST_CHECK_EQUAL(VersionBitsTipStatistics(consensus_params, deployment_id).threshold, threshold(0));
        // Next block should be signaling by default
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        const uint32_t bitmask = ((uint32_t)1) << consensus_params.vDeployments[deployment_id].bit;
        BOOST_CHECK_EQUAL(::ChainActive().Tip()->nVersion & bitmask, 0);
        BOOST_CHECK_EQUAL(pblocktemplate->block.nVersion & bitmask, bitmask);
    }

    signal(threshold(0) - 1, false); // 1 block short

    {
        // Still STARTED but new threshold should be lower at height = 999
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(::ChainActive().Height(), 999);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::STARTED);
        BOOST_CHECK_EQUAL(VersionBitsTipStatistics(consensus_params, deployment_id).threshold, threshold(1));
    }

    signal(threshold(1) - 1, false); // 1 block short again

    {
        // Still STARTED but new threshold should be even lower at height = 1499
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(::ChainActive().Height(), 1499);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::STARTED);
        BOOST_CHECK_EQUAL(VersionBitsTipStatistics(consensus_params, deployment_id).threshold, threshold(2));
    }

    signal(threshold(2), true); // just enough to lock in

    {
        // Advanced to LOCKED_IN at height = 1999
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(::ChainActive().Height(), 1999);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::LOCKED_IN);
    }

    for (int i = 0; i < 499; ++i) {
        // Still LOCKED_IN at height = 2498
        CreateAndProcessBlock({}, coinbaseKey);
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::LOCKED_IN);
    }
    CreateAndProcessBlock({}, coinbaseKey);

    {
        // Advance from LOCKED_IN to ACTIVE at height = 2499
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(::ChainActive().Height(), 2499);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        BOOST_CHECK_EQUAL(VersionBitsTipState(consensus_params, deployment_id), ThresholdState::ACTIVE);
        BOOST_CHECK_EQUAL(VersionBitsTipStateSinceHeight(consensus_params, deployment_id), 2500);
    }

    {
        // Reward split should stay ~50/50 before the first superblock after activation.
        // This applies even if reallocation was activated right at superblock height like it does here.
        // next block should be signaling by default
        LOCK(cs_main);
        deterministicMNManager->UpdatedBlockTip(::ChainActive().Tip());
        BOOST_ASSERT(deterministicMNManager->GetListAtChainTip().HasMN(tx.GetHash()));
        auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
    }

    for (int i = 0; i < 9; ++i) {
        CreateAndProcessBlock({}, coinbaseKey);
    }

    {
        LOCK(cs_main);
        auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), 13748571607);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, 6874285801); // 0.4999999998
    }

    // Reallocation should kick-in with the superblock mined at height = 2010,
    // there will be 19 adjustments, 3 superblocks long each
    for (int i = 0; i < 19; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 10; ++k) {
                CreateAndProcessBlock({}, coinbaseKey);
            }
            LOCK(cs_main);
            auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
            const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
            BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        }
    }

    {
        // Reward split should reach ~60/40 after reallocation is done
        LOCK(cs_main);
        auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), 10221599170);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, 6132959502); // 0.6
    }

    // Reward split should stay ~60/40 after reallocation is done,
    // check 10 next superblocks
    for (int i = 0; i < 10; ++i) {
        for (int k = 0; k < 10; ++k) {
            CreateAndProcessBlock({}, coinbaseKey);
        }
        LOCK(cs_main);
        auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
    }

    {
        // Reward split should reach ~60/40 after reallocation is done
        LOCK(cs_main);
        auto masternode_payment = GetMasternodePayment(::ChainActive().Height(), GetBlockSubsidy(::ChainActive().Tip()->nBits, ::ChainActive().Height(), consensus_params), 2500);
        const auto pblocktemplate = BlockAssembler(*m_node.mempool, Params()).CreateNewBlock(coinbasePubKey);
        BOOST_CHECK_EQUAL(pblocktemplate->block.vtx[0]->GetValueOut(), 9491484944);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, masternode_payment);
        BOOST_CHECK_EQUAL(pblocktemplate->voutMasternodePayments[0].nValue, 5694890966); // 0.6
    }
}

BOOST_AUTO_TEST_SUITE_END()
