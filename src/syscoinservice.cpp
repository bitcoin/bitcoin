#include "service.h"
#include "alias.h"
#include "offer.h"
#include "init.h"
#include "main.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include "messagecrypter.h"
#include "coincontrol.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
using namespace std;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=false, bool transferAlias=false);
void PutToServiceList(std::vector<CSyscoinService> &serviceList, CSyscoinService& index) {
	int i = serviceList.size() - 1;
	BOOST_REVERSE_FOREACH(CSyscoinService &o, serviceList) {
        if(!o.txid.IsNull() && o.txid == index.txid) {
        	serviceList[i] = index;
            return;
        }
        i--;
	}
    serviceList.push_back(index);
}
bool IsServiceOp(int op) {
    return op == OP_SERVICE_ACTIVATE
        || op == OP_SERVICE_UPDATE
        || op == OP_SERVICE_TRANSFER;
}

uint64_t GetServiceExpiration(const CSyscoinService& service) {
	uint64_t nTime = chainActive.Tip()->nTime + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(service.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;
	
	return nTime;
}


string serviceFromOp(int op) {
    switch (op) {
    case OP_SERVICE_ACTIVATE:
        return "serviceactivate";
    case OP_SERVICE_UPDATE:
        return "serviceupdate";
    case OP_SERVICE_TRANSFER:
        return "servicetransfer";
    default:
        return "<unknown service op>";
    }
}
bool CSyscoinService::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsService(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsService >> *this;

		vector<unsigned char> vchServiceData;
		Serialize(vchServiceData);
		const uint256 &calculatedHash = Hash(vchServiceData.begin(), vchServiceData.end());
		const vector<unsigned char> &vchRandService = vchFromValue(calculatedHash.GetHex());
		if(vchRandService != vchHash)
		{
			SetNull();
			return false;
		}

    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
bool CSyscoinService::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData, vchHash))
	{	
		return false;
	}
    return true;
}
void CSyscoinService::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsService(SER_NETWORK, PROTOCOL_VERSION);
    dsService << *this;
	vchData = vector<unsigned char>(dsService.begin(), dsService.end());

}
bool CSyscoinServiceDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CSyscoinService> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "servicei") {
            	const vector<unsigned char> &vchMyService= key.second;         
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty()){
					servicesCleaned++;
					EraseService(vchMyService);
					pcursor->Next();
					continue;
				}
				const CSyscoinService &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= GetServiceExpiration(txPos))
				{
					servicesCleaned++;
					EraseService(vchMyService);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
bool CSyscoinServiceDB::GetDBServices(std::vector<CSyscoinService>& services, const uint64_t &nExpireFilter, const std::vector<std::string>& aliasArray)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CSyscoinService> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "servicei") {       
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty())
				{
					pcursor->Next();
					continue;
				}
				const CSyscoinService &txPos = vtxPos.back();
				if(chainActive.Height() <= txPos.nHeight || chainActive[txPos.nHeight]->nTime < nExpireFilter)
				{
					pcursor->Next();
					continue;
				}
  				if (chainActive.Tip()->nTime >= GetServiceExpiration(txPos))
				{
					pcursor->Next();
					continue;
				}
				if(aliasArray.size() > 0)
				{
					if (std::find(aliasArray.begin(), aliasArray.end(), stringFromVch(txPos.vchAlias)) == aliasArray.end())
					{
						pcursor->Next();
						continue;
					}
				}
				services.push_back(txPos);	
            }
			
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
int IndexOfServiceOutput(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
    vector<vector<unsigned char> > vvch;
	int op;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		// find an output you own
		if (pwalletMain->IsMine(out) && DecodeServiceScript(out.scriptPubKey, op, vvch)) {
			return i;
		}
	}
	return -1;
}

