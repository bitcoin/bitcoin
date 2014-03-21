// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "init.h"
#include "rpcclient.h"
#include "rpcprotocol.h"
#include "ui_interface.h" /* for _(...) */
#include "chainparams.h"

#include <boost/filesystem/operations.hpp>

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
static bool AppInitRPC(int argc, char* argv[])
{
    //
    // Parameters
    //
    ParseParameters(argc, argv);
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", mapArgs["-datadir"].c_str());
        return false;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);
    // Check for -testnet or -regtest parameter (TestNet() calls are only valid after this clause)
    if (!SelectParamsFromCommandLine()) {
        fprintf(stderr, "Error: Invalid combination of -regtest and -testnet.\n");
        return false;
    }

    if (argc<2 || mapArgs.count("-?") || mapArgs.count("--help"))
    {
        // First part of help message is specific to RPC client
        std::string strUsage = _("Bitcoin RPC client version") + " " + FormatFullVersion() + "\n\n" +
            _("Usage:") + "\n" +
              "  bitcoin-cli [options] <command> [params]  " + _("Send command to Bitcoin server") + "\n" +
              "  bitcoin-cli [options] help                " + _("List commands") + "\n" +
              "  bitcoin-cli [options] help <command>      " + _("Get help for a command") + "\n";

        strUsage += "\n" + HelpMessageCli(true);

        fprintf(stdout, "%s", strUsage.c_str());
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    try
    {
        if(!AppInitRPC(argc, argv))
            return abs(RPC_MISC_ERROR);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
        return abs(RPC_MISC_ERROR);
    } catch (...) {
        PrintExceptionContinue(NULL, "AppInitRPC()");
        return abs(RPC_MISC_ERROR);
    }

    int ret = abs(RPC_MISC_ERROR);
    try
    {
        ret = CommandLineRPC(argc, argv);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
    }
    return ret;
}
