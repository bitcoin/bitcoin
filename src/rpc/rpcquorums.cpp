// Copyright (c) 2017-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <index/txindex.h>
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

namespace llmq {
extern const std::string CLSIG_REQUESTID_PREFIX;
}

static void quorum_list_help()
{
    throw std::runtime_error(
            "quorum list ( count )\n"
            "List of on-chain quorums\n"
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

static UniValue quorum_list(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        quorum_list_help();

    int count = -1;
    if (!request.params[1].isNull()) {
        count = ParseInt32V(request.params[1], "count");
        if (count < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count can't be negative");
        }
    }

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pindexTip;
    {
        LOCK(cs_main);
        pindexTip = chainActive.Tip();
    }

    for (auto& type : llmq::CLLMQUtils::GetEnabledQuorumTypes(pindexTip)) {
        const auto& llmq_params = llmq::GetLLMQParams(type);
        UniValue v(UniValue::VARR);

        auto quorums = llmq::quorumManager->ScanQuorums(type, pindexTip, count > -1 ? count : llmq_params.signingActiveQuorumCount);
        for (auto& q : quorums) {
            v.push_back(q->qc->quorumHash.ToString());
        }

        ret.pushKV(llmq_params.name, v);
    }


    return ret;
}

static void quorum_info_help()
{
    throw std::runtime_error(
            "quorum info llmqType \"quorumHash\" ( includeSkShare )\n"
            "Return information about a quorum\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"quorumHash\"          (string, required) Block hash of quorum.\n"
            "3. includeSkShare        (boolean, optional) Include secret key share in output.\n"
    );
}

static UniValue BuildQuorumInfo(const llmq::CQuorumCPtr& quorum, bool includeMembers, bool includeSkShare)
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

static UniValue quorum_info(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 3 && request.params.size() != 4))
        quorum_info_help();

    Consensus::LLMQType llmqType = (Consensus::LLMQType)ParseInt32V(request.params[1], "llmqType");
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 quorumHash = ParseHashV(request.params[2], "quorumHash");
    bool includeSkShare = false;
    if (!request.params[3].isNull()) {
        includeSkShare = ParseBoolV(request.params[3], "includeSkShare");
    }

    auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
    if (!quorum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
    }

    return BuildQuorumInfo(quorum, true, includeSkShare);
}

static void quorum_dkgstatus_help()
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

static UniValue quorum_dkgstatus(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 1 || request.params.size() > 2)) {
        quorum_dkgstatus_help();
    }

    int detailLevel = 0;
    if (!request.params[1].isNull()) {
        detailLevel = ParseInt32V(request.params[1], "detail_level");
        if (detailLevel < 0 || detailLevel > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid detail_level");
        }
    }

    llmq::CDKGDebugStatus status;
    llmq::quorumDKGDebugManager->GetLocalDebugStatus(status);

    auto ret = status.ToJson(detailLevel);

    CBlockIndex* pindexTip;
    {
        LOCK(cs_main);
        pindexTip = chainActive.Tip();
    }
    int tipHeight = pindexTip->nHeight;

    auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    UniValue minableCommitments(UniValue::VOBJ);
    UniValue quorumConnections(UniValue::VOBJ);
    for (const auto& type : llmq::CLLMQUtils::GetEnabledQuorumTypes(pindexTip)) {
        const auto& llmq_params = llmq::GetLLMQParams(type);

        if (fMasternodeMode) {
            const CBlockIndex* pindexQuorum;
            {
                LOCK(cs_main);
                pindexQuorum = chainActive[tipHeight - (tipHeight % llmq_params.dkgInterval)];
            }
            auto allConnections = llmq::CLLMQUtils::GetQuorumConnections(llmq_params.type, pindexQuorum, proTxHash, false);
            auto outboundConnections = llmq::CLLMQUtils::GetQuorumConnections(llmq_params.type, pindexQuorum, proTxHash, true);
            std::map<uint256, CAddress> foundConnections;
            g_connman->ForEachNode([&](const CNode* pnode) {
                auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
                if (!verifiedProRegTxHash.IsNull() && allConnections.count(verifiedProRegTxHash)) {
                    foundConnections.emplace(verifiedProRegTxHash, pnode->addr);
                }
            });
            UniValue arr(UniValue::VARR);
            for (auto& ec : allConnections) {
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("proTxHash", ec.ToString());
                if (foundConnections.count(ec)) {
                    obj.pushKV("connected", true);
                    obj.pushKV("address", foundConnections[ec].ToString(false));
                } else {
                    obj.pushKV("connected", false);
                }
                obj.pushKV("outbound", outboundConnections.count(ec) != 0);
                arr.push_back(obj);
            }
            quorumConnections.pushKV(llmq_params.name, arr);
        }

        LOCK(cs_main);
        llmq::CFinalCommitment fqc;
        if (llmq::quorumBlockProcessor->GetMineableCommitment(llmq_params.type,
                                                              tipHeight, fqc)) {
            UniValue obj(UniValue::VOBJ);
            fqc.ToJson(obj);
            minableCommitments.pushKV(llmq_params.name, obj);
        }
    }

    ret.pushKV("minableCommitments", minableCommitments);
    ret.pushKV("quorumConnections", quorumConnections);

    return ret;
}

