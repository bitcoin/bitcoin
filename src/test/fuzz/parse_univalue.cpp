// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <rpc/client.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/memory.h>

#include <limits>
#include <string>

void initialize()
{
    static const BasicTestingSetup basic_testing_setup{CBaseChainParams::REGTEST};
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    bool valid = true;
    const UniValue univalue = [&] {
        try {
            return ParseNonRFCJSONValue(fuzzed_data_provider.ConsumeRandomLengthString(1024));
        } catch (const std::runtime_error&) {
            valid = false;
            return NullUniValue;
        }
    }();
    if (!valid) {
        return;
    }
    const std::string random_string = fuzzed_data_provider.ConsumeRandomLengthString(1024);
    try {
        (void)ParseHashO(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexO(univalue, random_string);
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexUV(univalue, random_string);
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexV(univalue, random_string);
    } catch (const UniValue&) {
    }
    try {
        (void)ParseSighashString(univalue);
    } catch (const std::runtime_error&) {
    }
    try {
        (void)AmountFromValue(univalue);
    } catch (const UniValue&) {
    }
    try {
        FlatSigningProvider provider;
        (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseConfirmTarget(univalue, std::numeric_limits<unsigned int>::max());
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseDescriptorRange(univalue);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
}
