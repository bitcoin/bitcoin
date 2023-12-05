// Copyright (c) 2021-2022 The Bitcoin Core developers
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

/* Fuzz tests that test parsing from a string, and roundtripping via string. */
FUZZ_TARGET(miniscript_string, .init = FuzzInit)
{
    if (buffer.empty()) return;
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto str = provider.ConsumeBytesAsString(provider.remaining_bytes() - 1);
    const ParserContext parser_ctx{(MsCtx)provider.ConsumeBool()};
    auto parsed = miniscript::FromString(str, parser_ctx);
    if (!parsed) return;

    const auto str2 = parsed->ToString(parser_ctx);
    assert(str2);
    auto parsed2 = miniscript::FromString(*str2, parser_ctx);
    assert(parsed2);
    assert(*parsed == *parsed2);
}

/* Fuzz tests that test parsing from a script, and roundtripping via script. */
FUZZ_TARGET(miniscript_script)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CScript> script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (!script) return;

    const ScriptParserContext script_parser_ctx{(MsCtx)fuzzed_data_provider.ConsumeBool()};
    const auto ms = miniscript::FromScript(*script, script_parser_ctx);
    if (!ms) return;

    assert(ms->ToScript(script_parser_ctx) == *script);
}
