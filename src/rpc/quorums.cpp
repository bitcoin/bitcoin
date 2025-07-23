// Copyright (c) 2017-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <deploymentstatus.h>
#include <index/txindex.h>
#include <net_processing.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <util/check.h>
#include <validation.h>

#include <masternode/node.h>
#include <evo/deterministicmns.h>

#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <llmq/debug.h>
#include <llmq/dkgsession.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/snapshot.h>
#include <llmq/utils.h>

#include <iomanip>
#include <optional>

using node::GetTransaction;
using node::NodeContext;

static RPCHelpMan quorum_list()
{
    return RPCHelpMan{"quorum list",
        "List of on-chain quorums\n",
        {
            {"count", RPCArg::Type::NUM, RPCArg::DefaultHint{"The active quorum count if not specified"},
                "Number of quorums to list.\n"
                "Can be CPU/disk heavy when the value is larger than the number of active quorums."
            },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "quorumName", "List of quorum hashes per some quorum type",
                {
                    {RPCResult::Type::STR_HEX, "quorumHash", "Quorum hash. Note: most recent quorums come first."},
                }},
            }},
        RPCExamples{
            HelpExampleCli("quorum", "list")
    + HelpExampleCli("quorum", "list 10")
    + HelpExampleRpc("quorum", "list, 10")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    int count = -1;
    if (!request.params[0].isNull()) {
        count = ParseInt32V(request.params[0], "count");
        if (count < -1) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());

    for (const auto& type : llmq::GetEnabledQuorumTypes(pindexTip)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        UniValue v(UniValue::VARR);

        auto quorums = llmq_ctx.qman->ScanQuorums(type, pindexTip, count > -1 ? count : llmq_params_opt->signingActiveQuorumCount);
        for (const auto& q : quorums) {
            v.push_back(q->qc->quorumHash.ToString());
        }

        ret.pushKV(std::string(llmq_params_opt->name), v);
    }

    return ret;
},
    };
}

static RPCHelpMan quorum_list_extended()
{
    return RPCHelpMan{"quorum listextended",
        "Extended list of on-chain quorums\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::DefaultHint{"Tip height if not specified"}, "Active quorums at the height."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "quorumName", "List of quorum details per quorum type",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ, "xxxx", "Quorum hash. Note: most recent quorums come first.",
                        {
                            {RPCResult::Type::NUM, "creationHeight", "Block height where the DKG started."},
                            {RPCResult::Type::NUM, "quorumIndex", "Quorum index (applicable only to rotated quorums)."},
                            {RPCResult::Type::STR_HEX, "minedBlockHash", "Blockhash where the commitment was mined."},
                            {RPCResult::Type::NUM, "numValidMembers", "The total of valid members."},
                            {RPCResult::Type::STR_AMOUNT, "healthRatio", "The ratio of healthy members to quorum size. Range [0.0 - 1.0]."}
                        }}
                    }}
                }}
            }},
            RPCExamples{
                HelpExampleCli("quorum", "listextended")
                + HelpExampleCli("quorum", "listextended 2500")
                + HelpExampleRpc("quorum", "listextended, 2500")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    int nHeight = -1;
    if (!request.params[0].isNull()) {
        nHeight = ParseInt32V(request.params[0], "height");
        if (nHeight < 0 || nHeight > WITH_LOCK(cs_main, return chainman.ActiveChain().Height())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
        }
    }

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pblockindex = nHeight != -1 ? WITH_LOCK(cs_main, return chainman.ActiveChain()[nHeight]) : WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());

    for (const auto& type : llmq::GetEnabledQuorumTypes(pblockindex)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        const auto& llmq_params = llmq_params_opt.value();
        UniValue v(UniValue::VARR);

        auto quorums = llmq_ctx.qman->ScanQuorums(type, pblockindex, llmq_params.signingActiveQuorumCount);
        for (const auto& q : quorums) {
            size_t num_members = q->members.size();
            size_t num_valid_members = std::count_if(q->qc->validMembers.begin(), q->qc->validMembers.begin() + num_members, [](auto val){return val;});
            double health_ratio = num_members > 0 ? double(num_valid_members) / double(num_members) : 0.0;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << health_ratio;
            UniValue obj(UniValue::VOBJ);
            {
                UniValue j(UniValue::VOBJ);
                if (llmq_params.useRotation) {
                    j.pushKV("quorumIndex", q->qc->quorumIndex);
                }
                j.pushKV("creationHeight", q->m_quorum_base_block_index->nHeight);
                j.pushKV("minedBlockHash", q->minedBlockHash.ToString());
                j.pushKV("numValidMembers", (int32_t)num_valid_members);
                j.pushKV("healthRatio", ss.str());
                obj.pushKV(q->qc->quorumHash.ToString(),j);
            }
            v.push_back(obj);
        }
        ret.pushKV(std::string(llmq_params.name), v);
    }

    return ret;
},
    };
}

