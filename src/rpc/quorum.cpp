// Copyright (c) 2017-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <validation.h>

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_signing.h>

void quorum_list_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum list", "",
            {
                {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of quorums to list. Will list active quorums.\n"
                    "                   if \"count\" is not specified."}
            },
            RPCResult{
                "{\n"
                "  \"quorumName\" : [                    (array of strings) List of quorum hashes per some quorum type.\n"
                "     \"quorumHash\"                     (string) Quorum hash. Note: most recent quorums come first.\n"
                "     ,...\n"
                "  ],\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("quorum", "list") +
                HelpExampleCli("quorum", "list 10") +
                HelpExampleRpc("quorum", "list, 10")
            }
        }.ToString()
    );
}

UniValue quorum_list(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        quorum_list_help();

    LOCK(cs_main);

    int count = -1;
    if (request.params.size() > 1) {
        count = UniValue(UniValue::VNUM, request.params[1].getValStr()).get_int();
        if (count < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    for (auto& p : Params().GetConsensus().llmqs) {
        UniValue v(UniValue::VARR);

        auto quorums = llmq::quorumManager->ScanQuorums(p.first, ChainActive().Tip(), count > -1 ? count : p.second.signingActiveQuorumCount);
        for (auto& q : quorums) {
            v.push_back(q->qc.quorumHash.ToString());
        }

        ret.pushKV(p.second.name, v);
    }

    return ret;
}

void quorum_info_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum info", "",
            {
                {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},
                {"quorumHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Block hash of quorum.\n"},
                {"includeSkShare", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Include secret key share in output.\n"}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

UniValue BuildQuorumInfo(const llmq::CQuorumCPtr& quorum, bool includeMembers, bool includeSkShare)
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("height", quorum->pindexQuorum->nHeight);
    ret.pushKV("type", quorum->params.name);
    ret.pushKV("quorumHash", quorum->qc.quorumHash.ToString());
    ret.pushKV("minedBlock", quorum->minedBlockHash.ToString());

    if (includeMembers) {
        UniValue membersArr(UniValue::VARR);
        for (size_t i = 0; i < quorum->members.size(); i++) {
            auto& dmn = quorum->members[i];
            UniValue mo(UniValue::VOBJ);
            mo.pushKV("proTxHash", dmn->proTxHash.ToString());
            mo.pushKV("valid", quorum->qc.validMembers[i]);
            if (quorum->qc.validMembers[i]) {
                CBLSPublicKey pubKey = quorum->GetPubKeyShare(i);
                if (pubKey.IsValid()) {
                    mo.pushKV("pubKeyShare", pubKey.ToString());
                }
            }
            membersArr.push_back(mo);
        }

        ret.pushKV("members", membersArr);
    }
    ret.pushKV("quorumPublicKey", quorum->qc.quorumPublicKey.ToString());
    CBLSSecretKey skShare = quorum->GetSkShare();
    if (includeSkShare && skShare.IsValid()) {
        ret.pushKV("secretKeyShare", skShare.ToString());
    }
    return ret;
}

UniValue quorum_info(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 3 && request.params.size() != 4))
        quorum_info_help();

    LOCK(cs_main);

    Consensus::LLMQType llmqType = (Consensus::LLMQType)UniValue(UniValue::VNUM, request.params[1].getValStr()).get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const auto& llmqParams = Params().GetConsensus().llmqs.at(llmqType);

    uint256 quorumHash = ParseHashV(request.params[2], "quorumHash");
    bool includeSkShare = false;
    if (request.params.size() > 3) {
        includeSkShare = request.params[3].get_bool();
    }

    auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    return BuildQuorumInfo(quorum, true, includeSkShare);
}

void quorum_dkgstatus_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum dkgstatus",
            "\nReturn the status of the current DKG process.\n"
            "Works only when SPORK_3_QUORUM_DKG_ENABLED spork is ON.\n",
            {
                {"detail_level", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Detail level of output.\n"
                    "                        0=Only show counts. 1=Show member indexes. 2=Show member's ProTxHashes."}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

UniValue quorum_dkgstatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 1 || request.params.size() > 2)) {
        quorum_dkgstatus_help();
    }

    int detailLevel = 0;
    if (request.params.size() > 1) {
        detailLevel = UniValue(UniValue::VNUM, request.params[1].getValStr()).get_int();
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);

    auto ret = status.ToJson(detailLevel);

    LOCK(cs_main);
    int tipHeight = ChainActive().Height();

    UniValue minableCommitments(UniValue::VOBJ);
    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& params = p.second;
        llmq::CFinalCommitment fqc;
        if (llmq::quorumBlockProcessor->GetMinableCommitment(params.type, tipHeight, fqc)) {
            UniValue obj(UniValue::VOBJ);
            fqc.ToJson(obj);
            minableCommitments.pushKV(params.name, obj);
        }
    }

    ret.pushKV("minableCommitments", minableCommitments);

    return ret;
}

