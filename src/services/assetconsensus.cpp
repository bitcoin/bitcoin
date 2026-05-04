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
#include <messagesigner.h>
#include <util/rbf.h>
#include <undo.h>
#include <validationinterface.h>
#include <timedata.h>
#include <key_io.h>
#include <logging.h>
#include <core_io.h>
std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
const arith_uint256 nMax = arith_uint256(MAX_MONEY);
bool CheckSyscoinMintInternal(
    const CMintSyscoin &mintSyscoin,
    TxValidationState &state,
    const bool &fJustCheck,
    NEVMMintTxSet &setMintTxs,
    uint64_t &nAssetFromLog,
    CAmount &outputAmount,
    std::string &witnessAddress) {
    NEVMTxRoot txRootDB;
    if (!pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(mintSyscoin.nBlockHash, txRootDB)) {
        return FormatSyscoinErrorMessage(state, "mint-txroot-missing", fJustCheck);
    }
    const dev::RLP rlpReceiptParentNodes(&mintSyscoin.vchReceiptParentNodes);
    const dev::RLP rlpReceiptValue(
        dev::bytesConstRef(
            mintSyscoin.vchReceiptParentNodes.data() + mintSyscoin.posReceipt,
            mintSyscoin.vchReceiptParentNodes.size() - mintSyscoin.posReceipt
        )
    );
    
    if (!rlpReceiptValue.isList() || rlpReceiptValue.itemCount() != 4) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-structure", fJustCheck);
    }

    const uint64_t nStatus = rlpReceiptValue[0].toInt<uint64_t>(dev::RLP::VeryStrict);
    if (nStatus != 1) {
        return FormatSyscoinErrorMessage(state, "mint-receipt-status-failed", fJustCheck);
    }
    const dev::RLP rlpLogs(rlpReceiptValue[3]);
    const size_t itemCount = rlpLogs.itemCount();
    if (!rlpLogs.isList() || itemCount < 1 || itemCount > 10) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-logs-count", fJustCheck);
    }
    const std::vector<unsigned char>& vchManagerAddress = Params().GetConsensus().vchSyscoinVaultManager;
    const std::vector<unsigned char>& vchFreezeTopic = Params().GetConsensus().vchTokenFreezeMethod;

    for (size_t i = 0; i < itemCount; ++i) {
        nAssetFromLog = 0;
        outputAmount = 0;
        witnessAddress.clear();
        const dev::RLP& rlpLog = rlpLogs[i];
        if (!rlpLog.isList() || rlpLog.itemCount() < 3) {
            continue;
        }
        const dev::Address addressLog = rlpLog[0].toHash<dev::Address>(dev::RLP::VeryStrict);
        if (addressLog.asBytes() != vchManagerAddress) {
            continue;
        }

        const dev::RLP& rlpLogTopics = rlpLog[1];
        // Verify topics count
        if (!rlpLogTopics.isList() || rlpLogTopics.itemCount() < 3) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-topics-count", fJustCheck);
        }
        if (rlpLogTopics[0].toBytes(dev::RLP::VeryStrict) != vchFreezeTopic) {
            continue;
        }

        // Parse indexed asset guid from topics:
        dev::bytes vchAssetGuid = rlpLogTopics[1].toBytes(dev::RLP::VeryStrict);
        if (vchAssetGuid.size() != 32) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-asset-guid-topic-size", fJustCheck);
        }
        // Take the last 8 bytes (assuming assetGuid fits in 64 bits), 24 because all topics are 32 bytes
        nAssetFromLog = ReadBE64(vchAssetGuid.data() + 24);

        // Now parse non-indexed parameters from data:
        dev::bytes dataValue = rlpLog[2].toBytes(dev::RLP::VeryStrict);
        if (dataValue.size() < 96) {
            return FormatSyscoinErrorMessage(state, "mint-log-data-too-small", fJustCheck);
        }

        // satoshiValue (big-endian)
        std::vector<unsigned char> vchValue(dataValue.data(), dataValue.data() + 32);
        std::reverse(vchValue.begin(), vchValue.end());
        const arith_uint256 valueArith = UintToArith256(uint256(vchValue));
        if (valueArith > nMax) {
            return FormatSyscoinErrorMessage(state, "mint-value-overflow", fJustCheck);
        }
        outputAmount = static_cast<CAmount>(valueArith.GetLow64());
        if (!MoneyRange(outputAmount)) {
            return FormatSyscoinErrorMessage(state, "mint-value-out-of-range", fJustCheck);
        }

        // offset to string (big-endian)
        std::vector<unsigned char> vchOffset(dataValue.data() + 32, dataValue.data() + 64);
        std::reverse(vchOffset.begin(), vchOffset.end());
        const uint64_t offsetToString = UintToArith256(uint256(vchOffset)).GetLow64();

        // string length (big-endian)
        if (offsetToString + 32 > dataValue.size()) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-string-offset", fJustCheck);
        }
        std::vector<unsigned char> vchLenString(
            dataValue.data() + offsetToString,
            dataValue.data() + offsetToString + 32
        );
        std::reverse(vchLenString.begin(), vchLenString.end());
        const uint64_t lenString = UintToArith256(uint256(vchLenString)).GetLow64();

        // Parse the string
        if (offsetToString + 32 + lenString > dataValue.size()) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-string-length", fJustCheck);
        }

        // Add maximum length check to prevent excessive memory allocation
        const uint64_t MAX_WITNESS_ADDRESS_LENGTH = 1024; // Reasonable maximum
        if (lenString > MAX_WITNESS_ADDRESS_LENGTH) {
            return FormatSyscoinErrorMessage(state, "mint-witness-address-too-long", fJustCheck);
        }

        witnessAddress = std::string(
            reinterpret_cast<const char*>(dataValue.data() + offsetToString + 32), 
            lenString
        );
        break;
    }

    if (nAssetFromLog == 0 || outputAmount == 0 || witnessAddress.empty()) {
        return FormatSyscoinErrorMessage(state, "mint-missing-freeze-log", fJustCheck);
    }
    // check transaction spv proofs
    dev::RLPStream sTxRoot, sReceiptRoot;
    sTxRoot.append(dev::bytesConstRef(mintSyscoin.nTxRoot.data(), mintSyscoin.nTxRoot.size()));
    sReceiptRoot.append(dev::bytesConstRef(mintSyscoin.nReceiptRoot.data(), mintSyscoin.nReceiptRoot.size()));
    const dev::RLP rlpTxRoot(sTxRoot.out());
    const dev::RLP rlpReceiptRoot(sReceiptRoot.out());
    if(mintSyscoin.nTxRoot != txRootDB.nTxRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", fJustCheck);
    }
    if(mintSyscoin.nReceiptRoot != txRootDB.nReceiptRoot){
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", fJustCheck);
    }
    
    
    const dev::RLP rlpTxParentNodes(&mintSyscoin.vchTxParentNodes);
    const dev::bytesConstRef vchTxValueRef(
        mintSyscoin.vchTxParentNodes.data() + mintSyscoin.posTx,
        mintSyscoin.vchTxParentNodes.size() - mintSyscoin.posTx
    );
    const dev::h256 txHash = dev::sha3(vchTxValueRef);
    std::vector<unsigned char> vchTxHash(txHash.asBytes());
    // we must reverse the endian-ness because we store uint256 in BE but Eth uses LE.
    std::reverse(vchTxHash.begin(), vchTxHash.end());
    // validate mintSyscoin.nTxHash is the hash of vchTxValue, this is not the TXID which would require deserializataion of the transaction object, for our purpose we only need
    // uniqueness per transaction that is immutable and we do not care specifically for the txid but only that the hash cannot be reproduced for double-spend
    if(uint256S(HexStr(vchTxHash)) != mintSyscoin.nTxHash) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-hash", fJustCheck);
    }
    dev::RLP rlpTxValue(vchTxValueRef);
    
    // Create a bytesConstRef for the path to avoid passing a pointer
    dev::bytesConstRef vchTxPathRef(mintSyscoin.vchTxPath.data(), mintSyscoin.vchTxPath.size());
    
    // ensure eth tx not already spent in a previous block
    if(pnevmtxmintdb->ExistsTx(mintSyscoin.nTxHash)) {
        return FormatSyscoinErrorMessage(state, "mint-exists", fJustCheck);
    } 
    // sanity check is set in mempool during m_test_accept and when miner validates block
    // we care to ensure unique bridge id's in the mempool, not to emplace on test_accept
    if(fJustCheck) {
        if(setMintTxs.find(mintSyscoin.nTxHash) != setMintTxs.end()) {
            return state.Invalid(TxValidationResult::TX_MINT_DUPLICATE, "mint-duplicate-transfer");
        }
    }
    else {
        // ensure eth tx not already spent in current processing block or mempool(mapMintKeysMempool passed in)
        auto itMap = setMintTxs.insert(mintSyscoin.nTxHash);
        if(!itMap.second) {
            return state.Invalid(TxValidationResult::TX_MINT_DUPLICATE, "mint-duplicate-transfer");
        }
    }
    
    // verify receipt proof
    if(!VerifyProof(vchTxPathRef, rlpReceiptValue, rlpReceiptParentNodes, rlpReceiptRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-receipt-proof", fJustCheck);
    }
    // verify transaction proof
    if(!VerifyProof(vchTxPathRef, rlpTxValue, rlpTxParentNodes, rlpTxRoot)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-proof", fJustCheck);
    }
    if (!rlpTxValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-rlp-list", fJustCheck);
    }
    const size_t txItemCount = rlpTxValue.itemCount();
    if (txItemCount < 8) {
        return FormatSyscoinErrorMessage(state, "mint-tx-itemcount", fJustCheck);
    }

    dev::u256 nChainID = 0;
    if (txItemCount == 9) {  // Legacy transaction
        dev::u256 v = rlpTxValue[6].toInt<dev::u256>(dev::RLP::VeryStrict);
        if (v >= 35) {
            // EIP-155: chainId included in the signature
            nChainID = (v - 35) / 2;
        }
    } else if (txItemCount >= 12) {
        nChainID = rlpTxValue[0].toInt<dev::u256>(dev::RLP::VeryStrict);
    } else {
        return FormatSyscoinErrorMessage(state, "mint-unsupported-tx-format", fJustCheck);
    }
    
    // Compare extracted chain ID with Syscoin's expected Chain ID
    if(nChainID != (dev::u256(Params().GetConsensus().nNEVMChainID))) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-chainid", fJustCheck);
    }
    const size_t toFieldIndex = (txItemCount == 9) ? 3 : 5;
    dev::bytes vchAddress = rlpTxValue[toFieldIndex].toBytes(dev::RLP::VeryStrict);
    if (vchAddress.size() != 20) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-address-length", fJustCheck);
    }
    const dev::Address address160(vchAddress);
    // Verify "to" address is vault
    if (Params().GetConsensus().vchSyscoinVaultManager != address160.asBytes()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-contract-manager", fJustCheck);
    }
    
    return true;
}
bool CheckSyscoinMint(
    const CTransaction &tx,
    const uint256 &txHash,
    TxValidationState &state,
    const uint32_t &nHeight,
    const bool &fJustCheck,
    NEVMMintTxSet &setMintTxs,
    CAssetsMap &mapAssetIn,
    CAssetsMap &mapAssetOut
) {
    LogPrint(BCLog::SYS,"*** ASSET MINT blockHeight=%d tx=%s %s\n",
            nHeight, txHash.ToString(), fJustCheck ? "JUSTCHECK" : "BLOCK");
    const CMintSyscoin mintSyscoin(tx);
    if (mintSyscoin.IsNull()) {
        return FormatSyscoinErrorMessage(state, "mint-unserialize-failed", fJustCheck);
    }
    std::string witnessAddress;
    uint64_t nAssetFromLog;
    CAmount outputAmount;
    if(!CheckSyscoinMintInternal(mintSyscoin, state, fJustCheck, setMintTxs, nAssetFromLog, outputAmount, witnessAddress)) {
        return false; // state filled in by CheckSyscoinMintInternal
    }
    bool bFoundDest = false;
    for (const auto &vout : tx.vout) {
        if (vout.scriptPubKey.IsUnspendable()) {
            continue;
        }
        CTxDestination dest;
        if (!ExtractDestination(vout.scriptPubKey, dest)) {
            return FormatSyscoinErrorMessage(state, "mint-extract-destination", fJustCheck);  
        }
        if (EncodeDestination(dest) == witnessAddress && vout.assetInfo.nAsset == nAssetFromLog && vout.assetInfo.nValue == outputAmount) {
            bFoundDest = true;
            break;
        }
    }
    if (!bFoundDest) {
        return FormatSyscoinErrorMessage(state, "mint-mismatch-destination", fJustCheck);
    }
    // check that there is an output from the asset in the log
    auto itOut = mapAssetOut.find(nAssetFromLog);
    if (itOut == mapAssetOut.end()) {
        return FormatSyscoinErrorMessage(state, "mint-asset-output-notfound", fJustCheck);
    }

    // If there's also an input for this asset, remove it and see how much was net minted
    CAmount nTotalMinted;
    // if there is an input from this asset then there must be change so calculate the minted amount by subtracting outputs of the asset from input
    auto itIn = mapAssetIn.find(nAssetFromLog);
    if (itIn != mapAssetIn.end()) {
        nTotalMinted = itOut->second - itIn->second;
        // if there is input from this asset we find total minted and remove input
        mapAssetIn.erase(itIn);
    } else {
        // otherwise assume its a new output (no asset input) so the minted amount is the new output
        nTotalMinted = itOut->second;
    }
    // we only need to find nTotalMinted then we can remove asset from output (we enforce that input of the asset is also removed).
    // This is important because we now will have assets in and out (minus this asset). 
    // Since we may create a new asset output without an input via this function, 
    // we cannot gaurantee in==out unless we remove the asset from in and out. 
    // The same pattern is used with SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION where sysx(asset) is minted by burning sys (non-asset).
    mapAssetOut.erase(itOut);

    // Must match the bridging "outputAmount"
    if (outputAmount != nTotalMinted) {
        return FormatSyscoinErrorMessage(state, "mint-output-mismatch", fJustCheck);
    }
    if (!fJustCheck) {
        if (nHeight > 0) {
            LogPrint(BCLog::SYS,"CONNECTED ASSET MINT: asset=%llu tx=%s height=%d fJustCheck=%s\n",
                nAssetFromLog,
                txHash.ToString(),
                nHeight,
                fJustCheck ? "JUSTCHECK" : "BLOCK"
            );
        }
    }

    return true;
}

