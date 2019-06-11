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
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
extern UniValue convertaddress(const JSONRPCRequest& request);
CCriticalSection cs_assetallocation;
CCriticalSection cs_assetallocationarrival;
using namespace std;
AssetBalanceMap mempoolMapAssetBalances GUARDED_BY(cs_assetallocation);
ArrivalTimesMapImpl arrivalTimesMap GUARDED_BY(cs_assetallocationarrival);
std::unordered_set<std::string> assetAllocationConflicts;
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
	case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND:
		return "assetallocationsend";
	case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN:
		return "assetallocationburn"; 
    case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT:
        return "assetallocationmint";   
	case SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK:
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
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN){
        std::vector<unsigned char> vchEthAddress;
        if(!GetSyscoinBurnData(tx, this, vchEthAddress))
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
void CAssetAllocationDB::WriteMintIndex(const CTransaction& tx, const CMintSyscoin& mintSyscoin, const int &nHeight, const uint256& blockhash){
    if (fZMQAssetAllocation || fAssetIndex) {
        if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(), mintSyscoin.assetAllocationTuple.nAsset) == fAssetIndexGuids.end()){
            LogPrint(BCLog::SYS, "Asset allocation cannot be indexed because it is not set in -assetindexguids list\n");
            return;
        }
        UniValue output(UniValue::VOBJ);
        if(AssetMintTxToJson(tx, mintSyscoin, nHeight, blockhash, output)){
            if(fZMQAssetAllocation)
                GetMainSignals().NotifySyscoinUpdate(output.write().c_str(), "assetallocation");
            if(fAssetIndex)
                WriteAssetIndexForAllocation(mintSyscoin, tx.GetHash(), output); 
        }
    }
}
void CAssetAllocationDB::WriteAssetAllocationIndex(const CTransaction &tx, const CAsset& dbAsset, const int &nHeight, const uint256& blockhash) {
	if (fZMQAssetAllocation || fAssetIndex) {
        if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(),dbAsset.nAsset) == fAssetIndexGuids.end()){
            LogPrint(BCLog::SYS, "Asset allocation cannot be indexed because it is not set in -assetindexguids list\n");
            return;
        }
		UniValue oName(UniValue::VOBJ);
        CAssetAllocation allocation;
        if(AssetAllocationTxToJSON(tx, dbAsset, nHeight, blockhash, oName, allocation)){
            if(fZMQAssetAllocation)
                GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assetallocation");
            if(fAssetIndex)
                WriteAssetIndexForAllocation(allocation, tx.GetHash(), oName);          
        }
	}

}
void WriteAssetIndexForAllocation(const CAssetAllocation& assetallocation, const uint256& txid, const UniValue& oName){
    if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(), assetallocation.assetAllocationTuple.nAsset) == fAssetIndexGuids.end()){
        LogPrint(BCLog::SYS, "Asset allocation cannot be indexed because asset is not set in -assetindexguids list\n");
        return;
    }
    // sender
    WriteAssetAllocationIndexTXID(assetallocation.assetAllocationTuple, txid);
    
    // receivers
    for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
         WriteAssetAllocationIndexTXID(CAssetAllocationTuple(assetallocation.assetAllocationTuple.nAsset, amountTuple.first), txid);
    }
    // index into the asset as well     
    WriteAssetIndexTXID(assetallocation.assetAllocationTuple.nAsset, txid);
         
    // write payload only once for txid
    if(!passetindexdb->WritePayload(txid, oName))
        LogPrint(BCLog::SYS, "Failed to write asset index payload\n");        
    
}
void WriteAssetIndexForAllocation(const CMintSyscoin& mintSyscoin, const uint256& txid, const UniValue& oName){
    const CAssetAllocationTuple senderAllocationTuple(mintSyscoin.assetAllocationTuple.nAsset, CWitnessAddress(0, vchFromString("burn")));
    if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(), senderAllocationTuple.nAsset) == fAssetIndexGuids.end()){
        LogPrint(BCLog::SYS, "Mint Asset allocation cannot be indexed because asset is not set in -assetindexguids list\n");
        return;
    }
     // sender
    WriteAssetAllocationIndexTXID(senderAllocationTuple, txid);
      
    // receiver
    WriteAssetAllocationIndexTXID(mintSyscoin.assetAllocationTuple, txid);
    
    // index into the asset as well     
    WriteAssetIndexTXID(mintSyscoin.assetAllocationTuple.nAsset, txid);
    
    if(!passetindexdb->WritePayload(txid, oName))
        LogPrint(BCLog::SYS, "Failed to write asset index payload\n");        

}
void WriteAssetAllocationIndexTXID(const CAssetAllocationTuple& allocationTuple, const uint256& txid){
    // index the allocation
    int64_t page;
    if(!passetindexdb->ReadAssetAllocationPage(page)){
        page = 0;
        if(!passetindexdb->WriteAssetAllocationPage(page))
            LogPrint(BCLog::SYS, "Failed to write asset allocation page\n");           
    }
   
    std::vector<uint256> TXIDS;
    passetindexdb->ReadIndexTXIDs(allocationTuple, page, TXIDS);
    // new page needed
    if(((int)TXIDS.size()) >= fAssetIndexPageSize){
        TXIDS.clear();
        page++;
        if(!passetindexdb->WriteAssetAllocationPage(page))
            LogPrint(BCLog::SYS, "Failed to write asset allocation page\n");        
    }
    TXIDS.push_back(txid);
    if(!passetindexdb->WriteIndexTXIDs(allocationTuple, page, TXIDS))
        LogPrint(BCLog::SYS, "Failed to write asset allocation index txids\n");
        
}
bool GetAssetAllocation(const CAssetAllocationTuple &assetAllocationTuple, CAssetAllocation& txPos) {
    if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
        return false;
    return true;
}

bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, const CAsset& asset, UniValue& oAssetAllocation)
{
    CAmount nBalanceZDAG = assetallocation.nBalance;
    const string &allocationTupleStr = assetallocation.assetAllocationTuple.ToString();
    {
        LOCK(cs_assetallocation);
        AssetBalanceMap::iterator mapIt =  mempoolMapAssetBalances.find(allocationTupleStr);
        if(mapIt != mempoolMapAssetBalances.end())
            nBalanceZDAG = mapIt->second;
    }
    oAssetAllocation.__pushKV("asset_allocation", allocationTupleStr);
	oAssetAllocation.__pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
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
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress))
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
    entry.__pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
    entry.__pushKV("symbol", dbAsset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    entry.__pushKV("height", nHeight);
    entry.__pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    if(pwallet && filter_ismine && ::IsMine(*pwallet, assetallocation.assetAllocationTuple.witnessAddress.GetScriptForDestination()) & *filter_ismine){
        isSenderMine = true;
    }
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;  
    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            if(isSenderMine || (pwallet && filter_ismine && ::IsMine(*pwallet, amountTuple.first.GetScriptForDestination()) & *filter_ismine)){
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
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
	else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK)
		entry.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
    return true;
}
#endif
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry)
{
    CAssetAllocation assetallocation;
    std::vector<unsigned char> vchEthAddress;
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress))
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
    entry.__pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
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
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
    else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK)
        entry.__pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
    return true;
}
bool AssetAllocationTxToJSON(const CTransaction &tx, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash, UniValue &entry, CAssetAllocation& assetallocation)
{
    std::vector<unsigned char> vchEthAddress;
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN){
        if(!GetSyscoinBurnData(tx, &assetallocation, vchEthAddress))
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
    entry.__pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
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
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
         entry.__pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
    return true;
}

bool AssetMintTxToJson(const CTransaction& tx, UniValue &entry){
    CMintSyscoin mintsyscoin(tx);
    if (!mintsyscoin.IsNull() && ((tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && !mintsyscoin.assetAllocationTuple.IsNull()) || tx.nVersion == SYSCOIN_TX_VERSION_MINT)) {
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
        entry.__pushKV("txtype", tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT? "assetallocationmint": "syscoinmint");
        if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT)
            entry.__pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
        if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT){
            CAsset dbAsset;
            GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
            entry.__pushKV("asset_guid", (int)mintsyscoin.assetAllocationTuple.nAsset);
            entry.__pushKV("symbol", dbAsset.strSymbol);
            entry.__pushKV("sender", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
            UniValue oAssetAllocationReceiversArray(UniValue::VARR);

           
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.__pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
            oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
        
            entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
            entry.__pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        }
        else{
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(tx.vout[0].scriptPubKey, o, true);
            entry.__pushKV("scriptPubKey", o);
            entry.__pushKV("total", ValueFromAmount(tx.vout[0].nValue));
        }
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
        oSPVProofObj.__pushKV("ethblocknumber", (int)mintsyscoin.nBlockNumber); 
        entry.__pushKV("spv_proof", oSPVProofObj); 
        return true;
    } 
    return false;
}
bool AssetMintTxToJson(const CTransaction& tx, const CMintSyscoin& mintsyscoin, const int& nHeight, const uint256& blockhash, UniValue &entry){
    if (!mintsyscoin.IsNull() && ((tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT && !mintsyscoin.assetAllocationTuple.IsNull()) || tx.nVersion == SYSCOIN_TX_VERSION_MINT)) {
        entry.__pushKV("txtype", tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT? "assetallocationmint": "syscoinmint");
        if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT)
            entry.__pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
        if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT){
            CAsset dbAsset;
            GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
            entry.__pushKV("asset_guid", (int)mintsyscoin.assetAllocationTuple.nAsset);
            entry.__pushKV("symbol", dbAsset.strSymbol);
            entry.__pushKV("sender", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
            UniValue oAssetAllocationReceiversArray(UniValue::VARR);
           
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.__pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
            oAssetAllocationReceiversObj.__pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
        
            entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
            entry.__pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        }
        else{
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(tx.vout[0].scriptPubKey, o, true);
            entry.__pushKV("scriptPubKey", o);
            entry.__pushKV("total", ValueFromAmount(tx.vout[0].nValue));
        }
        entry.__pushKV("txid", tx.GetHash().GetHex());
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
        oSPVProofObj.__pushKV("ethblocknumber", (int)mintsyscoin.nBlockNumber); 
        entry.__pushKV("spv_proof", oSPVProofObj); 
        return true;                                        
    } 
    return false;                   
}

