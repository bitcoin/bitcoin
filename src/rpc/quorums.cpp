// Copyright (c) 2017-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/context.h>
#include <active/masternode.h>
#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <chainlock/handler.h>
#include <evo/deterministicmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <llmq/debug.h>
#include <llmq/dkgsession.h>
#include <llmq/observer.h>
#include <llmq/options.h>
#include <llmq/quorumproofs.h>
#include <evo/simplifiedmns.h>
#include <clientversion.h>
#include <llmq/quorumsman.h>
#include <llmq/signhash.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/snapshot.h>
#include <llmq/utils.h>
#include <rpc/json_help.h>
#include <util/helpers.h>

#include <chainparams.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <index/txindex.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/context.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <util/check.h>
#include <validation.h>

#include <iomanip>
#include <map>
#include <optional>

using node::GetTransaction;
using node::NodeContext;

static Consensus::LLMQType ParseLLMQType(const UniValue& value)
{
    const auto llmq_type{static_cast<Consensus::LLMQType>(value.getInt<uint8_t>())};
    if (!Params().GetLLMQ(llmq_type).has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }
    return llmq_type;
}

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
            RPCResult::Type::OBJ_DYN, "", "json object with quorum type name as keys",
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
        count = request.params[0].getInt<int>();
        if (count < -1) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());

    for (const auto& type : llmq::GetEnabledQuorumTypes(chainman, pindexTip)) {
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
            RPCResult::Type::OBJ_DYN, "", "json object with quorum type name as keys",
            {
                {RPCResult::Type::ARR, "quorumName", "List of quorum details per quorum type",
                {
                    {RPCResult::Type::OBJ_DYN, "", "json object with quorum hash as keys. Note: most recent quorums come first.",
                    {
                        {RPCResult::Type::OBJ, "xxxx", "Quorum details",
                        {
                            {RPCResult::Type::NUM, "quorumIndex", /*optional=*/true, "Quorum index (applicable only to rotated quorums)."},
                            {RPCResult::Type::NUM, "creationHeight", "Block height where the DKG started."},
                            {RPCResult::Type::STR_HEX, "minedBlockHash", "Blockhash where the commitment was mined."},
                            {RPCResult::Type::NUM, "numValidMembers", "The total of valid members."},
                            {RPCResult::Type::STR, "healthRatio", "The ratio of healthy members to quorum size. Range [0.0 - 1.0]."}
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
        nHeight = request.params[0].getInt<int>();
        if (nHeight < 0 || nHeight > WITH_LOCK(cs_main, return chainman.ActiveChain().Height())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
        }
    }

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pblockindex = nHeight != -1 ? WITH_LOCK(cs_main, return chainman.ActiveChain()[nHeight]) : WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());

    for (const auto& type : llmq::GetEnabledQuorumTypes(chainman, pblockindex)) {
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
                j.pushKV("numValidMembers", num_valid_members);
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
                                const llmq::CQuorum& quorum, bool includeMembers, bool includeSkShare)
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("height", quorum.m_quorum_base_block_index->nHeight);
    ret.pushKV("type", std::string(quorum.params.name));
    ret.pushKV("quorumHash", quorum.qc->quorumHash.ToString());
    ret.pushKV("quorumIndex", quorum.qc->quorumIndex);
    ret.pushKV("minedBlock", quorum.minedBlockHash.ToString());

    if (quorum.params.useRotation) {
        auto previousActiveCommitment = quorum_block_processor.GetLastMinedCommitmentsByQuorumIndexUntilBlock(quorum.params.type, quorum.m_quorum_base_block_index, quorum.qc->quorumIndex, 0);
        if (previousActiveCommitment.has_value()) {
            int previousConsecutiveDKGFailures = (quorum.m_quorum_base_block_index->nHeight - previousActiveCommitment.value()->nHeight) /  quorum.params.dkgInterval - 1;
            ret.pushKV("previousConsecutiveDKGFailures", previousConsecutiveDKGFailures);
        }
        else {
            ret.pushKV("previousConsecutiveDKGFailures", 0);
        }
    }

    if (includeMembers) {
        UniValue membersArr(UniValue::VARR);
        for (size_t i = 0; i < quorum.members.size(); i++) {
            const auto& dmn = quorum.members[i];
            UniValue mo(UniValue::VOBJ);
            mo.pushKV("proTxHash", dmn->proTxHash.ToString());
            if (IsDeprecatedRPCEnabled("service")) {
                mo.pushKV("service", dmn->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
            }
            mo.pushKV("addresses", GetNetInfoWithLegacyFields(*dmn->pdmnState, dmn->nType));
            mo.pushKV("pubKeyOperator", dmn->pdmnState->pubKeyOperator.ToString());
            mo.pushKV("valid", static_cast<bool>(quorum.qc->validMembers[i]));
            if (quorum.qc->validMembers[i]) {
                if (quorum.params.is_single_member()) {
                    mo.pushKV("pubKeyShare", dmn->pdmnState->pubKeyOperator.ToString());
                } else {
                    CBLSPublicKey pubKey = quorum.GetPubKeyShare(i);
                    if (pubKey.IsValid()) {
                        mo.pushKV("pubKeyShare", pubKey.ToString());
                    }
                }
            }
            membersArr.push_back(mo);
        }

        ret.pushKV("members", membersArr);
    }
    ret.pushKV("quorumPublicKey", quorum.qc->quorumPublicKey.ToString());
    const CBLSSecretKey& skShare = quorum.GetSkShare();
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
            {"includeSkShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Include secret key share in output.",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "height", "Quorum Height"},
                {RPCResult::Type::STR, "type", "Quorum type"},
                GetRpcResult("quorumHash"),
                GetRpcResult("quorumIndex"),
                {RPCResult::Type::STR_HEX, "minedBlock", "Blockhash where the commitment was mined."},
                {RPCResult::Type::NUM, "previousConsecutiveDKGFailures", /*optional=*/true, "Number of previous consecutive DKG failures. Only present for rotation-enabled quorums."},
                {RPCResult::Type::ARR, "members", "Members of quorum",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            GetRpcResult("proTxHash"),
                            GetRpcResult("service", /*optional=*/true),
                            GetRpcResult("addresses"),
                            GetRpcResult("pubKeyOperator"),
                            {RPCResult::Type::BOOL, "valid", "True if member is valid for this DKG"},
                            {RPCResult::Type::STR_HEX, "pubKeyShare", /*optional=*/true, "Share of BLS public key of the member. Only present if member is valid."}
                        }},
                    },
                },
                {RPCResult::Type::STR_HEX, "quorumPublicKey", "BLS public key of the quorum"},
                {RPCResult::Type::STR_HEX, "secretKeyShare", /*optional=*/true, "Share of the BLS secret key of the quorum. Only present if includeSkShare is set and the share is valid."},
            },
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

    const uint256 quorumHash(ParseHashV(request.params[1], "quorumHash"));
    bool includeSkShare = false;
    if (!request.params[2].isNull()) {
        includeSkShare = ParseBoolV(request.params[2], "includeSkShare");
    }

    const auto quorum = llmq_ctx.qman->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    return BuildQuorumInfo(*llmq_ctx.quorum_block_processor, *quorum, true, includeSkShare);
},
    };
}

