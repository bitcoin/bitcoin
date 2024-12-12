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
    AddressTypeSettingHidden::Register(argsman);
    AvoidPartialSpendsSettingHidden::Register(argsman);
    ChangeTypeSettingHidden::Register(argsman);
    ConsolidateFeeRateSettingHidden::Register(argsman);
    DisableWalletSettingHidden::Register(argsman);
    DiscardFeeSettingHidden::Register(argsman);
    FallbackFeeSettingHidden::Register(argsman);
    KeyPoolSettingHidden::Register(argsman);
    MaxApsFeeSettingHidden::Register(argsman);
    MaxTxFeeSettingHidden::Register(argsman);
    MinTxFeeSettingHidden::Register(argsman);
    PayTxFeeSettingHidden::Register(argsman);
    SignerSettingHidden::Register(argsman);
    SpendZeroConfChangeSettingHidden::Register(argsman);
    TxConfirmTargetSettingHidden::Register(argsman);
    WalletSettingHidden::Register(argsman);
    WalletBroadcastSettingHidden::Register(argsman);
    WalletDirSettingHidden::Register(argsman);
    WalletNotifySettingHidden::Register(argsman);
    WalletRbfSettingHidden::Register(argsman);
    DbLogSizeSettingHidden::Register(argsman);
    FlushWalletSettingHidden::Register(argsman);
    PrivDbSettingHidden::Register(argsman);
    WalletRejectLongChainsSettingHidden::Register(argsman);
    WalletCrossChainSettingHidden::Register(argsman);
    UnsafeSqliteSyncSettingHidden::Register(argsman);
    SwapBdbEndianSettingHidden::Register(argsman);
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();

namespace interfaces {

std::unique_ptr<WalletLoader> MakeWalletLoader(Chain& chain, ArgsManager& args)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

} // namespace interfaces
