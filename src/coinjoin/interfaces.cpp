// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/coinjoin.h>

#include <coinjoin/client.h>
#include <coinjoin/context.h>
#include <coinjoin/options.h>
#include <node/context.h>
#include <util/check.h>
#include <walletinitinterface.h>

#include <memory>
#include <string>

using node::NodeContext;
using wallet::CWallet;

namespace coinjoin {
namespace {

class CoinJoinClientImpl : public interfaces::CoinJoin::Client
{
    CCoinJoinClientManager& m_clientman;

public:
    explicit CoinJoinClientImpl(CCoinJoinClientManager& clientman)
        : m_clientman(clientman) {}

    void resetCachedBlocks() override
    {
        m_clientman.nCachedNumBlocks = std::numeric_limits<int>::max();
    }
    void resetPool() override
    {
        m_clientman.ResetPool();
    }
    void disableAutobackups() override
    {
        m_clientman.fCreateAutoBackups = false;
    }
    int getCachedBlocks() override
    {
        return m_clientman.nCachedNumBlocks;
    }
    void getJsonInfo(UniValue& obj) override
    {
        return m_clientman.GetJsonInfo(obj);
    }
    std::string getSessionDenoms() override
    {
        return m_clientman.GetSessionDenoms();
    }
    std::vector<std::string> getSessionStatuses() override
    {
        return m_clientman.GetStatuses();
    }
    void setCachedBlocks(int nCachedBlocks) override
    {
       m_clientman.nCachedNumBlocks = nCachedBlocks;
    }
    bool isMixing() override
    {
        return m_clientman.IsMixing();
    }
    bool startMixing() override
    {
        return m_clientman.StartMixing();
    }
    void stopMixing() override
    {
        m_clientman.StopMixing();
    }
};

class CoinJoinLoaderImpl : public interfaces::CoinJoin::Loader
{
private:
    CoinJoinWalletManager& walletman()
    {
        return *Assert(Assert(m_node.cj_ctx)->walletman);
    }

    interfaces::WalletLoader& wallet_loader()
    {
        return *Assert(m_node.wallet_loader);
    }

public:
    explicit CoinJoinLoaderImpl(NodeContext& node) :
        m_node(node)
    {
        // Enablement will be re-evaluated when a wallet is added or removed
        CCoinJoinClientOptions::SetEnabled(false);
    }

    void AddWallet(const std::shared_ptr<CWallet>& wallet) override
    {
        walletman().Add(wallet);
        g_wallet_init_interface.InitCoinJoinSettings(*this, wallet_loader());
    }
    void RemoveWallet(const std::string& name) override
    {
        walletman().Remove(name);
        g_wallet_init_interface.InitCoinJoinSettings(*this, wallet_loader());
    }
    void FlushWallet(const std::string& name) override
    {
        walletman().Flush(name);
    }
    std::unique_ptr<interfaces::CoinJoin::Client> GetClient(const std::string& name) override
    {
        auto clientman = walletman().Get(name);
        return clientman ? std::make_unique<CoinJoinClientImpl>(*clientman) : nullptr;
    }

    NodeContext& m_node;
};

} // namespace
} // namespace coinjoin

namespace interfaces {
std::unique_ptr<CoinJoin::Loader> MakeCoinJoinLoader(NodeContext& node) { return std::make_unique<coinjoin::CoinJoinLoaderImpl>(node); }
} // namespace interfaces
