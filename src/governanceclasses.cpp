// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#define ENABLE_SYS_DEBUG

#include <core_io.h>
#include <governanceclasses.h>
#include <init.h>
#include <validation.h>
#include <util/strencodings.h>

#include <boost/algorithm/string.hpp>
#include <univalue.h>

// DECLARE GLOBAL VARIABLES FOR GOVERNANCE CLASSES
CGovernanceTriggerManager triggerman;

// SPLIT UP STRING BY DELIMITER
// http://www.boost.org/doc/libs/1_58_0/doc/html/boost/algorithm/split_idp202406848.html
std::vector<std::string> SplitBy(const std::string& strCommand, const std::string& strDelimit)
{
    std::vector<std::string> vParts;
    boost::split(vParts, strCommand, boost::is_any_of(strDelimit));

    for(int q=0; q<(int)vParts.size(); q++) {
        if(strDelimit.find(vParts[q]) != std::string::npos) {
            vParts.erase(vParts.begin()+q);
            --q;
        }
    }

   return vParts;
}

CAmount ParsePaymentAmount(const std::string& strAmount)
{
    //DBG( std::cout << "ParsePaymentAmount Start: strAmount = " << strAmount << std::endl; );

    CAmount nAmount = 0;
    if (strAmount.empty()) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount is empty";
        throw std::runtime_error(ostr.str());
    }
    if(strAmount.size() > 20) {
        // String is much too long, the functions below impose stricter
        // requirements
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount string too long";
        throw std::runtime_error(ostr.str());
    }
    // Make sure the string makes sense as an amount
    // Note: No spaces allowed
    // Also note: No scientific notation
    size_t pos = strAmount.find_first_not_of("0123456789.");
    if (pos != std::string::npos) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount string contains invalid character";
        throw std::runtime_error(ostr.str());
    }

    pos = strAmount.find(".");
    if (pos == 0) {
        // JSON doesn't allow values to start with a decimal point
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, leading decimal point not allowed";
        throw std::runtime_error(ostr.str());
    }

    // Make sure there's no more than 1 decimal point
    if ((pos != std::string::npos) && (strAmount.find(".", pos+1) != std::string::npos)) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, too many decimal points";
        throw std::runtime_error(ostr.str());
    }

    // Note this code is taken from AmountFromValue in rpcserver.cpp
    // which is used for parsing the amounts in createrawtransaction.
    if (!ParseFixedPoint(strAmount, 8, &nAmount)) {
        nAmount = 0;
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: ParseFixedPoint failed for string: " << strAmount;
        throw std::runtime_error(ostr.str());
    }
    if (!MoneyRange(nAmount)) {
        nAmount = 0;
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, value outside of valid money range";
        throw std::runtime_error(ostr.str());
    }

    //DBG( std::cout << "ParsePaymentAmount Returning true nAmount = " << nAmount << std::endl; );

    return nAmount;
}

/**
*   Add Governance Object
*/

bool CGovernanceTriggerManager::AddNewTrigger(uint256 nHash)
{
    //DBG( std::cout << "CGovernanceTriggerManager::AddNewTrigger: Start" << std::endl; );
    AssertLockHeld(governance.cs);

    // IF WE ALREADY HAVE THIS HASH, RETURN
    if(mapTrigger.count(nHash)) {
        /*DBG(
            std::cout << "CGovernanceTriggerManager::AddNewTrigger: Already have hash"
                 << ", nHash = " << nHash.GetHex()
                 << ", count = " << mapTrigger.count(nHash)
                 << ", mapTrigger.size() = " << mapTrigger.size()
                 << std::endl; );*/
        return false;
    }

    CSuperblock_sptr pSuperblock;
    try  {
        CSuperblock_sptr pSuperblockTmp(new CSuperblock(nHash));
        pSuperblock = pSuperblockTmp;
    }
    catch(std::exception& e) {
        LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::AddNewTrigger -- Error creating superblock: %s\n", e.what());
        return false;
    }
    catch(...) {
        LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::AddNewTrigger: Unknown Error creating superblock\n");
        return false;
    }

    pSuperblock->SetStatus(SEEN_OBJECT_IS_VALID);

    //DBG( std::cout << "CGovernanceTriggerManager::AddNewTrigger: Inserting trigger" << std::endl; );
    mapTrigger.insert(std::make_pair(nHash, pSuperblock));
    

    return true;
}

