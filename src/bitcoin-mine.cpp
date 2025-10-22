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
#include <consensus/merkle.h>
#include <cstdlib>
#include <init/common.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <key_io.h>
#include <logging.h>
#include <pow.h>
#include <tinyformat.h>
#include <util/translation.h>

const uint64_t DEFAULT_MAX_TRIES{10'000};

static const char* const HELP_USAGE{R"(
bitcoin-mine is a test program for interacting with bitcoin-node via IPC.

Usage:
  bitcoin-mine [options]
)"};

static const char* HELP_EXAMPLES{R"(
Examples:
  # Start separate bitcoin-node that bitcoin-mine can connect to.
  bitcoin-node -regtest -ipcbind=unix

  # Connect to bitcoin-node and print tip block hash.
  bitcoin-mine -regtest

  # Run with debug output.
  bitcoin-mine -regtest -debug
)"};

const TranslateFn G_TRANSLATION_FUN{nullptr};

static void AddArgs(ArgsManager& args)
{
    SetupHelpOptions(args);
    SetupChainParamsBaseOptions(args);
    args.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-ipcconnect=<address>", "Connect to bitcoin-node process in the background to perform online operations. Valid <address> values are 'unix' to connect to the default socket, 'unix:<socket path>' to connect to a socket at a nonstandard path. Default value: unix", ArgsManager::ALLOW_ANY, OptionsCategory::IPC);
    args.AddArg("-maxtries=<n>", strprintf("Try to mine a block for <n> tries. Default %d", DEFAULT_MAX_TRIES), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
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
        std::string output{strprintf("%s bitcoin-mine version", CLIENT_NAME) + " " + FormatFullVersion() + "\n"};
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
    std::unique_ptr<interfaces::Init> mine_init{interfaces::MakeBasicInit("bitcoin-mine", argc > 0 ? argv[0] : "")};
    assert(mine_init);
    std::unique_ptr<interfaces::Init> node_init;
    try {
        std::string address{args.GetArg("-ipcconnect", "unix")};
        node_init = mine_init->ipc()->connectAddress(address);
    } catch (const std::exception& exception) {
        tfm::format(std::cerr, "Error: %s\n", exception.what());
        tfm::format(std::cerr, "Probably bitcoin-node is not running or not listening on a unix socket. Can be started with:\n\n");
        tfm::format(std::cerr, "    bitcoin-node -chain=%s -ipcbind=unix\n", args.GetChainTypeString());
        return EXIT_FAILURE;
    }
    assert(node_init);
    tfm::format(std::cout, "Connected to bitcoin-node\n");
    std::unique_ptr<interfaces::Mining> mining{node_init->makeMining()};
    assert(mining);

    auto tip{mining->getTip()};
    if (tip) {
        tfm::format(std::cout, "Tip hash is %s.\n", tip->hash.ToString());
    } else {
        tfm::format(std::cout, "Tip hash is null.\n");
    }

    auto consensus_params{Params().GetConsensus()};

    uint64_t max_tries{std::max<uint64_t>(DEFAULT_MAX_TRIES, args.GetIntArg("-maxtries", DEFAULT_MAX_TRIES))};
    auto tries_remaining{max_tries};
    auto block_template{mining->createNewBlock({})};
    auto block{block_template->getBlock()};
    block.hashMerkleRoot = BlockMerkleRoot(block);

    // Note: This loop ignores tip changes so the submitSolution call below
    // could fail even if the CheckProofOfWork succeeds! A more realistic miner
    // could avoid this problem by calling block_template->waitNext()
    // asynchronously to be notified when the tip changes.
    while (tries_remaining> 0 && block.nNonce < std::numeric_limits<uint32_t>::max() && !CheckProofOfWork(block.GetHash(), block.nBits, consensus_params)) {
        ++block.nNonce;
        --tries_remaining;
    }
    block_template->submitSolution(block.nVersion, block.nTime, block.nNonce, block_template->getCoinbaseTx());

    if (tip->hash != mining->getTip()->hash) {
        tfm::format(std::cout, "Mined a block, tip advanced to %s.\n", mining->getTip()->hash.ToString());
    } else {
        // Note: mining is only actually expected to succeed on -regtest
        // network. This is fine for testing and demonstration purposes.
        tfm::format(std::cout, "Failed to mine a block in %d iterations. Can try again.\n", max_tries);
    }

    return EXIT_SUCCESS;
}
