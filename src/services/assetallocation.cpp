// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <validation.h>
#include <txmempool.h>
#include <core_io.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif // ENABLE_WALLET
#include <wallet/rpcwallet.h>
#include <chainparams.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <key_io.h>
#include <future>
#include <rpc/util.h>
#include <services/assetconsensus.h>
CCriticalSection cs_assetallocation;
CCriticalSection cs_assetallocationarrival;
// SYSCOIN service rpc functions
extern UniValue sendrawtransaction(const JSONRPCRequest& request);

UniValue assetallocationlock(const JSONRPCRequest& request);
UniValue assetallocationsend(const JSONRPCRequest& request);
UniValue assetallocationsendmany(const JSONRPCRequest& request);
UniValue assetallocationmint(const JSONRPCRequest& request);
UniValue assetallocationburn(const JSONRPCRequest& request);
UniValue assetallocationinfo(const JSONRPCRequest& request);
UniValue assetallocationbalance(const JSONRPCRequest& request);
UniValue assetallocationsenderstatus(const JSONRPCRequest& request);
UniValue listassetallocations(const JSONRPCRequest& request);
UniValue tpstestinfo(const JSONRPCRequest& request);
UniValue tpstestadd(const JSONRPCRequest& request);
UniValue tpstestsetenabled(const JSONRPCRequest& request);
UniValue listassetallocationmempoolbalances(const JSONRPCRequest& request);

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
	return boost::lexical_cast<string>(nAsset) + "-" + witnessAddress.ToString();
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

