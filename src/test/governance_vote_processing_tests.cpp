// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <consensus/amount.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <governance/vote.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <key.h>
#include <key_io.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <messagesigner.h>
#include <netfulfilledman.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <streams.h>
#include <timedata.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <test/util/index.h>
#include <test/util/masternode.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono_literals;

namespace {
// TestChainSetup only accepts checkpointed chain lengths, so 107 blocks plus
// -dip3params=109:500 is the shortest chain a ProRegTx can be mined on. Same setup as
// TestChainDIP3Setup in evo_deterministicmns_tests.cpp.
constexpr int DIP3_ACTIVATION_HEIGHT{109};

void SignWithVotingKey(CGovernanceVote& vote, const CKey& key)
{
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(CMessageSigner::SignMessage(vote.GetSignatureString(), sig, key));
    vote.SetSignature(sig);
}

void SignWithOperatorKey(CGovernanceVote& vote, const CBLSSecretKey& key)
{
    // Vote signatures are always basic-scheme, regardless of the currently active BLS scheme:
    // CGovernanceVote::CheckSignature() deserializes and verifies with legacy=false.
    vote.SetSignature(
        key.Sign(vote.GetSignatureHash(), /*specificLegacyScheme=*/false).ToByteVector(/*specificLegacyScheme=*/false));
}

//! A trigger that is signed by the fixture masternode and passes IsValidLocally, but whose payload
//! is not a superblock, so SuperblockManager::AddTrigger rejects it after it was inserted.
CGovernanceObject MakeFailedTrigger(const COutPoint& mn_collateral, const CBLSSecretKey& operator_key,
                                    int64_t creation_time, int salt)
{
    const std::string data{strprintf(R"({"type":2,"salt":%d})", salt)};
    CGovernanceObject govobj{uint256{}, /*revision=*/1, creation_time, uint256{}, HexStr(data)};
    BOOST_REQUIRE(govobj.GetObjectType() == GovernanceObject::TRIGGER);
    govobj.SetMasternodeOutpoint(mn_collateral);
    govobj.SetSignature(operator_key.Sign(govobj.GetSignatureHash(), /*specificLegacyScheme=*/false)
                            .ToByteVector(/*specificLegacyScheme=*/false));
    return govobj;
}

// A chain with one registered masternode whose voting (ECDSA) and operator (BLS) keys are known
// to the test, so votes can be signed for real and CGovernanceVote::CheckSignature is actually
// reached. Without a populated masternode list every CGovernanceVote::IsValid() call would
// short-circuit at GetMNByCollateral and no signature would ever be verified.
struct GovernanceVoteSetup : public TestChainSetup {
    CKey mn_voting_key;
    CBLSSecretKey mn_operator_key;
    COutPoint mn_collateral;
    SimpleUTXOMap utxos;
    //! Fixed so a proposal can be rebuilt bit-for-bit (and keep its hash) once its fee tx is known.
    int64_t proposal_time{0};
    std::string proposal_payment_address;

    explicit GovernanceVoteSetup(const char* dip3_params = "-dip3params=109:500") :
        TestChainSetup(DIP3_ACTIVATION_HEIGHT - 2, CBaseChainParams::REGTEST, {dip3_params})
    {
        // A failed BOOST_REQUIRE below throws, and a throwing constructor means no destructor runs.
        // Tear the globals down by hand so one broken invariant here doesn't leave a live tx index
        // (and a dangling m_node reference in it) for every later test in the binary.
        try {
            SetUp();
        } catch (...) {
            TearDown();
            throw;
        }
    }

    ~GovernanceVoteSetup() { TearDown(); }

