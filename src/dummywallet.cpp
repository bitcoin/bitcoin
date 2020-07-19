// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/system.h>
#include <walletinitinterface.h>
#include <support/allocators/secure.h>

class CWallet;
enum class WalletCreationStatus;
struct bilingual_str;

namespace interfaces {
class Chain;
class Handler;
class Wallet;
}

class DummyWalletInit : public WalletInitInterface {
public:

    bool HasWalletSupport() const override {return false;}
    void AddWalletOptions(ArgsManager& argsman) const override;
    bool ParameterInteraction() const override {return true;}
    void Construct(NodeContext& node) const override {LogPrintf("No wallet support compiled in!\n");}
};

void DummyWalletInit::AddWalletOptions(ArgsManager& argsman) const
{
    argsman.AddHiddenArgs({
        "-addresstype",
        "-avoidpartialspends",
        "-changetype",
        "-disablewallet",
        "-discardfee=<amt>",
        "-fallbackfee=<amt>",
        "-keypool=<n>",
        "-maxtxfee=<amt>",
        "-mintxfee=<amt>",
        "-paytxfee=<amt>",
        "-rescan",
        "-salvagewallet",
        "-spendzeroconfchange",
        "-txconfirmtarget=<n>",
        "-upgradewallet",
        "-wallet=<path>",
        "-walletbroadcast",
        "-walletdir=<dir>",
        "-walletnotify=<cmd>",
        "-walletrbf",
        "-zapwallettxes=<mode>",
        "-dblogsize=<n>",
        "-flushwallet",
        "-privdb",
        "-walletrejectlongchains",
    });
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();

fs::path GetWalletDir()
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

std::vector<fs::path> ListWalletDir()
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

std::vector<std::shared_ptr<CWallet>> GetWallets()
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const std::string& name, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

WalletCreationStatus CreateWallet(interfaces::Chain& chain, const SecureString& passphrase, uint64_t wallet_creation_flags, const std::string& name, bilingual_str& error, std::vector<bilingual_str>& warnings, std::shared_ptr<CWallet>& result)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;
std::unique_ptr<interfaces::Handler> HandleLoadWallet(LoadWalletFn load_wallet)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

namespace interfaces {

std::unique_ptr<Wallet> MakeWallet(const std::shared_ptr<CWallet>& wallet)
{
    throw std::logic_error("Wallet function called in non-wallet build.");
}

} // namespace interfaces
