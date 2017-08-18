// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "chainparams.h"
#include "chainparamsbase.h"
#include "consensus/consensus.h"
#include "util.h"
#include "utilstrencodings.h"

#include "wallet/wallettools.h"

#include <stdio.h>


std::string HelpMessageCli()
{
    std::string strUsage;
    strUsage += HelpMessageGroup(_("Options:"));
    strUsage += HelpMessageOpt("-?", _("This help message"));
    strUsage += HelpMessageOpt("-file=<wallet-file>", strprintf(_("Specify wallet.dat file (default: %s)"), "./"+std::string(DEFAULT_WALLET_DAT)));

    strUsage += HelpMessageGroup(_("Commands:"));
    strUsage += HelpMessageOpt("info", _("Get wallet infos"));
    strUsage += HelpMessageOpt("create", _("Create new wallet file"));
    strUsage += HelpMessageOpt("salvage", _("Recover readable keypairs"));
    strUsage += HelpMessageOpt("zaptxes", _("Remove all transactions including metadata (will keep keys)"));

    return strUsage;
}

static bool WalletAppInit(int argc, char* argv[])
{
    ParseParameters(argc, argv);
    if (argc<2 || IsArgSet("-?") || IsArgSet("-h") || IsArgSet("-help") || IsArgSet("-version")) {
        std::string strUsage = strprintf(_("%s wallet-tool version"), PACKAGE_NAME) + " " + FormatFullVersion() + "\n";
        if (!IsArgSet("-version")) {
            strUsage += "\n" + _("Usage:") + "\n" +
            "  bitcoin-wallet-tool [options] <command>\n";
            strUsage += "\n" + HelpMessageCli();
        }

        fprintf(stdout, "%s", strUsage.c_str());
        return false;
    }

    // check for printtoconsole, allow -debug
    fPrintToConsole = GetBoolArg("-printtoconsole", GetBoolArg("-debug", false));

    // select params
    SelectParams(ChainNameFromCommandLine());

    return true;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();
    try {
        if(!WalletAppInit(argc, argv))
        return EXIT_FAILURE;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "WalletAppInit()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(NULL, "WalletAppInit()");
        return EXIT_FAILURE;
    }

    while (argc > 1 && IsSwitchChar(argv[1][0])) {
        argc--;
        argv++;
    }
    std::vector<std::string> args = std::vector<std::string>(&argv[1], &argv[argc]);
    std::string strMethod = args[0];

    std::string walletFile = GetArg("-file", DEFAULT_WALLET_DAT);

    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::executeWalletToolFunc(strMethod, walletFile))
        return EXIT_FAILURE;
    ECC_Stop();
    return true;
}