    void SetUp()
    {
        // CGovernanceObject::IsCollateralValid() reads the proposal fee tx out of the tx index;
        // without it no proposal can ever be accepted, orphan votes included.
        g_txindex = std::make_unique<TxIndex>(interfaces::MakeChain(m_node), /*n_cache_size=*/1 << 20,
                                              /*f_memory=*/true);
        BOOST_REQUIRE(g_txindex->Start());
        IndexWaitSynced(*g_txindex);

        utxos = BuildSimpleUtxoMap(m_coinbase_txns);

        // Activate DIP3, then register the masternode that votes in these tests.
        MineBlocks(1);
        auto protx = CreateProRegTx(*m_node.chainman, utxos, /*port=*/1, payout_script(), coinbaseKey, mn_voting_key,
                                    mn_operator_key);
        mn_collateral = COutPoint(protx.GetHash(), 0);
        MineBlock({protx});
        BOOST_REQUIRE(tip_mn_list().GetMNByCollateral(mn_collateral) != nullptr);

        BOOST_REQUIRE(m_node.mn_metaman->LoadCache(/*load_cache=*/false));
        BOOST_REQUIRE(m_node.netfulfilledman->LoadCache(/*load_cache=*/false));
        // Governance relay and CheckAndRemove() are no-ops until the node considers itself synced.
        // Two SwitchToNextAsset() calls get there from BLOCKCHAIN; the bound is just a loop guard.
        for (int i = 0; i < 5 && !m_node.mn_sync->IsSynced(); ++i) {
            m_node.mn_sync->SwitchToNextAsset();
        }
        BOOST_REQUIRE(m_node.mn_sync->IsSynced());

        m_node.govman = std::make_unique<CGovernanceManager>(*m_node.mn_metaman, *m_node.chainman,
                                                             *m_node.chain_helper->superblocks, *m_node.dmnman,
                                                             *m_node.mn_sync);
        BOOST_REQUIRE(m_node.govman->LoadCache(/*load_cache=*/false));

        proposal_time = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
        CKey payment_key;
        payment_key.MakeNewKey(true);
        proposal_payment_address = EncodeDestination(PKHash(payment_key.GetPubKey()));
    }

    void TearDown()
    {
        // govman holds a reference into chain_helper, so it must go first (matches PrepareShutdown
        // ordering in init.cpp).
        m_node.govman.reset();
        if (g_txindex) {
            SyncWithValidationInterfaceQueue();
            g_txindex->Stop();
            g_txindex.reset();
        }
    }

    CScript coinbase_script() const { return GetScriptForRawPubKey(coinbaseKey.GetPubKey()); }

    //! Masternode payouts and the proposal fee change must go to a P2PKH script (bad-protx-payee
    //! rejects the raw-pubkey coinbase script, IsCollateralValid() rejects anything else).
    CScript payout_script() const { return GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())); }

    CDeterministicMNList tip_mn_list() const { return m_node.dmnman->GetListAtChainTip(); }

    void MineBlock(const std::vector<CMutableTransaction>& txns)
    {
        const CBlock block = CreateAndProcessBlock(txns, coinbase_script());
        const CBlockIndex* tip{WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip())};
        BOOST_REQUIRE_EQUAL(tip->GetBlockHash(), block.GetHash());
        m_node.dmnman->UpdatedBlockTip(tip);
        SetMockTime(GetTime() + 1);
        IndexWaitSynced(*g_txindex);
    }

    void MineBlocks(int num_blocks)
    {
        for (int i = 0; i < num_blocks; ++i) {
            MineBlock({});
        }
    }

    //! The proposal both the orphan vote and the fee tx below commit to. The object hash
    //! deliberately excludes the collateral hash, so the same proposal can be built first with an
    //! empty collateral (to learn its hash) and again once the fee tx paying to that hash exists.
    CGovernanceObject MakeProposal(const uint256& collateral_hash) const
    {
        const std::string data{
            strprintf("{\"type\":1,\"name\":\"test-proposal\",\"start_epoch\":%d,\"end_epoch\":%d,\"payment_amount\":1.0,\"payment_address\":\"%s\",\"url\":\"https://dash.org\"}",
                      proposal_time, proposal_time + 100000, proposal_payment_address)};
        return CGovernanceObject{uint256{}, /*revision=*/1, proposal_time, collateral_hash, HexStr(data)};
    }

    //! Burns the proposal fee to an OP_RETURN committing to the object hash, the way
    //! `gobject prepare` does, and buries it under the confirmations the collateral check needs.
    uint256 ConfirmProposalCollateral(const uint256& govobj_hash)
    {
        CMutableTransaction tx;
        // IsCollateralValid() rejects the fee tx unless every output is P2PKH or unspendable, so
        // the change cannot go back to the burn script.
        const auto spent = FundTransaction(*m_node.chainman, tx, utxos,
                                           CScript() << OP_RETURN << ToByteVector(govobj_hash),
                                           GOVERNANCE_PROPOSAL_FEE_TX, /*script_change=*/payout_script());
        SignTransaction(tx, spent, coinbaseKey);

        MineBlock({tx});
        MineBlocks(GOVERNANCE_FEE_CONFIRMATIONS - 1);
        return tx.GetHash();
    }

    //! A vote's hash covers everything but its signature, so votes that should be distinguishable
    //! (e.g. a re-signed vote) need distinct timestamps.
    CGovernanceVote MakeVote(const uint256& parent_hash, vote_signal_enum_t signal, vote_outcome_enum_t outcome,
                             std::chrono::seconds time_offset = 0s) const
    {
        CGovernanceVote vote{mn_collateral, parent_hash, signal, outcome};
        vote.SetTime(GetAdjustedTime() + time_offset);
        return vote;
    }
};

