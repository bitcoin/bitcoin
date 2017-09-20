#include "escrow.h"
#include "offer.h"
#include "alias.h"
#include "cert.h"
#include "init.h"
#include "validation.h"
#include "core_io.h"
#include "util.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "policy/policy.h"
#include "script/script.h"
#include "chainparams.h"
#include "coincontrol.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <mongoc.h>
using namespace std;
extern mongoc_client_t *client;
extern mongoc_database_t *database;
extern mongoc_collection_t *alias_collection;
extern mongoc_collection_t *offer_collection;
extern mongoc_collection_t *escrow_collection;
extern mongoc_collection_t *cert_collection;
extern mongoc_collection_t *feedback_collection;
extern CScript _createmultisig_redeemScript(const UniValue& params);
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
void PutToEscrowList(std::vector<CEscrow> &escrowList, CEscrow& index) {
	int i = escrowList.size() - 1;
	BOOST_REVERSE_FOREACH(CEscrow &o, escrowList) {
        if(!o.txHash.IsNull() && o.txHash == index.txHash) {
        	escrowList[i] = index;
            return;
        }
        i--;
	}
    escrowList.push_back(index);
}
bool IsEscrowOp(int op) {
    return op == OP_ESCROW_ACTIVATE
        || op == OP_ESCROW_RELEASE
        || op == OP_ESCROW_REFUND
		|| op == OP_ESCROW_COMPLETE;
}
// % fee on escrow value for arbiter
int64_t GetEscrowArbiterFee(int64_t escrowValue, float fEscrowFee) {

	if(fEscrowFee == 0)
		fEscrowFee = 0.005;
	int fee = 1/fEscrowFee;
	int64_t nFee = escrowValue/fee;
	if(nFee < DEFAULT_MIN_RELAY_TX_FEE)
		nFee = DEFAULT_MIN_RELAY_TX_FEE;
	return nFee;
}
uint64_t GetEscrowExpiration(const CEscrow& escrow) {
	uint64_t nTime = chainActive.Tip()->nHeight + 1;
	CAliasUnprunable aliasBuyerPrunable,aliasSellerPrunable,aliasArbiterPrunable;
	if(paliasdb)
	{
		if (paliasdb->ReadAliasUnprunable(escrow.buyerAliasTuple.first, aliasBuyerPrunable) && !aliasBuyerPrunable.IsNull())
			nTime = aliasBuyerPrunable.nExpireTime;
		// buyer is expired try seller
		if(nTime <= chainActive.Tip()->nTime)
		{
			if (paliasdb->ReadAliasUnprunable(escrow.sellerAliasTuple.first, aliasSellerPrunable) && !aliasSellerPrunable.IsNull())
			{
				nTime = aliasSellerPrunable.nExpireTime;
				// seller is expired try the arbiter
				if(nTime <= chainActive.Tip()->nTime)
				{
					if (paliasdb->ReadAliasUnprunable(escrow.arbiterAliasTuple.first, aliasArbiterPrunable) && !aliasArbiterPrunable.IsNull())
						nTime = aliasArbiterPrunable.nExpireTime;
				}
			}
		}
	}
	return nTime;
}


