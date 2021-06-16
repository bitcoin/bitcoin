// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <services/assetconsensus.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <nevm/nevm.h>
#include <nevm/address.h>
#include <nevm/sha3.h>
#include <services/asset.h>
#include <script/standard.h>
#include <util/system.h>
#include <messagesigner.h>
#include <util/rbf.h>
#include <undo.h>
#include <validationinterface.h>
std::unique_ptr<CAssetDB> passetdb;
std::unique_ptr<CAssetNFTDB> passetnftdb;
std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
RecursiveMutex cs_setethstatus;
extern std::string EncodeDestination(const CTxDestination& dest);
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const bool &fJustCheck, const bool& bSanityCheck, const int& nHeight, const int64_t& nTime, const uint256& blockhash, NEVMMintTxMap &mapMintKeys, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut) {
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET MINT %d %s %s bSanityCheck=%d\n", nHeight,
            txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
    // unserialize mint object from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        return FormatSyscoinErrorMessage(state, "mint-unserialize", bSanityCheck);
    }
    auto it = tx.voutAssets.begin();
    const uint64_t &nAsset = it->key;
    auto itOut = mapAssetOut.find(nAsset);
    if(itOut == mapAssetOut.end()) {
        return FormatSyscoinErrorMessage(state, "mint-asset-output-notfound", bSanityCheck);             
    } 

    NEVMTxRoot txRootDB;
    bool readTxRootFail;
    {
        LOCK(cs_setethstatus);
        readTxRootFail = !pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(mintSyscoin.nBlockHash, txRootDB);
    }
    if(readTxRootFail) {
        return FormatSyscoinErrorMessage(state, "mint-txroot-missing", bSanityCheck);
    }
     
     // check transaction receipt validity
    dev::RLP rlpReceiptParentNodes(&mintSyscoin.vchReceiptParentNodes);
    std::vector<unsigned char> vchReceiptValue(mintSyscoin.vchReceiptParentNodes.begin()+mintSyscoin.posReceipt, mintSyscoin.vchReceiptParentNodes.end());
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
    // look for TokenFreeze event and get the last parameter which should be the precisions
    uint8_t nERC20Precision = 0;
    uint8_t nSPTPrecision = 0;
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
                // get last data field which should be our precisions
                const std::vector<unsigned char> bridgeIdValue(dataValue.begin()+64, dataValue.end());
                // get precision
                nERC20Precision = static_cast<uint8_t>(bridgeIdValue[31]);
                nSPTPrecision = static_cast<uint8_t>(bridgeIdValue[30]);
            }
        }
    }
    if(nERC20Precision == 0 || nSPTPrecision == 0) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-missing-precision", bSanityCheck);
    }
    // check transaction spv proofs
    dev::RLP rlpTxRoot(&mintSyscoin.vchTxRoot);
    dev::RLP rlpReceiptRoot(&mintSyscoin.vchReceiptRoot);

    if(!txRootDB.vchTxRoot.empty() && mintSyscoin.vchTxRoot != txRootDB.vchTxRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", bSanityCheck);
    }

    if(!txRootDB.vchReceiptRoot.empty() && mintSyscoin.vchReceiptRoot != txRootDB.vchReceiptRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", bSanityCheck);
    }
    
    dev::RLP rlpTxParentNodes(&mintSyscoin.vchTxParentNodes);
    std::vector<unsigned char> vchTxValue(mintSyscoin.vchTxParentNodes.begin()+mintSyscoin.posTx, mintSyscoin.vchTxParentNodes.end());
    // validate mintSyscoin.strTxHash is the hash of vchTxValue
    if(dev::sha3(vchTxValue).hex() != mintSyscoin.strTxHash) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-hash", bSanityCheck);
    }
    dev::RLP rlpTxValue(&vchTxValue);
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    dev::RLP rlpTxPath(&vchTxPath);
    // ensure eth tx not already spent in a previous block
    if(pnevmtxmintdb->Exists(mintSyscoin.strTxHash)) {
        return FormatSyscoinErrorMessage(state, "mint-exists", bSanityCheck);
    } 
    // sanity check is set in mempool during m_test_accept and when miner validates block
    // we care to ensure unique bridge id's in the mempool, not to emplace on test_accept
    if(bSanityCheck) {
        if(mapMintKeys.find(mintSyscoin.strTxHash) != mapMintKeys.end()) {
            return FormatSyscoinErrorMessage(state, "mint-duplicate-transfer", bSanityCheck);
        }
    }
    else {
        // ensure eth tx not already spent in current processing block or mempool(mapMintKeysMempool passed in)
        auto itMap = mapMintKeys.try_emplace(mintSyscoin.strTxHash, txHash);
        if(!itMap.second) {
            return FormatSyscoinErrorMessage(state, "mint-duplicate-transfer", bSanityCheck);
        }
    }
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
    
    CAmount outputAmount;
    uint64_t nAssetNEVM = 0;
    const std::vector<unsigned char> &rlpBytes = rlpTxValue[5].toBytes(dev::RLP::VeryStrict);
    std::vector<unsigned char> vchERC20ContractAddress;
    CTxDestination dest;
    std::string witnessAddress;
    if(!parseNEVMMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, nERC20Precision, nSPTPrecision, rlpBytes, outputAmount, nAssetNEVM, witnessAddress)) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-data", bSanityCheck);
    }
    if(!fRegTest) {
        bool bFoundDest = false;
        // look through outputs to find one that matches the destination with the right asset and asset amount
        for(const auto &vout: tx.vout) {
            if(!ExtractDestination(vout.scriptPubKey, dest)) {
                return FormatSyscoinErrorMessage(state, "mint-extract-destination", bSanityCheck);  
            }
            if(EncodeDestination(dest) == witnessAddress && vout.assetInfo.nAsset == nAsset && vout.assetInfo.nValue == outputAmount) {
                bFoundDest = true;
                break;
            }
        }
        if(!bFoundDest) {
            return FormatSyscoinErrorMessage(state, "mint-mismatch-destination", bSanityCheck);
        }
    }
    if(nAssetNEVM != nAsset) {
        return FormatSyscoinErrorMessage(state, "mint-mismatch-asset", bSanityCheck);
    }
    if(outputAmount <= 0) {
        return FormatSyscoinErrorMessage(state, "mint-value-negative", bSanityCheck);
    }
    
    // if input for this asset exists, must also include it as change in output, so output-input should be the new amount created
    auto itIn = mapAssetIn.find(nAsset);
    CAmount nTotal;
    if(itIn != mapAssetIn.end()) {
        nTotal = itOut->second.nAmount - itIn->second.nAmount;
        if (itIn->second.bZeroVal != itOut->second.bZeroVal) {	
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "mint-zeroval-mismatch");	
        }
    } else {
        nTotal = itOut->second.nAmount;
        // cannot create zero val output without an input
        if(itOut->second.bZeroVal) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "mint-zeroval-without-input");	
        }
    }

    if(outputAmount != nTotal) {
        return FormatSyscoinErrorMessage(state, "mint-mismatch-value", bSanityCheck);  
    }
    if (!MoneyRangeAsset(nTotal)) {
        return FormatSyscoinErrorMessage(state, "mint-value-outofrange", bSanityCheck);
    }
    if(!fJustCheck) {
        if(!bSanityCheck && nHeight > 0) {   
            LogPrint(BCLog::SYS,"CONNECTED ASSET MINT: op=%s asset=%llu hash=%s height=%d fJustCheck=%s\n",
                stringFromSyscoinTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK");      
        }           
    }               
    return true;
}
bool CheckSyscoinInputs(const CTransaction& tx, const Consensus::Params& params, const uint256& txHash, TxValidationState& state, const int &nHeight, const int64_t& nTime, NEVMMintTxMap &mapMintKeys, const bool &bSanityCheck, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut) {
    if(!fRegTest && nHeight < params.nUTXOAssetsBlock)
        return !IsSyscoinTx(tx.nVersion);
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY)
        return false;
    AssetMap mapAssets;
    return CheckSyscoinInputs(false, params, tx, txHash, state, true, nHeight, nTime, uint256(), bSanityCheck, mapAssets, mapMintKeys, mapAssetIn, mapAssetOut);
}