bool GetTxOfService(const vector<unsigned char> &vchService,
        CSyscoinService& txPos, CTransaction& tx, bool skipExpiresCheck) {
    vector<CSyscoinService> vtxPos;
    if (!pservicedb->ReadService(vchService, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (!skipExpiresCheck && chainActive.Tip()->nTime >= GetServiceExpiration(txPos)) {
        string service = stringFromVch(vchService);
        LogPrintf("GetTxOfService(%s) : expired", service.c_str());
        return false;
    }

    if (!GetSyscoinTransaction(nHeight, txPos.txid, tx, Params().GetConsensus()))
        return error("GetTxOfService() : could not read tx from disk");

    return true;
}

bool GetTxAndVtxOfService(const vector<unsigned char> &vchService,
        CSyscoinService& txPos, CTransaction& tx,  vector<CSyscoinService> &vtxPos, bool skipExpiresCheck) {
    if (!pservicedb->ReadService(vchService, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (!skipExpiresCheck && chainActive.Tip()->nTime >= GetServiceExpiration(txPos)) {
        string service = stringFromVch(vchService);
        LogPrintf("GetTxOfService(%s) : expired", service.c_str());
        return false;
    }

    if (!GetSyscoinTransaction(nHeight, txPos.txid, tx, Params().GetConsensus()))
        return error("GetTxOfService() : could not read tx from disk");

    return true;
}
bool GetVtxOfService(const vector<unsigned char> &vchService,
        CSyscoinService& txPos, vector<CSyscoinService> &vtxPos, bool skipExpiresCheck) {
    if (!pservicedb->ReadService(vchService, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (!skipExpiresCheck && chainActive.Tip()->nTime >= GetServiceExpiration(txPos)) {
        string service = stringFromVch(vchService);
        LogPrintf("GetTxOfService(%s) : expired", service.c_str());
        return false;
    }

    return true;
}
bool DecodeAndParseServiceTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	CSyscoinService service;
	bool decode = DecodeServiceTx(tx, op, nOut, vvch);
	bool parse = service.UnserializeFromTx(tx);
	return decode && parse;
}
bool DecodeServiceTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeServiceScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DecodeServiceScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
    if (!script.GetOp(pc, opcode)) return false;
    if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);

	bool found = false;
	for (;;) {
		vector<unsigned char> vch;
		if (!script.GetOp(pc, opcode, vch))
			return false;
		if (opcode == OP_DROP || opcode == OP_2DROP)
		{
			found = true;
			break;
		}
		if (!(opcode >= 0 && opcode <= OP_PUSHDATA4))
			return false;
		vvch.push_back(vch);
	}

	// move the pc to after any DROP or NOP
	while (opcode == OP_DROP || opcode == OP_2DROP) {
		if (!script.GetOp(pc, opcode))
			break;
	}

	pc--;
	return found && IsServiceOp(op);
}
bool DecodeServiceScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeServiceScript(script, op, vvch, pc);
}
bool RemoveServiceScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeServiceScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}

bool CheckServiceInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs,
        const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add service in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug)
		LogPrintf("*** SERVICE %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");
	bool foundAlias = false;
    const COutPoint *prevOutput = NULL;
    const CCoins *prevCoins;

	int prevAliasOp = 0;
    // Make sure service outputs are not spent by a regular transaction, or the service would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION) 
	{
		errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	vector<vector<unsigned char> > vvchPrevAliasArgs;
	// unserialize service from txn, check for valid
	CSyscoinService theService;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theService.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a service");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Service arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
			
		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int pop;
			prevOutput = &tx.vin[i].prevout;
			if(!prevOutput)
				continue;
			// ensure inputs are unspent when doing consensus check to add to block
			prevCoins = inputs.AccessCoins(prevOutput->hash);
			if(prevCoins == NULL)
				continue;
			if(prevCoins->vout.size() <= prevOutput->n || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch) || pop == OP_ALIAS_PAYMENT)
				continue;
			if(foundAlias)
				break;
			else if (!foundAlias && IsAliasOp(pop))
			{
				foundAlias = true; 
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}
	}


	
	CAliasIndex alias;
	CTransaction aliasTx;
	vector<CSyscoinService> vtxPos;
	string retError = "";
	if(fJustCheck)
	{
		if (vvchArgs.empty() ||  vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Service hex guid too long");
			return error(errorMessage.c_str());
		}
		if(theService.vchPrivateData.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2006 - " + _("Service private data too big");
			return error(errorMessage.c_str());
		}
		if(theService.vchPublicData.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Service public data too big");
			return error(errorMessage.c_str());
		}
		if(!theService.vchService.empty() && theService.vchService != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2008 - " + _("Guid in data output doesn't match guid in transaction");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_SERVICE_ACTIVATE:
			if (theService.vchService != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2009 - " + _("Service guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!theService.vchLinkAlias.empty())
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2010 - " + _("Service linked alias not allowed in activate");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theService.vchAlias != vvchPrevAliasArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2011 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;

		case OP_SERVICE_UPDATE:
			if (theService.vchService != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2014 - " + _("Service guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theService.vchAlias != vvchPrevAliasArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2016 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;

		case OP_SERVICE_TRANSFER:
			if (theService.vchService != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2018 - " + _("Service guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theService.vchAlias != vvchPrevAliasArgs[0])
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;

		default:
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Service transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}

    if (!fJustCheck ) {
		if(op != OP_SERVICE_ACTIVATE) 
		{
			// if not an servicenew, load the service data from the DB
			CSyscoinService dbService;
			if(!GetVtxOfService(vvchArgs[0], dbService, vtxPos))	
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to read from service DB");
				return true;
			}
			if(theService.vchPrivateData.empty())
				theService.vchPrivateData = dbService.vchPrivateData;
			if(theService.vchPublicData.empty())
				theService.vchPublicData = dbService.vchPublicData;
			
			// user can't update safety level after creation
			theService.safetyLevel = dbService.safetyLevel;
			theService.vchService = dbService.vchService;
			if(theService.vchAlias != dbService.vchAlias)
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2023 - " + _("Wrong alias input provided in this service transaction");
				theService.vchAlias = dbService.vchAlias;
			}
			if(op == OP_SERVICE_TRANSFER)
			{
				theService.vchAlias = theService.vchLinkAlias;		
			}
			else
			{
				theService.bTransferViewOnly = dbService.bTransferViewOnly;
			}
			theService.vchLinkAlias.clear();
			if(dbService.bTransferViewOnly)
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit or transfer this service. It is view-only.");
				theService = dbService;
			}
		}
		else
		{
			if (pservicedb->ExistsService(vvchArgs[0]))
			{
				errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Service already exists");
				return true;
			}
		}
        // set the service's txn-dependent values
		theService.nHeight = nHeight;
		theService.txid = tx.GetHash();
		PutToServiceList(vtxPos, theService);
        // write service  

        if (!dontaddtodb && !pservicedb->WriteService(vvchArgs[0], vtxPos))
		{
			errorMessage = "SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to serviceifcate DB");
            return error(errorMessage.c_str());
		}
		

      			
        // debug
		if(fDebug)
			LogPrintf( "CONNECTED SERVICE: op=%s service=%s hash=%s height=%d\n",
                serviceFromOp(op).c_str(),
                stringFromVch(vvchArgs[0]).c_str(),
                tx.GetHash().ToString().c_str(),
                nHeight);
    }
    return true;
}