static UniValue BuildQuorumInfo(const llmq::CQuorumBlockProcessor& quorum_block_processor,
                                const llmq::CQuorumCPtr& quorum, bool includeMembers, bool includeSkShare)
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("height", quorum->m_quorum_base_block_index->nHeight);
    ret.pushKV("type", std::string(quorum->params.name));
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());
    ret.pushKV("quorumIndex", quorum->qc->quorumIndex);
    ret.pushKV("minedBlock", quorum->minedBlockHash.ToString());

    if (quorum->params.useRotation) {
        auto previousActiveCommitment = quorum_block_processor.GetLastMinedCommitmentsByQuorumIndexUntilBlock(quorum->params.type, quorum->m_quorum_base_block_index, quorum->qc->quorumIndex, 0);
        if (previousActiveCommitment.has_value()) {
            int previousConsecutiveDKGFailures = (quorum->m_quorum_base_block_index->nHeight - previousActiveCommitment.value()->nHeight) /  quorum->params.dkgInterval - 1;
            ret.pushKV("previousConsecutiveDKGFailures", previousConsecutiveDKGFailures);
        }
        else {
            ret.pushKV("previousConsecutiveDKGFailures", 0);
        }
    }

    if (includeMembers) {
        UniValue membersArr(UniValue::VARR);
        for (size_t i = 0; i < quorum->members.size(); i++) {
            const auto& dmn = quorum->members[i];
            UniValue mo(UniValue::VOBJ);
            mo.pushKV("proTxHash", dmn->proTxHash.ToString());
            if (IsDeprecatedRPCEnabled("service")) {
                mo.pushKV("service", dmn->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
            }
            mo.pushKV("addresses", dmn->pdmnState->netInfo->ToJson());
            mo.pushKV("pubKeyOperator", dmn->pdmnState->pubKeyOperator.ToString());
            mo.pushKV("valid", quorum->qc->validMembers[i]);
            if (quorum->qc->validMembers[i]) {
                if (quorum->params.size == 1) {
                    mo.pushKV("pubKeyShare", dmn->pdmnState->pubKeyOperator.ToString());
                } else {
                    CBLSPublicKey pubKey = quorum->GetPubKeyShare(i);
                    if (pubKey.IsValid()) {
                        mo.pushKV("pubKeyShare", pubKey.ToString());
                    }
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
    return RPCHelpMan{"quorum info",
        "Return information about a quorum\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash of quorum."},
            {"includeSkShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include secret key share in output."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};
    if (!Params().GetLLMQ(llmqType).has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 quorumHash(ParseHashV(request.params[1], "quorumHash"));
    bool includeSkShare = false;
    if (!request.params[2].isNull()) {
        includeSkShare = ParseBoolV(request.params[2], "includeSkShare");
    }

    const auto quorum = llmq_ctx.qman->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    return BuildQuorumInfo(*llmq_ctx.quorum_block_processor, quorum, true, includeSkShare);
},
    };
}

static RPCHelpMan quorum_dkgstatus()
{
    return RPCHelpMan{"quorum dkgstatus",
        "Return the status of the current DKG process.\n"
        "Works only when SPORK_17_QUORUM_DKG_ENABLED spork is ON.\n",
        {
            {"detail_level", RPCArg::Type::NUM, RPCArg::Default{0},
                "Detail level of output.\n"
                "0=Only show counts. 1=Show member indexes. 2=Show member's ProTxHashes."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const CConnman& connman = EnsureConnman(node);
    CHECK_NONFATAL(node.sporkman);

    int detailLevel = 0;
    if (!request.params[0].isNull()) {
        detailLevel = ParseInt32V(request.params[0], "detail_level");
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq_ctx.dkg_debugman->GetLocalDebugStatus(status);

    auto ret = status.ToJson(*CHECK_NONFATAL(node.dmnman), *llmq_ctx.qsnapman, chainman, detailLevel);

    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    int tipHeight = pindexTip->nHeight;
    const uint256 proTxHash = node.mn_activeman ? node.mn_activeman->GetProTxHash() : uint256();

    UniValue minableCommitments(UniValue::VARR);
    UniValue quorumArrConnections(UniValue::VARR);
    for (const auto& type : llmq::GetEnabledQuorumTypes(pindexTip)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        const auto& llmq_params = llmq_params_opt.value();
        bool rotation_enabled = llmq::IsQuorumRotationEnabled(llmq_params, pindexTip);
        int quorums_num = rotation_enabled ? llmq_params.signingActiveQuorumCount : 1;

        for (const int quorumIndex : irange::range(quorums_num)) {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("llmqType", std::string(llmq_params.name));
            obj.pushKV("quorumIndex", quorumIndex);

            if (node.mn_activeman) {
                int quorumHeight = tipHeight - (tipHeight % llmq_params.dkgInterval) + quorumIndex;
                if (quorumHeight <= tipHeight) {
                    const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return chainman.ActiveChain()[quorumHeight]);
                    obj.pushKV("pQuorumBaseBlockIndex", pQuorumBaseBlockIndex->nHeight);
                    obj.pushKV("quorumHash", pQuorumBaseBlockIndex->GetBlockHash().ToString());
                    obj.pushKV("pindexTip", pindexTip->nHeight);

                    auto allConnections = llmq::utils::GetQuorumConnections(llmq_params, *node.dmnman,
                                                                            *llmq_ctx.qsnapman, *node.sporkman,
                                                                            pQuorumBaseBlockIndex, proTxHash, false);
                    auto outboundConnections = llmq::utils::GetQuorumConnections(llmq_params, *node.dmnman,
                                                                                 *llmq_ctx.qsnapman, *node.sporkman,
                                                                                 pQuorumBaseBlockIndex, proTxHash, true);
                    std::map<uint256, CAddress> foundConnections;
                    connman.ForEachNode([&](const CNode* pnode) {
                        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
                        if (!verifiedProRegTxHash.IsNull() && allConnections.count(verifiedProRegTxHash)) {
                            foundConnections.emplace(verifiedProRegTxHash, pnode->addr);
                        }
                    });
                    UniValue arr(UniValue::VARR);
                    for (const auto& ec : allConnections) {
                        UniValue ecj(UniValue::VOBJ);
                        ecj.pushKV("proTxHash", ec.ToString());
                        if (foundConnections.count(ec)) {
                            ecj.pushKV("connected", true);
                            ecj.pushKV("address", foundConnections[ec].ToStringAddrPort());
                        } else {
                            ecj.pushKV("connected", false);
                        }
                        ecj.pushKV("outbound", outboundConnections.count(ec) != 0);
                        arr.push_back(ecj);
                    }
                    obj.pushKV("quorumConnections", arr);
                }
            }
            quorumArrConnections.push_back(obj);
        }

        LOCK(cs_main);
        std::optional<std::vector<llmq::CFinalCommitment>> vfqc = llmq_ctx.quorum_block_processor->GetMineableCommitments(llmq_params, tipHeight);
        if (vfqc.has_value()) {
            for (const auto& fqc : vfqc.value()) {
                minableCommitments.push_back(fqc.ToJson());
            }
        }
    }
    ret.pushKV("quorumConnections", quorumArrConnections);
    ret.pushKV("minableCommitments", minableCommitments);
    return ret;
},
    };
}

static RPCHelpMan quorum_memberof()
{
    return RPCHelpMan{"quorum memberof",
        "Checks which quorums the given masternode is a member of.\n",
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "ProTxHash of the masternode."},
            {"scanQuorumsCount", RPCArg::Type::NUM, RPCArg::DefaultHint{"The active quorum count for each specific quorum type is used"},
                "Number of quorums to scan for.\n"
                "Can be CPU/disk heavy when the value is larger than the number of active quorums."
            },
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    uint256 protxHash(ParseHashV(request.params[0], "proTxHash"));
    int scanQuorumsCount = -1;
    if (!request.params[1].isNull()) {
        scanQuorumsCount = ParseInt32V(request.params[1], "scanQuorumsCount");
        if (scanQuorumsCount <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid scanQuorumsCount parameter");
        }
    }

    const CBlockIndex* pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    auto mnList = CHECK_NONFATAL(node.dmnman)->GetListForBlock(pindexTip);
    auto dmn = mnList.GetMN(protxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "masternode not found");
    }

    UniValue result(UniValue::VARR);
    for (const auto& type : llmq::GetEnabledQuorumTypes(pindexTip)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        size_t count = llmq_params_opt->signingActiveQuorumCount;
        if (scanQuorumsCount != -1) {
            count = (size_t)scanQuorumsCount;
        }
        auto quorums = llmq_ctx.qman->ScanQuorums(llmq_params_opt->type, count);
        for (auto& quorum : quorums) {
            if (quorum->IsMember(dmn->proTxHash)) {
                auto json = BuildQuorumInfo(*llmq_ctx.quorum_block_processor, quorum, false, false);
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

static UniValue quorum_sign_helper(const JSONRPCRequest& request, Consensus::LLMQType llmqType)
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[0], "id"));
    const uint256 msgHash(ParseHashV(request.params[1], "msgHash"));

    uint256 quorumHash;
    if (!request.params[2].isNull() && !request.params[2].get_str().empty()) {
        quorumHash = ParseHashV(request.params[2], "quorumHash");
    }
    bool fSubmit{true};
    if (!request.params[3].isNull()) {
        fSubmit = ParseBoolV(request.params[3], "submit");
    }
    if (fSubmit) {
        return llmq_ctx.sigman->AsyncSignIfMember(llmqType, *llmq_ctx.shareman, id, msgHash, quorumHash);
    } else {
        const auto pQuorum = [&]() {
            if (quorumHash.IsNull()) {
                return llmq::SelectQuorumForSigning(llmq_params_opt.value(), chainman.ActiveChain(), *llmq_ctx.qman, id);
            } else {
                return llmq_ctx.qman->GetQuorum(llmqType, quorumHash);
            }
        }();

        if (pQuorum == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }

        auto sigShare = llmq_ctx.shareman->CreateSigShare(pQuorum, id, msgHash);

        if (!sigShare.has_value() || !sigShare->sigShare.Get().IsValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "failed to create sigShare");
        }

        UniValue obj(UniValue::VOBJ);
        obj.pushKV("llmqType", static_cast<uint8_t>(llmqType));
        obj.pushKV("quorumHash", sigShare->getQuorumHash().ToString());
        obj.pushKV("quorumMember", sigShare->getQuorumMember());
        obj.pushKV("id", id.ToString());
        obj.pushKV("msgHash", msgHash.ToString());
        obj.pushKV("signHash", sigShare->GetSignHash().ToString());
        obj.pushKV("signature", sigShare->sigShare.Get().ToString());

        return obj;
    }
}

static RPCHelpMan quorum_sign()
{
    return RPCHelpMan{"quorum sign",
        "Threshold-sign a message\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "The quorum identifier."},
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true}, "Submits the signature share to the network if this is true. "
                                                                "Returns an object containing the signature share if this is false."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};

    JSONRPCRequest new_request{request};
    new_request.params.setArray();
    for (unsigned int i = 1; i < request.params.size(); ++i) {
        new_request.params.push_back(request.params[i]);
    }
    return quorum_sign_helper(new_request, llmqType);
},
    };
}

