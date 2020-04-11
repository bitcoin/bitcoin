// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <boost/algorithm/string.hpp>
#include <services/assetconsensus.h>
#include <validationinterface.h>
#include <boost/thread.hpp>
#include <services/rpc/assetrpc.h>
#include <rpc/server.h>
#include <chainparams.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern CAmount AmountFromValue(const UniValue& value);

std::unique_ptr<CAssetDB> passetdb;
using namespace std;

int GetSyscoinDataOutput(const CTransaction& tx) {
	for (unsigned int i = 0; i<tx.vout.size(); i++) {
		if (tx.vout[i].scriptPubKey.IsUnspendable())
			return i;
	}
	return -1;
}
string stringFromValue(const UniValue& value) {
	string strName = value.get_str();
	return strName;
}
vector<unsigned char> vchFromValue(const UniValue& value) {
	string strName = value.get_str();
	unsigned char *strbeg = (unsigned char*)strName.c_str();
	return vector<unsigned char>(strbeg, strbeg + strName.size());
}

std::vector<unsigned char> vchFromString(const std::string &str) {
	unsigned char *strbeg = (unsigned char*)str.c_str();
	return vector<unsigned char>(strbeg, strbeg + str.size());
}
string stringFromVch(const vector<unsigned char> &vch) {
	string res;
	vector<unsigned char>::const_iterator vi = vch.begin();
	while (vi != vch.end()) {
		res += (char)(*vi);
		vi++;
	}
	return res;
}
bool GetSyscoinData(const CTransaction &tx, vector<unsigned char> &vchData, int& nOut)
{
	nOut = GetSyscoinDataOutput(tx);
	if (nOut == -1)
		return false;

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData);
}
bool GetSyscoinData(const CScript &scriptPubKey, vector<unsigned char> &vchData)
{
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode != OP_RETURN)
		return false;
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
    const unsigned int & nSize = scriptPubKey.size();
    // allow up to 80 bytes of data after our stack on standard asset transactions
    unsigned int nDifferenceAllowed = 83;
    // if data is more than 1 byte we used 2 bytes to store the varint (good enough for 64kb which is within limit of opreturn data on sys tx's)
    if(nSize >= 0xff){
        nDifferenceAllowed++;
    }
    if(nSize > (vchData.size() + nDifferenceAllowed)){
        LogPrint(BCLog::SYS, "GetSyscoinData too big scriptPubKey size %d vchData %d\n", scriptPubKey.size(), vchData.size()); 
        return false;
    }
	return true;
}



string assetFromTx(const int &nVersion) {
    switch (nVersion) {
    case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
        return "assetactivate";
    case SYSCOIN_TX_VERSION_ASSET_UPDATE:
        return "assetupdate";
	case SYSCOIN_TX_VERSION_ASSET_SEND:
		return "assetsend";
    default:
        return "<unknown asset op>";
    }
}
bool CAsset::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
		CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
		dsAsset >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}

bool CAsset::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
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
bool FlushSyscoinDBs() {
    bool ret = true;
     if (pethereumtxrootsdb != nullptr)
     {
        if(!pethereumtxrootsdb->PruneTxRoots(fGethCurrentHeight))
        {
            LogPrintf("Failed to write to prune Ethereum TX Roots database!\n");
            ret = false;
        }
        if (!pethereumtxrootsdb->Flush()) {
            LogPrintf("Failed to write to ethereum tx root database!\n");
            ret = false;
        } 
     }
	return ret;
}

bool IsSyscoinMintTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT;
}
bool IsAssetTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE || nVersion == SYSCOIN_TX_VERSION_ASSET_SEND;
}
bool IsAssetAllocationTx(const int &nVersion) {
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION ||
        nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}
bool IsZdagTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND;
}
bool IsSyscoinTx(const int &nVersion){
    return IsAssetTx(nVersion) || IsAssetAllocationTx(nVersion) || IsSyscoinMintTx(nVersion);
}
bool IsSyscoinWithNoInputTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT || nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION;
}

bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output){
    vector<vector<unsigned char> > vvch;
    bool found = false;
    if(IsSyscoinMintTx(rawTx.nVersion)) {
        found = AssetMintTxToJson(rawTx, rawTx.GetHash(), output);
    }
    else if (IsAssetTx(rawTx.nVersion) || IsAssetAllocationTx(rawTx.nVersion)) {
        found = SysTxToJSON(rawTx, output);
    }
    
    return found;
}

