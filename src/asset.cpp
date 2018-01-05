// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "asset.h"
#include "assetallocation.h"
#include "alias.h"
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
extern mongoc_collection_t *asset_collection;
extern mongoc_collection_t *assethistory_collection;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend=false, bool transferAlias=false);
bool IsAssetOp(int op) {
    return op == OP_ASSET_ACTIVATE
		|| op == OP_ASSET_MINT
        || op == OP_ASSET_UPDATE
        || op == OP_ASSET_TRANSFER
		|| op == OP_ASSET_SEND;
}

uint64_t GetAssetExpiration(const CAsset& asset) {
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(asset.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;

	return nTime;
}

string assetFromOp(int op) {
    switch (op) {
    case OP_ASSET_ACTIVATE:
        return "assetactivate";
    case OP_ASSET_UPDATE:
        return "assetupdate";
	case OP_ASSET_MINT:
		return "assetmint";
    case OP_ASSET_TRANSFER:
        return "assettransfer";
	case OP_ASSET_SEND:
		return "assetsend";
    default:
        return "<unknown asset op>";
    }
}
bool CAsset::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
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
bool CAsset::UnserializeFromTx(const CTransaction &tx) {
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
void CAsset::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
void CAssetDB::WriteAssetIndex(const CAsset& asset, const int& op) {
	if (!asset_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);

	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(asset.vchAsset).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildAssetIndexerJson(asset, oName)) {
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(asset_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB ASSET UPDATE ERROR: %s\n", error.message);
		}
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
	WriteAssetIndexHistory(asset, op);
}
void CAssetDB::WriteAssetIndexHistory(const CAsset& asset, const int &op) {
	if (!assethistory_collection)
		return;
	bson_error_t error;
	bson_t *insert = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildAssetIndexerHistoryJson(asset, oName)) {
		oName.push_back(Pair("op", assetFromOp(op)));
		insert = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!insert || !mongoc_collection_insert(assethistory_collection, (mongoc_insert_flags_t)MONGOC_INSERT_NO_VALIDATE, insert, write_concern, &error)) {
			LogPrintf("MONGODB ASSET HISTORY ERROR: %s\n", error.message);
		}
	}

	if (insert)
		bson_destroy(insert);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAssetDB::EraseAssetIndexHistory(const std::vector<unsigned char>& vchAsset, bool cleanup) {
	if (!assethistory_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("asset", BCON_UTF8(stringFromVch(vchAsset).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(assethistory_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB ASSET HISTORY REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAssetDB::EraseAssetIndexHistory(const std::string& id) {
	if (!assethistory_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(id.c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(assethistory_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB ASSET HISTORY REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAssetDB::EraseAssetIndex(const std::vector<unsigned char>& vchAsset, bool cleanup) {
	if (!asset_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(vchAsset).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(asset_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB ASSET REMOVE ERROR: %s\n", error.message);
	}
	
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
	EraseAssetIndexHistory(vchAsset, cleanup);
}
bool CAssetDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAsset txPos;
	pair<string, vector<unsigned char> > key;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			if (pcursor->GetKey(key) && key.first == "asseti") {
				if (!GetAsset(key.second, txPos) || chainActive.Tip()->GetMedianTimePast() >= GetAssetExpiration(txPos))
				{
					servicesCleaned++;
					EraseAsset(key.second, true);
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
bool GetAsset(const vector<unsigned char> &vchAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
        return false;
	if (chainActive.Tip()->GetMedianTimePast() >= GetAssetExpiration(txPos)) {
		txPos.SetNull();
		return false;
	}
    return true;
}
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	CAsset asset;
	bool decode = DecodeAssetTx(tx, op, nOut, vvch);
	bool parse = asset.UnserializeFromTx(tx);
	if (decode&&parse) {
		type = ASSET;
		return true;
	}
	return false;
}
bool DecodeAssetTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeAssetScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DecodeAssetScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
    if (!script.GetOp(pc, opcode)) return false;
    if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);
	if (op != OP_SYSCOIN_ASSET)
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
bool DecodeAssetScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeAssetScript(script, op, vvch, pc);
}
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeAssetScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}

bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs,
        bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (!paliasdb || !passetdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add asset in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !dontaddtodb)
		LogPrintf("*** ASSET %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

	// unserialize asset from txn, check for valid
	CAsset theAsset;
	CAssetAllocation theAssetAllocation;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || (op != OP_ASSET_SEND &&!theAsset.UnserializeFromData(vchData, vchHash)) || (op == OP_ASSET_SEND && !theAssetAllocation.UnserializeFromData(vchData, vchHash)))
	{
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a asset");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
			
	}
	
	CAliasIndex alias;
	string retError = "";
	if(fJustCheck)
	{
		if (op != OP_ASSET_SEND) {
			if (theAsset.sCategory.size() > MAX_NAME_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Asset category too big");
				return error(errorMessage.c_str());
			}
			if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Asset public data too big");
				return error(errorMessage.c_str());
			}
		}
		switch (op) {
		case OP_ASSET_ACTIVATE:
			if (theAsset.vchAsset.size() > MAX_GUID_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("asset hex guid too long");
				return error(errorMessage.c_str());
			}
			if((theAsset.vchName.size() > MAX_ID_LENGTH || theAsset.vchName.empty()))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2012 - " + _("Asset title too big or is empty");
				return error(errorMessage.c_str());
			}
			if(!boost::algorithm::starts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Must use a asset category");
				return true;
			}
			break;

		case OP_ASSET_UPDATE:
			if(!theAsset.vchName.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Asset name cannot be changed");
				return error(errorMessage.c_str());
			}
			if(theAsset.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Must use a asset category");
				return true;
			}
			if (theAsset.nBalance < 0)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Balance must be greator than or equal to 0");
				return error(errorMessage.c_str());
			}
			break;

		case OP_ASSET_TRANSFER:
			break;
		case OP_ASSET_SEND:
			if (theAssetAllocation.listSendingAllocationInputs.empty() && theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset send must send an input or transfer balance");
				return error(errorMessage.c_str());
			}
			break;
		default:
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	const string &user1 = stringFromVch(vvchAliasArgs[0]);
	string user2 = "";
	string user3 = "";
	if (op == OP_ASSET_TRANSFER) {
		user2 = stringFromVch(theAsset.vchAlias);
	}
	string strResponseEnglish = "";
	string strResponse = GetSyscoinTransactionDescription(op, strResponseEnglish, ASSET);
	char nLockStatus = NOLOCK_UNCONFIRMED_STATE;
	if (!fJustCheck)
		nLockStatus = NOLOCK_CONFIRMED_STATE;
	CAsset dbAsset;
	if (!GetAsset(op == OP_ASSET_SEND? theAssetAllocation.vchAsset: theAsset.vchAsset, dbAsset))
	{
		if (op != OP_ASSET_ACTIVATE) {
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to read from asset DB");
			return true;
		}
	}
	// allocation send from asset requires pow, no 0-conf
	else if(op != OP_ASSET_SEND)
	{
		bool bSendLocked = false;
		passetdb->ReadISLock(theAsset.vchAsset, bSendLocked);
		if (!fJustCheck && bSendLocked) {
			if (dbAsset.nHeight >= nHeight)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height.");
				return true;
			}
			if (dbAsset.txHash != tx.GetHash())
			{
				if (fDebug)
					LogPrintf("ASSET txid mismatch! Recreating...\n");
				const string &txHashHex = dbAsset.txHash.GetHex();

				// recreate this asset tx from last known good position (last asset stored)
				if (op != OP_ASSET_ACTIVATE && !passetdb->ReadLastAsset(theAsset.vchAsset, dbAsset)) {
					dbAsset.SetNull();
				}
				if(!dontaddtodb){
					nLockStatus = LOCK_CONFLICT_CONFIRMED_STATE;
					if (!passetdb->EraseISLock(theAsset.vchAsset))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from asset DB");
						return error(errorMessage.c_str());
					}
					paliasdb->EraseAliasIndexTxHistory(txHashHex+"-"+stringFromVch(theAsset.vchAsset));
					passetdb->EraseAssetIndexHistory(txHashHex);
				}
			}
			else {
				if (!dontaddtodb) {
					nLockStatus = LOCK_NOCONFLICT_CONFIRMED_STATE;
					if (fDebug)
						LogPrintf("CONNECTED ASSET: op=%s asset=%s hash=%s height=%d fJustCheck=%d POW IS\n",
							assetFromOp(op).c_str(),
							stringFromVch(theAsset.vchAsset).c_str(),
							tx.GetHash().ToString().c_str(),
							nHeight,
							fJustCheck ? 1 : 0);
					if (!passetdb->Write(make_pair(std::string("assetp"), theAsset.vchAsset), dbAsset))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to write previous asset to asset DB");
						return error(errorMessage.c_str());
					}
					if (!passetdb->EraseISLock(theAsset.vchAsset))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from asset DB");
						return error(errorMessage.c_str());
					}
					if (strResponse != "") {
						paliasdb->UpdateAliasIndexTxHistoryLockStatus(tx.GetHash().GetHex() + "-" + stringFromVch(theAsset.vchAsset), nLockStatus);
					}
				}
				return true;
			}
		}
		else
		{
			if (fJustCheck && bSendLocked && dbAsset.nHeight >= nHeight && dbAsset.txHash != tx.GetHash())
			{
				if (!dontaddtodb) {
					nLockStatus = LOCK_CONFLICT_UNCONFIRMED_STATE;
					if (strResponse != "") {
						paliasdb->UpdateAliasIndexTxHistoryLockStatus(tx.GetHash().GetHex() + "-" + stringFromVch(theAsset.vchAsset), nLockStatus);
					}
				}
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height");
				return true;
			}
			if (dbAsset.nHeight > nHeight)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request cannot be lower than stored service block height");
				return true;
			}
			if (fJustCheck)
				nLockStatus = LOCK_NOCONFLICT_UNCONFIRMED_STATE;
		}
	}
	if (op == OP_ASSET_UPDATE || op == OP_ASSET_TRANSFER || op == OP_ASSET_SEND)
	{
		if (dbAsset.vchAlias != vvchAliasArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this asset. Asset owner must sign off on this change");
			return true;
		}
	}
	if (op == OP_ASSET_UPDATE) {
		theAsset.nBalance = dbAsset.nBalance + theAsset.nBalance;
		if (theAsset.nTotalSupply > 0 && theAsset.nBalance > theAsset.nTotalSupply)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Balance cannot exceed total supply");
			return true;
		}
	}
	else if(op != OP_ASSET_ACTIVATE)
		theAsset.nBalance = dbAsset.nBalance;

	if (op == OP_ASSET_SEND) {
		theAsset = dbAsset;

		CAssetAllocation dbAssetAllocation;
		const CAssetAllocationTuple allocationTuple(theAssetAllocation.vchAsset, vvchAliasArgs[0]);
		GetAssetAllocation(allocationTuple, dbAssetAllocation);
		if (theAssetAllocation.listSendingAllocationInputs.empty()) {
			if (!dbAssetAllocation.listAllocationInputs.empty()) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, request not sending with inputs and sender uses inputs in its allocation list");
				return true;
			}
			if (theAssetAllocation.listSendingAllocationAmounts.empty()) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, expected allocation amounts");
				return true;
			}
			// check balance is sufficient on sender
			CAmount nTotal = 0;
			for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				nTotal += amountTuple.second;
				if (amountTuple.second <= 0)
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Receiving amount must be positive");
					return true;
				}
			}
			if (theAsset.nBalance < nTotal) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Sender balance is insufficient");
				return true;
			}
			// adjust sender balance
			theAsset.nBalance -= nTotal;
			for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				CAssetAllocation receiverAllocation;
				const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, amountTuple.first);
				// don't need to check for existance of allocation because it may not exist, may be creating it here for the first time for receiver
				GetAssetAllocation(receiverAllocationTuple, receiverAllocation);

				// check receiver alias
				if (!GetAlias(amountTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to.");
					continue;
				}
				if (!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_ASSETS))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("An alias you are transferring to does not accept assets");
					continue;
				}

				// only POW can actually adjust the receiver qty (fJustCheck == false) when sending from asset to asset allocation, from asset allocation to asset allocation 0-conf is allowed
				if (!dontaddtodb && !fJustCheck) {
					if (receiverAllocation.IsNull()) {
						receiverAllocation.vchAlias = receiverAllocationTuple.vchAlias;
						receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
					}
					
					receiverAllocation.nBalance += amountTuple.second;
					receiverAllocation.nHeight = nHeight;
					receiverAllocation.txHash = tx.GetHash();
					// we know the receiver update is not a double spend so we lock it in with false meaning we should store previous db entry with this one
					if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, op, fJustCheck))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset allocation DB");
						continue;
					}
					if (strResponse != "") {
						paliasdb->WriteAliasIndexTxHistory(user1, stringFromVch(receiverAllocation.vchAlias), user3, tx.GetHash(), nHeight, strResponseEnglish, receiverAllocationTuple.ToString(), nLockStatus);
					}
				}
			}
		}
	}
	else if (op != OP_ASSET_ACTIVATE)
	{
		if (theAsset.vchAlias.empty())
			theAsset.vchAlias = dbAsset.vchAlias;
		if (theAsset.vchPubData.empty())
			theAsset.vchPubData = dbAsset.vchPubData;
		theAsset.vchName = dbAsset.vchName;
		if (theAsset.sCategory.empty())
			theAsset.sCategory = dbAsset.sCategory;

		theAsset.nTotalSupply = dbAsset.nTotalSupply;

		if (op == OP_ASSET_TRANSFER)
		{
			// check toalias
			if (!GetAlias(theAsset.vchAlias, alias))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to");
				return true;
			}
			if (!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_ASSETS))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("The alias you are transferring to does not accept assets");
				return true;
			}
		}
	}
	if (op == OP_ASSET_ACTIVATE)
	{
		if (fJustCheck && GetAsset(theAsset.vchAsset, theAsset))
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Asset already exists");
			return true;
		}
	}
	if (!dontaddtodb) {
		if (strResponse != "") {
			paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theAsset.vchAsset), nLockStatus);
		}
	}
	// set the asset's txn-dependent values
	theAsset.nHeight = nHeight;
	theAsset.txHash = tx.GetHash();
	// write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
	if (!dontaddtodb && (op != OP_ASSET_SEND || !fJustCheck)) {
		if (!passetdb->WriteAsset(theAsset, op, fJustCheck))
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset DB");
			return error(errorMessage.c_str());
		}
		// debug
		if (fDebug)
			LogPrintf("CONNECTED ASSET: op=%s asset=%s hash=%s height=%d fJustCheck=%d\n",
				assetFromOp(op).c_str(),
				stringFromVch(op == OP_ASSET_SEND? theAssetAllocation.vchAsset: theAsset.vchAsset).c_str(),
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : 0);
	}
    return true;
}

