// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "server.h"
#include "validation.h"

#include "llmq/quorums.h"
#include "llmq/quorums_debug.h"
#include "llmq/quorums_dkgsession.h"

void quorum_list_help()
{
    throw std::runtime_error(
            "quorum list ( count )\n"
            "\nArguments:\n"
            "1. count           (number, optional, default=10) Number of quorums to list.\n"
    );
}

UniValue quorum_list(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        quorum_list_help();

    LOCK(cs_main);

    int count = 10;
    if (request.params.size() > 1) {
        count = ParseInt32V(request.params[1], "count");
    }

    UniValue ret(UniValue::VOBJ);

    for (auto& p : Params().GetConsensus().llmqs) {
        UniValue v(UniValue::VARR);

        auto quorums = llmq::quorumManager->ScanQuorums(p.first, chainActive.Tip()->GetBlockHash(), count);
        for (auto& q : quorums) {
            v.push_back(q->quorumHash.ToString());
        }

        ret.push_back(Pair(p.second.name, v));
    }


    return ret;
}

void quorum_info_help()
{
    throw std::runtime_error(
            "quorum info llmqType \"quorumHash\" ( includeSkShare )\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"quorumHash\"          (string, required) Block hash of quorum.\n"
            "3. includeSkShare        (boolean, optional) Include secret key share in output.\n"
    );
}

UniValue quorum_info(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 3 && request.params.size() != 4))
        quorum_info_help();

    LOCK(cs_main);

    Consensus::LLMQType llmqType = (Consensus::LLMQType)ParseInt32V(request.params[1], "llmqType");
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 blockHash = ParseHashV(request.params[2], "quorumHash");
    bool includeSkShare = false;
    if (request.params.size() > 3) {
        includeSkShare = ParseBoolV(request.params[3], "includeSkShare");
    }

    auto quorum = llmq::quorumManager->GetQuorum(llmqType, blockHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    UniValue ret(UniValue::VOBJ);

    ret.push_back(Pair("height", quorum->height));
    ret.push_back(Pair("quorumHash", quorum->quorumHash.ToString()));

    UniValue membersArr(UniValue::VARR);
    for (size_t i = 0; i < quorum->members.size(); i++) {
        auto& dmn = quorum->members[i];
        UniValue mo(UniValue::VOBJ);
        mo.push_back(Pair("proTxHash", dmn->proTxHash.ToString()));
        mo.push_back(Pair("valid", quorum->validMembers[i]));
        if (quorum->validMembers[i]) {
            CBLSPublicKey pubKey = quorum->GetPubKeyShare(i);
            if (pubKey.IsValid()) {
                mo.push_back(Pair("pubKeyShare", pubKey.ToString()));
            }
        }
        membersArr.push_back(mo);
    }

    ret.push_back(Pair("members", membersArr));
    ret.push_back(Pair("quorumPublicKey", quorum->quorumPublicKey.ToString()));
    CBLSSecretKey skShare = quorum->GetSkShare();
    if (includeSkShare && skShare.IsValid()) {
        ret.push_back(Pair("secretKeyShare", skShare.ToString()));
    }

    return ret;
}

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

[[ noreturn ]] void quorum_help()
{
    throw std::runtime_error(
            "quorum \"command\" ...\n"
            "Set of commands for quorums/LLMQs.\n"
            "To get help on individual commands, use \"help quorum command\".\n"
            "\nArguments:\n"
            "1. \"command\"        (string, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  list              - List of on-chain quorums\n"
            "  info              - Return information about a quorum\n"
            "  dkgstatus         - Return the status of the current DKG process\n"
    );
}

UniValue quorum(const JSONRPCRequest& request)
{
    if (request.fHelp && request.params.empty()) {
        quorum_help();
    }

    std::string command;
    if (request.params.size() >= 1) {
        command = request.params[0].get_str();
    }

    if (command == "list") {
        return quorum_list(request);
    } else if (command == "info") {
        return quorum_info(request);
    } else if (command == "dkgstatus") {
        return quorum_dkgstatus(request);
    } else {
        quorum_help();
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
