// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <services/assetconsensus.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <ethereum/ethereum.h>
#include <ethereum/sha3.h>
#include <ethereum/address.h>
#include <ethereum/common.h>
#include <ethereum/commondata.h>
#include <boost/thread.hpp>
#include <services/rpc/assetrpc.h>
#include <validationinterface.h>
#include <utility> // std::unique
extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesSetImpl arrivalTimesSet;
extern std::unordered_set<std::string> assetAllocationConflicts;
extern RecursiveMutex cs_assetallocationmempoolbalance;
extern RecursiveMutex cs_assetallocationarrival;
extern RecursiveMutex cs_assetallocationconflicts;
std::unique_ptr<CBlockIndexDB> pblockindexdb;
std::unique_ptr<CLockedOutpointsDB> plockedoutpointsdb;
std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
std::unique_ptr<CEthereumMintedTxDB> pethereumtxmintdb;
AssetPrevTxMap mapSenderLockedOutPoints;
AssetPrevTxMap mapAssetPrevTxSender;
RecursiveMutex cs_assetallocationmempoolremovetx;
ArrivalTimesSet setToRemoveFromMempool GUARDED_BY(cs_assetallocationmempoolremovetx);
extern RecursiveMutex cs_setethstatus;
extern bool AbortNode(const std::string& strMessage, const std::string& userMessage = "", unsigned int prefix = 0);
using namespace std;
bool FormatSyscoinErrorMessage(TxValidationState& state, const std::string errorMessage, bool bErrorNotInvalid, bool bConsensus){
        if(bErrorNotInvalid){
            return state.Error(errorMessage);
        }
        else{
            return state.Invalid(bConsensus? TxValidationResult::TX_CONSENSUS: TxValidationResult::TX_CONFLICT, errorMessage);
        }  
}
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const bool &fJustCheck, const bool& bSanityCheck, const int& nHeight, const int64_t& nTime, const uint256& blockhash, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys)
{
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET MINT %d %d %s %s bSanityCheck=%d\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
    // unserialize mint object from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    CAsset dbAsset;
    if(mintSyscoin.IsNull())
    {
        return FormatSyscoinErrorMessage(state, "mint-unserialize", bSanityCheck);
    } 
    if(mintSyscoin.assetAllocationTuple.IsNull())
    {
        return FormatSyscoinErrorMessage(state, "mint-asset", bSanityCheck);
    } 
    if(!GetAsset(mintSyscoin.assetAllocationTuple.nAsset, dbAsset)) 
    {
        return FormatSyscoinErrorMessage(state, "mint-non-existing-asset", bSanityCheck);
    }
    
   
    
    // do this check only when not in IBD (initial block download) or litemode
    // if we are starting up and verifying the db also skip this check as fLoaded will be false until startup sequence is complete
    EthereumTxRoot txRootDB;
   
    const bool &ethTxRootShouldExist = !ibd && !fLiteMode && fLoaded && fGethSynced;
    {
        LOCK(cs_setethstatus);
        // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
        // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
        if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)){
            if(ethTxRootShouldExist){
                // we always want to pass state.Invalid() for txroot missing errors here meaning we flag the block as invalid and dos ban the sender maybe
                // the check in contextualcheckblock that does this prevents us from getting a block that's invalid flagged as error so it won't propagate the block, but if block does arrive we should dos ban peer and invalidate the block itself from connect block
                return FormatSyscoinErrorMessage(state, "mint-txroot-missing", bSanityCheck);
            }
        }
    }  
    // if we checking this on block we would have already verified this in checkblock
    if(ethTxRootShouldExist){
        // time must be between 1 week and 1 hour old to be accepted
        if(nTime < txRootDB.nTimestamp) {
            return FormatSyscoinErrorMessage(state, "invalid-timestamp", bSanityCheck);
        }
        // 3 hr on testnet and 1 week on mainnet
        else if((nTime - txRootDB.nTimestamp) > ((bGethTestnet == true)? 10800: 604800)) {
            return FormatSyscoinErrorMessage(state, "mint-blockheight-too-old", bSanityCheck);
        } 
        
        // ensure that we wait at least 1 hour before we are allowed process this mint transaction  
        // also ensure sanity test that the current height that our node thinks Eth is on isn't less than the requested block for spv proof
        else if((nTime - txRootDB.nTimestamp) < ((bGethTestnet == true)? 600: 3600)) {
            return FormatSyscoinErrorMessage(state, "mint-insufficient-confirmations", bSanityCheck);
        }
    }
    
     // check transaction receipt validity

    const std::vector<unsigned char> &vchReceiptParentNodes = mintSyscoin.vchReceiptParentNodes;
    dev::RLP rlpReceiptParentNodes(&vchReceiptParentNodes);
    std::vector<unsigned char> vchReceiptValue;
    if(mintSyscoin.vchReceiptValue.size() == 2){
        const uint16_t &posReceipt = (static_cast<uint16_t>(mintSyscoin.vchReceiptValue[1])) | (static_cast<uint16_t>(mintSyscoin.vchReceiptValue[0]) << 8);
        vchReceiptValue = std::vector<unsigned char>(mintSyscoin.vchReceiptParentNodes.begin()+posReceipt, mintSyscoin.vchReceiptParentNodes.end());
    }
    else{
        vchReceiptValue = mintSyscoin.vchReceiptValue;
    }
    dev::RLP rlpReceiptValue(&vchReceiptValue);
    
    if (!rlpReceiptValue.isList()){
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt", bSanityCheck);
    }
    if (rlpReceiptValue.itemCount() != 4){
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-count", bSanityCheck);
    }
    const uint64_t &nStatus = rlpReceiptValue[0].toInt<uint64_t>(dev::RLP::VeryStrict);
    if (nStatus != 1){
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-status", bSanityCheck);
    } 
    dev::RLP rlpReceiptLogsValue(rlpReceiptValue[3]);
    if (!rlpReceiptLogsValue.isList()){
        return FormatSyscoinErrorMessage(state, "mint-receipt-rlp-logs-list", bSanityCheck);
    }
    const size_t &itemCount = rlpReceiptLogsValue.itemCount();
    // just sanity checks for bounds
    if (itemCount < 1 || itemCount > 10){
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-logs-count", bSanityCheck);
    }
    // look for TokenFreeze event and get the last parameter which should be the BridgeTransferID
    uint32_t nBridgeTransferID = 0;
    for(uint32_t i = 0;i<itemCount;i++){
        dev::RLP rlpReceiptLogValue(rlpReceiptLogsValue[i]);
        if (!rlpReceiptLogValue.isList()){
            return FormatSyscoinErrorMessage(state, "mint-receipt-log-rlp-list", bSanityCheck);
        }
        // ensure this log has atleast the address to check against
        if (rlpReceiptLogValue.itemCount() < 1){
            return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count", bSanityCheck);
        }
        const dev::Address &address160Log = rlpReceiptLogValue[0].toHash<dev::Address>(dev::RLP::VeryStrict);
        if(Params().GetConsensus().vchSYSXERC20Manager == address160Log.asBytes()){
            // for mint log we should have exactly 3 entries in it, this event we control through our erc20manager contract
            if (rlpReceiptLogValue.itemCount() != 3){
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count-bridgeid", bSanityCheck);
            }
            // check topic
            dev::RLP rlpReceiptLogTopicsValue(rlpReceiptLogValue[1]);
            if (!rlpReceiptLogTopicsValue.isList()){
                return FormatSyscoinErrorMessage(state, "mint-receipt-log-topics-rlp-list", bSanityCheck);
            }
            if (rlpReceiptLogTopicsValue.itemCount() != 1){
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-topics-count", bSanityCheck);
            }
            // topic hash matches with TokenFreeze signature
            if(Params().GetConsensus().vchTokenFreezeMethod == rlpReceiptLogTopicsValue[0].toBytes(dev::RLP::VeryStrict)){
                const std::vector<unsigned char> &dataValue = rlpReceiptLogValue[2].toBytes(dev::RLP::VeryStrict);
                if(dataValue.size() < 96){
                     return FormatSyscoinErrorMessage(state, "mint-receipt-log-data-invalid-size", bSanityCheck);
                }
                // get last data field which should be our BridgeTransferID
                const std::vector<unsigned char> bridgeIdValue(dataValue.begin()+64, dataValue.end());
                nBridgeTransferID = static_cast<uint32_t>(bridgeIdValue[31]);
                nBridgeTransferID |= static_cast<uint32_t>(bridgeIdValue[30]) << 8;
                nBridgeTransferID |= static_cast<uint32_t>(bridgeIdValue[29]) << 16;
                nBridgeTransferID |= static_cast<uint32_t>(bridgeIdValue[28]) << 24;
            }
        }
    }
    if(nBridgeTransferID == 0){
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-missing-bridge-id", bSanityCheck);
    }
    // check transaction spv proofs
    dev::RLP rlpTxRoot(&mintSyscoin.vchTxRoot);
    dev::RLP rlpReceiptRoot(&mintSyscoin.vchReceiptRoot);

    if(!txRootDB.vchTxRoot.empty() && rlpTxRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchTxRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", bSanityCheck);
    }

    if(!txRootDB.vchReceiptRoot.empty() && rlpReceiptRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchReceiptRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", bSanityCheck);
    } 
    
    
    const std::vector<unsigned char> &vchTxParentNodes = mintSyscoin.vchTxParentNodes;
    dev::RLP rlpTxParentNodes(&vchTxParentNodes);
    dev::h256 hash;
    std::vector<unsigned char> vchTxValue;
    if(mintSyscoin.vchTxValue.size() == 2){
        const uint16_t &posTx = (static_cast<uint16_t>(mintSyscoin.vchTxValue[1])) | (static_cast<uint16_t>(mintSyscoin.vchTxValue[0]) << 8);
        vchTxValue = std::vector<unsigned char>(mintSyscoin.vchTxParentNodes.begin()+posTx, mintSyscoin.vchTxParentNodes.end());
        hash = dev::sha3(vchTxValue);
    }
    else{
        vchTxValue = mintSyscoin.vchTxValue;
        hash = dev::sha3(mintSyscoin.vchTxValue);
    }
    dev::RLP rlpTxValue(&vchTxValue);
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    dev::RLP rlpTxPath(&vchTxPath);
    const std::vector<unsigned char> &vchHash = hash.asBytes();
    // ensure eth tx not already spent
    if(pethereumtxmintdb->ExistsKey(vchHash)){
        return FormatSyscoinErrorMessage(state, "mint-exists", bSanityCheck);
    } 
    // add the key to flush to db later
    vecMintKeys.emplace_back(std::make_pair(std::make_pair(vchHash, nBridgeTransferID), txHash));
    
    // verify receipt proof
    if(!VerifyProof(&vchTxPath, rlpReceiptValue, rlpReceiptParentNodes, rlpReceiptRoot)){
        return FormatSyscoinErrorMessage(state, "mint-verify-receipt-proof", bSanityCheck);
    } 
    // verify transaction proof
    if(!VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot)){
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-proof", bSanityCheck);
    } 
    if (!rlpTxValue.isList()){
        return FormatSyscoinErrorMessage(state, "mint-tx-rlp-list", bSanityCheck);
    }
    if (rlpTxValue.itemCount() < 6){
        return FormatSyscoinErrorMessage(state, "mint-tx-itemcount", bSanityCheck);
    }        
    if (!rlpTxValue[5].isData()){
        return FormatSyscoinErrorMessage(state, "mint-tx-array", bSanityCheck);
    }        
    if (rlpTxValue[3].isEmpty()){
        return FormatSyscoinErrorMessage(state, "mint-tx-invalid-receiver", bSanityCheck);
    }                       
    const dev::Address &address160 = rlpTxValue[3].toHash<dev::Address>(dev::RLP::VeryStrict);

    // ensure ERC20Manager is in the "to" field for the contract, meaning the function was called on this contract for freezing supply
    if(Params().GetConsensus().vchSYSXERC20Manager != address160.asBytes()){
        return FormatSyscoinErrorMessage(state, "mint-invalid-contract-manager", bSanityCheck);
    }
    
    CAmount outputAmount;
    uint32_t nAsset = 0;
    const std::vector<unsigned char> &rlpBytes = rlpTxValue[5].toBytes(dev::RLP::VeryStrict);
    CWitnessAddress witnessAddress;
    std::vector<unsigned char> vchERC20ContractAddress;
    if(!parseEthMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, rlpBytes, dbAsset.vchContract, outputAmount, nAsset, dbAsset.nPrecision, witnessAddress)){
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-data", bSanityCheck);
    }
    if(!fUnitTest){
        if(witnessAddress != mintSyscoin.assetAllocationTuple.witnessAddress){
            return FormatSyscoinErrorMessage(state, "mint-mismatch-witness-address", bSanityCheck);
        }   
        if(nAsset != dbAsset.nAsset){
            return FormatSyscoinErrorMessage(state, "mint-mismatch-asset", bSanityCheck);
        }
    }
    if(outputAmount != mintSyscoin.nValueAsset){
        return FormatSyscoinErrorMessage(state, "mint-mismatch-value", bSanityCheck);
    }  
    if(outputAmount <= 0){
        return FormatSyscoinErrorMessage(state, "mint-burn-value", bSanityCheck);
    }  
    const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
    #if __cplusplus > 201402 
    auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
    #else
    auto result1 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
    #endif

    // sender as burn	
    const CAssetAllocationTuple senderAllocationTuple(mintSyscoin.assetAllocationTuple.nAsset, burnWitness);	
    const std::string &senderTupleStr = senderAllocationTuple.ToString();	
    #if __cplusplus > 201402 
    auto result2 = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
    #else
    auto result2 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));	
    #endif
    auto mapSenderAssetAllocation = result2.first;	
    const bool &mapSenderAssetAllocationNotFound = result2.second;	
    if(mapSenderAssetAllocationNotFound){	
        CAssetAllocationDBEntry senderAllocation;	
        GetAssetAllocation(senderAllocationTuple, senderAllocation);	
        if (senderAllocation.assetAllocationTuple.IsNull()) {           	
            senderAllocation.assetAllocationTuple.nAsset = std::move(senderAllocationTuple.nAsset);	
            senderAllocation.assetAllocationTuple.witnessAddress = std::move(senderAllocationTuple.witnessAddress); 	
        }	
        mapSenderAssetAllocation->second = std::move(senderAllocation);              	
    }

    auto mapAssetAllocation = result1.first;
    const bool &mapAssetAllocationNotFound = result1.second;
    if(mapAssetAllocationNotFound){
        CAssetAllocationDBEntry receiverAllocation;
        GetAssetAllocation(mintSyscoin.assetAllocationTuple, receiverAllocation);
        if (receiverAllocation.assetAllocationTuple.IsNull()) {           
            receiverAllocation.assetAllocationTuple.nAsset = std::move(mintSyscoin.assetAllocationTuple.nAsset);
            receiverAllocation.assetAllocationTuple.witnessAddress = std::move(mintSyscoin.assetAllocationTuple.witnessAddress);
        }
        mapAssetAllocation->second = std::move(receiverAllocation);             
    }
    mapAssetAllocation->second.nBalance += mintSyscoin.nValueAsset;
    mapSenderAssetAllocation->second.nBalance -= mintSyscoin.nValueAsset;
    if(mapSenderAssetAllocation->second.nBalance < 0){	
        return FormatSyscoinErrorMessage(state, "asset-insufficient-balance", bSanityCheck);
    }    	
    if(mapSenderAssetAllocation->second.nBalance == 0)	
        mapSenderAssetAllocation->second.SetNull();        
    if (!AssetRange(mapAssetAllocation->second.nBalance))
    {
        return FormatSyscoinErrorMessage(state, "new-balance-out-of-range", bSanityCheck);
    }
    
    if (!AssetRange(mintSyscoin.nValueAsset))
    {
        return FormatSyscoinErrorMessage(state, "mint-amount-out-of-range", bSanityCheck);
    }
    if(!fJustCheck){
        if(!bSanityCheck && nHeight > 0) {   
            LogPrint(BCLog::SYS,"CONNECTED ASSET MINT: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                senderTupleStr.c_str(),
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);      
        }           
    }               
    return true;
}
bool CheckSyscoinInputs(const CTransaction& tx, const uint256& txHash, TxValidationState& state, AssetBalanceMap &mapAssetAllocationBalances, const CCoinsViewCache &inputs, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const bool &bSanityCheck)
{
    AssetMap mapAssets;
    EthereumMintTxVec vecMintKeys;
    std::vector<COutPoint> vecLockedOutpoints;
    AssetAllocationMap mapAssetAllocations;
    return CheckSyscoinInputs(false, tx, txHash, state, inputs, fJustCheck, nHeight, nTime, uint256(), bSanityCheck, mapAssetAllocations, mapAssetAllocationBalances, mapAssets, vecMintKeys, vecLockedOutpoints);
}
bool CheckSyscoinInputs(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const CCoinsViewCache &inputs,  const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanityCheck, AssetAllocationMap &mapAssetAllocations, AssetBalanceMap &mapAssetAllocationBalances, AssetMap &mapAssets, EthereumMintTxVec &vecMintKeys, std::vector<COutPoint> &vecLockedOutpoints)
{
    bool good = true;
    try{
        if (IsAssetAllocationTx(tx.nVersion))
        {
            CAssetAllocation theAssetAllocation(tx);
            if(theAssetAllocation.assetAllocationTuple.IsNull()){
                return FormatSyscoinErrorMessage(state, "assetallocation-unserialize", bSanityCheck);
            }
            good = CheckAssetAllocationInputs(tx, txHash, theAssetAllocation, state, inputs, fJustCheck, nHeight, blockHash, mapAssetAllocations, mapAssetAllocationBalances, vecLockedOutpoints, bSanityCheck);
        }
        else if (IsAssetTx(tx.nVersion))
        {
            good = CheckAssetInputs(tx, txHash, state, inputs, fJustCheck, nHeight, blockHash, mapAssets, mapAssetAllocations, bSanityCheck);
        } 
        else if(IsSyscoinMintTx(tx.nVersion))
        {
            if(nHeight < Params().GetConsensus().nBridgeStartBlock){
                FormatSyscoinErrorMessage(state, "mint-disabled", bSanityCheck);
                good = false;
            }
            else{
                good = CheckSyscoinMint(ibd, tx, txHash, state, fJustCheck, bSanityCheck, nHeight, nTime, blockHash, mapAssets, mapAssetAllocations, vecMintKeys);
            }
        }
    } catch (...) {
        return FormatSyscoinErrorMessage(state, "checksyscoininputs-exception", bSanityCheck);
    }
    return good;
}
void SetZDAGConflict(const uint256 &txHash, const std::string &fSyscoinSender){
    {
        LOCK(cs_assetallocationconflicts);
        // add conflicting sender
        assetAllocationConflicts.insert(fSyscoinSender);
    }
    {
        LOCK(cs_assetallocationmempoolremovetx);
        // mark to remove from mempool, because if we remove right away then the transaction data cannot be relayed most of the time
        LogPrint(BCLog::SYS, "Double spend detected on tx %s!\n", txHash.GetHex());
        setToRemoveFromMempool.insert(txHash);
    }
}
void AddZDAGTx(const CTransactionRef &zdagTx, const AssetBalanceMap &mapAssetAllocationBalances) {
    const uint256 &txHash = zdagTx->GetHash();
    LOCK(cs_assetallocationmempoolbalance);
    for(const auto &assetAllocationBalance: mapAssetAllocationBalances){
        #if __cplusplus > 201402 
        auto result = mempoolMapAssetBalances.try_emplace(assetAllocationBalance.first,  std::move(assetAllocationBalance.second));
        #else
        auto result = mempoolMapAssetBalances.emplace(std::piecewise_construct,  std::forward_as_tuple(assetAllocationBalance.first),  std::forward_as_tuple(assetAllocationBalance.second));        
        #endif
        // if found, update it
        if(!result.second){
            result.first->second = std::move(assetAllocationBalance.second);
        }   
        {
            LOCK(cs_assetallocationarrival);
            ArrivalTimesSet& arrivalTimes = arrivalTimesSet[std::move(assetAllocationBalance.first)];
            arrivalTimes.insert(txHash);
        }
    }
    
}
// remove arrival time/mempool balances upon mempool removal, as well as any conflicts if arrival times vector is empty for the sender of this tx
void RemoveZDAGTx(const CTransactionRef &zdagTx) {
    if(!IsZdagTx(zdagTx->nVersion))
        return;
    const std::string &sender = GetSenderOfZdagTx(*zdagTx);
    if(sender.empty())
        return;
    const uint256& zdagTxhash = zdagTx->GetHash();
    {
        LOCK(cs_assetallocationarrival);
        auto arrivalTimesIt = arrivalTimesSet.find(sender);
        if(arrivalTimesIt != arrivalTimesSet.end()){
            arrivalTimesIt->second.erase(zdagTxhash);
            if(arrivalTimesIt->second.empty())
            {
                arrivalTimesSet.erase(arrivalTimesIt);
                {
                    LOCK(cs_assetallocationmempoolbalance);
                    mempoolMapAssetBalances.erase(sender);
                }
                {
                    LOCK(cs_assetallocationconflicts);
                    assetAllocationConflicts.erase(sender);
                }
            }
        }
    }
    {
        LOCK(cs_assetallocationmempoolremovetx);
        setToRemoveFromMempool.erase(zdagTxhash);
    }
    
}
bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull())
    {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Cannot unserialize data inside of this transaction relating to an assetallocationmint\n");
        return false;
    }
    // remove eth spend tx from our internal db
    dev::h256 hash;
    if(mintSyscoin.vchTxValue.size() == 2){
        const unsigned short &posTx = ((mintSyscoin.vchTxValue[0]<<8)|(mintSyscoin.vchTxValue[1]));
        const std::vector<unsigned char> &vchTxValue = std::vector<unsigned char>(mintSyscoin.vchTxParentNodes.begin()+posTx, mintSyscoin.vchTxParentNodes.end());
        hash = dev::sha3(vchTxValue);
    }
    else{
        hash = dev::sha3(mintSyscoin.vchTxValue);
    }

    const std::vector<unsigned char> &vchHash = hash.asBytes();
    vecMintKeys.emplace_back(std::make_pair(std::make_pair(vchHash, 0), txHash));

     const std::string &receiverTupleStr = mintSyscoin.assetAllocationTuple.ToString();
    #if __cplusplus > 201402 
    auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
    #else
    auto result1 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
    #endif

    // sender as burn	
    const CAssetAllocationTuple senderAllocationTuple(mintSyscoin.assetAllocationTuple.nAsset, burnWitness);	
    const std::string &senderTupleStr = senderAllocationTuple.ToString();	
    #if __cplusplus > 201402 
    auto result2 = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
    #else
    auto result2 = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));	
    #endif
    auto mapSenderAssetAllocation = result2.first;	
    const bool &mapSenderAssetAllocationNotFound = result2.second;	
    if(mapSenderAssetAllocationNotFound){	
        CAssetAllocationDBEntry senderAllocation;	
        GetAssetAllocation(senderAllocationTuple, senderAllocation);	
        if (senderAllocation.assetAllocationTuple.IsNull()) {           	
            senderAllocation.assetAllocationTuple.nAsset = std::move(senderAllocationTuple.nAsset);	
            senderAllocation.assetAllocationTuple.witnessAddress = std::move(senderAllocationTuple.witnessAddress); 	
        }	
        mapSenderAssetAllocation->second = std::move(senderAllocation);              	
    }

    auto mapAssetAllocation = result1.first;
    const bool &mapAssetAllocationNotFound = result1.second;
    if(mapAssetAllocationNotFound){
        CAssetAllocationDBEntry receiverAllocation;
        GetAssetAllocation(mintSyscoin.assetAllocationTuple, receiverAllocation);
        if (receiverAllocation.assetAllocationTuple.IsNull()) {           
            receiverAllocation.assetAllocationTuple.nAsset = std::move(mintSyscoin.assetAllocationTuple.nAsset);
            receiverAllocation.assetAllocationTuple.witnessAddress = std::move(mintSyscoin.assetAllocationTuple.witnessAddress);
        }
        mapAssetAllocation->second = std::move(receiverAllocation);             
    }
    // reverse allocations
    mapAssetAllocation->second.nBalance -= mintSyscoin.nValueAsset;
    mapSenderAssetAllocation->second.nBalance += mintSyscoin.nValueAsset;
    if(mapAssetAllocation->second.nBalance < 0) {
        return false;
    }       
    else if(mapAssetAllocation->second.nBalance == 0){
        mapAssetAllocation->second.SetNull();
    }
    return true; 
}
bool DisconnectAssetAllocation(const CTransaction &tx, const uint256& txid, const CAssetAllocation &theAssetAllocation, CCoinsViewCache& view, AssetAllocationMap &mapAssetAllocations){

    const std::string &senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();

    #if __cplusplus > 201402 
    auto result = mapAssetAllocations.try_emplace(senderTupleStr,  std::move(emptyAllocation));
    #else
    auto result = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
    #endif
    
    auto mapAssetAllocation = result.first;
    const bool & mapAssetAllocationNotFound = result.second;
    if(mapAssetAllocationNotFound){
        CAssetAllocationDBEntry senderAllocation;
        GetAssetAllocation(theAssetAllocation.assetAllocationTuple, senderAllocation);
        if (senderAllocation.assetAllocationTuple.IsNull()) {
            senderAllocation.assetAllocationTuple.nAsset = std::move(theAssetAllocation.assetAllocationTuple.nAsset);
            senderAllocation.assetAllocationTuple.witnessAddress = std::move(theAssetAllocation.assetAllocationTuple.witnessAddress);       
        } 
        mapAssetAllocation->second = std::move(senderAllocation);               
    }
    CAssetAllocationDBEntry& storedSenderAllocationRef = mapAssetAllocation->second;
    CAmount nTotal = 0;
    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
       
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocationDBEntry receiverAllocation;
        #if __cplusplus > 201402 
        auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
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
        CAssetAllocationDBEntry& storedReceiverAllocationRef = mapAssetAllocationReceiver->second;

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
    }
    return true; 
}
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys)
{
    if(tx.IsCoinBase())
        return true;
 
    if(IsSyscoinMintTx(tx.nVersion)){
        if(!DisconnectMintAsset(tx, txHash, mapAssetAllocations, vecMintKeys))
            return false;       
    }  
    else{
        if (IsAssetAllocationTx(tx.nVersion))
        {
            CAssetAllocation theAssetAllocation(tx);
            if(theAssetAllocation.assetAllocationTuple.IsNull()){
                LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not decode asset allocation\n");
                return false;
            }
            if(!DisconnectAssetAllocation(tx, txHash, theAssetAllocation, view, mapAssetAllocations))
                return false;       
        }
        else if (IsAssetTx(tx.nVersion))
        {
            if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
                if(!DisconnectAssetSend(tx, txHash, mapAssets, mapAssetAllocations))
                    return false;
            } else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {  
                if(!DisconnectAssetUpdate(tx, txHash, mapAssets))
                    return false;
            }
            else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER) {  
                 if(!DisconnectAssetTransfer(tx, txHash, mapAssets))
                    return false;
            }
            else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                if(!DisconnectAssetActivate(tx, txHash, mapAssets))
                    return false;
            }     
        }
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
bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, const CAssetAllocation &theAssetAllocation, TxValidationState &state, const CCoinsViewCache &inputs,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetAllocationMap &mapAssetAllocations, AssetBalanceMap &mapAssetAllocationBalances, std::vector<COutPoint> &vecLockedOutpoints, const bool &bSanityCheck) {
    if (passetallocationdb == nullptr)
        return false;
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %d %s %s bSanityCheck=%d\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
            

    string retError = "";
    if(fJustCheck)
    {
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
            if (theAssetAllocation.listSendingAllocationAmounts.empty())
            {
                return FormatSyscoinErrorMessage(state, "assetallocation-empty", bSanityCheck);
            }
            if (theAssetAllocation.listSendingAllocationAmounts.size() > 250)
            {
                return FormatSyscoinErrorMessage(state, "assetallocation-too-many-receivers", bSanityCheck);
            }
			if (!theAssetAllocation.lockedOutpoint.IsNull())
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-cannot-include-lockpoint", bSanityCheck);
			}
            break; 
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
			if (!theAssetAllocation.lockedOutpoint.IsNull())
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-cannot-include-lockpoint", bSanityCheck);
			}
            if(theAssetAllocation.listSendingAllocationAmounts.empty())
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-empty", bSanityCheck);
			}
            if(theAssetAllocation.listSendingAllocationAmounts.size() > 1)
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-too-many-receivers", bSanityCheck);
			}
            break;            
		case SYSCOIN_TX_VERSION_ALLOCATION_LOCK:
			if (theAssetAllocation.lockedOutpoint.IsNull())
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-missing-lockpoint", bSanityCheck);
			}
            if(!theAssetAllocation.listSendingAllocationAmounts.empty())
			{
                return FormatSyscoinErrorMessage(state, "assetallocation-too-many-receivers", bSanityCheck);
			}
			break;
        default:
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-op", bSanityCheck);
        }
    }

    const CWitnessAddress &user1 = theAssetAllocation.assetAllocationTuple.witnessAddress;
    const std::string &senderTupleStr = theAssetAllocation.assetAllocationTuple.ToString();
    CAssetAllocationDBEntry dbAssetAllocation;
    AssetAllocationMap::iterator mapAssetAllocation;
    CAsset dbAsset;
    if(fJustCheck){
        if (!GetAssetAllocation(theAssetAllocation.assetAllocationTuple, dbAssetAllocation))
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-allocation", bSanityCheck);
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
                return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-allocation", bSanityCheck);
            }
            mapAssetAllocation->second = std::move(dbAssetAllocation);             
        }
    }
    CAssetAllocationDBEntry& storedSenderAllocationRef = fJustCheck? dbAssetAllocation:mapAssetAllocation->second;
    
    if (!GetAsset(storedSenderAllocationRef.assetAllocationTuple.nAsset, dbAsset))
    {
        return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-asset", bSanityCheck);
    }   
    CAmount mapBalanceSenderCopy;
    const bool & isZdagTx = IsZdagTx(tx.nVersion);
    if(fJustCheck && !bSanityCheck && isZdagTx){
        LOCK(cs_assetallocationmempoolbalance); 
        #if __cplusplus > 201402 
        auto result = mempoolMapAssetBalances.try_emplace(senderTupleStr,  std::move(storedSenderAllocationRef.nBalance));
        #else
        auto result =  mempoolMapAssetBalances.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(std::move(storedSenderAllocationRef.nBalance))); 
        #endif
        mapBalanceSenderCopy = result.first->second;
    }
    else
        mapBalanceSenderCopy = storedSenderAllocationRef.nBalance;
            
    if(tx.nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION){
        const uint32_t &nBurnAsset = theAssetAllocation.assetAllocationTuple.nAsset;
        if(!fUnitTest && nBurnAsset != Params().GetConsensus().nSYSXAsset)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", bSanityCheck);
        }
        const CAssetAllocationTuple receiverAllocationTuple(nBurnAsset, theAssetAllocation.listSendingAllocationAmounts[0].first);
        const string& receiverTupleStr = receiverAllocationTuple.ToString();     
        if (!FindAssetOwnerInTx(inputs, tx, receiverAllocationTuple.witnessAddress))
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sender", bSanityCheck);
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
            return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-output", bSanityCheck);
        }
        const CAmount &nBurnAmount = tx.vout[nOut].nValue;
        if(nBurnAmount <= 0)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-positive-burn-amount", bSanityCheck);
        }
        if(nBurnAmount != theAssetAllocation.listSendingAllocationAmounts[0].second)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", bSanityCheck);
        }  
        if(user1 != burnWitness)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-address", bSanityCheck);
        }

        if (nBurnAmount <= 0 || nBurnAmount > dbAsset.nMaxSupply)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-amount-out-of-range", bSanityCheck);
        }        
       
        mapBalanceSenderCopy -= nBurnAmount;
        if (mapBalanceSenderCopy < 0) {         
            return FormatSyscoinErrorMessage(state, "assetallocation-insufficient-balance", bSanityCheck);
        }
        if (!fJustCheck) {   
            #if __cplusplus > 201402 
            auto resultReceiver = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
            #else
            auto resultReceiver = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr), std::forward_as_tuple(std::move(emptyAllocation)));
            #endif 
            
            auto mapAssetAllocationReceiver = resultReceiver.first;
            const bool& mapAssetAllocationReceiverNotFound = resultReceiver.second;
            if(mapAssetAllocationReceiverNotFound){
                CAssetAllocationDBEntry dbAssetAllocationReceiver;
                if (!GetAssetAllocation(receiverAllocationTuple, dbAssetAllocationReceiver)) {               
                    dbAssetAllocationReceiver.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                    dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);              
                }
                mapAssetAllocationReceiver->second = std::move(dbAssetAllocationReceiver);               
            } 
            mapAssetAllocationReceiver->second.nBalance += nBurnAmount; 
            if (!AssetRange(mapAssetAllocationReceiver->second.nBalance))
            {
                return FormatSyscoinErrorMessage(state, "new-balance-out-of-range", bSanityCheck);
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
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", bSanityCheck);
            } 
                    
        } else if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
            std::vector<unsigned char> vchEthAddress;
            std::vector<unsigned char> vchEthContract;
            uint32_t nAssetFromScript;
            CAmount nAmountFromScript;
            CWitnessAddress burnWitnessAddress;
            uint8_t nPrecision;
            if(!GetSyscoinBurnData(tx, nAssetFromScript, burnWitnessAddress, nAmountFromScript, vchEthAddress, nPrecision, vchEthContract)){
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-transaction", bSanityCheck);
            }
            if(dbAsset.nPrecision != nPrecision)
            {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-precision", bSanityCheck);
            }
            if(dbAsset.vchContract.empty() || dbAsset.vchContract != vchEthContract)
            {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-contract", bSanityCheck);
            }        
        }
       
        if(theAssetAllocation.listSendingAllocationAmounts[0].first != burnWitness)
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-address", bSanityCheck);
        } 
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, storedSenderAllocationRef.lockedOutpoint))
        {     
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sender", bSanityCheck);
        }       
        if (nBurnAmount <= 0 || (dbAsset.nTotalSupply > 0 && nBurnAmount > dbAsset.nTotalSupply))
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", bSanityCheck);
        }        
  		// ensure lockedOutpoint is cleared on PoW, it is useful only once typical for atomic scripts like CLTV based atomic swaps or hashlock type of usecases
		if (!bSanityCheck && !fJustCheck && !storedSenderAllocationRef.lockedOutpoint.IsNull()) {
			// this will flag the batch write function on plockedoutpointsdb to erase this outpoint
			vecLockedOutpoints.emplace_back(emptyPoint);
			storedSenderAllocationRef.lockedOutpoint.SetNull();
		}      
        if(dbAsset.vchContract.empty())
        {
            return FormatSyscoinErrorMessage(state, "assetallocation-missing-contract", bSanityCheck);
        } 
       
        mapBalanceSenderCopy -= nBurnAmount;
        if (mapBalanceSenderCopy < 0) {
            return FormatSyscoinErrorMessage(state, "assetallocation-insufficient-balance", bSanityCheck);
        }
        const CAssetAllocationTuple receiverAllocationTuple(nBurnAsset,  burnWitness);
        const string& receiverTupleStr = receiverAllocationTuple.ToString(); 
        if (!fJustCheck) {   
            #if __cplusplus > 201402 
            auto resultReceiver = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
            #else
            auto resultReceiver = mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
            #endif 
            
            auto mapAssetAllocationReceiver = resultReceiver.first;
            const bool& mapAssetAllocationReceiverNotFound = resultReceiver.second;
            if(mapAssetAllocationReceiverNotFound){
                CAssetAllocationDBEntry dbAssetAllocationReceiver;
                if (!GetAssetAllocation(receiverAllocationTuple, dbAssetAllocationReceiver)) {               
                    dbAssetAllocationReceiver.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                    dbAssetAllocationReceiver.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);              
                }
                mapAssetAllocationReceiver->second = std::move(dbAssetAllocationReceiver);                  
            } 
            mapAssetAllocationReceiver->second.nBalance += nBurnAmount;
            if (!AssetRange(mapAssetAllocationReceiver->second.nBalance))
            {
                return FormatSyscoinErrorMessage(state, "new-balance-out-of-range", bSanityCheck);
            }                                 
        }
    }
	else if (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK)
	{
		if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, storedSenderAllocationRef.lockedOutpoint))
		{             
			return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sender", bSanityCheck);
		}
        if (!bSanityCheck && !fJustCheck){
    		storedSenderAllocationRef.lockedOutpoint = theAssetAllocation.lockedOutpoint;
    		// this will batch write the outpoint in the calling function, we save the outpoint so that we cannot spend this outpoint without creating an SYSCOIN_TX_VERSION_ALLOCATION_SEND transaction
    		vecLockedOutpoints.emplace_back(std::move(theAssetAllocation.lockedOutpoint));
        }
	}
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND)
	{
        if (storedSenderAllocationRef.assetAllocationTuple != theAssetAllocation.assetAllocationTuple || !FindAssetOwnerInTx(inputs, tx, user1, storedSenderAllocationRef.lockedOutpoint))
        {     
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sender", bSanityCheck);
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
                return FormatSyscoinErrorMessage(state, "assetallocation-negative-amount", bSanityCheck);
            }           
        }
        if (!AssetRange(nTotal))
        {
            return FormatSyscoinErrorMessage(state, "amount-out-of-range", bSanityCheck);
        }
        mapBalanceSenderCopy -= nTotal;
        if (mapBalanceSenderCopy < 0) {
            return FormatSyscoinErrorMessage(state, "assetallocation-insufficient-balance", bSanityCheck);
        }
               
        for (unsigned int i = 0;i<theAssetAllocation.listSendingAllocationAmounts.size();i++) {
            const auto& amountTuple = theAssetAllocation.listSendingAllocationAmounts[i];
            if (amountTuple.first == theAssetAllocation.assetAllocationTuple.witnessAddress) {   
                return FormatSyscoinErrorMessage(state, "assetallocation-send-to-yourself", bSanityCheck);
            }
            if (!fJustCheck) {  
                const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
                const string &receiverTupleStr = receiverAllocationTuple.ToString();
                #if __cplusplus > 201402 
                auto result1 = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
                #else
                auto result1 =  mapAssetAllocations.emplace(std::piecewise_construct,  std::forward_as_tuple(receiverTupleStr),  std::forward_as_tuple(std::move(emptyAllocation)));
                #endif       
                
                auto mapBalanceReceiverBlock = result1.first;
                const bool& mapAssetAllocationReceiverBlockNotFound = result1.second;
                if(mapAssetAllocationReceiverBlockNotFound){
                    CAssetAllocationDBEntry receiverAllocation;
                    if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {                   
                        receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                        receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);                       
                    }
                    mapBalanceReceiverBlock->second = std::move(receiverAllocation);  
                }
                mapBalanceReceiverBlock->second.nBalance += amountTuple.second;
                if (!AssetRange(mapBalanceReceiverBlock->second.nBalance))
                {
                    return FormatSyscoinErrorMessage(state, "new-balance-out-of-range", bSanityCheck);
                }
            }
        }
        
    }
    // write assetallocation  
    // asset sends are the only ones confirming without PoW
    if(!fJustCheck){
        storedSenderAllocationRef.nBalance = std::move(mapBalanceSenderCopy);
        if(storedSenderAllocationRef.nBalance == 0)
            storedSenderAllocationRef.SetNull();    

        if(!bSanityCheck && nHeight > 0) {   
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                senderTupleStr.c_str(),
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);      
        }
                    
    }
    else if(!bSanityCheck && isZdagTx){
        #if __cplusplus > 201402 
        auto resultBalance = mapAssetAllocationBalances.try_emplace(senderTupleStr,  std::move(mapBalanceSenderCopy));
        #else
        auto resultBalance = mapAssetAllocationBalances.emplace(std::piecewise_construct,  std::forward_as_tuple(senderTupleStr),  std::forward_as_tuple(mapBalanceSenderCopy));
        #endif
        // if found, update it
        if(!resultBalance.second){
            resultBalance.first->second = std::move(mapBalanceSenderCopy);
        } 
    }    
    return true;
}