UniValue assetnew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 7)
        throw runtime_error(
			"assetnew [alias] [name] [public] [category=assets] [balance] [total_supply] [witness]\n"
						"<alias> An alias you own.\n"
						"<name> name, 20 characters max.\n"
                        "<public> public data, 256 characters max.\n"
						"<category> category, 256 characters max. Defaults to assets\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
    vector<unsigned char> vchName = vchFromString(params[1].get_str());
	vector<unsigned char> vchPubData = vchFromString(params[2].get_str());
	string strCategory = "assets";
	strCategory = params[3].get_str();
	vector<unsigned char> vchWitness;
	CAmount nBalance = AmountFromValue(params[4]);
	CAmount nTotalSupply = AmountFromValue(params[5]);
	vchWitness = vchFromValue(params[6]);
	// check for alias existence in DB
	CAliasIndex theAlias;

	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2500 - " + _("failed to read alias from alias DB"));

	
    // gather inputs
	vector<unsigned char> vchAsset = vchFromString(GenerateSyscoinGuid());
    // this is a syscoin transaction
    CWalletTx wtx;

    CScript scriptPubKeyOrig;
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);


    CScript scriptPubKey,scriptPubKeyAlias;

	// calculate net
    // build asset object
    CAsset newAsset;
	newAsset.vchAsset = vchAsset;
	newAsset.sCategory = vchFromString(strCategory);
	newAsset.vchName = vchName;
	newAsset.vchPubData = vchPubData;
	newAsset.vchAlias = vchAlias;
	newAsset.nBalance = nBalance;
	newAsset.nTotalSupply = nTotalSupply;

	vector<unsigned char> data;
	newAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_ACTIVATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	res.push_back(stringFromVch(vchAsset));
	return res;
}

