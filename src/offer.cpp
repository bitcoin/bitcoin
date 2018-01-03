// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
extern mongoc_collection_t *offer_collection;
extern mongoc_collection_t *offerhistory_collection;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend=false, bool transferAlias=false);
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

bool ValidateOfferTypeMask(const uint32_t &offerTypeMask) {
	uint64_t maxVal = OFFERTYPE_BUYNOW | OFFERTYPE_COIN | OFFERTYPE_AUCTION;
	return !(offerTypeMask < 1 || offerTypeMask > maxVal);
}

bool IsValidOfferType(const uint32_t &offerTypeMask) {
	return (offerTypeMask == OFFERTYPE_BUYNOW || offerTypeMask == OFFERTYPE_COIN || offerTypeMask == OFFERTYPE_AUCTION);
}

bool ValidateOfferTypeString(const std::string &offerTypeString) {
	bool retval = true;
	vector<string> strs;
	boost::split(strs, offerTypeString, boost::is_any_of("+"));
	for (size_t i = 0; i < strs.size(); i++) {
		if (strs[i].compare("BUYNOW") != 0 && strs[i].compare("COIN") != 0 && strs[i].compare("AUCTION") != 0) {
			retval = false;
			break;
		}
	}
	return retval;
}

uint32_t GetOfferTypeMaskFromString(const std::string &offerTypeString) {
	vector<string> strs;
	uint32_t retval = 0;
	boost::split(strs, offerTypeString, boost::is_any_of("+"));
	for (size_t i = 0; i < strs.size(); i++) {
		if (!strs[i].compare("BUYNOW")) {
			retval |= OFFERTYPE_BUYNOW;
		}
		else if (!strs[i].compare("COIN")) {
			retval |= OFFERTYPE_COIN;
		}
		else if (!strs[i].compare("AUCTION")) {
			retval |= OFFERTYPE_AUCTION;
		}
		else return 0;
	}
	return retval;
}

