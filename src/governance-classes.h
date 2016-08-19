// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERANCE_CLASSES_H
#define GOVERANCE_CLASSES_H

//#define ENABLE_DASH_DEBUG

#include "util.h"
#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "init.h"
#include "governance.h"

using namespace std;

#define TRIGGER_UNKNOWN             -1
#define TRIGGER_SUPERBLOCK          1000

class CSuperblock;
class CGovernanceTrigger;
class CGovernanceTriggerManager;
class CSuperblockManager;

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

CAmount ParsePaymentAmount(const std::string& strAmount);

class CGovernanceTriggerManager
{
    friend class CSuperblockManager;
    friend class CGovernanceManager;

public: // Typedefs
    typedef std::map<uint256, CSuperblock_sptr> trigger_m_t;

    typedef trigger_m_t::iterator trigger_m_it;

    typedef trigger_m_t::const_iterator trigger_m_cit;

private:
    trigger_m_t mapTrigger;

public:
    CGovernanceTriggerManager()
        : mapTrigger()
    {}

private:
    std::vector<CSuperblock_sptr> GetActiveTriggers();

    bool AddNewTrigger(uint256 nHash);

    void CleanAndRemove();

};

/**
*   Superblock Manager
*
*   Class for querying superblock information
*/

class CSuperblockManager
{
public:

    /**
     *   Is Valid Superblock Height
     *
     *   - See if this block can be a superblock
     */
    static bool IsValidSuperblockHeight(int nBlockHeight)
    {
        // SUPERBLOCKS CAN HAPPEN ONCE PER DAY
        return ((nBlockHeight % Params().GetConsensus().nSuperblockCycle) == 1);
    }

    static bool IsSuperblockTriggered(int nBlockHeight);
    static void CreateSuperblock(CMutableTransaction& txNew, CAmount nFees, int nBlockHeight);


    static std::string GetRequiredPaymentsString(int nBlockHeight);
    static bool IsValid(const CTransaction& txNew, int nBlockHeight);

private:
    static bool GetBestSuperblock(CSuperblock_sptr& pBlock, int nBlockHeight);

};

/**
*   Governance Object Payment
*
*/

class CGovernancePayment
{
public:
    CScript script;
    CAmount nAmount;
    bool fValid; 

    CGovernancePayment()
    {
        SetNull();
    }

    void SetNull()
    {
        script = CScript();
        nAmount = 0;
        fValid = false;
    }

    bool IsValid()
    {
        return fValid;
    }

    CGovernancePayment(CBitcoinAddress addrIn, CAmount nAmountIn)
    {
        try
        {
            CTxDestination dest = addrIn.Get();
            script = GetScriptForDestination(dest);
            nAmount = nAmountIn;
        } catch(...) {
            SetNull(); //set fValid to false
        }
    }
};


/**
*   Trigger : Superblock
*
*   - Create payments on the network
*/

class CSuperblock : public CGovernanceObject
{

    /*

        object structure: 
        {
            "governance_object_id" : last_id,
            "type" : govtypes.trigger,
            "subtype" : "superblock",
            "superblock_name" : superblock_name,
            "start_epoch" : start_epoch,
            "payment_addresses" : "addr1|addr2|addr3",
            "payment_amounts"   : "amount1|amount2|amount3"
        }
    */

private:
    uint256 nGovObjHash;

    int nEpochStart;
    int nStatus;
    std::vector<CGovernancePayment> vecPayments;

public:

    CSuperblock();

    CSuperblock(uint256& nHash);

    int GetStatus()
    {
        return nStatus;
    }

    void SetStatus(int nStatus_)
    {
        nStatus = nStatus_;
    }

    CGovernanceObject* GetGovernanceObject()  {
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

    // IS THIS TRIGGER ALREADY EXECUTED?
    bool IsExecuted()
    {
        return (nStatus == SEEN_OBJECT_EXECUTED);
    }

    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted()
    {
        nStatus = SEEN_OBJECT_EXECUTED;
    }

    int CountPayments()
    {
        return (int)vecPayments.size();
    }

    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentOut) 
    {
        if((nPaymentIndex<0) || (nPaymentIndex >= (int)vecPayments.size())) {
            return false;
        }

        paymentOut = vecPayments[nPaymentIndex];
        return true;
    }

    bool IsValid(const CTransaction& txNew);

private:
    void ParsePaymentSchedule(std::string& strPaymentAddresses, std::string& strPaymentAmounts);
};

#endif
