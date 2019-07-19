// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <services/assetconsensus.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <ethereum/ethereum.h>
#include <ethereum/address.h>
#include <ethereum/common.h>
#include <ethereum/commondata.h>
#include <boost/thread.hpp>
#include <services/rpc/assetrpc.h>
#include <validationinterface.h>
extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesMapImpl arrivalTimesMap;
extern std::unordered_set<std::string> assetAllocationConflicts;
extern CCriticalSection cs_assetallocationmempoolbalance;
extern CCriticalSection cs_assetallocationarrival;
extern CCriticalSection cs_assetallocationconflicts;
std::unique_ptr<CBlockIndexDB> pblockindexdb;
std::unique_ptr<CLockedOutpointsDB> plockedoutpointsdb;
std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
std::unique_ptr<CEthereumMintedTxDB> pethereumtxmintdb;
CCriticalSection cs_assetallocationprevout;
AssetTXPrevOutPointMap mempoolMapAssetTXPrevOutPoints GUARDED_BY(cs_assetallocationprevout);
int64_t nLastMultithreadMempoolFailure = 0;
std::vector<std::pair<uint256, int64_t> >  vecToRemoveFromMempool;
CCriticalSection cs_assetallocationmempoolremovetx;
using namespace std;
bool DisconnectSyscoinTransaction(const CTransaction& tx, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys)
{
    if(tx.IsCoinBase())
        return true;

    if (!IsSyscoinTx(tx.nVersion))
        return true;
 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT){
        if(!DisconnectMintAsset(tx, mapAssetSupplyStats, mapAssetAllocations, vecMintKeys))
            return false;       
    }  
    else{
        if (IsAssetAllocationTx(tx.nVersion))
        {
            if(!DisconnectAssetAllocation(tx, mapAssetSupplyStats, mapAssetAllocations))
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
            else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER) {  
                 if(!DisconnectAssetTransfer(tx, mapAssets))
                    return false;
            }
            else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                if(!DisconnectAssetActivate(tx, mapAssets, mapAssetSupplyStats))
                    return false;
            }     
        }
    }   
    return true;       
}