UniValue assetupdate(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 5)
        throw runtime_error(
			"assetupdate [guid] [public] [category=assets] [balance] [witness]\n"
						"Perform an update on an asset you control.\n"
						"<guid> Asset guidkey.\n"
                        "<public> Public data, 256 characters max.\n"                
						"<category> Category, 256 characters max. Defaults to assets\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAsset = vchFromValue(params[0]);

	string strData = "";
	string strPubData = "";
	string strCategory = "";
	strPubData = params[1].get_str();
	strCategory = params[2].get_str();
	CAmount nBalance = AmountFromValue(params[3]);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);
    // this is a syscoind txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig;
	CAsset theAsset;
	
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2504 - " + _("Could not find a asset with this key"));

	CAliasIndex theAlias;

	if (!GetAlias(theAsset.vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2505 - " + _("Failed to read alias from alias DB"));

	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

    // create ASSETUPDATE txn keys
    CScript scriptPubKey;

	if(strPubData != stringFromVch(theAsset.vchPubData))
		theAsset.vchPubData = vchFromString(strPubData);
	if(strCategory != stringFromVch(theAsset.sCategory))
		theAsset.sCategory = vchFromString(strCategory);

	theAsset.nBalance = nBalance;

	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_UPDATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
 	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue assettransfer(const UniValue& params, bool fHelp) {
 if (fHelp || params.size() != 3)
        throw runtime_error(
			"assettransfer [guid] [alias] [witness]\n"
						"Transfer a asset allocation you own to another alias.\n"
						"<guid> asset guidkey.\n"
						"<alias> alias to transfer to.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());

    // gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	// check for alias existence in DB
	CAliasIndex toAlias;
	if (!GetAlias(vchAlias, toAlias))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

    // this is a syscoin txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

	CAsset theAsset;
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CAliasIndex fromAlias;
	if(!GetAlias(theAsset.vchAlias, fromAlias))
	{
		 throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2511 - " + _("Could not find the asset alias"));
	}

	CSyscoinAddress sendAddr;
	GetAddress(toAlias, &sendAddr, scriptPubKeyOrig);
	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();
    CScript scriptPubKey;
	theAsset.vchAlias = toAlias.vchAlias;


	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_TRANSFER) << vchHashAsset << OP_2DROP << OP_DROP;
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
UniValue assetsend(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 5)
		throw runtime_error(
			"assetallocationsend [guid] [alias] aliasto amount [witness]\n"
			"Send an asset allocation you own to another alias.\n"
			"<guid> asset guidkey.\n"
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
	CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

	CAsset theAsset;
	if (!GetAsset(vchAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CScript scriptPubKey;
	CAssetAllocation theAssetAllocation;
	theAssetAllocation.vchAsset = vchAsset;
	theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(vchAliasTo, AmountFromValue(params[3])));

	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_SEND) << vchHashAsset << OP_2DROP << OP_DROP;
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

UniValue assetinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 > params.size())
        throw runtime_error("assetinfo <guid>\n"
                "Show stored values of a single asset and its .\n");

    vector<unsigned char> vchAsset = vchFromValue(params[0]);
	UniValue oAsset(UniValue::VOBJ);

	CAsset txPos;
	if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from asset DB"));

	if(!BuildAssetJson(txPos, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetJson(const CAsset& asset, UniValue& oAsset)
{
    oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
    oAsset.push_back(Pair("txid", asset.txHash.GetHex()));
    oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight) {
		CBlockIndex *pindex = chainActive[asset.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("name", stringFromVch(asset.vchName)));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	oAsset.push_back(Pair("balance", ValueFromAmount(asset.nBalance)));
	int64_t expired_time = GetAssetExpiration(asset);
	bool expired = false;
	if (expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}


	oAsset.push_back(Pair("expires_on", expired_time));
	oAsset.push_back(Pair("expired", expired));
	return true;
}
bool BuildAssetIndexerHistoryJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", asset.txHash.GetHex()));
	oAsset.push_back(Pair("asset", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight) {
		CBlockIndex *pindex = chainActive[asset.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("title", stringFromVch(asset.vchName)));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	return true;
}
bool BuildAssetIndexerJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("title", stringFromVch(asset.vchName)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	return true;
}
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = assetFromOp(op);
	CAsset asset;
	if(!asset.UnserializeFromData(vchData, vchHash))
		return;

	CAsset dbAsset;
	GetAsset(asset.vchAsset, dbAsset);
	

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(asset.vchAsset)));

	if(!asset.vchName.empty() && asset.vchName != dbAsset.vchName)
		entry.push_back(Pair("title", stringFromVch(asset.vchName)));

	if(!asset.vchPubData.empty() && asset.vchPubData != dbAsset.vchPubData)
		entry.push_back(Pair("publicdata", stringFromVch(asset.vchPubData)));

	if(!asset.vchAlias.empty() && asset.vchAlias != dbAsset.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(asset.vchAlias)));


}




