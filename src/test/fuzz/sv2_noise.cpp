// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_noise.h>
#include <logging.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/xoroshiro128plusplus.h>

#include <cstdint>
#include <util/vector.h>

namespace {


void Initialize()
{
    // Add test context for debugging. Usage:
    // --debug=sv2 --loglevel=sv2:trace --printtoconsole=1
    static const auto testing_setup = std::make_unique<const BasicTestingSetup>();
}
} // namespace

bool MaybeDamage(FuzzedDataProvider& provider, std::vector<std::byte>& transport)
{
    if (transport.size() == 0) return false;

    // Optionally damage 1 bit in the ciphertext.
    const bool damage = provider.ConsumeBool();
    if (damage) {
        unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0,
                                                                        transport.size() * 8U - 1U);
        unsigned damage_pos = damage_bit >> 3;
        LogTrace(BCLog::SV2, "Damage byte %d of %d\n", damage_pos, transport.size());
        std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
        transport.at(damage_pos) ^= damage_val;
    }
    return damage;
}

FUZZ_TARGET(sv2_noise_cipher_roundtrip, .init = Initialize)
{
    // Test that Sv2Noise's encryption and decryption agree.

    // To conserve fuzzer entropy, deterministically generate Alice and Bob keys.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto seed_ent = provider.ConsumeBytes<std::byte>(32);
    seed_ent.resize(32);
    CExtKey seed;
    seed.SetSeed(seed_ent);

    CExtKey tmp;
    if (!seed.Derive(tmp, 0)) return;
    CKey alice_authority_key{tmp.key};

    if (!seed.Derive(tmp, 1)) return;
    CKey alice_static_key{tmp.key};

    if (!seed.Derive(tmp, 2)) return;
    CKey alice_ephemeral_key{tmp.key};

    if (!seed.Derive(tmp, 10)) return;
    CKey bob_authority_key{tmp.key};

    if (!seed.Derive(tmp, 11)) return;
    CKey bob_static_key{tmp.key};

    if (!seed.Derive(tmp, 12)) return;
    CKey bob_ephemeral_key{tmp.key};

    // Create certificate
    // Pick random times in the past or future
    uint32_t now = provider.ConsumeIntegralInRange<uint32_t>(10000U, UINT32_MAX);
    SetMockTime(now);
    uint16_t version = provider.ConsumeBool() ? 0 : provider.ConsumeIntegral<uint16_t>();
    uint32_t past = provider.ConsumeIntegralInRange<uint32_t>(0, now);
    uint32_t future = provider.ConsumeIntegralInRange<uint32_t>(now, UINT32_MAX);
    uint32_t valid_from = int32_t(provider.ConsumeBool() ? past : future);
    uint32_t valid_to = int32_t(provider.ConsumeBool() ? future : past);

    auto bob_certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                                    XOnlyPubKey(bob_static_key.GetPubKey()), bob_authority_key);

    bool valid_certificate = version == 0 &&
                             (valid_from <= now) &&
                             (valid_to >= now);

    LogTrace(BCLog::SV2, "valid_certificate: %d - version %u, past: %u, now %u, future: %u\n", valid_certificate, version, past, now, future);

    // Alice's static is not used in the test
    // Alice needs to verify Bob's certificate, so we pass his authority key
    auto alice_handshake = std::make_unique<Sv2HandshakeState>(std::move(alice_static_key), XOnlyPubKey(bob_authority_key.GetPubKey()));
    alice_handshake->SetEphemeralKey(std::move(alice_ephemeral_key));
    // Bob is the responder and does not receive (or verify) Alice's certificate,
    // so we don't pass her authority key.
    auto bob_handshake = std::make_unique<Sv2HandshakeState>(std::move(bob_static_key), std::move(bob_certificate));
    bob_handshake->SetEphemeralKey(std::move(bob_ephemeral_key));

    // Handshake Act 1: e ->

    std::vector<std::byte> transport;
    transport.resize(Sv2HandshakeState::ELLSWIFT_PUB_KEY_SIZE);
    // Alice generates her ephemeral public key and write it into the buffer:
    alice_handshake->WriteMsgEphemeralPK(transport);

    bool damage_e = MaybeDamage(provider, transport);

    // Bob reads the ephemeral key ()
    // With EllSwift encoding this step can't fail
    bob_handshake->ReadMsgEphemeralPK(transport);
    ClearShrink(transport);

    // Handshake Act 2: <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE
    transport.resize(Sv2HandshakeState::HANDSHAKE_STEP2_SIZE);
    bob_handshake->WriteMsgES(transport);

    bool damage_es = MaybeDamage(provider, transport);

    // This ignores the remote possibility that the fuzzer finds two equivalent
    // EllSwift encodings by flipping a single ephemeral key bit.
    assert(alice_handshake->ReadMsgES(transport) == (valid_certificate && !damage_e && !damage_es));

    if (!valid_certificate || damage_e || damage_es) return;

    // Construct Sv2Cipher from the Sv2HandshakeState and test transport
    auto alice{Sv2Cipher(/*initiator=*/true, std::move(alice_handshake))};
    auto bob{Sv2Cipher(/*initiator=*/false, std::move(bob_handshake))};
    alice.FinishHandshake();
    bob.FinishHandshake();

    // Use deterministic RNG to generate content rather than creating it from
    // the fuzzer input.
    XoRoShiRo128PlusPlus rng(provider.ConsumeIntegral<uint64_t>());

    LIMITED_WHILE(provider.remaining_bytes(), 1000)
    {
        ClearShrink(transport);

        // Alice or Bob sends a message
        bool from_alice = provider.ConsumeBool();

        // Set content length (slightly above NOISE_MAX_CHUNK_SIZE)
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, NOISE_MAX_CHUNK_SIZE + 100);
        std::vector<std::byte> plain(length);
        for (auto& val : plain)
            val = std::byte{(uint8_t)rng()};

        const size_t encrypted_size = Sv2Cipher::EncryptedMessageSize(plain.size());
        transport.resize(encrypted_size);

        assert((from_alice ? alice : bob).EncryptMessage(plain, transport));

        const bool damage = MaybeDamage(provider, transport);

        std::vector<std::byte> plain_read;
        plain_read.resize(plain.size());

        bool ok = (from_alice ? bob : alice).DecryptMessage(transport, plain_read);
        assert(!ok == damage);
        if (!ok) break;

        assert(plain == plain_read);
    }
}