static RPCHelpMan quorum_platformsign()
{
    return RPCHelpMan{"quorum platformsign",
        "Threshold-sign a message. It signs messages only for platform quorums\n",
        {
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "The quorum identifier."},
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true}, "Submits the signature share to the network if this is true. "
                                                                "Returns an object containing the signature share if this is false."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const Consensus::LLMQType llmqType{Params().GetConsensus().llmqTypePlatform};
    return quorum_sign_helper(request, llmqType);
},
    };
}

static bool VerifyRecoveredSigLatestQuorums(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const llmq::CQuorumManager& qman,
                                            int signHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig)
{
    // First check against the current active set, if it fails check against the last active set
    for (int signOffset : {0, llmq_params.dkgInterval}) {
        if (llmq::VerifyRecoveredSig(llmq_params.type, active_chain, qman, signHeight, id, msgHash, sig, signOffset) == llmq::VerifyRecSigStatus::Valid) {
            return true;
        }
    }
    return false;
}

static RPCHelpMan quorum_verify()
{
    return RPCHelpMan{"quorum verify",
        "Test if a quorum signature is valid for a request id and a message hash\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "Quorum signature to verify."},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Default{""},
                "The quorum identifier.\n"
                "Set to \"\" if you want to specify signHeight instead."},
            {"signHeight", RPCArg::Type::NUM, RPCArg::Default{-1},
                "The height at which the message was signed.\n"
                "Only works when quorumHash is \"\"."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};

    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[1], "id"));
    const uint256 msgHash(ParseHashV(request.params[2], "msgHash"));

    const bool use_bls_legacy = bls::bls_legacy_scheme.load();
    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[3].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    if (request.params[4].isNull() || (request.params[4].get_str().empty() && !request.params[5].isNull())) {
        int signHeight{-1};
        if (!request.params[5].isNull()) {
            signHeight = ParseInt32V(request.params[5], "signHeight");
        }
        return VerifyRecoveredSigLatestQuorums(*llmq_params_opt, chainman.ActiveChain(), *llmq_ctx.qman, signHeight, id, msgHash, sig);
    }

    uint256 quorumHash(ParseHashV(request.params[4], "quorumHash"));
    const auto quorum = llmq_ctx.qman->GetQuorum(llmqType, quorumHash);

    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    uint256 signHash = llmq::BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
    return sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
},
    };
}

