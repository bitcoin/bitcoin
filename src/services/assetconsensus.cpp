// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <services/assetconsensus.h>
#include "init.h"
#include "validation.h"
#include "util.h"
#include "core_io.h"
#include "wallet/wallet.h"
#include "wallet/rpcwallet.h"
#include "chainparams.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <chrono>
#include "wallet/coincontrol.h"
#include <rpc/util.h>
#include <key_io.h>
#include <policy/policy.h>
#include <consensus/validation.h>
#include <wallet/fees.h>
#include <outputtype.h>
#include <boost/thread.hpp>
#include <merkleblock.h>
#include <services/asset.h>
#include <services/assetallocation.h>
#include <ethereum/ethereum.h>
#include <ethereum/Address.h>
#include <ethereum/Common.h>
#include <ethereum/CommonData.h>
extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesMapImpl arrivalTimesMap;

using namespace std::chrono;
using namespace std;
bool DisconnectSyscoinTransaction(const CTransaction& tx, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations, std::vector<uint256> & vecTXIDs)
{
    if(tx.IsCoinBase())
        return true;

    if (!IsSyscoinTx(tx.nVersion))
        return true;
    if(fAssetIndex)
        vecTXIDs.push_back(tx.GetHash());    
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT){
        if(!DisconnectMintAsset(tx, mapAssetAllocations))
            return false;       
    }
    else{
        if (IsAssetAllocationTx(tx.nVersion))
        {
            if(!DisconnectAssetAllocation(tx, mapAssetAllocations))
                return false;       
        }
        else if (IsAssetTx(tx.nVersion))
        {
            if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
                if(!DisconnectAssetSend(tx, mapAssets, mapAssetAllocations))
                    return false;
            } else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {  
                if(!DisconnectAssetUpdate(tx, mapAssets))
                    return false;
            }
            else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                if(!DisconnectAssetActivate(tx, mapAssets))
                    return false;
            }     
        }
    }   
    return true;       
}

