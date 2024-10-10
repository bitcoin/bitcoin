// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/rpc.h>

enum class ChainType;

namespace {
struct RPCWalletFuzzTestingSetup : public TestingSetup {
    RPCWalletFuzzTestingSetup(const ChainType chain_type, TestOpts opts) : TestingSetup{chain_type, opts}
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

    std::vector<std::string> GetWalletRPCCommands() const
    {
        Span<const CRPCCommand> tableWalletRPC = wallet::GetWalletRPCCommands();
        std::vector<std::string> supported_rpc_commands;
        for (const auto& c : tableWalletRPC) {
            tableRPC.appendCommand(c.name, &c);
            supported_rpc_commands.push_back(c.name);
        }
        return supported_rpc_commands;
    }
};

RPCWalletFuzzTestingSetup* rpc_wallet_testing_setup = nullptr;
std::string g_limit_to_rpc_command_wallet;

// RPC commands which are safe for fuzzing.
const std::vector<std::string> WALLET_RPC_COMMANDS_NOT_SAFE_FOR_FUZZING{
    "importwallet",
    "loadwallet",
};
// RPC commands which are safe for fuzzing.
const std::vector<std::string> WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING{
    "getbalances",
    "keypoolrefill",
    "newkeypool",
    "listaddressgroupings",
    "getwalletinfo",
    "createwalletdescriptor",
    "getnewaddress",
    "getrawchangeaddress",
    "setlabel",
    "fundrawtransaction",
    "abandontransaction",
    "abortrescan",
    "addmultisigaddress",
    "backupwallet",
    "bumpfee",
    "psbtbumpfee",
    "createwallet",
    "restorewallet",
    "dumpprivkey",
    "importmulti",
    "importdescriptors",
    "listdescriptors",
    "dumpwallet",
    "encryptwallet",
    "getaddressesbylabel",
    "listlabels",
    "walletdisplayaddress",
    "importprivkey",
    "importaddress",
    "importprunedfunds",
    "removeprunedfunds",
    "importpubkey",
    "getaddressinfo",
    "getbalance",
    "gethdkeys",
    "getreceivedbyaddress",
    "getreceivedbylabel",
    "gettransaction",
    "getunconfirmedbalance",
    "lockunspent",
    "listlockunspent",
    "listunspent",
    "walletpassphrase",
    "walletpassphrasechange",
    "walletlock",
    "signmessage",
    "sendtoaddress",
    "sendmany",
    "settxfee",
    "signrawtransactionwithwallet",
    "psbtbumpfee",
    "bumpfee",
    "send",
    "sendall",
    "walletprocesspsbt",
    "walletcreatefundedpsbt",
    "listreceivedbyaddress",
    "listreceivedbylabel",
    "listtransactions",
    "listsinceblock",
    "rescanblockchain",
    "listwalletdir",
    "listwallets",
    "setwalletflag",
    "createwallet",
    "unloadwallet",
    "sethdseed",
    "upgradewallet",
    "simulaterawtransaction",
    "migratewallet",
};


RPCWalletFuzzTestingSetup* InitializeRPCWalletFuzzTestingSetup()
{
    static const auto setup = MakeNoLogFileContext<RPCWalletFuzzTestingSetup>();
    SetRPCWarmupFinished();
    return setup.get();
}
}; // namespace


void FuzzInitWalletRPC()
{
    rpc_wallet_testing_setup = InitializeRPCWalletFuzzTestingSetup();
    const std::vector<std::string> supported_rpc_commands = rpc_wallet_testing_setup->GetWalletRPCCommands();
    RPCFuzz::initialize(WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING, WALLET_RPC_COMMANDS_NOT_SAFE_FOR_FUZZING, supported_rpc_commands);
}

void ExecuteFuzzCommandsForWalletRPC(std::vector<std::string> list_of_safe_commands, Span<const unsigned char> buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    bool good_data{true};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    const std::string rpc_command = fuzzed_data_provider.ConsumeRandomLengthString(64);
    if (!g_limit_to_rpc_command_wallet.empty() && rpc_command != g_limit_to_rpc_command_wallet) {
        return;
    }
    const bool safe_for_fuzzing = std::find(list_of_safe_commands.begin(), list_of_safe_commands.end(), rpc_command) != list_of_safe_commands.end();
    if (!safe_for_fuzzing) {
        return;
    }
    std::vector<std::string> arguments;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 100)
    {
        arguments.push_back(RPCFuzz::ConsumeRPCArgument(fuzzed_data_provider, good_data));
    }
    try {
        rpc_wallet_testing_setup->CallRPC(rpc_command, arguments);
    } catch (const UniValue& json_rpc_error) {
        const std::string error_msg{json_rpc_error.find_value("message").get_str()};
        if (error_msg.starts_with("Internal bug detected")) {
            // Only allow the intentional internal bug
            assert(error_msg.find("trigger_internal_bug") != std::string::npos);
        }
    }

}

FUZZ_TARGET(wallet_rpc, .init = FuzzInitWalletRPC)
{
    ExecuteFuzzCommandsForWalletRPC(WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING, buffer);
}

