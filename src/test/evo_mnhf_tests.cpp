// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/chainhelper.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <evo/specialtx.h>
#include <llmq/context.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/string.h>
#include <validation.h>
#include <versionbits.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

#include <cstdint>
#include <vector>


static CMutableTransaction CreateMNHFTx(const uint256& quorumHash, const CBLSSignature& cblSig, const uint16_t& versionBit)
{
    MNHFTxPayload extraPayload;
    extraPayload.nVersion = 1;
    extraPayload.signal.versionBit = versionBit;
    extraPayload.signal.quorumHash = quorumHash;
    extraPayload.signal.sig = cblSig;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_MNHF_SIGNAL;
    SetTxPayload(tx, extraPayload);

    return tx;
}

BOOST_FIXTURE_TEST_SUITE(evo_mnhf_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(verify_mnhf_specialtx_tests)
{
    int count = 10;
    uint16_t bit = 1;

    std::vector<CBLSSignature> vec_sigs;
    std::vector<CBLSPublicKey> vec_pks;
    std::vector<CBLSSecretKey> vec_sks;

    CBLSSecretKey sk;
    for (int i = 0; i < count; i++) {
        sk.MakeNewKey();
        vec_pks.push_back(sk.GetPublicKey());
        vec_sks.push_back(sk);
    }

    CBLSSecretKey ag_sk = CBLSSecretKey::AggregateInsecure(vec_sks);
    CBLSPublicKey ag_pk = CBLSPublicKey::AggregateInsecure(vec_pks);

    BOOST_CHECK(ag_sk.IsValid());
    BOOST_CHECK(ag_pk.IsValid());

    const bool use_legacy{false};
    uint256 verHash = uint256S(ToString(bit));
    auto sig = ag_sk.Sign(verHash, use_legacy);
    BOOST_CHECK(sig.VerifyInsecure(ag_pk, verHash, use_legacy));

    for (const int bit : {-1, VERSIONBITS_NUM_BITS, 255}) {
        BOOST_CHECK(!Params().IsValidMNActivation(bit, 0));
    }

    auto& chainman = Assert(m_node.chainman);
    auto& qman = *Assert(m_node.llmq_ctx)->qman;
    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return chainman->ActiveChain().Tip());
    uint256 hash = GetRandHash();
    TxValidationState state;

    { // wrong quorum (we don't have any indeed)
        const CTransaction tx{CTransaction(CreateMNHFTx(hash, sig, bit))};
        CheckMNHFTx(chainman->m_blockman, qman, CTransaction(tx), pindex, state);
        BOOST_CHECK_EQUAL(state.ToString(), "bad-mnhf-quorum-hash");
    }

    { // non EHF fork
        const CTransaction tx{CTransaction(CreateMNHFTx(hash, sig, 28))};
        CheckMNHFTx(chainman->m_blockman, qman, CTransaction(tx), pindex, state);
        BOOST_CHECK_EQUAL(state.ToString(), "bad-mnhf-non-ehf");
    }
}

/**
 * Reconfigure the permanent "testdummy" deployment (never buried, present on
 * every network) as an EHF deployment with a window entirely in the future.
 * This stands in for a version bit that has just been *reused* by a newer
 * deployment: DIP-0023 allows a later deployment to reuse a bit as long as its
 * window is later than the previous one's. We deliberately avoid referencing a
 * real deployment such as v24 here, since those get buried and removed from
 * vDeployments over time, which would break this fixture.
 */
struct MnhfBitReuseSetup : public RegTestingSetup {
    MnhfBitReuseSetup()
        : RegTestingSetup{{"-vbparams=testdummy:2000000000:9223372036854775807:0:250:200:150:5:1"}}
    {
    }
};

