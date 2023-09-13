// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/context.h>

#include <net.h>
#include <policy/fees.h>
#include <txmempool.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <coinjoin/server.h>

CJContext::CJContext(CChainState& chainstate, CConnman& connman, CTxMemPool& mempool, const CMasternodeSync& mn_sync, bool relay_txes) :
#ifdef ENABLE_WALLET
    clientman {
        [&]() -> CJClientManager* const {
            assert(::coinJoinClientManagers == nullptr);
            ::coinJoinClientManagers = std::make_unique<CJClientManager>(connman, mempool, mn_sync, queueman);
            return ::coinJoinClientManagers.get();
        }()
    },
    queueman {relay_txes ? std::make_unique<CCoinJoinClientQueueManager>(connman, *clientman, mn_sync) : nullptr},
#endif // ENABLE_WALLET
    server{std::make_unique<CCoinJoinServer>(chainstate, connman, mempool, mn_sync)}
{}

CJContext::~CJContext() {
#ifdef ENABLE_WALLET
    ::coinJoinClientManagers.reset();
#endif // ENABLE_WALLET
}
