// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_CONTEXT_H
#define SYSCOIN_WALLET_CONTEXT_H

#include <sync.h>

#include <functional>
#include <list>
#include <memory>
#include <vector>

class ArgsManager;
class CWallet;
namespace interfaces {
class Chain;
class Wallet;
} // namespace interfaces
// SYSCOIN
struct NodeContext;
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
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    Mutex wallets_mutex;
    std::vector<std::shared_ptr<CWallet>> wallets GUARDED_BY(wallets_mutex);
    std::list<LoadWalletFn> wallet_load_fns GUARDED_BY(wallets_mutex);

    /* SYSCOIN getauxwork is a wallet RPC but actually needs the NodeContext (unlike
       any of the upstream Bitcoin wallet RPCs).  */
    NodeContext* nodeContext{nullptr};
    WalletContext(const WalletContext& contextIn)  {
        chain = contextIn.chain;
        args = contextIn.args;
        nodeContext = contextIn.nodeContext;
    }    
    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the WalletContext struct doesn't need to #include class
    //! definitions for smart pointer and container members.
    WalletContext();
    ~WalletContext();
};

#endif // SYSCOIN_WALLET_CONTEXT_H