struct TestGovernanceStore : GovernanceStore {
    using GovernanceStore::last_object_rec;

    size_t ObjectCount() const
    {
        LOCK(cs_store);
        return mapObjects.size();
    }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(governance_vote_processing_tests, GovernanceVoteSetup)

struct GovernanceReorgSetup : GovernanceVoteSetup {
    GovernanceReorgSetup() :
        GovernanceVoteSetup("-dip3params=109:109")
    {
    }
};

BOOST_FIXTURE_TEST_CASE(votes_follow_masternode_identity_across_reorg, GovernanceReorgSetup)
{
    auto& chainman = *m_node.chainman;
    auto& govman = *m_node.govman;
    CKey owner_key, other_owner_key, voting_key;
    CBLSSecretKey operator_key, other_operator_key;
    voting_key.MakeNewKey(true);
    const auto registration = CreateProRegTx(chainman, utxos, 2, payout_script(), coinbaseKey, owner_key, operator_key);
    const auto other_registration = CreateProRegTx(chainman, utxos, 3, payout_script(), coinbaseKey, other_owner_key,
                                                   other_operator_key);
    MineBlock({registration});
    CBlockIndex* fork_block{WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip())};
    const auto old_id = tip_mn_list().GetMN(registration.GetHash())->GetInternalId();

    CProUpRegTx update;
    update.nVersion = ProTxVersion::GetMax(!bls::bls_legacy_scheme, /*is_extended_addr=*/false);
    update.proTxHash = registration.GetHash();
    update.pubKeyOperator.Set(operator_key.GetPublicKey(), /*specificLegacyScheme=*/bls::bls_legacy_scheme);
    update.keyIDVoting = voting_key.GetPubKey().GetID();
    update.scriptPayout = payout_script();
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    const auto spent = FundTransaction(chainman, tx, utxos, payout_script(), COIN);
    update.inputsHash = CalcTxInputsHash(CTransaction(tx));
    BOOST_REQUIRE(CHashSigner::SignHash(::SerializeHash(update), owner_key, update.vchSig));
    SetTxPayload(tx, update);
    SignTransaction(tx, spent, coinbaseKey);
    MineBlock({tx});
    govman.UpdatedBlockTip(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

    const auto proposal = MakeProposal(uint256::ONE);
    const auto parent_hash = proposal.GetHash();
    govman.AddGovernanceObjectForTesting(proposal);
    auto unchanged_vote = MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES);
    SignWithVotingKey(unchanged_vote, mn_voting_key);
    CGovernanceVote changed_vote{COutPoint{registration.GetHash(), 0}, parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES};
    SignWithVotingKey(changed_vote, voting_key);
    CGovernanceException exception;
    uint256 hash_to_request;
    BOOST_REQUIRE(govman.ProcessVote(unchanged_vote, exception, hash_to_request));
    BOOST_REQUIRE(govman.ProcessVote(changed_vote, exception, hash_to_request));
    BOOST_REQUIRE(govman.HaveVoteForHash(changed_vote.GetHash()));

