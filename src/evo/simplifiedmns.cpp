// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simplifiedmns.h"
#include "specialtx.h"
#include "deterministicmns.h"

#include "validation.h"
#include "univalue.h"
#include "consensus/merkle.h"

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const CDeterministicMN& dmn) :
    proRegTxHash(dmn.proTxHash),
    service(dmn.pdmnState->addr),
    keyIDOperator(dmn.pdmnState->keyIDOperator),
    keyIDVoting(dmn.pdmnState->keyIDVoting),
    isValid(dmn.pdmnState->nPoSeBanHeight == -1)
{
}

uint256 CSimplifiedMNListEntry::CalcHash() const
{
    CHashWriter hw(SER_GETHASH, CLIENT_VERSION);
    hw << *this;
    return hw.GetHash();
}

std::string CSimplifiedMNListEntry::ToString() const
{
    return strprintf("CSimplifiedMNListEntry(proRegTxHash=%s, service=%s, keyIDOperator=%s, keyIDVoting=%s, isValie=%d)",
        proRegTxHash.ToString(), service.ToString(false), keyIDOperator.ToString(), keyIDVoting.ToString(), isValid);
}

void CSimplifiedMNListEntry::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("proRegTxHash", proRegTxHash.ToString()));
    obj.push_back(Pair("service", service.ToString(false)));
    obj.push_back(Pair("keyIDOperator", keyIDOperator.ToString()));
    obj.push_back(Pair("keyIDVoting", keyIDOperator.ToString()));
    obj.push_back(Pair("isValid", isValid));
}

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList)
{
    mnList.reserve(dmnList.all_count());

    for (const auto& dmn : dmnList.all_range()) {
        mnList.emplace_back(*dmn);
    }

    std::sort(mnList.begin(), mnList.end(), [&](const CSimplifiedMNListEntry& a, const CSimplifiedMNListEntry& b) {
        return a.proRegTxHash.Compare(b.proRegTxHash) < 0;
    });
}

uint256 CSimplifiedMNList::CalcMerkleRoot(bool *pmutated) const
{
    std::vector<uint256> leaves;
    leaves.reserve(mnList.size());
    for (const auto& e : mnList) {
        leaves.emplace_back(e.CalcHash());
    }
    return ComputeMerkleRoot(leaves, pmutated);
}
