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
#include <chrono>

using namespace std::chrono;
using namespace std;
extern mongoc_collection_t *assetallocation_collection;
extern mongoc_collection_t *aliastxhistory_collection;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend = false, bool transferAlias = false);
static sorted_vector<CAssetAllocationTuple> assetAllocationConflicts;
bool IsAssetAllocationOp(int op) {
	return op == OP_ASSET_ALLOCATION_SEND;
}
string CAssetAllocationTuple::ToString() const {
	return stringFromVch(vchAsset) + "-" + stringFromVch(vchAlias);
}
uint64_t GetAssetAllocationExpiration(const CAssetAllocation& assetallocation) {
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(assetallocation.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;

	return nTime;
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
void CAssetAllocationDB::WriteAssetAllocationIndex(const CAssetAllocation& assetallocation) {
	if (!assetallocation_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);

	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(CAssetAllocationTuple(assetallocation.vchAsset, assetallocation.vchAlias).ToString().c_str()));
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
void CAssetAllocationDB::EraseAssetAllocationIndex(const CAssetAllocationTuple& assetAllocationTuple, bool cleanup) {
	if (!assetallocation_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(assetAllocationTuple.ToString().c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(assetallocation_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB ASSET ALLOCATION REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
bool CAssetAllocationDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAssetAllocation txPos;
	pair<string, CAssetAllocationTuple > key;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			if (pcursor->GetKey(key) && key.first == "assetallocationi") {
				if (!GetAssetAllocation(key.second, txPos) || chainActive.Tip()->GetMedianTimePast() >= GetAssetAllocationExpiration(txPos))
				{
					servicesCleaned++;
					EraseAssetAllocation(key.second, true);
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
bool GetAssetAllocation(const CAssetAllocationTuple &assetAllocationTuple,
        CAssetAllocation& txPos) {
    if (!passetallocationdb || !passetallocationdb->ReadAssetAllocation(assetAllocationTuple, txPos))
        return false;
	if (chainActive.Tip()->GetMedianTimePast() >= GetAssetAllocationExpiration(txPos)) {
		txPos.SetNull();
		return false;
	}
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
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;
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
bool RemoveAssetAllocationScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeAssetAllocationScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}
// revert allocation to previous state and remove 
bool RevertAssetAllocation(const CAssetAllocationTuple &assetAllocationToRemove, const uint256 &txHash, sorted_vector<CAssetAllocationTuple> &revertedAssetAllocations) {
	paliasdb->EraseAliasIndexTxHistory(txHash.GetHex() + "-" + assetAllocationToRemove.ToString());
	// only revert asset allocation once
	if (revertedAssetAllocations.find(assetAllocationToRemove) != revertedAssetAllocations.end())
		return true;

	string errorMessage = "";
	CAssetAllocation dbAssetAllocation;
	if (!passetallocationdb->ReadLastAssetAllocation(assetAllocationToRemove, dbAssetAllocation)) {
		dbAssetAllocation.SetNull();
		dbAssetAllocation.vchAlias = assetAllocationToRemove.vchAlias;
		dbAssetAllocation.vchAsset = assetAllocationToRemove.vchAsset;
	}
	LogPrintf("RevertAssetAllocations %s\n", assetAllocationToRemove.ToString().c_str());
	// write the state back to previous state
	if (!passetallocationdb->WriteAssetAllocation(dbAssetAllocation, INT64_MAX, false))
	{
		errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to write to asset allocation DB");
		return error(errorMessage.c_str());
	}
	passetallocationdb->EraseISArrivalTimes(assetAllocationToRemove);
	revertedAssetAllocations.insert(assetAllocationToRemove);
	sorted_vector<CAssetAllocationTuple>::const_iterator it = assetAllocationConflicts.find(assetAllocationToRemove);
	if (it != assetAllocationConflicts.end())
		assetAllocationConflicts.V.erase(it);
	return true;
	
}
bool CheckAssetAllocationInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const std::vector<unsigned char> &vchAlias,
        bool fJustCheck, int nHeight, sorted_vector<CAssetAllocationTuple> &revertedAssetAllocations, string &errorMessage, bool dontaddtodb) {
	if (!paliasdb || !passetallocationdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add assetallocation in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !dontaddtodb)
		LogPrintf("*** ASSET ALLOCATION %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

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
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}		
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}		
	}

	CAliasIndex alias;
	string retError = "";
	if(fJustCheck)
	{
		switch (op) {
		case OP_ASSET_ALLOCATION_SEND:
			if (theAssetAllocation.listSendingAllocationInputs.empty() && theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset send must send an input or transfer balance");
				return error(errorMessage.c_str());
			}
			break;

		default:
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	const CAssetAllocationTuple assetAllocationTuple(theAssetAllocation.vchAsset, vchAlias);
	const string &user3 = "";
	const string &user2 = "";
	const string &user1 = stringFromVch(vchAlias);
	string strResponseEnglish = "";
	string strResponse = GetSyscoinTransactionDescription(op, strResponseEnglish, ASSETALLOCATION);
	CAssetAllocation dbAssetAllocation;
	CAsset dbAsset;
	bool bRevert = false;
	bool bBalanceOverrun = false;
	bool bAddAllReceiversToConflictList = false;
	if (op == OP_ASSET_ALLOCATION_SEND)
	{
		if (!dontaddtodb) {
			bRevert = !fJustCheck;
			if (bRevert) {
				if (!RevertAssetAllocation(assetAllocationTuple, tx.GetHash(), revertedAssetAllocations))
				{
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to revert asset allocation");
					return error(errorMessage.c_str());
				}
			}
		}
		if (!GetAssetAllocation(assetAllocationTuple, dbAssetAllocation))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find sender asset allocation.");
			return true;
		}
		theAssetAllocation.vchAlias = vchAlias;
		theAssetAllocation.nBalance = dbAssetAllocation.nBalance;
		// get sender assetallocation
		// if no custom allocations are sent with request
			// if sender assetallocation has custom allocations, break as invalid assetsend request
			// ensure sender balance >= balance being sent
			// ensure balance being sent >= minimum divisible quantity
				// if minimum divisible quantity is 0, ensure the balance being sent is a while quantity
			// deduct balance from sender and add to receiver(s) in loop
		// if custom allocations are sent with index numbers in an array
			// loop through array of allocations that are sent along with request
				// get qty of allocation
				// get receiver assetallocation allocation if exists through receiver alias/assetallocation id tuple key
				// check the sender has the allocation in senders allocation list, remove from senders allocation list
				// add allocation to receivers allocation list
				// deduct qty from sender and add to receiver
				// commit receiver details to database using  through receiver alias/assetallocation id tuple as key
		// commit sender details to database
		if (dbAssetAllocation.vchAlias != vchAlias)
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot send this asset. Asset allocation owner must sign off on this change");
			return true;
		}
		if (theAssetAllocation.listSendingAllocationInputs.empty()) {
			if (!dbAssetAllocation.listAllocationInputs.empty()) {
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, request not sending with inputs and sender uses inputs in its allocation list");
				return true;
			}
			if (theAssetAllocation.listSendingAllocationAmounts.empty()) {
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, expected allocation amounts");
				return true;
			}
			// check balance is sufficient on sender
			CAmount nTotal = 0;
			for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, amountTuple.first);
				// one of the first things we do per receiver is revert it to last pow state on the pow(!fJustCheck)
				if (bRevert) {
					if (!RevertAssetAllocation(receiverAllocationTuple, tx.GetHash(), revertedAssetAllocations))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to revert asset allocation");
						return error(errorMessage.c_str());
					}
				}
				nTotal += amountTuple.second;
				if (amountTuple.second <= 0)
				{
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Receiving amount must be positive");
					return true;
				}
			}
			if (dbAssetAllocation.nBalance < nTotal) {
				errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Sender balance is insufficient");
				if (fJustCheck) {
					// add conflicting sender
					assetAllocationConflicts.insert(assetAllocationTuple);
					bBalanceOverrun = true;
				}
				return true;
			}
			else if (fJustCheck) {
				// if sender was is flagged as conflicting, add all receivers to conflict list
				if (assetAllocationConflicts.find(assetAllocationTuple) != assetAllocationConflicts.end())
				{
					bAddAllReceiversToConflictList = true;
				}
			}
			for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				CAssetAllocation receiverAllocation;
				if (amountTuple.first == vchAlias) {
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Cannot send an asset allocation to yourself");
					continue;
				}
				const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, amountTuple.first);
				if (fJustCheck) {
					// check receiver alias
					if (!GetAlias(amountTuple.first, alias))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to");
						continue;
					}
					if (!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_ASSETS))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("An alias you are transferring to does not accept assets");
						continue;
					}
					if (bAddAllReceiversToConflictList || bBalanceOverrun) {
						assetAllocationConflicts.insert(receiverAllocationTuple);
					}
					
				}
				
				if (!dontaddtodb) {
					if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {
						receiverAllocation.SetNull();
						receiverAllocation.vchAlias = receiverAllocationTuple.vchAlias;
						receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
					}
					if (!bBalanceOverrun) {
						receiverAllocation.txHash = tx.GetHash();
						receiverAllocation.nHeight = nHeight;
						receiverAllocation.nBalance += amountTuple.second;
						theAssetAllocation.nBalance -= amountTuple.second;
					}
					
					if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, INT64_MAX, fJustCheck))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset allocation DB");
						return error(errorMessage.c_str());
					}
					
					if (fJustCheck) {
						if (strResponse != "") {
							paliasdb->WriteAliasIndexTxHistory(user1, stringFromVch(receiverAllocation.vchAlias), user3, tx.GetHash(), nHeight, strResponseEnglish, receiverAllocationTuple.ToString());
						}
					}
				}
			}
		}
	}

	// write assetallocation  
	if (!dontaddtodb) {
		// set the assetallocation's txn-dependent 
		if (!bBalanceOverrun) {
			theAssetAllocation.nHeight = nHeight;
			theAssetAllocation.txHash = tx.GetHash();
		}
		
		int64_t ms = INT64_MAX;
		if (fJustCheck) {
			ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}
		if (!passetallocationdb->WriteAssetAllocation(theAssetAllocation, ms, fJustCheck))
		{
			errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset allocation DB");
			return error(errorMessage.c_str());
		}
		// debug
		if (fDebug)
			LogPrintf("CONNECTED ASSET ALLOCATION: op=%s assetallocation=%s hash=%s height=%d fJustCheck=%d at time %lld\n",
				assetFromOp(op).c_str(),
				assetAllocationTuple.ToString().c_str(),
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : 0, (long long)ms);
	}
    return true;
}
UniValue assetallocationsend(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 5)
		throw runtime_error(
			"assetallocationsend [asset] [aliasfrom] aliasto amount [witness]\n"
			"Send an asset allocation you own to another alias.\n"
			"<asset> Asset name.\n"
			"<aliasfrom> alias to transfer from.\n"
			"<aliasto> alias to transfer to.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAliasFrom = vchFromValue(params[1]);
	vector<unsigned char> vchAliasTo = vchFromValue(params[2]);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);
	// check for alias existence in DB
	CAliasIndex fromAlias;
	if (!GetAlias(vchAliasFrom, fromAlias))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyFromOrig;

	CAsset theAsset;
	if (!GetAsset(vchAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CScript scriptPubKey;
	CAssetAllocation theAssetAllocation;
	theAssetAllocation.vchAsset = vchAsset;
	theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(vchAliasTo, AmountFromValue(params[3])));

	CAssetAllocationTuple assetAllocationTuple(vchAsset, vchAliasFrom);
	if (!GetBoolArg("-unittest", false)) {
		// check to see if a transaction for this asset/alias tuple has arrived before minimum latency period
		ArrivalTimesMap arrivalTimes;
		passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
		const int64_t & nNow = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
		for (auto& arrivalTime : arrivalTimes) {
			// if this tx arrived within the minimum latency period flag it as potentially conflicting
			if ((nNow - (arrivalTime.second / 1000)) < ZDAG_MINIMUM_LATENCY_SECONDS) {
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("Please wait a few more seconds and try again..."));
			}
		}
	}
	if (assetAllocationConflicts.find(assetAllocationTuple) != assetAllocationConflicts.end())
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("This asset allocation is involved in a conflict which must be resolved with Proof-Of-Work. Please wait for a block confirmation and try again..."));
	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET_ALLOCATION) << CScript::EncodeOP_N(OP_ASSET_ALLOCATION_SEND) << vchHashAsset << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyFromOrig;
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
    if (fHelp || 2 != params.size())
        throw runtime_error("assetallocationinfo <asset> <alias>\n"
                "Show stored values of a single asset allocation.\n");

    vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	UniValue oAssetAllocation(UniValue::VOBJ);

	CAssetAllocation txPos;
	if (!passetallocationdb || !passetallocationdb->ReadAssetAllocation(CAssetAllocationTuple(vchAsset, vchAlias), txPos))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from assetallocation DB"));

	if(!BuildAssetAllocationJson(txPos, oAssetAllocation))
		oAssetAllocation.clear();
    return oAssetAllocation;
}
bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, UniValue& oAssetAllocation)
{
	CAssetAllocationTuple assetAllocationTuple(assetallocation.vchAsset, assetallocation.vchAlias);
    oAssetAllocation.push_back(Pair("_id", assetAllocationTuple.ToString()));
	oAssetAllocation.push_back(Pair("asset", stringFromVch(assetallocation.vchAsset)));
    oAssetAllocation.push_back(Pair("txid", assetallocation.txHash.GetHex()));
    oAssetAllocation.push_back(Pair("height", (int)assetallocation.nHeight));
	ArrivalTimesMap arrivalTimes;
	passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
	ArrivalTimesMap::iterator it = arrivalTimes.find(assetallocation.txHash);
	int64_t nArrivalTime = INT64_MAX;
	if (it != arrivalTimes.end())
		nArrivalTime = (*it).second;
	oAssetAllocation.push_back(Pair("time", nArrivalTime));
	oAssetAllocation.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	oAssetAllocation.push_back(Pair("balance", ValueFromAmount(assetallocation.nBalance)));
	int64_t expired_time = GetAssetAllocationExpiration(assetallocation);
	bool expired = false;
	if (expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}
	oAssetAllocation.push_back(Pair("expires_on", expired_time));
	oAssetAllocation.push_back(Pair("expired", expired));
	int nStatus = ZDAG_STATUS_OK;
	if (assetAllocationConflicts.find(assetAllocationTuple) != assetAllocationConflicts.end())
		nStatus = ZDAG_MAJOR_CONFLICT_OK;
	oAssetAllocation.push_back(Pair("status", nStatus));
	return true;
}
bool BuildAssetAllocationIndexerJson(const CAssetAllocation& assetallocation, UniValue& oAssetAllocation)
{
	oAssetAllocation.push_back(Pair("_id", CAssetAllocationTuple(assetallocation.vchAsset, assetallocation.vchAlias).ToString()));
	oAssetAllocation.push_back(Pair("txid", assetallocation.txHash.GetHex()));
	oAssetAllocation.push_back(Pair("asset", stringFromVch(assetallocation.vchAsset)));
	oAssetAllocation.push_back(Pair("height", (int)assetallocation.nHeight));
	oAssetAllocation.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	oAssetAllocation.push_back(Pair("balance", ValueFromAmount(assetallocation.nBalance)));
	return true;
}
void AssetAllocationTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = assetFromOp(op);
	CAssetAllocation assetallocation;
	if(!assetallocation.UnserializeFromData(vchData, vchHash))
		return;
	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", CAssetAllocationTuple(assetallocation.vchAsset, assetallocation.vchAlias).ToString()));
	entry.push_back(Pair("asset", stringFromVch(assetallocation.vchAsset)));
	entry.push_back(Pair("alias", stringFromVch(assetallocation.vchAlias)));
	UniValue oAssetAllocationReceiversArray(UniValue::VARR);
	if (!assetallocation.listSendingAllocationAmounts.empty()) {
		for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
			UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
			oAssetAllocationReceiversObj.push_back(Pair("alias", stringFromVch(amountTuple.first)));
			oAssetAllocationReceiversObj.push_back(Pair("amount", ValueFromAmount(amountTuple.second)));
			oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
		}
		
	}
	else if (!assetallocation.listSendingAllocationInputs.empty()) {

	}
	entry.push_back(Pair("allocation_amounts", oAssetAllocationReceiversArray));


}




