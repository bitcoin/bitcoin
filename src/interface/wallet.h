// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACE_WALLET_H
#define BITCOIN_INTERFACE_WALLET_H

#include <functional>
#include <memory>
#include <string>

class CWallet;

namespace interface {

class Handler;

//! Interface for accessing a wallet.
class Wallet
{
public:
    virtual ~Wallet() {}

    //! Register handler for show progress messages.
    using ShowProgressFn = std::function<void(const std::string& title, int progress)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;
};

//! Return implementation of Wallet interface. This function will be undefined
//! in builds where ENABLE_WALLET is false.
std::unique_ptr<Wallet> MakeWallet(CWallet& wallet);

} // namespace interface

#endif // BITCOIN_INTERFACE_WALLET_H
