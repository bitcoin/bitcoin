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
std::unique_ptr<CBlockIndexDB> pblockindexdb;
std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
std::unique_ptr<CEthereumMintedTxDB> pethereumtxmintdb;
extern RecursiveMutex cs_setethstatus;
extern bool AbortNode(const std::string& strMessage, const std::string& userMessage = "", unsigned int prefix = 0);
using namespace std;
bool FormatSyscoinErrorMessage(TxValidationState& state, const std::string errorMessage, bool bErrorNotInvalid, bool bConsensus){
        if(bErrorNotInvalid) {
            return state.Error(errorMessage);
        }
        else{
            return state.Invalid(bConsensus? TxValidationResult::TX_CONSENSUS: TxValidationResult::TX_CONFLICT, errorMessage);
        }  
}
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const bool &fJustCheck, const bool& bSanityCheck, const int& nHeight, const int64_t& nTime, const uint256& blockhash, AssetMap& mapAssets, EthereumMintTxVec &vecMintKeys)
{
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET MINT %d %d %s %s bSanityCheck=%d\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
    // unserialize mint object from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        return FormatSyscoinErrorMessage(state, "mint-unserialize", bSanityCheck);
    } 
    // do this check only when not in IBD (initial block download) or litemode
    // if we are starting up and verifying the db also skip this check as fLoaded will be false until startup sequence is complete
    EthereumTxRoot txRootDB;
   
    const bool &ethTxRootShouldExist = !ibd && !fLiteMode && fLoaded && fGethSynced;
    {
        LOCK(cs_setethstatus);
        // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
        // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
        if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)) {
            if(ethTxRootShouldExist) {
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
    if(mintSyscoin.vchReceiptValue.size() == 2) {
        const uint16_t &posReceipt = (static_cast<uint16_t>(mintSyscoin.vchReceiptValue[1])) | (static_cast<uint16_t>(mintSyscoin.vchReceiptValue[0]) << 8);
        vchReceiptValue = std::vector<unsigned char>(mintSyscoin.vchReceiptParentNodes.begin()+posReceipt, mintSyscoin.vchReceiptParentNodes.end());
    }
    else{
        vchReceiptValue = mintSyscoin.vchReceiptValue;
    }
    dev::RLP rlpReceiptValue(&vchReceiptValue);
    
    if (!rlpReceiptValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt", bSanityCheck);
    }
    if (rlpReceiptValue.itemCount() != 4) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-count", bSanityCheck);
    }
    const uint64_t &nStatus = rlpReceiptValue[0].toInt<uint64_t>(dev::RLP::VeryStrict);
    if (nStatus != 1) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-status", bSanityCheck);
    } 
    dev::RLP rlpReceiptLogsValue(rlpReceiptValue[3]);
    if (!rlpReceiptLogsValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-receipt-rlp-logs-list", bSanityCheck);
    }
    const size_t &itemCount = rlpReceiptLogsValue.itemCount();
    // just sanity checks for bounds
    if (itemCount < 1 || itemCount > 10) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-logs-count", bSanityCheck);
    }
    // look for TokenFreeze event and get the last parameter which should be the BridgeTransferID
    uint32_t nBridgeTransferID = 0;
    for(uint32_t i = 0;i<itemCount;i++) {
        dev::RLP rlpReceiptLogValue(rlpReceiptLogsValue[i]);
        if (!rlpReceiptLogValue.isList()) {
            return FormatSyscoinErrorMessage(state, "mint-receipt-log-rlp-list", bSanityCheck);
        }
        // ensure this log has at least the address to check against
        if (rlpReceiptLogValue.itemCount() < 1) {
            return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count", bSanityCheck);
        }
        const dev::Address &address160Log = rlpReceiptLogValue[0].toHash<dev::Address>(dev::RLP::VeryStrict);
        if(Params().GetConsensus().vchSYSXERC20Manager == address160Log.asBytes()) {
            // for mint log we should have exactly 3 entries in it, this event we control through our erc20manager contract
            if (rlpReceiptLogValue.itemCount() != 3) {
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count-bridgeid", bSanityCheck);
            }
            // check topic
            dev::RLP rlpReceiptLogTopicsValue(rlpReceiptLogValue[1]);
            if (!rlpReceiptLogTopicsValue.isList()) {
                return FormatSyscoinErrorMessage(state, "mint-receipt-log-topics-rlp-list", bSanityCheck);
            }
            if (rlpReceiptLogTopicsValue.itemCount() != 1) {
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-topics-count", bSanityCheck);
            }
            // topic hash matches with TokenFreeze signature
            if(Params().GetConsensus().vchTokenFreezeMethod == rlpReceiptLogTopicsValue[0].toBytes(dev::RLP::VeryStrict)) {
                const std::vector<unsigned char> &dataValue = rlpReceiptLogValue[2].toBytes(dev::RLP::VeryStrict);
                if(dataValue.size() < 96) {
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
    if(nBridgeTransferID == 0) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-missing-bridge-id", bSanityCheck);
    }
    // check transaction spv proofs
    dev::RLP rlpTxRoot(&mintSyscoin.vchTxRoot);
    dev::RLP rlpReceiptRoot(&mintSyscoin.vchReceiptRoot);

    if(!txRootDB.vchTxRoot.empty() && rlpTxRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchTxRoot) {
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", bSanityCheck);
    }

    if(!txRootDB.vchReceiptRoot.empty() && rlpReceiptRoot.toBytes(dev::RLP::VeryStrict) != txRootDB.vchReceiptRoot) {
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", bSanityCheck);
    } 
    
    
    const std::vector<unsigned char> &vchTxParentNodes = mintSyscoin.vchTxParentNodes;
    dev::RLP rlpTxParentNodes(&vchTxParentNodes);
    dev::h256 hash;
    std::vector<unsigned char> vchTxValue;
    if(mintSyscoin.vchTxValue.size() == 2) {
        const uint16_t &posTx = (static_cast<uint16_t>(mintSyscoin.vchTxValue[1])) | (static_cast<uint16_t>(mintSyscoin.vchTxValue[0]) << 8);
        vchTxValue = std::vector<unsigned char>(mintSyscoin.vchTxParentNodes.begin()+posTx, mintSyscoin.vchTxParentNodes.end());
        hash = dev::sha3(vchTxValue);
    }
    else {
        vchTxValue = mintSyscoin.vchTxValue;
        hash = dev::sha3(mintSyscoin.vchTxValue);
    }
    dev::RLP rlpTxValue(&vchTxValue);
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    dev::RLP rlpTxPath(&vchTxPath);
    const std::vector<unsigned char> &vchHash = hash.asBytes();
    // ensure eth tx not already spent
    if(pethereumtxmintdb->ExistsKey(vchHash)) {
        return FormatSyscoinErrorMessage(state, "mint-exists", bSanityCheck);
    } 
    // add the key to flush to db later
    vecMintKeys.emplace_back(std::make_pair(std::make_pair(vchHash, nBridgeTransferID), txHash));
    
    // verify receipt proof
    if(!VerifyProof(&vchTxPath, rlpReceiptValue, rlpReceiptParentNodes, rlpReceiptRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-receipt-proof", bSanityCheck);
    } 
    // verify transaction proof
    if(!VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-proof", bSanityCheck);
    } 
    if (!rlpTxValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-rlp-list", bSanityCheck);
    }
    if (rlpTxValue.itemCount() < 6) {
        return FormatSyscoinErrorMessage(state, "mint-tx-itemcount", bSanityCheck);
    }        
    if (!rlpTxValue[5].isData()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-array", bSanityCheck);
    }        
    if (rlpTxValue[3].isEmpty()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-invalid-receiver", bSanityCheck);
    }                       
    const dev::Address &address160 = rlpTxValue[3].toHash<dev::Address>(dev::RLP::VeryStrict);

    // ensure ERC20Manager is in the "to" field for the contract, meaning the function was called on this contract for freezing supply
    if(Params().GetConsensus().vchSYSXERC20Manager != address160.asBytes()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-contract-manager", bSanityCheck);
    }
    CAsset dbAsset;
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(mintSyscoin.assetAllocation.nAsset,  std::move(emptyAsset));
    #else
    auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(mintSyscoin.assetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif   
   
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        if (!GetAsset(mintSyscoin.assetAllocation.nAsset, dbAsset)) {
            return FormatSyscoinErrorMessage(state, "mint-non-existing-asset", bSanityCheck);             
        } 
        mapAsset->second = std::move(dbAsset);                        
    }
    CAsset& storedAssetRef = mapAsset->second;
    CAmount outputAmount;
    uint32_t nAsset = 0;
    const std::vector<unsigned char> &rlpBytes = rlpTxValue[5].toBytes(dev::RLP::VeryStrict);
    std::vector<unsigned char> vchERC20ContractAddress;
    CWitnessAddress witnessAddress;
    CTxDestination dest;
    if(!parseEthMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, rlpBytes, dbAsset.vchContract, outputAmount, nAsset, dbAsset.nPrecision, witnessAddress)) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-data", bSanityCheck);
    }
    if(!ExtractDestination(tx.vout[0].scriptPubKey, dest)) {
        return FormatSyscoinErrorMessage(state, "mint-extract-destination", bSanityCheck);  
    }
    if(!fUnitTest) {
        if(EncodeDestination(dest) != witnessAddress.ToString()) {
            return FormatSyscoinErrorMessage(state, "mint-mismatch-destination", bSanityCheck);  
        }
        if(nAsset != mintSyscoin.assetAllocation.nAsset) {
            return FormatSyscoinErrorMessage(state, "mint-mismatch-asset", bSanityCheck);
        }
    } 
    if(outputAmount <= 0) {
        return FormatSyscoinErrorMessage(state, "mint-value-negative", bSanityCheck);
    }
    if(mintSyscoin.assetAllocation.voutAssets.size() != 1) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-asset-outputs", bSanityCheck);  
    }
    const CAmount &nTotal = mintSyscoin.assetAllocation.voutAssets[0];
    if(outputAmount != nTotal) {
        return FormatSyscoinErrorMessage(state, "mint-mismatch-value", bSanityCheck);  
    }
    if(nTotal > storedAssetRef.nBurnBalance) {
        return FormatSyscoinErrorMessage(state, "mint-insufficient-burn-balance", bSanityCheck);  
    }
    storedAssetRef.nBurnBalance -= nTotal;
    if(!fJustCheck) {
        if(!bSanityCheck && nHeight > 0) {   
            LogPrint(BCLog::SYS,"CONNECTED ASSET MINT: op=%s asset=%d hash=%s height=%d fJustCheck=%s\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                mintSyscoin.assetAllocation.nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK");      
        }           
    }               
    return true;
}
bool CheckSyscoinInputs(const CTransaction& tx, const CAssetAllocation& allocation, const uint256& txHash, TxValidationState& state, const int &nHeight, const int64_t& nTime) {
    if(nHeight < nUTXOAssetsBlock)
        return true;
    AssetMap mapAssets;
    EthereumMintTxVec vecMintKeys;
    return CheckSyscoinInputs(false, tx, allocation, txHash, state, inputs, true, nHeight, nTime, uint256(), false, mapAssets, vecMintKeys);
}
bool CheckSyscoinInputs(const bool &ibd, const CTransaction& tx, const CAssetAllocation& allocation, const uint256& txHash, TxValidationState& state, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanityCheck, AssetMap &mapAssets, EthereumMintTxVec &vecMintKeys) {
    bool good = true;
    try{
        if(IsSyscoinMintTx(tx.nVersion)) {
            good = CheckSyscoinMint(ibd, tx, txHash, state, fJustCheck, bSanityCheck, nHeight, nTime, blockHash, mapAssets, vecMintKeys);
        }
        else if (IsAssetAllocationTx(tx.nVersion)) {
            good = CheckAssetAllocationInputs(tx, allocation, txHash, state, fJustCheck, nHeight, blockHash, mapAssets, bSanityCheck);
        }
        else if (IsAssetTx(tx.nVersion)) {
            good = CheckAssetInputs(tx, allocation, txHash, state, fJustCheck, nHeight, blockHash, mapAssets, bSanityCheck);
        } 
    } catch (...) {
        return FormatSyscoinErrorMessage(state, "checksyscoininputs-exception", bSanityCheck);
    }
    return good;
}
bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, EthereumMintTxVec &vecMintKeys, AssetMap &mapAssets){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Cannot unserialize data inside of this transaction relating to an assetallocationmint\n");
        return false;
    }
    // remove eth spend tx from our internal db
    dev::h256 hash;
    if(mintSyscoin.vchTxValue.size() == 2) {
        const unsigned short &posTx = ((mintSyscoin.vchTxValue[0]<<8)|(mintSyscoin.vchTxValue[1]));
        const std::vector<unsigned char> &vchTxValue = std::vector<unsigned char>(mintSyscoin.vchTxParentNodes.begin()+posTx, mintSyscoin.vchTxParentNodes.end());
        hash = dev::sha3(vchTxValue);
    }
    else {
        hash = dev::sha3(mintSyscoin.vchTxValue);
    }

    const std::vector<unsigned char> &vchHash = hash.asBytes();
    vecMintKeys.emplace_back(std::make_pair(std::make_pair(vchHash, 0), txHash));
    
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(mintSyscoin.assetAllocation.nAsset,  std::move(emptyAsset));
    #else
    auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(mintSyscoin.assetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif   
   
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        CAsset dbAsset;
        if (!GetAsset(mintSyscoin.assetAllocation.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectMintAsset: Could not get asset %d\n",mintSyscoin.assetAllocation.nAsset);
            return false;              
        } 
        mapAsset->second = std::move(dbAsset);                        
    }
    CAsset& storedAssetRef = mapAsset->second;
    CAmount outputAmount;
    CAmount nTotal = 0;
    for(const auto& voutAsset: mintSyscoin.assetAllocation.voutAssets) {
        nTotal += voutAsset;
    }
    storedAssetRef.nBurnBalance += nTotal;
    return true;
}
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, EthereumMintTxVec &vecMintKeys)
{
    if(tx.IsCoinBase())
        return true;
 
    if(IsSyscoinMintTx(tx.nVersion)) {
        if(!DisconnectMintAsset(tx, txHash, vecMintKeys, mapAssets))
            return false;       
    }
    else{
        if (IsAssetAllocationTx(tx.nVersion)) {
            if(!DisconnectAssetAllocationSend(tx, txHash, mapAssets))
                return false;
        }
        else if (IsAssetTx(tx.nVersion)) {
            if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
                if(!DisconnectAssetSend(tx, txHash, mapAssets))
                    return false;
            } else if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE) {  
                if(!DisconnectAssetUpdate(tx, txHash, mapAssets))
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
bool CheckAssetAllocationInputs(const CTransaction &tx, const CAssetAllocation &theAssetAllocation, const uint256& txHash, TxValidationState &state,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, const bool &bSanityCheck) {
    if (passetallocationdb == nullptr)
        return false;
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %d %s %s bSanityCheck=%d\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
        
    const int &nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-output", bSanityCheck);
    }
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
            const CAmount &nBurnAmount = theAssetAllocation.voutAssets.size() > nOut? theAssetAllocation.voutAssets[nOut]: 0;
            // if there were any burns we should adjust asset burn balance
            if(nBurnAmount > 0) {
                #if __cplusplus > 201402 
                auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
                #else
                auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
                #endif   
            
                auto mapAsset = result.first;
                const bool& mapAssetNotFound = result.second;
                if(mapAssetNotFound) {
                    CAsset dbAsset;
                    if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                        return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-asset", bSanityCheck);             
                    } 
                    mapAsset->second = std::move(dbAsset);                        
                }
                CAsset& storedAssetRef = mapAsset->second;
                storedAssetRef.nBurnBalance += nBurnAmount;
                if (storedAssetRef.nBurnBalance > storedAssetRef.nTotalSupply)) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-balance", bSanityCheck);
                }
            }
            break; 
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
            const CAmount &nBurnAmount = tx.vout[nOut].nValue;
            if(nBurnAmount <= 0) {
                return FormatSyscoinErrorMessage(state, "assetallocation-positive-burn-amount", bSanityCheck);
            }
            CAmount nAmountAsset = 0;
            for(const auto& voutAsset: theAssetAllocation.voutAssets){
                nAmountAsset += voutAsset;
            }
            // the burn amount in opreturn (sys) should match the asset output total (sysx)
            if(nBurnAmount != nAmountAsset) {
                return FormatSyscoinErrorMessage(state, "assetallocation-mismatch-burn-amount", bSanityCheck);
            }
            if(!fUnitTest && theAssetAllocation.nAsset != Params().GetConsensus().nSYSXAsset) {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", bSanityCheck);
            }
            #if __cplusplus > 201402 
            auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
            #else
            auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
            #endif   
        
            auto mapAsset = result.first;
            const bool& mapAssetNotFound = result.second;
            if(mapAssetNotFound) {
                CAsset dbAsset;
                if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-asset", bSanityCheck);             
                } 
                mapAsset->second = std::move(dbAsset);                        
            }
            CAsset& storedAssetRef = mapAsset->second;  
            if (nBurnAmount <= 0 || nBurnAmount > storedAssetRef.nBurnBalance) {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", bSanityCheck);
            }
            storedAssetRef.nBurnBalance -= nBurnAmount;
            break;
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
            if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM) {
                CBurnSyscoin burnSyscoin(tx);
                if(burnSyscoin.IsNull()) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-parse-burn", bSanityCheck);
                }
                if(storedAssetRef.vchContract.empty() || storedAssetRef.vchContract != burnSyscoin.vchEthContract) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-contract", bSanityCheck);
                }        
            }

            const CAmount &nBurnAmount = theAssetAllocation.voutAssets.size() > nOut? theAssetAllocation.voutAssets[nOut]: 0;
            if(nBurnAmount <= 0 || nBurnAmount > storedAssetRef.nTotalSupply) {
                return FormatSyscoinErrorMessage(state, "assetallocation-positive-burn-amount", bSanityCheck);
            }
            #if __cplusplus > 201402 
            auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
            #else
            auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
            #endif   
        
            auto mapAsset = result.first;
            const bool& mapAssetNotFound = result.second;
            if(mapAssetNotFound) {
                CAsset dbAsset;
                if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-non-existing-asset", bSanityCheck);             
                } 
                mapAsset->second = std::move(dbAsset);                        
            }
            CAsset& storedAssetRef = mapAsset->second;
            storedAssetRef.nBurnBalance += nBurnAmount;
            if (storedAssetRef.nBurnBalance > storedAssetRef.nTotalSupply)) {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-balance", bSanityCheck);
            }
            if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
                // the burn of asset in opreturn should match the output value of index 0 (sys)
                if(nBurnAmount != tx.vout[0].nValue) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", bSanityCheck);
                }  
                if(!fUnitTest && theAssetAllocation.nAsset != Params().GetConsensus().nSYSXAsset) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", bSanityCheck);
                }  
            }            
            if(storedAssetRef.vchContract.empty()) {
                return FormatSyscoinErrorMessage(state, "assetallocation-missing-contract", bSanityCheck);
            } 
            break;            
        default:
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-op", bSanityCheck);
    }

    if(!fJustCheck){
        if(!bSanityCheck && nHeight > 0) {  
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s hash=%s height=%d fJustCheck=%s\n",
                assetAllocationFromTx(tx.nVersion).c_str(),
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK");      
        }             
    }  
    return true;
}
// revert asset burn balance based on any burns done in asset allocation tx
bool DisconnectAssetAllocation(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets) {
    CAssetAllocation theAssetAllocation(tx);
    const int &nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not find data output\n");
        return false;
    }
    if(theAssetAllocation.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not decode asset allocation\n");
        return false;
    }
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
            const CAmount &nBurnAmount = tx.vout[nOut].nValue;
            #if __cplusplus > 201402 
            auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
            #else
            auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
            #endif   
        
            auto mapAsset = result.first;
            const bool& mapAssetNotFound = result.second;
            if(mapAssetNotFound) {
                CAsset dbAsset;
                if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                    LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not get asset %d\n",theAssetAllocation.nAsset);
                    return false;                
                } 
                mapAsset->second = std::move(dbAsset);                        
            }
            CAsset& storedAssetRef = mapAsset->second;  
            storedAssetRef.nBurnBalance += nBurnAmount; 
            break;  
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN: 
            const CAmount &nBurnAmount = theAssetAllocation.voutAssets[nOut];
            #if __cplusplus > 201402 
            auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
            #else
            auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
            #endif   
        
            auto mapAsset = result.first;
            const bool& mapAssetNotFound = result.second;
            if(mapAssetNotFound) {
                CAsset dbAsset;
                if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                    LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not get asset %d\n",theAssetAllocation.nAsset);
                    return false;               
                } 
                mapAsset->second = std::move(dbAsset);                        
            }
            CAsset& storedAssetRef = mapAsset->second;
            if (nBurnAmount > storedAssetRef.nBurnBalance) {
                LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Insufficient burn balance\n");
                return false;
            }
            storedAssetRef.nBurnBalance -= nBurnAmount;  
            break;
        case SYSCOIN_TX_VERSION_SYSCOIN_ALLOCATION_SEND: 
            const CAmount &nBurnAmount = theAssetAllocation.voutAssets.size() > nOut? theAssetAllocation.voutAssets[nOut]: 0;
            // if there were any burns we should adjust asset burn balance
            if(nBurnAmount > 0) {
                #if __cplusplus > 201402 
                auto result = mapAssets.try_emplace(theAssetAllocation.nAsset,  std::move(emptyAsset));
                #else
                auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(theAssetAllocation.nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
                #endif   
                auto mapAsset = result.first;
                const bool& mapAssetNotFound = result.second;
                if(mapAssetNotFound) {
                    CAsset dbAsset;
                    if (!GetAsset(theAssetAllocation.nAsset, dbAsset)) {
                        LogPrint(BCLog::SYS,"DisconnectAssetAllocation: Could not get asset %d\n",theAssetAllocation.nAsset);
                        return false;                
                    } 
                    mapAsset->second = std::move(dbAsset);                        
                }
                CAsset& storedAssetRef = mapAsset->second;
                if (storedAssetRef.nBurnBalance < nBurnAmount)) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-balance", bSanityCheck);
                }
                storedAssetRef.nBurnBalance -= nBurnAmount;
            }
            break;
        default:
            break;
    }
    return true;  
}
bool DisconnectAssetSend(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets) {
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not decode asset allocation in asset send\n");
        return false;
    } 
    const int &nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not find data output\n");
        return false;
    }
    const uint32_t &nAsset = theAssetAllocation.nAsset;
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(nAsset,  std::move(emptyAsset));
    #else
    auto result = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif   
   
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        CAsset dbAsset;
        if (!GetAsset(nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not get asset %d\n",nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                        
    }
    CAsset& storedAssetRef = mapAsset->second;
    CAmount nTotal = 0;
    for(const auto& voutAsset: theAssetAllocation.voutAssets){
        nTotal += voutAsset;
    }
    storedAssetRef.nBalance += nTotal;
    const CAmount &nBurnAmount = theAssetAllocation.voutAssets.size() > nOut? theAssetAllocation.voutAssets[nOut]: 0;
    // if there were any burns we should adjust asset burn balance
    if(nBurnAmount > 0) {
        if (storedAssetRef.nBurnBalance < nBurnAmount) {
            LogPrint(BCLog::SYS,"DisconnectAssetSend: Insufficient burn balance");
            return false;
        }
        storedAssetRef.nBurnBalance -= nBurnAmount;
    }         
    return true;  
}
bool DisconnectAssetUpdate(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets) {
    CAsset dbAsset;
    CAsset theAsset(tx);
    if(theAsset.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not decode asset\n");
        return false;
    }
    const uint32_t &nAsset = theAsset.assetAllocation.nAsset;
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif     

    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        if (!GetAsset(nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not get asset %d\n",nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                    
    }
    CAsset& storedAssetRef = mapAsset->second;   
           
    if(theAsset.nBalance > 0) {
        // reverse asset minting by the issuer
        storedAssetRef.nBalance -= theAsset.nBalance;
        storedAssetRef.nTotalSupply -= theAsset.nBalance;
        if(storedAssetRef.nBalance < 0 || storedAssetRef.nTotalSupply < 0) {
            LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Asset cannot be negative: Balance %lld, Supply: %lld\n",storedAssetRef.nBalance, storedAssetRef.nTotalSupply);
            return false;
        }                                        
    }         
    return true;  
}
bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets) {
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectAssetActivate: Could not decode asset allocation in asset activate\n");
        return false;
    } 
    const uint32_t &nAsset = theAssetAllocation.nAsset;
    #if __cplusplus > 201402 
    mapAssets.try_emplace(nAsset,  std::move(emptyAsset));
    #else
    mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif 
    return true;  
}
bool CheckAssetInputs(const CTransaction &tx, const CAssetAllocation &theAssetAllocation, const uint256& txHash, TxValidationState &state,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap& mapAssets, const bool &bSanityCheck) {
    if (passetdb == nullptr)
        return false;
    if (!bSanityCheck)
        LogPrint(BCLog::SYS, "*** ASSET %d %d %s %s\n", nHeight,
            ::ChainActive().Tip()->nHeight, txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK");

    // unserialize asset from txn, check for valid
    CAsset theAsset;
    vector<unsigned char> vchData;

    if(tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND) {
        theAsset = CAsset(tx);
        if(theAsset.IsNull()) {
            return FormatSyscoinErrorMessage(state, "asset-unserialize", bSanityCheck);
        }
    }
    const int &nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        return FormatSyscoinErrorMessage(state, "asset-missing-burn-output", bSanityCheck);
    } 
    CAsset dbAsset;
    const uint32_t &nAsset = tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND ? theAssetAllocation.nAsset : theAsset.assetAllocation.nAsset;
    #if __cplusplus > 201402 
    auto result = mapAssets.try_emplace(nAsset,  std::move(emptyAsset));
    #else
    auto result  = mapAssets.emplace(std::piecewise_construct,  std::forward_as_tuple(nAsset),  std::forward_as_tuple(std::move(emptyAsset)));
    #endif  
    auto mapAsset = result.first;
    const bool & mapAssetNotFound = result.second;    
    if (mapAssetNotFound) {
        if (!GetAsset(nAsset, dbAsset)) {
            if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                return FormatSyscoinErrorMessage(state, "asset-non-existing-asset", bSanityCheck);
            }
            else
                mapAsset->second = std::move(theAsset);      
        }
        else{
            if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                return FormatSyscoinErrorMessage(state, "asset-already-existing-asset", bSanityCheck);
            }
            mapAsset->second = std::move(dbAsset);      
        }
    }
    CAsset &storedAssetRef = mapAsset->second; 
    if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH) {
        return FormatSyscoinErrorMessage(state, "asset-pubdata-too-big", bSanityCheck);
    } 
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
            if (!fUnitTest && tx.vout[nOut].nValue < 500*COIN) {
                return FormatSyscoinErrorMessage(state, "asset-insufficient-fee", bSanityCheck);
            }
            if (theAsset.assetAllocation.nAsset <= (SYSCOIN_TX_VERSION_ALLOCATION_SEND*10)) {
                return FormatSyscoinErrorMessage(state, "asset-guid-invalid", bSanityCheck);
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
            }  
            if (theAsset.nPrecision > 8) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
            }
            if (theAsset.strSymbol.size() > 8 || theAsset.strSymbol.size() < 1) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
            }
            if (!AssetRange(theAsset.nMaxSupply)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-maxsupply", bSanityCheck);
            }
            if (theAsset.nBalance > theAsset.nMaxSupply || (theAsset.nBalance <= 0)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-totalsupply", bSanityCheck);
            }
            if (theAsset.nUpdateFlags > ASSET_UPDATE_ALL) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-flags", bSanityCheck);
            } 
            if (nAsset != GenerateSyscoinGuid(tx.vin[0].prevout)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-guid", bSanityCheck);
            }         
            // starting supply is the supplied balance upon init
            storedAssetRef.nTotalSupply = theAsset.nBalance;
            storedAssetRef.nBurnBalance = 0;  
            break;

        case SYSCOIN_TX_VERSION_ASSET_UPDATE:
            if (theAsset.nBalance < 0) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-balance", bSanityCheck);
            }
            if (!theAssetAllocation.IsNull()) {
                return FormatSyscoinErrorMessage(state, "asset-allocations-not-empty", bSanityCheck);
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
            }  
            if (theAsset.nUpdateFlags > ASSET_UPDATE_ALL) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-flags", bSanityCheck);
            }
            if (theAsset.nPrecision != storedAssetRef.nPrecision) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
            }
            if (theAsset.strSymbol != storedAssetRef.strSymbol) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
            }         
            if (theAsset.nBalance > 0 && !(storedAssetRef.nUpdateFlags & ASSET_UPDATE_SUPPLY)) {
                return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
            }          
            // increase total supply
            storedAssetRef.nTotalSupply += theAsset.nBalance;
            storedAssetRef.nBalance += theAsset.nBalance;
            if (theAsset.nBalance < 0 || (theAsset.nBalance > 0 && !AssetRange(theAsset.nBalance))) {
                return FormatSyscoinErrorMessage(state, "amount-out-of-range", bSanityCheck);
            }
            if (storedAssetRef.nTotalSupply > 0 && !AssetRange(storedAssetRef.nTotalSupply)) {
                return FormatSyscoinErrorMessage(state, "asset-amount-out-of-range", bSanityCheck);
            }
            if (storedAssetRef.nTotalSupply > storedAssetRef.nMaxSupply) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-supply", bSanityCheck);
            }
            if (!theAsset.vchPubData.empty()) {
                if (!(storedAssetRef.nUpdateFlags & ASSET_UPDATE_DATA)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
                }
                storedAssetRef.vchPubData = theAsset.vchPubData;
            }
                                        
            if (!theAsset.vchContract.empty()) {
                if (!(storedAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
                }
                storedAssetRef.vchContract = theAsset.vchContract;
            }
            if (theAsset.nUpdateFlags != storedAssetRef.nUpdateFlags) {
                if (theAsset.nUpdateFlags > 0 && !(storedAssetRef.nUpdateFlags & (ASSET_UPDATE_FLAGS | ASSET_UPDATE_ADMIN))) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-privileges", bSanityCheck);
                }
                storedAssetRef.nUpdateFlags = theAsset.nUpdateFlags;
            }             
            break;
            
        case SYSCOIN_TX_VERSION_ASSET_SEND:
            CAmount nTotal = 0;
            for(const auto& voutAsset: theAssetAllocation.voutAssets){
                nTotal += assetInfo.second;
            }
            if (storedAssetRef.nBalance < nTotal) {
                return FormatSyscoinErrorMessage(state, "asset-insufficient-balance", bSanityCheck);
            }
            storedAssetRef.nBalance -= nTotal;
            const CAmount &nBurnAmount = theAssetAllocation.voutAssets.size() > nOut? theAssetAllocation.voutAssets[nOut]: 0;
            // if there were any burns we should adjust asset burn balance
            if(nBurnAmount > 0) {
                storedAssetRef.nBurnBalance += nBurnAmount;
                if (storedAssetRef.nBurnBalance > storedAssetRef.nTotalSupply)) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-burn-balance", bSanityCheck);
                }
            }            
            break;
        default:
            return FormatSyscoinErrorMessage(state, "asset-invalid-op", bSanityCheck);
    }
    
    if (!bSanityCheck) {
        LogPrint(BCLog::SYS,"CONNECTED ASSET: tx=%s asset=%d hash=%s height=%d fJustCheck=%s\n",
                assetFromTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK");
    } 
    return true;
}
bool CBlockIndexDB::FlushErase(const std::vector<uint256> &vecTXIDs) {
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
bool CEthereumTxRootsDB::PruneTxRoots(const uint32_t &fNewGethSyncHeight) {
    LOCK(cs_setethstatus);
    uint32_t fNewGethCurrentHeight = fGethCurrentHeight;
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    uint32_t cutoffHeight = 0;
    if(fNewGethSyncHeight > 0) {
        // cutoff to keep blocks is ~3 week of blocks is about 120k blocks
        cutoffHeight = fNewGethSyncHeight - MAX_ETHEREUM_TX_ROOTS;
        if(fNewGethSyncHeight < MAX_ETHEREUM_TX_ROOTS) {
            LogPrint(BCLog::SYS, "Nothing to prune fGethSyncHeight = %d\n", fNewGethSyncHeight);
            return true;
        }
    }
    std::vector<unsigned char> txPos;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if(pcursor->GetKey(nKey)) {
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
bool CEthereumTxRootsDB::Init() {
    return PruneTxRoots(0);
}
bool CEthereumTxRootsDB::Clear() {
    LOCK(cs_setethstatus);
    vector<uint32_t> vecHeightKeys;
    uint32_t nKey = 0;
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    if (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            if(pcursor->GetKey(nKey)) {
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
            if(!pcursor->GetKey(nKey)) {
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
    if(mapTxRoots.size() < 2) {
        vecMissingBlockRanges.emplace_back(make_pair(nKeyCutoff, nCurrentSyncHeight));
        return;
    }
    auto setIt = mapTxRoots.begin();
    nKeyIndex = setIt->first;
    setIt++;
    // we should have at least DOWNLOAD_ETHEREUM_TX_ROOTS roots available from the tip for consensus checks
    if(nCurrentSyncHeight >= DOWNLOAD_ETHEREUM_TX_ROOTS && nKeyIndex > nKeyCutoff) {
        vecMissingBlockRanges.emplace_back(make_pair(nKeyCutoff, nKeyIndex-1));
    }
    std::vector<unsigned char> vchPrevHash;
    std::vector<uint32_t> vecRemoveKeys;
    // find sequence gaps in sorted key set 
    for (; setIt != mapTxRoots.end(); ++setIt) {
            const uint32_t &key = setIt->first;
            const uint32_t &nNextKeyIndex = nKeyIndex+1;
            if (key != nNextKeyIndex && (key-1) >= nNextKeyIndex)
                vecMissingBlockRanges.emplace_back(make_pair(nNextKeyIndex, key-1));
            // if continious index we want to ensure hash chain is also continious
            else {
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
    if(!vecRemoveKeys.empty()) {
        LogPrint(BCLog::SYS, "Detected an %d inconsistent hash chains in Ethereum headers, removing...\n", vecRemoveKeys.size());
        pethereumtxrootsdb->FlushErase(vecRemoveKeys);
    }
}
bool CEthereumTxRootsDB::FlushErase(const std::vector<uint32_t> &vecHeightKeys) {
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
bool CEthereumTxRootsDB::FlushWrite(const EthereumTxRootMap &mapTxRoots) {
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
bool CEthereumMintedTxDB::FlushWrite(const EthereumMintTxVec &vecMintKeys) {
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
bool CEthereumMintedTxDB::FlushErase(const EthereumMintTxVec &vecMintKeys) {
    if(vecMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecMintKeys) {
        batch.Erase(key.first.first);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d ethereum tx mints\n", vecMintKeys.size());
    return WriteBatch(batch);
}
