// Copyright (c) 2017-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <rpc/server.h>
#include <validation.h>

#include <masternode/activemasternode.h>
#include <evo/deterministicmns.h>

#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_signing_shares.h>
#include <rpc/util.h>
#include <net.h>
#include <rpc/blockchain.h>
#include <node/context.h>

static RPCHelpMan quorum_list()
{
    return RPCHelpMan{"quorum_list",
        "\nList of on-chain quorums\n",
        {
            {"count", RPCArg::Type::NUM, RPCArg::Default{0}, "Number of quorums to list. Will list active quorums\n"
                                "if \"count\" is not specified.\n"},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_list", "1")
            + HelpExampleRpc("quorum_list", "1")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    LOCK(cs_main);

    int count = -1;
    if (!request.params[0].isNull()) {
        count = request.params[0].get_int();
        if (count < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    for (auto& p : Params().GetConsensus().llmqs) {
        UniValue v(UniValue::VARR);
        std::vector<llmq::CQuorumCPtr> quorums;
        llmq::quorumManager->ScanQuorums(p.first, ::ChainActive().Tip(), count > -1 ? count : p.second.signingActiveQuorumCount, quorums);
        for (auto& q : quorums) {
            v.push_back(q->qc->quorumHash.ToString());
        }

        ret.pushKV(p.second.name, v);
    }


    return ret;
},
    };
} 

UniValue BuildQuorumInfo(const llmq::CQuorumCPtr& quorum, bool includeMembers, bool includeSkShare)
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("height", quorum->pindexQuorum->nHeight);
    ret.pushKV("type", quorum->params.name);
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());
    ret.pushKV("minedBlock", quorum->minedBlockHash.ToString());

    if (includeMembers) {
        UniValue membersArr(UniValue::VARR);
        for (size_t i = 0; i < quorum->members.size(); i++) {
            auto& dmn = quorum->members[i];
            UniValue mo(UniValue::VOBJ);
            mo.pushKV("proTxHash", dmn->proTxHash.ToString());
            mo.pushKV("pubKeyOperator", dmn->pdmnState->pubKeyOperator.Get().ToString());
            mo.pushKV("valid", quorum->qc->validMembers[i]);
            if (quorum->qc->validMembers[i]) {
                CBLSPublicKey pubKey = quorum->GetPubKeyShare(i);
                if (pubKey.IsValid()) {
                    mo.pushKV("pubKeyShare", pubKey.ToString());
                }
            }
            membersArr.push_back(mo);
        }

        ret.pushKV("members", membersArr);
    }
    ret.pushKV("quorumPublicKey", quorum->qc->quorumPublicKey.ToString());
    const CBLSSecretKey& skShare = quorum->GetSkShare();
    if (includeSkShare && skShare.IsValid()) {
        ret.pushKV("secretKeyShare", skShare.ToString());
    }
    return ret;
}

static RPCHelpMan quorum_info()
{
    return RPCHelpMan{"quorum_info",
        "\nReturn information about a quorum\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},      
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash of quorum.\n"},      
            {"includeSkShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include secret key share in output.\n"},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_info", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_info", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 quorumHash = ParseHashV(request.params[1], "quorumHash");
    bool includeSkShare = false;
    if (!request.params[2].isNull()) {
        includeSkShare = request.params[2].get_bool();
    }

    auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    return BuildQuorumInfo(quorum, true, includeSkShare);
},
    };
} 

static RPCHelpMan quorum_dkgstatus()
{
    return RPCHelpMan{"quorum_dkgstatus",
        "\nReturn the status of the current DKG process.\n",
        {
            {"detail_level", RPCArg::Type::NUM, RPCArg::Default{0}, "Detail level of output.\n"
                            "0=Only show counts. 1=Show member indexes. 2=Show member's ProTxHashes.\n"},                     
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_dkgstatus", "0")
            + HelpExampleRpc("quorum_dkgstatus", "0")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    int detailLevel = 0;
    if (!request.params[0].isNull()) {
        detailLevel = request.params[0].get_int();
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);

    auto ret = status.ToJson(detailLevel);

    LOCK(cs_main);
    int tipHeight = ::ChainActive().Height();

    UniValue minableCommitments(UniValue::VOBJ);
    UniValue quorumConnections(UniValue::VOBJ);
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& params = p.second;

        if (fMasternodeMode) {
            const CBlockIndex* pindexQuorum = ::ChainActive()[tipHeight - (tipHeight % params.dkgInterval)];
            if(!pindexQuorum)
                continue;
            auto allConnections = llmq::CLLMQUtils::GetQuorumConnections(params.type, pindexQuorum, activeMasternodeInfo.proTxHash, false);
            auto outboundConnections = llmq::CLLMQUtils::GetQuorumConnections(params.type, pindexQuorum, activeMasternodeInfo.proTxHash, true);
            std::map<uint256, CAddress> foundConnections;
            node.connman->ForEachNode([&](CNode* pnode) {
                LOCK(pnode->cs_mnauth);
                if (!pnode->verifiedProRegTxHash.IsNull() && allConnections.count(pnode->verifiedProRegTxHash)) {
                    foundConnections.try_emplace(pnode->verifiedProRegTxHash, pnode->addr);
                }
            });
            UniValue arr(UniValue::VARR);
            for (auto& ec : allConnections) {
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("proTxHash", ec.ToString());
                if (foundConnections.count(ec)) {
                    obj.pushKV("connected", true);
                    obj.pushKV("address", foundConnections[ec].ToString());
                } else {
                    obj.pushKV("connected", false);
                }
                obj.pushKV("outbound", outboundConnections.count(ec) != 0);
                arr.push_back(obj);
            }
            quorumConnections.pushKV(params.name, arr);
        }

        llmq::CFinalCommitment fqc;
        if (llmq::quorumBlockProcessor->GetMinableCommitment(params.type, tipHeight, fqc)) {
            if(!fqc.IsNull()) {
                UniValue obj(UniValue::VOBJ);
                fqc.ToJson(obj);
                minableCommitments.pushKV(params.name, obj);
            }
        }
    }

    ret.pushKV("minableCommitments", minableCommitments);
    ret.pushKV("quorumConnections", quorumConnections);

    return ret;
},
    };
} 