    BlockValidationState state;
    BOOST_REQUIRE(chainman.ActiveChainstate().InvalidateBlock(state, fork_block));
    MineBlock({other_registration});
    MineBlock({registration});
    BOOST_REQUIRE(old_id != tip_mn_list().GetMN(registration.GetHash())->GetInternalId());
    govman.UpdatedBlockTip(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

    BOOST_CHECK(!govman.HaveVoteForHash(changed_vote.GetHash()));
    BOOST_CHECK(govman.GetCurrentVotes(parent_hash, changed_vote.GetMasternodeOutpoint()).empty());
    BOOST_CHECK(govman.HaveVoteForHash(unchanged_vote.GetHash()));
    BOOST_CHECK_EQUAL(govman.GetCurrentVotes(parent_hash, mn_collateral).size(), 1U);
    const auto stored = govman.FindConstGovernanceObject(parent_hash);
    BOOST_REQUIRE(stored != nullptr);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);
}

// A vote that arrives before its parent object must be kept and replayed once the object shows up,
// otherwise every vote that races ahead of its proposal is lost for good.
BOOST_AUTO_TEST_CASE(orphan_vote_is_cached_and_applied_when_parent_arrives)
{
    auto& govman = *m_node.govman;

    // Build the proposal but keep it out of the manager: the vote arrives first.
    const uint256 parent_hash{MakeProposal(uint256{}).GetHash()};
    BOOST_REQUIRE(!govman.HaveObjectForHash(parent_hash));

    CGovernanceVote vote{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES)};
    SignWithVotingKey(vote, mn_voting_key);
    // Guard: the orphan gate rejects a vote it cannot verify against the masternode list, so if
    // the masternode or its signature were not recognized every check below would fail at once.
    // Requiring validity up front turns that diffuse failure into one pointed at the setup.
    BOOST_REQUIRE(vote.IsValid(tip_mn_list(), /*useVotingKey=*/true));

    CGovernanceException exception;
    uint256 hash_to_request;
    BOOST_CHECK(!govman.ProcessVote(vote, exception, hash_to_request));
    BOOST_CHECK_EQUAL(exception.GetType(), GOVERNANCE_EXCEPTION_WARNING);
    BOOST_CHECK_EQUAL(exception.GetNodePenalty(), 0);
    // The caller uses this to ask the sending peer for the missing object.
    BOOST_CHECK_EQUAL(hash_to_request, parent_hash);
    BOOST_CHECK(govman.GetOrphanVoteObjectHashes() == std::vector<uint256>{parent_hash});

    const uint256 collateral_hash{ConfirmProposalCollateral(parent_hash)};
    CGovernanceObject proposal{MakeProposal(collateral_hash)};
    BOOST_REQUIRE_EQUAL(proposal.GetHash(), parent_hash);
    govman.AddGovernanceObject(proposal, /*peer_str=*/"");
    BOOST_REQUIRE(govman.HaveObjectForHash(parent_hash));

    // The orphan vote is now applied to the object it was waiting for and is no longer pending.
    auto stored = govman.FindConstGovernanceObject(parent_hash);
    BOOST_REQUIRE(stored != nullptr);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);
    BOOST_CHECK_EQUAL(govman.GetCurrentVotes(parent_hash, mn_collateral).size(), 1U);
    BOOST_CHECK(govman.GetOrphanVoteObjectHashes().empty());
    // Known gap, pinned here so a fix has to update this test: CheckOrphanVotes() applies the
    // vote to the object but never indexes it in cmapVoteToObject, so the vote inv it relays
    // during the replay cannot be served to a peer that requests it.
    BOOST_CHECK(!govman.HaveVoteForHash(vote.GetHash()));

    // A peer re-sending the same vote must not be punished for it.
    CGovernanceException replay_exception;
    uint256 replay_hash_to_request;
    BOOST_CHECK(!govman.ProcessVote(vote, replay_exception, replay_hash_to_request));
    BOOST_CHECK_EQUAL(replay_exception.GetNodePenalty(), 0);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);
}

