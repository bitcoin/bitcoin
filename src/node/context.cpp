// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>

#include <addrman.h>
#include <banman.h>
#include <interfaces/chain.h>
#include <net.h>
#include <netfulfilledman.h>
#include <net_processing.h>
#include <netgroup.h>
#include <policy/fees.h>
#include <scheduler.h>
#include <txmempool.h>
#include <validation.h>

#include <active/context.h>
#include <chainlock/chainlock.h>
#include <chainlock/handler.h>
#include <coinjoin/coinjoin.h>
#include <coinjoin/walletman.h>
#include <evo/chainhelper.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <governance/governance.h>
#include <interfaces/coinjoin.h>
#include <llmq/context.h>
#include <llmq/observer/context.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <spork.h>

namespace node {
NodeContext::NodeContext() = default;
NodeContext::~NodeContext() = default;
} // namespace node
