// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <governance/vote.h>
#include <governance/votedb.h>
#include <key.h>
#include <netaddress.h>
#include <netfulfilledman.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>

namespace {
//! A vote carrying a valid BLS operator signature over its own signed content.
CGovernanceVote MakeSignedVote(const CBLSSecretKey& sk, const COutPoint& outpoint, const uint256& parent,
                               vote_signal_enum_t signal, int64_t time)
{
    CGovernanceVote vote{outpoint, parent, signal, VOTE_OUTCOME_YES};
    vote.SetTime(time);
    vote.SetSignature(sk.Sign(vote.GetSignatureHash(), /*legacy_scheme=*/false).ToByteVector(/*legacy=*/false));
    return vote;
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(governance_vote_sync_tests, BasicTestingSetup)

// CNetFulfilledRequestManager must stay keyed on the full CService, port included.
//
// It is tempting to strip the port: peer.addr carries the remote ephemeral source
// port for inbound peers, so the govsync quota is per-connection and an attacker
// resets it by reconnecting. But this map is shared with SyncManager's per-peer
// outbound bookkeeping ("full-sync", "governance-sync", "spork-sync",
// "mempool-sync") and the govsync quota is coupled to PeerMisbehaving(20).
// Several honest nodes legitimately share one address (NAT, colocation, CI), and
// collapsing them onto one key makes the second honest peer look like a repeat
// offender -- the ban score escalates to discouragement, and mnsync breaks
// outright (feature_governance.py, where every node is on 127.0.0.1).
//
// This test pins port-sensitivity as the intended behaviour. Bounding the cost
// of a single vote-sync request is a separate problem from this quota.
BOOST_AUTO_TEST_CASE(fulfilled_request_manager_is_port_sensitive)
{
    in_addr ipv4{};
    ipv4.s_addr = htonl(0x0a000001); // 10.0.0.1
    const CService peer_a{CNetAddr{ipv4}, /*port=*/40000};
    const CService peer_b{CNetAddr{ipv4}, /*port=*/40001};

    CNetFulfilledRequestManager fulfilled;
    BOOST_REQUIRE(fulfilled.LoadCache(/*load_cache=*/false));

    const std::string request{"mngovernancesync-votes-deadbeef"};
    BOOST_CHECK(!fulfilled.HasFulfilledRequest(peer_a, request));
    fulfilled.AddFulfilledRequest(peer_a, request);
    BOOST_CHECK(fulfilled.HasFulfilledRequest(peer_a, request));

    // A different peer on the same host must NOT inherit peer_a's quota, or it
    // would be misbehaviour-scored for its own first request.
    BOOST_CHECK_MESSAGE(!fulfilled.HasFulfilledRequest(peer_b, request),
                        "distinct peers sharing an address must not share a fulfilled-request quota");

    // Per-peer cleanup must likewise not disturb the other peer's entries.
    fulfilled.AddFulfilledRequest(peer_b, request);
    fulfilled.RemoveAllFulfilledRequests(peer_b);
    BOOST_CHECK(!fulfilled.HasFulfilledRequest(peer_b, request));
    BOOST_CHECK(fulfilled.HasFulfilledRequest(peer_a, request));
}

// Every vote-sync request walks the stored votes and calls IsValid/CheckSignature
// on each one, so an unchanged vote checked against the same key must reuse its
// memoised verdict instead of paying for another BLS pairing or ECDSA recovery.
BOOST_AUTO_TEST_CASE(vote_signature_verification_is_cached)
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    const CBLSPublicKey pk{sk.GetPublicKey()};

    CGovernanceVote vote{MakeSignedVote(sk, COutPoint{uint256S("01"), /*n=*/0}, uint256S("02"), VOTE_SIGNAL_VALID,
                                        1'700'000'000)};

    // Nothing memoised yet, so the first check has to run the pairing.
    BOOST_CHECK(!vote.GetMemoisedVerdict(pk).has_value());

    BOOST_REQUIRE(vote.CheckSignature(pk));
    BOOST_CHECK(vote.GetMemoisedVerdict(pk) == true);

    // A different operator key is a different question: it must not be answered
    // from the memo, and checking it takes the single slot.
    CBLSSecretKey sk_other;
    sk_other.MakeNewKey();
    const CBLSPublicKey pk_other{sk_other.GetPublicKey()};
    BOOST_CHECK(!vote.GetMemoisedVerdict(pk_other).has_value());
    BOOST_CHECK(!vote.CheckSignature(pk_other));
    BOOST_CHECK(vote.GetMemoisedVerdict(pk_other) == false);
    BOOST_CHECK(!vote.GetMemoisedVerdict(pk).has_value());

    // Re-checking the original key repopulates it.
    BOOST_REQUIRE(vote.CheckSignature(pk));
    BOOST_CHECK(vote.GetMemoisedVerdict(pk) == true);
}

// The cache must live on the vote instances stored in CGovernanceObjectVoteFile,
// not on ephemeral GetVotes() copies, so repeated sync walks stay cheap.
BOOST_AUTO_TEST_CASE(stored_vote_file_reuses_signature_cache)
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    const CBLSPublicKey pk{sk.GetPublicKey()};

    CGovernanceVote vote{MakeSignedVote(sk, COutPoint{uint256S("11"), /*n=*/1}, uint256S("22"), VOTE_SIGNAL_DELETE,
                                        1'700'000'100)};

    CGovernanceObjectVoteFile file;
    // Pre-check so AddVote stores a vote whose cache is already warm — the same
    // sequence ProcessVote uses (IsValid then AddVote).
    BOOST_REQUIRE(vote.CheckSignature(pk));
    file.AddVote(vote);
    BOOST_REQUIRE_EQUAL(file.GetVoteCount(), 1);

    // The stored instance must carry the warm memo, so a sync walk answers from
    // it rather than re-running the pairing for every vote it visits.
    file.ForEachVote([&](const CGovernanceVote& stored) {
        BOOST_CHECK_MESSAGE(stored.GetMemoisedVerdict(pk) == true,
                            "the stored vote lost the memo warmed before AddVote");
        BOOST_REQUIRE(stored.CheckSignature(pk));
        BOOST_CHECK(stored.GetMemoisedVerdict(pk) == true);
    });
}

// The memo must never be keyed on the verification key alone. nTime is part of
// the signed payload (GetSignatureString/SerializeHash) but is mutable via
// SetTime(), which governance.cpp and object.cpp call on reconstructed votes
// before verifying them. A key-only memo would return the stale "valid" verdict
// for content that was never signed -- a signature-check bypass.
BOOST_AUTO_TEST_CASE(signature_cache_invalidated_by_signed_field_mutation)
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    const CBLSPublicKey pk{sk.GetPublicKey()};

    CGovernanceVote vote{MakeSignedVote(sk, COutPoint{uint256S("33"), /*n=*/2}, uint256S("44"), VOTE_SIGNAL_FUNDING,
                                        1'700'000'200)};

    BOOST_REQUIRE(vote.CheckSignature(pk));

    // Mutate a signed field. The signature no longer covers this content, so the
    // very next check with the *same* key must re-verify and reject.
    vote.SetTime(1'700'009'999);
    BOOST_CHECK_MESSAGE(!vote.GetMemoisedVerdict(pk).has_value(),
                        "mutating a signed field must leave no usable memo");
    BOOST_CHECK_MESSAGE(!vote.CheckSignature(pk),
                        "mutating a signed field must invalidate the signature memo");

    // Restoring the original signed content makes the vote valid again.
    vote.SetTime(1'700'000'200);
    BOOST_CHECK(vote.CheckSignature(pk));
}

// The two CheckSignature overloads (CKeyID / CBLSPublicKey) share one memo slot.
// A verdict produced by the BLS overload must never be served to the ECDSA
// overload or vice versa: the key types serialise to different lengths and
// contents, so the derived cache keys must differ. If they ever collided, a
// BLS-valid vote would be reported valid for an unrelated voting key -- a
// signature-check bypass across key types.
BOOST_AUTO_TEST_CASE(signature_cache_does_not_confuse_key_types)
{
    CBLSSecretKey sk;
    sk.MakeNewKey();
    const CBLSPublicKey pk{sk.GetPublicKey()};

    CGovernanceVote vote{MakeSignedVote(sk, COutPoint{uint256S("55"), /*n=*/3}, uint256S("66"), VOTE_SIGNAL_VALID,
                                        1'700'000'300)};

    // Warm the memo with a *valid* BLS verdict.
    BOOST_REQUIRE(vote.CheckSignature(pk));

    // Now ask the ECDSA overload. The BLS signature is not a valid compact ECDSA
    // signature for any CKeyID, so this must re-verify and reject rather than
    // inherit the cached "valid".
    CKey ecdsa_key;
    ecdsa_key.MakeNewKey(/*fCompressedIn=*/true);
    const CKeyID keyid{ecdsa_key.GetPubKey().GetID()};

    BOOST_CHECK_MESSAGE(!vote.GetMemoisedVerdict(keyid).has_value(),
                        "a BLS verdict must not be visible to the CKeyID overload");
    BOOST_CHECK_MESSAGE(!vote.CheckSignature(keyid),
                        "a BLS verdict must not be reused by the CKeyID overload");

    // And the BLS verdict must still be reachable afterwards (recomputed, not corrupted).
    BOOST_CHECK(vote.CheckSignature(pk));
}

BOOST_AUTO_TEST_SUITE_END()
