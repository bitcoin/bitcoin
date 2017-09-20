#include "offer.h"
#include "alias.h"
#include "escrow.h"
#include "cert.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "consensus/validation.h"
#include "chainparams.h"
#include "coincontrol.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
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
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
bool IsOfferOp(int op) {
	return op == OP_OFFER_ACTIVATE
        || op == OP_OFFER_UPDATE;
}

bool ValidatePaymentOptionsMask(const uint64_t &paymentOptionsMask) {
	uint64_t maxVal = PAYMENTOPTION_SYS | PAYMENTOPTION_BTC | PAYMENTOPTION_ZEC;
	return !(paymentOptionsMask < 1 || paymentOptionsMask > maxVal);
}

bool IsValidPaymentOption(const uint64_t &paymentOptionsMask) {
	return (paymentOptionsMask == PAYMENTOPTION_SYS || paymentOptionsMask == PAYMENTOPTION_BTC || paymentOptionsMask == PAYMENTOPTION_ZEC);
}

bool ValidatePaymentOptionsString(const std::string &paymentOptionsString) {
	bool retval = true;
	vector<string> strs;
	boost::split(strs, paymentOptionsString, boost::is_any_of("+"));
	for (size_t i = 0; i < strs.size(); i++) {
		if(strs[i].compare("BTC") != 0 && strs[i].compare("SYS") != 0 && strs[i].compare("ZEC") != 0) {
			retval = false;
			break;
		}
	}
	return retval;
}

uint64_t GetPaymentOptionsMaskFromString(const std::string &paymentOptionsString) {
	vector<string> strs;
	uint64_t retval = 0;
	boost::split(strs, paymentOptionsString, boost::is_any_of("+"));
	for (size_t i = 0; i < strs.size(); i++) {
		if(!strs[i].compare("SYS")) {
			retval |= PAYMENTOPTION_SYS;
		}
		else if(!strs[i].compare("BTC")) {
			retval |= PAYMENTOPTION_BTC;
		}
		else if(!strs[i].compare("ZEC")) {
			retval |= PAYMENTOPTION_ZEC;
		}
		else return 0;
	}
	return retval;
}

bool IsPaymentOptionInMask(const uint64_t &mask, const uint64_t &paymentOption) {
  return mask & paymentOption ? true : false;
}

uint64_t GetOfferExpiration(const COffer& offer) {
	// dont prune by default, set nHeight to future time
	uint64_t nTime = chainActive.Tip()->nTime + 1;
	CAliasUnprunable aliasUnprunable;
	// if service alias exists in unprunable db (this should always exist for any alias that ever existed) then get the last expire height set for this alias and check against it for pruning
	if (paliasdb && paliasdb->ReadAliasUnprunable(offer.aliasTuple.first, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;
	return nTime;
}

string offerFromOp(int op) {
	switch (op) {
	case OP_OFFER_ACTIVATE:
		return "offeractivate";
	case OP_OFFER_UPDATE:
		return "offerupdate";
	default:
		return "<unknown offer op>";
	}
}
bool COffer::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsOffer(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsOffer >> *this;

		vector<unsigned char> vchOfferData;
		Serialize(vchOfferData);
		const uint256 &calculatedHash = Hash(vchOfferData.begin(), vchOfferData.end());
		const vector<unsigned char> &vchRandOffer = vchFromValue(calculatedHash.GetHex());
		if(vchRandOffer != vchHash)
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
bool COffer::UnserializeFromTx(const CTransaction &tx) {
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
void COffer::Serialize(vector<unsigned char> &vchData) {
    CDataStream dsOffer(SER_NETWORK, PROTOCOL_VERSION);
    dsOffer << *this;
	vchData = vector<unsigned char>(dsOffer.begin(), dsOffer.end());
}
bool COfferDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	COffer offer;
	pair<string, CNameTXIDTuple > keyTuple;
	pair<string, vector<unsigned char> > keyVch;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(keyTuple) && keyTuple.first == "offeri") {
				const CNameTXIDTuple &offerTuple = keyTuple.second;
  				if (!GetOffer(offerTuple.first, offer) || chainActive.Tip()->nTime >= GetOfferExpiration(offer))
				{
					servicesCleaned++;
					EraseOffer(offerTuple);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
int IndexOfOfferOutput(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
	vector<vector<unsigned char> > vvch;
	int op;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		// find an output you own
		if (pwalletMain->IsMine(out) && DecodeOfferScript(out.scriptPubKey, op, vvch)) {
			return i;
		}
	}
	return -1;
}

bool GetOffer(const CNameTXIDTuple &offerTuple,
				  COffer& txPos) {
	if (!pofferdb || !pofferdb->ReadOffer(offerTuple, txPos))
		return false;
	return true;
}
bool GetOffer(const vector<unsigned char> &vchOffer,
				  COffer& txPos) {
	uint256 txid;
	if (!pofferdb || !pofferdb->ReadOfferLastTXID(vchOffer, txid))
		return false;
	if (!pofferdb->ReadOffer(CNameTXIDTuple(vchOffer, txid), txPos))
		return false;
	if (chainActive.Tip()->nTime >= GetOfferExpiration(txPos))
	{
		txPos.SetNull();
		string offer = stringFromVch(vchOffer);
		if(fDebug)
			LogPrintf("GetOffer(%s) : expired", offer.c_str());
		return false;
	}
	return true;
}
bool DecodeAndParseOfferTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	COffer offer;
	bool decode = DecodeOfferTx(tx, op, nOut, vvch);
	bool parse = offer.UnserializeFromTx(tx);
	return decode && parse;
}
bool DecodeOfferTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch) {
	bool found = false;

	// Strict check - bug disallowed
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		if (DecodeOfferScript(out.scriptPubKey, op, vvch)) {
			nOut = i; found = true;
			break;
		}
	}
	if (!found) vvch.clear();
	return found;
}

bool DecodeOfferScript(const CScript& script, int& op,
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
	return found && IsOfferOp(op);
}
bool DecodeOfferScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch) {
	CScript::const_iterator pc = script.begin();
	return DecodeOfferScript(script, op, vvch, pc);
}
bool RemoveOfferScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
	int op;
	vector<vector<unsigned char> > vvch;
	CScript::const_iterator pc = scriptIn.begin();

	if (!DecodeOfferScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}

bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add offer in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug)
		LogPrintf("*** OFFER %d %d %s %s %s %d\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK", " VVCH SIZE: ", vvchArgs.size());
	bool foundAlias = false;
	const COutPoint *prevOutput = NULL;
	const CCoins *prevCoins;
	int prevAliasOp = 0;
	vector<vector<unsigned char> > vvchPrevAliasArgs;
	// unserialize msg from txn, check for valid
	COffer theOffer;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	CTxDestination payDest, commissionDest, dest, aliasDest;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theOffer.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR ERRCODE: 1000 - " + _("Cannot unserialize data inside of this transaction relating to an offer");
		return true;
	}
	// Make sure offer outputs are not spent by a regular transaction, or the offer would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION)
	{
		errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1001 - " + _("Non-Syscoin transaction found");
		return true;
	}
	if(fJustCheck)
	{
		if(vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1002 - " + _("Offer arguments incorrect size");
			return error(errorMessage.c_str());
		}
		
		if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1005 - " + _("Hash provided doesn't match the calculated hash of the data");
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
			if(!prevCoins->IsAvailable(prevOutput->n) || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch))
				continue;
			if(foundAlias)
				break;
			if (!foundAlias && IsAliasOp(pop, true) && vvch.size() >= 4 && vvch[3].empty() && ((theOffer.aliasTuple.first == vvch[0] && theOffer.aliasTuple.third == vvch[1])))
			{
				foundAlias = true;
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}

	}


	// unserialize offer from txn, check for valid
	COffer linkOffer;
	COffer myOffer;
	COfferLinkWhitelistEntry entry;
	CCert theCert;
	CAliasIndex theAlias, alias, linkAlias;
	vector<string> categories;
	string retError = "";
	// just check is for the memory pool inclusion, here we can stop bad transactions from entering before we get to include them in a block
	if(fJustCheck)
	{
		if (vvchArgs.empty() || vvchArgs[0].size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1006 - " + _("Offer guid too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.sDescription.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1007 - " + _("Offer description too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.sTitle.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1008 - " + _("Offer title too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.sCategory.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1009 - " + _("Offer category too long");
			return error(errorMessage.c_str());
		}
		if (theOffer.sPrice.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1009 - " + _("Offer price too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.linkOfferTuple.first.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Offer link guid hash too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.sCurrencyCode.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1011 - " + _("Offer currency code too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.linkWhitelist.entries.size() > 1)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1013 - " + _("Offer has too many affiliate entries, only one allowed per transaction");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_OFFER_ACTIVATE:
			if (!theOffer.vchOffer.empty() && theOffer.vchOffer != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1014 - " + _("Offer guid in the data output does not match the guid in the transaction");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			if(!ValidatePaymentOptionsMask(theOffer.paymentOptions))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Invalid payment option");
				return error(errorMessage.c_str());
			}
			if ( theOffer.vchOffer != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1018 - " + _("Offer input and offer guid mismatch");
				return error(errorMessage.c_str());
			}
			
			if(theOffer.nCommission > 100 || theOffer.nCommission < -90)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1019 - " + _("Commission percentage must be between -90 and 100");
				return error(errorMessage.c_str());
			}		
			if(theOffer.linkOfferTuple.first.empty())
			{
				if(theOffer.sCategory.size() < 1)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1020 - " + _("Offer category cannot be empty");
					return error(errorMessage.c_str());
				}
				if(theOffer.sTitle.size() < 1)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1021 - " + _("Offer title cannot be empty");
					return error(errorMessage.c_str());
				}
				if(theOffer.paymentOptions <= 0)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1022 - " + _("Invalid payment option specified");
					return error(errorMessage.c_str());
				}
			}
			if(theOffer.nQty < -1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1023 - " + _("Quantity must be greater than or equal to -1");
				return error(errorMessage.c_str());
			}
			if(!theOffer.certTuple.first.empty() && theOffer.nQty != 1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Quantity must be 1 for a digital offer");
				return error(errorMessage.c_str());
			}


			break;
		case OP_OFFER_UPDATE:
			if (!theOffer.vchOffer.empty() && theOffer.vchOffer != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1014 - " + _("Offer guid in the data output does not match the guid in the transaction");
				return error(errorMessage.c_str());
			}
			if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1026 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}		
			if(theOffer.paymentOptions > 0 && !ValidatePaymentOptionsMask(theOffer.paymentOptions))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1027 - " + _("Invalid payment option");
				return error(errorMessage.c_str());
			}
			if ( theOffer.vchOffer != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1029 - " + _("Offer input and offer guid mismatch");
				return error(errorMessage.c_str());
			}
			if(!theOffer.linkWhitelist.entries.empty() && theOffer.linkWhitelist.entries[0].nDiscountPct > 99 && theOffer.linkWhitelist.entries[0].nDiscountPct != 127)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1030 - " + _("Discount must be between 0 and 99");
				return error(errorMessage.c_str());
			}

			if(theOffer.nQty < -1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1031 - " + _("Quantity must be greater than or equal to -1");
				return error(errorMessage.c_str());
			}
			if(!theOffer.certTuple.first.empty() && theOffer.nQty != 1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1032 - " + _("Quantity must be 1 for a digital offer");
				return error(errorMessage.c_str());
			}
			if(theOffer.nCommission > 100 || theOffer.nCommission < -90)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1034 - " + _("Commission percentage must be between -90 and 100");
				return error(errorMessage.c_str());
			}
			break;
		default:
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1047 - " + _("Offer transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}


	if (!fJustCheck ) {
		COffer dbOffer;
		COffer serializedOffer;
		if(op == OP_OFFER_UPDATE) {
			// save serialized offer for later use
			serializedOffer = theOffer;
			// load the offer data from the DB
			if(!GetOffer(vvchArgs[0], theOffer))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1048 - " + _("Failed to read from offer DB");
				return true;
			}
			dbOffer = theOffer;
			serializedOffer.linkOfferTuple = theOffer.linkOfferTuple;
			serializedOffer.vchOffer = theOffer.vchOffer;
			serializedOffer.nSold = theOffer.nSold;
			theOffer = serializedOffer;
			if(!dbOffer.IsNull())
			{
				// if updating whitelist, we dont allow updating any offer details
				if(theOffer.linkWhitelist.entries.size() > 0)
					theOffer = dbOffer;
				else
				{
					// whitelist must be preserved in serialOffer and db offer must have the latest in the db for whitelists
					theOffer.linkWhitelist.entries = dbOffer.linkWhitelist.entries;
					// some fields are only updated if they are not empty to limit txn size, rpc sends em as empty if we arent changing them
					if(serializedOffer.sCategory.empty())
						theOffer.sCategory = dbOffer.sCategory;
					if(serializedOffer.sTitle.empty())
						theOffer.sTitle = dbOffer.sTitle;
					if (serializedOffer.sPrice.empty())
						theOffer.sPrice = dbOffer.sPrice;
					if(serializedOffer.sDescription.empty())
						theOffer.sDescription = dbOffer.sDescription;
					if(serializedOffer.certTuple.first.empty())
						theOffer.certTuple = dbOffer.certTuple;
					if(serializedOffer.sCurrencyCode.empty())
						theOffer.sCurrencyCode = dbOffer.sCurrencyCode;
					if(serializedOffer.paymentOptions <= 0)
						theOffer.paymentOptions = dbOffer.paymentOptions;


					// non linked offers cant edit commission
					if(theOffer.linkOfferTuple.first.empty())
						theOffer.nCommission = 0;
					if(!theOffer.linkAliasTuple.first.empty())
						theOffer.aliasTuple = theOffer.linkAliasTuple;
					theOffer.linkAliasTuple.first.clear();
				}
			}
		}
		else if(op == OP_OFFER_ACTIVATE)
		{
			uint256 txid;
			if (pofferdb->ReadOfferLastTXID(vvchArgs[0], txid))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1050 - " + _("Offer already exists");
				return true;
			}

			if(!theOffer.linkOfferTuple.first.empty())
			{
				if (!GetOffer( theOffer.linkOfferTuple.first, linkOffer))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1051 - " + _("Linked offer not found. It may be expired");
					return true;
				}
				if (!GetAlias(theOffer.aliasTuple, theAlias))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1051 - " + _("Linked offer alias not found. It may be expired");
					return true;
				}
				// if creating a linked offer we set some mandatory fields from the parent
				theOffer.nQty = linkOffer.nQty;
				theOffer.certTuple = linkOffer.certTuple;
				theOffer.sPrice = linkOffer.sPrice;
				theOffer.paymentOptions = linkOffer.paymentOptions;
				theOffer.fUnits = linkOffer.fUnits;
				theOffer.bCoinOffer = linkOffer.bCoinOffer;
				
			}
			// init sold to 0
 			theOffer.nSold = 0;
		}

		if(op == OP_OFFER_UPDATE)
		{
			// if the txn whitelist entry exists (meaning we want to remove or add)
			if(serializedOffer.linkWhitelist.entries.size() == 1)
			{
				// special case we use to remove all entries
				if(serializedOffer.linkWhitelist.entries[0].nDiscountPct == 127)
				{
					if(theOffer.linkWhitelist.entries.empty())
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1093 - " + _("Whitelist is already empty");
					}
					else
						theOffer.linkWhitelist.SetNull();
				}
				// the stored offer has this entry meaning we want to remove this entry
				else if(theOffer.linkWhitelist.GetLinkEntryByHash(serializedOffer.linkWhitelist.entries[0].aliasLinkVchRand, entry))
				{
					theOffer.linkWhitelist.RemoveWhitelistEntry(serializedOffer.linkWhitelist.entries[0].aliasLinkVchRand);
				}
				// we want to add it to the whitelist
				else
				{
					if(theOffer.linkWhitelist.entries.size() > 20)
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1094 -" + _("Too many affiliates for this offer");
					}
					else if(!serializedOffer.linkWhitelist.entries[0].aliasLinkVchRand.empty())
					{
						if (GetAlias(serializedOffer.linkWhitelist.entries[0].aliasLinkVchRand, theAlias))
						{
							theOffer.linkWhitelist.PutWhitelistEntry(serializedOffer.linkWhitelist.entries[0]);
						}
						else
						{
							errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1095 - " + _("Cannot find the alias you are trying to offer affiliate list. It may be expired");
						}
					}
				}

			}
			// if this offer is linked to a parent update it with parent information
			if(!theOffer.linkOfferTuple.first.empty())
			{
				if (!GetOffer(theOffer.linkOfferTuple.first, linkOffer))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Linked offer not found. It may be expired");
					return true;
				}
				theOffer.nQty = linkOffer.nQty;
				theOffer.certTuple = linkOffer.certTuple;
				theOffer.paymentOptions = linkOffer.paymentOptions;
				theOffer.bCoinOffer = linkOffer.bCoinOffer;
				theOffer.fUnits = linkOffer.fUnits;
				theOffer.sPrice = linkOffer.sPrice;
			}
		}
		theOffer.nHeight = nHeight;
		theOffer.txHash = tx.GetHash();
		// write offer
		if (!dontaddtodb && !pofferdb->WriteOffer(theOffer))
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to write to offer DB");
			return error(errorMessage.c_str());
		}

		// debug
		if (fDebug)
			LogPrintf( "CONNECTED OFFER: op=%s offer=%s qty=%u hash=%s height=%d\n",
				offerFromOp(op).c_str(),
				stringFromVch(vvchArgs[0]).c_str(),
				theOffer.nQty,
				tx.GetHash().ToString().c_str(),
				nHeight);
	}
	return true;
}