static void quorum_memberof_help()
{
    throw std::runtime_error(
            "quorum memberof \"proTxHash\" (quorumCount)\n"
            "Checks which quorums the given masternode is a member of.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"                (string, required) ProTxHash of the masternode.\n"
            "2. scanQuorumsCount           (number, optional) Number of quorums to scan for. If not specified,\n"
            "                              the active quorum count for each specific quorum type is used."
    );
}

static UniValue quorum_memberof(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 2 || request.params.size() > 3)) {
        quorum_memberof_help();
    }

    uint256 protxHash = ParseHashV(request.params[1], "proTxHash");
    int scanQuorumsCount = -1;
    if (!request.params[2].isNull()) {
        scanQuorumsCount = ParseInt32V(request.params[2], "scanQuorumsCount");
        if (scanQuorumsCount <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid scanQuorumsCount parameter");
        }
    }

    const CBlockIndex* pindexTip;
    {
        LOCK(cs_main);
        pindexTip = chainActive.Tip();
    }

    auto mnList = deterministicMNManager->GetListForBlock(pindexTip);
    auto dmn = mnList.GetMN(protxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "masternode not found");
    }

    UniValue result(UniValue::VARR);

    for (const auto& type : llmq::CLLMQUtils::GetEnabledQuorumTypes(pindexTip)) {
        const auto& llmq_params = llmq::GetLLMQParams(type);
        size_t count = llmq_params.signingActiveQuorumCount;
        if (scanQuorumsCount != -1) {
            count = (size_t)scanQuorumsCount;
        }
        auto quorums = llmq::quorumManager->ScanQuorums(llmq_params.type, count);
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

static void quorum_sign_help()
{
    throw std::runtime_error(
            "quorum sign llmqType \"id\" \"msgHash\" ( \"quorumHash\" submit )\n"
            "Threshold-sign a message\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
            "4. \"quorumHash\"          (string, optional) The quorum identifier.\n"
            "5. submit                (bool, optional, default=true) Submits the signature share to the network if this is true."
        "                             Returns an object containing the signature share if this is false.\n"
    );
}

static void quorum_verify_help()
{
    throw std::runtime_error(
            "quorum verify llmqType \"id\" \"msgHash\" \"signature\" ( \"quorumHash\" signHeight )\n"
            "Test if a quorum signature is valid for a request id and a message hash\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
            "4. \"signature\"           (string, required) Quorum signature to verify.\n"
            "5. \"quorumHash\"          (string, optional) The quorum identifier.\n"
            "                             Set to \"\" if you want to specify signHeight instead.\n"
            "6. signHeight                (int, optional) The height at which the message was signed.\n"
            "                             Only works when quorumHash is \"\".\n"
    );
}

static void quorum_hasrecsig_help()
{
    throw std::runtime_error(
            "quorum hasrecsig llmqType \"id\" \"msgHash\"\n"
            "Test if a valid recovered signature is present\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

static void quorum_getrecsig_help()
{
    throw std::runtime_error(
            "quorum getrecsig llmqType \"id\" \"msgHash\"\n"
            "Get a recovered signature\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

static void quorum_isconflicting_help()
{
    throw std::runtime_error(
            "quorum isconflicting llmqType \"id\" \"msgHash\"\n"
            "Test if a conflict exists\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
            "3. \"msgHash\"             (string, required) Message hash.\n"
    );
}

static UniValue quorum_sigs_cmd(const JSONRPCRequest& request)
{
    auto cmd = request.params[0].get_str();
    if (request.fHelp || (request.params.size() != 4)) {
        if (cmd == "sign") {
            if ((request.params.size() < 4) || (request.params.size() > 6)) {
                quorum_sign_help();
            }
        } else if (cmd == "verify") {
            if (request.params.size() < 5 || request.params.size() > 7) {
                quorum_verify_help();
            }
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
        uint256 quorumHash;
        if (!request.params[4].isNull() && !request.params[4].get_str().empty()) {
            quorumHash = ParseHashV(request.params[4], "quorumHash");
        }
        bool fSubmit{true};
        if (!request.params[5].isNull()) {
            fSubmit = ParseBoolV(request.params[5], "submit");
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
    } else if (cmd == "verify") {
        CBLSSignature sig;
        if (!sig.SetHexStr(request.params[4].get_str())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
        }

        if (request.params[5].isNull() || (request.params[5].get_str().empty() && !request.params[6].isNull())) {
            int signHeight{-1};
            if (!request.params[6].isNull()) {
                signHeight = ParseInt32V(request.params[6], "signHeight");
            }
            // First check against the current active set, if it fails check against the last active set
            int signOffset{llmq::GetLLMQParams(llmqType).dkgInterval};
            return llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, msgHash, sig, 0) ||
                   llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, msgHash, sig, signOffset);
        } else {
            uint256 quorumHash = ParseHashV(request.params[5], "quorumHash");
            llmq::CQuorumCPtr quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);

            if (!quorum) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "quorum not found");
            }

            uint256 signHash = llmq::CLLMQUtils::BuildSignHash(llmqType, quorum->qc->quorumHash, id, msgHash);
            return sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash);
        }
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

static void quorum_selectquorum_help()
{
    throw std::runtime_error(
            "quorum selectquorum llmqType \"id\"\n"
            "Returns the quorum that would/should sign a request\n"
            "\nArguments:\n"
            "1. llmqType              (int, required) LLMQ type.\n"
            "2. \"id\"                  (string, required) Request id.\n"
    );
}

static UniValue quorum_selectquorum(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3) {
        quorum_selectquorum_help();
    }

    Consensus::LLMQType llmqType = (Consensus::LLMQType)ParseInt32V(request.params[1], "llmqType");
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid LLMQ type");
    }

    uint256 id = ParseHashV(request.params[2], "id");

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
}

static void quorum_dkgsimerror_help()
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

static UniValue quorum_dkgsimerror(const JSONRPCRequest& request)
{
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

static void quorum_getdata_help()
{
    throw std::runtime_error(
        "quorum getdata nodeId llmqType \"quorumHash\" dataMask ( \"proTxHash\" )\n"
        "Send a QGETDATA message to the specified peer.\n"
        "\nArguments:\n"
        "1. nodeId          (integer, required) The internal nodeId of the peer to request quorum data from.\n"
        "2. llmqType        (integer, required) The quorum type related to the quorum data being requested.\n"
        "3. \"quorumHash\"    (string, required) The quorum hash related to the quorum data being requested.\n"
        "4. dataMask        (integer, required) Specify what data to request.\n"
        "                                       Possible values: 1 - Request quorum verification vector\n"
        "                                                        2 - Request encrypted contributions for member defined by \"proTxHash\". \"proTxHash\" must be specified if this option is used.\n"
        "                                                        3 - Request both, 1 and 2\n"
        "5. \"proTxHash\"     (string, optional) The proTxHash the contributions will be requested for. Must be member of the specified LLMQ.\n"
        );
}

static UniValue quorum_getdata(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() < 5 || request.params.size() > 6)) {
        quorum_getdata_help();
    }

    NodeId nodeId = ParseInt64V(request.params[1], "nodeId");
    Consensus::LLMQType llmqType = static_cast<Consensus::LLMQType>(ParseInt32V(request.params[2], "llmqType"));
    uint256 quorumHash = ParseHashV(request.params[3], "quorumHash");
    uint16_t nDataMask = static_cast<uint16_t>(ParseInt32V(request.params[4], "dataMask"));
    uint256 proTxHash;

    // Check if request wants ENCRYPTED_CONTRIBUTIONS data
    if (nDataMask & llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {
        if (!request.params[5].isNull()) {
            proTxHash = ParseHashV(request.params[5], "proTxHash");
            if (proTxHash.IsNull()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "proTxHash invalid");
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "proTxHash missing");
        }
    }

    const CBlockIndex* pQuorumIndex{nullptr};
    {
        LOCK(cs_main);
        pQuorumIndex = LookupBlockIndex(quorumHash);
    }
    return g_connman->ForNode(nodeId, [&](CNode* pNode) {
        return llmq::quorumManager->RequestQuorumData(pNode, llmqType, pQuorumIndex, nDataMask, proTxHash);
    });
}


