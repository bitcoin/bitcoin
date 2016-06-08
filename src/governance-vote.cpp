// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "masternode.h"
#include "governance.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>


CGovernanceVote::CGovernanceVote()
{
    vinMasternode = CTxIn();
    nParentHash = uint256();
    nVoteSignal = VOTE_SIGNAL_NONE;
    nVoteOutcome = VOTE_OUTCOME_NONE;
    nTime = 0;
    fValid = true;
    fSynced = false;
    vchSig.clear();
}

CGovernanceVote::CGovernanceVote(CTxIn vinMasternodeIn, uint256 nParentHashIn, int nVoteSignalIn, int nVoteOutcomeIn)
{
    vinMasternode = vinMasternodeIn;
    nParentHash = nParentHashIn;
    nVoteSignal = nVoteSignalIn;
    nVoteOutcome = nVoteOutcomeIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

void CGovernanceVote::Relay()
{
    CInv inv(MSG_GOVERNANCE_VOTE, GetHash());
    RelayInv(inv, MSG_GOVERNANCE_PEER_PROTO_VERSION);
}

bool CGovernanceVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

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

bool CGovernanceVote::IsValid(bool fSignatureCheck)
{
    if(nTime > GetTime() + (60*60)){
        LogPrint("mngovernance", "CGovernanceVote::IsValid() - vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    // support up to 50 actions (implemented in sentinel)
    if(nVoteSignal > 50)
    {
        LogPrint("mngovernance", "CGovernanceVote::IsValid() - Client attempted to vote on invalid action(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    // 0=none, 1=yes, 2=no, 3=abstain. Beyond that reject votes
    if(nVoteOutcome > 3)
    {
        LogPrint("mngovernance", "CGovernanceVote::IsValid() - Client attempted to vote on invalid outcome(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;   
    }

    CMasternode* pmn = mnodeman.Find(vinMasternode);
    if(pmn == NULL)
    {
        LogPrint("mngovernance", "CGovernanceVote::IsValid() - Unknown Masternode - %s\n", vinMasternode.ToString());
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string errorMessage;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CGovernanceVote::IsValid() - Verify message failed - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}