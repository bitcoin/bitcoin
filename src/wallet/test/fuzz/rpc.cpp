// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep
#include <interfaces/wallet.h>
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
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/time.h>
#include <wallet/context.h>
#include <wallet/rpc/wallet.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace wallet {
namespace {
struct WalletRPCFuzzTestingSetup : public TestingSetup {
    fs::path m_fuzz_wallets_dir;
    std::unique_ptr<interfaces::WalletLoader> m_wallet_loader;

    WalletRPCFuzzTestingSetup(const ChainType chain_type, TestOpts opts)
        : TestingSetup{chain_type, opts}
    {
        m_fuzz_wallets_dir = m_path_root / "fuzz-wallets";
        fs::create_directories(m_fuzz_wallets_dir);
        gArgs.ForceSetArg("-walletdir", fs::PathToString(m_fuzz_wallets_dir));

        m_wallet_loader = interfaces::MakeWalletLoader(*m_node.chain, *Assert(m_node.args));
        m_node.wallet_loader = m_wallet_loader.get();
        m_wallet_loader->registerRpcs();
    }

    WalletContext& GetWalletContext()
    {
        return *Assert(m_wallet_loader->context());
    }

    void CallRPC(const std::string& rpc_method, const std::vector<std::string>& arguments, const std::optional<std::string>& wallet_name)
    {
        JSONRPCRequest request;
        request.context = &m_node;
        if (wallet_name) {
            request.URI = "/wallet/" + *wallet_name;
        }
        request.strMethod = rpc_method;
        try {
            request.params = RPCConvertValues(rpc_method, arguments);
        } catch (const std::runtime_error&) {
            return;
        }

        tableRPC.execute(request);
    }
};

WalletRPCFuzzTestingSetup* g_setup = nullptr;
std::string g_limit_to_rpc_command;

//! Keep wallet RPC fuzz inputs small to avoid oversized parameter lists.
constexpr size_t MAX_FUZZ_RPC_ARGUMENTS{25};

// RPC commands which are not appropriate for fuzzing.
const std::vector<std::string> WALLET_RPC_COMMANDS_NOT_SAFE_FOR_FUZZING{
    "backupwallet",          // writes to an arbitrary path supplied by fuzz input
    "createwallet",          // wallet lifecycle is managed by the fuzz target
    "createwalletdescriptor", // can expand the keypool for new descriptors
    "encryptwallet",         // prohibitively slow encryption and keypool flush
    "importdescriptors",     // can trigger a wallet rescan
    "keypoolrefill",         // prohibitively slow keypool top-up
    "loadwallet",            // loads an arbitrary wallet path supplied by fuzz input
    "migratewallet",         // prohibitively slow migration
    "rescanblockchain",      // prohibitively slow rescan
    "restorewallet",         // reads from an arbitrary path supplied by fuzz input
    "unloadwallet",          // interferes with the per-iteration fuzz wallets
    "walletpassphrase",      // encryption-related and not useful for unencrypted fuzz wallets
    "walletpassphrasechange", // encryption-related and not useful for unencrypted fuzz wallets
};

// RPC commands which are safe for fuzzing.
const std::vector<std::string> WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING{
    "abandontransaction",
    "abortrescan",
    "addhdkey",
    "bumpfee",
    "fundrawtransaction",
    "getaddressinfo",
    "getaddressesbylabel",
    "getbalance",
    "getbalances",
    "gethdkeys",
    "getnewaddress",
    "getrawchangeaddress",
    "getreceivedbyaddress",
    "getreceivedbylabel",
    "gettransaction",
    "getwalletinfo",
    "importprunedfunds",
    "listaddressgroupings",
    "listdescriptors",
    "listlabels",
    "listlockunspent",
    "listreceivedbyaddress",
    "listreceivedbylabel",
    "listsinceblock",
    "listtransactions",
    "listunspent",
    "listwalletdir",
    "listwallets",
    "lockunspent",
    "psbtbumpfee",
    "removeprunedfunds",
    "send",
    "sendall",
    "sendmany",
    "sendtoaddress",
    "setlabel",
    "setwalletflag",
    "signmessage",
    "signrawtransactionwithwallet",
    "simulaterawtransaction",
    "walletcreatefundedpsbt",
#ifdef ENABLE_EXTERNAL_SIGNER
    "walletdisplayaddress",
#endif
    "walletlock",
    "walletprocesspsbt",
};

const std::unordered_set<std::string> WALLET_MANAGER_RPC_COMMANDS{
    "listwalletdir",
    "listwallets",
};

std::vector<std::string> GetSupportedWalletRPCCommands()
{
    std::vector<std::string> supported_rpc_commands;
    for (const auto& command : GetWalletRPCCommands()) {
        supported_rpc_commands.emplace_back(command.name);
    }
    return supported_rpc_commands;
}

void SetupMinimalFuzzWallet(CWallet& wallet)
{
    LOCK(wallet.cs_wallet);
    wallet.m_keypool_size = 1;

    FlatSigningProvider keys;
    std::string error;
    const std::string descriptor{
        "wpkh([5aa9973a/66h/4h/2h]tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk/0/*)"};
    auto parsed_descs = Parse(descriptor, keys, error, /*require_checksum=*/false);
    if (parsed_descs.empty()) return;

    WalletDescriptor w_desc{std::move(parsed_descs.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0};
    const auto spk_manager = wallet.AddWalletDescriptor(w_desc, keys, /*label=*/"", /*internal=*/false);
    if (!spk_manager) return;
    wallet.AddActiveScriptPubKeyMan(spk_manager->get().GetID(), *Assert(w_desc.descriptor->GetOutputType()), /*internal=*/false);
}

struct FuzzWallet {
    std::string name;
    fs::path path;
};

class FuzzWallets {
    WalletContext& m_context;

public:
    std::vector<FuzzWallet> wallets;

    explicit FuzzWallets(WalletContext& context) : m_context{context} {}

    ~FuzzWallets()
    {
        for (const FuzzWallet& fuzz_wallet : wallets) {
            UnloadAndDelete(fuzz_wallet);
        }
    }

    void Create(FuzzedDataProvider& fuzzed_data_provider)
    {
        static uint64_t wallet_counter{0};
        const size_t num_wallets{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 3)};
        wallets.reserve(num_wallets);

        for (size_t i = 0; i < num_wallets; ++i) {
            const std::string wallet_name{strprintf("fuzz-wallet-%llu-%zu", wallet_counter++, i)};
            const fs::path wallet_path{m_fuzz_wallets_dir / fs::PathFromString(wallet_name)};

            DatabaseOptions options;
            options.require_create = true;
            options.create_flags = WALLET_FLAG_DESCRIPTORS | WALLET_FLAG_BLANK_WALLET;
            DatabaseStatus status;
            bilingual_str error;
            std::vector<bilingual_str> warnings;
            const std::shared_ptr<CWallet> wallet = CreateWallet(m_context, wallet_name, /*load_on_start=*/std::nullopt, options, status, error, warnings);
            if (!wallet) continue;

            SetupMinimalFuzzWallet(*wallet);
            wallets.push_back({wallet_name, wallet_path});
        }
    }

    std::optional<std::string> PickWalletName(FuzzedDataProvider& fuzzed_data_provider) const
    {
        if (wallets.empty()) return std::nullopt;
        return wallets[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, wallets.size() - 1)].name;
    }

private:
    const fs::path& m_fuzz_wallets_dir{g_setup->m_fuzz_wallets_dir};

