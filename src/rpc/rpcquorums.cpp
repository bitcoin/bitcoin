// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "server.h"
#include "validation.h"

#include "llmq/quorums.h"
#include "llmq/quorums_blockprocessor.h"
#include "llmq/quorums_debug.h"
#include "llmq/quorums_dkgsession.h"
#include "llmq/quorums_signing.h"

void quorum_list_help()
{
    throw std::runtime_error(
            "quorum list ( count )\n"
            "\nArguments:\n"
            "1. count           (number, optional) Number of quorums to list. Will list active quorums\n"
            "                   if \"count\" is not specified.\n"
            "\nResult:\n"
            "{\n"
            "  \"quorumName\" : [                    (array of strings) List of quorum hashes per some quorum type.\n"
            "     \"quorumHash\"                     (string) Quorum hash. Note: most recent quorums come first.\n"
            "     ,...\n"
            "  ],\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("quorum", "list")
            + HelpExampleCli("quorum", "list 10")
            + HelpExampleRpc("quorum", "list, 10")
    );
}

UniValue quorum_list(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        quorum_list_help();

    LOCK(cs_main);

    int count = -1;
    if (request.params.size() > 1) {
        count = ParseInt32V(request.params[1], "count");
        if (count < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    for (auto& p : Params().GetConsensus().llmqs) {
        UniValue v(UniValue::VARR);

        auto quorums = llmq::quorumManager->ScanQuorums(p.first, chainActive.Tip(), count > -1 ? count : p.second.signingActiveQuorumCount);
        for (auto& q : quorums) {
            v.push_back(q->qc.quorumHash.ToString());
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

    const auto& llmqParams = Params().GetConsensus().llmqs.at(llmqType);

    uint256 quorumHash = ParseHashV(request.params[2], "quorumHash");
    bool includeSkShare = false;
    if (request.params.size() > 3) {
        includeSkShare = ParseBoolV(request.params[3], "includeSkShare");
    }

    auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    UniValue ret(UniValue::VOBJ);

    ret.push_back(Pair("height", quorum->height));
    ret.push_back(Pair("quorumHash", quorum->qc.quorumHash.ToString()));
    ret.push_back(Pair("minedBlock", quorum->minedBlockHash.ToString()));

    UniValue membersArr(UniValue::VARR);
    for (size_t i = 0; i < quorum->members.size(); i++) {
        auto& dmn = quorum->members[i];
        UniValue mo(UniValue::VOBJ);
        mo.push_back(Pair("proTxHash", dmn->proTxHash.ToString()));
        mo.push_back(Pair("valid", quorum->qc.validMembers[i]));
        if (quorum->qc.validMembers[i]) {
            CBLSPublicKey pubKey = quorum->GetPubKeyShare(i);
            if (pubKey.IsValid()) {
                mo.push_back(Pair("pubKeyShare", pubKey.ToString()));
            }
        }
        membersArr.push_back(mo);
    }

    ret.push_back(Pair("members", membersArr));
    ret.push_back(Pair("quorumPublicKey", quorum->qc.quorumPublicKey.ToString()));
    CBLSSecretKey skShare = quorum->GetSkShare();
    if (includeSkShare && skShare.IsValid()) {
        ret.push_back(Pair("secretKeyShare", skShare.ToString()));
    }

    return ret;
}

void quorum_dkgstatus_help()
{
    throw std::runtime_error(
            "quorum dkgstatus ( detail_level )\n"
            "Return the status of the current DKG process.\n"
            "Works only when SPORK_17_QUORUM_DKG_ENABLED spork is ON.\n"
            "\nArguments:\n"
            "1. detail_level         (number, optional, default=0) Detail level of output.\n"
            "                        0=Only show counts. 1=Show member indexes. 2=Show member's ProTxHashes.\n"
    );
}

UniValue quorum_dkgstatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 1 || request.params.size() > 2)) {
        quorum_dkgstatus_help();
    }

    int detailLevel = 0;
    if (request.params.size() > 1) {
        detailLevel = ParseInt32V(request.params[1], "detail_level");
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);

    auto ret = status.ToJson(detailLevel);

    LOCK(cs_main);
    int tipHeight = chainActive.Height();

    UniValue minableCommitments(UniValue::VOBJ);
    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& params = p.second;
        llmq::CFinalCommitment fqc;
        if (llmq::quorumBlockProcessor->GetMinableCommitment(params.type, tipHeight, fqc)) {
            UniValue obj(UniValue::VOBJ);
            fqc.ToJson(obj);
            minableCommitments.push_back(Pair(params.name, obj));
        }
    }

    ret.push_back(Pair("minableCommitments", minableCommitments));

    return ret;
}