bool SysTxToJSON(const CTransaction& tx, UniValue& output)
{
    bool found = false;
    if (IsAssetTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetTxToJSON(tx, output);
    else if (IsAssetAllocationTx(tx.nVersion) || tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetAllocationTxToJSON(tx, output);
    return found;
}
int32_t GenerateSyscoinGuid(const COutPoint& outPoint)
{
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

void CAsset::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}

bool GetAsset(const uint32_t &nAsset,
        CAsset& txPos) {
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}



bool BuildAssetJson(const CAsset& asset,UniValue& oAsset)
{
    oAsset.__pushKV("asset_guid", asset.nAsset);
    oAsset.__pushKV("symbol", asset.strSymbol);
    oAsset.__pushKV("txid", asset.txHash.GetHex());
	oAsset.__pushKV("public_value", stringFromVch(asset.vchPubData));
    oAsset.__pushKV("contract", asset.vchContract.empty()? "" : "0x"+HexStr(asset.vchContract));
	oAsset.__pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));
	oAsset.__pushKV("burn_balance", ValueFromAssetAmount(asset.nBurnBalance, asset.nPrecision));
	oAsset.__pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision));
	oAsset.__pushKV("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision));
	oAsset.__pushKV("update_flags", asset.nUpdateFlags);
	oAsset.__pushKV("precision", asset.nPrecision);
	return true;
}
bool AssetTxToJSON(const CTransaction& tx, UniValue &entry)
{
	CAsset asset(tx);
	if(asset.IsNull())
		return false;

    int nHeight = 0;
    const uint256& txHash = tx.GetHash();
    CBlockIndex* blockindex = nullptr;
    uint256 blockhash;
    pblockindexdb->ReadBlockHash(txHash, blockhash);
        	

	entry.__pushKV("txtype", assetFromTx(tx.nVersion));
	entry.__pushKV("asset_guid", asset.nAsset);
    entry.__pushKV("symbol", asset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    
	if (!asset.vchPubData.empty())
		entry.__pushKV("public_value", stringFromVch(asset.vchPubData));

	if (!asset.vchContract.empty())
		entry.__pushKV("contract", "0x" + HexStr(asset.vchContract));

	if (asset.nUpdateFlags > 0)
		entry.__pushKV("update_flags", asset.nUpdateFlags);

	if (asset.nBalance > 0)
		entry.__pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));

	if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE) {
		entry.__pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision));
        entry.__pushKV("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision));
		entry.__pushKV("precision", asset.nPrecision);
	}
    entry.__pushKV("blockhash", blockhash.GetHex());  
    return true;
}

bool CAssetDB::Flush(const AssetMap &mapAssets){
    if(mapAssets.empty())
        return true;
	int write = 0;
	int erase = 0;
    CDBBatch batch(*this);
    std::map<std::string, std::vector<uint32_t> > mapGuids;
    std::vector<uint32_t> emptyVec;
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
bool CAssetDB::ScanAssets(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
    uint32_t nAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &txid = find_value(oOptions, "txid");
		if (txid.isStr()) {
			strTxid = txid.get_str();
		}
		const UniValue &assetObj = find_value(oOptions, "asset_guid");
		if (assetObj.isNum()) {
			nAsset = assetObj.get_uint();
		}
	}
	std::unique_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAsset txPos;
	uint32_t key = 0;
	uint32_t index = 0;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
            key = 0;
			if (pcursor->GetKey(key) && key != 0 && (nAsset == 0 || nAsset != key)) {
				pcursor->GetValue(txPos);
                if(txPos.IsNull()){
                    pcursor->Next();
                    continue;
                }
				if (!strTxid.empty() && strTxid != txPos.txHash.GetHex())
				{
					pcursor->Next();
					continue;
				}
				UniValue oAsset(UniValue::VOBJ);
				if (!BuildAssetJson(txPos, oAsset))
				{
					pcursor->Next();
					continue;
				}
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				oRes.push_back(oAsset);
				if (index >= count + from)
					break;
			}
			pcursor->Next();
		}
		catch (std::exception &e) {
			return error("%s() : deserialize error", __PRETTY_FUNCTION__);
		}
	}
	return true;
}