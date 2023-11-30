#include <common/sv2_noise.h>
#include <key.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sv2_noise_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(MixKey_test)
{
    Sv2SymmetricState i_ss;
    Sv2SymmetricState r_ss;
    BOOST_CHECK_EQUAL(r_ss.GetChainingKey(), i_ss.GetChainingKey());

    CKey initiator_key;
    initiator_key.MakeNewKey(true);
    while (!initiator_key.HasEvenY()) {
        initiator_key.MakeNewKey(true);
    }
    BOOST_CHECK(initiator_key.HasEvenY());
    auto initiator_pk = XOnlyPubKey(initiator_key.GetPubKey());

    CKey responder_key;
    responder_key.MakeNewKey(true);
    while (!responder_key.HasEvenY()) {
        responder_key.MakeNewKey(true);
    }
    BOOST_CHECK(responder_key.HasEvenY());
    auto responder_pk = XOnlyPubKey(responder_key.GetPubKey());

    unsigned char ecdh_output_1[32];
    initiator_key.ECDH(responder_pk, ecdh_output_1);


    unsigned char ecdh_output_2[32];
    responder_key.ECDH(initiator_pk, ecdh_output_2);

    BOOST_CHECK(std::memcmp(&ecdh_output_1[0], &ecdh_output_2[0], 32) == 0);

    i_ss.MixKey(Span(ecdh_output_1));
    r_ss.MixKey(Span(ecdh_output_2));

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

    // TODO: Stratum v2 spec requires signing the static key using the authority key,
    //       but SRI currently implements this incorrectly.
    alice_authority_key = alice_static_key;

    auto alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                               XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);

    BOOST_REQUIRE(alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    auto malory_authority_key{GenerateRandomKey()};
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(malory_authority_key.GetPubKey())));

    // Tolerate moderate difference in clock
    valid_from = now + 100;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                               XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    // But not too much
    valid_from = now + 10000;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                               XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(!alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    valid_from = now;

    // Tolerate moderate difference in clock
    valid_to = now - 100;
    alice_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                               XOnlyPubKey(alice_static_key.GetPubKey()), alice_authority_key);
    BOOST_REQUIRE(alice_certificate.Validate(XOnlyPubKey(alice_authority_key.GetPubKey())));

    // But not too much
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
    uint32_t valid_to =  std::numeric_limits<unsigned int>::max();

    // TODO: Stratum v2 spec requires signing the static key using the authority key,
    //       but SRI currently implements this incorrectly.
    bob_authority_key = bob_static_key;
    auto bob_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                             XOnlyPubKey(bob_static_key.GetPubKey()), bob_authority_key);

    // Alice's static is not used in the test
    // Alice needs to verify Bob's certificate, so we pass his authority key
    auto alice_handshake = std::make_unique<Sv2HandshakeState>(std::move(alice_static_key), XOnlyPubKey(bob_authority_key.GetPubKey()));
    // Bob is the responder and does not receive (or verify) Alice's certificate,
    // so we don't pass her authority key.
    auto bob_handshake = std::make_unique<Sv2HandshakeState>(std::move(bob_static_key), std::move(bob_certificate));

    // Handshake Act 1: e ->

    std::vector<uint8_t> transport_buffer;
    transport_buffer.resize(KEY_SIZE);
    Span<std::byte> transport_span{MakeWritableByteSpan(transport_buffer)};
    // Alice generates her ephemeral public key and write it into the buffer:
    alice_handshake->WriteMsgEphemeralPK(transport_span);
    XOnlyPubKey alice_pubkey(Span(&transport_buffer[0], XOnlyPubKey::size()));
    BOOST_CHECK(alice_pubkey.IsFullyValid());

    // Bob reads the ephemeral key
    bob_handshake->ReadMsgEphemeralPK(transport_span);

    ClearShrink(transport_buffer);

    // Handshake Act 2: <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE
    transport_buffer.resize(INITIATOR_EXPECTED_HANDSHAKE_MESSAGE_LENGTH);
    transport_span = MakeWritableByteSpan(transport_buffer);
    bob_handshake->WriteMsgES(transport_span);
    BOOST_REQUIRE(alice_handshake->ReadMsgES(transport_span));

    // Construct Sv2Cipher from the Sv2HandshakeState and test transport
    auto alice{Sv2Cipher(/*initiator=*/true, std::move(alice_handshake))};
    auto bob{Sv2Cipher(/*initiator=*/false, std::move(bob_handshake))};
    alice.FinishHandshake();
    bob.FinishHandshake();

    ClearShrink(transport_buffer);

    const std::vector<uint8_t> TEST = { // hello world
        0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64,
    };

    const size_t encrypted_size = Sv2Cipher::EncryptedMessageSize(TEST.size());
    BOOST_CHECK_EQUAL(encrypted_size, TEST.size() + POLY1305_TAGLEN);

    transport_buffer.resize(encrypted_size);
    transport_span = MakeWritableByteSpan(transport_buffer);

    auto plain_send{MakeByteSpan(TEST)};
    BOOST_TEST_CHECKPOINT("Alice encrypts message");
    alice.EncryptMessage(plain_send, transport_span);

    BOOST_TEST_CHECKPOINT("Bob decrypts message");
    BOOST_REQUIRE(bob.DecryptMessage(transport_span));
    BOOST_CHECK_EQUAL(HexStr(transport_span.subspan(0, TEST.size())), HexStr(TEST));
}
BOOST_AUTO_TEST_SUITE_END()
