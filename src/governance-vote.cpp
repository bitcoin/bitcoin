// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #include "core_io.h"
// #include "main.h"
// #include "init.h"

//todo: which of these do we need?
// #include "governance.h"
#include "governance-vote.h"
// #include "masternode.h"
// #include "darksend.h"
// #include "masternodeman.h"
// #include "masternode-sync.h"
// #include "util.h"
// #include "addrman.h"
// #include <boost/filesystem.hpp>
// #include <boost/lexical_cast.hpp>

CGovernanceVote::CGovernanceVote()
{
    pParent = NULL;
    nGovernanceType = 0;
    vin = CTxIn();
    nParentHash = uint256();
    nVote = VOTE_ABSTAIN;
    nTime = 0;
    fValid = true;

    fSynced = false;
}

CGovernanceVote::CGovernanceVote(CGovernanceNode& pParentNodeIn, CTxIn vinIn, uint256 nParentHashIn, int nVoteIn)
{
    pParent = &pParentNodeIn;
    nGovernanceType = (int)pParentNodeIn->GetGovernanceType();
    vin = vinIn;
    nParentHash = nParentHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

void CGovernanceVote::Relay()
{
    CInv inv(MSG_GOVERNANCE_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

bool CGovernanceVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nParentHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CGovernanceVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CGovernanceVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CGovernanceVote::IsValid(bool fSignatureCheck, std::string& strReason)
{
    if(nTime > GetTime() + (60*60)){
        strReason = strprintf("vote is too far ahead of current time - %s - nTime %lli - Max Time %lli %d", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    if(!pParent1 && !pParent2)
    {
        strReason = "Invalid pParent (it's null)";
        return false;
    } else if (pParent1) {
        if(nTime < GetValidStartTimestamp() || nTime > GetValidEndTimestamp())
        {
            strReason = strprintf("vote time is out of range - %s - nTime %lli - Min/Max Time %lli, %lli", GetHash().ToString(), nTime, GetValidStartTimestamp(), GetValidEndTimestamp());
            return false;
        }
    }

    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        strReason = "Unknown Masternode " + vin.ToString();
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nParentHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        strReason = strprintf("Verify message failed - Error: %s", errorMessage);
        return false;
    }

    return true;
}

void CGovernanceVote::SetParent(CGovernanceNode* pGovObjectParent)
{
    pParent = pGovObjectParent;
}