UniValue servicenew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
		"servicenew <alias> <public> [private]\n"
						"<alias> An alias you own.\n"
                        "<public> public data, 256 characters max.\n"
						"<private> private data, 256 characters max.\n"	
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchPublicData = vchFromString(params[1].get_str());
	string strPrivateData = "";
	if(CheckParam(params, 2))
		strPrivateData = params[2].get_str();


	// check for alias existence in DB
	CTransaction aliastx;
	CAliasIndex theAlias;

	if (!GetTxOfAlias(vchAlias, theAlias, aliastx))
		throw runtime_error("SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2500 - " + _("failed to read alias from alias DB"));

	
    // gather inputs
	vector<unsigned char> vchService = vchFromString(GenerateSyscoinGuid());
    // this is a syscoin transaction
    CWalletTx wtx;

    CScript scriptPubKeyOrig;
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);


    CScript scriptPubKey,scriptPubKeyAlias;

	// calculate net
    // build service object
    CSyscoinService newService;
	newService.vchService = vchService;
	if(!strPrivateData.empty())
		newService.vchPrivateData = ParseHex(strPrivateData);
	newService.vchPublicData = vchPublicData;
	newService.nHeight = chainActive.Tip()->nHeight;
	newService.vchAlias = vchAlias;
	newService.safetyLevel = 0;

	vector<unsigned char> data;
	newService.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashService = vchFromValue(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SERVICE_ACTIVATE) << vchService << vchHashService << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);

	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(vchAlias, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(so, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(wtx.GetHash().GetHex());
		res.push_back(stringFromVch(vchService));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchService));
		res.push_back("false");
	}
	return res;
}

