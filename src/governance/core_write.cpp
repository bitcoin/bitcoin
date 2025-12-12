// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/common.h>
#include <governance/governance.h>

#include <rpc/util.h>
#include <util/check.h>

#include <univalue.h>

RPCResult CGovernanceManager::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Count of governance objects and votes",
    {
        {RPCResult::Type::NUM, "objects_total", "Total number of all governance objects"},
        {RPCResult::Type::NUM, "proposals", "Number of governance proposals"},
        {RPCResult::Type::NUM, "triggers", "Number of triggers"},
        {RPCResult::Type::NUM, "other", "Total number of unknown governance objects"},
        {RPCResult::Type::NUM, "erased", "Number of removed (expired) objects"},
        {RPCResult::Type::NUM, "votes", "Total number of votes"},
    }};
}

UniValue CGovernanceManager::ToJson() const
{
    LOCK(cs_store);

    int nProposalCount = 0;
    int nTriggerCount = 0;
    int nOtherCount = 0;

    for (const auto& [_, govobj] : mapObjects) {
        switch (Assert(govobj)->GetObjectType()) {
        case GovernanceObject::PROPOSAL:
            nProposalCount++;
            break;
        case GovernanceObject::TRIGGER:
            nTriggerCount++;
            break;
        default:
            nOtherCount++;
            break;
        }
    }

    UniValue jsonObj(UniValue::VOBJ);
    jsonObj.pushKV("objects_total", mapObjects.size());
    jsonObj.pushKV("proposals", nProposalCount);
    jsonObj.pushKV("triggers", nTriggerCount);
    jsonObj.pushKV("other", nOtherCount);
    jsonObj.pushKV("erased", mapErasedGovernanceObjects.size());
    jsonObj.pushKV("votes", cmapVoteToObject.GetSize());
    return jsonObj;
}

RPCResult CGovernanceObject::GetInnerJsonHelp(const std::string& key, bool optional)
{
    return Governance::Object::GetJsonHelp(key, optional);
}

UniValue CGovernanceObject::GetInnerJson() const
{
    return m_obj.ToJson();
}

UniValue CGovernanceObject::GetVotesJson(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t signal) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("AbsoluteYesCount", GetAbsoluteYesCount(tip_mn_list, signal));
    obj.pushKV("YesCount", GetYesCount(tip_mn_list, signal));
    obj.pushKV("NoCount", GetNoCount(tip_mn_list, signal));
    obj.pushKV("AbstainCount", GetAbstainCount(tip_mn_list, signal));
    return obj;
}

namespace Governance {
RPCResult Object::GetJsonHelp(const std::string& key, bool optional)
{
    return {RPCResult::Type::OBJ, key, optional, key.empty() ? "" : "Object info",
    {
        {RPCResult::Type::STR_HEX, "objectHash", "Hash of proposal object"},
        {RPCResult::Type::STR_HEX, "parentHash", "Hash of the parent object (root node has a hash of 0)"},
        GetRpcResult("collateralHash"),
        {RPCResult::Type::NUM, "createdAt", "Proposal creation timestamp"},
        {RPCResult::Type::NUM, "revision", "Proposal revision number"},
        {RPCResult::Type::OBJ, "data", "", {
            // Fields emitted through GetDataAsPlainString(), read by CProposalValidator
            {RPCResult::Type::STR, "end_epoch", /*optional=*/true, "Proposal end timestamp"},
            {RPCResult::Type::STR, "name", /*optional=*/true, "Proposal name"},
            {RPCResult::Type::STR, "payment_address", /*optional=*/true, "Proposal payment address"},
            {RPCResult::Type::STR, "payment_amount", /*optional=*/true, "Proposal payment amount"},
            {RPCResult::Type::STR, "start_epoch", /*optional=*/true, "Proposal start timestamp"},
            {RPCResult::Type::STR, "type", /*optional=*/true, "Object type"},
            {RPCResult::Type::STR, "url", /*optional=*/true, "Proposal URL"},
            // Failure case for GetDataAsPlainString()
            {RPCResult::Type::STR, "plain", /*optional=*/true, "Governance object data as string"},
            // Always emitted by ToJson()
            {RPCResult::Type::STR_HEX, "hex", "Governance object data as hex"},
        }},
    }};
}

UniValue Object::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("objectHash", GetHash().ToString());
    obj.pushKV("parentHash", hashParent.ToString());
    obj.pushKV("collateralHash", collateralHash.ToString());
    obj.pushKV("createdAt", time);
    obj.pushKV("revision", revision);
    UniValue data;
    if (!data.read(GetDataAsPlainString())) {
        data.clear();
        data.setObject();
        data.pushKV("plain", GetDataAsPlainString());
    }
    data.pushKV("hex", GetDataAsHexString());
    obj.pushKV("data", data);
    return obj;
}
} // namespace Governance
