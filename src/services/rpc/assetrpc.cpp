// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <validation.h>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <future>
#include <rpc/util.h>
#include <services/assetconsensus.h>
#include <services/rpc/assetrpc.h>
#include <chainparams.h>
#include <rpc/server.h>
#include <validationinterface.h>
using namespace std;
extern std::string exePath;
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
extern bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
extern std::unordered_set<std::string> assetAllocationConflicts;
// SYSCOIN service rpc functions
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
using namespace std;
UniValue convertaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{"convertaddress",
            "\nConvert between Syscoin 3 and Syscoin 4 formats. P2WPKH can be shown as P2PKH in Syscoin 3.\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to get the information of."}
            },
            RPCResult{
                "{\n"
                "  \"v3address\" : \"address\",        (string) The syscoin 3 address validated\n"
                "  \"v4address\" : \"address\",        (string) The syscoin 4 address validated\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("convertaddress", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"")
                + HelpExampleRpc("convertaddress", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"")
            }
            }.ToString());
    }
    
    UniValue ret(UniValue::VOBJ);
    CTxDestination dest = DecodeDestination(request.params[0].get_str());

    // Make sure the destination is valid
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    std::string currentV4Address = "";
    std::string currentV3Address = "";
    if (auto witness_id = boost::get<WitnessV0KeyHash>(&dest)) {
        currentV4Address =  EncodeDestination(dest);
        currentV3Address =  EncodeDestination(PKHash(*witness_id));
    }
    else if (auto key_id = boost::get<PKHash>(&dest)) {
        currentV4Address =  EncodeDestination(WitnessV0KeyHash(*key_id));
        currentV3Address =  EncodeDestination(*key_id);
    }
    else if (auto script_id = boost::get<ScriptHash>(&dest)) {
        currentV4Address =  EncodeDestination(*script_id);
        currentV3Address =  currentV4Address;
    }
    else if (auto script_id = boost::get<WitnessV0ScriptHash>(&dest)) {
        currentV4Address =  EncodeDestination(dest);
        currentV3Address =  currentV4Address;
    }  
    ret.pushKV("v3address", currentV3Address);
    ret.pushKV("v4address", currentV4Address);
                       
    
    return ret;
}
unsigned int addressunspent(const string& strAddressFrom, COutPoint& outpoint)
{
    UniValue paramsUTXO(UniValue::VARR);
    UniValue utxoParams(UniValue::VARR);
    utxoParams.push_back("addr(" + strAddressFrom + ")");
    paramsUTXO.push_back("start");
    paramsUTXO.push_back(utxoParams);
    JSONRPCRequest request;
    request.params = paramsUTXO;
    UniValue resUTXOs = scantxoutset(request);
    UniValue utxoArray(UniValue::VARR);
    if (resUTXOs.isObject()) {
        const UniValue& resUtxoUnspents = find_value(resUTXOs.get_obj(), "unspents");
        if (!resUtxoUnspents.isArray())
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No unspent outputs found in addresses provided"));
        utxoArray = resUtxoUnspents.get_array();
    }   
    else
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No unspent outputs found in addresses provided"));
        
    unsigned int count = 0;
    {
        LOCK(mempool.cs);
        for (unsigned int i = 0; i < utxoArray.size(); i++)
        {
            const UniValue& utxoObj = utxoArray[i].get_obj();
            const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
            const int& nOut = find_value(utxoObj, "vout").get_int();

            const COutPoint &outPointToCheck = COutPoint(txid, nOut);
            bool locked = false;
            // spending as non allocation send while using a locked outpoint should be invalid
            if (plockedoutpointsdb->ReadOutpoint(outPointToCheck, locked) && locked)
                continue;
            if (mempool.mapNextTx.find(outPointToCheck) != mempool.mapNextTx.end())
                continue;
            if (outpoint.IsNull())
                outpoint = outPointToCheck;
            count++;
        }
    }
    return count;
}
UniValue ValueFromAssetAmount(const CAmount& amount,int precision)
{
    if (precision < 0 || precision > 8)
        throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs;
    int64_t divByAmount = 1;
    int64_t remainder = 0;
    string strPrecision = "0";
    if (precision > 0) {
        divByAmount = pow(10, precision);
        quotient = n_abs / divByAmount;
        remainder = n_abs % divByAmount;
        strPrecision = itostr(precision);
    }

    return UniValue(UniValue::VNUM,
        strprintf("%s%d.%0" + strPrecision + "d", sign ? "-" : "", quotient, remainder));
}
CAmount AssetAmountFromValue(UniValue& value, int precision)
{
    if(precision < 0 || precision > 8)
        throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
    if (!value.isNum() && !value.isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
    if (value.isStr() && value.get_str() == "-1") {
        value.setInt((int64_t)(MAX_ASSET / ((int)pow(10, precision))));
    }
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), precision, &amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!AssetRange(amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
    return amount;
}
bool AssetRange(const CAmount& amount, int precision)
{

    if (precision < 0 || precision > 8)
        throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs;
    if (precision > 0) {
        int64_t divByAmount = pow(10, precision);
        quotient = n_abs / divByAmount;
    }
    if (!AssetRange(quotient))
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
	oTPSTestResults.__pushKV("enabled", fTPSTestEnabled);
    oTPSTestResults.__pushKV("testinitiatetime", (int64_t)nTPSTestingStartTime);
	oTPSTestResults.__pushKV("teststarttime", (int64_t)nTPSTestingSendRawEndTime);
   
	for (auto &receivedTime : vecTPSTestReceivedTimesMempool) {
		UniValue oTPSTestStatusObj(UniValue::VOBJ);
		oTPSTestStatusObj.__pushKV("txid", receivedTime.first.GetHex());
		oTPSTestStatusObj.__pushKV("time", receivedTime.second);
		oTPSTestReceiversMempool.push_back(oTPSTestStatusObj);
	}
	oTPSTestResults.__pushKV("receivers", oTPSTestReceiversMempool);
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
	result.__pushKV("status", "success");
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
	result.__pushKV("status", "success");
	return result;
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
                "{\n"
                "  \"amount\": xx        (numeric) The balance of a single asset allocation.\n"
                "}\n"
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

    UniValue oRes(UniValue::VOBJ);
    oRes.__pushKV("amount", ValueFromAssetAmount(txPos.nBalance, theAsset.nPrecision));
    return oRes;
}

UniValue assetallocationbalances(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error(
            RPCHelpMan{"assetallocationbalances",
                "\nShow stored balance of multiple asset allocations.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The guid of the asset"},
                    {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The addresses owning the allocations",
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "syscoin address"},
                                },
                    }
                },
                RPCResult{
                "{\n"
                "  \"address1\": xx,       (numeric) The balance of a single asset allocation.\n"
                "  \"address2\": xx        (numeric) The balance of a single asset allocation.\n"
                "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationbalances","\"asset_guid\" \"[\\\"address1\\\",\\\"address2\\\"]\"")
                    + HelpExampleRpc("assetallocationbalances", "\"asset_guid\", \"[\\\"address1\\\",\\\"address2\\\"]\"")
                }
            }.ToString());

    const int &nAsset = params[0].get_int();
    const UniValue &headerArray = params[1].get_array();

    CAsset theAsset;
    if (!GetAsset(nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1508 - " + _("Could not find a asset with this key"));

    UniValue oRes(UniValue::VOBJ);
    for(size_t i =0;i<headerArray.size();i++){
        string strAddressFrom = headerArray[i].get_str();
        string witnessProgramHex = "";
        unsigned char witnessVersion = 0;
        if(strAddressFrom != "burn"){
            const CTxDestination &dest = DecodeDestination(strAddressFrom);
            UniValue detail = DescribeAddress(dest);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                continue;
            witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
            witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
        }

        UniValue oAssetAllocation(UniValue::VOBJ);
        const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddressFrom == "burn"? vchFromString("burn"): ParseHex(witnessProgramHex)));
        CAssetAllocation txPos;
        if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
            continue;

        oRes.__pushKV(strAddressFrom, ValueFromAssetAmount(txPos.nBalance, theAsset.nPrecision));
    }
    return oRes;
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
                    "    \"asset_allocation\":   (string) The unique key for this allocation\n"
                    "    \"asset_guid\":         (string) The guid of the asset\n"
                    "    \"symbol\":             (string) The asset symbol\n"
                    "    \"address\":            (string) The address of the owner of this allocation\n"
                    "    \"balance\":            (numeric) The current balance\n"
                    "    \"balance_zdag\":       (numeric) The zdag balance\n"
                    "    \"locked_outpoint\":    (string) The locked UTXO if applicable for this allocation\n"
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
UniValue listassetindexallocations(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error(
            RPCHelpMan{"listassetindexallocations",
                "\nReturn a list of asset allocations an address is associated with.\n",
                {
                    {"address", RPCArg::Type::NUM, RPCArg::Optional::NO, "Address to find assets associated with."}
                },
                RPCResult{
                    "[\n"
                    "  {\n"
                    "    \"asset_allocation\":   (string) The unique key for this allocation\n"
                    "    \"asset_guid\":         (string) The guid of the asset\n"
                    "    \"symbol\":             (string) The asset symbol\n"
                    "    \"address\":            (string) The address of the owner of this allocation\n"
                    "    \"balance\":            (numeric) The current balance\n"
                    "    \"balance_zdag\":       (numeric) The zdag balance\n"
                    "    \"locked_outpoint\":    (string) The locked UTXO if applicable for this allocation\n"
                    "  },\n"
                    "  ...\n"
                    "]\n"
                },
                RPCExamples{
                    HelpExampleCli("listassetindexallocations", "sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7")
                    + HelpExampleRpc("listassetindexallocations", "sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7")
                }
            }.ToString());
    if(!fAssetIndex){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("You must reindex syscoin with -assetindex enabled"));
    }       
    UniValue requestParam(UniValue::VARR);
    requestParam.push_back(params[0].get_str());
    JSONRPCRequest jsonRequest;
    jsonRequest.params = requestParam;
    const UniValue &convertedAddressValue = convertaddress(jsonRequest);
    const std::string & v4address = find_value(convertedAddressValue.get_obj(), "v4address").get_str();
    const CTxDestination &dest = DecodeDestination(v4address);
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
 
    UniValue oRes(UniValue::VARR);
    std::vector<uint32_t> assetGuids;
    passetallocationdb->ReadAssetsByAddress(CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)), assetGuids);
    
    CWitnessAddress witnessAddress(witnessVersion, ParseHex(witnessProgramHex));
    for(const uint32_t& guid: assetGuids){
        UniValue oAssetAllocation(UniValue::VOBJ);
        const CAssetAllocationTuple assetAllocationTuple(guid, witnessAddress);
        CAssetAllocation txPos;
        if (passetallocationdb == nullptr || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
            continue;
        CAsset theAsset;
        if (!GetAsset(guid, theAsset))
           continue;

        if(BuildAssetAllocationJson(txPos, theAsset, oAssetAllocation)){
            oRes.push_back(oAssetAllocation);
        }
    }
    return oRes;
}

