// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
#define SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
#include <amount.h>
#include <governance/governance.h>
#include <script/script.h>
#include <script/standard.h>
#include <util/system.h>
#include <threadsafety.h>
#include <uint256.h>

class CTxOut;
class CTransaction;
class CSuperblock;
class CGovernanceTriggerManager;
class CSuperblockManager;

typedef std::shared_ptr<CSuperblock> CSuperblock_sptr;

// DECLARE GLOBAL VARIABLES FOR GOVERNANCE CLASSES
extern CGovernanceTriggerManager triggerman;

/**
*   Trigger Manager
*
*   - Track governance objects which are triggers
*   - After triggers are activated and executed, they can be removed
*/

class CGovernanceTriggerManager
{
    friend class CSuperblockManager;
    friend class CGovernanceManager;

private:

    std::map<uint256, CSuperblock_sptr> mapTrigger;

    std::vector<CSuperblock_sptr> GetActiveTriggers() EXCLUSIVE_LOCKS_REQUIRED(governance.cs);
    bool AddNewTrigger(uint256 nHash);
    void CleanAndRemove();

public:
    CGovernanceTriggerManager() :
        mapTrigger() {}
};

/**
*   Superblock Manager
*
*   Class for querying superblock information
*/

class CSuperblockManager
{
private:
    static bool GetBestSuperblock(CSuperblock_sptr& pSuperblockRet, int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(governance.cs);

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

    CGovernancePayment() :
        fValid(false),
        script(),
        nAmount(0)
    {
    }
    CGovernancePayment(const CTxDestination& destIn, const CAmount &nAmountIn);

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
    int nStatus;
    std::vector<CGovernancePayment> vecPayments;

    void ParsePaymentSchedule(const std::string& strPaymentAddresses, const std::string& strPaymentAmounts);

public:
    CSuperblock();
    explicit CSuperblock(uint256& nHash);

    static bool IsValidBlockHeight(int nBlockHeight);
    static void GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet);
    static CAmount GetPaymentsLimit(int nBlockHeight);

    int GetStatus() const { return nStatus; }
    void SetStatus(int nStatusIn) { nStatus = nStatusIn; }

    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted() { nStatus = SEEN_OBJECT_EXECUTED; }

    CGovernanceObject* GetGovernanceObject() EXCLUSIVE_LOCKS_REQUIRED(governance.cs);

    int GetBlockHeight() const
    {
        return nBlockHeight;
    }

    int CountPayments() { return (int)vecPayments.size(); }
    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet);
    CAmount GetPaymentsTotalAmount();

    bool IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward);
    bool IsExpired() const;
};

#endif // SYSCOIN_GOVERNANCE_GOVERNANCECLASSES_H