[[ noreturn ]] static void quorum_help()
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
    );
}

static UniValue _quorum(const JSONRPCRequest& request)
{
    if (request.fHelp && request.params.empty()) {
        quorum_help();
    }

    std::string command;
    if (!request.params[0].isNull()) {
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
    } else if (command == "sign" || command == "verify" || command == "hasrecsig" || command == "getrecsig" || command == "isconflicting") {
        return quorum_sigs_cmd(request);
    } else if (command == "selectquorum") {
        return quorum_selectquorum(request);
    } else if (command == "dkgsimerror") {
        return quorum_dkgsimerror(request);
    } else if (command == "getdata") {
        return quorum_getdata(request);
    } else {
        quorum_help();
    }
}

static void verifychainlock_help()
{
    throw std::runtime_error(
            "verifychainlock \"blockHash\" \"signature\" ( blockHeight )\n"
            "Test if a quorum signature is valid for a ChainLock.\n"
            "\nArguments:\n"
            "1. \"blockHash\"           (string, required) The block hash of the ChainLock.\n"
            "2. \"signature\"           (string, required) The signature of the ChainLock.\n"
            "3. blockHeight           (integer, optional) The height of the ChainLock. There will be an internal lookup of \"blockHash\" if this is not provided.\n"
    );
}

