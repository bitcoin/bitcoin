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

#include <algorithm>
#include <unordered_set>

std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
const arith_uint256 nMax = arith_uint256(MAX_MONEY);

static bool GetCanonicalSyscoinData(const CScript& script, std::vector<unsigned char>& data)
{
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN ||
        !script.GetOp(pc, opcode, data) || opcode > OP_PUSHDATA4) {
        return false;
    }
    if (data.size() >= 36 && data[0] == 0xaa && data[1] == 0x21 &&
        data[2] == 0xa9 && data[3] == 0xed) {
        if (!script.GetOp(pc, opcode, data) || opcode > OP_PUSHDATA4) {
            return false;
        }
    }
    return pc == script.end();
}

static bool CheckAssetAllocationCanonical(
    const CTransaction& tx,
    TxValidationState& state,
    const bool& fJustCheck,
    const bool& fRequireSingleAsset
) {
    CAssetAllocation allocation(tx);
    if (allocation.IsNull()) {
        return FormatSyscoinErrorMessage(state, "assetallocation-unserialize-failed", fJustCheck);
    }
    if (fRequireSingleAsset && allocation.voutAssets.size() != 1) {
        return FormatSyscoinErrorMessage(state, "assetallocation-single-asset", fJustCheck);
    }

    const bool requireCanonicalBurn =
        tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM;
    std::vector<unsigned char> raw_data;
    if (requireCanonicalBurn) {
        int data_output;
        if (!GetSyscoinData(tx, raw_data, data_output) || data_output < 0) {
            return FormatSyscoinErrorMessage(state, "assetallocation-unserialize-failed", fJustCheck);
        }
        if (!GetCanonicalSyscoinData(tx.vout[data_output].scriptPubKey, raw_data)) {
            return FormatSyscoinErrorMessage(state, "assetallocation-noncanonical-script", fJustCheck);
        }
    }

    std::unordered_set<uint64_t> setAssets;
    std::unordered_set<uint32_t> setOutputs;
    for (const auto& asset : allocation.voutAssets) {
        if (asset.key == 0) {
            return FormatSyscoinErrorMessage(state, "assetallocation-null-asset", fJustCheck);
        }
        if (asset.values.empty()) {
            return FormatSyscoinErrorMessage(state, "assetallocation-empty-values", fJustCheck);
        }
        if (!setAssets.insert(asset.key).second) {
            return FormatSyscoinErrorMessage(state, "assetallocation-duplicate-asset", fJustCheck);
        }
        for (const auto& output : asset.values) {
            if (!MoneyRange(output.nValue)) {
                return FormatSyscoinErrorMessage(state, "assetallocation-noncanonical-encoding", fJustCheck);
            }
            if (!setOutputs.insert(output.n).second) {
                return FormatSyscoinErrorMessage(state, "assetallocation-duplicate-output", fJustCheck);
            }
        }
    }
    if (requireCanonicalBurn) {
        CBurnSyscoin burn(tx);
        if (burn.IsNull() || burn.vchNEVMAddress.size() != 20) {
            return FormatSyscoinErrorMessage(state, "assetallocation-invalid-nevm-destination", fJustCheck);
        }
        if (std::all_of(burn.vchNEVMAddress.begin(), burn.vchNEVMAddress.end(),
                        [](const unsigned char byte) { return byte == 0; })) {
            return FormatSyscoinErrorMessage(state, "assetallocation-null-nevm-destination", fJustCheck);
        }
        std::vector<unsigned char> canonical_burn_data;
        burn.SerializeData(canonical_burn_data);
        if (raw_data != canonical_burn_data) {
            return FormatSyscoinErrorMessage(state, "assetallocation-noncanonical-encoding", fJustCheck);
        }
    }
    return true;
}

static bool ReadABIUint64(
    const dev::bytes& data,
    const uint64_t offset,
    const bool require_canonical,
    uint64_t& value)
{
    if (offset > data.size()) {
        return false;
    }
    const size_t word_offset = static_cast<size_t>(offset);
    if (data.size() - word_offset < 32 ||
        (require_canonical &&
         !std::all_of(data.begin() + word_offset, data.begin() + word_offset + 24,
                      [](const unsigned char byte) { return byte == 0; }))) {
        return false;
    }
    value = ReadBE64(data.data() + word_offset + 24);
    return true;
}