/**
*
*   Clean And Remove
*
*/

void CGovernanceTriggerManager::CleanAndRemove()
{
    LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- Start\n");
    AssertLockHeld(governance.cs);

    // Remove triggers that are invalid or expired
    LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- mapTrigger.size() = %d\n", mapTrigger.size());

    trigger_m_it it = mapTrigger.begin();
    while(it != mapTrigger.end()) {
        bool remove = false;
        CGovernanceObject* pObj = nullptr;
        CSuperblock_sptr& pSuperblock = it->second;
        if(!pSuperblock) {
            LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- NULL superblock marked for removal\n");
            remove = true;
        } else {
            pObj = governance.FindGovernanceObject(it->first);
            if(!pObj || pObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
                LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- Unknown or non-trigger superblock\n");
                pSuperblock->SetStatus(SEEN_OBJECT_ERROR_INVALID);
            }
            
            LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- superblock status = %d\n", pSuperblock->GetStatus());
            switch(pSuperblock->GetStatus()) {
            case SEEN_OBJECT_ERROR_INVALID:
            case SEEN_OBJECT_UNKNOWN:
                LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- Unknown or invalid trigger found\n");
                remove = true;
                break;
            case SEEN_OBJECT_IS_VALID:
            case SEEN_OBJECT_EXECUTED:
                remove = pSuperblock->IsExpired();
                break;
            default:
                break;
            }
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- %smarked for removal\n", remove ? "" : "NOT ");

        if(remove) {
            /*DBG(
                std::string strDataAsPlainString = "NULL";
                if(pObj) {
                    strDataAsPlainString = pObj->GetDataAsPlainString();
                }
                std::cout << "CGovernanceTriggerManager::CleanAndRemove: Removing object: "
                     << strDataAsPlainString
                     << std::endl;
               );*/
            LogPrint(BCLog::GOBJECT, "CGovernanceTriggerManager::CleanAndRemove -- Removing trigger object\n");
            // mark corresponding object for deletion
            if (pObj) {
                pObj->fCachedDelete = true;
                if (pObj->nDeletionTime == 0) {
                    pObj->nDeletionTime = GetAdjustedTime();
                }
            }
            // delete the trigger
            mapTrigger.erase(it++);
        }
        else  {
            ++it;
        }
    }

    //DBG( std::cout << "CGovernanceTriggerManager::CleanAndRemove: End" << std::endl; );
}

/**
*   Get Active Triggers
*
*   - Look through triggers and scan for active ones
*   - Return the triggers in a list
*/

std::vector<CSuperblock_sptr> CGovernanceTriggerManager::GetActiveTriggers()
{
    AssertLockHeld(governance.cs);
    std::vector<CSuperblock_sptr> vecResults;

   // DBG( std::cout << "GetActiveTriggers: mapTrigger.size() = " << mapTrigger.size() << std::endl; );

    // LOOK AT THESE OBJECTS AND COMPILE A VALID LIST OF TRIGGERS
    for (const auto& pair : mapTrigger) {
        CGovernanceObject* pObj = governance.FindGovernanceObject(pair.first);
        if(pObj) {
            //DBG( std::cout << "GetActiveTriggers: pObj->GetDataAsPlainString() = " << pObj->GetDataAsPlainString() << std::endl; );
            vecResults.push_back(pair.second);
        }
    }

    //DBG( std::cout << "GetActiveTriggers: vecResults.size() = " << vecResults.size() << std::endl; );

    return vecResults;
}

/**
*   Is Superblock Triggered
*
*   - Does this block have a non-executed and activated trigger?
*/

bool CSuperblockManager::IsSuperblockTriggered(int nBlockHeight)
{
    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(governance.cs);
    // GET ALL ACTIVE TRIGGERS
    std::vector<CSuperblock_sptr> vecTriggers = triggerman.GetActiveTriggers();

    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- vecTriggers.size() = %d\n", vecTriggers.size());

   
    for (const auto& pSuperblock : vecTriggers)
    {
        if(!pSuperblock) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }

        CGovernanceObject* pObj = pSuperblock->GetGovernanceObject();

        if(!pObj) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- pObj == NULL, continuing\n");
            continue;
        }

        LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- data = %s\n", pObj->GetDataAsPlainString());

        // note : 12.1 - is epoch calculation correct?

        if(nBlockHeight != pSuperblock->GetBlockHeight()) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- block height doesn't match nBlockHeight = %d, blockStart = %d, continuing\n",
                     nBlockHeight,
                     pSuperblock->GetBlockHeight());
            continue;
        }

        // MAKE SURE THIS TRIGGER IS ACTIVE VIA FUNDING CACHE FLAG

        pObj->UpdateSentinelVariables();

        if(pObj->IsSetCachedFunding()) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- fCacheFunding = true, returning true\n");
            return true;
        }
        else  {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- fCacheFunding = false, continuing\n");
        }
    }

    return false;
}