bool CheckSyscoinMint(const bool ibd, const CTransaction& tx, std::string& errorMessage, const bool &fJustCheck, const bool& bSanity, const bool& bMiner, const int& nHeight, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations)
{
    static bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    // unserialize assetallocation from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    CAsset dbAsset;
    if(mintSyscoin.IsNull())
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Cannot unserialize data inside of this transaction relating to an syscoinmint");
        return false;
    }
    if(tx.nVersion == SYSCOIN_TX_VERSION_MINT && !mintSyscoin.assetAllocationTuple.IsNull())
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Tried to mint Syscoin but asset information was present");
        return false;
    }  
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && mintSyscoin.assetAllocationTuple.IsNull())
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Tried to mint asset but asset information was not present");
        return false;
    } 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && !GetAsset(mintSyscoin.assetAllocationTuple.nAsset, dbAsset)) 
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Failed to read from asset DB");
        return false;
    }
    // do this check only when not in IBD (initial block download) or litemode
    // if we are starting up and verifying the db also skip this check as fLoaded will be false until startup sequence is complete
    std::vector<unsigned char> vchTxRoot;
    if(!ibd && !fLiteMode && fLoaded && fGethSynced){
        
        int32_t cutoffHeight;
       
        // validate that the block passed is commited to by the tx root he also passes in, then validate the spv proof to the tx root below  
        if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoot(mintSyscoin.nBlockNumber, vchTxRoot)){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Invalid transaction root for SPV proof");
            return false;
        }  
        {
            LOCK(cs_ethsyncheight);
            // cutoff is ~1 week of blocks is about 40K blocks
            cutoffHeight = fGethSyncHeight - MAX_ETHEREUM_TX_ROOTS;
            if(cutoffHeight > 0 && mintSyscoin.nBlockNumber <= (uint32_t)cutoffHeight) {
                errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("The block height is too old, your SPV proof is invalid. SPV Proof must be done within ~1.5 months of the burn transaction on Ethereum blockchain");
                return false;
            } 
            // ensure that we wait atleast ETHEREUM_CONFIRMS_REQUIRED blocks (~1 hour) before we are allowed process this mint transaction  
            if(fGethSyncHeight <= 0 || (fGethSyncHeight - mintSyscoin.nBlockNumber < (bGethTestnet? 10: ETHEREUM_CONFIRMS_REQUIRED))){
                errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Not enough confirmations on Ethereum to process this mint transaction");
                return false;
            } 
        }
    }
    
    dev::RLP rlpTxRoot(&mintSyscoin.vchTxRoot);
    if(!vchTxRoot.empty() && rlpTxRoot.toBytes(dev::RLP::VeryStrict) != vchTxRoot){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Mismatching Tx Roots");
        return false;
    } 
    const std::vector<unsigned char> &vchParentNodes = mintSyscoin.vchParentNodes;
    dev::RLP rlpParentNodes(&vchParentNodes);
    const std::vector<unsigned char> &vchValue = mintSyscoin.vchValue;
    dev::RLP rlpValue(&vchValue);
    const std::vector<unsigned char> &vchPath = mintSyscoin.vchPath;
    if(!VerifyProof(&vchPath, rlpValue, rlpParentNodes, rlpTxRoot)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Could not verify ethereum transaction using SPV proof");
        return false;
    } 
    if (!rlpValue.isList()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction RLP must be a list");
        return false;
    }
    if (rlpValue.itemCount() < 6){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction RLP invalid item count");
        return false;
    }        
    if (!rlpValue[5].isData()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction data RLP must be an array");
        return false;
    }        
    if (rlpValue[3].isEmpty()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Invalid transaction receiver");
        return false;
    }                       
    const dev::Address &address160 = rlpValue[3].toHash<dev::Address>(dev::RLP::VeryStrict);
    if(tx.nVersion == SYSCOIN_TX_VERSION_MINT && Params().GetConsensus().vchSYSXContract != address160.asBytes()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Receiver not the expected SYSX contract address");
        return false;
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && dbAsset.vchContract != address160.asBytes()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Receiver not the expected SYSX contract address");
        return false;
    }
    
    CAmount outputAmount;
    uint32_t nAsset = 0;
    const std::vector<unsigned char> &rlpBytes = rlpValue[5].toBytes(dev::RLP::VeryStrict);
    CWitnessAddress witnessAddress;
    if(!parseEthMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, rlpBytes, outputAmount, nAsset, witnessAddress)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Could not parse and validate transaction data");
        return false;
    }
    if(tx.nVersion == SYSCOIN_TX_VERSION_MINT) {
        int witnessversion;
        std::vector<unsigned char> witnessprogram;
        if (tx.vout[0].scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)){
            if(!fUnitTest && (witnessAddress.vchWitnessProgram != witnessprogram || witnessAddress.nVersion != (unsigned char)witnessversion)){
                errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Witness address does not match extracted witness address from burn transaction");
                return false;
            }
        }
        else{
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Witness program not detected in the first output of the mint transaction");
            return false;
        } 
       
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT)
    {
        if(witnessAddress != mintSyscoin.assetAllocationTuple.witnessAddress){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Minting address does not match address passed into burn function");
            return false;
        }
    }    
    if(tx.nVersion == SYSCOIN_TX_VERSION_MINT && nAsset != 0){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Cannot mint an asset in a syscoin mint operation");
        return false;
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && nAsset != dbAsset.nAsset){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Invalid asset being minted, does not match asset GUID encoded in transaction data");
        return false;
    }
    if(tx.nVersion == SYSCOIN_TX_VERSION_MINT && outputAmount != tx.vout[0].nValue){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn amount must match mint amount");
        return false;
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && outputAmount != mintSyscoin.nValueAsset){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn amount must match asset mint amount");
        return false;
    }  
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT){
        if(outputAmount <= 0){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn amount must be positive");
            return false;
        }  
    
        const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
        auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr, std::move(emptyAllocation));
        auto mapAssetAllocation = result1.first;
        const bool &mapAssetAllocationNotFound = result1.second;
        if(mapAssetAllocationNotFound){
            CAssetAllocation receiverAllocation;
            GetAssetAllocation(mintSyscoin.assetAllocationTuple, receiverAllocation);
            if (receiverAllocation.assetAllocationTuple.IsNull()) {           
                receiverAllocation.assetAllocationTuple.nAsset = std::move(mintSyscoin.assetAllocationTuple.nAsset);
                receiverAllocation.assetAllocationTuple.witnessAddress = std::move(mintSyscoin.assetAllocationTuple.witnessAddress);
                if(fAssetIndex && !fJustCheck && !bSanity && !bMiner){
                    std::vector<uint32_t> assetGuids;
                    passetindexdb->ReadAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                    if(std::find(assetGuids.begin(), assetGuids.end(), receiverAllocation.assetAllocationTuple.nAsset) == assetGuids.end())
                        assetGuids.push_back(receiverAllocation.assetAllocationTuple.nAsset);
                    
                    passetindexdb->WriteAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                } 
            }
            mapAssetAllocation->second = std::move(receiverAllocation);              
        }
        CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
        // sender as burn
        const CAssetAllocationTuple senderAllocationTuple(mintSyscoin.assetAllocationTuple.nAsset, CWitnessAddress(0, vchFromString("burn")));
        const std::string &senderTupleStr = senderAllocationTuple.ToString();
        auto result2 = mapAssetAllocations.try_emplace(senderTupleStr, std::move(emptyAllocation));
        auto mapSenderAssetAllocation = result2.first;
        const bool &mapSenderAssetAllocationNotFound = result2.second;
        if(mapSenderAssetAllocationNotFound){
            CAssetAllocation senderAllocation;
            GetAssetAllocation(senderAllocationTuple, senderAllocation);
            if (senderAllocation.assetAllocationTuple.IsNull()) {           
                senderAllocation.assetAllocationTuple.nAsset = std::move(senderAllocationTuple.nAsset);
                senderAllocation.assetAllocationTuple.witnessAddress = std::move(senderAllocationTuple.witnessAddress); 
            }
            mapSenderAssetAllocation->second = std::move(senderAllocation);              
        }
        CAssetAllocation& storedSenderAllocationRef = mapSenderAssetAllocation->second;
        if (!AssetRange(mintSyscoin.nValueAsset))
        {
            errorMessage = "SYSCOIN_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Amount out of money range");
            return false;
        }

        // update balances  
        storedReceiverAllocationRef.nBalance += mintSyscoin.nValueAsset;
        storedSenderAllocationRef.nBalance -= mintSyscoin.nValueAsset;
        if(storedSenderAllocationRef.nBalance < 0){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn balance cannot go below 0");
            return false;
        }    
        if(storedSenderAllocationRef.nBalance == 0)
            storedSenderAllocationRef.SetNull();  
        if(!fJustCheck && !bSanity && !bMiner)     
            passetallocationdb->WriteMintIndex(tx, mintSyscoin, nHeight);         
                                       
    }
    return true;
}
bool CheckSyscoinInputs(const bool ibd, const CTransaction& tx, CValidationState& state, const CCoinsViewCache &inputs, bool fJustCheck, bool &bOverflow, int nHeight, const CBlock& block, const bool &bSanity, const bool &bMiner, std::vector<uint256> &txsToRemove)
{
    AssetAllocationMap mapAssetAllocations;
    AssetMap mapAssets;
  
    if (nHeight == 0)
        nHeight = chainActive.Height()+1;   
    std::string errorMessage;
    bool good = true;

    bOverflow=false;
    if (block.vtx.empty()) {
        if(!IsSyscoinTx(tx.nVersion))
            return true;
        
        if(tx.IsCoinBase())
            return true;
       
        if (IsAssetAllocationTx(tx.nVersion))
        {
            errorMessage.clear();
            good = CheckAssetAllocationInputs(tx, inputs, fJustCheck, nHeight, mapAssetAllocations,errorMessage, bOverflow, bSanity, bMiner);
        }
        else if (IsAssetTx(tx.nVersion))
        {
            errorMessage.clear();
            good = CheckAssetInputs(tx, inputs, fJustCheck, nHeight, mapAssets, mapAssetAllocations, errorMessage, bSanity, bMiner);
        }
        else if(IsSyscoinMintTx(tx.nVersion)) 
        {
            errorMessage.clear();
            good = CheckSyscoinMint(ibd, tx, errorMessage, fJustCheck, bSanity, bMiner, nHeight, mapAssets, mapAssetAllocations);
        }
  
        if (!good || !errorMessage.empty())
            return state.DoS(100, false, REJECT_INVALID, errorMessage);
      
        return true;
    }
    else if (!block.vtx.empty()) {
        for (unsigned int i = 0; i < block.vtx.size(); i++)
        {

            good = true;
            const CTransaction &tx = *(block.vtx[i]);     
            if(!bMiner && fBlockIndex && !fJustCheck && !bSanity){
                if(!passetindexdb->WriteBlockHash(tx.GetHash(), block.GetHash())){
                    return state.DoS(0, false, REJECT_INVALID, "Could not write block hash to asset index db");
                }
            }  
            if(tx.IsCoinBase()) 
                continue;                 
            if(!IsSyscoinTx(tx.nVersion))
                continue;      
            if (IsAssetAllocationTx(tx.nVersion))
            {
                errorMessage.clear();
                // fJustCheck inplace of bSanity to preserve global structures from being changed during test calls, fJustCheck is actually passed in as false because we want to check in PoW mode
                good = CheckAssetAllocationInputs(tx, inputs, false, nHeight, mapAssetAllocations, errorMessage, bOverflow, fJustCheck, bMiner);

            }
            else if (IsAssetTx(tx.nVersion))
            {
                errorMessage.clear();
                good = CheckAssetInputs(tx, inputs, false, nHeight, mapAssets, mapAssetAllocations, errorMessage, fJustCheck, bMiner);
            } 
            else if(IsSyscoinMintTx(tx.nVersion))
            {
                errorMessage.clear();
                good = CheckSyscoinMint(ibd, tx, errorMessage, false, fJustCheck, bMiner, nHeight, mapAssets, mapAssetAllocations);
            }
             
                        
            if (!good)
            {
                if (!errorMessage.empty()) {
                    // if validation fails we should not include this transaction in a block
                    if(bMiner){
                        good = true;
                        errorMessage.clear();
                        txsToRemove.push_back(tx.GetHash());
                        continue;
                    }
                }
                
            } 
        }

        if(!bSanity && !fJustCheck){
            if(!bMiner && (!passetallocationdb->Flush(mapAssetAllocations) || !passetdb->Flush(mapAssets))){
                good = false;
                errorMessage = "Error flushing to asset dbs";
            }
            mapAssetAllocations.clear();
            mapAssets.clear();
        }        
        if (!good || !errorMessage.empty())
            return state.DoS(bOverflow? 10: 100, false, REJECT_INVALID, errorMessage);
    }
    return true;
}

