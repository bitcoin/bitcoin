// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_CONTEXT_H
#define BITCOIN_WALLET_CONTEXT_H

class ArgsManager;
namespace interfaces {
class Chain;
} // namespace interfaces

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
    ArgsManager* args{nullptr};

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the WalletContext struct doesn't need to #include class
    //! definitions for smart pointer and container members.
    WalletContext();
    ~WalletContext();
};

#endif // BITCOIN_WALLET_CONTEXT_H
