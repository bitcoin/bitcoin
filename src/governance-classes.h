// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_CLASSES_H
#define GOVERNANCE_CLASSES_H

//#define ENABLE_DASH_DEBUG

#include "base58.h"
#include "governance.h"
#include "key.h"
#include "script/standard.h"
#include "util.h"

#include <boost/shared_ptr.hpp>

class CSuperblock;
class CGovernanceTrigger;
class CGovernanceTriggerManager;
class CSuperblockManager;

static const int TRIGGER_UNKNOWN            = -1;
static const int TRIGGER_SUPERBLOCK         = 1000;

typedef boost::shared_ptr<CSuperblock> CSuperblock_sptr;

// DECLARE GLOBAL VARIABLES FOR GOVERNANCE CLASSES
extern CGovernanceTriggerManager triggerman;

// SPLIT A STRING UP - USED FOR SUPERBLOCK PAYMENTS
std::vector<std::string> SplitBy(std::string strCommand, std::string strDelimit);

/**
*   Trigger Mananger
*
*   - Track governance objects which are triggers
*   - After triggers are activated and executed, they can be removed
*/

class CGovernanceTriggerManager
{
    friend class CSuperblockManager;
    friend class CGovernanceManager;

private:
    typedef std::map<uint256, CSuperblock_sptr> trigger_m_t;
    typedef trigger_m_t::iterator trigger_m_it;
    typedef trigger_m_t::const_iterator trigger_m_cit;

    trigger_m_t mapTrigger;

    std::vector<CSuperblock_sptr> GetActiveTriggers();
    bool AddNewTrigger(uint256 nHash);
    void CleanAndRemove();

public:
    CGovernanceTriggerManager() : mapTrigger() {}
};

/**
*   Superblock Manager
*
*   Class for querying superblock information
*/

class CSuperblockManager
{
private:
    static bool GetBestSuperblock(CSuperblock_sptr& pSuperblockRet, int nBlockHeight);

public:

    static bool IsSuperblockTriggered(int nBlockHeight);

    static void CreateSuperblock(CMutableTransaction& txNewRet, int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet);

    static std::string GetRequiredPaymentsString(int nBlockHeight);
    static bool IsValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward);
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

    CGovernancePayment()
        :fValid(false),
         script(),
         nAmount(0)
    {}

    CGovernancePayment(CBitcoinAddress addrIn, CAmount nAmountIn)
        :fValid(false),
         script(),
         nAmount(0)
    {
        try
        {
            CTxDestination dest = addrIn.Get();
            script = GetScriptForDestination(dest);
            nAmount = nAmountIn;
            fValid = true;
        }
        catch(std::exception& e)
        {
            LogPrintf("CGovernancePayment Payment not valid: addrIn = %s, nAmountIn = %d, what = %s\n",
                     addrIn.ToString(), nAmountIn, e.what());
        }
        catch(...)
        {
            LogPrintf("CGovernancePayment Payment not valid: addrIn = %s, nAmountIn = %d\n",
                      addrIn.ToString(), nAmountIn);
        }
    }

    bool IsValid() { return fValid; }
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

    int nEpochStart;
    int nStatus;
    std::vector<CGovernancePayment> vecPayments;

    void ParsePaymentSchedule(std::string& strPaymentAddresses, std::string& strPaymentAmounts);

public:

    CSuperblock();
    CSuperblock(uint256& nHash);

    static bool IsValidBlockHeight(int nBlockHeight);
    static CAmount GetPaymentsLimit(int nBlockHeight);

    int GetStatus() { return nStatus; }
    void SetStatus(int nStatusIn) { nStatus = nStatusIn; }

    // IS THIS TRIGGER ALREADY EXECUTED?
    bool IsExecuted() { return nStatus == SEEN_OBJECT_EXECUTED; }
    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted() { nStatus = SEEN_OBJECT_EXECUTED; }

    CGovernanceObject* GetGovernanceObject()
    {
        AssertLockHeld(governance.cs);
        CGovernanceObject* pObj = governance.FindGovernanceObject(nGovObjHash);
        return pObj;
    }

    int GetBlockStart()
    {
        /* // 12.1 TRIGGER EXECUTION */
        /* // NOTE : Is this over complicated? */

        /* //int nRet = 0; */
        /* int nTipEpoch = 0; */
        /* int nTipBlock = chainActive.Tip()->nHeight+1; */

        /* // GET TIP EPOCK / BLOCK */

        /* // typically it should be more than the current time */
        /* int nDiff = nEpochStart - nTipEpoch; */
        /* int nBlockDiff = nDiff / (2.6*60); */

        /* // calculate predicted block height */
        /* int nMod = (nTipBlock + nBlockDiff) % Params().GetConsensus().nSuperblockCycle; */

        /* return (nTipBlock + nBlockDiff)-nMod; */
        return nEpochStart;
    }

    int CountPayments() { return (int)vecPayments.size(); }
    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet);
    CAmount GetPaymentsTotalAmount();

    bool IsValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward);
};

#endif
