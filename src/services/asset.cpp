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

bool GetAsset(const uint32_t &nAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}

bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nAsset, CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut) {
    if (mapAssetOut.empty()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-outputs-empty");
    }
    CAssetsMap::const_iterator itOut;
    const bool &isNoInput = IsSyscoinWithNoInputTx(tx.nVersion);
    if(isNoInput) {
        itOut = mapAssetOut.find(nAsset);
        if (itOut == mapAssetOut.end()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-output-first-asset-not-found");
        }
        // add first asset out to in so it matches, the rest should be the same
        // the first one is verified by checksyscoininputs() later on (part of asset send is also)
        // emplace will add if it doesn't exist or update it below
        auto it = mapAssetIn.emplace(nAsset, std::make_pair(itOut->second.first, itOut->second.second));
        if (!it.second) {
            it.first->second = std::make_pair(itOut->second.first, itOut->second.second);
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
        if (auto witness_id = boost::get<WitnessV0KeyHash>(&txDest)) {	
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

void CAuxFeeDetails::ToJson(UniValue& value) const {
    UniValue feeStruct(UniValue::VARR);
    for(const auto& auxfee: vecAuxFees) {
        UniValue auxfeeArr(UniValue::VARR);
        auxfeeArr.push_back(auxfee.nBound);
        auxfeeArr.push_back(auxfee.nPercent);
        feeStruct.push_back(auxfeeArr);
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
    value.pushKV("endpoint", strEndPoint);
    value.pushKV("instant_transfers", bEnableInstantTransfers);
    value.pushKV("hd_required", bRequireHD);
}