void ResyncAssetAllocationStates(){ 
    int count = 0;
     {
        
        vector<string> vecToRemoveMempoolBalances;
        LOCK2(cs_main, mempool.cs);
        LOCK(cs_assetallocation);
        LOCK(cs_assetallocationarrival);
        for (auto&indexObj : mempoolMapAssetBalances) {
            vector<uint256> vecToRemoveArrivalTimes;
            const string& strSenderTuple = indexObj.first;
            // if no arrival time for this mempool balance, remove it
            auto arrivalTimes = arrivalTimesMap.find(strSenderTuple);
            if(arrivalTimes == arrivalTimesMap.end()){
                vecToRemoveMempoolBalances.push_back(strSenderTuple);
                continue;
            }
            for(auto& arrivalTime: arrivalTimes->second){
                const uint256& txHash = arrivalTime.first;
                const CTransactionRef txRef = mempool.get(txHash);
                // if mempool doesnt have txid then remove from both arrivalTime and mempool balances
                if (!txRef){
                    vecToRemoveArrivalTimes.push_back(txHash);
                }
                if(!arrivalTime.first.IsNull() && ((chainActive.Tip()->GetMedianTimePast()*1000) - arrivalTime.second) > 1800000){
                    vecToRemoveArrivalTimes.push_back(txHash);
                }
            }
            // if we are removing everything from arrivalTime map then might as well remove it from parent altogether
            if(vecToRemoveArrivalTimes.size() >= arrivalTimes->second.size()){
                arrivalTimesMap.erase(strSenderTuple);
                vecToRemoveMempoolBalances.push_back(strSenderTuple);
            } 
            // otherwise remove the individual txids
            else{
                for(auto &removeTxHash: vecToRemoveArrivalTimes){
                    arrivalTimes->second.erase(removeTxHash);
                }
            }         
        }
        count+=vecToRemoveMempoolBalances.size();
        for(auto& senderTuple: vecToRemoveMempoolBalances){
            mempoolMapAssetBalances.erase(senderTuple);
            // also remove from assetAllocationConflicts
            sorted_vector<string>::const_iterator it = assetAllocationConflicts.find(senderTuple);
            if (it != assetAllocationConflicts.end()) {
                assetAllocationConflicts.V.erase(const_iterator_cast(assetAllocationConflicts.V, it));
            }
        }       
    }   
    if(count > 0)
        LogPrint(BCLog::SYS,"removeExpiredMempoolBalances removed %d expired asset allocation transactions from mempool balances\n", count);

}
bool ResetAssetAllocation(const string &senderStr, const uint256 &txHash, const bool &bMiner, const bool& bCheckExpiryOnly) {
    bool removeAllConflicts = true;
    if(!bMiner){
        {
            LOCK(cs_assetallocationarrival);
            // remove the conflict once we revert since it is assumed to be resolved on POW
            auto arrivalTimes = arrivalTimesMap.find(senderStr);
            
            if(arrivalTimes != arrivalTimesMap.end()){
                // remove only if all arrival times are either expired (30 mins) or no more zdag transactions left for this sender
                for(auto& arrivalTime: arrivalTimes->second){
                    // ensure mempool has the tx and its less than 30 mins old
                    if(bCheckExpiryOnly && !mempool.get(arrivalTime.first))
                        continue;
                    if(!arrivalTime.first.IsNull() && ((chainActive.Tip()->GetMedianTimePast()*1000) - arrivalTime.second) <= 1800000){
                        removeAllConflicts = false;
                        break;
                    }
                }
            }
            if(removeAllConflicts){
                if(arrivalTimes != arrivalTimesMap.end())
                    arrivalTimesMap.erase(arrivalTimes);
                sorted_vector<string>::const_iterator it = assetAllocationConflicts.find(senderStr);
                if (it != assetAllocationConflicts.end()) {
                    assetAllocationConflicts.V.erase(const_iterator_cast(assetAllocationConflicts.V, it));
                }   
            }
            else if(!bCheckExpiryOnly){
                arrivalTimes->second.erase(txHash);
                if(arrivalTimes->second.size() <= 0)
                    removeAllConflicts = true;
            }
        }
        if(removeAllConflicts)
        {
            LOCK(cs_assetallocation);
            mempoolMapAssetBalances.erase(senderStr);
        }
    }
    

    return removeAllConflicts;
    
}
bool DisconnectMintAsset(const CTransaction &tx, AssetAllocationMap &mapAssetAllocations){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull())
    {
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Cannot unserialize data inside of this transaction relating to an syscoinmint\n");
        return false;
    }   
    // recver
    const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
    auto result1 = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
    auto mapAssetAllocation = result1.first;
    const bool& mapAssetAllocationNotFound = result1.second;
    if(mapAssetAllocationNotFound){
        CAssetAllocation receiverAllocation;
        GetAssetAllocation(mintSyscoin.assetAllocationTuple, receiverAllocation);
        if (receiverAllocation.assetAllocationTuple.IsNull()) {
            receiverAllocation.assetAllocationTuple.nAsset = std::move(mintSyscoin.assetAllocationTuple.nAsset);
            receiverAllocation.assetAllocationTuple.witnessAddress = std::move(mintSyscoin.assetAllocationTuple.witnessAddress);
        } 
        mapAssetAllocation->second = std::move(receiverAllocation);                 
    }
    CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
    // sender
    const CAssetAllocationTuple senderAllocationTuple(mintSyscoin.assetAllocationTuple.nAsset, CWitnessAddress(0, vchFromString("burn")));
    const std::string &senderTupleStr = senderAllocationTuple.ToString();
    auto result2 = mapAssetAllocations.try_emplace(std::move(senderTupleStr), std::move(emptyAllocation));
    auto mapSenderAssetAllocation = result2.first;
    const bool& mapSenderAssetAllocationNotFound = result2.second;
    if(mapSenderAssetAllocationNotFound){
        CAssetAllocation senderAllocation;
        GetAssetAllocation(senderAllocationTuple, senderAllocation);
        if (senderAllocation.assetAllocationTuple.IsNull()) {
            senderAllocation.assetAllocationTuple.nAsset = std::move(senderAllocationTuple.nAsset);
            senderAllocation.assetAllocationTuple.witnessAddress = std::move(senderAllocationTuple.witnessAddress);
        } 
        mapAssetAllocation->second = std::move(senderAllocation);                 
    }
    CAssetAllocation& storedSenderAllocationRef = mapSenderAssetAllocation->second;
    
    storedSenderAllocationRef.nBalance += mintSyscoin.nValueAsset;
    storedReceiverAllocationRef.nBalance -= mintSyscoin.nValueAsset;
    if(storedReceiverAllocationRef.nBalance < 0) {
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Receiver balance of %s is negative: %lld\n",mintSyscoin.assetAllocationTuple.ToString(), storedReceiverAllocationRef.nBalance);
        return false;
    }       
    else if(storedReceiverAllocationRef.nBalance == 0){
        storedReceiverAllocationRef.SetNull();
    }
    if(fAssetIndex){
        const uint256& txid = tx.GetHash();
        if(!passetindexdb->EraseIndexTXID(mintSyscoin.assetAllocationTuple, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase mint asset allocation from asset allocation index\n");
        }
        if(!passetindexdb->EraseIndexTXID(mintSyscoin.assetAllocationTuple.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase mint asset allocation from asset index\n");
        } 
        if(!passetindexdb->EraseIndexTXID(senderAllocationTuple, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase mint sender asset allocation from asset allocation index\n");
        }
        if(!passetindexdb->EraseIndexTXID(senderAllocationTuple.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase mint sender asset allocation from asset index\n");
        }       
    }    
    return true; 
}
bool DisconnectAssetAllocation(const CTransaction &tx, AssetAllocationMap &mapAssetAllocations){
    const uint256& txid = tx.GetHash();
    CAssetAllocation theAssetAllocation(tx);

    const std::string &senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset allocation\n");
        return false;
    }
    auto result = mapAssetAllocations.try_emplace(std::move(senderTupleStr), std::move(emptyAllocation));
    auto mapAssetAllocation = result.first;
    const bool & mapAssetAllocationNotFound = result.second;
    if(mapAssetAllocationNotFound){
        CAssetAllocation senderAllocation;
        GetAssetAllocation(theAssetAllocation.assetAllocationTuple, senderAllocation);
        if (senderAllocation.assetAllocationTuple.IsNull()) {
            senderAllocation.assetAllocationTuple.nAsset = std::move(theAssetAllocation.assetAllocationTuple.nAsset);
            senderAllocation.assetAllocationTuple.witnessAddress = std::move(theAssetAllocation.assetAllocationTuple.witnessAddress);       
        } 
        mapAssetAllocation->second = std::move(senderAllocation);                 
    }
    CAssetAllocation& storedSenderAllocationRef = mapAssetAllocation->second;

    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
       
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocation receiverAllocation;
        
        auto result1 = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
        auto mapAssetAllocationReceiver = result1.first;
        const bool& mapAssetAllocationReceiverNotFound = result1.second;
        if(mapAssetAllocationReceiverNotFound){
            GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
            if (receiverAllocation.assetAllocationTuple.IsNull()) {
                receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);
            } 
            mapAssetAllocationReceiver->second = std::move(receiverAllocation);                 
        }
        CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocationReceiver->second;

        // reverse allocations
        storedReceiverAllocationRef.nBalance -= amountTuple.second;
        storedSenderAllocationRef.nBalance += amountTuple.second; 
        if(storedReceiverAllocationRef.nBalance < 0) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Receiver balance of %s is negative: %lld\n",receiverAllocationTuple.ToString(), storedReceiverAllocationRef.nBalance);
            return false;
        }
        else if(storedReceiverAllocationRef.nBalance == 0){
            storedReceiverAllocationRef.SetNull();  
        }
        if(fAssetIndex){
            if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple, txid)){
                 LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase receiver allocation from asset allocation index\n");
            }
            if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple.nAsset, txid)){
                 LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase receiver allocation from asset index\n");
            }
        }                                       
    }
    if(fAssetIndex){
        if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase sender allocation from asset allocation index\n");
        }
        if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase sender allocation from asset index\n");
        }       
    }         
    return true; 
}
bool CheckAssetAllocationInputs(const CTransaction &tx, const CCoinsViewCache &inputs,
        bool fJustCheck, int nHeight, AssetAllocationMap &mapAssetAllocations, string &errorMessage, bool& bOverflow, const bool &bSanityCheck, const bool &bMiner) {
    if (passetallocationdb == nullptr)
        return false;
    const uint256 & txHash = tx.GetHash();
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %d %s %s\n", nHeight,
            chainActive.Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK");
            

    // unserialize assetallocation from txn, check for valid
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.assetAllocationTuple.IsNull())
    {
        errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Cannot unserialize data inside of this transaction relating to an assetallocation");
        return error(errorMessage.c_str());
    }

    string retError = "";
    if(fJustCheck)
    {
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND:
            if (theAssetAllocation.listSendingAllocationAmounts.empty())
            {
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Asset send must send an input or transfer balance");
                return error(errorMessage.c_str());
            }
            if (theAssetAllocation.listSendingAllocationAmounts.size() > 250)
            {
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1005 - " + _("Too many receivers in one allocation send, maximum of 250 is allowed at once");
                return error(errorMessage.c_str());
            }
            break; 
        case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN:       
            break;     
        default:
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1009 - " + _("Asset transaction has unknown op");
            return error(errorMessage.c_str());
        }
    }

    const CWitnessAddress &user1 = theAssetAllocation.assetAllocationTuple.witnessAddress;
    const string & senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();

    CAssetAllocation dbAssetAllocation;
    AssetAllocationMap::iterator mapAssetAllocation;
    CAsset dbAsset;
    if(fJustCheck){
        if (!GetAssetAllocation(theAssetAllocation.assetAllocationTuple, dbAssetAllocation))
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Cannot find sender asset allocation");
            return error(errorMessage.c_str());
        }     
    }
    else{
        auto result = mapAssetAllocations.try_emplace(senderTupleStr, std::move(emptyAllocation));
        mapAssetAllocation = result.first;
        const bool& mapAssetAllocationNotFound = result.second;
        
        if(mapAssetAllocationNotFound){
            if (!GetAssetAllocation(theAssetAllocation.assetAllocationTuple, dbAssetAllocation))
            {
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Cannot find sender asset allocation");
                return error(errorMessage.c_str());
            }        
            mapAssetAllocation->second = std::move(dbAssetAllocation);                   
        }
    }
    CAssetAllocation& storedSenderAllocationRef = fJustCheck? dbAssetAllocation:mapAssetAllocation->second;
    
    if (!GetAsset(storedSenderAllocationRef.assetAllocationTuple.nAsset, dbAsset))
    {
        errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1011 - " + _("Failed to read from asset DB");
        return error(errorMessage.c_str());
    }
        
    AssetBalanceMap::iterator mapBalanceSender;
    CAmount mapBalanceSenderCopy;
    bool mapSenderMempoolBalanceNotFound = false;
    if(fJustCheck){
        LOCK(cs_assetallocation); 
        auto result = mempoolMapAssetBalances.try_emplace(senderTupleStr, std::move(storedSenderAllocationRef.nBalance));
        mapBalanceSender = result.first;
        mapSenderMempoolBalanceNotFound = result.second;
        mapBalanceSenderCopy = mapBalanceSender->second;
    }
    else
        mapBalanceSenderCopy = storedSenderAllocationRef.nBalance;     
           
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
    {       
        std::vector<unsigned char> vchEthAddress;
        uint32_t nAssetFromScript;
        CAmount nAmountFromScript;
        CWitnessAddress burnWitnessAddress;
        if(!GetSyscoinBurnData(tx, nAssetFromScript, burnWitnessAddress, nAmountFromScript, vchEthAddress)){
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Cannot unserialize data inside of this transaction relating to an assetallocationburn");
            return error(errorMessage.c_str());
        }   
        if(burnWitnessAddress != user1)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Mismatching deserailized witness address");
            return error(errorMessage.c_str());
        }
        if(storedSenderAllocationRef.assetAllocationTuple.nAsset != nAssetFromScript)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid asset details entered in the script output");
            return error(errorMessage.c_str());
        }
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1))
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
            return error(errorMessage.c_str());
        }
        if(dbAsset.vchContract.empty())
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Cannot burn, no contract provided in asset by owner");
            return error(errorMessage.c_str());
        } 
        if (nAmountFromScript <= 0 || nAmountFromScript > dbAsset.nTotalSupply)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Amount out of money range");
            return error(errorMessage.c_str());
        }        
        mapBalanceSenderCopy -= nAmountFromScript;
        if (mapBalanceSenderCopy < 0) {
            if(fJustCheck)
            {
                LOCK(cs_assetallocation); 
                if(mapSenderMempoolBalanceNotFound){
                    mempoolMapAssetBalances.erase(mapBalanceSender);
                }
            }
            bOverflow = true;
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
        if (!fJustCheck) {   
            const CAssetAllocationTuple receiverAllocationTuple(nAssetFromScript,  CWitnessAddress(0, vchFromString("burn")));
            const string& receiverTupleStr = receiverAllocationTuple.ToString();  
            auto result = mapAssetAllocations.try_emplace(receiverTupleStr, std::move(emptyAllocation));
            auto mapAssetAllocationReceiver = result.first;
            const bool& mapAssetAllocationReceiverNotFound = result.second;
            if(mapAssetAllocationReceiverNotFound){
                CAssetAllocation dbAssetAllocationReceiver;
                if (!GetAssetAllocation(receiverAllocationTuple, dbAssetAllocationReceiver)) {               
                    dbAssetAllocationReceiver.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                    dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress); 
                    if(fAssetIndex && !bMiner){
                        std::vector<uint32_t> assetGuids;
                        passetindexdb->ReadAssetsByAddress(dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress, assetGuids);
                        if(std::find(assetGuids.begin(), assetGuids.end(), dbAssetAllocationReceiver.assetAllocationTuple.nAsset) == assetGuids.end())
                            assetGuids.push_back(dbAssetAllocationReceiver.assetAllocationTuple.nAsset);
                        
                        passetindexdb->WriteAssetsByAddress(dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress, assetGuids);
                    }              
                }
                mapAssetAllocationReceiver->second = std::move(dbAssetAllocationReceiver);                   
            } 
            mapAssetAllocationReceiver->second.nBalance += nAmountFromScript;                        
        }else if (!bSanityCheck && !bMiner) {
            LOCK(cs_assetallocationarrival);
            // add conflicting sender if using ZDAG
            assetAllocationConflicts.insert(senderTupleStr);
        }
    }
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND)
    {
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1))
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015a - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
            return error(errorMessage.c_str());
        }       
        
        // check balance is sufficient on sender
        CAmount nTotal = 0;
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            if (amountTuple.second <= 0)
            {
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1020 - " + _("Receiving amount must be positive");
                return error(errorMessage.c_str());
            }           
        }
        mapBalanceSenderCopy -= nTotal;
        if (mapBalanceSenderCopy < 0) {
            if(fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationarrival);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);
            }
            if(fJustCheck)
            {
                LOCK(cs_assetallocation); 
                if(mapSenderMempoolBalanceNotFound){
                    mempoolMapAssetBalances.erase(mapBalanceSender);
                }
            }
            bOverflow = true;            
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1021 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
               
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            if (amountTuple.first == theAssetAllocation.assetAllocationTuple.witnessAddress) {
                if(fJustCheck)
                {
                    LOCK(cs_assetallocation); 
                    if(mapSenderMempoolBalanceNotFound){
                        mempoolMapAssetBalances.erase(mapBalanceSender);
                    }
                }           
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1022 - " + _("Cannot send an asset allocation to yourself");
                return error(errorMessage.c_str());
            }
        
            const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
            const string &receiverTupleStr = receiverAllocationTuple.ToString();
            AssetBalanceMap::iterator mapBalanceReceiver;
            AssetAllocationMap::iterator mapBalanceReceiverBlock;            
            if(fJustCheck){
                LOCK(cs_assetallocation);
                auto result = mempoolMapAssetBalances.try_emplace(std::move(receiverTupleStr), 0);
                auto mapBalanceReceiver = result.first;
                const bool& mapAssetAllocationReceiverNotFound = result.second;
                if(mapAssetAllocationReceiverNotFound){
                    CAssetAllocation receiverAllocation;
                    GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
                    mapBalanceReceiver->second = std::move(receiverAllocation.nBalance);
                }
                if(!bSanityCheck){
                    mapBalanceReceiver->second += amountTuple.second;
                }
            }  
            else{           
                auto result = mapAssetAllocations.try_emplace( receiverTupleStr, std::move(emptyAllocation));
                auto mapBalanceReceiverBlock = result.first;
                const bool& mapAssetAllocationReceiverBlockNotFound = result.second;
                if(mapAssetAllocationReceiverBlockNotFound){
                    CAssetAllocation receiverAllocation;
                    if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {                   
                        receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                        receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress); 
                        if(fAssetIndex && !bMiner){
                            std::vector<uint32_t> assetGuids;
                            passetindexdb->ReadAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                            if(std::find(assetGuids.begin(), assetGuids.end(), receiverAllocation.assetAllocationTuple.nAsset) == assetGuids.end())
                                assetGuids.push_back(receiverAllocation.assetAllocationTuple.nAsset);
                            
                            passetindexdb->WriteAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                        }                       
                    }
                    mapBalanceReceiverBlock->second = std::move(receiverAllocation);   
                }
                mapBalanceReceiverBlock->second.nBalance += amountTuple.second; 
                // to remove mempool balances but need to check to ensure that all txid's from arrivalTimes are first gone before removing receiver mempool balance
                // otherwise one can have a conflict as a sender and send himself an allocation and clear the mempool balance inadvertently
                ResetAssetAllocation(receiverTupleStr, txHash, bMiner);           
            }

        }   
    }
    // write assetallocation  
    // asset sends are the only ones confirming without PoW
    if(!fJustCheck){
        if (!bSanityCheck) {
            ResetAssetAllocation(senderTupleStr, txHash, bMiner);
           
        } 
        storedSenderAllocationRef.listSendingAllocationAmounts.clear();
        storedSenderAllocationRef.nBalance = std::move(mapBalanceSenderCopy);
        if(storedSenderAllocationRef.nBalance == 0)
            storedSenderAllocationRef.SetNull();    

        if(!bMiner) {   
            // send notification on pow, for zdag transactions this is the second notification meaning the zdag tx has been confirmed
            passetallocationdb->WriteAssetAllocationIndex(tx, dbAsset, true, nHeight);  
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                senderTupleStr.c_str(),
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);      
        }
                    
    }
    else{
        if(!bSanityCheck){
            LOCK(cs_assetallocationarrival);
            ArrivalTimesMap &arrivalTimes = arrivalTimesMap[senderTupleStr];
            arrivalTimes[txHash] = GetTimeMillis();
        }
        if(!bSanityCheck)
        {
            // only send a realtime notification on zdag, send another when pow happens (above)
            if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND)
                passetallocationdb->WriteAssetAllocationIndex(tx, dbAsset, false, nHeight);
            {
                LOCK(cs_assetallocation);
                mapBalanceSender->second = std::move(mapBalanceSenderCopy);
            }
        }
    } 
    return true;
}

