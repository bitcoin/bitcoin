// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <services/asset.h>
#include <consensus/validation.h>
std::unique_ptr<CAssetDB> passetdb;

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

bool CAsset::UnserializeFromTx(const CTransactionRef &tx) {
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


int32_t GenerateSyscoinGuid(const COutPoint& outPoint) {
    const arith_uint256 &txidArith = UintToArith256(outPoint.hash);
    int32_t low32 = (int32_t)txidArith.GetLow64();
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

bool GetAsset(const int32_t &nAsset,
        CAsset& txPos) {
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}

bool ReserializeAssetCommitment(CMutableTransaction& mtx) {
    // load tx.voutAssets from tx.vout.assetInfo info
    mtx.LoadAssetsFromVout();
    CTransactionRef tx(MakeTransactionRef(mtx));
    // store tx.voutAssets into OP_RETURN data overwriting previous commitment
    std::vector<unsigned char> data;
    if(IsSyscoinMintTx(tx->nVersion)) {
        CMintSyscoin mintSyscoin(tx);
        mintSyscoin.assetAllocation.voutAssets = tx->voutAssets;
        mintSyscoin.SerializeData(data);
    } else if(tx->nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
        CBurnSyscoin burnSyscoin(tx);
        burnSyscoin.assetAllocation.voutAssets = tx->voutAssets;
        burnSyscoin.SerializeData(data);
    } else if(IsAssetTx(tx->nVersion)) {
        CAsset asset(tx);
        asset.assetAllocation.voutAssets = tx->voutAssets;
        asset.SerializeData(data); 
    } else if(IsAssetAllocationTx(tx->nVersion)) {
        CAssetAllocation allocation(tx);
        allocation.voutAssets = tx->voutAssets;
        allocation.SerializeData(data); 
    }
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

bool CAssetDB::Flush(const AssetMap &mapAssets){
    if(mapAssets.empty()) {
        return true;
	}
	int write = 0;
	int erase = 0;
    CDBBatch batch(*this);
    std::map<std::string, std::vector<int32_t> > mapGuids;
    std::vector<int32_t> emptyVec;
    for (const auto &key : mapAssets) {
		if (key.second.IsNull()) {
			erase++;
			batch.Erase(key.first);
		}
		else {
			write++;
			batch.Write(key.first, key.second);
		}
    }
    LogPrint(BCLog::SYS, "Flushing %d assets (erased %d, written %d)\n", mapAssets.size(), erase, write);
    return WriteBatch(batch);
}

template<typename Stream>
void CAsset::Serialize(Stream &s) const {	
    s << assetAllocation;
    s << vchPubData;
    s << strSymbol;
    s << nUpdateFlags;
    s << nPrecision;
    s << vchContract;
    ::Serialize(s, Using<AmountCompression>(nBalance));
    ::Serialize(s, Using<AmountCompression>(nTotalSupply));
    ::Serialize(s, Using<AmountCompression>(nMaxSupply));
}
template<typename Stream>
void CAsset::Unserialize(Stream &s) {	
    s >> assetAllocation;
    s >> vchPubData;
    s >> strSymbol;
    s >> nUpdateFlags;
    s >> nPrecision;
    s >> vchContract;
    ::Unserialize(s, Using<AmountCompression>(nBalance));
    ::Unserialize(s, Using<AmountCompression>(nTotalSupply));
    ::Unserialize(s, Using<AmountCompression>(nMaxSupply));
}