bool DisconnectAssetSend(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations){
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
        CAssetAllocationDBEntry receiverAllocation;
        #if __cplusplus > 201402 
        auto result = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
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
        CAssetAllocationDBEntry& storedReceiverAllocationRef = mapAssetAllocation->second;
                    
        // reverse allocation
        if(storedReceiverAllocationRef.nBalance >= amountTuple.second){
            storedReceiverAllocationRef.nBalance -= amountTuple.second;
            storedSenderRef.nBalance += amountTuple.second;
        } 

        if(storedReceiverAllocationRef.nBalance == 0){
            storedReceiverAllocationRef.SetNull();       
        }                                            
    }             
    return true;  
}
bool DisconnectAssetUpdate(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets){
    
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
    return true;  
}
bool DisconnectAssetTransfer(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets){
    
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
    return true;  
}
bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets){
    
    CAsset theAsset(tx);
    
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectAssetActivate: Could not decode asset in asset activate\n");
        return false;
    }
    #if __cplusplus > 201402 
    mapAssets.try_emplace(std::move(theAsset.nAsset),  std::move(emptyAsset));
    #else
    mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(std::move(theAsset.nAsset)),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif   
    return true;  
}
bool CheckAssetInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &state, const CCoinsViewCache &inputs,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations, const bool &bSanityCheck) {
    if (passetdb == nullptr)
        return false;
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
        return FormatSyscoinErrorMessage(state, "asset-unserialize", bSanityCheck);
    }
    

    if(fJustCheck)
    {
        if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND) {
            if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
            {
                return FormatSyscoinErrorMessage(state, "asset-pubdata-too-big", bSanityCheck);
            }
        }
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
            if(!fUnitTest && nHeight >= Params().GetConsensus().nBridgeStartBlock && tx.vout[nDataOut].nValue < 500*COIN)
            {
                return FormatSyscoinErrorMessage(state, "asset-insufficient-fee", bSanityCheck);
            }
            if (theAsset.nAsset <= SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN)
            {
                return FormatSyscoinErrorMessage(state, "asset-guid-invalid", bSanityCheck);
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
            }  
            if (theAsset.nPrecision > 8)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
            }
            if (theAsset.strSymbol.size() > 8 || theAsset.strSymbol.size() < 1)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
            }
            if (!AssetRange(theAsset.nMaxSupply, theAsset.nPrecision))
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-maxsupply", bSanityCheck);
            }
            if (theAsset.nBalance > theAsset.nMaxSupply || theAsset.nBalance <= 0)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-totalsupply", bSanityCheck);
            }
            if (!theAsset.witnessAddress.IsValid())
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-address", bSanityCheck);
            }
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                return FormatSyscoinErrorMessage(state, "asset-invalid-flags", bSanityCheck);
            } 
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                return FormatSyscoinErrorMessage(state, "asset-invalid-transfer-address", bSanityCheck);
            }      
            break;

        case SYSCOIN_TX_VERSION_ASSET_UPDATE:
            if (theAsset.nBalance < 0){
                return FormatSyscoinErrorMessage(state, "asset-invalid-balance", bSanityCheck);
            }
            if (!theAssetAllocation.assetAllocationTuple.IsNull())
            {
                return FormatSyscoinErrorMessage(state, "asset-allocations-not-empty", bSanityCheck);
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
            }  
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                return FormatSyscoinErrorMessage(state, "asset-invalid-flags", bSanityCheck);
            }  
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                return FormatSyscoinErrorMessage(state, "asset-invalid-transfer-address", bSanityCheck);
            }           
            break;
            
        case SYSCOIN_TX_VERSION_ASSET_SEND:
            if (theAssetAllocation.listSendingAllocationAmounts.empty())
            {
                return FormatSyscoinErrorMessage(state, "asset-missing-allocations", bSanityCheck);
            }
            if (theAssetAllocation.listSendingAllocationAmounts.size() > 250)
            {
                return FormatSyscoinErrorMessage(state, "asset-too-many-receivers", bSanityCheck);
            }
            if(!theAsset.witnessAddressTransfer.IsNull())   {
                return FormatSyscoinErrorMessage(state, "asset-invalid-transfer-address", bSanityCheck);
            }  
            break;
        case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
            if(theAsset.witnessAddressTransfer.IsNull())   {
                return FormatSyscoinErrorMessage(state, "asset-missing-transfer-address", bSanityCheck);
            }  
            break;
        default:
            return FormatSyscoinErrorMessage(state, "asset-invalid-op", bSanityCheck);
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
                return FormatSyscoinErrorMessage(state, "asset-non-existing-asset", bSanityCheck);
            }
            else
                mapAsset->second = std::move(theAsset);      
        }
        else{
            if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE){
                return FormatSyscoinErrorMessage(state, "asset-already-existing-asset", bSanityCheck);
            }
            mapAsset->second = std::move(dbAsset);      
        }
    }
    CAsset &storedSenderAssetRef = mapAsset->second;   
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER) {
        if (theAsset.nAsset != storedSenderAssetRef.nAsset || storedSenderAssetRef.witnessAddress != theAsset.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-sender", bSanityCheck);
        } 
		if(theAsset.nPrecision != storedSenderAssetRef.nPrecision)
		{
            return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
		}
        if(theAsset.strSymbol != storedSenderAssetRef.strSymbol)
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
        }     
        storedSenderAssetRef.witnessAddress = theAsset.witnessAddressTransfer;   
        // sanity to ensure transfer field is never set on the actual asset in db  
        storedSenderAssetRef.witnessAddressTransfer.SetNull();      
    }

    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {
        if (theAsset.nAsset != storedSenderAssetRef.nAsset || storedSenderAssetRef.witnessAddress != theAsset.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-sender", bSanityCheck);
        }
		if (theAsset.nPrecision != storedSenderAssetRef.nPrecision)
		{
            return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
		}
        if(theAsset.strSymbol != storedSenderAssetRef.strSymbol)
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
        }         
        if (theAsset.nBalance > 0 && !(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_SUPPLY))
        {
            return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
        }          
        // increase total supply
        storedSenderAssetRef.nTotalSupply += theAsset.nBalance;
        storedSenderAssetRef.nBalance += theAsset.nBalance;
        if (theAsset.nBalance < 0 || (theAsset.nBalance > 0 && !AssetRange(theAsset.nBalance, storedSenderAssetRef.nPrecision)))
        {
            return FormatSyscoinErrorMessage(state, "amount-out-of-range", bSanityCheck);
        }
        if (storedSenderAssetRef.nTotalSupply > 0 && !AssetRange(storedSenderAssetRef.nTotalSupply, storedSenderAssetRef.nPrecision))
        {
            return FormatSyscoinErrorMessage(state, "asset-amount-out-of-range", bSanityCheck);
        }
        if (storedSenderAssetRef.nTotalSupply > storedSenderAssetRef.nMaxSupply)
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-supply", bSanityCheck);
        }
		if (!theAsset.vchPubData.empty()) {
			if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_DATA))
			{
				return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
			}
			storedSenderAssetRef.vchPubData = theAsset.vchPubData;
		}
                                    
		if (!theAsset.vchContract.empty() && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_TRANSFER) {
			if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
			{
				return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
			}
			storedSenderAssetRef.vchContract = theAsset.vchContract;
		}
        if(nHeight < Params().GetConsensus().nBridgeStartBlock){
            if(theAsset.nUpdateFlags > 0){
                storedSenderAssetRef.nUpdateFlags = theAsset.nUpdateFlags; 
            }   
        }
        else if (theAsset.nUpdateFlags != storedSenderAssetRef.nUpdateFlags) {
			if (theAsset.nUpdateFlags > 0 && !(storedSenderAssetRef.nUpdateFlags & (ASSET_UPDATE_FLAGS | ASSET_UPDATE_ADMIN))) {
				return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
			}
			storedSenderAssetRef.nUpdateFlags = theAsset.nUpdateFlags;
        } 
    }      
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
        if (storedSenderAssetRef.nAsset != theAssetAllocation.assetAllocationTuple.nAsset || storedSenderAssetRef.witnessAddress != theAssetAllocation.assetAllocationTuple.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
             return FormatSyscoinErrorMessage(state, "asset-invalid-sender", bSanityCheck);
        }

        // check balance is sufficient on sender
        CAmount nTotal = 0;
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            if (amountTuple.second <= 0)
            {
                return FormatSyscoinErrorMessage(state, "asset-invalid-amount", bSanityCheck);
            }
        }
        if (!AssetRange(nTotal))
        {
            return FormatSyscoinErrorMessage(state, "amount-out-of-range", bSanityCheck);
        }
        if (storedSenderAssetRef.nBalance < nTotal) {
            return FormatSyscoinErrorMessage(state, "asset-insufficient-balance", bSanityCheck);
        }
        for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
            if (!bSanityCheck) {
                CAssetAllocationDBEntry receiverAllocation;
                const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
                const string& receiverTupleStr = receiverAllocationTuple.ToString();
                #if __cplusplus > 201402 
                auto result = mapAssetAllocations.try_emplace(receiverTupleStr,  std::move(emptyAllocation));
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
                if (!AssetRange(mapAssetAllocation->second.nBalance))
                {
                    return FormatSyscoinErrorMessage(state, "new-balance-out-of-range", bSanityCheck);
                }                                       
                // adjust sender balance
                storedSenderAssetRef.nBalance -= amountTuple.second;                              
            }
        }
    }
    else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE)
    {
        if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
        {
             return FormatSyscoinErrorMessage(state, "asset-invalid-sender", bSanityCheck);
        }          
        // starting supply is the supplied balance upon init
        storedSenderAssetRef.nTotalSupply = storedSenderAssetRef.nBalance;
    }
    // set the asset's txn-dependent values
    storedSenderAssetRef.nHeight = nHeight;
    storedSenderAssetRef.txHash = txHash;
    // write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
    if (!bSanityCheck && !fJustCheck && nHeight > 0) {
        LogPrint(BCLog::SYS,"CONNECTED ASSET: tx=%s symbol=%d hash=%s height=%d fJustCheck=%d\n",
                assetFromTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? 1 : 0);
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
bool CheckSyscoinLockedOutpoints(const CTransactionRef &tx, TxValidationState &state) {
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
                return FormatSyscoinErrorMessage(state, "lock-non-allocation-input", true, false);
			}
		}
	}
	// ensure that the locked outpoint is being spent
	else if(assetAllocationVersion){
		CAssetAllocationDBEntry assetAllocationDB;
		if (!GetAssetAllocation(theAssetAllocation.assetAllocationTuple, assetAllocationDB)) {
            return FormatSyscoinErrorMessage(state, "lock-non-existing-allocation", true, false);
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
            return FormatSyscoinErrorMessage(state, "lock-missing-lockpoint", false, false);
		}
	}
	return true;
}
bool CEthereumTxRootsDB::PruneTxRoots(const uint32_t &fNewGethSyncHeight) {
    LOCK(cs_setethstatus);
    uint32_t fNewGethCurrentHeight = fGethCurrentHeight;
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    uint32_t cutoffHeight = 0;
    if(fNewGethSyncHeight > 0)
    {
        // cutoff to keep blocks is ~3 week of blocks is about 120k blocks
        cutoffHeight = fNewGethSyncHeight - MAX_ETHEREUM_TX_ROOTS;
        if(fNewGethSyncHeight < MAX_ETHEREUM_TX_ROOTS){
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

    fGethSyncHeight = fNewGethSyncHeight;
    fGethCurrentHeight = fNewGethCurrentHeight;   
    return FlushErase(vecHeightKeys);
}
bool CEthereumTxRootsDB::Init(){
    return PruneTxRoots(0);
}
bool CEthereumTxRootsDB::Clear(){
    LOCK(cs_setethstatus);
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    if (pcursor->Valid()) {
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
    fGethSyncHeight = 0;
    fGethCurrentHeight = 0;     
    return FlushErase(vecHeightKeys);
}

void CEthereumTxRootsDB::AuditTxRootDB(std::vector<std::pair<uint32_t, uint32_t> > &vecMissingBlockRanges){
    LOCK(cs_setethstatus);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    uint32_t nKeyIndex = 0;
    uint32_t nCurrentSyncHeight = 0;
    nCurrentSyncHeight = fGethSyncHeight;

    uint32_t nKeyCutoff = nCurrentSyncHeight - DOWNLOAD_ETHEREUM_TX_ROOTS;
    if(nCurrentSyncHeight < DOWNLOAD_ETHEREUM_TX_ROOTS)
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
            mapTxRoots.emplace(std::piecewise_construct,  std::forward_as_tuple(nKey), std::forward_as_tuple(txRoot));
            #endif            
            
            pcursor->Next();
        }
        catch (std::exception &e) {
            return;
        }
    }
    if(mapTxRoots.size() < 2){
        vecMissingBlockRanges.emplace_back(make_pair(nKeyCutoff, nCurrentSyncHeight));
        return;
    }
    auto setIt = mapTxRoots.begin();
    nKeyIndex = setIt->first;
    setIt++;
    // we should have at least DOWNLOAD_ETHEREUM_TX_ROOTS roots available from the tip for consensus checks
    if(nCurrentSyncHeight >= DOWNLOAD_ETHEREUM_TX_ROOTS && nKeyIndex > nKeyCutoff){
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
                    // this is fine because relayer fetches hundreds headers at a time anyway
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
        batch.Write(key.first.first, key.second);
        // write the bridge transfer ID if it existed (should on mainnet, and testnet after canceltransfer feature introduced)
        if(key.first.second > 0){
            // create link between keys for reorg compatibility because bridge transfer id isn't serialized
            // we could have easily done key.first.second, key.second but that would break under reorgs
            batch.Write(key.first.second, key.first.first);
        } 
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d ethereum tx mints\n", vecMintKeys.size());
    return WriteBatch(batch);
}
bool CEthereumMintedTxDB::FlushErase(const EthereumMintTxVec &vecMintKeys){
    if(vecMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecMintKeys) {
        batch.Erase(key.first.first);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d ethereum tx mints\n", vecMintKeys.size());
    return WriteBatch(batch);
}