bool CSuperblockManager::GetBestSuperblock(CSuperblock_sptr& pSuperblockRet, int nBlockHeight)
{
    if(!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    AssertLockHeld(governance.cs);
    std::vector<CSuperblock_sptr> vecTriggers = triggerman.GetActiveTriggers();
    int nYesCount = 0;

    for (const auto& pSuperblock : vecTriggers) {
        if(!pSuperblock) {
            //DBG( std::cout << "GetBestSuperblock Not a superblock, continuing" << std::endl; );
            continue;
        }

        CGovernanceObject* pObj = pSuperblock->GetGovernanceObject();

        if(!pObj) {
            //DBG( std::cout << "GetBestSuperblock pObj is NULL, continuing" << std::endl; );
            continue;
        }

        if(nBlockHeight != pSuperblock->GetBlockHeight()) {
            //DBG( std::cout << "GetBestSuperblock Not the target block, continuing" << std::endl; );
            continue;
        }

        // DO WE HAVE A NEW WINNER?

        int nTempYesCount = pObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
        //DBG( std::cout << "GetBestSuperblock nTempYesCount = " << nTempYesCount << std::endl; );
        if(nTempYesCount > nYesCount) {
            nYesCount = nTempYesCount;
            pSuperblockRet = pSuperblock;
            //DBG( std::cout << "GetBestSuperblock Valid superblock found, pSuperblock set" << std::endl; );
        }
    }

    return nYesCount > 0;
}

/**
*   Create Superblock Payments
*
*   - Create the correct payment structure for a given superblock
*/

void CSuperblockManager::CreateSuperblock(CMutableTransaction& txNewRet, int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet)
{
    //DBG( std::cout << "CSuperblockManager::CreateSuperblock Start" << std::endl; );

    LOCK(governance.cs);

    // GET THE BEST SUPERBLOCK FOR THIS BLOCK HEIGHT

    CSuperblock_sptr pSuperblock;
    if(!CSuperblockManager::GetBestSuperblock(pSuperblock, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CSuperblockManager::CreateSuperblock -- Can't find superblock for height %d\n", nBlockHeight);
        return;
    }

    // make sure it's empty, just in case
    voutSuperblockRet.clear();

    // CONFIGURE SUPERBLOCK OUTPUTS

    // Superblock payments are appended to the end of the coinbase vout vector
   //DBG( std::cout << "CSuperblockManager::CreateSuperblock Number payments: " << pSuperblock->CountPayments() << std::endl; );

    // TODO: How many payments can we add before things blow up?
    //       Consider at least following limits:
    //          - max coinbase tx size
    //          - max "budget" available
    for(int i = 0; i < pSuperblock->CountPayments(); i++) {
        CGovernancePayment payment;
        //DBG( std::cout << "CSuperblockManager::CreateSuperblock i = " << i << std::endl; );
        if(pSuperblock->GetPayment(i, payment)) {
            //DBG( std::cout << "CSuperblockManager::CreateSuperblock Payment found " << std::endl; );
            // SET COINBASE OUTPUT TO SUPERBLOCK SETTING

            CTxOut txout = CTxOut(payment.nAmount, payment.script);
            txNewRet.vout.push_back(txout);
            voutSuperblockRet.push_back(txout);

            // PRINT NICE LOG OUTPUT FOR SUPERBLOCK PAYMENT

            CTxDestination address1;
            ExtractDestination(payment.script, address1);


            LogPrint(BCLog::GOBJECT, "NEW Superblock : output %d (addr %s, amount %d)\n", i, EncodeDestination(address1), payment.nAmount);
        } else {
           //DBG( std::cout << "CSuperblockManager::CreateSuperblock Payment not found " << std::endl; );
        }
    }

    //DBG( std::cout << "CSuperblockManager::CreateSuperblock End" << std::endl; );
}

bool CSuperblockManager::IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward)
{
    // GET BEST SUPERBLOCK, SHOULD MATCH
    LOCK(governance.cs);

    CSuperblock_sptr pSuperblock;
    if(CSuperblockManager::GetBestSuperblock(pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(txNew, nBlockHeight, blockReward);
    }

    return false;
}

void CSuperblockManager::ExecuteBestSuperblock(int nBlockHeight)
{
    LOCK(governance.cs);

    CSuperblock_sptr pSuperblock;
    if(GetBestSuperblock(pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
    }
}

CSuperblock::
CSuperblock()
    : nGovObjHash(),
      nBlockHeight(0),
      nStatus(SEEN_OBJECT_UNKNOWN),
      vecPayments()
{}

CSuperblock::
CSuperblock(uint256& nHash)
    : nGovObjHash(nHash),
      nBlockHeight(0),
      nStatus(SEEN_OBJECT_UNKNOWN),
      vecPayments()
{
    //DBG( std::cout << "CSuperblock Constructor Start" << std::endl; );

    CGovernanceObject* pGovObj = GetGovernanceObject();

    if(!pGovObj) {
        //DBG( std::cout << "CSuperblock Constructor pGovObjIn is NULL, returning" << std::endl; );
        throw std::runtime_error("CSuperblock: Failed to find Governance Object");
    }

    /*DBG( std::cout << "CSuperblock Constructor pGovObj : "
         << pGovObj->GetDataAsPlainString()
         << ", nObjectType = " << pGovObj->GetObjectType()
         << std::endl; );*/

    if (pGovObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
        //DBG( std::cout << "CSuperblock Constructor pGovObj not a trigger, returning" << std::endl; );
        throw std::runtime_error("CSuperblock: Governance Object not a trigger");
    }

    UniValue obj = pGovObj->GetJSONObject();

    // FIRST WE GET THE START HEIGHT, THE BLOCK HEIGHT AT WHICH THE PAYMENT SHALL OCCUR
    nBlockHeight = obj["event_block_height"].get_int();

    // NEXT WE GET THE PAYMENT INFORMATION AND RECONSTRUCT THE PAYMENT VECTOR
    std::string strAddresses = obj["payment_addresses"].get_str();
    std::string strAmounts = obj["payment_amounts"].get_str();
    ParsePaymentSchedule(strAddresses, strAmounts);

    LogPrint(BCLog::GOBJECT, "CSuperblock -- nBlockHeight = %d, strAddresses = %s, strAmounts = %s, vecPayments.size() = %d\n",
             nBlockHeight, strAddresses, strAmounts, vecPayments.size());

}

/**
 *   Is Valid Superblock Height
 *
 *   - See if a block at this height can be a superblock
 */

bool CSuperblock::IsValidBlockHeight(int nBlockHeight)
{
    // SUPERBLOCKS CAN HAPPEN ONLY after hardfork and only ONCE PER CYCLE
    return nBlockHeight >= Params().GetConsensus().nSuperblockStartBlock &&
            ((nBlockHeight % Params().GetConsensus().nSuperblockCycle) == 0);
}

void CSuperblock::GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    int nSuperblockStartBlock = consensusParams.nSuperblockStartBlock;
    int nSuperblockCycle = consensusParams.nSuperblockCycle;

    // Get first superblock
    int nFirstSuperblockOffset = (nSuperblockCycle - nSuperblockStartBlock % nSuperblockCycle) % nSuperblockCycle;
    int nFirstSuperblock = nSuperblockStartBlock + nFirstSuperblockOffset;

    if(nBlockHeight < nFirstSuperblock) {
        nLastSuperblockRet = 0;
        nNextSuperblockRet = nFirstSuperblock;
    } else {
        nLastSuperblockRet = nBlockHeight - nBlockHeight % nSuperblockCycle;
        nNextSuperblockRet = nLastSuperblockRet + nSuperblockCycle;
    }
}

CAmount CSuperblock::GetPaymentsLimit(int nBlockHeight)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();

    if(!IsValidBlockHeight(nBlockHeight)) {
        return 0;
    }
    const int nSuperblock = nBlockHeight / consensusParams.nSuperblockCycle;
    CAmount nPaymentsLimit;
    if(nSuperblock > 36){
    	// some part of all blocks issued during the cycle goes to superblock, see GetBlockSubsidy
    	CAmount nTotalRewardWithMasternodes;
    	const CAmount &nSuperblockPartOfSubsidy = GetBlockSubsidy(nBlockHeight, consensusParams, nTotalRewardWithMasternodes, true);
    	nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.nSuperblockCycle;
    }
    // bootstrapping period
    else {
        switch(nSuperblock)
        {
            case 1:
                nPaymentsLimit = 2000000*COIN;
                break;
            case 2:
                nPaymentsLimit = 1900000*COIN;
                break;
            case 3:
                nPaymentsLimit = 1805000*COIN;
                break; 
            case 4:
                nPaymentsLimit = 1714750*COIN;
                break;
            case 5:
                nPaymentsLimit = 1629012.50*COIN;
                break;
            case 6:
                nPaymentsLimit = 1547561.88*COIN;
                break;
            case 7:
                nPaymentsLimit = 1470183.78*COIN;
                break; 
            case 8:
                nPaymentsLimit = 1396674.59*COIN;
                break; 
            case 9:
                nPaymentsLimit = 1326840.86*COIN;
                break;
            case 10:
                nPaymentsLimit = 1260498.82*COIN;
                break;
            case 11:
                nPaymentsLimit = 1197473.88*COIN;
                break; 
            case 12:
                nPaymentsLimit = 1137600.18*COIN;
                break; 
            case 13:
                nPaymentsLimit = 1080720.18*COIN;
                break;
            case 14:
                nPaymentsLimit = 1026684.17*COIN;
                break;
            case 15:
                nPaymentsLimit = 975349.96*COIN;
                break; 
            case 16:
                nPaymentsLimit = 926582.46*COIN;
                break; 
            case 17:
                nPaymentsLimit = 880253.34*COIN;
                break;
            case 18:
                nPaymentsLimit = 836240.67*COIN;
                break;
            case 19:
                nPaymentsLimit = 794428.64*COIN;
                break; 
            case 20:
                nPaymentsLimit = 754707.21*COIN;
                break; 
            case 21:
                nPaymentsLimit = 716971.84*COIN;
                break;
            case 22:
                nPaymentsLimit = 681123.25*COIN;
                break;
            case 23:
                nPaymentsLimit = 647067.09*COIN;
                break; 
            case 24:
                nPaymentsLimit = 614713.74*COIN;
                break;
            case 25:
                nPaymentsLimit = 583978.05*COIN;
                break;
            case 26:
                nPaymentsLimit = 554779.15*COIN;
                break;
            case 27:
                nPaymentsLimit = 527040.19*COIN;
                break; 
            case 28:
                nPaymentsLimit = 500688.18*COIN;
                break;
            case 29:
                nPaymentsLimit = 475653.77*COIN;
                break; 
            case 30:
                nPaymentsLimit = 451871.08*COIN;
                break;
            case 31:
                nPaymentsLimit = 429277.53*COIN;
                break;
            case 32:
                nPaymentsLimit = 407813.65*COIN;
                break; 
            case 33:
                nPaymentsLimit = 387422.97*COIN;
                break;
            case 34:
                nPaymentsLimit = 368051.82*COIN;
                break;
            case 35:
                nPaymentsLimit = 349649.23*COIN;
                break;    
            case 36:
                nPaymentsLimit = 332166.77*COIN;
                break;                                                                                                                                                       
        }
        /*  
            A = Accrued Amount (principal + interest)
            P = Principal Amount
            r = Annual Monthly Interest Rate as a decimal
            r = R/100
            t = Time Involved in months(superblock number).
            n = number of compounding periods per unit t; at the END of each period (1 in our case)
        */
        const double& A = 2000000.0*COIN;
        const double& r = 0.05;
        const double& t = (double)nSuperblock - 1;
        const double& P = A * pow(1.0 - r, t);
        nPaymentsLimit = (CAmount)P; 
    }
    LogPrint(BCLog::GOBJECT, "CSuperblock::GetPaymentsLimit -- Valid superblock height %d, payments max %lld\n", nBlockHeight, nPaymentsLimit);

    return nPaymentsLimit;
}

void CSuperblock::ParsePaymentSchedule(const std::string& strPaymentAddresses, const std::string& strPaymentAmounts)
{
    // SPLIT UP ADDR/AMOUNT STRINGS AND PUT IN VECTORS

    std::vector<std::string> vecParsed1;
    std::vector<std::string> vecParsed2;
    vecParsed1 = SplitBy(strPaymentAddresses, "|");
    vecParsed2 = SplitBy(strPaymentAmounts, "|");

    // IF THESE DON'T MATCH, SOMETHING IS WRONG

    if (vecParsed1.size() != vecParsed2.size()) {
        std::ostringstream ostr;
        ostr << "CSuperblock::ParsePaymentSchedule -- Mismatched payments and amounts";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        throw std::runtime_error(ostr.str());
    }

    if (vecParsed1.size() == 0) {
        std::ostringstream ostr;
        ostr << "CSuperblock::ParsePaymentSchedule -- Error no payments";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        throw std::runtime_error(ostr.str());
    }

    // LOOP THROUGH THE ADDRESSES/AMOUNTS AND CREATE PAYMENTS
    /*
      ADDRESSES = [ADDR1|2|3|4|5|6]
      AMOUNTS = [AMOUNT1|2|3|4|5|6]
    */

    //DBG( std::cout << "CSuperblock::ParsePaymentSchedule vecParsed1.size() = " << vecParsed1.size() << std::endl; );

    for (int i = 0; i < (int)vecParsed1.size(); i++) {
        const CTxDestination &address = DecodeDestination(vecParsed1[i]);
        if (!IsValidDestination(address)) {
            std::ostringstream ostr;
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid Syscoin Address : " <<  vecParsed1[i];
            LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
            throw std::runtime_error(ostr.str());
        }

        CAmount nAmount = ParsePaymentAmount(vecParsed2[i]);

        /*DBG( std::cout << "CSuperblock::ParsePaymentSchedule: "
             << "amount string = " << vecParsed2[i]
             << ", nAmount = " << nAmount
             << std::endl; );*/

        CGovernancePayment payment(address, nAmount);
        if(payment.IsValid()) {
            vecPayments.push_back(payment);
        }
        else {
            vecPayments.clear();
            std::ostringstream ostr;
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid payment found: address = " << EncodeDestination(address)
                 << ", amount = " << nAmount;
            LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
            throw std::runtime_error(ostr.str());
        }
    }
}

bool CSuperblock::GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet)
{
    if((nPaymentIndex<0) || (nPaymentIndex >= (int)vecPayments.size())) {
        return false;
    }

    paymentRet = vecPayments[nPaymentIndex];
    return true;
}

