// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <services/nevmconsensus.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <nevm/nevm.h>
#include <nevm/address.h>
#include <nevm/sha3.h>
#include <messagesigner.h>
#include <logging.h>
#include <util/rbf.h>
#include <undo.h>
#include <validationinterface.h>
#include <timedata.h>
#include <key_io.h>
std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
std::unique_ptr<CBlockIndexDB> pblockindexdb;
std::unique_ptr<CNEVMDataDB> pnevmdatadb;
RecursiveMutex cs_setethstatus;
uint32_t nLastKnownHeightOnStart = 0;
bool fNEVMConnection = false;
bool fRegTest = false;
bool fSigNet = false;
bool CheckSyscoinMint(const CTransaction& tx, TxValidationState& state, const bool &fJustCheck, const uint32_t& nHeight, NEVMMintTxMap &mapMintKeys, CAmount &nValue) {
    const uint256& txHash = tx.GetHash();
    // unserialize mint object from txn, check for valid
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        return FormatSyscoinErrorMessage(state, "mint-unserialize", fJustCheck);
    }
    NEVMTxRoot txRootDB;
    {
        LOCK(cs_setethstatus);
        if(!pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(mintSyscoin.nBlockHash, txRootDB)) {
            if(nHeight > nLastKnownHeightOnStart)
                return FormatSyscoinErrorMessage(state, "mint-txroot-missing", fJustCheck);
        }
    }
     
     // check transaction receipt validity
    dev::RLP rlpReceiptParentNodes(&mintSyscoin.vchReceiptParentNodes);
    std::vector<unsigned char> vchReceiptValue(mintSyscoin.vchReceiptParentNodes.begin()+mintSyscoin.posReceipt, mintSyscoin.vchReceiptParentNodes.end());
    dev::RLP rlpReceiptValue(&vchReceiptValue);
    
    if (!rlpReceiptValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt", fJustCheck);
    }
    if (rlpReceiptValue.itemCount() != 4) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-count", fJustCheck);
    }
    const uint64_t &nStatus = rlpReceiptValue[0].toInt<uint64_t>(dev::RLP::VeryStrict);
    if (nStatus != 1) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-receipt-status", fJustCheck);
    } 
    dev::RLP rlpReceiptLogsValue(rlpReceiptValue[3]);
    if (!rlpReceiptLogsValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-receipt-rlp-logs-list", fJustCheck);
    }
    const size_t &itemCount = rlpReceiptLogsValue.itemCount();
    // just sanity checks for bounds
    if (itemCount < 1 || itemCount > 10) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-logs-count", fJustCheck);
    }
    // look for TokenFreeze event and get the last parameter which should be the precisions
    bool foundTokenFreeze = false;
    for(uint32_t i = 0;i<itemCount;i++) {
        dev::RLP rlpReceiptLogValue(rlpReceiptLogsValue[i]);
        if (!rlpReceiptLogValue.isList()) {
            return FormatSyscoinErrorMessage(state, "mint-receipt-log-rlp-list", fJustCheck);
        }
        // ensure this log has at least the address to check against
        if (rlpReceiptLogValue.itemCount() < 1) {
            return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count", fJustCheck);
        }
        const dev::Address &address160Log = rlpReceiptLogValue[0].toHash<dev::Address>(dev::RLP::VeryStrict);
        if(Params().GetConsensus().vchSYSXERC20Manager == address160Log.asBytes()) {
            // for mint log we should have exactly 3 entries in it, this event we control through our erc20manager contract
            if (rlpReceiptLogValue.itemCount() != 3) {
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-count-bridgeid", fJustCheck);
            }
            // check topic
            dev::RLP rlpReceiptLogTopicsValue(rlpReceiptLogValue[1]);
            if (!rlpReceiptLogTopicsValue.isList()) {
                return FormatSyscoinErrorMessage(state, "mint-receipt-log-topics-rlp-list", fJustCheck);
            }
            if (rlpReceiptLogTopicsValue.itemCount() != 1) {
                return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-log-topics-count", fJustCheck);
            }
            // topic hash matches with TokenFreeze signature
            if(Params().GetConsensus().vchTokenFreezeMethod == rlpReceiptLogTopicsValue[0].toBytes(dev::RLP::VeryStrict)) {
                const std::vector<unsigned char> &dataValue = rlpReceiptLogValue[2].toBytes(dev::RLP::VeryStrict);
                if(dataValue.size() < 128) {
                     return FormatSyscoinErrorMessage(state, "mint-receipt-log-data-invalid-size", fJustCheck);
                }
                foundTokenFreeze = true;
            }
        }
    }
    if(!foundTokenFreeze) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-missing-tokenfreeze", fJustCheck);
    }
    
    // check transaction spv proofs
    std::vector<unsigned char> rlpTxRootVec(txRootDB.nTxRoot.begin(), txRootDB.nTxRoot.end());
    dev::RLPStream sTxRoot, sReceiptRoot;
    sTxRoot.append(rlpTxRootVec);
    std::vector<unsigned char> rlpReceiptRootVec(txRootDB.nReceiptRoot.begin(),  txRootDB.nReceiptRoot.end());
    sReceiptRoot.append(rlpReceiptRootVec);
    dev::RLP rlpTxRoot(sTxRoot.out());
    dev::RLP rlpReceiptRoot(sReceiptRoot.out());
    if(mintSyscoin.nTxRoot != txRootDB.nTxRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", fJustCheck);
    }
    if(mintSyscoin.nReceiptRoot != txRootDB.nReceiptRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", fJustCheck);
    }
    
    
    dev::RLP rlpTxParentNodes(&mintSyscoin.vchTxParentNodes);
    std::vector<unsigned char> vchTxValue(mintSyscoin.vchTxParentNodes.begin()+mintSyscoin.posTx, mintSyscoin.vchTxParentNodes.end());
    std::vector<unsigned char> vchTxHash(dev::sha3(vchTxValue).asBytes());
    // we must reverse the endian-ness because we store uint256 in BE but Eth uses LE.
    std::reverse(vchTxHash.begin(), vchTxHash.end());
    // validate mintSyscoin.nTxHash is the hash of vchTxValue, this is not the TXID which would require deserializataion of the transaction object, for our purpose we only need
    // uniqueness per transaction that is immutable and we do not care specifically for the txid but only that the hash cannot be reproduced for double-spend
    if(uint256S(HexStr(vchTxHash)) != mintSyscoin.nTxHash) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-hash", fJustCheck);
    }
    dev::RLP rlpTxValue(&vchTxValue);
    const std::vector<unsigned char> &vchTxPath = mintSyscoin.vchTxPath;
    // ensure eth tx not already spent in a previous block
    if(pnevmtxmintdb->ExistsTx(mintSyscoin.nTxHash) && nHeight > nLastKnownHeightOnStart) {
        return FormatSyscoinErrorMessage(state, "mint-exists", fJustCheck);
    } 
    // sanity check is set in mempool during m_test_accept and when miner validates block
    // we care to ensure unique bridge id's in the mempool, not to emplace on test_accept
    if(fJustCheck) {
        if(mapMintKeys.find(mintSyscoin.nTxHash) != mapMintKeys.end()) {
            return state.Invalid(TxValidationResult::TX_MINT_DUPLICATE, "mint-duplicate-transfer");
        }
    }
    else {
        // ensure eth tx not already spent in current processing block or mempool(mapMintKeysMempool passed in)
        auto itMap = mapMintKeys.try_emplace(mintSyscoin.nTxHash, txHash);
        if(!itMap.second) {
            return state.Invalid(TxValidationResult::TX_MINT_DUPLICATE, "mint-duplicate-transfer");
        }
    }
    // verify receipt proof
    if(!VerifyProof(&vchTxPath, rlpReceiptValue, rlpReceiptParentNodes, rlpReceiptRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-receipt-proof", fJustCheck);
    }
    // verify transaction proof
    if(!VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-proof", fJustCheck);
    }
    if (!rlpTxValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-rlp-list", fJustCheck);
    }
    if (rlpTxValue.itemCount() < 8) {
        return FormatSyscoinErrorMessage(state, "mint-tx-itemcount", fJustCheck);
    }
    const dev::u256& nChainID = rlpTxValue[0].toInt<dev::u256>(dev::RLP::VeryStrict);
    if(nChainID != (dev::u256(Params().GetConsensus().nNEVMChainID))) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-chainid", fJustCheck);
    }
    if (!rlpTxValue[7].isData()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-array", fJustCheck);
    }    
    if (rlpTxValue[5].isEmpty()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-invalid-receiver", fJustCheck);
    }             
    const dev::Address &address160 = rlpTxValue[5].toHash<dev::Address>(dev::RLP::VeryStrict);

    // ensure ERC20Manager is in the "to" field for the contract, meaning the function was called on this contract for freezing supply
    if(Params().GetConsensus().vchSYSXERC20Manager != address160.asBytes()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-contract-manager", fJustCheck);
    }
    const std::vector<unsigned char> &rlpBytes = rlpTxValue[7].toBytes(dev::RLP::VeryStrict);
    std::vector<unsigned char> vchERC20ContractAddress;
    CTxDestination dest;
    std::string witnessAddress;
    if(!parseNEVMMethodInputData(Params().GetConsensus().vchSYSXBurnMethodSignature, 16, 8, rlpBytes, nValue, witnessAddress)) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-data", fJustCheck);
    }
    if(!fRegTest) {
        bool bFoundDest = false;
        // look through outputs to find one that matches the destination with the right amount
        for(const auto &vout: tx.vout) {
            if(!ExtractDestination(vout.scriptPubKey, dest)) {
                return FormatSyscoinErrorMessage(state, "mint-extract-destination", fJustCheck);  
            }
            if(EncodeDestination(dest) == witnessAddress && vout.nValue == nValue) {
                bFoundDest = true;
                break;
            }
        }
        if(!bFoundDest) {
            return FormatSyscoinErrorMessage(state, "mint-mismatch-destination", fJustCheck);
        }
    }
    if(!fJustCheck) {
        if(nHeight > 0) {   
            LogPrint(BCLog::SYS,"CONNECTED MINT: hash=%s height=%d\n",
                txHash.ToString().c_str(),
                nHeight);      
        }           
    }               
    return true;
}
bool DisconnectMint(const CTransaction &tx, const uint256& txHash, NEVMMintTxMap &mapMintKeys){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectMint: Cannot unserialize data inside of this transaction relating to a mint\n");
        return false;
    }
    mapMintKeys.try_emplace(mintSyscoin.nTxHash, txHash);
    return true;
}

bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CTxUndo& txundo, CCoinsViewCache& view, NEVMMintTxMap &mapMintKeys, NEVMDataVec &NEVMDataVecOut) {
    if(IsSyscoinMintTx(tx.nVersion)) {
        if(!DisconnectMint(tx, txHash, mapMintKeys))
            return false;       
    }
    else if (tx.IsNEVMData()) {
        CNEVMData nevmData(tx);
        if(nevmData.IsNull()) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: nevm-data-invalid\n");
            return false; 
        }
        NEVMDataVecOut.emplace_back(nevmData.vchVersionHash); 
    } 
    return true;       
}

void CNEVMTxRootsDB::FlushDataToCache(const NEVMTxRootMap &mapNEVMTxRoots) {
    if(mapNEVMTxRoots.empty()) {
        return;
    }
    for (auto const& [key, val] : mapNEVMTxRoots) {
        mapCache.try_emplace(key, val);
    }
    LogPrint(BCLog::SYS, "Flushing to cache, storing %d nevm tx roots\n", mapNEVMTxRoots.size());
}
bool CNEVMTxRootsDB::FlushCacheToDisk() {
    if(mapCache.empty()) {
        return true;
    }
    CDBBatch batch(*this);    
    for (auto const& [key, val] : mapCache) {
        batch.Write(key, val);
    }
    LogPrint(BCLog::SYS, "Flushing cache to disk, storing %d nevm tx roots\n", mapCache.size());
    auto res = WriteBatch(batch, true);
    mapCache.clear();
    return res;
}
bool CNEVMTxRootsDB::ReadTxRoots(const uint256& nBlockHash, NEVMTxRoot& txRoot) {
    auto it = mapCache.find(nBlockHash);
    if(it != mapCache.end()){
        txRoot = it->second;
        return true;
    } else {
        return Read(nBlockHash, txRoot);
    }
    return false;
} 
bool CNEVMTxRootsDB::FlushErase(const std::vector<uint256> &vecBlockHashes) {
    if(vecBlockHashes.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecBlockHashes) {
        batch.Erase(key);
        auto it = mapCache.find(key);
        if(it != mapCache.end()){
            mapCache.erase(it);
        }
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx roots\n", vecBlockHashes.size());
    return WriteBatch(batch, true);
}

// called on connect
void CNEVMMintedTxDB::FlushDataToCache(const NEVMMintTxMap &mapNEVMTxRoots) {
    if(mapNEVMTxRoots.empty()) {
        return;
    }
    for (auto const& [key, val] : mapNEVMTxRoots) {
        mapCache.try_emplace(key, val);
    }
    LogPrint(BCLog::SYS, "Flushing to cache, storing %d nevm tx mints\n", mapNEVMTxRoots.size());
}
bool CNEVMMintedTxDB::FlushCacheToDisk() {
    if(mapCache.empty()) {
        return true;
    }
    CDBBatch batch(*this);    
    for (auto const& [key, val] : mapCache) {
        batch.Write(key, val);
    }
    LogPrint(BCLog::SYS, "Flushing cache to disk, storing %d nevm tx mints\n", mapCache.size());
    auto res = WriteBatch(batch, true);
    mapCache.clear();
    return res;
}
bool CNEVMMintedTxDB::FlushErase(const NEVMMintTxMap &mapNEVMTxRoots) {
    if(mapNEVMTxRoots.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapNEVMTxRoots) {
        batch.Erase(key);
        auto it = mapCache.find(key.first);
        if(it != mapCache.end()){
            mapCache.erase(it);
        }
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx mints\n", mapNEVMTxRoots.size());
    return WriteBatch(batch, true);
}
bool CNEVMMintedTxDB::ExistsTx(const uint256& nTxHash) {
    return (mapCache.find(nTxHash) != mapCache.end()) || Exists(nTxHash);
}

void CNEVMDataDB::FlushDataToCache(const PoDAMAPMemory &mapPoDA, const int64_t& nMedianTime) {
    if(mapPoDA.empty()) {
        return;
    }
    int nCount = 0;
    for (auto const& [key, val] : mapPoDA) {
        // Could be null if we are reindexing blocks from disk and we get the VH without data (because data wasn't provided in payload).
        // This is OK because we still want to provide the VH's to NEVM for indexing into DB (lookup via precompile).
        if(!val) {
            continue;
        }
        // mapPoDA has a pointer of data back to tx vout and we copy it here because the block will lose its memory soon as its
        // stored to disk we create a copy here in our cache (which later gets written to disk in FlushStateToDisk)
        auto inserted = mapCache.try_emplace(key, /* data */ *val, /* MTP */ nMedianTime);
        // for duplicate blobs, allow to update median time
        if(!inserted.second) {
            inserted.first->second.second = nMedianTime;
        }
        nCount++;
    }
    if(nCount > 0)
        LogPrint(BCLog::SYS, "Flushing to cache, storing %d nevm blobs\n", nCount);
}
bool CNEVMDataDB::FlushCacheToDisk() {
    if(mapCache.empty()) {
        return true;
    }
    CDBBatch batch(*this);    
    for (auto const& [key, val] : mapCache) {
        const auto &pairData = std::make_pair(key, true);
        const auto &pairMTP = std::make_pair(key, false);
        // write the size of the data
        batch.Write(key, (uint32_t)val.first.size());
        // write the data
        batch.Write(pairData, val.first);
        // write the MTP
        batch.Write(pairMTP, val.second);
    }
    LogPrint(BCLog::SYS, "Flushing cache to disk, storing %d nevm blobs\n", mapCache.size());
    auto res = WriteBatch(batch, true);
    mapCache.clear();
    return res;
}
bool CNEVMDataDB::ReadData(const std::vector<uint8_t>& nVersionHash, std::vector<uint8_t>& vchData) {
    
    auto it = mapCache.find(nVersionHash);
    if(it != mapCache.end()){
        vchData = it->second.first;
        return true;
    } else {
        const auto& pair = std::make_pair(nVersionHash, true);
        return Read(pair, vchData);
    }
    return false;
} 
bool CNEVMDataDB::ReadMTP(const std::vector<uint8_t>& nVersionHash, int64_t &nMedianTime) {
    auto it = mapCache.find(nVersionHash);
    if(it != mapCache.end()){
        nMedianTime = it->second.second;
        return true;
    } else {
        const auto& pair = std::make_pair(nVersionHash, false);
        return Read(pair, nMedianTime);
    }
    return false;
}
bool CNEVMDataDB::ReadDataSize(const std::vector<uint8_t>& nVersionHash, uint32_t &nSize) {
    auto it = mapCache.find(nVersionHash);
    if(it != mapCache.end()){
        nSize = it->second.first.size();
        return true;
    } else {
        return Read(nVersionHash, nSize);
    }
    return false;
}
bool CNEVMDataDB::FlushEraseMTPs(const NEVMDataVec &vecDataKeys) {
    if(vecDataKeys.empty())
        return true;
    CDBBatch batch(*this);    
    for (const auto &key : vecDataKeys) {
        const auto &pairMTP = std::make_pair(key, false);
        // only set if it already exists (override) rather than create a new insertion
        if(Exists(pairMTP))   
            batch.Write(pairMTP, 0);
        // set in cache as well
        auto it = mapCache.find(key);
        if(it != mapCache.end()) {
            it->second.second = 0;
        }
    }
    LogPrint(BCLog::SYS, "Flushing, resetting %d nevm MTPs\n", vecDataKeys.size());
    return WriteBatch(batch, true);
}
bool CNEVMDataDB::FlushErase(const NEVMDataVec &vecDataKeys) {
    if(vecDataKeys.empty())
        return true;
    CDBBatch batch(*this);    
    for (const auto &key : vecDataKeys) {
        const auto &pairData = std::make_pair(key, true);
        const auto &pairMTP = std::make_pair(key, false);
        // erase size
        batch.Erase(key);
        // erase data and MTP keys
        if(Exists(pairData)) {
            batch.Erase(pairData);
        }
        if(Exists(pairMTP))   
            batch.Erase(pairMTP);
        // remove from cache as well
        auto it = mapCache.find(key);
        if(it != mapCache.end())
            mapCache.erase(it);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d nevm entries\n", vecDataKeys.size());
    return WriteBatch(batch, true);
}
bool CNEVMDataDB::BlobExists(const std::vector<uint8_t>& vchVersionHash) {
    return (mapCache.find(vchVersionHash) != mapCache.end()) || Exists(std::make_pair(vchVersionHash, true));
}
bool CNEVMDataDB::Prune(const int64_t nMedianTime) {
    auto it = mapCache.begin();
    while (it != mapCache.end()) {
        const bool isExpired = (nMedianTime > (it->second.second+NEVM_DATA_EXPIRE_TIME));
        if (it->second.second > 0 && isExpired) {
            mapCache.erase(it++);
        }
        else {
            ++it;
        }
    }
    CDBBatch batch(*this);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    std::pair<std::vector<unsigned char>, bool> pair;
    int64_t nTime;
    while (pcursor->Valid()) {
        try {
            nTime = 0;
            // check if expired if so delete data
            if(pcursor->GetKey(pair) && pair.second == false && pcursor->GetValue(nTime) && nTime > 0 && nMedianTime > (nTime+NEVM_DATA_EXPIRE_TIME)) {
                // erase both pairs
                batch.Erase(pair);
                pair.second = true;
                if(Exists(pair)) {  
                    batch.Erase(pair);
                }
                // erase size
                batch.Erase(pair.first);
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    return WriteBatch(batch, true);
}
bool CNEVMDataDB::ClearZeroMPT() {
    CDBBatch batch(*this);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    std::pair<std::vector<unsigned char>, bool> pair;
    int64_t nTime;
    while (pcursor->Valid()) {
        try {
            nTime = 0;
            // check if expired if so delete data
            if(pcursor->GetKey(pair) && pair.second == false && pcursor->GetValue(nTime) && nTime == 0) {
                // erase both pairs
                batch.Erase(pair);
                pair.second = true;
                if(Exists(pair)) {  
                    batch.Erase(pair);
                }
                // erase size
                batch.Erase(pair.first);
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    return WriteBatch(batch, true);
}