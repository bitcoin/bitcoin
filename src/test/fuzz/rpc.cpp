// Copyright (c) 2021-2022 The Bitcoin Core developers
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
#include <util/chaintype.h>
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
    RPCFuzzTestingSetup(const ChainType chain_type, const std::vector<const char*>& extra_args) : TestingSetup{chain_type, extra_args}
    {
    }

    void CallRPC(const std::string& rpc_method, const std::vector<std::string>& arguments)
    {
        node::JSONRPCRequest request;
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
    "dumpwallet", // avoid writing to disk
    "enumeratesigners",
    "echoipc",              // avoid assertion failure (Assertion `"EnsureAnyNodeContext(request.context).init" && check' failed.)
    "generatetoaddress",    // avoid prohibitively slow execution (when `num_blocks` is large)
    "generatetodescriptor", // avoid prohibitively slow execution (when `nblocks` is large)
    "gettxoutproof",        // avoid prohibitively slow execution
    "importmempool", // avoid reading from disk
    "importwallet", // avoid reading from disk
    "loadtxoutset",   // avoid reading from disk
    "loadwallet",   // avoid reading from disk
    "savemempool",           // disabled as a precautionary measure: may take a file path argument in the future
    "setban",                // avoid DNS lookups
    "stop",                  // avoid shutdown state
    "getnevmblockchaininfo",
};

// RPC commands which are safe for fuzzing.
const std::vector<std::string> RPC_COMMANDS_SAFE_FOR_FUZZING{
    "mnauth",
    "mnsync",
    "spork",
    "masternode_connect",
    "masternode_list",
    "masternode_winners",
    "masternode_payments",
    "masternode_count",
    "masternode_winner",
    "masternode_status",
    "masternode_current",
    "masternode_sign",
    "masternode_verify",
    "getgovernanceinfo",
    "getsuperblockbudget",
    "gobject_count",
    "gobject_deserialize",
    "gobject_check",
    "gobject_get",
    "gobject_list",
    "gobject_diff",
    "voteraw",
    "getchainlocks",
    "bls_generate",
    "bls_fromsecret",
    "protx_list",
    "protx_info",
    "quorum_list",
    "quorum_info",
    "quorum_dkgstatus",
    "quorum_memberof",
    "quorum_selectquorum",
    "quorum_dkgsimerror",
    "quorum_hasrecsig",
    "quorum_getrecsig",
    "quorum_verify",
    "quorum_isconflicting",
    "quorum_sign",
    "gobject_getcurrentvotes",
    "gobject_submit",
    "createauxblock",
    "submitauxblock",
    "syscoingettxroots",
    "syscoingetspvproof",
    "syscoindecoderawtransaction",
    "getnevmblobdata",
    "listnevmblobdata",
    "syscoinstopgeth",
    "syscoinstartgeth",
    "syscoincheckmint",
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
    "getprioritisedtransactions",
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
    "syscoincreatenevmblob",
    "syscoincreaterawnevmblob",
    "protx_update_service",
    "protx_register",
    "protx_register_prepare",
    "protx_update_registrar",
    "protx_list_wallet",
    "settestparams",
    "signmessagebech32",
    "getauxblock",
    "protx_info_wallet",
    "protx_register_fund",
    "protx_register_submit",
    "protx_revoke",
    "gobject_vote_alias",
    "gobject_vote_many",
    "gobject_prepare",
    "gobject_list_prepared",
    "masternode_outputs"

};

std::string ConsumeScalarRPCArgument(FuzzedDataProvider& fuzzed_data_provider)
{
    const size_t max_string_length = 4096;
    const size_t max_base58_bytes_length{64};
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
            r = EncodeBase58(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_base58_bytes_length)));
        },
        [&] {
            // base58 argument with checksum
            r = EncodeBase58Check(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_base58_bytes_length)));
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
            // SYSCOIN
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
            r = EncodeBase64(data_stream);
        },
        [&] {
            // base58 encoded key
            CKey key = ConsumePrivateKey(fuzzed_data_provider);
            if (!key.IsValid()) {
                return;
            }
            r = EncodeSecret(key);
        },
        [&] {
            // hex encoded pubkey
            CKey key = ConsumePrivateKey(fuzzed_data_provider);
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
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100) {
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
    const char* limit_to_rpc_command_env = std::getenv("LIMIT_TO_RPC_COMMAND");
    if (limit_to_rpc_command_env != nullptr) {
        g_limit_to_rpc_command = std::string{limit_to_rpc_command_env};
    }
}

FUZZ_TARGET(rpc, .init = initialize_rpc)
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
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100) {
        arguments.push_back(ConsumeRPCArgument(fuzzed_data_provider));
    }
    try {
        rpc_testing_setup->CallRPC(rpc_command, arguments);
    } catch (const UniValue& json_rpc_error) {
        const std::string error_msg{json_rpc_error.find_value("message").get_str()};
        // Once c++20 is allowed, starts_with can be used.
        // if (error_msg.starts_with("Internal bug detected")) {
        if (0 == error_msg.rfind("Internal bug detected", 0)) {
            // Only allow the intentional internal bug
            assert(error_msg.find("trigger_internal_bug") != std::string::npos);
        }
    }
}
