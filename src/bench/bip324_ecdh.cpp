// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <key.h>
#include <pubkey.h>
#include <random.h>
#include <secp256k1.h>
#include <secp256k1_ellswift.h>

#include <cstddef>

CKey GetRandomKey()
{
    CKey key;
    key.MakeNewKey(true);
    return key;
}

int GetEll64(const CKey& key, unsigned char* ell64, secp256k1_context* ctx)
{
    std::array<unsigned char, 32> rnd32;
    GetRandBytes(rnd32);
    return secp256k1_ellswift_create(ctx, ell64, reinterpret_cast<const unsigned char*>(key.data()), rnd32.data());
}

static void BIP324_ECDH(benchmark::Bench& bench)
{
    ECC_Start();
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    assert(ctx != nullptr);
    assert(secp256k1_context_randomize(ctx, nullptr));
    auto our_key = GetRandomKey();
    auto their_key = GetRandomKey();

    unsigned char our_ell64[64], their_ell64[64];
    if (!GetEll64(our_key, our_ell64, ctx)) {
        assert(false);
    }

    if (!GetEll64(their_key, their_ell64, ctx)) {
        assert(false);
    }

    bench.batch(1).unit("ecdh").run([&] {
        assert(our_key.ComputeBIP324ECDHSecret({reinterpret_cast<std::byte*>(their_ell64), 64},
                                               {reinterpret_cast<std::byte*>(our_ell64), 64},
                                               true)
                   .has_value());
    });
    secp256k1_context_destroy(ctx);
    ECC_Stop();
}

BENCHMARK(BIP324_ECDH, benchmark::PriorityLevel::HIGH);
