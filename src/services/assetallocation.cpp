// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <future>
#include <validationinterface.h>
#include <services/assetconsensus.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif
#include <services/rpc/assetrpc.h>
#include <rpc/server.h>
#include <chainparams.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
extern UniValue convertaddress(const JSONRPCRequest& request);
RecursiveMutex cs_assetallocationmempoolbalance;
RecursiveMutex cs_assetallocationarrival;
RecursiveMutex cs_assetallocationconflicts;
RecursiveMutex cs_setethstatus;
using namespace std;
AssetBalanceMap mempoolMapAssetBalances GUARDED_BY(cs_assetallocationmempoolbalance);
ArrivalTimesVecImpl arrivalTimesVec GUARDED_BY(cs_assetallocationarrival);
std::unordered_set<std::string> assetAllocationConflicts GUARDED_BY(cs_assetallocationconflicts);
extern RecursiveMutex cs_assetallocationmempoolremovetx;
extern std::unordered_set<uint256, SaltedTxidHasher> setToRemoveFromMempool;
string CWitnessAddress::ToString() const {
    if (vchWitnessProgram.size() <= 4 && stringFromVch(vchWitnessProgram) == "burn")
        return "burn";
    
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
CScript CWitnessAddress::GetScriptForDestination() const {
    CTxDestination destination;
    CScript script;
    if(GetDestination(destination)){
        return ::GetScriptForDestination(destination);     
    }
    return script;
}
CScript CWitnessAddress::GetScriptForDestination(CTxDestination& destination) const {
    CScript script;
    if(GetDestination(destination)){
        if(!destination.empty()){
            return ::GetScriptForDestination(destination);     
        }
    }
    return script;
}
bool CWitnessAddress::GetDestination(CTxDestination & destination) const {
    CScript script;
    if (vchWitnessProgram.size() <= 4 && stringFromVch(vchWitnessProgram) == "burn")
        return false;
    
    if(nVersion == 0){
        if (vchWitnessProgram.size() == WITNESS_V0_KEYHASH_SIZE) {
            destination = WitnessV0KeyHash(vchWitnessProgram);
        }
        else if (vchWitnessProgram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            destination = WitnessV0ScriptHash(vchWitnessProgram);
        }
        else
            return false;
    }
    else
        return false;
    return true;
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
string CAssetAllocationTuple::ToString() const {
	return itostr(nAsset) + "-" + witnessAddress.ToString();
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
	case SYSCOIN_TX_VERSION_ALLOCATION_LOCK:
		return "assetallocationlock";
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
	if (!IsAssetAllocationTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
	{
		SetNull();
		return false;
	}
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
        std::vector<unsigned char> vchEthAddress;
        std::vector<unsigned char> vchEthContract;
        if(!GetSyscoinBurnData(tx, this, vchEthAddress, vchEthContract))
        {
            SetNull();
            return false;
        }
    }
    else{
        if (!GetSyscoinData(tx, vchData, nOut))
        {
            SetNull();
            return false;
        }
    	if(!UnserializeFromData(vchData))
    	{	
    		return false;
    	}
    }
    return true;
}
void CAssetAllocation::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
bool CAssetAllocationDBEntry::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsAsset >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
void CAssetAllocationDBEntry::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
bool GetAssetAllocation(const CAssetAllocationTuple &assetAllocationTuple, CAssetAllocationDBEntry& txPos) {
    if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
        return false;
    return true;
}

bool BuildAssetAllocationJson(const CAssetAllocationDBEntry& assetallocation, const CAsset& asset, UniValue& oAssetAllocation)
{
    CAmount nBalanceZDAG = assetallocation.nBalance;
    const string &allocationTupleStr = assetallocation.assetAllocationTuple.ToString();
    {
        LOCK(cs_assetallocationmempoolbalance);
        AssetBalanceMap::iterator mapIt =  mempoolMapAssetBalances.find(allocationTupleStr);
        if(mapIt != mempoolMapAssetBalances.end())
            nBalanceZDAG = mapIt->second;
    }
    oAssetAllocation.__pushKV("asset_allocation", allocationTupleStr);
	oAssetAllocation.__pushKV("asset_guid", assetallocation.assetAllocationTuple.nAsset);
    oAssetAllocation.__pushKV("symbol", asset.strSymbol);
	oAssetAllocation.__pushKV("address",  assetallocation.assetAllocationTuple.witnessAddress.ToString());
	oAssetAllocation.__pushKV("balance", ValueFromAssetAmount(assetallocation.nBalance, asset.nPrecision));
    oAssetAllocation.__pushKV("balance_zdag", ValueFromAssetAmount(nBalanceZDAG, asset.nPrecision));
    if(!assetallocation.lockedOutpoint.IsNull())
        oAssetAllocation.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
	return true;
}
// TODO: clean this up copied to support disable-wallet build
#ifdef ENABLE_WALLET
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry, CWallet* const pwallet, const isminefilter* filter_ismine)
{
    CAssetAllocation assetallocation;
    std::vector<unsigned char> vchEthAddress;
    std::vector<unsigned char> vchEthContract;
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress, vchEthContract))
        {
            return false;
        }
    }
    else
        assetallocation = CAssetAllocation(tx);

    if(assetallocation.assetAllocationTuple.IsNull())
        return false;
    CAsset dbAsset;
    GetAsset(assetallocation.assetAllocationTuple.nAsset, dbAsset);
    int nHeight = 0;
    const uint256& txHash = tx.GetHash();
    CBlockIndex* blockindex = nullptr;
    uint256 blockhash;
    if(pblockindexdb->ReadBlockHash(txHash, blockhash)){ 
        LOCK(cs_main);
        blockindex = LookupBlockIndex(blockhash);
    }
    if(blockindex)
    {
        nHeight = blockindex->nHeight;
    }
    bool isSenderMine = false;
    entry.__pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.__pushKV("asset_allocation", assetallocation.assetAllocationTuple.ToString());
    entry.__pushKV("asset_guid", assetallocation.assetAllocationTuple.nAsset);
    entry.__pushKV("symbol", dbAsset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    entry.__pushKV("height", nHeight);
    entry.__pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    if(pwallet && filter_ismine && pwallet->IsMine(assetallocation.assetAllocationTuple.witnessAddress.GetScriptForDestination()) & *filter_ismine){
        isSenderMine = true;
    }
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;  
    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            if(isSenderMine || (pwallet && filter_ismine && pwallet->IsMine(amountTuple.first.GetScriptForDestination()) & *filter_ismine)){
                UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
                oAssetAllocationReceiversObj.__pushKV("address", strReceiver);
                oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision));
                oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
            }
        }
    }
    entry.__pushKV("allocations", oAssetAllocationReceiversArray);
    entry.__pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.__pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
         entry.__pushKV("ethereum_contract", "0x" + HexStr(vchEthContract));
    }
	else if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK)
		entry.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
    return true;
}
#endif
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry)
{
    CAssetAllocation assetallocation;
    std::vector<unsigned char> vchEthAddress;
    std::vector<unsigned char> vchEthContract;
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress, vchEthContract))
        {
            return false;
        }
    }
    else
        assetallocation = CAssetAllocation(tx);

    if(assetallocation.assetAllocationTuple.IsNull())
        return false;
    CAsset dbAsset;
    GetAsset(assetallocation.assetAllocationTuple.nAsset, dbAsset);
    int nHeight = 0;
    const uint256& txHash = tx.GetHash();
    CBlockIndex* blockindex = nullptr;
    uint256 blockhash;
    if(pblockindexdb->ReadBlockHash(txHash, blockhash)){ 
        LOCK(cs_main);
        blockindex = LookupBlockIndex(blockhash);
    }
    if(blockindex)
    {
        nHeight = blockindex->nHeight;
    }
    entry.__pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.__pushKV("asset_allocation", assetallocation.assetAllocationTuple.ToString());
    entry.__pushKV("asset_guid", assetallocation.assetAllocationTuple.nAsset);
    entry.__pushKV("symbol", dbAsset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    entry.__pushKV("height", nHeight);
    entry.__pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;  
    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.__pushKV("address", strReceiver);
            oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
            
        }
    }
    entry.__pushKV("allocations", oAssetAllocationReceiversArray);
    entry.__pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.__pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
         entry.__pushKV("ethereum_contract", "0x" + HexStr(vchEthContract));
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK)
        entry.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
    return true;
}
bool AssetAllocationTxToJSON(const CTransaction &tx, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash, UniValue &entry, CAssetAllocation& assetallocation)
{
    std::vector<unsigned char> vchEthAddress;
    std::vector<unsigned char> vchEthContract;
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress, vchEthContract))
        {
            return false;
        }
    }
    else
        assetallocation = CAssetAllocation(tx);

    if(assetallocation.assetAllocationTuple.IsNull() || dbAsset.IsNull())
        return false;
    entry.__pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.__pushKV("asset_allocation", assetallocation.assetAllocationTuple.ToString());
    entry.__pushKV("asset_guid", assetallocation.assetAllocationTuple.nAsset);
    entry.__pushKV("symbol", dbAsset.strSymbol);
    entry.__pushKV("txid", tx.GetHash().GetHex());
    entry.__pushKV("height", nHeight);
    entry.__pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;

    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.__pushKV("address", strReceiver);
            oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);          
        }
    }
    entry.__pushKV("allocations", oAssetAllocationReceiversArray);
    entry.__pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.__pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
         entry.__pushKV("ethereum_contract", "0x" + HexStr(vchEthContract));
    }
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK)
        entry.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());         
    return true;
}

bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, UniValue &entry){
    CMintSyscoin mintsyscoin(tx);
    if (!mintsyscoin.IsNull() && !mintsyscoin.assetAllocationTuple.IsNull()) {
        int nHeight = 0;
        CBlockIndex* blockindex = nullptr;
        uint256 blockhash;
        if(pblockindexdb->ReadBlockHash(txHash, blockhash)){ 
            LOCK(cs_main);
            blockindex = LookupBlockIndex(blockhash);
        }
        if(blockindex)
        {
            nHeight = blockindex->nHeight;
        }
        entry.__pushKV("txtype", "assetallocationmint");
      
        entry.__pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
        entry.__pushKV("asset_guid", mintsyscoin.assetAllocationTuple.nAsset);
        entry.__pushKV("symbol", dbAsset.strSymbol);
        entry.__pushKV("sender", burnWitnessStr);
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        oAssetAllocationReceiversObj.__pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    
        entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
        entry.__pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        entry.__pushKV("txid", txHash.GetHex());
        entry.__pushKV("height", nHeight);
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
bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, const CMintSyscoin& mintsyscoin, const int& nHeight, const uint256& blockhash, UniValue &entry){
    if (!mintsyscoin.IsNull() && !mintsyscoin.assetAllocationTuple.IsNull()) {
        entry.__pushKV("txtype", tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT? "assetallocationmint": "syscoinmint");
        entry.__pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
       
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
        entry.__pushKV("asset_guid", mintsyscoin.assetAllocationTuple.nAsset);
        entry.__pushKV("symbol", dbAsset.strSymbol);
        entry.__pushKV("sender", burnWitnessStr);
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        oAssetAllocationReceiversObj.__pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    
        entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
        entry.__pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        entry.__pushKV("txid", txHash.GetHex());
        entry.__pushKV("height", nHeight);
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

bool CAssetAllocationMempoolDB::ScanAssetAllocationMempoolBalances(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
    string strTxid = "";
    vector<string> vecSenders;
    vector<string> vecReceivers;
    string strAsset = "";
    if (!oOptions.isNull()) {
       
        const UniValue &senders = find_value(oOptions, "senders");
        if (senders.isArray()) {
            const UniValue &sendersArray = senders.get_array();
            for (unsigned int i = 0; i < sendersArray.size(); i++) {
                const UniValue &sender = sendersArray[i].get_obj();
                const UniValue &senderStr = find_value(sender, "address");
                if (senderStr.isStr()) {
                    vecSenders.push_back(senderStr.get_str());
                }
            }
        }
    }
    uint32_t index = 0;
    {
        LOCK(cs_assetallocationmempoolbalance);
        for (auto&indexObj : mempoolMapAssetBalances) {
            
            if (!vecSenders.empty() && std::find(vecSenders.begin(), vecSenders.end(), indexObj.first) == vecSenders.end())
                continue;
            index += 1;
            if (index <= from) {
                continue;
            }
            UniValue resultObj(UniValue::VOBJ);
            resultObj.__pushKV(indexObj.first, ValueFromAmount(indexObj.second));
            oRes.push_back(resultObj);
            if (index >= count + from)
                break;
        }       
    }
    return true;
}

bool CAssetAllocationDB::Flush(const AssetAllocationMap &mapAssetAllocations){
    if(mapAssetAllocations.empty())
        return true;
    CDBBatch batch(*this);
	int write = 0;
	int erase = 0;
    std::map<std::string, std::vector<uint32_t> > mapGuids;
    std::vector<uint32_t> emptyVec;
    for (const auto &key : mapAssetAllocations) {
        if(key.second.nBalance <= 0){
			erase++;
            batch.Erase(key.second.assetAllocationTuple);
        }
        else{
			write++;
            batch.Write(key.second.assetAllocationTuple, key.second);
        }
    }
	LogPrint(BCLog::SYS, "Flushing %d assets allocations (erased %d, written %d)\n", mapAssetAllocations.size(), erase, write);
    return WriteBatch(batch);
}
bool CAssetAllocationDB::ScanAssetAllocations(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<CWitnessAddress> vecWitnessAddresses;
	uint32_t nAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &assetObj = find_value(oOptions, "asset_guid");
		if(assetObj.isNum()) {
			nAsset = assetObj.get_uint();
		}

		const UniValue &owners = find_value(oOptions, "addresses");
		if (owners.isArray()) {
			const UniValue &ownersArray = owners.get_array();
			for (unsigned int i = 0; i < ownersArray.size(); i++) {
				const UniValue &owner = ownersArray[i].get_obj();
				const UniValue &ownerValue = find_value(owner, "address");
				if (ownerValue.isStr()) {
                    vecWitnessAddresses.push_back(DescribeWitnessAddress(ownerValue.get_str())); 
				}
			}
		}
	}

	std::unique_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAssetAllocationDBEntry txPos;
    CAssetAllocationTuple key;
	CAsset theAsset;
	uint32_t index = 0;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
            key.SetNull();
			if (pcursor->GetKey(key) && !key.IsNull() && (nAsset == 0 || nAsset == key.nAsset)) {
				pcursor->GetValue(txPos); 
                if(txPos.assetAllocationTuple.IsNull()){
                    pcursor->Next();
                    continue;
                }      
				if (!vecWitnessAddresses.empty() && std::find(vecWitnessAddresses.begin(), vecWitnessAddresses.end(), txPos.assetAllocationTuple.witnessAddress) == vecWitnessAddresses.end())
				{
					pcursor->Next();
					continue;
				}
                if (!GetAsset(key.nAsset, theAsset))
                {
                    pcursor->Next();
                    continue;
                } 
				UniValue oAssetAllocation(UniValue::VOBJ);
				if (!BuildAssetAllocationJson(txPos, theAsset, oAssetAllocation)) 
				{
					pcursor->Next();
					continue;
				}
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				oRes.push_back(oAssetAllocation);
				if (index >= count + from) {
					break;
				}
			}
			pcursor->Next();
		}
		catch (std::exception &e) {
			return error("%s() : deserialize error", __PRETTY_FUNCTION__);
		}
	}
	return true;
}
void GetActorsFromSyscoinTx(const CTransactionRef& txRef, bool bJustSender, bool bGetAddress, ActorSet& actorSet){
    if(IsSyscoinMintTx(txRef->nVersion)){
        CMintSyscoin theMintSyscoin(*txRef);
        if(!theMintSyscoin.IsNull())
            GetActorsFromMintTx(theMintSyscoin, bJustSender, bGetAddress, actorSet);
    }
    else if(IsAssetTx(txRef->nVersion)){
        CAsset theAsset;
        CAssetAllocation theAssetAllocation;
        if(txRef->nVersion == SYSCOIN_TX_VERSION_ASSET_SEND){
            theAssetAllocation = CAssetAllocation(*txRef);
            if(!theAssetAllocation.assetAllocationTuple.IsNull())
                GetActorsFromAssetTx(theAsset, theAssetAllocation, txRef->nVersion, bJustSender, bGetAddress, actorSet);
                
        }
        else{
            theAsset = CAsset(*txRef);
            if(!theAsset.IsNull())
                GetActorsFromAssetTx(theAsset, theAssetAllocation, txRef->nVersion, bJustSender, bGetAddress, actorSet);
        }
    }
    else if(IsAssetAllocationTx(txRef->nVersion)){
        CAssetAllocation theAssetAllocation(*txRef);
        if(!theAssetAllocation.assetAllocationTuple.IsNull())
            GetActorsFromAssetAllocationTx(theAssetAllocation, txRef->nVersion, bJustSender, bGetAddress, actorSet);
    }
}
void GetActorsFromMintTx(const CMintSyscoin& theMintSyscoin, bool bJustSender, bool bGetAddress, ActorSet& actorSet) {
    const std::string &receiverAddress = theMintSyscoin.assetAllocationTuple.witnessAddress.ToString();
    if(receiverAddress != "burn" || (receiverAddress == "burn" && !bGetAddress))
        actorSet.insert(std::move(receiverAddress));
}
void GetActorsFromAssetTx(const CAsset& theAsset, const CAssetAllocation& theAssetAllocation, int nVersion, bool bJustSender, bool bGetAddress, ActorSet& actorSet) {
    CAssetAllocationTuple receiverAllocationTuple;
    if(nVersion == SYSCOIN_TX_VERSION_ASSET_SEND){
        actorSet.insert(theAssetAllocation.assetAllocationTuple.witnessAddress.ToString());           
    }
    else{
        actorSet.insert(theAsset.witnessAddress.ToString());
    }
    if(bJustSender)
        return;
    switch (nVersion) {
        case SYSCOIN_TX_VERSION_ASSET_SEND:
            for (unsigned int i = 0;i<theAssetAllocation.listSendingAllocationAmounts.size();i++) {
                receiverAllocationTuple = CAssetAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, theAssetAllocation.listSendingAllocationAmounts[i].first);
                const std::string &receiverAddress = receiverAllocationTuple.witnessAddress.ToString();
                if(!bGetAddress && receiverAddress == "burn")
                    continue;
                actorSet.insert(std::move(receiverAddress));
            }  
            break; 
        case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
        case SYSCOIN_TX_VERSION_ASSET_UPDATE:
            break;
        case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
            const std::string &receiverTupleStr = theAsset.witnessAddressTransfer.ToString();
            if(receiverTupleStr != "burn" || (receiverTupleStr == "burn" && !bGetAddress))
                actorSet.insert(std::move(receiverTupleStr));
            break;
    }
}
void GetActorsFromAssetAllocationTx(const CAssetAllocation &theAssetAllocation, int nVersion, bool bJustSender, bool bGetAddress, ActorSet& actorSet) {
    bool bAddSender = true;
    CAssetAllocationTuple receiverAllocationTuple;
    switch (nVersion) {
        case SYSCOIN_TX_VERSION_ALLOCATION_SEND:
            if(!bJustSender){
                for (unsigned int i = 0;i<theAssetAllocation.listSendingAllocationAmounts.size();i++) {
                    receiverAllocationTuple = CAssetAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, theAssetAllocation.listSendingAllocationAmounts[i].first);
                    const std::string &receiverAddress = receiverAllocationTuple.witnessAddress.ToString();
                    if(!bGetAddress && receiverAddress == "burn")
                        continue;
                    actorSet.insert(bGetAddress? std::move(receiverAddress): receiverAllocationTuple.ToString());
                }
            }    
            break; 
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM:
        case SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN:
            // if getting address, this is empty because "burn" is not an address
            if(!bJustSender && !bGetAddress){
                receiverAllocationTuple = CAssetAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, burnWitness);
                actorSet.insert(receiverAllocationTuple.ToString());
            }
            break;
        case SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION:
            // clear sender if getting address as sender is "burn" and invalid address
            if(bGetAddress || bJustSender){
                bAddSender = false;
            }
            receiverAllocationTuple = CAssetAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, theAssetAllocation.listSendingAllocationAmounts[0].first);
            actorSet.insert(bGetAddress? receiverAllocationTuple.witnessAddress.ToString(): receiverAllocationTuple.ToString());
            break;            
		case SYSCOIN_TX_VERSION_ALLOCATION_LOCK:
			break;
    }
    if(bAddSender){
        actorSet.insert(bGetAddress? theAssetAllocation.assetAllocationTuple.witnessAddress.ToString(): theAssetAllocation.assetAllocationTuple.ToString());
    }
    
}
template <typename Stream, typename Operation>
void CAssetAllocation::SerializationOp(Stream& s, Operation ser_action) {
    READWRITE(assetAllocationTuple);
    READWRITE(listSendingAllocationAmounts);
    if(::ChainActive().Tip()->nHeight <= Params().GetConsensus().nBridgeStartBlock){
        CAmount nBalance = 0;
        READWRITE(nBalance);
        READWRITE(lockedOutpoint);
    }
    else{
        if(!ser_action.ForRead())
            lockedOutpointSet = lockedOutpoint.n == COutPoint::NULL_INDEX? 0: 1;
        READWRITE(lockedOutpointSet);
        if(lockedOutpointSet == 1)
            READWRITE(lockedOutpoint);
    }
}
template <typename Stream, typename Operation>
void CAssetAllocationDBEntry::SerializationOp(Stream& s, Operation ser_action) {
    READWRITE(assetAllocationTuple);
    READWRITE(nBalance);
    if(::ChainActive().Tip()->nHeight <= Params().GetConsensus().nBridgeStartBlock){
        RangeAmountTuples listSendingAllocationAmounts;
        READWRITE(listSendingAllocationAmounts);
        READWRITE(lockedOutpoint);
    }
    else{
        if(!ser_action.ForRead())
            lockedOutpointSet = lockedOutpoint.n == COutPoint::NULL_INDEX? 0: 1;
        READWRITE(lockedOutpointSet);
        if(lockedOutpointSet == 1)
            READWRITE(lockedOutpoint);
    }
}
bool GetSenderOfZdagTx(const CTransaction &tx, std::string& senderStr){
    if(!IsAssetAllocationTx(tx.nVersion))
        return false;
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        return false;
    }
    ActorSet actorSet;
    GetActorsFromAssetAllocationTx(theAssetAllocation, tx.nVersion, true, false, actorSet);
    senderStr = *actorSet.begin();
    return true;
}