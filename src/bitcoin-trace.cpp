// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chainparamsbase.h>
#include <chainparams.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <compat/compat.h>
#include <init/common.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/signalinterrupt.h>
#include <util/translation.h>

#include <csignal>

static const char* const HELP_USAGE{R"(
bitcoin-trace is a test program for tracing a bitcoin-node process via IPC.

Usage:
  bitcoin-trace [options]
)"};

static const char* HELP_EXAMPLES{R"(
Examples:
  # Start separate bitcoin-node that bitcoin-trace can connect to.
  bitcoin-node -regtest -ipcbind=unix

  # Run printing traces.
  bitcoin-trace -regtest -debug

  # Run with debug output.
  bitcoin-trace -regtest -debug
)"};

const TranslateFn G_TRANSLATION_FUN{nullptr};

static std::optional<util::SignalInterrupt> g_shutdown;
std::string g_shutdown_message;

void HandleCtrlC(int)
{
    // (void)! needed to suppress -Wunused-result warning from GCC
    (void)!write(STDOUT_FILENO, g_shutdown_message.data(), g_shutdown_message.size());
    // Return value is intentionally ignored because there is not a better way
    // of handling this failure in a signal handler.
    (void)(*Assert(g_shutdown))();
}

static void AddArgs(ArgsManager& args)
{
    SetupHelpOptions(args);
    SetupChainParamsBaseOptions(args);
    args.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-ipcconnect=<address>", "Connect to bitcoin-node process in the background to perform online operations. Valid <address> values are 'unix' to connect to the default socket, 'unix:<socket path>' to connect to a socket at a nonstandard path. Default value: unix", ArgsManager::ALLOW_ANY, OptionsCategory::IPC);
    init::AddLoggingArgs(args);
}

MAIN_FUNCTION
{
    ArgsManager& args = gArgs;
    AddArgs(args);
    std::string error_message;
    if (!args.ParseParameters(argc, argv, error_message)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error_message);
        return EXIT_FAILURE;
    }
    if (!args.ReadConfigFiles(error_message, true)) {
        tfm::format(std::cerr, "Error reading config files: %s\n", error_message);
        return EXIT_FAILURE;
    }
    if (HelpRequested(args) || args.IsArgSet("-version")) {
        std::string output{strprintf("%s bitcoin-trace version", CLIENT_NAME) + " " + FormatFullVersion() + "\n"};
        if (args.IsArgSet("-version")) {
            output += FormatParagraph(LicenseInfo());
        } else {
            output += HELP_USAGE;
            output += args.GetHelpMessage();
            output += HELP_EXAMPLES;
        }
        tfm::format(std::cout, "%s", output);
        return EXIT_SUCCESS;
    }
    if (!CheckDataDirOption(args)) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", args.GetArg("-datadir", ""));
        return EXIT_FAILURE;
    }
    SelectParams(args.GetChainType());

    // Set logging options but override -printtoconsole default to depend on -debug rather than -daemon
    init::SetLoggingOptions(args);
    if (auto result{init::SetLoggingCategories(args)}; !result) {
        tfm::format(std::cerr, "Error: %s\n", util::ErrorString(result).original);
        return EXIT_FAILURE;
    }
    if (auto result{init::SetLoggingLevel(args)}; !result) {
        tfm::format(std::cerr, "Error: %s\n", util::ErrorString(result).original);
        return EXIT_FAILURE;
    }
    LogInstance().m_print_to_console = args.GetBoolArg("-printtoconsole", LogInstance().GetCategoryMask());
    if (!init::StartLogging(args)) {
        tfm::format(std::cerr, "Error: StartLogging failed\n");
        return EXIT_FAILURE;
    }

    // Connect to bitcoin-node process, or fail and print an error.
    std::unique_ptr<interfaces::Init> trace_init{interfaces::MakeBasicInit("bitcoin-trace", argc > 0 ? argv[0] : "")};
    assert(trace_init);
    std::unique_ptr<interfaces::Init> node_init;
    try {
        std::string address{args.GetArg("-ipcconnect", "unix")};
        node_init = trace_init->ipc()->connectAddress(address);
    } catch (const std::exception& exception) {
        tfm::format(std::cerr, "Error: %s\n", exception.what());
        tfm::format(std::cerr, "Probably bitcoin-node is not running or not listening on a unix socket. Can be started with:\n\n");
        tfm::format(std::cerr, "    bitcoin-node -chain=%s -ipcbind=unix\n", args.GetChainTypeString());
        return EXIT_FAILURE;
    }
    assert(node_init);
    tfm::format(std::cout, "Connected to bitcoin-node\n");
    std::unique_ptr<interfaces::Tracing> tracing{node_init->makeTracing()};
    assert(tracing);

    g_shutdown.emplace();
    g_shutdown_message = "SIGINT received — starting to shut down.\n";
    struct sigaction sa{};
    sa.sa_handler = HandleCtrlC;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);

    class Callback : public interfaces::UtxoCacheTrace
    {
        void add(const interfaces::UtxoInfo& coin) override
        {
            tfm::format(std::cout, "-- add utxo: outpoint hash %s index %s height %s value %s is_coinbase %s\n", coin.outpoint_hash.ToString(), coin.outpoint_n, coin.height, coin.value, coin.is_coinbase);
        }
        void spend(const interfaces::UtxoInfo& coin) override
        {
            tfm::format(std::cout, "-- spend utxo: outpoint hash %s index %s height %s value %s is_coinbase %s\n", coin.outpoint_hash.ToString(), coin.outpoint_n, coin.height, coin.value, coin.is_coinbase);
        }
        void uncache(const interfaces::UtxoInfo& coin) override
        {
            tfm::format(std::cout, "-- uncache utxo: outpoint hash %s index %s height %s value %s is_coinbase %s\n", coin.outpoint_hash.ToString(), coin.outpoint_n, coin.height, coin.value, coin.is_coinbase);
        }
    };

    auto handler{tracing->traceUtxoCache(std::make_unique<Callback>())};
    if (!g_shutdown->wait()) {
        tfm::format(std::cerr, "Error waiting for shutdown signal.\n");
    }

    return EXIT_SUCCESS;
}
