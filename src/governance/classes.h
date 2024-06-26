// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_GOVERNANCE_CLASSES_H
#define BITCOIN_GOVERNANCE_CLASSES_H

#include <amount.h>
#include <governance/object.h>
#include <script/script.h>
#include <script/standard.h>
#include <uint256.h>

class CChain;
class CGovernanceManager;
class CSuperblock;
class CSuperblockManager;
class CTxOut;
class CTransaction;

using CSuperblock_sptr = std::shared_ptr<CSuperblock>;

CAmount ParsePaymentAmount(const std::string& strAmount);

/**
*   Superblock Manager
*
*   Class for querying superblock information
*/

class CSuperblockManager
{
private:
    static bool GetBestSuperblock(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet, int nBlockHeight);

public:
    static bool IsSuperblockTriggered(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight);

    static bool GetSuperblockPayments(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet);
    static void ExecuteBestSuperblock(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight);

    static bool IsValid(CGovernanceManager& govman, const CChain& active_chain, const CDeterministicMNList& tip_mn_list, const CTransaction& txNew, int nBlockHeight, CAmount blockReward);
};

/**
*   Governance Object Payment
*
*/

class CGovernancePayment
{
private:
    bool fValid;

public:
    CScript script;
    CAmount nAmount;
    uint256 proposalHash;

    CGovernancePayment() :
        fValid(false),
        script(),
        nAmount(0),
        proposalHash(0)
    {
    }

    CGovernancePayment(const CTxDestination& destIn, CAmount nAmountIn, const uint256& proposalHash);

    bool IsValid() const { return fValid; }
};


/**
*   Trigger : Superblock
*
*   - Create payments on the network
*
*   object structure:
*   {
*       "governance_object_id" : last_id,
*       "type" : govtypes.trigger,
*       "subtype" : "superblock",
*       "superblock_name" : superblock_name,
*       "start_epoch" : start_epoch,
*       "payment_addresses" : "addr1|addr2|addr3",
*       "payment_amounts"   : "amount1|amount2|amount3"
*   }
*/

class CSuperblock : public CGovernanceObject
{
private:
    uint256 nGovObjHash;

    int nBlockHeight;
    SeenObjectStatus nStatus;
    std::vector<CGovernancePayment> vecPayments;

    void ParsePaymentSchedule(const std::string& strPaymentAddresses, const std::string& strPaymentAmounts, const std::string& strProposalHashes);

public:
    CSuperblock();
    CSuperblock(int nBlockHeight, std::vector<CGovernancePayment> vecPayments);
    explicit CSuperblock(CGovernanceManager& govman, uint256& nHash);

    static bool IsValidBlockHeight(int nBlockHeight);
    static void GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet);
    static CAmount GetPaymentsLimit(const CChain& active_chain, int nBlockHeight);

    SeenObjectStatus GetStatus() const { return nStatus; }
    void SetStatus(SeenObjectStatus nStatusIn) { nStatus = nStatusIn; }

    std::string GetHexStrData() const;

    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted() { nStatus = SeenObjectStatus::Executed; }

    CGovernanceObject* GetGovernanceObject(CGovernanceManager& govman);

    int GetBlockHeight() const
    {
        return nBlockHeight;
    }

    int CountPayments() const { return (int)vecPayments.size(); }
    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet);
    CAmount GetPaymentsTotalAmount();

    bool IsValid(CGovernanceManager& govman, const CChain& active_chain, const CTransaction& txNew, int nBlockHeight, CAmount blockReward);
    bool IsExpired(const CGovernanceManager& govman) const;

    std::vector<uint256> GetProposalHashes() const;
};

#endif // BITCOIN_GOVERNANCE_CLASSES_H