UniValue offernew(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 7 || params.size() > 13)
		throw runtime_error(
		"offernew <alias> <category> <title> <quantity> <price> <description> <currency> [cert. guid] [payment options=SYS] [private=false] [units] [coinoffer=false] [witness]\n"
						"<alias> An alias you own.\n"
						"<category> category, 255 chars max.\n"
						"<title> title, 255 chars max.\n"
						"<quantity> quantity, > 0 or -1 for infinite\n"
						"<price> price in <currency>\n"
						"<description> description, 1 KB max.\n"
						"<currency> The currency code that you want your offer to be in ie: USD.\n"
						"<cert. guid> Set this to the guid of a certificate you wish to sell\n"
						"<paymentOptions> 'SYS' to accept SYS only, 'BTC' for BTC only, 'ZEC' for zcash only, or a |-delimited string to accept multiple currencies (e.g. 'BTC|SYS' to accept BTC or SYS). Leave empty for default. Defaults to 'SYS'.\n"		
						"<private> set to Yes if this offer should be private not be searchable. Defaults to No.\n"
						"<units> Units that 1 qty represents. For example if selling 1 BTC.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());
	// gather inputs
	float fPrice;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAliasIndex alias;
	if (!GetAlias(vchAlias, alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1500 - " + _("Could not find an alias with this name"));
	CAliasIndex pegAlias;
	if (!GetAlias(alias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1500 - " + _("Could not find an alias with this name"));

	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
	vector<unsigned char> vchCategory =  vchFromValue(params[1]);
	vector<unsigned char> vchTitle =  vchFromValue(params[2]);
	vector<unsigned char> vchCert;

	int nQty;

	try {
		nQty =  boost::lexical_cast<int>(params[3].get_str());
	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1503 - " + _("Invalid quantity value, must be less than 4294967296 and greater than or equal to -1"));
	}
	fPrice = boost::lexical_cast<float>(params[4].get_str());
	vector<unsigned char> vchDescription = vchFromValue(params[5]);
	vector<unsigned char> vchCurrency = vchFromValue(params[6]);
	CScript scriptPubKeyOrig;
	CScript scriptPubKey;
	if(CheckParam(params, 7))
		vchCert = vchFromValue(params[7]);
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOptions = "SYS";
	if(CheckParam(params, 8))
		paymentOptions = params[8].get_str();		
	boost::algorithm::to_upper(paymentOptions);
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOptions))
	{
		string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1504 - " + _("Could not validate the payment options value");
		throw runtime_error(err.c_str());
	}
	// payment options - and convert payment options string to a bitmask for the txn
	uint64_t paymentOptionsMask = GetPaymentOptionsMaskFromString(paymentOptions);
	bool bPrivate = false;
	if(CheckParam(params, 9))
		bPrivate = params[9].get_str() == "true"? true: false;
	float fUnits=1.0f;
	if(CheckParam(params, 10))
	{
		try {
			fUnits =  boost::lexical_cast<float>(params[10].get_str());
		} catch (std::exception &e) {
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1503 - " + _("Invalid units value"));
		}
	}
	bool bCoinOffer = false;
	if(CheckParam(params, 11))
		bCoinOffer = params[11].get_str() == "true"? true: false;
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 12))
		vchWitness = vchFromValue(params[12]);
	int precision = 2;
	CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(latestAliasPegTuple, vchCurrency, fPrice, precision);
	if(nPricePerUnit == 0)
	{
		string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1505 - " + _("Could not find currency in the peg alias");
		throw runtime_error(err.c_str());
	}
	// if we are selling a cert ensure it exists and pubkey's match (to ensure it doesnt get transferred prior to accepting by user)
	CCert theCert;
	if(!vchCert.empty())
	{
		if (!GetCert( vchCert, theCert))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1506 - " + _("Creating an offer with a cert that does not exist"));
		}
		else if(!boost::algorithm::istarts_with(stringFromVch(vchCategory), "certificates"))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1507 - " + _("Offer selling a certificate must use a certificate category"));
		}
		else if(theCert.aliasTuple.first != vchAlias)
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1508 - " + _("Cannot create this offer because the certificate alias does not match the offer alias"));
		}
	}
	else if(boost::algorithm::istarts_with(stringFromVch(vchCategory), "certificates"))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1509 - " + _("Offer not selling a certificate cannot use a certificate category"));
	}

	// this is a syscoin transaction
	CWalletTx wtx;

	// generate rand identifier
	vector<unsigned char> vchOffer = vchFromString(GenerateSyscoinGuid());
	


	// unserialize offer from txn, serialize back
	// build offer
	COffer newOffer;
	newOffer.aliasTuple = CNameTXIDTuple(alias.vchAlias, alias.txHash,alias.vchGUID);
	newOffer.vchOffer = vchOffer;
	newOffer.sCategory = vchCategory;
	newOffer.sTitle = vchTitle;
	newOffer.sDescription = vchDescription;
	newOffer.nQty = nQty;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	if (!vchCert.empty())
		newOffer.certTuple = CNameTXIDTuple(theCert.vchCert, theCert.txHash);
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = bPrivate;
	newOffer.paymentOptions = paymentOptionsMask;
	newOffer.fUnits = fUnits;
	newOffer.bCoinOffer = bCoinOffer;
	newOffer.paymentPrecision = precision;
	newOffer.sPrice = strprintf("%.*f", precision, fPrice);

	vector<unsigned char> data;
	newOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	CSyscoinAddress aliasAddress;
	GetAddress(alias, &aliasAddress, scriptPubKeyOrig);
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, alias.vchAlias, alias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, alias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(vchAlias, vchWitness, alias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
		res.push_back(stringFromVch(vchOffer));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchOffer));
		res.push_back("false");
	}
	return res;
}