UniValue serviceupdate(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 5)
        throw runtime_error(
		"serviceupdate <guid> [public] [private]\n"
						"Perform an update on an service you control.\n"
						"<guid> service guidkey.\n"
                        "<public> public data, 256 characters max.\n"
						"<private> private data, 256 characters max.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchService = vchFromValue(params[0]);
	string strPrivateData = "";
	string strPubData = "";
	if(CheckParam(params, 1))
		strPubData = params[1].get_str();
	if(CheckParam(params, 2))
		strPrivateData = params[2].get_str();



    // this is a syscoind txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig;


    // look for a transaction with this key
    CTransaction tx;
	CSyscoinService theService;
	
    if (!GetTxOfService( vchService, theService, tx))
        throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2504 - " + _("Could not find a service with this key"));

	CTransaction aliastx;
	CAliasIndex theAlias;

	if (!GetTxOfAlias(theService.vchAlias, theAlias, aliastx))
		throw runtime_error("SYSCOIN_SERVICE_CONSENSUS_ERROR: ERRCODE: 2505 - " + _("Failed to read alias from alias DB"));

	CSyscoinService copyService = theService;
	theService.ClearService();
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

    // create SERVICEUPDATE txn keys
    CScript scriptPubKey;

	if(!strPrivateData.empty())
		theService.vchPrivateData = ParseHex(strPrivateData);
	if(!strPubData.empty())
		theService.vchPublicData = vchFromString(strPubData);

	theService.vchAlias = theAlias.vchAlias;
	theService.nHeight = chainActive.Tip()->nHeight;
	vector<unsigned char> data;
	theService.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashService = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SERVICE_UPDATE) << vchService << vchHashService << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(theAlias.vchAlias, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);	
 	UniValue res(UniValue::VARR);

	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(so, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
		res.push_back(wtx.GetHash().GetHex());
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}


UniValue servicetransfer(const UniValue& params, bool fHelp) {
 if (fHelp || params.size() < 2 || params.size() > 7)
        throw runtime_error(
		"servicetransfer <guid> <alias> [public] [private] [transfer_access_flags]\n"
						"Transfer a service you own to another alias.\n"
						"<guid> service guidkey.\n"
						"<alias> alias to transfer to.\n"
                        "<public> public data, 256 characters max.\n"
						"<private> private data, 256 characters max.\n"				
						"<transfer_access_flags> Transfer the service under certain conditions that the new owner must adhere to (affect ability to update or transfer).\n"
						+ HelpRequiringPassphrase());

    // gather & validate inputs
	vector<unsigned char> vchService = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);

	string strPrivateData = "";
	string strPubData = "";
	if(CheckParam(params, 2))
		strPubData = params[2].get_str();
	if(CheckParam(params, 3))
		strPrivateData = params[3].get_str();
	if(CheckParam(params, 6))
		strViewOnly = params[6].get_str();


	if(CheckParam(params, 3))
		strViewOnly = params[3].get_str();
	// check for alias existence in DB
	CTransaction tx;
	CAliasIndex toAlias;
	if (!GetTxOfAlias(vchAlias, toAlias, tx))
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

    // this is a syscoin txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

    CTransaction aliastx;
	CSyscoinService theService;
    if (!GetTxOfService( vchService, theService, tx))
        throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a service with this key"));

	CAliasIndex fromAlias;
	if(!GetTxOfAlias(theService.vchAlias, fromAlias, aliastx))
	{
		 throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2511 - " + _("Could not find the service alias"));
	}

	CSyscoinAddress sendAddr;
	GetAddress(toAlias, &sendAddr, scriptPubKeyOrig);
	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CSyscoinService copyService = theService;
	theService.ClearService();
    CScript scriptPubKey;
	theService.nHeight = chainActive.Tip()->nHeight;
	theService.vchAlias = fromAlias.vchAlias;
	theService.vchLinkAlias = toAlias.vchAlias;
	theService.safetyLevel = copyService.safetyLevel;

	if(!strPrivateData.empty())
		theService.vchPrivateData = ParseHex(strPrivateData);
	if(!strPubData.empty())
		theService.vchPublicData = vchFromString(strPubData);


	vector<unsigned char> data;
	theService.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashService = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SERVICE_TRANSFER) << vchService << vchHashService << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
    // send the service pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyFromOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyFromOrig, fromAlias.vchAlias, fromAlias.vchAliasPeg, chainActive.Tip()->nHeight, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fromAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(fromAlias.vchAlias, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);

	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(so, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
		res.push_back(wtx.GetHash().GetHex());
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}