bool DisconnectMintAsset(const CTransaction &tx, NEVMMintTxSet &setMintTxs){
    CMintSyscoin mintSyscoin(tx);
    if(mintSyscoin.IsNull()) {
        LogPrint(BCLog::SYS,"DisconnectMintAsset: Cannot unserialize data inside of this transaction relating to an assetallocationmint\n");
        return false;
    }
    setMintTxs.insert(mintSyscoin.nTxHash);
    return true;
}

bool CheckSyscoinInputs(const Consensus::Params& params, const CTransaction& tx, const uint256& txHash, TxValidationState& state, const uint32_t &nHeight, const bool &fJustCheck, NEVMMintTxSet &setMintTxs, CAssetsMap& mapAssetIn, CAssetsMap& mapAssetOut) {
    bool good = true;
    if(nHeight < (uint32_t)params.nNexusStartBlock)
        return !tx.HasAssets();
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY || tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN_LEGACY1)
        return false;
    try{
        if(IsSyscoinMintTx(tx.nVersion)) {
            good = CheckSyscoinMint(tx, txHash, state, nHeight, fJustCheck, setMintTxs, mapAssetIn, mapAssetOut);
        }
        else if (IsAssetAllocationTx(tx.nVersion)) {
            good = CheckAssetAllocationInputs(tx, txHash, state, nHeight, fJustCheck, mapAssetIn, mapAssetOut);
        }
        if (good && tx.HasAssets() && mapAssetIn != mapAssetOut) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-io-mismatch");
        }
    } catch (const std::exception& e) {
        return FormatSyscoinErrorMessage(state, e.what(), fJustCheck);
    } catch (...) {
        return FormatSyscoinErrorMessage(state, "checksyscoininputs-exception", fJustCheck);
    }
    return good;
}

bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &state,
    const uint32_t &nHeight, const bool &fJustCheck, CAssetsMap &mapAssetIn, CAssetsMap &mapAssetOut) {
    LogPrint(BCLog::SYS,"*** ASSET ALLOCATION %d %s %s\n", nHeight,
            txHash.ToString().c_str(),
            fJustCheck ? "JUSTCHECK" : "BLOCK");
        
    const int nOut = GetSyscoinDataOutput(tx);
    if(nOut < 0) {
        return FormatSyscoinErrorMessage(state, "assetallocation-missing-burn-output", fJustCheck);
    }
    switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
        break;
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
        {   
            const uint64_t &nAsset = Params().GetConsensus().nSYSXAsset;
            const CAmount &nBurnAmount = tx.vout[nOut].nValue;
            if(nBurnAmount <= 0) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-invalid-amount", fJustCheck);
            }
            auto itOut = mapAssetOut.find(nAsset);
            if(itOut == mapAssetOut.end()) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-asset-output-notfound", fJustCheck);             
            }
            // if input for this asset exists, must also include it as change in output, so output-input should be the new amount created
            auto itIn = mapAssetIn.find(nAsset);
            CAmount nTotal;
            if(itIn != mapAssetIn.end()) {
                nTotal = itOut->second - itIn->second;
                mapAssetIn.erase(itIn);
            } else {
                nTotal = itOut->second;
            }
            mapAssetOut.erase(itOut);
            // erase in / out of this asset as equality is checked for the rest after CheckSyscoinInputs()
            // the burn amount in opreturn (SYS) should match total output for SYSX
            if(nTotal != nBurnAmount) {
                return FormatSyscoinErrorMessage(state, "syscoin-burn-mismatch-amount", fJustCheck);
            }
        }
        break;
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
        {
            const CAmount &nBurnAmount = tx.vout[nOut].assetInfo.nValue;
            if (nBurnAmount <= 0) {
                return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-amount", fJustCheck);
            }
            if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
                if(nOut == 0) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-burn-index", fJustCheck);
                }
                // the burn of asset in opreturn should match the output value of index 0 (sys)
                if(nBurnAmount != tx.vout[0].nValue) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-mismatch-burn-amount", fJustCheck);
                }  
                if(tx.vout[nOut].assetInfo.nAsset != Params().GetConsensus().nSYSXAsset) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-invalid-sysx-asset", fJustCheck);
                }  
            }
        } 
        break;
        default:
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-op", fJustCheck);
    }

    if(!fJustCheck){
        if(nHeight > 0) {  
            LogPrint(BCLog::SYS,"CONNECTED ASSET ALLOCATION: op=%s hash=%s height=%d fJustCheck=%s\n",
                stringFromSyscoinTx(tx.nVersion).c_str(),
                txHash.ToString().c_str(),
                nHeight,
                "BLOCK");      
        }             
    }  
    return true;
}
// called on connect

