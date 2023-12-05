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

FUZZ_TARGET(ellswift_roundtrip, .init = initialize_key)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    CKey key = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!key.IsValid()) return;

    auto ent32 = fdp.ConsumeBytes<std::byte>(32);
    ent32.resize(32);

    auto encoded_ellswift = key.EllSwiftCreate(ent32);
    auto decoded_pubkey = encoded_ellswift.Decode();

    assert(key.VerifyPubKey(decoded_pubkey));
}