// Votes are only counted if they carry a signature from a registered masternode.
BOOST_AUTO_TEST_CASE(unsigned_and_unknown_masternode_votes_are_rejected)
{
    auto& govman = *m_node.govman;

    const CGovernanceObject proposal{MakeProposal(uint256::ONE)};
    const uint256 parent_hash{proposal.GetHash()};
    govman.AddGovernanceObjectForTesting(proposal);
    BOOST_REQUIRE(govman.HaveObjectForHash(parent_hash));

    // Forged signature: right masternode, wrong key.
    CKey attacker_key;
    attacker_key.MakeNewKey(true);
    CGovernanceVote forged{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES)};
    SignWithVotingKey(forged, attacker_key);

    CGovernanceException exception;
    uint256 hash_to_request;
    BOOST_CHECK(!govman.ProcessVote(forged, exception, hash_to_request));
    BOOST_CHECK_EQUAL(exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(exception.GetNodePenalty(), 20);
    BOOST_CHECK(!govman.HaveVoteForHash(forged.GetHash()));

    // Re-sending the same invalid vote is still rejected and punished.
    CGovernanceException repeat_exception;
    BOOST_CHECK(!govman.ProcessVote(forged, repeat_exception, hash_to_request));
    BOOST_CHECK_EQUAL(repeat_exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(repeat_exception.GetNodePenalty(), 20);

    // Forged operator signature: right masternode, wrong BLS key. A non-funding signal, so the
    // operator-key (BLS) verification path is the one that must reject it.
    CBLSSecretKey attacker_operator_key;
    attacker_operator_key.MakeNewKey();
    CGovernanceVote forged_bls{MakeVote(parent_hash, VOTE_SIGNAL_VALID, VOTE_OUTCOME_YES)};
    SignWithOperatorKey(forged_bls, attacker_operator_key);

    CGovernanceException forged_bls_exception;
    BOOST_CHECK(!govman.ProcessVote(forged_bls, forged_bls_exception, hash_to_request));
    BOOST_CHECK_EQUAL(forged_bls_exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(forged_bls_exception.GetNodePenalty(), 20);
    BOOST_CHECK(!govman.HaveVoteForHash(forged_bls.GetHash()));

    // Correctly signed, but the outpoint belongs to no masternode.
    CKey stranger_key;
    stranger_key.MakeNewKey(true);
    CGovernanceVote stranger{COutPoint{uint256S("42"), 0}, parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES};
    stranger.SetTime(GetAdjustedTime());
    SignWithVotingKey(stranger, stranger_key);

    CGovernanceException stranger_exception;
    BOOST_CHECK(!govman.ProcessVote(stranger, stranger_exception, hash_to_request));
    BOOST_CHECK_EQUAL(stranger_exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(stranger_exception.GetNodePenalty(), 20);

    // A vote dated too far in the future is rejected even with a valid signature.
    CGovernanceVote from_the_future{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES, 2h)};
    SignWithVotingKey(from_the_future, mn_voting_key);

    CGovernanceException future_exception;
    BOOST_CHECK(!govman.ProcessVote(from_the_future, future_exception, hash_to_request));
    BOOST_CHECK_EQUAL(future_exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(future_exception.GetNodePenalty(), 20);

    auto stored = govman.FindConstGovernanceObject(parent_hash);
    BOOST_REQUIRE(stored != nullptr);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 0);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_VALID), 0);
}

