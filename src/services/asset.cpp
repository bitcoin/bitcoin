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
extern ArrivalTimesSetImpl arrivalTimesSet;
extern RecursiveMutex cs_assetallocationmempoolbalance;
extern RecursiveMutex cs_assetallocationarrival;
extern ArrivalTimesSet setToRemoveFromMempool;
extern RecursiveMutex cs_assetallocationmempoolremovetx;

std::unique_ptr<CAssetDB> passetdb;
std::unique_ptr<CAssetAllocationDB> passetallocationdb;
std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;

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
                LogPrintf("Flushing Asset Allocation Arrival Times...size %d\n", arrivalTimesSet.size());
                if(!passetallocationmempooldb->WriteAssetAllocationMempoolArrivalTimes(arrivalTimesSet)){
                    LogPrintf("Failed to write to asset allocation mempool arrival time database!\n");
                    ret = false; 
                }
                arrivalTimesSet.clear();
            }
            {
                LOCK(cs_assetallocationmempoolremovetx);
                LogPrintf("Flushing Asset Allocation Mempool Removal Transactions...size %d\n", setToRemoveFromMempool.size());
                if(!passetallocationmempooldb->WriteAssetAllocationMempoolToRemoveSet(setToRemoveFromMempool)){
                    LogPrintf("Failed to write to asset allocation mempool to remove database!\n");
                    ret = false; 
                }
                setToRemoveFromMempool.clear();
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
    if(tx.vin.empty())
        return false;
	CTxDestination dest;
    int witnessversion;
    bool foundOutPoint = false;
    bool foundOwner = false;
    std::vector<unsigned char> witnessprogram;
    const Coin& prevCoins = inputs.AccessCoin(tx.vin[0].prevout);
    if (prevCoins.IsSpent() || prevCoins.IsCoinBase()) {
        return false;
    }
    if (lockedOutpoint == tx.vin[0].prevout){
        foundOutPoint = true;
    }
    if(prevCoins.out.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram) && witnessAddressToMatch.vchWitnessProgram == witnessprogram && witnessAddressToMatch.nVersion == (unsigned char)witnessversion){
        foundOwner = true;
    }
    if(foundOwner && foundOutPoint)
        return true;
	
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