UniValue tpstestinfo(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 0 != params.size())
		throw runtime_error("tpstestinfo\n"
			"Gets TPS Test information for receivers of assetallocation transfers\n");
	if(!fTPSTest)
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("This function requires tpstest configuration to be set upon startup. Please shutdown and enable it by adding it to your syscoin.conf file and then call 'tpstestsetenabled true'."));
	
	UniValue oTPSTestResults(UniValue::VOBJ);
	UniValue oTPSTestReceivers(UniValue::VARR);
	UniValue oTPSTestReceiversMempool(UniValue::VARR);
	oTPSTestResults.pushKV("enabled", fTPSTestEnabled);
    oTPSTestResults.pushKV("testinitiatetime", (int64_t)nTPSTestingStartTime);
	oTPSTestResults.pushKV("teststarttime", (int64_t)nTPSTestingSendRawEndTime);
   
	for (auto &receivedTime : vecTPSTestReceivedTimesMempool) {
		UniValue oTPSTestStatusObj(UniValue::VOBJ);
		oTPSTestStatusObj.pushKV("txid", receivedTime.first.GetHex());
		oTPSTestStatusObj.pushKV("time", receivedTime.second);
		oTPSTestReceiversMempool.push_back(oTPSTestStatusObj);
	}
	oTPSTestResults.pushKV("receivers", oTPSTestReceiversMempool);
	return oTPSTestResults;
}
UniValue tpstestsetenabled(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 1 != params.size())
		throw runtime_error("tpstestsetenabled [enabled]\n"
			"\nSet TPS Test to enabled/disabled state. Must have -tpstest configuration set to make this call.\n"
			"\nArguments:\n"
			"1. enabled                  (boolean, required) TPS Test enabled state. Set to true for enabled and false for disabled.\n"
			"\nExample:\n"
			+ HelpExampleCli("tpstestsetenabled", "true"));
	if(!fTPSTest)
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("This function requires tpstest configuration to be set upon startup. Please shutdown and enable it by adding it to your syscoin.conf file and then try again."));
	fTPSTestEnabled = params[0].get_bool();
	if (!fTPSTestEnabled) {
		vecTPSTestReceivedTimesMempool.clear();
		nTPSTestingSendRawEndTime = 0;
		nTPSTestingStartTime = 0;
	}
	UniValue result(UniValue::VOBJ);
	result.pushKV("status", "success");
	return result;
}
UniValue tpstestadd(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 1 > params.size() || params.size() > 2)
		throw runtime_error("tpstestadd [starttime] [{\"tx\":\"hex\"},...]\n"
			"\nAdds raw transactions to the test raw tx queue to be sent to the network at starttime.\n"
			"\nArguments:\n"
			"1. starttime                  (numeric, required) Unix epoch time in micro seconds for when to send the raw transaction queue to the network. If set to 0, will not send transactions until you call this function again with a defined starttime.\n"
			"2. \"raw transactions\"                (array, not-required) A json array of signed raw transaction strings\n"
			"     [\n"
			"       {\n"
			"         \"tx\":\"hex\",    (string, required) The transaction hex\n"
			"       } \n"
			"       ,...\n"
			"     ]\n"
			"\nExample:\n"
			+ HelpExampleCli("tpstestadd", "\"223233433839384\" \"[{\\\"tx\\\":\\\"first raw hex tx\\\"},{\\\"tx\\\":\\\"second raw hex tx\\\"}]\""));
	if (!fTPSTest)
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("This function requires tpstest configuration to be set upon startup. Please shutdown and enable it by adding it to your syscoin.conf file and then call 'tpstestsetenabled true'."));

	bool bFirstTime = vecTPSRawTransactions.empty();
	nTPSTestingStartTime = params[0].get_int64();
	UniValue txs;
	if(params.size() > 1)
		txs = params[1].get_array();
	if (fTPSTestEnabled) {
		for (unsigned int idx = 0; idx < txs.size(); idx++) {
			const UniValue& tx = txs[idx];
			UniValue paramsRawTx(UniValue::VARR);
			paramsRawTx.push_back(find_value(tx.get_obj(), "tx").get_str());

			JSONRPCRequest request;
			request.params = paramsRawTx;
			vecTPSRawTransactions.push_back(request);
		}
		if (bFirstTime) {
			// define a task for the worker to process
			std::packaged_task<void()> task([]() {
				while (nTPSTestingStartTime <= 0 || GetTimeMicros() < nTPSTestingStartTime) {
					MilliSleep(0);
				}
				nTPSTestingSendRawStartTime = nTPSTestingStartTime;

				for (auto &txReq : vecTPSRawTransactions) {
					sendrawtransaction(txReq);
				}
			});
			bool isThreadPosted = false;
			for (int numTries = 1; numTries <= 50; numTries++)
			{
				// send task to threadpool pointer from init.cpp
				isThreadPosted = threadpool->tryPost(task);
				if (isThreadPosted)
				{
					break;
				}
				MilliSleep(10);
			}
			if (!isThreadPosted)
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("thread pool queue is full"));
		}
	}
	UniValue result(UniValue::VOBJ);
	result.pushKV("status", "success");
	return result;
}
template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(width) << std::hex << (val|0);
    return ss.str();
}
UniValue assetallocationburn(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 4 != params.size())
		throw runtime_error(
			"assetallocationburn [asset_guid] [address] [amount] [ethereum_destination_address]\n"
			"<asset_guid> Asset guid.\n"
			"<address> Address that owns this asset allocation.\n"
			"<amount> Amount of asset to burn to SYSX.\n"
            "<ethereum_destination_address> The 20 byte (40 character) hex string of the ethereum destination address. Leave empty to burn as normal without the bridge. If it is left empty this will process as a normal assetallocationsend to the burn address.\n");

	const int &nAsset = params[0].get_int();
	string strAddress = params[1].get_str();
    
	const CTxDestination &addressFrom = DecodeDestination(strAddress);

    
    UniValue detail = DescribeAddress(addressFrom);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();            	
	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));
    {
        LOCK(cs_assetallocationarrival);
        // check to see if a transaction for this asset/address tuple has arrived before minimum latency period
        const ArrivalTimesMap &arrivalTimes = arrivalTimesMap[assetAllocationTuple.ToString()];
        const int64_t & nNow = GetTimeMillis();
        int minLatency = ZDAG_MINIMUM_LATENCY_SECONDS * 1000;
        if (fUnitTest)
            minLatency = 1000;
        for (auto& arrivalTime : arrivalTimes) {
            // if this tx arrived within the minimum latency period flag it as potentially conflicting
            if ((nNow - arrivalTime.second) < minLatency) {
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1503 - " + _("Please wait a few more seconds and try again..."));
            }
        }
    }
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));
        
    UniValue amountObj = params[2];
	CAmount amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
	string ethAddress = params[3].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist
    // if no eth address provided just send as a std asset allocation send but to burn address
    if(ethAddress.empty()){
        UniValue output(UniValue::VARR);
        UniValue outputObj(UniValue::VOBJ);
        outputObj.pushKV("address", "burn");
        outputObj.pushKV("amount", ValueFromAssetAmount(amount, theAsset.nPrecision));
        output.push_back(outputObj);
        UniValue paramsFund(UniValue::VARR);
        paramsFund.push_back(nAsset);
        paramsFund.push_back(strAddress);
        paramsFund.push_back(output);
        paramsFund.push_back("");
        JSONRPCRequest requestMany;
        requestMany.params = paramsFund;
        return assetallocationsendmany(requestMany);  
    }
    // convert to hex string because otherwise cscript will push a cscriptnum which is 4 bytes but we want 8 byte hex representation of an int64 pushed
    const std::string amountHex = int_to_hex(amount);
    const std::string witnessVersionHex = int_to_hex(assetAllocationTuple.witnessAddress.nVersion);
    const std::string assetHex = int_to_hex(nAsset);

    
	vector<CRecipient> vecSend;


	CScript scriptData;
	scriptData << OP_RETURN << ParseHex(assetHex) << ParseHex(amountHex) << ParseHex(ethAddress) << ParseHex(witnessVersionHex) << assetAllocationTuple.witnessAddress.vchWitnessProgram;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(strAddress, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN, "", vecSend);
}
UniValue assetallocationmint(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 9 != params.size())
        throw runtime_error(
            "assetallocationmint [asset_guid] [address] [amount] [blocknumber] [tx_hex] [txroot_hex] [txmerkleproof_hex] [txmerkleroofpath_hex] [witness]\n"
            "<asset_guid> Asset guid.\n"
            "<address> Address that will get this minting.\n"
            "<amount> Amount of asset to mint. Note that fees will be taken from the owner address.\n"
            "<blocknumber> Block number of the block that included the burn transaction on Ethereum.\n"
            "<tx_hex> Raw transaction hex of the burn transaction on Ethereum.\n"
            "<txroot_hex> The transaction merkle root that commits this transaction to the block header.\n"
            "<txmerkleproof_hex> The list of parent nodes of the Merkle Patricia Tree for SPV proof.\n"
            "<txmerkleroofpath_hex> The merkle path to walk through the tree to recreate the merkle root.\n"
            "<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");

    const int &nAsset = params[0].get_int();
    string strAddress = params[1].get_str();
    CAmount nAmount = AmountFromValue(params[2]);
    uint32_t nBlockNumber = (uint32_t)params[3].get_int();
    
    string vchValue = params[4].get_str();
    string vchTxRoot = params[5].get_str();
    string vchParentNodes = params[6].get_str();
    string vchPath = params[7].get_str();
    string strWitness = params[8].get_str();
    if(!fGethSynced){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Geth is not synced, please wait until it syncs up and try again"));
    }
    const CTxDestination &dest = DecodeDestination(strAddress);
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
    vector<CRecipient> vecSend;
    
    CMintSyscoin mintSyscoin;
    mintSyscoin.assetAllocationTuple = CAssetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
    mintSyscoin.nValueAsset = nAmount;
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.vchValue = ParseHex(vchValue);
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchParentNodes = ParseHex(vchParentNodes);
    mintSyscoin.vchPath = ParseHex(vchPath);
    
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
       
    return syscointxfund_helper(strAddress, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT, strWitness, vecSend);
}

