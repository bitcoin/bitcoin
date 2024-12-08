// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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
#include <wallet/init_settings.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
#include <wallet/coincontrol.h>
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
    AddresstypeSetting::Register(argsman);
    AvoidpartialspendsSetting::Register(argsman);
    ChangetypeSetting::Register(argsman);
    ConsolidatefeerateSetting::Register(argsman);
    DisablewalletSetting::Register(argsman);
    DiscardfeeSetting::Register(argsman);

    FallbackfeeSetting::Register(argsman);
    KeypoolSetting::Register(argsman);
    MaxapsfeeSetting::Register(argsman);
    MaxtxfeeSetting::Register(argsman);
    MintxfeeSetting::Register(argsman);
    PaytxfeeSetting::Register(argsman);
#ifdef ENABLE_EXTERNAL_SIGNER
    SignerSetting::Register(argsman);
#endif
    SpendzeroconfchangeSetting::Register(argsman);
    TxconfirmtargetSetting::Register(argsman);
    WalletSetting::Register(argsman);
    WalletbroadcastSetting::Register(argsman);
    WalletdirSetting::Register(argsman);
#if HAVE_SYSTEM
    WalletnotifySetting::Register(argsman);
#endif
    WalletrbfSetting::Register(argsman);

#ifdef USE_BDB
    DblogsizeSetting::Register(argsman);
    FlushwalletSetting::Register(argsman);
    PrivdbSetting::Register(argsman);
    SwapbdbendianSetting::Register(argsman);
#else
    DblogsizeSetting::Hidden::Register(argsman);
    FlushwalletSetting::Hidden::Register(argsman);
    PrivdbSetting::Hidden::Register(argsman);
    SwapbdbendianSetting::Hidden::Register(argsman);
#endif

#ifdef USE_SQLITE
    UnsafesqlitesyncSetting::Register(argsman);
#else
    UnsafesqlitesyncSetting::Hidden::Register(argsman);
#endif

    WalletrejectlongchainsSetting::Register(argsman);
    WalletcrosschainSetting::Register(argsman);
}

bool WalletInit::ParameterInteraction() const
{
#ifdef USE_BDB
     if (!BerkeleyDatabaseSanityCheck()) {
         return InitError(Untranslated("A version conflict was detected between the run-time BerkeleyDB library and the one used during compilation."));
     }
#endif
    if (DisablewalletSetting::Get(gArgs)) {
        for (const std::string& wallet : WalletSetting::Get(gArgs)) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }

        return true;
    }

    if (BlocksonlySetting::Get(gArgs, DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    return true;
}

void WalletInit::Construct(NodeContext& node) const
{
    ArgsManager& args = *Assert(node.args);
    if (DisablewalletSetting::Get(args)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }
    auto wallet_loader = node.init->makeWalletLoader(*node.chain);
    node.wallet_loader = wallet_loader.get();
    node.chain_clients.emplace_back(std::move(wallet_loader));
}
} // namespace wallet

const WalletInitInterface& g_wallet_init_interface = wallet::WalletInit();