static RPCResult quorum_dkgstatus_help()
{
    auto ret = llmq::CDKGDebugManager::GetJsonHelp(/*key=*/"", /*optional=*/false, /*inner_optional=*/true);
    auto mod_inner = ret.m_inner;
    mod_inner.push_back({RPCResult::Type::ARR, "quorumConnections", "Array of objects containing quorum connection information", {
        {RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::STR, "llmqType", "Quorum type name"},
            GetRpcResult("quorumIndex"),
            {RPCResult::Type::NUM, "pQuorumBaseBlockIndex", /*optional=*/true, "Height of the quorum’s base block"},
            GetRpcResult("quorumHash", /*optional=*/true),
            {RPCResult::Type::NUM, "pindexTip", /*optional=*/true, "Height of the quorum index tip"},
            {RPCResult::Type::ARR, "quorumConnections", /*optional=*/true, "", {
                {RPCResult::Type::OBJ, "", "", {
                    GetRpcResult("proTxHash"),
                    {RPCResult::Type::BOOL, "connected", "Returns true if connection is active"},
                    {RPCResult::Type::STR, "address", /*optional=*/true, "IP address and port of the masternode"},
                    {RPCResult::Type::BOOL, "outbound", /*optional=*/true, "Returns true if outbound connection"},
            }}}}
        }}}});
    mod_inner.push_back({RPCResult::Type::ARR, "minableCommitments", "Array of objects containing commitments the next block would include", {
        llmq::CFinalCommitment::GetJsonHelp(/*key=*/"", /*optional=*/false)}});
    return RPCResult{ret.m_type, ret.m_key_name, ret.m_description, mod_inner};
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
        quorum_dkgstatus_help(),
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    int detailLevel = 0;
    if (!request.params[0].isNull()) {
        detailLevel = request.params[0].getInt<int>();
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    UniValue ret(UniValue::VOBJ);
    UniValue minableCommitments(UniValue::VARR);
    UniValue quorumArrConnections(UniValue::VARR);

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (const auto* debugman = node.active_ctx ? node.active_ctx->dkgdbgman.get()
                                               : node.observer_ctx ? node.observer_ctx->dkgdbgman.get()
                                                                   : nullptr; debugman) {
        ret = debugman->ToJson(detailLevel);
    }

    const CConnman& connman = EnsureConnman(node);
    const ChainstateManager& chainman = EnsureChainman(node);
    const CBlockIndex* const pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const int tipHeight = pindexTip->nHeight;
    const uint256 proTxHash = node.active_ctx ? node.active_ctx->nodeman->GetProTxHash() : uint256{};
    for (const auto& type : llmq::GetEnabledQuorumTypes(chainman, pindexTip)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        const auto& llmq_params = llmq_params_opt.value();
        bool rotation_enabled = llmq::IsQuorumRotationEnabled(llmq_params, pindexTip);
        int quorums_num = rotation_enabled ? llmq_params.signingActiveQuorumCount : 1;

        for (const int quorumIndex : util::irange(quorums_num)) {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("llmqType", std::string(llmq_params.name));
            obj.pushKV("quorumIndex", quorumIndex);

            if (node.active_ctx) {
                int quorumHeight = tipHeight - (tipHeight % llmq_params.dkgInterval) + quorumIndex;
                if (quorumHeight <= tipHeight) {
                    const CBlockIndex* pQuorumBaseBlockIndex = pindexTip->GetAncestor(quorumHeight);
                    obj.pushKV("pQuorumBaseBlockIndex", pQuorumBaseBlockIndex->nHeight);
                    obj.pushKV("quorumHash", pQuorumBaseBlockIndex->GetBlockHash().ToString());
                    obj.pushKV("pindexTip", pindexTip->nHeight);

                    auto allConnections = llmq::utils::GetQuorumConnections(llmq_params, *CHECK_NONFATAL(node.sporkman),
                                                                            {*node.dmnman, *llmq_ctx.qsnapman, chainman,
                                                                             pQuorumBaseBlockIndex},
                                                                            proTxHash, /*onlyOutbound=*/false);
                    auto outboundConnections = llmq::utils::GetQuorumConnections(llmq_params, *node.sporkman,
                                                                                 {*node.dmnman, *llmq_ctx.qsnapman,
                                                                                  chainman, pQuorumBaseBlockIndex},
                                                                                 proTxHash, /*onlyOutbound=*/true);
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
        std::optional<std::vector<llmq::CFinalCommitment>> vfqc =
            llmq_ctx.quorum_block_processor->GetMineableCommitments(llmq_params, chainman.ActiveChain().Height() + 1);
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
        RPCResult{
            RPCResult::Type::ARR, "quorums", "",
            {
                {RPCResult::Type::OBJ, "", "Quorum Info",
                {
                    {RPCResult::Type::ELISION, "", "See `help quorum info` for details"},
                    {RPCResult::Type::BOOL, "isValidMember", ""},
                    {RPCResult::Type::NUM, "memberIndex", ""},
                }},
            },
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    uint256 protxHash(ParseHashV(request.params[0], "proTxHash"));
    int scanQuorumsCount = -1;
    if (!request.params[1].isNull()) {
        scanQuorumsCount = request.params[1].getInt<int>();
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
    for (const auto& type : llmq::GetEnabledQuorumTypes(chainman, pindexTip)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        size_t count = llmq_params_opt->signingActiveQuorumCount;
        if (scanQuorumsCount != -1) {
            count = static_cast<size_t>(scanQuorumsCount);
        }
        auto quorums = llmq_ctx.qman->ScanQuorums(llmq_params_opt->type, count);
        for (auto& quorum : quorums) {
            if (quorum->IsMember(dmn->proTxHash)) {
                auto json = BuildQuorumInfo(*llmq_ctx.quorum_block_processor, *quorum, false, false);
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
    if (!node.active_ctx) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Only available in masternode mode.");
    }

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
    if (!llmq::CSigSharesManager::IsQuorumSigningAllowed(chainman)) {
        throw JSONRPCError(RPC_MISC_ERROR,
                           "Quorum signing is disabled until snapshot background validation completes");
    }
    if (fSubmit) {
        // Platform re-signs expired withdrawals under the same request id with a new message
        // hash, so platform-type sessions must not refuse a changed message
        const bool allow_diff_msghash{llmqType == Params().GetConsensus().llmqTypePlatform};
        return CHECK_NONFATAL(node.active_ctx)
            ->shareman->AsyncSignIfMember(llmqType, id, msgHash, quorumHash, /*allowReSign=*/false,
                                          /*allowDiffMsgHashSigning=*/allow_diff_msghash);
    } else {
        const auto pQuorum = [&]() {
            if (quorumHash.IsNull()) {
                const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
                return llmq::SelectQuorumForSigning(llmq_params_opt.value(), active_chain, *llmq_ctx.qman, id);
            } else {
                return llmq_ctx.qman->GetQuorum(llmqType, quorumHash);
            }
        }();

        if (pQuorum == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
        }

        auto sigShare = CHECK_NONFATAL(node.active_ctx)->shareman->CreateSigShare(*pQuorum, id, msgHash);

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

namespace {
const RPCResults quorum_sign_result{
    RPCResult{"if submit is set to true", RPCResult::Type::BOOL, "result", "result of signing, true if success"},
    RPCResult{"if submit is not set or set to false", RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::NUM, "llmqType", "Quorum type"},
            {RPCResult::Type::STR_HEX, "quorumHash", "Quorum Hash"},
            {RPCResult::Type::NUM, "quorumMember", "Number of quorum member"},
            {RPCResult::Type::STR_HEX, "id", "Request ID"},
            {RPCResult::Type::STR_HEX, "msgHash", "Hash of message"},
            {RPCResult::Type::STR_HEX, "signHash", "Hash of signature"},
            {RPCResult::Type::STR_HEX, "signature", "Hex encoded signature"},
        },
    },
};
} // anonymous namespace

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
                                                                "Returns an object containing the signature share if this is false.",
             RPCArgOptions{.skip_type_check = true}},
        },
        quorum_sign_result,
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

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
                                                                "Returns an object containing the signature share if this is false.",
             RPCArgOptions{.skip_type_check = true}},
        },
        quorum_sign_result,
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if the signature is valid"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    CHECK_NONFATAL(llmq_params_opt.has_value());

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
            signHeight = request.params[5].getInt<int>();
        }
        const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
        return VerifyRecoveredSigLatestQuorums(*llmq_params_opt, active_chain, *llmq_ctx.qman, signHeight, id, msgHash, sig);
    }

    uint256 quorumHash(ParseHashV(request.params[4], "quorumHash"));
    const auto quorum = llmq_ctx.qman->GetQuorum(llmqType, quorumHash);

    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    llmq::SignHash signHash{llmqType, quorum->qc->quorumHash, id, msgHash};
    return sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash.Get());
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if node has this recovered signature"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

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
        llmq::CRecoveredSig::GetJsonHelp(/*key=*/"", /*optional=*/false),
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

    const uint256 id(ParseHashV(request.params[1], "id"));
    const uint256 msgHash(ParseHashV(request.params[2], "msgHash"));

    llmq::CRecoveredSig recSig;
    if (!llmq_ctx.sigman->GetRecoveredSig(llmqType, id, msgHash, recSig)) {
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if this msgHash is conflicting with previous signing sessions"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};

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
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "quorumHash", "Hash of chosen quorum"},
                {RPCResult::Type::ARR, "recoveryMembers", "List of members to use for signature recovery",
                    {{RPCResult::Type::STR_HEX, "hash", "ProTxHash of member"}}
                },
            }
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[0])};
    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    CHECK_NONFATAL(llmq_params_opt.has_value());

    const uint256 id(ParseHashV(request.params[1], "id"));

    UniValue ret(UniValue::VOBJ);

    const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
    const auto quorum = llmq::SelectQuorumForSigning(llmq_params_opt.value(), active_chain, *llmq_ctx.qman, id);
    if (!quorum) {
        throw JSONRPCError(RPC_MISC_ERROR, "no quorums active");
    }
    ret.pushKV("quorumHash", quorum->qc->quorumHash.ToString());

    UniValue recoveryMembers(UniValue::VARR);
    for (int i = 0; i < quorum->params.recoveryMembers; ++i) {
        auto dmn = llmq::CSigSharesManager::SelectMemberForRecovery(*quorum, id, i);
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
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string type_str = request.params[0].get_str();
    int32_t rate = request.params[1].getInt<int>();

    if (rate < 0 || rate > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid rate. Must be between 0 and 100");
    }

    if (const llmq::DKGError::type type = llmq::DKGError::from_string(type_str);
            type == llmq::DKGError::type::_COUNT) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type. See DKGError class implementation");
    } else {
        llmq::SetSimulatedDKGErrorRate(type, static_cast<double>(rate) / 100);
        return NullUniValue;
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if the message QGETDATA has been successfully sent"},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    CConnman& connman = EnsureConnman(node);

    NodeId nodeId = request.params[0].getInt<int64_t>();
    const Consensus::LLMQType llmqType{ParseLLMQType(request.params[1])};
    uint256 quorumHash(ParseHashV(request.params[2], "quorumHash"));
    uint16_t nDataMask = static_cast<uint16_t>(request.params[3].getInt<int>());
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
        if (pNode->GetVerifiedProRegTxHash().IsNull()) return false;
        if (!quorum->m_quorum_base_block_index) return false;
        const llmq::CQuorumDataRequest request(llmqType, quorum->qc->quorumHash, nDataMask, proTxHash);
        const llmq::CQuorumDataRequestKey key(pNode->GetVerifiedProRegTxHash(), true, quorum->qc->quorumHash, llmqType);
        if (llmq_ctx.qman->RegisterDataRequest(key, request) != llmq::DataRequestRegistration::Accepted) return false;
        connman.PushMessage(pNode, CNetMsgMaker(pNode->GetCommonVersion()).Make(NetMsgType::QGETDATA, request));
        return true;
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
            {"extraShare", RPCArg::Type::BOOL, RPCArg::Default{false}, "Extra share",
             RPCArgOptions{.skip_type_check = true}},
            {"baseBlockHashes", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "The list of block hashes",
            {
                {"baseBlockHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            }},
        },
        llmq::CQuorumRotationInfo::GetJsonHelp(/*key=*/"", /*optional=*/false),
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    llmq::CGetQuorumRotationInfo cmd;
    llmq::CQuorumRotationInfo quorumRotationInfoRet;
    std::string strError;

    cmd.blockRequestHash = ParseHashV(request.params[0], "blockRequestHash");
    cmd.extraShare = request.params[1].isNull() ? false : ParseBoolV(request.params[1], "extraShare");

    if (!request.params[2].isNull()) {
        const auto& hashes = request.params[2].get_array();
        for (const auto& hash : hashes.getValues()) {
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
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::DefaultHint{"local active masternode proTxHash, if any"},
                "The proTxHash of the masternode to report upcoming DKG participation for. Empty string is treated as the default."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "active_dkgs", "Total number of active DKG sessions this node is participating in right now"},
                {RPCResult::Type::NUM, "next_dkg", "The number of blocks until the next potential DKG session"},
                GetRpcResult("proTxHash", /*optional=*/true),
                {RPCResult::Type::ARR, "upcoming_dkgs", /*optional=*/true, "Upcoming DKG sessions for the given proTxHash whose work block is already mined. For rotated quorums all indices in a cycle share the cycle base work block",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        GetRpcResult("llmqType"),
                        GetRpcResult("quorumIndex"),
                        {RPCResult::Type::NUM, "quorumHeight", "The height at which the quorum session starts"},
                        {RPCResult::Type::NUM, "blocksUntilStart", "The number of blocks until the quorum session starts"},
                        {RPCResult::Type::BOOL, "known", "Whether participation could be determined"},
                        {RPCResult::Type::STR, "reason", /*optional=*/true, "Why participation could not be determined"},
                        {RPCResult::Type::BOOL, "isMember", /*optional=*/true, "Whether the masternode is a member of the upcoming quorum"},
                        {RPCResult::Type::NUM, "workBlockHeight", /*optional=*/true, "The height of the work block used to compute membership"},
                        {RPCResult::Type::STR_HEX, "workBlockHash", /*optional=*/true, "The hash of the work block used to compute membership"},
                    }},
                }},
            }
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    if (!node.active_ctx && !node.observer_ctx) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Only available in masternode or watch-only mode.");
    }
    const auto& dkgdbgman = *(node.active_ctx ? node.active_ctx->dkgdbgman.get() : node.observer_ctx->dkgdbgman.get());

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("active_dkgs", dkgdbgman.GetSessionCount());

    const ChainstateManager& chainman = EnsureChainman(node);
    const auto& consensus = chainman.GetParams().GetConsensus();
    const CBlockIndex* const pindexTip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    CHECK_NONFATAL(pindexTip);
    const int nTipHeight{pindexTip->nHeight};
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
    ret.pushKV("next_dkg", minNextDKG(consensus, nTipHeight));

    const auto quorum_type_known_enabled = [&](Consensus::LLMQType llmq_type, int quorum_base_height) {
        const int quorum_base_predecessor_height{quorum_base_height - 1};
        if (quorum_base_predecessor_height <= nTipHeight) {
            const CBlockIndex* const pQuorumBasePredecessor = pindexTip->GetAncestor(quorum_base_predecessor_height);
            return pQuorumBasePredecessor != nullptr && chainman.IsQuorumTypeEnabled(llmq_type, pQuorumBasePredecessor);
        }

        // The future predecessor is not mined yet, but both DIP0024 conditions are pure height
        // checks (buried deployment); evaluate them at the future height and let pindexTip stand
        // in for the rest (versionbits TESTDUMMY is monotonic once active).
        return chainman.IsQuorumTypeEnabled(llmq_type, pindexTip,
                                            /*optDIP0024IsActive=*/quorum_base_height >=
                                                consensus.DeploymentHeight(Consensus::DEPLOYMENT_DIP0024),
                                            /*optHaveDIP0024Quorums=*/quorum_base_predecessor_height >=
                                                consensus.DIP0024QuorumsHeight);
    };

    uint256 proTxHash;
    if (!request.params[0].isNull() && !request.params[0].get_str().empty()) {
        proTxHash = ParseHashV(request.params[0], "proTxHash");
    } else if (node.active_ctx) {
        proTxHash = node.active_ctx->nodeman->GetProTxHash();
    }

    if (!proTxHash.IsNull()) {
        ret.pushKV("proTxHash", proTxHash.ToString());
        UniValue upcoming(UniValue::VARR);
        for (const auto& llmq_params : consensus.llmqs) {
            // Whether a rotated cycle applies at the *upcoming* cycle base is what matters here,
            // not whether the tip's own current cycle is rotated. Iterate every possible index
            // for any llmq type that supports rotation and decide per-entry below.
            const int quorums_num = llmq_params.useRotation ? llmq_params.signingActiveQuorumCount : 1;

            for (const int quorumIndex : util::irange(quorums_num)) {
                int quorumHeight = nTipHeight - (nTipHeight % llmq_params.dkgInterval) + quorumIndex;
                if (quorumHeight <= nTipHeight) {
                    quorumHeight += llmq_params.dkgInterval;
                }
                const int cycleBaseHeight{quorumHeight - quorumIndex};
                if (!quorum_type_known_enabled(llmq_params.type, cycleBaseHeight)) {
                    continue;
                }

                // IsQuorumRotationEnabled gates on DIP0024 (a buried deployment) at the block
                // preceding the cycle base, so the upcoming cycle's rotation state is a pure
                // height check. A rotation-capable type without rotation active at its cycle
                // base has no canonical member selection, so skip it (this can only happen
                // around DIP0024 activation).
                if (llmq_params.useRotation &&
                    (cycleBaseHeight < 1 || cycleBaseHeight < consensus.DeploymentHeight(Consensus::DEPLOYMENT_DIP0024))) {
                    continue;
                }

                UniValue obj(UniValue::VOBJ);
                obj.pushKV("llmqType", static_cast<int>(llmq_params.type));
                obj.pushKV("quorumIndex", quorumIndex);
                obj.pushKV("quorumHeight", quorumHeight);
                obj.pushKV("blocksUntilStart", quorumHeight - nTipHeight);

                // All indices of a rotated cycle share the work block of their cycle base, so
                // gate availability on the work block height rather than each index's start
                // height; for non-rotated types cycleBaseHeight == quorumHeight anyway
                // workHeight cannot be negative: the upcoming base is a positive multiple of
                // dkgInterval, and every dkgInterval exceeds WORK_DIFF_DEPTH
                const int workHeight{cycleBaseHeight - llmq::WORK_DIFF_DEPTH};
                if (workHeight > nTipHeight) {
                    continue;
                }

                const CBlockIndex* const pWorkBlockIndex = pindexTip->GetAncestor(workHeight);
                if (!DeploymentActiveAfter(pWorkBlockIndex, consensus, Consensus::DEPLOYMENT_V20)) {
                    obj.pushKV("known", false);
                    obj.pushKV("reason", "pre-v20 quorum selection needs future quorum base block hash");
                    upcoming.push_back(obj);
                    continue;
                }

                const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
                const auto predicted_members = llmq::utils::ComputeQuorumMembersFromWorkBlock(
                    llmq_params.type,
                    {*CHECK_NONFATAL(node.dmnman), *CHECK_NONFATAL(llmq_ctx.qsnapman), chainman, pindexTip},
                    pWorkBlockIndex, quorumHeight);
                if (!predicted_members.has_value()) {
                    obj.pushKV("known", false);
                    obj.pushKV("reason", "rotated quorum snapshots are not available yet");
                    upcoming.push_back(obj);
                    continue;
                }

                obj.pushKV("known", true);

                bool is_member{false};
                for (const auto& member : *predicted_members) {
                    if (member->proTxHash == proTxHash) {
                        is_member = true;
                        break;
                    }
                }
                obj.pushKV("isMember", is_member);
                obj.pushKV("workBlockHeight", pWorkBlockIndex->nHeight);
                obj.pushKV("workBlockHash", pWorkBlockIndex->GetBlockHash().ToString());
                upcoming.push_back(obj);
            }
        }
        ret.pushKV("upcoming_dkgs", upcoming);
    }

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
            RPCResult{RPCResult::Type::NONE, "", ""},
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if the chainlock is valid"},
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
        const bool use_legacy_signature{!DeploymentActiveAfter(pIndex, chainman.GetConsensus(), Consensus::DEPLOYMENT_V19)};
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
    const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
    return chainlock::VerifyChainLock(Params().GetConsensus(), active_chain, *CHECK_NONFATAL(llmq_ctx.qman),
                                      chainlock::ChainLockSig{nBlockHeight, nBlockHash, sig}) ==
           llmq::VerifyRecSigStatus::Valid;
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
        RPCResult{RPCResult::Type::BOOL, "", "Returns true if the instantsend lock is valid"},
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

    const CBlockIndex* pBlockIndex{nullptr};
    {
        LOCK(cs_main);
        if (signHeight == -1) {
            pBlockIndex = chainman.ActiveChain().Tip();
        } else {
            pBlockIndex = chainman.ActiveChain()[signHeight];
        }
    }

    if (pBlockIndex == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    CBLSSignature sig;
    const bool use_bls_legacy{!DeploymentActiveAfter(pBlockIndex, chainman.GetConsensus(), Consensus::DEPLOYMENT_V19)};
    if (!sig.SetHexStr(request.params[2].get_str(), use_bls_legacy)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto llmq_params_opt = Params().GetLLMQ(llmqType);
    CHECK_NONFATAL(llmq_params_opt.has_value());
    const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
    return VerifyRecoveredSigLatestQuorums(*llmq_params_opt, active_chain, *CHECK_NONFATAL(llmq_ctx.qman),
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
    CHECK_NONFATAL(node.chainlocks);
    const int32_t bestCLHeight = node.chainlocks->GetBestChainLock().getHeight();
    if (nBlockHeight <= bestCLHeight) return bestCLHeight;

    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[1].get_str(), false) && !sig.SetHexStr(request.params[1].get_str(), true)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    const ChainstateManager& chainman = EnsureChainman(node);
    const auto clsig{chainlock::ChainLockSig(nBlockHeight, nBlockHash, sig)};
    const CChain& active_chain = *WITH_LOCK(::cs_main, return &chainman.ActiveChain());
    const llmq::VerifyRecSigStatus ret{
        chainlock::VerifyChainLock(Params().GetConsensus(), active_chain, *llmq_ctx.qman, clsig)};
    if (ret == llmq::VerifyRecSigStatus::NoQuorum) {
        LOCK(cs_main);
        const CBlockIndex* pIndex{chainman.ActiveChain().Tip()};
        throw JSONRPCError(RPC_MISC_ERROR, strprintf("No quorum found. Current tip height: %d hash: %s\n", pIndex->nHeight, pIndex->GetBlockHash().ToString()));
    }
    if (ret != llmq::VerifyRecSigStatus::Valid) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature");
    }

    PeerManager& peerman = EnsurePeerman(node);
    CHECK_NONFATAL(node.clhandler);
    peerman.PostProcessMessage(node.clhandler->ProcessNewChainLock(-1, clsig, *llmq_ctx.qman, ::SerializeHash(clsig)));
    return node.chainlocks->GetBestChainLock().getHeight();
},
    };
}


