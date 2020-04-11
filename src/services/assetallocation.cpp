// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <future>
#include <validationinterface.h>
#include <services/assetconsensus.h>
#include <services/rpc/assetrpc.h>
#include <rpc/server.h>
#include <chainparams.h>
#include <services/witnessaddress.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
RecursiveMutex cs_setethstatus;
using namespace std;
std::string CWitnessAddress::ToString() const {
    if(nVersion == 0){
        if (vchWitnessProgram.size() == WITNESS_V0_KEYHASH_SIZE) {
            return EncodeDestination(WitnessV0KeyHash(vchWitnessProgram));
        }
        else if (vchWitnessProgram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            return EncodeDestination(WitnessV0ScriptHash(vchWitnessProgram));
        }
    }
    return "";
}
bool CWitnessAddress::IsValid() const {
    const size_t& size = vchWitnessProgram.size();
    // this is a hard limit 2->40
    if(size < 2 || size > 40){
        return false;
    }
    // BIP 142, version 0 must be of p2wpkh or p2wpsh size
    if(nVersion == 0){
        return (size == WITNESS_V0_KEYHASH_SIZE || size == WITNESS_V0_SCRIPTHASH_SIZE);
    }
    // otherwise mark as valid for future softfork expansion
    return true;
}
string assetAllocationFromTx(const int &nVersion) {
    switch (nVersion) {
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
bool CAssetAllocation::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsAsset >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
bool CAssetAllocation::UnserializeFromTx(const CTransaction &tx) {
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
void CAssetAllocation::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
bool CMintSyscoin::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsMS >> *this;
    } catch (std::exception &e) {
        SetNull();
        return false;
    }
    return true;
}
bool CMintSyscoin::UnserializeFromTx(const CTransaction &tx) {
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
void CMintSyscoin::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsMint(SER_NETWORK, PROTOCOL_VERSION);
    dsMint << *this;
    vchData = vector<unsigned char>(dsMint.begin(), dsMint.end());
}

bool CBurnSyscoin::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsMS >> *this;
    } catch (std::exception &e) {
        SetNull();
        return false;
    }
    return true;
}
bool CBurnSyscoin::UnserializeFromTx(const CTransaction &tx) {
    if(tx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM)
        return false;
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
void CBurnSyscoin::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsBurn(SER_NETWORK, PROTOCOL_VERSION);
    dsBurn << *this;
    vchData = vector<unsigned char>(dsBurn.begin(), dsBurn.end());
}

bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry)
{
    CAssetAllocation assetallocation(tx);
    if(assetallocation.IsNull())
        return false;
    CAsset dbAsset;
    GetAsset(assetallocation.nAsset, dbAsset);
    int nHeight = 0;
    const uint256& txHash = tx.GetHash();
    CBlockIndex* blockindex = nullptr;
    uint256 blockhash;
    pblockindexdb->ReadBlockHash(txHash, blockhash);
    entry.__pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.__pushKV("asset_guid", assetallocation.nAsset);
    entry.__pushKV("symbol", dbAsset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;
    for(int i =0; i < mintsyscoin.assetAllocation.voutAssets.size();i++) {
        nTotal += voutAsset;
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        oAssetAllocationReceiversObj.__pushKV("n", i);
        oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.assetAllocation.voutAssets[i], dbAsset.nPrecision));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    }

    entry.__pushKV("allocations", oAssetAllocationReceiversArray);
    entry.__pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.__pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
         CBurnSyscoin burnSyscoin(tx);
         entry.__pushKV("ethereum_destination", "0x" + HexStr(burnSyscoin.vchEthAddress));
         entry.__pushKV("ethereum_contract", "0x" + HexStr(dbAsset.vchContract));
    }
    return true;
}


bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, UniValue &entry){
    CMintSyscoin mintsyscoin(tx);
    if (!mintsyscoin.IsNull()) {
        int nHeight = 0;
        CBlockIndex* blockindex = nullptr;
        uint256 blockhash;
        pblockindexdb->ReadBlockHash(txHash, blockhash);
        entry.__pushKV("txtype", "assetallocationmint");
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocation.nAsset, dbAsset);
        entry.__pushKV("asset_guid", mintsyscoin.assetAllocation.nAsset);
        entry.__pushKV("symbol", dbAsset.strSymbol);
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        CAmount nTotal = 0;
        for(int i =0; i < mintsyscoin.assetAllocation.voutAssets.size();i++) {
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            nTotal += voutAsset;
            oAssetAllocationReceiversObj.__pushKV("n", i);
            oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.assetAllocation.voutAssets[i], dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
        }
    
        entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
        entry.__pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
        entry.__pushKV("txid", txHash.GetHex());
        entry.__pushKV("blockhash", blockhash.GetHex());
        UniValue oSPVProofObj(UniValue::VOBJ);
        oSPVProofObj.__pushKV("txvalue", HexStr(mintsyscoin.vchTxValue));   
        oSPVProofObj.__pushKV("txparentnodes", HexStr(mintsyscoin.vchTxParentNodes)); 
        oSPVProofObj.__pushKV("txroot", HexStr(mintsyscoin.vchTxRoot));
        oSPVProofObj.__pushKV("txpath", HexStr(mintsyscoin.vchTxPath)); 
        oSPVProofObj.__pushKV("receiptvalue", HexStr(mintsyscoin.vchReceiptValue));   
        oSPVProofObj.__pushKV("receiptparentnodes", HexStr(mintsyscoin.vchReceiptParentNodes)); 
        oSPVProofObj.__pushKV("receiptroot", HexStr(mintsyscoin.vchReceiptRoot)); 
        oSPVProofObj.__pushKV("ethblocknumber", mintsyscoin.nBlockNumber); 
        entry.__pushKV("spv_proof", oSPVProofObj); 
        return true;
    } 
    return false;
}

    return false;                   
}