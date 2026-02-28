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
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_btccheckpoints.h>
#include <llmq/quorums_utils.h>
#include <rpc/util.h>
#include <net.h>
#include <rpc/blockchain.h>
#include <node/context.h>
#include <rpc/server_util.h>
#include <index/txindex.h>
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
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int count = -1;
    if (!request.params[0].isNull()) {
        count = request.params[0].getInt<int>();
        if (count < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }
    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return node.chainman->ActiveTip());
    UniValue ret(UniValue::VOBJ);

    UniValue v(UniValue::VARR);
    auto quorums = llmq::quorumManager->ScanQuorums(pindexTip, count > -1 ? count : Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount);
    for (auto& q : quorums) {
        v.push_back(q->qc->quorumHash.ToString());
    }

    ret.pushKV("quorums", v);

    return ret;
},
    };
} 

UniValue BuildQuorumInfo(const llmq::CQuorumCPtr& quorum, bool includeMembers, bool includeSkShare)
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("height", quorum->m_quorum_base_block_index->nHeight);
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());
    ret.pushKV("minedBlock", quorum->minedBlockHash.ToString());

    if (includeMembers) {
        UniValue membersArr(UniValue::VARR);
        for (size_t i = 0; i < quorum->members.size(); i++) {
            auto& dmn = quorum->members[i];
            UniValue mo(UniValue::VOBJ);
            mo.pushKV("proTxHash", dmn->proTxHash.ToString());
            mo.pushKV("pubKeyOperator", dmn->pdmnState->pubKeyOperator.ToString());
            bool member = quorum->qc->validMembers[i];
            mo.pushKV("valid", member);
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
    const CBLSSecretKey skShare = quorum->GetSkShare();
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
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash of quorum.\n"},      
            {"includeSkShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include secret key share in output.\n"},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_info", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_info", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    uint256 quorumHash = ParseHashV(request.params[0], "quorumHash");
    bool includeSkShare = false;
    if (!request.params[1].isNull()) {
        includeSkShare = request.params[1].get_bool();
    }

    auto quorum = llmq::quorumManager->GetQuorum(quorumHash);
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
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int detailLevel = 0;
    if (!request.params[0].isNull()) {
        detailLevel = request.params[0].getInt<int>();
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);

    auto ret = status.ToJson(*node.chainman, detailLevel);
    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return node.chainman->ActiveTip());
    int tipHeight = pindexTip->nHeight;

    auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    UniValue minableCommitments(UniValue::VARR);
    UniValue quorumArrConnections(UniValue::VARR);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    const auto& params = Params().GetConsensus().llmqTypeChainLocks;
    UniValue obj(UniValue::VOBJ);
    if (fMasternodeMode) {
        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return node.chainman->ActiveChain()[tipHeight - (tipHeight % params.dkgInterval)]);
        if(!pQuorumBaseBlockIndex)
            return ret;
        obj.pushKV("pQuorumBaseBlockIndex", pQuorumBaseBlockIndex->nHeight);
        obj.pushKV("quorumHash", pQuorumBaseBlockIndex->GetBlockHash().ToString());
        obj.pushKV("pindexTip", pindexTip->nHeight);

        auto allConnections = llmq::CLLMQUtils::GetQuorumConnections(pQuorumBaseBlockIndex, proTxHash, false);
        auto outboundConnections = llmq::CLLMQUtils::GetQuorumConnections(pQuorumBaseBlockIndex, proTxHash, true);
        std::map<uint256, CAddress> foundConnections;
        node.connman->ForEachNode([&](CNode* pnode) {
            auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
            if (!verifiedProRegTxHash.IsNull() && allConnections.count(verifiedProRegTxHash)) {
                foundConnections.emplace(verifiedProRegTxHash, pnode->addr);
            }
        });
        UniValue arr(UniValue::VARR);
        for (auto& ec : allConnections) {
            obj.pushKV("proTxHash", ec.ToString());
            if (foundConnections.count(ec)) {
                obj.pushKV("connected", true);
                obj.pushKV("address", foundConnections[ec].ToStringAddr());
            } else {
                obj.pushKV("connected", false);
            }
            obj.pushKV("outbound", outboundConnections.count(ec) != 0);
            arr.push_back(obj);
        }
        obj.pushKV("quorumConnections", arr);
    }
    quorumArrConnections.push_back(obj);
    LOCK(cs_main);
    llmq::CFinalCommitment fqc;
    if (llmq::quorumBlockProcessor->GetMinableCommitment(tipHeight, fqc)) {
        if(!fqc.IsNull()) {
            UniValue obj(UniValue::VOBJ);
            fqc.ToJson(obj);
            minableCommitments.push_back(obj);
        }
    }
    

    ret.pushKV("minableCommitments", minableCommitments);
    ret.pushKV("quorumConnections", quorumArrConnections);

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
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);
    uint256 protxHash = ParseHashV(request.params[0], "proTxHash");
    int scanQuorumsCount = -1;
    if (!request.params[1].isNull()) {
        scanQuorumsCount = request.params[1].getInt<int>();
        if (scanQuorumsCount <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid scanQuorumsCount parameter");
        }
    }

    CDeterministicMNCPtr dmn;
    {
        LOCK(cs_main);
        const CBlockIndex* pindexTip = node.chainman->ActiveTip();
        auto mnList = deterministicMNManager->GetListForBlock(pindexTip);
        dmn = mnList.GetMN(protxHash);
        if (!dmn) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "masternode not found");
        }
    }

    UniValue result(UniValue::VARR);

    auto& params = Params().GetConsensus().llmqTypeChainLocks;
    size_t count = params.signingActiveQuorumCount;
    if (scanQuorumsCount != -1) {
        count = (size_t)scanQuorumsCount;
    }
    auto quorums = llmq::quorumManager->ScanQuorums(count);
    for (auto& quorum : quorums) {
        if (quorum->IsMember(dmn->proTxHash)) {
            auto json = BuildQuorumInfo(quorum, false, false);
            json.pushKV("isValidMember", quorum->IsValidMember(dmn->proTxHash));
            json.pushKV("memberIndex", quorum->GetMemberIndex(dmn->proTxHash));
            result.push_back(json);
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
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"}, 
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The quorum identifier.\n"},
            {"submit", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Submits the signature share to the network if this is true.\n"},               
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_sign", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_sign", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 msgHash = ParseHashV(request.params[1], "msgHash");
    uint256 quorumHash;
    if (!request.params[2].isNull() && !request.params[2].get_str().empty()) {
        quorumHash = ParseHashV(request.params[2], "quorumHash");
    }
    bool fSubmit{true};
    if (!request.params[3].isNull()) {
        fSubmit = request.params[3].get_bool();
    }
    if (fSubmit) {
        return llmq::quorumSigningManager->AsyncSignIfMember( id, msgHash, quorumHash);
    } else {
        const auto pQuorum = [&]() {
            if (quorumHash.IsNull()) {
                return llmq::SelectQuorumForSigning(*node.chainman, id);
            } else {
                return llmq::quorumManager->GetQuorum(quorumHash);
            }
        }();

        if (pQuorum == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }

        auto sigShare = llmq::quorumSigSharesManager->CreateSigShare(pQuorum, id, msgHash);

        if (!sigShare.has_value() || !sigShare->sigShare.Get().IsValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "failed to create sigShare");
        }


        UniValue obj(UniValue::VOBJ);
        obj.pushKV("quorumHash", sigShare->getQuorumHash().ToString());
        obj.pushKV("quorumMember", sigShare->getQuorumMember());
        obj.pushKV("id", id.ToString());
        obj.pushKV("msgHash", msgHash.ToString());
        obj.pushKV("signHash", sigShare->GetSignHash().ToString());
        obj.pushKV("signature", sigShare->sigShare.Get().ToString());

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
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_hasrecsig", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_hasrecsig", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 msgHash = ParseHashV(request.params[1], "msgHash");
    return llmq::quorumSigningManager->HasRecoveredSig( id, msgHash);
},
    };
} 
static bool VerifyRecoveredSigLatestQuorums(const Consensus::LLMQParams& llmq_params, ChainstateManager& chainman,
    int signHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig)
{
    // First check against the current active set, if it fails check against the last active set
    for (int signOffset : {0, llmq_params.dkgInterval}) {
        if (llmq::VerifyRecoveredSig(chainman, signHeight, id, msgHash, sig, signOffset) == llmq::VerifyRecSigStatus::Valid) {
            return true;
        }
    }
    return false;
}

