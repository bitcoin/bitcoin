// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <boost/algorithm/string.hpp>
#include <services/assetconsensus.h>
#include <validationinterface.h>
#include <boost/thread.hpp>
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
extern CAmount AmountFromValue(const UniValue& value);
extern UniValue convertaddress(const JSONRPCRequest& request);
extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesMapImpl arrivalTimesMap;
extern CCriticalSection cs_assetallocationmempoolbalance;
extern CCriticalSection cs_assetallocationarrival;
extern std::vector<std::pair<uint256, uint32_t> >  vecToRemoveFromMempool;
extern CCriticalSection cs_assetallocationmempoolremovetx;

std::unique_ptr<CAssetDB> passetdb;
std::unique_ptr<CAssetAllocationDB> passetallocationdb;
std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;
std::unique_ptr<CAssetIndexDB> passetindexdb;


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
bool GetSyscoinBurnData(const CTransaction &tx, CAssetAllocation* theAssetAllocation, std::vector<unsigned char> &vchEthAddress, std::vector<unsigned char> &vchEthContract)
{   
    if(!theAssetAllocation) 
        return false;  
    uint32_t nAssetFromScript;
    CAmount nAmountFromScript;
    CWitnessAddress burnWitnessAddress;
    uint8_t nPrecision;
    if(!GetSyscoinBurnData(tx, nAssetFromScript, burnWitnessAddress, nAmountFromScript, vchEthAddress, nPrecision, vchEthContract)){
        return false;
    }
    theAssetAllocation->SetNull();
    theAssetAllocation->assetAllocationTuple.nAsset = nAssetFromScript;
    theAssetAllocation->assetAllocationTuple.witnessAddress = burnWitnessAddress;
    theAssetAllocation->listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(burnWitness.nVersion, burnWitness.vchWitnessProgram), nAmountFromScript));
    return true;

} 
bool GetSyscoinBurnData(const CTransaction &tx, uint32_t& nAssetFromScript, CWitnessAddress& burnWitnessAddress, CAmount &nAmountFromScript, std::vector<unsigned char> &vchEthAddress, uint8_t &nPrecision, std::vector<unsigned char> &vchEthContract)
{
    if(tx.nVersion != SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Invalid transaction version\n");
        return false;
    }
    int nOut = GetSyscoinDataOutput(tx);
    if (nOut == -1){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Data index must be positive\n");
        return false;
    }

    const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
    std::vector<std::vector< unsigned char> > vvchArgs;
    if(!GetSyscoinBurnData(scriptPubKey, vvchArgs)){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Cannot get burn data\n");
        return false;
    }
        
    if(vvchArgs.size() != 7){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Wrong argument size %d\n", vvchArgs.size());
        return false;
    }
          
    if(vvchArgs[0].size() != 4){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: nAssetFromScript - Wrong argument size %d\n", vvchArgs[0].size());
        return false;
    }
        
    nAssetFromScript  = static_cast<uint32_t>(vvchArgs[0][3]);
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][2]) << 8;
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][1]) << 16;
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][0]) << 24;
            
    if(vvchArgs[1].size() != 8){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: nAmountFromScript - Wrong argument size %d\n", vvchArgs[1].size());
        return false; 
    }
    uint64_t result = static_cast<uint64_t>(vvchArgs[1][7]);
    result |= static_cast<uint64_t>(vvchArgs[1][6]) << 8;
    result |= static_cast<uint64_t>(vvchArgs[1][5]) << 16;
    result |= static_cast<uint64_t>(vvchArgs[1][4]) << 24; 
    result |= static_cast<uint64_t>(vvchArgs[1][3]) << 32;  
    result |= static_cast<uint64_t>(vvchArgs[1][2]) << 40;  
    result |= static_cast<uint64_t>(vvchArgs[1][1]) << 48;  
    result |= static_cast<uint64_t>(vvchArgs[1][0]) << 56;   
    nAmountFromScript = (CAmount)result;
    
    if(vvchArgs[2].empty()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Ethereum address empty\n");
        return false; 
    }
    vchEthAddress = vvchArgs[2]; 

    nPrecision = static_cast<uint8_t>(vvchArgs[3][0]);

    if(vvchArgs[4].empty()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Ethereum contract empty\n");
        return false; 
    }
    vchEthContract = vvchArgs[4]; 

    if(vvchArgs[5].size() != 1){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Witness address version - Wrong argument size %d\n", vvchArgs[3].size());
        return false;
    }
    const unsigned char &nWitnessVersion = static_cast<unsigned char>(vvchArgs[5][0]);
    
    if(vvchArgs[6].empty()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Witness address empty\n");
        return false;
    }     
    
    burnWitnessAddress = CWitnessAddress(nWitnessVersion, vvchArgs[6]);   
    return true; 
}
bool GetSyscoinBurnData(const CScript &scriptPubKey, std::vector<std::vector<unsigned char> > &vchData)
{
    int nTotalSize = 0;
    CScript::const_iterator pc = scriptPubKey.begin();
    opcodetype opcode;
    if (!scriptPubKey.GetOp(pc, opcode))
        return false;
    if (opcode != OP_RETURN)
        return false;
    vector<unsigned char> vchArg;
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();
    vchData.push_back(vchArg);
    vchArg.clear();
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();
    vchData.push_back(vchArg);
    vchArg.clear();       
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();
    vchData.push_back(vchArg);
    vchArg.clear();        
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();
    vchData.push_back(vchArg);
    vchArg.clear();   
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();
    vchData.push_back(vchArg);
    vchArg.clear(); 
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();    
    vchData.push_back(vchArg);
    vchArg.clear();
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    nTotalSize += vchArg.size();    
    vchData.push_back(vchArg);
    vchArg.clear();
    if(pc != scriptPubKey.end()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData invalid data proceeding push data for burn elements...\n"); 
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
    case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
        return "assettransfer";
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
bool CAsset::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	int nOut;
	if (!IsAssetTx(tx.nVersion) || !GetSyscoinData(tx, vchData, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData))
	{	
		return false;
	}
    return true;
}
bool CMintSyscoin::UnserializeFromTx(const CTransaction &tx) {
    vector<unsigned char> vchData;
    int nOut;
    if (!IsSyscoinMintTx(tx.nVersion) || !GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    if(!UnserializeFromData(vchData))
    {   
        return false;
    }
    return true;
}

bool FlushSyscoinDBs() {
    bool ret = true;
	 {
        if (passetallocationmempooldb != nullptr)
        {
            ResyncAssetAllocationStates();
            {
                LOCK(cs_assetallocationmempoolbalance);
                LogPrintf("Flushing Asset Allocation Mempool Balances...size %d\n", mempoolMapAssetBalances.size());
                if(!passetallocationmempooldb->WriteAssetAllocationMempoolBalances(mempoolMapAssetBalances)){
                    LogPrintf("Failed to write to asset allocation mempool balance database!\n");
                    ret = false; 
                }
                mempoolMapAssetBalances.clear();
            }
            {
                LOCK(cs_assetallocationarrival);
                LogPrintf("Flushing Asset Allocation Arrival Times...size %d\n", arrivalTimesMap.size());
                if(!passetallocationmempooldb->WriteAssetAllocationMempoolArrivalTimes(arrivalTimesMap)){
                    LogPrintf("Failed to write to asset allocation mempool arrival time database!\n");
                    ret = false; 
                }
                arrivalTimesMap.clear();
            }
            {
                LOCK(cs_assetallocationmempoolremovetx);
                LogPrintf("Flushing Asset Allocation Mempool Removal Transactions...size %d\n", vecToRemoveFromMempool.size());
                if(!passetallocationmempooldb->WriteAssetAllocationMempoolToRemoveVector(vecToRemoveFromMempool)){
                    LogPrintf("Failed to write to asset allocation mempool to remove database!\n");
                    ret = false; 
                }
                vecToRemoveFromMempool.clear();
            }           
            if (!passetallocationmempooldb->Flush()) {
                LogPrintf("Failed to write to asset allocation mempool database!\n");
                ret = false;
            }            
        }
	 }
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
bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress &witnessAddressToMatch) {
	CTxDestination dest;
	int witnessversion;
	std::vector<unsigned char> witnessprogram;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Coin& prevCoins = inputs.AccessCoin(tx.vin[i].prevout);
		if (prevCoins.IsSpent() || prevCoins.IsCoinBase()) {
			continue;
		}
		if (prevCoins.out.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram) && witnessAddressToMatch.vchWitnessProgram == witnessprogram && witnessAddressToMatch.nVersion == (unsigned char)witnessversion)
			return true;
	}
	return false;
}
bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress &witnessAddressToMatch, const COutPoint& lockedOutpoint) {
    if (lockedOutpoint.IsNull()){
		return FindAssetOwnerInTx(inputs, tx, witnessAddressToMatch);
    }
	CTxDestination dest;
    int witnessversion;
    bool foundOutPoint = false;
    bool foundOwner = false;
    std::vector<unsigned char> witnessprogram;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Coin& prevCoins = inputs.AccessCoin(tx.vin[i].prevout);
		if (prevCoins.IsSpent() || prevCoins.IsCoinBase()) {
			continue;
		}
        if (lockedOutpoint == tx.vin[i].prevout){
            foundOutPoint = true;
        }
        if(prevCoins.out.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram) && witnessAddressToMatch.vchWitnessProgram == witnessprogram && witnessAddressToMatch.nVersion == (unsigned char)witnessversion){
            foundOwner = true;
        }
        if(foundOwner && foundOutPoint)
            return true;
	}
	return false;
}