bool IsOfferTypeInMask(const uint32_t &mask, const uint32_t &offerType) {
	return mask & offerType ? true : false;
}
uint64_t GetOfferExpiration(const COffer& offer) {
	// dont prune by default, set nHeight to future time
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	// if service alias exists in unprunable db (this should always exist for any alias that ever existed) then get the last expire height set for this alias and check against it for pruning
	if (paliasdb && paliasdb->ReadAliasUnprunable(offer.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
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
	pair<string, vector<unsigned char> > keyTuple;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(keyTuple) && keyTuple.first == "offeri") {
  				if (!GetOffer(keyTuple.second, offer) || chainActive.Tip()->GetMedianTimePast() >= GetOfferExpiration(offer))
				{
					servicesCleaned++;
					EraseOffer(keyTuple.second, true);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

bool GetOffer(const vector<unsigned char> &vchOffer,
				  COffer& txPos) {
	if (!pofferdb || !pofferdb->ReadOffer(vchOffer, txPos))
		return false;
	if (chainActive.Tip()->GetMedianTimePast() >= GetOfferExpiration(txPos))
	{
		txPos.SetNull();
		return false;
	}
	return true;
}
bool DecodeAndParseOfferTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	COffer offer;
	bool decode = DecodeOfferTx(tx, op, nOut, vvch);
	bool parse = offer.UnserializeFromTx(tx);
	if (decode && parse) {
		type = OFFER;
		return true;
	}
	return false;
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
	if (op != OP_SYSCOIN_OFFER)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsOfferOp(op))
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

bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (!pofferdb || !paliasdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add offer in coinbase transaction, skipping...");
		return true;
	}
	const uint64_t &nTime = chainActive.Tip()->GetMedianTimePast();
	if (fDebug && !dontaddtodb)
		LogPrintf("*** OFFER %d %d %s %s %s %d\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK", " VVCH SIZE: ", vvchArgs.size());
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

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1002 - " + _("Offer arguments incorrect size");
			return error(errorMessage.c_str());
		}
		
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1005 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
	}

	if (theOffer.vchAlias != vvchAliasArgs[0]) {
		errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4003 - " + _("Alias input mismatch");
		if (fJustCheck)
			return error(errorMessage.c_str());
		else
			return true;
	}
	// unserialize offer from txn, check for valid
	COffer linkOffer;
	COffer myOffer;
	CCert theCert;
	CAliasIndex theAlias, alias, linkAlias;
	vector<string> categories;
	string retError = "";
	// just check is for the memory pool inclusion, here we can stop bad transactions from entering before we get to include them in a block
	if(fJustCheck)
	{
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
		if (theOffer.fPrice <= 0)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1009 - " + _("Offer price must be a positive number");
			return error(errorMessage.c_str());
		}
		if(theOffer.vchLinkOffer.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1010 - " + _("Offer link guid hash too long");
			return error(errorMessage.c_str());
		}
		if(theOffer.sCurrencyCode.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1011 - " + _("Offer currency code too long");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_OFFER_ACTIVATE:
			if(!ValidatePaymentOptionsMask(theOffer.paymentOptions))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Invalid payment option");
				return error(errorMessage.c_str());
			}
			if (!ValidateOfferTypeMask(theOffer.offerType))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("Invalid offer type");
				return error(errorMessage.c_str());
			}
			if ( theOffer.vchOffer != theOffer.vchOffer)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1018 - " + _("Offer input and offer guid mismatch");
				return error(errorMessage.c_str());
			}
			
			if(theOffer.nCommission > 100 || theOffer.nCommission < -90)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1019 - " + _("Commission percentage must be between -90 and 100");
				return error(errorMessage.c_str());
			}		
			if(theOffer.vchLinkOffer.empty())
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
			if(!theOffer.vchCert.empty() && theOffer.nQty != 1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Quantity must be 1 for a digital offer");
				return error(errorMessage.c_str());
			}

			if (IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_AUCTION))
			{
				if (theOffer.auctionOffer.fReservePrice < 0)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Reserve price must be greator or equal to 0");
					return error(errorMessage.c_str());
				}
				if (theOffer.auctionOffer.fDepositPercentage < 0)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Auction deposit percentage must be greator or equal to 0");
					return error(errorMessage.c_str());
				}
				if (theOffer.auctionOffer.nExpireTime > 0 && theOffer.auctionOffer.nExpireTime < nTime)
				{
					errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Invalid auction expiry");
					return error(errorMessage.c_str());
				}
			}
			break;
		case OP_OFFER_UPDATE:		
			if(theOffer.paymentOptions > 0 && !ValidatePaymentOptionsMask(theOffer.paymentOptions))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1027 - " + _("Invalid payment option");
				return error(errorMessage.c_str());
			}
			if (!ValidateOfferTypeMask(theOffer.offerType))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1027 - " + _("Invalid offer type");
				return error(errorMessage.c_str());
			}

			if(theOffer.nQty < -1)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1031 - " + _("Quantity must be greater than or equal to -1");
				return error(errorMessage.c_str());
			}
			if(!theOffer.vchCert.empty() && theOffer.nQty != 1)
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
	const string &user1 = stringFromVch(theOffer.vchAlias);
	string user2 = "";
	string user3 = "";
	if (op == OP_OFFER_UPDATE) {
		if (!theOffer.vchLinkAlias.empty())
			user2 = stringFromVch(theOffer.vchLinkAlias);
	}
	string strResponseEnglish = "";
	string strResponse = GetSyscoinTransactionDescription(op, strResponseEnglish, OFFER);
	char nLockStatus = NOLOCK_UNCONFIRMED_STATE;
	if (!fJustCheck)
		nLockStatus = NOLOCK_CONFIRMED_STATE;
	COffer dbOffer;
	// load the offer data from the DB
	if (!GetOffer(theOffer.vchOffer, dbOffer))
	{
		if (op == OP_OFFER_UPDATE) {
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1048 - " + _("Failed to read from offer DB");
			return true;
		}
	}
	else
	{
		bool bSendLocked = false;
		if (!fJustCheck && pofferdb->ReadISLock(theOffer.vchOffer, bSendLocked) && bSendLocked) {
			if (dbOffer.nHeight >= nHeight)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height.");
				return true;
			}
			if (dbOffer.txHash != tx.GetHash())
			{
				if (fDebug)
					LogPrintf("OFFER txid mismatch! Recreating...\n");
				if (op != OP_OFFER_ACTIVATE && !pofferdb->ReadLastOffer(theOffer.vchOffer, dbOffer)) {
					dbOffer.SetNull();
				}
				if (!dontaddtodb) {
					nLockStatus = LOCK_CONFLICT_CONFIRMED_STATE;
					if (!pofferdb->EraseISLock(theOffer.vchOffer))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from offer DB");
						return error(errorMessage.c_str());
					}
					const string &txHashHex = dbOffer.txHash.GetHex();
					paliasdb->EraseAliasIndexTxHistory(txHashHex);
					pofferdb->EraseOfferIndexHistory(txHashHex);
					pofferdb->EraseExtTXID(dbOffer.txHash);
				}
			}
			else {
				if (!dontaddtodb) {
					nLockStatus = LOCK_NOCONFLICT_CONFIRMED_STATE;
					if (fDebug)
						LogPrintf("CONNECTED OFFER: op=%s offer=%s qty=%u hash=%s height=%d fJustCheck=%d POW IS\n",
							offerFromOp(op).c_str(),
							stringFromVch(theOffer.vchOffer).c_str(),
							theOffer.nQty,
							tx.GetHash().ToString().c_str(),
							nHeight,
							fJustCheck ? 1 : 0);
					if (!pofferdb->Write(make_pair(std::string("offerp"), theOffer.vchOffer), dbOffer))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to write previous offer to offer DB");
						return error(errorMessage.c_str());
					}
					if (!pofferdb->EraseISLock(theOffer.vchOffer))
					{
						errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to erase Instant Send lock from offer DB");
						return error(errorMessage.c_str());
					}
					if (strResponse != "") {
						paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theOffer.vchOffer), nLockStatus);
					}
				}
				return true;
			}
		}
		else
		{
			if (fJustCheck && bSendLocked && dbOffer.nHeight >= nHeight && dbOffer.txHash != tx.GetHash())
			{
				if (!dontaddtodb) {
					nLockStatus = LOCK_CONFLICT_UNCONFIRMED_STATE;
					if (strResponse != "") {
						paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theOffer.vchOffer), nLockStatus);
					}
				}
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request must be less than or equal to the stored service block height.");
				return true;
			}
			if (dbOffer.nHeight > nHeight)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Block height of service request cannot be lower than stored service block height.");
				return true;
			}
			if (fJustCheck)
				nLockStatus = LOCK_NOCONFLICT_UNCONFIRMED_STATE;
		}
	}
	if (op == OP_OFFER_UPDATE) {
		if (dbOffer.vchAlias != theOffer.vchAlias)
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this offer. Offer owner must sign off on this change.");
			return true;
		}


		// some fields are only updated if they are not empty to limit txn size, rpc sends em as empty if we arent changing them
		if (theOffer.sCategory.empty())
			theOffer.sCategory = dbOffer.sCategory;
		if (theOffer.sTitle.empty())
			theOffer.sTitle = dbOffer.sTitle;
		if (theOffer.sDescription.empty())
			theOffer.sDescription = dbOffer.sDescription;
		if (theOffer.vchCert.empty())
			theOffer.vchCert = dbOffer.vchCert;
		if (theOffer.sCurrencyCode.empty())
			theOffer.sCurrencyCode = dbOffer.sCurrencyCode;
		if (theOffer.paymentOptions <= 0)
			theOffer.paymentOptions = dbOffer.paymentOptions;

		theOffer.vchLinkOffer = dbOffer.vchLinkOffer;


		if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
		{
			if (!IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_AUCTION))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Cannot change to this offer type until auction expires");
				return true;
			}
			if (theOffer.auctionOffer.fReservePrice < 0)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Reserve price must be greator or equal to 0");
				return true;
			}
			if (theOffer.auctionOffer.fDepositPercentage < 0)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Auction deposit percentage must be greator or equal to 0");
				return true;
			}
			if (dbOffer.auctionOffer.nExpireTime > 0 && dbOffer.auctionOffer.nExpireTime >= nTime && dbOffer.auctionOffer != theOffer.auctionOffer)
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1024 - " + _("Cannot modify auction parameters while it is active. Please wait until auction has expired before updating.");
				return true;
			}
		}
		// non linked offers cant edit commission
		if (theOffer.vchLinkOffer.empty())
			theOffer.nCommission = 0;
		if (!theOffer.vchLinkAlias.empty())
			theOffer.vchAlias = theOffer.vchLinkAlias;
	}
	else if(op == OP_OFFER_ACTIVATE)
	{
		COfferLinkWhitelistEntry entry;
		if (fJustCheck && GetOffer(theOffer.vchOffer, theOffer))
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1050 - " + _("Offer already exists");
			return true;
		}

		if (!theOffer.vchLinkOffer.empty())
		{
			if (!GetOffer(theOffer.vchLinkOffer, linkOffer))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1051 - " + _("Linked offer not found. It may be expired");
				return true;
			}
			if (!GetAlias(linkOffer.vchAlias, theAlias))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1051 - " + _("Linked offer alias not found. It may be expired");
				return true;
			}
			if (theAlias.offerWhitelist.GetLinkEntryByHash(theOffer.vchAlias, entry))
			{
				if (theOffer.nCommission <= -entry.nDiscountPct)
				{
					throw runtime_error("SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1514 - " + _("This resold offer must be of higher price than the original offer including any discount"));
				}
			}
			// make sure alias exists in the root offer affiliate list
			else
			{
				throw runtime_error("SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1515 - " + _("Cannot find this alias in the parent offer affiliate list"));
			}

			if (!linkOffer.vchLinkOffer.empty())
			{
				throw runtime_error("SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1516 - " + _("Cannot link to an offer that is already linked to another offer"));
			}
			else if (linkOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkOffer.sCategory), "wanted"))
			{
				throw runtime_error("SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1517 - " + _("Cannot link to a wanted offer"));
			}
			// if creating a linked offer we set some mandatory fields from the parent
			theOffer.nQty = linkOffer.nQty;
			theOffer.vchCert = linkOffer.vchCert;
			theOffer.fPrice = linkOffer.fPrice;
			theOffer.paymentOptions = linkOffer.paymentOptions;
			theOffer.fUnits = linkOffer.fUnits;
			theOffer.offerType = linkOffer.offerType;
			theOffer.sCurrencyCode = linkOffer.sCurrencyCode;
			theOffer.auctionOffer = linkOffer.auctionOffer;
		}
	}

	if(op == OP_OFFER_UPDATE)
	{
		// if this offer is linked to a parent update it with parent information
		if(!theOffer.vchLinkOffer.empty())
		{
			if (!GetOffer(theOffer.vchLinkOffer, linkOffer))
			{
				errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Linked offer not found. It may be expired");
				return true;
			}
			theOffer.nQty = linkOffer.nQty;
			theOffer.vchCert = linkOffer.vchCert;
			theOffer.paymentOptions = linkOffer.paymentOptions;
			theOffer.offerType = linkOffer.offerType;
			theOffer.fUnits = linkOffer.fUnits;
			theOffer.fPrice = linkOffer.fPrice;
			theOffer.sCurrencyCode = linkOffer.sCurrencyCode;
			theOffer.auctionOffer = linkOffer.auctionOffer;
		}
	}
	if(!dontaddtodb) {
		if (strResponse != "") {
			paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theOffer.vchOffer), nLockStatus);
		}
	}
	theOffer.vchLinkAlias.clear();
	theOffer.nHeight = nHeight;
	theOffer.txHash = tx.GetHash();
	// write offer
	if (!dontaddtodb) {
		if (!pofferdb->WriteOffer(theOffer, dbOffer, op, fJustCheck))
		{
			errorMessage = "SYSCOIN_OFFER_CONSENSUS_ERROR: ERRCODE: 1096 - " + _("Failed to write to offer DB");
			return error(errorMessage.c_str());
		}

		// debug
		if (fDebug)
			LogPrintf("CONNECTED OFFER: op=%s offer=%s qty=%u hash=%s height=%d fJustCheck=%d\n",
				offerFromOp(op).c_str(),
				stringFromVch(theOffer.vchOffer).c_str(),
				theOffer.nQty,
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : -1);
	}
	return true;
}
UniValue offernew(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 17)
		throw runtime_error(
			"offernew [alias] [category] [title] [quantity] [price] [description] [currency] [cert. guid] [payment options=SYS] [private=false] [units] [offertype=BUYNOW] [auction_expires] [auction_reserve] [auction_require_witness] [auction_deposit] [witness]\n"
						"<alias> An alias you own.\n"
						"<category> category, 256 characters max.\n"
						"<title> title, 256 characters max.\n"
						"<quantity> quantity, > 0 or -1 for infinite\n"
						"<price> price in <currency>\n"
						"<description> description, 512 characters max.\n"
						"<currency> The currency code that you want your offer to be in ie: USD.\n"
						"<cert. guid> Set this to the guid of a certificate you wish to sell\n"
						"<paymentOptions> 'SYS' to accept SYS only, 'BTC' for BTC only, 'ZEC' for zcash only, or a |-delimited string to accept multiple currencies (e.g. 'BTC|SYS' to accept BTC or SYS). Leave empty for default. Defaults to 'SYS'.\n"		
						"<private> set to Yes if this offer should be private not be searchable. Defaults to No.\n"
						"<units> Units that 1 qty represents. For example if selling 1 BTC. Default is 1.\n"
						"<offertype> Options of how an offer is sold. 'BUYNOW' for regular Buy It Now offer, 'AUCTION' to auction this offer while providing auction_expires/auction_reserve/auction_require_witness parameters, 'COIN' for offers selling cryptocurrency, or a | -delimited string to create an offer with multiple options(e.g. 'BUYNOW|AUCTION' to create an offer that is sold through an auction but has Buy It Now enabled as well).Leave empty for default. Defaults to 'BUYNOW'.\n"
						"<auction_expires> If offerType is AUCTION, Datetime of expiration of an auction. Once merchant creates an offer as an auction, the expiry must be non-zero. The auction parameters will not be updateable until an auction expires.\n"
						"<auction_reserve> If offerType is AUCTION, Reserve price of an offer publicly. Bids must be of higher price than the reserve price. Any bid below the reserve price will be rejected by consensus checks in escrow. Default is 0.\n"
						"<auction_require_witness> If offerType is AUCTION, Require a witness signature for bids of an offer, or release/refund of an escrow funds in an auction for the offer. Set to true if you wish to require witness signature. Default is false.\n"
						"<auction_deposit> If offerType is AUCTION. If you require a deposit for each bidder to ensure stake to bidders set this to a percentage of the offer price required to place deposit when doing an initial bid. For Example: 1% of the offer price would be 0.01. Default is 0.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());
	// gather inputs
	float fPrice;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAliasIndex alias;
	if (!GetAlias(vchAlias, alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1500 - " + _("Could not find an alias with this name"));

	vector<unsigned char> vchCategory =  vchFromValue(params[1]);
	vector<unsigned char> vchTitle =  vchFromValue(params[2]);
	vector<unsigned char> vchCert;

	int nQty;
	nQty =  params[3].get_int();

	fPrice = params[4].get_real();
	vector<unsigned char> vchDescription = vchFromValue(params[5]);
	vector<unsigned char> vchCurrency = vchFromValue(params[6]);
	CScript scriptPubKeyOrig;
	CScript scriptPubKey;
	vchCert = vchFromValue(params[7]);
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOptions = "SYS";
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
	bPrivate = params[9].get_bool();
	float fUnits;
	fUnits = params[10].get_real();
	string offerType = "BUYNOW";
	offerType = params[11].get_str();

	boost::algorithm::to_upper(offerType);
	if (!ValidateOfferTypeString(offerType))
	{
		string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1504 - " + _("Could not validate offer type");
		throw runtime_error(err.c_str());
	}
	uint32_t offerTypeMask = GetOfferTypeMaskFromString(offerType);

	uint64_t nExpireTime = chainActive.Tip()->GetMedianTimePast() + 3600;
	nExpireTime = params[12].get_int64();

	float fReservePrice = 0;
	fReservePrice = params[13].get_real();

	bool bRequireWitness = false;
	bRequireWitness = params[14].get_bool();
	

	float fAuctionDeposit = 0;
	fAuctionDeposit = params[15].get_real();

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[16]);


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
		else if(theCert.vchAlias != vchAlias)
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
	newOffer.vchAlias = alias.vchAlias;
	newOffer.vchOffer = vchOffer;
	newOffer.sCategory = vchCategory;
	newOffer.sTitle = vchTitle;
	newOffer.sDescription = vchDescription;
	newOffer.nQty = nQty;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	if (!vchCert.empty())
		newOffer.vchCert = theCert.vchCert;
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = bPrivate;
	newOffer.paymentOptions = paymentOptionsMask;
	newOffer.fUnits = fUnits;
	newOffer.offerType = offerTypeMask;
	newOffer.fPrice = fPrice;
	newOffer.auctionOffer.bRequireWitness = bRequireWitness;
	newOffer.auctionOffer.fReservePrice = fReservePrice;
	newOffer.auctionOffer.nExpireTime = nExpireTime;
	newOffer.auctionOffer.fDepositPercentage = fAuctionDeposit;

	vector<unsigned char> data;
	newOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	CSyscoinAddress aliasAddress;
	GetAddress(alias, &aliasAddress, scriptPubKeyOrig);
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_OFFER) << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

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
	res.push_back(stringFromVch(vchOffer));
	return res;
}

