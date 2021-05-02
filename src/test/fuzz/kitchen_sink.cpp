// Copyright (c) 2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/util.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/error.h>
#include <util/translation.h>

#include <cstdint>
#include <vector>

// The fuzzing kitchen sink: Fuzzing harness for functions that need to be
// fuzzed but a.) don't belong in any existing fuzzing harness file, and
// b.) are not important enough to warrant their own fuzzing harness file.
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const TransactionError transaction_error = fuzzed_data_provider.PickValueInArray<TransactionError>({TransactionError::OK, TransactionError::MISSING_INPUTS, TransactionError::ALREADY_IN_CHAIN, TransactionError::P2P_DISABLED, TransactionError::MEMPOOL_REJECTED, TransactionError::MEMPOOL_ERROR, TransactionError::INVALID_PSBT, TransactionError::PSBT_MISMATCH, TransactionError::SIGHASH_MISMATCH, TransactionError::MAX_FEE_EXCEEDED});
    (void)JSONRPCTransactionError(transaction_error);
    (void)RPCErrorFromTransactionError(transaction_error);
    (void)TransactionErrorString(transaction_error);
}