bool IsSyscoinMintTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_MINT;
}
bool IsAssetTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE || nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER || nVersion == SYSCOIN_TX_VERSION_ASSET_SEND;
}
bool IsAssetAllocationTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN || nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION ||
        nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND || nVersion == SYSCOIN_TX_VERSION_ALLOCATION_LOCK;
}
bool IsSyscoinTx(const int &nVersion){
    return IsAssetTx(nVersion) || IsAssetAllocationTx(nVersion) || IsSyscoinMintTx(nVersion);
}
#ifdef ENABLE_WALLET
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output, CWallet* const pwallet, const isminefilter* filter_ismine){
    vector<vector<unsigned char> > vvch;
    bool found = false;
    if(IsSyscoinMintTx(rawTx.nVersion)){
        found = AssetMintTxToJson(rawTx, rawTx.GetHash(), output);
    }
    else if (IsAssetTx(rawTx.nVersion) || IsAssetAllocationTx(rawTx.nVersion)){
        found = SysTxToJSON(rawTx, output, pwallet, filter_ismine);
    }
    
    return found;
}
#endif
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output){
    vector<vector<unsigned char> > vvch;
    bool found = false;
    if(IsSyscoinMintTx(rawTx.nVersion)){
        found = AssetMintTxToJson(rawTx, rawTx.GetHash(), output);
    }
    else if (IsAssetTx(rawTx.nVersion) || IsAssetAllocationTx(rawTx.nVersion)){
        found = SysTxToJSON(rawTx, output);
    }
    
    return found;
}
// TODO cleanup, this is to support disable-wallet build make sure to wrap around ENABLE_WALLET
#ifdef ENABLE_WALLET
bool SysTxToJSON(const CTransaction& tx, UniValue& output, CWallet* const pwallet, const isminefilter* filter_ismine)
{
    bool found = false;
    if (IsAssetTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetTxToJSON(tx, output);
    else if (IsAssetAllocationTx(tx.nVersion) || tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetAllocationTxToJSON(tx, output, pwallet, filter_ismine);
    return found;
}
#endif
bool SysTxToJSON(const CTransaction& tx, UniValue& output)
{
    bool found = false;
    if (IsAssetTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetTxToJSON(tx, output);
    else if (IsAssetAllocationTx(tx.nVersion) || tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetAllocationTxToJSON(tx, output);
    return found;
}
int GenerateSyscoinGuid()
{
    int rand = 0;
    while(rand <= SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN)
	    rand = GetRand(std::numeric_limits<int>::max());
    return rand;
}

bool IsOutpointMature(const COutPoint& outpoint)
{
	Coin coin;
	GetUTXOCoin(outpoint, coin);
	if (coin.IsSpent() || coin.IsCoinBase())
		return false;
	int numConfirmationsNeeded = 0;
    {
        LOCK(cs_main);
        if (coin.nHeight > -1 && ::ChainActive().Tip())
            return (::ChainActive().Height() - coin.nHeight) >= numConfirmationsNeeded;
    }
	// don't have chainActive or coin height is neg 1 or less
	return false;

}
void CAsset::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
void CMintSyscoin::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsMint(SER_NETWORK, PROTOCOL_VERSION);
    dsMint << *this;
    vchData = vector<unsigned char>(dsMint.begin(), dsMint.end());

}
bool WriteAssetIndexTXID(const uint32_t& nAsset, const uint256& txid){
    uint32_t page;
    if(!passetindexdb->ReadAssetPage(nAsset, page)){
        page = 0;
        if(!passetindexdb->WriteAssetPage(nAsset, page)){
           LogPrint(BCLog::SYS, "Failed to write asset page\n");   
           return false; 
        }              
    }
    std::vector<uint256> TXIDS;
    passetindexdb->ReadIndexTXIDs(nAsset, page, TXIDS);
    // new page needed
    if(((int)TXIDS.size()) >= fAssetIndexPageSize){

        TXIDS.clear();
        page++;
        if(!passetindexdb->WriteAssetPage(nAsset, page)){
            LogPrint(BCLog::SYS, "Failed to write asset page\n");
            return false;
        }
    }
    TXIDS.push_back(txid);
    if(!passetindexdb->WriteIndexTXIDs(nAsset, page, TXIDS)){
        LogPrint(BCLog::SYS, "Failed to write asset index txids\n");
        return false;
    }
    return true;
}
bool CAssetDB::WriteAssetIndex(const CTransaction& tx, const uint256& txid, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash) {
	if (fZMQAsset || fAssetIndex) {
        if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(),dbAsset.nAsset) == fAssetIndexGuids.end()){
            LogPrint(BCLog::SYS, "Asset cannot be indexed because it is not set in -assetindexguids list\n");
            return true;
        }
		UniValue oName(UniValue::VOBJ);
        // assetsends write allocation indexes
        if(tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND && AssetTxToJSON(tx, nHeight, blockhash, oName)){
            if(fZMQAsset)
                GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assetrecord");
            if(fAssetIndex)
            {
                WriteAssetIndexTXID(dbAsset.nAsset, txid);
                if(!passetindexdb->WritePayload(txid, oName)){
                    LogPrint(BCLog::SYS, "Failed to write asset index payload\n");
                    return false;
                }
            }
        }
	}
    return true;
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
	oAsset.__pushKV("address", asset.witnessAddress.ToString());
    oAsset.__pushKV("contract", asset.vchContract.empty()? "" : "0x"+HexStr(asset.vchContract));
	oAsset.__pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));
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
    if(pblockindexdb->ReadBlockHash(txHash, blockhash)){ 
        LOCK(cs_main);
        blockindex = LookupBlockIndex(blockhash);
    }
    if(blockindex)
    {
        nHeight = blockindex->nHeight;
    }
        	

	entry.__pushKV("txtype", assetFromTx(tx.nVersion));
	entry.__pushKV("asset_guid", asset.nAsset);
    entry.__pushKV("symbol", asset.strSymbol);
    entry.__pushKV("txid", txHash.GetHex());
    entry.__pushKV("height", nHeight);
    
	if (!asset.vchPubData.empty())
		entry.__pushKV("public_value", stringFromVch(asset.vchPubData));

	if (!asset.vchContract.empty())
		entry.__pushKV("contract", "0x" + HexStr(asset.vchContract));

	if (!asset.witnessAddress.IsNull())
		entry.__pushKV("sender", asset.witnessAddress.ToString());

	if (!asset.witnessAddressTransfer.IsNull())
		entry.__pushKV("address_transfer", asset.witnessAddressTransfer.ToString());

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
bool AssetTxToJSON(const CTransaction& tx, const int& nHeight, const uint256& blockhash, UniValue &entry)
{
    CAsset asset(tx);
    if(asset.IsNull())
        return false;
    entry.__pushKV("txtype", assetFromTx(tx.nVersion));
    entry.__pushKV("asset_guid", asset.nAsset);
    entry.__pushKV("symbol", asset.strSymbol);
    entry.__pushKV("txid", tx.GetHash().GetHex());
    entry.__pushKV("height", nHeight);

    if(!asset.vchPubData.empty())
        entry.__pushKV("public_value", stringFromVch(asset.vchPubData));
        
    if(!asset.vchContract.empty())
        entry.__pushKV("contract", "0x"+HexStr(asset.vchContract));
        
    if(!asset.witnessAddress.IsNull())
        entry.__pushKV("sender", asset.witnessAddress.ToString());

    if(!asset.witnessAddressTransfer.IsNull())
        entry.__pushKV("address_transfer", asset.witnessAddressTransfer.ToString());

    if(asset.nUpdateFlags > 0)
        entry.__pushKV("update_flags", asset.nUpdateFlags);
              
    if (asset.nBalance > 0)
        entry.__pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));

    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE){
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
    if(fAssetIndex){
        for (const auto &key : mapAssets) {
            if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), key.first) == fAssetIndexGuids.end())
                continue;
            const string& witnessStr = key.second.witnessAddress.ToString();
            auto it = mapGuids.emplace(std::piecewise_construct,  std::forward_as_tuple(std::move(witnessStr)),  std::forward_as_tuple(std::move(emptyVec)));
            
            std::vector<uint32_t> &assetGuids = it.first->second;
            // if wasn't found and was added to the map
            if(it.second)
                ReadAssetsByAddress(key.second.witnessAddress, assetGuids);
            // erase asset address association
            if (key.second.IsNull()) {
                auto itVec = std::find(assetGuids.begin(), assetGuids.end(),  key.first);
                if(itVec != assetGuids.end()){
                    assetGuids.erase(itVec);  
                    // ensure we erase only the ones that are actually being cleared
                    if(assetGuids.empty())
                        assetGuids.emplace_back(0);
                }
            }
            else{
                // add asset address association
                auto itVec = std::find(assetGuids.begin(), assetGuids.end(),  key.first);
                if(itVec == assetGuids.end()){
                    // if we had the special erase flag we remove that and add the real guid
                    if(assetGuids.size() == 1 && assetGuids[0] == 0)
                        assetGuids.clear();
                    assetGuids.emplace_back(key.first);
                }
            }      
        }
    }
    for (const auto &key : mapAssets) {
		if (key.second.IsNull()) {
			erase++;
			batch.Erase(key.first);
		}
		else {
			write++;
			batch.Write(key.first, key.second);
		}
        if(fAssetIndex){
            if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(), fAssetIndexGuids.end(), key.first) == fAssetIndexGuids.end())
                continue;
            auto it = mapGuids.find(key.second.witnessAddress.ToString());
            if(it == mapGuids.end())
                continue;
            const std::vector<uint32_t>& assetGuids = it->second;
            // check for special clearing flag before batch erase
            if(assetGuids.size() == 1 && assetGuids[0] == 0)
                batch.Erase(key.second.witnessAddress);   
            else
                batch.Write(key.second.witnessAddress, assetGuids); 
            // we have processed this address so don't process again
            mapGuids.erase(it);        
        }
    }
    LogPrint(BCLog::SYS, "Flushing %d assets (erased %d, written %d)\n", mapAssets.size(), erase, write);
    return WriteBatch(batch);
}
bool CAssetDB::ScanAssets(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<CWitnessAddress > vecWitnessAddresses;
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

		const UniValue &owners = find_value(oOptions, "addresses");
		if (owners.isArray()) {
			const UniValue &ownersArray = owners.get_array();
			for (unsigned int i = 0; i < ownersArray.size(); i++) {
				const UniValue &owner = ownersArray[i].get_obj();
                 
				const UniValue &ownerStr = find_value(owner, "address");
				if (ownerStr.isStr())
					vecWitnessAddresses.push_back(DescribeWitnessAddress(ownerStr.get_str()));
			}
		}
	}
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
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
				if (!vecWitnessAddresses.empty() && std::find(vecWitnessAddresses.begin(), vecWitnessAddresses.end(), txPos.witnessAddress) == vecWitnessAddresses.end())
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
bool CAssetIndexDB::ScanAssetIndex(uint32_t page, const UniValue& oOptions, UniValue& oRes) {
    CAssetAllocationTuple assetTuple;
    uint32_t nAsset = 0;
    if (!oOptions.isNull()) {
        const UniValue &assetObj = find_value(oOptions, "asset_guid");
        if (assetObj.isNum()) {

            nAsset = assetObj.get_uint();
        }
        else{
            LogPrint(BCLog::SYS, "ScanAssetIndex: failed, asset guid is not a number\n");
            return false;
        }

        const UniValue &addressObj = find_value(oOptions, "address");
        if (addressObj.isStr()) {
            assetTuple = CAssetAllocationTuple(nAsset, DescribeWitnessAddress(addressObj.get_str()));
        }
    }
    else{
        LogPrint(BCLog::SYS, "ScanAssetIndex: failed, options are not found\n");
        return false;
    }
    vector<uint256> vecTX;
    uint32_t pageFound;
    bool scanAllocation = !assetTuple.IsNull();
    if(scanAllocation){
        if(!ReadAssetAllocationPage(nAsset, pageFound))
            return true;
    }
    else{
        if(!ReadAssetPage(nAsset, pageFound))
            return true;
    }
    if(pageFound < page){
        LogPrint(BCLog::SYS, "ScanAssetIndex: failed, no entries found in the page table pageFound %d vs %d page\n",pageFound, page);
        return false;
    }
    // order by highest page first
    page = pageFound - page;
    if(scanAllocation){
        if(!ReadIndexTXIDs(assetTuple, page, vecTX)){
            LogPrint(BCLog::SYS, "ScanAssetIndex: failed, cannot read TXIDs of allocation\n");
            return false;
        }
    }
    else{
        if(!ReadIndexTXIDs(nAsset, page, vecTX)){
            LogPrint(BCLog::SYS, "ScanAssetIndex: failed, cannot read TXIDs of asset\n");
            return false;
        }
    }
    for(const uint256& txid: vecTX){
        UniValue oObj(UniValue::VOBJ);
        if(!ReadPayload(txid, oObj))
            continue;
           
        oRes.push_back(oObj);
    }
    
    return true;
}
bool CAssetIndexDB::FlushErase(const std::vector<uint256> &vecTXIDs){
    if(vecTXIDs.empty() || !fAssetIndex)
        return true;

    CDBBatch batch(*this);
    for (const uint256 &txid : vecTXIDs) {
        // erase payload
        batch.Erase(txid);
    }
    LogPrint(BCLog::SYS, "Flushing %d asset index removals\n", vecTXIDs.size());
    return WriteBatch(batch);
}
CAmount getAuxFee(const std::string &public_data, const CAmount& nAmount, CWitnessAddress & address) {
    UniValue publicObj;
    if(!publicObj.read(public_data))
        return -1;
    const UniValue &auxFeesObj = find_value(publicObj, "aux_fees");
    if(!auxFeesObj.isObject())
        return -1;
    const UniValue &addressObj = find_value(auxFeesObj, "address");
    if(!addressObj.isStr())
        return -1;
    address = DescribeWitnessAddress(addressObj.get_str());
    const UniValue &feeStructObj = find_value(auxFeesObj, "fee_struct");
    if(!feeStructObj.isArray())
        return -1;
    const UniValue &feeStructArray = feeStructObj.get_array();
    if(feeStructArray.size() == 0)
        return -1;
     
    CAmount nAccumulatedFee = 0;
    CAmount nBoundAmount = 0;
    CAmount nNextBoundAmount = 0;
    double nRate = 0;
    for(unsigned int i =0;i<feeStructArray.size();i++){
        if(!feeStructArray[i].isArray())
            return -1;
        const UniValue &feeStruct = feeStructArray[i].get_array();
        const UniValue &feeStructNext = feeStructArray[i < feeStructArray.size()-1? i+1:i].get_array();
        if(!feeStruct[0].isStr() && !feeStruct[0].isNum())
            return -1;
        if(!feeStructNext[0].isStr() && !feeStructNext[0].isNum())
                return -1;    
        nBoundAmount = AmountFromValue(feeStruct[0]);
        nNextBoundAmount = AmountFromValue(feeStructNext[0]);
        if(!feeStruct[1].isStr())
            return -1;
        if(!ParseDouble(feeStruct[1].get_str(), &nRate))
            return -1;
        // case where amount is in between the bounds
        if(nAmount >= nBoundAmount && nAmount < nNextBoundAmount){
            return (nAmount - nBoundAmount) * nRate + nAccumulatedFee;    
        }
        nBoundAmount = nNextBoundAmount - nBoundAmount;
        nAccumulatedFee += (nBoundAmount * nRate);
    }
    return (nAmount - nBoundAmount) * nRate + nAccumulatedFee;    
}

