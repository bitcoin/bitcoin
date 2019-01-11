// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "server.h"
#include "validation.h"

#include "llmq/quorums_debug.h"
#include "llmq/quorums_dkgsession.h"

void quorum_dkgstatus_help()
{
    throw std::runtime_error(
            "quorum dkgstatus (\"proTxHash\" detail_level)\n"
            "\nArguments:\n"
            "1. \"proTxHash\"          (string, optional, default=0) ProTxHash of masternode to show status for.\n"
            "                        If set to an empty string, the local status is shown.\n"
            "2. detail_level         (number, optional, default=\"\") Detail level of output.\n"
            "                        0=Only show counts. 1=Show member indexes. 2=Show member's ProTxHashes.\n"
    );
}

UniValue quorum_dkgstatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 1 || request.params.size() > 3)) {
        quorum_dkgstatus_help();
    }

    uint256 proTxHash;
    if (request.params.size() > 1 && request.params[1].get_str() != "") {
        proTxHash = ParseHashV(request.params[1], "proTxHash");
    }

    int detailLevel = 0;
    if (request.params.size() > 2) {
        detailLevel = ParseInt32V(request.params[2], "detail_level");
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    if (proTxHash.IsNull()) {
        llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);
    } else {
        if (!llmq::quorumDKGDebugManager->GetDebugStatusForMasternode(proTxHash, status)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("no status for %s found", proTxHash.ToString()));
        }
    }

    return status.ToJson(detailLevel);
}

UniValue quorum(const JSONRPCRequest& request)
{
    if (request.params.empty()) {
        throw std::runtime_error(
                "quorum \"command\" ...\n"
        );
    }

    std::string command = request.params[0].get_str();

    if (command == "dkgstatus") {
        return quorum_dkgstatus(request);
    } else {
        throw std::runtime_error("invalid command: " + command);
    }
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "evo",                "quorum",                 &quorum,                 false, {}  },
};

void RegisterQuorumsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
