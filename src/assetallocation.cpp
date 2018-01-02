// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assetallocation.h"
#include "alias.h"
#include "asset.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include "coincontrol.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <mongoc.h>
using namespace std;
extern mongoc_collection_t *assetallocation_collection;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend=false, bool transferAlias=false);
bool IsAssetAllocationOp(int op) {
    return op == OP_ASSET_ALLOCATION_SEND
}



string assetAllocationFromOp(int op) {
    switch (op) {
	case OP_ASSET_ALLOCATION_SEND:
		return "assetallocationsend";
    default:
        return "<unknown assetallocation op>";
    }
}
bool CAssetAllocation::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsAsset >> *this;

		vector<unsigned char> vchAssetData;
		Serialize(vchAssetData);
		const uint256 &calculatedHash = Hash(vchAssetData.begin(), vchAssetData.end());
		const vector<unsigned char> &vchRandAsset = vchFromValue(calculatedHash.GetHex());
		if(vchRandAsset != vchHash)
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
bool CAssetAllocation::UnserializeFromTx(const CTransaction &tx) {
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
void CAssetAllocation::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
void CAssetAllocationDB::WriteAssetAllocationIndex(const CAssetAllocation& assetallocation, const int& op) {
	if (!assetallocation_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);

	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(assetallocation.vchAssetAllocation).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildAssetAllocationIndexerJson(assetallocation, oName)) {
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(assetallocation_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB ASSET ALLOCATION UPDATE ERROR: %s\n", error.message);
		}
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
bool GetAssetAllocation(const vector<unsigned char> &vchAssetAllocation,
        CAssetAllocation& txPos) {
    if (!passetallocationdb->ReadAssetAllocation(vchAssetAllocation, txPos))
        return false;
    return true;
}
bool DecodeAndParseAssetAllocationTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	CAssetAllocation assetallocation;
	bool decode = DecodeAssetAllocationTx(tx, op, nOut, vvch);
	bool parse = assetallocation.UnserializeFromTx(tx);
	if (decode&&parse) {
		type = ASSETALLOCATION;
		return true;
	}
	return false;
}
bool DecodeAssetAllocationTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeAssetAllocationScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DecodeAssetAllocationScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
    if (!script.GetOp(pc, opcode)) return false;
    if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);
	if (op != OP_SYSCOIN_ASSET_ALLOCATION)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsAssetOp(op))
		return false;

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
	return found;
}
bool DecodeAssetAllocationScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeAssetAllocationScript(script, op, vvch, pc);
}
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeAssetAllocationScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}