void quorum_sign_help()
{
    throw std::runtime_error(
            "quorum sign llmqType \"id\" \"msgHash\"\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

void quorum_hasrecsig_help()
{
    throw std::runtime_error(
            "quorum hasrecsig llmqType \"id\" \"msgHash\"\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

void quorum_getrecsig_help()
{
    throw std::runtime_error(
            "quorum getrecsig llmqType \"id\" \"msgHash\"\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

void quorum_isconflicting_help()
{
    throw std::runtime_error(
            "quorum isconflicting llmqType \"id\" \"msgHash\"\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

UniValue quorum_sigs_cmd(const JSONRPCRequest& request)
{
    auto cmd = request.params[0].get_str();
    if (request.fHelp || (request.params.size() != 4)) {
        if (cmd == "sign") {
            quorum_sign_help();
        } else if (cmd == "hasrecsig") {
            quorum_hasrecsig_help();
        } else if (cmd == "getrecsig") {
            quorum_getrecsig_help();
        } else if (cmd == "isconflicting") {
            quorum_isconflicting_help();
        } else {
            // shouldn't happen as it's already handled by the caller
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid cmd");
        }
    }

    Consensus::LLMQType llmqType = (Consensus::LLMQType)ParseInt32V(request.params[1], "llmqType");
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[2], "id");
    uint256 msgHash = ParseHashV(request.params[3], "msgHash");

    if (cmd == "sign") {
        return llmq::quorumSigningManager->AsyncSignIfMember(llmqType, id, msgHash);
    } else if (cmd == "hasrecsig") {
        return llmq::quorumSigningManager->HasRecoveredSig(llmqType, id, msgHash);
    } else if (cmd == "getrecsig") {
        llmq::CRecoveredSig recSig;
        if (!llmq::quorumSigningManager->GetRecoveredSigForId(llmqType, id, recSig)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "recovered signature not found");
        }
        if (recSig.msgHash != msgHash) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "recovered signature not found");
        }
        return recSig.ToJson();
    } else if (cmd == "isconflicting") {
        return llmq::quorumSigningManager->IsConflicting(llmqType, id, msgHash);
    } else {
        // shouldn't happen as it's already handled by the caller
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid cmd");
    }
}

void quorum_dkgsimerror_help()
{
    throw std::runtime_error(
            "quorum dkgsimerror \"type\" rate\n"
            "This enables simulation of errors and malicious behaviour in the DKG. Do NOT use this on mainnet\n"
            "as you will get yourself very likely PoSe banned for this.\n"
            "\nArguments:\n"
            "1. \"type\"                (string, required) Error type.\n"
            "2. rate                  (number, required) Rate at which to simulate this error type.\n"
    );
}

UniValue quorum_dkgsimerror(const JSONRPCRequest& request)
{
    auto cmd = request.params[0].get_str();
    if (request.fHelp || (request.params.size() != 3)) {
        quorum_dkgsimerror_help();
    }

    std::string type = request.params[1].get_str();
    double rate = ParseDoubleV(request.params[2], "rate");

    if (rate < 0 || rate > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 1");
    }

    llmq::SetSimulatedDKGErrorRate(type, rate);

    return UniValue();
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
            "  dkgsimerror       - Simulates DKG errors and malicious behavior.\n"
            "  dkgstatus         - Return the status of the current DKG process\n"
            "  sign              - Threshold-sign a message\n"
            "  hasrecsig         - Test if a valid recovered signature is present\n"
            "  getrecsig         - Get a recovered signature\n"
            "  isconflicting     - Test if a conflict exists\n"
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
    } else if (command == "sign" || command == "hasrecsig" || command == "getrecsig" || command == "isconflicting") {
        return quorum_sigs_cmd(request);
    } else if (command == "dkgsimerror") {
        return quorum_dkgsimerror(request);
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