UniValue assetallocationsenderstatus(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 3 != params.size())
		throw runtime_error(
            RPCHelpMan{"assetallocationsenderstatus",
                "\nShow status as it pertains to any current Z-DAG conflicts or warnings related to a sender or sender/txid combination of an asset allocation transfer. Leave txid empty if you are not checking for a specific transfer.\n"
                "Return value is in the status field and can represent 3 levels(0, 1 or 2)\n"
                "Level -1 means not found, not a ZDAG transaction, perhaps it is already confirmed.\n"
                "Level 0 means OK.\n"
                "Level 1 means warning (checked that in the mempool there are more spending balances than current POW sender balance). An active stance should be taken and perhaps a deeper analysis as to potential conflicts related to the sender.\n"
                "Level 2 means an active double spend was found and any depending asset allocation sends are also flagged as dangerous and should wait for POW confirmation before proceeding.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The guid of the asset"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the sender"},
                    {"txid", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The transaction id of the assetallocationsend"}
                },
                RPCResult{
                    "{\n"
                    "  \"status\":      (numeric) The status level of the transaction\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationsenderstatus", "\"asset_guid\" \"address\" \"txid\"")
                    + HelpExampleRpc("assetallocationsenderstatus", "\"asset_guid\", \"address\", \"txid\"")
                }
            }.ToString());

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
    	
        oAssetAllocationStatus.__pushKV("status", nStatus);
    }
	return oAssetAllocationStatus;
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
                 "    \"asset_allocation\":   (string) The unique key for this allocation\n"
                 "    \"asset_guid\":         (string) The guid of the asset\n"
                 "    \"symbol\":             (string) The asset symbol\n"
                 "    \"address\":            (string) The address of the owner of this allocation\n"
                 "    \"balance\":            (numeric) The current balance\n"
                 "    \"balance_zdag\":       (numeric) The zdag balance\n"
                 "    \"locked_outpoint\":    (string) The locked UTXO if applicable for this allocation\n"
                 "  }\n"
                 "  ...\n"
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
                RPCResults{},
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

