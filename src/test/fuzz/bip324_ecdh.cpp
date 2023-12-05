// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

void initialize_key()
{
    ECC_Start();
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(bip324_ecdh, .init = initialize_key)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    // We generate private key, k1.
    CKey k1 = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!k1.IsValid()) return;

    // They generate private key, k2.
    CKey k2 = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!k2.IsValid()) return;

    // We construct an ellswift encoding for our key, k1_ellswift.
    auto ent32_1 = fdp.ConsumeBytes<std::byte>(32);
    ent32_1.resize(32);
    auto k1_ellswift = k1.EllSwiftCreate(ent32_1);

    // They construct an ellswift encoding for their key, k2_ellswift.
    auto ent32_2 = fdp.ConsumeBytes<std::byte>(32);
    ent32_2.resize(32);
    auto k2_ellswift = k2.EllSwiftCreate(ent32_2);

    // They construct another (possibly distinct) ellswift encoding for their key, k2_ellswift_bad.
    auto ent32_2_bad = fdp.ConsumeBytes<std::byte>(32);
    ent32_2_bad.resize(32);
    auto k2_ellswift_bad = k2.EllSwiftCreate(ent32_2_bad);
    assert((ent32_2_bad == ent32_2) == (k2_ellswift_bad == k2_ellswift));

    // Determine who is who.
    bool initiating = fdp.ConsumeBool();

    // We compute our shared secret using our key and their public key.
    auto ecdh_secret_1 = k1.ComputeBIP324ECDHSecret(k2_ellswift, k1_ellswift, initiating);
    // They compute their shared secret using their key and our public key.
    auto ecdh_secret_2 = k2.ComputeBIP324ECDHSecret(k1_ellswift, k2_ellswift, !initiating);
    // Those must match, as everyone is behaving correctly.
    assert(ecdh_secret_1 == ecdh_secret_2);

    if (k1_ellswift != k2_ellswift) {
        // Unless the two keys are exactly identical, acting as the wrong party breaks things.
        auto ecdh_secret_bad = k1.ComputeBIP324ECDHSecret(k2_ellswift, k1_ellswift, !initiating);
        assert(ecdh_secret_bad != ecdh_secret_1);
    }

    if (k2_ellswift_bad != k2_ellswift) {
        // Unless both encodings created by them are identical, using the second one breaks things.
        auto ecdh_secret_bad = k1.ComputeBIP324ECDHSecret(k2_ellswift_bad, k1_ellswift, initiating);
        assert(ecdh_secret_bad != ecdh_secret_1);
    }
}
