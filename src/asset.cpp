// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "asset.h"
#include "alias.h"
#include "offer.h"
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
		|| op == OP_ASSET_SEND
        || op == OP_ASSET_TRANSFER;
}

uint64_t GetAssetExpiration(const CAsset& asset) {
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(asset.aliasTuple.first, aliasUnprunable) && !aliasUnprunable.IsNull())
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
	case OP_ASSET_SEND:
		return "assetsend";
    case OP_ASSET_TRANSFER:
        return "assettransfer";
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
	pair<string, CNameTXIDTuple > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "asseti") {
				const CNameTXIDTuple &assetTuple = key.second;
  				if (!GetAsset(assetTuple.first, txPos) || chainActive.Tip()->GetMedianTimePast() >= GetAssetExpiration(txPos))
				{
					servicesCleaned++;
					EraseAsset(assetTuple, true);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

bool GetAsset(const CNameTXIDTuple &assetTuple,
	CAsset& txPos) {
	if (!passetdb || !passetdb->ReadAsset(assetTuple, txPos))
		return false;
	return true;
}
bool GetAsset(const vector<unsigned char> &vchAsset,
        CAsset& txPos) {
	uint256 txid;
	if (!passetdb || !passetdb->ReadAssetLastTXID(vchAsset, txid) )
		return false;
    if (!passetdb->ReadAsset(CNameTXIDTuple(vchAsset, txid), txPos))
        return false;
    if (chainActive.Tip()->GetMedianTimePast() >= GetAssetExpiration(txPos)) {
		txPos.SetNull();
        string asset = stringFromVch(vchAsset);
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

bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs,
        bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add asset in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !dontaddtodb)
		LogPrintf("*** ASSET %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");
	int prevAliasOp = 0;
    // Make sure asset outputs are not spent by a regular transaction, or the asset would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION) 
	{
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	vector<vector<unsigned char> > vvchPrevAliasArgs;
	// unserialize asset from txn, check for valid
	CAsset theAsset;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theAsset.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a asset");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
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

			else if (IsAliasOp(pop) && vvch.size() >= 2 && theAsset.aliasTuple.first == vvch[0] && theAsset.aliasTuple.third == vvch[1])
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
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Asset hex guid too long");
			return error(errorMessage.c_str());
		}
		if(theAsset.sCategory.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Asset category too big");
			return error(errorMessage.c_str());
		}
		if(theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Asset public data too big");
			return error(errorMessage.c_str());
		}
		if(!theAsset.vchAsset.empty() && theAsset.vchAsset != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2008 - " + _("Guid in data output doesn't match guid in transaction");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_ASSET_ACTIVATE:
			if (theAsset.vchAsset != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2009 - " + _("Asset guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!theAsset.linkAliasTuple.first.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2010 - " + _("Asset linked alias not allowed in activate");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2011 - " + _("Alias input mismatch");
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
			if (theAsset.vchAsset != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2014 - " + _("Asset guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!theAsset.vchName.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Asset name cannot be changed");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2016 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			if(theAsset.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Must use a asset category");
				return true;
			}
			break;

		case OP_ASSET_TRANSFER:
			if (theAsset.vchAsset != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2018 - " + _("Asset guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;
		case OP_ASSET_SEND:
			if (theAsset.vchAsset != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2018 - " + _("Asset guid mismatch");
				return error(errorMessage.c_str());
			}
			if (!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			break;

		default:
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	const string &user1 = stringFromVch(theAsset.aliasTuple.first);
	string user2 = "";
	string user3 = "";
	if (op == OP_ASSET_TRANSFER) {
		if (!theAsset.linkAliasTuple.IsNull())
			user2 = stringFromVch(theAsset.linkAliasTuple.first);
	}
	CAsset dbAsset;
	if (!GetAsset(vvchArgs[0], dbAsset))
	{
		if (op != OP_ASSET_ACTIVATE) {
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to read from asset DB");
			return true;
		}
	}
	else
	{
		bool bSendLocked = false;
		if (!fJustCheck && passetdb->ReadISLock(vvchArgs[0], bSendLocked) && bSendLocked) {
			if (dbAsset.nHeight >= nHeight)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height.");
				return true;
			}
			if (dbAsset.txHash != tx.GetHash())
			{
				const string &txHashHex = dbAsset.txHash.GetHex();
				//vector<string> lastReceiverList = dbAsset.listReceivers;
				// recreate this asset tx from last known good position (last asset stored)
				if (op != OP_ASSET_ACTIVATE && !passetdb->ReadLastAsset(vvchArgs[0], dbAsset)) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1048 - " + _("Failed to read last escrow from escrow DB");
					return true;
				}
				// deal with asset send reverting
				if (op == OP_ASSET_SEND) {

				}
				if (!dontaddtodb && !passetdb->EraseISLock(vvchArgs[0]))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from asset DB");
					return error(errorMessage.c_str());
				}
				if (!dontaddtodb) {
					paliasdb->EraseAliasIndexTxHistory(txHashHex);
					passetdb->EraseAssetIndexHistory(txHashHex);
				}
			}
			else {
				if (!dontaddtodb) {
					if (fDebug)
						LogPrintf("CONNECTED ASSET: op=%s asset=%s hash=%s height=%d fJustCheck=%d\n",
							assetFromOp(op).c_str(),
							stringFromVch(vvchArgs[0]).c_str(),
							tx.GetHash().ToString().c_str(),
							nHeight,
							fJustCheck ? 1 : 0);
					if (!passetdb->Write(make_pair(std::string("assetp"), vvchArgs[0]), dbAsset))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to write previous asset to asset DB");
						return error(errorMessage.c_str());
					}
					if (!passetdb->EraseISLock(vvchArgs[0]))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from asset DB");
						return error(errorMessage.c_str());
					}
				}
				return true;
			}
		}
		else if (dbAsset.nHeight > nHeight)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request cannot be lower than stored service block height.");
			return true;
		}
	}
	if (op != OP_ASSET_ACTIVATE)
	{
		if (theAsset.vchPubData.empty())
			theAsset.vchPubData = dbAsset.vchPubData;
		theAsset.vchName = dbAsset.vchName;
		if (theAsset.sCategory.empty())
			theAsset.sCategory = dbAsset.sCategory;

		if (op == OP_ASSET_TRANSFER || op == OP_ASSET_SEND)
		{
			// check toalias
			if (!GetAlias(theAsset.linkAliasTuple.first, alias))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to. It may be expired");
				return true;
			}
			if (op == OP_ASSET_SEND && dbAsset.aliasTuple != theAsset.aliasTuple)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot send this asset. Asset allocation owner must sign off on this change.");
				return true;
			}
			// set ownership of asset allocation if sending asset allocation
			if (op == OP_ASSET_SEND)
				theAsset.aliasTuple = theAsset.linkAliasTuple;
			// change asset ownership
			else if (op == OP_ASSET_TRANSFER)
				theAsset.ownerAliasTuple = theAsset.aliasTuple;
			if (!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_ASSETS))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("The alias you are transferring to does not accept assets");
				return true;
			}
		}
		else if (OP_ASSET_SEND) {
			// get sender asset
			// if no custom allocations are sent with request
				// if sender asset has custom allocations, break as invalid assetsend request
				// ensure sender balance >= balance being sent
				// ensure balance being sent >= minimum divisible quantity
					// if minimum divisible quantity is 0, ensure the balance being sent is a while quantity
				// deduct balance from sender and add to receiver
			// if custom allocations are sent with index numbers in an array
				// loop through array of allocations that are sent along with request
					// get qty of allocation
					// get receiver asset allocation if exists through receiver alias/asset id tuple key
					// check the sender has the allocation in senders allocation list, remove from senders allocation list
					// add allocation to receivers allocation list
					// deduct qty from sender and add to receiver
					// commit receiver details to database using  through receiver alias/asset id tuple as key
			// commit sender details to database
			// return
		}
		if (op == OP_ASSET_UPDATE || op == OP_ASSET_TRANSFER)
		{
			if (dbAsset.ownerAliasTuple != theAsset.aliasTuple)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this asset. Asset owner must sign off on this change.");
				return true;
			}
		}
	}
	else
	{
		uint256 txid;
		if (passetdb->ReadAssetLastTXID(vvchArgs[0], txid))
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Asset already exists");
			return true;
		}
		theAsset.aliasTuple = theAsset.ownerAliasTuple;
	}
	if (!dontaddtodb) {
		string strResponseEnglish = "";
		string strResponseGUID = "";
		string strResponse = GetSyscoinTransactionDescription(op, vvchArgs, strResponseEnglish, strResponseGUID, ASSET);
		if (strResponse != "") {
			paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, strResponseGUID);
		}
	}
	theAsset.linkAliasTuple.first.clear();
	// set the asset's txn-dependent values
	theAsset.nHeight = nHeight;
	theAsset.txHash = tx.GetHash();
	// write asset  
	if (!dontaddtodb) {
		if (!passetdb->WriteAsset(theAsset, dbAsset, op, fJustCheck))
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to assetifcate DB");
			return error(errorMessage.c_str());
		}
		// debug
		if (fDebug)
			LogPrintf("CONNECTED ASSET: op=%s asset=%s hash=%s height=%d fJustCheck=%d\n",
				assetFromOp(op).c_str(),
				stringFromVch(vvchArgs[0]).c_str(),
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : 0);
	}
    return true;
}

