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
#include <chainparamsbase.h>
int64_t nRandomResetSec = 0;
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

std::vector<unsigned char> vchFromString(const std::string &str) {
	unsigned char *strbeg = (unsigned char*)str.c_str();
	return std::vector<unsigned char>(strbeg, strbeg + str.size());
}
std::string stringFromVch(const std::vector<unsigned char> &vch) {
	std::string res;
	std::vector<unsigned char>::const_iterator vi = vch.begin();
	while (vi != vch.end()) {
		res += (char)(*vi);
		vi++;
	}
	return res;
}




bool CAsset::UnserializeFromData(const std::vector<unsigned char> &vchData) {
    try {
		CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
		Unserialize(dsAsset);
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}

bool CAsset::UnserializeFromTx(const CTransaction &tx) {
	std::vector<unsigned char> vchData;
	int nOut;
	if (!GetSyscoinData(tx, vchData, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData))
	{	
		SetNull();
		return false;
	}
    return true;
}


uint32_t GenerateSyscoinGuid(const COutPoint& outPoint) {
    arith_uint256 txidArith = UintToArith256(outPoint.hash);
    txidArith += outPoint.n;
    return txidArith.GetLow32();
}

void CAsset::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsAsset);
	vchData = std::vector<unsigned char>(dsAsset.begin(), dsAsset.end());
}

bool GetAsset(const uint32_t &nAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}

bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nAsset, std::unordered_map<uint32_t, std::pair<bool, CAmount> > &mapAssetIn, const std::unordered_map<uint32_t, std::pair<bool, CAmount> > &mapAssetOut) {
    if (mapAssetOut.empty()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-outputs-empty");
    }
    std::unordered_map<uint32_t, std::pair<bool, CAmount> >::const_iterator itOut;
    const bool &isNoInput = IsSyscoinWithNoInputTx(tx.nVersion);
    if(isNoInput) {
        itOut = mapAssetOut.find(nAsset);
        if (itOut == mapAssetOut.end()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-asset-output-first-asset-not-found");
        }
    }
    // case 1: asset send without zero val input, covered by this check
    // case 2: asset send with zero val of another asset, covered by mapAssetIn != mapAssetOut
    // case 3: asset send with multiple zero val input/output, covered by GetAssetValueOut() for output and CheckTxInputs() for input
    // case 4: asset send sending assets without inputs of those assets, covered by this check + mapAssetIn != mapAssetOut
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
        if (mapAssetIn.empty()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-inputs-empty");
        }
        auto itIn = mapAssetIn.find(nAsset);
        if (itIn == mapAssetIn.end()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-input-first-asset-not-found");
        }
        // check that the first input asset and first output asset match
        // note that we only care about first asset because below we will end up enforcing in == out for the rest
        if (itIn->first != itOut->first) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-guid-mismatch");
        }
        // check that the first input asset has zero val input spent
        if (!itIn->second.first) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-missing-zero-val-input");
        }
        // check that the first output asset has zero val output
        if (!itOut->second.first) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-missing-zero-val-output");
        }
    }
    // asset send also falls into this so for the first out we need to check for asset send seperately above
    if (isNoInput) {
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
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-io-mismatch");
    }
    return true;
}

CAuxFeeDetails::CAuxFeeDetails(const UniValue& value, const uint8_t &nPrecision){
    if(!value.isObject()) {
        SetNull();
        return;
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
        if(auxFeeArr.size() != 2 || (!auxFeeArr[0].isNum() && !auxFeeArr[0].isStr()) || !auxFeeArr[1].isStr()) {
            SetNull();
            return;
        }
        double fPct;
        if(ParseDouble(auxFeeArr[1].get_str(), &fPct) && fPct <= 65.535){
            const uint16_t& nPercent = (uint16_t)(fPct*1000);
            vecAuxFees.push_back(CAuxFee(AssetAmountFromValue(auxFeeArr[0], nPrecision), nPercent));
        } else {
            SetNull();
            return;
        }
    }
}