UniValue assetallocationsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 4)
        throw runtime_error(
            "assetallocationsend [asset_guid] [addressfrom] [addressTo] [amount]\n"
            "Send an asset allocation you own to another address.\n"
            "<asset_guid> Asset guid.\n"
            "<addressfrom> Address that owns this asset allocation.\n"
            "<addressTo> Address to transfer to.\n"
            "<amount> Quantity of asset to send.\n");
            
    UniValue output(UniValue::VARR);
    UniValue outputObj(UniValue::VOBJ);
    outputObj.pushKV("address", params[2].get_str());
    outputObj.pushKV("amount", params[3].get_str());
    output.push_back(outputObj);
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(params[0].get_int());
    paramsFund.push_back(params[1].get_str());
    paramsFund.push_back(output);
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    return assetallocationsendmany(requestMany);          
}

UniValue assetallocationsendmany(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 4)
		throw runtime_error(
			"assetallocationsend [asset_guid] [addressfrom] ([{\"address\":\"string\",\"amount\":amount},...] [witness]\n"
			"Send an asset allocation you own to another address. Maximum recipients is 250.\n"
			"<asset_guid> Asset guid.\n"
			"<addressfrom> Address that owns this asset allocation.\n"
			"<address> Address to transfer to.\n"
			"<amount> Quantity of asset to send.\n"
			"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");

	// gather & validate inputs
	const int &nAsset = params[0].get_int();
	string vchAddressFrom = params[1].get_str();
	UniValue valueTo = params[2];
	vector<unsigned char> vchWitness;
    string strWitness = params[3].get_str();
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");
	string strAddressFrom;
	const string &strAddress = vchAddressFrom;
    CTxDestination addressFrom;
    string witnessProgramHex;
    unsigned char witnessVersion = 0;
    if(strAddress != "burn"){
	    addressFrom = DecodeDestination(strAddress);
    	if (IsValidDestination(addressFrom)) {
    		strAddressFrom = strAddress;
    	
            UniValue detail = DescribeAddress(addressFrom);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str(); 
            witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
        }  
    }
    
	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddress == "burn"? vchFromString("burn"): ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));

	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));

	theAssetAllocation.SetNull();
    theAssetAllocation.assetAllocationTuple.nAsset = std::move(assetAllocationTuple.nAsset);
    theAssetAllocation.assetAllocationTuple.witnessAddress = std::move(assetAllocationTuple.witnessAddress); 
	UniValue receivers = valueTo.get_array();
	
	for (unsigned int idx = 0; idx < receivers.size(); idx++) {
		const UniValue& receiver = receivers[idx];
		if (!receiver.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"address'\" or \"amount\"}");

		const UniValue &receiverObj = receiver.get_obj();
        const std::string &toStr = find_value(receiverObj, "address").get_str();
        CWitnessAddress recpt;
        if(toStr != "burn"){
            CTxDestination dest = DecodeDestination(toStr);
            if(!IsValidDestination(dest))
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2509 - " + _("Asset must be sent to a valid syscoin address"));

            UniValue detail = DescribeAddress(dest);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
            unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
            recpt.vchWitnessProgram = ParseHex(witnessProgramHex);
            recpt.nVersion = witnessVersion;
        } 
        else{
            recpt.vchWitnessProgram = vchFromString("burn");
            recpt.nVersion = 0;
        }  
		UniValue amountObj = find_value(receiverObj, "amount");
		if (amountObj.isNum() || amountObj.isStr()) {
			const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(recpt.nVersion, recpt.vchWitnessProgram), amount));
		}
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in receiver array");

	}
    
    {
        LOCK(cs_assetallocationarrival);
    	// check to see if a transaction for this asset/address tuple has arrived before minimum latency period
    	const ArrivalTimesMap &arrivalTimes = arrivalTimesMap[assetAllocationTuple.ToString()];
    	const int64_t & nNow = GetTimeMillis();
    	int minLatency = ZDAG_MINIMUM_LATENCY_SECONDS * 1000;
    	if (fUnitTest)
    		minLatency = 1000;
    	for (auto& arrivalTime : arrivalTimes) {
    		// if this tx arrived within the minimum latency period flag it as potentially conflicting
    		if ((nNow - arrivalTime.second) < minLatency) {
    			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1503 - " + _("Please wait a few more seconds and try again..."));
    		}
    	}
    }

	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);   


	// send the asset pay txn
	vector<CRecipient> vecSend;
	
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(vchAddressFrom, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND, strWitness, vecSend);
}

