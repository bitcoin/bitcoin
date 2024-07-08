// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
#define SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
#include <consensus/amount.h>
#include <governance/governance.h>
#include <script/script.h>
#include <threadsafety.h>
#include <uint256.h>
#include <addresstype.h>
#include <kernel/cs_main.h>

class CTxOut;
class CTransaction;

class CSuperblock;
class CSuperblockManager;

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
    static bool GetBestSuperblock(CSuperblock_sptr& pSuperblockRet, int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(governance->cs);

public:
    static bool IsSuperblockTriggered(int nBlockHeight);

    static bool GetSuperblockPayments(int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet);
    static void ExecuteBestSuperblock(int nBlockHeight);

    static bool IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward);
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
    explicit CSuperblock(uint256& nHash);

    static bool IsValidBlockHeight(int nBlockHeight);
    static void GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet);
    static CAmount GetPaymentsLimit(int nBlockHeight);

    SeenObjectStatus GetStatus() const { return nStatus; }
    void SetStatus(SeenObjectStatus nStatusIn) { nStatus = nStatusIn; }

    std::string GetHexStrData() const;

    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted() { nStatus = SeenObjectStatus::Executed; }

    CGovernanceObject* GetGovernanceObject() EXCLUSIVE_LOCKS_REQUIRED(governance->cs);

    int GetBlockHeight() const
    {
        return nBlockHeight;
    }

    int CountPayments() const { return (int)vecPayments.size(); }
    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet);
    CAmount GetPaymentsTotalAmount();

    bool IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward);
    bool IsExpired() const;

    std::vector<uint256> GetProposalHashes() const;
};

#endif // SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
