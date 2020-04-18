// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_INIT_H
#define BITCOIN_WALLET_INIT_H

#include <walletinitinterface.h>
#include <string>

class CRPCTable;
class CScheduler;

class WalletInit : public WalletInitInterface {
public:

    //! Return the wallets help message.
    std::string GetHelpString(bool showDebug) override;

    //! Wallets parameter interaction
    bool ParameterInteraction() override;

    //! Register wallet RPCs.
    void RegisterRPC(CRPCTable &tableRPC) override;

    //! Responsible for reading and validating the -wallet arguments and verifying the wallet database.
    //  This function will perform salvage on the wallet if requested, as long as only one wallet is
    //  being loaded (WalletParameterInteraction forbids -salvagewallet, -zapwallettxes or -upgradewallet with multiwallet).
    bool Verify() override;

    //! Load wallet databases.
    bool Open() override;

    //! Complete startup of wallets.
    void Start(CScheduler& scheduler) override;

    //! Flush all wallets in preparation for shutdown.
    void Flush() override;

    //! Stop all wallets. Wallets will be flushed first.
    void Stop() override;

    //! Close all wallets.
    void Close() override;

    // Dash Specific Wallet Init
    void AutoLockMasternodeCollaterals() override;
    void InitPrivateSendSettings() override;
    void InitKeePass() override;
    bool InitAutoBackup() override;
};

#endif // BITCOIN_WALLET_INIT_H
