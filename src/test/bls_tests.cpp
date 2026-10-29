// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <bls/bls_batchverifier.h>
#include <bls/bls_worker.h>
#include <clientversion.h>
#include <crypto/common.h>
#include <random.h>
#include <streams.h>
#include <util/irange.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bls_tests)

void FuncSign(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSSecretKey sk1, sk2;
    sk1.MakeNewKey();
    sk2.MakeNewKey();

    uint256 msgHash1 = uint256::ONE;
    uint256 msgHash2 = uint256::TWO;

    auto sig1 = sk1.Sign(msgHash1, legacy_scheme);
    auto sig2 = sk2.Sign(msgHash1, legacy_scheme);
    BOOST_CHECK(sig1.VerifyInsecure(sk1.GetPublicKey(), msgHash1));
    BOOST_CHECK(!sig1.VerifyInsecure(sk1.GetPublicKey(), msgHash2));
    BOOST_CHECK(!sig2.VerifyInsecure(sk1.GetPublicKey(), msgHash1));
    BOOST_CHECK(!sig2.VerifyInsecure(sk2.GetPublicKey(), msgHash2));
    BOOST_CHECK(sig2.VerifyInsecure(sk2.GetPublicKey(), msgHash1));

    return;
}

void FuncSerialize(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSSecretKey sk;
    CDataStream ds2(SER_DISK, CLIENT_VERSION), ds3(SER_DISK, CLIENT_VERSION);
    uint256 msgHash = uint256::ONE;

    sk.MakeNewKey();
    CBLSSignature sig1 = sk.Sign(msgHash, legacy_scheme);
    ds2 << sig1;
    ds3 << CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(sig1), !legacy_scheme);

    CBLSSignature sig2;
    ds2 >> sig2;
    BOOST_CHECK(sig1 == sig2);

    CBLSSignature sig3;
    ds3 >> CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(sig3), !legacy_scheme);
    BOOST_CHECK(sig1 == sig3);

    return;
}