bool CheckSyscoinMint(const bool ibd, const CTransaction& tx, std::string& errorMessage, const bool &fJustCheck, const bool& bSanity, const bool& bMiner, const int& nHeight, const uint256& blockhash, AssetMap& mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys, bool &bTxRootError)
{
    static bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    // unserialize mint object from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    CAsset dbAsset;
    if(mintSyscoin.IsNull())
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Cannot unserialize data inside of this transaction relating to an syscoinmint");
        return false;
    } 
    if(mintSyscoin.assetAllocationTuple.IsNull())
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Tried to mint asset but asset information was not present");
        return false;
    } 
    if(!GetAsset(mintSyscoin.assetAllocationTuple.nAsset, dbAsset)) 
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Failed to read from asset DB");
        return false;
    }
    
   
    
    // do this check only when not in IBD (initial block download) or litemode
    // if we are starting up and verifying the db also skip this check as fLoaded will be false until startup sequence is complete
    EthereumTxRoot txRootDB;
   
    uint32_t cutoffHeight;
    const bool &ethTxRootShouldExist = !ibd && !fLiteMode && fLoaded && fGethSynced;
    // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
    // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
    if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)){
        if(ethTxRootShouldExist){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Missing transaction root for SPV proof at Ethereum block: ") + itostr(mintSyscoin.nBlockNumber);
            bTxRootError = true;
            return false;
        }
    }  
    if(ethTxRootShouldExist){
        LOCK(cs_ethsyncheight);
        // cutoff is ~1 week of blocks is about 40K blocks
        cutoffHeight = fGethSyncHeight - MAX_ETHEREUM_TX_ROOTS;
        if(fGethSyncHeight >= MAX_ETHEREUM_TX_ROOTS && mintSyscoin.nBlockNumber <= (uint32_t)cutoffHeight) {
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("The block height is too old, your SPV proof is invalid. SPV Proof must be done within 40000 blocks of the burn transaction on Ethereum blockchain");
            bTxRootError = true;
            return false;
        } 
        
        // ensure that we wait at least ETHEREUM_CONFIRMS_REQUIRED blocks (~1 hour) before we are allowed process this mint transaction  
        // also ensure sanity test that the current height that our node thinks Eth is on isn't less than the requested block for spv proof
        if(fGethCurrentHeight <  mintSyscoin.nBlockNumber || fGethSyncHeight <= 0 || (fGethSyncHeight - mintSyscoin.nBlockNumber < (bGethTestnet? 10: ETHEREUM_CONFIRMS_REQUIRED))){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Not enough confirmations on Ethereum to process this mint transaction. Blocks required: ") + itostr(ETHEREUM_CONFIRMS_REQUIRED - (fGethSyncHeight - mintSyscoin.nBlockNumber));
            bTxRootError = true;
            return false;
        } 
    }
    
     // check transaction receipt validity

    const std::vector<unsigned char> &vchReceiptParentNodes = mintSyscoin.vchReceiptParentNodes;
    dev::RLP rlpReceiptParentNodes(&vchReceiptParentNodes);

    const std::vector<unsigned char> &vchReceiptValue = mintSyscoin.vchReceiptValue;
    dev::RLP rlpReceiptValue(&vchReceiptValue);
    
    if (!rlpReceiptValue.isList()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction Receipt RLP must be a list");
        return false;
    }
    if (rlpReceiptValue.itemCount() != 4){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction Receipt RLP invalid item count");
        return false;
    } 
    const uint64_t &nStatus = rlpReceiptValue[0].toInt<uint64_t>(dev::RLP::VeryStrict);
    if (nStatus != 1){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction Receipt showing invalid status, make sure transaction was accepted by Ethereum EVM");
        return false;
    } 

     
    // check transaction spv proofs
    dev::RLP rlpTxRoot(&mintSyscoin.vchTxRoot);
    dev::RLP rlpReceiptRoot(&mintSyscoin.vchReceiptRoot);

    if(!txRootDB.vchTxRoot.empty() && rlpTxRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchTxRoot){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Mismatching Tx Roots");
        bTxRootError = true; // roots can be wrong because geth may not give us affected headers post re-org
        return false;
    }

    if(!txRootDB.vchReceiptRoot.empty() && rlpReceiptRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchReceiptRoot){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Mismatching Receipt Roots");
        bTxRootError = true; // roots can be wrong because geth may not give us affected headers post re-org
        return false;
    } 
    
    
    const std::vector<unsigned char> &vchTxParentNodes = mintSyscoin.vchTxParentNodes;
    dev::RLP rlpTxParentNodes(&vchTxParentNodes);
    const std::vector<unsigned char> &vchTxValue = mintSyscoin.vchTxValue;
    dev::RLP rlpTxValue(&vchTxValue);
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    dev::RLP rlpTxPath(&vchTxPath);
    const uint32_t &nPath = rlpTxPath.toInt<uint32_t>(dev::RLP::VeryStrict);
    
    // ensure eth tx not already spent
    const std::pair<uint64_t, uint32_t> &ethKey = std::make_pair(mintSyscoin.nBlockNumber, nPath);
    if(pethereumtxmintdb->ExistsKey(ethKey)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("This block number+transaction index pair has already been used to mint");
        return false;
    } 
    // add the key to flush to db later
    vecMintKeys.emplace_back(ethKey);
    
    // verify receipt proof
    if(!VerifyProof(&vchTxPath, rlpReceiptValue, rlpReceiptParentNodes, rlpReceiptRoot)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Could not verify ethereum transaction receipt using SPV proof");
        return false;
    } 
    // verify transaction proof
    if(!VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Could not verify ethereum transaction using SPV proof");
        return false;
    } 
    if (!rlpTxValue.isList()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction RLP must be a list");
        return false;
    }
    if (rlpTxValue.itemCount() < 6){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction RLP invalid item count");
        return false;
    }        
    if (!rlpTxValue[5].isData()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Transaction data RLP must be an array");
        return false;
    }        
    if (rlpTxValue[3].isEmpty()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Invalid transaction receiver");
        return false;
    }                       
    const dev::Address &address160 = rlpTxValue[3].toHash<dev::Address>(dev::RLP::VeryStrict);

    if(dbAsset.vchContract != address160.asBytes()){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Receiver not the expected SYSX contract address");
        return false;
    }
    
    CAmount outputAmount;
    uint32_t nAsset = 0;
    const std::vector<unsigned char> &rlpBytes = rlpTxValue[5].toBytes(dev::RLP::VeryStrict);
    CWitnessAddress witnessAddress;
    if(!parseEthMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, rlpBytes, outputAmount, nAsset, witnessAddress)){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Could not parse and validate transaction data");
        return false;
    }
    if(!fUnitTest){
        if(witnessAddress != mintSyscoin.assetAllocationTuple.witnessAddress){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Minting address does not match address passed into burn function");
            return false;
        }
        
        if(nAsset != dbAsset.nAsset){
            errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Invalid asset being minted, does not match asset GUID encoded in transaction data");
            return false;
        }
    }
    if(outputAmount != mintSyscoin.nValueAsset){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn amount must match asset mint amount");
        return false;
    }  
    if(outputAmount <= 0){
        errorMessage = "SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Burn amount must be positive");
        return false;
    }  
    if(!fJustCheck){
        const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
        #if __cplusplus > 201402 
        auto result1 = mapAssetAllocations.try_emplace(std::move(receiverTupleStr),  std::move(emptyAllocation));
        #else
        auto result1 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
        #endif

        auto mapAssetAllocation = result1.first;
        const bool &mapAssetAllocationNotFound = result1.second;
        if(mapAssetAllocationNotFound){
            CAssetAllocation receiverAllocation;
            GetAssetAllocation(mintSyscoin.assetAllocationTuple, receiverAllocation);
            if (receiverAllocation.assetAllocationTuple.IsNull()) {           
                receiverAllocation.assetAllocationTuple.nAsset = std::move(mintSyscoin.assetAllocationTuple.nAsset);
                receiverAllocation.assetAllocationTuple.witnessAddress = std::move(mintSyscoin.assetAllocationTuple.witnessAddress);
            }
            mapAssetAllocation->second = std::move(receiverAllocation);             
        }
        mapAssetAllocation->second.nBalance += mintSyscoin.nValueAsset; 
        // update supply stats if index enabled
        if(fAssetSupplyStatsIndex){
            #if __cplusplus > 201402 
            auto result = mapAssetSupplyStats.try_emplace(nAsset,  std::move(emptyAssetSupplyStats));
            #else
            auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
            #endif  
            auto mapAssetSupplyStat = result.first;
            const bool &mapAssetSupplyStatsNotFound = result.second;
            if(mapAssetSupplyStatsNotFound && passetsupplystatsdb->ExistStats(nAsset)){
                CAssetSupplyStats dbAssetSupplyStats;
                if (!passetsupplystatsdb->ReadStats(nAsset, dbAssetSupplyStats)) {
                    errorMessage = "SYSCOIN_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Could not read asset supply stats");
                    return error(errorMessage.c_str());                  
                } 
                mapAssetSupplyStat->second = std::move(dbAssetSupplyStats);      
            }
            mapAssetSupplyStat->second.nBalanceBridge -= outputAmount;
            if(nAsset == Params().GetConsensus().nSYSXAsset)
                mapAssetSupplyStat->second.nBalanceSPT += outputAmount;
        }
    }
    if (!AssetRange(mintSyscoin.nValueAsset))
    {
        errorMessage = "SYSCOIN_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Amount out of money range");
        return false;
    }


    if(!fJustCheck && !bSanity && !bMiner)     
        passetallocationdb->WriteMintIndex(tx, mintSyscoin, nHeight, blockhash);         
                                
    return true;
}
bool CheckSyscoinInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache &inputs, const bool &fJustCheck, int nHeight, const bool &bSanity, bool& bSenderConflict)
{
    AssetAllocationMap mapAssetAllocations;
    AssetMap mapAssets;
    AssetSupplyStatsMap mapAssetSupplyStats;
    EthereumMintTxVec vecMintKeys;
    std::vector<COutPoint> vecLockedOutpoints;
    return CheckSyscoinInputs(false, tx, state, inputs, fJustCheck, bSenderConflict, nHeight, 0, uint256(), bSanity, false, mapAssetAllocations, mempoolMapAssetTXPrevOutPoints, mapAssets, mapAssetSupplyStats, vecMintKeys, vecLockedOutpoints);
}
bool CheckSyscoinInputs(const bool ibd, const CTransaction& tx, CValidationState& state, const CCoinsViewCache &inputs,  const bool &fJustCheck, bool &bSenderConflict, int nHeight, const uint32_t& nTime, const uint256 & blockHash, const bool &bSanity, const bool &bMiner, AssetAllocationMap &mapAssetAllocations, AssetTXPrevOutPointMap &mapAssetTXPrevOutPoints, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, EthereumMintTxVec &vecMintKeys, std::vector<COutPoint> &vecLockedOutpoints)
{
    if (nHeight == 0)
        nHeight = ::ChainActive().Height()+1;
    std::string errorMessage;
    bool good = true;
    bool bTxRootError = false;
    bSenderConflict=false;
    good = true;
    const bool& isBlock = !blockHash.IsNull();
    // reset MT mempool failure time during connectblock, ensure at least 30 seconds or latch on next block
    if(nLastMultithreadMempoolFailure > 0 && isBlock && (nTime > (nLastMultithreadMempoolFailure+30)) ){
        nLastMultithreadMempoolFailure = 0;
    }
    // find entry less than 30 seconds old out of vector to remove from mempool as part of zdag dbl spend relay logic
    if(isBlock && vecToRemoveFromMempool.size() > 0 && nTime > 0){
        LOCK2(cs_main, mempool.cs);
        LOCK(cs_assetallocationmempoolremovetx);
        for (auto rit = vecToRemoveFromMempool.begin(); rit != vecToRemoveFromMempool.end(); ++rit){
            if(nTime <= (rit->second+30) )
                break;
            const CTransactionRef &txRef = mempool.get(rit->first);
            if(txRef){
                const CTransaction &tx = *txRef;
                mempool.removeConflicts(tx);
                mempool.removeRecursive(tx, MemPoolRemovalReason::SYSCOINCONSENSUS);
                mempool.ClearPrioritisation(tx.GetHash());
                GetMainSignals().TransactionRemovedFromMempool(txRef);
            }
            vecToRemoveFromMempool.erase(rit);
            if(vecToRemoveFromMempool.size() == 0)
                break;
        }
    }    
    
    if(!IsSyscoinTx(tx.nVersion))
        return true;
    // fJustCheck inplace of bSanity to preserve global structures from being changed during test calls, fJustCheck is actually passed in as false because we want to check in PoW mode if blockhash isn't null
    const bool &bSanityInternal = blockHash.IsNull()? bSanity: fJustCheck;
    const bool &bJustCheckInternal = blockHash.IsNull()? fJustCheck: false;
    if (IsAssetAllocationTx(tx.nVersion))
    {
        good = CheckAssetAllocationInputs(tx, inputs, bJustCheckInternal, nHeight, blockHash, mapAssetSupplyStats, mapAssetAllocations, mapAssetTXPrevOutPoints, vecLockedOutpoints, errorMessage, bSenderConflict, bSanityInternal, bMiner);

    }
    else if (IsAssetTx(tx.nVersion))
    {
        good = CheckAssetInputs(tx, inputs, bJustCheckInternal, nHeight, blockHash, mapAssets, mapAssetAllocations, mapAssetTXPrevOutPoints, errorMessage, bSanityInternal, bMiner);
    } 
    else if(IsSyscoinMintTx(tx.nVersion))
    {
        if(nHeight <= Params().GetConsensus().nBridgeStartBlock){
            errorMessage = "Bridge is disabled";
            good = false;
        }
        else{
            good = CheckSyscoinMint(ibd, tx, errorMessage, bJustCheckInternal, bSanityInternal, bMiner, nHeight, blockHash, mapAssets, mapAssetSupplyStats, mapAssetAllocations, vecMintKeys, bTxRootError);
        }
    }              
    if (!good)
    {
        if (!errorMessage.empty()) {
            // if validation fails we should not include this transaction in a block
            if(bMiner){
                return state.Error(errorMessage);
            }
        }
    } 
    
    if (!good || !errorMessage.empty()){
        if(bTxRootError)
            return state.Error(errorMessage);
        else
            return state.Invalid(bSenderConflict? ValidationInvalidReason::TX_CONFLICT: ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, errorMessage);
    }
    return true;
}

