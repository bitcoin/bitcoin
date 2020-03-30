// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <util/memory.h>
#include <wallet/rpcwallet.h>

namespace interfaces {
namespace {

class WalletRPCsClientImpl : public ChainClient
{
public:
    WalletRPCsClientImpl(Chain& chain)
        : m_chain(chain)
    {
    }
    void registerRpcs() override
    {
        g_rpc_chain = &m_chain;
        return RegisterWalletRPCCommands(m_chain, m_rpc_handlers);
    }

    Chain& m_chain;
    std::vector<std::unique_ptr<Handler>> m_rpc_handlers;
};

} // namespace

std::unique_ptr<ChainClient> MakeWalletRPCsClient(Chain& chain)
{
    return MakeUnique<WalletRPCsClientImpl>(chain);
}

} // namespace interfaces
