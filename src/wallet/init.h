// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_INIT_H
#define BITCOIN_WALLET_INIT_H

#include <string>

class CRPCTable;
class CScheduler;

class WalletInit {
public:

    //! Return the wallets help message.
    static std::string GetHelpString(bool showDebug);

    //! Wallets parameter interaction
    static bool ParameterInteraction();

    //! Register wallet RPCs.
    static void RegisterRPC(CRPCTable &tableRPC);

    //! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
    //  This function will perform salvage on the wallet if requested, as long as only one wallet is
    //  being loaded (WalletParameterInteraction forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
    static bool Verify();

    //! Load wallet databases.
    static bool Open();

    //! Complete startup of wallets.
    static void Start(CScheduler& scheduler);

    //! Flush all wallets in preparation for shutdown.
    static void Flush();

    //! Stop all wallets. Wallets will be flushed first.
    static void Stop();

    //! Close all wallets.
    static void Close();
};

#endif // BITCOIN_WALLET_INIT_H