void quorum_memberof_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum memberof",
            "\nChecks which quorums the given masternode is a member of.\n",
            {
                {"proTxHash", RPCArg::Type::STR, RPCArg::Optional::NO, "ProTxHash of the masternode.\n"},
                {"scanQuorumsCount", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of quorums to scan for. If not specified,\n"
                    "                              the active quorum count for each specific quorum type is used."
                },
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

UniValue quorum_memberof(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 2 || request.params.size() > 3)) {
        quorum_memberof_help();
    }

    uint256 protxHash = ParseHashV(request.params[1], "proTxHash");
    int scanQuorumsCount = -1;
    if (request.params.size() >= 3) {
        scanQuorumsCount = UniValue(UniValue::VNUM, request.params[2].getValStr()).get_int();
        if (scanQuorumsCount <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid scanQuorumsCount parameter");
        }
    }

    const CBlockIndex* pindexTip;
    {
        LOCK(cs_main);
        pindexTip = ChainActive().Tip();
    }

    auto mnList = deterministicMNManager->GetListForBlock(pindexTip);
    auto dmn = mnList.GetMN(protxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "masternode not found");
    }

    UniValue result(UniValue::VARR);

    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& params = p.second;
        size_t count = params.signingActiveQuorumCount;
        if (scanQuorumsCount != -1) {
            count = (size_t)scanQuorumsCount;
        }
        auto quorums = llmq::quorumManager->ScanQuorums(params.type, count);
        for (auto& quorum : quorums) {
            if (quorum->IsMember(dmn->proTxHash)) {
                auto json = BuildQuorumInfo(quorum, false, false);
                json.pushKV("isValidMember", quorum->IsValidMember(dmn->proTxHash));
                json.pushKV("memberIndex", quorum->GetMemberIndex(dmn->proTxHash));
                result.push_back(json);
            }
        }
    }

    return result;
}

void quorum_sign_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum sign", "",
            {
                {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},
                {"id", RPCArg::Type::STR, RPCArg::Optional::NO, "Request id."},
                {"msgHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Message hash."}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

void quorum_hasrecsig_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum hasrecsig", "",
            {
                {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},
                {"id", RPCArg::Type::STR, RPCArg::Optional::NO, "Request id."},
                {"msgHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Message hash."}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

void quorum_getrecsig_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum getrecsig", "",
            {
                {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},
                {"id", RPCArg::Type::STR, RPCArg::Optional::NO, "Request id."},
                {"msgHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Message hash."}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

void quorum_isconflicting_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum isconflicting", "",
            {
                {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},
                {"id", RPCArg::Type::STR, RPCArg::Optional::NO, "Request id."},
                {"msgHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Message hash."}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
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

    Consensus::LLMQType llmqType = (Consensus::LLMQType)UniValue(UniValue::VNUM, request.params[1].getValStr()).get_int();
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
        RPCHelpMan{"quorum dkgsimerror",
            "\nThis enables simulation of errors and malicious behaviour in the DKG. Do NOT use this on mainnet\n"
            "as you will get yourself very likely PoSe banned for this.\n",
            {
                {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Error type.\n"},
                {"rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Rate at which to simulate this error type..\n"}
            },
            RPCResults{},
            RPCExamples{""}
        }.ToString()
    );
}

UniValue quorum_dkgsimerror(const JSONRPCRequest& request)
{
    auto cmd = request.params[0].get_str();
    if (request.fHelp || (request.params.size() != 3)) {
        quorum_dkgsimerror_help();
    }

    std::string type = request.params[1].get_str();
    double rate = UniValue(UniValue::VNUM, request.params[2].getValStr()).get_real();

    if (rate < 0 || rate > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 1");
    }

    llmq::SetSimulatedDKGErrorRate(type, rate);

    return UniValue();
}


[[ noreturn ]] void quorum_help()
{
    throw std::runtime_error(
        RPCHelpMan{"quorum",
            "\nSet of commands for quorums/LLMQ actions.\n",
            {
                {"command", RPCArg::Type::STR, /* default */ "all commands", "The command to get help on"},
            },
            RPCResult{
                "  list              - List of on-chain quorums\n"
                "  info              - Return information about a quorum\n"
                "  dkgsimerror       - Simulates DKG errors and malicious behavior.\n"
                "  dkgstatus         - Return the status of the current DKG process\n"
                "  memberof          - Checks which quorums the given masternode is a member of\n"
                "  sign              - Threshold-sign a message\n"
                "  hasrecsig         - Test if a valid recovered signature is present\n"
                "  getrecsig         - Get a recovered signature\n"
                "  isconflicting     - Test if a conflict exists\n"
            },
            RPCExamples{""},
        }.ToString()
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
    } else if (command == "memberof") {
        return quorum_memberof(request);
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
  {   "quorum",             "quorum",                 &quorum,                 {}  },
};

void RegisterQuorumsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