string escrowFromOp(int op) {
    switch (op) {
    case OP_ESCROW_ACTIVATE:
        return "escrowactivate";
    case OP_ESCROW_RELEASE:
        return "escrowrelease";
    case OP_ESCROW_REFUND:
        return "escrowrefund";
	case OP_ESCROW_COMPLETE:
		return "escrowcomplete";
    default:
        return "<unknown escrow op>";
    }
}
bool CEscrow::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsEscrow(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsEscrow >> *this;

		vector<unsigned char> vchEscrowData;
		Serialize(vchEscrowData);
		const uint256 &calculatedHash = Hash(vchEscrowData.begin(), vchEscrowData.end());
		const vector<unsigned char> &vchRandEscrow = vchFromValue(calculatedHash.GetHex());
		if(vchRandEscrow != vchHash)
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
bool CEscrow::UnserializeFromTx(const CTransaction &tx) {
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
void CEscrow::Serialize(vector<unsigned char>& vchData) {
    CDataStream dsEscrow(SER_NETWORK, PROTOCOL_VERSION);
    dsEscrow << *this;
	vchData = vector<unsigned char>(dsEscrow.begin(), dsEscrow.end());

}
void CEscrowDB::WriteEscrowIndex(const CEscrow& escrow, const std::vector<std::vector<unsigned char> > &vvchArgs) {
	if (!escrow_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(escrow.vchEscrow).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildEscrowJson(escrow, vvchArgs, oName)) {
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(escrow_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB ESCROW UPDATE ERROR: %s\n", error.message);
		}
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CEscrowDB::EraseEscrowIndex(const std::vector<unsigned char>& vchEscrow) {
	if (!escrow_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(vchEscrow).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(escrow_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CEscrowDB::WriteEscrowFeedbackIndex(const CEscrow& escrow) {
	if (!feedback_collection)
		return;
	bson_error_t error;
	bson_t *insert = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	BuildFeedbackJson(escrow, oName);
	insert = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!insert || !mongoc_collection_insert(feedback_collection, (mongoc_insert_flags_t)MONGOC_INSERT_NO_VALIDATE, insert, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW FEEDBACK ERROR: %s\n", error.message);
	}
	
	if (insert)
		bson_destroy(insert);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CEscrowDB::EraseEscrowFeedbackIndex(const std::vector<unsigned char>& vchEscrow) {
	if (!feedback_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	string id;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	id = stringFromVch(vchEscrow);
	selector = BCON_NEW("escrow", BCON_UTF8(id.c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(feedback_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW FEEDBACK REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
bool CEscrowDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CEscrow txPos;
	pair<string, CNameTXIDTuple > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "escrowi") {
				const CNameTXIDTuple &escrowTuple = key.second;
  				if (!GetEscrow(escrowTuple.first, txPos) || chainActive.Tip()->nTime >= GetEscrowExpiration(txPos))
				{
					servicesCleaned++;
					EraseEscrow(escrowTuple);
				}
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

int IndexOfEscrowOutput(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
    vector<vector<unsigned char> > vvch;
	int op;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		// find an output you own
		if (pwalletMain->IsMine(out) && DecodeEscrowScript(out.scriptPubKey, op, vvch)) {
			return i;
		}
	}
	return -1;
}
bool GetEscrow(const CNameTXIDTuple &escrowTuple,
	CEscrow& txPos) {
	if (!pescrowdb || !pescrowdb->ReadEscrow(escrowTuple, txPos))
		return false;
	if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos)) {
		string escrow = stringFromVch(escrowTuple.first);
		LogPrintf("GetEscrow(%s) : expired", escrow.c_str());
		return false;
	}
	return true;
}
bool GetEscrow(const vector<unsigned char> &vchEscrow,
        CEscrow& txPos) {
	uint256 txid;
	if (!pescrowdb || !pescrowdb->ReadEscrowLastTXID(vchEscrow, txid))
		return false;
	if (!pescrowdb->ReadEscrow(CNameTXIDTuple(vchEscrow, txid), txPos))
		return false;
   if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos)) {
	   txPos.SetNull();
        string escrow = stringFromVch(vchEscrow);
        LogPrintf("GetEscrow(%s) : expired", escrow.c_str());
        return false;
    }
    return true;
}
bool DecodeAndParseEscrowTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	CEscrow escrow;
	bool decode = DecodeEscrowTx(tx, op, nOut, vvch);
	bool parse = escrow.UnserializeFromTx(tx);
	return decode && parse;
}
bool DecodeEscrowTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeEscrowScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
	if (!found) vvch.clear();
    return found;
}

bool DecodeEscrowScript(const CScript& script, int& op,
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
	return found && IsEscrowOp(op);
}
bool DecodeEscrowScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeEscrowScript(script, op, vvch, pc);
}

bool RemoveEscrowScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeEscrowScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}
bool ValidateExternalPayment(const CEscrow& theEscrow, const bool &dontaddtodb, string& errorMessage)
{

	if(!theEscrow.extTxId.IsNull())
	{
		if(pofferdb->ExistsExtTXID(theEscrow.extTxId))
		{
			errorMessage = _("External Transaction ID specified was already used to pay for an offer");
			return true;
		}
		if (!dontaddtodb && !pofferdb->WriteExtTXID(theEscrow.extTxId))
		{
			errorMessage = _("Failed to External Transaction ID to DB");
			return false;
		}
	}
	return true;
}
bool CheckEscrowInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add escrow in coinbase transaction, skipping...");
		return true;
	}
	const COutPoint *prevOutput = NULL;
	const CCoins *prevCoins;
	int prevAliasOp = 0;
	bool foundAlias = false;
	if (fDebug)
		LogPrintf("*** ESCROW %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

    // Make sure escrow outputs are not spent by a regular transaction, or the escrow would be lost
    if (tx.nVersion != SYSCOIN_TX_VERSION)
	{
		errorMessage = "SYSCOIN_ESCROW_MESSAGE_ERROR: ERRCODE: 4000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	 // unserialize escrow UniValue from txn, check for valid
    CEscrow theEscrow;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theEscrow.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR ERRCODE: 4001 - " + _("Cannot unserialize data inside of this transaction relating to an escrow");
		return true;
	}

	vector<vector<unsigned char> > vvchPrevAliasArgs;
	if(fJustCheck)
	{
		if(vvchArgs.size() != 3)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4002 - " + _("Escrow arguments incorrect size");
			return error(errorMessage.c_str());
		}
		if(!theEscrow.IsNull())
		{
			if(vvchArgs.size() <= 2 || vchHash != vvchArgs[2])
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4003 - " + _("Hash provided doesn't match the calculated hash of the data");
				return true;
			}
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
			if(!prevCoins->IsAvailable(prevOutput->n) || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch))
				continue;
			if(foundAlias)
				break;

			else if (!foundAlias && IsAliasOp(pop, true) && vvch.size() >= 4 && vvch[3].empty())
			{
				foundAlias = true;
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}
	}

	CAliasIndex buyerAlias, sellerAlias, arbiterAlias;
    COffer theOffer;
	string retError = "";
	CTransaction txOffer;
	int escrowOp = OP_ESCROW_ACTIVATE;
	bool bPaymentAck = false;
	COffer dbOffer;
	if(fJustCheck)
	{
		if (vvchArgs.empty() || vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4004 - " + _("Escrow guid too big");
			return error(errorMessage.c_str());
		}
		if(theEscrow.vchRedeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4005 - " + _("Escrow redeem script too long");
			return error(errorMessage.c_str());
		}
		if(theEscrow.feedback.vchFeedback.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4006 - " + _("Feedback too long");
			return error(errorMessage.c_str());
		}
		if (theEscrow.nQty <= 0)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4006 - " + _("Quantity of order must be greator than 0");
			return error(errorMessage.c_str());
		}
		if(theEscrow.offerTuple.first.size() > MAX_ID_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4008 - " + _("Escrow offer guid too long");
			return error(errorMessage.c_str());
		}
		if(!theEscrow.vchEscrow.empty() && theEscrow.vchEscrow != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4009 - " + _("Escrow guid in data output doesn't match guid in transaction");
			return error(errorMessage.c_str());
		}
		switch (op) {
			case OP_ESCROW_ACTIVATE:
				if (theEscrow.bPaymentAck)
				{
					if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.linkAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.linkAliasTuple.third != vvchPrevAliasArgs[1])
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Alias input mismatch");
						return error(errorMessage.c_str());
					}
				}
				else
				{
					if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.buyerAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.buyerAliasTuple.third != vvchPrevAliasArgs[1])
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4011 - " + _("Alias input mismatch");
						return error(errorMessage.c_str());
					}
					if(theEscrow.op != OP_ESCROW_ACTIVATE)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4012 - " + _("Invalid op, should be escrow activate");
						return error(errorMessage.c_str());
					}
				}
				if (theEscrow.vchEscrow != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4014 - " + _("Escrow Guid mismatch");
					return error(errorMessage.c_str());
				}
				if(!IsValidPaymentOption(theEscrow.nPaymentOption))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4015 - " + _("Invalid payment option");
					return error(errorMessage.c_str());
				}
				if (!theEscrow.extTxId.IsNull() && theEscrow.nPaymentOption == PAYMENTOPTION_SYS)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4016 - " + _("External payment cannot be paid with SYS");
					return error(errorMessage.c_str());
				}
				if (theEscrow.extTxId.IsNull() && theEscrow.nPaymentOption != PAYMENTOPTION_SYS)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4017 - " + _("External payment missing transaction ID");
					return error(errorMessage.c_str());
				}
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4018 - " + _("Cannot leave feedback in escrow activation");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_RELEASE:
				if (vvchArgs.size() <= 1 || vvchArgs[1].size() > 1)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4019 - " + _("Escrow release status too large");
					return error(errorMessage.c_str());
				}
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4020 - " + _("Cannot leave feedback in escrow release");
					return error(errorMessage.c_str());
				}
				if(vvchArgs[1] == vchFromString("1"))
				{
					if(theEscrow.op != OP_ESCROW_COMPLETE)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4021 - " + _("Invalid op, should be escrow complete");
						return error(errorMessage.c_str());
					}

				}
				else
				{
					if(theEscrow.op != OP_ESCROW_RELEASE)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4022 - " + _("Invalid op, should be escrow release");
						return error(errorMessage.c_str());
					}
				}

				break;
			case OP_ESCROW_COMPLETE:
				if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.linkAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.linkAliasTuple.third != vvchPrevAliasArgs[1])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4023 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				if (theEscrow.op != OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4024 - " + _("Invalid op, should be escrow complete");
					return error(errorMessage.c_str());
				}
				if(theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4025 - " + _("Feedback must leave a message");
					return error(errorMessage.c_str());
				}

				if(theEscrow.op != OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4026 - " + _("Invalid op, should be escrow complete");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_REFUND:
				if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.linkAliasTuple.first != vvchPrevAliasArgs[0]  || theEscrow.linkAliasTuple.third != vvchPrevAliasArgs[1])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4027 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				if (vvchArgs.size() <= 1 || vvchArgs[1].size() > 1)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4028 - " + _("Escrow refund status too large");
					return error(errorMessage.c_str());
				}
				if (theEscrow.vchEscrow != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4029 - " + _("Guid mismatch");
					return error(errorMessage.c_str());
				}
				if(vvchArgs[1] == vchFromString("1"))
				{
					if(theEscrow.op != OP_ESCROW_COMPLETE)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4030 - " + _("Invalid op, should be escrow complete");
						return error(errorMessage.c_str());
					}
				}
				else
				{
					if(theEscrow.op != OP_ESCROW_REFUND)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4031 - " + _("Invalid op, should be escrow refund");
						return error(errorMessage.c_str());
					}
				}
				// Check input
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4032 - " + _("Cannot leave feedback in escrow refund");
					return error(errorMessage.c_str());
				}



				break;
			default:
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4033 - " + _("Escrow transaction has unknown op");
				return error(errorMessage.c_str());
		}
	}



    if (!fJustCheck ) {
		if(op == OP_ESCROW_ACTIVATE)
		{
			if (!theEscrow.bPaymentAck)
			{
				if(!GetAlias(theEscrow.buyerAliasTuple.first, buyerAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4034 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if(!GetAlias(theEscrow.arbiterAliasTuple.first, arbiterAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4035 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if(!GetAlias(theEscrow.sellerAliasTuple.first, sellerAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4036 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
			}
		}
		// make sure escrow settings don't change (besides rawTx) outside of activation
		if(op != OP_ESCROW_ACTIVATE || theEscrow.bPaymentAck)
		{
			// save serialized escrow for later use
			CEscrow serializedEscrow = theEscrow;
			if(!GetEscrow(vvchArgs[0], theEscrow))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4037 - " + _("Failed to read from escrow DB");
				return true;
			}
			if(serializedEscrow.buyerAliasTuple != theEscrow.buyerAliasTuple ||
				serializedEscrow.arbiterAliasTuple != theEscrow.arbiterAliasTuple ||
				serializedEscrow.sellerAliasTuple != theEscrow.sellerAliasTuple)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4038 - " + _("Invalid aliases used for escrow transaction");
				return true;
			}
			if(serializedEscrow.bPaymentAck && theEscrow.bPaymentAck)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4039 - " + _("Escrow already acknowledged");
			}
			if (theEscrow.vchEscrow != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Escrow Guid mismatch");
				return true;
			}

			// these are the only settings allowed to change outside of activate
			if(!serializedEscrow.scriptSigs.empty() && op != OP_ESCROW_ACTIVATE)
				theEscrow.scriptSigs = serializedEscrow.scriptSigs;
			escrowOp = serializedEscrow.op;
			if(op == OP_ESCROW_ACTIVATE && serializedEscrow.bPaymentAck)
			{
				if(serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4041 - " + _("Only seller can acknowledge an escrow payment");
					return true;
				}
				else
					theEscrow.bPaymentAck = true;
			}
			if(op == OP_ESCROW_REFUND && vvchArgs[1] == vchFromString("0"))
			{
				CAliasIndex alias;
				if(!GetAlias(theEscrow.sellerAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
				if(!GetAlias(theEscrow.arbiterAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4043 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}

				if(theEscrow.op == OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4044 - " + _("Can only refund an active escrow");
					return true;
				}
				else if(theEscrow.op == OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4045 - " + _("Cannot refund an escrow that is already released");
					return true;
				}
				else if(serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4046 - " + _("Only arbiter or seller can initiate an escrow refund");
					return true;
				}
				// only the arbiter can re-refund an escrow
				else if(theEscrow.op == OP_ESCROW_REFUND && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4047 - " + _("Only arbiter can refund an escrow after it has already been refunded");
					return true;
				}
				// refund qty
				if (GetOffer(theEscrow.offerTuple.first, dbOffer))
				{
					int nQty = dbOffer.nQty;
					COffer myLinkOffer;
					if (GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
					{
						nQty = myLinkOffer.nQty;
					}
						
					if(nQty != -1)
					{
						if(theEscrow.extTxId.IsNull())
							nQty += theEscrow.nQty;
						if (!myLinkOffer.IsNull())
						{
							myLinkOffer.nQty = nQty;
							myLinkOffer.nSold--;
							if (!dontaddtodb && !pofferdb->WriteOffer(myLinkOffer))
							{
								errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4048 - " + _("Failed to write to offer link to DB");
								return error(errorMessage.c_str());
							}
						}
						else
						{
							dbOffer.nQty = nQty;
							dbOffer.nSold--;
							if (!dontaddtodb && !pofferdb->WriteOffer(dbOffer))
							{
								errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4049 - " + _("Failed to write to offer to DB");
								return error(errorMessage.c_str());
							}
						}
					}
				}
			}
			else if(op == OP_ESCROW_REFUND && vvchArgs[1] == vchFromString("1"))
			{
				if(theEscrow.op != OP_ESCROW_REFUND)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Can only claim a refunded escrow");
					return true;
				}
				else if(!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				else if(serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4051 - " + _("Only buyer can claim an escrow refund");
					return true;
				}
			}
			else if(op == OP_ESCROW_RELEASE && vvchArgs[1] == vchFromString("0"))
			{
				CAliasIndex alias;
				if(!GetAlias(theEscrow.buyerAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4052 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if(!GetAlias(theEscrow.arbiterAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4053 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if(theEscrow.op == OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4054 - " + _("Can only release an active escrow");
					return true;
				}
				else if(theEscrow.op == OP_ESCROW_REFUND)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4055 - " + _("Cannot release an escrow that is already refunded");
					return true;
				}
				else if(serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4056 - " + _("Only arbiter or buyer can initiate an escrow release");
					return true;
				}
				// only the arbiter can re-release an escrow
				else if(theEscrow.op == OP_ESCROW_RELEASE && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4057 - " + _("Only arbiter can release an escrow after it has already been released");
					return true;
				}
			}
			else if(op == OP_ESCROW_RELEASE && vvchArgs[1] == vchFromString("1"))
			{
				if(theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4058 - " + _("Can only claim a released escrow");
					return true;
				}
				else if(!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				else if(serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4059 - " + _("Only seller can claim an escrow release");
					return true;
				}
			}
			else if(op == OP_ESCROW_COMPLETE)
			{
				vector<unsigned char> vchSellerAlias = theEscrow.sellerAliasTuple.first;
				if(!theEscrow.linkSellerAliasTuple.first.empty())
					vchSellerAlias = theEscrow.linkSellerAliasTuple.first;
					
				if (serializedEscrow.offerTuple.first != theEscrow.offerTuple.first) {
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Offer GUID incorrect in feedback payload");
					serializedEscrow = theEscrow;
				}
				else if(serializedEscrow.feedback.nFeedbackUserFrom ==  serializedEscrow.feedback.nFeedbackUserTo)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4061 - " + _("Cannot send yourself feedback");
					serializedEscrow = theEscrow;
				}
				else if(serializedEscrow.feedback.nRating > 5)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4063 - " + _("Invalid rating, must be less than or equal to 5 and greater than or equal to 0");
					serializedEscrow = theEscrow;
				}
				else if(serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKBUYER && serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only buyer can leave this feedback");
					serializedEscrow = theEscrow;
				}
				else if(serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKSELLER && serializedEscrow.linkAliasTuple.first != vchSellerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4065 - " + _("Only seller can leave this feedback");
					serializedEscrow = theEscrow;
				}
				else if(serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKARBITER && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Only arbiter can leave this feedback");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKBUYER && serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKSELLER && serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKARBITER)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 1062 - " + _("Unknown feedback user type");
					return true;
				}
				serializedEscrow.txHash = tx.GetHash();
				serializedEscrow.nHeight = nHeight;
				if (!dontaddtodb) {
					pescrowdb->WriteEscrowFeedbackIndex(serializedEscrow);
				}
				return true;
			}

		}
		else
		{
			COffer myLinkOffer;
			uint256 lastTXID;
			if (pescrowdb->ReadEscrowLastTXID(vvchArgs[0], lastTXID))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4071 - " + _("Escrow already exists");
				return true;
			}
			if(theEscrow.nQty <= 0)
				theEscrow.nQty = 1;

			if (GetOffer( theEscrow.offerTuple, dbOffer))
			{
				if(dbOffer.bPrivate && !dbOffer.linkWhitelist.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4072 - " + _("Cannot purchase this private offer, must purchase through an affiliate");
					return true;
				}
				if(dbOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(dbOffer.sCategory), "wanted"))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4073 - " + _("Cannot purchase a wanted offer");
				}
				int nQty = dbOffer.nQty;
				// if this is a linked offer we must update the linked offer qty
				if (GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
				{
					nQty = myLinkOffer.nQty;
				}
				if(nQty != -1)
				{
					if(theEscrow.nQty > nQty)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Not enough quantity left in this offer for this purchase");
						return true;
					}
					if(theEscrow.extTxId.IsNull())
						nQty -= theEscrow.nQty;
					if (!myLinkOffer.IsNull())
					{
						myLinkOffer.nQty = nQty;
						myLinkOffer.nSold++;
						if (!dontaddtodb && !pofferdb->WriteOffer(myLinkOffer))
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4075 - " + _("Failed to write to offer link to DB");
							return error(errorMessage.c_str());
						}
					}
					else
					{
						dbOffer.nQty = nQty;
						dbOffer.nSold++;
						if (!dontaddtodb && !pofferdb->WriteOffer(dbOffer))
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4076 - " + _("Failed to write to offer to DB");
							return error(errorMessage.c_str());
						}
					}
				}
			}
			else
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4077 - " + _("Cannot find offer for this escrow. It may be expired");
				return true;
			}
			if(!theOffer.linkOfferTuple.first.empty() && myLinkOffer.IsNull())
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Cannot find linked offer for this escrow");
				return true;
			}
			if(theEscrow.nPaymentOption != PAYMENTOPTION_SYS)
			{
				bool noError = ValidateExternalPayment(theEscrow, dontaddtodb, errorMessage);
				if(!errorMessage.empty())
				{
					errorMessage =  "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4079 - " + errorMessage;
					if(!noError)
						return error(errorMessage.c_str());
					else
						return true;
				}
			}
		}
		

        // set the escrow's txn-dependent values
		if(!bPaymentAck)
			theEscrow.op = escrowOp;
		theEscrow.txHash = tx.GetHash();
		theEscrow.nHeight = nHeight;
        // write escrow
		if (!dontaddtodb && !pescrowdb->WriteEscrow(vvchArgs, theEscrow))
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4080 - " + _("Failed to write to escrow DB");
			return error(errorMessage.c_str());
		}
		if(fDebug)
			LogPrintf( "CONNECTED ESCROW: op=%s escrow=%s hash=%s height=%d\n",
                escrowFromOp(op).c_str(),
                stringFromVch(vvchArgs[0]).c_str(),
                tx.GetHash().ToString().c_str(),
                nHeight);
	}
    return true;
}
UniValue generateescrowmultisig(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 4 ||  params.size() > 5)
        throw runtime_error(
		"generateescrowmultisig <buyer> <offer guid> <qty> <arbiter> [payment option=SYS]\n"
                        + HelpRequiringPassphrase());

	vector<unsigned char> vchBuyer = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	unsigned int nQty = 1;

	try {
		nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4500 - " + _("Invalid quantity value. Quantity must be less than 4294967296."));
	}
	vector<unsigned char> vchArbiter = vchFromValue(params[3]);
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOption = "SYS";
	if(CheckParam(params, 4))
		paymentOption = params[4].get_str();
	
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOption))
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4501 - " + _("Could not validate the payment options value");
		throw runtime_error(err.c_str());
	}
	uint64_t paymentOptionMask = GetPaymentOptionsMaskFromString(paymentOption);
	CAliasIndex arbiteralias;
	if (!GetAlias(vchArbiter, arbiteralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4502 - " + _("Failed to read arbiter alias from DB"));

	CAliasIndex buyeralias;
	if (!GetAlias(vchBuyer, buyeralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4503 - " + _("Failed to read arbiter alias from DB"));

	COffer theOffer, linkedOffer;
	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4504 - " + _("Could not find an offer with this identifier"));

	CAliasIndex selleralias;
	if (!GetAlias( theOffer.aliasTuple.first, selleralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4505 - " + _("Could not find seller alias with this identifier"));
	
	CAliasIndex pegAlias;
	if (!GetAlias(selleralias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 1500 - " + _("Could not find seller peg alias with this name"));

	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
	int precision = 2;
	CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(latestAliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), 1, precision);
	if (nPricePerUnit == 0)
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 1549 - " + _("Could not find payment currency in the peg alias");
		throw runtime_error(err.c_str());
	}

	COfferLinkWhitelistEntry foundEntry;
	if(!theOffer.linkOfferTuple.first.empty())
	{
		COffer offerTmp;
		if (!GetOffer( theOffer.linkOfferTuple.first, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4506 - " + _("Trying to accept a linked offer but could not find parent offer"));

		CAliasIndex theLinkedAlias;
		if (!GetAlias( linkedOffer.aliasTuple.first, theLinkedAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4507 - " + _("Could not find an alias with this identifier"));
		selleralias = theLinkedAlias;
	}
	else
	{
		// if offer is not linked, look for a discount for the buyer
		theOffer.linkWhitelist.GetLinkEntryByHash(buyeralias.vchAlias, foundEntry);

	}
	UniValue arrayParams(UniValue::VARR);
	UniValue arrayOfKeys(UniValue::VARR);

	// standard 2 of 3 multisig
	arrayParams.push_back(2);
	arrayOfKeys.push_back(stringFromVch(arbiteralias.vchAlias));
	arrayOfKeys.push_back(stringFromVch(selleralias.vchAlias));
	arrayOfKeys.push_back(stringFromVch(buyeralias.vchAlias));
	arrayParams.push_back(arrayOfKeys);
	UniValue resCreate;
	CScript redeemScript;
	try
	{
		resCreate = tableRPC.execute("createmultisig", arrayParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4508 - " + _("Could not create escrow transaction: Invalid response from createescrow"));

	CAmount nTotal = AmountFromValue(strprintf("%.*f", 8, theOffer.GetPrice(foundEntry)*nQty));
	if (stringFromVch(theOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
	{
		UniValue paramsConvert(UniValue::VARR);
		paramsConvert.push_back(stringFromVch(selleralias.vchAlias));
		paramsConvert.push_back(stringFromVch(theOffer.sCurrencyCode));
		paramsConvert.push_back(GetPaymentOptionsString(paymentOptionMask));
		paramsConvert.push_back(boost::lexical_cast<string>(theOffer.GetPrice(foundEntry)*nQty));
		const UniValue &r = tableRPC.execute("aliasconvertcurrency", paramsConvert);
		nTotal = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "convertedrate").get_str()));
	}

	float fEscrowFee = getEscrowFee(selleralias.aliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), precision);
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotal, fEscrowFee);
	int nExtFeePerByte = getFeePerByte(selleralias.aliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), precision);
	// multisig spend is about 400 bytes
	nTotal = nTotal + nEscrowFee + (nExtFeePerByte*400);
	resCreate.push_back(Pair("total", strprintf("%.*f", precision, ValueFromAmount(nTotal).get_real())));
	resCreate.push_back(Pair("merchant_aliaspeg_txid", latestAliasPegTuple.second.GetHex()));
	return resCreate;
}

UniValue escrownew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 4 ||  params.size() > 9)
        throw runtime_error(
			"escrownew <alias> <offer> <quantity> <arbiter alias> [extTx] [merchantAliasPegTx] [payment option] [redeemScript] [witness]\n"
						"<alias> An alias you own.\n"
                        "<offer> GUID of offer that this escrow is managing.\n"
                        "<quantity> Quantity of items to buy of offer.\n"
						"<arbiter alias> Alias of Arbiter.\n"
						"<extTx> External transaction ID if paid with another blockchain.\n"
						"<merchantAliasPegTx> 'merchant_aliaspeg_txid' result from the output of generateescrowmultisig for external blockchain escrow.\n"
						"<paymentOption> If extTx is defined, specify a valid payment option used to make payment. Default is SYS.\n"
						"<redeemScript> If paid in external chain, enter redeemScript that generateescrowmultisig returns\n"
                        "<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	unsigned int nQty = 1;

	try {
		nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4511 - " + _("Invalid quantity value. Quantity must be less than 4294967296."));
	}
	uint64_t nHeight = chainActive.Tip()->nHeight;
	string strArbiter = params[3].get_str();
	boost::algorithm::to_lower(strArbiter);
	// check for alias existence in DB
	CAliasIndex arbiteralias;
	if (!GetAlias(vchFromString(strArbiter), arbiteralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Failed to read arbiter alias from DB"));
	
	string extTxIdStr = "";
	if(CheckParam(params, 4))
		extTxIdStr = params[4].get_str();

	uint256 merchantAliasPegTx;
	if (CheckParam(params, 5))
		merchantAliasPegTx.SetHex(params[5].get_str());
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOption = "SYS";
	if(CheckParam(params, 6))
	{
		paymentOption = params[6].get_str();
		boost::algorithm::to_upper(paymentOption);
	}
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOption))
	{
		// TODO change error number to something unique
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4510 - " + _("Could not validate the payment option value");
		throw runtime_error(err.c_str());
	}
	// payment options - and convert payment options string to a bitmask for the txn
	uint64_t paymentOptionMask = GetPaymentOptionsMaskFromString(paymentOption);
	vector<unsigned char> vchRedeemScript;
	if(CheckParam(params, 7))
		vchRedeemScript = vchFromValue(params[7]);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 8))
		vchWitness = vchFromValue(params[8]);

	CAliasIndex buyeralias;
	if (!GetAlias(vchAlias, buyeralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find buyer alias with this name"));
	

	COffer theOffer, linkedOffer;

	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Could not find an offer with this identifier"));

	CAliasIndex selleralias;
	if (!GetAlias( theOffer.aliasTuple.first, selleralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4514 - " + _("Could not find seller alias with this identifier"));

	if(theOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(theOffer.sCategory), "wanted"))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4515 - " + _("Cannot purchase a wanted offer"));

	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	COfferLinkWhitelistEntry foundEntry;
	CAliasIndex theLinkedAlias, reselleralias;
	if(!theOffer.linkOfferTuple.first.empty())
	{
		
		if (!GetOffer( theOffer.linkOfferTuple.first, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Trying to accept a linked offer but could not find parent offer"));

		if (!GetAlias( linkedOffer.aliasTuple.first, theLinkedAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4517 - " + _("Could not find an alias with this identifier"));

		if(linkedOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkedOffer.sCategory), "wanted"))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4518 - " + _("Cannot purchase a wanted offer"));

		linkedOffer.linkWhitelist.GetLinkEntryByHash(theOffer.aliasTuple.first, foundEntry);

		reselleralias = selleralias;
		selleralias = theLinkedAlias;
	}
	else
		theOffer.linkWhitelist.GetLinkEntryByHash(buyeralias.vchAlias, foundEntry);
	CAliasIndex sellerAliasPeg;

	if (merchantAliasPegTx.IsNull() && !GetAlias(selleralias.aliasPegTuple.first, sellerAliasPeg))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4518 - " + _("Cannot fnd seller alias peg"));

	const CNameTXIDTuple &merchantAliasPegTuple = CNameTXIDTuple(selleralias.aliasPegTuple.first, merchantAliasPegTx.IsNull()? sellerAliasPeg.txHash: merchantAliasPegTx);

	int paymentPrecision = 2;
	int precision = 2;
	CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(merchantAliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), 1, precision);
	if (nPricePerUnit == 0)
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 1549 - " + _("Could not find payment currency in the peg alias");
		throw runtime_error(err.c_str());
	}
	nPricePerUnit = convertCurrencyCodeToSyscoin(merchantAliasPegTuple, theOffer.sCurrencyCode, 1, paymentPrecision);
	if (nPricePerUnit == 0)
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 1549 - " + _("Could not find offer currency in the peg alias");
		throw runtime_error(err.c_str());
	}
	CSyscoinAddress buyerAddress;
	GetAddress(buyeralias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyeralias.vchAlias  << buyeralias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;


    // gather inputs
	vector<unsigned char> vchEscrow = vchFromString(GenerateSyscoinGuid());

    // this is a syscoin transaction
    CWalletTx wtx;
    CScript scriptPubKey, scriptPubKeyBuyer,scriptBuyer;


	vector<unsigned char> redeemScript;
	string strAddress;
	if(vchRedeemScript.empty())
	{
		UniValue arrayParams(UniValue::VARR);
		arrayParams.push_back(stringFromVch(buyeralias.vchAlias));
		arrayParams.push_back(stringFromVch(vchOffer));
		arrayParams.push_back( boost::lexical_cast<string>(nQty));
		arrayParams.push_back(stringFromVch(arbiteralias.vchAlias));
		UniValue resCreate;
		try
		{
			resCreate = tableRPC.execute("generateescrowmultisig", arrayParams);
		}
		catch (UniValue& objError)
		{
			throw runtime_error(find_value(objError, "message").get_str());
		}
		if (!resCreate.isObject())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4522 - " + _("Could not generate escrow multisig address: Invalid response from generateescrowmultisig"));
		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		if (redeemScript_value.isStr())
		{
			redeemScript = ParseHex(redeemScript_value.get_str());
		}
		else
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));
		const UniValue& address_value = find_value(o, "address");
		strAddress = address_value.get_str();
	}
	else
	{
		redeemScript = ParseHex(stringFromVch(vchRedeemScript));
	}
	CAmount nCommission = 0;

	CAmount nTotalOfferPrice = AmountFromValue(strprintf("%.*f", 8, theOffer.GetPrice(foundEntry)*nQty));
	if (stringFromVch(theOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
	{
		UniValue paramsConvert(UniValue::VARR);
		paramsConvert.push_back(stringFromVch(selleralias.vchAlias));
		paramsConvert.push_back(stringFromVch(theOffer.sCurrencyCode));
		paramsConvert.push_back(GetPaymentOptionsString(paymentOptionMask));
		paramsConvert.push_back(boost::lexical_cast<string>(theOffer.GetPrice(foundEntry)*nQty));
		UniValue r = tableRPC.execute("aliasconvertcurrency", paramsConvert);
		nTotalOfferPrice = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "convertedrate").get_str()));
	}
	if (!theOffer.linkOfferTuple.first.empty())
	{
		CAmount nTotalLinkedOfferPrice = AmountFromValue(strprintf("%.*f", 8, linkedOffer.GetPrice(foundEntry)*nQty));
		if (stringFromVch(linkedOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
		{
			UniValue paramsConvert(UniValue::VARR);
			paramsConvert.push_back(stringFromVch(theLinkedAlias.vchAlias));
			paramsConvert.push_back(stringFromVch(linkedOffer.sCurrencyCode));
			paramsConvert.push_back(GetPaymentOptionsString(paymentOptionMask));
			paramsConvert.push_back(boost::lexical_cast<string>(linkedOffer.GetPrice(foundEntry)*nQty));
			UniValue r = tableRPC.execute("aliasconvertcurrency", paramsConvert);
			nTotalLinkedOfferPrice = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "convertedrate").get_str()));
		}
		nCommission = nTotalOfferPrice - nTotalLinkedOfferPrice;
		if (nCommission < 0)
			nCommission = 0;
	}
	CSyscoinAddress address(strAddress);
	scriptPubKey = GetScriptForDestination(address.Get());
	// send to escrow address
	CAmount nTotalWithBuyerDiscount = AmountFromValue(strprintf("%.*f", 8, theOffer.GetPrice(foundEntry)*nQty));
	if (stringFromVch(theOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
	{
		UniValue paramsConvert(UniValue::VARR);
		paramsConvert.push_back(stringFromVch(selleralias.vchAlias));
		paramsConvert.push_back(stringFromVch(theOffer.sCurrencyCode));
		paramsConvert.push_back(GetPaymentOptionsString(paymentOptionMask));
		paramsConvert.push_back(boost::lexical_cast<string>(theOffer.GetPrice(foundEntry)*nQty));
		UniValue r = tableRPC.execute("aliasconvertcurrency", paramsConvert);
		nTotalWithBuyerDiscount = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "convertedrate").get_str()));
	}

	int nFeePerByte = getFeePerByte(selleralias.aliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), precision);
	float fEscrowFee = getEscrowFee(selleralias.aliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), precision);
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotalWithBuyerDiscount, fEscrowFee);
	if(stringFromVch(theOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
		nEscrowFee = convertSyscoinToCurrencyCode(selleralias.aliasPegTuple, vchFromString(GetPaymentOptionsString(paymentOptionMask)), nEscrowFee, precision);
	
	CAmount nNetworkFee = (nFeePerByte * 400);
	vector<CRecipient> vecSend;
	CAmount nAmountWithFee = nTotalWithBuyerDiscount+nEscrowFee+nNetworkFee;
	CAmount nTotalInCurrency = nTotalWithBuyerDiscount;
	if (stringFromVch(theOffer.sCurrencyCode) != GetPaymentOptionsString(paymentOptionMask))
	{
		UniValue paramsConvert(UniValue::VARR);
		paramsConvert.push_back(stringFromVch(selleralias.vchAlias));
		paramsConvert.push_back(GetPaymentOptionsString(paymentOptionMask));
		paramsConvert.push_back(stringFromVch(theOffer.sCurrencyCode));
		paramsConvert.push_back(boost::lexical_cast<string>(ValueFromAmount(nTotalWithBuyerDiscount).get_real()));
		UniValue r = tableRPC.execute("aliasconvertcurrency", paramsConvert);
		nTotalInCurrency = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "convertedrate").get_str()));
	}

	CWalletTx escrowWtx;
	CRecipient recipientEscrow  = {scriptPubKey, nAmountWithFee, false};
	if(extTxIdStr.empty())
		vecSend.push_back(recipientEscrow);

	// send to seller/arbiter so they can track the escrow through GUI
    // build escrow
    CEscrow newEscrow;
	newEscrow.op = OP_ESCROW_ACTIVATE;
	newEscrow.vchEscrow = vchEscrow;
	newEscrow.buyerAliasTuple = CNameTXIDTuple(buyeralias.vchAlias, buyeralias.txHash, buyeralias.vchGUID);
	newEscrow.arbiterAliasTuple = CNameTXIDTuple(arbiteralias.vchAlias, arbiteralias.txHash, arbiteralias.vchGUID);
	newEscrow.vchRedeemScript = redeemScript;
	newEscrow.offerTuple = CNameTXIDTuple(theOffer.vchOffer, theOffer.txHash);
	newEscrow.extTxId = uint256S(extTxIdStr);
	newEscrow.sellerAliasTuple = CNameTXIDTuple(selleralias.vchAlias, selleralias.txHash, selleralias.vchGUID);
	newEscrow.linkSellerAliasTuple = CNameTXIDTuple(reselleralias.vchAlias, reselleralias.txHash, reselleralias.vchGUID);
	newEscrow.nQty = nQty;
	newEscrow.nPaymentOption = paymentOptionMask;
	newEscrow.nHeight = chainActive.Tip()->nHeight;
	newEscrow.sCurrencyCode = stringFromVch(theOffer.sCurrencyCode);
	newEscrow.bCoinOffer = theOffer.bCoinOffer;
	newEscrow.nCommission = nCommission;
	newEscrow.nNetworkFee = nNetworkFee;
	newEscrow.nArbiterFee = nEscrowFee;
	newEscrow.nTotal = nAmountWithFee;
	newEscrow.sTotal = strprintf("%.*f", paymentPrecision, ValueFromAmount(nTotalInCurrency).get_real());
	
	vector<unsigned char> data;
	newEscrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyBuyer += scriptPubKeyAliasOrig;


	// send the tranasction
	
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, buyeralias.vchAlias, buyeralias.aliasPegTuple, aliasPaymentRecipient);


	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, buyeralias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(buyeralias.vchAlias, vchWitness, selleralias.aliasPegTuple.first, stringFromVch(theOffer.sCurrencyCode), aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
		res.push_back(stringFromVch(vchEscrow));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchEscrow));
		res.push_back("false");
	}
	return res;
}
UniValue escrowacknowledge(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 1 || params.size() > 2)
		throw runtime_error(
			"escrowacknowledge <escrow guid> [witness]\n"
			"Acknowledge escrow payment as seller of offer.\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	if (CheckParam(params, 1))
		vchWitness = vchFromValue(params[1]);



	// this is a syscoin transaction
	CWalletTx wtx;
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4536 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}



	CScript scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += sellerScript;

	escrow.ClearEscrow();
	escrow.bPaymentAck = true;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(sellerAliasLatest.vchAlias, sellerAliasLatest.txHash, sellerAliasLatest.vchGUID);

	vector<unsigned char> data;
	escrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;

	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyOrigBuyer += buyerScript;

	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(sellerScript, sellerAliasLatest.vchAlias, sellerAliasLatest.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, sellerAliasLatest.aliasPegTuple, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, sellerAliasLatest.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);

	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if (bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;

}
UniValue escrowrelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 4 || params.size() < 3)
        throw runtime_error(
			"escrowrelease <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [witness]\n"
			"Releases escrow funds to seller, seller needs to sign the output transaction and send to the network. User role represents either 'buyer' or 'arbiter'. Third parameter is array of input (txid, vout, amount) pairs to be used to fund the release of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	const UniValue &inputs = params[2].get_array();
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 3))
		vchWitness = vchFromValue(params[3]);
    // this is a syscoin transaction
    CWalletTx wtx;

	CEscrow escrow;
    if (!GetEscrow( vchEscrow,escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find a escrow with this key"));
	COffer theOffer;
	if (!GetOffer(escrow.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find offer related to this escrow"));
	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest, linkSellerAliasLatest;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment, linkSellerAddress;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}
	if (GetAlias(escrow.linkSellerAliasTuple.first, linkSellerAliasLatest))
	{
		CScript script;
		GetAddress(linkSellerAliasLatest, &linkSellerAddress, script, escrow.nPaymentOption);
	}
	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;

	CAmount nEscrowTotal = escrow.nTotal;
	CAmount nTotal = escrow.nTotal - escrow.nArbiterFee - escrow.nNetworkFee;

	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (nBalance < nEscrowTotal) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(nEscrowTotal) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
	}
	nBalance -= nEscrowTotal;
	vector<unsigned char> vchLinkAlias;
	CAliasIndex theAlias;

	// who is initiating release arbiter or buyer?
	if(role == "arbiter")
	{
		scriptPubKeyAliasOrig = arbiterScript;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += scriptPubKeyAliasOrig;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
	}
	else if(role == "buyer")
	{
		scriptPubKeyAliasOrig = buyerScript;
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += scriptPubKeyAliasOrig;
		vchLinkAlias = buyerAliasLatest.vchAlias;
		theAlias = buyerAliasLatest;
	}

    // inputs buyer txHash
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createAddressUniValue(UniValue::VOBJ);
	if(role == "arbiter")
	{
		// if linked offer send commission to affiliate
		if(!theOffer.linkOfferTuple.first.empty())
		{
			if(escrow.nCommission > 0)
				createAddressUniValue.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal- escrow.nCommission)));
		}
		else
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal+nBalance)));
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	}
	else if(role == "buyer")
	{
		// if linked offer send commission to affiliate
		if(!theOffer.linkOfferTuple.first.empty())
		{
			if(escrow.nCommission > 0)
				createAddressUniValue.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal-escrow.nCommission)));
		}
		else
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal+nBalance)));
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	}

	arrayCreateParams.push_back(inputs);
	arrayCreateParams.push_back(createAddressUniValue);
	arrayCreateParams.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams.push_back(escrow.extTxId.IsNull());
	UniValue resCreate;
	try
	{
		resCreate = tableRPC.execute("createrawtransaction", arrayCreateParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate.isStr())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not create escrow transaction: Invalid response from createrawtransaction"));
	string createEscrowSpendingTx = resCreate.get_str();

	if(pwalletMain)
	{
		CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
		pwalletMain->AddCScript(inner);
	}

 	UniValue signUniValue(UniValue::VARR);
 	signUniValue.push_back(createEscrowSpendingTx);

	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", signUniValue);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resSign.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4534 - " + _("Could not sign escrow transaction: Invalid response from signrawtransaction"));

	const UniValue& o = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();

	if(createEscrowSpendingTx == hex_str)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4535 - " + _("Could not sign escrow transaction: Signature not added to transaction"));
	CTransaction signedTx;
	DecodeHexTx(signedTx, hex_str);
	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_RELEASE;
	for (int i = 0; i < signedTx.vin.size(); i++) {
		if(!signedTx.vin[i].scriptSig.empty())
			escrow.scriptSigs.push_back((CScriptBase)signedTx.vin[i].scriptSig);
	}
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.bPaymentAck = false;
	escrow.linkAliasTuple = CNameTXIDTuple(theAlias.vchAlias, theAlias.txHash, theAlias.vchGUID);

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

    CScript scriptPubKeyOrigSeller;

    scriptPubKeyOrigSeller << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigSeller += sellerScript;


	vector<CRecipient> vecSend;
	CRecipient recipientSeller;
	CreateRecipient(scriptPubKeyOrigSeller, recipientSeller);
	vecSend.push_back(recipientSeller);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple,data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, theAlias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);

	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign1 = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign1.get_obj();
	hex_str = "";
	string txid_str = "";
	const UniValue& hex_value1 = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value1.isStr())
		hex_str = hex_value1.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}