static RPCHelpMan quorum_verify()
{
    return RPCHelpMan{"quorum_verify",
        "\nTest if a quorum signature is valid for a request id and a message hash.\n",
        { 
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},   
            {"signature", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Quorum signature to verify.\n"},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The quorum identifier. Set to \"\" if you want to specify signHeight instead.\n"},   
            {"signHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The height at which the message was signed. Only works when quorumHash is \"\".\n"},                
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_verify", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_verify", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 msgHash = ParseHashV(request.params[1], "msgHash");

    const bool use_bls_legacy = bls::bls_legacy_scheme.load();
    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[2].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }
    if (request.params[3].isNull() || (request.params[3].get_str().empty() && !request.params[4].isNull())) {
        int signHeight{-1};
        if (!request.params[4].isNull()) {
            signHeight = request.params[4].getInt<int>();
        }
        return VerifyRecoveredSigLatestQuorums(Params().GetConsensus().llmqTypeChainLocks, *node.chainman, signHeight, id, msgHash, sig);
    } else {
        uint256 quorumHash = ParseHashV(request.params[3], "quorumHash");
        llmq::CQuorumCPtr quorum = llmq::quorumManager->GetQuorum( quorumHash);
        if (!quorum) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }
        uint256 signHash = llmq::BuildSignHash( quorum->qc->quorumHash, id, msgHash);
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
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_getrecsig", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_getrecsig", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 msgHash = ParseHashV(request.params[1], "msgHash");

    llmq::CRecoveredSig recSig;
    if (!llmq::quorumSigningManager->GetRecoveredSigForId( id, recSig)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "recovered signature not found");
    }
    if (recSig.getMsgHash() != msgHash) {
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
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},   
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash.\n"},                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_isconflicting", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_isconflicting", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 msgHash = ParseHashV(request.params[1], "msgHash");


    return llmq::quorumSigningManager->IsConflicting(id, msgHash);
},
    };
} 

