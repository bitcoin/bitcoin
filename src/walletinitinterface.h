// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLETINITINTERFACE_H
#define BITCOIN_WALLETINITINTERFACE_H

class ArgsManager;
namespace interfaces {
class WalletLoader;
namespace CoinJoin {
class Loader;
} // namespace CoinJoin
} // namespace interfaces
namespace node {
struct NodeContext;
} // namespace node

class WalletInitInterface {
public:
    /** Is the wallet component enabled */
    virtual bool HasWalletSupport() const = 0;
    /** Get wallet help string */
    virtual void AddWalletOptions(ArgsManager& argsman) const = 0;
    /** Check wallet parameter interaction */
    virtual bool ParameterInteraction() const = 0;
    /** Add wallets that should be opened to list of chain clients. */
    virtual void Construct(node::NodeContext& node) const = 0;

    // Dash Specific WalletInitInterface
    virtual void AutoLockMasternodeCollaterals(interfaces::WalletLoader& wallet_loader) const = 0;
    virtual void InitCoinJoinSettings(interfaces::CoinJoin::Loader& coinjoin_loader, interfaces::WalletLoader& wallet_loader) const = 0;
    virtual void InitAutoBackup() const = 0;

    virtual ~WalletInitInterface() {}
};

extern const WalletInitInterface& g_wallet_init_interface;

#endif // BITCOIN_WALLETINITINTERFACE_H
