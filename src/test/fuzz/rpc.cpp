// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <core_io.h>
#include <key.h>
#include <key_io.h>
#include <node/context.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <rpc/blockchain.h>
#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct RPCFuzzTestingSetup : public TestingSetup {
    RPCFuzzTestingSetup(const std::string& chain_name, const std::vector<const char*>& extra_args) : TestingSetup{chain_name, extra_args}
    {
    }

    UniValue CallRPC(const std::string& rpc_method, const std::vector<std::string>& arguments)
    {
        JSONRPCRequest request;
        request.context = &m_node;
        request.strMethod = rpc_method;
        request.params = RPCConvertValues(rpc_method, arguments);
        return tableRPC.execute(request);
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
    "analyzepsbt",    // avoid signed integer overflow in CFeeRate::GetFee(unsigned long) (https://github.com/bitcoin/bitcoin/issues/20607)
    "dumptxoutset",   // avoid writing to disk
#ifdef ENABLE_WALLET
    "dumpwallet", // avoid writing to disk
#endif
    "echoipc",              // avoid assertion failure (Assertion `"EnsureAnyNodeContext(request.context).init" && check' failed.)
    "generatetoaddress",    // avoid prohibitively slow execution (when `num_blocks` is large)
    "generatetodescriptor", // avoid prohibitively slow execution (when `nblocks` is large)
    "gettxoutproof",        // avoid prohibitively slow execution
#ifdef ENABLE_WALLET
    "importwallet", // avoid reading from disk
    "loadwallet",   // avoid reading from disk
#endif
    "prioritisetransaction", // avoid signed integer overflow in CTxMemPool::PrioritiseTransaction(uint256 const&, long const&) (https://github.com/bitcoin/bitcoin/issues/20626)
    "savemempool",           // disabled as a precautionary measure: may take a file path argument in the future
    "setban",                // avoid DNS lookups
    "stop",                  // avoid shutdown state
};