static RPCHelpMan quorum_selectquorum()
{
    return RPCHelpMan{"quorum_selectquorum",
        "\nReturns the quorum that would/should sign a request.\n",
        { 
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id.\n"},                    
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("quorum_selectquorum", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("quorum_selectquorum", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);

    uint256 id = ParseHashV(request.params[0], "id");

    UniValue ret(UniValue::VOBJ);

    const auto quorum = llmq::SelectQuorumForSigning(*node.chainman, id);
    if (!quorum) {
        throw JSONRPCError(RPC_MISC_ERROR, "no quorums active");
    }
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());

    UniValue recoveryMembers(UniValue::VARR);
    const auto& params = Params().GetConsensus().llmqTypeChainLocks;
    for (int i = 0; i < params.recoveryMembers; i++) {
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
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    std::string type_str = request.params[0].get_str();
    double rate = request.params[1].get_real();

    if (rate < 0 || rate > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 1");
    }
    
    if (const llmq::DKGError::type type = llmq::DKGError::from_string(type_str);
            type == llmq::DKGError::type::_COUNT) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type. See DKGError class implementation");
    } else {
        llmq::SetSimulatedDKGErrorRate(type, rate);
        return UniValue();
    }
},
    };
} 


static RPCHelpMan verifychainlock()
{
    return RPCHelpMan{"verifychainlock",
        "\nTest if a quorum signature is valid for a ChainLock.\n",
        {
            {"blockHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash of the ChainLock."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature of the ChainLock."},
            {"signers", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The quorum signers of the ChainLock."}
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("verifychainlock", "0x0 0x0 0x0")
            + HelpExampleRpc("verifychainlock", "\"0x0\", \"0x0\", \"0x0\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const uint256 nBlockHash(ParseHashV(request.params[0], "blockHash"));

    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    int nBlockHeight;
    const CBlockIndex* pIndex = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(nBlockHash));
    if (pIndex == nullptr) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "blockHash not found");
    }
    nBlockHeight = pIndex->nHeight;

    const bool use_bls_legacy = bls::bls_legacy_scheme.load();
    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[1].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }
    
    const auto signers = llmq::CLLMQUtils::HexStrToBits(request.params[2].get_str(), Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount);
    if (!signers) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signers");
    }
    const auto clSig = llmq::CChainLockSig(nBlockHeight, nBlockHash, sig, *signers);
    return llmq::chainLocksHandler->VerifyAggregatedChainLock(clSig, pIndex, SerializeHash(clSig));
},
    };
} 