UniValue offerlink(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 3 || params.size() > 5)
		throw runtime_error(
		"offerlink <alias> <guid> <commission> [description] [witness]\n"
						"<alias> An alias you own.\n"
						"<guid> offer guid that you are linking to\n"
						"<commission> percentage of profit desired over original offer price, > 0, ie: 5 for 5%\n"
						"<description> description, 1 KB max. Defaults to original description. Leave as '' to use default.\n"
						+ HelpRequiringPassphrase());
	// gather inputs
	COfferLinkWhitelistEntry whiteListEntry;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAliasIndex alias;
	if (!GetAlias(vchAlias, alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1510 - " + _("Could not find an alias with this name"));
 

	vector<unsigned char> vchLinkOffer = vchFromValue(params[1]);
	vector<unsigned char> vchDescription;
	COffer linkOffer;
	if (vchLinkOffer.empty() || !GetOffer( vchLinkOffer, linkOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1513 - " + _("Could not find an offer with this guid"));

	int commissionInteger = boost::lexical_cast<int>(params[2].get_str());
	if(CheckParam(params, 3))
	{

		vchDescription = vchFromValue(params[3]);
		if(vchDescription.empty())
		{
			vchDescription = linkOffer.sDescription;
		}
	}
	else
	{
		vchDescription = linkOffer.sDescription;
	}
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 4))
		vchWitness = vchFromValue(params[4]);
	COfferLinkWhitelistEntry entry;
	if(linkOffer.linkWhitelist.GetLinkEntryByHash(vchAlias, entry))
	{
		if(commissionInteger <= -entry.nDiscountPct)
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1514 - " + _("This resold offer must be of higher price than the original offer including any discount"));
		}
	}
	// make sure alias exists in the root offer affiliate list
	else
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1515 - " + _("Cannot find this alias in the parent offer affiliate list"));
	}
	if (!linkOffer.linkOfferTuple.first.empty())
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1516 - " + _("Cannot link to an offer that is already linked to another offer"));
	}    
	else if(linkOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkOffer.sCategory), "wanted"))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1517 - " + _("Cannot link to a wanted offer"));
	}	
	CScript scriptPubKeyOrig;
	CScript scriptPubKey;

	// this is a syscoin transaction
	CWalletTx wtx;


	// generate rand identifier
	vector<unsigned char> vchOffer = vchFromString(GenerateSyscoinGuid());
	

	// build offer
	COffer newOffer;
	newOffer.vchOffer = vchOffer;
	newOffer.aliasTuple = CNameTXIDTuple(alias.vchAlias, alias.txHash, alias.vchGUID);
	newOffer.sDescription = vchDescription;
	newOffer.nCommission = commissionInteger;
	newOffer.linkOfferTuple = CNameTXIDTuple(linkOffer.vchOffer, linkOffer.txHash);
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.paymentOptions = linkOffer.paymentOptions;
	newOffer.paymentPrecision = linkOffer.paymentPrecision;
	theOffer.sCurrencyCode = linkOffer.sCurrencyCode;
	theOffer.sCategory = linkOffer.sCategory;
	theOffer.sTitle = linkOffer.sTitle;
	//create offeractivate txn keys

	vector<unsigned char> data;
	newOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	CSyscoinAddress aliasAddress;
	GetAddress(alias, &aliasAddress, scriptPubKeyOrig);
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;


	string strError;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, alias.vchAlias, alias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, alias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(vchAlias, vchWitness, alias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);

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
		res.push_back(stringFromVch(vchOffer));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchOffer));
		res.push_back("false");
	}	return res;
}

