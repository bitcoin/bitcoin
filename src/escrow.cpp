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
extern mongoc_collection_t *escrowbid_collection;
extern mongoc_collection_t *cert_collection;
extern mongoc_collection_t *feedback_collection;
extern CScript _createmultisig_redeemScript(const UniValue& params);
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
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
		|| op == OP_ESCROW_COMPLETE
		|| op == OP_ESCROW_BID
		|| op == OP_ESCROW_ACKNOWLEDGE
		|| op == OP_ESCROW_ADD_SHIPPING;
}
// % fee on escrow value for arbiter
int64_t GetEscrowArbiterFee(const int64_t &escrowValue, const float &fEscrowFee) {

	float fFee = fEscrowFee
	if (fFee < 0.005)
		fFee = 0.005;
	int fee = 1 / fFee;
	int64_t nFee = escrowValue / fee;
	if (nFee < DEFAULT_MIN_RELAY_TX_FEE)
		nFee = DEFAULT_MIN_RELAY_TX_FEE;
	return nFee;
}

// % fee on escrow value for witness
int64_t GetEscrowWitnessFee(const int64_t &escrowValue, const float &fWitnessFee) {

	if (fWitnessFee <= 0)
		return 0;
	int fee = 1 / fWitnessFee;
	int64_t nFee = escrowValue / fee;
	return nFee;
}
// Minimum amount of deposit per auction bid
int64_t GetEscrowDepositFee(const int64_t &escrowValue, const float &fDepositPercentage) {
	if (fDepositPercentage <= 0)
		return 0;
	int fee = 1 / fDepositPercentage;
	int64_t nMinDep = escrowValue / fee;
	return nMinDep;
}
// check that the minimum arbiter fee is found in nBalance (escrow address balance)
bool ValidateArbiterFee(const CAmount &nBalance, const CEscrow &escrow) {
	CAmount nFee = nBalance;
	nFee -= escrow.nTotalWithoutFee;
	nFee -= escrow.nDeposit;
	nFee -= escrow.nNetworkFee;
	nFee -= escrow.nWitnessFee;
	nFee -= escrow.nShipping;
	return GetEscrowArbiterFee(escrow.nTotalWithoutFee, 0.005) <= nFee && nFee >= escrow.nArbiterFee;
}
// check that the minimum deposit is found in nBalance (escrow address balance)
bool ValidateDepositFee(const CAmount &nBalance, const float &fDepositPercentage, const CEscrow &escrow) {
	CAmount nFee = nBalance;
	nFee -= escrow.nTotalWithoutFee;
	nFee -= escrow.nArbiterFee;
	nFee -= escrow.nNetworkFee;
	nFee -= escrow.nWitnessFee;
	nFee -= escrow.nShipping;
	return GetEscrowDepositFee(escrow.nTotalWithoutFee, fDepositPercentage) <= nFee && nFee >= escrow.nDeposit;
}
// check that the minimum network fee is found in nBalance (escrow address balance)
bool ValidateNetworkFee(const CAmount &nBalance, const CEscrow &escrow) {
	CAmount nFee = nBalance;
	nFee -= escrow.nTotalWithoutFee;
	nFee -= escrow.nArbiterFee;
	nFee -= escrow.nDeposit;
	nFee -= escrow.nWitnessFee;
	nFee -= escrow.nShipping;
	return getFeePerByte(escrow.nPaymentOption)*400 <= nFee && nFee >= escrow.nNetworkFee;
}
// check that the witness fee (if present) is found in nBalance (escrow address balance)
bool ValidateWitnessFee(const CAmount &nBalance, const CEscrow &escrow) {
	CAmount nFee = nBalance;
	nFee -= escrow.nTotalWithoutFee;
	nFee -= escrow.nArbiterFee;
	nFee -= escrow.nNetworkFee;
	nFee -= escrow.nDeposit;
	nFee -= escrow.nShipping;
	return nFee >= escrow.nWitnessFee;
}
// check that the shipping amount (if present) is found in nBalance (escrow address balance)
bool ValidateShipping(const CAmount &nBalance, const CEscrow &escrow) {
	CAmount nFee = nBalance;
	nFee -= escrow.nTotalWithoutFee;
	nFee -= escrow.nArbiterFee;
	nFee -= escrow.nNetworkFee;
	nFee -= escrow.nDeposit;
	nFee -= escrow.nWitnessFee;
	return nFee >= escrow.nShipping;
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
void CEscrowDB::WriteEscrowBidIndex(const CEscrow& escrow, const string& status) {
	if (!escrowbid_collection || escrow.op != OP_ESCROW_BID)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(escrow.txHash.GetHex().c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	BuildEscrowBidJson(escrow, status, oName);
	update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!update || !mongoc_collection_update(escrowbid_collection, update_flags, selector, update, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW BID UPDATE ERROR: %s\n", error.message);
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CEscrowDB::RefundEscrowBidIndex(const std::vector<unsigned char>& vchEscrow, const string& status) {
	if (!escrowbid_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_MULTI_UPDATE);
	selector = BCON_NEW("escrow", BCON_UTF8(stringFromVch(vchEscrow).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	bson_t *update = BCON_NEW("$set", "{",
		"status", BCON_UTF8(status.c_str()),
		"}");
	if (!update || !mongoc_collection_update(escrowbid_collection, update_flags, selector, update, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW BID REFUND ERROR: %s\n", error.message);
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CEscrowDB::EraseEscrowBidIndex(const std::vector<unsigned char>& vchEscrow) {
	if (!escrowbid_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("escrow", BCON_UTF8(stringFromVch(vchEscrow).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(escrow_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB ESCROW BID REMOVE ERROR: %s\n", error.message);
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
	bool foundWitnessAlias = false;
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

			else if (!foundAlias && IsAliasOp(pop, true) && vvch.size() >= 4 && theEscrow.vchWitness == vvch[3])
			{
				if (theEscrow.vchWitness.empty())
				{
					foundAlias = true;
					prevAliasOp = pop;
					vvchPrevAliasArgs = vvch;
				}
				else {
					foundWitnessAlias = true;
				}
			}
		}
	}

	CAliasIndex buyerAlias, sellerAlias, arbiterAlias;
    COffer theOffer, myLinkOffer;
	string retError = "";
	CTransaction txOffer;
	int escrowOp = OP_ESCROW_ACTIVATE;
	COffer dbOffer;
	if(fJustCheck)
	{
		if (vvchArgs.empty() || vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4004 - " + _("Escrow guid too big");
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
			case OP_ESCROW_ACKNOWLEDGE:
				if (!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.linkAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.linkAliasTuple.third != vvchPrevAliasArgs[1])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_ACTIVATE:
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
				if (!theEscrow.vchWitness.empty() && !foundWitnessAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Escrow witness missing");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_BID:
				
				if (!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.buyerAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.buyerAliasTuple.third != vvchPrevAliasArgs[1])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4011 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				if (theEscrow.op != OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4012 - " + _("Invalid op, should be escrow activate");
					return error(errorMessage.c_str());
				}
				
				if (theEscrow.vchEscrow != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4014 - " + _("Escrow Guid mismatch");
					return error(errorMessage.c_str());
				}
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4018 - " + _("Cannot leave feedback in escrow bid");
					return error(errorMessage.c_str());
				}
				if (!theEscrow.vchWitness.empty() && !foundWitnessAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Escrow witness missing in bid");
					return error(errorMessage.c_str());
				}
				if(theEscrow.offerTuple.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Escrow offer information missing in bid");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_ADD_SHIPPING:

				if (!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty() || theEscrow.buyerAliasTuple.first != vvchPrevAliasArgs[0] || theEscrow.buyerAliasTuple.third != vvchPrevAliasArgs[1])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4011 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				if (theEscrow.op != OP_ESCROW_ADD_SHIPPING)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4012 - " + _("Invalid op, should be escrow activate");
					return error(errorMessage.c_str());
				}

				if (theEscrow.vchEscrow != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4014 - " + _("Escrow Guid mismatch");
					return error(errorMessage.c_str());
				}
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4018 - " + _("Cannot leave feedback in escrow bid");
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
					if (!theEscrow.vchWitness.empty() && !foundWitnessAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Escrow witness missing");
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
					if (!theEscrow.vchWitness.empty() && !foundWitnessAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Escrow witness missing");
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
		// make sure escrow settings don't change (besides scriptSigs/nTotal's) outside of activation
		if (op != OP_ESCROW_ACTIVATE)
		{
			// save serialized escrow for later use
			CEscrow serializedEscrow = theEscrow;
			if (!GetEscrow(vvchArgs[0], theEscrow))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4037 - " + _("Failed to read from escrow DB");
				return true;
			}
			if (serializedEscrow.buyerAliasTuple != theEscrow.buyerAliasTuple ||
				serializedEscrow.arbiterAliasTuple != theEscrow.arbiterAliasTuple ||
				serializedEscrow.sellerAliasTuple != theEscrow.sellerAliasTuple)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4038 - " + _("Invalid aliases used for escrow transaction");
				return true;
			}
			if (op == OP_ESCROW_ACKNOWLEDGE && theEscrow.bPaymentAck)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4039 - " + _("Escrow already acknowledged");
			}
			if (theEscrow.vchEscrow != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Escrow Guid mismatch");
				return true;
			}

			if (!serializedEscrow.scriptSigs.empty())
				theEscrow.scriptSigs = serializedEscrow.scriptSigs;

			escrowOp = serializedEscrow.op;
			if (op == OP_ESCROW_BID) {
				if (theEscrow.offerTuple != serializedEscrow.offerTuple)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Escrow bid offer mismatch");
					return true;
				}
				if (theEscrow.op != OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Can only bid on an active escrow");
					return true;
				}
				if (serializedEscrow.nTotalWithFee <= theEscrow.nTotalWithFee)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Bid must be higher than the previous bid, please enter a higher amount");
					return true;
				}
				if (!GetOffer(theEscrow.offerTuple, dbOffer))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find escrow offer. It may be expired");
					return true;
				}
				if (theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot bid on an auction after you have used Buy It Now to purchase an offer");
					return true;
				}
				if (!IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot bid on an offer that is not an auction");
					return true;
				}
				if (dbOffer.auctionOffer.fReservePrice > serializedEscrow.fBidPerUnit)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot bid below offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
					return true;
				}
				if (dbOffer.auctionOffer.nExpireTime <= chainActive.Tip()->nTime)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction has expired, cannot place bid!");
					return true;
				}
				if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
				{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction requires a witness signature for each bid but none provided");
				return true;
				}
				if (!dbOffer.linkOfferTuple.first.empty())
				{
					if (!GetOffer(dbOffer.linkOfferTuple, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (!IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot bid on a linked offer that is not an auction");
						return true;
					}
					if (myLinkOffer.auctionOffer.fReservePrice > serializedEscrow.fBidPerUnit)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot bid below linked offer reserve price of: ") + boost::lexical_cast<string>(myLinkOffer.auctionOffer.fReservePrice) + " " + stringFromVch(myLinkOffer.sCurrencyCode);
						return true;
					}
					if (myLinkOffer.auctionOffer.nExpireTime <= chainActive.Tip()->nTime)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Linked offer auction has expired, cannot place bid!");
						return true;
					}
					if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Linked offer auction requires a witness signature for each bid but none provided");
						return true;
					}
				}
				if (!serializedEscrow.vchWitness.empty())
					theEscrow.vchWitness = serializedEscrow.vchWitness;
				theEscrow.fBidPerUnit = serializedEscrow.fBidPerUnit;
				theEscrow.nBidPerUnit = serializedEscrow.nBidPerUnit;
				theEscrow.op = escrowOp;
				theEscrow.txHash = tx.GetHash();
				theEscrow.nHeight = nHeight;
				// write escrow bid
				if (!dontaddtodb)
				{
					pescrowdb->WriteEscrowBid(theEscrow);
				}
			}
			else if (op == OP_ESCROW_ADD_SHIPPING) {
				if (theEscrow.op == OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4044 - " + _("Can only add shipping to an active escrow");
					return true;
				}
				if (serializedEscrow.nShipping <= theEscrow.nShipping)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Shipping total was not increased");
					return true;
				}
				if (serializedEscrow.nTotalWithFee <= theEscrow.nTotalWithFee)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Total with fee must be increased with added shipping cost");
					return true;
				}
				theEscrow.nTotalWithFee = serializedEscrow.nTotalWithFee;
				theEscrow.nShipping = serializedEscrow.nShipping;
			}
			else if (op == OP_ESCROW_ACKNOWLEDGE)
			{
				if (theEscrow.op != OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Can only acknowledge an active escrow");
					return true;
				}
				if (serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4041 - " + _("Only seller can acknowledge an escrow payment");
					return true;
				}
				else
					theEscrow.bPaymentAck = true;


				int nQty = dbOffer.nQty;
				// if this is a linked offer we must update the linked offer qty
				if (GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
				{
					nQty = myLinkOffer.nQty;
				}
				if (nQty != -1)
				{
					if (theEscrow.nQty > nQty)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Not enough quantity left in this offer for this purchase");
						return true;
					}
					if (theEscrow.extTxId.IsNull())
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
			else if (op == OP_ESCROW_REFUND && vvchArgs[1] == vchFromString("0"))
			{
				if (!GetOffer(theEscrow.offerTuple, dbOffer))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find escrow offer. It may be expired");
					return true;
				}
				if (!dbOffer.linkOfferTuple.first.empty())
				{
					if (!GetOffer(dbOffer.linkOfferTuple, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction refund requires a witness signature but none provided");
							return true;
						}
					}
				}
				if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction refund requires a witness signature but none provided");
						return true;
					}
				}

				CAliasIndex alias;
				if (!GetAlias(theEscrow.sellerAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.arbiterAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4043 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}

				if (theEscrow.op == OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4044 - " + _("Can only refund an active escrow");
					return true;
				}
				else if (theEscrow.op == OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4045 - " + _("Cannot refund an escrow that is already released");
					return true;
				}
				else if (serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4046 - " + _("Only arbiter or seller can initiate an escrow refund");
					return true;
				}
				// only the arbiter can re-refund an escrow
				else if (theEscrow.op == OP_ESCROW_REFUND && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4047 - " + _("Only arbiter can refund an escrow after it has already been refunded");
					return true;
				}
				// refund qty only if ack'd
				if (theEscrow.bPaymentAck) {
					if (GetOffer(theEscrow.offerTuple.first, dbOffer))
					{
						int nQty = dbOffer.nQty;
						COffer myLinkOffer;
						if (GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
						{
							nQty = myLinkOffer.nQty;
						}

						if (nQty != -1)
						{
							if (theEscrow.extTxId.IsNull())
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
				if (!serializedEscrow.vchWitness.empty())
					theEscrow.vchWitness = serializedEscrow.vchWitness;

				// if this escrow was actually a series of bids, set the bid status to 'refunded' in escrow bid collection
				if (!dontaddtodb && !theEscrow.bBuyNow) {
					pescrowdb->RefundEscrowBid(theEscrow.vchEscrow);
				}
			}
			else if (op == OP_ESCROW_REFUND && vvchArgs[1] == vchFromString("1"))
			{
				if (theEscrow.op != OP_ESCROW_REFUND)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Can only claim a refunded escrow");
					return true;
				}
				else if (!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				else if (serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4051 - " + _("Only buyer can claim an escrow refund");
					return true;
				}
			}
			else if (op == OP_ESCROW_RELEASE && vvchArgs[1] == vchFromString("0"))
			{
				if (!GetOffer(theEscrow.offerTuple, dbOffer))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find escrow offer. It may be expired");
					return true;
				}
				if (!dbOffer.linkOfferTuple.first.empty())
				{
					if (!GetOffer(dbOffer.linkOfferTuple, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
						{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction release requires a witness signature but none provided");
						return true;
						}
					}
				}
				if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer auction release requires a witness signature but none provided");
					return true;
					}
				}

				CAliasIndex alias;
				if (!GetAlias(theEscrow.buyerAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4052 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.arbiterAliasTuple.first, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4053 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if (theEscrow.op == OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4054 - " + _("Can only release an active escrow");
					return true;
				}
				else if (theEscrow.op == OP_ESCROW_REFUND)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4055 - " + _("Cannot release an escrow that is already refunded");
					return true;
				}
				else if (serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4056 - " + _("Only arbiter or buyer can initiate an escrow release");
					return true;
				}
				// only the arbiter can re-release an escrow
				else if (theEscrow.op == OP_ESCROW_RELEASE && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4057 - " + _("Only arbiter can release an escrow after it has already been released");
					return true;
				}
				if (!serializedEscrow.vchWitness.empty())
					theEscrow.vchWitness = serializedEscrow.vchWitness;

			}
			else if (op == OP_ESCROW_RELEASE && vvchArgs[1] == vchFromString("1"))
			{
				if (theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4058 - " + _("Can only claim a released escrow");
					return true;
				}
				else if (!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				else if (serializedEscrow.linkAliasTuple.first != theEscrow.sellerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4059 - " + _("Only seller can claim an escrow release");
					return true;
				}
				// reduce qty if not already done so by ack
				if (!theEscrow.bPaymentAck)
				{
					int nQty = dbOffer.nQty;
					// if this is a linked offer we must update the linked offer qty
					if (GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
					{
						nQty = myLinkOffer.nQty;
					}
					if (nQty != -1)
					{
						if (theEscrow.nQty > nQty)
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Not enough quantity left in this offer for this purchase");
							return true;
						}
						if (theEscrow.extTxId.IsNull())
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
			}
			else if (op == OP_ESCROW_COMPLETE)
			{
				vector<unsigned char> vchSellerAlias = theEscrow.sellerAliasTuple.first;
				if (!theEscrow.linkSellerAliasTuple.first.empty())
					vchSellerAlias = theEscrow.linkSellerAliasTuple.first;

				if (serializedEscrow.offerTuple.first != theEscrow.offerTuple.first) {
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Offer GUID incorrect in feedback payload");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nFeedbackUserFrom == serializedEscrow.feedback.nFeedbackUserTo)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4061 - " + _("Cannot send yourself feedback");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nRating > 5)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4063 - " + _("Invalid rating, must be less than or equal to 5 and greater than or equal to 0");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKBUYER && serializedEscrow.linkAliasTuple.first != theEscrow.buyerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only buyer can leave this feedback");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKSELLER && serializedEscrow.linkAliasTuple.first != vchSellerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4065 - " + _("Only seller can leave this feedback");
					serializedEscrow = theEscrow;
				}
				else if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKARBITER && serializedEscrow.linkAliasTuple.first != theEscrow.arbiterAliasTuple.first)
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

			if (GetOffer( theEscrow.offerTuple.first, dbOffer))
			{
				if(dbOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(dbOffer.sCategory), "wanted"))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4073 - " + _("Cannot purchase a wanted offer");
				}
				if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (dbOffer.auctionOffer.fReservePrice > theEscrow.fBidPerUnit)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot purchase below offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
						return true;
					}
					if (dbOffer.auctionOffer.nExpireTime <= chainActive.Tip()->nTime && !theEscrow.bBuyNow)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("This auction has expired, cannot place bid");
						return true;
					}
				}
				if (!IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_BUYNOW) && theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Offer does not support the Buy It Now feature");
					return true;
				}
				if (dbOffer.nQty != -1)
				{
					if (theEscrow.nQty > dbOffer.nQty)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Not enough quantity left in this offer for this purchase");
						return true;
					}
				}
				theEscrow.offerTuple = CNameTXIDTuple(dbOffer.vchOffer, dbOffer.txHash);
			}
			else
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4077 - " + _("Cannot find offer for this escrow. It may be expired");
				return true;
			}
			if (!dbOffer.linkOfferTuple.first.empty())
			{
				if (!GetOffer(dbOffer.linkOfferTuple.first, myLinkOffer))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Cannot find linked offer for this escrow");
					return true;
				}
				if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (myLinkOffer.auctionOffer.fReservePrice > theEscrow.fBidPerUnit)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot purchase below linked offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
						return true;
					}
					if (myLinkOffer.auctionOffer.nExpireTime <= chainActive.Tip()->nTime && !theEscrow.bBuyNow)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("This linked offer auction has expired, cannot place bid");
						return true;
					}
				}
				if (!IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_BUYNOW) && theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Linked offer does not support the Buy It Now feature");
					return true;
				}
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
UniValue escrowbid(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 4 || params.size() > 5)
		throw runtime_error(
			"escrowbid <alias> <escrow> <bid_in_payment_option> <bid_in_offer_currency> [witness]\n"
			"<alias> An alias you own.\n"
			"<escrow> Escrow GUID to place bid on.\n"
			"<bid_in_payment_option> Amount to bid on offer through escrow. Bid is in payment option currency. Example: If offer is paid in SYS and you have deposited 10 SYS in escrow and would like to increase your total bid to 14 SYS enter 14 here. It is per unit of purchase.\n"
			"<bid_in_offer_currency> Converted value of bid_in_payment_option from paymentOption currency to offer currency. For example: offer is priced in USD and purchased in BTC, this field will be the BTC/USD value. It is per unit of purchase.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchEscrow = vchFromValue(params[1]);
	CAmount nBid = AmountFromValue(params[2]);
	float fBid = boost::lexical_cast<float>(params[3].get_str());
	uint64_t nHeight = chainActive.Tip()->nHeight;
	// check for alias existence in DB
	CAliasIndex bidalias;
	if (!GetAlias(vchAlias, bidalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Failed to read alias from DB"));

	vector<unsigned char> vchWitness;
	if (CheckParam(params, 4))
		vchWitness = vchFromValue(params[4]);

	CAliasIndex bidderalias;
	if (!GetAlias(vchAlias, bidderalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find alias with this name"));
	CScript scriptPubKeyAliasOrig, scriptPubKeyAlias;
	CSyscoinAddress buyerAddress;
	GetAddress(bidderalias, &buyerAddress, scriptPubKeyAliasOrig);

	CEscrow theEscrow;

	if (!GetEscrow(vchEscrow, theEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Could not find an escrow with this identifier"));
	CSyscoinAddress address(theEscrow.escrowAddress);
	CScript scriptPubKey = GetScriptForDestination(address.Get());

	CWalletTx wtx;
	vector<CRecipient> vecSend;

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << bidderalias.vchAlias << bidderalias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;
	const CNameTXIDTuple &offerTuple = theEscrow.offerTuple;
	theEscrow.ClearEscrow();
	theEscrow.op = OP_ESCROW_ACTIVATE;
	theEscrow.offerTuple = offerTuple;
	theEscrow.nBidPerUnit = nBid;
	theEscrow.fBidPerUnit = fBid;
	theEscrow.nHeight = chainActive.Tip()->nHeight;
	theEscrow.linkAliasTuple = CNameTXIDTuple(bidderalias.vchAlias, bidderalias.txHash, bidderalias.vchGUID);
	theEscrow.vchWitness = vchWitness;
	vector<unsigned char> data;
	theEscrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;
	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_BID) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigBuyer += scriptPubKeyAliasOrig;

	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient recipientBidder;
	CreateRecipient(scriptPubKeyAliasOrig, recipientBidder);
	vecSend.push_back(recipientBidder);


	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, bidderalias.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(vchAlias, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
UniValue escrowaddshipping(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 3 || params.size() > 4)
		throw runtime_error(
			"escrowbid <alias> <escrow> <shipping amount> [witness]\n"
			"<alias> An alias you own.\n"
			"<escrow> Escrow GUID to add shipping to.\n"
			"<shipping amount> Amount to add to shipping for merchant. Amount is in payment option currency. Example: If merchant requests 0.1 BTC for shipping and escrow is paid in BTC, enter 0.1 here.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchEscrow = vchFromValue(params[1]);
	CAmount nShipping = AmountFromValue(params[2]);
	uint64_t nHeight = chainActive.Tip()->nHeight;
	// check for alias existence in DB
	CAliasIndex bidalias;
	if (!GetAlias(vchAlias, bidalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Failed to read alias from DB"));

	vector<unsigned char> vchWitness;
	if (CheckParam(params, 3))
		vchWitness = vchFromValue(params[3]);

	CAliasIndex bidderalias;
	if (!GetAlias(vchAlias, bidderalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find alias with this name"));
	CScript scriptPubKeyAliasOrig, scriptPubKeyAlias;
	CEscrow theEscrow;

	if (!GetEscrow(vchEscrow, theEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Could not find an escrow with this identifier"));

	CSyscoinAddress address(theEscrow.escrowAddress);
	CScript scriptPubKey = GetScriptForDestination(address.Get());

	CWalletTx wtx;
	CRecipient recipientEscrow = { scriptPubKey, nShipping, false };
	vector<CRecipient> vecSend;
	if (theEscrow.extTxId.IsNull())
		vecSend.push_back(recipientEscrow);

	CSyscoinAddress buyerAddress;
	GetAddress(bidderalias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << bidderalias.vchAlias << bidderalias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;

	theEscrow.ClearEscrow();
	theEscrow.op = OP_ESCROW_ADD_SHIPPING;
	theEscrow.nTotalWithFee += nShipping;
	theEscrow.nShipping += nShipping;
	theEscrow.linkAliasTuple = CNameTXIDTuple(bidderalias.vchAlias, bidderalias.txHash, bidderalias.vchGUID);
	theEscrow.nHeight = chainActive.Tip()->nHeight;
	vector<unsigned char> data;
	theEscrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;
	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_ADD_SHIPPING) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigBuyer += scriptPubKeyAliasOrig;


	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient recipientBidder;
	CreateRecipient(scriptPubKeyAliasOrig, recipientBidder);
	vecSend.push_back(recipientBidder);


	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, bidderalias.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(vchAlias, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
UniValue escrownew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 7 ||  params.size() > 15)
        throw runtime_error(
			"escrownew <getamountandaddress> <alias> <offer> <quantity> <buynow> <bid_in_payment_option> <arbiter alias> [bid_in_offer_currency] [shipping amount] [network fee] [arbiter fee] [witness fee] [extTx] [payment option] [witness]\n"
				"<getamountandaddress> True or false. Get deposit and total escrow amount aswell as escrow address for funding. If buynow is false pass bid amount in bid_in_payment_option to get total needed to complete escrow. If buynow is true amount is calculated based on offer price and quantity.\n"
				"<alias> An alias you own.\n"
                "<offer> GUID of offer that this escrow is managing.\n"
                "<quantity> Quantity of items to buy of offer.\n"
				"<buynow> Specify whether the escrow involves purchasing offer for the full offer price if set to true, or through a bidding auction if set to false. If buynow is false, an initial deposit may be used to secure a bid if required by the seller.\n"
				"<bid_in_payment_option> Amount you are willing to pay escrow for this offer. Amount is in paymentOption currency. It is per unit of purchase. If buynow is set to true, this value is disregarded. \n"
				"<arbiter alias> Alias of Arbiter.\n"
				"<bid_in_offer_currency> Converted value of amount_in_payment_option from paymentOption currency to offer currency. For example: offer is priced in USD and purchased in BTC, this field will be the BTC/USD value. If buynow is set to true, this value is disregarded.\n"
				"<shipping amount> Amount to add to shipping for merchant. Amount is in paymentOption currency. Example: If merchant requests 0.1 BTC for shipping and escrow is paid in BTC, enter 0.1 here. Default is 0. Buyer can also add shipping using escrowaddshipping upon merchant request.\n"
				"<network fee> Network fee in satoshi per byte for the transaction. Generally the escrow transaction is about 400 bytes. Default is 25 for SYS or ZEC and 250 for BTC payments.\n"
				"<arbiter fee> Arbiter fee in fractional amount of the amount_in_payment_option value. For example 0.75% is 0.0075 and represents 0.0075*amount_in_payment_option satoshis paid to arbiter in the event arbiter is used to resolve a dispute. Default and minimum is 0.005.\n"
				"<witness fee> Witness fee in fractional amount of the amount_in_payment_option value. For example 0.3% is 0.003 and represents 0.003*amount_in_payment_option satoshis paid to witness in the event witness signs off on an escrow through any of the following calls escrownew/escrowbid/escrowrelease/escrowrefund. Default is 0.\n"
				"<extTx> External transaction ID if paid with another blockchain.\n"
				"<paymentOption> If extTx is defined, specify a valid payment option used to make payment. Default is SYS.\n"
                "<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
				+ HelpRequiringPassphrase());
	bool bGetAmountAndAddress = params[0].get_str() == "true" ? true : false;
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	vector<unsigned char> vchOffer = vchFromValue(params[2]);
	unsigned int nQty = 1;

	try {
		nQty = boost::lexical_cast<unsigned int>(params[3].get_str());
	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4511 - " + _("Invalid quantity value. Quantity must be less than 4294967296."));
	}
	bool bBuyNow = params[4].get_str() == "true"? true: false;
	CAmount nBidPerUnit = AmountFromValue(params[5]);
	uint64_t nHeight = chainActive.Tip()->nHeight;
	string strArbiter = params[6].get_str();
	boost::algorithm::to_lower(strArbiter);
	// check for alias existence in DB
	CAliasIndex arbiteralias;
	if (!GetAlias(vchFromString(strArbiter), arbiteralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Failed to read arbiter alias from DB"));

	float fBidPerUnit = 0;
	if (CheckParam(params, 7))
		fBidPerUnit = boost::lexical_cast<float>(params[7].get_str());

	string extTxIdStr = "";
	if(CheckParam(params, 12))
		extTxIdStr = params[12].get_str();

	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOption = "SYS";
	if(CheckParam(params, 13))
	{
		paymentOption = params[13].get_str();
		boost::algorithm::to_upper(paymentOption);
	}
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOption))
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4510 - " + _("Could not validate the payment option value");
		throw runtime_error(err.c_str());
	}
	// payment options - and convert payment options string to a bitmask for the txn
	uint64_t paymentOptionMask = GetPaymentOptionsMaskFromString(paymentOption);

	CAmount nShipping = 0;
	if (CheckParam(params, 8))
	{
		nShipping = AmountFromValue(params[8].get_str());
	}

	int nNetworkFee = getFeePerByte(paymentOptionMask);
	float fEscrowFee = getEscrowFee();

	float fWitnessFee = 0;
	if (CheckParam(params, 9))
	{
		nNetworkFee = boost::lexical_cast<int>(params[9].get_str());
	}
	if (CheckParam(params, 10))
	{
		fEscrowFee = boost::lexical_cast<float>(params[10].get_str());
	}
	if (CheckParam(params, 11))
	{
		fWitnessFee = boost::lexical_cast<float>(params[11].get_str());
	}

	vector<unsigned char> vchWitness;
	if(CheckParam(params, 14))
		vchWitness = vchFromValue(params[14]);

	CAliasIndex buyeralias;
	if (!GetAlias(vchAlias, buyeralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find buyer alias with this name"));
	

	COffer theOffer, linkedOffer;

	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Could not find an offer with this identifier"));
	float fDepositPercentage = theOffer.auctionOffer.fDepositPercentage;
	CAliasIndex selleralias;
	if (!GetAlias( theOffer.aliasTuple.first, selleralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4514 - " + _("Could not find seller alias with this identifier"));

	if(theOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(theOffer.sCategory), "wanted"))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4515 - " + _("Cannot purchase a wanted offer"));

	CAmount nTotalOfferPrice = AmountFromValue(strprintf("%.*f", 8, theOffer.GetPrice(COfferLinkWhitelistEntry())*nQty));

	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	CAliasIndex theLinkedAlias, reselleralias;
	
	CAmount nCommission = 0;
	if(!theOffer.linkOfferTuple.first.empty())
	{
		if (!GetOffer( theOffer.linkOfferTuple.first, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Trying to accept a linked offer but could not find parent offer"));

		if (!GetAlias( linkedOffer.aliasTuple.first, theLinkedAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4517 - " + _("Could not find an alias with this identifier"));

		if(linkedOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkedOffer.sCategory), "wanted"))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4518 - " + _("Cannot purchase a wanted offer"));

		reselleralias = selleralias;
		selleralias = theLinkedAlias;
		fDepositPercentage = linkedOffer.auctionOffer.fDepositPercentage;
		OfferLinkWhitelistEntry foundEntry;
		theLinkedAlias.offerWhitelist.GetLinkEntryByHash(theOffer.aliasTuple.first, foundEntry);
		CAmount nTotalLinkedOfferPrice = AmountFromValue(strprintf("%.*f", 8, linkedOffer.GetPrice(foundEntry)*nQty));
		nCommission = nTotalOfferPrice - nTotalLinkedOfferPrice;
		if (nCommission < 0)
			nCommission = 0;
	}

	


	CSyscoinAddress buyerAddress;
	GetAddress(buyeralias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyeralias.vchAlias  << buyeralias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;


    // gather inputs
	vector<unsigned char> vchEscrow = vchFromString(GenerateSyscoinGuid());

    // this is a syscoin transaction
    CWalletTx wtx;
    CScript scriptPubKey, scriptPubKeyBuyer;

	string strAddress;


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

	const UniValue &o = resCreate.get_obj();
	const UniValue& redeemScript_value = find_value(o, "redeemScript");
	if (redeemScript_value.isStr())
	{
		redeemScript = ParseHex(redeemScript_value.get_str());
	}
	else
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));

	CScriptID innerID(redeemScript);
	CSyscoinAddress escrowAddress(innerID, PaymentOptionToAddressType(paymentOptionMask));

	const UniValue& address_value = find_value(o, "address");
	strAddress = address_value.get_str();
	
	CSyscoinAddress address(strAddress);
	scriptPubKey = GetScriptForDestination(address.Get());

	CAmount nEscrowFee = GetEscrowArbiterFee(nTotalOfferPrice, fEscrowFee);
	CAmount nWitnessFee = GetEscrowWitnessFee(nTotalOfferPrice, fWitnessFee);
	CAmount nDepositFee = GetEscrowDepositFee(nTotalOfferPrice, fDepositPercentage);
	// 400 bytes * network fee per byte
	nNetworkFee *= 400;
	vector<CRecipient> vecSend;
	CAmount nFees = nEscrowFee + nNetworkFee + nWitnessFee + nShipping;
	if (!bBuyNow)
		nFees += nDepositFee;
	CAmount nAmountWithFee = nTotalOfferPrice+nFees;
	CWalletTx wtx;
	CRecipient recipientEscrow  = {scriptPubKey, bBuyNow? nAmountWithFee: nFees, false};
	// if we are paying with SYS and we are using buy it now to buy at offer price or there is a deposit required as a bidder, then add this recp to vecSend to create payment otherwise no payment to escrow *yet*
	if(extTxIdStr.empty() && (theOffer.auctionOffer.fDepositPercentage > 0 || bBuyNow))
		vecSend.push_back(recipientEscrow);

	if (bGetAmountAndAddress) {
		UniValue res(UniValue::VOBJ);
		res.push_back(Pair("totalwithfees", ValueFromAmount(nAmountWithFee)));
		res.push_back(Pair("fees", ValueFromAmount(nFees)));
		res.push_back(Pair("address", strAddress));
		return res;
	}
	// send to seller/arbiter so they can track the escrow through GUI
    // build escrow
    CEscrow newEscrow;
	newEscrow.op = OP_ESCROW_ACTIVATE;
	newEscrow.vchEscrow = vchEscrow;
	newEscrow.buyerAliasTuple = CNameTXIDTuple(buyeralias.vchAlias, buyeralias.txHash, buyeralias.vchGUID);
	newEscrow.arbiterAliasTuple = CNameTXIDTuple(arbiteralias.vchAlias, arbiteralias.txHash, arbiteralias.vchGUID);
	newEscrow.offerTuple = CNameTXIDTuple(theOffer.vchOffer, theOffer.txHash);
	newEscrow.extTxId = uint256S(extTxIdStr);
	newEscrow.sellerAliasTuple = CNameTXIDTuple(selleralias.vchAlias, selleralias.txHash, selleralias.vchGUID);
	newEscrow.linkSellerAliasTuple = CNameTXIDTuple(reselleralias.vchAlias, reselleralias.txHash, reselleralias.vchGUID);
	newEscrow.nQty = nQty;
	newEscrow.nPaymentOption = paymentOptionMask;
	newEscrow.nHeight = chainActive.Tip()->nHeight;
	newEscrow.nCommission = nCommission;
	newEscrow.nNetworkFee = nNetworkFee;
	newEscrow.nArbiterFee = nEscrowFee;
	newEscrow.nWitnessFee = nWitnessFee;
	newEscrow.nTotalWithFee = nAmountWithFee;
	newEscrow.nTotalWithoutFee = nTotalOfferPrice;
	newEscrow.nDeposit = nDepositFee;
	newEscrow.nShipping = nShipping;
	newEscrow.nBidPerUnit = nBidPerUnit;
	newEscrow.fBidPerUnit = fBidPerUnit;
	newEscrow.vchWitness = vchWitness;
	newEscrow.bBuyNow = bBuyNow;
	newEscrow.escrowAddress = escrowAddress.ToString();
	vector<unsigned char> data;
	newEscrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

	if (newEscrow.nTotalWithFee != (newEscrow.nTotalWithoutFee + nFees))
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4040 - " + _("Mismatch when calculating total amount with fees"));
	}
	if (newEscrow.bBuyNow && newEscrow.nDeposit > 0) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Cannot include deposit when using Buy It Now"));
	}

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
	CreateAliasRecipient(scriptPubKeyAliasOrig, buyeralias.vchAlias, aliasPaymentRecipient);


	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(buyeralias.vchAlias, vchWitness, stringFromVch(theOffer.sCurrencyCode), aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.linkAliasTuple = CNameTXIDTuple(sellerAliasLatest.vchAlias, sellerAliasLatest.txHash, sellerAliasLatest.vchGUID);
	escrow.op = OP_ESCROW_ACKNOWLEDGE;

	vector<unsigned char> data;
	escrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;

	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_ACKNOWLEDGE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyOrigBuyer += buyerScript;

	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(sellerScript, sellerAliasLatest.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
	COffer theOffer, linkedOffer;
	if (!GetOffer(escrow.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find offer related to this escrow"));
	float fDepositPercentage = theOffer.auctionOffer.fDepositPercentage;
	if (!theOffer.linkOfferTuple.first.empty())
	{

		if (!GetOffer(theOffer.linkOfferTuple, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Trying to accept a linked offer but could not find parent offer"));
		fDepositPercentage = linkedOffer.auctionOffer.fDepositPercentage;
	}

	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest, linkSellerAliasLatest, witnessAliasLatest;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment, linkSellerAddress, witnessAddressPayment;
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
	if (GetAlias(escrow.vchWitness, witnessAliasLatest))
	{
		CScript script;
		GetAddress(witnessAliasLatest, &witnessAddressPayment, script, escrow.nPaymentOption);
	}
	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	CAmount nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (escrow.bBuyNow) {
		if (nBalance < escrow.nTotalWithFee) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nTotalWithFee) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	else
	{
		if (nBalance < (escrow.nBidPerUnit+nEscrowFees)) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nBidPerUnit+nEscrowFees) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	CAmount nBalanceTmp = nBalance;
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
	nBalance -= escrow.nNetworkFee;
	nBalance -= escrow.nArbiterFee;
	nBalance -= escrow.nDeposit;
	nBalance -= escrow.nWitnessFee;
	// if linked offer send commission to affiliate
	if (!theOffer.linkOfferTuple.first.empty())
	{
		nBalance -= escrow.nCommission;
		if (escrow.nCommission > 0)
			createAddressUniValue.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
	}
	if (role == "arbiter")
	{
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
		if (escrow.nDeposit > 0)
			createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nDeposit)));
	}
	else if (role == "buyer")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee + escrow.nDeposit)));
	}
	if (escrow.nWitnessFee > 0)
		createAddressUniValue.push_back(Pair(witnessAddressPayment.ToString(), ValueFromAmount(escrow.nWitnessFee)));
	createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nBalance)));


	if (!ValidateArbiterFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate arbiter fee in escrow"));
	}
	if (!ValidateDepositFee(nBalanceTmp, fDepositPercentage, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate deposit in escrow"));
	}
	if (!ValidateNetworkFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate network fee in escrow"));
	}
	if (!ValidateWitnessFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate witness fee in escrow"));
	}
	if (!ValidateShipping(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate shipping in escrow"));
	}
	if (escrow.nTotalWithFee != (escrow.nTotalWithoutFee + nEscrowFees))
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4040 - " + _("Mismatch when calculating total amount with fees"));
	}
	if (escrow.bBuyNow && escrow.nDeposit > 0) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Cannot include deposit when using Buy It Now"));
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
		UniValue arrayParams(UniValue::VARR);
		UniValue arrayOfKeys(UniValue::VARR);

		// standard 2 of 3 multisig
		arrayParams.push_back(2);
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.sellerAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.buyerAliasTuple.first));
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

		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		if (redeemScript_value.isStr())
		{
			redeemScript = ParseHex(redeemScript_value.get_str());
		}
		else
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));

		pwalletMain->AddCScript(redeemScript);
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
	escrow.vchWitness = vchWitness;
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
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	try
	{
		SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	}
	catch (UniValue& objError)
	{
	}
	
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
    if (fHelp || params.size() != 3)
        throw runtime_error(
		"escrowclaimrelease <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]>\n"
                        "Claim escrow funds released from buyer or arbiter using escrowrelease. User role represents either 'seller' or 'arbiter'. Third parameter is array of input (txid, vout, amount) pairs to be used to fund the release of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	const UniValue &inputs = params[2].get_array();

	UniValue ret(UniValue::VARR);
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find a escrow with this key"));
	COffer theOffer, linkedOffer;
	if (!GetOffer(escrow.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find offer related to this escrow"));
	float fDepositPercentage = theOffer.auctionOffer.fDepositPercentage;
	if (!theOffer.linkOfferTuple.first.empty())
	{

		if (!GetOffer(theOffer.linkOfferTuple, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Trying to accept a linked offer but could not find parent offer"));
		fDepositPercentage = linkedOffer.auctionOffer.fDepositPercentage;
	}
	CAliasIndex buyerAlias, arbiterAlias, sellerAlias, buyerAliasLatest, arbiterAliasLatest, sellerAliasLatest, linkSellerAliasLatest, witnessAliasLatest;
	CSyscoinAddress buyerAddressPayment, arbiterAddressPayment, sellerAddressPayment, linkSellerAddress, witnessAddressPayment;
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
	if (GetAlias(escrow.vchWitness, witnessAliasLatest))
	{
		CScript script;
		GetAddress(witnessAliasLatest, &witnessAddressPayment, script, escrow.nPaymentOption);
	}

	CAmount nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (escrow.bBuyNow) {
		if (nBalance < escrow.nTotalWithFee) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nTotalWithFee) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	else
	{
		if (nBalance < (escrow.nBidPerUnit + nEscrowFees)) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nBidPerUnit + nEscrowFees) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	CAmount nBalanceTmp = nBalance;
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createAddressUniValue(UniValue::VOBJ);
	nBalance -= escrow.nNetworkFee;
	nBalance -= escrow.nArbiterFee;
	nBalance -= escrow.nDeposit;
	nBalance -= escrow.nWitnessFee;
	// if linked offer send commission to affiliate
	if (!theOffer.linkOfferTuple.first.empty())
	{
		nBalance -= escrow.nCommission;
		if (escrow.nCommission > 0)
			createAddressUniValue.push_back(Pair(linkSellerAddress.ToString(), ValueFromAmount(escrow.nCommission)));
	}
	if (role == "arbiter")
	{
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
		if (escrow.nDeposit > 0)
			createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nDeposit)));
	}
	else if (role == "buyer")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee + escrow.nDeposit)));
	}
	if (escrow.nWitnessFee > 0)
		createAddressUniValue.push_back(Pair(witnessAddressPayment.ToString(), ValueFromAmount(escrow.nWitnessFee)));
	createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nBalance)));

	if (!ValidateArbiterFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate arbiter fee in escrow"));
	}
	if (!ValidateDepositFee(nBalanceTmp, fDepositPercentage, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate deposit in escrow"));
	}
	if (!ValidateNetworkFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate network fee in escrow"));
	}
	if (!ValidateWitnessFee(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate witness fee in escrow"));
	}
	if (!ValidateShipping(nBalanceTmp, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not validate shipping in escrow"));
	}
	if (escrow.nTotalWithFee != (escrow.nTotalWithoutFee + nEscrowFees))
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4040 - " + _("Mismatch when calculating total amount with fees"));
	}
	if (escrow.bBuyNow && escrow.nDeposit > 0) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Cannot include deposit when using Buy It Now"));
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

	if (pwalletMain)
	{
		UniValue arrayParams(UniValue::VARR);
		UniValue arrayOfKeys(UniValue::VARR);

		// standard 2 of 3 multisig
		arrayParams.push_back(2);
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.sellerAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.buyerAliasTuple.first));
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

		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		if (redeemScript_value.isStr())
		{
			redeemScript = ParseHex(redeemScript_value.get_str());
		}
		else
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));

		pwalletMain->AddCScript(redeemScript);
	}

	CTransaction rawTx;
	DecodeHexTx(rawTx, createEscrowSpendingTx);
	CMutableTransaction rawTxm(rawTx);
	for (int i = 0; i < escrow.scriptSigs.size(); i++) {
		if (rawTxm.vin.size() >= i)
			rawTxm.vin[i].scriptSig = CScript(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
	}
	string strRawTx = EncodeHexTx(rawTxm);

	UniValue signUniValue(UniValue::VARR);
	signUniValue.push_back(strRawTx);

	UniValue resSign;
	string hex_str = "";
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

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();

	CTransaction rawTransaction;
	DecodeHexTx(rawTransaction, hex_str);
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
	CreateAliasRecipient(sellerScript, sellerAliasLatest.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
		throw runtime_error(find_value(objError, "message").get_str());
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


	CAmount nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (escrow.bBuyNow) {
		if (nBalance < escrow.nTotalWithFee) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nTotalWithFee) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	else
	{
		if (nBalance < (escrow.nBidPerUnit + nEscrowFees)) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nBidPerUnit + nEscrowFees) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	CAmount nBalanceTmp = nBalance;

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
	nBalance -= escrow.nNetworkFee;
	if (role == "arbiter")
	{
		nBalance -= escrow.nArbiterFee;
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	}
	createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nBalance)));
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
		vector<unsigned char> redeemScript;
		UniValue arrayParams(UniValue::VARR);
		UniValue arrayOfKeys(UniValue::VARR);

		// standard 2 of 3 multisig
		arrayParams.push_back(2);
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterSellerTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterBuyerTuple.first));
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

		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		if (redeemScript_value.isStr())
		{
			redeemScript = ParseHex(redeemScript_value.get_str());
		}
		else
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));

		pwalletMain->AddCScript(redeemScript);
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
	escrow.vchWitness = vchWitness;
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
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
    if (fHelp || params.size() != 3)
        throw runtime_error(
		"escrowclaimrefund <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]>\n"
                        "Claim escrow funds released from seller or arbiter using escrowrefund. User role represents either 'buyer' or 'arbiter'. Third parameter is array of input (txid, vout, amount) pairs to be used to fund the refund of payment.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	const UniValue &inputs = params[2].get_array();

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

	CAmount nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	if (escrow.bBuyNow) {
		if (nBalance < escrow.nTotalWithFee) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nTotalWithFee) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	else
	{
		if (nBalance < (escrow.nBidPerUnit + nEscrowFees)) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Not enough funds in the escrow address to process this transaction. Expected amount: ") + boost::lexical_cast<string>(escrow.nBidPerUnit + nEscrowFees) + _(" Amount Found: ") + boost::lexical_cast<string>(nBalance));
		}
	}
	CAmount nBalanceTmp = nBalance;
	// refunds buyer from escrow
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createAddressUniValue(UniValue::VOBJ);
	nBalance -= escrow.nNetworkFee;
	if (role == "arbiter")
	{
		nBalance -= escrow.nArbiterFee;
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(escrow.nArbiterFee)));
	}
	createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nBalance)));
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

	if (pwalletMain)
	{
		vector<unsigned char> redeemScript;
		UniValue arrayParams(UniValue::VARR);
		UniValue arrayOfKeys(UniValue::VARR);

		// standard 2 of 3 multisig
		arrayParams.push_back(2);
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterAliasTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterSellerTuple.first));
		arrayOfKeys.push_back(stringFromVch(escrow.arbiterBuyerTuple.first));
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

		const UniValue &o = resCreate.get_obj();
		const UniValue& redeemScript_value = find_value(o, "redeemScript");
		if (redeemScript_value.isStr())
		{
			redeemScript = ParseHex(redeemScript_value.get_str());
		}
		else
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not create escrow transaction: could not find redeem script in response"));

		pwalletMain->AddCScript(redeemScript);
	}

	CTransaction rawTx;
	DecodeHexTx(rawTx, createEscrowSpendingTx);
	CMutableTransaction rawTxm(rawTx);
	for (int i = 0; i < escrow.scriptSigs.size(); i++) {
		if (rawTxm.vin.size() >= i)
			rawTxm.vin[i].scriptSig = CScript(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
	}
	string strRawTx = EncodeHexTx(rawTxm);

	UniValue signUniValue(UniValue::VARR);
	signUniValue.push_back(strRawTx);

	UniValue resSign;
	string hex_str = "";
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

	const UniValue& hex_value = find_value(o, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();

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
	CreateAliasRecipient(buyerScript, buyerAliasLatest.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
		throw runtime_error(find_value(objError, "message").get_str());
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
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(escrow.linkAliasTuple.first, vchWitness, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
void BuildEscrowBidJson(const CEscrow& escrow, const string& status, UniValue& oBid) {
	string sBidTime;
	if (chainActive.Height() >= escrow.nHeight) {
		CBlockIndex *pindex = chainActive[escrow.nHeight];
		if (pindex) {
			sBidTime = strprintf("%llu", pindex->nTime);
		}
	}
	oBid.push_back(Pair("_id", escrow.txHash.GetHex()));
	oBid.push_back(Pair("offer", stringFromVch(escrow.offerTuple.first)));
	oBid.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));
	oBid.push_back(Pair("time", sBidTime));
	oBid.push_back(Pair("bidder", stringFromVch(escrow.linkAliasTuple.first)));
	oBid.push_back(Pair("bid_in_offer_currency_per_unit", escrow.fBidPerUnit));
	oBid.push_back(Pair("bid_in_payment_option_per_unit", ValueFromAmount(escrow.nBidPerUnit)));
	oBid.push_back(Pair("witness", stringFromVch(escrow.vchWitness)));
	oBid.push_back(Pair("status", status));
}
bool BuildEscrowJson(const CEscrow &escrow, const std::vector<std::vector<unsigned char> > &vvch, UniValue& oEscrow)
{
	COffer theOffer;
	if (!GetOffer(escrow.offerTuple, theOffer))
		return false;
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
	oEscrow.push_back(Pair("witness", stringFromVch(escrow.vchWitness)));
	oEscrow.push_back(Pair("offer", stringFromVch(escrow.offerTuple.first)));
	oEscrow.push_back(Pair("offerlink_seller", stringFromVch(escrow.linkSellerAliasTuple.first)));
	oEscrow.push_back(Pair("quantity", (int)escrow.nQty));
	oEscrow.push_back(Pair("total_with_fee", ValueFromAmount(escrow.nTotalWithFee)));
	oEscrow.push_back(Pair("total_without_fee", ValueFromAmount(escrow.nTotalWithoutFee)));
	oEscrow.push_back(Pair("bid_in_offer_currency_per_unit", escrow.fBidPerUnit));
	oEscrow.push_back(Pair("bid_in_payment_option_per_unit", ValueFromAmount(escrow.nBidPerUnit)));
	oEscrow.push_back(Pair("buynow", escrow.bBuyNow));
	oEscrow.push_back(Pair("commission", ValueFromAmount(escrow.nCommission)));
	oEscrow.push_back(Pair("arbiterfee", ValueFromAmount(escrow.nArbiterFee)));
	oEscrow.push_back(Pair("networkfee", ValueFromAmount(escrow.nNetworkFee)));
	oEscrow.push_back(Pair("witnessfee", ValueFromAmount(escrow.nWitnessFee)));
	oEscrow.push_back(Pair("shipping", ValueFromAmount(escrow.nShipping)));
	oEscrow.push_back(Pair("deposit", ValueFromAmount(escrow.nDeposit)));
	oEscrow.push_back(Pair("currency", IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_COIN) ? GetPaymentOptionsString(escrow.nPaymentOption): theOffer.sCurrencyCode));
	oEscrow.push_back(Pair("exttxid", escrow.extTxId.IsNull()? "": escrow.extTxId.GetHex()));
	CScript inner(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end());
	oEscrow.push_back(Pair("escrowaddress", escrow.escrowAddress));
	string strRedeemTxId = "";
	if(!escrow.redeemTxId.IsNull())
		strRedeemTxId = escrow.redeemTxId.GetHex();
    oEscrow.push_back(Pair("paymentoption", GetPaymentOptionsString(escrow.nPaymentOption)));
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