bool CheckSyscoinInputs(const bool &ibd, const Consensus::Params& params, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanityCheck, AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut) {
    bool good = true;
    try{
        if(IsSyscoinMintTx(tx.nVersion)) {
            good = CheckSyscoinMint(ibd, tx, txHash, state, fJustCheck, bSanityCheck, nHeight, nTime, blockHash, mapMintKeys, mapAssetIn, mapAssetOut);
        }
        else if (IsAssetAllocationTx(tx.nVersion)) {
            good = CheckAssetAllocationInputs(tx, txHash, state, fJustCheck, nHeight, blockHash, bSanityCheck, mapAssetIn, mapAssetOut);
        }
        else if (IsAssetTx(tx.nVersion)) {
            good = CheckAssetInputs(params, tx, txHash, state, fJustCheck, nHeight, blockHash, mapAssets, bSanityCheck, mapAssetIn, mapAssetOut);
        } 
    } catch (...) {
        return FormatSyscoinErrorMessage(state, "checksyscoininputs-exception", bSanityCheck);
    }
    return good;
}

bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, NEVMMintTxMap &mapMintKeys){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Cannot unserialize data inside of this transaction relating to an assetallocationmint\n");
        return false;
    }
    mapMintKeys.try_emplace(mintSyscoin.strTxHash, txHash);
    return true;
}

bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CTxUndo& txundo, CCoinsViewCache& view, AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys) {
 
    if(IsSyscoinMintTx(tx.nVersion)) {
        if(!DisconnectMintAsset(tx, txHash, mapMintKeys))
            return false;       
    }
    else {
        if (IsAssetTx(tx.nVersion)) {
            if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
                if(!DisconnectAssetSend(tx, txHash, txundo, mapAssets))
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

uint256 GetNotarySigHash(const CTransaction&tx, const CAssetOut &vecOut) {
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    for (const auto& txin : tx.vin) {
        ss << txin.prevout;
    }
    CTxDestination txDest;
    ss << vecOut.key;
    for (const auto& voutAsset: vecOut.values) {
        if (ExtractDestination(tx.vout[voutAsset.n].scriptPubKey, txDest)) {
            ss << EncodeDestination(txDest);
            ss << voutAsset.nValue;
        }
    }
    return ss.GetHash();
}

bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &state,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut) {
    if (!bSanityCheck)
        LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %s %s bSanityCheck=%d\n", nHeight,
            txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK", bSanityCheck? 1: 0);
        
    const int &nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-output", bSanityCheck);
    }
    // fill notary signatures for every unique base asset that requires it
    std::unordered_map<uint64_t, std::pair<std::vector<unsigned char>, const CAssetOut*> > mapBaseAssets;
    for(const CAssetOut& vecOut: tx.voutAssets) {
        std::vector<unsigned char> vchNotaryKeyID;
        const uint32_t nBaseAsset = GetBaseAssetID(vecOut.key);
        if(GetAssetNotaryKeyID(nBaseAsset, vchNotaryKeyID) && !vchNotaryKeyID.empty()) {
            auto it = mapBaseAssets.try_emplace(nBaseAsset, vchNotaryKeyID, &vecOut);
            // if inserted
            if(it.second) {
                // ensure that the first time its inserted the notary signature cannot be empty
                if(it.first->second.second->vchNotarySig.empty()) {
                     return FormatSyscoinErrorMessage(state, "assetallocation-notary-sig-empty", fJustCheck);
                }
            // if it already exists in map ensure notary signature is empty (only need it on the first map entry per base id)
            } else if(!vecOut.vchNotarySig.empty()) {
                return FormatSyscoinErrorMessage(state, "assetallocation-notary-sig-unexpected", fJustCheck);
            }
        }
    }
    for(const auto &vecOutPair: mapBaseAssets) {
        // if asset has notary signature requirement set
        if (!CHashSigner::VerifyHash(GetNotarySigHash(tx, *vecOutPair.second.second), CKeyID(uint160(vecOutPair.second.first)), vecOutPair.second.second->vchNotarySig)) {
            return FormatSyscoinErrorMessage(state, "assetallocation-notary-sig", fJustCheck);
        }
        
    }
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
        break; 
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
        {   
            auto it = tx.voutAssets.begin();
            const uint64_t &nAsset = it->key;
            const CAmount &nBurnAmount = tx.vout[nOut].nValue;
            if(nBurnAmount <= 0) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-invalid-amount", bSanityCheck);
            }
            auto itOut = mapAssetOut.find(nAsset);
            if(itOut == mapAssetOut.end()) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-asset-output-notfound", bSanityCheck);             
            }
            // if input for this asset exists, must also include it as change in output, so output-input should be the new amount created
            auto itIn = mapAssetIn.find(nAsset);
            CAmount nTotal;
            if(itIn != mapAssetIn.end()) {
                nTotal = itOut->second.nAmount - itIn->second.nAmount;
                if (itIn->second.bZeroVal != itOut->second.bZeroVal) {	
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "syscoin-burn-zeroval-mismatch");	
                }
            } else {
                nTotal = itOut->second.nAmount;
                // cannot create zero val output without an input
                if(itOut->second.bZeroVal) {
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "syscoin-burn-zeroval-without-input");	
                }
            }

            // the burn amount in opreturn (SYS) should match total output for SYSX
            if(nTotal != nBurnAmount) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-mismatch-amount", bSanityCheck);
            }
            if(nAsset != Params().GetConsensus().nSYSXAsset) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-invalid-sysx-asset", bSanityCheck);
            }
        }
        break;
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
        {
            const CAmount &nBurnAmount = tx.vout[nOut].assetInfo.nValue;
            if (nBurnAmount <= 0) {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", bSanityCheck);
            }
            if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
                // the burn of asset in opreturn should match the output value of index 0 (sys)
                if(nBurnAmount != tx.vout[0].nValue) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-mismatch-burn-amount", bSanityCheck);
                }  
                if(tx.vout[nOut].assetInfo.nAsset != Params().GetConsensus().nSYSXAsset) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", bSanityCheck);
                }  
            }            
        } 
        break;
        default:
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-op", bSanityCheck);
    }

    if(!fJustCheck){
        if(!bSanityCheck && nHeight > 0) {  
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s hash=%s height=%d fJustCheck=%s\n",
                stringFromSyscoinTx(tx.nVersion).c_str(),
                txHash.ToString().c_str(),
                nHeight,
                "BLOCK");      
        }             
    }  
    return true;
}

