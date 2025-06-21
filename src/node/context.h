// Copyright (c) 2019-2021 The Bitcoin Core developers
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
class CActiveMasternodeManager;
class AddrMan;
class CBlockPolicyEstimator;
class CConnman;
class CCreditPoolManager;
class CDeterministicMNManager;
class CChainstateHelper;
class ChainstateManager;
class CEvoDB;
class CGovernanceManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNetFulfilledRequestManager;
class CScheduler;
class CSporkManager;
class CTxMemPool;
class CMNHFManager;
class NetGroupManager;
class PeerManager;
struct CJContext;
struct LLMQContext;

namespace interfaces {
class Chain;
class ChainClient;
class Init;
class WalletLoader;
namespace CoinJoin {
class Loader;
} // namspace CoinJoin
} // namespace interfaces

namespace node {
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
    //! Init interface for initializing current process and connecting to other processes.
    interfaces::Init* init{nullptr};
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    std::unique_ptr<CTxMemPool> mempool;
    std::unique_ptr<const NetGroupManager> netgroupman;
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator;
    std::unique_ptr<PeerManager> peerman;
    std::unique_ptr<ChainstateManager> chainman;
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<interfaces::Chain> chain;
    //! List of all chain clients (wallet processes or other client) connected to node.
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    //! Reference to chain client that should used to load or create wallets
    //! opened by the gui.
    interfaces::WalletLoader* wallet_loader{nullptr};
    std::unique_ptr<interfaces::CoinJoin::Loader> coinjoin_loader{nullptr};
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};
    //! Dash
    std::unique_ptr<CActiveMasternodeManager> mn_activeman;
    std::unique_ptr<CCreditPoolManager> cpoolman;
    std::unique_ptr<CEvoDB> evodb;
    std::unique_ptr<CChainstateHelper> chain_helper;
    std::unique_ptr<CDeterministicMNManager> dmnman;
    std::unique_ptr<CGovernanceManager> govman;
    std::unique_ptr<CJContext> cj_ctx;
    std::unique_ptr<CMasternodeMetaMan> mn_metaman;
    std::unique_ptr<CMasternodeSync> mn_sync;
    std::unique_ptr<CMNHFManager> mnhf_manager;
    std::unique_ptr<CNetFulfilledRequestManager> netfulfilledman;
    std::unique_ptr<CSporkManager> sporkman;
    std::unique_ptr<LLMQContext> llmq_ctx;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};
} // namespace node

#endif // BITCOIN_NODE_CONTEXT_H