static RPCHelpMan gettxchainlocks()
{
    return RPCHelpMan{
        "gettxchainlocks",
        "\nReturns the block height at which each transaction was mined, and indicates whether it is in the mempool, ChainLocked, or neither.\n",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "The transaction ids (no more than 100)",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction hash"},
                },
            },
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Response is an array with the same size as the input txids",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "height", "The block height"},
                    {RPCResult::Type::BOOL, "chainlock", "The state of the corresponding block ChainLock"},
                    {RPCResult::Type::BOOL, "mempool", "Mempool status for the transaction"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("gettxchainlocks", "'[\"mytxid\",...]'")
        + HelpExampleRpc("gettxchainlocks", "[\"mytxid\",...]")
        },
        [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    UniValue result_arr(UniValue::VARR);
    UniValue txids = request.params[0].get_array();
    if (txids.size() > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 txids only");
    }

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }
    LOCK(cs_main);
    for (size_t idx = 0;idx < txids.size();idx++) {
        UniValue result(UniValue::VOBJ);
        const uint256 txid(ParseHashV(txids[idx], "txid"));
        if (txid == Params().GenesisBlock().hashMerkleRoot) {
            // Special exception for the genesis block coinbase transaction
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved");
        }

        uint256 hash_block;
        const CBlockIndex* pindex{nullptr};
        uint32_t nBlockHeight{0};
        if(pblockindexdb->ReadBlockHeight(txid, nBlockHeight)) {	
            pindex = chainman.ActiveChain()[nBlockHeight];	
        }
        const auto tx_ref = GetTransaction(pindex, node.mempool.get(), txid, hash_block, chainman.m_blockman);
        if (tx_ref == nullptr) {
            result.pushKV("height", 0);
            result.pushKV("chainlock", false);
            result.pushKV("mempool", false);
            result_arr.push_back(result);
            continue;
        }
        result.pushKV("height", nBlockHeight);
        result.pushKV("chainlock", pindex? llmq::chainLocksHandler->HasChainLock(nBlockHeight, hash_block): false);
        result.pushKV("mempool", pindex == nullptr);
        result_arr.push_back(result);
    }
    return result_arr;
},
    };
}

static RPCHelpMan getbestchainlock()
{
    return RPCHelpMan{"getbestchainlock",
        "\nReturns information about the best ChainLock. Throws an error if there is no known ChainLock yet.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "blockhash", "The block hash hex-encoded"},
                {RPCResult::Type::NUM, "height", "The block height or index"},
                {RPCResult::Type::STR_HEX, "signature", "The ChainLock's BLS signature"},
                {RPCResult::Type::STR_HEX, "signers", "The ChainLock's quorum signers"},
                {RPCResult::Type::BOOL, "known_block", "True if the block is known by our node"},
            }},
        RPCExamples{
            HelpExampleCli("getbestchainlock", "")
            + HelpExampleRpc("getbestchainlock", "")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);

    const node::NodeContext& node = EnsureAnyNodeContext(request.context);

    llmq::CChainLockSig clsig = llmq::chainLocksHandler->GetBestChainLock();
    if (clsig.IsNull()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to find any ChainLock");
    }
    result.pushKV("blockhash", clsig.blockHash.GetHex());
    result.pushKV("height", clsig.nHeight);
    result.pushKV("signature", clsig.sig.ToString());
    result.pushKV("signers", llmq::CLLMQUtils::ToHexStr(clsig.signers));
    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);
    result.pushKV("known_block", chainman.m_blockman.LookupBlockIndex(clsig.blockHash) != nullptr);
    return result;
},
    };
}