static RPCHelpMan quorum_hasrecsig()
{
    return RPCHelpMan{"quorum hasrecsig",
        "Test if a valid recovered signature is present\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};
    if (!Params().GetLLMQ(llmqType).has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[1], "id"));
    const uint256 msgHash(ParseHashV(request.params[2], "msgHash"));

    return llmq_ctx.sigman->HasRecoveredSig(llmqType, id, msgHash);
},
    };
}

static RPCHelpMan quorum_getrecsig()
{
    return RPCHelpMan{"quorum getrecsig",
        "Get a recovered signature\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};
    if (!Params().GetLLMQ(llmqType).has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[1], "id"));
    const uint256 msgHash(ParseHashV(request.params[2], "msgHash"));

    llmq::CRecoveredSig recSig;
    if (!llmq_ctx.sigman->GetRecoveredSigForId(llmqType, id, recSig)) {
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
    return RPCHelpMan{"quorum isconflicting",
        "Test if a conflict exists\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"msgHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Message hash."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};
    if (!Params().GetLLMQ(llmqType).has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[1], "id"));
    const uint256 msgHash(ParseHashV(request.params[2], "msgHash"));

    return llmq_ctx.sigman->IsConflicting(llmqType, id, msgHash);
},
    };
}

static RPCHelpMan quorum_selectquorum()
{
    return RPCHelpMan{"quorum selectquorum",
        "Returns the quorum that would/should sign a request\n",
        {
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "LLMQ type."},
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{static_cast<Consensus::LLMQType>(ParseInt32V(request.params[0], "llmqType"))};
    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    const uint256 id(ParseHashV(request.params[1], "id"));

    UniValue ret(UniValue::VOBJ);

    const auto quorum = llmq::SelectQuorumForSigning(llmq_params_opt.value(), chainman.ActiveChain(), *llmq_ctx.qman, id);
    if (!quorum) {
        throw JSONRPCError(RPC_MISC_ERROR, "no quorums active");
    }
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());

    UniValue recoveryMembers(UniValue::VARR);
    for (int i = 0; i < quorum->params.recoveryMembers; ++i) {
        auto dmn = llmq_ctx.shareman->SelectMemberForRecovery(quorum, id, i);
        recoveryMembers.push_back(dmn->proTxHash.ToString());
    }
    ret.pushKV("recoveryMembers", recoveryMembers);

    return ret;
},
    };
}

