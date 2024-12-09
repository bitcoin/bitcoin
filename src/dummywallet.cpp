// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dummywallet_settings.h>
#include <common/args.h>
#include <logging.h>
#include <walletinitinterface.h>

class ArgsManager;

namespace interfaces {
class Chain;
class Handler;
class Wallet;
class WalletLoader;
}

class DummyWalletInit : public WalletInitInterface {
public:

    bool HasWalletSupport() const override {return false;}
    void AddWalletOptions(ArgsManager& argsman) const override;
    bool ParameterInteraction() const override {return true;}
    void Construct(node::NodeContext& node) const override {LogPrintf("No wallet support compiled in!\n");}
};

void DummyWalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    AddresstypeSettingHidden::Register(argsman);
    AvoidpartialspendsSettingHidden::Register(argsman);
    ChangetypeSettingHidden::Register(argsman);
    ConsolidatefeerateSettingHidden::Register(argsman);
    DisablewalletSettingHidden::Register(argsman);
    DiscardfeeSettingHidden::Register(argsman);
    FallbackfeeSettingHidden::Register(argsman);
    KeypoolSettingHidden::Register(argsman);
    MaxapsfeeSettingHidden::Register(argsman);
    MaxtxfeeSettingHidden::Register(argsman);
    MintxfeeSettingHidden::Register(argsman);
    PaytxfeeSettingHidden::Register(argsman);
    SignerSettingHidden::Register(argsman);
    SpendzeroconfchangeSettingHidden::Register(argsman);
    TxconfirmtargetSettingHidden::Register(argsman);
    WalletSettingHidden::Register(argsman);
    WalletbroadcastSettingHidden::Register(argsman);
    WalletdirSettingHidden::Register(argsman);
    WalletnotifySettingHidden::Register(argsman);
    WalletrbfSettingHidden::Register(argsman);
    DblogsizeSettingHidden::Register(argsman);
    FlushwalletSettingHidden::Register(argsman);
    PrivdbSettingHidden::Register(argsman);
    WalletrejectlongchainsSettingHidden::Register(argsman);
    WalletcrosschainSettingHidden::Register(argsman);
    UnsafesqlitesyncSettingHidden::Register(argsman);
    SwapbdbendianSettingHidden::Register(argsman);
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();

namespace interfaces {

std::unique_ptr<WalletLoader> MakeWalletLoader(Chain& chain, ArgsManager& args)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

} // namespace interfaces
