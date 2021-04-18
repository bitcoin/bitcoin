// Copyright (c) 2009-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/fuzz.h>
#include <util/memory.h>

void initialize()
{
    static const ECCVerifyHandle verify_handle;
    ECC_Start();
    SelectParams(CBaseChainParams::MAIN);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string descriptor(buffer.begin(), buffer.end());
    FlatSigningProvider signing_provider;
    std::string error;
    for (const bool require_checksum : {true, false}) {
        const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
        if (desc) {
            (void)desc->ToString();
            (void)desc->IsRange();
            (void)desc->IsSolvable();
        }
    }
}