static RPCHelpMan quorum_dkgsimerror()
{
    return RPCHelpMan{"quorum dkgsimerror",
        "This enables simulation of errors and malicious behaviour in the DKG. Do NOT use this on mainnet\n"
        "as you will get yourself very likely PoSe banned for this.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Error type."},
            {"rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Rate at which to simulate this error type (between 0 and 100)."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string type_str = request.params[0].get_str();
    int32_t rate = ParseInt32V(request.params[1], "rate");

    if (rate < 0 || rate > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 100");
    }

    if (const llmq::DKGError::type type = llmq::DKGError::from_string(type_str);
            type == llmq::DKGError::type::_COUNT) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type. See DKGError class implementation");
    } else {
        llmq::SetSimulatedDKGErrorRate(type, static_cast<double>(rate) / 100);
        return UniValue();
    }
},
    };
}

static RPCHelpMan quorum_getdata()
{
    return RPCHelpMan{"quorum getdata",
        "Send a QGETDATA message to the specified peer.\n",
        {
            {"nodeId", RPCArg::Type::NUM, RPCArg::Optional::NO, "The internal nodeId of the peer to request quorum data from."},
            {"llmqType", RPCArg::Type::NUM, RPCArg::Optional::NO, "The quorum type related to the quorum data being requested."},
            {"quorumHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The quorum hash related to the quorum data being requested."},
            {"dataMask", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "Specify what data to request.\n"
                "Possible values: 1 - Request quorum verification vector\n"
                "2 - Request encrypted contributions for member defined by \"proTxHash\". \"proTxHash\" must be specified if this option is used.\n"
                "3 - Request both, 1 and 2"},
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "The proTxHash the contributions will be requested for. Must be member of the specified LLMQ."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    CConnman& connman = EnsureConnman(node);

    NodeId nodeId = ParseInt64V(request.params[0], "nodeId");
    Consensus::LLMQType llmqType = static_cast<Consensus::LLMQType>(ParseInt32V(request.params[1], "llmqType"));
    uint256 quorumHash(ParseHashV(request.params[2], "quorumHash"));
    uint16_t nDataMask = static_cast<uint16_t>(ParseInt32V(request.params[3], "dataMask"));
    uint256 proTxHash;

    // Check if request wants ENCRYPTED_CONTRIBUTIONS data
    if (nDataMask & llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
        if (!request.params[4].isNull()) {
            proTxHash = ParseHashV(request.params[4], "proTxHash");
            if (proTxHash.IsNull()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "proTxHash invalid");
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "proTxHash missing");
        }
    }

    const auto quorum = llmq_ctx.qman->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }
    return connman.ForNode(nodeId, [&](CNode* pNode) {
        return llmq_ctx.qman->RequestQuorumData(pNode, connman, quorum, nDataMask, proTxHash);
    });
},
    };
}