CAmount CSuperblock::GetPaymentsTotalAmount()
{
    CAmount nPaymentsTotalAmount = 0;
    int nPayments = CountPayments();

    for(int i = 0; i < nPayments; i++) {
        nPaymentsTotalAmount += vecPayments[i].nAmount;
    }

    return nPaymentsTotalAmount;
}

/**
*   Is Transaction Valid
*
*   - Does this transaction match the superblock?
*/

bool CSuperblock::IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward)
{
    // TODO : LOCK(cs);
    // No reason for a lock here now since this method only accesses data
    // internal to *this and since CSuperblock's are accessed only through
    // shared pointers there's no way our object can get deleted while this
    // code is running.
    if(!IsValidBlockHeight(nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- ERROR: Block invalid, incorrect block height\n");
        return false;
    }

    std::string strPayeesPossible = "";

    // CONFIGURE SUPERBLOCK OUTPUTS

    int nOutputs = txNew.vout.size();
    int nPayments = CountPayments();
    int nMinerPayments = nOutputs - nPayments;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid nOutputs = %d, nPayments = %d, GetDataAsHexString = %s\n",
             nOutputs, nPayments, GetGovernanceObject()->GetDataAsHexString());

    // We require an exact match (including order) between the expected
    // superblock payments and the payments actually in the block.

    if(nMinerPayments < 0) {
        // This means the block cannot have all the superblock payments
        // so it is not valid.
        // TODO: could that be that we just hit coinbase size limit?
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- ERROR: Block invalid, too few superblock payments\n");
        return false;
    }

    // payments should not exceed limit
    CAmount nPaymentsTotalAmount = GetPaymentsTotalAmount();
    CAmount nPaymentsLimit = GetPaymentsLimit(nBlockHeight);
    if(nPaymentsTotalAmount > nPaymentsLimit) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- ERROR: Block invalid, payments limit exceeded: payments %lld, limit %lld\n", nPaymentsTotalAmount, nPaymentsLimit);
        return false;
    }

    // miner should not get more than he would usually get
    CAmount nBlockValue = txNew.GetValueOut();
    
	if(nBlockValue > (blockReward + nPaymentsTotalAmount)) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- ERROR: Block invalid, block value limit exceeded: block %lld, limit %lld\n", nBlockValue, blockReward + nPaymentsTotalAmount);
        return false;
    }

    int nVoutIndex = 0;
    for(int i = 0; i < nPayments; i++) {
        CGovernancePayment payment;
        if(!GetPayment(i, payment)) {
            // This shouldn't happen so log a warning
            LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- WARNING: Failed to find payment: %d of %d total payments\n", i, nPayments);
            continue;
        }

        bool fPaymentMatch = false;

        for (int j = nVoutIndex; j < nOutputs; j++) {
            // Find superblock payment
            fPaymentMatch = ((payment.script == txNew.vout[j].scriptPubKey) &&
                             (payment.nAmount == txNew.vout[j].nValue));

            if (fPaymentMatch) {
                nVoutIndex = j;
                break;
            }
        }

        if(!fPaymentMatch) {
            // Superblock payment not found!

            CTxDestination address1;
            ExtractDestination(payment.script, address1);
            LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- ERROR: Block invalid: %d payment %d to %s not found\n", i, payment.nAmount, EncodeDestination(address1));

            return false;
        }
    }

    return true;
}