static RPCHelpMan quorum_memberof()
{
    return RPCHelpMan{"quorum_memberof",
        "\nChecks which quorums the given masternode is a member of.\n",
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "ProTxHash of the masternode.\n"},  
            {"scanQuorumsCount", RPCArg::Type::NUM, RPCArg::Default{-1}, "Number of quorums to scan for. If not specified,\n"
                                 "the active quorum count for each specific quorum type is used."},                     
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_memberof", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_memberof", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    uint256 protxHash = ParseHashV(request.params[0], "proTxHash");
    int scanQuorumsCount = -1;
    if (!request.params[1].isNull()) {
        scanQuorumsCount = request.params[1].get_int();
        if (scanQuorumsCount <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid scanQuorumsCount parameter");
        }
    }

    CDeterministicMNCPtr dmn;
    {
        LOCK(cs_main);
        const CBlockIndex* pindexTip = ::ChainActive().Tip();
        CDeterministicMNList mnList;
        if(deterministicMNManager)
            deterministicMNManager->GetListForBlock(pindexTip, mnList);
        dmn = mnList.GetMN(protxHash);
        if (!dmn) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "masternode not found");
        }
    }

    UniValue result(UniValue::VARR);

    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& params = p.second;
        size_t count = params.signingActiveQuorumCount;
        if (scanQuorumsCount != -1) {
            count = (size_t)scanQuorumsCount;
        }
        std::vector<llmq::CQuorumCPtr> quorums;
        llmq::quorumManager->ScanQuorums(params.type, count, quorums);
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
},
    };
} 


static RPCHelpMan quorum_sign()
{
    return RPCHelpMan{"quorum_sign",
        "\nThreshold-sign a message.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"}, 
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The quorum identifier.\n"},
            {"submit", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Submits the signature share to the network if this is true.\n"},               
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_sign", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_sign", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");
    uint256 msgHash = ParseHashV(request.params[2], "msgHash");
    uint256 quorumHash;
    if (!request.params[3].isNull() && !request.params[3].get_str().empty()) {
        quorumHash = ParseHashV(request.params[3], "quorumHash");
    }
    bool fSubmit{true};
    if (!request.params[4].isNull()) {
        fSubmit = request.params[4].get_bool();
    }
    if (fSubmit) {
        return llmq::quorumSigningManager->AsyncSignIfMember(llmqType, id, msgHash, quorumHash);
    } else {
        llmq::CQuorumCPtr pQuorum;

        if (quorumHash.IsNull()) {
            pQuorum = llmq::quorumSigningManager->SelectQuorumForSigning(llmqType, id);
        } else {
            pQuorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
        }

        if (pQuorum == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }

        llmq::CSigShare sigShare = llmq::quorumSigSharesManager->CreateSigShare(pQuorum, id, msgHash);

        if (!sigShare.sigShare.Get().IsValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "failed to create sigShare");
        }

        UniValue obj(UniValue::VOBJ);
        obj.pushKV("llmqType", llmqType);
        obj.pushKV("quorumHash", sigShare.quorumHash.ToString());
        obj.pushKV("quorumMember", sigShare.quorumMember);
        obj.pushKV("id", id.ToString());
        obj.pushKV("msgHash", msgHash.ToString());
        obj.pushKV("signHash", sigShare.GetSignHash().ToString());
        obj.pushKV("signature", sigShare.sigShare.Get().ToString());

        return obj;
    }
},
    };
} 

static RPCHelpMan quorum_hasrecsig()
{
    return RPCHelpMan{"quorum_hasrecsig",
        "\nTest if a valid recovered signature is present.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_hasrecsig", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_hasrecsig", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");
    uint256 msgHash = ParseHashV(request.params[2], "msgHash");
    return llmq::quorumSigningManager->HasRecoveredSig(llmqType, id, msgHash);
},
    };
} 

