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

/** Fuzz target that runs TestNode on nodes generated using ConsumeNodeStable. */
FUZZ_TARGET(miniscript_stable, .init = FuzzInit)
{
    // Run it under both P2WSH and Tapscript contexts.
    for (const auto script_ctx: {MsCtx::P2WSH, MsCtx::TAPSCRIPT}) {
        FuzzedDataProvider provider(buffer.data(), buffer.size());
        TestNode(script_ctx, GenNode(script_ctx, [&](Type needed_type) {
            return ConsumeNodeStable(script_ctx, provider, needed_type);
        }, ""_mst), provider);
    }
}
