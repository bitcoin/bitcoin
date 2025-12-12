// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/common.h>
#include <governance/governance.h>

#include <util/check.h>

#include <univalue.h>

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

UniValue CGovernanceObject::ToJson() const
{
    return m_obj.ToJson();
}

namespace Governance {
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
