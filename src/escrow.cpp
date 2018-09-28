// Copyright (c) 2015-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
#include "wallet/coincontrol.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
using namespace std;
extern CScript _createmultisig_redeemScript(const UniValue& params);
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
		|| op == OP_ESCROW_RELEASE_COMPLETE
		|| op == OP_ESCROW_REFUND_COMPLETE
		|| op == OP_ESCROW_BID
		|| op == OP_ESCROW_FEEDBACK
		|| op == OP_ESCROW_ACKNOWLEDGE
		|| op == OP_ESCROW_ADD_SHIPPING;
}
// % fee on escrow value for arbiter
int64_t GetEscrowArbiterFee(const int64_t &escrowValue, const float &fEscrowFee) {

	float fFee = fEscrowFee;
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
// check that the minimum arbiter fee is found
bool ValidateArbiterFee(const CEscrow &escrow) {
	CAmount nTotalWithoutFee = escrow.nAmountOrBidPerUnit*escrow.nQty;
	return GetEscrowArbiterFee(nTotalWithoutFee, 0.005) <= escrow.nArbiterFee;
}
// check that the minimum deposit is found
bool ValidateDepositFee(const float &fDepositPercentage, const CEscrow &escrow) {
	CAmount nTotalWithoutFee = escrow.nAmountOrBidPerUnit*escrow.nQty;
	return GetEscrowDepositFee(nTotalWithoutFee, fDepositPercentage) <= escrow.nDeposit;
}
// check that the minimum network fee is found
bool ValidateNetworkFee(const CEscrow &escrow) {
	return getFeePerByte(escrow.nPaymentOption)*400 <= escrow.nNetworkFee;
}

uint64_t GetEscrowExpiration(const CEscrow& escrow) {
	uint64_t nTime = chainActive.Tip()->nHeight + 1;
	CAliasUnprunable aliasBuyerPrunable,aliasSellerPrunable,aliasArbiterPrunable;
	if(paliasdb)
	{
		if (paliasdb->ReadAliasUnprunable(escrow.vchBuyerAlias, aliasBuyerPrunable) && !aliasBuyerPrunable.IsNull())
			nTime = aliasBuyerPrunable.nExpireTime;
		// buyer is expired try seller
		if (nTime <= (uint64_t)chainActive.Tip()->GetMedianTimePast())
		{
			if (paliasdb->ReadAliasUnprunable(escrow.vchSellerAlias, aliasSellerPrunable) && !aliasSellerPrunable.IsNull())
			{
				nTime = aliasSellerPrunable.nExpireTime;
				// seller is expired try the arbiter
				if(nTime <= (int64_t)chainActive.Tip()->GetMedianTimePast())
				{
					if (paliasdb->ReadAliasUnprunable(escrow.vchArbiterAlias, aliasArbiterPrunable) && !aliasArbiterPrunable.IsNull())
						nTime = aliasArbiterPrunable.nExpireTime;
				}
			}
		}
	}
	return nTime;
}

int escrowEnumFromOp(int op) {
	switch (op) {
	case OP_ESCROW_ACTIVATE:
		return EscrowStatus::InEscrow;
	case OP_ESCROW_RELEASE:
		return EscrowStatus::EscrowReleased;
	case OP_ESCROW_REFUND:
		return EscrowStatus::EscrowRefunded;
	case OP_ESCROW_REFUND_COMPLETE:
		return EscrowStatus::EscrowRefundComplete;
	case OP_ESCROW_RELEASE_COMPLETE:
		return EscrowStatus::EscrowReleaseComplete;
	default:
		return EscrowStatus::Unknown;
	}
	return EscrowStatus::Unknown;
}

string escrowFromOp(int op) {
    switch (op) {
    case OP_ESCROW_ACTIVATE:
        return "escrowactivate";
    case OP_ESCROW_RELEASE:
        return "escrowrelease";
    case OP_ESCROW_REFUND:
        return "escrowrefund";
	case OP_ESCROW_REFUND_COMPLETE:
		return "escrowrefundcomplete";
	case OP_ESCROW_RELEASE_COMPLETE:
		return "escrowreleasecomplete";
	case OP_ESCROW_BID:
		return "escrowbid";
	case OP_ESCROW_FEEDBACK:
		return "escrowfeedback";
	case OP_ESCROW_ACKNOWLEDGE:
		return "escrowacknowledge";
	case OP_ESCROW_ADD_SHIPPING:
		return "escrowaddshipping";
    default:
        return "<unknown escrow op>";
    }
	return "<unknown escrow op>";
}
bool CEscrow::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
		CDataStream dsEscrow(vchData, SER_NETWORK, PROTOCOL_VERSION);
		dsEscrow >> *this;
		vector<unsigned char> vchSerializedData;
		Serialize(vchSerializedData);
		const uint256 &calculatedHash = Hash(vchSerializedData.begin(), vchSerializedData.end());
		const vector<unsigned char> &vchRand = vchFromValue(calculatedHash.GetHex());
		if (vchRand != vchHash) {
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
void CEscrowDB::WriteEscrowIndex(const COffer& offer, const CEscrow& escrow, const std::vector<std::vector<unsigned char> > &vvchArgs) {
	if (IsArgSet("-zmqpubescrowrecord")) {
		UniValue oName(UniValue::VOBJ);
		if (BuildEscrowIndexerJson(offer, escrow, oName)) {
			GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "escrowrecord");
		}
	}
}
void CEscrowDB::WriteEscrowFeedbackIndex(const COffer &offer, const CEscrow& escrow) {
	if (IsArgSet("-zmqpubescrowfeedback")) {
		UniValue oName(UniValue::VOBJ);
		BuildFeedbackJson(offer, escrow, oName);
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "escrowfeedback");
	}
}
void CEscrowDB::WriteEscrowBidIndex(const COffer& offer, const CEscrow& escrow, const string& status) {
	if (escrow.op != OP_ESCROW_ACTIVATE)
		return;
	if (IsArgSet("-zmqpubescrowbid")) {
		UniValue oName(UniValue::VOBJ);
		BuildEscrowBidJson(offer, escrow, status, oName);
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "escrowbid");
	}
}
bool CEscrowDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CEscrow txPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "escrowi") {
  				if (!GetEscrow(key.second, txPos) || chainActive.Tip()->GetMedianTimePast() >= (int64_t)GetEscrowExpiration(txPos))
				{
					servicesCleaned++;
					EraseEscrow(key.second, true);
				}
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

bool GetEscrow(const vector<unsigned char> &vchEscrow,
        CEscrow& txPos) {

	if (!pescrowdb || !pescrowdb->ReadEscrow(vchEscrow, txPos))
		return false;
   if (chainActive.Tip()->GetMedianTimePast() >= (int64_t)GetEscrowExpiration(txPos)) {
		txPos.SetNull();
        return false;
    }
    return true;
}
bool DecodeAndParseEscrowTx(const CTransaction& tx, int& op,
	vector<vector<unsigned char> >& vvch, char& type)
{
	CEscrow escrow;
	bool decode = DecodeEscrowTx(tx, op, vvch);
	bool parse = escrow.UnserializeFromTx(tx);
	if (decode && parse) {
		type = ESCROW;
		return true;
	}
	return false;
}
bool DecodeEscrowTx(const CTransaction& tx, int& op,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeEscrowScript(out.scriptPubKey, op, vvchRead)) {
            found = true; vvch = vvchRead;
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
	if (op != OP_SYSCOIN_ESCROW)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsEscrowOp(op))
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
		vvch.emplace_back(std::move(vch));
	}

	// move the pc to after any DROP or NOP
	while (opcode == OP_DROP || opcode == OP_2DROP) {
		if (!script.GetOp(pc, opcode))
			break;
	}

	pc--;
	return found;
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
bool ValidateExternalPayment(const CEscrow& theEscrow, const bool &bSanityCheck, string& errorMessage)
{

	if(!theEscrow.extTxId.IsNull())
	{
		if(pofferdb->ExistsExtTXID(theEscrow.extTxId))
		{
			errorMessage = _("External Transaction ID specified was already used to pay for an offer");
			return true;
		}
		if (!bSanityCheck && !pofferdb->WriteExtTXID(theEscrow.extTxId))
		{
			errorMessage = _("Failed to External Transaction ID to DB");
			return false;
		}
	}
	return true;
}
bool CheckEscrowInputs(const CTransaction &tx, int op, const vector<vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, string &errorMessage, bool bSanityCheck) {
	if (!pescrowdb || !paliasdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !bSanityCheck)
	{
		LogPrintf("*Trying to add escrow in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !bSanityCheck)
		LogPrintf("*** ESCROW %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");


	 // unserialize escrow UniValue from txn, check for valid
    CEscrow theEscrow;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theEscrow.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR ERRCODE: 4000 - " + _("Cannot unserialize data inside of this transaction relating to an escrow");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4001 - " + _("Escrow arguments incorrect size");
			return error(errorMessage.c_str());
		}
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4002 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
		
	}

	CAliasIndex buyerAlias, sellerAlias, arbiterAlias;
    COffer theOffer, myLinkOffer;
	string retError = "";
	CTransaction txOffer;
	COffer dbOffer;
	if(fJustCheck)
	{
		if (theEscrow.vchRedeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4003 - " + _("Escrow redeem script too long");
			return error(errorMessage.c_str());
		}
		if(theEscrow.feedback.vchFeedback.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4004 - " + _("Feedback too long");
			return error(errorMessage.c_str());
		}
		if (theEscrow.nQty <= 0)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4005 - " + _("Quantity of order must be greator than 0");
			return error(errorMessage.c_str());
		}
		if(theEscrow.vchOffer.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4006 - " + _("Escrow offer guid too long");
			return error(errorMessage.c_str());
		}
		switch (op) {
			case OP_ESCROW_ACKNOWLEDGE:
				break;
			case OP_ESCROW_ACTIVATE:
				if (theEscrow.vchEscrow.size() > MAX_GUID_LENGTH)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4007 - " + _("escrow hex guid too long");
					return error(errorMessage.c_str());
				}
				if(!IsValidPaymentOption(theEscrow.nPaymentOption))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4008 - " + _("Invalid payment option");
					return error(errorMessage.c_str());
				}
				if (!theEscrow.extTxId.IsNull() && theEscrow.nPaymentOption == PAYMENTOPTION_SYS)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4009 - " + _("External payment cannot be paid with SYS");
					return error(errorMessage.c_str());
				}
				if (theEscrow.extTxId.IsNull() && theEscrow.nPaymentOption != PAYMENTOPTION_SYS)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("External payment missing transaction ID");
					return error(errorMessage.c_str());
				}
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4011 - " + _("Cannot leave feedback in escrow activation");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_BID:
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4012 - " + _("Cannot leave feedback in escrow bid");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_ADD_SHIPPING:
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4013 - " + _("Cannot leave feedback in escrow bid");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_RELEASE:
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4014 - " + _("Cannot leave feedback in escrow release");
					return error(errorMessage.c_str());
				}									
				if (theEscrow.role != EscrowRoles::BUYER && theEscrow.role != EscrowRoles::ARBITER)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4015 - " + _("Invalid role specified. Only arbiter or buyer can initiate an escrow release");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_RELEASE_COMPLETE:
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4016 - " + _("Cannot leave feedback in escrow release");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ESCROW_FEEDBACK:
				if(theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4017 - " + _("Feedback must leave a message");
					return error(errorMessage.c_str());
				}

				break;
			case OP_ESCROW_REFUND:									
				if (theEscrow.role != EscrowRoles::SELLER && theEscrow.role != EscrowRoles::ARBITER)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4018 - " + _("Invalid role specified. Only arbiter or seller can initiate an escrow refund");
					return error(errorMessage.c_str());
				}
				
				// Check input
				if(!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4019 - " + _("Cannot leave feedback in escrow refund");
					return error(errorMessage.c_str());
				}


				break;
			case OP_ESCROW_REFUND_COMPLETE:
				// Check input
				if (!theEscrow.feedback.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4020 - " + _("Cannot leave feedback in escrow refund");
					return error(errorMessage.c_str());
				}


				break;
			default:
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4021 - " + _("Escrow transaction has unknown op");
				return error(errorMessage.c_str());
		}
	}
	if (!fJustCheck) {
		if (op == OP_ESCROW_ACTIVATE)
		{
			if (!theEscrow.bPaymentAck)
			{
				if (!GetAlias(theEscrow.vchBuyerAlias, buyerAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4022 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.vchArbiterAlias, arbiterAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4023 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.vchSellerAlias, sellerAlias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4024 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
			}
		}
		if (vvchAliasArgs.size() >= 4 && !vvchAliasArgs[3].empty())
			theEscrow.vchWitness = vvchAliasArgs[3];
		CEscrow serializedEscrow = theEscrow;
		string strResponseEnglish = "";
		string strResponseGUID = "";
		CTransaction txTmp;
		GetSyscoinTransactionDescription(txTmp, op, strResponseEnglish, ESCROW, strResponseGUID);
		string user1 = "";
		string user2 = "";
		string user3 = "";
		if (!GetEscrow(serializedEscrow.vchEscrow, theEscrow))
		{
			if (op != OP_ESCROW_ACTIVATE) {
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4025 - " + _("Failed to read from escrow DB");
				return true;
			}
		}
		if (!GetOffer(theEscrow.vchOffer, dbOffer))
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4048 - " + _("Cannot find escrow offer. It may be expired");
			return true;
		}
		// make sure escrow settings don't change (besides scriptSigs/nTotal's) outside of activation
		if (op != OP_ESCROW_ACTIVATE)
		{
			if (op == OP_ESCROW_ACKNOWLEDGE && theEscrow.bPaymentAck)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4026 - " + _("Escrow already acknowledged");
			}

			if (!serializedEscrow.scriptSigs.empty())
				theEscrow.scriptSigs = serializedEscrow.scriptSigs;


			if (op == OP_ESCROW_BID) {
				if (theEscrow.op != OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4027 - " + _("Can only bid on an active escrow");
					return true;
				}
				if (vvchAliasArgs[0] != theEscrow.vchBuyerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4028 - " + _("Only buyer can bid on escrow");
					return true;
				}
				if (serializedEscrow.nAmountOrBidPerUnit <= theEscrow.nAmountOrBidPerUnit || serializedEscrow.fBidPerUnit <= theEscrow.fBidPerUnit)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4029 - " + _("Bid must be higher than the previous bid, please enter a higher amount");
					return true;
				}
				if (theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4031 - " + _("Cannot bid on an auction after you have used Buy It Now to purchase an offer");
					return true;
				}
				if (!IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4032 - " + _("Cannot bid on an offer that is not an auction");
					return true;
				}
				if (dbOffer.auctionOffer.fReservePrice > serializedEscrow.fBidPerUnit)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4033 - " + _("Cannot bid below offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
					return true;
				}
				if (dbOffer.auctionOffer.nExpireTime > 0 && dbOffer.auctionOffer.nExpireTime < chainActive.Tip()->GetMedianTimePast())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4034 - " + _("Offer auction has expired, cannot place bid!");
					return true;
				}
				if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4035 - " + _("Offer auction requires a witness signature for each bid but none provided");
					return true;
				}
				if (!dbOffer.vchLinkOffer.empty())
				{
					if (!GetOffer(dbOffer.vchLinkOffer, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4036 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (!IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4037 - " + _("Cannot bid on a linked offer that is not an auction");
						return true;
					}
					if (myLinkOffer.auctionOffer.fReservePrice > serializedEscrow.fBidPerUnit)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4038 - " + _("Cannot bid below linked offer reserve price of: ") + boost::lexical_cast<string>(myLinkOffer.auctionOffer.fReservePrice) + " " + stringFromVch(myLinkOffer.sCurrencyCode);
						return true;
					}
					if (myLinkOffer.auctionOffer.nExpireTime > 0 && myLinkOffer.auctionOffer.nExpireTime < chainActive.Tip()->GetMedianTimePast())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4039 - " + _("Linked offer auction has expired, cannot place bid!");
						return true;
					}
					if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Linked offer auction requires a witness signature for each bid but none provided");
						return true;
					}
				}	
				theEscrow.fBidPerUnit = serializedEscrow.fBidPerUnit;
				theEscrow.nAmountOrBidPerUnit = serializedEscrow.nAmountOrBidPerUnit;
				theEscrow.txHash = tx.GetHash();
				theEscrow.nHeight = nHeight;
				// write escrow bid
				if (!bSanityCheck)
				{
					pescrowdb->WriteEscrowBid(dbOffer, theEscrow, "valid");
				}
			}
			else if (op == OP_ESCROW_ADD_SHIPPING) {
				if (theEscrow.op != OP_ESCROW_ACTIVATE && theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4041 - " + _("Can only add shipping to an active escrow");
					return true;
				}
				if (serializedEscrow.nShipping <= theEscrow.nShipping)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Shipping total was not increased");
					return true;
				}
				if (vvchAliasArgs[0] != theEscrow.vchBuyerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4043 - " + _("Only buyer can add shipping to an escrow");
					return true;
				}
				theEscrow.nShipping = serializedEscrow.nShipping;
			}
			else if (op == OP_ESCROW_ACKNOWLEDGE)
			{
				if (theEscrow.op != OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4044 - " + _("Can only acknowledge an active escrow");
					return true;
				}
				if (vvchAliasArgs[0] != theEscrow.vchSellerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4045 - " + _("Only seller can acknowledge an escrow payment");
					return true;
				}
				theEscrow.bPaymentAck = true;

				int nQty = dbOffer.nQty;
				// if this is a linked offer we must update the linked offer qty
				if (GetOffer(dbOffer.vchOffer, myLinkOffer))
				{
					nQty = myLinkOffer.nQty;
				}
				if (nQty != -1)
				{
					if (theEscrow.nQty > nQty)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4046 - " + _("Not enough quantity left in this offer for this purchase");
						return true;
					}
				}
			}
			else if (op == OP_ESCROW_REFUND)
			{
				if (theEscrow.op != OP_ESCROW_ACTIVATE && theEscrow.op != OP_ESCROW_REFUND && theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4047 - " + _("Can only refund an active escrow");
					return true;
				}
				if (!dbOffer.vchLinkOffer.empty())
				{
					if (!GetOffer(dbOffer.vchLinkOffer, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4049 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4050 - " + _("Offer auction refund requires a witness signature but none provided");
							return true;
						}
					}
				}
				if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4051 - " + _("Offer auction refund requires a witness signature but none provided");
						return true;
					}
				}

				CAliasIndex alias;
				if (!GetAlias(theEscrow.vchSellerAlias, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4052 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.vchArbiterAlias, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4053 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}

				if (vvchAliasArgs[0] != theEscrow.vchSellerAlias && vvchAliasArgs[0] != theEscrow.vchArbiterAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4054 - " + _("Only arbiter or seller can initiate an escrow refund");
					return true;
				}
				theEscrow.role = serializedEscrow.role;
				theEscrow.op = op;
				// if this escrow was actually a series of bids, set the bid status to 'refunded' in escrow bid collection
				if (!bSanityCheck && !theEscrow.bBuyNow) {
					pescrowdb->WriteEscrowBid(dbOffer, theEscrow, "refunded");
				}
			}
			else if (op == OP_ESCROW_REFUND_COMPLETE)
			{
				if (theEscrow.op != OP_ESCROW_REFUND)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4055 - " + _("Can only claim a refunded escrow");
					return true;
				}
				if (!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				if (vvchAliasArgs[0] != theEscrow.vchBuyerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4056 - " + _("Only buyer can claim an escrow refund");
					return true;
				}
				theEscrow.op = op;
			}
			else if (op == OP_ESCROW_RELEASE)
			{
				if (theEscrow.op != OP_ESCROW_ACTIVATE && theEscrow.op != OP_ESCROW_REFUND && theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4057 - " + _("Can only release an active escrow");
					return true;
				}
				if (!dbOffer.vchLinkOffer.empty())
				{
					if (!GetOffer(dbOffer.vchLinkOffer, myLinkOffer))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4059 - " + _("Cannot find linked offer for this escrow");
						return true;
					}
					if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
					{
						if (myLinkOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4060 - " + _("Offer auction release requires a witness signature but none provided");
							return true;
						}
					}
				}
				if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (dbOffer.auctionOffer.bRequireWitness && serializedEscrow.vchWitness.empty())
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4061 - " + _("Offer auction release requires a witness signature but none provided");
						return true;
					}
				}

				CAliasIndex alias;
				if (!GetAlias(theEscrow.vchBuyerAlias, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4062 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if (!GetAlias(theEscrow.vchArbiterAlias, alias))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4063 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if (vvchAliasArgs[0] != theEscrow.vchBuyerAlias && vvchAliasArgs[0] != theEscrow.vchArbiterAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only arbiter or buyer can initiate an escrow release");
					return true;
				}
				theEscrow.role = serializedEscrow.role;
				theEscrow.op = op;
			}
			else if (op == OP_ESCROW_RELEASE_COMPLETE)
			{
				if (theEscrow.op != OP_ESCROW_RELEASE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4065 - " + _("Can only claim a released escrow");
					return true;
				}
				if (!serializedEscrow.redeemTxId.IsNull())
					theEscrow.redeemTxId = serializedEscrow.redeemTxId;
				if (vvchAliasArgs[0] != theEscrow.vchSellerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Only seller can claim an escrow release");
					return true;
				}
				theEscrow.op = op;
			}
			else if (op == OP_ESCROW_FEEDBACK)
			{
				if (theEscrow.op == OP_ESCROW_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4067 - " + _("Can not leave feedback on an active escrow");
					return true;
				}
				vector<unsigned char> vchSellerAlias = theEscrow.vchSellerAlias;
				if (!theEscrow.vchLinkSellerAlias.empty())
					vchSellerAlias = theEscrow.vchLinkSellerAlias;

				if (serializedEscrow.feedback.nFeedbackUserFrom == serializedEscrow.feedback.nFeedbackUserTo)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4068 - " + _("Cannot send yourself feedback");
					return true;
				}
				if (serializedEscrow.feedback.nRating > 5)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4069 - " + _("Invalid rating, must be less than or equal to 5 and greater than or equal to 0");
					return true;
				}
				if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKBUYER && vvchAliasArgs[0] != theEscrow.vchBuyerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4070 - " + _("Only buyer can leave this feedback");
					return true;
				}
				if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKSELLER && vvchAliasArgs[0] != vchSellerAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4071 - " + _("Only seller can leave this feedback");
					return true;
				}
				if (serializedEscrow.feedback.nFeedbackUserFrom == FEEDBACKARBITER && vvchAliasArgs[0] != theEscrow.vchArbiterAlias)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4072 - " + _("Only arbiter can leave this feedback");
					return true;
				}
				if (serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKBUYER && serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKSELLER && serializedEscrow.feedback.nFeedbackUserFrom != FEEDBACKARBITER)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4073 - " + _("Unknown feedback user type");
					return true;
				}
				serializedEscrow.txHash = tx.GetHash();
				serializedEscrow.nHeight = nHeight;
				serializedEscrow.vchOffer = theEscrow.vchOffer;
				if (!bSanityCheck) {
					pescrowdb->WriteEscrowFeedbackIndex(dbOffer, serializedEscrow);
				}
			}

		}
		else
		{
			if (vvchAliasArgs[0] != theEscrow.vchBuyerAlias)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only buyer can initiate an escrow");
				return true;
			}
			COffer myLinkOffer;
			if (fJustCheck && GetEscrow(serializedEscrow.vchEscrow, theEscrow))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Escrow already exists");
				return true;
			}
			if (theEscrow.nQty <= 0)
				theEscrow.nQty = 1;

			if (dbOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(dbOffer.sCategory), "wanted"))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4075 - " + _("Cannot purchase a wanted offer");
			}
			if (IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_AUCTION))
			{
				if (dbOffer.auctionOffer.fReservePrice > theEscrow.fBidPerUnit)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4076 - " + _("Cannot purchase below offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
					return true;
				}
				if (dbOffer.auctionOffer.nExpireTime > 0 && dbOffer.auctionOffer.nExpireTime < chainActive.Tip()->GetMedianTimePast() && !theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4077 - " + _("This auction has expired, cannot place bid");
					return true;
				}
			}
			if (!IsOfferTypeInMask(dbOffer.offerType, OFFERTYPE_BUYNOW) && theEscrow.bBuyNow)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4078 - " + _("Offer does not support the Buy It Now feature");
				return true;
			}
			if (dbOffer.nQty != -1)
			{
				if (theEscrow.nQty > dbOffer.nQty)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4079 - " + _("Not enough quantity left in this offer for this purchase");
					return true;
				}
			}
			theEscrow.vchOffer = dbOffer.vchOffer;
			theEscrow.op = op;

			if (!dbOffer.vchLinkOffer.empty())
			{
				if (!GetOffer(dbOffer.vchLinkOffer, myLinkOffer))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4081 - " + _("Cannot find linked offer for this escrow");
					return true;
				}
				if (IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_AUCTION))
				{
					if (myLinkOffer.auctionOffer.fReservePrice > theEscrow.fBidPerUnit)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4082 - " + _("Cannot purchase below linked offer reserve price of: ") + boost::lexical_cast<string>(dbOffer.auctionOffer.fReservePrice) + " " + stringFromVch(dbOffer.sCurrencyCode);
						return true;
					}
					if (myLinkOffer.auctionOffer.nExpireTime > 0 && myLinkOffer.auctionOffer.nExpireTime < chainActive.Tip()->GetMedianTimePast() && !theEscrow.bBuyNow)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4083 - " + _("This linked offer auction has expired, cannot place bid");
						return true;
					}
				}
				if (!IsOfferTypeInMask(myLinkOffer.offerType, OFFERTYPE_BUYNOW) && theEscrow.bBuyNow)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4084 - " + _("Linked offer does not support the Buy It Now feature");
					return true;
				}
			}
			if (theEscrow.nPaymentOption != PAYMENTOPTION_SYS)
			{
				bool noError = ValidateExternalPayment(theEscrow, bSanityCheck, errorMessage);
				if (!errorMessage.empty())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4085 - " + errorMessage;
					if (!noError)
						return error(errorMessage.c_str());
					else
						return true;
				}
			}
		}

		// set the escrow's txn-dependent values
		theEscrow.txHash = tx.GetHash();
		theEscrow.nHeight = nHeight;
		// write escrow
		if (!bSanityCheck) {
			if (!pescrowdb->WriteEscrow(vvchArgs, dbOffer, theEscrow))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4086 - " + _("Failed to write to escrow DB");
				return error(errorMessage.c_str());
			}
			if (fDebug)
				LogPrintf("CONNECTED ESCROW: op=%s escrow=%s hash=%s height=%d fJustCheck=%d\n",
					escrowFromOp(op).c_str(),
					stringFromVch(serializedEscrow.vchEscrow).c_str(),
					tx.GetHash().ToString().c_str(),
					nHeight,
					fJustCheck ? 1 : -1);
		}
	}
    return true;
}
UniValue escrowbid(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 5)
		throw runtime_error(
			"escrowbid [alias] [escrow] [bid_in_payment_option] [bid_in_offer_currency] [witness]\n"
			"<alias> An alias you own.\n"
			"<escrow> Escrow GUID to place bid on.\n"
			"<bid_in_payment_option> Amount to bid on offer through escrow. Bid is in payment option currency. Example: If offer is paid in SYS and you have deposited 10 SYS in escrow and would like to increase your total bid to 14 SYS enter 14 here. It is per unit of purchase.\n"
			"<bid_in_offer_currency> Converted value of bid_in_payment_option from paymentOption currency to offer currency. For example: offer is priced in USD and purchased in BTC, this field will be the BTC/USD value. It is per unit of purchase.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchEscrow = vchFromValue(params[1]);
	CAmount nBid = AmountFromValue(params[2].get_real());
	float fBid = params[3].get_real();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);
	CAliasIndex bidderalias;
	ToLowerCase(vchAlias);
	if (!GetAlias(vchAlias, bidderalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4501 - " + _("Could not find alias with this name"));
	CScript scriptPubKeyAliasOrig, scriptPubKeyAlias;
	CSyscoinAddress buyerAddress;
	GetAddress(bidderalias, &buyerAddress, scriptPubKeyAliasOrig);

	CEscrow theEscrow;

	if (!GetEscrow(vchEscrow, theEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4502 - " + _("Could not find an escrow with this identifier"));

	CWalletTx wtx;
	vector<CRecipient> vecSend;

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << bidderalias.vchAlias << bidderalias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;
	theEscrow.ClearEscrow();
	theEscrow.nAmountOrBidPerUnit = nBid;
	theEscrow.fBidPerUnit = fBid;
	vector<unsigned char> data;
	theEscrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;
	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_BID) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigBuyer += scriptPubKeyAliasOrig;

	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	return syscointxfund_helper(vchAlias, vchWitness, aliasRecipient, vecSend);
}
UniValue escrowaddshipping(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 3)
		throw runtime_error(
			"escrowaddshipping [escrow] [shipping amount] [witness]\n"
			"<escrow> Escrow GUID to add shipping to.\n"
			"<shipping amount> Amount to add to shipping for merchant. Amount is in payment option currency. Example: If merchant requests 0.1 BTC for shipping and escrow is paid in BTC, enter 0.1 here.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());

	vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	CAmount nShipping = AmountFromValue(params[1].get_real());

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	CScript scriptPubKeyAliasOrig, scriptPubKeyAlias;
	CEscrow theEscrow;

	if (!GetEscrow(vchEscrow, theEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4503 - " + _("Could not find an escrow with this identifier"));

	CAliasIndex bidderalias;
	if (!GetAlias(theEscrow.vchBuyerAlias, bidderalias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4504 - " + _("Could not find alias with this name"));

	CScriptID innerID(CScript(theEscrow.vchRedeemScript.begin(), theEscrow.vchRedeemScript.end()));
	CSyscoinAddress address(innerID);
	CScript scriptPubKey = GetScriptForDestination(address.Get());

	CWalletTx wtx;
	CRecipient recipientEscrow = { scriptPubKey, nShipping, false };
	vector<CRecipient> vecSend;
	if (theEscrow.extTxId.IsNull())
		vecSend.push_back(recipientEscrow);

	CSyscoinAddress buyerAddress;
	GetAddress(bidderalias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << bidderalias.vchAlias << bidderalias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;

	theEscrow.ClearEscrow();
	theEscrow.nShipping += nShipping;
	vector<unsigned char> data;
	theEscrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;
	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_ADD_SHIPPING) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigBuyer += scriptPubKeyAliasOrig;


	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	return syscointxfund_helper(bidderalias.vchAlias, vchWitness, aliasRecipient, vecSend);
}
UniValue escrownew(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 16)
        throw runtime_error(
			"escrownew [getamountandaddress] [alias] [arbiter alias] [offer] [quantity] [buynow] [price_per_unit_in_payment_option] [shipping_amount] [network_fee] [arbiter_fee] [witness_fee] [extTx] [payment_option] [bid_in_payment_option] [bid_in_offer_currency] [witness]\n"
				"<getamountandaddress> True or false. Get deposit and total escrow amount aswell as escrow address for funding. If buynow is false pass bid amount in bid_in_payment_option to get total needed to complete escrow. If buynow is true amount is calculated based on offer price and quantity.\n"
				"<alias> An alias you own.\n"
				"<arbiter alias> Alias of Arbiter.\n"
                "<offer> GUID of offer that this escrow is managing.\n"
                "<quantity> Quantity of items to buy of offer.\n"
				"<buynow> Specify whether the escrow involves purchasing offer for the full offer price if set to true, or through a bidding auction if set to false. If buynow is false, an initial deposit may be used to secure a bid if required by the seller.\n"
				"<price_per_unit_in_payment_option> Total amount of the offer price. Amount is in paymentOption currency. It is per unit of purchase. \n"
				"<shipping amount> Amount to add to shipping for merchant. Amount is in paymentOption currency. Example: If merchant requests 0.1 BTC for shipping and escrow is paid in BTC, enter 0.1 here. Default is 0. Buyer can also add shipping using escrowaddshipping upon merchant request.\n"
				"<network fee> Network fee in satoshi per byte for the transaction. Generally the escrow transaction is about 400 bytes. Default is 25 for SYS or ZEC and 250 for BTC payments.\n"
				"<arbiter fee> Arbiter fee in fractional amount of the amount_in_payment_option value. For example 0.75% is 0.0075 and represents 0.0075*amount_in_payment_option satoshis paid to arbiter in the event arbiter is used to resolve a dispute. Default and minimum is 0.005.\n"
				"<witness fee> Witness fee in fractional amount of the amount_in_payment_option value. For example 0.3% is 0.003 and represents 0.003*amount_in_payment_option satoshis paid to witness in the event witness signs off on an escrow through any of the following calls escrownew/escrowbid/escrowrelease/escrowrefund. Default is 0.\n"
				"<extTx> External transaction ID if paid with another blockchain.\n"
				"<paymentOption> If extTx is defined, specify a valid payment option used to make payment. Default is SYS.\n"
				"<bid_in_payment_option> Initial bid amount you are willing to pay escrow for this offer. Amount is in paymentOption currency. It is per unit of purchase. If buynow is set to true, this value is disregarded.\n"
				"<bid_in_offer_currency> Converted value of bid_in_payment_option from paymentOption currency to offer currency. For example: offer is priced in USD and purchased in BTC, this field will be the BTC/USD value. If buynow is set to true, this value is disregarded.\n"
                "<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
				+ HelpRequiringPassphrase());
	bool bGetAmountAndAddress = params[0].get_bool();
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	string strArbiter = params[2].get_str();
	vector<unsigned char> vchOffer = vchFromValue(params[3]);
	unsigned int nQty = 1;

	nQty = params[4].get_int();
	bool bBuyNow = params[5].get_bool();
	CAmount nPricePerUnit = AmountFromValue(params[6].get_real());
	
	boost::algorithm::to_lower(strArbiter);
	// check for alias existence in DB
	CAliasIndex arbiteralias;
	if (!GetAlias(vchFromString(strArbiter), arbiteralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4505 - " + _("Failed to read arbiter alias from DB"));

	CAmount nShipping = 0;
	nShipping = AmountFromValue(params[7].get_real());
	

	int nNetworkFee = 0;
	float fEscrowFee = getEscrowFee();

	float fWitnessFee = 0;
	nNetworkFee = params[8].get_int();
	
	fEscrowFee = params[9].get_real();
	fWitnessFee = params[10].get_real();

	string extTxIdStr = "";
	extTxIdStr = params[11].get_str();

	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOption = "SYS";
	paymentOption = params[12].get_str();
	boost::algorithm::to_upper(paymentOption);
	
	// payment options - validate payment options string
	if (!ValidatePaymentOptionsString(paymentOption))
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4506 - " + _("Could not validate the payment option value");
		throw runtime_error(err.c_str());
	}
	// payment options - and convert payment options string to a bitmask for the txn
	uint64_t paymentOptionMask = GetPaymentOptionsMaskFromString(paymentOption);
	if (nNetworkFee <= 0)
		nNetworkFee = getFeePerByte(paymentOptionMask);

	CAmount nBidPerUnit = 0;
	nBidPerUnit = AmountFromValue(params[13].get_real());
	
	float fBidPerUnit = 0;
	fBidPerUnit = params[14].get_real();

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[15]);

	CAliasIndex buyeralias;
	ToLowerCase(vchAlias);
	if (!GetAlias(vchAlias, buyeralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4507 - " + _("Could not find buyer alias with this name"));
	

	COffer theOffer, linkedOffer;

	if (!GetOffer( vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4508 - " + _("Could not find offer with this identifier"));
	float fDepositPercentage = theOffer.auctionOffer.fDepositPercentage;
	CAliasIndex selleralias;
	if (!GetAlias( theOffer.vchAlias, selleralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Could not find seller alias with this identifier"));

	if(theOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(theOffer.sCategory), "wanted"))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4510 - " + _("Cannot purchase a wanted offer"));


	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	CAliasIndex theLinkedAlias, reselleralias;
	
	CAmount nCommission = 0;
	if(!theOffer.vchLinkOffer.empty())
	{
		if (!GetOffer( theOffer.vchLinkOffer, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4511 - " + _("Trying to accept a linked offer but could not find parent offer"));

		if (!GetAlias( linkedOffer.vchAlias, theLinkedAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find an alias with this identifier"));

		if(linkedOffer.sCategory.size() > 0 && boost::algorithm::istarts_with(stringFromVch(linkedOffer.sCategory), "wanted"))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Cannot purchase a wanted offer"));

		reselleralias = selleralias;
		selleralias = theLinkedAlias;
		fDepositPercentage = linkedOffer.auctionOffer.fDepositPercentage;
		COfferLinkWhitelistEntry foundEntry;
		theLinkedAlias.offerWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);
		int discount = foundEntry.nDiscountPct;
		int commission = theOffer.nCommission;
		int markup = discount + commission;
		if(markup > 0)
			nCommission = nPricePerUnit*(markup/100);
	}

	


	CSyscoinAddress buyerAddress;
	GetAddress(buyeralias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyeralias.vchAlias  << buyeralias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
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
		JSONRPCRequest request1;
		request1.params = arrayParams;
		resCreate = createmultisig(request1);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4514 - " + _("Could not create escrow transaction: Invalid response from createescrow"));

	const UniValue &o = resCreate.get_obj();
	const UniValue& redeemScript_value = find_value(o, "redeemScript");
	if (redeemScript_value.isStr())
	{
		std::vector<unsigned char> data(ParseHex(redeemScript_value.get_str()));
		redeemScript = CScript(data.begin(), data.end());
	}
	else
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4515 - " + _("Could not create escrow transaction: could not find redeem script in response"));

	CScriptID innerID(redeemScript);

	const UniValue& address_value = find_value(o, "address");
	strAddress = address_value.get_str();
	
	CSyscoinAddress address(strAddress);
	scriptPubKey = GetScriptForDestination(address.Get());
	CAmount nTotalOfferPrice = nPricePerUnit*nQty;
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
	newEscrow.vchEscrow = vchEscrow;
	newEscrow.vchBuyerAlias = buyeralias.vchAlias;
	newEscrow.vchArbiterAlias = arbiteralias.vchAlias;
	newEscrow.vchOffer = theOffer.vchOffer;
	newEscrow.extTxId = uint256S(extTxIdStr);
	newEscrow.vchSellerAlias = selleralias.vchAlias;
	newEscrow.vchLinkSellerAlias = reselleralias.vchAlias;
	newEscrow.nQty = nQty;
	newEscrow.nPaymentOption = paymentOptionMask;
	newEscrow.nCommission = nCommission;
	newEscrow.nNetworkFee = nNetworkFee;
	newEscrow.nArbiterFee = nEscrowFee;
	newEscrow.nWitnessFee = nWitnessFee;
	newEscrow.vchRedeemScript = ParseHex(redeemScript_value.get_str());
	newEscrow.nDeposit = nDepositFee;
	newEscrow.nShipping = nShipping;
	newEscrow.nAmountOrBidPerUnit = bBuyNow? nPricePerUnit: nBidPerUnit;
	newEscrow.fBidPerUnit = fBidPerUnit;
	newEscrow.bBuyNow = bBuyNow;
	vector<unsigned char> data;
	newEscrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

	if (nAmountWithFee != (nTotalOfferPrice + nFees))
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Mismatch when calculating total amount with fees"));
	}
	if (newEscrow.bBuyNow && newEscrow.nDeposit > 0) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4517 - " + _("Cannot include deposit when using Buy It Now"));
	}

    vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyBuyer += scriptPubKeyAliasOrig;


	// send the tranasction
	
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	
	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);


	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	UniValue res = syscointxfund_helper(buyeralias.vchAlias, vchWitness, aliasRecipient, vecSend);
	res.push_back(stringFromVch(vchEscrow));
	return res;
}
UniValue escrowacknowledge(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 2)
		throw runtime_error(
			"escrowacknowledge [escrow guid] [witness]\n"
			"Acknowledge escrow payment as seller of offer.\n"
			"<instantsend> Set to true to use InstantSend to send this transaction or false otherwise.\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[1]);

	// this is a syscoin transaction
	CWalletTx wtx;
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4518 - " + _("Could not find an escrow with this key"));

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress, resellerPaymentAddress;
	CScript arbiterScript;
	if (GetAlias(escrow.vchArbiterAlias, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	if (GetAlias(escrow.vchBuyerAlias, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	if (GetAlias(escrow.vchSellerAlias, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}




	CScript scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += sellerScript;

	escrow.ClearEscrow();

	vector<unsigned char> data;
	escrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());

	CScript scriptPubKeyOrigBuyer;

	scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_ACKNOWLEDGE) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigBuyer += buyerScript;

	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	return syscointxfund_helper(sellerAliasLatest.vchAlias, vchWitness, aliasRecipient, vecSend);

}
UniValue escrowcreaterawtransaction(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 4)
		throw runtime_error(
			"escrowcreaterawtransaction [type] [escrow guid] [{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...] [user role]\n"
			"Creates raw transaction for escrow refund or release, sign the output raw transaction and pass it via the rawtx parameter to escrowrelease. Type is 'refund' or 'release'. Third parameter is array of input (txid, vout, amount) pairs to be used to fund escrow payment. User role represents either 'seller', 'buyer' or 'arbiter', represents who signed for the payment of the escrow. 'seller' or 'arbiter' is valid for type 'refund' (if you are the buyer during refund leave this empty), while 'buyer' or 'arbiter' is valid for type 'release' (if you are the seller during release leave this empty). You only need to provide this parameter when calling escrowrelease or escrowrefund. \n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	string type = params[0].get_str();
	vector<unsigned char> vchEscrow = vchFromValue(params[1]);
	const UniValue &inputs = params[2].get_array();
	string role = "";
	role = params[3].get_str();

	// this is a syscoin transaction
	CWalletTx wtx;

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4519 - " + _("Could not find an escrow with this key"));
	COffer theOffer, linkedOffer;
	if (!GetOffer(escrow.vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4520 - " + _("Could not find offer related to this escrow"));
	float fDepositPercentage = theOffer.auctionOffer.fDepositPercentage;
	if (!theOffer.vchLinkOffer.empty())
	{

		if (!GetOffer(theOffer.vchLinkOffer, linkedOffer))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4521 - " + _("Trying to accept a linked offer but could not find parent offer"));
		fDepositPercentage = linkedOffer.auctionOffer.fDepositPercentage;
	}

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest, witnessAliasLatest;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress, resellerPaymentAddress, witnessAddressPayment;
	CScript arbiterScript;
	if (GetAlias(escrow.vchArbiterAlias, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	if (GetAlias(escrow.vchBuyerAlias, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	if (GetAlias(escrow.vchSellerAlias, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}
	CScript resellerScript;
	if (GetAlias(escrow.vchLinkSellerAlias, resellerAliasLatest))
	{
		GetAddress(resellerAliasLatest, &resellerPaymentAddress, resellerScript, escrow.nPaymentOption);
	}
	if (GetAlias(escrow.vchWitness, witnessAliasLatest))
	{
		CScript script;
		GetAddress(witnessAliasLatest, &witnessAddressPayment, script, escrow.nPaymentOption);
	}
	CScript scriptPubKeyAlias;
	CAmount nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	CAmount nBalance = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		const UniValue& inputsObj = inputs[i].get_obj();
		nBalance += find_value(inputsObj, "satoshis").get_int64();
	}
	CAmount nTotalWithFee = (escrow.nAmountOrBidPerUnit*escrow.nQty) + nEscrowFees;
	CAmount nBalanceTmp = nBalance;
	// subtract total from the amount found in the address, if this is negative the UI should complain that not enough funds were found and that more funds are required
	nBalance -= nTotalWithFee;

	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createAddressUniValue(UniValue::VOBJ);
	if (type == "refund") {
		nBalanceTmp -= escrow.nNetworkFee;
		if (escrow.role == EscrowRoles::ARBITER || role == "arbiter")
		{
			nBalanceTmp -= escrow.nArbiterFee;
			createAddressUniValue.push_back(Pair(arbiterPaymentAddress.ToString(), ValueFromAmount(escrow.nArbiterFee)));
		}
		createAddressUniValue.push_back(Pair(buyerPaymentAddress.ToString(), ValueFromAmount(nBalanceTmp)));
	}
	else if (type == "release") {
		nBalanceTmp -= escrow.nNetworkFee;
		nBalanceTmp -= escrow.nArbiterFee;
		nBalanceTmp -= escrow.nDeposit;
		nBalanceTmp -= escrow.nWitnessFee;
		// if linked offer send commission to affiliate
		if (!theOffer.vchLinkOffer.empty())
		{
			nBalanceTmp -= escrow.nCommission;
			if (escrow.nCommission > 0)
				createAddressUniValue.push_back(Pair(resellerPaymentAddress.ToString(), ValueFromAmount(escrow.nCommission)));
		}
		if (escrow.role == EscrowRoles::ARBITER || role == "arbiter")
		{
			createAddressUniValue.push_back(Pair(arbiterPaymentAddress.ToString(), ValueFromAmount(escrow.nArbiterFee)));
			if (escrow.nDeposit > 0)
				createAddressUniValue.push_back(Pair(buyerPaymentAddress.ToString(), ValueFromAmount(escrow.nDeposit)));
		}
		else if (escrow.role == EscrowRoles::BUYER || role == "buyer")
		{
			createAddressUniValue.push_back(Pair(buyerPaymentAddress.ToString(), ValueFromAmount(escrow.nArbiterFee + escrow.nDeposit)));
		}
		if (escrow.nWitnessFee > 0)
			createAddressUniValue.push_back(Pair(witnessAddressPayment.ToString(), ValueFromAmount(escrow.nWitnessFee)));
		createAddressUniValue.push_back(Pair(sellerPaymentAddress.ToString(), ValueFromAmount(nBalanceTmp)));

	}
	if (!ValidateArbiterFee(escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4522 - " + _("Could not validate arbiter fee in escrow"));
	}
	if (!ValidateDepositFee(fDepositPercentage, escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4523 - " + _("Could not validate deposit in escrow"));
	}
	if (!ValidateNetworkFee(escrow)) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not validate network fee in escrow"));
	}
	if (escrow.bBuyNow && escrow.nDeposit > 0) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4525 - " + _("Cannot include deposit when using Buy It Now"));
	}
	arrayCreateParams.push_back(inputs);
	arrayCreateParams.push_back(createAddressUniValue);
	UniValue resCreate;
	try
	{
		JSONRPCRequest request1;
		request1.params = arrayCreateParams;
		resCreate = createrawtransaction(request1);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resCreate.isStr())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4526 - " + _("Could not create escrow transaction: Invalid response from createrawtransaction"));

	string createEscrowSpendingTx = resCreate.get_str();
	string strRawTx = createEscrowSpendingTx;
	// if this is called prior to escrowcompleterelease, then it probably has been signed already, so apply the existing inputs signatures to the escrow creation transaction
	// and pass the new raw transaction to the next person to sign and call the escrowcompleterelease with the final raw tx.
	if (!escrow.scriptSigs.empty()) {
		CMutableTransaction rawTxm;
		DecodeHexTx(rawTxm, createEscrowSpendingTx);
		for (int i = 0; i < escrow.scriptSigs.size(); i++) {
			if (rawTxm.vin.size() >= i)
				rawTxm.vin[i].scriptSig = CScript(escrow.scriptSigs[i].begin(), escrow.scriptSigs[i].end());
		}
		CTransaction rawTx(rawTxm);
		strRawTx = EncodeHexTx(rawTxm);
	}

	UniValue res(UniValue::VARR);
	res.push_back(strRawTx);
	res.push_back(ValueFromAmount(nBalance));
	return res;
}
UniValue escrowrelease(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 4)
        throw runtime_error(
			"escrowrelease [escrow guid] [user role] [rawtx] [witness]\n"
			"Releases escrow funds to seller. User role represents either 'buyer' or 'arbiter'. Third parameter (rawtx) is the signed response from escrowcreaterawtransaction. You must sign this transaction externally prior to passing in.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	string rawtx = params[2].get_str();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[3]);
    // this is a syscoin transaction
    CWalletTx wtx;

	CEscrow escrow;
    if (!GetEscrow( vchEscrow,escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4527 - " + _("Could not find an escrow with this key"));
	COffer theOffer;
	if (!GetOffer(escrow.vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4528 - " + _("Could not find offer related to this escrow"));

	CAliasIndex buyerAliasLatest, arbiterAliasLatest;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment;
	CScript arbiterScript;
	if (GetAlias(escrow.vchArbiterAlias, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}
	CScript buyerScript;
	if (GetAlias(escrow.vchBuyerAlias, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}

	CScript scriptPubKeyAlias;
	CAliasIndex theAlias;

	// who is initiating release arbiter or buyer?
	if(role == "arbiter")
	{
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += arbiterScript;
		theAlias = arbiterAliasLatest;
	}
	else if(role == "buyer")
	{
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += buyerScript;
		theAlias = buyerAliasLatest;
	}
	else
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Invalid role"));

	CMutableTransaction signedTx;
	DecodeHexTx(signedTx, rawtx);
	escrow.ClearEscrow();
	for (int i = 0; i < signedTx.vin.size(); i++) {
		if(!signedTx.vin[i].scriptSig.empty())
			escrow.scriptSigs.push_back((CScriptBase)signedTx.vin[i].scriptSig);
	}
	escrow.bPaymentAck = false;
	vector<unsigned char> data;
	unsigned char escrowrole = 0;
	if (role == "arbiter")
		escrowrole = EscrowRoles::ARBITER;
	else if(role == "buyer")
		escrowrole = EscrowRoles::BUYER;
	escrow.role = escrowrole;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());

    CScript scriptPubKeyOrigSeller;

    scriptPubKeyOrigSeller << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchHashEscrow << OP_2DROP << OP_DROP;
    scriptPubKeyOrigSeller += buyerScript;


	vector<CRecipient> vecSend;
	CRecipient recipientSeller;
	CreateRecipient(scriptPubKeyOrigSeller, recipientSeller);
	vecSend.push_back(recipientSeller);

	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(theAlias.vchAlias, vchWitness, aliasRecipient, vecSend);
}

UniValue escrowcompleterelease(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
			"escrowcompleterelease [escrow guid] [rawtx] [witness]\n"
                         "Completes an escrow release by creating the escrow complete release transaction on syscoin blockchain.\n"
						 "<rawtx> Raw fully signed syscoin escrow transaction. It is the signed response from escrowcreaterawtransaction. You must sign this transaction externally prior to passing in.\n"
                         "<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						 + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CMutableTransaction myRawTx;
	DecodeHexTx(myRawTx,rawTx);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
    // this is a syscoin transaction
    CWalletTx wtx;

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4530 - " + _("Could not find an escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex sellerAliasLatest;
	CSyscoinAddress sellerPaymentAddress;

	CScript sellerScript;
	if (GetAlias(escrow.vchSellerAlias, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}


	CScript scriptPubKeyAlias;
	
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += sellerScript;

	escrow.ClearEscrow();
	escrow.bPaymentAck = false;
	escrow.redeemTxId = myRawTx.GetHash();
    CScript scriptPubKeyBuyer, scriptPubKeySeller;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());
    scriptPubKeyBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_RELEASE_COMPLETE) << vchHashEscrow << OP_2DROP << OP_DROP;
    scriptPubKeyBuyer += sellerScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientArbiter;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);



	const UniValue res = syscointxfund_helper(sellerAliasLatest.vchAlias, vchWitness, aliasRecipient, vecSend);
	UniValue returnRes;
	UniValue sendParams(UniValue::VARR);
	sendParams.push_back(rawTx);
	try
	{
		// broadcast the payment transaction to syscoin network if not external transaction
		if (!extPayment) {
			JSONRPCRequest request1;
			request1.params = sendParams;
			returnRes = sendrawtransaction(request1);
		}
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	return res;
}
UniValue escrowrefund(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 4)
		throw runtime_error(
			"escrowrefund [escrow guid] [user role] [rawtx] [witness]\n"
			"Refunds escrow funds to buyer. User role represents either 'seller' or 'arbiter'. Third parameter (rawtx) is the signed response from escrowcreaterawtransaction. You must sign this transaction externally prior to passing in.\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	string rawtx = params[2].get_str();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[3]);
	// this is a syscoin transaction
	CWalletTx wtx;

	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4531 - " + _("Could not find an escrow with this key"));
	COffer theOffer;
	if (!GetOffer(escrow.vchOffer, theOffer))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4532 - " + _("Could not find offer related to this escrow"));

	CAliasIndex sellerAliasLatest, arbiterAliasLatest;
	CSyscoinAddress arbiterAddressPayment, sellerAddressPayment;
	CScript arbiterScript;
	if (GetAlias(escrow.vchArbiterAlias, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}
	CScript sellerScript;
	if (GetAlias(escrow.vchSellerAlias, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}

	CScript scriptPubKeyAlias;
	CAliasIndex theAlias;

	// who is initiating refund arbiter or seller?
	if (role == "arbiter")
	{
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += arbiterScript;
		theAlias = arbiterAliasLatest;
	}
	else if (role == "seller")
	{
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += sellerScript;
		theAlias = sellerAliasLatest;
	}
	else
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4533 - " + _("Invalid role"));

	CMutableTransaction signedTx;
	DecodeHexTx(signedTx, rawtx);
	escrow.ClearEscrow();
	for (int i = 0; i < signedTx.vin.size(); i++) {
		if (!signedTx.vin[i].scriptSig.empty())
			escrow.scriptSigs.push_back((CScriptBase)signedTx.vin[i].scriptSig);
	}
	escrow.bPaymentAck = false;
	unsigned char escrowrole = 0;
	if (role == "arbiter")
		escrowrole = EscrowRoles::ARBITER;
	else if (role == "seller")
		escrowrole = EscrowRoles::SELLER;
	escrow.role = escrowrole;
	vector<unsigned char> data;
	escrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());

	CScript scriptPubKeyOrigSeller;

	scriptPubKeyOrigSeller << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyOrigSeller += sellerScript;


	vector<CRecipient> vecSend;
	CRecipient recipientSeller;
	CreateRecipient(scriptPubKeyOrigSeller, recipientSeller);
	vecSend.push_back(recipientSeller);

	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(theAlias.vchAlias, vchWitness, aliasRecipient, vecSend);
}

UniValue escrowcompleterefund(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 3)
		throw runtime_error(
			"escrowcompleterefund [escrow guid] [rawtx] [witness]\n"
			"Completes an escrow refund by creating the escrow complete refund transaction on syscoin blockchain.\n"
			"<rawtx> Raw fully signed syscoin escrow transaction. It is the signed response from escrowcreaterawtransaction. You must sign this transaction externally prior to passing in.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CMutableTransaction myRawTx;
	DecodeHexTx(myRawTx, rawTx);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	// this is a syscoin transaction
	CWalletTx wtx;



	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4534 - " + _("Could not find an escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex buyerAliasLatest;
	CSyscoinAddress buyerPaymentAddress;

	CScript buyerScript;
	if (GetAlias(escrow.vchBuyerAlias, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript scriptPubKeyAlias;

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += buyerScript;



	escrow.ClearEscrow();
	escrow.bPaymentAck = false;
	escrow.redeemTxId = myRawTx.GetHash();
	CScript scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyArbiter;

	vector<unsigned char> data;
	escrow.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_REFUND_COMPLETE) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyBuyer += buyerScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);


	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	const UniValue& res = syscointxfund_helper(buyerAliasLatest.vchAlias, vchWitness, aliasRecipient, vecSend);
	UniValue returnRes;
	UniValue sendParams(UniValue::VARR);
	sendParams.push_back(rawTx);
	try
	{
		// broadcast the payment transaction to syscoin network if not external transaction
		if (!extPayment) {
			JSONRPCRequest request1;
			request1.params = sendParams;
			returnRes = sendrawtransaction(request1);
		}
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	return res;
}
UniValue escrowfeedback(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 6)
        throw runtime_error(
			"escrowfeedback [escrow guid] [userfrom] [feedback] [rating] [userto] [witness]\n"
                        "Send feedback for primary and secondary users in escrow, depending on who you are. Ratings are numbers from 1 to 5. User From and User To is either 'buyer', 'seller', 'reseller', or 'arbiter'.\n"
                        + HelpRequiringPassphrase());
   // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string userfrom = params[1].get_str();
	int nRating = 0;
	vector<unsigned char> vchFeedback;
	vchFeedback = vchFromValue(params[2]);
	nRating = params[3].get_int();
	string userto = params[4].get_str();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[5]);
    // this is a syscoin transaction
    CWalletTx wtx;
	CEscrow escrow;
	if (!GetEscrow(vchEscrow, escrow))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4535 - " + _("Could not find an escrow with this key"));

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest;
	CSyscoinAddress arbiterPaymentAddress, buyerPaymentAddress, sellerPaymentAddress, resellerPaymentAddress;
	CScript arbiterScript;
	if (GetAlias(escrow.vchArbiterAlias, arbiterAliasLatest))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript, escrow.nPaymentOption);
	}

	CScript buyerScript;
	if (GetAlias(escrow.vchBuyerAlias, buyerAliasLatest))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript, escrow.nPaymentOption);
	}

	CScript sellerScript;
	if (GetAlias(escrow.vchSellerAlias, sellerAliasLatest))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript, escrow.nPaymentOption);
	}
	CScript resellerScript;
	if (GetAlias(escrow.vchLinkSellerAlias, resellerAliasLatest))
	{
		GetAddress(resellerAliasLatest, &resellerPaymentAddress, resellerScript, escrow.nPaymentOption);
	}

	CAliasIndex theAlias;
	CScript scriptPubKeyAlias;

	if(userfrom == "buyer")
	{
			
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += buyerScript;
		theAlias = buyerAliasLatest;
	}
	else if(userfrom == "seller")
	{	
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += sellerScript;
		theAlias = sellerAliasLatest;
	}
	else if(userfrom == "reseller")
	{
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << resellerAliasLatest.vchAlias << resellerAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += resellerScript;
		theAlias = resellerAliasLatest;
	}
	else if(userfrom == "arbiter")
	{		
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += arbiterScript;
		theAlias = arbiterAliasLatest;
	}
	escrow.ClearEscrow();
	escrow.vchEscrow = vchEscrow;
	escrow.bPaymentAck = false;
	// buyer
	CFeedback feedback;
	feedback.nRating = nRating;
	feedback.vchFeedback = vchFeedback;
	if(userfrom == "buyer")
	{
		feedback.nFeedbackUserFrom = FEEDBACKBUYER;
		if (userto == "seller")
			feedback.nFeedbackUserTo = FEEDBACKSELLER;
		else if (userto == "arbiter")
			feedback.nFeedbackUserTo = FEEDBACKARBITER;
		
	}
	// seller
	else if(userfrom == "seller")
	{
		feedback.nFeedbackUserFrom = FEEDBACKSELLER;
		if (userto == "buyer")
			feedback.nFeedbackUserTo = FEEDBACKBUYER;
		else if (userto == "arbiter")
			feedback.nFeedbackUserTo = FEEDBACKARBITER;

	}
	else if(userfrom == "reseller")
	{
		feedback.nFeedbackUserFrom = FEEDBACKBUYER;
		if (userto == "buyer")
			feedback.nFeedbackUserTo = FEEDBACKBUYER;
		else if (userto == "arbiter")
			feedback.nFeedbackUserTo = FEEDBACKARBITER;

	}
	// arbiter
	else if(userfrom == "arbiter")
	{
		feedback.nFeedbackUserFrom = FEEDBACKARBITER;
		if (userto == "buyer")
			feedback.nFeedbackUserTo = FEEDBACKBUYER;
		else if (userto == "seller")
			feedback.nFeedbackUserTo = FEEDBACKSELLER;

	}
	else
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4536 - " + _("You must be either the arbiter, buyer or seller to leave feedback on this escrow"));
	}
	escrow.feedback = feedback;
	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromString(hash.GetHex());
	CScript scriptPubKeyBuyer, scriptPubKeySeller,scriptPubKeyArbiter;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientSeller, recipientArbiter;
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_FEEDBACK) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyBuyer += buyerScript;
	scriptPubKeyArbiter << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_FEEDBACK) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeyArbiter += arbiterScript;
	scriptPubKeySeller << CScript::EncodeOP_N(OP_SYSCOIN_ESCROW) << CScript::EncodeOP_N(OP_ESCROW_FEEDBACK) << vchHashEscrow << OP_2DROP << OP_DROP;
	scriptPubKeySeller += sellerScript;
	CreateRecipient(scriptPubKeySeller, recipientSeller);
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	CreateRecipient(scriptPubKeyArbiter, recipientArbiter);
	// buyer
	if(userfrom == "buyer")
	{
		vecSend.push_back(recipientBuyer);
	}
	// seller
	else if(userfrom == "seller" || userfrom == "reseller")
	{
		vecSend.push_back(recipientSeller);
	}
	// arbiter
	else if(userfrom == "arbiter")
	{
		vecSend.push_back(recipientArbiter);
	}
	CRecipient aliasRecipient;
	CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	return syscointxfund_helper(theAlias.vchAlias, vchWitness, aliasRecipient, vecSend);
}
UniValue escrowinfo(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error("escrowinfo <guid>\n"
                "Show stored values of a single escrow\n");

    vector<unsigned char> vchEscrow = vchFromValue(params[0]);

    UniValue oEscrow(UniValue::VOBJ);
	CEscrow txPos;
	if (!pescrowdb || !pescrowdb->ReadEscrow(vchEscrow, txPos))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4537 - " + _("Failed to read from escrow DB"));

	if(!BuildEscrowJson(txPos, oEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4538 - " + _("Could not find this escrow"));
    return oEscrow;
}
void BuildFeedbackJson(const COffer& offer, const CEscrow& escrow, UniValue& oFeedback) {
	string sFeedbackTime;
	if (escrow.feedback.IsNull())
		return;
	if (chainActive.Height() >= escrow.nHeight-1) {
		CBlockIndex *pindex = chainActive[escrow.nHeight-1];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->GetMedianTimePast());
		}
	}
	const string &id = stringFromVch(escrow.vchEscrow) + CFeedback::FeedbackUserToString(escrow.feedback.nFeedbackUserFrom) + CFeedback::FeedbackUserToString(escrow.feedback.nFeedbackUserTo);
	oFeedback.push_back(Pair("_id", id));
	oFeedback.push_back(Pair("offer", stringFromVch(escrow.vchOffer)));
	oFeedback.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));
	oFeedback.push_back(Pair("txid", escrow.txHash.GetHex()));
	oFeedback.push_back(Pair("time", sFeedbackTime));
	oFeedback.push_back(Pair("rating", escrow.feedback.nRating));
	oFeedback.push_back(Pair("feedbackuserfrom", escrow.feedback.nFeedbackUserFrom));
	oFeedback.push_back(Pair("feedbackuserto", escrow.feedback.nFeedbackUserTo));
	oFeedback.push_back(Pair("feedback", stringFromVch(escrow.feedback.vchFeedback)));
}
void BuildEscrowBidJson(const COffer& offer, const CEscrow& escrow, const string& status, UniValue& oBid) {
	oBid.push_back(Pair("_id", escrow.txHash.GetHex()));
	int64_t nTime = 0;
	if (chainActive.Height() >= escrow.nHeight-1) {
		CBlockIndex *pindex = chainActive[escrow.nHeight-1];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oBid.push_back(Pair("time", nTime));
	oBid.push_back(Pair("offer", stringFromVch(escrow.vchOffer)));
	oBid.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));
	oBid.push_back(Pair("height", (int)escrow.nHeight));
	oBid.push_back(Pair("bidder", stringFromVch(escrow.vchBuyerAlias)));
	oBid.push_back(Pair("bid_in_offer_currency_per_unit", escrow.fBidPerUnit));
	oBid.push_back(Pair("bid_in_payment_option_per_unit", ValueFromAmount(escrow.nAmountOrBidPerUnit)));
	oBid.push_back(Pair("witness", stringFromVch(escrow.vchWitness)));
	oBid.push_back(Pair("status", status));
}
bool BuildEscrowJson(const CEscrow &escrow, UniValue& oEscrow)
{
	COffer theOffer;
	if (!GetOffer(escrow.vchOffer, theOffer))
		return false;
    oEscrow.push_back(Pair("_id", stringFromVch(escrow.vchEscrow)));
	int64_t nTime = 0;
	if (chainActive.Height() >= escrow.nHeight-1) {
		CBlockIndex *pindex = chainActive[escrow.nHeight-1];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oEscrow.push_back(Pair("time", nTime));
	oEscrow.push_back(Pair("seller", stringFromVch(escrow.vchSellerAlias)));
	oEscrow.push_back(Pair("arbiter", stringFromVch(escrow.vchArbiterAlias)));
	oEscrow.push_back(Pair("buyer", stringFromVch(escrow.vchBuyerAlias)));
	oEscrow.push_back(Pair("witness", stringFromVch(escrow.vchWitness)));
	oEscrow.push_back(Pair("offer", stringFromVch(escrow.vchOffer)));
	oEscrow.push_back(Pair("offer_price", theOffer.GetPrice()));
	oEscrow.push_back(Pair("offer_title", stringFromVch(theOffer.sTitle)));
	oEscrow.push_back(Pair("reseller", stringFromVch(escrow.vchLinkSellerAlias)));
	oEscrow.push_back(Pair("quantity", (int)escrow.nQty));
	const CAmount &nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	const CAmount &nTotalWithoutFee = escrow.nAmountOrBidPerUnit*escrow.nQty;
	const CAmount &nTotalWithFee = nTotalWithoutFee + nEscrowFees;
	oEscrow.push_back(Pair("total_with_fee", ValueFromAmount(nTotalWithFee)));
	oEscrow.push_back(Pair("total_without_fee", ValueFromAmount(nTotalWithoutFee)));
	oEscrow.push_back(Pair("bid_in_offer_currency_per_unit", escrow.fBidPerUnit));
	oEscrow.push_back(Pair("total_or_bid_in_payment_option_per_unit", ValueFromAmount(escrow.nAmountOrBidPerUnit)));
	oEscrow.push_back(Pair("buynow", escrow.bBuyNow));
	oEscrow.push_back(Pair("commission", ValueFromAmount(escrow.nCommission)));
	oEscrow.push_back(Pair("arbiterfee", ValueFromAmount(escrow.nArbiterFee)));
	oEscrow.push_back(Pair("networkfee", ValueFromAmount(escrow.nNetworkFee)));
	oEscrow.push_back(Pair("witnessfee", ValueFromAmount(escrow.nWitnessFee)));
	oEscrow.push_back(Pair("shipping", ValueFromAmount(escrow.nShipping)));
	oEscrow.push_back(Pair("deposit", ValueFromAmount(escrow.nDeposit)));
	oEscrow.push_back(Pair("currency", IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_COIN) ? GetPaymentOptionsString(escrow.nPaymentOption): stringFromVch(theOffer.sCurrencyCode)));
	oEscrow.push_back(Pair("exttxid", escrow.extTxId.IsNull()? "": escrow.extTxId.GetHex()));
	CScriptID innerID(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()));
	CSyscoinAddress address(innerID, PaymentOptionToAddressType(escrow.nPaymentOption));
	oEscrow.push_back(Pair("escrowaddress", address.ToString()));
	string strRedeemTxId = "";
	if(!escrow.redeemTxId.IsNull())
		strRedeemTxId = escrow.redeemTxId.GetHex();
    oEscrow.push_back(Pair("paymentoption", GetPaymentOptionsString(escrow.nPaymentOption)));
	oEscrow.push_back(Pair("redeem_txid", strRedeemTxId));
	oEscrow.push_back(Pair("redeem_script", HexStr(escrow.vchRedeemScript)));
    oEscrow.push_back(Pair("txid", escrow.txHash.GetHex()));
    oEscrow.push_back(Pair("height", (int)escrow.nHeight));
	oEscrow.push_back(Pair("role", (int)escrow.role));
	int64_t expired_time = GetEscrowExpiration(escrow);
	bool expired = false;
    if(expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}

	oEscrow.push_back(Pair("expired", expired));
	oEscrow.push_back(Pair("acknowledged", escrow.bPaymentAck));
	oEscrow.push_back(Pair("status", escrowEnumFromOp(escrow.op)));
	return true;
}
bool BuildEscrowIndexerJson(const COffer& theOffer, const CEscrow &escrow, UniValue& oEscrow)
{
	oEscrow.push_back(Pair("_id", stringFromVch(escrow.vchEscrow)));
	int64_t nTime = 0;
	if (chainActive.Height() >= escrow.nHeight-1) {
		CBlockIndex *pindex = chainActive[escrow.nHeight-1];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oEscrow.push_back(Pair("time", nTime));
	oEscrow.push_back(Pair("offer", stringFromVch(escrow.vchOffer)));
	oEscrow.push_back(Pair("height", (int)escrow.nHeight));
	oEscrow.push_back(Pair("seller", stringFromVch(escrow.vchSellerAlias)));
	oEscrow.push_back(Pair("arbiter", stringFromVch(escrow.vchArbiterAlias)));
	oEscrow.push_back(Pair("buyer", stringFromVch(escrow.vchBuyerAlias)));
	oEscrow.push_back(Pair("currency", IsOfferTypeInMask(theOffer.offerType, OFFERTYPE_COIN) ? GetPaymentOptionsString(escrow.nPaymentOption) : stringFromVch(theOffer.sCurrencyCode)));
	oEscrow.push_back(Pair("offer_price", theOffer.GetPrice()));
	oEscrow.push_back(Pair("offer_title", stringFromVch(theOffer.sTitle)));
	oEscrow.push_back(Pair("quantity", (int)escrow.nQty));
	const CAmount &nEscrowFees = escrow.nDeposit + escrow.nArbiterFee + escrow.nWitnessFee + escrow.nNetworkFee + escrow.nShipping;
	const CAmount &nTotalWithoutFee = escrow.nAmountOrBidPerUnit*escrow.nQty;
	const CAmount &nTotalWithFee = nTotalWithoutFee + nEscrowFees;
	oEscrow.push_back(Pair("total_with_fee", ValueFromAmount(nTotalWithFee)));
	oEscrow.push_back(Pair("total_without_fee", ValueFromAmount(nTotalWithoutFee)));
	oEscrow.push_back(Pair("exttxid", escrow.extTxId.IsNull() ? "" : escrow.extTxId.GetHex()));
	CScriptID innerID(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()));
	CSyscoinAddress address(innerID, PaymentOptionToAddressType(escrow.nPaymentOption));
	oEscrow.push_back(Pair("escrowaddress", address.ToString()));
	oEscrow.push_back(Pair("paymentoption", GetPaymentOptionsString(escrow.nPaymentOption)));
	oEscrow.push_back(Pair("status", escrowEnumFromOp(escrow.op)));
	return true;
}
void EscrowTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	
	CEscrow escrow;
	if(!escrow.UnserializeFromData(vchData, vchHash))
		return;

	CEscrow dbEscrow;
	GetEscrow(escrow.vchEscrow, dbEscrow);

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
bool CEscrowDB::ScanEscrows(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<unsigned char> vchEscrow, vchBuyerAlias, vchSellerAlias, vchArbiterAlias;
	int nStartBlock = 0;
	if (!oOptions.isNull()) {
		const UniValue &txid = find_value(oOptions, "txid");
		if (txid.isStr()) {
			strTxid = txid.get_str();
		}
		const UniValue &escrowObj = find_value(oOptions, "escrow");
		if (escrowObj.isStr()) {
			vchEscrow = vchFromValue(escrowObj);
		}

		const UniValue &aliasBuyerObj = find_value(oOptions, "buyeralias");
		if (aliasBuyerObj.isStr()) {
			vchBuyerAlias = vchFromValue(aliasBuyerObj);
		}

		const UniValue &aliasSellerObj = find_value(oOptions, "selleralias");
		if (aliasSellerObj.isStr()) {
			vchSellerAlias = vchFromValue(aliasSellerObj);
		}

		const UniValue &aliasArbiterObj = find_value(oOptions, "arbiteralias");
		if (aliasArbiterObj.isStr()) {
			vchArbiterAlias = vchFromValue(aliasArbiterObj);
		}

		const UniValue &startblock = find_value(oOptions, "startblock");
		if (startblock.isNum()) {
			nStartBlock = startblock.get_int();
		}
	}

	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CEscrow txPos;
	pair<string, vector<unsigned char> > key;
	int index = 0;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			if (pcursor->GetKey(key) && key.first == "escrowi") {
				pcursor->GetValue(txPos);
				if (nStartBlock > 0 && txPos.nHeight < nStartBlock)
				{
					pcursor->Next();
					continue;
				}
				if (!strTxid.empty() && strTxid != txPos.txHash.GetHex())
				{
					pcursor->Next();
					continue;
				}
				if (!vchEscrow.empty() && vchEscrow != txPos.vchEscrow)
				{
					pcursor->Next();
					continue;
				}
				if (!vchBuyerAlias.empty() && vchBuyerAlias != txPos.vchBuyerAlias)
				{
					pcursor->Next();
					continue;
				}
				if (!vchSellerAlias.empty() && vchSellerAlias != txPos.vchSellerAlias)
				{
					pcursor->Next();
					continue;
				}
				if (!vchArbiterAlias.empty() && vchArbiterAlias != txPos.vchArbiterAlias)
				{
					pcursor->Next();
					continue;
				}
				UniValue oEscrow(UniValue::VOBJ);
				if (!BuildEscrowJson(txPos, oEscrow))
				{
					pcursor->Next();
					continue;
				}
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				oRes.push_back(oEscrow);
				if (index >= count + from)
					break;
			}
			pcursor->Next();
		}
		catch (std::exception &e) {
			return error("%s() : deserialize error", __PRETTY_FUNCTION__);
		}
	}
	return true;
}
UniValue listescrows(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 3 < params.size())
		throw runtime_error("listescrows [count] [from] [{options}]\n"
			"scan through all escrows.\n"
			"[count]          (numeric, optional, unbounded=0, default=10) The number of results to return, 0 to return all.\n"
			"[from]           (numeric, optional, default=0) The number of results to skip.\n"
			"[options]        (object, optional) A json object with options to filter results\n"
			"    {\n"
			"      \"txid\":txid					(string) Transaction ID to filter results for\n"
			"	     \"escrow\":guid				(string) Escrow GUID to filter.\n"
			"      \"buyeralias\":alias		(string) Buyer Alias name to filter.\n"
			"      \"selleralias\":alias	(string) Seller Alias name to filter.\n"
			"      \"arbiteralias\":alias	(string) Arbiter Alias name to filter.\n"
			"      \"startblock\":block 	(number) Earliest block to filter from. Block number is the block at which the transaction would have confirmed.\n"
			"    }\n"
			+ HelpExampleCli("listcerts", "0")
			+ HelpExampleCli("listcerts", "10 10")
			+ HelpExampleCli("listcerts", "0 0 '{\"escrow\":\"32bff1fa844c124\"}'")
			+ HelpExampleCli("listcerts", "0 0 '{\"buyeralias\":\"buyer-alias\",\"selleralias\":\"seller-alias\",\"arbiteralias\":\"arbiter-alias\"}'")
			+ HelpExampleCli("listcerts", "0 0 '{\"txid\":\"1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1\",\"startblock\":0}'")
		);
	UniValue options;
	int count = 10;
	int from = 0;
	if (params.size() > 0) {
		count = params[0].get_int();
		if (count == 0) {
			count = INT_MAX;
		} else
		if (count < 0) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4539 - " + _("'count' must be 0 or greater"));
		}
	}
	if (params.size() > 1) {
		from = params[1].get_int();
		if (from < 0) {
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4539 - " + _("'from' must be 0 or greater"));
		}
	}
	if (params.size() > 2) {
		options = params[2];
	}

	UniValue oRes(UniValue::VARR);
	if (!pescrowdb->ScanEscrows(count, from, options, oRes))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4539 - " + _("Scan failed"));
	return oRes;
}