// RPC commands which are safe for fuzzing.
const std::vector<std::string> RPC_COMMANDS_SAFE_FOR_FUZZING{
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
    "disconnectnode",
    "echo",
    "echojson",
    "estimaterawfee",
    "estimatesmartfee",
    "finalizepsbt",
    "generate",
    "generateblock",
    "getaddednodeinfo",
    "getbestblockhash",
    "getblock",
    "getblockchaininfo",
    "getblockcount",
    "getblockfilter",
    "getblockhash",
    "getblockheader",
    "getblockstats",
    "getblocktemplate",
    "getchaintips",
    "getchaintxstats",
    "getconnectioncount",
    "getdescriptorinfo",
    "getdifficulty",
    "getindexinfo",
    "getmemoryinfo",
    "getmempoolancestors",
    "getmempooldescendants",
    "getmempoolentry",
    "getmempoolinfo",
    "getmininginfo",
    "getnettotals",
    "getnetworkhashps",
    "getnetworkinfo",
    "getnodeaddresses",
    "getpeerinfo",
    "getrawmempool",
    "getrawtransaction",
    "getrpcinfo",
    "gettxout",
    "gettxoutsetinfo",
    "help",
    "invalidateblock",
    "joinpsbts",
    "listbanned",
    "logging",
    "mockscheduler",
    "ping",
    "preciousblock",
    "pruneblockchain",
    "reconsiderblock",
    "scantxoutset",
    "sendrawtransaction",
    "setmocktime",
    "setnetworkactive",
    "signmessagewithprivkey",
    "signrawtransactionwithkey",
    "submitblock",
    "submitheader",
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

std::string ConsumeScalarRPCArgument(FuzzedDataProvider& fuzzed_data_provider)
{
    const size_t max_string_length = 4096;
    std::string r;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // string argument
            r = fuzzed_data_provider.ConsumeRandomLengthString(max_string_length);
        },
        [&] {
            // base64 argument
            r = EncodeBase64(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // hex argument
            r = HexStr(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // bool argument
            r = fuzzed_data_provider.ConsumeBool() ? "true" : "false";
        },
        [&] {
            // range argument
            r = "[" + ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>()) + "," + ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>()) + "]";
        },
        [&] {
            // integral argument (int64_t)
            r = ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>());
        },
        [&] {
            // integral argument (uint64_t)
            r = ToString(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
        },
        [&] {
            // floating point argument
            r = strprintf("%f", fuzzed_data_provider.ConsumeFloatingPoint<double>());
        },
        [&] {
            // tx destination argument
            r = EncodeDestination(ConsumeTxDestination(fuzzed_data_provider));
        },
        [&] {
            // uint160 argument
            r = ConsumeUInt160(fuzzed_data_provider).ToString();
        },
        [&] {
            // uint256 argument
            r = ConsumeUInt256(fuzzed_data_provider).ToString();
        },
        [&] {
            // base32 argument
            r = EncodeBase32(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // base58 argument
            r = EncodeBase58(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length)));
        },
        [&] {
            // base58 argument with checksum
            r = EncodeBase58Check(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length)));
        },
        [&] {
            // hex encoded block
            std::optional<CBlock> opt_block = ConsumeDeserializable<CBlock>(fuzzed_data_provider);
            if (!opt_block) {
                return;
            }
            CDataStream data_stream{SER_NETWORK, PROTOCOL_VERSION};
            data_stream << *opt_block;
            r = HexStr(data_stream);
        },
        [&] {
            // hex encoded block header
            std::optional<CBlockHeader> opt_block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
            if (!opt_block_header) {
                return;
            }
            CDataStream data_stream{SER_NETWORK, PROTOCOL_VERSION};
            data_stream << *opt_block_header;
            r = HexStr(data_stream);
        },
        [&] {
            // hex encoded tx
            std::optional<CMutableTransaction> opt_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
            if (!opt_tx) {
                return;
            }
            CDataStream data_stream{SER_NETWORK, fuzzed_data_provider.ConsumeBool() ? PROTOCOL_VERSION : (PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS)};
            data_stream << *opt_tx;
            r = HexStr(data_stream);
        },
        [&] {
            // base64 encoded psbt
            std::optional<PartiallySignedTransaction> opt_psbt = ConsumeDeserializable<PartiallySignedTransaction>(fuzzed_data_provider);
            if (!opt_psbt) {
                return;
            }
            CDataStream data_stream{SER_NETWORK, PROTOCOL_VERSION};
            data_stream << *opt_psbt;
            r = EncodeBase64({data_stream.begin(), data_stream.end()});
        },
        [&] {
            // base58 encoded key
            const std::vector<uint8_t> random_bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(32);
            CKey key;
            key.Set(random_bytes.begin(), random_bytes.end(), fuzzed_data_provider.ConsumeBool());
            if (!key.IsValid()) {
                return;
            }
            r = EncodeSecret(key);
        },
        [&] {
            // hex encoded pubkey
            const std::vector<uint8_t> random_bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(32);
            CKey key;
            key.Set(random_bytes.begin(), random_bytes.end(), fuzzed_data_provider.ConsumeBool());
            if (!key.IsValid()) {
                return;
            }
            r = HexStr(key.GetPubKey());
        });
    return r;
}

std::string ConsumeArrayRPCArgument(FuzzedDataProvider& fuzzed_data_provider)
{
    std::vector<std::string> scalar_arguments;
    while (fuzzed_data_provider.ConsumeBool()) {
        scalar_arguments.push_back(ConsumeScalarRPCArgument(fuzzed_data_provider));
    }
    return "[\"" + Join(scalar_arguments, "\",\"") + "\"]";
}