bool DisconnectAssetSend(const CTransaction &tx, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations){
    const uint256 &txid = tx.GetHash();
    CAsset dbAsset;
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset allocation in asset send\n");
        return false;
    } 
    auto result  = mapAssets.try_emplace(theAssetAllocation.assetAllocationTuple.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAssetAllocation.assetAllocationTuple.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAssetAllocation.assetAllocationTuple.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    CAsset& storedSenderRef = mapAsset->second;
               
               
    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocation receiverAllocation;
        auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
        auto mapAssetAllocation = result.first;
        const bool &mapAssetAllocationNotFound = result.second;
        if(mapAssetAllocationNotFound){
            GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
            if (receiverAllocation.assetAllocationTuple.IsNull()) {
                receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);
            } 
            mapAssetAllocation->second = std::move(receiverAllocation);                 
        }
        CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
                    

        // reverse allocation
        if(storedReceiverAllocationRef.nBalance >= amountTuple.second){
            storedReceiverAllocationRef.nBalance -= amountTuple.second;
            storedSenderRef.nBalance += amountTuple.second;
        } 
        if(storedReceiverAllocationRef.nBalance == 0){
            storedReceiverAllocationRef.SetNull();       
        }
        if(fAssetIndex){
            if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple, txid)){
                 LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase receiver allocation from asset allocation index\n");
            }
            if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple.nAsset, txid)){
                 LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase receiver allocation from asset index\n");
            } 
        }                                             
    }     
    if(fAssetIndex){
        if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase sender allocation from asset allocation index\n");
        }
        if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase sender allocation from asset index\n");
        }       
    }          
    return true;  
}
bool DisconnectAssetUpdate(const CTransaction &tx, AssetMap &mapAssets){
    
    CAsset dbAsset;
    CAsset theAsset(tx);
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset\n");
        return false;
    }
    auto result = mapAssets.try_emplace(theAsset.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    CAsset& storedSenderRef = mapAsset->second;   
           
    if(theAsset.nBalance > 0){
        // reverse asset minting by the issuer
        storedSenderRef.nBalance -= theAsset.nBalance;
        storedSenderRef.nTotalSupply -= theAsset.nBalance;
        if(storedSenderRef.nBalance < 0 || storedSenderRef.nTotalSupply < 0) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Asset cannot be negative: Balance %lld, Supply: %lld\n",storedSenderRef.nBalance, storedSenderRef.nTotalSupply);
            return false;
        }                                          
    } 
    if(fAssetIndex){
        const uint256 &txid = tx.GetHash();
        if(!passetindexdb->EraseIndexTXID(theAsset.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase asset update from asset index\n");
        }
    }         
    return true;  
}
bool DisconnectAssetActivate(const CTransaction &tx, AssetMap &mapAssets){
    
    CAsset theAsset(tx);
    
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset in asset activate\n");
        return false;
    }
    auto result = mapAssets.try_emplace(theAsset.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        CAsset dbAsset;
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    mapAsset->second.SetNull();  
    if(fAssetIndex){
        const uint256 &txid = tx.GetHash();
        if(!passetindexdb->EraseIndexTXID(theAsset.nAsset, txid)){
             LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not erase asset activate from asset index\n");
        }       
    }       
    return true;  
}
bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs,
        bool fJustCheck, int nHeight, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations, string &errorMessage, const bool &bSanityCheck, const bool &bMiner) {
    if (passetdb == nullptr)
        return false;
    const uint256& txHash = tx.GetHash();
    if (!bSanityCheck)
        LogPrint(BCLog::SYS, "*** ASSET %d %d %s %s\n", nHeight,
            chainActive.Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK");

    // unserialize asset from txn, check for valid
    CAsset theAsset;
    CAssetAllocation theAssetAllocation;
    vector<unsigned char> vchData;

    int nDataOut;
    if(!GetSyscoinData(tx, vchData, nDataOut) || (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND && !theAsset.UnserializeFromData(vchData)) || (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND && !theAssetAllocation.UnserializeFromData(vchData)))
    {
        errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2000 - " + _("Cannot unserialize data inside of this transaction relating to an asset");
        return error(errorMessage.c_str());
    }
    

    if(fJustCheck)
    {
        if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND) {
            if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Asset public data too big");
                return error(errorMessage.c_str());
            }
        }
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
            if (theAsset.nAsset <= SYSCOIN_TX_VERSION_MINT)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("asset guid invalid");
                return error(errorMessage.c_str());
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Contract address not proper size");
                return error(errorMessage.c_str());
            }  
            if (theAsset.nPrecision > 8)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Precision must be between 0 and 8");
                return error(errorMessage.c_str());
            }
            if (theAsset.nMaxSupply != -1 && !AssetRange(theAsset.nMaxSupply, theAsset.nPrecision))
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2014 - " + _("Max supply out of money range");
                return error(errorMessage.c_str());
            }
            if (theAsset.nBalance > theAsset.nMaxSupply)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Total supply cannot exceed maximum supply");
                return error(errorMessage.c_str());
            }
            if (!theAsset.witnessAddress.IsValid())
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Address specified is invalid");
                return error(errorMessage.c_str());
            }
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Invalid update flags");
                return error(errorMessage.c_str());
            }          
            break;

        case SYSCOIN_TX_VERSION_ASSET_UPDATE:
            if (theAsset.nBalance < 0){
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Balance must be greater than or equal to 0");
                return error(errorMessage.c_str());
            }
            if (!theAssetAllocation.assetAllocationTuple.IsNull())
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Cannot update allocations");
                return error(errorMessage.c_str());
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Contract address not proper size");
                return error(errorMessage.c_str());
            }  
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Invalid update flags");
                return error(errorMessage.c_str());
            }           
            break;
            
        case SYSCOIN_TX_VERSION_ASSET_SEND:
            if (theAssetAllocation.listSendingAllocationAmounts.empty())
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2020 - " + _("Asset send must send an input or transfer balance");
                return error(errorMessage.c_str());
            }
            if (theAssetAllocation.listSendingAllocationAmounts.size() > 250)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Too many receivers in one allocation send, maximum of 250 is allowed at once");
                return error(errorMessage.c_str());
            }
            break;
        case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
            break;
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2023 - " + _("Asset transaction has unknown op");
            return error(errorMessage.c_str());
        }
    }

    CAsset dbAsset;
    const uint32_t &nAsset = tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND ? theAssetAllocation.assetAllocationTuple.nAsset : theAsset.nAsset;
    auto result = mapAssets.try_emplace(nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool & mapAssetNotFound = result.second; 
    if (mapAssetNotFound)
    {
        if(!GetAsset(nAsset, dbAsset)){
            if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Failed to read from asset DB");
                return error(errorMessage.c_str());
            }
            else
                 mapAsset->second = std::move(theAsset); 
        }
        else{
            if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE){
                errorMessage =  "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2041 - " + _("Asset already exists");
                return error(errorMessage.c_str());
            }
            mapAsset->second = std::move(dbAsset); 
        }
    }
    CAsset &storedSenderAssetRef = mapAsset->second;
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER) {
    
        if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot transfer this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        }           
    }

    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {
        if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        }

        if (theAsset.nBalance > 0 && !(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_SUPPLY))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update supply");
            return error(errorMessage.c_str());
        }          
        // increase total supply
        storedSenderAssetRef.nTotalSupply += theAsset.nBalance;
        storedSenderAssetRef.nBalance += theAsset.nBalance;

        if (!AssetRange(storedSenderAssetRef.nTotalSupply, storedSenderAssetRef.nPrecision))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Total supply out of money range");
            return error(errorMessage.c_str());
        }
        if (storedSenderAssetRef.nTotalSupply > storedSenderAssetRef.nMaxSupply)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2030 - " + _("Total supply cannot exceed maximum supply");
            return error(errorMessage.c_str());
        }

    }      
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
        if (storedSenderAssetRef.witnessAddress != theAssetAllocation.assetAllocationTuple.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        }

        // check balance is sufficient on sender
        CAmount nTotal = 0;
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            if (amountTuple.second <= 0)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2032 - " + _("Receiving amount must be positive");
                return error(errorMessage.c_str());
            }
        }
        if (storedSenderAssetRef.nBalance < nTotal) {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2033 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            if (!bSanityCheck) {
                CAssetAllocation receiverAllocation;
                const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
                const string& receiverTupleStr = receiverAllocationTuple.ToString();
                auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
                auto mapAssetAllocation = result.first;
                const bool& mapAssetAllocationNotFound = result.second;
               
                if(mapAssetAllocationNotFound){
                    GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
                    if (receiverAllocation.assetAllocationTuple.IsNull()) {
                        receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                        receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);
                        if(fAssetIndex && !fJustCheck && !bMiner){
                            std::vector<uint32_t> assetGuids;
                            passetindexdb->ReadAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                            if(std::find(assetGuids.begin(), assetGuids.end(), receiverAllocation.assetAllocationTuple.nAsset) == assetGuids.end())
                                assetGuids.push_back(receiverAllocation.assetAllocationTuple.nAsset);
                            
                            passetindexdb->WriteAssetsByAddress(receiverAllocation.assetAllocationTuple.witnessAddress, assetGuids);
                        }                        
                    } 
                    mapAssetAllocation->second = std::move(receiverAllocation);                
                }
                
                CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
                
                storedReceiverAllocationRef.nBalance += amountTuple.second;
                                        
                // adjust sender balance
                storedSenderAssetRef.nBalance -= amountTuple.second;                              
            }
        }
        if (!bSanityCheck && !fJustCheck && !bMiner)
            passetallocationdb->WriteAssetAllocationIndex(tx, storedSenderAssetRef, true, nHeight);  
    }
    else if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_ACTIVATE)
    {         
        if (!theAsset.witnessAddress.IsNull())
            storedSenderAssetRef.witnessAddress = theAsset.witnessAddress;
        if (!theAsset.vchPubData.empty())
            storedSenderAssetRef.vchPubData = theAsset.vchPubData;
        else if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_DATA))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update public data");
            return error(errorMessage.c_str());
        }
                            
        if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update smart contract burn method signature");
            return error(errorMessage.c_str());
        }
        
        if (!theAsset.vchContract.empty() && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_TRANSFER)
            storedSenderAssetRef.vchContract = theAsset.vchContract;             
        else if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update smart contract");
            return error(errorMessage.c_str());
        }    
              
        if (theAsset.nUpdateFlags != storedSenderAssetRef.nUpdateFlags && (!(storedSenderAssetRef.nUpdateFlags & (ASSET_UPDATE_FLAGS | ASSET_UPDATE_ADMIN)))) {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2040 - " + _("Insufficient privileges to update flags");
            return error(errorMessage.c_str());
        }
        storedSenderAssetRef.nUpdateFlags = theAsset.nUpdateFlags;


    }
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE)
    {
        if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot create this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        }          
        // starting supply is the supplied balance upon init
        storedSenderAssetRef.nTotalSupply = storedSenderAssetRef.nBalance;
    }
    // set the asset's txn-dependent values
    storedSenderAssetRef.nHeight = nHeight;
    storedSenderAssetRef.txHash = txHash;
    // write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
    if (!bSanityCheck && !fJustCheck && !bMiner) {
        passetdb->WriteAssetIndex(tx, storedSenderAssetRef, nHeight);
        LogPrint(BCLog::SYS,"CONNECTED ASSET: tx=%s symbol=%d hash=%s height=%d fJustCheck=%d\n",
                assetFromTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);
    }
    
    return true;
}