UniValue offerlink(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 5)
		throw runtime_error(
			"offerlink [alias] [guid] [commission] [description] [witness]\n"
						"<alias> An alias you own.\n"
						"<guid> offer guid that you are linking to\n"
						"<commission> percentage of profit desired over original offer price, > 0, ie: 5 for 5%\n"
						"<description> description, 512 characters max.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAliasIndex alias;
	if (!GetAlias(vchAlias, alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1510 - " + _("Could not find an alias with this name"));
 

	vector<unsigned char> vchLinkOffer = vchFromValue(params[1]);
	vector<unsigned char> vchDescription;
	COffer linkOffer;
	if (vchLinkOffer.empty() || !GetOffer( vchLinkOffer, linkOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1513 - " + _("Could not find an offer with this guid"));
	CAliasIndex linkAlias;
	if (!GetAlias(linkOffer.vchAlias, linkAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1513 - " + _("Could not find an alias associated with this offer"));

	int commissionInteger = params[2].get_int();
	vchDescription = vchFromValue(params[3]);
	if(vchDescription.empty())
	{
		vchDescription = linkOffer.sDescription;
	}
	
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);
	CScript scriptPubKeyOrig;
	CScript scriptPubKey;

	// this is a syscoin transaction
	CWalletTx wtx;


	// generate rand identifier
	vector<unsigned char> vchOffer = vchFromString(GenerateSyscoinGuid());
	

	// build offer
	COffer newOffer = linkOffer;
	newOffer.ClearOffer();
	newOffer.vchOffer = vchOffer;
	newOffer.vchAlias = alias.vchAlias;
	newOffer.sDescription = vchDescription;
	newOffer.nCommission = commissionInteger;
	newOffer.vchLinkOffer = linkOffer.vchOffer;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.sCategory = linkOffer.sCategory;
	newOffer.sTitle = linkOffer.sTitle;
	//create offeractivate txn keys

	vector<unsigned char> data;
	newOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	CSyscoinAddress aliasAddress;
	GetAddress(alias, &aliasAddress, scriptPubKeyOrig);
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_OFFER) << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;


	string strError;

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
	res.push_back(stringFromVch(vchOffer));
	return res;
}
UniValue offerupdate(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 18)
		throw runtime_error(
			"offerupdate [alias] [guid] [category] [title] [quantity] [price] [description] [currency] [private=false] [cert. guid] [commission] [paymentOptions] [offerType=BUYNOW] [auction_expires] [auction_reserve] [auction_require_witness] [auction_deposit] [witness]\n"
						"Perform an update on an offer you control.\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	string strCategory = "";
	string strDescription = "";
	string strTitle = "";
	string strCert = "";
	string strCurrency = "";
	string paymentOptions = "";
	strCategory = params[2].get_str();
	strTitle = params[3].get_str();
	int nQty = params[4].get_int();
	float fPrice = params[5].get_real();
	strDescription = params[6].get_str();
	strCurrency = params[7].get_str();
	bool bPrivate = params[8].get_bool();
	strCert = params[9].get_str();
	int nCommission = params[10].get_int();
	paymentOptions = params[11].get_str();	
	boost::algorithm::to_upper(paymentOptions);
	
	
	if(!paymentOptions.empty() && !ValidatePaymentOptionsString(paymentOptions))
	{
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1532 - " + _("Could not validate payment options string"));
	}
	uint64_t paymentOptionsMask = GetPaymentOptionsMaskFromString(paymentOptions);

	string strOfferType = "";
	strOfferType = params[12].get_str();

	boost::algorithm::to_upper(strOfferType);
	if (!strOfferType.empty() && !ValidateOfferTypeString(strOfferType))
	{
		string err = "SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1504 - " + _("Could not validate offer type");
		throw runtime_error(err.c_str());
	}
	uint32_t offerTypeMask = GetOfferTypeMaskFromString(strOfferType);
	uint64_t nExpireTime = 0;
	nExpireTime = params[13].get_int64();
	float fReservePrice = 0;
	fReservePrice = params[14].get_real();
	bool bRequireWitness = false;
	bRequireWitness = params[15].get_bool();
	float fAuctionDeposit = 0;
	fAuctionDeposit = params[16].get_real();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[17]);

	CAliasIndex alias, linkAlias;

	// this is a syscoind txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;

	COffer theOffer, linkOffer;
	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1534 - " + _("Could not find an offer with this guid"));

	if (!GetAlias(theOffer.vchAlias, alias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1535 - " + _("Could not find an alias with this name"));
	if (vchAlias != alias.vchAlias && !GetAlias(vchAlias, linkAlias))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1536 - " + _("Could not find an alias with this name"));

	CCert theCert;
	if(!strCert.empty())
	{
		if (!GetCert( vchFromString(strCert), theCert))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1538 - " + _("Updating an offer with a cert that does not exist"));
		}
		else if(theOffer.vchLinkOffer.empty() && theCert.vchAlias != theOffer.vchAlias)
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
	if(!theOffer.vchLinkOffer.empty())
	{
		COffer linkOffer;
		if (!GetOffer( theOffer.vchLinkOffer, linkOffer))
		{
			throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 1542 - " + _("Linked offer not found. It may be expired"));
		}
		if (!linkOffer.vchLinkOffer.empty())
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

	if(strCurrency != stringFromVch(offerCopy.sCurrencyCode))
		theOffer.sCurrencyCode = vchFromString(strCurrency);
	if(strTitle != stringFromVch(offerCopy.sTitle))
		theOffer.sTitle = vchFromString(strTitle);
	if(strCategory != stringFromVch(offerCopy.sCategory))
		theOffer.sCategory = vchFromString(strCategory);
	if(strDescription != stringFromVch(offerCopy.sDescription))
		theOffer.sDescription = vchFromString(strDescription);
	// linked offers can't change these settings, they are overrided by parent info
	if(offerCopy.vchLinkOffer.empty())
	{
		if(!strCert.empty())
			theOffer.vchCert = theCert.vchCert;

		theOffer.fPrice = fPrice;
	}

	theOffer.nCommission = nCommission;
	theOffer.paymentOptions = paymentOptionsMask;

	if(!vchAlias.empty() && vchAlias != alias.vchAlias)
		theOffer.vchLinkAlias = linkAlias.vchAlias;
	
	theOffer.nQty = nQty;
	theOffer.bPrivate = bPrivate;
	theOffer.offerType = offerTypeMask;

	theOffer.auctionOffer.bRequireWitness = bRequireWitness;
	theOffer.auctionOffer.fReservePrice = fReservePrice;
	theOffer.auctionOffer.nExpireTime = nExpireTime;
	theOffer.auctionOffer.fDepositPercentage = fAuctionDeposit;

	theOffer.nHeight = chainActive.Tip()->nHeight;

	vector<unsigned char> data;
	theOffer.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashOffer = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_OFFER) << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchHashOffer << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << alias.vchAlias << alias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
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


	SendMoneySyscoin(alias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

void COfferDB::WriteOfferIndex(const COffer& offer, const int &op) {
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
	if (BuildOfferIndexerJson(offer, oName)) {
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
	WriteOfferIndexHistory(offer, op);
}
void COfferDB::WriteOfferIndexHistory(const COffer& offer, const int &op) {
	if (!offerhistory_collection)
		return;
	bson_error_t error;
	bson_t *insert = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	string serviceFromOp = "";
	if (IsEscrowOp(op))
		serviceFromOp = escrowFromOp(op);
	else if (IsOfferOp(op))
		serviceFromOp = offerFromOp(op);


	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);

	if (BuildOfferIndexerHistoryJson(offer, oName)) {
		oName.push_back(Pair("op", serviceFromOp));
		insert = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!insert || !mongoc_collection_insert(offerhistory_collection, (mongoc_insert_flags_t)MONGOC_INSERT_NO_VALIDATE, insert, write_concern, &error)) {
			LogPrintf("MONGODB OFFER HISTORY ERROR: %s\n", error.message);
		}
	}

	if (insert)
		bson_destroy(insert);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferIndexHistory(const std::vector<unsigned char>& vchOffer, bool cleanup) {
	if (!offerhistory_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("offer", BCON_UTF8(stringFromVch(vchOffer).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(offerhistory_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB OFFER HISTORY REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferIndexHistory(const string &id) {
	if (!offerhistory_collection)
		return;
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(id.c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(offerhistory_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB OFFER HISTORY REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void COfferDB::EraseOfferIndex(const std::vector<unsigned char>& vchOffer, bool cleanup) {
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
	if (!mongoc_collection_remove(offer_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB OFFER REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
	EraseOfferIndexHistory(vchOffer, cleanup);
}
UniValue offerinfo(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size())
		throw runtime_error("offerinfo <guid>\n"
				"Show offer details\n");

	UniValue oOffer(UniValue::VOBJ);
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	COffer txPos;

	if (!pofferdb || !pofferdb->ReadOffer(vchOffer, txPos))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from offer DB"));

	if(!BuildOfferJson(txPos, oOffer))
		throw runtime_error("SYSCOIN_OFFER_RPC_ERROR ERRCODE: 1593 - " + _("Could not find this offer"));

	return oOffer;

}
bool BuildOfferJson(const COffer& theOffer, UniValue& oOffer)
{
	CAliasIndex latestAlias;
	GetAlias(theOffer.vchAlias, latestAlias);
	CTransaction linkTx;
	COffer linkOffer;
	if( !theOffer.vchLinkOffer.empty())
	{
		if(!GetOffer( theOffer.vchLinkOffer, linkOffer))
			return false;
	}

	bool expired = false;

	int64_t expired_time;
	expired = 0;

	expired_time = 0;
	vector<unsigned char> vchCert;
	if(!theOffer.vchCert.empty())
		vchCert = theOffer.vchCert;
	oOffer.push_back(Pair("_id", stringFromVch(theOffer.vchOffer)));
	oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
	oOffer.push_back(Pair("txid", theOffer.txHash.GetHex()));
	expired_time =  GetOfferExpiration(theOffer);
    if(expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}

	oOffer.push_back(Pair("expires_on", expired_time));
	oOffer.push_back(Pair("expired", expired));
	oOffer.push_back(Pair("height", (int)theOffer.nHeight));
	oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
	oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	int nQty = theOffer.nQty;
	string offerTypeStr = "";
	CAuctionOffer auctionOffer;
	if (IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_AUCTION))
		auctionOffer = theOffer.auctionOffer;
	if(!theOffer.vchLinkOffer.empty()) {
		oOffer.push_back(Pair("currency", stringFromVch(linkOffer.sCurrencyCode)));
		oOffer.push_back(Pair("price", linkOffer.GetPrice(theOffer.nCommission)));
		oOffer.push_back(Pair("commission", theOffer.nCommission));
		oOffer.push_back(Pair("offerlink_guid", stringFromVch(theOffer.vchLinkOffer)));
		oOffer.push_back(Pair("offerlink_seller", stringFromVch(linkOffer.vchAlias)));
		oOffer.push_back(Pair("paymentoptions", GetPaymentOptionsString(linkOffer.paymentOptions)));
		oOffer.push_back(Pair("offer_units", linkOffer.fUnits));
		nQty = linkOffer.nQty;
		offerTypeStr = GetOfferTypeString(linkOffer.offerType);
		if (IsOfferTypeInMask(linkOffer.offerType, OFFERTYPE_AUCTION))
			auctionOffer = linkOffer.auctionOffer;
	}
	else
	{
		oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
		oOffer.push_back(Pair("price", theOffer.GetPrice()));
		oOffer.push_back(Pair("commission", 0));
		oOffer.push_back(Pair("offerlink_guid", ""));
		oOffer.push_back(Pair("offerlink_seller", ""));
		oOffer.push_back(Pair("paymentoptions", GetPaymentOptionsString(theOffer.paymentOptions)));
		oOffer.push_back(Pair("offer_units", theOffer.fUnits));
		offerTypeStr = GetOfferTypeString(theOffer.offerType);
	}
	oOffer.push_back(Pair("quantity", nQty));
	oOffer.push_back(Pair("private", theOffer.bPrivate));
	oOffer.push_back(Pair("description", stringFromVch(theOffer.sDescription)));
	oOffer.push_back(Pair("alias", stringFromVch(theOffer.vchAlias)));
	oOffer.push_back(Pair("address", EncodeBase58(latestAlias.vchAddress)));
	oOffer.push_back(Pair("offertype", offerTypeStr));
	oOffer.push_back(Pair("auction_expires_on", auctionOffer.nExpireTime));
	oOffer.push_back(Pair("auction_reserve_price", auctionOffer.fReservePrice));
	oOffer.push_back(Pair("auction_require_witness", auctionOffer.bRequireWitness));
	oOffer.push_back(Pair("auction_deposit", auctionOffer.fDepositPercentage));

	return true;
}
bool BuildOfferIndexerJson(const COffer& theOffer, UniValue& oOffer)
{
	COffer linkOffer;
	if (!theOffer.vchLinkOffer.empty())
	{
		if (!GetOffer(theOffer.vchLinkOffer, linkOffer))
			return false;
	}
	vector<unsigned char> vchCert;
	if (!theOffer.vchCert.empty())
		vchCert = theOffer.vchCert;
	oOffer.push_back(Pair("_id", stringFromVch(theOffer.vchOffer)));
	oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
	oOffer.push_back(Pair("height", (int)theOffer.nHeight));
	oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
	oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	int nQty = theOffer.nQty;
	string offerTypeStr = "";
	CAuctionOffer auctionOffer;
	if (IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_AUCTION))
		auctionOffer = theOffer.auctionOffer;
	if (!theOffer.vchLinkOffer.empty()) {
		oOffer.push_back(Pair("currency", stringFromVch(linkOffer.sCurrencyCode)));
		oOffer.push_back(Pair("price", linkOffer.GetPrice(theOffer.nCommission)));
		oOffer.push_back(Pair("paymentoptions", GetPaymentOptionsString(linkOffer.paymentOptions)));
		oOffer.push_back(Pair("offer_units", linkOffer.fUnits));
		nQty = linkOffer.nQty;
		offerTypeStr = GetOfferTypeString(linkOffer.offerType);
		if (IsOfferTypeInMask(linkOffer.offerType, OFFERTYPE_AUCTION))
			auctionOffer = linkOffer.auctionOffer;
	}
	else
	{
		oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
		oOffer.push_back(Pair("price", theOffer.GetPrice()));
		oOffer.push_back(Pair("paymentoptions", GetPaymentOptionsString(theOffer.paymentOptions)));
		oOffer.push_back(Pair("offer_units", theOffer.fUnits));
		offerTypeStr = GetOfferTypeString(theOffer.offerType);
	}

	oOffer.push_back(Pair("quantity", nQty));
	oOffer.push_back(Pair("private", theOffer.bPrivate));
	oOffer.push_back(Pair("alias", stringFromVch(theOffer.vchAlias)));
	oOffer.push_back(Pair("offertype", offerTypeStr));
	oOffer.push_back(Pair("auction_expires_on", auctionOffer.nExpireTime));
	oOffer.push_back(Pair("auction_reserve_price", auctionOffer.fReservePrice));
	return true;
}
bool BuildOfferIndexerHistoryJson(const COffer& theOffer, UniValue& oOffer)
{
	vector<unsigned char> vchCert;
	if (!theOffer.vchCert.empty())
		vchCert = theOffer.vchCert;
	oOffer.push_back(Pair("_id", theOffer.txHash.GetHex()));
	oOffer.push_back(Pair("offer", stringFromVch(theOffer.vchOffer)));
	oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
	oOffer.push_back(Pair("height", (int)theOffer.nHeight));
	oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
	oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	int nQty = theOffer.nQty;
	string offerTypeStr = "";
	CAuctionOffer auctionOffer;
	if (IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_AUCTION))
		auctionOffer = theOffer.auctionOffer;
	if (!theOffer.vchLinkOffer.empty()) {
		oOffer.push_back(Pair("currency", ""));
		oOffer.push_back(Pair("price", 0));
		oOffer.push_back(Pair("commission", theOffer.nCommission));
		oOffer.push_back(Pair("paymentoptions", ""));
		nQty = 0;
		offerTypeStr = "";
		auctionOffer.SetNull();
	}
	else
	{
		oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
		oOffer.push_back(Pair("price", theOffer.GetPrice()));
		oOffer.push_back(Pair("commission", 0));
		oOffer.push_back(Pair("paymentoptions", GetPaymentOptionsString(theOffer.paymentOptions)));
		offerTypeStr = GetOfferTypeString(theOffer.offerType);
	}

	oOffer.push_back(Pair("quantity", nQty));
	oOffer.push_back(Pair("private", theOffer.bPrivate));
	oOffer.push_back(Pair("description", stringFromVch(theOffer.sDescription)));
	oOffer.push_back(Pair("alias", stringFromVch(theOffer.vchAlias)));
	oOffer.push_back(Pair("offertype", offerTypeStr));
	oOffer.push_back(Pair("auction_expires_on", auctionOffer.nExpireTime));
	oOffer.push_back(Pair("auction_reserve_price", auctionOffer.fReservePrice));
	oOffer.push_back(Pair("auction_require_witness", auctionOffer.bRequireWitness));
	oOffer.push_back(Pair("auction_deposit", auctionOffer.fDepositPercentage));
	return true;
}
std::string GetOfferTypeString(const uint32_t &offerType)
{
	vector<std::string> types;
	if(IsPaymentOptionInMask(offerType, OFFERTYPE_BUYNOW)) {
		types.push_back(std::string("BUYNOW"));
	}
	if(IsPaymentOptionInMask(offerType, OFFERTYPE_COIN)) {
		types.push_back(std::string("COIN"));
	}
	if(IsPaymentOptionInMask(offerType, OFFERTYPE_AUCTION)) {
		types.push_back(std::string("AUCTION"));
	}
	return boost::algorithm::join(types, "+");
}
std::string GetPaymentOptionsString(const uint64_t &paymentOptions)
{
	vector<std::string> currencies;
	if (IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_SYS)) {
		currencies.push_back(std::string("SYS"));
	}
	if (IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_BTC)) {
		currencies.push_back(std::string("BTC"));
	}
	if (IsPaymentOptionInMask(paymentOptions, PAYMENTOPTION_ZEC)) {
		currencies.push_back(std::string("ZEC"));
	}
	return boost::algorithm::join(currencies, "+");
}
CChainParams::AddressType PaymentOptionToAddressType(const uint64_t &paymentOption)
{
	if (paymentOption == PAYMENTOPTION_SYS)
		return CChainParams::ADDRESS_SYS;
	else if (paymentOption == PAYMENTOPTION_BTC)
		return CChainParams::ADDRESS_BTC;
	else if(paymentOption == PAYMENTOPTION_ZEC)
		return CChainParams::ADDRESS_ZEC;
	return CChainParams::ADDRESS_SYS;
}

void OfferTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = offerFromOp(op);
	COffer offer;
	if(!offer.UnserializeFromData(vchData, vchHash))
		return;

	COffer dbOffer;
	GetOffer(offer.vchOffer, dbOffer);

	entry.push_back(Pair("_id", stringFromVch(offer.vchOffer)));
	if(!offer.vchCert.empty() && offer.vchCert != dbOffer.vchCert)
		entry.push_back(Pair("cert", stringFromVch(offer.vchCert)));

	if(!offer.vchLinkAlias.empty() && offer.vchLinkAlias != dbOffer.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(offer.vchLinkAlias)));

	if(!offer.vchLinkOffer.empty() && offer.vchLinkOffer != dbOffer.vchLinkOffer)
		entry.push_back(Pair("offerlink", stringFromVch(offer.vchLinkOffer)));

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
		entry.push_back(Pair("price", offer.GetPrice()));

	if(offer.bPrivate != dbOffer.bPrivate)
		entry.push_back(Pair("private", offer.bPrivate));
}
float COffer::GetPrice(const int commission) const {
	float price = fPrice;
	int comm = commission > 0 ? commission : nCommission;
	if (comm != 0)
		price += price*((float)comm / 100.0f);
	return price;
}