static RPCHelpMan getchainlockbyheight()
{
    return RPCHelpMan{
        "getchainlockbyheight",
        "Read a historical ChainLock from coinbases on disk.\n"
        "Only ChainLocks embedded in a later block's coinbase on the active chain can be returned.\n"
        "The current best ChainLock is unavailable until it has been embedded; use getbestchainlock to read it.\n"
        "Heights skipped by coinbase ChainLocks cannot be returned, even if they are ChainLocked.\n"
        "A completed lookup with no matching embedded ChainLock returns \"Chainlock not found for height\".\n"
        "Lookups may fail if required block data is unavailable (e.g. on pruned nodes).\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "Block height"},
        },
        RPCResult{RPCResult::Type::OBJ,
                  "",
                  "",
                  {
                      {RPCResult::Type::NUM, "height", "Chainlocked height"},
                      {RPCResult::Type::STR_HEX, "blockhash", "Block hash"},
                      {RPCResult::Type::STR_HEX, "signature", "BLS signature"},
                      {RPCResult::Type::NUM, "cbtx_height", "Height where CL was embedded"},
                  }},
        RPCExamples{HelpExampleCli("getchainlockbyheight", "100") + HelpExampleRpc("getchainlockbyheight", "100")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const int height = request.params[0].getInt<int>();
            if (height < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "height must be non-negative");
            }

            const NodeContext& node = EnsureAnyNodeContext(request.context);
            const ChainstateManager& chainman = EnsureChainman(node);
            CBlockIndex* tip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
            CHECK_NONFATAL(tip != nullptr);
            try {
                chainlock::CoinbaseChainLockReader reader(tip);
                const auto entry = reader.Find(height, height);
                if (!entry) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Chainlock not found for height");
                {
                    LOCK(cs_main);
                    if (!chainman.ActiveChain().Contains(entry->carrier))
                        throw JSONRPCError(RPC_MISC_ERROR, "Chain changed during ChainLock lookup; retry");
                }
                UniValue result(UniValue::VOBJ);
                result.pushKV("height", height);
                result.pushKV("blockhash", entry->block_hash.ToString());
                result.pushKV("signature", entry->Signed().getSig().ToString());
                result.pushKV("cbtx_height", entry->carrier->nHeight);
                return result;
            } catch (const std::exception& e) {
                throw JSONRPCError(RPC_MISC_ERROR, e.what());
            }
        },
    };
}