bool DisconnectAssetSend(const CTransaction &tx, const uint256& txid, const CTxUndo& txundo, AssetMap &mapAssets) {
    CAmount inAmount = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const Coin& coin = txundo.vprevout[i];
        if (!coin.out.assetInfo.IsNull()) {
            inAmount += coin.out.assetInfo.nValue;
        }
    }
    CAmount outAmount = 0;
    std::vector<uint64_t> vecNFTKeys;
    const uint32_t& nBaseAsset = GetBaseAssetID(tx.voutAssets[0].key);
    for (unsigned int i = 0; i < tx.voutAssets.size(); ++i) {
        outAmount += tx.GetAssetValueOut(tx.voutAssets[i].values);
        const uint32_t& nBaseAssetInternal = GetBaseAssetID(tx.voutAssets[i].key);
        if(tx.voutAssets[i].key != nBaseAsset && nBaseAssetInternal == nBaseAsset) {
            vecNFTKeys.emplace_back(tx.voutAssets[i].key);
        }
    }
    
    auto result = mapAssets.try_emplace(nBaseAsset,  std::make_pair(vecNFTKeys, std::move(emptyAsset))); 
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        CAsset dbAsset;
        if (!GetAsset(nBaseAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetSend: Could not get asset %d\n",nBaseAsset);
            return false;               
        } 
        mapAsset->second.second = std::move(dbAsset);                        
    }
    CAsset& storedAssetRef = mapAsset->second.second;
    const CAmount &nDiff = (outAmount - inAmount);
    storedAssetRef.nTotalSupply -= nDiff; 
    if(storedAssetRef.nTotalSupply <= 0) {
        storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_SUPPLY;
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
    // nothing was updated
    if(theAsset.nUpdateMask == 0) {
        return true;
    }
    auto it = tx.voutAssets.begin();
    const uint64_t &nAsset = it->key;
    const uint32_t& nBaseAsset = GetBaseAssetID(nAsset);
    std::vector<uint64_t> vecNFTKeys;
    auto result = mapAssets.try_emplace(nBaseAsset,  std::make_pair(vecNFTKeys, std::move(emptyAsset))); 
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound) {
        if (!GetAsset(nBaseAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectAssetUpdate: Could not get asset %d\n",nBaseAsset);
            return false;               
        } 
        mapAsset->second.second = std::move(dbAsset);                    
    }
    CAsset& storedAssetRef = mapAsset->second.second;  
    // undo data fields from last update
    // if fields changed then undo them using prev fields
    if(theAsset.nUpdateMask & ASSET_UPDATE_DATA) {
        storedAssetRef.strPubData = theAsset.strPrevPubData;
        if(!storedAssetRef.strPubData.empty())
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_DATA;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_DATA;
    }
    if(theAsset.nUpdateMask & ASSET_UPDATE_CONTRACT) {
        storedAssetRef.vchContract = theAsset.vchPrevContract;
        if(!storedAssetRef.vchContract.empty())
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_CONTRACT;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_CONTRACT;
    }
    if(theAsset.nUpdateMask & ASSET_UPDATE_NOTARY_KEY) {
        storedAssetRef.vchNotaryKeyID = theAsset.vchPrevNotaryKeyID;
        if(!storedAssetRef.vchNotaryKeyID.empty())
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_NOTARY_KEY;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_NOTARY_KEY;      
    }
    if(theAsset.nUpdateMask & ASSET_UPDATE_NOTARY_DETAILS) {
        storedAssetRef.notaryDetails = theAsset.prevNotaryDetails;
        if(!storedAssetRef.notaryDetails.IsNull())
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_NOTARY_DETAILS;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_NOTARY_DETAILS;          
    }
    if(theAsset.nUpdateMask & ASSET_UPDATE_AUXFEE) {
        storedAssetRef.auxFeeDetails = theAsset.prevAuxFeeDetails;
        if(!storedAssetRef.auxFeeDetails.IsNull())
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_AUXFEE;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_AUXFEE;
    }
    if(theAsset.nUpdateMask & ASSET_UPDATE_CAPABILITYFLAGS) {
        storedAssetRef.nUpdateCapabilityFlags = theAsset.nPrevUpdateCapabilityFlags;
        if(storedAssetRef.nUpdateCapabilityFlags != 0)
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_CAPABILITYFLAGS;
        else
            storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_CAPABILITYFLAGS;  
    }
    return true;  
}

bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txid, AssetMap &mapAssets) {
    auto it = tx.voutAssets.begin();
    const uint32_t &nBaseAsset = GetBaseAssetID(it->key);
    std::vector<uint64_t> vecNFTKeys;
    mapAssets.try_emplace(nBaseAsset,  std::make_pair(vecNFTKeys, std::move(emptyAsset))); 
    return true;  
}