UniValue escrowclaimrelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 3 || params.size() < 2)
        throw runtime_error(
		"escrowclaimrelease <escrow guid> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [witness]\n"
                        "Claim escrow funds released from buyer or arbiter using escrowrelease. Second parameter is array of input (txid, vout, amount) pairs to be used to fund the release of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	const UniValue &inputs = params[1].get_array();
	UniValue ret(UniValue::VARR);
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4540 - " + _("Could not find a escrow with this key"));
	COffer theOffer;
	if (!GetOffer(escrow.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find offer related to this escrow"));
	CAliasIndex buyerAlias, arbiterAlias, sellerAlias, buyerAliasLatest, arbiterAliasLatest, sellerAliasLatest, linkSellerAliasLatest;
	CSyscoinAddress buyerAddressPayment, arbiterAddressPayment, sellerAddressPayment, linkSellerAddress;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}
	if(GetAlias(escrow.linkSellerAliasTuple.first, linkSellerAliasLatest))
	{
		CScript script;
		GetAddress(linkSellerAliasLatest, &linkSellerAddress, script, escrow.nPaymentOption);
	}

	CAmount nEscrowTotal = escrow.nTotal;
	CAmount nTotal = escrow.nTotal - escrow.nArbiterFee - escrow.nNetworkFee;
	
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (nBalance < nEscrowTotal) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(nEscrowTotal) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
	}
	nBalance -= nEscrowTotal;

	UniValue arrayCreateParams1(UniValue::VARR);
	UniValue arrayCreateParams2(UniValue::VARR);
	UniValue arrayCreateParamsArbiter(UniValue::VOBJ);
	UniValue arrayCreateParamsBuyer(UniValue::VOBJ);
	// if linked offer send commission to affiliate
	if (!theOffer.linkOfferTuple.first.empty())
	{
		if (escrow.nCommission > 0)
			arrayCreateParamsArbiter.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
		arrayCreateParamsArbiter.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal - escrow.nCommission)));
	}
	else
		arrayCreateParamsArbiter.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal + nBalance)));

	arrayCreateParamsArbiter.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));

	// if linked offer send commission to affiliate
	if (!theOffer.linkOfferTuple.first.empty())
	{
		if (escrow.nCommission > 0)
			arrayCreateParamsBuyer.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
		arrayCreateParamsBuyer.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal - escrow.nCommission)));
	}
	else
		arrayCreateParamsBuyer.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nTotal + nBalance)));

	arrayCreateParamsBuyer.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	

	arrayCreateParams1.push_back(inputs);
	arrayCreateParams1.push_back(arrayCreateParamsArbiter);
	arrayCreateParams1.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams1.push_back(escrow.extTxId.IsNull());

	arrayCreateParams2.push_back(inputs);
	arrayCreateParams2.push_back(arrayCreateParamsBuyer);
	arrayCreateParams2.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams2.push_back(escrow.extTxId.IsNull());

	UniValue resCreate1, resCreate2;
	try
	{
		resCreate1 = tableRPC.execute("createrawtransaction", arrayCreateParams1);
		resCreate2 = tableRPC.execute("createrawtransaction", arrayCreateParams2);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());	
	}
	if (!resCreate1.isStr() || !resCreate2.isStr())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not create escrow transaction: Invalid response from createrawtransaction"));
	string createEscrowSpendingTx1 = resCreate1.get_str();
	string createEscrowSpendingTx2 = resCreate2.get_str();
	CTransaction rawTx1;
	DecodeHexTx(rawTx1, createEscrowSpendingTx1);
	CTransaction rawTx2;
	DecodeHexTx(rawTx2, createEscrowSpendingTx2);
	for (int i = 0; i < escrow.scriptSigs.size(); i++) {
		if (rawTx1.vin.size() >= i)
			rawTx1.vin[i].scriptSig = CScript() << std::vector<unsigned char>(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
		if (rawTx2.vin.size() >= i)
			rawTx2.vin[i].scriptSig = CScript() << std::vector<unsigned char>(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
	}
	string strRawTx1 = EncodeHexTx(rawTx1);
	string strRawTx2 = EncodeHexTx(rawTx2);
    // Seller signs it
	if(pwalletMain)
	{
		CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
		pwalletMain->AddCScript(inner);
	}
	UniValue resSign;
	try
	{
		UniValue signUniValue(UniValue::VARR);
		signUniValue.push_back(strRawTx1);
		resSign = tableRPC.execute("signrawtransaction", signUniValue);
	}
	catch (UniValue& objError)
	{
		try
		{
			UniValue signUniValue(UniValue::VARR);
			signUniValue.push_back(strRawTx2);
			resSign = tableRPC.execute("signrawtransaction", signUniValue);
		}
		catch (UniValue& objError)
		{
			throw runtime_error(find_value(objError, "message").get_str());
		}
	}
	if (!resSign.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4560 - " + _("Could not sign escrow transaction: Invalid response from signrawtransaction"));

	const UniValue& o = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();


	const UniValue& complete_value = find_value(o, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(!bComplete)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4561 - " + _("Escrow is incomplete"));

	CTransaction rawTransaction;
	DecodeHexTx(rawTransaction,hex_str);
	ret.push_back(hex_str);
	ret.push_back(rawTransaction.GetHash().GetHex());
	ret.push_back(nBalance);
	return ret;


}
UniValue escrowcompleterelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
		"escrowcompleterelease <escrow guid> <rawtx> [witness]\n"
                         "Completes an escrow release by creating the escrow complete release transaction on syscoin blockchain.\n"
						 "<rawtx> Raw syscoin escrow transaction. Enter the raw tx result from escrowclaimrelease.\n"
                         "<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						 + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CTransaction myRawTx;
	DecodeHexTx(myRawTx,rawTx);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 2))
		vchWitness = vchFromValue(params[2]);
    // this is a syscoin transaction
    CWalletTx wtx;

	

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4562 - " + _("Could not find a escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, buyerAlias, sellerAlias, arbiterAlias;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}


	CScript scriptPubKeyAlias;
	
	scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += sellerScript;



	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(sellerAliasLatest.vchAlias, sellerAliasLatest.txHash, sellerAliasLatest.vchGUID);
	escrow.redeemTxId = myRawTx.GetHash();

    CScript scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyArbiter;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
    scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyBuyer += buyerScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientArbiter;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(sellerScript, sellerAliasLatest.vchAlias, sellerAliasLatest.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, sellerAliasLatest.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, sellerAliasLatest.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue returnRes;
	UniValue sendParams(UniValue::VARR);
	sendParams.push_back(rawTx);
	try
	{
		// broadcast the payment transaction to syscoin network if not external transaction
		if (!extPayment)
			returnRes = tableRPC.execute("sendrawtransaction", sendParams);
	}
	catch (UniValue& objError)
	{
	}
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	UniValue res(UniValue::VARR);
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}
UniValue escrowrefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 4 || params.size() < 3)
        throw runtime_error(
		"escrowrefund <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [witness]\n"
                        "Refunds escrow funds back to buyer, buyer needs to sign the output transaction and send to the network. User role represents either 'seller' or 'arbiter'. Third parameter is array of input (txid, vout, amount) pairs to be used to fund the refund of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	const UniValue &inputs = params[2].get_array();
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 3))
		vchWitness = vchFromValue(params[3]);
    // this is a syscoin transaction
    CWalletTx wtx;

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4564 - " + _("Could not find a escrow with this key"));

 
	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}

	CAmount nEscrowTotal = escrow.nTotal;
	CAmount nTotal = escrow.nTotal - escrow.nArbiterFee - escrow.nNetworkFee;

	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (nBalance < nEscrowTotal) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(nEscrowTotal) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
	}
	nBalance -= nEscrowTotal;
	vector<unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	CScript scriptPubKeyAlias;
	CScript scriptPubKeyAliasOrig;
	// who is initiating release arbiter or seller?
	if(role == "arbiter")
	{	
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += arbiterScript;
		scriptPubKeyAliasOrig = arbiterScript;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
	}
	else if(role == "seller")
	{		
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += sellerScript;
		scriptPubKeyAliasOrig = sellerScript;
		vchLinkAlias = sellerAliasLatest.vchAlias;
		theAlias = sellerAliasLatest;
	}

	// refunds buyer from escrow
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createAddressUniValue(UniValue::VOBJ);
	
	if(role == "arbiter")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nTotal+nBalance)));
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	}
	else if(role == "seller")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nTotal+nBalance+escrow.nArbiterFee)));
	}
	arrayCreateParams.push_back(inputs);
	arrayCreateParams.push_back(createAddressUniValue);
	arrayCreateParams.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams.push_back(escrow.extTxId.IsNull());
	UniValue resCreate;
	try
	{
		resCreate = tableRPC.execute("createrawtransaction", arrayCreateParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate.isStr())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4572 - " + _("Could not create escrow transaction: Invalid response from createrawtransaction"));
	string createEscrowSpendingTx = resCreate.get_str();
	// Buyer/Arbiter signs it
	if(pwalletMain)
	{
		CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
		pwalletMain->AddCScript(inner);
	}
 	UniValue signUniValue(UniValue::VARR);
 	signUniValue.push_back(createEscrowSpendingTx);
	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", signUniValue);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resSign.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4574 - " + _("Could not sign escrow transaction: Invalid response from signrawtransaction"));

	const UniValue& o = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();

	if(createEscrowSpendingTx == hex_str)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4575 - " + _("Could not sign escrow transaction: Signature not added to transaction"));

	CTransaction signedTx;
	DecodeHexTx(signedTx, hex_str);
	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_REFUND;
	for (int i = 0; i < signedTx.vin.size(); i++) {
		if (!signedTx.vin[i].scriptSig.empty())
			escrow.scriptSigs.push_back((CScriptBase)signedTx.vin[i].scriptSig);
	}
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(theAlias.vchAlias, theAlias.txHash, theAlias.vchGUID);

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

    CScript scriptPubKeyOrigBuyer, scriptPubKeyOrigArbiter;


	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigBuyer += buyerScript;


	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);

	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, sellerAliasLatest.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign1 = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign1.get_obj();
	hex_str = "";
	string txid_str = "";
	const UniValue& hex_value1 = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value1.isStr())
		hex_str = hex_value1.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}
