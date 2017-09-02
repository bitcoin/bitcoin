// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "chainparams.h"
#include "clientversion.h"
#include "compat.h"
#include "fs.h"
#include "rpc/server.h"
#include "init.h"
#include "noui.h"
#include "scheduler.h"
#include "util.h"
#include "httpserver.h"
#include "httprpc.h"
#include "utilstrencodings.h"

#include <boost/thread.hpp>

#include <stdio.h>

/* Introduction text for doxygen: */

/*! \mainpage Developer documentation
 *
 * \section intro_sec Introduction
 *
 * This is the developer documentation of the reference client for an experimental new digital currency called Bitcoin (https://www.bitcoin.org/),
 * which enables instant payments to anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
 * with no central authority: managing transactions and issuing money are carried out collectively by the network.
 *
 * The software is a community-driven open source project, released under the MIT license.
 *
 * \section Navigation
 * Use the buttons <code>Namespaces</code>, <code>Classes</code> or <code>Files</code> at the top of the page to start navigating the code.
 */

void WaitForShutdown(boost::thread_group* threadGroup)
{
    bool fShutdown = ShutdownRequested();
    // Tell the main threads to shutdown.
    while (!fShutdown)
    {
        MilliSleep(200);
        fShutdown = ShutdownRequested();
    }
    if (threadGroup)
    {
        Interrupt(*threadGroup);
        threadGroup->join_all();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char* argv[])
{
    boost::thread_group threadGroup;
    CScheduler scheduler;

    bool fRet = false;

    //
    // Parameters
    //
    // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
    gArgs.ParseParameters(argc, argv);

    // Process help and version before taking care about datadir
    if (gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") ||  gArgs.IsArgSet("-help") || gArgs.IsArgSet("-version"))
    {
        std::string strUsage = strprintf(_("%s Daemon"), _(PACKAGE_NAME)) + " " + _("version") + " " + FormatFullVersion() + "\n";

        if (gArgs.IsArgSet("-version"))
        {
            strUsage += FormatParagraph(LicenseInfo());
        }
        else
        {
            strUsage += "\n" + _("Usage:") + "\n" +
                  "  bitcoind [options]                     " + strprintf(_("Start %s Daemon"), _(PACKAGE_NAME)) + "\n";

            strUsage += "\n" + HelpMessage(HMM_BITCOIND);
        }

        fprintf(stdout, "%s", strUsage.c_str());
        return true;
    }

    try
    {
        if (!fs::is_directory(GetDataDir(false)))
        {
            fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
            return false;
        }
        try
        {
            gArgs.ReadConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));
        } catch (const std::exception& e) {
            fprintf(stderr,"Error reading configuration file: %s\n", e.what());
            return false;
        }
        // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
        try {
            SelectParams(ChainNameFromCommandLine());
        } catch (const std::exception& e) {
            fprintf(stderr, "Error: %s\n", e.what());
            return false;
        }

        // Error out when loose non-argument tokens are encountered on command line
        for (int i = 1; i < argc; i++) {
            if (!IsSwitchChar(argv[i][0])) {
                fprintf(stderr, "Error: Command line contains unexpected token '%s', see bitcoind -h for a list of options.\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        }

        // -server defaults to true for bitcoind but not for the GUI so do this here
        gArgs.SoftSetBoolArg("-server", true);
        // Set this early so that parameter interactions go to console
        InitLogging();
        InitParameterInteraction();
        if (!AppInitBasicSetup())
        {
            // InitError will have been called with detailed error, which ends up on console
            exit(EXIT_FAILURE);
        }
        if (!AppInitParameterInteraction())
        {
            // InitError will have been called with detailed error, which ends up on console
            exit(EXIT_FAILURE);
        }
        if (!AppInitSanityChecks())
        {
            // InitError will have been called with detailed error, which ends up on console
            exit(EXIT_FAILURE);
        }
        if (gArgs.GetBoolArg("-daemon", false))
        {
#if HAVE_DECL_DAEMON
            fprintf(stdout, "Bitcoin server starting\n");

            // Daemonize
            if (daemon(1, 0)) { // don't chdir (1), do close FDs (0)
                fprintf(stderr, "Error: daemon() failed: %s\n", strerror(errno));
                return false;
            }
#else
            fprintf(stderr, "Error: -daemon is not supported on this operating system\n");
            return false;
#endif // HAVE_DECL_DAEMON
        }
        // Lock data directory after daemonization
        if (!AppInitLockDataDirectory())
        {
            // If locking the data directory failed, exit immediately
            exit(EXIT_FAILURE);
        }
        fRet = AppInitMain(threadGroup, scheduler);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

    if (!fRet)
    {
        Interrupt(threadGroup);
        threadGroup.join_all();
    } else {
        WaitForShutdown(&threadGroup);
    }
    Shutdown();

    return fRet;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();

    // Connect bitcoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