void CNEVMTxRootsDB::FlushDataToCache(const NEVMTxRootMap &mapNEVMTxRoots) {
    LOCK(cs_cache);
    for (const auto& entry : mapNEVMTxRoots) {
        auto result = mapCache.emplace(entry.first, entry.second);
        if (!result.second) {
            result.first->second = entry.second;
        }
    }
}
bool CNEVMTxRootsDB::FlushCacheToDisk(std::size_t CHUNK_ITEMS, bool fSync)
{
    LOCK(cs_cache);
    if (mapCache.empty()) return true;

    CDBBatch batch(*this);
    std::size_t items = 0;
    std::size_t count = 0;
    auto flush = [&]() {
        if (batch.SizeEstimate() == 0) return true;
        if (!WriteBatch(batch, fSync)) return false;
        batch.Clear();
        items = 0;
        return true;
    };

    for (auto it = mapCache.begin(); it != mapCache.end(); ) {
        batch.Write(it->first, it->second);
        count++;
        if (++items == CHUNK_ITEMS) {
            if (!flush()) return false;
        }
        // entry is now durable → erase from cache
        it = mapCache.erase(it);
    }
    if (!flush()) return false;       // last partial chunk

    LogPrint(BCLog::SYS,
             "Flushed NEVM-tx-roots cache, %zu items written in %zu-entry chunks\n",
             count, CHUNK_ITEMS);
    return true;
}

