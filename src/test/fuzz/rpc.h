// rpc.h

#ifndef BITCOIN_TEST_FUZZ_RPC_H
#define BITCOIN_TEST_FUZZ_RPC_H

#include <base58.h>
#include <key.h>
#include <key_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <wallet/rpc/wallet.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <string>
#include <span>

// Forward declarations
enum class ChainType;
class FuzzedDataProvider;

namespace RPCFuzz {

// Externally visible functions and variables
extern struct RPCFuzzTestingSetup* rpc_testing_setup;
extern std::string g_limit_to_rpc_command;

struct RPCFuzzTestingSetup : public TestingSetup {
    RPCFuzzTestingSetup(const ChainType chain_type, TestOpts opts);

    void CallRPC(const std::string& rpc_method, const std::vector<std::string>& arguments);
    std::vector<std::string> GetRPCCommands() const;
};

// Functions
std::string ConsumeScalarRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);
std::string ConsumeArrayRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);
std::string ConsumeRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data);
RPCFuzzTestingSetup* InitializeRPCFuzzTestingSetup();
void initialize(std::vector<std::string> rpc_commands_safe_for_fuzzing, std::vector<std::string> rpc_commands_not_safe_for_fuzzing, std::vector<std::string> supported_rpc_commands);
void FuzzInitRPC();
void ExecuteFuzzCommands(std::vector<std::string> list_of_safe_commands, Span<const unsigned char> buffer);

} // namespace RPCFuzz

#endif // BITCOIN_TEST_FUZZ_RPC_H