UniValue offeraddwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 4)
		throw runtime_error(
		"offeraddwhitelist <offer guid> <alias guid> [discount percentage] [witness]\n"
		"Add to the affiliate list of your offer(controls who can resell).\n"
						"<offer guid> offer guid that you are adding to\n"
						"<alias guid> alias guid representing an alias that you want to add to the affiliate list\n"
						"<discount percentage> percentage of discount given to affiliate for this offer. 0 to 99.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchAlias =  vchFromValue(params[1]);
	int nDiscountPctInteger = 0;

	if(CheckParam(params, 2))
		nDiscountPctInteger = boost::lexical_cast<int>(params[2].get_str());
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 3))
		vchWitness = vchFromValue(params[3]);
	CWalletTx wtx;

	// this is a syscoin txn
	CScript scriptPubKeyOrig;
	// create OFFERUPDATE txn key
	CScript scriptPubKey;


	COffer theOffer;
	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1518 - " + _("Could not find an offer with this guid"));

	CAliasIndex theAlias;
	if (!GetAlias( theOffer.aliasTuple.first, theAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1519 - " + _("Could not find an alias with this name"));


	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);


	COfferLinkWhitelistEntry foundEntry;
	if(theOffer.linkWhitelist.GetLinkEntryByHash(vchAlias, foundEntry))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1522 - " + _("This alias entry already exists on affiliate list"));

	COfferLinkWhitelistEntry entry;
	entry.aliasLinkVchRand = vchAlias;
	entry.nDiscountPct = nDiscountPctInteger;
	theOffer.ClearOffer();
	theOffer.linkWhitelist.PutWhitelistEntry(entry);
	if (theOffer.linkWhitelist.entries.size() != 1 || !theOffer.linkWhitelist.GetLinkEntryByHash(vchAlias, foundEntry))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1523 - " + _("This alias entry was not added to affiliate list"));

	theOffer.nHeight = chainActive.Tip()->nHeight;


	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, theAlias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);

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
UniValue offerremovewhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 3)
		throw runtime_error(
		"offerremovewhitelist <offer guid> <alias guid> [witness]\n"
		"Remove from the affiliate list of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 2))
		vchWitness = vchFromValue(params[2]);
	CTransaction txCert;
	CCert theCert;
	CWalletTx wtx;

	// this is a syscoin txn
	CScript scriptPubKeyOrig;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;

	COffer theOffer;

	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1523 - " + _("Could not find an offer with this guid"));
	CAliasIndex theAlias;
	if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), theAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1524 - " + _("Could not find an alias with this name"));
	

	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

	// create OFFERUPDATE txn keys
	COfferLinkWhitelistEntry foundEntry;
	if(!theOffer.linkWhitelist.GetLinkEntryByHash(vchAlias, foundEntry))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1527 - " + _("This alias entry was not found on affiliate list"));
	theOffer.ClearOffer();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	theOffer.linkWhitelist.PutWhitelistEntry(foundEntry);

	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, theAlias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);

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
UniValue offerclearwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 1 || params.size() > 2)
		throw runtime_error(
		"offerclearwhitelist <offer guid> [witness]\n"
		"Clear the affiliate list of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 1))
		vchWitness = vchFromValue(params[1]);
	// this is a syscoind txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig;

	
	COffer theOffer;
	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1528 - " + _("Could not find an offer with this guid"));

	CAliasIndex theAlias;
	if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), theAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1529 - " + _("Could not find an alias with this name"));


	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

	theOffer.ClearOffer();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;

	COfferLinkWhitelistEntry entry;
	// special case to clear all entries for this offer
	entry.nDiscountPct = 127;
	theOffer.linkWhitelist.PutWhitelistEntry(entry);

	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, theAlias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);

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