void ResyncAssetAllocationStates(){ 
    int count = 0;
     {
        
        vector<string> vecToRemoveMempoolBalances;
        LOCK2(cs_main, mempool.cs);
        LOCK(cs_assetallocationmempoolbalance);
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
                const CTransactionRef &txRef = mempool.get(txHash);
                // if mempool doesn't have txid then remove from both arrivalTime and mempool balances
                if (!txRef){
                    vecToRemoveArrivalTimes.push_back(txHash);
                }
                else if(!arrivalTime.first.IsNull() && ((::ChainActive().Tip()->GetMedianTimePast()*1000) - arrivalTime.second) > 1800000){
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
            {
                LOCK(cs_assetallocationconflicts);
                unordered_set<string>::const_iterator it = assetAllocationConflicts.find(senderTuple);
                if (it != assetAllocationConflicts.end()) {
                    assetAllocationConflicts.erase(it);
                }
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
                    if(!arrivalTime.first.IsNull() && ((::ChainActive().Tip()->GetMedianTimePast()*1000) - arrivalTime.second) <= 1800000){
                        removeAllConflicts = false;
                        break;
                    }
                }
            }
            if(removeAllConflicts){
                LOCK(cs_assetallocationconflicts);
                if(arrivalTimes != arrivalTimesMap.end())
                    arrivalTimesMap.erase(arrivalTimes);
                unordered_set<string>::const_iterator it = assetAllocationConflicts.find(senderStr);
                if (it != assetAllocationConflicts.end()) {
                    assetAllocationConflicts.erase(it);
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
            LOCK(cs_assetallocationmempoolbalance);
            mempoolMapAssetBalances.erase(senderStr);
            if(!bCheckExpiryOnly){
                LOCK(cs_assetallocationconflicts);
                unordered_set<string>::const_iterator it = assetAllocationConflicts.find(senderStr);
                if (it != assetAllocationConflicts.end()) {
                    assetAllocationConflicts.erase(it);
                }  
            }
        }
    }
    

    return removeAllConflicts;
    
}
bool DisconnectMintAsset(const CTransaction &tx, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull())
    {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Cannot unserialize data inside of this transaction relating to an assetallocationmint\n");
        return false;
    }   
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    dev::RLP rlpTxPath(&vchTxPath);
    const uint32_t &nPath = rlpTxPath.toInt<uint32_t>(dev::RLP::VeryStrict);
    // remove eth spend tx from our internal db
    const std::pair<uint64_t, uint32_t> &ethKey = std::make_pair(mintSyscoin.nBlockNumber, nPath);
    vecMintKeys.emplace_back(ethKey);  
    // recver
    const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
    #if __cplusplus > 201402 
    auto result1 = mapAssetAllocations.try_emplace(std::move(receiverTupleStr),  std::move(emptyAllocation));
    #else
    auto result1 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
    #endif

    
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
    
    storedReceiverAllocationRef.nBalance -= mintSyscoin.nValueAsset;
    if(storedReceiverAllocationRef.nBalance < 0) {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Receiver balance of %s is negative: %lld\n",mintSyscoin.assetAllocationTuple.ToString(), storedReceiverAllocationRef.nBalance);
        return false;
    }       
    else if(storedReceiverAllocationRef.nBalance == 0){
        storedReceiverAllocationRef.SetNull();
    }
    if(fAssetIndex){
        const uint256& txid = tx.GetHash();
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), mintSyscoin.assetAllocationTuple.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(mintSyscoin.assetAllocationTuple, txid)){
                LogPrint(BCLog::SYS,"DisconnectMintAsset: Could not erase mint asset allocation from asset allocation index\n");
            }
            if(!passetindexdb->EraseIndexTXID(mintSyscoin.assetAllocationTuple.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectMintAsset: Could not erase mint asset allocation from asset index\n");
            } 
        }      
    }
    // update supply stats if index enabled
    if(fAssetSupplyStatsIndex){
        #if __cplusplus > 201402 
        auto result = mapAssetSupplyStats.try_emplace(mintSyscoin.assetAllocationTuple.nAsset,  std::move(emptyAssetSupplyStats));
        #else
        auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(mintSyscoin.assetAllocationTuple.nAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
        #endif  
        auto mapAssetSupplyStat = result.first;
        const bool &mapAssetSupplyStatsNotFound = result.second;
        if(mapAssetSupplyStatsNotFound && passetsupplystatsdb->ExistStats(mintSyscoin.assetAllocationTuple.nAsset)){
            CAssetSupplyStats dbAssetSupplyStats;
            if (!passetsupplystatsdb->ReadStats(mintSyscoin.assetAllocationTuple.nAsset, dbAssetSupplyStats)) {
                LogPrint(BCLog::SYS,"DisconnectMintAsset: Could not get asset supply stats %d\n",mintSyscoin.assetAllocationTuple.nAsset);
                return false;               
            } 
            mapAssetSupplyStat->second = std::move(dbAssetSupplyStats);      
        } 
        mapAssetSupplyStat->second.nBalanceBridge += mintSyscoin.nValueAsset;
        if(mintSyscoin.assetAllocationTuple.nAsset == Params().GetConsensus().nSYSXAsset)
            mapAssetSupplyStat->second.nBalanceSPT -= mintSyscoin.nValueAsset;
        
    } 
    return true; 
}
bool DisconnectAssetAllocation(const CTransaction &tx, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations){
    const uint256& txid = tx.GetHash();
    CAssetAllocation theAssetAllocation(tx);

    const std::string &senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not decode asset allocation\n");
        return false;
    }
    #if __cplusplus > 201402 
    auto result = mapAssetAllocations.try_emplace(std::move(senderTupleStr),  std::move(emptyAllocation));
    #else
    auto result = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
    #endif
    
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
    CAmount nTotal = 0;
    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
       
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocation receiverAllocation;
        #if __cplusplus > 201402 
        auto result1 = mapAssetAllocations.try_emplace(std::move(receiverTupleStr),  std::move(emptyAllocation));
        #else
        auto result1 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
        #endif

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
        nTotal += amountTuple.second;
        if(storedReceiverAllocationRef.nBalance < 0) {
            LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Receiver balance of %s is negative: %lld\n",receiverAllocationTuple.ToString(), storedReceiverAllocationRef.nBalance);
            return false;
        }
        else if(storedReceiverAllocationRef.nBalance == 0){
            storedReceiverAllocationRef.SetNull();  
        }
        if(fAssetIndex){
            if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), receiverAllocationTuple.nAsset) != fAssetIndexGuids.end()){
                if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple, txid)){
                    LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not erase receiver allocation from asset allocation index\n");
                }
                if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple.nAsset, txid)){
                    LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not erase receiver allocation from asset index\n");
                }
            }
        }                                       
    }
    if(fAssetIndex){
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), theAssetAllocation.assetAllocationTuple.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not erase sender allocation from asset allocation index\n");
            }
            if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not erase sender allocation from asset index\n");
            }
        }     
    }
    // update supply stats if index enabled
    if(fAssetSupplyStatsIndex && 
    (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM ||
    tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN ||
    tx.nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION)){
        #if __cplusplus > 201402 
        auto result = mapAssetSupplyStats.try_emplace(theAssetAllocation.assetAllocationTuple.nAsset,  std::move(emptyAssetSupplyStats));
        #else
        auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.assetAllocationTuple.nAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
        #endif  
        auto mapAssetSupplyStat = result.first;
        const bool &mapAssetSupplyStatsNotFound = result.second;
        if(mapAssetSupplyStatsNotFound && passetsupplystatsdb->ExistStats(theAssetAllocation.assetAllocationTuple.nAsset)){
            CAssetSupplyStats dbAssetSupplyStats;
            if (!passetsupplystatsdb->ReadStats(theAssetAllocation.assetAllocationTuple.nAsset, dbAssetSupplyStats)) {
                LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not get asset supply stats %d\n",theAssetAllocation.assetAllocationTuple.nAsset);
                return false;               
            } 
            mapAssetSupplyStat->second = std::move(dbAssetSupplyStats);
        }
        if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
            mapAssetSupplyStat->second.nBalanceBridge -= nTotal;
        }
        if(theAssetAllocation.assetAllocationTuple.nAsset == Params().GetConsensus().nSYSXAsset)
            mapAssetSupplyStat->second.nBalanceSPT -= nTotal;
        
    
             
    }       
    return true; 
}
CAmount FindBurnAmountFromTx(const CTransaction& tx){
    for(const auto& out: tx.vout){
        if(out.scriptPubKey.IsUnspendable())
            return out.nValue;
    }
    return 0;
}
bool CheckAssetAllocationInputs(const CTransaction &tx, const CCoinsViewCache &inputs,
        bool fJustCheck, int nHeight, const uint256& blockhash, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, AssetTXPrevOutPointMap &mapAssetTXPrevOutPoints, std::vector<COutPoint> &vecLockedOutpoints, string &errorMessage, bool& bSenderConflict, const bool &bSanityCheck, const bool &bMiner) {
    if (passetallocationdb == nullptr)
        return false;
    const uint256 & txHash = tx.GetHash();
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %d %s %s\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
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
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
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
			if (!theAssetAllocation.lockedOutpoint.IsNull())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Cannot include locked outpoint information for allocation send");
				return error(errorMessage.c_str());
			}
            break; 
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
			if (!theAssetAllocation.lockedOutpoint.IsNull())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Cannot include locked outpoint information for allocation burn");
				return error(errorMessage.c_str());
			}
            if(theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Must include burn allocation to this transaction");
				return error(errorMessage.c_str());
			}
            break;            
		case SYSCOIN_TX_VERSION_ALLOCATION_LOCK:
			if (theAssetAllocation.lockedOutpoint.IsNull())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Asset allocation lock must include outpoint information");
				return error(errorMessage.c_str());
			}
			break;
        default:
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1009 - " + _("Asset transaction has unknown op");
            return error(errorMessage.c_str());
        }
    }

    const CWitnessAddress &user1 = theAssetAllocation.assetAllocationTuple.witnessAddress;
    const string & senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();
    const string &user1Str = user1.ToString();
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
        #if __cplusplus > 201402 
        auto result = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
        #else
        auto result = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
        #endif
        
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
    if(fJustCheck && !bSanityCheck){
        LOCK(cs_assetallocationmempoolbalance); 
        #if __cplusplus > 201402 
        auto result = mempoolMapAssetBalances.try_emplace(senderTupleStr,  std::move(storedSenderAllocationRef.nBalance));
        #else
        auto result =  mempoolMapAssetBalances.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(storedSenderAllocationRef.nBalance))); 
        #endif
        
        mapBalanceSender = result.first;
        mapSenderMempoolBalanceNotFound = result.second;
        mapBalanceSenderCopy = mapBalanceSender->second;
    }
    else
        mapBalanceSenderCopy = storedSenderAllocationRef.nBalance;

    // ensure output linking
    AssetTXPrevOutPointMap::iterator mapAssetTXPrevOutPoint;
    bool mapAssetTXPrevOutPointNotFound;
    {
        // ensure output linking
        LOCK(cs_assetallocationprevout); 
        #if __cplusplus > 201402 
        auto result = mapAssetTXPrevOutPoints.try_emplace(user1Str,  emptyOutPoint);
        #else
        auto result  = mapAssetTXPrevOutPoints.emplace(std::piecewise_construct,  std::forward_as_tuple(user1Str),  std::forward_as_tuple(emptyOutPoint));
        #endif  
        mapAssetTXPrevOutPoint = result.first;
        mapAssetTXPrevOutPointNotFound = storedSenderAllocationRef.lockedOutpoint.IsNull()? result.second: false;
    }          
    if(tx.nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION){
        const uint32_t &nBurnAsset = theAssetAllocation.assetAllocationTuple.nAsset;
        if(!fUnitTest && nBurnAsset != Params().GetConsensus().nSYSXAsset)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid Syscoin Bridge Asset GUID specified");
            return error(errorMessage.c_str());
        }
        if (!FindAssetOwnerInTx(inputs, tx, theAssetAllocation.listSendingAllocationAmounts[0].first,mapAssetTXPrevOutPointNotFound? storedSenderAllocationRef.lockedOutpoint: mapAssetTXPrevOutPoint->second))
        {
            // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot move syscoin to this asset. Receiver of Asset Allocation must sign off on this change");
            return error(errorMessage.c_str());
        }
        // ensure lockedOutpoint is cleared on PoW, it is useful only once typical for atomic scripts like CLTV based atomic swaps or hashlock type of usecases
		if (!bSanityCheck && !fJustCheck && !storedSenderAllocationRef.lockedOutpoint.IsNull()) {
			// this will flag the batch write function on plockedoutpointsdb to erase this outpoint
			vecLockedOutpoints.emplace_back(emptyPoint);
			storedSenderAllocationRef.lockedOutpoint.SetNull();
		}              
        const int &nOut = GetSyscoinDataOutput(tx);
        if(nOut < 0)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Could not find burn output in transaction");
            return error(errorMessage.c_str());
        }
        const CAmount &nBurnAmount = tx.vout[nOut].nValue;
        if(nBurnAmount <= 0)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid burn amount found in transaction");
            return error(errorMessage.c_str());
        }
        if(nBurnAmount != theAssetAllocation.listSendingAllocationAmounts[0].second)
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid burn amount found in transaction");
            return error(errorMessage.c_str());
        }  
        if(user1 != CWitnessAddress(0, vchFromString("burn")))
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid burn sender address found in transaction");
            return error(errorMessage.c_str());
        }

        if (nBurnAmount <= 0 || nBurnAmount > dbAsset.nMaxSupply)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Amount out of money range");
            return error(errorMessage.c_str());
        }        
       
        mapBalanceSenderCopy -= nBurnAmount;
        if (mapBalanceSenderCopy < 0) {
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
        if (!fJustCheck) {   
            const CAssetAllocationTuple receiverAllocationTuple(nBurnAsset, theAssetAllocation.listSendingAllocationAmounts[0].first);
            const string& receiverTupleStr = receiverAllocationTuple.ToString(); 
            #if __cplusplus > 201402 
            auto resultReceiver = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
            #else
            auto resultReceiver = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
            #endif 
            
            auto mapAssetAllocationReceiver = resultReceiver.first;
            const bool& mapAssetAllocationReceiverNotFound = resultReceiver.second;
            if(mapAssetAllocationReceiverNotFound){
                CAssetAllocation dbAssetAllocationReceiver;
                if (!GetAssetAllocation(receiverAllocationTuple, dbAssetAllocationReceiver)) {               
                    dbAssetAllocationReceiver.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                    dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);              
                }
                mapAssetAllocationReceiver->second = std::move(dbAssetAllocationReceiver);               
            } 
            mapAssetAllocationReceiver->second.nBalance += nBurnAmount; 

            // update supply stats if index enabled
            if(fAssetSupplyStatsIndex){
                #if __cplusplus > 201402 
                auto result = mapAssetSupplyStats.try_emplace(nBurnAsset,  std::move(emptyAssetSupplyStats));
                #else
                auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(nBurnAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
                #endif  
                auto mapAssetSupplyStat = result.first;
                const bool &mapAssetSupplyStatsNotFound = result.second;
                if(mapAssetSupplyStatsNotFound && passetsupplystatsdb->ExistStats(nBurnAsset)){
                    CAssetSupplyStats dbAssetSupplyStats;
                    if (!passetsupplystatsdb->ReadStats(nBurnAsset, dbAssetSupplyStats)) {
                        errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Could not read asset supply stats");
                        return error(errorMessage.c_str());            
                    } 
                    mapAssetSupplyStat->second = std::move(dbAssetSupplyStats);      
                }
                if(nBurnAsset == Params().GetConsensus().nSYSXAsset)
                    mapAssetSupplyStat->second.nBalanceSPT += nBurnAmount; 
            }                          
        } 
    }          
    if (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM || tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN)
    {     
        const uint32_t &nBurnAsset = theAssetAllocation.assetAllocationTuple.nAsset;
        const CAmount &nBurnAmount = theAssetAllocation.listSendingAllocationAmounts[0].second;
        if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN){
            if(!fUnitTest && nBurnAsset != Params().GetConsensus().nSYSXAsset)
            {
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid Syscoin Bridge Asset GUID specified");
                return error(errorMessage.c_str());
            }         
        }
       
        if(theAssetAllocation.listSendingAllocationAmounts[0].first != CWitnessAddress(0, vchFromString("burn")))
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Invalid burn address found in transaction");
            return error(errorMessage.c_str());
        } 
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, mapAssetTXPrevOutPointNotFound? storedSenderAllocationRef.lockedOutpoint: mapAssetTXPrevOutPoint->second))
        {
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }       
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
            return error(errorMessage.c_str());
        }       
        if (nBurnAmount <= 0 || nBurnAmount > dbAsset.nTotalSupply)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Amount out of money range");
            return error(errorMessage.c_str());
        }        
  		// ensure lockedOutpoint is cleared on PoW, it is useful only once typical for atomic scripts like CLTV based atomic swaps or hashlock type of usecases
		if (!bSanityCheck && !fJustCheck && !storedSenderAllocationRef.lockedOutpoint.IsNull()) {
			// this will flag the batch write function on plockedoutpointsdb to erase this outpoint
			vecLockedOutpoints.emplace_back(emptyPoint);
			storedSenderAllocationRef.lockedOutpoint.SetNull();
		}      
        if(dbAsset.vchContract.empty())
        {
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Cannot burn, no contract provided in asset by owner");
            return error(errorMessage.c_str());
        } 
       
        mapBalanceSenderCopy -= nBurnAmount;
        if (mapBalanceSenderCopy < 0) {
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;  
            }
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
        if (!fJustCheck) {   
            const CAssetAllocationTuple receiverAllocationTuple(nBurnAsset,  CWitnessAddress(0, vchFromString("burn")));
            const string& receiverTupleStr = receiverAllocationTuple.ToString(); 
            #if __cplusplus > 201402 
            auto resultReceiver = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
            #else
            auto resultReceiver = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
            #endif 
            
            auto mapAssetAllocationReceiver = resultReceiver.first;
            const bool& mapAssetAllocationReceiverNotFound = resultReceiver.second;
            if(mapAssetAllocationReceiverNotFound){
                CAssetAllocation dbAssetAllocationReceiver;
                if (!GetAssetAllocation(receiverAllocationTuple, dbAssetAllocationReceiver)) {               
                    dbAssetAllocationReceiver.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                    dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);              
                }
                mapAssetAllocationReceiver->second = std::move(dbAssetAllocationReceiver);                  
            } 
            mapAssetAllocationReceiver->second.nBalance += nBurnAmount;
            // update supply stats if index enabled
            if(fAssetSupplyStatsIndex){
                #if __cplusplus > 201402 
                auto result = mapAssetSupplyStats.try_emplace(nBurnAsset,  std::move(emptyAssetSupplyStats));
                #else
                auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(nBurnAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
                #endif  
                auto mapAssetSupplyStat = result.first;
                const bool &mapAssetSupplyStatsNotFound = result.second;
                if(mapAssetSupplyStatsNotFound && passetsupplystatsdb->ExistStats(nBurnAsset)){
                    CAssetSupplyStats dbAssetSupplyStats;
                    if (!passetsupplystatsdb->ReadStats(nBurnAsset, dbAssetSupplyStats)) {
                        errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Could not read asset supply stats");
                        return error(errorMessage.c_str());                  
                    } 
                    mapAssetSupplyStat->second = std::move(dbAssetSupplyStats);      
                }
                if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
                    mapAssetSupplyStat->second.nBalanceBridge += nBurnAmount;
                }
                if(nBurnAsset == Params().GetConsensus().nSYSXAsset)
                    mapAssetSupplyStat->second.nBalanceSPT -= nBurnAmount;
            }                                   
        }
    }
	else if (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK)
	{
		if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, mapAssetTXPrevOutPointNotFound? storedSenderAllocationRef.lockedOutpoint: mapAssetTXPrevOutPoint->second))
		{       
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }           
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015a - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
			return error(errorMessage.c_str());
		}
        if (!bSanityCheck && !fJustCheck){
    		storedSenderAllocationRef.lockedOutpoint = theAssetAllocation.lockedOutpoint;
    		// this will batch write the outpoint in the calling function, we save the outpoint so that we cannot spend this outpoint without creating an SYSCOIN_TX_VERSION_ALLOCATION_SEND transaction
    		vecLockedOutpoints.emplace_back(std::move(theAssetAllocation.lockedOutpoint));
        }
	}
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND)
	{
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, mapAssetTXPrevOutPointNotFound? storedSenderAllocationRef.lockedOutpoint: mapAssetTXPrevOutPoint->second))
        {       
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1015a - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
            return error(errorMessage.c_str());
        }
		// ensure lockedOutpoint is cleared on PoW if it was set once a send happens, it is useful only once typical for atomic scripts like CLTV based atomic swaps or hashlock type of usecases
		if (!bSanityCheck && !fJustCheck && !storedSenderAllocationRef.lockedOutpoint.IsNull()) {
			// this will flag the batch write function on plockedoutpointsdb to erase this outpoint
			vecLockedOutpoints.emplace_back(emptyPoint);
			storedSenderAllocationRef.lockedOutpoint.SetNull();
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
           // check to make sure mapSenderMempoolBalanceNotFound is false, this ensures that we have a sender tx competing with this one (its not the first one we've seen)
            if(!mapSenderMempoolBalanceNotFound && fJustCheck && !bSanityCheck && !bMiner)
            {
                LOCK(cs_assetallocationconflicts);
                // add conflicting sender
                assetAllocationConflicts.insert(senderTupleStr);  
                bSenderConflict = true;   
            }      
            errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1021 - " + _("Sender balance is insufficient");
            return error(errorMessage.c_str());
        }
               
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            if (amountTuple.first == theAssetAllocation.assetAllocationTuple.witnessAddress) {        
                errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1022 - " + _("Cannot send an asset allocation to yourself");
                return error(errorMessage.c_str());
            }
        
            const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
            const string &receiverTupleStr = receiverAllocationTuple.ToString();
            AssetBalanceMap::iterator mapBalanceReceiver;
            AssetAllocationMap::iterator mapBalanceReceiverBlock;            
            if(fJustCheck && !bSanityCheck){
                LOCK(cs_assetallocationmempoolbalance);
                #if __cplusplus > 201402 
                auto result1 = mempoolMapAssetBalances.try_emplace(std::move(receiverTupleStr),  0);
                #else
                auto result1 = mempoolMapAssetBalances.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(0));
                #endif 

                auto mapBalanceReceiver = result1.first;
                const bool& mapAssetAllocationReceiverNotFound = result1.second;
                if(mapAssetAllocationReceiverNotFound){
                    CAssetAllocation receiverAllocation;
                    GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
                    mapBalanceReceiver->second = receiverAllocation.nBalance;
                }
                if(!bSanityCheck){
                    mapBalanceReceiver->second += amountTuple.second;
                }
            }  
            else{     
                #if __cplusplus > 201402 
                auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
                #else
                auto result1 =  mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
                #endif       
               
                auto mapBalanceReceiverBlock = result1.first;
                const bool& mapAssetAllocationReceiverBlockNotFound = result1.second;
                if(mapAssetAllocationReceiverBlockNotFound){
                    CAssetAllocation receiverAllocation;
                    if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {                   
                        receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                        receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);                       
                    }
                    mapBalanceReceiverBlock->second = std::move(receiverAllocation);  
                }
                mapBalanceReceiverBlock->second.nBalance += amountTuple.second;
                if(!fJustCheck){ 
                    // to remove mempool balances but need to check to ensure that all txid's from arrivalTimes are first gone before removing receiver mempool balance
                    // otherwise one can have a conflict as a sender and send himself an allocation and clear the mempool balance inadvertently
                    ResetAssetAllocation(receiverTupleStr, txHash, bMiner);      
                } 
            }
        }   
    }
    // write assetallocation  
    // asset sends are the only ones confirming without PoW
    if(!fJustCheck){
        // lock tx does not do any mempool balance or sender conflict stuff
        if (!bSanityCheck) {
            ResetAssetAllocation(senderTupleStr, txHash, bMiner);
           
        } 
        storedSenderAllocationRef.listSendingAllocationAmounts.clear();
        storedSenderAllocationRef.nBalance = std::move(mapBalanceSenderCopy);
        if(storedSenderAllocationRef.nBalance == 0)
            storedSenderAllocationRef.SetNull();    

        if(!bMiner) {   
            // send notification on pow, for zdag transactions this is the second notification meaning the zdag tx has been confirmed
            passetallocationdb->WriteAssetAllocationIndex(tx, dbAsset, nHeight, blockhash);  
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                senderTupleStr.c_str(),
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);      
        }
                    
    }
    else if(!bSanityCheck){
		
        LOCK(cs_assetallocationarrival);
        ArrivalTimesMap &arrivalTimes = arrivalTimesMap[senderTupleStr];
        arrivalTimes[txHash] = GetTimeMillis();       

        if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND){
            // send a realtime notification on zdag, send another when pow happens (above)
            passetallocationdb->WriteAssetAllocationIndex(tx, dbAsset, nHeight, blockhash);
        }
        
        {
            LOCK(cs_assetallocationmempoolbalance);
            mapBalanceSender->second = std::move(mapBalanceSenderCopy);
        }
    }
    if(!bMiner && !bSanityCheck){
        LOCK(cs_assetallocationprevout);
        // set prev out so subsequent tx from this can link to it
        // change should be last output which is the one linked
        mapAssetTXPrevOutPoint->second = COutPoint(txHash, tx.vout.size()-1);
    }     
    return true;
}

