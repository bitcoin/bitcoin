// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/args.h>
#include <init.h>
#include <init_settings.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <outputtype.h>
#include <univalue.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/init_settings.h>
#include <wallet/wallet.h>
#include <walletinitinterface.h>

using node::NodeContext;

namespace wallet {
class WalletInit : public WalletInitInterface
{
public:
    //! Was the wallet component compiled in.
    bool HasWalletSupport() const override {return true;}

    //! Return the wallets help message.
    void AddWalletOptions(ArgsManager& argsman) const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    //! Add wallets that should be opened to list of chain clients.
    void Construct(NodeContext& node) const override;
};

void WalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    AddressTypeSetting::Register(argsman);
    AvoidPartialSpendsSetting::Register(argsman);
    ChangeTypeSetting::Register(argsman);
    ConsolidateFeeRateSetting::Register(argsman);
    DisableWalletSetting::Register(argsman);
    DiscardFeeSetting::Register(argsman);

    FallbackFeeSetting::Register(argsman);
    KeyPoolSetting::Register(argsman);
    MaxApsFeeSetting::Register(argsman);
    MaxTxFeeSetting::Register(argsman);
    MinTxFeeSetting::Register(argsman);
    PayTxFeeSetting::Register(argsman);
#ifdef ENABLE_EXTERNAL_SIGNER
    SignerSetting::Register(argsman);
#endif
    SpendZeroConfChangeSetting::Register(argsman);
    TxConfirmTargetSetting::Register(argsman);
    WalletSetting::Register(argsman);
    WalletBroadcastSetting::Register(argsman);
    WalletDirSetting::Register(argsman);
#if HAVE_SYSTEM
    WalletNotifySetting::Register(argsman);
#endif
    WalletRbfSetting::Register(argsman);

    UnsafeSqliteSyncSetting::Register(argsman);

    WalletRejectLongChainsSetting::Register(argsman);
    WalletCrossChainSetting::Register(argsman);
}

bool WalletInit::ParameterInteraction() const
{
    if (DisableWalletSetting::Get(gArgs)) {
        for (const std::string& wallet : WalletSetting::Get(gArgs)) {
            LogInfo("Parameter interaction: -disablewallet -> ignoring -wallet=%s", wallet);
        }

        return true;
    }

    if (BlocksOnlySetting::Get(gArgs, DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogInfo("Parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0");
    }

    return true;
}

void WalletInit::Construct(NodeContext& node) const
{
    ArgsManager& args = *Assert(node.args);
    if (DisableWalletSetting::Get(args)) {
        LogInfo("Wallet disabled!");
        return;
    }
    auto wallet_loader = node.init->makeWalletLoader(*node.chain);
    node.wallet_loader = wallet_loader.get();
    node.chain_clients.emplace_back(std::move(wallet_loader));
}
} // namespace wallet

const WalletInitInterface& g_wallet_init_interface = wallet::WalletInit();