void FuncSetHexStr(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    // Note: 2nd bool argument for SetHexStr for bls::PrivateKey has a meaning modOrder, not is-legacy
    CBLSSecretKey sk;
    std::string strValidSecret = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    // Note: invalid string passed to SetHexStr() should cause it to fail and reset key internal data
    BOOST_CHECK(sk.SetHexStr(strValidSecret, false));
    BOOST_CHECK(!sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1g", false)); // non-hex
    BOOST_CHECK(!sk.IsValid());
    BOOST_CHECK(sk == CBLSSecretKey());
    // Try few more invalid strings
    BOOST_CHECK(sk.SetHexStr(strValidSecret, false));
    BOOST_CHECK(!sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e", false)); // hex but too short
    BOOST_CHECK(!sk.IsValid());
    BOOST_CHECK(sk.SetHexStr(strValidSecret, false));
    BOOST_CHECK(
        !sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", false)); // hex but too long
    BOOST_CHECK(!sk.IsValid());

    return;
}

void FuncKeyAgg(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSSecretKey sk1, sk2;
    sk1.MakeNewKey();
    sk2.MakeNewKey();

    CBLSPublicKey ag_pk = sk1.GetPublicKey();
    ag_pk.AggregateInsecure(sk2.GetPublicKey());

    CBLSSecretKey ag_sk = sk1;
    ag_sk.AggregateInsecure(sk2);

    BOOST_CHECK(ag_pk == ag_sk.GetPublicKey());

    uint256 msgHash1 = uint256::ONE;
    uint256 msgHash2 = uint256::TWO;

    auto sig = ag_sk.Sign(msgHash1, legacy_scheme);
    BOOST_CHECK(sig.VerifyInsecure(ag_pk, msgHash1));
    BOOST_CHECK(!sig.VerifyInsecure(ag_pk, msgHash2));
}

void FuncKeyAggVec(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    std::vector<CBLSSecretKey> vec_sk;
    std::vector<CBLSPublicKey> vec_pk;

    {
        auto ret = CBLSSecretKey::AggregateInsecure(vec_sk);
        BOOST_CHECK(ret == CBLSSecretKey());
    }
    {
        auto ret = CBLSPublicKey::AggregateInsecure(vec_pk);
        BOOST_CHECK(ret == CBLSPublicKey());
    }

    // In practice, we only aggregate 400 key shares at any given time, something substantially larger than that should
    // be good. Plus this is very very fast, so who cares!
    int key_count = 10000;
    vec_sk.reserve(key_count);
    vec_pk.reserve(key_count);
    CBLSSecretKey sk;
    for (int i = 0; i < key_count; i++) {
        sk.MakeNewKey();
        vec_sk.push_back(sk);
        vec_pk.push_back(sk.GetPublicKey());
    }

    CBLSSecretKey ag_sk = CBLSSecretKey::AggregateInsecure(vec_sk);
    CBLSPublicKey ag_pk = CBLSPublicKey::AggregateInsecure(vec_pk);

    BOOST_CHECK(ag_sk.IsValid());
    BOOST_CHECK(ag_pk.IsValid());

    uint256 msgHash1 = uint256::ONE;
    uint256 msgHash2 = uint256::TWO;

    auto sig = ag_sk.Sign(msgHash1, legacy_scheme);
    BOOST_CHECK(sig.VerifyInsecure(ag_pk, msgHash1));
    BOOST_CHECK(!sig.VerifyInsecure(ag_pk, msgHash2));
}


void FuncSigAggSub(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    int count = 20;
    std::vector<CBLSPublicKey> vec_pks;
    std::vector<uint256> vec_hashes;
    std::vector<CBLSSignature> vec_sigs;
    vec_pks.reserve(count);
    vec_hashes.reserve(count);
    vec_sigs.reserve(count);

    CBLSSignature sig;
    auto sk = CBLSSecretKey();
    uint256 hash;
    for (int i = 0; i < count; i++) {
        sk.MakeNewKey();
        vec_pks.push_back(sk.GetPublicKey());
        hash = GetRandHash();
        vec_hashes.push_back(hash);
        CBLSSignature sig_i = sk.Sign(hash, legacy_scheme);
        vec_sigs.push_back(sig_i);
        if (i == 0) {
            // first sig is assigned directly
            sig = sig_i;
        } else {
            // all other sigs are aggregated into the previously computed/stored sig
            sig.AggregateInsecure(sig_i);
        }
        BOOST_CHECK(sig.VerifyInsecureAggregated(vec_pks, vec_hashes));
    }
    // Create an aggregated signature from the vector of individual signatures
    auto vecSig = CBLSSignature::AggregateInsecure(vec_sigs);
    BOOST_CHECK(vecSig.VerifyInsecureAggregated(vec_pks, vec_hashes));
    // Check that these two signatures are equal
    BOOST_CHECK(sig == vecSig);

    // Test that the sig continues to be valid when subtracting sigs via `SubInsecure`

    for (int i = 0; i < count - 1; i++) {
        auto top_sig = vec_sigs.back();
        vec_pks.pop_back();
        vec_hashes.pop_back();
        BOOST_CHECK(!sig.VerifyInsecureAggregated(vec_pks, vec_hashes));
        sig.SubInsecure(top_sig);
        BOOST_CHECK(sig.VerifyInsecureAggregated(vec_pks, vec_hashes));
        vec_sigs.pop_back();
    }
    // Check that the final left-over sig validates
    BOOST_CHECK_EQUAL(vec_sigs.size(), 1);
    BOOST_CHECK_EQUAL(vec_pks.size(), 1);
    BOOST_CHECK_EQUAL(vec_hashes.size(), 1);
    BOOST_CHECK(vec_sigs[0].VerifyInsecure(vec_pks[0], vec_hashes[0]));
}


void FuncSigAggSecure(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    int count = 10;

    uint256 hash = GetRandHash();

    std::vector<CBLSSignature> vec_sigs;
    std::vector<CBLSPublicKey> vec_pks;

    CBLSSecretKey sk;
    for (int i = 0; i < count; i++) {
        sk.MakeNewKey();
        vec_pks.push_back(sk.GetPublicKey());
        vec_sigs.push_back(sk.Sign(hash, legacy_scheme));
    }

    auto sec_agg_sig = CBLSSignature::AggregateSecure(vec_sigs, vec_pks, hash);
    BOOST_CHECK(sec_agg_sig.IsValid());
    BOOST_CHECK(sec_agg_sig.VerifySecureAggregated(vec_pks, hash));
}

void FuncDHExchange(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSSecretKey sk1, sk2;
    sk1.MakeNewKey();
    sk2.MakeNewKey();

    CBLSPublicKey pk1, pk2;
    pk1 = sk1.GetPublicKey();
    pk2 = sk2.GetPublicKey();

    // Perform diffie-helman exchange
    CBLSPublicKey pke1, pke2;
    pke1.DHKeyExchange(sk1, pk2);
    pke2.DHKeyExchange(sk2, pk1);

    BOOST_CHECK(pke1.IsValid());
    BOOST_CHECK(pke2.IsValid());
    BOOST_CHECK(pke1 == pke2);
}

struct Message
{
    uint32_t sourceId;
    uint32_t msgId;
    uint256 msgHash;
    CBLSSecretKey sk;
    CBLSPublicKey pk;
    CBLSSignature sig;
    bool valid;
};

static void AddMessage(std::vector<Message>& vec, uint32_t sourceId, uint32_t msgId, uint8_t msgHash, bool valid)
{
    bool legacy_scheme = bls::bls_legacy_scheme.load();
    Message m;
    m.sourceId = sourceId;
    m.msgId = msgId;
    m.msgHash = uint256(msgHash);
    m.sk.MakeNewKey();
    m.pk = m.sk.GetPublicKey();
    m.sig = m.sk.Sign(m.msgHash, legacy_scheme);
    m.valid = valid;

    if (!valid) {
        CBLSSecretKey tmp;
        tmp.MakeNewKey();
        m.sig = tmp.Sign(m.msgHash, legacy_scheme);
    }

    vec.emplace_back(m);
}

static void Verify(std::vector<Message>& vec, bool secureVerification, bool perMessageFallback)
{
    CBLSBatchVerifier<uint32_t, uint32_t> batchVerifier(secureVerification, perMessageFallback);

    std::set<uint32_t> expectedBadMessages;
    std::set<uint32_t> expectedBadSources;
    for (auto& m : vec) {
        if (!m.valid) {
            expectedBadMessages.emplace(m.msgId);
            expectedBadSources.emplace(m.sourceId);
        }

        batchVerifier.PushMessage(m.sourceId, m.msgId, m.msgHash, m.sig, m.pk);
    }

    batchVerifier.Verify();

    BOOST_CHECK(batchVerifier.badSources == expectedBadSources);

    if (perMessageFallback) {
        BOOST_CHECK(batchVerifier.badMessages == expectedBadMessages);
    } else {
        BOOST_CHECK(batchVerifier.badMessages.empty());
    }
}

static void Verify(std::vector<Message>& vec)
{
    Verify(vec, false, false);
    Verify(vec, true, false);
    Verify(vec, false, true);
    Verify(vec, true, true);
}

void FuncBatchVerifier(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    std::vector<Message> msgs;

    // distinct messages from distinct sources
    AddMessage(msgs, 1, 1, 1, true);
    AddMessage(msgs, 2, 2, 2, true);
    AddMessage(msgs, 3, 3, 3, true);
    Verify(msgs);

    // distinct messages from same source
    AddMessage(msgs, 4, 4, 4, true);
    AddMessage(msgs, 4, 5, 5, true);
    AddMessage(msgs, 4, 6, 6, true);
    Verify(msgs);

    // invalid sig
    AddMessage(msgs, 7, 7, 7, false);
    Verify(msgs);

    // same message as before, but from another source and with valid sig
    AddMessage(msgs, 8, 8, 7, true);
    Verify(msgs);

    // same message as before, but from another source and signed with another key
    AddMessage(msgs, 9, 9, 7, true);
    Verify(msgs);

    msgs.clear();
    // same message, signed by multiple keys
    AddMessage(msgs, 1, 1, 1, true);
    AddMessage(msgs, 1, 2, 1, true);
    AddMessage(msgs, 1, 3, 1, true);
    AddMessage(msgs, 2, 4, 1, true);
    AddMessage(msgs, 2, 5, 1, true);
    AddMessage(msgs, 2, 6, 1, true);
    Verify(msgs);

    // last message invalid from one source
    AddMessage(msgs, 1, 7, 1, false);
    Verify(msgs);
}

void FuncThresholdSignature(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    [[maybe_unused]] const uint256 hash = GetRandHash();

    constexpr size_t m_size = 20;
    constexpr size_t m_threshold = 15;

    std::vector<CBLSSecretKey> v_threshold_sks;
    std::vector<CBLSPublicKey> v_threshold_pks;
    for ([[maybe_unused]] const auto i : irange::range(m_threshold)) {
        CBLSSecretKey sk;
        sk.MakeNewKey();
        v_threshold_sks.push_back(sk);
        v_threshold_pks.emplace_back(sk.GetPublicKey());
    }

    CBLSSecretKey thr_sk = v_threshold_sks[0];
    CBLSPublicKey thr_pk = v_threshold_pks[0];
    CBLSSignature thr_sig = thr_sk.Sign(hash, legacy_scheme);

    std::vector<CBLSId> v_size_ids;
    std::vector<CBLSSecretKey> v_size_sk_shares;
    std::vector<CBLSPublicKey> v_size_pk_shares;
    for ([[maybe_unused]] const auto m_shares : irange::range(m_size)) {
        v_size_ids.emplace_back(GetRandHash());
        CBLSSecretKey sk;
        BOOST_CHECK(sk.SecretKeyShare(v_threshold_sks, v_size_ids[m_shares]));
        v_size_sk_shares.push_back(sk);
        CBLSPublicKey pk;
        BOOST_CHECK(pk.PublicKeyShare(v_threshold_pks, v_size_ids[m_shares]));
        v_size_pk_shares.push_back(pk);

        std::vector<CBLSSignature> v_share_sigs;
        std::vector<CBLSId> v_share_ids;
        for ([[maybe_unused]] const auto j : irange::range(m_shares)) {
            v_share_sigs.emplace_back(v_size_sk_shares[j].Sign(hash, legacy_scheme));
            BOOST_CHECK(v_share_sigs.back().VerifyInsecure(v_size_pk_shares[j], hash));
            v_share_ids.push_back(v_size_ids[j]);
        }

        CBLSSignature rec_share_sig;
        BOOST_CHECK_EQUAL(rec_share_sig.Recover(v_share_sigs, v_share_ids), m_shares >= 2);
        BOOST_CHECK_EQUAL(rec_share_sig.IsValid(), m_shares >= 2);
        BOOST_CHECK_EQUAL(rec_share_sig == thr_sig, m_shares >= m_threshold);
        BOOST_CHECK_EQUAL(rec_share_sig.VerifyInsecure(thr_pk, hash), m_shares >= m_threshold);
    }
}

BOOST_AUTO_TEST_CASE(bls_sethexstr_tests)
{
    FuncSetHexStr(true);
    FuncSetHexStr(false);
}

BOOST_AUTO_TEST_CASE(bls_serialize_tests)
{
    FuncSerialize(true);
    FuncSerialize(false);
}

BOOST_AUTO_TEST_CASE(bls_sig_tests)
{
    FuncSign(true);
    FuncSign(false);
}

BOOST_AUTO_TEST_CASE(bls_key_agg_tests)
{
    FuncKeyAgg(true);
    FuncKeyAgg(false);
}

BOOST_AUTO_TEST_CASE(bls_key_agg_vec_tests)
{
    FuncKeyAggVec(true);
    FuncKeyAggVec(false);
}

BOOST_AUTO_TEST_CASE(bls_sig_agg_sub_tests)
{
    FuncSigAggSub(true);
    FuncSigAggSub(false);
}

BOOST_AUTO_TEST_CASE(bls_sig_agg_secure_tests)
{
    FuncSigAggSecure(true);
    FuncSigAggSecure(false);
}

BOOST_AUTO_TEST_CASE(bls_dh_exchange_tests)
{
    FuncDHExchange(true);
    FuncDHExchange(false);
}

BOOST_AUTO_TEST_CASE(batch_verifier_tests)
{
    FuncBatchVerifier(true);
    FuncBatchVerifier(false);
}

BOOST_AUTO_TEST_CASE(bls_threshold_signature_tests)
{
    FuncThresholdSignature(true);
    FuncThresholdSignature(false);
}

// Regression test: a DKG justification path can reach
// CBLSWorker::AsyncVerifyContributionShare() with a null verification vector when
// the corresponding member's contribution was never received (e.g. on a non-member
// observer). The singular overload dereferenced *vvec with no null check and aborted
// the process; it must instead treat a missing vvec as a failed verification, like
// the plural VerifyVerificationVectors() path. A *valid* id is required so the guard
// does not short-circuit before reaching the (previously) unguarded dereference.
void FuncVerifyContributionShareNullVvec(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSWorker worker;
    worker.Start();

    const CBLSId id{uint256::ONE};
    BOOST_REQUIRE(id.IsValid());

    CBLSSecretKey sk;
    sk.MakeNewKey();

    const BLSVerificationVectorPtr null_vvec; // default-constructed null shared_ptr
    BOOST_REQUIRE(null_vvec == nullptr);

    // Must return false (verification failed) instead of dereferencing the null vvec.
    BOOST_CHECK(worker.AsyncVerifyContributionShare(id, null_vvec, sk).get() == false);

    worker.Stop();
}

BOOST_AUTO_TEST_CASE(bls_verify_contribution_share_null_vvec_tests)
{
    FuncVerifyContributionShareNullVvec(true);
    FuncVerifyContributionShareNullVvec(false);
}

// Regression: an empty verification-vector set schedules no work at all, so
// before the guard nothing ever invoked doneCallback and .get() threw
// std::future_error(broken_promise). In production that throw escapes
// VerifyPendingContributions on the DKG phase-handler thread, which only catches
// AbortPhaseException, and std::terminate aborts dashd.
// Contract: return an empty result vector, on both the aggregated and the
// one-by-one path.
void FuncVerifyContributionSharesEmptyVvecs(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSWorker worker;
    worker.Start();

    const CBLSId id{uint256::ONE};
    BOOST_REQUIRE(id.IsValid());

    std::vector<BLSVerificationVectorPtr> vvecs; // empty
    std::vector<CBLSSecretKey> skShares;         // empty

    // Must return gracefully (empty result) instead of destroying the promise.
    const auto result = worker.VerifyContributionShares(id, vvecs, skShares);
    BOOST_CHECK(result.empty());

    const auto result_no_agg = worker.VerifyContributionShares(id, vvecs, skShares,
                                                               /*parallel=*/true,
                                                               /*aggregated=*/false);
    BOOST_CHECK(result_no_agg.empty());

    worker.Stop();
}

BOOST_AUTO_TEST_CASE(bls_verify_contribution_shares_empty_vvecs_tests)
{
    FuncVerifyContributionSharesEmptyVvecs(true);
    FuncVerifyContributionSharesEmptyVvecs(false);
}

// Positive control for the empty-input guard: it must not change which
// contributions are accepted. Accepting an invalid share would corrupt the
// recovered quorum key; rejecting an honest one would stall DKG. This property
// was previously only covered by src/bench/bls_dkg.cpp, which does not run under
// `make check`.
//
// QUORUM_SIZE exceeds CONTRIBUTION_VERIFY_BATCH_SIZE so the aggregated path
// builds more than one batch. Corrupting a share in the first batch must then
// fail that batch, fall back to per-contribution verification, blame exactly the
// corrupted index, and leave the second batch accepted wholesale - an invalid
// share must never be masked by the valid shares it is aggregated with.
void FuncVerifyContributionSharesHonest(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    constexpr size_t QUORUM_SIZE{CBLSWorker::CONTRIBUTION_VERIFY_BATCH_SIZE + 2};
    constexpr int THRESHOLD{6};

    CBLSWorker worker;
    worker.Start();

    std::vector<CBLSId> ids;
    ids.reserve(QUORUM_SIZE);
    for (const size_t i : irange::range(QUORUM_SIZE)) {
        uint256 id;
        WriteLE64(id.begin(), i + 1);
        ids.emplace_back(id);
    }

    // Every member runs GenerateContributions and broadcasts its vvec; member i's
    // share for us (member 0) is skShares[i][0].
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    vvecs.reserve(QUORUM_SIZE);
    skContributions.reserve(QUORUM_SIZE);
    for ([[maybe_unused]] const size_t i : irange::range(QUORUM_SIZE)) {
        BLSVerificationVectorPtr vvec;
        std::vector<CBLSSecretKey> skShares;
        BOOST_REQUIRE(worker.GenerateContributions(THRESHOLD, ids, vvec, skShares));
        BOOST_REQUIRE(vvec != nullptr);
        BOOST_REQUIRE_EQUAL(vvec->size(), size_t(THRESHOLD));
        BOOST_REQUIRE_EQUAL(skShares.size(), QUORUM_SIZE);
        vvecs.emplace_back(std::move(vvec));
        skContributions.emplace_back(skShares[0]); // the share addressed to ids[0]
    }

    const CBLSId& myId = ids[0];

    // 1. All-honest input must be accepted in full, on both code paths.
    for (const bool aggregated : {true, false}) {
        const auto result = worker.VerifyContributionShares(myId, vvecs, skContributions,
                                                            /*parallel=*/true, aggregated);
        BOOST_REQUIRE_EQUAL(result.size(), QUORUM_SIZE);
        for (const size_t i : irange::range(QUORUM_SIZE)) {
            BOOST_CHECK_MESSAGE(result[i], "honest contribution " << i << " rejected (aggregated="
                                                                  << aggregated << ")");
        }
    }

    // 2. Corrupt one share and expect exactly that index to be blamed, nobody else.
    //    With aggregated=true the corrupted index sits in the first batch, so that
    //    batch must fail and fall back to per-contribution verification while the
    //    second batch stays accepted wholesale. With aggregated=false there is a
    //    single batch verified one-by-one, so only the blame-exactness holds.
    constexpr size_t BAD_IDX{3};
    static_assert(BAD_IDX < CBLSWorker::CONTRIBUTION_VERIFY_BATCH_SIZE,
                  "corrupted index must land in the first batch");
    auto corrupted = skContributions;
    corrupted[BAD_IDX].MakeNewKey(); // valid key, wrong value -> pk mismatch

    for (const bool aggregated : {true, false}) {
        const auto result = worker.VerifyContributionShares(myId, vvecs, corrupted,
                                                            /*parallel=*/true, aggregated);
        BOOST_REQUIRE_EQUAL(result.size(), QUORUM_SIZE);
        for (const size_t i : irange::range(QUORUM_SIZE)) {
            const bool expected{i != BAD_IDX};
            BOOST_CHECK_MESSAGE(result[i] == expected,
                                "index " << i << " expected " << expected << " got " << result[i]
                                         << " (aggregated=" << aggregated << ")");
        }
    }

    worker.Stop();
}

BOOST_AUTO_TEST_CASE(bls_verify_contribution_shares_honest_tests)
{
    FuncVerifyContributionSharesHonest(true);
    FuncVerifyContributionSharesHonest(false);
}

// A duplicate coefficient point between two members' verification vectors must not fail the
// other members' contributions. CBLSWorker::VerifyContributionShares() batches the members'
// vvecs, and the batched VerifyVerificationVectors() check rejects a coefficient shared
// between two vvecs. Failing the whole batch on that let one member, by copying another
// member's coefficient into its own vvec, mark every honest member in the batch invalid.
// The fix verifies each contribution against its own vvec, so only the member whose share no
// longer matches its own vvec fails. Regression: on the old all-or-nothing code every entry
// came back false, so the honest members verifying true is what distinguishes fixed from broken.
void FuncVerifyContributionSharesDuplicateCoefficient(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSWorker worker;
    worker.Start();

    const int threshold = 2;
    std::vector<CBLSId> ids;
    for (int i = 0; i < 4; i++) {
        uint256 id;
        WriteLE64(id.begin(), i + 1);
        ids.emplace_back(id);
    }

    // Three dealers each generate a verification vector and their secret-key shares to each id.
    std::vector<BLSVerificationVectorPtr> vvecs(3);
    std::vector<std::vector<CBLSSecretKey>> skShares(3);
    for (int d = 0; d < 3; d++) {
        BOOST_REQUIRE(worker.GenerateContributions(threshold, ids, vvecs[d], skShares[d]));
    }

    // Receiver R collects each dealer's share to it.
    const size_t R = 0;
    const CBLSId& recvId = ids[R];
    std::vector<CBLSSecretKey> recvShares{skShares[0][R], skShares[1][R], skShares[2][R]};

    // All honest: every contribution verifies.
    std::vector<bool> ok = worker.VerifyContributionShares(recvId, vvecs, recvShares);
    BOOST_REQUIRE_EQUAL(ok.size(), 3U);
    BOOST_CHECK(ok[0] && ok[1] && ok[2]);

    // Dealer 0 copies dealer 1's leading coefficient into its own vvec (a cross-member
    // duplicate) without touching its share, making the batch vvec check fail.
    vvecs[0]->at(0) = vvecs[1]->at(0);
    std::vector<bool> res = worker.VerifyContributionShares(recvId, vvecs, recvShares);
    BOOST_REQUIRE_EQUAL(res.size(), 3U);
    BOOST_CHECK(!res[0]);  // the member that planted the duplicate is caught alone
    BOOST_CHECK(res[1]);   // the honest member whose coefficient was copied is unaffected
    BOOST_CHECK(res[2]);   // the other honest member is unaffected

    worker.Stop();
}

BOOST_AUTO_TEST_CASE(bls_verify_contribution_shares_duplicate_coefficient_tests)
{
    FuncVerifyContributionSharesDuplicateCoefficient(true);
    FuncVerifyContributionSharesDuplicateCoefficient(false);
}

// A dummy BLS object that satisfies the minimal interface expected by CBLSLazyWrapper.
class DummyBLS
{
public:
    // Define a fixed serialization size (for testing purposes).
    static const size_t SerSize = 4;
    std::array<uint8_t, SerSize> data{};

    DummyBLS() { data.fill(0); }

    // A dummy validity check: valid if any byte is non-zero.
    bool IsValid() const
    {
        return std::any_of(data.begin(), data.end(), [](uint8_t c) { return c != 0; });
    }

    // Convert to bytes; ignore the legacy flag for simplicity.
    std::array<uint8_t, SerSize> ToBytes(bool /*legacy*/) const { return data; }

    // Set from bytes; again, ignore the legacy flag.
    void SetBytes(const std::array<uint8_t, SerSize>& bytes, bool /*legacy*/) { data = bytes; }

    // A dummy malleability check: simply compares the stored data to the given bytes.
    bool CheckMalleable(const std::array<uint8_t, SerSize>& bytes, bool /*legacy*/) const { return data == bytes; }

    // Reset the object to an "empty" state.
    void Reset() { data.fill(0); }

    // Produce a string representation.
    std::string ToString(bool /*legacy*/) const { return HexStr(data); }

    // Equality operator.
    bool operator==(const DummyBLS& other) const { return data == other.data; }
};

// Define a type alias for our lazy wrapper instantiated with DummyBLS.
using LazyDummyBLS = CBLSLazyWrapper<DummyBLS>;

// Test 1: Two default (unset) wrappers should compare equal.
BOOST_AUTO_TEST_CASE(test_default_equality)
{
    LazyDummyBLS lazy1;
    LazyDummyBLS lazy2;
    // Neither instance has been set, so they represent the default/null object.
    BOOST_CHECK(lazy1 == lazy2);
}

// Test 2: A default wrapper and one initialized with a nonzero DummyBLS should compare unequal.
BOOST_AUTO_TEST_CASE(test_non_default_vs_default)
{
    LazyDummyBLS lazy_default;
    LazyDummyBLS lazy_set;
    DummyBLS obj;
    obj.data = {1, 0, 0, 0}; // nonzero data makes the object valid
    lazy_set.Set(obj, false);
    BOOST_CHECK(!(lazy_default == lazy_set));
    BOOST_CHECK(lazy_default != lazy_set);
}

// Test 2: A default wrapper and one initialized with a nonzero DummyBLS should compare unequal.
BOOST_AUTO_TEST_CASE(test_non_default_vs_different)
{
    LazyDummyBLS lazy_a;
    LazyDummyBLS lazy_b;
    DummyBLS obj;
    obj.data = {1, 2, 3, 4}; // nonzero data makes the object valid
    lazy_a.Set(obj, false);
    obj.data = {2, 2, 3, 4}; // nonzero data makes the object valid
    lazy_b.Set(obj, false);
    BOOST_CHECK(lazy_a != lazy_b);
}

// Test 3: Two wrappers set with the same underlying DummyBLS value compare equal.
BOOST_AUTO_TEST_CASE(test_equality_same_value)
{
    LazyDummyBLS lazy1;
    LazyDummyBLS lazy2;
    BOOST_CHECK(lazy1 == lazy2);
    DummyBLS obj;
    obj.data = {5, 6, 7, 8};
    lazy1.Set(obj, false);
    BOOST_CHECK(lazy1 != lazy2);
    lazy2.Set(obj, false);
    BOOST_CHECK(lazy1 == lazy2);
}

// Test 4: Serialization and unserialization preserve the wrapped value.
BOOST_AUTO_TEST_CASE(test_serialization_unserialization)
{
    LazyDummyBLS lazy1;
    DummyBLS obj;
    obj.data = {9, 10, 11, 12};
    // Set with a specific legacy flag (true in this case)
    lazy1.Set(obj, true);

    // Serialize the lazy object into a data stream.
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    lazy1.Serialize(ds, true);

    // Create a new instance and unserialize the data into it.
    LazyDummyBLS lazy2;
    lazy2.Unserialize(ds, true);
    BOOST_CHECK(lazy1 == lazy2);
    BOOST_CHECK(lazy2.Get() == obj);
}

// Test 5: Two wrappers wrapping the same object should have the same hash.
BOOST_AUTO_TEST_CASE(test_get_hash_consistency)
{
    LazyDummyBLS lazy1;
    LazyDummyBLS lazy2;
    DummyBLS obj;
    obj.data = {13, 14, 15, 16};
    lazy1.Set(obj, false);
    lazy2.Set(obj, false);
    uint256 hash1 = lazy1.GetHash();
    uint256 hash2 = lazy2.GetHash();
    BOOST_CHECK(hash1 == hash2);
}

BOOST_AUTO_TEST_SUITE_END()