    void UnloadAndDelete(const FuzzWallet& fuzz_wallet)
    {
        std::shared_ptr<CWallet> wallet = GetWallet(m_context, fuzz_wallet.name);
        if (wallet) {
            std::vector<bilingual_str> warnings;
            RemoveWallet(m_context, wallet, /*load_on_start=*/std::nullopt, warnings);
            WaitForDeleteWallet(std::move(wallet));
        }
        std::error_code ec;
        fs::remove(fuzz_wallet.path, ec);
    }
};

void initialize_wallet_rpc()
{
    static const auto setup = MakeNoLogFileContext<WalletRPCFuzzTestingSetup>(ChainType::REGTEST, {.extra_args{"-keypool=1", "-unsafesqlitesync"}});
    g_setup = setup.get();
    SetRPCWarmupFinished();
    fuzzrpc::ValidateRpcCommands(WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING, WALLET_RPC_COMMANDS_NOT_SAFE_FOR_FUZZING, GetSupportedWalletRPCCommands(), __FILE__);
    const char* limit_to_rpc_command_env = std::getenv("LIMIT_TO_RPC_COMMAND");
    if (limit_to_rpc_command_env != nullptr) {
        g_limit_to_rpc_command = std::string{limit_to_rpc_command_env};
    }
}

} // namespace

FUZZ_TARGET(wallet_rpc, .init = initialize_wallet_rpc)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    NodeClockContext clock_ctx{ConsumeTime(fuzzed_data_provider)};

    FuzzWallets fuzz_wallets{g_setup->GetWalletContext()};
    fuzz_wallets.Create(fuzzed_data_provider);
    if (fuzz_wallets.wallets.empty()) return;

    const std::string rpc_command = fuzzed_data_provider.ConsumeRandomLengthString(64);
    if (!g_limit_to_rpc_command.empty() && rpc_command != g_limit_to_rpc_command) {
        return;
    }
    const bool safe_for_fuzzing = std::find(WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING.begin(), WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING.end(), rpc_command) != WALLET_RPC_COMMANDS_SAFE_FOR_FUZZING.end();
    if (!safe_for_fuzzing) {
        return;
    }

    bool good_data{true};
    std::vector<std::string> arguments;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), MAX_FUZZ_RPC_ARGUMENTS)
    {
        arguments.push_back(fuzzrpc::ConsumeRPCArgument(fuzzed_data_provider, good_data));
    }

    const std::optional<std::string> wallet_endpoint = WALLET_MANAGER_RPC_COMMANDS.contains(rpc_command) ? std::nullopt : fuzz_wallets.PickWalletName(fuzzed_data_provider);
    try {
        g_setup->CallRPC(rpc_command, arguments, wallet_endpoint);
    } catch (const UniValue& json_rpc_error) {
        const std::string error_msg{json_rpc_error.find_value("message").get_str()};
        if (error_msg.starts_with("Internal bug detected")) {
            // Only allow the intentional internal bug
            assert(error_msg.find("trigger_internal_bug") != std::string::npos);
        }
    }
}

} // namespace wallet