bool CNEVMTxRootsDB::ReadTxRoots(const uint256& nBlockHash, NEVMTxRoot& txRoot) {
    LOCK(cs_cache);
    auto it = mapCache.find(nBlockHash);
    if (it != mapCache.end()) {
        txRoot = it->second;
        return true;
    }
    return Read(nBlockHash, txRoot);
} 
bool CNEVMTxRootsDB::FlushErase(const std::vector<uint256> &vecBlockHashes) {
    LOCK(cs_cache);
    if(vecBlockHashes.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto& hash: vecBlockHashes) {
        batch.Erase(hash);
        auto it = mapCache.find(hash);
        if (it != mapCache.end()) {
            mapCache.erase(hash);
        }
    }
    if(vecBlockHashes.size() > 0)
        LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx roots\n", vecBlockHashes.size());
    return WriteBatch(batch, true);
}
void CNEVMMintedTxDB::FlushDataToCache(const NEVMMintTxSet &mapNEVMTxRoots) {
    LOCK(cs_cache);
    for (auto const& key : mapNEVMTxRoots) {
        mapCache.insert(key);
    }
}
bool CNEVMMintedTxDB::FlushCacheToDisk(std::size_t CHUNK_ITEMS, bool fSync)
{
    LOCK(cs_cache);
    if (mapCache.empty()) return true;

    CDBBatch batch(*this);
    std::size_t items = 0;
    std::size_t count = 0;

    auto flush = [&]() {
        if (batch.SizeEstimate() == 0) return true;
        if (!WriteBatch(batch, fSync)) return false;
        batch.Clear();
        items = 0;
        return true;
    };

    for (auto it = mapCache.begin(); it != mapCache.end(); ) {
        batch.Write(*it, true);       // value is a dummy bool
        count++;
        if (++items == CHUNK_ITEMS) {
            if (!flush()) return false;
        }
        it = mapCache.erase(it);      // safe to purge now
    }
    if (!flush()) return false;

    LogPrint(BCLog::SYS,
             "Flushed NEVM-minted-tx cache, %zu items written in %zu-entry chunks\n",
             count, CHUNK_ITEMS);
    return true;
}

bool CNEVMMintedTxDB::FlushErase(const NEVMMintTxSet &mapNEVMTxRoots) {
    LOCK(cs_cache);
    if(mapNEVMTxRoots.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapNEVMTxRoots) {
        batch.Erase(key);
        auto it = mapCache.find(key);
        if(it != mapCache.end()){
            mapCache.erase(it);
        }
    }
    if(mapNEVMTxRoots.size() > 0)
        LogPrint(BCLog::SYS, "Flushing, erasing %d nevm tx mints\n", mapNEVMTxRoots.size());
    return WriteBatch(batch, true);
}
bool CNEVMMintedTxDB::ExistsTx(const uint256& nTxHash) {
    LOCK(cs_cache);
    return (mapCache.find(nTxHash) != mapCache.end()) || Exists(nTxHash);
}
std::string stringFromSyscoinTx(const int &nVersion) {
    switch (nVersion) {
	case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
		return "assetallocationsend";
	case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM:
		return "assetallocationburntonevm"; 
	case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
		return "assetallocationburntosyscoin";
	case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
		return "syscoinburntoassetallocation";            
    case SYSCOIN_TX_VERSION_ALLOCATION_MINT:
        return "assetallocationmint";   
    default:
        return "<unknown assetallocation op>";
    }
}