bool CheckSyscoinMintInternal(
    const CMintSyscoin &mintSyscoin,
    TxValidationState &state,
    const bool &fJustCheck,
    const bool fBridgeCanonicalActive,
    NEVMMintTxSet &setMintTxs,
    uint64_t &nAssetFromLog,
    CAmount &outputAmount,
    std::string &witnessAddress) {
    NEVMTxRoot txRootDB;
    if (!pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(mintSyscoin.nBlockHash, txRootDB)) {
        return FormatSyscoinErrorMessage(state, "mint-txroot-missing", fJustCheck);
    }
    if (mintSyscoin.nTxRoot != txRootDB.nTxRoot) {
        return FormatSyscoinErrorMessage(state, "mint-mismatching-txroot", fJustCheck);
    }
    if (mintSyscoin.nReceiptRoot != txRootDB.nReceiptRoot) {
        return FormatSyscoinErrorMessage(state, "mint-mismatching-receiptroot", fJustCheck);
    }
    if (mintSyscoin.posTx >= mintSyscoin.vchTxParentNodes.size()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-tx-position", fJustCheck);
    }
    if (mintSyscoin.posReceipt >= mintSyscoin.vchReceiptParentNodes.size()) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-receipt-position", fJustCheck);
    }

    dev::RLPStream sTxRoot, sReceiptRoot;
    sTxRoot.append(dev::bytesConstRef(mintSyscoin.nTxRoot.data(), mintSyscoin.nTxRoot.size()));
    sReceiptRoot.append(dev::bytesConstRef(mintSyscoin.nReceiptRoot.data(), mintSyscoin.nReceiptRoot.size()));
    const dev::RLP rlpTxRoot(sTxRoot.out());
    const dev::RLP rlpReceiptRoot(sReceiptRoot.out());
    const dev::RLP rlpTxParentNodes(&mintSyscoin.vchTxParentNodes);
    const dev::RLP rlpReceiptParentNodes(&mintSyscoin.vchReceiptParentNodes);
    const dev::bytesConstRef vchTxValueRef(
        mintSyscoin.vchTxParentNodes.data() + mintSyscoin.posTx,
        mintSyscoin.vchTxParentNodes.size() - mintSyscoin.posTx
    );
    const dev::RLP rlpReceiptValue(
        dev::bytesConstRef(
            mintSyscoin.vchReceiptParentNodes.data() + mintSyscoin.posReceipt,
            mintSyscoin.vchReceiptParentNodes.size() - mintSyscoin.posReceipt
        )
    );
    const dev::RLP rlpTxValue(vchTxValueRef);
    const dev::bytesConstRef vchTxPathRef(mintSyscoin.vchTxPath.data(), mintSyscoin.vchTxPath.size());

    const dev::h256 txHash = dev::sha3(vchTxValueRef);
    std::vector<unsigned char> vchTxHash(txHash.asBytes());
    std::reverse(vchTxHash.begin(), vchTxHash.end());
    if (uint256S(HexStr(vchTxHash)) != mintSyscoin.nTxHash) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-hash", fJustCheck);
    }
    // A positive replay lookup can reject before the expensive MPT proofs. A
    // negative lookup never authorizes the mint. VerifyDB may consume a
    // one-shot overlay entry for mints it disconnected in the same pass.
    if (pnevmtxmintdb->ExistsTx(mintSyscoin.nTxHash) &&
        !pnevmtxmintdb->ConsumeVerifyOverlay(mintSyscoin.nTxHash)) {
        return FormatSyscoinErrorMessage(state, "mint-exists", fJustCheck);
    }

    std::optional<uint8_t> receiptEnvelopeType;
    std::optional<uint8_t> txEnvelopeType;
    if (!VerifyProof(vchTxPathRef, rlpReceiptValue, rlpReceiptParentNodes,
                     rlpReceiptRoot, &receiptEnvelopeType)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-receipt-proof", fJustCheck);
    }
    if (!VerifyProof(vchTxPathRef, rlpTxValue, rlpTxParentNodes,
                     rlpTxRoot, &txEnvelopeType)) {
        return FormatSyscoinErrorMessage(state, "mint-verify-tx-proof", fJustCheck);
    }
    // Before activation, type 0x7f failed proof matching under the historical
    // exclusive-bound parser. Preserve that rejection while using the correct
    // inclusive EIP-2718 decoder.
    if (!fBridgeCanonicalActive &&
        ((receiptEnvelopeType.has_value() && *receiptEnvelopeType == 0x7f) ||
         (txEnvelopeType.has_value() && *txEnvelopeType == 0x7f))) {
        return FormatSyscoinErrorMessage(state, "mint-unsupported-tx-format", fJustCheck);
    }
    if (fBridgeCanonicalActive && receiptEnvelopeType != txEnvelopeType) {
        return FormatSyscoinErrorMessage(state, "mint-envelope-type-mismatch", fJustCheck);
    }

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
        if (!rlpLogTopics.isList() || rlpLogTopics.itemCount() == 0) {
            continue;
        }
        if (rlpLogTopics[0].toBytes(dev::RLP::VeryStrict) != vchFreezeTopic) {
            continue;
        }
        if (fBridgeCanonicalActive && rlpLog.itemCount() != 3) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-field-count", fJustCheck);
        }
        // TokenFreeze(uint64,address,uint256,string) has exactly two indexed arguments.
        if ((fBridgeCanonicalActive && rlpLogTopics.itemCount() != 3) ||
            (!fBridgeCanonicalActive && rlpLogTopics.itemCount() < 3)) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-topics-count", fJustCheck);
        }

        // Parse indexed asset guid from topics:
        dev::bytes vchAssetGuid = rlpLogTopics[1].toBytes(dev::RLP::VeryStrict);
        if (vchAssetGuid.size() != 32 ||
            (fBridgeCanonicalActive &&
             !std::all_of(vchAssetGuid.begin(), vchAssetGuid.begin() + 24,
                          [](const unsigned char byte) { return byte == 0; }))) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-asset-guid-topic-size", fJustCheck);
        }
        nAssetFromLog = ReadBE64(vchAssetGuid.data() + 24);
        if (nAssetFromLog == 0) {
            return FormatSyscoinErrorMessage(state, "mint-log-null-asset-guid", fJustCheck);
        }

        if (fBridgeCanonicalActive) {
            const dev::bytes vchFreezer = rlpLogTopics[2].toBytes(dev::RLP::VeryStrict);
            if (vchFreezer.size() != 32 ||
                !std::all_of(vchFreezer.begin(), vchFreezer.begin() + 12,
                             [](const unsigned char byte) { return byte == 0; })) {
                return FormatSyscoinErrorMessage(state, "mint-log-invalid-freezer-topic", fJustCheck);
            }
        }

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

        uint64_t offsetToString;
        if (!ReadABIUint64(dataValue, 32, fBridgeCanonicalActive, offsetToString) ||
            (fBridgeCanonicalActive && (offsetToString != 64 || offsetToString % 32 != 0))) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-string-offset", fJustCheck);
        }

        uint64_t lenString;
        if (!ReadABIUint64(dataValue, offsetToString, fBridgeCanonicalActive, lenString)) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-string-length", fJustCheck);
        }

        const uint64_t MAX_WITNESS_ADDRESS_LENGTH = 1024; // Reasonable maximum
        if (lenString > MAX_WITNESS_ADDRESS_LENGTH) {
            return FormatSyscoinErrorMessage(state, "mint-witness-address-too-long", fJustCheck);
        }
        const size_t payloadStart = static_cast<size_t>(offsetToString) + 32;
        if (lenString > dataValue.size() - payloadStart) {
            return FormatSyscoinErrorMessage(state, "mint-log-invalid-string-length", fJustCheck);
        }
        const size_t paddedLength = (static_cast<size_t>(lenString) + 31) & ~size_t{31};
        if (fBridgeCanonicalActive &&
            (paddedLength > dataValue.size() - payloadStart ||
             payloadStart + paddedLength != dataValue.size() ||
             !std::all_of(dataValue.begin() + payloadStart + lenString, dataValue.end(),
                          [](const unsigned char byte) { return byte == 0; }))) {
            return FormatSyscoinErrorMessage(state, "mint-log-noncanonical-string", fJustCheck);
        }

        witnessAddress = std::string(
            reinterpret_cast<const char*>(dataValue.data() + payloadStart),
            lenString
        );
        break;
    }

    if (nAssetFromLog == 0 || outputAmount == 0 || witnessAddress.empty()) {
        return FormatSyscoinErrorMessage(state, "mint-missing-freeze-log", fJustCheck);
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
    
    if (!rlpTxValue.isList()) {
        return FormatSyscoinErrorMessage(state, "mint-tx-rlp-list", fJustCheck);
    }
    const size_t txItemCount = rlpTxValue.itemCount();
    if (txItemCount < 8) {
        return FormatSyscoinErrorMessage(state, "mint-tx-itemcount", fJustCheck);
    }

    dev::u256 nChainID = 0;
    size_t toFieldIndex;
    if (fBridgeCanonicalActive) {
        if (!txEnvelopeType.has_value()) {
            if (txItemCount != 9) {
                return FormatSyscoinErrorMessage(state, "mint-invalid-legacy-tx-format", fJustCheck);
            }
            const dev::u256 v = rlpTxValue[6].toInt<dev::u256>(dev::RLP::VeryStrict);
            if (v >= 35) {
                nChainID = (v - 35) / 2;
            }
            toFieldIndex = 3;
        } else if (*txEnvelopeType == 1 && txItemCount == 11) {
            nChainID = rlpTxValue[0].toInt<dev::u256>(dev::RLP::VeryStrict);
            toFieldIndex = 4;
        } else if (*txEnvelopeType == 2 && txItemCount == 12) {
            nChainID = rlpTxValue[0].toInt<dev::u256>(dev::RLP::VeryStrict);
            toFieldIndex = 5;
        } else {
            return FormatSyscoinErrorMessage(state, "mint-unsupported-tx-format", fJustCheck);
        }
    } else if (txItemCount == 9) {
        const dev::u256 v = rlpTxValue[6].toInt<dev::u256>(dev::RLP::VeryStrict);
        if (v >= 35) {
            nChainID = (v - 35) / 2;
        }
        toFieldIndex = 3;
    } else if (txItemCount >= 12) {
        nChainID = rlpTxValue[0].toInt<dev::u256>(dev::RLP::VeryStrict);
        toFieldIndex = 5;
    } else {
        return FormatSyscoinErrorMessage(state, "mint-unsupported-tx-format", fJustCheck);
    }
    
    // Compare extracted chain ID with Syscoin's expected Chain ID
    if(nChainID != (dev::u256(Params().GetConsensus().nNEVMChainID))) {
        return FormatSyscoinErrorMessage(state, "mint-invalid-chainid", fJustCheck);
    }
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
    const bool fBridgeCanonicalActive =
        nHeight >= (uint32_t)Params().GetConsensus().nCLReceiptStartBlock;
    if(!CheckSyscoinMintInternal(mintSyscoin, state, fJustCheck, fBridgeCanonicalActive,
                                setMintTxs, nAssetFromLog, outputAmount, witnessAddress)) {
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
    if (fBridgeCanonicalActive) {
        // Enforce that that it consumes no asset inputs.
        if (mapAssetOut.size() != 1) {
            return FormatSyscoinErrorMessage(state, "mint-single-asset", fJustCheck);
        }
        if (!mapAssetIn.empty()) {
            return FormatSyscoinErrorMessage(state, "mint-no-asset-inputs", fJustCheck);
        }
    }
    // check that there is an output from the asset in the log
    auto itOut = mapAssetOut.find(nAssetFromLog);
    if (itOut == mapAssetOut.end()) {
        return FormatSyscoinErrorMessage(state, "mint-asset-output-notfound", fJustCheck);
    }

    CAmount nTotalMinted;
    if (fBridgeCanonicalActive) {
        // simply the asset's total output.
        nTotalMinted = itOut->second;
    } else {
        // net minted amount by subtracting matching inputs.
        auto itIn = mapAssetIn.find(nAssetFromLog);
        if (itIn != mapAssetIn.end()) {
            nTotalMinted = itOut->second - itIn->second;
            mapAssetIn.erase(itIn);
        } else {
            nTotalMinted = itOut->second;
        }
    }
    // Remove the minted asset from the output map so the caller's in==out equality check holds.
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
        if (nHeight >= (uint32_t)params.nCLReceiptStartBlock && tx.HasAssets()) {
            const bool fRequireSingleAsset =
                IsSyscoinMintTx(tx.nVersion) ||
                tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM;
            if (!CheckAssetAllocationCanonical(tx, state, fJustCheck,
                                               fRequireSingleAsset)) {
                return false;
            }
        }
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
            if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM && nHeight >= (uint32_t)Params().GetConsensus().nCLReceiptStartBlock) {
                // Structural bounds.
                if (tx.vin.size() >= 100) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-nevm-too-many-inputs", fJustCheck);
                }
                if (tx.vout.size() >= 10) {
                    return FormatSyscoinErrorMessage(state, "assetallocation-nevm-too-many-outputs", fJustCheck);
                }
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
void CNEVMMintedTxDB::SetVerifyOverlay(const NEVMMintTxSet& overlay)
{
    LOCK(cs_cache);
    mapVerifyOverlay = overlay;
}
void CNEVMMintedTxDB::ClearVerifyOverlay()
{
    LOCK(cs_cache);
    mapVerifyOverlay.clear();
}
bool CNEVMMintedTxDB::ConsumeVerifyOverlay(const uint256& nTxHash)
{
    LOCK(cs_cache);
    auto it = mapVerifyOverlay.find(nTxHash);
    if (it == mapVerifyOverlay.end()) {
        return false;
    }
    mapVerifyOverlay.erase(it);
    return true;
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