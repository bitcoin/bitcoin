// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

class ArgsManager;
class BanMan;
class CConnman;
class CScheduler;
class CTxMemPool;
class ChainstateManager;
class PeerLogicValidation;
namespace interfaces {
class Chain;
class ChainClient;
} // namespace interfaces

//! NodeContext struct containing references to chain state and connection
//! state.
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
struct NodeContext {
    std::unique_ptr<CConnman> connman;
    CTxMemPool* mempool{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<PeerLogicValidation> peer_logic;
    ChainstateManager* chainman{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<interfaces::Chain> chain;
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};

#endif // BITCOIN_NODE_CONTEXT_H
