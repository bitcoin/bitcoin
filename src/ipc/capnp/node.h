// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_NODE_H
#define BITCOIN_IPC_CAPNP_NODE_H

#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <ipc/capnp/node.capnp.h>
#include <mp/proxy.h>
#include <rpc/server.h>
#include <scheduler.h>

#include <memory>
#include <string>

class RPCTimerInterface;

//! Specialization of Node proxy server needed to add m_timer_interface
//! member used by rpcSetTimerInterfaceIfUnset and rpcUnsetTimerInterface
//! methods.
template <>
struct mp::ProxyServerCustom<ipc::capnp::messages::Node, interfaces::Node>
    : public mp::ProxyServerBase<ipc::capnp::messages::Node, interfaces::Node>
{
public:
    using ProxyServerBase::ProxyServerBase;
    std::unique_ptr<RPCTimerInterface> m_timer_interface;
};

//! Specialization of Node client to manage memory of WalletLoader& reference
//! returned by walletLoader().
template <>
class mp::ProxyClientCustom<ipc::capnp::messages::Node, interfaces::Node>
    : public mp::ProxyClientBase<ipc::capnp::messages::Node, interfaces::Node>
{
public:
    using ProxyClientBase::ProxyClientBase;
    interfaces::WalletLoader& walletLoader() override;

private:
    std::unique_ptr<interfaces::WalletLoader> m_wallet_loader;
};

//! Specialization of Node::walletLoader client code to manage memory of
//! WalletLoader& reference returned by walletLoader().
template <>
struct mp::ProxyClientMethodTraits<ipc::capnp::messages::Node::CustomWalletLoaderParams>
    : public FunctionTraits<std::unique_ptr<interfaces::WalletLoader> (interfaces::Node::*const)()>
{
};

#endif // BITCOIN_IPC_CAPNP_NODE_H
