// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "base58.h"
#include "clientversion.h"
#include "init.h"
#include "netbase.h"
#include "masternode.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "script/standard.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET

#include "evo/deterministicmns.h"

#include <string>


CMasternode::CMasternode() :
    masternode_info_t{ }
{}

CMasternode::CMasternode(const CMasternode& other) :
    masternode_info_t{other},
    nMixingTxCount(other.nMixingTxCount)
{}

CMasternode::CMasternode(const uint256 &proTxHash, const CDeterministicMNCPtr& dmn) :
    masternode_info_t{ dmn->collateralOutpoint }
{
}

masternode_info_t CMasternode::GetInfo() const
{
    masternode_info_t info{*this};
    info.fInfoValid = true;
    return info;
}

void CMasternode::AddGovernanceVote(uint256 nGovernanceObjectHash)
{
    // Insert a zero value, or not. Then increment the value regardless. This
    // ensures the value is in the map.
    const auto& pair = mapGovernanceObjectsVotedOn.emplace(nGovernanceObjectHash, 0);
    pair.first->second++;
}

void CMasternode::RemoveGovernanceObject(uint256 nGovernanceObjectHash)
{
    // Whether or not the govobj hash exists in the map first is irrelevant.
    mapGovernanceObjectsVotedOn.erase(nGovernanceObjectHash);
}

/**
*   FLAG GOVERNANCE ITEMS AS DIRTY
*
*   - When masternode come and go on the network, we must flag the items they voted on to recalc it's cached flags
*
*/
void CMasternode::FlagGovernanceItemsAsDirty()
{
    for (auto& govObjHashPair : mapGovernanceObjectsVotedOn) {
        mnodeman.AddDirtyGovernanceObjectHash(govObjHashPair.first);
    }
}
