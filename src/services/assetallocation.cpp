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
RecursiveMutex cs_setethstatus;
using namespace std;


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
bool FillAssetInfo(CAssetAllocation& assetAllocation) {
    // fill in vecAssetInfo with lookup against tx output index which is used by coins and txmempool to iterate asset info based on output index
    assetAllocation.vecAssetInfo.reserve(tx.vout.size());
    std::unordered_set<int8_t> seenIndex;
    size_t nTotalAssetOutputs = 0;
    for(const auto &allocationMapEntry: assetAllocation.listReceivingAssets){
        const uint32_t & nAsset = allocationMapEntry.first;
        const std::map<uint8_t, CAmount> & mapAssets = allocationMapEntry.second;
        nTotalAssetOutputs += mapAssets.size();
        for (auto assetIt: mapAssets) {
            // if output index is out of range its invalid
            if(assetIt->first >= tx.vout.size()){
                SetNull();
                return false;
            }
            CAssetCoinInfo assetInfo(nAsset, assetIt->second);
            assetAllocation.vecAssetInfo[assetIt->first] = std::move(assetInfo));
            auto itInserted = seenIndex.emplace(assetIt->first);
            // ensure indexes are unique and used only once
            if(!itInserted.second){
                SetNull();
                return false;
            }
        }
    }
    if(nTotalAssetOutputs > tx.vout.size()){
        SetNull();
        return false;
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
    
    return FillAssetInfo(*this);
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

bool BuildAssetAllocationJson(const CAssetCoinInfo& assetallocation, const CAsset& asset, UniValue& oAssetAllocation)
{
    CAmount nBalanceZDAG = assetallocation.nBalance;
	oAssetAllocation.__pushKV("asset_guid", assetallocation.assetAllocationTuple.nAsset);
    oAssetAllocation.__pushKV("symbol", asset.strSymbol);
	oAssetAllocation.__pushKV("address",  assetallocation.assetAllocationTuple.witnessAddress.ToString());
	oAssetAllocation.__pushKV("balance", ValueFromAssetAmount(assetallocation.nBalance, asset.nPrecision));
    oAssetAllocation.__pushKV("balance_zdag", ValueFromAssetAmount(nBalanceZDAG, asset.nPrecision));
	return true;
}
// TODO: clean this up copied to support disable-wallet build
#ifdef ENABLE_WALLET
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry, const CWallet* const pwallet, const isminefilter* filter_ismine)
{
    CAssetAllocation assetallocation(tx);
    std::vector<unsigned char> vchEthAddress;
    std::vector<unsigned char> vchEthContract;
    
        assetallocation = CAssetAllocation(tx);

    if(assetallocation.IsNull())
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

    if(assetallocation.IsNull())
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

    if(assetallocation.IsNull() || dbAsset.IsNull())
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
    return true;
}

bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, UniValue &entry){
    CMintSyscoin mintsyscoin(tx);
    if (!mintsyscoin.IsNull() && !mintsyscoin.IsNull()) {
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
        entry.__pushKV("sender", "");
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
    if (!mintsyscoin.IsNull() && !mintsyscoin.IsNull()) {
        entry.__pushKV("txtype", tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT? "assetallocationmint": "syscoinmint");
        entry.__pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
       
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
        entry.__pushKV("asset_guid", mintsyscoin.assetAllocationTuple.nAsset);
        entry.__pushKV("symbol", dbAsset.strSymbol);
        entry.__pushKV("sender", "");
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
template <typename Stream, typename Operation>
void CAssetAllocation::SerializationOp(Stream& s, Operation ser_action) {
    READWRITE(listReceivingAssets);
}