static UniValue verifychainlock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3) {
        verifychainlock_help();
    }

    const uint256 nBlockHash = ParseHashV(request.params[0], "blockHash");

    CBLSSignature chainLockSig;
    if (!chainLockSig.SetHexStr(request.params[1].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    int nBlockHeight;
    if (request.params[2].isNull()) {
        LOCK(cs_main);
        const CBlockIndex* pIndex = LookupBlockIndex(nBlockHash);
        if (pIndex == nullptr) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "blockHash not found");
        }
        nBlockHeight = pIndex->nHeight;
    } else {
        nBlockHeight = ParseInt32V(request.params[2], "blockHeight");
    }

    const auto llmqType = Params().GetConsensus().llmqTypeChainLocks;
    const uint256 nRequestId = ::SerializeHash(std::make_pair(llmq::CLSIG_REQUESTID_PREFIX, nBlockHeight));
    return llmq::CSigningManager::VerifyRecoveredSig(llmqType, nBlockHeight, nRequestId, nBlockHash, chainLockSig);
}

static void verifyislock_help()
{
    throw std::runtime_error(
            "verifyislock \"id\" \"txid\" \"signature\" ( maxHeight )\n"
            "Test if a quorum signature is valid for an InstantSend Lock\n"
            "\nArguments:\n"
            "1. \"id\"                  (string, required) Request id.\n"
            "2. \"txid\"                (string, required) The transaction id.\n"
            "3. \"signature\"           (string, required) The InstantSend Lock signature to verify.\n"
            "4. maxHeight                 (int, optional) The maximum height to search quorums from.\n"
    );
}

static UniValue verifyislock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 3 || request.params.size() > 4) {
        verifyislock_help();
    }

    uint256 id = ParseHashV(request.params[0], "id");
    uint256 txid = ParseHashV(request.params[1], "txid");

    CBLSSignature sig;
    if (!sig.SetHexStr(request.params[2].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature format");
    }

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    CBlockIndex* pindexMined{nullptr};
    {
        LOCK(cs_main);
        CTransactionRef tx;
        uint256 hash_block;
        if (GetTransaction(txid, tx, Params().GetConsensus(), hash_block, true) && !hash_block.IsNull()) {
            pindexMined = LookupBlockIndex(hash_block);
        }
    }

    int maxHeight{-1};
    if (!request.params[3].isNull()) {
        maxHeight = ParseInt32V(request.params[3], "maxHeight");
    }

    int signHeight;
    if (pindexMined == nullptr || pindexMined->nHeight > maxHeight) {
        signHeight = maxHeight;
    } else { // pindexMined->nHeight <= maxHeight
        signHeight = pindexMined->nHeight;
    }

    auto llmqType = Params().GetConsensus().llmqTypeInstantSend;

    // First check against the current active set, if it fails check against the last active set
    int signOffset{llmq::GetLLMQParams(llmqType).dkgInterval};
    return llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, txid, sig, 0) ||
           llmq::quorumSigningManager->VerifyRecoveredSig(llmqType, signHeight, id, txid, sig, signOffset);
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                "quorum",                 &_quorum,                 {}  },
    { "evo",                "verifychainlock",        &verifychainlock,        {"blockHash", "signature", "blockHeight"} },
    { "evo",                "verifyislock",           &verifyislock,           {"id", "txid", "signature", "maxHeight"}  },
};

void RegisterQuorumsRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