/**
 * DIP-0023 bit reuse: Dash buries old deployments and drops them from
 * vDeployments, so on reindex a historical MnEHF signal for a buried deployment
 * must stay valid even after its bit is reused by a newer deployment whose
 * window has not started yet.
 *
 * Here the reused bit (testdummy) has a future window. A query at a time before
 * that window represents revalidating a buried signal mined by the earlier (now
 * removed) deployment that owned the bit. The out-of-range check runs before the
 * useEHF check, so this exercises the same code path regardless of useEHF; we
 * still set useehf=1 above so the scenario matches a reused EHF bit exactly.
 *
 * On unpatched develop IsValidMNActivation() returns true for this case (the
 * out-of-range deployment is skipped and the unknown-bit fallback accepts it),
 * so the block stays valid on reindex. PR #7446 makes it return false, which
 * rejects a block that was accepted before the reuse -> consensus divergence.
 */
BOOST_FIXTURE_TEST_CASE(mnhf_bit_reuse_reindex_tests, MnhfBitReuseSetup)
{
    const CChainParams& params = Params();
    const auto& reused = params.GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];
    const int reused_bit = reused.bit;
    const int64_t before_window = reused.nStartTime - 1; // buried-signal era, before the reuse window

    // Sanity: a genuinely unknown/retired bit still validates (unchanged fallback).
    BOOST_CHECK(params.IsValidMNActivation(/*nBit=*/5, before_window));

    // Bit reuse: the buried signal predates the reused deployment's window.
    // DIP-0023 compatibility requires it to remain valid on reindex.
    BOOST_CHECK(params.IsValidMNActivation(reused_bit, before_window));
}

struct V20FromGenesisSetup : public RegTestingSetup {
    V20FromGenesisSetup() :
        RegTestingSetup{{"-dip3params=0:0", "-testactivationheight=v19@0", "-testactivationheight=v20@0",
                         "-testactivationheight=mn_rr@0"}}
    {
    }
};

BOOST_FIXTURE_TEST_CASE(genesis_has_no_signals_when_v20_is_active_from_genesis, V20FromGenesisSetup)
{
    auto& chainman = *Assert(m_node.chainman);
    const CBlockIndex* genesis = WITH_LOCK(::cs_main, return chainman.ActiveChain().Genesis());
    BOOST_REQUIRE(genesis);
    BOOST_REQUIRE(DeploymentActiveAt(*genesis, chainman.GetConsensus(), Consensus::DEPLOYMENT_V20));
    BOOST_CHECK(Assert(m_node.chain_helper)->ehf_manager->GetSignalsStage(genesis).empty());
}

BOOST_FIXTURE_TEST_CASE(signals_unavailable_for_unconnected_block, TestChainV19Setup)
{
    auto& chainman = *Assert(m_node.chainman);
    auto& mnhfman = *Assert(m_node.chain_helper)->ehf_manager;

    const CBlock connected = CreateBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()), chainman.ActiveChainstate());
    const CBlock sibling = CreateBlock({}, CScript() << OP_TRUE, chainman.ActiveChainstate());
    BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<const CBlock>(connected), true, nullptr));
    BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<const CBlock>(sibling), true, nullptr));

    const auto [connected_index, sibling_index] = WITH_LOCK(::cs_main, return std::make_pair(
        chainman.m_blockman.LookupBlockIndex(connected.GetHash()), chainman.m_blockman.LookupBlockIndex(sibling.GetHash())));
    BOOST_REQUIRE(connected_index && sibling_index);
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()), connected_index);
    BOOST_CHECK(WITH_LOCK(::cs_main, return (sibling_index->nStatus & BLOCK_HAVE_DATA) &&
                                            !sibling_index->IsValid(BLOCK_VALID_SCRIPTS)));
    BOOST_REQUIRE(DeploymentActiveAfter(sibling_index, chainman.GetConsensus(), Consensus::DEPLOYMENT_V20));

    BOOST_CHECK_NO_THROW(mnhfman.GetSignalsStage(connected_index));
    BOOST_CHECK_THROW(mnhfman.GetSignalsStage(sibling_index), EvoDbMissingStateError);

    // Connecting the sibling stores its state
    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, connected_index));
    BOOST_REQUIRE(chainman.ActiveChainstate().ActivateBestChain(state));
    BOOST_CHECK_EQUAL(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()), sibling_index);
    BOOST_CHECK_NO_THROW(mnhfman.GetSignalsStage(sibling_index));
}

BOOST_AUTO_TEST_SUITE_END()
