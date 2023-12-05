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
