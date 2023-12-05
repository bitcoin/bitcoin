// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/miniscript.h>
#include <util/strencodings.h>

void FuzzInit()
{
    ECC_Start();
    TEST_DATA.Init();
}

void FuzzInitSmart()
{
    FuzzInit();
    SMARTINFO.Init();
}

/** Fuzz target that runs TestNode on nodes generated using ConsumeNodeSmart. */
FUZZ_TARGET(miniscript_smart, .init = FuzzInitSmart)
{
    /** The set of types we aim to construct nodes for. Together they cover all. */
    static constexpr std::array<Type, 4> BASE_TYPES{"B"_mst, "V"_mst, "K"_mst, "W"_mst};

    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const auto script_ctx{(MsCtx)provider.ConsumeBool()};
    TestNode(script_ctx, GenNode(script_ctx, [&](Type needed_type) {
        return ConsumeNodeSmart(script_ctx, provider, needed_type);
    }, PickValue(provider, BASE_TYPES), true), provider);
}
