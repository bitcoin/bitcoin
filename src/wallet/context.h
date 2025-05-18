// Copyright (c) 2020-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_WALLET_CONTEXT_H
#define TORTOISECOIN_WALLET_CONTEXT_H

#include <sync.h>

#include <functional>
#include <list>
#include <memory>
#include <vector>

class ArgsManager;
class CScheduler;
namespace interfaces {
class Chain;
class Wallet;
} // namespace interfaces

namespace wallet {
class CWallet;
using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;

//! WalletContext struct containing references to state shared between CWallet
//! instances, like the reference to the chain interface, and the list of opened
//! wallets.
//!
//! Future shared state can be added here as an alternative to adding global
//! variables.
//!
//! The struct isn't intended to have any member functions. It should just be a
//! collection of state pointers that doesn't pull in dependencies or implement
//! behavior.
struct WalletContext {
    interfaces::Chain* chain{nullptr};
    CScheduler* scheduler{nullptr};
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    // It is unsafe to lock this after locking a CWallet::cs_wallet mutex because
    // this could introduce inconsistent lock ordering and cause deadlocks.
    Mutex wallets_mutex;
    std::vector<std::shared_ptr<CWallet>> wallets GUARDED_BY(wallets_mutex);
    std::list<LoadWalletFn> wallet_load_fns GUARDED_BY(wallets_mutex);

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the WalletContext struct doesn't need to #include class
    //! definitions for smart pointer and container members.
    WalletContext();
    ~WalletContext();
};
} // namespace wallet

#endif // TORTOISECOIN_WALLET_CONTEXT_H
