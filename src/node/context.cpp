// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>

#include <addrman.h>
#include <banman.h>
#include <coinjoin/context.h>
#include <evo/chainhelper.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <governance/governance.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <llmq/context.h>
#include <masternode/active/context.h>
#include <masternode/meta.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net.h>
#include <netfulfilledman.h>
#include <net_processing.h>
#include <netgroup.h>
#include <policy/fees.h>
#include <scheduler.h>
#include <spork.h>
#include <txmempool.h>
#include <validation.h>

namespace node {
NodeContext::NodeContext() {}
NodeContext::~NodeContext() {}
} // namespace node
