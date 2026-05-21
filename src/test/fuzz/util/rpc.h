// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_RPC_H
#define BITCOIN_TEST_FUZZ_UTIL_RPC_H

#include <string>
#include <string_view>
#include <vector>

class FuzzedDataProvider;

namespace fuzzrpc {

std::string ConsumeScalarRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);
std::string ConsumeArrayRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);
std::string ConsumeRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);

void ValidateRpcCommands(const std::vector<std::string>& rpc_commands_safe_for_fuzzing,
                         const std::vector<std::string>& rpc_commands_not_safe_for_fuzzing,
                         const std::vector<std::string>& supported_rpc_commands,
                         std::string_view source_file);

} // namespace fuzzrpc

#endif // BITCOIN_TEST_FUZZ_UTIL_RPC_H
