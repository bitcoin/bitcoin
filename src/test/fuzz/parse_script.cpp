// Copyright (c) 2009-2020 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(parse_script)
{
    const std::string script_string(buffer.begin(), buffer.end());
    try {
        (void)ParseScript(script_string);
    } catch (const std::runtime_error&) {
    }
}