bool DisconnectAssetSend(const CTransaction &tx, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations){
    const uint256 &txid = tx.GetHash();
    CAsset dbAsset;
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not decode asset allocation in asset send\n");
        return false;
    } 
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(theAssetAllocation.assetAllocationTuple.nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.assetAllocationTuple.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif   
   
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAssetAllocation.assetAllocationTuple.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not get asset %d\n",theAssetAllocation.assetAllocationTuple.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                        
    }
    CAsset& storedSenderRef = mapAsset->second;
               
               
    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocation receiverAllocation;
        #if __cplusplus > 201402 
        auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr),  std::move(emptyAllocation));
        #else
        auto result = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
        #endif 
        
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
            if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), receiverAllocationTuple.nAsset) != fAssetIndexGuids.end()){
                if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple, txid)){
                    LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not erase receiver allocation from asset allocation index\n");
                }
                if(!passetindexdb->EraseIndexTXID(receiverAllocationTuple.nAsset, txid)){
                    LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not erase receiver allocation from asset index\n");
                } 
            }
        }                                             
    }     
    if(fAssetIndex){
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), theAssetAllocation.assetAllocationTuple.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not erase sender allocation from asset allocation index\n");
            }
            if(!passetindexdb->EraseIndexTXID(theAssetAllocation.assetAllocationTuple.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not erase sender allocation from asset index\n");
            }
        }     
    }          
    return true;  
}
bool DisconnectAssetUpdate(const CTransaction &tx, AssetMap &mapAssets){
    
    CAsset dbAsset;
    CAsset theAsset(tx);
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not decode asset\n");
        return false;
    }
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(theAsset.nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAsset.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif     

    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not get asset %d\n",theAsset.nAsset);
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
            LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Asset cannot be negative: Balance %lld, Supply: %lld\n",storedSenderRef.nBalance, storedSenderRef.nTotalSupply);
            return false;
        }                                          
    } 
    if(fAssetIndex){
        const uint256 &txid = tx.GetHash();
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), theAsset.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(theAsset.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not erase asset update from asset index\n");
            }
        }
    }         
    return true;  
}
bool DisconnectAssetTransfer(const CTransaction &tx, AssetMap &mapAssets){
    
    CAsset dbAsset;
    CAsset theAsset(tx);
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetTransfer: Could not decode asset\n");
        return false;
    }
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(theAsset.nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAsset.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif  
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetTransfer: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                    
    }
    CAsset& storedSenderRef = mapAsset->second; 
    // theAsset.witnessAddress  is enforced to be the sender of the transfer which was the owner at the time of transfer
    // so set it back to reverse the transfer
    storedSenderRef.witnessAddress = theAsset.witnessAddress;   
    if(fAssetIndex){
        const uint256 &txid = tx.GetHash();
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), theAsset.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(theAsset.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetTransfer: Could not erase asset update from asset index\n");
            }
        }
    }         
    return true;  
}
bool DisconnectAssetActivate(const CTransaction &tx, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats){
    
    CAsset theAsset(tx);
    
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetActivate: Could not decode asset in asset activate\n");
        return false;
    }
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(theAsset.nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAsset.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif  
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        CAsset dbAsset;
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetActivate: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);      
    }
    mapAsset->second.SetNull();  
    if(fAssetIndex){
        const uint256 &txid = tx.GetHash();
        if(fAssetIndexGuids.empty() || std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), theAsset.nAsset) != fAssetIndexGuids.end()){
            if(!passetindexdb->EraseIndexTXID(theAsset.nAsset, txid)){
                LogPrint(BCLog::SYS,"DisconnectAssetActivate: Could not erase asset activate from asset index\n");
            }
        }    
    } 
    // remove supply stats if index enabled
    if(fAssetSupplyStatsIndex){
        #if __cplusplus > 201402 
        auto result = mapAssetSupplyStats.try_emplace(theAsset.nAsset,  std::move(emptyAssetSupplyStats));
        #else
        auto result  = mapAssetSupplyStats.emplace(std::piecewise_construct,  std::forward_as_tuple(theAsset.nAsset),  std::forward_as_tuple(std::move(emptyAssetSupplyStats)));
        #endif  
        auto mapAssetSupplyStat = result.first;
        mapAssetSupplyStat->second.SetNull();
    }      
    return true;  
}
bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs,
        bool fJustCheck, int nHeight, const uint256& blockhash, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations, AssetTXPrevOutPointMap &mapAssetTXPrevOutPoints, string &errorMessage, const bool &bSanityCheck, const bool &bMiner) {
    if (passetdb == nullptr)
        return false;
    const uint256& txHash = tx.GetHash();
    if (!bSanityCheck)
        LogPrint(BCLog::SYS, "*** ASSET %d %d %s %s\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
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
            if(!fUnitTest && nHeight >= Params().GetConsensus().nBridgeStartBlock && tx.vout[nDataOut].nValue < 500*COIN)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Insufficient fees included to create asset (You need to pay at least 500 SYS).");
                return error(errorMessage.c_str());               
            }
            if (theAsset.nAsset <= SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN)
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
            if (theAsset.strSymbol.size() > 8 || theAsset.strSymbol.size() < 1)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Symbol must be between 1 and 8");
                return error(errorMessage.c_str());
            }
            if (!AssetRange(theAsset.nMaxSupply, theAsset.nPrecision))
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
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot include transfer address upon activation");
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
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot include transfer address upon update");
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
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot include transfer address upon sending asset");
                return error(errorMessage.c_str());
            }  
            break;
        case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
            if(theAsset.witnessAddressTransfer.IsNull())   {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Must include transfer address upon transferring asset");
                return error(errorMessage.c_str());
            } 
            break;
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2023 - " + _("Asset transaction has unknown op");
            return error(errorMessage.c_str());
        }
    }

    CAsset dbAsset;
    const uint32_t &nAsset = tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND ? theAssetAllocation.assetAllocationTuple.nAsset : theAsset.nAsset;
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif  
    auto mapAsset = result.first;
    const bool & mapAssetNotFound = result.second;    
    if (mapAssetNotFound)
    {
        if (!GetAsset(nAsset, dbAsset)){
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
    // ensure output linking
    AssetTXPrevOutPointMap::iterator mapAssetTXPrevOutPoint;
    bool mapAssetTXPrevOutPointNotFound;
    {
        const std::string & senderStr = storedSenderAssetRef.witnessAddress.ToString();
        // ensure output linking
        LOCK(cs_assetallocationprevout); 
        #if __cplusplus > 201402 
        auto result = mapAssetTXPrevOutPoints.try_emplace(senderStr,  emptyOutPoint);
        #else
        auto result  = mapAssetTXPrevOutPoints.emplace(std::piecewise_construct,  std::forward_as_tuple(senderStr),  std::forward_as_tuple(emptyOutPoint));
        #endif  
        mapAssetTXPrevOutPoint = result.first;
        mapAssetTXPrevOutPointNotFound = result.second;
    }     
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER) {
        if (theAsset.nAsset != storedSenderAssetRef.nAsset || storedSenderAssetRef.witnessAddress != theAsset.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress, mapAssetTXPrevOutPointNotFound? std::move(emptyOutPoint): mapAssetTXPrevOutPoint->second))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot transfer this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        } 
		if(theAsset.nPrecision != storedSenderAssetRef.nPrecision)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot transfer this asset. Precision cannot be changed.");
			return error(errorMessage.c_str());
		}
        if(theAsset.strSymbol != storedSenderAssetRef.strSymbol)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot transfer this asset. Symbol cannot be changed.");
            return error(errorMessage.c_str());
        }        
        storedSenderAssetRef.witnessAddress = theAsset.witnessAddressTransfer;   
        // sanity to ensure transfer field is never set on the actual asset in db  
        storedSenderAssetRef.witnessAddressTransfer.SetNull();      
    }

    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {
        if (theAsset.nAsset != storedSenderAssetRef.nAsset || storedSenderAssetRef.witnessAddress != theAsset.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress, mapAssetTXPrevOutPointNotFound? std::move(emptyOutPoint): mapAssetTXPrevOutPoint->second))
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Asset owner must sign off on this change");
            return error(errorMessage.c_str());
        }
		if (theAsset.nPrecision != storedSenderAssetRef.nPrecision)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Precision cannot be changed.");
			return error(errorMessage.c_str());
		}
        if(theAsset.strSymbol != storedSenderAssetRef.strSymbol)
        {
            errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Symbol cannot be changed.");
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
		if (!theAsset.vchPubData.empty()) {
			if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_DATA))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update public data");
				return error(errorMessage.c_str());
			}
			storedSenderAssetRef.vchPubData = theAsset.vchPubData;
		}
                                    
		if (!theAsset.vchContract.empty() && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_TRANSFER) {
			if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update smart contract");
				return error(errorMessage.c_str());
			}
			storedSenderAssetRef.vchContract = theAsset.vchContract;
		}
 
        if (theAsset.nUpdateFlags > 0) {
			if (!(storedSenderAssetRef.nUpdateFlags & (ASSET_UPDATE_FLAGS | ASSET_UPDATE_ADMIN))) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2040 - " + _("Insufficient privileges to update flags");
				return error(errorMessage.c_str());
			}
			storedSenderAssetRef.nUpdateFlags = theAsset.nUpdateFlags;
        } 
    }      
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
        if (storedSenderAssetRef.nAsset != theAssetAllocation.assetAllocationTuple.nAsset || storedSenderAssetRef.witnessAddress != theAssetAllocation.assetAllocationTuple.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress, mapAssetTXPrevOutPointNotFound? std::move(emptyOutPoint): mapAssetTXPrevOutPoint->second))
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
                #if __cplusplus > 201402 
                auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr),  std::move(emptyAllocation));
                #else
                auto result = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
                #endif 
                
                auto mapAssetAllocation = result.first;
                const bool& mapAssetAllocationNotFound = result.second;
               
                if(mapAssetAllocationNotFound){
                    GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
                    if (receiverAllocation.assetAllocationTuple.IsNull()) {
                        receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                        receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);                       
                    } 
                    mapAssetAllocation->second = std::move(receiverAllocation);                   
                }
				// adjust receiver balance
                mapAssetAllocation->second.nBalance += amountTuple.second;
                                        
                // adjust sender balance
                storedSenderAssetRef.nBalance -= amountTuple.second;                              
            }
        }
        if (!bSanityCheck && !fJustCheck && !bMiner)
            passetallocationdb->WriteAssetAllocationIndex(tx, storedSenderAssetRef, nHeight, blockhash);  
    }
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE)
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
        passetdb->WriteAssetIndex(tx, storedSenderAssetRef, nHeight, blockhash);
        LogPrint(BCLog::SYS,"CONNECTED ASSET: tx=%s symbol=%d hash=%s height=%d fJustCheck=%d\n",
                assetFromTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);
    }
    if(!bMiner && !bSanityCheck){
        LOCK(cs_assetallocationprevout);
        // set prev out so subsequent tx from this can link to it
        // change should be last output which is the one linked
        mapAssetTXPrevOutPoint->second = COutPoint(txHash, tx.vout.size()-1);
    }  
    return true;
}
bool CBlockIndexDB::FlushErase(const std::vector<uint256> &vecTXIDs){
    if(vecTXIDs.empty())
        return true;

    CDBBatch batch(*this);
    for (const uint256 &txid : vecTXIDs) {
        batch.Erase(txid);
    }
    LogPrint(BCLog::SYS, "Flushing %d block index removals\n", vecTXIDs.size());
    return WriteBatch(batch);
}
bool CBlockIndexDB::FlushWrite(const std::vector<std::pair<uint256, uint256> > &blockIndex){
    if(blockIndex.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &pair : blockIndex) {
        batch.Write(pair.first, pair.second);
    }
    LogPrint(BCLog::SYS, "Flush writing %d block indexes\n", blockIndex.size());
    return WriteBatch(batch);
}
bool CLockedOutpointsDB::FlushErase(const std::vector<COutPoint> &lockedOutpoints) {
	if (lockedOutpoints.empty())
		return true;

	CDBBatch batch(*this);
	for (const auto &outpoint : lockedOutpoints) {
		batch.Erase(outpoint);
	}
	LogPrint(BCLog::SYS, "Flushing %d locked outpoints removals\n", lockedOutpoints.size());
	return WriteBatch(batch);
}
bool CLockedOutpointsDB::FlushWrite(const std::vector<COutPoint> &lockedOutpoints) {
	if (lockedOutpoints.empty())
		return true;
	CDBBatch batch(*this);
	int write = 0;
	int erase = 0;
	for (const auto &outpoint : lockedOutpoints) {
		if (outpoint.IsNull()) {
			erase++;
			batch.Erase(outpoint);
		}
		else {
			write++;
			batch.Write(outpoint, true);
		}
	}
	LogPrint(BCLog::SYS, "Flushing %d locked outpoints (erased %d, written %d)\n", lockedOutpoints.size(), erase, write);
	return WriteBatch(batch);
}
bool CheckSyscoinLockedOutpoints(const CTransactionRef &tx, CValidationState& state) {
	// SYSCOIN
	const CTransaction &myTx = (*tx);
    bool assetAllocationVersion = IsAssetAllocationTx(myTx.nVersion);
    CAssetAllocation theAssetAllocation(myTx);
	// if not an allocation send ensure the outpoint locked isn't being spent
	if (!assetAllocationVersion && theAssetAllocation.assetAllocationTuple.IsNull()) {
		for (unsigned int i = 0; i < myTx.vin.size(); i++)
		{
			bool locked = false;
			// spending as non allocation send while using a locked outpoint should be invalid
			if (plockedoutpointsdb && plockedoutpointsdb->ReadOutpoint(myTx.vin[i].prevout, locked) && locked) {
                return state.Invalid(ValidationInvalidReason::TX_MISSING_INPUTS, false, REJECT_INVALID, "non-allocation-input");
			}
		}
	}
	// ensure that the locked outpoint is being spent
	else if(assetAllocationVersion){
		CAssetAllocation assetAllocationDB;
		if (!GetAssetAllocation(theAssetAllocation.assetAllocationTuple, assetAllocationDB)) {
            return state.Invalid(ValidationInvalidReason::TX_MISSING_INPUTS, false, REJECT_INVALID, "non-existing-allocation");
		}
		bool found = assetAllocationDB.lockedOutpoint.IsNull();
        
		for (unsigned int i = 0; i < myTx.vin.size(); i++)
		{
			bool locked = false;
			// spending as allocation send while using a locked outpoint should be invalid if tx doesn't include locked outpoint
			if (!found && assetAllocationDB.lockedOutpoint == myTx.vin[i].prevout && plockedoutpointsdb && plockedoutpointsdb->ReadOutpoint(myTx.vin[i].prevout, locked) && locked) {
				found = true;
				break;
			}
		}
		if (!found) {
            return state.Invalid(ValidationInvalidReason::TX_MISSING_INPUTS, false, REJECT_INVALID, "missing-lockpoint");
		}
	}
	return true;
}
bool CEthereumTxRootsDB::PruneTxRoots(const uint32_t &fNewGethSyncHeight) {
    uint32_t fNewGethCurrentHeight = fGethCurrentHeight;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    uint32_t cutoffHeight = 0;
    if(fNewGethSyncHeight > 0)
    {
        const uint32_t &nCutoffHeight = MAX_ETHEREUM_TX_ROOTS*3;
        // cutoff to keep blocks is ~3 week of blocks is about 120k blocks
        cutoffHeight = fNewGethSyncHeight - nCutoffHeight;
        if(fNewGethSyncHeight < nCutoffHeight){
            LogPrint(BCLog::SYS, "Nothing to prune fGethSyncHeight = %d\n", fNewGethSyncHeight);
            return true;
        }
    }
    std::vector<unsigned char> txPos;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if(pcursor->GetKey(nKey)){
                // if height is before cutoff height or after tip height passed in (re-org), remove the txroot from db
                if (fNewGethSyncHeight > 0 && (nKey < cutoffHeight || nKey > fNewGethSyncHeight)) {
                    vecHeightKeys.emplace_back(nKey);
                }
                else if(nKey > fNewGethCurrentHeight)
                    fNewGethCurrentHeight = nKey;
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }

    {
        LOCK(cs_ethsyncheight);
        fGethSyncHeight = fNewGethSyncHeight;
        fGethCurrentHeight = fNewGethCurrentHeight;
    }      
    return FlushErase(vecHeightKeys);
}
bool CEthereumTxRootsDB::Init(){
    return PruneTxRoots(0);
}
bool CEthereumTxRootsDB::Clear(){
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if(pcursor->GetKey(nKey)){
                vecHeightKeys.emplace_back(nKey);
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }

    {
        LOCK(cs_ethsyncheight);
        fGethSyncHeight = 0;
        fGethCurrentHeight = 0;
    }      
    return FlushErase(vecHeightKeys);
}
void CEthereumTxRootsDB::AuditTxRootDB(std::vector<std::pair<uint32_t, uint32_t> > &vecMissingBlockRanges){
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    uint32_t nKeyIndex = 0;
    uint32_t nCurrentSyncHeight = 0;
    {
        LOCK(cs_ethsyncheight);
        nCurrentSyncHeight = fGethSyncHeight;
       
    }
    uint32_t nKeyCutoff = nCurrentSyncHeight - MAX_ETHEREUM_TX_ROOTS;
    if(nCurrentSyncHeight < MAX_ETHEREUM_TX_ROOTS)
        nKeyCutoff = 0;
    std::vector<unsigned char> txPos;
    std::map<uint32_t, EthereumTxRoot> mapTxRoots;
    
    // sort keys numerically
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if(!pcursor->GetKey(nKey)){
                 pcursor->Next();
                 continue;
            }
            EthereumTxRoot txRoot;
            pcursor->GetValue(txRoot);
            #if __cplusplus > 201402 
            mapTxRoots.try_emplace(std::move(nKey), std::move(txRoot));
            #else
            mapTxRoots.emplace(nKey, txRoot);
            #endif            
            
            pcursor->Next();
        }
        catch (std::exception &e) {
            return;
        }
    } 
    while(mapTxRoots.size() < 2){
        vecMissingBlockRanges.emplace_back(make_pair(nKeyCutoff, nCurrentSyncHeight));
        return;
    }
    auto setIt = mapTxRoots.begin();
    nKeyIndex = setIt->first;
    setIt++;
    // we should have at least MAX_ETHEREUM_TX_ROOTS roots available from the tip for consensus checks
    if(nCurrentSyncHeight >= MAX_ETHEREUM_TX_ROOTS && nKeyIndex > nKeyCutoff){
        vecMissingBlockRanges.emplace_back(make_pair(nKeyCutoff, nKeyIndex-1));
    }
    std::vector<unsigned char> vchPrevHash;
    std::vector<uint32_t> vecRemoveKeys;
    // find sequence gaps in sorted key set 
    for (; setIt != mapTxRoots.end(); ++setIt){
            const uint32_t &key = setIt->first;
            const uint32_t &nNextKeyIndex = nKeyIndex+1;
            if (key != nNextKeyIndex && (key-1) >= nNextKeyIndex)
                vecMissingBlockRanges.emplace_back(make_pair(nNextKeyIndex, key-1));
            // if continious index we want to ensure hash chain is also continious
            else{
                // if prevhash of prev txroot != hash of this tx root then request inconsistent roots again
                const EthereumTxRoot &txRoot = setIt->second;
                auto prevRootPair = std::prev(setIt);
                const EthereumTxRoot &txRootPrev = prevRootPair->second;
                if(txRoot.vchPrevHash != txRootPrev.vchBlockHash){
                    // get a range of -50 to +50 around effected tx root to minimize chance that you will be requesting 1 root at a time in a long range fork
                    // this is fine because relayer fetches 100 headers at a time anyway
                    vecMissingBlockRanges.emplace_back(make_pair(std::max(0,(int32_t)key-50), std::min((int32_t)key+50, (int32_t)nCurrentSyncHeight)));
                    vecRemoveKeys.push_back(key);
                }
            }
            nKeyIndex = key;   
    } 
    if(!vecRemoveKeys.empty()){
        LogPrint(BCLog::SYS, "Detected an %d inconsistent hash chains in Ethereum headers, removing...\n", vecRemoveKeys.size());
        pethereumtxrootsdb->FlushErase(vecRemoveKeys);
    }
}
bool CEthereumTxRootsDB::FlushErase(const std::vector<uint32_t> &vecHeightKeys){
    if(vecHeightKeys.empty())
        return true;
    const uint32_t &nFirst = vecHeightKeys.front();
    const uint32_t &nLast = vecHeightKeys.back();
    CDBBatch batch(*this);
    for (const auto &key : vecHeightKeys) {
        batch.Erase(key);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d ethereum tx roots, block range (%d-%d)\n", vecHeightKeys.size(), nFirst, nLast);
    return WriteBatch(batch);
}
bool CEthereumTxRootsDB::FlushWrite(const EthereumTxRootMap &mapTxRoots){
    if(mapTxRoots.empty())
        return true;
    const uint32_t &nFirst = mapTxRoots.begin()->first;
    uint32_t nLast = nFirst;
    CDBBatch batch(*this);
    for (const auto &key : mapTxRoots) {
        batch.Write(key.first, key.second);
        nLast = key.first;
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d ethereum tx roots, block range (%d-%d)\n", mapTxRoots.size(), nFirst, nLast);
    return WriteBatch(batch);
}
bool CEthereumMintedTxDB::FlushWrite(const EthereumMintTxVec &vecMintKeys){
    if(vecMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecMintKeys) {
        batch.Write(key, true);
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d ethereum tx mints\n", vecMintKeys.size());
    return WriteBatch(batch);
}
bool CEthereumMintedTxDB::FlushErase(const EthereumMintTxVec &vecMintKeys){
    if(vecMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecMintKeys) {
        batch.Erase(key);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d ethereum tx mints\n", vecMintKeys.size());
    return WriteBatch(batch);
}