UniValue syscoindecoderawtransaction(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoindecoderawtransaction",
            "\nDecode raw syscoin transaction (serialized, hex-encoded) and display information pertaining to the service that is included in the transactiion data output(OP_RETURN)\n",
            {
                {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction hex string."}
            },
            RPCResult{
            "{\n"
            "  \"txtype\" : \"txtype\",         (string) The syscoin transaction type\n"
            "  \"asset_guid\" : n,              (numeric) The asset guid\n"
            "  \"symbol\" : \"symbol\",         (string) The asset symbol\n"
            "  \"txid\" : \"id\",               (string) The transaction id\n"
            "  \"height\" : n,                  (numeric) The blockheight of the transaction \n"
            "  \"sender\" : \"address\",        (string) The address of the sender\n"
            "  \"allocations\" : [              (array of json objects)\n"
            "    {\n"
            "      \"address\": \"address\",    (string) The address of the receiver\n"
            "      \"amount\" : n,              (numeric) The amount of the transaction\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "  \"total\" : n,                   (numeric) The total amount in this transaction\n"
            "  \"confirmed\" : true|false       (boolean) If the transaction is confirmed\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("syscoindecoderawtransaction", "\"hexstring\"")
                + HelpExampleRpc("syscoindecoderawtransaction", "\"hexstring\"")
            }
            }.ToString());

    string hexstring = params[0].get_str();
    CMutableTransaction tx;
    if(!DecodeHexTx(tx, hexstring, false, true))
        DecodeHexTx(tx, hexstring, true, true);
    CTransaction rawTx(tx);
    if (rawTx.IsNull())
        throw runtime_error("SYSCOIN_RPC_ERROR: ERRCODE: 5512 - " + _("Could not decode transaction"));
    
    UniValue output(UniValue::VOBJ);
    if(!DecodeSyscoinRawtransaction(rawTx, output))
        throw runtime_error("SYSCOIN_RPC_ERROR: ERRCODE: 5512 - " + _("Not a Syscoin transaction"));
    return output;
}
CAmount getaddressbalance(const string& strAddress)
{
    UniValue paramsUTXO(UniValue::VARR);
    UniValue utxoParams(UniValue::VARR);
    utxoParams.push_back("addr(" + strAddress + ")");
    paramsUTXO.push_back("start");
    paramsUTXO.push_back(utxoParams);
    JSONRPCRequest request;
    request.params = paramsUTXO;
    UniValue resUTXOs = scantxoutset(request);
    return AmountFromValue(find_value(resUTXOs.get_obj(), "total_amount"));
}
UniValue addressbalance(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"addressbalance",
            "\nShow the Syscoin balance of an address\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to holding the balance"}
            },
            RPCResult{
            "{\n"
            "  \"amount\": xx            (numeric) Syscoin balance of the address\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("addressbalance", "\"sysrt1qea3v4dj5kjxjgtysdxd3mszjz56530ugw467dq\"")
                + HelpExampleRpc("addressbalance", "\"sysrt1qea3v4dj5kjxjgtysdxd3mszjz56530ugw467dq\"")
                }
            }.ToString());
    string address = params[0].get_str();
    UniValue res(UniValue::VOBJ);
    res.__pushKV("amount", ValueFromAmount(getaddressbalance(address)));
    return res;
}
UniValue assetinfo(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error(
            RPCHelpMan{"assetinfo",
                "\nShow stored values of a single asset and its.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid"}
                },
                RPCResult{
                    "{\n"
                    "  \"asset_guid\":          (numeric) The asset guid\n"
                    "  \"txid\":         (string) The transaction id that created this asset\n"
                    "  \"public_value\":  (string) The public value attached to this asset\n"
                    "  \"address\":      (string) The address that controls this address\n"
                    "  \"contract\":     (string) The ethereum contract address\n"
                    "  \"balance\":      (numeric) The current balance\n"
                    "  \"total_supply\": (numeric) The total supply of this asset\n"
                    "  \"max_supply\":   (numeric) The maximum supply of this asset\n"
                    "  \"update_flag\":  (numeric) The flag in decimal \n"
                    "  \"precision\":    (numeric) The precision of this asset \n"   
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetinfo", "\"assetguid\"")
                    + HelpExampleRpc("assetinfo", "\"assetguid\"")
                }
            }.ToString());

    const int &nAsset = params[0].get_int();
    UniValue oAsset(UniValue::VOBJ);

    CAsset txPos;
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2511 - " + _("Failed to read from asset DB"));

    if(!BuildAssetJson(txPos, oAsset))
        oAsset.clear();
    return oAsset;
}
UniValue listassets(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 3 < params.size())
        throw runtime_error(
            RPCHelpMan{"listassets",
                "\nScan through all assets.\n",
                {
                    {"count", RPCArg::Type::NUM, "10", "The number of results to return."},
                    {"from", RPCArg::Type::NUM, "0", "The number of results to skip."},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "A json object with options to filter results.",
                        {
                            {"txid", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Transaction ID to filter results for"},
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
                 "    \"asset_guid\":   (numeric) The asset guid\n"
                 "    \"symbol\":       (string) The asset symbol\n"
                 "    \"txid\":         (string) The transaction id that created this asset\n"
                 "    \"public_value\":  (string) The public value attached to this asset\n"
                 "    \"address\":      (string) The address that controls this address\n"
                 "    \"contract\":     (string) The ethereum contract address\n"
                 "    \"balance\":      (numeric) The current balance\n"
                 "    \"total_supply\": (numeric) The total supply of this asset\n"
                 "    \"max_supply\":   (numeric) The maximum supply of this asset\n"
                 "    \"update_flag\":  (numeric) The flag in decimal \n"
                 "    \"precision\":    (numeric) The precision of this asset \n"   
                 "  },\n"
                 "  ...\n"
                 "]\n"
                 },
                 RPCExamples{
                    HelpExampleCli("listassets", "0")
                    + HelpExampleCli("listassets", "10 10")
                    + HelpExampleCli("listassets", "0 0 '{\"addresses\":[{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"},{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}]}'")
                    + HelpExampleCli("listassets", "0 0 '{\"asset_guid\":3473733}'")
                    + HelpExampleRpc("listassets", "0, 0, '{\"addresses\":[{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"},{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}]}'")
                    + HelpExampleRpc("listassets", "0, 0, '{\"asset_guid\":3473733}'")
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
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("'count' must be 0 or greater"));
        }
    }
    if (params.size() > 1) {
        from = params[1].get_int();
        if (from < 0) {
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("'from' must be 0 or greater"));
        }
    }
    if (params.size() > 2) {
        options = params[2];
    }

    UniValue oRes(UniValue::VARR);
    if (!passetdb->ScanAssets(count, from, options, oRes))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Scan failed"));
    return oRes;
}
UniValue getblockhashbytxid(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"getblockhashbytxid",
                "\nReturns hash of block in best-block-chain at txid provided.\n",
                {
                    {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "A transaction that is in the block."}
                },
                RPCResult{
                "{\n"
                "  \"hex\": \"hexstring\"     (string) The block hash that contains the txid\n"
                "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockhashbytxid", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
                    + HelpExampleRpc("getblockhashbytxid", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
                }
            }.ToString());
    LOCK(cs_main);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    uint256 blockhash;
    if(!pblockindexdb->ReadBlockHash(hash, blockhash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found in asset index");

    const CBlockIndex* pblockindex = LookupBlockIndex(blockhash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }
    UniValue res{UniValue::VOBJ};
    res.__pushKV("hex", pblockindex->GetBlockHash().GetHex());
    return res;
}
UniValue syscoingetspvproof(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            RPCHelpMan{"syscoingetspvproof",
            "\nReturns SPV proof for use with inter-chain transfers.\n",
            {
                {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "A transaction that is in the block"},
                {"blockhash", RPCArg::Type::STR, "\"\"", "Block containing txid"}
            },
            RPCResult{
            "\"proof\"         (string) JSON representation of merkl/ nj   nk ne proof (transaction index, siblings and block header and some other information useful for moving coins/assets to another chain)\n"
            },
            RPCExamples{
                HelpExampleCli("syscoingetspvproof", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
                + HelpExampleRpc("syscoingetspvproof", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
            }
            }.ToString());
    LOCK(cs_main);
    UniValue res(UniValue::VOBJ);
    uint256 txhash = ParseHashV(request.params[0], "parameter 1");
    uint256 blockhash;
    if(request.params.size() > 1)
        blockhash = ParseHashV(request.params[1], "parameter 2");
    if(!pblockindexdb->ReadBlockHash(txhash, blockhash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found in asset index");
    
    CBlockIndex* pblockindex = LookupBlockIndex(blockhash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }
    CTransactionRef tx;
    uint256 hash_block;
    if (!GetTransaction(txhash, tx, Params().GetConsensus(), hash_block, pblockindex))   
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not found"); 

    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }   
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pblockindex->GetBlockHeader(Params().GetConsensus());
    const std::string &rawTx = EncodeHexTx(CTransaction(*tx), PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    res.__pushKV("transaction",rawTx);
    // get first 80 bytes of header (non auxpow part)
    res.__pushKV("header", HexStr(ssBlock.begin(), ssBlock.begin()+80));
    UniValue siblings(UniValue::VARR);
    // store the index of the transaction we are looking for within the block
    int nIndex = 0;
    for (unsigned int i = 0;i < block.vtx.size();i++) {
        const uint256 &txHashFromBlock = block.vtx[i]->GetHash();
        if(txhash == txHashFromBlock)
            nIndex = i;
        siblings.push_back(txHashFromBlock.GetHex());
    }
    res.__pushKV("siblings", siblings);
    res.__pushKV("index", nIndex);
    UniValue assetVal;
    try{
        UniValue paramsDecode(UniValue::VARR);
        paramsDecode.push_back(rawTx);   
        JSONRPCRequest requestDecodeRPC;
        requestDecodeRPC.params = paramsDecode;
        UniValue resDecode = syscoindecoderawtransaction(requestDecodeRPC);
        assetVal = find_value(resDecode.get_obj(), "asset_guid"); 
    }
    catch(const runtime_error& e){
    }
    if(!assetVal.isNull()) {
        CAsset asset;
        if (!GetAsset(assetVal.get_int(), asset))
             throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Asset not found"));
        if(asset.vchContract.empty())
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Asset contract is empty"));
         res.__pushKV("contract", HexStr(asset.vchContract));    
                   
    }
    else{
        res.__pushKV("contract", HexStr(Params().GetConsensus().vchSYSXContract));
    }
    return res;
}
UniValue listassetindex(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error(
            RPCHelpMan{"listassetindex",
            "\nScan through all asset index and return paged results based on page number passed in. Requires assetindex config parameter enabled and optional assetindexpagesize which is 25 by default.\n",
            {
                {"page", RPCArg::Type::NUM, "0", "Return specific page number of transactions. Lower page number means more recent transactions."},
                {"options", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json object with options to filter results", 
                    {
                        {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset GUID to filter."},
                        {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address to filter.  Leave empty to scan globally through asset"}
                    }
                }
            },
            RPCResult{
                 "[\n"
                 "  {\n"
                 "    \"asset_guid\":   (numeric) The asset guid\n"
                 "    \"symbol\":       (string) The asset symbol\n"
                 "    \"txid\":         (string) The transaction id that created this asset\n"
                 "    \"public_value\":  (string) The public value attached to this asset\n"
                 "    \"address\":      (string) The address that controls this address\n"
                 "    \"contract\":     (string) The ethereum contract address\n"
                 "    \"balance\":      (numeric) The current balance\n"
                 "    \"total_supply\": (numeric) The total supply of this asset\n"
                 "    \"max_supply\":   (numeric) The maximum supply of this asset\n"
                 "    \"update_flag\":  (numeric) The flag in decimal \n"
                 "    \"precision\":    (numeric) The precision of this asset \n"   
                 "  },\n"
                 "  ...\n"
                 "]\n"
            },
            RPCExamples{
                HelpExampleCli("listassetindex", "0 '{\"asset_guid\":92922}'")
                + HelpExampleCli("listassetindex", "2 '{\"asset_guid\":92922, \"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}'")
                + HelpExampleRpc("listassetindex", "0, '{\"asset_guid\":92922}'")
                + HelpExampleRpc("listassetindex", "2, '{\"asset_guid\":92922, \"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}'")
            }
        }.ToString());
    if(!fAssetIndex){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("You must start syscoin with -assetindex enabled"));
    }
    UniValue options;
    int64_t page = params[0].get_int64();
   
    if (page < 0) {
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("'page' must be 0 or greater"));
    }

    options = params[1];
    
    UniValue oRes(UniValue::VARR);
    if (!passetindexdb->ScanAssetIndex(page, options, oRes))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Scan failed"));
    return oRes;
}
UniValue listassetindexassets(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error(
            RPCHelpMan{"listassetindexassets",
                "\nReturn a list of assets an address is associated with.\n",
                {
                    {"address", RPCArg::Type::NUM, RPCArg::Optional::NO, "Address to find assets associated with."}
                },
                RPCResult{
                    "[\n"
                    "  {\n"
                    "    \"asset_guid\":   (numeric) The asset guid\n"
                    "    \"symbol\":       (string) The asset symbol\n"
                    "    \"txid\":         (string) The transaction id that created this asset\n"
                    "    \"public_value\":  (string) The public value attached to this asset\n"
                    "    \"address\":      (string) The address that controls this address\n"
                    "    \"contract\":     (string) The ethereum contract address\n"
                    "    \"balance\":      (numeric) The current balance\n"
                    "    \"total_supply\": (numeric) The total supply of this asset\n"
                    "    \"max_supply\":   (numeric) The maximum supply of this asset\n"
                    "    \"update_flag\":  (numeric) The flag in decimal \n"
                    "    \"precision\":    (numeric) The precision of this asset \n"
                    "  },\n"
                    "  ...\n"
                    "]\n"
                },
                RPCExamples{
                    HelpExampleCli("listassetindexassets", "sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7")
                    + HelpExampleRpc("listassetindexassets", "sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7")
                }
            }.ToString());
    if(!fAssetIndex){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("You must reindex syscoin with -assetindex enabled"));
    }  
    UniValue requestParam(UniValue::VARR);
    requestParam.push_back(params[0].get_str());
    JSONRPCRequest jsonRequest;
    jsonRequest.params = requestParam;
    const UniValue &convertedAddressValue = convertaddress(jsonRequest);     
    const std::string & v4address = find_value(convertedAddressValue.get_obj(), "v4address").get_str();    
    const CTxDestination &dest = DecodeDestination(v4address);
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
 
    UniValue oRes(UniValue::VARR);
    std::vector<uint32_t> assetGuids;
    const CWitnessAddress witnessAddress(witnessVersion, ParseHex(witnessProgramHex));
    passetdb->ReadAssetsByAddress(witnessAddress, assetGuids);
    
    for(const uint32_t& guid: assetGuids){
        UniValue oAsset(UniValue::VOBJ);
        CAsset theAsset;
        if (!GetAsset(guid, theAsset))
           continue;
        // equality: catch case where asset is transferred
        if(theAsset.witnessAddress == witnessAddress && BuildAssetJson(theAsset, oAsset)){
            oRes.push_back(oAsset);
        }
    }
    return oRes;
}
UniValue syscoinstopgeth(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 0 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoinstopgeth",
            "\nStops Geth and the relayer from running.\n",
            {},
            RPCResult{
            "{\n"
            "    \"status\": xx     (string) Result\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("syscoinstopgeth", "")
                + HelpExampleRpc("syscoinstopgeth", "")
            }
            }.ToString());
    if(!StopRelayerNode(relayerPID))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop relayer"));
    if(!StopGethNode(gethPID))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop Geth"));
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", "success");
    return ret;
}
UniValue syscoinstartgeth(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 0 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoinstartgeth",
            "\nStarts Geth and the relayer.\n",
            {},
            RPCResult{
            "{\n"
            "    \"status\": xx     (string) Result\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("syscoinstartgeth", "")
                + HelpExampleRpc("syscoinstartgeth", "")
            }
            }.ToString());
    
    StopRelayerNode(relayerPID);
    StopGethNode(gethPID);
    int wsport = gArgs.GetArg("-gethwebsocketport", 8646);
    bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    if(!StartGethNode(exePath, gethPID, bGethTestnet, wsport))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not start Geth"));
    if(!StartRelayerNode(exePath, relayerPID, bGethTestnet, wsport))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop relayer"));
    
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", "success");
    return ret;
}
UniValue syscoinsetethstatus(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoinsetethstatus",
                "\nSets ethereum syncing and network status for indication status of network sync.\n",
                {
                    {"syncing_status", RPCArg::Type::STR, RPCArg::Optional::NO, "Sycning status ether 'syncing' or 'synced'"},
                    {"highest_block", RPCArg::Type::NUM, RPCArg::Optional::NO, "What the highest block height on Ethereum is found to be.  Usually coupled with syncing_status of 'syncing'.  Set to 0 if sync_status is 'synced'"}
                },
                RPCResult{
                "{\n"
                "    \"status\": xx     (string) Result\n"
                "}\n"
                },
                RPCExamples{
                    HelpExampleCli("syscoinsetethstatus", "\"syncing\" 7000000")
                    + HelpExampleCli("syscoinsetethstatus", "\"synced\" 0")
                    + HelpExampleRpc("syscoinsetethstatus", "\"syncing\", 7000000")
                    + HelpExampleRpc("syscoinsetethstatus", "\"synced\", 0")
                }
                }.ToString());
    string status = params[0].get_str();
    int highestBlock = params[1].get_int();
    const uint32_t nGethOldHeight = fGethCurrentHeight;
    UniValue ret(UniValue::VOBJ);
    UniValue retArray(UniValue::VARR);
    if(highestBlock > 0){
        if(!pethereumtxrootsdb->PruneTxRoots(highestBlock))
        {
            LogPrintf("Failed to write to prune Ethereum TX Roots database!\n");
            ret.__pushKV("missing_blocks", retArray);
            return ret;
        }
    }
    std::vector<std::pair<uint32_t, uint32_t> > vecMissingBlockRanges;
    pethereumtxrootsdb->AuditTxRootDB(vecMissingBlockRanges);
    fGethSyncStatus = status; 
    if(!fGethSynced && fGethSyncStatus == "synced" && vecMissingBlockRanges.empty())  {     
        fGethSynced = true;
    }
   
    for(const auto& range: vecMissingBlockRanges){
        UniValue retRange(UniValue::VOBJ);
        retRange.__pushKV("from", (int)range.first);
        retRange.__pushKV("to", (int)range.second);
        retArray.push_back(retRange);
    }
    LogPrint(BCLog::SYS, "syscoinsetethstatus old height %d new height %d\n", nGethOldHeight, fGethCurrentHeight);
    ret.__pushKV("missing_blocks", retArray);
    if(fZMQEthStatus){
        UniValue oEthStatus(UniValue::VOBJ);
        oEthStatus.__pushKV("geth_sync_status",  fGethSyncStatus);
        oEthStatus.__pushKV("geth_total_blocks",  (int)fGethSyncHeight);
        oEthStatus.__pushKV("geth_current_block",  (int)fGethCurrentHeight);
        oEthStatus.push_back(ret);
        GetMainSignals().NotifySyscoinUpdate(oEthStatus.write().c_str(), "ethstatus");
    }    
    return ret;
}
UniValue syscoinsetethheaders(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoinsetethheaders",
                "\nSets Ethereum headers in Syscoin to validate transactions through the SYSX bridge.\n",
                {
                    {"headers", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of arrays (block number, tx root) from Ethereum blockchain", 
                        {
                            {"", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "An array of [block number, tx root] ",
                                {
                                    {"block_number", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The block height number"},
                                    {"block_hash", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Hash of the block"},
                                    {"previous_hash", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Hash of the previous block"},
                                    {"tx_root", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The Ethereum TX root of the block height"},
                                    {"receipt_root", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The Ethereum TX Receipt root of the block height"}
                                }
                            }
                        },
                        "[blocknumber, blockhash, previoushash, txroot, txreceiptroot] ..."
                    }
                },
                RPCResult{
                "{\n"
                "    \"status\": xx     (string) Result\n"
                "}\n"
                },
                RPCExamples{
                    HelpExampleCli("syscoinsetethheaders", "\"[[7043888,\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\"],...]\"")
                    + HelpExampleRpc("syscoinsetethheaders", "\"[[7043888,\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\"],...]\"")
                }
            }.ToString());  

    EthereumTxRootMap txRootMap;       
    const UniValue &headerArray = params[0].get_array();
    
    for(size_t i =0;i<headerArray.size();i++){
        EthereumTxRoot txRoot;
        const UniValue &tupleArray = headerArray[i].get_array();
        if(tupleArray.size() != 5)
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Invalid size in a blocknumber/txroots input, should be size of 5"));
        const uint32_t &nHeight = (uint32_t)tupleArray[0].get_int();
        string blockHash = tupleArray[1].get_str();
        boost::erase_all(blockHash, "0x");  // strip 0x
        txRoot.vchBlockHash = ParseHex(blockHash);
        string prevHash = tupleArray[2].get_str();
        boost::erase_all(prevHash, "0x");  // strip 0x
        txRoot.vchPrevHash = ParseHex(prevHash);
        string txRootStr = tupleArray[3].get_str();
        boost::erase_all(txRootStr, "0x");  // strip 0x
        txRoot.vchTxRoot = ParseHex(txRootStr);
        string txReceiptRoot = tupleArray[4].get_str();
        boost::erase_all(txReceiptRoot, "0x");  // strip 0x
        txRoot.vchReceiptRoot = ParseHex(txReceiptRoot);
        txRootMap.emplace(std::piecewise_construct,  std::forward_as_tuple(nHeight),  std::forward_as_tuple(txRoot));
    } 
    bool res = pethereumtxrootsdb->FlushWrite(txRootMap);
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", res? "success": "fail");
    return ret;
}
 
UniValue syscoingettxroots(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{"syscoingettxroot",
            "\nGet Ethereum transaction and receipt roots based on block height.\n",
            {
                {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block height to lookup."}
            },
            RPCResult{
                "{\n"
                "  \"txroot\" : \"hash\",        (string) The transaction merkle root\n"
                "  \"receiptroot\" : \"hash\",        (string) The receipt merkle root\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("syscoingettxroots", "23232322")
                + HelpExampleRpc("syscoingettxroots", "23232322")
            }
            }.ToString());
    }
    int nHeight = request.params[0].get_int();
    std::pair<std::vector<unsigned char>,std::vector<unsigned char>> vchTxRoots;
    EthereumTxRoot txRootDB;
    if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(nHeight, txRootDB)){
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read transaction roots");
    }
      
    UniValue ret(UniValue::VOBJ);  
    ret.pushKV("blockhash", HexStr(txRootDB.vchBlockHash)); 
    ret.pushKV("prevhash", HexStr(txRootDB.vchPrevHash)); 
    ret.pushKV("txroot", HexStr(txRootDB.vchTxRoot));
    ret.pushKV("receiptroot", HexStr(txRootDB.vchReceiptRoot));
    
    return ret;
} 

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------
    { "syscoin",            "syscoingettxroots",                &syscoingettxroots,             {"height"} },
    { "syscoin",            "getblockhashbytxid",               &getblockhashbytxid,            {"txid"} },
    { "syscoin",            "syscoingetspvproof",               &syscoingetspvproof,            {"txid","blockhash"} },
    { "syscoin",            "convertaddress",                   &convertaddress,                {"address"} },
    { "syscoin",            "syscoindecoderawtransaction",      &syscoindecoderawtransaction,   {}},
    { "syscoin",            "addressbalance",                   &addressbalance,                {}},
    { "syscoin",            "assetinfo",                        &assetinfo,                     {"asset_guid"}},
    { "syscoin",            "listassets",                       &listassets,                    {"count","from","options"} },
    { "syscoin",            "assetallocationinfo",              &assetallocationinfo,           {"asset_guid"}},
    { "syscoin",            "assetallocationbalance",           &assetallocationbalance,        {"asset_guid"}},
    { "syscoin",            "assetallocationbalances",          &assetallocationbalances,       {"asset_guid","addresses"} },
    { "syscoin",            "assetallocationsenderstatus",      &assetallocationsenderstatus,   {"asset_guid"}},
    { "syscoin",            "listassetallocations",             &listassetallocations,          {"count","from","options"} },
    { "syscoin",            "listassetallocationmempoolbalances",             &listassetallocationmempoolbalances,          {"count","from","options"} },
    { "syscoin",            "listassetindex",                   &listassetindex,                {"page","options"} },
    { "syscoin",            "listassetindexassets",             &listassetindexassets,          {"address"} },
    { "syscoin",            "listassetindexallocations",        &listassetindexallocations,     {"address"} },
    { "syscoin",            "tpstestinfo",                      &tpstestinfo,                   {} },
    { "syscoin",            "tpstestadd",                       &tpstestadd,                    {"starttime","rawtxs"} },
    { "syscoin",            "tpstestsetenabled",                &tpstestsetenabled,             {"enabled"} },
    { "syscoin",            "syscoinsetethstatus",              &syscoinsetethstatus,           {"syncing_status","highestBlock"} },
    { "syscoin",            "syscoinsetethheaders",             &syscoinsetethheaders,          {"headers"} },
    { "syscoin",            "syscoinstopgeth",                  &syscoinstopgeth,               {} },
    { "syscoin",            "syscoinstartgeth",                 &syscoinstartgeth,              {} },
};
// clang-format on
void RegisterAssetRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