std::string ConsumeRPCArgument(FuzzedDataProvider& fuzzed_data_provider)
{
    return fuzzed_data_provider.ConsumeBool() ? ConsumeScalarRPCArgument(fuzzed_data_provider) : ConsumeArrayRPCArgument(fuzzed_data_provider);
}

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
    const std::vector<std::string> supported_rpc_commands = rpc_testing_setup->GetRPCCommands();
    for (const std::string& rpc_command : supported_rpc_commands) {
        const bool safe_for_fuzzing = std::find(RPC_COMMANDS_SAFE_FOR_FUZZING.begin(), RPC_COMMANDS_SAFE_FOR_FUZZING.end(), rpc_command) != RPC_COMMANDS_SAFE_FOR_FUZZING.end();
        const bool not_safe_for_fuzzing = std::find(RPC_COMMANDS_NOT_SAFE_FOR_FUZZING.begin(), RPC_COMMANDS_NOT_SAFE_FOR_FUZZING.end(), rpc_command) != RPC_COMMANDS_NOT_SAFE_FOR_FUZZING.end();
        if (!(safe_for_fuzzing || not_safe_for_fuzzing)) {
            std::cerr << "Error: RPC command \"" << rpc_command << "\" not found in RPC_COMMANDS_SAFE_FOR_FUZZING or RPC_COMMANDS_NOT_SAFE_FOR_FUZZING. Please update " << __FILE__ << ".\n";
            std::terminate();
        }
        if (safe_for_fuzzing && not_safe_for_fuzzing) {
            std::cerr << "Error: RPC command \"" << rpc_command << "\" found in *both* RPC_COMMANDS_SAFE_FOR_FUZZING and RPC_COMMANDS_NOT_SAFE_FOR_FUZZING. Please update " << __FILE__ << ".\n";
            std::terminate();
        }
    }
    for (const std::string& rpc_command : RPC_COMMANDS_SAFE_FOR_FUZZING) {
        const bool supported_rpc_command = std::find(supported_rpc_commands.begin(), supported_rpc_commands.end(), rpc_command) != supported_rpc_commands.end();
        if (!supported_rpc_command) {
            std::cerr << "Error: Unknown RPC command \"" << rpc_command << "\" found in RPC_COMMANDS_SAFE_FOR_FUZZING. Please update " << __FILE__ << ".\n";
            std::terminate();
        }
    }
    for (const std::string& rpc_command : RPC_COMMANDS_NOT_SAFE_FOR_FUZZING) {
        const bool supported_rpc_command = std::find(supported_rpc_commands.begin(), supported_rpc_commands.end(), rpc_command) != supported_rpc_commands.end();
        if (!supported_rpc_command) {
            std::cerr << "Error: Unknown RPC command \"" << rpc_command << "\" found in RPC_COMMANDS_NOT_SAFE_FOR_FUZZING. Please update " << __FILE__ << ".\n";
            std::terminate();
        }
    }
    const char* limit_to_rpc_command_env = std::getenv("LIMIT_TO_RPC_COMMAND");
    if (limit_to_rpc_command_env != nullptr) {
        g_limit_to_rpc_command = std::string{limit_to_rpc_command_env};
    }
}

FUZZ_TARGET_INIT(rpc, initialize_rpc)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    const std::string rpc_command = fuzzed_data_provider.ConsumeRandomLengthString(64);
    if (!g_limit_to_rpc_command.empty() && rpc_command != g_limit_to_rpc_command) {
        return;
    }
    const bool safe_for_fuzzing = std::find(RPC_COMMANDS_SAFE_FOR_FUZZING.begin(), RPC_COMMANDS_SAFE_FOR_FUZZING.end(), rpc_command) != RPC_COMMANDS_SAFE_FOR_FUZZING.end();
    if (!safe_for_fuzzing) {
        return;
    }
    std::vector<std::string> arguments;
    while (fuzzed_data_provider.ConsumeBool()) {
        arguments.push_back(ConsumeRPCArgument(fuzzed_data_provider));
    }
    try {
        rpc_testing_setup->CallRPC(rpc_command, arguments);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
}