static RPCHelpMan quorum_rotationinfo()
{
    return RPCHelpMan{
        "quorum rotationinfo",
        "Get quorum rotation information\n",
        {
            {"blockRequestHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The blockHash of the request."},
            {"extraShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Extra share"},
            {"baseBlockHashes", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The list of block hashes",
            {
                {"baseBlockHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            }},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    ;

    llmq::CGetQuorumRotationInfo cmd;
    llmq::CQuorumRotationInfo quorumRotationInfoRet;
    std::string strError;

    cmd.blockRequestHash = ParseHashV(request.params[0], "blockRequestHash");
    cmd.extraShare = request.params[1].isNull() ? false : ParseBoolV(request.params[1], "extraShare");

    if (!request.params[2].isNull()) {
        UniValue hashes;
        if (request.params[2].isStr() && hashes.read(request.params[2].get_str()) && hashes.isArray()) {
            // pass
        } else if (request.params[2].isArray()) {
            hashes = request.params[2].get_array();
        } else {
            throw std::runtime_error(std::string("Error parsing JSON: ") + request.params[2].get_str());
        }
        for (const auto& hash : hashes.get_array().getValues()) {
            cmd.baseBlockHashes.emplace_back(ParseHashV(hash, "baseBlockHash"));
        }
    }

    LOCK(cs_main);

    if (!BuildQuorumRotationInfo(*CHECK_NONFATAL(node.dmnman), *llmq_ctx.qsnapman, chainman, *llmq_ctx.qman,
                                 *llmq_ctx.quorum_block_processor, cmd, false, quorumRotationInfoRet, strError)) {
        throw JSONRPCError(RPC_INVALID_REQUEST, strError);
    }

    return quorumRotationInfoRet.ToJson();
},
    };
}

static RPCHelpMan quorum_dkginfo()
{
    return RPCHelpMan{
        "quorum dkginfo",
        "Return information regarding DKGs.\n",
        {
            {},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "active_dkgs", "Total number of active DKG sessions this node is participating in right now"},
                {RPCResult::Type::NUM, "next_dkg", "The number of blocks until the next potential DKG session"},
            }
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    llmq::CDKGDebugStatus status;
    llmq_ctx.dkg_debugman->GetLocalDebugStatus(status);
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("active_dkgs", int(status.sessions.size()));

    const int nTipHeight{WITH_LOCK(cs_main, return chainman.ActiveChain().Height())};
    auto minNextDKG = [](const Consensus::Params& consensusParams, int nTipHeight) {
        int minDkgWindow{std::numeric_limits<int>::max()};
        for (const auto& params: consensusParams.llmqs) {
            if (params.useRotation && (nTipHeight % params.dkgInterval <= params.signingActiveQuorumCount)) {
                return 1;
            }
            minDkgWindow = std::min(minDkgWindow, params.dkgInterval - (nTipHeight % params.dkgInterval));
        }
        return minDkgWindow;
    };
    ret.pushKV("next_dkg", minNextDKG(Params().GetConsensus(), nTipHeight));

    return ret;
},
    };
}

static RPCHelpMan quorum_help()
{
    return RPCHelpMan{
            "quorum",
            "Set of commands for quorums/LLMQs.\n"
            "To get help on individual commands, use \"help quorum command\".\n"
            "\nAvailable commands:\n"
            "  list              - List of on-chain quorums\n"
            "  listextended      - Extended list of on-chain quorums\n"
            "  info              - Return information about a quorum\n"
            "  dkginfo           - Return information about DKGs\n"
            "  dkgsimerror       - Simulates DKG errors and malicious behavior\n"
            "  dkgstatus         - Return the status of the current DKG process\n"
            "  memberof          - Checks which quorums the given masternode is a member of\n"
            "  sign              - Threshold-sign a message\n"
            "  verify            - Test if a quorum signature is valid for a request id and a message hash\n"
            "  hasrecsig         - Test if a valid recovered signature is present\n"
            "  getrecsig         - Get a recovered signature\n"
            "  isconflicting     - Test if a conflict exists\n"
            "  selectquorum      - Return the quorum that would/should sign a request\n"
            "  getdata           - Request quorum data from other masternodes in the quorum\n"
            "  rotationinfo      - Request quorum rotation information\n",
            {
                {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
            },
            RPCResults{},
            RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan verifychainlock()
{
    return RPCHelpMan{"verifychainlock",
        "Test if a quorum signature is valid for a ChainLock.\n",
        {
            {"blockHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash of the ChainLock."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature of the ChainLock."},
            {"blockHeight", RPCArg::Type::NUM, RPCArg::DefaultHint{"There will be an internal lookup of \"blockHash\" if this is not provided."}, "The height of the ChainLock."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const uint256 nBlockHash(ParseHashV(request.params[0], "blockHash"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    int nBlockHeight;
    const CBlockIndex* pIndex{nullptr};
    if (request.params[2].isNull()) {
        pIndex = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(nBlockHash));
        if (pIndex == nullptr) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "blockHash not found");
        }
        nBlockHeight = pIndex->nHeight;
    } else {
        nBlockHeight = request.params[2].getInt<int>();
        LOCK(cs_main);
        if (nBlockHeight < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
        }
        if (nBlockHeight <= chainman.ActiveChain().Height()) {
            pIndex = chainman.ActiveChain()[nBlockHeight];
        }
    }

    CBLSSignature sig;
    if (pIndex) {
        const bool use_legacy_signature{!DeploymentActiveAfter(pIndex, Params().GetConsensus(), Consensus::DEPLOYMENT_V19)};
        if (!sig.SetHexStr(request.params[1].get_str(), use_legacy_signature)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
        }
    } else {
        if (!sig.SetHexStr(request.params[1].get_str(), false) &&
                !sig.SetHexStr(request.params[1].get_str(), true)
        ) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
        }
    }

    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return llmq_ctx.clhandler->VerifyChainLock(llmq::CChainLockSig(nBlockHeight, nBlockHash, sig)) == llmq::VerifyRecSigStatus::Valid;
},
    };
}

static RPCHelpMan verifyislock()
{
    return RPCHelpMan{"verifyislock",
        "Test if a quorum signature is valid for an InstantSend Lock\n",
        {
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Request id."},
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "The InstantSend Lock signature to verify."},
            {"maxHeight", RPCArg::Type::NUM, RPCArg::Default{-1}, "The maximum height to search quorums from."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const uint256 id(ParseHashV(request.params[0], "id"));
    const uint256 txid(ParseHashV(request.params[1], "txid"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const CBlockIndex* pindexMined{nullptr};
    {
        LOCK(cs_main);
        uint256 hash_block;
        CTransactionRef tx = GetTransaction(/* block_index */ nullptr,  /* mempool */ nullptr, txid, Params().GetConsensus(), hash_block);
        if (tx && !hash_block.IsNull()) {
            pindexMined = chainman.m_blockman.LookupBlockIndex(hash_block);
        }
    }

    int maxHeight{-1};
    if (!request.params[3].isNull()) {
        maxHeight = request.params[3].getInt<int>();
    }

    int signHeight;
    if (pindexMined == nullptr || pindexMined->nHeight > maxHeight) {
        signHeight = maxHeight;
    } else { // pindexMined->nHeight <= maxHeight
        signHeight = pindexMined->nHeight;
    }

    CBlockIndex* pBlockIndex{nullptr};
    {
        LOCK(cs_main);
        if (signHeight == -1) {
            pBlockIndex = chainman.ActiveChain().Tip();
        } else {
            pBlockIndex = chainman.ActiveChain()[signHeight];
        }
    }

    CHECK_NONFATAL(pBlockIndex != nullptr);

    CBLSSignature sig;
    const bool use_bls_legacy{!DeploymentActiveAfter(pBlockIndex, Params().GetConsensus(), Consensus::DEPLOYMENT_V19)};
    if (!sig.SetHexStr(request.params[2].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    CHECK_NONFATAL(llmq_params_opt.has_value());
    return VerifyRecoveredSigLatestQuorums(*llmq_params_opt, chainman.ActiveChain(), *CHECK_NONFATAL(llmq_ctx.qman),
                                           signHeight, id, txid, sig);
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
                       {"blockHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "The height of the ChainLock."},
               },
               RPCResult{
                    RPCResult::Type::NUM, "", "The height of the current best ChainLock"},
               RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const uint256 nBlockHash(ParseHashV(request.params[0], "blockHash"));

    const int nBlockHeight = request.params[2].getInt<int>();
    if (nBlockHeight <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid block height");
    }
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const int32_t bestCLHeight = llmq_ctx.clhandler->GetBestChainLock().getHeight();
    if (nBlockHeight <= bestCLHeight) return bestCLHeight;

    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[1].get_str(), false) && !sig.SetHexStr(request.params[1].get_str(), true)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }


    const auto clsig{llmq::CChainLockSig(nBlockHeight, nBlockHash, sig)};
    const llmq::VerifyRecSigStatus ret{llmq_ctx.clhandler->VerifyChainLock(clsig)};
    if (ret == llmq::VerifyRecSigStatus::NoQuorum) {
        LOCK(cs_main);
        const ChainstateManager& chainman = EnsureChainman(node);
        const CBlockIndex* pIndex{chainman.ActiveChain().Tip()};
        throw JSONRPCError(RPC_MISC_ERROR, strprintf("No quorum found. Current tip height: %d hash: %s\n", pIndex->nHeight, pIndex->GetBlockHash().ToString()));
    }
    if (ret != llmq::VerifyRecSigStatus::Valid) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature");
    }

    PeerManager& peerman = EnsurePeerman(node);
    peerman.PostProcessMessage(llmq_ctx.clhandler->ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig)));
    return llmq_ctx.clhandler->GetBestChainLock().getHeight();
},
    };
}


void RegisterQuorumsRPCCommands(CRPCTable &tableRPC)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "evo",                &quorum_help,            },
    { "evo",                &quorum_list,            },
    { "evo",                &quorum_list_extended,   },
    { "evo",                &quorum_info,            },
    { "evo",                &quorum_dkginfo,         },
    { "evo",                &quorum_dkgstatus,       },
    { "evo",                &quorum_memberof,        },
    { "evo",                &quorum_sign,            },
    { "evo",                &quorum_platformsign,    },
    { "evo",                &quorum_verify,          },
    { "evo",                &quorum_hasrecsig,       },
    { "evo",                &quorum_getrecsig,       },
    { "evo",                &quorum_isconflicting,   },
    { "evo",                &quorum_selectquorum,    },
    { "evo",                &quorum_dkgsimerror,     },
    { "evo",                &quorum_getdata,         },
    { "evo",                &quorum_rotationinfo,    },
    { "evo",                &submitchainlock,        },
    { "evo",                &verifychainlock,        },
    { "evo",                &verifyislock,           },
};
// clang-format on
    for (const auto& command : commands) {
        tableRPC.appendCommand(command.name, &command);
    }
}
