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


CBudgetVote::CBudgetVote()
{
    vin = CTxIn();
    nProposalHash = uint256();
    nVote = VOTE_ABSTAIN;
    nTime = 0;
    fValid = true;
    fSynced = false;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, uint256 nProposalHashIn, int nVoteIn)
{
    vin = vinIn;
    nProposalHash = nProposalHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

void CBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

bool CBudgetVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CBudgetVote::IsValid(bool fSignatureCheck)
{
    if(nTime > GetTime() + (60*60)){
        LogPrint("mnbudget", "CBudgetVote::IsValid() - vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrint("mnbudget", "CBudgetVote::IsValid() - Unknown Masternode - %s\n", vin.ToString());
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::IsValid() - Verify message failed - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}