// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/coinjoin.h>

#include <coinjoin/client.h>
#include <wallet/wallet.h>

#include <memory>
#include <string>

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
    std::string getSessionDenoms() override
    {
        return m_clientman.GetSessionDenoms();
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
    CoinJoinWalletManager& m_walletman;

public:
    explicit CoinJoinLoaderImpl(CoinJoinWalletManager& walletman)
        : m_walletman(walletman) {}

    void AddWallet(CWallet& wallet) override
    {
        m_walletman.Add(wallet);
    }
    void RemoveWallet(const std::string& name) override
    {
        m_walletman.Remove(name);
    }
    void FlushWallet(const std::string& name) override
    {
        m_walletman.Flush(name);
    }
    std::unique_ptr<interfaces::CoinJoin::Client> GetClient(const std::string& name) override
    {
        auto clientman = m_walletman.Get(name);
        return clientman ? std::make_unique<CoinJoinClientImpl>(*clientman) : nullptr;
    }
    CoinJoinWalletManager& walletman() override
    {
        return m_walletman;
    }
};

} // namespace
} // namespace coinjoin

namespace interfaces {
std::unique_ptr<CoinJoin::Loader> MakeCoinJoinLoader(CoinJoinWalletManager& walletman) { return std::make_unique<coinjoin::CoinJoinLoaderImpl>(walletman); }
} // namespace interfaces