bool CheckAssetInputs(const Consensus::Params& params, const CTransaction &tx, const uint256& txHash, TxValidationState &state,
        const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap& mapAssets, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut) {
    if (!passetdb)
        return false;
    if (!bSanityCheck)
        LogPrint(BCLog::SYS, "*** ASSET %d %s %s\n", nHeight,
            txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK");

    // unserialize asset from txn, check for valid
    CAsset theAsset;
    std::vector<unsigned char> vchData;
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
    auto it = tx.voutAssets.begin();
    const uint64_t &nAsset = it->key;
    const uint32_t &nBaseAsset = GetBaseAssetID(nAsset);
    const std::vector<CAssetOutValue> &vecVout = it->values;
    auto itOut = mapAssetOut.find(nBaseAsset);
    if(itOut == mapAssetOut.end()) {
        return FormatSyscoinErrorMessage(state, "asset-output-notfound", bSanityCheck);             
    }
    // check that the first output asset has zero val output
    if (!itOut->second.bZeroVal) {
        return FormatSyscoinErrorMessage(state, "asset-output-zeroval", bSanityCheck);
    }
    std::vector<uint64_t> vecNFTKeys;
    if(nHeight >= params.nNEVMStartBlock || fRegTest) {
        CAssetsSet mapAssetNFTSet;
        for(auto &voutAsset: tx.voutAssets) {
            const uint32_t& nBaseAssetInternal = GetBaseAssetID(voutAsset.key);
            if(voutAsset.key != nBaseAsset && nBaseAssetInternal == nBaseAsset) {
                vecNFTKeys.emplace_back(voutAsset.key);
                auto result = mapAssetNFTSet.emplace(voutAsset.key);
                const bool & mapAssetNFTNotFound = result.second;
                // check that the NFTID doesn't already exist
                if (!mapAssetNFTNotFound || ExistsNFTAsset(voutAsset.key)) {
                    return FormatSyscoinErrorMessage(state, "asset-nft-duplicate", bSanityCheck);
                }
            }
        }
    }
    CAsset dbAsset;
    auto result = mapAssets.try_emplace(nBaseAsset,  std::make_pair(vecNFTKeys, std::move(emptyAsset))); 
    auto mapAsset = result.first;
    const bool & mapAssetNotFound = result.second;    
    if (mapAssetNotFound) {
        if (!GetAsset(nBaseAsset, dbAsset)) {
            if (tx.nVersion != SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                return FormatSyscoinErrorMessage(state, "asset-non-existing", bSanityCheck);
            }
            else
                mapAsset->second.second = std::move(theAsset);      
        }
        else{
            if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
                return FormatSyscoinErrorMessage(state, "asset-already-existing", bSanityCheck);
            }
            mapAsset->second.second = std::move(dbAsset);      
        }
    } else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
        return FormatSyscoinErrorMessage(state, "asset-already-existing", bSanityCheck);
    }
    CAsset &storedAssetRef = mapAsset->second.second; 
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
        {
            auto itIn = mapAssetIn.find(nBaseAsset);
            // sanity: asset should never exist as an input because it hasn't been created yet
            if(itIn != mapAssetIn.end()) {
                return FormatSyscoinErrorMessage(state, "asset-input-found", bSanityCheck);           
            }
            if(vecVout.size() != 1) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-vout-size", bSanityCheck);
            }
            if(itOut->second.nAmount != 0) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-vout-zeroval", bSanityCheck);
            }
            if (nHeight < Params().GetConsensus().nNEVMStartBlock) {
                if (tx.vout[nOut].nValue < COST_ASSET) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-fee", bSanityCheck);
                }
            }
            if (nBaseAsset <= SYSCOIN_TX_MIN_ASSET_GUID) {
                return FormatSyscoinErrorMessage(state, "asset-guid-too-low", bSanityCheck);
            }
            if (storedAssetRef.nPrecision > 8) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
            }
            if (storedAssetRef.strSymbol.size() > MAX_SYMBOL_SIZE || storedAssetRef.strSymbol.size() < MIN_SYMBOL_SIZE) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
            }
            // must be init and cannot update supply (only when doing assetsend)
            if(!(storedAssetRef.nUpdateMask & ASSET_INIT) || (storedAssetRef.nUpdateMask & ASSET_UPDATE_SUPPLY)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-mask", bSanityCheck);
            }
            if(!storedAssetRef.vchPrevContract.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-prev-contract", bSanityCheck);
            }
            if(storedAssetRef.nUpdateMask & ASSET_UPDATE_CONTRACT) {
                if (storedAssetRef.vchContract.size() != MAX_GUID_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
                }
            } else if (!storedAssetRef.vchContract.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-empty-contract", bSanityCheck); 
            }

            if(!storedAssetRef.strPrevPubData.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-prev-pubdata", bSanityCheck);
            }
            if(storedAssetRef.nUpdateMask & ASSET_UPDATE_DATA) {
                if (storedAssetRef.strPubData.empty()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-pubdata", bSanityCheck);
                }
                if (storedAssetRef.strPubData.size() > MAX_VALUE_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-pubdata-too-big", bSanityCheck);
                }
            } else if (!storedAssetRef.strPubData.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-empty-pubdata", bSanityCheck);
            }

            if(!storedAssetRef.vchPrevNotaryKeyID.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-prev-notary-key", bSanityCheck);
            }
            if(storedAssetRef.nUpdateMask & ASSET_UPDATE_NOTARY_KEY) {
                if (storedAssetRef.vchNotaryKeyID.size() != MAX_GUID_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-notary-key", bSanityCheck);
                }  
            } else if (!storedAssetRef.vchNotaryKeyID.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-empty-notary-key", bSanityCheck);
            }

            if(!storedAssetRef.prevNotaryDetails.IsNull()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-prev-notary", bSanityCheck);
            }
            if(storedAssetRef.nUpdateMask & ASSET_UPDATE_NOTARY_DETAILS) {
                if(storedAssetRef.notaryDetails.IsNull()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-notary", bSanityCheck);
                }
                if (storedAssetRef.notaryDetails.strEndPoint.size() > MAX_VALUE_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-notary-too-big", bSanityCheck);
                }
            } else if (!storedAssetRef.notaryDetails.IsNull()) {
                return FormatSyscoinErrorMessage(state, "asset-empty-notary", bSanityCheck);
            }

            if(!storedAssetRef.prevAuxFeeDetails.IsNull()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-prev-auxfee", bSanityCheck);
            }
            if(storedAssetRef.nUpdateMask & ASSET_UPDATE_AUXFEE) {
                if(storedAssetRef.auxFeeDetails.IsNull()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-auxfee", bSanityCheck);
                }
                if(!storedAssetRef.auxFeeDetails.vchAuxFeeKeyID.empty() && (storedAssetRef.auxFeeDetails.vchAuxFeeKeyID.size() != MAX_GUID_LENGTH)) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-auxfee-key", bSanityCheck);
                }
                if (storedAssetRef.auxFeeDetails.vecAuxFees.size() > MAX_AUXFEES) {
                    return FormatSyscoinErrorMessage(state, "asset-auxfee-too-big", bSanityCheck);
                }
            } else if (!storedAssetRef.auxFeeDetails.IsNull()) {
                return FormatSyscoinErrorMessage(state, "asset-empty-auxfee", bSanityCheck);
            }
            if (!MoneyRangeAsset(storedAssetRef.nMaxSupply)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-maxsupply", bSanityCheck);
            }
            if (nHeight >= Params().GetConsensus().nUTXOAssetsBlockProvisioning) {
                if (nBaseAsset != GenerateSyscoinGuid(tx.vin[0].prevout)) {
                    return FormatSyscoinErrorMessage(state, "asset-guid-not-deterministic", bSanityCheck);
                }
                const std::string& decodedSymbol = DecodeBase64(storedAssetRef.strSymbol);
                if (ToUpper(decodedSymbol) == "SYSX") {
                    return FormatSyscoinErrorMessage(state, "asset-reserved-symbol-sysx", bSanityCheck);
                }
            }
            if(storedAssetRef.nUpdateCapabilityFlags > ASSET_CAPABILITY_ALL) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-capabilityflags", bSanityCheck);
            }
            // activate not allowed to use RBF because GUID is deterministic of first input which cannot be reused       
            if (SignalsOptInRBF(tx)) { 
                return FormatSyscoinErrorMessage(state, "asset-activate-using-rbf", bSanityCheck);
            }
            // clear vouts as we don't need to store them once we have processed.
            storedAssetRef.voutAssets.clear();
            
        }
        break;

        case SYSCOIN_TX_VERSION_ASSET_UPDATE:
        {
            auto itIn = mapAssetIn.find(nBaseAsset);
            if(itIn == mapAssetIn.end()) {
                return FormatSyscoinErrorMessage(state, "asset-input-notfound", bSanityCheck);           
            }
            // check that the first input asset has zero val input spent
            if (!itIn->second.bZeroVal) {
                return FormatSyscoinErrorMessage(state, "asset-input-zeroval", bSanityCheck);
            }
            if (theAsset.nPrecision != storedAssetRef.nPrecision) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-precision", bSanityCheck);
            } 
            if (!theAsset.strSymbol.empty()) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-symbol", bSanityCheck);
            }
            if (theAsset.notaryDetails.strEndPoint.size() > MAX_VALUE_LENGTH) {
                 return FormatSyscoinErrorMessage(state, "asset-invalid-notaryendpoint", bSanityCheck);
            }
            if (theAsset.auxFeeDetails.vecAuxFees.size() > MAX_AUXFEES) {
                 return FormatSyscoinErrorMessage(state, "asset-invalid-auxfees", bSanityCheck);
            }
            // cannot init or send supply
            if((theAsset.nUpdateMask & ASSET_INIT) || (theAsset.nUpdateMask & ASSET_UPDATE_SUPPLY)) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-mask", bSanityCheck);
            }
            if(theAsset.nUpdateMask & ASSET_UPDATE_DATA) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_DATA)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-pubdata-privileges", bSanityCheck);
                }
                if (theAsset.strPubData.size() > MAX_VALUE_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-pubdata-too-big", bSanityCheck);
                } 
                // ensure prevdata is set to what db is now to be able to undo in disconnectblock
                if(theAsset.strPrevPubData != storedAssetRef.strPubData) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevdata", bSanityCheck);
                }
                // replace db data with new data
                storedAssetRef.strPubData = std::move(theAsset.strPubData);
                if (!storedAssetRef.strPubData.empty()) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_DATA;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_DATA;
                }
            } else {
                if(!theAsset.strPrevPubData.empty() || !theAsset.strPubData.empty()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-data", bSanityCheck);
                }
            }
                                        
            if(theAsset.nUpdateMask & ASSET_UPDATE_CONTRACT) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_CONTRACT)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-contract-privileges", bSanityCheck);
                }
                if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
                }
                if(theAsset.vchPrevContract != storedAssetRef.vchContract) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevcontract", bSanityCheck);
                }
                storedAssetRef.vchContract = std::move(theAsset.vchContract);
                if (!storedAssetRef.vchContract.empty()) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_CONTRACT;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_CONTRACT;
                }
            } else {
                if(!theAsset.vchContract.empty() || !theAsset.vchPrevContract.empty()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-contract", bSanityCheck);
                }
            }

            if(theAsset.nUpdateMask & ASSET_UPDATE_NOTARY_KEY) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_NOTARY_KEY)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-notary-key-privileges", bSanityCheck);
                }
                if (!theAsset.vchNotaryKeyID.empty() && theAsset.vchNotaryKeyID.size() != MAX_GUID_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-notary-key", bSanityCheck);
                }  
                if(theAsset.vchPrevNotaryKeyID != storedAssetRef.vchNotaryKeyID) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevnotary-key", bSanityCheck);
                }
                storedAssetRef.vchNotaryKeyID = std::move(theAsset.vchNotaryKeyID);
                if (!storedAssetRef.vchNotaryKeyID.empty()) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_NOTARY_KEY;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_NOTARY_KEY;
                }
            } else {
                if(!theAsset.vchNotaryKeyID.empty() || !theAsset.vchPrevNotaryKeyID.empty()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-notary-key", bSanityCheck);
                }   
            }

            if(theAsset.nUpdateMask & ASSET_UPDATE_NOTARY_DETAILS) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_NOTARY_DETAILS)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-notary-privileges", bSanityCheck);
                }
                if (theAsset.notaryDetails.strEndPoint.size() > MAX_VALUE_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-notary-too-big", bSanityCheck);
                }
                if(theAsset.prevNotaryDetails != storedAssetRef.notaryDetails) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevnotary", bSanityCheck);
                }
                storedAssetRef.notaryDetails = std::move(theAsset.notaryDetails);
                if (!storedAssetRef.notaryDetails.IsNull()) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_NOTARY_DETAILS;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_NOTARY_DETAILS;
                }
            } else {
                if(!theAsset.notaryDetails.IsNull() || !theAsset.prevNotaryDetails.IsNull()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-notary", bSanityCheck);
                }   
            }

            if(theAsset.nUpdateMask & ASSET_UPDATE_AUXFEE) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_AUXFEE)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-auxfee-key-privileges", bSanityCheck);
                }
                if(!theAsset.auxFeeDetails.vchAuxFeeKeyID.empty() && theAsset.auxFeeDetails.vchAuxFeeKeyID.size() != MAX_GUID_LENGTH) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-auxfee-key", bSanityCheck);
                }
                if (theAsset.auxFeeDetails.vecAuxFees.size() > MAX_AUXFEES) {
                    return FormatSyscoinErrorMessage(state, "asset-auxfees-too-big", bSanityCheck);
                }
                if(theAsset.prevAuxFeeDetails != storedAssetRef.auxFeeDetails) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevauxfees", bSanityCheck);
                }
                storedAssetRef.auxFeeDetails = std::move(theAsset.auxFeeDetails);
                
                if (!storedAssetRef.auxFeeDetails.IsNull()) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_AUXFEE;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_AUXFEE;
                }
            } else {
                if(!theAsset.auxFeeDetails.IsNull() || !theAsset.prevAuxFeeDetails.IsNull()) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-auxfees", bSanityCheck);
                }   
            }

            if(theAsset.nUpdateMask & ASSET_UPDATE_CAPABILITYFLAGS) {
                if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_CAPABILITYFLAGS)) {
                    return FormatSyscoinErrorMessage(state, "asset-insufficient-flags-privileges", bSanityCheck);
                }
                if(theAsset.nPrevUpdateCapabilityFlags != storedAssetRef.nUpdateCapabilityFlags) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-prevflags", bSanityCheck);
                }
                if(theAsset.nUpdateCapabilityFlags > ASSET_CAPABILITY_ALL) {
                    return FormatSyscoinErrorMessage(state, "asset-invalid-capabilityflags", bSanityCheck);
                }
                storedAssetRef.nUpdateCapabilityFlags = std::move(theAsset.nUpdateCapabilityFlags);
                if (storedAssetRef.nUpdateCapabilityFlags != 0) {
                    storedAssetRef.nUpdateMask |= ASSET_UPDATE_CAPABILITYFLAGS;
                } else {
                    storedAssetRef.nUpdateMask &= ~ASSET_UPDATE_CAPABILITYFLAGS;
                }
            }
        }         
        break;
            
        case SYSCOIN_TX_VERSION_ASSET_SEND:
        {        
            auto itIn = mapAssetIn.find(nBaseAsset);
            if(itIn == mapAssetIn.end()) {
                return FormatSyscoinErrorMessage(state, "asset-input-notfound", bSanityCheck);           
            } 
            // check that the first input asset has zero val input spent
            if (!itIn->second.bZeroVal) {
                return FormatSyscoinErrorMessage(state, "asset-input-zeroval", bSanityCheck);
            }
            if (!(storedAssetRef.nUpdateCapabilityFlags & ASSET_UPDATE_SUPPLY)) {
                return FormatSyscoinErrorMessage(state, "asset-insufficient-supply-privileges", bSanityCheck);
            }
            // get all output assets and get base ID and whichever ones match nBaseAsset should be added to input map
            CAmount inAmount = itIn->second.nAmount;
            CAmount outAmount = itOut->second.nAmount;
            // track in/out amounts and add to total for any NFT inputs+outputs
            for(auto &itOutNFT: mapAssetOut) {
                const uint32_t& nBaseAssetInternal = GetBaseAssetID(itOutNFT.first);
                // skip first asset and ensure base asset matches base of first asset
                if(itOutNFT.first != nBaseAsset && nBaseAssetInternal == nBaseAsset) { 
                    // NFT output cannot be zero-val
                    if (itOutNFT.second.bZeroVal) {
                        return FormatSyscoinErrorMessage(state, "asset-nft-output-zeroval", bSanityCheck);
                    }
                    // add all output amounts that match the base asset of the first output
                    outAmount += itOutNFT.second.nAmount;
                    // if any inputs from this NFT asset were used add them as input amount
                    auto itIn = mapAssetIn.find(itOutNFT.first);
                    if(itIn != mapAssetIn.end()) {
                       inAmount += itIn->second.nAmount;           
                    }
                }
            }
            // db will be stored with total supply
            // even though new assets must set this flag, it may be unset by disconnectassetsend
            storedAssetRef.nUpdateMask |= ASSET_UPDATE_SUPPLY;
            // this can go negative, inputing an asset without change and not issuing the equivalent will effectively "burn" out of existence but also reduce the supply so it can issue more in the future
            // for this you can issue an assetSend inputing assets from user wishing to burn, as well as zero val input of asset from owner and no output
            const CAmount &nTotal = outAmount - inAmount;
            storedAssetRef.nTotalSupply += nTotal;
            if (nTotal < -MAX_ASSET || nTotal > MAX_ASSET || !MoneyRangeAsset(storedAssetRef.nTotalSupply)) {
                return FormatSyscoinErrorMessage(state, "asset-supply-outofrange", bSanityCheck);
            }
            if (storedAssetRef.nTotalSupply > storedAssetRef.nMaxSupply) {
                return FormatSyscoinErrorMessage(state, "asset-invalid-supply", bSanityCheck);
            }  
        }         
        break;  
        default:
        {
            return FormatSyscoinErrorMessage(state, "asset-invalid-op", bSanityCheck);
        }
    }
    
    if (!bSanityCheck) {
        LogPrint(BCLog::SYS,"CONNECTED ASSET: tx=%s asset=%llu hash=%s height=%d fJustCheck=%s\n",
                stringFromSyscoinTx(tx.nVersion).c_str(),
                nAsset,
                txHash.ToString().c_str(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK");
    } 
    return true;
}

bool CNEVMTxRootsDB::Clear() {
    LOCK(cs_setethstatus);
    std::vector<uint256> vecBlockHashes;
    uint256 nEthBlock;
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    while (pcursor->Valid()) {
        try {
            if(pcursor->GetKey(nEthBlock)) {
                vecBlockHashes.emplace_back(nEthBlock);
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    fGethCurrentHeight = 0;   
    return FlushErase(vecBlockHashes);
}
bool CNEVMTxRootsDB::FlushErase(const std::vector<uint256> &vecBlockHashes) {
    if(vecBlockHashes.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecBlockHashes) {
        batch.Erase(key);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx roots\n", vecBlockHashes.size());
    return WriteBatch(batch);
}

bool CNEVMTxRootsDB::FlushWrite(const NEVMTxRootMap &mapNEVMTxRoots) {
    if(mapNEVMTxRoots.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapNEVMTxRoots) {
        batch.Write(key.first, key.second);
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d nevm tx roots\n", mapNEVMTxRoots.size());
    return WriteBatch(batch);
}

// called on connect
bool CNEVMMintedTxDB::FlushWrite(const NEVMMintTxMap &mapMintKeys) {
    if(mapMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapMintKeys) {
        batch.Write(key.first, key.second);
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d nevm tx mints\n", mapMintKeys.size());
    return WriteBatch(batch);
}

// called on disconnect
bool CNEVMMintedTxDB::FlushErase(const NEVMMintTxMap &mapMintKeys) {
    if(mapMintKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapMintKeys) {
        batch.Erase(key.first);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx mints\n", mapMintKeys.size());
    return WriteBatch(batch);
}


bool CAssetNFTDB::Flush(const AssetMap &mapAssets) {
    if(mapAssets.empty()) {
        return true;
	}
	int write = 0;
	int erase = 0;
    CDBBatch batch(*this);
    for (const auto &key : mapAssets) {
		if (key.second.second.IsNull()) {
            // delete the asset guids used for NFT uniqueness
            for (const auto &keyNFT : key.second.first) {
                erase++;
                batch.Erase(keyNFT);
            }
		}
		else {
            // write the uint64 (asset ID for NFT uniqueness purposes)
            for (const auto &keyNFT : key.second.first) {
                write++;
                batch.Write(keyNFT, true);
            }
		}
    }
    LogPrint(BCLog::SYS, "Flushing %d NFT assets (erased %d, written %d)\n", mapAssets.size(), erase, write);
    return WriteBatch(batch);
}

bool CAssetDB::Flush(const AssetMap &mapAssets) {
    if(mapAssets.empty()) {
        return true;
	}
	int write = 0;
	int erase = 0;
    CDBBatch batch(*this);
    for (const auto &key : mapAssets) {
		if (key.second.second.IsNull()) {
			erase++;
			batch.Erase(key.first);
            // erase keyID field copy
            batch.Erase(std::make_pair(key.first, true));
            // delete the asset guids used for NFT uniqueness
            for (const auto &keyNFT : key.second.first) {
                batch.Erase(keyNFT);
            }
		}
		else {
			write++;
            // int32 (guid) -> CAsset
			batch.Write(key.first, key.second.second);
            // write the uint64 (asset ID for NFT uniqueness purposes)
            for (const auto &keyNFT : key.second.first) {
                batch.Write(keyNFT, true);
            }
            // for optimization on lookups store the keyID separately
            if(key.second.second.vchNotaryKeyID.empty()) {
                batch.Erase(std::make_pair(key.first, true));
            } else {
                batch.Write(std::make_pair(key.first, true), key.second.second.vchNotaryKeyID);
            }
		}
    }
    LogPrint(BCLog::SYS, "Flushing %d assets (erased %d, written %d)\n", mapAssets.size(), erase, write);
    return WriteBatch(batch);
}

CNEVMTxRootsDB::CNEVMTxRootsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.GetDataDirNet() / "nevmtxroots", nCacheSize, fMemory, fWipe) {
}

CNEVMMintedTxDB::CNEVMMintedTxDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.GetDataDirNet() / "nevmminttx", nCacheSize, fMemory, fWipe) {
}

CAssetDB::CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.GetDataDirNet() / "asset", nCacheSize, fMemory, fWipe) {
}

CAssetNFTDB::CAssetNFTDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.GetDataDirNet() / "assetnft", nCacheSize, fMemory, fWipe) {
}

CAssetOldDB::CAssetOldDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.GetDataDirNet() / "assets", nCacheSize, fMemory, fWipe) {
}
