// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/init.h>
#include <common/system.h>
#include <compat/compat.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <kernel/context.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <node/warnings.h>
#include <noui.h>
#include <util/check.h>
#include <util/exception.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/syserror.h>
#include <util/threadnames.h>
#include <util/tokenpipe.h>
#include <util/translation.h>

#include <any>
#include <functional>
#include <optional>

using node::NodeContext;

const TranslateFn G_TRANSLATION_FUN{nullptr};

#if HAVE_DECL_FORK

/** Custom implementation of daemon(). This implements the same order of operations as glibc.
 * Opens a pipe to the child process to be able to wait for an event to occur.
 *
 * @returns 0 if successful, and in child process.
 *          >0 if successful, and in parent process.
 *          -1 in case of error (in parent process).
 *
 *          In case of success, endpoint will be one end of a pipe from the child to parent process,
 *          which can be used with TokenWrite (in the child) or TokenRead (in the parent).
 */
int fork_daemon(bool nochdir, bool noclose, TokenPipeEnd& endpoint)
{
    // communication pipe with child process
    std::optional<TokenPipe> umbilical = TokenPipe::Make();
    if (!umbilical) {
        return -1; // pipe or pipe2 failed.
    }

    int pid = fork();
    if (pid < 0) {
        return -1; // fork failed.
    }
    if (pid != 0) {
        // Parent process gets read end, closes write end.
        endpoint = umbilical->TakeReadEnd();
        umbilical->TakeWriteEnd().Close();

        int status = endpoint.TokenRead();
        if (status != 0) { // Something went wrong while setting up child process.
            endpoint.Close();
            return -1;
        }

        return pid;
    }
    // Child process gets write end, closes read end.
    endpoint = umbilical->TakeWriteEnd();
    umbilical->TakeReadEnd().Close();

#if HAVE_DECL_SETSID
    if (setsid() < 0) {
        exit(1); // setsid failed.
    }
#endif

    if (!nochdir) {
        if (chdir("/") != 0) {
            exit(1); // chdir failed.
        }
    }
    if (!noclose) {
        // Open /dev/null, and clone it into STDIN, STDOUT and STDERR to detach
        // from terminal.
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            bool err = dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0;
            // Don't close if fd<=2 to try to handle the case where the program was invoked without any file descriptors open.
            if (fd > 2) close(fd);
            if (err) {
                exit(1); // dup2 failed.
            }
        } else {
            exit(1); // open /dev/null failed.
        }
    }
    endpoint.TokenWrite(0); // Success
    return 0;
}

#endif

static bool ParseArgs(NodeContext& node, int argc, char* argv[])
{
    ArgsManager& args{*Assert(node.args)};
    // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
    SetupServerArgs(args, node.init->canListenIpc());
    std::string error;
    if (!args.ParseParameters(argc, argv, error)) {
        return InitError(Untranslated(strprintf("Error parsing command line arguments: %s", error)));
    }

    if (auto error = common::InitConfig(args)) {
        return InitError(error->message, error->details);
    }

    // Error out when loose non-argument tokens are encountered on command line
    for (int i = 1; i < argc; i++) {
        if (!IsSwitchChar(argv[i][0])) {
            return InitError(Untranslated(strprintf("Command line contains unexpected token '%s', see bitcoind -h for a list of options.", argv[i])));
        }
    }
    return true;
}

static bool ProcessInitCommands(interfaces::Init& init, ArgsManager& args)
{
    // Process help and version before taking care about datadir
    if (HelpRequested(args) || args.GetBoolArg("-version", false)) {
        std::string strUsage = CLIENT_NAME " daemon version " + FormatFullVersion();
        if (const char* exe_name{init.exeName()}) {
            strUsage += " ";
            strUsage += exe_name;
        }
        strUsage += "\n";

        if (args.GetBoolArg("-version", false)) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                "The " CLIENT_NAME " daemon (bitcoind) is a headless program that connects to the Bitcoin network to validate and relay transactions and blocks, as well as relaying addresses.\n\n"
                "It provides the backbone of the Bitcoin network and its RPC, REST and ZMQ services can provide various transaction, block and address-related services.\n\n"
                "There is an optional wallet component which provides transaction services.\n\n"
                "It can be used in a headless environment or as part of a server setup.\n"
                "\n"
                "Usage: bitcoind [options]\n"
                "\n";
            strUsage += args.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", strUsage);
        return true;
    }

    return false;
}