static RPCHelpMan getquorumproofchain()
{
    return RPCHelpMan{
        "getquorumproofchain",
        "Generate a DASHNC02 mining-transaction proof and authenticated record openings. Reads required historical "
        "blocks on demand.\n",
        {
            {"checkpoint_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Release checkpoint block hash"},
            {"height", RPCArg::Type::NUM, RPCArg::Default{0},
             "Minimum certified target height; zero selects the latest available ChainLock"},
            {"quorum_hash", RPCArg::Type::STR, RPCArg::Default{""}, "Optional Platform quorum hash to open"},
            {"llmq_type", RPCArg::Type::NUM, RPCArg::Default{0}, "Required with quorum_hash"},
            {"node_count", RPCArg::Type::NUM, RPCArg::Default{0}, "Number of eligible EvoNode records (0..15)"},
        },
        RPCResult{RPCResult::Type::OBJ,
                  "",
                  "",
                  {
                      {RPCResult::Type::STR_HEX, "proof_hex", "DASHNC02 bytes"},
                      {RPCResult::Type::STR_HEX, "bootstrap_hex", "Proof and record openings; empty when no records requested"},
                      {RPCResult::Type::OBJ, "target", "Authenticated target state", {{RPCResult::Type::ELISION, "", ""}}},
                  }},
        RPCExamples{HelpExampleCli("getquorumproofchain", "\"checkpoint_hash\"")},
        [&](const RPCHelpMan&, const JSONRPCRequest& request) -> UniValue {
            const auto& node = EnsureAnyNodeContext(request.context);
            const auto& ctx = EnsureLLMQContext(node);
            const auto& chainman = EnsureChainman(node);
            const auto anchorHash = ParseHashV(request.params[0], "checkpoint_hash");
            const int32_t minimum = request.params[1].isNull() ? 0 : request.params[1].getInt<int32_t>();
            const auto quorumText = request.params[2].isNull() ? std::string{} : request.params[2].get_str();
            const int type = request.params[3].isNull() ? 0 : request.params[3].getInt<int>();
            const int nodeCount = request.params[4].isNull() ? 0 : request.params[4].getInt<int>();
            if (minimum < 0 || type < 0 || type > 255 || nodeCount < 0 || nodeCount > 15 ||
                (quorumText.empty() != (type == 0)) || (!quorumText.empty() && (quorumText.size() != 64 || !IsHex(quorumText)))) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proof request");
            }
            const CBlockIndex* checkpoint;
            CBlockIndex* tip = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
            CHECK_NONFATAL(tip != nullptr);
            {
                LOCK(cs_main);
                checkpoint = chainman.m_blockman.LookupBlockIndex(anchorHash);
                if (!checkpoint || tip->GetAncestor(checkpoint->nHeight) != checkpoint)
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Checkpoint is not on the active chain");
            }
            try {
                chainlock::CoinbaseChainLockReader reader(tip);
                const int64_t start = std::max<int64_t>(minimum, int64_t(checkpoint->nHeight) + 1);
                if (start > tip->nHeight) throw std::runtime_error("No archived certificate within search budget");
                const int maximum = int(std::min<int64_t>(tip->nHeight, start + int64_t(llmq::MAX_PROOF_HEADERS)));
                const CBlockIndex* target_guard{nullptr};
                const CBlockIndex* target{nullptr};
                llmq::QuorumProofBuilder builder(*ctx.quorum_block_processor, *ctx.qman, tip, chainman, reader);
                std::optional<llmq::QuorumProofChain> proof;
                // At a quorum-mining boundary, the signing offset can select a
                // retired quorum absent from the checkpoint root. Height is a
                // minimum, so try later certificates within the original budget.
                for (int64_t next = start; next <= maximum;) {
                    target_guard = nullptr;
                    std::optional<chainlock::CoinbaseChainLock> target_chainlock;
                    if (minimum == 0) {
                        target_chainlock = reader.Read(tip->nHeight);
                    } else {
                        target_chainlock = reader.Find(int(next), maximum);
                    }
                    chainlock::ChainLockSig target_signature;
                    if (target_chainlock) {
                        target_signature = target_chainlock->Signed();
                        target_guard = target_chainlock->carrier;
                    }
                    // Platform can already reference a tip ChainLock before another
                    // block embeds it. The existing manager supplies the same final
                    // certificate; historical handoffs still come from disk.
                    if (minimum == 0 || !target_chainlock) {
                        const auto live = CHECK_NONFATAL(node.chainlocks)->GetBestChainLock();
                        const auto* live_index = tip->GetAncestor(live.getHeight());
                        if (live_index && live_index->GetBlockHash() == live.getBlockHash() &&
                            live.getHeight() >= next && live.getHeight() > target_signature.getHeight() &&
                            (minimum == 0 || live.getHeight() <= maximum)) {
                            target_signature = live;
                            target_guard = live_index;
                        }
                    }
                    if (!target_guard || target_signature.getHeight() <= checkpoint->nHeight)
                        throw std::runtime_error("No archived certificate within search budget");
                    target = tip->GetAncestor(target_signature.getHeight());
                    proof = builder.Build(checkpoint, target_signature);
                    if (proof || minimum == 0) break;
                    next = int64_t(target_signature.getHeight()) + 1;
                }
                if (!proof) throw std::runtime_error("No bridge to snapshot within search budget");
                // Build() already verified the proof and matched the result against the
                // chain; the target is exactly the state at the target block.
                const auto state = llmq::QuorumProofBuilder::StateAt(target);
                std::vector<llmq::ProofProjection> records;
                if (!quorumText.empty()) {
                    const auto hash = uint256S(quorumText);
                    const auto commitments = builder.ActiveCommitments(target);
                    std::vector<uint256> leaves;
                    leaves.reserve(commitments.size());
                    for (const auto& commitment : commitments)
                        leaves.push_back(SerializeHash(commitment));
                    bool found = false;
                    for (size_t i = 0; i < commitments.size(); ++i) {
                        const auto& commitment = commitments[i];
                        if (commitment.quorumHash != hash || int(commitment.llmqType) != type) continue;
                        CDataStream raw(SER_NETWORK, PROTOCOL_VERSION);
                        raw << commitment;
                        records.push_back({0, {UCharCast(raw.data()), UCharCast(raw.data()) + raw.size()}, llmq::ProofMerklePath::Build(leaves, i)});
                        found = true;
                        break;
                    }
                    if (!found) throw std::runtime_error("Requested quorum is not in the target root");
                }
                if (nodeCount > 0) {
                    const auto [sml, leaves] = llmq::MasternodeLeavesAt(*CHECK_NONFATAL(node.dmnman), target);
                    int included = 0;
                    for (size_t i = 0; i < sml->mnList.size() && included < nodeCount; ++i) {
                        const auto& entry = *sml->mnList[i];
                        if (!entry.isValid || entry.confirmedHash.IsNull() || entry.nType != MnType::Evo) continue;
                        // Exactly CalcHash's serialization, without the network-only version prefix.
                        CDataStream raw(SER_GETHASH, CLIENT_VERSION);
                        raw << entry;
                        records.push_back({1, {UCharCast(raw.data()), UCharCast(raw.data()) + raw.size()}, llmq::ProofMerklePath::Build(leaves, i)});
                        ++included;
                    }
                    if (included == 0) throw std::runtime_error("No eligible EvoNode records at target");
                }
                {
                    LOCK(cs_main);
                    if (!chainman.ActiveChain().Contains(target_guard))
                        throw std::runtime_error("Chain changed during proof construction; retry");
                }
                UniValue result(UniValue::VOBJ);
                result.pushKV("proof_hex", HexStr(proof->Encode()));
                result.pushKV("bootstrap_hex",
                              records.empty() ? "" : HexStr(llmq::EncodeBootstrap(*proof, state, records)));
                result.pushKV("target", state.ToJson());
                return result;
            } catch (const std::exception& e) {
                throw JSONRPCError(RPC_MISC_ERROR, e.what());
            }
        }};
}

