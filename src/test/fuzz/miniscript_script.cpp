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
