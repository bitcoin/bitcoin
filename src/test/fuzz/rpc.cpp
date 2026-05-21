// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/rpc.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <univalue.h>
#include <util/time.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
enum class ChainType;

namespace {
struct RPCFuzzTestingSetup : public TestingSetup {
    RPCFuzzTestingSetup(const ChainType chain_type, TestOpts opts) : TestingSetup{chain_type, opts}
    {
    }

    void CallRPC(const std::string& rpc_method, const std::vector<std::string>& arguments)
    {
        JSONRPCRequest request;
        request.context = &m_node;
        request.strMethod = rpc_method;
        try {
            request.params = RPCConvertValues(rpc_method, arguments);
        } catch (const std::runtime_error&) {
            return;
        }
        tableRPC.execute(request);
    }

    std::vector<std::string> GetRPCCommands() const
    {
        return tableRPC.listCommands();
    }
};

RPCFuzzTestingSetup* rpc_testing_setup = nullptr;
std::string g_limit_to_rpc_command;

// RPC commands which are not appropriate for fuzzing: such as RPC commands
// reading or writing to a filename passed as an RPC parameter, RPC commands
// resulting in network activity, etc.
const std::vector<std::string> RPC_COMMANDS_NOT_SAFE_FOR_FUZZING{
    "addconnection",  // avoid DNS lookups
    "addnode",        // avoid DNS lookups
    "addpeeraddress", // avoid DNS lookups
    "dumptxoutset",   // avoid writing to disk
    "enumeratesigners",
    "echoipc",              // avoid assertion failure (Assertion `"EnsureAnyNodeContext(request.context).init" && check' failed.)
    "exportasmap",          // avoid writing to disk
    "generatetoaddress",    // avoid prohibitively slow execution (when `num_blocks` is large)
    "generatetodescriptor", // avoid prohibitively slow execution (when `nblocks` is large)
    "gettxoutproof",        // avoid prohibitively slow execution
    "importmempool",        // avoid reading from disk
    "loadtxoutset",         // avoid reading from disk
    "loadwallet",           // avoid reading from disk
    "savemempool",          // disabled as a precautionary measure: may take a file path argument in the future
    "setban",               // avoid DNS lookups
    "stop",                 // avoid shutdown state
};

// RPC commands which are safe for fuzzing.
const std::vector<std::string> RPC_COMMANDS_SAFE_FOR_FUZZING{
    "abortprivatebroadcast",
    "analyzepsbt",
    "clearbanned",
    "combinepsbt",
    "combinerawtransaction",
    "converttopsbt",
    "createmultisig",
    "createpsbt",
    "createrawtransaction",
    "decodepsbt",
    "decoderawtransaction",
    "decodescript",
    "deriveaddresses",
    "descriptorprocesspsbt",
    "disconnectnode",
    "echo",
    "echojson",
    "estimaterawfee",
    "estimatesmartfee",
    "finalizepsbt",
    "generate",
    "generateblock",
    "getaddednodeinfo",
    "getaddrmaninfo",
    "getbestblockhash",
    "getblock",
    "getblockchaininfo",
    "getblockcount",
    "getblockfilter",
    "getblockfrompeer", // when no peers are connected, no p2p message is sent
    "getblockhash",
    "getblockheader",
    "getblockstats",
    "getblocktemplate",
    "getchaintips",
    "getchainstates",
    "getchaintxstats",
    "getconnectioncount",
    "getdeploymentinfo",
    "getdescriptoractivity",
    "getdescriptorinfo",
    "getdifficulty",
    "getindexinfo",
    "getmemoryinfo",
    "getmempoolancestors",
    "getmempooldescendants",
    "getmempoolentry",
    "getmempoolfeeratediagram",
    "getmempoolcluster",
    "getmempoolinfo",
    "getmininginfo",
    "getnettotals",
    "getnetworkhashps",
    "getnetworkinfo",
    "getnodeaddresses",
    "getorphantxs",
    "getpeerinfo",
    "getprioritisedtransactions",
    "getprivatebroadcastinfo",
    "getrawaddrman",
    "getrawmempool",
    "getrawtransaction",
    "getrpcinfo",
    "gettxout",
    "gettxoutsetinfo",
    "gettxspendingprevout",
    "help",
    "invalidateblock",
    "joinpsbts",
    "listbanned",
    "logging",
    "mockscheduler",
    "ping",
    "preciousblock",
    "prioritisetransaction",
    "pruneblockchain",
    "reconsiderblock",
    "scanblocks",
    "scantxoutset",
    "sendmsgtopeer", // when no peers are connected, no p2p message is sent
    "sendrawtransaction",
    "setmocktime",
    "setnetworkactive",
    "signmessagewithprivkey",
    "signrawtransactionwithkey",
    "submitblock",
    "submitheader",
    "submitpackage",
    "syncwithvalidationinterfacequeue",
    "testmempoolaccept",
    "uptime",
    "utxoupdatepsbt",
    "validateaddress",
    "verifychain",
    "verifymessage",
    "verifytxoutproof",
    "waitforblock",
    "waitforblockheight",
    "waitfornewblock",
};

RPCFuzzTestingSetup* InitializeRPCFuzzTestingSetup()
{
    static const auto setup = MakeNoLogFileContext<RPCFuzzTestingSetup>();
    SetRPCWarmupFinished();
    return setup.get();
}
}; // namespace

void initialize_rpc()
{
    rpc_testing_setup = InitializeRPCFuzzTestingSetup();
    fuzzrpc::ValidateRpcCommands(RPC_COMMANDS_SAFE_FOR_FUZZING, RPC_COMMANDS_NOT_SAFE_FOR_FUZZING, rpc_testing_setup->GetRPCCommands(), __FILE__);
    const char* limit_to_rpc_command_env = std::getenv("LIMIT_TO_RPC_COMMAND");
    if (limit_to_rpc_command_env != nullptr) {
        g_limit_to_rpc_command = std::string{limit_to_rpc_command_env};
    }
}

FUZZ_TARGET(rpc, .init = initialize_rpc)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    bool good_data{true};
    NodeClockContext clock_ctx{ConsumeTime(fuzzed_data_provider)};
    const std::string rpc_command = fuzzed_data_provider.ConsumeRandomLengthString(64);
    if (!g_limit_to_rpc_command.empty() && rpc_command != g_limit_to_rpc_command) {
        return;
    }
    const bool safe_for_fuzzing = std::find(RPC_COMMANDS_SAFE_FOR_FUZZING.begin(), RPC_COMMANDS_SAFE_FOR_FUZZING.end(), rpc_command) != RPC_COMMANDS_SAFE_FOR_FUZZING.end();
    if (!safe_for_fuzzing) {
        return;
    }
    std::vector<std::string> arguments;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 100)
    {
        arguments.push_back(fuzzrpc::ConsumeRPCArgument(fuzzed_data_provider, good_data));
    }
    try {
        std::optional<test_only_CheckFailuresAreExceptionsNotAborts> maybe_mock{};
        if (rpc_command == "echo") {
            // Avoid aborting fuzzing for this specific test-only RPC with an
            // intentional trigger_internal_bug
            maybe_mock.emplace();
        }
        rpc_testing_setup->CallRPC(rpc_command, arguments);
    } catch (const UniValue& json_rpc_error) {
        const std::string error_msg{json_rpc_error.find_value("message").get_str()};
        if (error_msg.starts_with("Internal bug detected")) {
            // Only allow the intentional internal bug
            assert(error_msg.find("trigger_internal_bug") != std::string::npos);
        }
    }
}
