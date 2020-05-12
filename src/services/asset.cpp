// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <services/asset.h>
#include <services/assetconsensus.h>
#include <consensus/validation.h>
#include <dbwrapper.h>

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
    const arith_uint256 &txidArith = UintToArith256(outPoint.hash);
    uint32_t low32 = (uint32_t)txidArith.GetLow64();
    low32 += outPoint.n;
    if(low32 < 0){
        low32 *= -1;
    }
    if(low32 <= SYSCOIN_TX_VERSION_ALLOCATION_SEND*10){
        low32 = SYSCOIN_TX_VERSION_ALLOCATION_SEND*10;
    }
    return low32;
}

void CAsset::SerializeData( std::vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    Serialize(dsAsset);
	vchData = std::vector<unsigned char>(dsAsset.begin(), dsAsset.end());
}

bool GetAsset(const uint32_t &nAsset,
        CAsset& txPos) {
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}

bool ReserializeAssetCommitment(CMutableTransaction& mtx) {
    // load tx.voutAssets from tx.vout.assetInfo info
    // when change is added this function should be called and preceding this the vout.assetInfo
    // should all be in sync with the asset commitment in OP_RETURN. This will resync that commitment
    // because when change is added the vout.assetInfo is filled and we should capture that in LoadAssetsFromVout as it
    // re-orders tx->voutAssets if change address was somewhere before the last asset output
    mtx.LoadAssetsFromVout();
    // store tx.voutAssets into OP_RETURN data overwriting previous commitment
    const CTransactionRef& tx = MakeTransactionRef(mtx);
    std::vector<unsigned char> data;
    if(IsSyscoinMintTx(tx->nVersion)) {
        CMintSyscoin mintSyscoin(*tx);
        mintSyscoin.voutAssets = tx->voutAssets;
        mintSyscoin.SerializeData(data);
    } else if(tx->nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
        CBurnSyscoin burnSyscoin(*tx);
        burnSyscoin.voutAssets = tx->voutAssets;
        burnSyscoin.SerializeData(data);
    } else if(IsAssetTx(tx->nVersion)) {
        CAsset asset(*tx);
        asset.voutAssets = tx->voutAssets;
        asset.SerializeData(data); 
    } else if(IsAssetAllocationTx(tx->nVersion)) {
        CAssetAllocation allocation(*tx);
        allocation.voutAssets = tx->voutAssets;
        allocation.SerializeData(data); 
    }
    // find previous commitment (OP_RETURN) and replace script
    CScript scriptData;
    scriptData << OP_RETURN << data;
    bool bFoundData = false;
    for(auto& vout: mtx.vout) {
        if(vout.scriptPubKey.IsUnspendable()) {
            vout.scriptPubKey = scriptData;
            bFoundData = true;
            break;
        }
    }
    if(!bFoundData) {
        return false;
    }
    return true;
}

bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const std::unordered_map<uint32_t, CAmount> &mapAssetIn, const std::unordered_map<uint32_t, CAmount> &mapAssetOut) {
    if(!IsSyscoinWithNoInputTx(tx.nVersion)) {
        if(mapAssetIn != mapAssetOut) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assets-io-mismatch");
        }
    // for asset sends ensure that all inputs of a single asset, and all outputs match the asset guid
    } else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND) {
        if(mapAssetIn.size() != 1) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-wrong-number-inputs");
        }
        const uint32_t &nAsset = mapAssetIn.begin()->first;
        for(const auto& outAsset: mapAssetOut) {
            if(outAsset.first != nAsset) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetsend-io-mismatch");
            }
        }
    }
    return true;
}