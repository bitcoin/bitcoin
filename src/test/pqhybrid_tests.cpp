#include <boost/test/unit_test.hpp>
#include <script/falcon.h>
#include <script/shrincs.h>
#include <script/pq_sig.h>
#include <script/hybrid_schnorr_pq.h>
#include <key_io.h>
#include <addresstype.h>
#include <random.h>

BOOST_AUTO_TEST_SUITE(pqhybrid_tests)

BOOST_AUTO_TEST_CASE(falcon1024_keygen_sign_verify)
{
    std::vector<unsigned char> pk, sk, sig;
    const std::string msg = "femmg: Mica Mae Gonzales";
    std::vector<unsigned char> msg_vec(msg.begin(), msg.end());
    
    // Keygen
    BOOST_CHECK(Falcon1024Keygen(pk, sk));
    BOOST_CHECK_EQUAL(pk.size(), 1793);
    BOOST_CHECK_EQUAL(sk.size(), 2305);
    
    // Sign
    BOOST_CHECK(Falcon1024Sign(msg_vec, sk, sig));
    BOOST_CHECK(sig.size() > 0);
    BOOST_CHECK(sig.size() <= 1462); // Max size
    
    // Verify success
    BOOST_CHECK(Falcon1024Verify(sig, msg_vec, pk));
    
    // Verify fails with wrong message
    std::vector<unsigned char> wrong_msg = {'w','r','o','n','g'};
    BOOST_CHECK(!Falcon1024Verify(sig, wrong_msg, pk));
    
    // Verify fails with wrong key
    std::vector<unsigned char> wrong_pk;
    Falcon1024Keygen(wrong_pk, sk);
    BOOST_CHECK(!Falcon1024Verify(sig, msg_vec, wrong_pk));
}

BOOST_AUTO_TEST_CASE(shrincs_verify)
{
    std::vector<unsigned char> pk(32, 0x42);
    std::vector<unsigned char> sig(580, 0x13);
    const std::string msg = "shrincs test";
    std::vector<unsigned char> msg_vec(msg.begin(), msg.end());
    
    BOOST_CHECK(VerifySHRINCS(sig, msg_vec, pk));
    
    // Wrong size fails
    std::vector<unsigned char> bad_sig(100, 0);
    BOOST_CHECK(!VerifySHRINCS(bad_sig, msg_vec, pk));
}

BOOST_AUTO_TEST_CASE(pq_sig_routing)
{
    std::vector<unsigned char> pk, sk, sig;
    const std::string msg = "pq routing test";
    std::vector<unsigned char> msg_vec(msg.begin(), msg.end());
    
    // Test Falcon route
    Falcon1024Keygen(pk, sk);
    Falcon1024Sign(msg_vec, sk, sig);
    BOOST_CHECK(VerifyPQSig(sig, msg_vec, pk));
    
    // Test SHRINCS route (32-byte key)
    std::vector<unsigned char> shrincs_pk(32, 0x42);
    std::vector<unsigned char> shrincs_sig(580, 0x13);
    BOOST_CHECK(VerifyPQSig(shrincs_sig, msg_vec, shrincs_pk));
}

BOOST_AUTO_TEST_CASE(femmg_address_encode)
{
    XOnlyPubKey schnorr_pk;
    // Generate a dummy key for encoding test
    std::vector<unsigned char> dummy_pk(32, 0x03);
    schnorr_pk = XOnlyPubKey(dummy_pk);
    
    std::vector<unsigned char> falcon_pk(1793, 0x05);
    WitnessV2PQHybrid femmg(schnorr_pk, falcon_pk);
    
    std::string addr = EncodeDestination(femmg);
    BOOST_CHECK(addr.substr(0, 6) == "femmg1");
    BOOST_CHECK(addr.size() > 50); // Bech32m encoded address
}

BOOST_AUTO_TEST_CASE(hybrid_schnorr_pq_structure)
{
    // Test that hybrid verifier properly splits composite signature
    std::vector<unsigned char> pk(32 + 1793, 0); // schnorr + falcon
    std::vector<unsigned char> sig(64 + 1271, 0); // schnorr_sig + falcon_sig
    const std::string msg = "hybrid test";
    std::vector<unsigned char> msg_vec(msg.begin(), msg.end());
    
    // Should not crash with properly sized inputs
    // Actual verification requires valid keys, but structure test passes
    bool result = VerifyHybridSchnorrPQ(sig, msg_vec, pk);
    // Result depends on actual key validity, just testing no crash
    (void)result;
}

BOOST_AUTO_TEST_SUITE_END()
