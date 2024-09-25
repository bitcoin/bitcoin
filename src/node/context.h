// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <atomic>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

class ArgsManager;
class AddrMan;
class BanMan;
class BaseIndex;
class CBlockPolicyEstimator;
class CConnman;
class ValidationSignals;
class CScheduler;
class CTxMemPool;
class ChainstateManager;
class ECC_Context;
class NetGroupManager;
class PeerManager;
namespace interfaces {
class Chain;
class ChainClient;
class Mining;
class Init;
class WalletLoader;
} // namespace interfaces
namespace kernel {
struct Context;
}
namespace util {
class SignalInterrupt;
}

namespace node {
class KernelNotifications;
class Warnings;

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
    //! libbitcoin_kernel context
    std::unique_ptr<kernel::Context> kernel;
    std::unique_ptr<ECC_Context> ecc_context;
    //! Init interface for initializing current process and connecting to other processes.
    interfaces::Init* init{nullptr};
    //! Function to request a shutdown.
    std::function<bool()> shutdown_request;
    //! Interrupt object used to track whether node shutdown was requested.
    util::SignalInterrupt* shutdown_signal{nullptr};
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    std::unique_ptr<CTxMemPool> mempool;
    std::unique_ptr<const NetGroupManager> netgroupman;
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator;
    std::unique_ptr<PeerManager> peerman;
    std::unique_ptr<ChainstateManager> chainman;
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::vector<BaseIndex*> indexes; // raw pointers because memory is not managed by this struct
    std::unique_ptr<interfaces::Chain> chain;
    //! List of all chain clients (wallet processes or other client) connected to node.
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    //! Reference to chain client that should used to load or create wallets
    //! opened by the gui.
    std::unique_ptr<interfaces::Mining> mining;
    interfaces::WalletLoader* wallet_loader{nullptr};
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};
    //! Issues blocking calls about sync status, errors and warnings
    std::unique_ptr<KernelNotifications> notifications;
    //! Issues calls about blocks and transactions
    std::unique_ptr<ValidationSignals> validation_signals;
    std::atomic<int> exit_status{EXIT_SUCCESS};
    //! Manages all the node warnings
    std::unique_ptr<node::Warnings> warnings;
    std::thread background_init_thread;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};
} // namespace node

#endif // BITCOIN_NODE_CONTEXT_H