bool CSuperblock::IsExpired()
{
    bool fExpired{false};
    int nExpirationBlocks{0};
    // Executed triggers are kept for another superblock cycle (approximately 1 month),
    // other valid triggers are kept for ~1 day only, everything else is pruned after ~1h.
    switch (nStatus) {
        case SEEN_OBJECT_EXECUTED:
            nExpirationBlocks = Params().GetConsensus().nSuperblockCycle;
            break;
        case SEEN_OBJECT_IS_VALID:
            nExpirationBlocks = 576;
            break;
        default:
            nExpirationBlocks = 24;
            break;
    }

    int nExpirationBlock = nBlockHeight + nExpirationBlocks;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- nBlockHeight = %d, nExpirationBlock = %d\n", nBlockHeight, nExpirationBlock);

    if(governance.GetCachedBlockHeight() > nExpirationBlock) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Outdated trigger found\n");
        fExpired = true;
        CGovernanceObject* pgovobj = GetGovernanceObject();
        if(pgovobj) {
            LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Expiring outdated object: %s\n", pgovobj->GetHash().ToString());
            pgovobj->fExpired = true;
            pgovobj->nDeletionTime = GetAdjustedTime();
        }
    }

    return fExpired;
}

/**
*   Get Required Payment String
*
*   - Get a string representing the payments required for a given superblock
*/

std::string CSuperblockManager::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(governance.cs);
    std::string ret = "Unknown";

    // GET BEST SUPERBLOCK

    CSuperblock_sptr pSuperblock;
    if(!GetBestSuperblock(pSuperblock, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CSuperblockManager::GetRequiredPaymentsString -- Can't find superblock for height %d\n", nBlockHeight);
        return "error";
    }

    // LOOP THROUGH SUPERBLOCK PAYMENTS, CONFIGURE OUTPUT STRING

    for(int i = 0; i < pSuperblock->CountPayments(); i++) {
        CGovernancePayment payment;
        if(pSuperblock->GetPayment(i, payment)) {
            // PRINT NICE LOG OUTPUT FOR SUPERBLOCK PAYMENT

            CTxDestination address1;
            ExtractDestination(payment.script, address1);
         

            // RETURN NICE OUTPUT FOR CONSOLE

            if(ret != "Unknown") {
                ret += ", " + EncodeDestination(address1);
            }
            else {
                ret = EncodeDestination(address1);
            }
        }
    }

    return ret;
}
