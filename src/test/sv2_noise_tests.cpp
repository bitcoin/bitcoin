#include <common/sv2_noise.h>
#include <key.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sv2_noise_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(MixKey_test)
{
    Sv2SymmetricState i_ss;
    Sv2SymmetricState r_ss;
    BOOST_CHECK_EQUAL(r_ss.GetChainingKey(), i_ss.GetChainingKey());

    CKey initiator_key{GenerateRandomKey()};
    auto initiator_pk = initiator_key.EllSwiftCreate(MakeByteSpan(GetRandHash()));

    CKey responder_key{GenerateRandomKey()};
    auto responder_pk = responder_key.EllSwiftCreate(MakeByteSpan(GetRandHash()));

    auto ecdh_output_1 = initiator_key.ComputeBIP324ECDHSecret(responder_pk, initiator_pk, true);
    auto ecdh_output_2 = responder_key.ComputeBIP324ECDHSecret(initiator_pk, responder_pk, false);

    BOOST_CHECK(std::memcmp(&ecdh_output_1[0], &ecdh_output_2[0], 32) == 0);

    i_ss.MixKey(ecdh_output_1);
    r_ss.MixKey(ecdh_output_2);

    BOOST_CHECK_EQUAL(r_ss.GetChainingKey(), i_ss.GetChainingKey());
}

BOOST_AUTO_TEST_CASE(certificate_test)
{
    auto alice_static_key{GenerateRandomKey()};
    auto alice_authority_key{GenerateRandomKey()};

    // Create certificate
    auto epoch_now = std::chrono::system_clock::now().time_since_epoch();
    uint32_t now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(epoch_now).count());
    uint16_t version = 0;
    uint32_t valid_from = now;
    uint32_t valid_to = std::numeric_limits<unsigned int>::max();

    auto alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                      XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);

    BOOST_REQUIRE(alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    auto malory_authority_key{GenerateRandomKey()};
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(malory_authority_key.GetPubKey())));

    // Check that certificate is not from the future
    valid_from = now + 1;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                 XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    valid_from = now;

    // Check certificate expiration
    valid_to = now - 10000;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                 XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    valid_to = now;

    // Only version 0 is supported
    version = 1;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                 XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));
}

BOOST_AUTO_TEST_CASE(handshake_and_transport_test)
{
    // Alice initiates a handshake with Bob

    auto alice_static_key{GenerateRandomKey()};
    auto bob_static_key{GenerateRandomKey()};
    auto bob_authority_key{GenerateRandomKey()};

    // Create certificates
    auto epoch_now = std::chrono::system_clock::now().time_since_epoch();
    uint16_t version = 0;
    uint32_t valid_from = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(epoch_now).count());
    uint32_t valid_to = std::numeric_limits<unsigned int>::max();

    auto bob_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                    XOnlyPubKey(bob_static_key.GetPubKey()),
                                                    bob_authority_key);

    // Alice's static is not used in the test
    // Alice needs to verify Bob's certificate, so we pass his authority key
    auto alice_handshake = std::make_unique<Sv2HandshakeState>(std::move(alice_static_key),
                                                               XOnlyPubKey(bob_authority_key.GetPubKey()));
    // Bob is the responder and does not receive (or verify) Alice's certificate,
    // so we don't pass her authority key.
    auto bob_handshake = std::make_unique<Sv2HandshakeState>(std::move(bob_static_key),
                                                             std::move(bob_certificate));

    // Handshake Act 1: e ->

    std::vector<std::byte> transport;
    transport.resize(Sv2HandshakeState::ELLSWIFT_PUB_KEY_SIZE);
    // Alice generates her ephemeral public key and write it into the buffer:
    alice_handshake->WriteMsgEphemeralPK(transport);
    EllSwiftPubKey alice_pubkey(transport);

    // Bob reads the ephemeral key
    bob_handshake->ReadMsgEphemeralPK(transport);

    ClearShrink(transport);

    // Handshake Act 2: <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE
    transport.resize(Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);
    bob_handshake->WriteMsgES(transport);
    BOOST_REQUIRE(alice_handshake->ReadMsgES(transport));

    // Construct Sv2Cipher from the Sv2HandshakeState and test transport
    auto alice{Sv2Cipher(/*initiator=*/true, std::move(alice_handshake))};
    auto bob{Sv2Cipher(/*initiator=*/false, std::move(bob_handshake))};
    alice.FinishHandshake();
    bob.FinishHandshake();

    ClearShrink(transport);

    const std::vector<uint8_t> TEST = {
        // hello world
        0x68,
        0x65,
        0x6C,
        0x6C,
        0x6F,
        0x20,
        0x77,
        0x6F,
        0x72,
        0x6C,
        0x64,
    };

    const size_t encrypted_size = Sv2Cipher::EncryptedMessageSize(TEST.size());
    BOOST_CHECK_EQUAL(encrypted_size, TEST.size() + Poly1305::TAGLEN);

    transport.resize(encrypted_size);

    auto plain_send{MakeByteSpan(TEST)};
    BOOST_TEST_CHECKPOINT("Alice encrypts message");
    BOOST_REQUIRE(alice.EncryptMessage(plain_send, transport));

    std::vector<std::byte> plain_receive(TEST.size(), std::byte(0));
    BOOST_TEST_CHECKPOINT("Bob decrypts message");
    BOOST_REQUIRE(bob.DecryptMessage(transport, plain_receive));
    BOOST_CHECK_EQUAL(HexStr(plain_receive), HexStr(TEST));
}
BOOST_AUTO_TEST_SUITE_END()
