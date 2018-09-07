// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>
#include <util.h>
#include <walletinitinterface.h>

class DummyWalletInit : public WalletInitInterface {
public:

    void AddWalletOptions() const override;
    bool ParameterInteraction() const override {return true;}
    void RegisterRPC(CRPCTable &) const override {}
    bool Verify() const override {return true;}
    bool Open() const override {LogPrintf("No wallet support compiled in!\n"); return true;}
    void Start(CScheduler& scheduler) const override {}
    void Flush() const override {}
    void Stop() const override {}
    void Close() const override {}
};

void DummyWalletInit::AddWalletOptions() const
{
    std::vector<std::string> opts = {"-addresstype", "-changetype", "-disablewallet", "-discardfee=<amt>", "-fallbackfee=<amt>",
        "-keypool=<n>", "-mintxfee=<amt>", "-paytxfee=<amt>", "-rescan", "-salvagewallet", "-spendzeroconfchange",  "-txconfirmtarget=<n>",
        "-upgradewallet", "-wallet=<path>", "-walletbroadcast", "-walletdir=<dir>", "-walletnotify=<cmd>", "-walletrbf", "-zapwallettxes=<mode>",
        "-dblogsize=<n>", "-flushwallet", "-privdb", "-walletrejectlongchains"};
    gArgs.AddHiddenArgs(opts);
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();