bool CheckAssetAllocationInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs,
        bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add assetallocation in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !dontaddtodb)
		LogPrintf("*** ASSET ALLOCATION %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");
	int prevAliasOp = 0;
    // Make sure assetallocation outputs are not spent by a regular transaction, or the assetallocation would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION) 
	{
		errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	vector<vector<unsigned char> > vvchPrevAliasArgs;
	// unserialize assetallocation from txn, check for valid
	CAssetAllocation theAssetAllocation;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theAssetAllocation.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a assetallocation");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int pop;
			CCoins prevCoins;
			// ensure inputs are unspent when doing consensus check to add to block
			if (!GetUTXOCoins(tx.vin[i].prevout, prevCoins) || !IsSyscoinScript(prevCoins.vout[tx.vin[i].prevout.n].scriptPubKey, pop, vvch))
			{
				continue;
			}

			else if (IsAliasOp(pop) && vvch.size() >= 2 && theAssetAllocation.vchLinkAlias == vvch[0] && theAssetAllocation.vchLinkAlias.third == vvch[1])
			{
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
				break;
			}
		}
			
	}

	
	CAliasIndex alias;
	string retError = "";
	if(fJustCheck)
	{
		if (vvchArgs.empty() ||  vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Asset hex guid too long");
			return error(errorMessage.c_str());
		}
		if(theAssetAllocation.sCategory.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Asset category too big");
			return error(errorMessage.c_str());
		}
		if(theAssetAllocation.vchPubData.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Asset public data too big");
			return error(errorMessage.c_str());
		}
		if(!theAssetAllocation.vchAssetAllocation.empty() && theAssetAllocation.vchAssetAllocation != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2008 - " + _("Guid in data output doesn't match guid in transaction");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_ASSET_ALLOCATION_SEND:
			if (theAssetAllocation.vchAssetAllocation != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2018 - " + _("Asset guid mismatch");
				return error(errorMessage.c_str());
			}
			if (!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;

		default:
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	const string &user2 = stringFromVch(theAssetAllocation.vchAlias);
	const string &user1 = stringFromVch(theAssetAllocation.vchLinkAlias);
	const CAssetAllocationTuple assetAllocationTuple(vvchArgs[0], theAssetAllocation.vchAlias);
	string user3 = "";
	CAssetAllocation dbAssetAllocation;
	CAsset dbAsset;
	if (GetAssetAllocation(vvchArgs[0], dbAssetAllocation))
		bool bSendLocked = false;
		if (!fJustCheck && passetallocationdb->ReadISLock(assetAllocationTuple, bSendLocked) && bSendLocked) {
			if (dbAssetAllocation.nHeight >= nHeight)
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height.");
				return true;
			}
			if (dbAssetAllocation.txHash != tx.GetHash())
			{
				if (fDebug)
					LogPrintf("ASSET ALLOCATION txid mismatch! Recreating...\n");
				const string &txHashHex = dbAssetAllocation.txHash.GetHex();
				//vector<string> lastReceiverList = dbAssetAllocation.listReceivers;
				// recreate this assetallocation tx from last known good position (last assetallocation stored)
				if (!passetallocationdb->ReadLastAssetAllocation(assetAllocationTuple, theAssetAllocation)) {
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1048 - " + _("Failed to read last escrow from escrow DB");
					return true;
				}
				// deal with assetallocation send reverting
				if (op == OP_ASSET_ALLOCATION_SEND) {

				}
				if (!dontaddtodb && !passetallocationdb->EraseISLock(assetAllocationTuple))
				{
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from assetallocation DB");
					return error(errorMessage.c_str());
				}
				if (!dontaddtodb) {
					paliasdb->EraseAliasIndexTxHistory(txHashHex);
				}
			}
			else {
				if (!dontaddtodb) {
					if (fDebug)
						LogPrintf("CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d POW IS\n",
							assetFromOp(op).c_str(),
							stringFromVch(vvchArgs[0]).c_str(),
							tx.GetHash().ToString().c_str(),
							nHeight,
							fJustCheck ? 1 : 0);
					if (!passetallocationdb->EraseISLock(assetAllocationTuple))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from assetallocation DB");
						return error(errorMessage.c_str());
					}
				}
				return true;
			}
		}
		else if (dbAssetAllocation.nHeight > nHeight)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request cannot be lower than stored service block height.");
			return true;
		}
	}
	if (op == OP_ASSET_ALLOCATION_SEND)
	{
		if (fJustCheck && GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Asset allocation already exists");
			return true;
		}
		// check toalias
		if (!GetAlias(theAssetAllocation.vchAlias, alias))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to.");
			return true;
		}
		if (!GetAsset(theAssetAllocation.vchAsset, dbAsset))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find asset related to this allocation.");
			return true;
		}
		// if transfering from asset allocation
		if (dbAssetAllocation.vchAlias != theAssetAllocation.vchLinkAlias && dbAsset.vchAlias != theAssetAllocation.vchLinkAlias)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot send this assetallocation. Asset allocation owner must sign off on this change.");
			return true;
		}
		if (!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_ASSETS))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("The alias you are transferring to does not accept assets");
			return true;
		}
		// get sender assetallocation
		// if no custom allocations are sent with request
			// if sender assetallocation has custom allocations, break as invalid assetsend request
			// ensure sender balance >= balance being sent
			// ensure balance being sent >= minimum divisible quantity
				// if minimum divisible quantity is 0, ensure the balance being sent is a while quantity
			// deduct balance from sender and add to receiver
		// if custom allocations are sent with index numbers in an array
			// loop through array of allocations that are sent along with request
				// get qty of allocation
				// get receiver assetallocation allocation if exists through receiver alias/assetallocation id tuple key
				// check the sender has the allocation in senders allocation list, remove from senders allocation list
				// add allocation to receivers allocation list
				// deduct qty from sender and add to receiver
				// commit receiver details to database using  through receiver alias/assetallocation id tuple as key
		// commit sender details to database
		// return
	}
	if (!dontaddtodb) {
		string strResponseEnglish = "";
		string strResponseGUID = "";
		string strResponse = GetSyscoinTransactionDescription(op, vvchArgs, strResponseEnglish, strResponseGUID, ASSET);
		if (strResponse != "") {
			paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, strResponseGUID);
		}
	}
	// set the assetallocation's txn-dependent values
	theAssetAllocation.nHeight = nHeight;
	theAssetAllocation.txHash = tx.GetHash();
	// write assetallocation  
	if (!dontaddtodb) {
		if (!passetallocationdb->WriteAssetAllocation(theAssetAllocation, dbAssetAllocation, op, fJustCheck))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to assetifcate DB");
			return error(errorMessage.c_str());
		}
		// debug
		if (fDebug)
			LogPrintf("CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d\n",
				assetFromOp(op).c_str(),
				stringFromVch(vvchArgs[0]).c_str(),
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : 0);
	}
    return true;
}
UniValue assetallocationsend(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 3)
		throw runtime_error(
			"assetallocationsend [guid] [aliasfrom] {\"alias\":amount,\"n\":number...} [witness]\n"
			"Send an asset allocation you own to another alias.\n"
			"<guid> asset guidkey.\n"
			"<aliasfrom> alias to transfer from.\n"
			"<aliasto> alias to transfer to.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchAssetAllocation = vchFromValue(params[0]);
	vector<unsigned char> vchAliasFrom = vchFromValue(params[1]);

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	// check for alias existence in DB
	CAliasIndex fromAlias;
	if (!GetAlias(vchAliasFrom, fromAlias))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

	CAssetAllocation theAssetAllocation;
	if (!GetAssetAllocation(CAssetAllocationTuple(vchAssetAllocation, vchAliasFrom), theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));


	CSyscoinAddress sendAddr;
	GetAddress(toAlias, &sendAddr, scriptPubKeyOrig);
	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CAssetAllocation copyAsset = theAssetAllocation;
	theAssetAllocation.ClearAsset();
	CScript scriptPubKey;
	theAssetAllocation.nHeight = chainActive.Tip()->nHeight;
	theAssetAllocation.vchLinkAlias = fromAlias.vchAlias;
	theAssetAllocation.vchAlias = toAlias.vchAlias;


	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET_ALLOCATION) << CScript::EncodeOP_N(OP_ASSET_ALLOCATION_SEND) << vchAssetAllocation << vchHashAsset << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;
	// send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyFromOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyFromOrig, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(fromAlias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue assetallocationinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 > params.size())
        throw runtime_error("assetallocationinfo <guid>\n"
                "Show stored values of a single asset allocation.\n");

    vector<unsigned char> vchAssetAllocation = vchFromValue(params[0]);
	UniValue oAsset(UniValue::VOBJ);
    vector<unsigned char> vchValue;

	CAssetAllocation txPos;
	if (!GetAssetAllocation(vchAssetAllocation, txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from assetallocation DB"));

	if(!BuildAssetAllocationJson(txPos, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, UniValue& oAsset)
{
    oAsset.push_back(Pair("_id", stringFromVch(assetallocation.vchAssetAllocation)));
    oAsset.push_back(Pair("txid", assetallocation.txHash.GetHex()));
    oAsset.push_back(Pair("height", (int64_t)assetallocation.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= assetallocation.nHeight) {
		CBlockIndex *pindex = chainActive[assetallocation.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("name", stringFromVch(assetallocation.vchName)));
	oAsset.push_back(Pair("publicvalue", stringFromVch(assetallocation.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(assetallocation.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	return true;
}
bool BuildAssetAllocationIndexerHistoryJson(const CAssetAllocation& assetallocation, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", assetallocation.txHash.GetHex()));
	oAsset.push_back(Pair("asset", stringFromVch(assetallocation.vchAssetAllocation)));
	oAsset.push_back(Pair("height", (int64_t)assetallocation.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= assetallocation.nHeight) {
		CBlockIndex *pindex = chainActive[assetallocation.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("title", stringFromVch(assetallocation.vchName)));
	oAsset.push_back(Pair("publicvalue", stringFromVch(assetallocation.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(assetallocation.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	return true;
}
bool BuildAssetAllocationIndexerJson(const CAssetAllocation& assetallocation, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", stringFromVch(assetallocation.vchAssetAllocation)));
	oAsset.push_back(Pair("title", stringFromVch(assetallocation.vchName)));
	oAsset.push_back(Pair("height", (int)assetallocation.nHeight));
	oAsset.push_back(Pair("category", stringFromVch(assetallocation.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	return true;
}
void AssetAllocationTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = assetFromOp(op);
	CAssetAllocation assetallocation;
	if(!assetallocation.UnserializeFromData(vchData, vchHash))
		return;

	CAssetAllocation dbAssetAllocation;
	GetAssetAllocation(assetallocation.vchAssetAllocation, dbAssetAllocation);
	

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(assetallocation.vchAssetAllocation)));

	if(!assetallocation.vchName.empty() && assetallocation.vchName != dbAssetAllocation.vchName)
		entry.push_back(Pair("title", stringFromVch(assetallocation.vchName)));

	if(!assetallocation.vchPubData.empty() && assetallocation.vchPubData != dbAssetAllocation.vchPubData)
		entry.push_back(Pair("publicdata", stringFromVch(assetallocation.vchPubData)));

	if(!assetallocation.vchLinkAlias.empty() && assetallocation.vchLinkAlias != dbAssetAllocation.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(assetallocation.vchLinkAlias)));
	else if(assetallocation.vchAlias != dbAssetAllocation.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));

}




