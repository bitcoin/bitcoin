// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_IPC_CAPNP_WALLET_H
#define BITCOIN_IPC_CAPNP_WALLET_H

#include <interfaces/wallet.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/wallet.capnp.h>
#include <mp/proxy.h>

class CScheduler;
namespace interfaces {
class WalletLoader;
} // namespace interfaces
namespace ipc {
namespace capnp {
namespace messages {
class SchedulerThread;
struct WalletLoader;
} // namespace messages
} // namespace capnp
} // namespace ipc

//! Specialization of WalletLoader proxy server needed to hold a CScheduler instance.
template <>
struct mp::ProxyServerCustom<ipc::capnp::messages::WalletLoader, interfaces::WalletLoader>
    : public mp::ProxyServerBase<ipc::capnp::messages::WalletLoader, interfaces::WalletLoader>
{
public:
    using ProxyServerBase::ProxyServerBase;
    ipc::capnp::messages::SchedulerThread* m_scheduler{nullptr};
};

#endif // BITCOIN_IPC_CAPNP_WALLET_H