UniValue assetallocationlock(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 5)
		throw runtime_error(
			"assetallocationlock [asset_guid] [addressfrom] [txid] [output_index] [witness]\n"
			"Lock an asset allocation to a specific UTXO (txid/output). This is useful for things such as hashlock and CLTV type operations where script checks are done on UTXO prior to spending which extend to an assetallocationsend.\n"
			"<asset_guid> Asset guid.\n"
			"<addressfrom> Address that owns this asset allocation.\n"
			"<txid> Transaction hash.\n"
			"<output_index> Output index inside the transaction output array.\n"
			"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");

	// gather & validate inputs
	const int &nAsset = params[0].get_int();
	string vchAddressFrom = params[1].get_str();
	uint256 txid = uint256S(params[2].get_str());
	int outputIndex = params[3].get_int();
	vector<unsigned char> vchWitness;
	string strWitness = params[4].get_str();

	string strAddressFrom;
	const string &strAddress = vchAddressFrom;
	CTxDestination addressFrom;
	string witnessProgramHex;
	unsigned char witnessVersion = 0;
	if (strAddress != "burn") {
		addressFrom = DecodeDestination(strAddress);
		if (IsValidDestination(addressFrom)) {
			strAddressFrom = strAddress;

			UniValue detail = DescribeAddress(addressFrom);
			if (find_value(detail.get_obj(), "iswitness").get_bool() == false)
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
			witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
			witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
		}
	}

	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddress == "burn" ? vchFromString("burn") : ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));

	theAssetAllocation.SetNull();
	theAssetAllocation.assetAllocationTuple.nAsset = std::move(assetAllocationTuple.nAsset);
	theAssetAllocation.assetAllocationTuple.witnessAddress = std::move(assetAllocationTuple.witnessAddress);
	theAssetAllocation.lockedOutpoint = COutPoint(txid, outputIndex);
	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);

	// send the asset pay txn
	vector<CRecipient> vecSend;

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(vchAddressFrom, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK, strWitness, vecSend);
}
UniValue assetallocationbalance(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error("assetallocationbalance <asset_guid> <address>\n"
                "Show stored balance of a single asset allocation.\n");

    const int &nAsset = params[0].get_int();
    string strAddressFrom = params[1].get_str();
    string witnessProgramHex = "";
    unsigned char witnessVersion = 0;
    if(strAddressFrom != "burn"){
        const CTxDestination &dest = DecodeDestination(strAddressFrom);
        UniValue detail = DescribeAddress(dest);
        if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
            throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
        witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
        witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
    }
    UniValue oAssetAllocation(UniValue::VOBJ);
    const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddressFrom == "burn"? vchFromString("burn"): ParseHex(witnessProgramHex)));
    CAssetAllocation txPos;
    if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1507 - " + _("Failed to read from assetallocation DB"));

    CAsset theAsset;
    if (!GetAsset(nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1508 - " + _("Could not find a asset with this key"));

        
    return ValueFromAssetAmount(txPos.nBalance, theAsset.nPrecision);
}

UniValue assetallocationinfo(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error("assetallocationinfo <asset_guid> <address>\n"
                "Show stored values of a single asset allocation.\n");

    const int &nAsset = params[0].get_int();
    string strAddressFrom = params[1].get_str();
    string witnessProgramHex = "";
    unsigned char witnessVersion = 0;
    if(strAddressFrom != "burn"){
        const CTxDestination &dest = DecodeDestination(strAddressFrom);
        UniValue detail = DescribeAddress(dest);
        if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
            throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
        witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
        witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
    }
    UniValue oAssetAllocation(UniValue::VOBJ);
    const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddressFrom == "burn"? vchFromString("burn"): ParseHex(witnessProgramHex)));
    CAssetAllocation txPos;
    if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1507 - " + _("Failed to read from assetallocation DB"));


	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1508 - " + _("Could not find a asset with this key"));


	if(!BuildAssetAllocationJson(txPos, theAsset, oAssetAllocation))
		oAssetAllocation.clear();
    return oAssetAllocation;
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
UniValue assetallocationsenderstatus(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 3 != params.size())
		throw runtime_error("assetallocationsenderstatus <asset_guid> <address> <txid>\n"
			"Show status as it pertains to any current Z-DAG conflicts or warnings related to a sender or sender/txid combination of an asset allocation transfer. Leave txid empty if you are not checking for a specific transfer.\n"
			"Return value is in the status field and can represent 3 levels(0, 1 or 2)\n"
			"Level -1 means not found, not a ZDAG transaction, perhaps it is already confirmed.\n"
			"Level 0 means OK.\n"
			"Level 1 means warning (checked that in the mempool there are more spending balances than current POW sender balance). An active stance should be taken and perhaps a deeper analysis as to potential conflicts related to the sender.\n"
			"Level 2 means an active double spend was found and any depending asset allocation sends are also flagged as dangerous and should wait for POW confirmation before proceeding.\n");

	const int &nAsset = params[0].get_int();
	string strAddressSender = params[1].get_str();

    
    const CTxDestination &dest = DecodeDestination(strAddressSender);
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
	uint256 txid;
	txid.SetNull();
	if(!params[2].get_str().empty())
		txid.SetHex(params[2].get_str());
	UniValue oAssetAllocationStatus(UniValue::VOBJ);
	const CAssetAllocationTuple assetAllocationTupleSender(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
    {
        LOCK2(cs_main, mempool.cs);
        ResetAssetAllocation(assetAllocationTupleSender.ToString(), txid, false, true);
        
    	int nStatus = ZDAG_STATUS_OK;
    	if (assetAllocationConflicts.find(assetAllocationTupleSender.ToString()) != assetAllocationConflicts.end())
    		nStatus = ZDAG_MAJOR_CONFLICT;
    	else
    		nStatus = DetectPotentialAssetAllocationSenderConflicts(assetAllocationTupleSender, txid);
    	
        oAssetAllocationStatus.pushKV("status", nStatus);
    }
	return oAssetAllocationStatus;
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
    oAssetAllocation.pushKV("asset_allocation", allocationTupleStr);
	oAssetAllocation.pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
	oAssetAllocation.pushKV("address",  assetallocation.assetAllocationTuple.witnessAddress.ToString());
	oAssetAllocation.pushKV("balance", ValueFromAssetAmount(assetallocation.nBalance, asset.nPrecision));
    oAssetAllocation.pushKV("balance_zdag", ValueFromAssetAmount(nBalanceZDAG, asset.nPrecision));
	return true;
}

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
    if(pblockindexdb->ReadBlockHash(txHash, blockhash))  
        blockindex = LookupBlockIndex(blockhash);
    if(blockindex)
    {
        nHeight = blockindex->nHeight;
    }
    entry.pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.pushKV("asset_allocation", assetallocation.assetAllocationTuple.ToString());
    entry.pushKV("txid", txHash.GetHex());
    entry.pushKV("height", nHeight);
    entry.pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
    entry.pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;  
    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.pushKV("address", strReceiver);
            oAssetAllocationReceiversObj.pushKV("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);          
        }
    }
    entry.pushKV("allocations", oAssetAllocationReceiversArray);
    entry.pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
         entry.pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
	else if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK)
		entry.pushKV("locked_outpoint", assetallocation.lockedOutpoint.ToStringShort());
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
    entry.pushKV("txtype", assetAllocationFromTx(tx.nVersion));
    entry.pushKV("asset_allocation", assetallocation.assetAllocationTuple.ToString());
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);
    entry.pushKV("asset_guid", (int)assetallocation.assetAllocationTuple.nAsset);
    entry.pushKV("sender", assetallocation.assetAllocationTuple.witnessAddress.ToString());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    CAmount nTotal = 0;

    if (!assetallocation.listSendingAllocationAmounts.empty()) {
        for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
            nTotal += amountTuple.second;
            const string& strReceiver = amountTuple.first.ToString();
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            oAssetAllocationReceiversObj.pushKV("address", strReceiver);
            oAssetAllocationReceiversObj.pushKV("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);          
        }
    }
    entry.pushKV("allocations", oAssetAllocationReceiversArray);
    entry.pushKV("total", ValueFromAssetAmount(nTotal, dbAsset.nPrecision));
    entry.pushKV("blockhash", blockhash.GetHex()); 
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN)
         entry.pushKV("ethereum_destination", "0x" + HexStr(vchEthAddress));
    return true;
}

