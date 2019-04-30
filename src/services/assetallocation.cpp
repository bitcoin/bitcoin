// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <validation.h>
#include <txmempool.h>
#include <core_io.h>
#include <wallet/wallet.h>
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
            RPCHelpMan{"assetallocationburn",
                "\nBurn an asset allocation in order to use the bridge\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to burn to SYSX"},
                    {"ethereum_destination_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The 20 byte (40 character) hex string of the ethereum destination address. Leave empty to burn as normal without the bridge.  If it is left empty this will process as a normal assetallocationsend to the burn address"}
                },
                RPCResult{""},
                RPCExamples{
                    HelpExampleCli("assetallocationburn", "\"asset\" \"address\" \"amount\" \"ethereum_destination_address\"")
                    + HelpExampleRpc("assetallocationburn", "\"asset\", \"address\", \"amount\", \"ethereum_destination_address\"")
                }
            }.ToString());

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
            RPCHelpMan{"assetallocationmint",
                "\nMint assetallocation to come back from the bridge\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Mint to this address."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to mint.  Note that fees will be taken from the owner address"},
                    {"blocknumer", RPCArg::Type::NUM, RPCArg::Optional::NO, "Block number of the block that included the burn transaction on Ethereum."},
                    {"tx_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Raw transaction hex of the burn transaction on Ethereum."},
                    {"txroot_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction merkle root that commits this transaction to the block header."},
                    {"txmerkleproof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The list of parent nodes of the Merkle Patricia Tree for SPV proof."},
                    {"txmerkleroofpath_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The merkle path to walk through the tree to recreate the merkle root."},
                    {"witness", RPCArg::Type::STR, "\"\"", "Witness address that will sign for web-of-trust notarization of this transaction."}
                },
                RPCResult{
                    "[\n"
                    "  \"txid\"                 (string) The transaction ID"
                    "]\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationmint", "\"assetguid\" \"addressto\" \"amount\" \"blocknumber\" \"tx_hex\" \"txroot_hex\" \"txmerkleproof_hex\" \"txmerkleproofpath_hex\" \"witness\"")
                    + HelpExampleRpc("assetallocationmint", "\"assetguid\", \"addressto\", \"amount\", \"blocknumber\", \"tx_hex\", \"txroot_hex\", \"txmerkleproof_hex\", \"txmerkleproofpath_hex\", \"\"")
                }
            }.ToString());

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
            RPCHelpMan{"assetallocationsend",
                "\nSend an asset allocation you own to another address.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid"},
                    {"addressfrom", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation from"},
                    {"addressto", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation to"},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send"}
                },
                RPCResult{
                    "[\n"
                    "  \"hexstring\":    (string) the unsigned transaction hexstring.\n"
                    "]\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationsend", "\"assetguid\" \"addressfrom\" \"addressto\" \"amount\"")
                    + HelpExampleRpc("assetallocationsend", "\"assetguid\", \"addressfrom\", \"addressto\", \"amount\"")
                }
            }.ToString());
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
            RPCHelpMan{"assetallocationsendmany",
			    "\nSend an asset allocation you own to another address. Maximimum recipients is 250.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"addressfrom", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
                    {"amounts", RPCArg::Type::ARR, RPCArg::Optional::NO, "Array of assetallocationsend objects",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "The assetallocationsend object",
                                {
                                    {"addressto", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address to transfer to"},
                                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "Quantity of asset to send"}
                                }
                            },
                         },
                         "[assetallocationsend object]..."
                     },
                     {"witness", RPCArg::Type::STR, "\"\"", "Witness address that will sign for web-of-trust notarization of this transaction"}
                },
                RPCResult{
                    "[\n"
                    "  \"hexstring\":    (string) the unsigned transaction hexstring.\n"
                    "]\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationsendmany", "\"assetguid\" \"addressfrom\" \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
                    + HelpExampleCli("assetallocationsendmany", "\"assetguid\" \"addressfrom\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
                    + HelpExampleRpc("assetallocationsendmany", "\"assetguid\", \"addressfrom\", \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\', \"\"")
                    + HelpExampleRpc("assetallocationsendmany", "\"assetguid\", \"addressfrom\", \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\", \"\"")
                }
            }.ToString());

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
UniValue assetallocationbalance(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error(
            RPCHelpMan{"assetallocationbalance",
                "\nShow stored balance of a single asset allocation.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The guid of the asset"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the allocation owner"}
                },
                RPCResult{
                "\"balance\"        (numeric) The balance of a single asset allocation.\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationbalance","\"asset_guid\" \"address\"")
                    + HelpExampleRpc("assetallocationbalance", "\"asset_guid\", \"address\"")
                }
            }.ToString());

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
        throw runtime_error(
            RPCHelpMan{"assetallocationinfo",
                "\nShow stored values of a single asset allocation.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The guid of the asset"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the owner"}
                },
                RPCResult{
                    "{\n"
                    "  \"_id\":           (string) The unique id of this allocation\n"
                    "  \"asset_guid\":         (string) The guid of the asset\n"
                    "  \"address\":       (string) The address of the owner of this allocation\n"
                    "  \"balance\":       (numeric) The current balance\n"
                    "  \"balance_zdag\":  (numeric) The zdag balance\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationinfo", "\"assetguid\" \"address\"")
                    + HelpExampleRpc("assetallocationinfo", "\"assetguid\", \"address\"")
                }
            }.ToString());

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
			"Level 2 means an active double spend was found and any depending asset allocation sends are also flagged as dangerous and should wait for POW confirmation before proceeding.\n"
            "\nArguments:\n"
            "1. \"asset\":      (numeric, required) The guid of the asset\n"
            "2. \"address\":    (string, required) The address of the sender\n"
            "3. \"txid\":       (string, required) The transaction id of the assetallocationsend\n"   
            "\nResult:\n"
            "{\n"
            "  \"status\":      (numeric) The status level of the transaction\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("assetallocationsenderstatus", "\"asset\" \"address\" \"txid\"")
            + HelpExampleRpc("assetallocationsenderstatus", "\"asset\", \"address\", \"txid\"")
            );

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
    if(pblockindexdb && pblockindexdb->ReadBlockHash(txHash, blockhash))  
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
        if(pblockindexdb && pblockindexdb->ReadBlockHash(txHash, blockhash))
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
		throw runtime_error(
            RPCHelpMan{"listassetallocations",
			    "\nScan through all asset allocations.\n",
                {
                    {"count", RPCArg::Type::NUM, "10", "The number of results to return."},
                    {"from", RPCArg::Type::NUM, "0", "The number of results to skip."},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "A json object with options to filter results.",
                        {
                            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Asset GUID to filter"},
                            {"addresses", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "A json array with owners",  
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address to filter"},
                                },
                                "[addressobjects,...]"
                            }
                        }
                     }
                 },
                 RPCResult{
                 "[\n"
                 "  {\n"
                 "    \"_id\":           (string) The unique id of this allocation\n"
                 "    \"asset_guid\":         (string) The guid of the asset\n"
                 "    \"address\":       (string) The address of the owner of this allocation\n"
                 "    \"balance\":       (numeric) The current balance\n"
                 "    \"balance_zdag\":  (numeric) The zdag balance\n"
                 "  }\n"
                 " ...\n"
                 "]\n"
                 },
                 RPCExamples{
			         HelpExampleCli("listassetallocations", "0")
        			+ HelpExampleCli("listassetallocations", "10 10")
		        	+ HelpExampleCli("listassetallocations", "0 0 '{\"asset_guid\":92922}'")
	        		+ HelpExampleCli("listassetallocations", "0 0 '{\"addresses\":[{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"},{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}]}'")
		        	+ HelpExampleRpc("listassetallocations", "0")
        			+ HelpExampleRpc("listassetallocations", "10, 10")
	        		+ HelpExampleRpc("listassetallocations", "0, 0, '{\"asset_guid\":92922}'")
		        	+ HelpExampleRpc("listassetallocations", "0, 0, '{\"addresses\":[{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"},{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}]}'")
                 }
            }.ToString());
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
        throw runtime_error(
            RPCHelpMan{"listassetallocationmempoolbalances",
                "\nScan through all asset allocation mempool balances. Useful for ZDAG analysis on senders of allocations.\n",
                {
                    {"count", RPCArg::Type::NUM, "10", "The number of results to return."},
                    {"from", RPCArg::Type::NUM, "0", "The number of results to skip."},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "A json object with options to filter results.",
                        {
                            {"addresses_array", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "A json array with owners",  
                                {
                                    {"sender_address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address to filter"},
                                },
                                "[address,...]"
                            }
                        }
                     }
                },
                RPCResult{""},
                RPCExamples{
                    HelpExampleCli("listassetallocationmempoolbalances", "0")
                    + HelpExampleCli("listassetallocationmempoolbalances", "10 10")
                    + HelpExampleCli("listassetallocationmempoolbalances", "0 0 '{\"senders\":[{\"address\":\"sysrt1q9hrtqlcpvd089hswwa3gtsy29f8pugc3wah3fl\"},{\"address\":\"sysrt1qea3v4dj5kjxjgtysdxd3mszjz56530ugw467dq\"}]}'")
                    + HelpExampleRpc("listassetallocationmempoolbalances", "0")
                    + HelpExampleRpc("listassetallocationmempoolbalances", "10, 10")
                    + HelpExampleRpc("listassetallocationmempoolbalances", "0, 0, '{\"senders\":[{\"address\":\"sysrt1q9hrtqlcpvd089hswwa3gtsy29f8pugc3wah3fl\"},{\"address\":\"sysrt1qea3v4dj5kjxjgtysdxd3mszjz56530ugw467dq\"}]}'")
                }
            }.ToString());
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