UniValue offerwhitelist(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error("offerwhitelist <offer guid> [witness]\n"
                "List all affiliates for this offer.\n");
    UniValue oRes(UniValue::VARR);
    vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 1))
		vchWitness = vchFromValue(params[1]);

	COffer theOffer;

	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("could not find an offer with this guid");

	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CAliasIndex theAlias;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		if (GetAlias(entry.aliasLinkVchRand, theAlias))
		{
			UniValue oList(UniValue::VOBJ);
			oList.push_back(Pair("alias", stringFromVch(entry.aliasLinkVchRand)));
			oList.push_back(Pair("expires_on",theAlias.nExpireTime));
			oList.push_back(Pair("offer_discount_percentage", entry.nDiscountPct));
			oRes.push_back(oList);
		}
    }
    return oRes;
}
UniValue offerupdate(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 14)
		throw runtime_error(
		"offerupdate <alias> <guid> [category] [title] [quantity] [price] [description] [currency] [private=false] [cert. guid] [commission] [paymentOptions] [witness]\n"
						"Perform an update on an offer you control.\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	string strPrivate = "";
	string strQty = "";
	string strPrice = "";
	string strCommission = "";
	string strCategory = "";
	string strDescription = "";
	string strTitle = "";
	string strCert = "";
	string strCurrency = "";
	string paymentOptions = "";
	if(CheckParam(params, 2))
		strCategory = params[2].get_str();
	if(CheckParam(params, 3))
		strTitle = params[3].get_str();
	if(CheckParam(params, 4))
	{
		strQty = params[4].get_str();
		try {
			int nQty = boost::lexical_cast<int>(strQty);

		} catch (std::exception &e) {
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1533 - " + _("Invalid quantity value. Quantity must be less than 4294967296 and greater than or equal to -1"));
		}
	}
	if(CheckParam(params, 5))
	{
		strPrice = params[5].get_str();
		try {
			float fPrice = boost::lexical_cast<float>(strPrice);

		} catch (std::exception &e) {
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1533 - " + _("Invalid price value"));
		}
	}
	if(CheckParam(params, 6))
		strDescription = params[6].get_str();
	if(CheckParam(params, 7))
		strCurrency = params[7].get_str();
	if(CheckParam(params, 8))
		strPrivate = params[8].get_str();
	if(CheckParam(params, 9))
		strCert = params[9].get_str();
	if(CheckParam(params, 11))
		strCommission = params[11].get_str();
	if(CheckParam(params, 12))
	{
		paymentOptions = params[12].get_str();	
		boost::algorithm::to_upper(paymentOptions);
	}
	
	if(!paymentOptions.empty() && !ValidatePaymentOptionsString(paymentOptions))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1532 - " + _("Could not validate payment options string"));
	}
	uint64_t paymentOptionsMask = GetPaymentOptionsMaskFromString(paymentOptions);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 13))
		vchWitness = vchFromValue(params[13]);

	CAliasIndex alias, linkAlias;

	// this is a syscoind txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;

	COffer theOffer, linkOffer;
	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1534 - " + _("Could not find an offer with this guid"));

	if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1535 - " + _("Could not find an alias with this name"));
	if (vchAlias != alias.vchAlias && !GetAlias(vchAlias, linkAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1536 - " + _("Could not find an alias with this name"));
	CAliasIndex pegAlias;
	if (!GetAlias(vchAlias != alias.vchAlias? linkAlias.aliasPegTuple.first: alias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1500 - " + _("Could not find peg alias with this name"));

	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);

	CCert theCert;
	if(!strCert.empty())
	{
		if (!GetCert( vchFromString(strCert), theCert))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1538 - " + _("Updating an offer with a cert that does not exist"));
		}
		else if(theOffer.linkOfferTuple.first.empty() && theCert.aliasTuple.first != theOffer.aliasTuple.first)
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1539 - " + _("Cannot update this offer because the certificate alias does not match the offer alias"));
		}
		if(!boost::algorithm::istarts_with(strCategory, "certificates"))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1540 - " + _("Offer selling a certificate must use a certificate category"));
		}
	}
	else if(boost::algorithm::istarts_with(strCategory, "certificates"))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1541 - " + _("Offer not selling a certificate cannot use a certificate category"));
	}
	if(!theOffer.linkOfferTuple.first.empty())
	{
		COffer linkOffer;
		COfferLinkWhitelistEntry entry;
		if (!GetOffer( theOffer.linkOfferTuple.first, linkOffer))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1542 - " + _("Linked offer not found. It may be expired"));
		}
		else if(!linkOffer.linkWhitelist.GetLinkEntryByHash(vchAlias, entry))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1543 - " + _("Cannot find this alias in the parent offer affiliate list"));
		}
		if (!linkOffer.linkOfferTuple.first.empty())
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1544 - " + _("Cannot link to an offer that is already linked to another offer"));
		}
		else if(linkOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkOffer.sCategory), "wanted"))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1545 - " + _("Cannot link to a wanted offer"));
		}
	}
	if(strCategory.size() > 0 && boost::algorithm::istarts_with(strCategory, "wanted") && !boost::algorithm::istarts_with(stringFromVch(theOffer.sCategory), "wanted"))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1546 - " + _("Cannot change category to wanted"));
	}
	CSyscoinAddress aliasAddress;
	GetAddress(alias, &aliasAddress, scriptPubKeyOrig);

	// create OFFERUPDATE, ALIASUPDATE txn keys
	CScript scriptPubKey;

	COffer offerCopy = theOffer;
	theOffer.ClearOffer();
	theOffer.nHeight = chainActive.Tip()->nHeight;

	if(!strCurrency.empty())
		theOffer.sCurrencyCode = vchFromString(strCurrency);
	if(!strTitle.empty())
		theOffer.sTitle = vchFromString(strTitle);
	if(!strCategory.empty())
		theOffer.sCategory = vchFromString(strCategory);
	if(!strDescription.empty())
		theOffer.sDescription = vchFromString(strDescription);
	float fPrice = 1;
	// linked offers can't change these settings, they are overrided by parent info
	if(!strPrice.empty() && offerCopy.linkOfferTuple.first.empty())
	{
		fPrice = boost::lexical_cast<float>(strPrice);
		if(!strCert.empty())
			theOffer.certTuple = CNameTXIDTuple(theCert.vchCert, theCert.txHash);
		int precision = 2;
		CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(latestAliasPegTuple, strCurrency.empty()? offerCopy.sCurrencyCode: vchFromString(strCurrency), fPrice, precision);
		if(nPricePerUnit == 0)
		{
			string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1549 - " + _("Could not find currency in the peg alias");
			throw runtime_error(err.c_str());
		}
		theOffer.sPrice = strprintf("%.*f", precision, fPrice);
		theOffer.paymentPrecision = precision;
	}

	if(strCommission.empty())
		theOffer.nCommission = offerCopy.nCommission;
	else
		theOffer.nCommission = boost::lexical_cast<int>(strCommission);
	if(paymentOptions.empty())
		theOffer.paymentOptions = offerCopy.paymentOptions;
	else
		theOffer.paymentOptions = paymentOptionsMask;

	if(!vchAlias.empty() && vchAlias != alias.vchAlias)
		theOffer.linkAliasTuple = CNameTXIDTuple(linkAlias.vchAlias, linkAlias.txHash, linkAlias.vchGUID);
	if(strQty.empty())
		theOffer.nQty = offerCopy.nQty;
	else
		theOffer.nQty = boost::lexical_cast<int>(strQty);
	if(strPrivate.empty())
		theOffer.bPrivate = offerCopy.bPrivate;
	else
		theOffer.bPrivate = strPrivate == "true"? true: false;
	theOffer.nHeight = chainActive.Tip()->nHeight;

	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, alias.vchAlias, alias.aliasPegTuple, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, alias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;


	SendMoneySyscoin(alias.vchAlias, vchWitness, alias.aliasPegTuple.first, "", aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
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
void COfferDB::WriteOfferIndex(const COffer& offer) {
	if (!offer_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(offer.vchOffer).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildOfferJson(offer, oName)) {
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(offer_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB OFFER UPDATE ERROR: %s\n", error.message);
		}
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferIndex(const std::vector<unsigned char>& vchOffer) {
	if (!offer_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(vchOffer).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(offer_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB OFFER REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
UniValue offerinfo(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size())
		throw runtime_error("offerinfo <guid>\n"
				"Show offer details\n");

	UniValue oOffer(UniValue::VOBJ);
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	COffer txPos;
	uint256 txid;
	if (!pofferdb || !pofferdb->ReadOfferLastTXID(vchOffer, txid))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from offer DB"));
	if (!pofferdb->ReadOffer(CNameTXIDTuple(vchOffer, txid), txPos))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from offer DB"));

	CTransaction tx;
	if (!GetSyscoinTransaction(txPos.nHeight, txPos.txHash, tx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to read offer tx"));
	vector<vector<unsigned char> > vvch;
	int op, nOut;
	if (!DecodeOfferTx(tx, op, nOut, vvch))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to decode offer"));

	if(!BuildOfferJson(txPos, oOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1593 - " + _("Could not find this offer"));

	return oOffer;

}
bool BuildOfferJson(const COffer& theOffer, UniValue& oOffer)
{
	CAliasIndex alias, latestAlias;
	if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), alias))
		return false;
	uint256 txid;
	if (!paliasdb || !paliasdb->ReadAliasLastTXID(theOffer.aliasTuple.first, txid))
		return false;
	if (!paliasdb->ReadAlias(CNameTXIDTuple(theOffer.aliasTuple.first, txid), latestAlias))
		return false;
	if(theOffer.aliasTuple.first != alias.vchAlias)
		return false;
	CTransaction linkTx;
	COffer linkOffer;
	CAliasIndex linkAlias;
	if( !theOffer.linkOfferTuple.first.empty())
	{
		if(!GetOffer( theOffer.linkOfferTuple.first, linkOffer))
			return false;
		if(!GetAlias(linkOffer.aliasTuple.first, linkAlias))
			return false;
	}

	bool expired = false;

	int64_t expired_time;
	expired = 0;

	expired_time = 0;
	vector<unsigned char> vchCert;
	if(!theOffer.certTuple.first.empty())
		vchCert = theOffer.certTuple.first;
	oOffer.push_back(Pair("_id", stringFromVch(theOffer.vchOffer)));
	oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
	oOffer.push_back(Pair("txid", theOffer.txHash.GetHex()));
	expired_time =  GetOfferExpiration(theOffer);
    if(expired_time <= chainActive.Tip()->nTime)
	{
		expired = true;
	}

	oOffer.push_back(Pair("expires_on", expired_time));
	oOffer.push_back(Pair("expired", expired));
	oOffer.push_back(Pair("height", (int64_t)theOffer.nHeight));
	oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
	oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	int nQty = theOffer.nQty;
	if(!theOffer.linkOfferTuple.first.empty())
		nQty = linkOffer.nQty;
	if(nQty == -1)
		oOffer.push_back(Pair("quantity", "unlimited"));
	else
		oOffer.push_back(Pair("quantity", nQty));
	oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
	oOffer.push_back(Pair("price", strprintf("%.*f", theOffer.paymentPrecision, theOffer.GetPrice(COfferLinkWhitelistEntry(), true))));
	if(!theOffer.linkOfferTuple.first.empty()) {
		oOffer.push_back(Pair("commission", theOffer.nCommission));
		oOffer.push_back(Pair("offerlink_guid", stringFromVch(theOffer.linkOfferTuple.first)));
		oOffer.push_back(Pair("offerlink_seller", stringFromVch(linkOffer.aliasTuple.first)));

	}
	else
	{
		oOffer.push_back(Pair("commission", 0));
		oOffer.push_back(Pair("offerlink_guid", ""));
		oOffer.push_back(Pair("offerlink_seller", ""));
	}
	oOffer.push_back(Pair("private", theOffer.bPrivate));
	int paymentOptions = theOffer.paymentOptions;
	if(!theOffer.linkOfferTuple.first.empty())
		paymentOptions = linkOffer.paymentOptions;
	oOffer.push_back(Pair("paymentoptions", paymentOptions));
	oOffer.push_back(Pair("paymentoptions_display", GetPaymentOptionsString(paymentOptions)));
	oOffer.push_back(Pair("alias_peg", stringFromVch(latestAlias.aliasPegTuple.first)));
	oOffer.push_back(Pair("description", stringFromVch(theOffer.sDescription)));
	oOffer.push_back(Pair("alias", stringFromVch(theOffer.aliasTuple.first)));
	oOffer.push_back(Pair("address", EncodeBase58(latestAlias.vchAddress)));
	int sold = theOffer.nSold;
 	if(!theOffer.linkOfferTuple.first.empty())
 		sold = linkOffer.nSold;
 	oOffer.push_back(Pair("offers_sold", sold));
	oOffer.push_back(Pair("coinoffer", theOffer.bCoinOffer));
	oOffer.push_back(Pair("offer_units", theOffer.fUnits));
	return true;
}
std::string GetPaymentOptionsString(const uint64_t &paymentOptions)
{
	vector<std::string> currencies;
	if(IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_SYS)) {
		currencies.push_back(std::string("SYS"));
	}
	if(IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_BTC)) {
		currencies.push_back(std::string("BTC"));
	}
	if(IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_ZEC)) {
		currencies.push_back(std::string("ZEC"));
	}
	return boost::algorithm::join(currencies, "+");
}
CChainParams::AddressType PaymentOptionToAddressType(const uint64_t &paymentOption)
{
	CChainParams::AddressType myAddressType = CChainParams::ADDRESS_SYS;
	if(paymentOption == PAYMENTOPTION_ZEC)
		myAddressType = CChainParams::ADDRESS_ZEC;
	return myAddressType;
}

void OfferTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = offerFromOp(op);
	COffer offer;
	if(!offer.UnserializeFromData(vchData, vchHash))
		return;

	COffer dbOffer;
	GetOffer(CNameTXIDTuple(offer.vchOffer, offer.txHash), dbOffer);

	entry.push_back(Pair("_id", stringFromVch(offer.vchOffer)));

	if(!offer.linkWhitelist.IsNull())
	{
		if(offer.linkWhitelist.entries[0].nDiscountPct == 127)
			entry.push_back(Pair("whitelist", _("Whitelist was cleared")));
		else
			entry.push_back(Pair("whitelist", _("Whitelist entries were added or removed")));
	}


	if(!offer.certTuple.first.empty() && offer.certTuple != dbOffer.certTuple)
		entry.push_back(Pair("cert", stringFromVch(offer.certTuple.first)));

	if(!offer.aliasTuple.first.empty() && offer.aliasTuple != dbOffer.aliasTuple)
		entry.push_back(Pair("alias", stringFromVch(offer.aliasTuple.first)));

	if(!offer.linkOfferTuple.first.empty() && offer.linkOfferTuple != dbOffer.linkOfferTuple)
		entry.push_back(Pair("offerlink", stringFromVch(offer.linkOfferTuple.first)));

	if(offer.nCommission  != 0 && offer.nCommission != dbOffer.nCommission)
		entry.push_back(Pair("commission",boost::lexical_cast<string>(offer.nCommission)));

	if(offer.paymentOptions > 0 && offer.paymentOptions != dbOffer.paymentOptions)
		entry.push_back(Pair("paymentoptions",GetPaymentOptionsString( offer.paymentOptions)));

	if(!offer.sDescription.empty() && offer.sDescription != dbOffer.sDescription)
		entry.push_back(Pair("description", stringFromVch(offer.sDescription)));

	if(!offer.sTitle.empty() && offer.sTitle != dbOffer.sTitle)
		entry.push_back(Pair("title", stringFromVch(offer.sTitle)));

	if(!offer.sCategory.empty() && offer.sCategory != dbOffer.sCategory)
		entry.push_back(Pair("category", stringFromVch(offer.sCategory)));

	if(offer.nQty != dbOffer.nQty)
		entry.push_back(Pair("quantity", boost::lexical_cast<string>(offer.nQty)));

	if(!offer.sCurrencyCode.empty()  && offer.sCurrencyCode != dbOffer.sCurrencyCode)
		entry.push_back(Pair("currency", stringFromVch(offer.sCurrencyCode)));

	if(offer.GetPrice() != dbOffer.GetPrice())
		entry.push_back(Pair("price", strprintf("%.*f", offer.paymentPrecision, offer.GetPrice(COfferLinkWhitelistEntry(), true))));

	if(offer.bPrivate != dbOffer.bPrivate)
		entry.push_back(Pair("private", offer.bPrivate));
}
bool COfferLinkWhitelist::GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) const {
	entry.SetNull();
	for (unsigned int i = 0; i<entries.size(); i++) {
		if (entries[i].aliasLinkVchRand == ahash) {
			entry = entries[i];
			return true;
		}
	}
	return false;
}
double COffer::GetPrice(const COfferLinkWhitelistEntry& entry, bool display) const {
	double price = boost::lexical_cast<double>(sPrice);
	if (!display)
	{
		if (bCoinOffer)
			price = fUnits;
		else if (fUnits != 1)
			price *= fUnits;
	}

	char nDiscount = entry.nDiscountPct;
	if (entry.nDiscountPct > 99)
		nDiscount = 0;
	// nMarkup is a percentage, commission minus discount
	char nMarkup = nCommission - nDiscount;
	if (nMarkup != 0)
	{
		double lMarkup = 1 / (nMarkup / 100.0);
		lMarkup = floorf(lMarkup * 100) / 100;
		double priceMarkup = price / lMarkup;
		price += priceMarkup;
	}
	return price;
}
