// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "init.h"
#include "rpcclient.h"
#include "ui_interface.h" /* for _(...) */
#include "chainparams.h"

#include <iostream>
#include <string>

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
        std::cerr << "Error: Specified data directory \"" << mapArgs["-datadir"] << "\" does not exist.\n";
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
        std::cout << _<std::string>("Bitcoin RPC client version") << " " << FormatFullVersion() << "\n" 
                  << "\n"
                  << _<std::string>("Usage:") << "\n"
                  << "  bitcoin-cli [options] <command> [params]  " << _<std::string>("Send command to Bitcoin server") << "\n"
                  << "  bitcoin-cli [options] help                " << _<std::string>("List commands") << "\n"
                  << "  bitcoin-cli [options] help <command>      " << _<std::string>("Get help for a command") << "\n"
                  << "\n" 
                  << HelpMessageCli(true);

        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    try
    {
        if(!AppInitRPC(argc, argv))
            return 1;
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
    } catch (...) {
        PrintExceptionContinue(NULL, "AppInitRPC()");
    }

    try
    {
        if(!CommandLineRPC(argc, argv))
            return 1;
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
    }
    return 0;
}
