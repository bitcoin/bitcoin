// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>
#include <merkleblock.h>
#include <node/types.h>
#include <policy/fees.h>
#include <rpc/util.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/translation.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

using common::TransactionErrorString;
using node::TransactionError;

namespace {
constexpr TransactionError ALL_TRANSACTION_ERROR[] = {
    TransactionError::MISSING_INPUTS,
    TransactionError::ALREADY_IN_UTXO_SET,
    TransactionError::MEMPOOL_REJECTED,
    TransactionError::MEMPOOL_ERROR,
    TransactionError::MAX_FEE_EXCEEDED,
};
}; // namespace

// The fuzzing kitchen sink: Fuzzing harness for functions that need to be
// fuzzed but a.) don't belong in any existing fuzzing harness file, and
// b.) are not important enough to warrant their own fuzzing harness file.
FUZZ_TARGET(kitchen_sink)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const TransactionError transaction_error = fuzzed_data_provider.PickValueInArray(ALL_TRANSACTION_ERROR);
    (void)JSONRPCTransactionError(transaction_error);
    (void)RPCErrorFromTransactionError(transaction_error);
    (void)TransactionErrorString(transaction_error);

    (void)StringForFeeEstimateHorizon(fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS));

    const OutputType output_type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
    const std::string& output_type_string = FormatOutputType(output_type);
    const std::optional<OutputType> parsed = ParseOutputType(output_type_string);
    assert(parsed);
    assert(output_type == parsed.value());
    (void)ParseOutputType(fuzzed_data_provider.ConsumeRandomLengthString(64));

    const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::vector<bool> bits = BytesToBits(bytes);
    const std::vector<uint8_t> bytes_decoded = BitsToBytes(bits);
    assert(bytes == bytes_decoded);
}
