// Copyright (c) 2019-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <bls/bls_batchverifier.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <streams.h>
#include <clientversion.h>
BOOST_FIXTURE_TEST_SUITE(bls_tests, BasicTestingSetup)

void FuncSign(const bool legacy_scheme)
{
    bls::bls_legacy_scheme.store(legacy_scheme);

    CBLSSecretKey sk1, sk2;
    sk1.MakeNewKey();
    sk2.MakeNewKey();

    uint256 msgHash1 = uint256::ONEV;
    uint256 msgHash2 = uint256::TWOV;

    auto sig1 = sk1.Sign(msgHash1);
    auto sig2 = sk2.Sign(msgHash1);
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
    uint256 msgHash = uint256::ONEV;

    sk.MakeNewKey();
    CBLSSignature sig1 = sk.Sign(msgHash);
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

    CBLSSecretKey sk;
    std::string strValidSecret = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    // Note: invalid string passed to SetHexStr() should cause it to fail and reset key internal data
    BOOST_CHECK(sk.SetHexStr(strValidSecret));
    BOOST_CHECK(!sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1g")); // non-hex
    BOOST_CHECK(!sk.IsValid());
    BOOST_CHECK(sk == CBLSSecretKey());
    // Try few more invalid strings
    BOOST_CHECK(sk.SetHexStr(strValidSecret));
    BOOST_CHECK(!sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e")); // hex but too short
    BOOST_CHECK(!sk.IsValid());
    BOOST_CHECK(sk.SetHexStr(strValidSecret));
    BOOST_CHECK(!sk.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20")); // hex but too long
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

    uint256 msgHash1 = uint256::ONEV;
    uint256 msgHash2 = uint256::TWOV;

    auto sig = ag_sk.Sign(msgHash1);
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

    uint256 msgHash1 = uint256::ONEV;
    uint256 msgHash2 = uint256::TWOV;

    auto sig = ag_sk.Sign(msgHash1);
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
        CBLSSignature sig_i = sk.Sign(hash);
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
        vec_sigs.push_back(sk.Sign(hash));
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
    Message m;
    m.sourceId = sourceId;
    m.msgId = msgId;
    m.msgHash = uint256(msgHash);
    m.sk.MakeNewKey();
    m.pk = m.sk.GetPublicKey();
    m.sig = m.sk.Sign(m.msgHash);
    m.valid = valid;

    if (!valid) {
        CBLSSecretKey tmp;
        tmp.MakeNewKey();
        m.sig = tmp.Sign(m.msgHash);
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

BOOST_AUTO_TEST_SUITE_END()