static RPCHelpMan getbestbtccheckpoint()
{
    return RPCHelpMan{"getbestbtccheckpoint",
        "\nReturns information about the best BTC checkpoint attestation (btcc). Throws an error if none is known yet.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "syshash", "The Syscoin block hash (attested)"},
                {RPCResult::Type::NUM, "height", "The Syscoin block height (attested)"},
                {RPCResult::Type::STR_HEX, "signature", "The attestation's BLS signature"},
                {RPCResult::Type::STR_HEX, "signers", "The attestation's quorum signers"},
                {RPCResult::Type::BOOL, "known_block", "True if the block is known by our node"},
            }},
        RPCExamples{
            HelpExampleCli("getbestbtccheckpoint", "")
            + HelpExampleRpc("getbestbtccheckpoint", "")
        },
        [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);

    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    if (!llmq::btcCheckpointsHandler) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "BTC checkpoint handler not available");
    }

    llmq::CBTCCheckpointSig btcc = llmq::btcCheckpointsHandler->GetBestBTCCheckpoint();
    if (btcc.IsNull()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to find any BTC checkpoint attestation");
    }
    result.pushKV("syshash", btcc.sysHash.GetHex());
    result.pushKV("height", btcc.nHeight);
    result.pushKV("signature", btcc.sig.ToString());
    result.pushKV("signers", llmq::CLLMQUtils::ToHexStr(btcc.signers));
    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);
    result.pushKV("known_block", chainman.m_blockman.LookupBlockIndex(btcc.sysHash) != nullptr);
    return result;
},
    };
}

static RPCHelpMan submitchainlock()
{
    
    return RPCHelpMan{"submitchainlock",
               "Submit a ChainLock signature if needed\n",
               {
                       {"blockHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash of the ChainLock."},
                       {"signature", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The signature of the ChainLock."},
                       {"signers", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The quorum signers of the ChainLock."},
               },
               RPCResult{
                    RPCResult::Type::NUM, "", "The height of the current best ChainLock"},
               RPCExamples{""},
        [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const uint256 nBlockHash(ParseHashV(request.params[0], "blockHash"));
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    int nBlockHeight;
    const CBlockIndex* pIndex = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(nBlockHash));
    if (pIndex == nullptr) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "blockHash not found");
    }
    nBlockHeight = pIndex->nHeight;
   
    const auto signers = llmq::CLLMQUtils::HexStrToBits(request.params[2].get_str(), Params().GetConsensus().llmqTypeChainLocks.signingActiveQuorumCount);
    if (!signers) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signers");
    }
    const int32_t bestCLHeight = llmq::chainLocksHandler->GetBestChainLock().nHeight;
    if (nBlockHeight <= bestCLHeight) return bestCLHeight;

    const bool use_bls_legacy = bls::bls_legacy_scheme.load();
    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[1].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }
    auto clsig = llmq::CChainLockSig(nBlockHeight, nBlockHash, sig, *signers);
    
    BlockValidationState state;
    if(!llmq::chainLocksHandler->ProcessNewChainLock(0, clsig, state, ::SerializeHash(clsig))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, state.GetRejectReason());
    }
    return llmq::chainLocksHandler->GetBestChainLock().nHeight;
},
    };
}

void RegisterQuorumsRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[] =
    {
        {"evo", &quorum_list},
        {"evo", &quorum_info},
        {"evo", &quorum_dkgstatus},
        {"evo", &quorum_memberof},
        {"evo", &quorum_selectquorum},
        {"evo", &quorum_dkgsimerror},
        {"evo", &quorum_hasrecsig},
        {"evo", &quorum_verify},
        {"evo", &quorum_getrecsig},
        {"evo", &quorum_isconflicting},
        {"evo", &quorum_sign},
        {"evo", &submitchainlock},
        {"evo", &verifychainlock},
        {"evo", &getbestchainlock},
        {"evo", &getbestbtccheckpoint},
        {"evo", &gettxchainlocks},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
