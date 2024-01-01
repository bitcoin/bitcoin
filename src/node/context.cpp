// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>

#include <addrman.h>
#include <banman.h>
#include <coinjoin/context.h>
#include <evo/creditpool.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <llmq/context.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <net.h>
#include <net_processing.h>
#include <policy/fees.h>
#include <scheduler.h>
#include <txmempool.h>

NodeContext::NodeContext() {}
NodeContext::~NodeContext() {}
