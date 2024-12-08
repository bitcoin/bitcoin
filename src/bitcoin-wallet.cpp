// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <bitcoin-wallet_settings.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <compat/compat.h>
#include <interfaces/init.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <tinyformat.h>
#include <util/exception.h>
#include <util/translation.h>
#include <wallet/wallettool.h>

#include <exception>
#include <functional>
#include <string>
#include <tuple>

using util::Join;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void SetupWalletToolArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);
    SetupChainParamsBaseOptions(argsman);

    VersionSetting::Register(argsman);
    DatadirSetting::Register(argsman);
    WalletSetting::Register(argsman);
    DumpfileSetting::Register(argsman);
    DebugSetting::Register(argsman);
    DescriptorsSetting::Register(argsman);
    LegacySetting::Register(argsman);
    FormatSetting::Register(argsman);
    PrinttoconsoleSetting::Register(argsman);
    WithinternalbdbSetting::Register(argsman);

    argsman.AddCommand("info", "Get wallet info");
    argsman.AddCommand("create", "Create new wallet file");
    argsman.AddCommand("salvage", "Attempt to recover private keys from a corrupt wallet. Warning: 'salvage' is experimental.");
    argsman.AddCommand("dump", "Print out all of the wallet key-value records");
    argsman.AddCommand("createfromdump", "Create new wallet file from dumped records");
}

static std::optional<int> WalletAppInit(ArgsManager& args, int argc, char* argv[])
{
    SetupWalletToolArgs(args);
    std::string error_message;
    if (!args.ParseParameters(argc, argv, error_message)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error_message);
        return EXIT_FAILURE;
    }
    const bool missing_args{argc < 2};
    if (missing_args || HelpRequested(args) || VersionSetting::Get(args)) {
        std::string strUsage = strprintf("%s bitcoin-wallet utility version", CLIENT_NAME) + " " + FormatFullVersion() + "\n";

        if (VersionSetting::Get(args)) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                "bitcoin-wallet is an offline tool for creating and interacting with " CLIENT_NAME " wallet files.\n\n"
                "By default bitcoin-wallet will act on wallets in the default mainnet wallet directory in the datadir.\n\n"
                "To change the target wallet, use the -datadir, -wallet and (test)chain selection arguments.\n"
                "\n"
                "Usage: bitcoin-wallet [options] <command>\n"
                "\n";
            strUsage += "\n" + args.GetHelpMessage();
        }
        tfm::format(std::cout, "%s", strUsage);
        if (missing_args) {
            tfm::format(std::cerr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // check for printtoconsole, allow -debug
    LogInstance().m_print_to_console = PrinttoconsoleSetting::Get(args, DebugSetting::Get(args));

    if (!CheckDataDirOption(args)) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", DatadirSetting::Get(args));
        return EXIT_FAILURE;
    }
    // Check for chain settings (Params() calls are only valid after this clause)
    SelectParams(args.GetChainType());

    return std::nullopt;
}

MAIN_FUNCTION
{
    ArgsManager& args = gArgs;
#ifdef WIN32
    common::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif

    int exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeWalletInit(argc, argv, exit_status);
    if (!init) {
        return exit_status;
    }

    SetupEnvironment();
    RandomInit();
    try {
        if (const auto maybe_exit{WalletAppInit(args, argc, argv)}) return *maybe_exit;
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "WalletAppInit()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "WalletAppInit()");
        return EXIT_FAILURE;
    }

    const auto command = args.GetCommand();
    if (!command) {
        tfm::format(std::cerr, "No method provided. Run `bitcoin-wallet -help` for valid methods.\n");
        return EXIT_FAILURE;
    }
    if (command->args.size() != 0) {
        tfm::format(std::cerr, "Error: Additional arguments provided (%s). Methods do not take arguments. Please refer to `-help`.\n", Join(command->args, ", "));
        return EXIT_FAILURE;
    }

    ECC_Context ecc_context{};
    if (!wallet::WalletTool::ExecuteWalletToolFunc(args, command->command)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