static bool AppInit(NodeContext& node)
{
    bool fRet = false;
    ArgsManager& args = *Assert(node.args);

#if HAVE_DECL_FORK
    // Communication with parent after daemonizing. This is used for signalling in the following ways:
    // - a boolean token is sent when the initialization process (all the Init* functions) have finished to indicate
    // that the parent process can quit, and whether it was successful/unsuccessful.
    // - an unexpected shutdown of the child process creates an unexpected end of stream at the parent
    // end, which is interpreted as failure to start.
    TokenPipeEnd daemon_ep;
#endif
    std::any context{&node};
    try
    {
        // -server defaults to true for bitcoind but not for the GUI so do this here
        args.SoftSetBoolArg("-server", true);
        // Set this early so that parameter interactions go to console
        InitLogging(args);
        InitParameterInteraction(args);
        if (!AppInitBasicSetup(args, node.exit_status)) {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (!AppInitParameterInteraction(args)) {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }

        node.warnings = std::make_unique<node::Warnings>();

        node.kernel = std::make_unique<kernel::Context>();
        node.ecc_context = std::make_unique<ECC_Context>();
        if (!AppInitSanityChecks(*node.kernel))
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }

        if (args.GetBoolArg("-daemon", DEFAULT_DAEMON) || args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT)) {
#if HAVE_DECL_FORK
            tfm::format(std::cout, CLIENT_NAME " starting\n");

            // Daemonize
            switch (fork_daemon(1, 0, daemon_ep)) { // don't chdir (1), do close FDs (0)
            case 0: // Child: continue.
                // If -daemonwait is not enabled, immediately send a success token the parent.
                if (!args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT)) {
                    daemon_ep.TokenWrite(1);
                    daemon_ep.Close();
                }
                break;
            case -1: // Error happened.
                return InitError(Untranslated(strprintf("fork_daemon() failed: %s", SysErrorString(errno))));
            default: { // Parent: wait and exit.
                int token = daemon_ep.TokenRead();
                if (token) { // Success
                    exit(EXIT_SUCCESS);
                } else { // fRet = false or token read error (premature exit).
                    tfm::format(std::cerr, "Error during initialization - check debug.log for details\n");
                    exit(EXIT_FAILURE);
                }
            }
            }
#else
            return InitError(Untranslated("-daemon is not supported on this operating system"));
#endif // HAVE_DECL_FORK
        }
        // Lock critical directories after daemonization
        if (!AppInitLockDirectories())
        {
            // If locking a directory failed, exit immediately
            return false;
        }
        fRet = AppInitInterfaces(node) && AppInitMain(node);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

#if HAVE_DECL_FORK
    if (daemon_ep.IsOpen()) {
        // Signal initialization status to parent, then close pipe.
        daemon_ep.TokenWrite(fRet);
        daemon_ep.Close();
    }
#endif
    return fRet;
}

MAIN_FUNCTION
{
#ifdef WIN32
    common::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif

    NodeContext node;
    int exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeNodeInit(node, argc, argv, exit_status);
    if (!init) {
        return exit_status;
    }

    SetupEnvironment();

    // Connect bitcoind signal handlers
    noui_connect();

    util::ThreadSetInternalName("init");

    // Interpret command line arguments
    ArgsManager& args = *Assert(node.args);
    if (!ParseArgs(node, argc, argv)) return EXIT_FAILURE;
    // Process early info return commands such as -help or -version
    if (ProcessInitCommands(*init, args)) return EXIT_SUCCESS;

    // Start application
    if (!AppInit(node) || !Assert(node.shutdown_signal)->wait()) {
        node.exit_status = EXIT_FAILURE;
    }
    Interrupt(node);
    Shutdown(node);

    return node.exit_status;
}