bool AssetMintTxToJson(const CTransaction& tx, UniValue &entry){
    CMintSyscoin mintsyscoin(tx);
    if (!mintsyscoin.IsNull() && !mintsyscoin.assetAllocationTuple.IsNull()) {
        int nHeight = 0;
        const uint256& txHash = tx.GetHash();
        CBlockIndex* blockindex = nullptr;
        uint256 blockhash;
        if(pblockindexdb->ReadBlockHash(txHash, blockhash))
            blockindex = LookupBlockIndex(blockhash);
        if(blockindex)
        {
            nHeight = blockindex->nHeight;
        }
        entry.pushKV("txtype", "assetallocationmint");
        entry.pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
        entry.pushKV("txid", txHash.GetHex());
        entry.pushKV("height", nHeight);
        entry.pushKV("asset_guid", (int)mintsyscoin.assetAllocationTuple.nAsset);
        entry.pushKV("sender", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
       
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        oAssetAllocationReceiversObj.pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        oAssetAllocationReceiversObj.pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    
        entry.pushKV("allocations", oAssetAllocationReceiversArray); 
        entry.pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        entry.pushKV("blockhash", blockhash.GetHex());   
        UniValue oSPVProofObj(UniValue::VOBJ);
        oSPVProofObj.pushKV("value", HexStr(mintsyscoin.vchValue));   
        oSPVProofObj.pushKV("parentnodes", HexStr(mintsyscoin.vchParentNodes)); 
        oSPVProofObj.pushKV("txroot", HexStr(mintsyscoin.vchTxRoot));
        oSPVProofObj.pushKV("path", HexStr(mintsyscoin.vchPath));  
        oSPVProofObj.pushKV("ethblocknumber", (int)mintsyscoin.nBlockNumber); 
        entry.pushKV("spv_proof", oSPVProofObj); 
        return true;                                        
    } 
    return false;                   
}
bool AssetMintTxToJson(const CTransaction& tx, const CMintSyscoin& mintsyscoin, const int& nHeight, const uint256& blockhash, UniValue &entry){
    if (!mintsyscoin.IsNull() && !mintsyscoin.assetAllocationTuple.IsNull()) {
        entry.pushKV("txtype", "assetallocationmint");
        entry.pushKV("asset_allocation", mintsyscoin.assetAllocationTuple.ToString());
        entry.pushKV("txid", tx.GetHash().GetHex());
        entry.pushKV("height", nHeight);       
        entry.pushKV("asset_guid", (int)mintsyscoin.assetAllocationTuple.nAsset);
        entry.pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        CAsset dbAsset;
        GetAsset(mintsyscoin.assetAllocationTuple.nAsset, dbAsset);
       
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        oAssetAllocationReceiversObj.pushKV("address", mintsyscoin.assetAllocationTuple.witnessAddress.ToString());
        oAssetAllocationReceiversObj.pushKV("amount", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
        
        entry.pushKV("allocations", oAssetAllocationReceiversArray);
        entry.pushKV("total", ValueFromAssetAmount(mintsyscoin.nValueAsset, dbAsset.nPrecision));
        entry.pushKV("blockhash", blockhash.GetHex()); 
        UniValue oSPVProofObj(UniValue::VOBJ);
        oSPVProofObj.pushKV("value", HexStr(mintsyscoin.vchValue));   
        oSPVProofObj.pushKV("parentnodes", HexStr(mintsyscoin.vchParentNodes)); 
        oSPVProofObj.pushKV("txroot", HexStr(mintsyscoin.vchTxRoot));
        oSPVProofObj.pushKV("path", HexStr(mintsyscoin.vchPath));  
        oSPVProofObj.pushKV("ethblocknumber", (int)mintsyscoin.nBlockNumber); 
        entry.pushKV("spv_proof", oSPVProofObj); 
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
            resultObj.pushKV(indexObj.first, ValueFromAmount(indexObj.second));
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
    for (const auto &key : mapAssetAllocations) {
        if(key.second.nBalance <= 0){
            batch.Erase(key.second.assetAllocationTuple);
        }
        else{
            batch.Write(key.second.assetAllocationTuple, key.second);
        }
    }
    LogPrint(BCLog::SYS, "Flushing %d asset allocations\n", mapAssetAllocations.size());
    return WriteBatch(batch);
}
bool CAssetAllocationDB::ScanAssetAllocations(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<CWitnessAddress> vecWitnessAddresses;
	uint32_t nAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &assetObj = find_value(oOptions, "asset_guid");
		if(assetObj.isNum()) {
			nAsset = boost::lexical_cast<uint32_t>(assetObj.get_int());
		}

		const UniValue &owners = find_value(oOptions, "addresses");
		if (owners.isArray()) {
			const UniValue &ownersArray = owners.get_array();
			for (unsigned int i = 0; i < ownersArray.size(); i++) {
				const UniValue &owner = ownersArray[i].get_obj();
				const UniValue &ownerStr = find_value(owner, "address");
				if (ownerStr.isStr()) {
                    const CTxDestination &dest = DecodeDestination(ownerStr.get_str());
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
			if (pcursor->GetKey(key) && (nAsset == 0 || nAsset == key.nAsset)) {
				pcursor->GetValue(txPos);              
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

UniValue listassetallocations(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 3 < params.size())
		throw runtime_error("listassetallocations [count] [from] [{options}]\n"
			"scan through all asset allocations.\n"
			"[count]          (numeric, optional, default=10) The number of results to return.\n"
			"[from]           (numeric, optional, default=0) The number of results to skip.\n"
			"[options]        (array, optional) A json object with options to filter results\n"
			"    {\n"
			"	   \"asset_guid\":guid			    (number) Asset GUID to filter.\n"
			"	   \"addresses\"					(array) a json array with addresses owning allocations\n"
			"		[\n"
			"			{\n"
			"				\"address\":string		(string) Address to filter.\n"
			"			} \n"
			"			,...\n"
			"		]\n"
			"    }\n"
			+ HelpExampleCli("listassetallocations", "0")
			+ HelpExampleCli("listassetallocations", "10 10")
			+ HelpExampleCli("listassetallocations", "0 0 '{\"asset_guid\":92922}'")
			+ HelpExampleCli("listassetallocations", "0 0 '{\"addresses\":[{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"},{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"}]}'")
		);
	UniValue options;
	int count = 10;
	int from = 0;
	if (params.size() > 0) {
		count = params[0].get_int();
		if (count == 0) {
			count = 10;
		} else
		if (count < 0) {
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("'count' must be 0 or greater"));
		}
	}
	if (params.size() > 1) {
		from = params[1].get_int();
		if (from < 0) {
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("'from' must be 0 or greater"));
		}
	}
	if (params.size() > 2) {
		options = params[2];
	}
	UniValue oRes(UniValue::VARR);
	if (!passetallocationdb->ScanAssetAllocations(count, from, options, oRes))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("Scan failed"));
	return oRes;
}
UniValue listassetallocationmempoolbalances(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 3 < params.size())
        throw runtime_error("listassetallocationmempoolbalances [count] [from] [{options}]\n"
            "scan through all asset allocation mempool balances. Useful for ZDAG analysis on senders of allocations.\n"
            "[count]          (numeric, optional, default=10) The number of results to return.\n"
            "[from]           (numeric, optional, default=0) The number of results to skip.\n"
            "[options]        (array, optional) A json object with options to filter results\n"
            "    {\n"
            "      \"senders\"                    (array) a json array with senders\n"
            "       [\n"
            "           {\n"
            "               \"address\":string      (string) Sender address to filter.\n"
            "           } \n"
            "           ,...\n"
            "       ]\n"
            "    }\n"
            + HelpExampleCli("listassetallocationmempoolbalances", "0")
            + HelpExampleCli("listassetallocationmempoolbalances", "10 10")
            + HelpExampleCli("listassetallocationmempoolbalances", "0 0 '{\"senders\":[{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"},{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"}]}'")
        );
    UniValue options;
    int count = 10;
    int from = 0;
    if (params.size() > 0) {
        count = params[0].get_int();
        if (count == 0) {
            count = 10;
        } else
        if (count < 0) {
            throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("'count' must be 0 or greater"));
        }
    }
    if (params.size() > 1) {
        from = params[1].get_int();
        if (from < 0) {
            throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("'from' must be 0 or greater"));
        }
    }
    if (params.size() > 2) {
        options = params[2];
    }
    UniValue oRes(UniValue::VARR);
    if (!passetallocationmempooldb->ScanAssetAllocationMempoolBalances(count, from, options, oRes))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1510 - " + _("Scan failed"));
    return oRes;
}