BOOST_AUTO_TEST_CASE(invalid_signature_does_not_suppress_valid_vote)
{
    auto& govman = *m_node.govman;
    const CGovernanceObject proposal{MakeProposal(uint256::ONE)};
    const uint256 parent_hash{proposal.GetHash()};
    govman.AddGovernanceObjectForTesting(proposal);
    auto stored = govman.FindConstGovernanceObject(parent_hash);
    BOOST_REQUIRE(stored != nullptr);

    CKey attacker_key;
    attacker_key.MakeNewKey(true);
    CGovernanceVote forged{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES)};
    SignWithVotingKey(forged, attacker_key);

    CGovernanceException exception;
    uint256 hash_to_request;
    BOOST_CHECK(!govman.ProcessVote(forged, exception, hash_to_request));
    BOOST_CHECK_EQUAL(exception.GetNodePenalty(), 20);
    vote_rec_t vote_record;
    BOOST_CHECK(!stored->GetCurrentMNVotes(mn_collateral, vote_record));

    CGovernanceVote legitimate{forged};
    SignWithVotingKey(legitimate, mn_voting_key);
    BOOST_CHECK_EQUAL(forged.GetHash(), legitimate.GetHash());
    BOOST_CHECK(govman.ProcessVote(legitimate, exception, hash_to_request));
    BOOST_CHECK(govman.HaveVoteForHash(legitimate.GetHash()));
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);

    CBLSSecretKey attacker_operator_key;
    attacker_operator_key.MakeNewKey();
    CGovernanceVote forged_bls{MakeVote(parent_hash, VOTE_SIGNAL_VALID, VOTE_OUTCOME_YES)};
    SignWithOperatorKey(forged_bls, attacker_operator_key);
    BOOST_CHECK(!govman.ProcessVote(forged_bls, exception, hash_to_request));
    BOOST_CHECK_EQUAL(exception.GetNodePenalty(), 20);
    BOOST_REQUIRE(stored->GetCurrentMNVotes(mn_collateral, vote_record));
    BOOST_CHECK(!vote_record.mapInstances.contains(VOTE_SIGNAL_VALID));

    CGovernanceVote legitimate_bls{forged_bls};
    SignWithOperatorKey(legitimate_bls, mn_operator_key);
    BOOST_CHECK_EQUAL(forged_bls.GetHash(), legitimate_bls.GetHash());
    BOOST_CHECK(govman.ProcessVote(legitimate_bls, exception, hash_to_request));
    BOOST_CHECK(govman.HaveVoteForHash(legitimate_bls.GetHash()));
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_VALID), 1);
}

