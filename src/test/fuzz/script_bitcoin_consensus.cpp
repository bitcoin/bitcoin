// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/bitcoinconsensus.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(script_bitcoin_consensus)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint8_t> random_bytes_1 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::vector<uint8_t> random_bytes_2 = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const CAmount money = ConsumeMoney(fuzzed_data_provider);
    bitcoinconsensus_error err;
    bitcoinconsensus_error* err_p = fuzzed_data_provider.ConsumeBool() ? &err : nullptr;
    const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const unsigned int flags = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    assert(bitcoinconsensus_version() == BITCOINCONSENSUS_API_VER);
    if ((flags & SCRIPT_VERIFY_WITNESS) != 0 && (flags & SCRIPT_VERIFY_P2SH) == 0) {
        return;
    }
    (void)bitcoinconsensus_verify_script(random_bytes_1.data(), random_bytes_1.size(), random_bytes_2.data(), random_bytes_2.size(), n_in, flags, err_p);
    (void)bitcoinconsensus_verify_script_with_amount(random_bytes_1.data(), random_bytes_1.size(), money, random_bytes_2.data(), random_bytes_2.size(), n_in, flags, err_p);

    std::vector<UTXO> spent_outputs;
    std::vector<std::vector<unsigned char>> spent_spks;
    if (n_in <= 24386) {
        spent_outputs.reserve(n_in);
        spent_spks.reserve(n_in);
        for (size_t i = 0; i < n_in; ++i) {
            spent_spks.push_back(ConsumeRandomLengthByteVector(fuzzed_data_provider));
            const CAmount value{ConsumeMoney(fuzzed_data_provider)};
            const auto spk_size{static_cast<unsigned>(spent_spks.back().size())};
            spent_outputs.push_back({.scriptPubKey = spent_spks.back().data(), .scriptPubKeySize = spk_size, .value = value});
        }
    }

    const auto spent_outs_size{static_cast<unsigned>(spent_outputs.size())};

    (void)bitcoinconsensus_verify_script_with_spent_outputs(
            random_bytes_1.data(), random_bytes_1.size(), money, random_bytes_2.data(), random_bytes_2.size(),
            spent_outputs.data(), spent_outs_size, n_in, flags, err_p);
}