UniValue serviceinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size() || 2 < params.size())
        throw runtime_error("serviceinfo <guid> [walletless=No]\n"
                "Show stored values of a single service and its .\n");

    vector<unsigned char> vchService = vchFromValue(params[0]);

	string strWalletless = "No";
	if(params.size() >= 2)
		strWalletless = params[1].get_str();

	vector<CSyscoinService> vtxPos;

	UniValue oService(UniValue::VOBJ);
    vector<unsigned char> vchValue;

	if (!pservicedb->ReadService(vchService, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2515 - " + _("Failed to read from service DB"));

	CAliasIndex alias;
	CTransaction aliastx;
	if (!GetTxOfAlias(vtxPos.back().vchAlias, alias, aliastx, true))
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2516 - " + _("Failed to read xfer alias from alias DB"));

	if(!BuildServiceJson(vtxPos.back(), alias, oService, strWalletless))
		oService.clear();
    return oService;
}

UniValue servicelist(const UniValue& params, bool fHelp) {
    if (fHelp || 3 < params.size())
        throw runtime_error("servicelist [\"alias\",...] [<service>] [walletless=No]\n"
                "list services that an array of aliases own. Set of aliases to look up based on alias.");
	UniValue aliasesValue(UniValue::VARR);
	vector<string> aliases;
	if(params.size() >= 1)
	{
		if(params[0].isArray())
		{
			aliasesValue = params[0].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
		else
		{
			string aliasName =  params[0].get_str();
			boost::algorithm::to_lower(aliasName);
			if(!aliasName.empty())
				aliases.push_back(aliasName);
		}
	}
	vector<unsigned char> vchNameUniq;
    if (params.size() >= 2 && !params[1].get_str().empty())
        vchNameUniq = vchFromValue(params[1]);

	string strWalletless = "No";
	if(params.size() >= 3)
		strWalletless = params[2].get_str();

	UniValue oRes(UniValue::VARR);
	map< vector<unsigned char>, int > vNamesI;
	map< vector<unsigned char>, UniValue > vNamesO;
	if(aliases.size() > 0)
	{
		for(unsigned int aliasIndex =0;aliasIndex<aliases.size();aliasIndex++)
		{
			const string &name = aliases[aliasIndex];
			const vector<unsigned char> &vchAlias = vchFromString(name);
			vector<CAliasIndex> vtxPos;
			if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
				continue;
			CTransaction tx;
			for(auto& it : boost::adaptors::reverse(vtxPos)) {
				const CAliasIndex& theAlias = it;
				if(!GetSyscoinTransaction(theAlias.nHeight, theAlias.txid, tx, Params().GetConsensus()))
					continue;
				CSyscoinService service(tx);
				if(!service.IsNull())
				{
					if (vNamesI.find(service.vchService) != vNamesI.end() && (theAlias.nHeight <= vNamesI[service.vchService] || vNamesI[service.vchService] < 0))
						continue;
					if (vchNameUniq.size() > 0 && vchNameUniq != service.vchService)
						continue;
					vector<CSyscoinService> vtxServicePos;
					if (!pservicedb->ReadService(service.vchService, vtxServicePos) || vtxServicePos.empty())
						continue;
					const CSyscoinService &theService = vtxServicePos.back();
					if(theService.vchAlias != theAlias.vchAlias)
						continue;
					
					UniValue oService(UniValue::VOBJ);
					vNamesI[service.vchService] = theAlias.nHeight;
					if(BuildServiceJson(theService, theAlias, oService, strWalletless))
					{
						oRes.push_back(oService);
					}
				}	
			}
		}
	}
    return oRes;
}
bool BuildServiceJson(const CSyscoinService& service, const CAliasIndex& alias, UniValue& oService, const string &strWalletless)
{
	if(service.safetyLevel >= SAFETY_LEVEL2)
		return false;
	if(alias.safetyLevel >= SAFETY_LEVEL2)
		return false;
	string sHeight = strprintf("%llu", service.nHeight);
    oService.push_back(Pair("service", stringFromVch(service.vchService)));
    oService.push_back(Pair("txid", service.txid.GetHex()));
    oService.push_back(Pair("height", sHeight));
	string strPrivateData = "";
	
    oService.push_back(Pair("privatevalue", HexStr(service.vchPrivateData)));
	oService.push_back(Pair("publicvalue", stringFromVch(service.vchPublicData)));
	unsigned char safetyLevel = max(service.safetyLevel, alias.safetyLevel );
	oService.push_back(Pair("safetylevel", safetyLevel));

	oService.push_back(Pair("alias", stringFromVch(service.vchAlias)));
	oService.push_back(Pair("access_flags", service.nAccessFlags));
	int64_t expired_time = GetServiceExpiration(service);
	bool expired = false;
    if(expired_time <= chainActive.Tip()->nTime)
	{
		expired = true;
	}  
	int64_t expires_in = expired_time - chainActive.Tip()->nTime;
	if(expires_in < -1)
		expires_in = -1;

	oService.push_back(Pair("expires_in", expires_in));
	oService.push_back(Pair("expires_on", expired_time));
	oService.push_back(Pair("expired", expired));
	return true;
}

UniValue servicehistory(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size() || 2 < params.size())
        throw runtime_error("servicehistory <guid> [walletless=No]\n"
                 "List all stored values of an service.\n");

    vector<unsigned char> vchService = vchFromValue(params[0]);

	string strWalletless = "No";
	if(params.size() >= 2)
		strWalletless = params[1].get_str();

    UniValue oRes(UniValue::VARR);
 
    vector<CSyscoinService> vtxPos;
    if (!pservicedb->ReadService(vchService, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2518 - " + _("Failed to read from service DB"));

	vector<CAliasIndex> vtxAliasPos;
	if (!paliasdb->ReadAlias(vtxPos.back().vchAlias, vtxAliasPos) || vtxAliasPos.empty())
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR: ERRCODE: 2519 - " + _("Failed to read from alias DB"));
	
    CSyscoinService txPos2;
	CAliasIndex alias;
	CTransaction tx;
	vector<vector<unsigned char> > vvch;
	int op, nOut;
    BOOST_FOREACH(txPos2, vtxPos) {
		vector<CAliasIndex> vtxAliasPos;
		if(!paliasdb->ReadAlias(txPos2.vchAlias, vtxAliasPos) || vtxAliasPos.empty())
			continue;
		if (!GetSyscoinTransaction(txPos2.nHeight, txPos2.txid, tx, Params().GetConsensus())) {
			continue;
		}
		if (!DecodeServiceTx(tx, op, nOut, vvch) )
			continue;

		alias.nHeight = txPos2.nHeight;
		alias.GetAliasFromList(vtxAliasPos);

		UniValue oService(UniValue::VOBJ);
		string opName = serviceFromOp(op);
		oService.push_back(Pair("servicetype", opName));
		if(BuildServiceJson(txPos2, alias, oService, strWalletless))
			oRes.push_back(oService);
    }
    
    return oRes;
}
void ServiceTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = serviceFromOp(op);
	CSyscoinService service;
	if(!service.UnserializeFromData(vchData, vchHash))
		return;

	bool isExpired = false;
	vector<CAliasIndex> aliasVtxPos;
	vector<CSyscoinService> serviceVtxPos;
	CTransaction servicetx, aliastx;
	CSyscoinService dbService;
	if(GetTxAndVtxOfService(service.vchService, dbService, servicetx, serviceVtxPos, true))
	{
		dbService.nHeight = service.nHeight;
		dbService.GetServiceFromList(serviceVtxPos);
	}
	CAliasIndex dbAlias;
	if(GetTxAndVtxOfAlias(service.vchAlias, dbAlias, aliastx, aliasVtxPos, isExpired, true))
	{
		dbAlias.nHeight = service.nHeight;
		dbAlias.GetAliasFromList(aliasVtxPos);
	}
	string noDifferentStr = _("<No Difference Detected>");

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("service", stringFromVch(service.vchService)));

	string dataValue = noDifferentStr;
	if(!service.vchPrivateData.empty() && service.vchPrivateData != dbService.vchPrivateData)
		dataValue = HexStr(service.vchPrivateData);

	entry.push_back(Pair("privatevalue", dataValue));

	string dataPubValue = noDifferentStr;
	if(!service.vchPublicData.empty() && service.vchPublicData != dbService.vchPublicData)
		dataPubValue = stringFromVch(service.vchPublicData);

	entry.push_back(Pair("publicvalue", dataPubValue));

	string aliasValue = noDifferentStr;
	if(!service.vchLinkAlias.empty() && service.vchLinkAlias != dbService.vchAlias)
		aliasValue = stringFromVch(service.vchLinkAlias);
	if(service.vchAlias != dbService.vchAlias)
		aliasValue = stringFromVch(service.vchAlias);

	entry.push_back(Pair("alias", aliasValue));


	string transferViewOnlyValue = noDifferentStr;
	if(service.bTransferViewOnly != dbService.bTransferViewOnly)
		transferViewOnlyValue = service.bTransferViewOnly? "Yes": "No";

	entry.push_back(Pair("access_flags", transferViewOnlyValue));


	string safetyLevelValue = noDifferentStr;
	if(service.safetyLevel != dbService.safetyLevel)
		safetyLevelValue = service.safetyLevel;

	entry.push_back(Pair("safetylevel", safetyLevelValue));



}
UniValue servicestats(const UniValue& params, bool fHelp) {
	if (fHelp || 2 < params.size())
		throw runtime_error("servicestats unixtime=0 [\"alias\",...]\n"
				"Show statistics for all non-expired services. Only services created or updated after unixtime are returned. Set of services to look up based on array of aliases passed in. Leave empty for all services.\n");
	vector<string> aliases;
	uint64_t nExpireFilter = 0;
	if(params.size() >= 1)
		nExpireFilter = params[0].get_int64();
	if(params.size() >= 2)
	{
		if(params[1].isArray())
		{
			UniValue aliasesValue = params[1].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
		else
		{
			string aliasName =  params[1].get_str();
			boost::algorithm::to_lower(aliasName);
			if(!aliasName.empty())
				aliases.push_back(aliasName);
		}
	}
	UniValue oServiceStats(UniValue::VOBJ);
	std::vector<CSyscoinService> services;
	if (!pservicedb->GetDBServices(services, nExpireFilter, aliases))
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR ERRCODE: 2521 - " + _("Scan failed"));	
	if(!BuildServiceStatsJson(services, oServiceStats))
		throw runtime_error("SYSCOIN_SERVICE_RPC_ERROR ERRCODE: 2522 - " + _("Could not find this service"));

	return oServiceStats;

}
/* Output some stats about services
	- Total number of services
*/
bool BuildServiceStatsJson(const std::vector<CSyscoinService> &services, UniValue& oServiceStats)
{
	uint32_t totalServices = services.size();
	oServiceStats.push_back(Pair("totalservices", (int)totalServices));
	UniValue oServices(UniValue::VARR);
	BOOST_REVERSE_FOREACH(const CSyscoinService &service, services) {
		UniValue oService(UniValue::VOBJ);
		CAliasIndex alias;
		CTransaction aliastx;
		if (!GetTxOfAlias(service.vchAlias, alias, aliastx, true))
			continue;
		if(!BuildServiceJson(service, alias, oService, "Yes"))
			continue;
		oServices.push_back(oService);
	}
	oServiceStats.push_back(Pair("services", oServices)); 
	return true;
}



