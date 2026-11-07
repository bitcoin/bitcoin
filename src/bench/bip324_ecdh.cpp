// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <crypto/hkdf_sha256_32.h>
#include <key.h>
#include <pubkey.h>
#include <random.h>
#include <span.h>
#include <support/cleanse.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>

static void BIP324_ECDH(benchmark::Bench& bench)
{
    ECC_Context ecc_context{};
    FastRandomContext rng;

    std::array<std::byte, 32> key_data;
    std::array<std::byte, EllSwiftPubKey::size()> our_ellswift_data;
    std::array<std::byte, EllSwiftPubKey::size()> their_ellswift_data;

    rng.fillrand(key_data);
    rng.fillrand(our_ellswift_data);
    rng.fillrand(their_ellswift_data);

    bench.batch(1).unit("ecdh").run([&] {
        CKey key;
        key.Set(key_data.data(), key_data.data() + 32, true);
        EllSwiftPubKey our_ellswift(our_ellswift_data);
        EllSwiftPubKey their_ellswift(their_ellswift_data);

        auto ret = key.ComputeBIP324ECDHSecret(their_ellswift, our_ellswift, true);

        // To make sure that the computation is not the same on every iteration (ellswift decoding
        // is variable-time), distribute bytes from the shared secret over the 3 inputs. The most
        // important one is their_ellswift, because that one is actually decoded, so it's given most
        // bytes. The data is copied into the middle, so that both halves are affected:
        // - Copy 8 bytes from the resulting shared secret into middle of the private key.
        std::copy(ret.begin(), ret.begin() + 8, key_data.begin() + 12);
        // - Copy 8 bytes from the resulting shared secret into the middle of our ellswift key.
        std::copy(ret.begin() + 8, ret.begin() + 16, our_ellswift_data.begin() + 28);
        // - Copy 16 bytes from the resulting shared secret into the middle of their ellswift key.
        std::copy(ret.begin() + 16, ret.end(), their_ellswift_data.begin() + 24);
    });
}

BENCHMARK(BIP324_ECDH);

static void BIP324_HKDF(benchmark::Bench& bench)
{
    FastRandomContext rng;
    auto secret{rng.randbytes<32>()};
    const std::string salt{"bitcoin_v2_shared_secret\xf9\xbe\xb4\xd9"};
    const std::array<std::string, 6> labels{"initiator_L", "initiator_P", "responder_L", "responder_P", "garbage_terminators", "session_id"};

    bench.unit("derivation").run([&] {
        CHKDF_HMAC_SHA256_L32 hkdf{secret, salt};
        std::array<std::byte, 32> out;
        for (const auto& label : labels) {
            hkdf.Expand32(label, out);
        }
        secret[0] = out[0];
        memory_cleanse(out.data(), out.size());
        memory_cleanse(&hkdf, sizeof(hkdf));
    });
}

BENCHMARK(BIP324_HKDF);