bool CAssetAllocationMempoolDB::ScanAssetAllocationMempoolBalances(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
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
    int index = 0;
    {
        LOCK(cs_assetallocation);
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
    if(fAssetIndex){
        for (const auto &key : mapAssetAllocations) {
            auto it = mapGuids.emplace(std::piecewise_construct,  std::forward_as_tuple(key.second.assetAllocationTuple.witnessAddress.ToString()),  std::forward_as_tuple(emptyVec));
            std::vector<uint32_t> &assetGuids = it.first->second;
            if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), key.second.assetAllocationTuple.nAsset) == fAssetIndexGuids.end())
                continue;
            // if wasn't found and was added to the map
            if(it.second)
                ReadAssetsByAddress(key.second.assetAllocationTuple.witnessAddress, assetGuids);
            // erase asset address association
            if(key.second.nBalance <= 0){
                auto itVec = std::find(assetGuids.begin(), assetGuids.end(),  key.second.assetAllocationTuple.nAsset);
                if(itVec != assetGuids.end()){
                    assetGuids.erase(itVec);  
                    // ensure we erase only the ones that are actually being cleared
                    if(assetGuids.empty())
                        assetGuids.emplace_back(0);
                }
            }
            else{
                // add asset address association
                auto itVec = std::find(assetGuids.begin(), assetGuids.end(),  key.second.assetAllocationTuple.nAsset);
                if(itVec == assetGuids.end()){
                    // if we had the special erase flag we remove that and add the real guid
                    if(assetGuids.size() == 1 && assetGuids[0] == 0)
                        assetGuids.clear();
                    assetGuids.emplace_back(key.second.assetAllocationTuple.nAsset);
                }
            }      
        }
    }
    for (const auto &key : mapAssetAllocations) {
        if(key.second.nBalance <= 0){
			erase++;
            batch.Erase(key.second.assetAllocationTuple);
        }
        else{
			write++;
            batch.Write(key.second.assetAllocationTuple, key.second);
        }
        if(fAssetIndex){
            if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), key.second.assetAllocationTuple.nAsset) == fAssetIndexGuids.end())
                continue;
            auto it = mapGuids.find(key.second.assetAllocationTuple.witnessAddress.ToString());
            if(it == mapGuids.end())
                continue;
            const std::vector<uint32_t>& assetGuids = it->second;
            // check for special clearing flag before batch erase
            if(assetGuids.size() == 1 && assetGuids[0] == 0)
                batch.Erase(key.second.assetAllocationTuple.witnessAddress);   
            else
                batch.Write(key.second.assetAllocationTuple.witnessAddress, assetGuids); 
            // we have processed this address so don't process again
            mapGuids.erase(it);        
        }
    }
	LogPrint(BCLog::SYS, "Flushing %d assets allocations (erased %d, written %d)\n", mapAssetAllocations.size(), erase, write);
    return WriteBatch(batch);
}
bool CAssetAllocationDB::ScanAssetAllocations(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<CWitnessAddress> vecWitnessAddresses;
	uint32_t nAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &assetObj = find_value(oOptions, "asset_guid");
		if(assetObj.isNum()) {
			nAsset = (uint32_t)assetObj.get_int();
		}

		const UniValue &owners = find_value(oOptions, "addresses");
		if (owners.isArray()) {
			const UniValue &ownersArray = owners.get_array();
			for (unsigned int i = 0; i < ownersArray.size(); i++) {
				const UniValue &owner = ownersArray[i].get_obj();
				const UniValue &ownerStr = find_value(owner, "address");
				if (ownerStr.isStr()) {
                    UniValue requestParam(UniValue::VARR);
                    requestParam.push_back(ownerStr.get_str());
                    JSONRPCRequest jsonRequest;
                    jsonRequest.params = requestParam;
                    const UniValue &convertedAddressValue = convertaddress(jsonRequest);
                    const std::string & v4address = find_value(convertedAddressValue.get_obj(), "v4address").get_str();
                    const CTxDestination &dest = DecodeDestination(v4address);                   
                    UniValue detail = DescribeAddress(dest);
                    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
                    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
                    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
					vecWitnessAddresses.push_back(CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
				}
			}
		}
	}

	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAssetAllocation txPos;
    CAssetAllocationTuple key;
	CAsset theAsset;
	int index = 0;
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
int DetectPotentialAssetAllocationSenderConflicts(const CAssetAllocationTuple& assetAllocationTupleSender, const uint256& lookForTxHash) {
    LOCK(cs_assetallocationarrival);
	CAssetAllocation dbAssetAllocation;
	// get last POW asset allocation balance to ensure we use POW balance to check for potential conflicts in mempool (real-time balances).
	// The idea is that real-time spending amounts can in some cases overrun the POW balance safely whereas in some cases some of the spends are 
	// put in another block due to not using enough fees or for other reasons that miners don't mine them.
	// We just want to flag them as level 1 so it warrants deeper investigation on receiver side if desired (if fund amounts being transferred are not negligible)
	if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTupleSender, dbAssetAllocation))
		return ZDAG_NOT_FOUND;
        

	// ensure that this transaction exists in the arrivalTimes DB (which is the running stored lists of all real-time asset allocation sends not in POW)
	// the arrivalTimes DB is only added to for valid asset allocation sends that happen in real-time and it is removed once there is POW on that transaction
    const ArrivalTimesMap& arrivalTimes = arrivalTimesMap[assetAllocationTupleSender.ToString()];
	if(arrivalTimes.empty())
		return ZDAG_NOT_FOUND;
	// sort the arrivalTimesMap ascending based on arrival time value

	// Declaring the type of Predicate for comparing arrivalTimesMap
	typedef std::function<bool(std::pair<uint256, int64_t>, std::pair<uint256, int64_t>)> Comparator;

	// Defining a lambda function to compare two pairs. It will compare two pairs using second field
	Comparator compFunctor =
		[](std::pair<uint256, int64_t> elem1, std::pair<uint256, int64_t> elem2)
	{
		return elem1.second < elem2.second;
	};

	// Declaring a set that will store the pairs using above comparison logic
	std::set<std::pair<uint256, int64_t>, Comparator> arrivalTimesSet(
		arrivalTimes.begin(), arrivalTimes.end(), compFunctor);

	// go through arrival times and check that balances don't overrun the POW balance
	pair<uint256, int64_t> lastArrivalTime;
	lastArrivalTime.second = GetTimeMillis();
	unordered_map<string, CAmount> mapBalances;
	// init sender balance, track balances by address
	// this is important because asset allocations can be sent/received within blocks and will overrun balances prematurely if not tracked properly, for example pow balance 3, sender sends 3, gets 2 sends 2 (total send 3+2=5 > balance of 3 from last stored state, this is a valid scenario and shouldn't be flagged)
	CAmount &senderBalance = mapBalances[assetAllocationTupleSender.witnessAddress.ToString()];
	senderBalance = dbAssetAllocation.nBalance;
	int minLatency = ZDAG_MINIMUM_LATENCY_SECONDS * 1000;
	if (fUnitTest)
		minLatency = 1000;
	for (auto& arrivalTime : arrivalTimesSet)
	{
		// ensure mempool has this transaction and it is not yet mined, get the transaction in question
		const CTransactionRef txRef = mempool.get(arrivalTime.first);
		if (!txRef)
			continue;
		const CTransaction &tx = *txRef;

		// if this tx arrived within the minimum latency period flag it as potentially conflicting
		if (abs(arrivalTime.second - lastArrivalTime.second) < minLatency) {
			return ZDAG_MINOR_CONFLICT;
		}
		const uint256& txHash = tx.GetHash();
		CAssetAllocation assetallocation(tx);


		if (!assetallocation.listSendingAllocationAmounts.empty()) {
			for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
				senderBalance -= amountTuple.second;
				mapBalances[amountTuple.first.ToString()] += amountTuple.second;
				// if running balance overruns the stored balance then we have a potential conflict
				if (senderBalance < 0) {
					return ZDAG_MINOR_CONFLICT;
				}
			}
		}
		// even if the sender may be flagged, the order of events suggests that this receiver should get his money confirmed upon pow because real-time balance is sufficient for this receiver
		if (txHash == lookForTxHash) {
			return ZDAG_STATUS_OK;
		}
	}
	return lookForTxHash.IsNull()? ZDAG_STATUS_OK: ZDAG_NOT_FOUND;
}