UniValue assetnew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 5)
        throw runtime_error(
			"assetnew [alias] [name] [public] [category=assets] [witness]\n"
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
	vchWitness = vchFromValue(params[4]);
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
	newAsset.nHeight = chainActive.Tip()->nHeight;
	newAsset.ownerAliasTuple = CNameTXIDTuple(vchAlias, theAlias.txHash, theAlias.vchGUID);

	vector<unsigned char> data;
	newAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_ACTIVATE) << vchAsset << vchHashAsset << OP_2DROP << OP_2DROP;
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
    if (fHelp || params.size() != 4)
        throw runtime_error(
			"assetupdate [guid] [public] [category=assets] [witness]\n"
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

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[3]);
    // this is a syscoind txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig;
	CAsset theAsset;
	
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2504 - " + _("Could not find a asset with this key"));

	CAliasIndex theAlias;

	if (!GetAlias(theAsset.aliasTuple.first, theAlias))
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

	theAsset.nHeight = chainActive.Tip()->nHeight;
	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_UPDATE) << vchAsset << vchHashAsset << OP_2DROP << OP_2DROP;
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
UniValue assetsend(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 3)
		throw runtime_error(
			"assetsend [guid] {\"alias\":amount,\"n\":number...} [witness]\n"
			"Send an asset allocation you own to another alias.\n"
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
	if (!GetAsset(vchAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CAliasIndex fromAlias;
	if (!GetAlias(theAsset.aliasTuple.first, fromAlias))
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
	theAsset.nHeight = chainActive.Tip()->nHeight;
	theAsset.aliasTuple = CNameTXIDTuple(fromAlias.vchAlias, fromAlias.txHash, fromAlias.vchGUID);
	theAsset.linkAliasTuple = CNameTXIDTuple(toAlias.vchAlias, toAlias.txHash, toAlias.vchGUID);


	vector<unsigned char> data;
	theAsset.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_SEND) << vchAsset << vchHashAsset << OP_2DROP << OP_2DROP;
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
	if(!GetAlias(theAsset.aliasTuple.first, fromAlias))
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
	theAsset.nHeight = chainActive.Tip()->nHeight;
	theAsset.aliasTuple = CNameTXIDTuple(fromAlias.vchAlias, fromAlias.txHash, fromAlias.vchGUID);
	theAsset.linkAliasTuple = CNameTXIDTuple(toAlias.vchAlias, toAlias.txHash, toAlias.vchGUID);


	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_TRANSFER) << vchAsset << vchHashAsset << OP_2DROP << OP_2DROP;
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
    vector<unsigned char> vchValue;

	CAsset txPos;
	uint256 txid;
	if (!passetdb || !passetdb->ReadAssetLastTXID(vchAsset, txid))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read last txid from asset DB"));
	if (!GetAsset(CNameTXIDTuple(vchAsset, txid), txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from asset DB"));

	if(!BuildAssetJson(txPos, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetJson(const CAsset& asset, UniValue& oAsset)
{
    oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
    oAsset.push_back(Pair("txid", asset.txHash.GetHex()));
    oAsset.push_back(Pair("height", (int64_t)asset.nHeight));
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
	oAsset.push_back(Pair("alias", stringFromVch(asset.aliasTuple.first)));
	int64_t expired_time = GetAssetExpiration(asset);
	bool expired = false;
    if(expired_time <= chainActive.Tip()->GetMedianTimePast())
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
	oAsset.push_back(Pair("height", (int64_t)asset.nHeight));
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
	oAsset.push_back(Pair("alias", stringFromVch(asset.aliasTuple.first)));
	return true;
}
bool BuildAssetIndexerJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("title", stringFromVch(asset.vchName)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.aliasTuple.first)));
	return true;
}
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = assetFromOp(op);
	CAsset asset;
	if(!asset.UnserializeFromData(vchData, vchHash))
		return;

	CAsset dbAsset;
	GetAsset(CNameTXIDTuple(asset.vchAsset, asset.txHash), dbAsset);
	

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(asset.vchAsset)));

	if(!asset.vchName.empty() && asset.vchName != dbAsset.vchName)
		entry.push_back(Pair("title", stringFromVch(asset.vchName)));

	if(!asset.vchPubData.empty() && asset.vchPubData != dbAsset.vchPubData)
		entry.push_back(Pair("publicdata", stringFromVch(asset.vchPubData)));

	if(!asset.linkAliasTuple.first.empty() && asset.linkAliasTuple != dbAsset.aliasTuple)
		entry.push_back(Pair("alias", stringFromVch(asset.linkAliasTuple.first)));
	else if(asset.aliasTuple.first != dbAsset.aliasTuple.first)
		entry.push_back(Pair("alias", stringFromVch(asset.aliasTuple.first)));

}