UniValue CAuxFeeDetails::ToJson() const {
    UniValue value(UniValue::VOBJ);
    UniValue feeStruct(UniValue::VARR);
    for(const auto& auxfee: vecAuxFees) {
        UniValue auxfeeArr(UniValue::VARR);
        auxfeeArr.push_back(auxfee.nBound);
        auxfeeArr.push_back(auxfee.nPercent);
        feeStruct.push_back(auxfeeArr);
    }
    value.pushKV("fee_struct", feeStruct);
    return value;
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

UniValue CNotaryDetails::ToJson() const {
    UniValue value(UniValue::VOBJ);
    value.pushKV("endpoint", DecodeBase64(strEndPoint));
    value.pushKV("instant_transfers", bEnableInstantTransfers);
    value.pushKV("hd_required", bRequireHD);
    return value;
}

void recursive_copy(const fs::path &src, const fs::path &dst)
{
  if (fs::exists(dst)){
    throw std::runtime_error(dst.generic_string() + " exists");
  }

  if (fs::is_directory(src)) {
    fs::create_directories(dst);
    for (fs::directory_entry& item : fs::directory_iterator(src)) {
      recursive_copy(item.path(), dst/item.path().filename());
    }
  } 
  else if (fs::is_regular_file(src)) {
    fs::copy(src, dst);
  } 
  else {
    throw std::runtime_error(dst.generic_string() + " not dir or file");
  }
}

void DoGethMaintenance() {
    bool ibd = false;
    {
        LOCK(cs_main);
        ibd = ::ChainstateActive().IsInitialBlockDownload();
    }
    // hasn't started yet so start
    if(gethPID == 0 || relayerPID == 0) {
        gethPID = relayerPID = -1;
        LogPrintf("GETH: Starting Geth and Relayer because PID's were uninitialized\n");
        int wsport = gArgs.GetArg("-gethwebsocketport", 8646);
        int ethrpcport = gArgs.GetArg("-gethrpcport", 8645);
        bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
        const std::string mode = gArgs.GetArg("-gethsyncmode", "light");
        StartGethNode(exePath, gethPID, wsport, ethrpcport, mode);
        int rpcport = gArgs.GetArg("-rpcport", BaseParams().RPCPort());
        StartRelayerNode(exePath, relayerPID, rpcport, wsport, ethrpcport);
        nRandomResetSec = GetRandInt(600);
        nLastGethHeaderTime = GetSystemTimeInSeconds();
    // if not syncing chain restart geth/relayer if its been long enough since last blocks from relayer
    } else if(!ibd){
        const uint64_t nTimeSeconds = GetSystemTimeInSeconds();
        // it's been >= 10 minutes (+ some minutes for randomization up to another 10 min) since an Ethereum block so clean data dir and resync
        if((nTimeSeconds - nLastGethHeaderTime) > (600 + nRandomResetSec)) {
            LogPrintf("GETH: Last header time not received in sufficient time, trying to resync...\n");
            // reset timer so it will only do this check atleast once every interval (around 10 mins average) if geth seems stuck
            nLastGethHeaderTime = nTimeSeconds;
            // stop geth and relayer
            LogPrintf("GETH: Stopping Geth and Relayer\n"); 
            StopGethNode(gethPID);
            StopRelayerNode(relayerPID);
            // copy wallet dir if exists
            fs::path dataDir = GetDataDir(true);
            fs::path gethDir = dataDir / "geth";
            fs::path gethKeyStoreDir = gethDir / "keystore";
            fs::path keyStoreTmpDir = dataDir / "keystoretmp";
            bool existedKeystore = fs::exists(gethKeyStoreDir);
            if(existedKeystore){
                LogPrintf("GETH: Copying keystore for Geth to a temp directory\n"); 
                try{
                    recursive_copy(gethKeyStoreDir, keyStoreTmpDir);
                } catch(const  std::runtime_error& e) {
                    LogPrintf("Failed copying keystore geth directory to keystoretmp %s\n", e.what());
                    return;
                }
            }
            LogPrintf("GETH: Removing Geth data directory\n");
            // clean geth data dir
            fs::remove_all(gethDir);
            // replace keystore dir
            if(existedKeystore){
                LogPrintf("GETH: Replacing keystore with temp keystore directory\n");
                try{
                    fs::create_directory(gethDir);
                    recursive_copy(keyStoreTmpDir, gethKeyStoreDir);
                } catch(const  std::runtime_error& e) {
                    LogPrintf("Failed copying keystore geth keystoretmp directory to keystore %s\n", e.what());
                    return;
                }
                fs::remove_all(keyStoreTmpDir);
            }
            LogPrintf("GETH: Restarting Geth and Relayer\n");
            // start node and relayer again
            int wsport = gArgs.GetArg("-gethwebsocketport", 8646);
            int ethrpcport = gArgs.GetArg("-gethrpcport", 8645);
            bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
            const std::string mode = gArgs.GetArg("-gethsyncmode", "light");
            StartGethNode(exePath, gethPID, wsport, ethrpcport, mode);
            int rpcport = gArgs.GetArg("-rpcport", BaseParams().RPCPort());
            StartRelayerNode(exePath, relayerPID, rpcport, wsport, ethrpcport);
            // reset randomized reset number
            nRandomResetSec = GetRandInt(600);
            // set flag that geth is resyncing
            fGethSynced = false;
            LogPrintf("GETH: Done, waiting for resync...\n");
        }
    }
}