// Funding a proposal is the one decision reserved for the voting key; every other signal may be
// signed by the operator (BLS) key. Mixing the two up must not be accepted either way around.
//
// Rate checks are live on this path (unlike the orphan replay above, which disables them), so each
// vote below has to be the first *accepted* one for its (masternode, signal) pair: a rejected vote
// leaves no vote instance behind, but a second accepted vote for the same signal would be turned
// away by the 1h GOVERNANCE_UPDATE_MIN limit instead of the rule under test.
BOOST_AUTO_TEST_CASE(proposal_funding_votes_require_the_voting_key)
{
    auto& govman = *m_node.govman;

    const CGovernanceObject proposal{MakeProposal(uint256::ONE)};
    const uint256 parent_hash{proposal.GetHash()};
    govman.AddGovernanceObjectForTesting(proposal);
    auto stored = govman.FindConstGovernanceObject(parent_hash);
    BOOST_REQUIRE(stored != nullptr);

    // Operator key on a funding vote: valid BLS signature, wrong key for this signal.
    CGovernanceVote funding_by_operator{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES)};
    SignWithOperatorKey(funding_by_operator, mn_operator_key);
    BOOST_REQUIRE(funding_by_operator.IsValid(tip_mn_list(), /*useVotingKey=*/false));

    CGovernanceException exception;
    uint256 hash_to_request;
    BOOST_CHECK(!govman.ProcessVote(funding_by_operator, exception, hash_to_request));
    BOOST_CHECK_EQUAL(exception.GetType(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
    BOOST_CHECK_EQUAL(exception.GetNodePenalty(), 20);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 0);

    // Voting key on a funding vote: accepted and counted. Dated a second later so it is a vote in
    // its own right rather than the rejected one above with a different signature glued on.
    CGovernanceVote funding_by_voting_key{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES, 1s)};
    SignWithVotingKey(funding_by_voting_key, mn_voting_key);

    CGovernanceException funding_exception;
    BOOST_CHECK(govman.ProcessVote(funding_by_voting_key, funding_exception, hash_to_request));
    BOOST_CHECK(govman.HaveVoteForHash(funding_by_voting_key.GetHash()));
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);

    // Operator key on a non-funding signal: accepted and counted.
    CGovernanceVote valid_by_operator{MakeVote(parent_hash, VOTE_SIGNAL_VALID, VOTE_OUTCOME_YES)};
    SignWithOperatorKey(valid_by_operator, mn_operator_key);

    CGovernanceException valid_exception;
    BOOST_CHECK(govman.ProcessVote(valid_by_operator, valid_exception, hash_to_request));
    BOOST_CHECK(govman.HaveVoteForHash(valid_by_operator.GetHash()));
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_VALID), 1);

    // A known good vote received twice is dropped quietly, not punished.
    CGovernanceException duplicate_exception;
    BOOST_CHECK(!govman.ProcessVote(funding_by_voting_key, duplicate_exception, hash_to_request));
    BOOST_CHECK_EQUAL(duplicate_exception.GetNodePenalty(), 0);
    BOOST_CHECK_EQUAL(stored->GetAbsoluteYesCount(tip_mn_list(), VOTE_SIGNAL_FUNDING), 1);
}

BOOST_AUTO_TEST_CASE(legacy_governance_cache_is_discarded)
{
    const uint256 parent_hash{MakeProposal(uint256::ONE).GetHash()};
    CKey attacker_key;
    attacker_key.MakeNewKey(true);

    CGovernanceVote forged{MakeVote(parent_hash, VOTE_SIGNAL_FUNDING, VOTE_OUTCOME_YES)};
    SignWithVotingKey(forged, attacker_key);

    CacheMap<uint256, CGovernanceVote> legacy_invalid_votes{3};
    legacy_invalid_votes.Insert(forged.GetHash(), forged);
    const auto proposal = std::make_shared<CGovernanceObject>(MakeProposal(uint256::ONE));
    const std::map<uint256, std::shared_ptr<CGovernanceObject>> objects{{proposal->GetHash(), proposal}};

    // A version 16 cache, including its invalid-vote field, is dropped as a whole.
    CDataStream legacy{SER_DISK, CLIENT_VERSION};
    legacy << std::string{"CGovernanceManager-Version-16"}
           << std::map<uint256, int64_t>{}
           << legacy_invalid_votes
           << CacheMultiMap<uint256, governance::OrphanVote>{3}
           << objects
           << std::map<COutPoint, TestGovernanceStore::last_object_rec>{}
           << CDeterministicMNList{};

    TestGovernanceStore legacy_store;
    legacy_store.Unserialize(legacy);
    BOOST_CHECK_EQUAL(legacy_store.ObjectCount(), 0U);

    // The current format has no invalid-vote field and round-trips.
    CDataStream current{SER_DISK, CLIENT_VERSION};
    current << std::string{"CGovernanceManager-Version-17"}
            << std::map<uint256, int64_t>{}
            << CacheMultiMap<uint256, governance::OrphanVote>{3}
            << objects
            << std::map<COutPoint, TestGovernanceStore::last_object_rec>{}
            << CDeterministicMNList{};

    TestGovernanceStore store;
    store.Unserialize(current);
    BOOST_CHECK_EQUAL(store.ObjectCount(), 1U);

    CDataStream saved{SER_DISK, CLIENT_VERSION};
    store.Serialize(saved);
    TestGovernanceStore reloaded;
    reloaded.Unserialize(saved);
    BOOST_CHECK_EQUAL(reloaded.ObjectCount(), 1U);
    BOOST_CHECK(saved.empty());
}

