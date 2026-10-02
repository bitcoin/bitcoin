// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/license_info.h>
#include <common/system.h>
#include <compat/compat.h>
#include <consensus/amount.h>
#include <key.h>
#include <logging.h>
#include <netbase.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <privbcast/input.h>
#include <privbcast/job.h>
#include <privbcast/params.h>
#include <privbcast/report.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef WIN32
#include <unistd.h>
#else
#include <fcntl.h>
#include <io.h>
#endif

const TranslateFn G_TRANSLATION_FUN{nullptr};

namespace {

constexpr int CONTINUE_EXECUTION{-1};

/** Tor's SocksPort. */
const std::string DEFAULT_TOR{"127.0.0.1:9050"};
constexpr uint16_t DEFAULT_TOR_PORT{9050};
constexpr CAmount DEFAULT_MAX_BURN_AMOUNT{0};
constexpr bool DEFAULT_PROGRESS{true};

/** Set on SIGINT and SIGTERM, or Ctrl-C and Ctrl-Break: cancels the job (C8). */
std::atomic<bool> g_cancel{false};
static_assert(std::atomic<bool>::is_always_lock_free, "set from a signal handler");

/** Write to stderr, as all the tool's progress lines and error messages are, through WriteIfReady()
 *  (Interface: bitcoin-privbcast). On Windows it is written in any case. */
void WriteToStderr(const std::string& text)
{
#ifndef WIN32
    privbcast::WriteIfReady(STDERR_FILENO, text);
#else
    fwrite(text.data(), 1, text.size(), stderr);
#endif
}

/** Report an exception that escaped, through WriteToStderr. */
void PrintException(const std::exception* e, std::string_view where)
{
    WriteToStderr(strprintf("Error: %s in %s\n", e ? e->what() : "unknown exception", where));
}

/** Write the error message through WriteToStderr, for an exit with EXIT_FAILURE. */
int Fail(const std::string& message)
{
    WriteToStderr(strprintf("Error: %s\n", message));
    return EXIT_FAILURE;
}

void SetupPrivbcastArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#ifdef HAVE_SOCKADDR_UN
    argsman.AddArg("-tor=<ip:port|path>", strprintf("Reach the network through the Tor SOCKS5 proxy at this loopback address. May be a local file path prefixed with 'unix:'. (default: %s)", DEFAULT_TOR),
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
#else
    argsman.AddArg("-tor=<ip:port>", strprintf("Reach the network through the Tor SOCKS5 proxy at this loopback address (default: %s)", DEFAULT_TOR),
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-maxburnamount=<amt>", strprintf("Refuse a transaction with an output to a provably unspendable script (such as OP_RETURN) whose value is above this amount, in %s (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_MAX_BURN_AMOUNT)),
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-progress", strprintf("Write progress lines to stderr (default: %u)", DEFAULT_PROGRESS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-debug=<category>", "Also write the debug lines of <category> to stderr. If <category> is not supplied or if <category> is 1 or \"all\", write all debug lines. "
                   "If <category> is 0 or \"none\", any categories given before it are ignored. Other valid values for <category> are: " + LogInstance().LogCategoriesString() +
                   ". This option can be specified multiple times.",
                   ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-seed=<name>", "Query this DNS seed instead of the chain's; can be specified multiple times (regtest only)",
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-timedivisor=<n>", strprintf("Divide every duration of the job, its internal timeouts included, by <n>, from 1 to %d (default: 1; regtest only)", privbcast::MAX_TIME_DIVISOR),
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::DEBUG_TEST);

    argsman.AddCommand("send", "Broadcast the transaction given in hex on stdin, or a parent and its child, then print the report");
    argsman.AddCommand("discover", "Run only the peer discovery of a job, then print the candidates found");

    SetupChainParamsBaseOptions(argsman);
}

// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
int AppInitPrivbcast(ArgsManager& args, int argc, char* argv[])
{
    SetupPrivbcastArgs(args);
    std::string error;
    if (!args.ParseParameters(argc, argv, error)) {
        WriteToStderr(strprintf("Error parsing command line arguments: %s\n", error));
        return EXIT_FAILURE;
    }

    if (argc < 2 || HelpRequested(args) || args.GetBoolArg("-version", false)) {
        std::string usage = CLIENT_NAME " bitcoin-privbcast utility version " + FormatFullVersion() + "\n";

        if (args.GetBoolArg("-version", false)) {
            usage += FormatParagraph(LicenseInfo());
        } else {
            usage += "\n"
                "bitcoin-privbcast broadcasts a transaction without revealing the sender. It reaches the network only\n"
                "through Tor, connects to a few peers on a schedule drawn when it starts, announces the transaction\n"
                "and stops. Available [commands] are listed below.\n"
                "\n"
                "Usage:  bitcoin-privbcast [options] send < <hex-transaction-file>\n"
                "or:     bitcoin-privbcast [options] discover\n"
                "\n"
                "send also takes two transactions, an unconfirmed parent and its child in either order: it then\n"
                "announces the child and serves the parent to peers that ask for it.\n"
                "send prints its report on stdout and exits with status 0 if at least one peer was sent the\n"
                "announcement in full, even if the job failed afterwards; otherwise 1 on an error or if the job\n"
                "failed, and 2 if it ran without failing. Progress lines go to stderr.\n";
            usage += "\n" + args.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", usage);

        if (argc < 2) return Fail("too few parameters");
        return EXIT_SUCCESS;
    }

    // Check for chain settings (Params() calls are only valid after this clause)
    try {
        SelectParams(args.GetChainType());
    } catch (const std::exception& e) {
        return Fail(e.what());
    }

    return CONTINUE_EXECUTION;
}

/** Enable the categories whose debug lines go to stderr: privatebroadcast for the progress lines,
 *  and those of -debug, as bitcoind reads it. */
util::Result<void> SetLogCategories(const ArgsManager& args)
{
    if (args.GetBoolArg("-progress", DEFAULT_PROGRESS)) LogInstance().EnableCategory(BCLog::PRIVBROADCAST);
    const std::vector<std::string> categories{args.GetArgs("-debug")};
    // Categories given before the last -debug=0 or -debug=none are ignored.
    const auto last_none{std::find_if(categories.rbegin(), categories.rend(),
                                      [](const std::string& cat) { return cat == "0" || cat == "none"; })};
    for (auto it{last_none.base()}; it != categories.end(); ++it) {
        if (!LogInstance().EnableCategory(*it)) return util::Error{Untranslated(strprintf("Unsupported logging category -debug=%s", *it))};
    }
    return {};
}

/** Everything a job takes from the command line and the chain, all checked before anything is read
 *  from stdin or sent to the network. The regtest overrides are the only settings that change what
 *  a job does, and they are refused on other chains (U1). */
util::Result<privbcast::JobInputs> GetJobInputs(const ArgsManager& args)
{
    privbcast::JobInputs inputs;
    if (args.IsArgSet("-timedivisor") && Params().GetChainType() != ChainType::REGTEST) return util::Error{Untranslated("-timedivisor is only available on regtest")};
    // The chain's own seeds only (U1).
    if (args.IsArgSet("-signetseednode")) return util::Error{Untranslated("-signetseednode is not available: a job queries only the chain's own seeds")};
    if (args.IsArgSet("-signetchallenge")) return util::Error{Untranslated("-signetchallenge is not available: a custom signet has no seeds for a job to query")};

    auto proxy{privbcast::ParseTor(args.GetArg("-tor", DEFAULT_TOR), DEFAULT_TOR_PORT)};
    if (!proxy) return util::Error{util::ErrorString(proxy)};
    inputs.proxy = *proxy;

    auto seeds{privbcast::GetSeedMaterial(args, Params(), "-seed", /*check_chain_names=*/true)};
    if (!seeds) return util::Error{util::ErrorString(seeds)};
    inputs.seeds = std::move(*seeds);

    if (const auto value{args.GetArg("-timedivisor")}) {
        const auto divisor{ToIntegral<int>(*value)};
        if (!divisor || *divisor < 1 || *divisor > privbcast::MAX_TIME_DIVISOR) {
            return util::Error{Untranslated(strprintf("-timedivisor=%s is not a number from 1 to %d", *value, privbcast::MAX_TIME_DIVISOR))};
        }
        inputs.timing = privbcast::Timing{*divisor};
    }

    inputs.cancel = &g_cancel;
    return inputs;
}

/** The transaction on stdin, or a parent and its child in either order, read up to its bound and
 *  checked (D3). Sets the job's transaction, and for two its parent. */
util::Result<void> ReadTransactions(CAmount max_burn, privbcast::JobInputs& inputs)
{
#ifdef WIN32
    // In text mode the C runtime ends the input at a control-Z and turns CRLF into LF: a valid
    // transaction followed by a control-Z would pass whatever comes after it, and the bound would
    // count bytes after translation. A process started without stdin has nothing to switch.
    if (const int fd{_fileno(stdin)}; fd >= 0 && _setmode(fd, _O_BINARY) == -1) {
        return util::Error{Untranslated("cannot read the input in binary mode")};
    }
#endif
    auto text{privbcast::ReadBounded(std::cin, privbcast::MAX_STDIN_BYTES)};
    if (!text) return util::Error{util::ErrorString(text)};
    // std::cin, synced with C stdio, takes a read error on stdin for its end: only stdin's error
    // indicator tells them apart.
    if (std::ferror(stdin)) return util::Error{Untranslated("reading the input failed")};
    auto txs{privbcast::ParseTransactions(*text)};
    if (!txs) return util::Error{util::ErrorString(txs)};
    if (txs->size() == 2) {
        auto package{privbcast::CheckPackage((*txs)[0], (*txs)[1], max_burn)};
        if (!package) return util::Error{util::ErrorString(package)};
        inputs.tx = package->child;
        inputs.parent = package->parent;
        return {};
    }
    const CTransactionRef tx{txs->front()};
    if (auto checked{privbcast::CheckForBroadcast(*tx, max_burn)}; !checked) return util::Error{util::ErrorString(checked)};
    inputs.tx = tx;
    return {};
}

#ifndef WIN32
void HandleCancelSignal(int)
{
    g_cancel = true;
}
#else
BOOL WINAPI HandleConsoleCtrl(DWORD type)
{
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    g_cancel = true;
    return TRUE;
}
#endif

/** SIGINT and SIGTERM, or Ctrl-C and Ctrl-Break, cancel the job; the report is printed all the
 *  same. */
void SetupSignalHandlers()
{
#ifndef WIN32
    struct sigaction sa{};
    sa.sa_handler = HandleCancelSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
#else
    SetConsoleCtrlHandler(HandleConsoleCtrl, TRUE);
#endif
}

int Run(const ArgsManager& args)
{
    const auto cmd{args.GetCommand()};
    if (!cmd) return Fail("must specify a command");
    if (!cmd->args.empty()) return Fail(strprintf("%s takes no arguments", cmd->command));

    // Usage errors first, then the input: nothing touches the network before both are checked.
    auto inputs{GetJobInputs(args)};
    if (!inputs) return Fail(util::ErrorString(inputs).original);
    const std::optional<CAmount> max_burn{ParseMoney(args.GetArg("-maxburnamount", FormatMoney(DEFAULT_MAX_BURN_AMOUNT)))};
    if (!max_burn) return Fail(strprintf("-maxburnamount=%s is not an amount", args.GetArg("-maxburnamount", "")));
    if (auto logging{SetLogCategories(args)}; !logging) return Fail(util::ErrorString(logging).original);
    const bool send{cmd->command == "send"};
    if (send) {
        if (auto read{ReadTransactions(*max_burn, *inputs)}; !read) return Fail(util::ErrorString(read).original);
    }

    SetupSignalHandlers();

    // For the BIP324 keys.
    ECC_Context ecc_context{};
    privbcast::Job job{std::move(*inputs)};
    // A failure of the job ends it with a report that says why (H2). Only a failure to make the
    // report throws, and exits through the catch in main().
    const privbcast::Report report{send ? job.Run() : job.RunDiscoveryOnly()};
    if (report.summary.error) LogError("The job failed: %s", *report.summary.error);
    if (!send) {
        tfm::format(std::cout, "%s\n", privbcast::ToUniValue(report.discovery, /*candidates=*/true).write(/*prettyIndent=*/2));
        return report.summary.error ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    tfm::format(std::cout, "%s\n", privbcast::ToUniValue(report).write(/*prettyIndent=*/2));
    return privbcast::ExitStatus(report);
}

} // namespace

MAIN_FUNCTION
{
#ifndef WIN32
    // A reader of stdout or stderr that went away fails the write instead of ending the process,
    // from the first diagnostic on (H2).
    signal(SIGPIPE, SIG_IGN);
#endif
    ArgsManager& args = gArgs;
    SetupEnvironment();
    // Everything for stderr goes through WriteToStderr, from before the arguments are checked:
    // error messages, and logged lines in the logger's format, timestamps included (H1). stdout
    // carries the report.
    LogInstance().PushBackCallback(WriteToStderr);
    LogInstance().StartLogging();
    if (!SetupNetworking()) return Fail("Initializing networking failed");

    try {
        int ret = AppInitPrivbcast(args, argc, argv);
        if (ret != CONTINUE_EXECUTION) {
            return ret;
        }
    } catch (const std::exception& e) {
        PrintException(&e, "AppInitPrivbcast()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintException(nullptr, "AppInitPrivbcast()");
        return EXIT_FAILURE;
    }

    try {
        return Run(args);
    } catch (const std::exception& e) {
        PrintException(&e, "Run()");
    } catch (...) {
        PrintException(nullptr, "Run()");
    }
    return EXIT_FAILURE;
}
