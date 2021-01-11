// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <services/asset.h>
#include <services/assetconsensus.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/system.h>
#include <key_io.h>
#include <core_io.h>
std::unordered_map<uint32_t, uint8_t> mapAssetPrecision;
std::string stringFromSyscoinTx(const int &nVersion) {
    switch (nVersion) {
    case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
		return "assetactivate";
    case SYSCOIN_TX_VERSION_ASSET_UPDATE:
		return "assetupdate";     
	case SYSCOIN_TX_VERSION_ASSET_SEND:
		return "assetsend";
	case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
		return "assetallocationsend";
	case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
		return "assetallocationburntoethereum"; 
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



uint32_t GenerateSyscoinGuid(const COutPoint& outPoint) {
    arith_uint256 txidArith = UintToArith256(outPoint.hash);
    txidArith += outPoint.n;
    return txidArith.GetLow32();
}
bool GetAssetPrecision(const uint32_t &nAsset, uint8_t& nPrecision) {
    auto itP = mapAssetPrecision.try_emplace(nAsset, 0);
    // if added, meaning cache miss then fetch right asset and and fill cache otherwise return cache entry
    if (itP.second) {
        CAsset txPos;
        if (!GetAsset(nAsset, txPos))
            return false;
        itP.first->second = txPos.nPrecision;
    }
    nPrecision = itP.first->second;
    return true;
}
bool GetAsset(const uint32_t &nAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}
bool GetAssetNotaryKeyID(const uint32_t &nAsset,
        std::vector<unsigned char>& keyID) {
    if (!passetdb || !passetdb->ReadAssetNotaryKeyID(nAsset, keyID))
        return false;
    return true;
}
// mapAssetIn needs to be copied by value because its modified to check for equality with mapAssetOut if isNoInput is true, we reuse mapAssetIn in CheckSyscoinInputs
// and do not want to pollute consensus checks elsewhere so therefore we don't modify the reference to mapAssetIn
bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nAsset, CAssetsMap mapAssetIn, const CAssetsMap &mapAssetOut) {
    if (mapAssetOut.empty()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-outputs-empty");
    }
    const bool &isNoInput = IsSyscoinWithNoInputTx(tx.nVersion);
    if(isNoInput) {
        auto itOut = mapAssetOut.find(nAsset);
        if (itOut == mapAssetOut.end()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-output-first-asset-not-found");
        }
        // add first asset out to in so it matches, the rest should be the same
        // the first one is verified by checksyscoininputs() later on (part of asset send is also)
        // emplace will add if it doesn't exist or update it below
        auto itIn = mapAssetIn.try_emplace(nAsset, itOut->second.first, itOut->second.second);
        if (!itIn.second) {
            itIn.first->second = std::make_pair(itOut->second.first, itOut->second.second);
        }
    }
    // this will check that all assets with inputs match amounts being sent on outputs
    // it will also ensure that inputs and outputs per asset are equal with respects to zero-val inputs/outputs (asset ownership utxos) 
    if (mapAssetIn != mapAssetOut) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-io-mismatch");
    }
    return true;
}

CAuxFeeDetails::CAuxFeeDetails(const UniValue& value, const uint8_t &nPrecision){
    if(!value.isObject()) {
        SetNull();
        return;
    }
    const UniValue& addressObj = find_value(value.get_obj(), "auxfee_address");
    if(!addressObj.isStr()) {
        SetNull();
        return;
    }
    const std::string &strAuxFee = addressObj.get_str();
    if(!strAuxFee.empty()) {
        CTxDestination txDest = DecodeDestination(strAuxFee);
        if (!IsValidDestination(txDest)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Invalid auxfee address");
        }
        if (auto witness_id = std::get_if<WitnessV0KeyHash>(&txDest)) {	
            CKeyID keyID = ToKeyID(*witness_id);
            vchAuxFeeKeyID = std::vector<unsigned char>(keyID.begin(), keyID.end());
        } else {
            throw JSONRPCError(RPC_WALLET_ERROR, "Invalid auxfee address: Please use P2PWKH address.");
        }
    }
    const UniValue& arrObj = find_value(value.get_obj(), "fee_struct");
    if(!arrObj.isArray()) {
        SetNull();
        return;
    }
    const UniValue& arr = arrObj.get_array();
    for (size_t i = 0; i < arr.size(); ++i) {
        const UniValue& auxFeeObj = arr[i];
        if(!auxFeeObj.isArray()) {
            SetNull();
            return;
        }
        const UniValue& auxFeeArr = auxFeeObj.get_array();
        if(auxFeeArr.size() != 2 || (!auxFeeArr[0].isNum() && !auxFeeArr[0].isStr()) || !auxFeeArr[1].isNum()) {
            SetNull();
            return;
        }
        int64_t iPct = (int64_t)(auxFeeArr[1].get_real()*100000.0);
        if (iPct < 0 || iPct > 65535) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "percentage must be between 0.00 and 65.535");
        }
        vecAuxFees.push_back(CAuxFee(AssetAmountFromValue(auxFeeArr[0], nPrecision), (uint16_t)iPct));
    }
}
UniValue AssetPublicDataToJson(const std::string &strPubData) {
    UniValue pubDataObj(UniValue::VOBJ);
    if(pubDataObj.read(strPubData)) {
        pubDataObj.pushKV("desc", DecodeBase64(pubDataObj["desc"].get_str()));
    }
    return pubDataObj;
}
void CAuxFeeDetails::ToJson(UniValue& value, const uint32_t& nAsset) const {
    UniValue feeStruct(UniValue::VARR);
    for(const auto& auxfee: vecAuxFees) {
        UniValue auxfeeObj(UniValue::VOBJ);
        auxfeeObj.__pushKV("bound", ValueFromAmount(auxfee.nBound, nAsset));
        auxfeeObj.__pushKV("percentage", auxfee.nPercent / 100000.0);
        feeStruct.push_back(auxfeeObj);
    }
    value.__pushKV("auxfee_address", vchAuxFeeKeyID.empty()? "" : EncodeDestination(WitnessV0KeyHash(uint160{vchAuxFeeKeyID})));
    value.__pushKV("fee_struct", feeStruct);
}

CNotaryDetails::CNotaryDetails(const UniValue& value){
    if(!value.isObject()) {
        SetNull();
        return;
    }
    const UniValue& endpointObj = find_value(value.get_obj(), "endpoint");
    if(!endpointObj.isStr()) {
        SetNull();
        return;
    }
    strEndPoint = EncodeBase64(endpointObj.get_str());
    const UniValue& isObj = find_value(value.get_obj(), "instant_transfers");
    if(!isObj.isBool()) {
        SetNull();
        return;
    }  
    bEnableInstantTransfers = isObj.get_bool()? 1: 0;
    const UniValue& hdObj = find_value(value.get_obj(), "hd_required");
    if(!hdObj.isBool()) {
        SetNull();
        return;
    }   
    bRequireHD = hdObj.get_bool()? 1: 0;
}

void CNotaryDetails::ToJson(UniValue& value) const {
    value.pushKV("endpoint", DecodeBase64(strEndPoint));
    value.pushKV("instant_transfers", bEnableInstantTransfers);
    value.pushKV("hd_required", bRequireHD);
}