// MasternodeRateCheck allows a masternode that has no rate-buffer entry yet, and only accepted
// triggers used to create one. A key whose triggers all fail AddTrigger was therefore never
// throttled: every one cost a BLS verification and a mapErasedGovernanceObjects entry.
BOOST_AUTO_TEST_CASE(failed_triggers_advance_the_masternode_rate_limit)
{
    const int64_t now{GetTime()};
    constexpr int attempts{12};
    int stored{0};
    for (int i = 0; i < attempts; ++i) {
        CGovernanceObject govobj{MakeFailedTrigger(mn_collateral, mn_operator_key, now + i, /*salt=*/i)};
        const uint256 hash{govobj.GetHash()};
        // Rate rejection is not misbehaviour, so ProcessObject reports success either way.
        WITH_LOCK(::cs_main, BOOST_CHECK(m_node.govman->ProcessObject(/*peer_str=*/"test-peer", hash, govobj)));
        if (m_node.govman->HaveObjectForHash(hash)) {
            ++stored;
            BOOST_CHECK(Assert(m_node.govman->FindGovernanceObject(hash))->IsSetCachedDelete());
        }
    }

    // The rate is only computed over a full buffer, so the attempt that would fill it is the
    // first one rated, and these timestamps are far too close together to pass.
    BOOST_CHECK_EQUAL(stored, RATE_BUFFER_SIZE - 1);
    BOOST_CHECK(m_node.govman->FetchRelayInventory().empty());
}

// Rate accounting now runs before AddTrigger, but scheduling the deferred re-announcement of a
// trigger created close to the future-deviation limit must not: a trigger that AddTrigger rejected
// would be announced, and served on GETDATA, once it aged past RELIABLE_PROPAGATION_TIME.
BOOST_AUTO_TEST_CASE(failed_trigger_is_not_scheduled_for_deferred_relay)
{
    const int64_t now{GetTime()};
    // Inside (now + MAX_TIME_FUTURE_DEVIATION - RELIABLE_PROPAGATION_TIME, now + MAX_TIME_FUTURE_DEVIATION],
    // i.e. 1h - 60s < offset <= 1h: accepted by MasternodeRateCheck, too new to propagate reliably.
    CGovernanceObject govobj{MakeFailedTrigger(mn_collateral, mn_operator_key, now + 3550, /*salt=*/0)};
    const uint256 hash{govobj.GetHash()};
    WITH_LOCK(::cs_main, BOOST_CHECK(m_node.govman->ProcessObject(/*peer_str=*/"test-peer", hash, govobj)));
    BOOST_REQUIRE(m_node.govman->HaveObjectForHash(hash));
    BOOST_CHECK(!m_node.govman->HaveSyncableObjectForHash(hash));
    BOOST_CHECK(m_node.govman->FetchRelayInventory().empty());

    // Age it past RELIABLE_PROPAGATION_TIME and run the deferred-relay pass.
    SetMockTime(std::chrono::seconds{now + 120});
    m_node.govman->UpdatedBlockTip(WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip()));
    BOOST_CHECK(m_node.govman->FetchRelayInventory().empty());
}

BOOST_AUTO_TEST_SUITE_END()