UniValue escrowclaimrefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 3 || params.size() < 2)
        throw runtime_error(
		"escrowclaimrefund <escrow guid> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [witness]\n"
                        "Claim escrow funds released from seller or arbiter using escrowrefund. Second parameter is array of input (txid, vout, amount) pairs to be used to fund the refund of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	const UniValue &inputs = params[1].get_array();
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 2))
		vchWitness = vchFromValue(params[2]);
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4576 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias;
	CPubKey sellerKey;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);

	CAliasIndex buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CAmount nEscrowTotal = escrow.nTotal;
	CAmount nTotal = escrow.nTotal - escrow.nArbiterFee - escrow.nNetworkFee;

	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (nBalance < nEscrowTotal) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(nEscrowTotal) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
	}
	nBalance -= nEscrowTotal;
	UniValue arrayCreateParams1(UniValue::VARR);
	UniValue arrayCreateParams2(UniValue::VARR);
	UniValue arrayCreateParamsArbiter(UniValue::VOBJ);
	UniValue arrayCreateParamsSeller(UniValue::VOBJ);
	
	arrayCreateParamsArbiter.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nTotal + nBalance)));
	arrayCreateParamsArbiter.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	
	arrayCreateParamsSeller.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nTotal + nBalance + escrow.nArbiterFee)));
	

	arrayCreateParams1.push_back(inputs);
	arrayCreateParams1.push_back(arrayCreateParamsArbiter);
	arrayCreateParams1.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams1.push_back(escrow.extTxId.IsNull());

	arrayCreateParams2.push_back(inputs);
	arrayCreateParams2.push_back(arrayCreateParamsSeller);
	arrayCreateParams2.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams2.push_back(escrow.extTxId.IsNull());

	UniValue resCreate1, resCreate2;
	try
	{
		resCreate1 = tableRPC.execute("createrawtransaction", arrayCreateParams1);
		resCreate2 = tableRPC.execute("createrawtransaction", arrayCreateParams2);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate1.isStr() || !resCreate2.isStr())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not create escrow transaction: Invalid response from createrawtransaction"));
	string createEscrowSpendingTx1 = resCreate1.get_str();
	string createEscrowSpendingTx2 = resCreate2.get_str();
	CTransaction rawTx1;
	DecodeHexTx(rawTx1, createEscrowSpendingTx1);
	CTransaction rawTx2;
	DecodeHexTx(rawTx2, createEscrowSpendingTx2);
	for (int i = 0; i < escrow.scriptSigs.size(); i++) {
		if (rawTx1.vin.size() >= i)
			rawTx1.vin[i].scriptSig = CScript() << std::vector<unsigned char>(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
		if (rawTx2.vin.size() >= i)
			rawTx2.vin[i].scriptSig = CScript() << std::vector<unsigned char>(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
	}
	string strRawTx1 = EncodeHexTx(rawTx1);
	string strRawTx2 = EncodeHexTx(rawTx2);
	// Buyer signs it
	if (pwalletMain)
	{
		CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
		pwalletMain->AddCScript(inner);
	}
	UniValue resSign;
	try
	{
		UniValue signUniValue(UniValue::VARR);
		signUniValue.push_back(strRawTx1);
		resSign = tableRPC.execute("signrawtransaction", signUniValue);
	}
	catch (UniValue& objError)
	{
		try
		{
			UniValue signUniValue(UniValue::VARR);
			signUniValue.push_back(strRawTx2);
			resSign = tableRPC.execute("signrawtransaction", signUniValue);
		}
		catch (UniValue& objError)
		{
			throw runtime_error(find_value(objError, "message").get_str());
		}
	}
	if (!resSign.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4560 - " + _("Could not sign escrow transaction: Invalid response from signrawtransaction"));

 	
	const UniValue& o = resSign.get_obj();
	string hex_str = "";

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();

	const UniValue& complete_value = find_value(o, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(!bComplete)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4595 - " + _("Escrow is incomplete"));

	CTransaction rawTransaction;
	DecodeHexTx(rawTransaction,hex_str);
	UniValue ret(UniValue::VARR);
	ret.push_back(hex_str);
	ret.push_back(rawTransaction.GetHash().GetHex());
	ret.push_back(nBalance);
	return ret;
}
UniValue escrowcompleterefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
		"escrowcompleterefund <escrow guid> <rawtx> [witness]\n"
                         "Completes an escrow refund by creating the escrow complete refund transaction on syscoin blockchain.\n"
						 "<rawtx> Raw syscoin escrow transaction. Enter the raw tx result from escrowclaimrefund.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CTransaction myRawTx;
	DecodeHexTx(myRawTx,rawTx);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 2))
		vchWitness = vchFromValue(params[2]);
    // this is a syscoin transaction
    CWalletTx wtx;

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4596 - " + _("Could not find a escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, buyerAlias, sellerAlias, arbiterAlias;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}

	CScript scriptPubKeyAlias;

	scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += buyerScript;



	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(buyerAliasLatest.vchAlias, buyerAliasLatest.txHash, buyerAliasLatest.vchGUID);
	escrow.redeemTxId = myRawTx.GetHash();

    CScript scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyArbiter;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
    scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyBuyer += buyerScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(buyerScript, buyerAliasLatest.vchAlias, buyerAliasLatest.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, buyerAliasLatest.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, buyerAliasLatest.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue returnRes;
	UniValue sendParams(UniValue::VARR);
	sendParams.push_back(rawTx);
	try
	{
		// broadcast the payment transaction to syscoin network if not external transaction
		if (!extPayment)
			returnRes = tableRPC.execute("sendrawtransaction", sendParams);
	}
	catch (UniValue& objError)
	{
	}
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	UniValue res(UniValue::VARR);
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}
UniValue escrowfeedback(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 6 || params.size() > 7)
        throw runtime_error(
		"escrowfeedback <escrow guid> <user role> <feedbackprimary> <ratingprimary> <feedbacksecondary> <ratingasecondary> [witness]\n"
                        "Send feedback for primary and secondary users in escrow, depending on who you are. Ratings are numbers from 1 to 5. User Role is either 'buyer', 'seller', 'reseller', or 'arbiter'.\n"
						"If you are the buyer, feedbackprimary is for seller and feedbacksecondary is for arbiter.\n"
						"If you are the seller, feedbackprimary is for buyer and feedbacksecondary is for arbiter.\n"
						"If you are the arbiter, feedbackprimary is for buyer and feedbacksecondary is for seller.\n"
						"If arbiter didn't do any work for this escrow you can leave his feedback empty and rating as a 0.\n"
                        + HelpRequiringPassphrase());
   // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	int nRatingPrimary = 0;
	int nRatingSecondary = 0;
	vector<unsigned char> vchFeedbackPrimary;
	vector<unsigned char> vchFeedbackSecondary;
	vchFeedbackPrimary = vchFromValue(params[2]);
	nRatingPrimary = boost::lexical_cast<int>(params[3].get_str());
	vchFeedbackSecondary = vchFromValue(params[4]);
	nRatingSecondary = boost::lexical_cast<int>(params[5].get_str());
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 6))
		vchWitness = vchFromValue(params[6]);
    // this is a syscoin transaction
    CWalletTx wtx;
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4598 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest, buyerAlias, sellerAlias, arbiterAlias, resellerAlias;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress, resellerPaymentAddress;
	CScript arbiterScript;
	GetAlias(CNameTXIDTuple(escrow.arbiterAliasTuple.first, escrow.arbiterAliasTuple.second), arbiterAlias);
	if (GetAlias(escrow.arbiterAliasTuple.first, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	GetAlias(CNameTXIDTuple(escrow.buyerAliasTuple.first, escrow.buyerAliasTuple.second), buyerAlias);
	if (GetAlias(escrow.buyerAliasTuple.first, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	GetAlias(CNameTXIDTuple(escrow.sellerAliasTuple.first, escrow.sellerAliasTuple.second), sellerAlias);
	if (GetAlias(escrow.sellerAliasTuple.first, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}
	CScript resellerScript;
	GetAlias(CNameTXIDTuple(escrow.linkSellerAliasTuple.first, escrow.linkSellerAliasTuple.second), resellerAlias);
	if (GetAlias(escrow.linkSellerAliasTuple.first, resellerAliasLatest))
	{
		GetAddress(resellerAliasLatest, &resellerPaymentAddress, resellerScript, escrow.nPaymentOption);
	}

	vector <unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;

	if(role == "buyer")
	{
			
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += buyerScript;
		scriptPubKeyAliasOrig = buyerScript;
		vchLinkAlias = buyerAliasLatest.vchAlias;
		theAlias = buyerAliasLatest;
	}
	else if(role == "seller")
	{	
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += sellerScript;
		scriptPubKeyAliasOrig = sellerScript;
		vchLinkAlias = sellerAliasLatest.vchAlias;
		theAlias = sellerAliasLatest;
	}
	else if(role == "reseller")
	{
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << resellerAliasLatest.vchAlias << resellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += resellerScript;
		scriptPubKeyAliasOrig = resellerScript;
		vchLinkAlias = resellerAliasLatest.vchAlias;
		theAlias = resellerAliasLatest;
	}
	else if(role == "arbiter")
	{		
		scriptPubKeyAlias  = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += arbiterScript;
		scriptPubKeyAliasOrig = arbiterScript;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
	}

	escrow.ClearEscrow();
	escrow.vchEscrow = vchEscrow;
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(theAlias.vchAlias, theAlias.txHash, theAlias.vchGUID);
	// buyer
	if(role == "buyer")
	{
		CFeedback sellerFeedback(FEEDBACKBUYER, FEEDBACKSELLER);
		sellerFeedback.vchFeedback = vchFeedbackPrimary;
		sellerFeedback.nRating = nRatingPrimary;
		CFeedback arbiterFeedback(FEEDBACKBUYER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;
		
	}
	// seller
	else if(role == "seller")
	{
		CFeedback buyerFeedback(FEEDBACKSELLER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		CFeedback arbiterFeedback(FEEDBACKSELLER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;

	}
	else if(role == "reseller")
	{
		CFeedback buyerFeedback(FEEDBACKSELLER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		CFeedback arbiterFeedback(FEEDBACKSELLER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;

	}
	// arbiter
	else if(role == "arbiter")
	{
		CFeedback buyerFeedback(FEEDBACKARBITER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		CFeedback sellerFeedback(FEEDBACKARBITER, FEEDBACKSELLER);
		sellerFeedback.vchFeedback = vchFeedbackSecondary;
		sellerFeedback.nRating = nRatingSecondary;

	}
	else
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4603 - " + _("You must be either the arbiter, buyer or seller to leave feedback on this escrow"));
	}
	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
	CScript scriptPubKeyBuyer, scriptPubKeySeller,scriptPubKeyArbiter;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientSeller, recipientArbiter;
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_COMPLETE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyBuyer += buyerScript;
	scriptPubKeyArbiter << CScript::EncodeOP_N(OP_ESCROW_COMPLETE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyArbiter += arbiterScript;
	scriptPubKeySeller << CScript::EncodeOP_N(OP_ESCROW_COMPLETE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeySeller += sellerScript;
	CreateRecipient(scriptPubKeySeller, recipientSeller);
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	CreateRecipient(scriptPubKeyArbiter, recipientArbiter);
	// buyer
	if(role == "buyer")
	{
		vecSend.push_back(recipientBuyer);
	}
	// seller
	else if(role == "seller" || role == "reseller")
	{
		vecSend.push_back(recipientSeller);
	}
	// arbiter
	else if(role == "arbiter")
	{
		vecSend.push_back(recipientArbiter);
	}
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, theAlias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	UniValue signParams(UniValue::VARR);
	signParams.push_back(EncodeHexTx(wtx));
	const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
	const UniValue& so = resSign.get_obj();
	string hex_str = "";
	string txid_str = "";
	const UniValue& hex_value = find_value(so, "hex");
	const UniValue& txid_value = find_value(so, "txid");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	if (txid_value.isStr())
		txid_str = txid_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(bComplete)
	{
		res.push_back(txid_str);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back("false");
	}
	return res;
}
UniValue escrowinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 > params.size())
        throw runtime_error("escrowinfo <guid>\n"
                "Show stored values of a single escrow\n");

    vector<unsigned char> vchEscrow = vchFromValue(params[0]);

    UniValue oEscrow(UniValue::VOBJ);
	CEscrow txPos;
	uint256 txid;
	if (!pescrowdb || !pescrowdb->ReadEscrowLastTXID(vchEscrow, txid))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from escrow DB"));
	if (!pescrowdb->ReadEscrow(CNameTXIDTuple(vchEscrow, txid), txPos))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from escrow DB"));

	CTransaction tx;
	if (!GetSyscoinTransaction(txPos.nHeight, txPos.txHash, tx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to read from escrow tx"));
	vector<vector<unsigned char> > vvch;
	int op, nOut;
	if (!DecodeEscrowTx(tx, op, nOut, vvch))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to decode escrow"));

	if(!BuildEscrowJson(txPos, vvch, oEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4605 - " + _("Could not find this escrow"));
    return oEscrow;
}
void BuildFeedbackJson(const CEscrow& escrow, UniValue& oFeedback) {
	string sFeedbackTime;
	if (escrow.feedback.IsNull())
		return;
	if (chainActive.Height() >= escrow.nHeight) {
		CBlockIndex *pindex = chainActive[escrow.nHeight];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->nTime);
		}
	}
	const string &id = stringFromVch(escrow.vchEscrow) + boost::lexical_cast<std::string>(escrow.feedback.nFeedbackUserTo);
	oFeedback.push_back(Pair("_id", id));
	oFeedback.push_back(Pair("offer", stringFromVch(escrow.offerTuple.first)));
	oFeedback.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));
	oFeedback.push_back(Pair("txid", escrow.txHash.GetHex()));
	oFeedback.push_back(Pair("time", sFeedbackTime));
	oFeedback.push_back(Pair("rating", escrow.feedback.nRating));
	oFeedback.push_back(Pair("feedbackuser", escrow.feedback.nFeedbackUserFrom));
	oFeedback.push_back(Pair("feedback", stringFromVch(escrow.feedback.vchFeedback)));
}
bool BuildEscrowJson(const CEscrow &escrow, const std::vector<std::vector<unsigned char> > &vvch, UniValue& oEscrow)
{
    oEscrow.push_back(Pair("_id", stringFromVch(escrow.vchEscrow)));
	int64_t nTime = 0;
	if (chainActive.Height() >= escrow.nHeight) {
		CBlockIndex *pindex = chainActive[escrow.nHeight];
		if (pindex) {
			nTime = pindex->nTime;
		}
	}
	oEscrow.push_back(Pair("time", nTime));
	oEscrow.push_back(Pair("seller", stringFromVch(escrow.sellerAliasTuple.first)));
	oEscrow.push_back(Pair("arbiter", stringFromVch(escrow.arbiterAliasTuple.first)));
	oEscrow.push_back(Pair("buyer", stringFromVch(escrow.buyerAliasTuple.first)));
	oEscrow.push_back(Pair("offer", stringFromVch(escrow.offerTuple.first)));
	oEscrow.push_back(Pair("offerlink_seller", stringFromVch(escrow.linkSellerAliasTuple.first)));
	oEscrow.push_back(Pair("quantity", (int)escrow.nQty));
	const UniValue &totalValue = ValueFromAmount(escrow.nTotal);
	oEscrow.push_back(Pair("stotal", escrow.sTotal));
	oEscrow.push_back(Pair("total", totalValue));
	oEscrow.push_back(Pair("commission", ValueFromAmount(escrow.nCommission)));
	oEscrow.push_back(Pair("arbiterfee", ValueFromAmount(escrow.nArbiterFee)));
	oEscrow.push_back(Pair("networkfee", ValueFromAmount(escrow.nNetworkFee)));
	oEscrow.push_back(Pair("currency", escrow.bCoinOffer? GetPaymentOptionsString(escrow.nPaymentOption):escrow.sCurrencyCode));
	oEscrow.push_back(Pair("exttxid", escrow.extTxId.IsNull()? "": escrow.extTxId.GetHex()));
	CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
	CScriptID innerID(inner);
	const CChainParams::AddressType &myAddressType = PaymentOptionToAddressType(escrow.nPaymentOption);
	CSyscoinAddress escrowAddress(innerID, myAddressType);	
	oEscrow.push_back(Pair("escrowaddress", escrowAddress.ToString()));
	string strRedeemTxId = "";
	if(!escrow.redeemTxId.IsNull())
		strRedeemTxId = escrow.redeemTxId.GetHex();
    oEscrow.push_back(Pair("paymentoption", (int)escrow.nPaymentOption));
	oEscrow.push_back(Pair("redeem_txid", strRedeemTxId));
    oEscrow.push_back(Pair("txid", escrow.txHash.GetHex()));
    oEscrow.push_back(Pair("height", (int64_t)escrow.nHeight));
	int64_t expired_time = GetEscrowExpiration(escrow);
	bool expired = false;
    if(expired_time <= chainActive.Tip()->nTime)
	{
		expired = true;
	}
	string status = "unknown";
	if(escrow.op == OP_ESCROW_ACTIVATE)
		status = "in escrow";
	else if(escrow.op == OP_ESCROW_RELEASE && vvch[1] == vchFromString("0"))
		status = "escrow released";
	else if(escrow.op == OP_ESCROW_RELEASE && vvch[1] == vchFromString("1"))
		status = "escrow release complete";
	else if(escrow.op == OP_ESCROW_REFUND && vvch[1] == vchFromString("0"))
		status = "escrow refunded";
	else if(escrow.op == OP_ESCROW_REFUND && vvch[1] == vchFromString("1"))
		status = "escrow refund complete";
	else if(escrow.op == OP_ESCROW_COMPLETE)
		status = "escrow complete";
	if(escrow.bPaymentAck)
		status += " (acknowledged)";
	else if (!escrow.feedback.IsNull())
		status += " (feedback)";
	oEscrow.push_back(Pair("expired", expired));
	oEscrow.push_back(Pair("status", status));
	return true;
}
void EscrowTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	
	CEscrow escrow;
	if(!escrow.UnserializeFromData(vchData, vchHash))
		return;

	CEscrow dbEscrow;
	GetEscrow(CNameTXIDTuple(escrow.vchEscrow, escrow.txHash), dbEscrow);

	string opName = escrowFromOp(escrow.op);
	if(escrow.bPaymentAck)
		opName += "("+_("acknowledged")+")";
	else if(!escrow.feedback.IsNull())
		opName += "("+_("feedback")+")";
	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(escrow.vchEscrow)));

	if(escrow.bPaymentAck && escrow.bPaymentAck != dbEscrow.bPaymentAck)
		entry.push_back(Pair("paymentacknowledge", escrow.bPaymentAck));

	if(!escrow.feedback.IsNull())
		entry.push_back(Pair("feedback",_("Escrow feedback was given")));
}
