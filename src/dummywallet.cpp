// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util/system.h>
#include <walletinitinterface.h>

class ArgsManager;
class CWallet;

namespace interfaces {
class Chain;
class Handler;
class Wallet;
class WalletClient;
class WalletLoader;
namespace CoinJoin {
class Loader;
} // namespace CoinJoin
} // namespace interfaces
namespace node {
class NodeContext;
} // namespace node

class DummyWalletInit : public WalletInitInterface {
public:

    bool HasWalletSupport() const override {return false;}
    void AddWalletOptions(ArgsManager& argsman) const override;
    bool ParameterInteraction() const override {return true;}
    void Construct(node::NodeContext& node) const override {LogPrintf("No wallet support compiled in!\n");}

    // Dash Specific WalletInitInterface InitCoinJoinSettings
    void AutoLockMasternodeCollaterals(interfaces::WalletLoader& wallet_loader) const override {}
    void InitCoinJoinSettings(interfaces::CoinJoin::Loader& coinjoin_loader, interfaces::WalletLoader& wallet_loader) const override {}
    void InitAutoBackup() const override {}
};

void DummyWalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    argsman.AddHiddenArgs({
        "-avoidpartialspends",
        "-consolidatefeerate=<amt>",
        "-createwalletbackups=<n>",
        "-disablewallet",
        "-instantsendnotify=<cmd>",
        "-keypool=<n>",
        "-maxapsfee=<n>",
        "-maxtxfee=<amt>",
        "-rescan=<mode>",
        "-salvagewallet",
        "-spendzeroconfchange",
        "-wallet=<path>",
        "-walletbackupsdir=<dir>",
        "-walletbroadcast",
        "-walletdir=<dir>",
        "-walletnotify=<cmd>",
        "-discardfee=<amt>",
        "-fallbackfee=<amt>",
        "-mintxfee=<amt>",
        "-paytxfee=<amt>",
        "-txconfirmtarget=<n>",
        "-hdseed=<hex>",
        "-mnemonic=<text>",
        "-mnemonicpassphrase=<text>",
        "-usehd",
        "-enablecoinjoin",
        "-coinjoinamount=<n>",
        "-coinjoinautostart",
        "-coinjoindenomsgoal=<n>",
        "-coinjoindenomshardcap=<n>",
        "-coinjoinmultisession",
        "-coinjoinrounds=<n>",
        "-coinjoinsessions=<n>",
        "-dblogsize=<n>",
        "-flushwallet",
        "-privdb",
        "-walletrejectlongchains",
        "-unsafesqlitesync"
    });
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();

namespace interfaces {

std::unique_ptr<CoinJoin::Loader> MakeCoinJoinLoader(node::NodeContext& node)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

std::unique_ptr<Wallet> MakeWallet(const std::shared_ptr<CWallet>& wallet)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

std::unique_ptr<WalletClient> MakeWalletLoader(Chain& chain, ArgsManager& args, node::NodeContext& node_context,
                                               interfaces::CoinJoin::Loader& coinjoin_loader)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

} // namespace interfaces
