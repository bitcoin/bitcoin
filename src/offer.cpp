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
extern mongoc_collection_t *offeraccept_collection;
extern mongoc_collection_t *escrow_collection;
extern mongoc_collection_t *cert_collection;
extern mongoc_collection_t *feedback_collection;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
bool IsOfferOp(int op) {
	return op == OP_OFFER_ACTIVATE
        || op == OP_OFFER_UPDATE
        || op == OP_OFFER_ACCEPT;
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
	case OP_OFFER_ACCEPT:
		return "offeraccept";
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
	COfferAccept offerAccept;
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
			else if (pcursor->GetKey(keyVch) && keyVch.first == "offera") {
				COffer offer;
				pcursor->GetValue(offerAccept);
				if (!GetOffer(offerAccept.offerTuple.first, offer) || chainActive.Tip()->nTime >= GetOfferExpiration(offer))
				{
					servicesCleaned++;
					EraseOfferAccept(keyVch.second);
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
bool GetOfferAccept(const vector<unsigned char> &vchOfferAccept, COfferAccept &theOfferAccept) {
	if (!pofferdb || !pofferdb->ReadOfferAccept(vchOfferAccept, theOfferAccept))
		return false;
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
int FindOfferAcceptPayment(const CTransaction& tx, const CAmount &nPrice) {
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		if(tx.vout[i].nValue == nPrice)
			return i;
	}
	return -1;
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
		LogPrintf("*** OFFER %d %d %s %s %s %s %s %d\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			op==OP_OFFER_ACCEPT ? "OFFERACCEPT: ": "",
			op==OP_OFFER_ACCEPT && vvchArgs.size() > 1? stringFromVch(vvchArgs[0]).c_str(): "",
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
		if(op != OP_OFFER_ACCEPT && vvchArgs.size() != 2)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1002 - " + _("Offer arguments incorrect size");
			return error(errorMessage.c_str());
		}
		else if(op == OP_OFFER_ACCEPT && vvchArgs.size() != 3)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1003 - " + _("OfferAccept arguments incorrect size");
			return error(errorMessage.c_str());
		}
		
		if(op == OP_OFFER_ACCEPT)
		{
			if(vvchArgs.size() <= 2 || vchHash != vvchArgs[2])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1004 - " + _("Hash provided doesn't match the calculated hash of the data");
				return true;
			}
		}
		else
		{
			if(vvchArgs.size() <= 1 || vchHash != vvchArgs[1])
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1005 - " + _("Hash provided doesn't match the calculated hash of the data");
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
			if (!foundAlias && IsAliasOp(pop, true) && vvch.size() >= 4 && vvch[3].empty() && ((theOffer.accept.IsNull() && theOffer.aliasTuple.first == vvch[0] && theOffer.aliasTuple.third == vvch[1]) || (!theOffer.accept.IsNull() && theOffer.accept.buyerAliasTuple.first == vvch[0] && theOffer.accept.buyerAliasTuple.third == vvch[1])))
			{
				foundAlias = true;
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}

	}


	// unserialize offer from txn, check for valid
	COfferAccept theOfferAccept;
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
			if(!theOffer.accept.IsNull())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1017 - " + _("Cannot have accept information on offer activation");
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
			if(theOffer.nPrice <= 0)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1025 - " + _("Offer price must be greater than 0");
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
			if(!theOffer.accept.IsNull())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1028 - " + _("Cannot have accept information on offer update");
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
			if(theOffer.nPrice <= 0)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1033 - " + _("Offer price must be greater than 0");
				return error(errorMessage.c_str());
			}
			if(theOffer.nCommission > 100 || theOffer.nCommission < -90)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1034 - " + _("Commission percentage must be between -90 and 100");
				return error(errorMessage.c_str());
			}
			break;
		case OP_OFFER_ACCEPT:
			theOfferAccept = theOffer.accept;
			if(!IsAliasOp(prevAliasOp, true) || vvchPrevAliasArgs.empty())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1035 - " + _("Alias input mismatch");
				return error(errorMessage.c_str());
			}
			if(!theOfferAccept.feedback.IsNull())
			{
				if(vvchArgs.size() <= 2 || vvchArgs[2] != vchFromString("1"))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1036 - " + _("Invalid feedback transaction");
					return error(errorMessage.c_str());
				}
				if(theOfferAccept.feedback.vchFeedback.empty())
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1037 - " + _("Cannot leave empty feedback");
					return error(errorMessage.c_str());
				}
				if(theOfferAccept.feedback.vchFeedback.size() > MAX_VALUE_LENGTH)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1038 - " + _("Feedback too long");
					return error(errorMessage.c_str());
				}
				break;
			}
			else
			{
                if(!IsValidPaymentOption(theOfferAccept.nPaymentOption))
                {
                    errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1040 - " + _("Invalid payment option");
                    return error(errorMessage.c_str());
                }
			}

			if (theOfferAccept.IsNull())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1041 - " + _("Offeraccept object cannot be empty");
				return error(errorMessage.c_str());
			}
			if (!IsValidAliasName(theOfferAccept.buyerAliasTuple.first))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1042 - " + _("Invalid offer buyer alias");
				return error(errorMessage.c_str());
			}
			if (vvchArgs.size() <= 1 || vvchArgs[0].size() > MAX_GUID_LENGTH)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1043 - " + _("Offeraccept transaction with guid too big");
				return error(errorMessage.c_str());
			}
			if (theOfferAccept.vchAcceptRand.size() > MAX_GUID_LENGTH)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1044 - " + _("Offer accept hex guid too long");
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
			serializedOffer.accept.SetNull();
			theOffer = serializedOffer;
			theOffer.accept.SetNull();
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
				}
				else
				{
					// if creating a linked offer we set some mandatory fields from the parent
					theOffer.nQty = linkOffer.nQty;
					theOffer.sCurrencyCode = linkOffer.sCurrencyCode;
					theOffer.certTuple = linkOffer.certTuple;
					theOffer.SetPrice(linkOffer.nPrice);
					theOffer.sPrice = linkOffer.sPrice;
					theOffer.sCategory = linkOffer.sCategory;
					theOffer.sTitle = linkOffer.sTitle;
					theOffer.paymentOptions = linkOffer.paymentOptions;
					theOffer.fUnits = linkOffer.fUnits;
					theOffer.bCoinOffer = linkOffer.bCoinOffer;
				}
			}
			// init sold to 0
 			theOffer.nSold = 0;
		}
		else if (op == OP_OFFER_ACCEPT) {
			theOfferAccept = theOffer.accept;
			theOfferAccept.txHash = tx.GetHash();
			theOfferAccept.nHeight = nHeight;
			theOfferAccept.vchAcceptRand = vvchArgs[0];
			COfferAccept offerAccept;
			COffer acceptOffer;

			if(theOfferAccept.bPaymentAck)
			{
				if (!GetOfferAccept(vvchArgs[0], offerAccept))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1053 - " + _("Could not find offer accept from mempool or disk");
					return true;
				}
				if (!GetOffer(offerAccept.offerTuple, acceptOffer))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1053 - " + _("Could not find offer related to offer accept");
					return true;
				}
				if(!acceptOffer.linkOfferTuple.first.empty())
				{
					if(!GetOffer( acceptOffer.linkOfferTuple.first, linkOffer))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1054 - " + _("Could not get linked offer");
						return true;
					}
					if(theOfferAccept.buyerAliasTuple.first != linkOffer.aliasTuple.first)
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1055 - " + _("Only root merchant can acknowledge offer payment");
						return true;
					}
				}	
				else
				{
					if(theOfferAccept.buyerAliasTuple.first != theOffer.aliasTuple.first)
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1056 - " + _("Only merchant can acknowledge offer payment");
						return true;
					}
				}
				if(offerAccept.bPaymentAck)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1057 - " + _("Offer payment already acknowledged");
				}
				theOfferAccept = offerAccept;
				theOfferAccept.bPaymentAck = true;

				// write offer
				if (!dontaddtodb && !pofferdb->WriteOfferAccept(theOfferAccept))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1058 - " + _("Failed to write to offer accept DB");
					return error(errorMessage.c_str());
				}
				if (fDebug)
					LogPrintf( "CONNECTED OFFER ACK: op=%s offer=%s qty=%u hash=%s height=%d\n",
						offerFromOp(op).c_str(),
						stringFromVch(vvchArgs[0]).c_str(),
						theOffer.nQty,
						tx.GetHash().ToString().c_str(),
						nHeight);
				return true;
			}


			
			else if(!theOfferAccept.feedback.IsNull())
			{
				if (!GetOfferAccept(vvchArgs[0], offerAccept))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1059 - " + _("Could not find offer accept from mempool or disk");
					return true;
				}
				if (!GetOffer(offerAccept.offerTuple, acceptOffer))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1053 - " + _("Could not find offer related to offer accept");
					return true;
				}
				if (theOfferAccept.vchAcceptRand != offerAccept.vchAcceptRand) {
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Accept GUID incorrect in feedback payload");
					return true;
				}
				else if (theOfferAccept.feedback.nFeedbackUserFrom == theOfferAccept.feedback.nFeedbackUserTo)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4061 - " + _("Cannot send yourself feedback");
					return true;
				}
				else if (theOfferAccept.feedback.nRating > 5)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4063 - " + _("Invalid rating, must be less than or equal to 5 and greater than or equal to 0");
					return true;
				}
				else if (theOfferAccept.feedback.nFeedbackUserFrom == FEEDBACKBUYER && theOfferAccept.buyerAliasTuple.first != offerAccept.buyerAliasTuple.first)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only buyer can leave this feedback");
					return true;
				}
				else if (theOfferAccept.feedback.nFeedbackUserFrom == FEEDBACKSELLER && theOfferAccept.buyerAliasTuple.first != acceptOffer.aliasTuple.first)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4065 - " + _("Only seller can leave this feedback");
					return true;
				}
				else if(theOfferAccept.feedback.nFeedbackUserFrom != FEEDBACKBUYER && theOfferAccept.feedback.nFeedbackUserFrom != FEEDBACKSELLER)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1062 - " + _("Unknown feedback user type");
					return true;
				}
			
				if(!dontaddtodb)
					pofferdb->WriteOfferAcceptFeedbackIndex(theOfferAccept);
				if (fDebug)
					LogPrintf( "CONNECTED OFFER FEEDBACK: op=%s offer=%s qty=%u hash=%s height=%d\n",
						offerFromOp(op).c_str(),
						stringFromVch(vvchArgs[0]).c_str(),
						theOffer.nQty,
						tx.GetHash().ToString().c_str(),
						nHeight);
				return true;

			}
			if (GetOfferAccept(vvchArgs[0], offerAccept))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1065 - " + _("Offer payment already exists");
				return true;
			}
			if (!GetOffer(theOfferAccept.offerTuple, theOffer))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1065 - " + _("Could not get offer at the time of accept");
				return true;
			}

			if(theOffer.bPrivate && !theOffer.linkWhitelist.IsNull())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1066 - " + _("Cannot purchase this private offer, must purchase through an affiliate");
				return true;
			}
			if (offerAccept.linkOfferTuple.first != theOffer.linkOfferTuple.first)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1066 - " + _("Linked offer does not match offer being purchased");
				return true;
			}
			if(!theOffer.linkOfferTuple.first.empty())
			{
				if(!GetOffer(offerAccept.linkOfferTuple, linkOffer))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1067 - " + _("Could not get linked offer");
					return true;
				}
				if(!GetAlias(CNameTXIDTuple(linkOffer.aliasTuple.first, linkOffer.aliasTuple.second), linkAlias))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1068 - " + _("Cannot find alias for this linked offer. It may be expired");
					return true;
				}
				if(!IsPaymentOptionInMask(linkOffer.paymentOptions, theOfferAccept.nPaymentOption))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1069 - " + _("User selected payment option not found in list of accepted offer payment options");
					return true;
				}
				else if(!theOfferAccept.txExtId.IsNull() && (linkOffer.paymentOptions == PAYMENTOPTION_SYS || theOfferAccept.nPaymentOption == PAYMENTOPTION_SYS))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1070 - " + _("External chain payment cannot be made with this offer");
					return true;
				}
				linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.aliasTuple.first, entry);
				if(entry.IsNull())
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1071 - " + _("Linked offer alias does not exist on the root offer affiliate list");
					return true;
				}
				if(theOffer.nCommission <= -entry.nDiscountPct)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1072 - " + _("This resold offer must be of higher price than the original offer including any discount");
					return true;
				}
			}
			if(!IsPaymentOptionInMask(theOffer.paymentOptions, theOfferAccept.nPaymentOption))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1073 - " + _("User selected payment option not found in list of accepted offer payment options");
				return true;
			}
			else if(!theOfferAccept.txExtId.IsNull() && (theOffer.paymentOptions == PAYMENTOPTION_SYS || theOfferAccept.nPaymentOption == PAYMENTOPTION_SYS))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1074 - " + _("External chain payment cannot be made with this offer");
				return true;
			}		
			if(theOfferAccept.txExtId.IsNull() && theOfferAccept.nPaymentOption != PAYMENTOPTION_SYS)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1075 - " + _("External chain payment txid missing");
				return true;
			}
			if(theOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(theOffer.sCategory), "wanted"))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1076 - " + _("Cannot purchase a wanted offer");
				return true;
			}
			if(!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), alias))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1077 - " + _("Cannot find alias for this offer. It may be expired");
				return true;
			}
			const string &strAliasAddress = EncodeBase58(alias.vchAddress);
			// check that user pays enough in syscoin if the currency of the offer is not external purchase
			if(theOfferAccept.txExtId.IsNull())
			{
				CAmount nPrice;
				CAmount nCommission;
				// try to get the whitelist entry here from the sellers whitelist, apply the discount with GetPrice()
				if(theOffer.linkOfferTuple.first.empty())
				{
					theOffer.linkWhitelist.GetLinkEntryByHash(theOfferAccept.buyerAliasTuple.first, entry);
					nPrice = theOffer.GetPrice(entry);
					nCommission = 0;
				}
				else
				{
					linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.aliasTuple.first, entry);
					nPrice = linkOffer.GetPrice(entry);
					nCommission = theOffer.GetPrice() - nPrice;
				}
				if(nPrice != theOfferAccept.nPrice)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1078 - " + _("Offer payment does not specify the correct payment amount");
					return true;
				}
				CAmount nTotalValue = ( nPrice * theOfferAccept.nQty );
				CAmount nTotalCommission = ( nCommission * theOfferAccept.nQty );
				int nOutPayment, nOutCommission;
				nOutPayment = FindOfferAcceptPayment(tx, nTotalValue);
				if(nOutPayment < 0)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1079 - " + _("Offer payment does not pay enough according to the offer price");
					return true;
				}
				if(!theOffer.linkOfferTuple.first.empty())
				{
					nOutCommission = FindOfferAcceptPayment(tx, nTotalCommission);
					if(nOutCommission < 0)
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1080 - " + _("Offer payment does not include enough commission to affiliate");
						return true;
					}
					CSyscoinAddress destaddy;
					if (!ExtractDestination(tx.vout[nOutPayment].scriptPubKey, payDest))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1081 - " + _("Could not extract payment destination from scriptPubKey");
						return true;
					}
					destaddy = CSyscoinAddress(payDest);
					CSyscoinAddress commissionaddy;
					if (!ExtractDestination(tx.vout[nOutCommission].scriptPubKey, commissionDest))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1082 - " + _("Could not extract commission destination from scriptPubKey");
						return true;
					}
					commissionaddy = CSyscoinAddress(commissionDest);
					if(EncodeBase58(linkAlias.vchAddress) != destaddy.ToString())
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1083 - " + _("Payment destination does not match merchant address");
						return true;
					}
					if(strAliasAddress != commissionaddy.ToString())
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1084 - " + _("Commission destination does not match affiliate address");
						return true;
					}
				}
				else
				{
					
					if (!ExtractDestination(tx.vout[nOutPayment].scriptPubKey, payDest))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1085 - " + _("Could not extract payment destination from scriptPubKey");
						return true;
					}
					CSyscoinAddress destaddy(payDest);
					if(strAliasAddress != destaddy.ToString())
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1086 - " + _("Payment destination does not match merchant address");
						return true;
					}
				}
				// autonomously transfer certificate to new owner (buyer) if paid in full amount with SYS
				if (!theOffer.certTuple.first.empty())
				{
					CCert cert;
					if (GetCert(theOffer.certTuple.first, cert))
					{
						// ensure cert owner is merchant of offer before transferring
						if (cert.aliasTuple.first == theOffer.aliasTuple.first) {
							cert.aliasTuple = theOfferAccept.buyerAliasTuple;
							cert.nHeight = nHeight;
							cert.txHash = tx.GetHash();
							if (!dontaddtodb && !pcertdb->WriteCert(cert))
							{
								errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1090 - " + _("Failed to write to certificate to DB");
								return error(errorMessage.c_str());
							}
						}
					}
					else {
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1088 - " + _("Cannot read certificate from database");
						return true;
					}
			
				}
			}
			// check that the script for the offer update is sent to the correct destination
			if (!ExtractDestination(tx.vout[nOut].scriptPubKey, dest))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1087 - " + _("Cannot extract destination from output script");
				return true;
			}
			CSyscoinAddress destaddy(dest);
			if(strAliasAddress != destaddy.ToString())
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1088 - " + _("Payment address does not match merchant address");
				return true;
			}

			theOfferAccept.vchAcceptRand = vvchArgs[0];
			theOfferAccept.nHeight = nHeight;
			// decrease qty + increase # sold
			if(theOfferAccept.nQty <= 0)
 				theOfferAccept.nQty = 1;
			int nQty = theOffer.nQty;
			// if this is a linked offer we must update the linked offer qty
			if (!linkOffer.IsNull())
			{
				nQty = linkOffer.nQty;
			}
			if(nQty != -1)
			{
				if(theOfferAccept.nQty > nQty)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1089 - " + _("Not enough quantity left in this offer for this purchase");
					return true;
				}
				if(theOfferAccept.txExtId.IsNull())
					nQty -= theOfferAccept.nQty;
			}
			theOffer.nSold++;
			theOffer.nQty = nQty;
			if (!linkOffer.IsNull())
 			{
 				linkOffer.nHeight = nHeight;
 				linkOffer.nQty = nQty;
 				linkOffer.nSold++;
 				linkOffer.txHash = tx.GetHash();
				if (!dontaddtodb || !pofferdb->WriteOffer(linkOffer))
 				{
 					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1090 - " + _("Failed to write to offer link to DB");
 					return error(errorMessage.c_str());
 				}
 			}
			if(!theOfferAccept.txExtId.IsNull())
			{
				if(pofferdb->ExistsExtTXID(theOfferAccept.txExtId))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1091 - " + _("BTC Transaction ID specified was already used to pay for an offer");
					return true;
				}
				if(!dontaddtodb && !pofferdb->WriteExtTXID(theOfferAccept.txExtId))
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1092 - " + _("Failed to BTC Transaction ID to DB");
					return error(errorMessage.c_str());
				}
			}
			if (!dontaddtodb && !pofferdb->WriteOfferAccept(theOfferAccept))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1090 - " + _("Failed to write to offer accept to DB");
				return error(errorMessage.c_str());
			}
		}


		if(op == OP_OFFER_UPDATE)
		{
			// ensure the accept is null as this should just have the offer information and no accept information
			theOffer.accept.SetNull();
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
				theOffer.SetPrice(linkOffer.nPrice);
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
	newOffer.SetPrice(nPricePerUnit);
	if (!vchCert.empty())
		newOffer.certTuple = CNameTXIDTuple(theCert.vchCert, theCert.txHash);
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = bPrivate;
	newOffer.paymentOptions = paymentOptionsMask;
	newOffer.fUnits = fUnits;
	newOffer.bCoinOffer = bCoinOffer;
	CAmount currencyPrice = convertSyscoinToCurrencyCode(latestAliasPegTuple, newOffer.sCurrencyCode, newOffer.GetDisplayPrice(), precision);
	newOffer.sPrice = strprintf("%.*f", precision, ValueFromAmount(currencyPrice).get_real());

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
	newOffer.SetPrice(linkOffer.GetPrice());
	newOffer.paymentOptions = linkOffer.paymentOptions;
	newOffer.nCommission = commissionInteger;
	newOffer.linkOfferTuple = CNameTXIDTuple(linkOffer.vchOffer, linkOffer.txHash);
	newOffer.nHeight = chainActive.Tip()->nHeight;
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

	CAmount nPricePerUnit = offerCopy.GetPrice();
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
		nPricePerUnit = convertCurrencyCodeToSyscoin(latestAliasPegTuple, strCurrency.empty()? offerCopy.sCurrencyCode: vchFromString(strCurrency), fPrice, precision);
		if(nPricePerUnit == 0)
		{
			string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1549 - " + _("Could not find currency in the peg alias");
			throw runtime_error(err.c_str());
		}
	
		theOffer.SetPrice(nPricePerUnit);
		theOffer.sPrice = strprintf("%.*f", precision, fPrice);
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
UniValue offeraccept(const UniValue& params, bool fHelp) {
	if (fHelp || 2 > params.size() || params.size() > 6)
		throw runtime_error("offeraccept <alias> <guid> [quantity] [Ext TxId] [payment option] [witness]\n"
				"Accept&Pay for a confirmed offer.\n"
				"<alias> An alias of the buyer.\n"
				"<guid> guidkey from offer.\n"
				"<quantity> quantity to buy. Defaults to 1.\n"
				"<Ext TxId> If paid in another coin, enter the Transaction ID here. Default is empty.\n"
				"<paymentOption> If Ext TxId is defined, specify a valid payment option used to make payment. Default is SYS.\n"
				"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
				+ HelpRequiringPassphrase());
	CSyscoinAddress refundAddr;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	vector<unsigned char> vchExtTxId;

	int64_t nHeight = chainActive.Tip()->nHeight;
	unsigned int nQty = 1;
	if(CheckParam(params, 2))
	{
		try {
			nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
		} catch (std::exception &e) {
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1550 - " + _("Quantity must be less than 4294967296"));
		}
	}

	if(CheckParam(params, 3))
		vchExtTxId = vchFromValue(params[3]);
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOptions = "SYS";
	if(CheckParam(params, 4))
		paymentOptions = params[4].get_str();
	boost::algorithm::to_upper(paymentOptions);
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOptions))
	{
		string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1551 - " + _("Could not validate the payment options value");
		throw runtime_error(err.c_str());
	}
	// payment options - and convert payment options string to a bitmask for the txn
	uint64_t paymentOptionsMask = GetPaymentOptionsMaskFromString(paymentOptions);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 5))
		vchWitness = vchFromValue(params[5]);	
	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyAliasOrig;
	vector<unsigned char> vchAccept = vchFromString(GenerateSyscoinGuid());

	// create OFFERACCEPT txn keys
	CScript scriptPubKeyAccept, scriptPubKeyAcceptLinked, scriptPubKeyPayment;
	CScript scriptPubKeyAlias;
	COffer theOffer;
	if (!GetOffer( vchOffer, theOffer))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1552 - " + _("Could not find an offer with this identifier"));
	}

	COffer linkOffer;
	if(!theOffer.linkOfferTuple.first.empty() && !GetOffer( theOffer.linkOfferTuple.first, linkOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1553 - " + _("Could not get linked offer"));

	CAliasIndex theAlias;
	if(!GetAlias(theOffer.aliasTuple.first, theAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1554 - " + _("Could not find the alias associated with this offer"));

	CAliasIndex buyerAlias;
	if (!GetAlias(vchAlias, buyerAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1555 - " + _("Could not find buyer alias with this name"));
	CAliasIndex pegAlias;
	if (!GetAlias(theAlias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1500 - " + _("Could not find peg alias with this name"));

	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);

	vector<string> rateList;
	int precision = 2;
	int nFeePerByte;
	double nRate;
	float fEscrowFee;
	if(getCurrencyToSYSFromAlias(latestAliasPegTuple, theOffer.sCurrencyCode, nRate, rateList,precision, nFeePerByte, fEscrowFee) != "")
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1556 - " + _("Could not find currency in the peg alias: ") + " Currency: " + stringFromVch(theOffer.sCurrencyCode).c_str() + " Peg Alias: " + stringFromVch(latestAliasPegTuple.first).c_str());
	}
	CCert theCert;
	// trying to purchase a cert
	if(!theOffer.certTuple.first.empty())
	{
		if (!GetCert( theOffer.certTuple.first, theCert))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1557 - " + _("Cannot purchase an expired certificate"));
		}
		else if(theOffer.linkOfferTuple.first.empty())
		{
			if(theCert.aliasTuple.first != theOffer.aliasTuple.first)
			{
				throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1558 - " + _("Cannot purchase this offer because the certificate has been transferred or it is linked to another offer"));
			}
		}
	}
	if(!theOffer.linkOfferTuple.first.empty())
	{
		if (!linkOffer.linkOfferTuple.first.empty())
		{
			 throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1560 - " + _("Cannot purchase offers that are linked more than once"));
		}
		CAliasIndex linkAlias;
		if(!GetAlias(linkOffer.aliasTuple.first, linkAlias))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1561 - " + _("Cannot find alias for this linked offer"));
		}
		else if(!theOffer.certTuple.first.empty() && theCert.aliasTuple.first != linkOffer.aliasTuple.first)
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1562 - " + _("Cannot purchase this linked offer because the certificate has been transferred or it is linked to another offer"));
		}
		else if(linkOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkOffer.sCategory), "wanted"))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1563 - " + _("Cannot purchase a wanted offer"));
		}
	}
	COfferLinkWhitelistEntry foundEntry;
	CAmount nPrice;
	CAmount nCommission;
	if(theOffer.linkOfferTuple.first.empty())
	{
		theOffer.linkWhitelist.GetLinkEntryByHash(buyerAlias.vchAlias, foundEntry);
		nPrice = theOffer.GetPrice(foundEntry);
		nCommission = 0;
	}
	else
	{
		linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.aliasTuple.first, foundEntry);
		nPrice = linkOffer.GetPrice(foundEntry);
		nCommission = theOffer.GetPrice() - nPrice;
		if(nCommission < 0)
			nCommission = 0;
	}
	CSyscoinAddress buyerAddress;
	GetAddress(buyerAlias, &buyerAddress, scriptPubKeyAliasOrig);
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAlias.vchAlias  << buyerAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;

	string strCipherText = "";

	CAliasIndex theLinkedAlias;
	if(!theOffer.linkOfferTuple.first.empty())
	{
		if (!GetAlias(linkOffer.aliasTuple.first, theLinkedAlias))
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1564 - " + _("Could not find an alias with this name"));
		// tiny alias payment so that linked alias can get a notification when calling offerlist (with accepts) that the linked alias made a sale
		scriptPubKeyAcceptLinked << CScript::EncodeOP_N(OP_ALIAS_PAYMENT) << linkOffer.aliasTuple.first << vchFromString("0") <<  theLinkedAlias.aliasPegTuple.first << linkOffer.sCurrencyCode << OP_DROP << OP_2DROP << OP_2DROP;
	}

	
	COfferAccept txAccept;
	txAccept.nQty = nQty;
	txAccept.nPrice = nPrice;
	txAccept.offerTuple = CNameTXIDTuple(theOffer.vchOffer, theOffer.txHash);
	if (!theOffer.linkOfferTuple.first.empty())
		txAccept.linkOfferTuple = CNameTXIDTuple(linkOffer.vchOffer, linkOffer.txHash);
	txAccept.buyerAliasTuple = CNameTXIDTuple(buyerAlias.vchAlias, buyerAlias.txHash, buyerAlias.vchGUID);
	txAccept.nPaymentOption = paymentOptionsMask;
    CAmount nTotalValue = ( nPrice * nQty );
	CAmount nTotalCommission = ( nCommission * nQty );
	CAmount currencyPrice = convertSyscoinToCurrencyCode(latestAliasPegTuple, theOffer.sCurrencyCode, nTotalValue + nTotalCommission, precision);
	txAccept.sTotal = strprintf("%.*f", precision, ValueFromAmount(currencyPrice).get_real());
	if(!vchExtTxId.empty())
	{
		uint256 txExtId(uint256S(stringFromVch(vchExtTxId)));
		txAccept.txExtId = txExtId;
	}
	COffer copyOffer = theOffer;
	theOffer.ClearOffer();
	theOffer.accept = txAccept;
	theOffer.nHeight = chainActive.Tip()->nHeight;
	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());

    CScript scriptPayment, scriptPubKeyCommission, scriptPubKeyOrig, scriptPubLinkKeyOrig, scriptPaymentCommission;
	CSyscoinAddress currentAddress;
	GetAddress(theAlias, &currentAddress, scriptPubKeyOrig);

	CSyscoinAddress linkAddress;
	GetAddress(theLinkedAlias, &linkAddress, scriptPubLinkKeyOrig);
	scriptPubKeyAcceptLinked += scriptPubKeyOrig;
	scriptPubKeyAccept << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << vchAccept << vchFromString("0") << vchHashOffer << OP_2DROP << OP_2DROP << OP_DROP;
	if(!copyOffer.linkOfferTuple.first.empty())
	{
		scriptPayment = scriptPubLinkKeyOrig;
		scriptPaymentCommission = scriptPubKeyOrig;
	}
	else
	{
		scriptPayment = scriptPubKeyOrig;
	}
	scriptPubKeyAccept += scriptPubKeyOrig;
	scriptPubKeyPayment += scriptPayment;
	scriptPubKeyCommission += scriptPaymentCommission;

	vector<CRecipient> vecSend;
	CRecipient acceptRecipient;
	CreateRecipient(scriptPubKeyAccept, acceptRecipient);
	CRecipient paymentRecipient = {scriptPubKeyPayment, nTotalValue, false};
	CRecipient paymentCommissionRecipient = {scriptPubKeyCommission, nTotalCommission, false};
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasLinkedRecipient;
	CreateRecipient(scriptPubKeyAcceptLinked, aliasLinkedRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyAliasOrig, buyerAlias.vchAlias, buyerAlias.aliasPegTuple, aliasPaymentRecipient);

	if(vchExtTxId.empty())
	{
		vecSend.push_back(acceptRecipient);
		vecSend.push_back(paymentRecipient);
		if(!copyOffer.linkOfferTuple.first.empty() && nTotalCommission > 0)
			vecSend.push_back(paymentCommissionRecipient);
	}
	else
	{
		vecSend.push_back(acceptRecipient);
	}
	if(!copyOffer.linkOfferTuple.first.empty())
		vecSend.push_back(aliasLinkedRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, buyerAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);

	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(buyerAlias.vchAlias, vchWitness, theAlias.aliasPegTuple.first, stringFromVch(copyOffer.sCurrencyCode), aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);

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
		res.push_back(stringFromVch(vchAccept));
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(stringFromVch(vchAccept));
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
void COfferDB::WriteOfferAcceptIndex(const COfferAccept& offerAccept) {
	if (!offeraccept_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	string id;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	if (BuildOfferAcceptJson(offerAccept, oName)) {
		selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(offerAccept.vchAcceptRand).c_str()));
		write_concern = mongoc_write_concern_new();
		mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(offeraccept_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB OFFER ACCEPT ERROR: %s\n", error.message);
		}
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferAcceptIndex(const std::vector<unsigned char>& vchOffer) {
	if (!offeraccept_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	string id;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	id = stringFromVch(vchOffer);
	selector = BCON_NEW("offer", BCON_UTF8(id.c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(offeraccept_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB OFFER ACCEPT REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::WriteOfferAcceptFeedbackIndex(const COfferAccept& offerAccept) {
	if (!feedback_collection)
		return;
	bson_error_t error;
	bson_t *insert = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	BuildFeedbackJson(offerAccept, oName);
	insert = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!insert || !mongoc_collection_insert(feedback_collection, (mongoc_insert_flags_t)MONGOC_INSERT_NO_VALIDATE, insert, write_concern, &error)) {
		LogPrintf("MONGODB OFFER ACCEPT FEEDBACK ERROR: %s\n", error.message);
	}
	
	if (insert)
		bson_destroy(insert);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferAcceptFeedbackIndex(const std::vector<unsigned char>& vchOffer) {
	if (!feedback_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	string id;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	id = stringFromVch(vchOffer);
	selector = BCON_NEW("offer", BCON_UTF8(id.c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(feedback_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB OFFER ACCEPT FEEDBACK REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
UniValue offeracceptfeedback(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
			"offeracceptfeedback <offeraccept guid> <user role> <feedback> <rating> [witness]\n"
                        "Send feedback and rating for offer accept specified. Ratings are numbers from 1 to 5. User Role is either 'buyer', 'seller'. If you are the 'buyer' for example, you are leaving rating/feedback for the 'seller'.\n"
                        + HelpRequiringPassphrase());
   // gather & validate inputs
	int nRating = 0;
	vector<unsigned char> vchAcceptRand = vchFromValue(params[0]);
	string role = params[1].get_str();
	vector<unsigned char> vchFeedback = vchFromValue(params[2]);
	try {
		nRating = boost::lexical_cast<int>(params[3].get_str());
		if(nRating < 0 || nRating > 5)
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1567 - " + _("Invalid rating value, must be less than or equal to 5 and greater than or equal to 0"));

	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1568 - " + _("Invalid rating value"));
	}

	vector<unsigned char> vchWitness;
	if(CheckParam(params, 4))
		vchWitness = vchFromValue(params[4]);
    // this is a syscoin transaction
    CWalletTx wtx;

	COffer theOffer;
	COfferAccept theOfferAccept;
	
	if (!GetOfferAccept(vchAcceptRand, theOfferAccept))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1570 - " + _("Could not find this offer purchase"));

	if (!GetOffer(theOfferAccept.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1570 - " + _("Could not find this offer purchase"));

	CAliasIndex buyerAlias, sellerAlias;

	if(!GetAlias(theOfferAccept.buyerAliasTuple.first, buyerAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1573 - " + _("Could not find buyer alias"));
	if(!GetAlias(theOffer.aliasTuple.first, sellerAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1574 - " + _("Could not find merchant alias"));
	CSyscoinAddress buyerAddress;
	CScript buyerScript, sellerScript;
	GetAddress(buyerAlias, &buyerAddress, buyerScript);

	CSyscoinAddress sellerAddress;
	GetAddress(sellerAlias, &sellerAddress, sellerScript);

	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	CScript scriptPubKey;
	vector<unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	if (role == "buyer")
	{
		vchLinkAlias = buyerAlias.vchAlias;
		theAlias = buyerAlias;
	}
	else if (role == "seller")
	{
		vchLinkAlias = sellerAlias.vchAlias;
		theAlias = sellerAlias;
	}


	theOffer.ClearOffer();
	theOffer.accept = theOfferAccept;
	theOffer.accept.vchAcceptRand.clear();
	theOffer.accept.buyerAliasTuple = CNameTXIDTuple(theAlias.vchAlias, theAlias.txHash, theAlias.vchGUID);
	theOffer.accept.bPaymentAck = false;
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// buyer
	if (role == "buyer")
	{
		CFeedback sellerFeedback(FEEDBACKBUYER, FEEDBACKSELLER);
		sellerFeedback.vchFeedback = vchFeedback;
		sellerFeedback.nRating = nRating;
		theOffer.accept.feedback = sellerFeedback;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAlias.vchAlias << buyerAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += buyerScript;
		scriptPubKeyAliasOrig = buyerScript;
	}
	// seller
	else if (role == "seller")
	{
		CFeedback buyerFeedback(FEEDBACKSELLER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedback;
		buyerFeedback.nRating = nRating;
		theOffer.accept.feedback = buyerFeedback;
		scriptPubKeyAlias = CScript() <<  CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAlias.vchAlias << sellerAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
		scriptPubKeyAlias += sellerScript;
		scriptPubKeyAliasOrig = sellerScript;

	}
	else
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1581 - " + _("You must be either the buyer or seller to leave feedback on this offer purchase"));
	}

	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());

	vector<CRecipient> vecSend;
	CRecipient recipientAlias, recipientPaymentAlias, recipient;


	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << theOfferAccept.vchAcceptRand << vchFromString("1") << vchHashOffer << OP_2DROP <<  OP_2DROP << OP_DROP;
	scriptPubKey += sellerScript;
	CreateRecipient(scriptPubKey, recipient);
	CreateRecipient(scriptPubKeyAlias, recipientAlias);
	CreateAliasRecipient(scriptPubKeyAliasOrig, theAlias.vchAlias, theAlias.aliasPegTuple, recipientPaymentAlias);

	vecSend.push_back(recipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(vchLinkAlias, vchWitness, theAlias.aliasPegTuple.first, "", recipientAlias, recipientPaymentAlias, vecSend, wtx, &coinControl);
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
UniValue offeracceptacknowledge(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
		"offeracceptacknowledge <offeraccept guid> [witness] \n"
                        "Acknowledge offer payment as seller of offer.\n"
                        + HelpRequiringPassphrase());
   // gather & validate inputs
	vector<unsigned char> vchAcceptRand = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 1))
		vchWitness = vchFromValue(params[1]);
    // this is a syscoin transaction
    CWalletTx wtx;

	

    // look for a transaction with this key
	COffer theOffer, linkOffer;
	COfferAccept theOfferAccept;


	CScript buyerScript, sellerScript;
	CAliasIndex buyerAlias, sellerAlias;
	if (!GetOfferAccept(vchAcceptRand, theOfferAccept))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1586 - " + _("Could not find this offer purchase"));

	if(!GetAlias(CNameTXIDTuple(theOfferAccept.buyerAliasTuple.first, theOfferAccept.buyerAliasTuple.second), buyerAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1587 - " + _("Could not find buyer alias"));

	if (!GetOffer(theOfferAccept.offerTuple, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1582 - " + _("Could not find an offer with this guid"));


	

	if (!theOffer.linkOfferTuple.first.empty())
	{
		if (!GetOffer(theOffer.linkOfferTuple, theOffer))
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1583 - " + _("Could not find a linked offer with this guid"));
		if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), sellerAlias))
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1584 - " + _("Could not find merchant alias"));
	}
	else
	{
		if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), sellerAlias))
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1585 - " + _("Could not find merchant alias"));
	}

	CSyscoinAddress sellerAddress;
	GetAddress(sellerAlias, &sellerAddress, sellerScript);
	CSyscoinAddress buyerAddress;
	GetAddress(buyerAlias, &buyerAddress, buyerScript);

	CScript scriptPubKeyAlias;
	CScript scriptPubKeyBuyer;

	CKeyID keyID;
	if (!sellerAddress.GetKeyID(keyID))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1588 - " + _("Seller address does not refer to a key"));
	CKey vchSecret;
	if (!pwalletMain->GetKey(keyID, vchSecret))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1589 - " + _("Private key for seller address is not known"));



	theOffer.ClearOffer();
	theOffer.accept.bPaymentAck = true;
	theOffer.accept.buyerAliasTuple = CNameTXIDTuple(sellerAlias.vchAlias, sellerAlias.txHash, sellerAlias.vchGUID);
	theOffer.accept.nPaymentOption = theOfferAccept.nPaymentOption;
	theOffer.nHeight = chainActive.Tip()->nHeight;

	scriptPubKeyAlias = CScript() <<  CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAlias.vchAlias << sellerAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKeyAlias += sellerScript;


	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());

	vector<CRecipient> vecSend;
	CRecipient recipientAlias, recipientPaymentAlias, recipient, recipientBuyer;


	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << theOfferAccept.vchAcceptRand << vchFromString("0") << vchHashOffer << OP_2DROP <<  OP_2DROP << OP_DROP;
	scriptPubKeyBuyer += buyerScript;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	CreateRecipient(scriptPubKeyAlias, recipientAlias);
	CreateAliasRecipient(sellerScript, sellerAlias.vchAlias, sellerAlias.aliasPegTuple, recipientPaymentAlias);

	vecSend.push_back(recipientBuyer);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, sellerAlias.aliasPegTuple, data, fee);
	vecSend.push_back(fee);



	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(sellerAlias.vchAlias, vchWitness, sellerAlias.aliasPegTuple.first, "", recipientAlias, recipientPaymentAlias, vecSend, wtx, &coinControl);
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
UniValue offeracceptinfo(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size())
		throw runtime_error("offeracceptinfo <acceptguid>\n"
			"Show details of an offer purchase\n");

	UniValue oOfferAccept(UniValue::VOBJ);
	vector<unsigned char> vchOfferAccept = vchFromValue(params[0]);
	COfferAccept offerAccept;
	if (!GetOfferAccept(vchOfferAccept, offerAccept))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from offeraccept DB"));

	CTransaction tx;
	if (!GetSyscoinTransaction(offerAccept.nHeight, offerAccept.txHash, tx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to read offeraccept tx"));

	vector<vector<unsigned char> > vvch;
	int op, nOut;
	if (!DecodeOfferTx(tx, op, nOut, vvch))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to decode offeraccept"));

	if (!BuildOfferAcceptJson(offerAccept, oOfferAccept))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1593 - " + _("Could not find this offeraccept"));

	return oOfferAccept;

}
void BuildFeedbackJson(const COfferAccept& theOfferAccept, UniValue& oFeedback) {
	if (theOfferAccept.feedback.IsNull())
		return;
	string sFeedbackTime;
	if (chainActive.Height() >= theOfferAccept.nHeight) {
		CBlockIndex *pindex = chainActive[theOfferAccept.nHeight];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->nTime);
		}
	}
	const string &id = stringFromVch(theOfferAccept.vchAcceptRand) + boost::lexical_cast<std::string>(theOfferAccept.feedback.nFeedbackUserTo);
	oFeedback.push_back(Pair("_id", id));
	oFeedback.push_back(Pair("offer", stringFromVch(theOfferAccept.offerTuple.first)));
	oFeedback.push_back(Pair("offeraccept", stringFromVch(theOfferAccept.vchAcceptRand)));
	oFeedback.push_back(Pair("escrow", ""));
	oFeedback.push_back(Pair("txid", theOfferAccept.txHash.GetHex()));
	oFeedback.push_back(Pair("time", sFeedbackTime));
	oFeedback.push_back(Pair("rating", theOfferAccept.feedback.nRating));
	oFeedback.push_back(Pair("feedbackfrom", theOfferAccept.feedback.nFeedbackUserFrom));
	oFeedback.push_back(Pair("feedbackto", theOfferAccept.feedback.nFeedbackUserTo));
	oFeedback.push_back(Pair("feedback", stringFromVch(theOfferAccept.feedback.vchFeedback)));
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
	oOffer.push_back(Pair("sysprice", theOffer.GetDisplayPrice()));
	oOffer.push_back(Pair("price", theOffer.sPrice));
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
bool BuildOfferAcceptJson(const COfferAccept& theOfferAccept, UniValue& oOfferAccept)
{
	COffer theOffer, linkOffer;
	if (!GetOffer(theOfferAccept.offerTuple, theOffer))
		return false;
	if (!theOfferAccept.linkOfferTuple.first.empty() && !GetOffer(theOfferAccept.linkOfferTuple, linkOffer))
		return false;
	CAliasIndex sellerAlias;
	if (!GetAlias(CNameTXIDTuple(theOffer.aliasTuple.first, theOffer.aliasTuple.second), sellerAlias))
		return false;
	oOfferAccept.push_back(Pair("_id", stringFromVch(theOfferAccept.vchAcceptRand)));
	oOfferAccept.push_back(Pair("offer", stringFromVch(theOffer.vchOffer)));
	int64_t nTime = 0;
	CBlockIndex *pindex = chainActive[theOfferAccept.nHeight];
	if (pindex) {
		nTime = pindex->nTime;
	}
	oOfferAccept.push_back(Pair("txid", theOffer.txHash.GetHex()));
	oOfferAccept.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	string strExtId = "";
	if(!theOfferAccept.txExtId.IsNull())
		strExtId = theOfferAccept.txExtId.GetHex();
	oOfferAccept.push_back(Pair("exttxid", strExtId));
	oOfferAccept.push_back(Pair("paymentoption", (int)theOfferAccept.nPaymentOption));
	oOfferAccept.push_back(Pair("paymentoption_display", GetPaymentOptionsString(theOfferAccept.nPaymentOption)));
	oOfferAccept.push_back(Pair("height", (int64_t)theOfferAccept.nHeight));
	oOfferAccept.push_back(Pair("time", nTime));
	oOfferAccept.push_back(Pair("quantity", (int)theOfferAccept.nQty));
	oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
	oOfferAccept.push_back(Pair("address", EncodeBase58(sellerAlias.vchAddress)));
	if(theOffer.GetPrice() > 0)
		oOfferAccept.push_back(Pair("offer_discount_percentage", (int)(100.0f - 100.0f*((float)theOfferAccept.nPrice/(float)theOffer.nPrice))));
	else
		oOfferAccept.push_back(Pair("offer_discount_percentage", 0));

	oOfferAccept.push_back(Pair("systotal", theOfferAccept.nPrice* theOfferAccept.nQty));
	oOfferAccept.push_back(Pair("sysprice", theOfferAccept.nPrice));
	oOfferAccept.push_back(Pair("total", theOfferAccept.sTotal));
	oOfferAccept.push_back(Pair("buyer", stringFromVch(theOfferAccept.buyerAliasTuple.first)));
	oOfferAccept.push_back(Pair("seller", stringFromVch(theOffer.aliasTuple.first)));
	if(!linkOffer.IsNull())
		oOfferAccept.push_back(Pair("rootseller", stringFromVch(linkOffer.aliasTuple.first)));
	else
		oOfferAccept.push_back(Pair("rootseller", ""));
	string statusStr = "Paid";
	if(!theOfferAccept.txExtId.IsNull())
		statusStr = "Paid with external coin";
	if(theOfferAccept.bPaymentAck)
		statusStr += " (acknowledged)";
	oOfferAccept.push_back(Pair("status",statusStr));
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

	if(offer.accept.bPaymentAck)
		opName += "("+_("acknowledged")+")";
	else if(!offer.accept.feedback.IsNull())
		opName += "("+_("feedback")+")";
	entry.push_back(Pair("txtype", opName));
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

	if(offer.accept.bPaymentAck && offer.accept.bPaymentAck != dbOffer.accept.bPaymentAck)
		entry.push_back(Pair("paymentacknowledge", offer.accept.bPaymentAck));

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
		entry.push_back(Pair("price", boost::lexical_cast<string>(offer.GetPrice())));

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