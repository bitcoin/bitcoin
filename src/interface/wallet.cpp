// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interface/wallet.h>

#include <interface/handler.h>
#include <wallet/wallet.h>

#include <memory>

namespace interface {
namespace {

class WalletImpl : public Wallet
{
public:
    WalletImpl(CWallet& wallet) : m_wallet(wallet) {}

    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeHandler(m_wallet.ShowProgress.connect(fn));
    }

    CWallet& m_wallet;
};

} // namespace

std::unique_ptr<Wallet> MakeWallet(CWallet& wallet) { return MakeUnique<WalletImpl>(wallet); }

} // namespace interface