static RPCHelpMan verifyquorumproofchain()
{
    return RPCHelpMan{"verifyquorumproofchain",
        "Verify DASHNC02 against an independently trusted full snapshot. Never obtains trust roots from proof data.\n",
        {
            {"checkpoint", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Trusted snapshot", {
                {"network", RPCArg::Type::NUM, RPCArg::Optional::NO, "0 mainnet, 1 testnet"},
                {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "Snapshot height"},
                {"block_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Snapshot hash"},
                {"masternode_root", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Masternode root"},
                {"quorum_root", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Quorum root"},
            }},
            {"proof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "DASHNC02 proof"},
            {"minimum_height", RPCArg::Type::NUM, RPCArg::Default{0}, "Caller freshness policy"},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::BOOL, "valid", "Whether verification succeeded"},
            {RPCResult::Type::OBJ, "target", true, "Authenticated target", {{RPCResult::Type::ELISION, "", ""}}},
            {RPCResult::Type::STR, "error", true, "Verification error"},
        }},
        RPCExamples{HelpExampleCli("verifyquorumproofchain", "'{\"network\":0,\"height\":1987776,\"block_hash\":\"<hash>\",\"masternode_root\":\"<hash>\",\"quorum_root\":\"<hash>\"}' \"<proof_hex>\"")},
        [&](const RPCHelpMan&, const JSONRPCRequest& request) -> UniValue {
            llmq::ProofState trusted;
            std::string text;
            uint32_t minimum{0};
            try {
                trusted = llmq::ProofState::FromJson(request.params[0]);
                text = request.params[1].get_str();
                minimum = request.params[2].isNull() ? 0 : request.params[2].getInt<uint32_t>();
            } catch (const std::exception& e) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, e.what());
            }
            if (text.size() > llmq::MAX_PROOF_BYTES * 2 || !IsHex(text)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Proof hex size/encoding");
            }

            UniValue result(UniValue::VOBJ);
            try {
                auto proof = llmq::QuorumProofChain::Decode(ParseHex(text));
                auto target = proof.Verify(trusted);
                if (target.height < minimum) throw std::runtime_error("Stale proof target");
                result.pushKV("valid", true);
                result.pushKV("target", target.ToJson());
            } catch (const std::exception& e) {
                result.pushKV("valid", false);
                result.pushKV("error", e.what());
            }
            return result;
        }};
}

void RegisterQuorumsRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"evo", &quorum_help},
        {"evo", &quorum_list},
        {"evo", &quorum_list_extended},
        {"evo", &quorum_info},
        {"evo", &quorum_dkginfo},
        {"evo", &quorum_dkgstatus},
        {"evo", &quorum_memberof},
        {"evo", &quorum_sign},
        {"evo", &quorum_platformsign},
        {"evo", &quorum_verify},
        {"evo", &quorum_hasrecsig},
        {"evo", &quorum_getrecsig},
        {"evo", &quorum_isconflicting},
        {"evo", &quorum_selectquorum},
        {"evo", &quorum_dkgsimerror},
        {"evo", &quorum_getdata},
        {"evo", &quorum_rotationinfo},
        {"evo", &submitchainlock},
        {"evo", &verifychainlock},
        {"evo", &verifyislock},
        {"evo", &getchainlockbyheight},
        {"evo", &getquorumproofchain},
        {"evo", &verifyquorumproofchain},
    };
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
