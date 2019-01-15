// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_init.h"

#include "quorums_blockprocessor.h"
#include "quorums_commitment.h"
#include "quorums_debug.h"
#include "quorums_dkgsessionmgr.h"

#include "scheduler.h"

namespace llmq
{

static CBLSWorker blsWorker;

void InitLLMQSystem(CEvoDB& evoDb, CScheduler* scheduler)
{
    quorumDKGDebugManager = new CDKGDebugManager(scheduler);
    quorumBlockProcessor = new CQuorumBlockProcessor(evoDb);
    quorumDKGSessionManager = new CDKGSessionManager(evoDb, blsWorker);
}

void DestroyLLMQSystem()
{
    delete quorumDKGSessionManager;
    quorumDKGSessionManager = NULL;
    delete quorumBlockProcessor;
    quorumBlockProcessor = nullptr;
    delete quorumDKGDebugManager;
    quorumDKGDebugManager = nullptr;
}

}