static RPCHelpMan quorum_verify()
{
    return RPCHelpMan{"quorum_verify",
        "\nTest if a quorum signature is valid for a request id and a message hash.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},   
            {"signature", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Quorum signature to verify.\n"},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The quorum identifier. Set to \"\" if you want to specify signHeight instead.\n"},   
            {"signHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The height at which the message was signed. Only works when quorumHash is \"\".\n"},                
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_verify", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_verify", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");
    uint256 msgHash = ParseHashV(request.params[2], "msgHash");
    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[3].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }
    if (request.params[4].isNull() || (request.params[4].get_str().empty() && !request.params[5].isNull())) {
        int signHeight{-1};
        if (!request.params[5].isNull()) {
            signHeight = request.params[5].get_int();
        }
        // First check against the current active set, if it fails check against the last active set
        int signOffset{Params().GetConsensus().llmqs.at(llmqType).dkgInterval};
        return llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, msgHash, sig, 0) ||
                llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, msgHash, sig, signOffset);
    } else {
        uint256 quorumHash = ParseHashV(request.params[4], "quorumHash");
        llmq::CQuorumCPtr quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
        if (!quorum) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }
        uint256 signHash = llmq::CLLMQUtils::BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
        return sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
    }
},
    };
} 

static RPCHelpMan quorum_getrecsig()
{
    return RPCHelpMan{"quorum_getrecsig",
        "\nGet a recovered signature.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_getrecsig", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_getrecsig", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");
    uint256 msgHash = ParseHashV(request.params[2], "msgHash");

    llmq::CRecoveredSig recSig;
    if (!llmq::quorumSigningManager->GetRecoveredSigForId(llmqType, id, recSig)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "recovered signature not found");
    }
    if (recSig.msgHash != msgHash) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "recovered signature not found");
    }
    return recSig.ToJson();
},
    };
} 

static RPCHelpMan quorum_isconflicting()
{
    return RPCHelpMan{"quorum_isconflicting",
        "\nTest if a conflict exists.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_isconflicting", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_isconflicting", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");
    uint256 msgHash = ParseHashV(request.params[2], "msgHash");


    return llmq::quorumSigningManager->IsConflicting(llmqType, id, msgHash);
},
    };
} 

static RPCHelpMan quorum_selectquorum()
{
    return RPCHelpMan{"quorum_selectquorum",
        "\nReturns the quorum that would/should sign a request.\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type.\n"},  
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},                    
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_selectquorum", "0 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_selectquorum", "0, \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint8_t llmqType = (uint8_t)request.params[0].get_int();
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[1], "id");

    UniValue ret(UniValue::VOBJ);

    auto quorum = llmq::quorumSigningManager->SelectQuorumForSigning(llmqType, id);
    if (!quorum) {
        throw JSONRPCError(RPC_MISC_ERROR, "no quorums active");
    }
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());

    UniValue recoveryMembers(UniValue::VARR);
    for (int i = 0; i < quorum->params.recoveryMembers; i++) {
        auto dmn = llmq::quorumSigSharesManager->SelectMemberForRecovery(quorum, id, i);
        recoveryMembers.push_back(dmn->proTxHash.ToString());
    }
    ret.pushKV("recoveryMembers", recoveryMembers);

    return ret;
},
    };
} 

static RPCHelpMan quorum_dkgsimerror()
{
    return RPCHelpMan{"quorum_dkgsimerror",
        "\nThis enables simulation of errors and malicious behaviour in the DKG. Do NOT use this on mainnet\n"
            "as you will get yourself very likely PoSe banned for this.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Error type.\n"
                        "Supported error types:\n"
                        " - contribution-omit\n"
                        " - contribution-lie\n"
                        " - complain-lie\n"
                        " - justify-lie\n"
                        " - justify-omit\n"
                        " - commit-omit\n"
                        " - commit-lie\n"},
            {"rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Rate at which to simulate this error type.\n"},                    
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_dkgsimerror", "contribution-omit 1")
            + HelpExampleRpc("quorum_dkgsimerror", "\"contribution-omit\", 1")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string type = request.params[0].get_str();
    double rate = request.params[1].get_real();

    if (rate < 0 || rate > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 1");
    }

    llmq::SetSimulatedDKGErrorRate(type, rate);

    return UniValue();
},
    };
} 

void RegisterQuorumsRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                &quorum_list,                        },
    { "evo",                &quorum_info,                        },
    { "evo",                &quorum_dkgstatus,                   },
    { "evo",                &quorum_memberof,                    },
    { "evo",                &quorum_selectquorum,                },
    { "evo",                &quorum_dkgsimerror,                 },
    { "evo",                &quorum_hasrecsig,                   },
    { "evo",                &quorum_verify,                      },
    { "evo",                &quorum_getrecsig,                   },
    { "evo",                &quorum_isconflicting,               },
    { "evo",                &quorum_sign,                        },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
