#include "escrow.h"
#include "offer.h"
#include "alias.h"
#include "cert.h"
#include "init.h"
#include "main.h"
#include "core_io.h"
#include "util.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "policy/policy.h"
#include "script/script.h"
#include "chainparams.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
extern CScript _createmultisig_redeemScript(const UniValue& params);
using namespace std;
extern CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);
extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const CWalletTx* wtxInAlias=NULL, int nTxOutAlias = 0, bool syscoinMultiSigTx=false, const CCoinControl* coinControl=NULL, const CWalletTx* wtxInLinkAlias=NULL,  int nTxOutLinkAlias = 0)
;
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
		if (paliasdb->ReadAliasUnprunable(escrow.vchBuyerAlias, aliasBuyerPrunable) && !aliasBuyerPrunable.IsNull())
			nTime = aliasBuyerPrunable.nExpireTime;
		// buyer is expired try seller
		if(nTime <= chainActive.Tip()->nTime)
		{
			if (paliasdb->ReadAliasUnprunable(escrow.vchSellerAlias, aliasSellerPrunable) && !aliasSellerPrunable.IsNull())
			{
				nTime = aliasSellerPrunable.nExpireTime;
				// seller is expired try the arbiter
				if(nTime <= chainActive.Tip()->nTime)
				{
					if (paliasdb->ReadAliasUnprunable(escrow.vchArbiterAlias, aliasArbiterPrunable) && !aliasArbiterPrunable.IsNull())
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
bool CEscrowDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CEscrow> vtxPos;
	uint256 txHash;
	CTransaction fundingTx;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "escrowi") {
            	const vector<unsigned char> &vchMyEscrow= key.second;         
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty()){
					servicesCleaned++;
					EraseEscrow(vchMyEscrow);
					pcursor->Next();
					continue;
				}
				const CEscrow &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos))
				{
					servicesCleaned++;
					EraseEscrow(vchMyEscrow);	
				}
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}

bool CEscrowDB::ScanEscrows(const std::vector<unsigned char>& vchEscrow, const string& strRegexp, const vector<string>& aliasArray, unsigned int nMax,
							std::vector<std::pair<CEscrow, CEscrow> >& escrowScan) {
	string strSearchLower = strRegexp;
	boost::algorithm::to_lower(strSearchLower);
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	if(!vchEscrow.empty())
		pcursor->Seek(make_pair(string("escrowi"), vchEscrow));
	else
		pcursor->SeekToFirst();
	vector<CEscrow> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "escrowi") {
            	const vector<unsigned char> &vchMyEscrow = key.second;
                
				pcursor->GetValue(vtxPos);
				if (vtxPos.empty()){
					pcursor->Next();
					continue;
				}
				const CEscrow &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos))
				{
					pcursor->Next();
					continue;
				}
				const string &escrow = stringFromVch(vchMyEscrow);
				const string &offerstr = stringFromVch(txPos.vchOffer);
			

				string buyerAliasLower = stringFromVch(txPos.vchBuyerAlias);
				string sellerAliasLower = stringFromVch(txPos.vchSellerAlias);
				string arbiterAliasLower = stringFromVch(txPos.vchArbiterAlias);
				string linkSellerAliasLower = stringFromVch(txPos.vchLinkSellerAlias);
				if(aliasArray.size() > 0)
				{
					bool notFoundLinkSeller = true;
					if(!linkSellerAliasLower.empty())
						notFoundLinkSeller = (std::find(aliasArray.begin(), aliasArray.end(), linkSellerAliasLower) == aliasArray.end());
					if (std::find(aliasArray.begin(), aliasArray.end(), buyerAliasLower) == aliasArray.end() &&
						std::find(aliasArray.begin(), aliasArray.end(), sellerAliasLower) == aliasArray.end() &&
						std::find(aliasArray.begin(), aliasArray.end(), arbiterAliasLower) == aliasArray.end() &&
						notFoundLinkSeller)
					{
						pcursor->Next();
						continue;
					}
				}
				if (strRegexp != "" && strRegexp != offerstr && strRegexp != escrow && strSearchLower != buyerAliasLower && strSearchLower != sellerAliasLower && strSearchLower != arbiterAliasLower)
				{
					pcursor->Next();
					continue;
				}
                escrowScan.push_back(make_pair(txPos, vtxPos.front()));
            }
            if (escrowScan.size() >= nMax)
                break;

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
bool GetTxOfEscrow(const vector<unsigned char> &vchEscrow,
        CEscrow& txPos, CTransaction& tx) {
    vector<CEscrow> vtxPos;
    if (!pescrowdb->ReadEscrow(vchEscrow, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos)) {
        string escrow = stringFromVch(vchEscrow);
        LogPrintf("GetTxOfEscrow(%s) : expired", escrow.c_str());
        return false;
    }
    if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
        return error("GetTxOfEscrow() : could not read tx from disk");

    return true;
}
bool GetTxAndVtxOfEscrow(const vector<unsigned char> &vchEscrow,
        CEscrow& txPos, CTransaction& tx, vector<CEscrow> &vtxPos) {

    if (!pescrowdb->ReadEscrow(vchEscrow, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
   if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos)) {
        string escrow = stringFromVch(vchEscrow);
        LogPrintf("GetTxOfEscrow(%s) : expired", escrow.c_str());
        return false;
    }
    if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
        return error("GetTxOfEscrow() : could not read tx from disk");

    return true;
}
bool GetVtxOfEscrow(const vector<unsigned char> &vchEscrow,
        CEscrow& txPos, vector<CEscrow> &vtxPos) {

    if (!pescrowdb->ReadEscrow(vchEscrow, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
   if (chainActive.Tip()->nTime >= GetEscrowExpiration(txPos)) {
        string escrow = stringFromVch(vchEscrow);
        LogPrintf("GetTxOfEscrow(%s) : expired", escrow.c_str());
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
		if(pescrowdb->ExistsEscrowTx(theEscrow.extTxId) || pofferdb->ExistsOfferTx(theEscrow.extTxId))
		{
			errorMessage = _("External Transaction ID specified was already used to pay for an offer");
			return true;
		}
	}
	if(!dontaddtodb && !pescrowdb->WriteEscrowTx(theEscrow.vchEscrow, theEscrow.extTxId))
	{
		errorMessage = _("Failed to External Transaction ID to DB");
		return false;
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
			if(prevCoins->vout.size() <= prevOutput->n || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch) || pop == OP_ALIAS_PAYMENT)
				continue;
			if(foundAlias)
				break;

			else if (!foundAlias && IsAliasOp(pop))
			{
				foundAlias = true;
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}
	}

	vector<COffer> myVtxPos,myLinkVtxPos;
	CAliasIndex buyerAlias, sellerAlias, arbiterAlias;
	CTransaction aliasTx;
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
		if(theEscrow.feedback.size() > 0 && theEscrow.feedback[0].vchFeedback.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4006 - " + _("Feedback too long");
			return error(errorMessage.c_str());
		}
		if(theEscrow.feedback.size() > 1 && theEscrow.feedback[1].vchFeedback.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4007 - " + _("Feedback too long");
			return error(errorMessage.c_str());
		}
		if(theEscrow.vchOffer.size() > MAX_ID_LENGTH)
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
					if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theEscrow.vchLinkAlias != vvchPrevAliasArgs[0] )
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4010 - " + _("Alias input mismatch");
						return error(errorMessage.c_str());
					}
				}
				else
				{
					if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theEscrow.vchBuyerAlias != vvchPrevAliasArgs[0] )
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4011 - " + _("Alias input mismatch");
						return error(errorMessage.c_str());
					}
					if(theEscrow.op != OP_ESCROW_ACTIVATE)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4012 - " + _("Invalid op, should be escrow activate");
						return error(errorMessage.c_str());
					}
					if (theEscrow.vchPaymentMessage.size() > MAX_ENCRYPTED_NAME_LENGTH)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4013 - " + _("Payment message too long");
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
				if(!theEscrow.feedback.empty())
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
				if(!theEscrow.feedback.empty())
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
				if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theEscrow.vchLinkAlias != vvchPrevAliasArgs[0] )
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4023 - " + _("Alias input mismatch");
					return error(errorMessage.c_str());
				}
				if (theEscrow.op != OP_ESCROW_COMPLETE)
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4024 - " + _("Invalid op, should be escrow complete");
					return error(errorMessage.c_str());
				}
				if(theEscrow.feedback.empty())
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
				if(!IsAliasOp(prevAliasOp) || vvchPrevAliasArgs.empty() || theEscrow.vchLinkAlias != vvchPrevAliasArgs[0] )
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
				if(!theEscrow.feedback.empty())
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
				vector<CAliasIndex> vtxAlias;
				bool isExpired = false;
				if(!GetVtxOfAlias(theEscrow.vchBuyerAlias, buyerAlias, vtxAlias, isExpired))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4034 - " + _("Cannot find buyer alias. It may be expired");
					return true;
				}
				if(!GetVtxOfAlias(theEscrow.vchArbiterAlias, arbiterAlias, vtxAlias, isExpired))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4035 - " + _("Cannot find arbiter alias. It may be expired");
					return true;
				}
				if(!GetVtxOfAlias(theEscrow.vchSellerAlias, sellerAlias, vtxAlias, isExpired))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4036 - " + _("Cannot find seller alias. It may be expired");
					return true;
				}
			}
		}
		vector<CEscrow> vtxPos;
		// make sure escrow settings don't change (besides rawTx) outside of activation
		if(op != OP_ESCROW_ACTIVATE || theEscrow.bPaymentAck)
		{
			// save serialized escrow for later use
			CEscrow serializedEscrow = theEscrow;
			if(!GetVtxOfEscrow(vvchArgs[0], theEscrow, vtxPos))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4037 - " + _("Failed to read from escrow DB");
				return true;
			}
			if(serializedEscrow.vchBuyerAlias != theEscrow.vchBuyerAlias || 
				serializedEscrow.vchArbiterAlias != theEscrow.vchArbiterAlias ||
				serializedEscrow.vchSellerAlias != theEscrow.vchSellerAlias)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4038 - " + _("Invalid aliases used for escrow transaction");
				return true;
			}
			if(serializedEscrow.bPaymentAck && theEscrow.bPaymentAck)
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4039 - " + _("Escrow already acknowledged");
			}
			// make sure we have found this escrow in db
			if(!vtxPos.empty())
			{
				if (theEscrow.vchEscrow != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4040 - " + _("Escrow Guid mismatch");
					return true;
				}

				// these are the only settings allowed to change outside of activate
				if(!serializedEscrow.rawTx.empty() && op != OP_ESCROW_ACTIVATE)
					theEscrow.rawTx = serializedEscrow.rawTx;
				escrowOp = serializedEscrow.op;
				if(op == OP_ESCROW_ACTIVATE && serializedEscrow.bPaymentAck)
				{
					if(serializedEscrow.vchLinkAlias != theEscrow.vchSellerAlias)
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
					vector<CAliasIndex> vtxAlias;
					bool isExpired = false;
					if(!GetVtxOfAlias(theEscrow.vchSellerAlias, alias, vtxAlias, isExpired))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4042 - " + _("Cannot find seller alias. It may be expired");
						return true;
					}
					if(!GetVtxOfAlias(theEscrow.vchArbiterAlias, alias, vtxAlias, isExpired))
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
					else if(serializedEscrow.vchLinkAlias != theEscrow.vchSellerAlias && serializedEscrow.vchLinkAlias != theEscrow.vchArbiterAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4046 - " + _("Only arbiter or seller can initiate an escrow refund");
						return true;
					}
					// only the arbiter can re-refund an escrow
					else if(theEscrow.op == OP_ESCROW_REFUND && serializedEscrow.vchLinkAlias != theEscrow.vchArbiterAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4047 - " + _("Only arbiter can refund an escrow after it has already been refunded");
						return true;
					}
					// refund qty
					if (GetVtxOfOffer( theEscrow.vchOffer, dbOffer, myVtxPos))
					{
						int nQty = dbOffer.nQty;
						COffer myLinkOffer;
						if (pofferdb->ExistsOffer(dbOffer.vchLinkOffer)) {
							if (pofferdb->ReadOffer(dbOffer.vchLinkOffer, myLinkVtxPos) && !myLinkVtxPos.empty())
							{
								myLinkOffer = myLinkVtxPos.back();
								nQty = myLinkOffer.nQty;
							}
						}
						if(nQty != -1)
						{
							nQty += theEscrow.nQty;
							if (!myLinkOffer.IsNull())
							{
								myLinkOffer.nQty = nQty;
								myLinkOffer.nSold--;
								myLinkOffer.PutToOfferList(myLinkVtxPos);
								if (!dontaddtodb && !pofferdb->WriteOffer(dbOffer.vchLinkOffer, myLinkVtxPos))
								{
									errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4048 - " + _("Failed to write to offer link to DB");
									return error(errorMessage.c_str());
								}
							}
							else
							{
								dbOffer.nQty = nQty;
								dbOffer.nSold--;
								dbOffer.PutToOfferList(myVtxPos);
								if (!dontaddtodb && !pofferdb->WriteOffer(theEscrow.vchOffer, myVtxPos))
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
					else if(serializedEscrow.vchLinkAlias != theEscrow.vchBuyerAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4051 - " + _("Only buyer can claim an escrow refund");
						return true;
					}
				}
				else if(op == OP_ESCROW_RELEASE && vvchArgs[1] == vchFromString("0"))
				{
					CAliasIndex alias;
					vector<CAliasIndex> vtxAlias;
					bool isExpired = false;
					if(!GetVtxOfAlias(theEscrow.vchBuyerAlias, alias, vtxAlias, isExpired))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4052 - " + _("Cannot find buyer alias. It may be expired");
						return true;
					}
					if(!GetVtxOfAlias(theEscrow.vchArbiterAlias, alias, vtxAlias, isExpired))
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
					else if(serializedEscrow.vchLinkAlias != theEscrow.vchBuyerAlias && serializedEscrow.vchLinkAlias != theEscrow.vchArbiterAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4056 - " + _("Only arbiter or buyer can initiate an escrow release");
						return true;
					}
					// only the arbiter can re-release an escrow
					else if(theEscrow.op == OP_ESCROW_RELEASE && serializedEscrow.vchLinkAlias != theEscrow.vchArbiterAlias)
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
					else if(serializedEscrow.vchLinkAlias != theEscrow.vchSellerAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4059 - " + _("Only seller can claim an escrow release");
						return true;
					}
				}
				else if(op == OP_ESCROW_COMPLETE)
				{
					vector<unsigned char> vchSellerAlias = theEscrow.vchSellerAlias;
					if(!theEscrow.vchLinkSellerAlias.empty())
						vchSellerAlias = theEscrow.vchLinkSellerAlias;
					if(serializedEscrow.feedback.size() != 2)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4060 - " + _("Invalid number of escrow feedbacks provided");
						serializedEscrow = theEscrow;
					}
					if(serializedEscrow.feedback[0].nFeedbackUserFrom ==  serializedEscrow.feedback[0].nFeedbackUserTo ||
						serializedEscrow.feedback[1].nFeedbackUserFrom ==  serializedEscrow.feedback[1].nFeedbackUserTo)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4061 - " + _("Cannot send yourself feedback");
						serializedEscrow = theEscrow;
					}
					else if(serializedEscrow.feedback[0].vchFeedback.size() <= 0 && serializedEscrow.feedback[1].vchFeedback.size() <= 0)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4062 - " + _("Feedback must leave a message");
						serializedEscrow = theEscrow;
					}
					else if(serializedEscrow.feedback[0].nRating > 5 || serializedEscrow.feedback[1].nRating > 5)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4063 - " + _("Invalid rating, must be less than or equal to 5 and greater than or equal to 0");
						serializedEscrow = theEscrow;
					}
					else if((serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKBUYER || serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKBUYER) && serializedEscrow.vchLinkAlias != theEscrow.vchBuyerAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4064 - " + _("Only buyer can leave this feedback");
						serializedEscrow = theEscrow;
					}
					else if((serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKSELLER || serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKSELLER) && serializedEscrow.vchLinkAlias != vchSellerAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4065 - " + _("Only seller can leave this feedback");
						serializedEscrow = theEscrow;
					}
					else if((serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKARBITER || serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKARBITER) && serializedEscrow.vchLinkAlias != theEscrow.vchArbiterAlias)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4066 - " + _("Only arbiter can leave this feedback");
						serializedEscrow = theEscrow;
					}
					serializedEscrow.feedback[0].nHeight = nHeight;
					serializedEscrow.feedback[0].txHash = tx.GetHash();
					serializedEscrow.feedback[1].nHeight = nHeight;
					serializedEscrow.feedback[1].txHash = tx.GetHash();
					int numBuyerRatings, numSellerRatings, numArbiterRatings, feedbackBuyerCount, feedbackSellerCount, feedbackArbiterCount;
					FindFeedback(theEscrow.feedback, numBuyerRatings, numSellerRatings, numArbiterRatings, feedbackBuyerCount, feedbackSellerCount, feedbackArbiterCount);

					// has this user already rated?
					if(numBuyerRatings > 0)
					{
						if(serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKBUYER)
							serializedEscrow.feedback[0].nRating = 0;
						if(serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKBUYER)
							serializedEscrow.feedback[1].nRating = 0;
					}
					if(numSellerRatings > 0)
					{
						if(serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKSELLER)
							serializedEscrow.feedback[0].nRating = 0;
						if(serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKSELLER)
							serializedEscrow.feedback[1].nRating = 0;
					}
					if(numArbiterRatings > 0)
					{
						if(serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKARBITER)
							serializedEscrow.feedback[0].nRating = 0;
						if(serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKARBITER)
							serializedEscrow.feedback[1].nRating = 0;
					}

					if(feedbackBuyerCount >= 10 && (serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKBUYER || serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKBUYER))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4067 - " + _("Cannot exceed 10 buyer feedbacks");
						serializedEscrow = theEscrow;
					}
					else if(feedbackSellerCount >= 10 && (serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKSELLER || serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKSELLER))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4068 - " + _("Cannot exceed 10 seller feedbacks");
						serializedEscrow = theEscrow;
					}
					else if(feedbackArbiterCount >= 10 && (serializedEscrow.feedback[0].nFeedbackUserFrom == FEEDBACKARBITER || serializedEscrow.feedback[1].nFeedbackUserFrom == FEEDBACKARBITER))
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4069 - " + _("Cannot exceed 10 arbiter feedbacks");
						serializedEscrow = theEscrow;
					}
					if(!dontaddtodb)
						HandleEscrowFeedback(serializedEscrow, theEscrow, vtxPos);
					return true;
				}
			}
			else
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4070 - " + _("Escrow not found when trying to update");
				return true;
			}

		}
		else
		{
			COffer myLinkOffer;
			if (pescrowdb->ExistsEscrow(vvchArgs[0]))
			{
				errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4071 - " + _("Escrow already exists");
				return true;
			}
			if(theEscrow.nQty <= 0)
				theEscrow.nQty = 1;

			if (GetVtxOfOffer( theEscrow.vchOffer, dbOffer, myVtxPos))
			{
				if(dbOffer.bPrivate && !dbOffer.linkWhitelist.IsNull())
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4072 - " + _("Cannot purchase this private offer, must purchase through an affiliate");
					return true;
				}
				if(dbOffer.sCategory.size() > 0 && boost::algorithm::starts_with(stringFromVch(dbOffer.sCategory), "wanted"))
				{
					errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4073 - " + _("Cannot purchase a wanted offer");
				}
				int nQty = dbOffer.nQty;
				// if this is a linked offer we must update the linked offer qty
				if (pofferdb->ExistsOffer(dbOffer.vchLinkOffer)) {
					if (pofferdb->ReadOffer(dbOffer.vchLinkOffer, myLinkVtxPos) && !myLinkVtxPos.empty())
					{
						myLinkOffer = myLinkVtxPos.back();
						nQty = myLinkOffer.nQty;
					}
				}
				if(nQty != -1)
				{
					if(theEscrow.nQty > nQty)
					{
						errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4074 - " + _("Not enough quantity left in this offer for this purchase");
						return true;
					}
					nQty -= theEscrow.nQty;
					if (!myLinkOffer.IsNull())
					{
						myLinkOffer.nQty = nQty;
						myLinkOffer.nSold++;
						myLinkOffer.PutToOfferList(myLinkVtxPos);
						if (!dontaddtodb && !pofferdb->WriteOffer(dbOffer.vchLinkOffer, myLinkVtxPos))
						{
							errorMessage = "SYSCOIN_ESCROW_CONSENSUS_ERROR: ERRCODE: 4075 - " + _("Failed to write to offer link to DB");
							return error(errorMessage.c_str());
						}
					}
					else
					{
						dbOffer.nQty = nQty;
						dbOffer.nSold++;
						dbOffer.PutToOfferList(myVtxPos);
						if (!dontaddtodb && !pofferdb->WriteOffer(theEscrow.vchOffer, myVtxPos))
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
			if(!theOffer.vchLinkOffer.empty() && myLinkOffer.IsNull())
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
		PutToEscrowList(vtxPos, theEscrow);
        // write escrow

        if (!dontaddtodb && !pescrowdb->WriteEscrow(vvchArgs[0], vtxPos))
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
void HandleEscrowFeedback(const CEscrow& serializedEscrow, CEscrow& dbEscrow, vector<CEscrow> &vtxPos)
{
	for(int i =0;i<serializedEscrow.feedback.size();i++)
	{
		if(serializedEscrow.feedback[i].nRating > 0)
		{
			CSyscoinAddress address;
			if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKBUYER)
				address = CSyscoinAddress(stringFromVch(dbEscrow.vchBuyerAlias));
			else if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKSELLER)
			{
				if(!dbEscrow.vchLinkSellerAlias.empty())
					address = CSyscoinAddress(stringFromVch(dbEscrow.vchLinkSellerAlias));
				else
					address = CSyscoinAddress(stringFromVch(dbEscrow.vchSellerAlias));
			}
			else if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKARBITER)
				address = CSyscoinAddress(stringFromVch(dbEscrow.vchArbiterAlias));
			if(address.IsValid() && address.isAlias)
			{
				vector<CAliasIndex> vtxPos;
				const vector<unsigned char> &vchAlias = vchFromString(address.aliasName);
				if (paliasdb->ReadAlias(vchAlias, vtxPos) && !vtxPos.empty())
				{

					CAliasIndex alias = vtxPos.back();
					if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKBUYER)
					{
						alias.nRatingCountAsBuyer++;
						alias.nRatingAsBuyer += serializedEscrow.feedback[i].nRating;
					}
					else if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKSELLER)
					{
						alias.nRatingCountAsSeller++;
						alias.nRatingAsSeller += serializedEscrow.feedback[i].nRating;
					}
					else if(serializedEscrow.feedback[i].nFeedbackUserTo == FEEDBACKARBITER)
					{
						alias.nRatingCountAsArbiter++;
						alias.nRatingAsArbiter += serializedEscrow.feedback[i].nRating;
					}


					PutToAliasList(vtxPos, alias);
					paliasdb->WriteAlias(vchAlias, vtxPos);
				}
			}

		}
		dbEscrow.feedback.push_back(serializedEscrow.feedback[i]);
	}
	PutToEscrowList(vtxPos, dbEscrow);
	pescrowdb->WriteEscrow(dbEscrow.vchEscrow, vtxPos);
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
	if(params.size() >= 5 && !params[4].get_str().empty() && params[4].get_str() != "NONE")
	{
		paymentOption = params[4].get_str();
	}
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOption))
	{
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4501 - " + _("Could not validate the payment options value");
		throw runtime_error(err.c_str());
	}

	CAliasIndex arbiteralias;
	CTransaction arbiteraliastx;
	if (!GetTxOfAlias(vchArbiter, arbiteralias, arbiteraliastx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4502 - " + _("Failed to read arbiter alias from DB"));

	CAliasIndex buyeralias;
	CTransaction buyeraliastx;
	if (!GetTxOfAlias(vchBuyer, buyeralias, buyeraliastx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4503 - " + _("Failed to read arbiter alias from DB"));

	CTransaction txOffer, txAlias;
	vector<COffer> offerVtxPos;
	COffer theOffer, linkedOffer;
	if (!GetTxAndVtxOfOffer( vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4504 - " + _("Could not find an offer with this identifier"));

	CAliasIndex selleralias;
	if (!GetTxOfAlias( theOffer.vchAlias, selleralias, txAlias, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4505 - " + _("Could not find seller alias with this identifier"));
	
	COfferLinkWhitelistEntry foundEntry;
	if(!theOffer.vchLinkOffer.empty())
	{
		CTransaction tmpTx;
		vector<COffer> offerTmpVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkedOffer, tmpTx, offerTmpVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4506 - " + _("Trying to accept a linked offer but could not find parent offer"));

		CAliasIndex theLinkedAlias;
		CTransaction txLinkedAlias;
		if (!GetTxOfAlias( linkedOffer.vchAlias, theLinkedAlias, txLinkedAlias, true))
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
	arrayOfKeys.push_back(HexStr(arbiteralias.vchPubKey));
	arrayOfKeys.push_back(HexStr(selleralias.vchPubKey));
	arrayOfKeys.push_back(HexStr(buyeralias.vchPubKey));
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

	int precision = 2;
	float fEscrowFee = getEscrowFee(selleralias.vchAliasPeg, vchFromString(paymentOption), chainActive.Tip()->nHeight, precision);
	CAmount nTotal = theOffer.GetPrice(foundEntry)*nQty;
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotal, fEscrowFee);
	CAmount nExtFee = convertSyscoinToCurrencyCode(selleralias.vchAliasPeg, vchFromString(paymentOption), nEscrowFee, chainActive.Tip()->nHeight, precision);
	CAmount nExtTotal = convertSyscoinToCurrencyCode(selleralias.vchAliasPeg, vchFromString(paymentOption), theOffer.GetPrice(foundEntry), chainActive.Tip()->nHeight, precision)*nQty;
	int nExtFeePerByte = getFeePerByte(selleralias.vchAliasPeg, vchFromString(paymentOption), chainActive.Tip()->nHeight, precision);
	// multisig spend is about 400 bytes
	nExtTotal += nExtFee + (nExtFeePerByte*400);
	resCreate.push_back(Pair("total", ValueFromAmount(nExtTotal)));
	resCreate.push_back(Pair("height", chainActive.Tip()->nHeight));
	return resCreate;
}

UniValue escrownew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() < 5 ||  params.size() > 9)
        throw runtime_error(
		"escrownew <alias> <offer> <quantity> <message> <arbiter alias> [extTx] [payment option=SYS] [redeemScript] [height]\n"
						"<alias> An alias you own.\n"
                        "<offer> GUID of offer that this escrow is managing.\n"
                        "<quantity> Quantity of items to buy of offer.\n"
						"<message> Delivery details to seller. 256 characters max\n"
						"<arbiter alias> Alias of Arbiter.\n"
						"<extTx> External transaction ID if paid with another blockchain.\n"
						"<paymentOption> If extTx is defined, specify a valid payment option used to make payment. Default is SYS.\n"
						"<redeemScript> If paid in external chain, enter redeemScript that generateescrowmultisig returns\n"
						"<height> If paid in extneral chain, enter height that generateescrowmultisig returns\n"
                        + HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	uint64_t nHeight = chainActive.Tip()->nHeight;
	string strArbiter = params[4].get_str();
	boost::algorithm::to_lower(strArbiter);
	// check for alias existence in DB
	CAliasIndex arbiteralias;
	CTransaction aliastx, buyeraliastx;
	if (!GetTxOfAlias(vchFromString(strArbiter), arbiteralias, aliastx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4509 - " + _("Failed to read arbiter alias from DB"));
	
	string extTxIdStr;
	if(params.size() >= 6)
		extTxIdStr = params[5].get_str();

	vector<unsigned char> vchMessage = vchFromValue(params[3]);
	// payment options - get payment options string if specified otherwise default to SYS
	string paymentOptions = "SYS";
	if(params.size() >= 7 && !params[6].get_str().empty() && params[6].get_str() != "NONE")
	{
		paymentOptions = params[6].get_str();
		boost::algorithm::to_upper(paymentOptions);
	}
	// payment options - validate payment options string
	if(!ValidatePaymentOptionsString(paymentOptions))
	{
		// TODO change error number to something unique
		string err = "SYSCOIN_ESCROW_RPC_ERROR ERRCODE: 4510 - " + _("Could not validate the payment options value");
		throw runtime_error(err.c_str());
	}
		// payment options - and convert payment options string to a bitmask for the txn
	unsigned char paymentOptionsMask = (unsigned char) GetPaymentOptionsMaskFromString(paymentOptions);
	vector<unsigned char> vchRedeemScript;
	if(params.size() >= 8)
		vchRedeemScript = vchFromValue(params[7]);
	if(params.size() >= 9)
		nHeight = boost::lexical_cast<uint64_t>(params[8].get_str());

	unsigned int nQty = 1;

	try {
		nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
	} catch (std::exception &e) {
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4511 - " + _("Invalid quantity value. Quantity must be less than 4294967296."));
	}

    if (vchMessage.size() <= 0)
        vchMessage = vchFromString("ESCROW");


	CAliasIndex buyeralias;
	if (!GetTxOfAlias(vchAlias, buyeralias, buyeraliastx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4512 - " + _("Could not find buyer alias with this name"));
	

	COffer theOffer, linkedOffer;

	CTransaction txOffer, txAlias;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( vchOffer, theOffer, txOffer, offerVtxPos))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4513 - " + _("Could not find an offer with this identifier"));

	CAliasIndex selleralias;
	if (!GetTxOfAlias( theOffer.vchAlias, selleralias, txAlias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4514 - " + _("Could not find seller alias with this identifier"));

	if(theOffer.sCategory.size() > 0 && boost::algorithm::starts_with(stringFromVch(theOffer.sCategory), "wanted"))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4515 - " + _("Cannot purchase a wanted offer"));

	const CWalletTx *wtxAliasIn = NULL;

	CScript scriptPubKeyAlias, scriptPubKeyAliasOrig;
	COfferLinkWhitelistEntry foundEntry;
	CAliasIndex theLinkedAlias, reselleralias;
	CAmount nCommission;
	if(!theOffer.vchLinkOffer.empty())
	{
		
		CTransaction tmpTx;
		vector<COffer> offerTmpVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkedOffer, tmpTx, offerTmpVtxPos))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4516 - " + _("Trying to accept a linked offer but could not find parent offer"));

		
		CTransaction txLinkedAlias;
		if (!GetTxOfAlias( linkedOffer.vchAlias, theLinkedAlias, txLinkedAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4517 - " + _("Could not find an alias with this identifier"));
		if(linkedOffer.sCategory.size() > 0 && boost::algorithm::starts_with(stringFromVch(linkedOffer.sCategory), "wanted"))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4518 - " + _("Cannot purchase a wanted offer"));

		linkedOffer.linkWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);

		reselleralias = selleralias;
		selleralias = theLinkedAlias;
	}
	else
		theOffer.linkWhitelist.GetLinkEntryByHash(buyeralias.vchAlias, foundEntry);

	if(!IsMyAlias(buyeralias))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4519 - " + _("You must own the buyer alias to complete this transaction"));
	COutPoint outPoint;
	int numResults  = aliasunspent(buyeralias.vchAlias, outPoint);	
	wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);

	CSyscoinAddress buyerAddress;
	GetAddress(buyeralias, &buyerAddress, scriptPubKeyAliasOrig);

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyeralias.vchAlias  << buyeralias.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;


    // gather inputs
	vector<unsigned char> vchEscrow = vchFromString(GenerateSyscoinGuid());

    // this is a syscoin transaction
    CWalletTx wtx;
	EnsureWalletIsUnlocked();
    CScript scriptPubKey, scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyRootSeller, scriptPubKeyArbiter,scriptBuyer, scriptSeller,scriptRootSeller,scriptArbiter;

	string strCipherText = "";
	// encrypt to offer owner
	if(!EncryptMessage(selleralias, vchMessage, strCipherText))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4520 - " + _("Could not encrypt message to seller"));

	if (strCipherText.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4521 - " + _("Payment message length cannot exceed 1024 characters"));

	CSyscoinAddress arbiterAddress;
	GetAddress(arbiteralias, &arbiterAddress, scriptArbiter);
	CSyscoinAddress sellerAddress;
	GetAddress(selleralias, &sellerAddress, scriptRootSeller);
	CSyscoinAddress resellerAddress;
	GetAddress(reselleralias, &resellerAddress, scriptSeller);

	vector<unsigned char> redeemScript;
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
	}
	else
	{
			redeemScript = ParseHex(stringFromVch(vchRedeemScript));
	}
	scriptPubKey = CScript(redeemScript.begin(), redeemScript.end());
	int precision = 2;
	// send to escrow address
	CAmount nTotal = theOffer.GetPrice(foundEntry)*nQty;
	float fEscrowFee = getEscrowFee(selleralias.vchAliasPeg, vchFromString("SYS"), chainActive.Tip()->nHeight, precision);
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotal, fEscrowFee);
	int nFeePerByte = getFeePerByte(selleralias.vchAliasPeg, vchFromString("SYS"), chainActive.Tip()->nHeight,precision);

	vector<CRecipient> vecSend;
	CAmount nAmountWithFee = nTotal+nEscrowFee+(nFeePerByte*400);
	CWalletTx escrowWtx;
	CRecipient recipientEscrow  = {scriptPubKey, nAmountWithFee, false};
	if(extTxIdStr.empty())
		vecSend.push_back(recipientEscrow);

	// send to seller/arbiter so they can track the escrow through GUI
    // build escrow
    CEscrow newEscrow;
	newEscrow.op = OP_ESCROW_ACTIVATE;
	newEscrow.vchEscrow = vchEscrow;
	newEscrow.vchBuyerAlias = buyeralias.vchAlias;
	newEscrow.vchArbiterAlias = arbiteralias.vchAlias;
	newEscrow.vchRedeemScript = redeemScript;
	newEscrow.vchOffer = vchOffer;
	newEscrow.extTxId = uint256S(extTxIdStr);
	newEscrow.vchSellerAlias = selleralias.vchAlias;
	newEscrow.vchLinkSellerAlias = reselleralias.vchAlias;
	newEscrow.vchPaymentMessage = vchFromString(strCipherText);
	newEscrow.nQty = nQty;
	newEscrow.nPaymentOption = paymentOptionsMask;
	newEscrow.nHeight = nHeight;
	newEscrow.nAcceptHeight = chainActive.Tip()->nHeight;

	vector<unsigned char> data;
	newEscrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
	scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeySeller << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow  << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyArbiter << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeySeller += scriptSeller;
	scriptPubKeyArbiter += scriptArbiter;
	scriptPubKeyBuyer += scriptPubKeyAliasOrig;

	scriptPubKeyRootSeller << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
	scriptPubKeyRootSeller += scriptRootSeller;


	// send the tranasction

	CRecipient recipientArbiter;
	CreateRecipient(scriptPubKeyArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);
	CRecipient recipientSeller;
	CreateRecipient(scriptPubKeySeller, recipientSeller);
	
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient recipientRootSeller;
	CreateRecipient(scriptPubKeyRootSeller, recipientRootSeller);
	if(!reselleralias.IsNull())
		vecSend.push_back(recipientSeller);
		
	vecSend.push_back(recipientRootSeller);
	

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);


	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, buyeralias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);




	SendMoneySyscoin(vecSend,recipientBuyer.nAmount+recipientArbiter.nAmount+recipientSeller.nAmount+recipientRootSeller.nAmount+aliasRecipient.nAmount+recipientEscrow.nAmount+fee.nAmount, false, wtx, wtxAliasIn, outPoint.n, buyeralias.multiSigInfo.vchAliases.size() > 0);
	UniValue res(UniValue::VARR);
	if(buyeralias.multiSigInfo.vchAliases.size() > 0)
	{
		UniValue signParams(UniValue::VARR);
		signParams.push_back(EncodeHexTx(wtx));
		const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
		const UniValue& so = resSign.get_obj();
		string hex_str = "";

		const UniValue& hex_value = find_value(so, "hex");
		if (hex_value.isStr())
			hex_str = hex_value.get_str();
		const UniValue& complete_value = find_value(so, "complete");
		bool bComplete = false;
		if (complete_value.isBool())
			bComplete = complete_value.get_bool();
		if(bComplete)
		{
			res.push_back(wtx.GetHash().GetHex());
			res.push_back(stringFromVch(vchEscrow));
		}
		else
		{
			res.push_back(hex_str);
			res.push_back(stringFromVch(vchEscrow));
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
		res.push_back(stringFromVch(vchEscrow));
	}
	return res;
}
UniValue escrowrelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 3 || params.size() < 2)
        throw runtime_error(
		"escrowrelease <escrow guid> <user role> [rawTx]\n"
                        "Releases escrow funds to seller, seller needs to sign the output transaction and send to the network. User role represents either 'buyer' or 'arbiter'. Enter in rawTx if this is an external payment release.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	string rawTx;
	if(params.size() >= 3)
		rawTx = params[2].get_str();

    // this is a syscoin transaction
    CWalletTx wtx;

	EnsureWalletIsUnlocked();

    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4524 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest, resellerAlias, resellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx, buyeraliastx, arbiteraliastx, reselleraliastx;
	bool isExpired;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment, resellerAddressPayment;
	CScript arbiterScript;
	if(GetTxAndVtxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, aliasVtxPos, isExpired, true))
	{
		arbiterAlias.nHeight = vtxPos.front().nHeight;
		arbiterAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);

	}

	aliasVtxPos.clear();
	CScript buyerScript;
	if(GetTxAndVtxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, aliasVtxPos, isExpired, true))
	{
		buyerAlias.nHeight = vtxPos.front().nHeight;
		buyerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}
	aliasVtxPos.clear();
	CScript sellerScript;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		sellerAlias.nHeight = vtxPos.front().nHeight;
		sellerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}

	const CWalletTx *wtxAliasIn = NULL;
	CScript scriptPubKeyAlias;

	COffer theOffer, linkOffer;
	CTransaction txOffer;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( escrow.vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4525 - " + _("Could not find an offer with this identifier"));
	theOffer.nHeight = vtxPos.front().nAcceptHeight;
	theOffer.GetOfferFromList(offerVtxPos);
	CScript resellerScript;
	if(!theOffer.vchLinkOffer.empty())
	{
		vector<COffer> offerLinkVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, txOffer, offerLinkVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4526 - " + _("Could not find an offer with this identifier"));
		linkOffer.nHeight = vtxPos.front().nAcceptHeight;
		linkOffer.GetOfferFromList(offerLinkVtxPos);
		if(GetTxAndVtxOfAlias(theOffer.vchAlias, resellerAliasLatest, reselleraliastx, aliasVtxPos, isExpired, true))
		{
			resellerAlias.nHeight = vtxPos.front().nHeight;
			resellerAlias.GetAliasFromList(aliasVtxPos);
			GetAddress(resellerAliasLatest, &resellerAddressPayment, resellerScript, escrow.nPaymentOption);
		}
	}
	CAmount nCommission;
	COfferLinkWhitelistEntry foundEntry;
	if(theOffer.vchLinkOffer.empty())
	{
		theOffer.linkWhitelist.GetLinkEntryByHash(buyerAlias.vchAlias, foundEntry);
		nCommission = 0;
	}
	else
	{
		linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);
		nCommission = theOffer.GetPrice() - linkOffer.GetPrice(foundEntry);
		if(nCommission < 0)
			nCommission = 0;
	}
	CAmount nExpectedCommissionAmount, nExpectedAmount, nEscrowFee, nEscrowTotal;
	int nFeePerByte;
	int precision = 2;
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		string paymentOptionStr = GetPaymentOptionsString(escrow.nPaymentOption);
		nExpectedCommissionAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nCommission, vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		nExpectedAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), theOffer.GetPrice(foundEntry), vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(theOffer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);	
		nEscrowFee = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nEscrowFee, vtxPos.front().nAcceptHeight, precision);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);	
	}
	else
	{
		nExpectedCommissionAmount = nCommission*escrow.nQty;
		nExpectedAmount = theOffer.GetPrice(foundEntry)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(nExpectedAmount, fEscrowFee);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight,precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);
	}
    CTransaction fundingTx;
	if (!GetSyscoinTransaction(vtxPos.front().nHeight, vtxPos.front().txHash, fundingTx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4527 - " + _("Failed to find escrow transaction"));
	if (!rawTx.empty() && !DecodeHexTx(fundingTx,rawTx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4528 - " + _("Could not decode external payment transaction"));
	unsigned int nOutMultiSig = 0;
	for(unsigned int i=0;i<fundingTx.vout.size();i++)
	{
		if(fundingTx.vout[i].nValue == nEscrowTotal)
		{
			nOutMultiSig = i;
			break;
		}
	}
	CAmount nAmount = fundingTx.vout[nOutMultiSig].nValue;
	if(nAmount != nEscrowTotal)
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4529 - " + _("Expected amount of escrow does not match what is held in escrow. Expected amount: ") +  boost::lexical_cast<string>(nEscrowTotal));
	}
	vector<unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	COutPoint outPoint;
	int numResults=0;
	// who is initiating release arbiter or buyer?
	if(role == "arbiter")
	{
		if(!IsMyAlias(arbiterAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4530 - " + _("You must own the arbiter alias to complete this transaction"));
		numResults  = aliasunspent(arbiterAliasLatest.vchAlias, outPoint);		
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		CScript scriptPubKeyOrig;
		scriptPubKeyOrig = arbiterScript;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyOrig;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
	}
	else if(role == "buyer")
	{
		if(!IsMyAlias(buyerAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4531 - " + _("You must own the buyer alias to complete this transaction"));
		
		numResults  = aliasunspent(buyerAliasLatest.vchAlias, outPoint);
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		CScript scriptPubKeyOrig;
		scriptPubKeyOrig = buyerScript;
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyOrig;
		vchLinkAlias = buyerAliasLatest.vchAlias;
		theAlias = buyerAliasLatest;
	}

	// create a raw tx that sends escrow amount to seller and collateral to buyer
    // inputs buyer txHash
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createTxInputsArray(UniValue::VARR);
	UniValue createTxInputUniValue(UniValue::VOBJ);
	UniValue createAddressUniValue(UniValue::VOBJ);
	createTxInputUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
	createTxInputUniValue.push_back(Pair("vout", (int)nOutMultiSig));
	createTxInputsArray.push_back(createTxInputUniValue);
	if(role == "arbiter")
	{
		// if linked offer send commission to affiliate
		if(!theOffer.vchLinkOffer.empty())
		{
			if(nExpectedCommissionAmount > 0)
				createAddressUniValue.push_back(Pair(resellerAddressPayment.ToString(), ValueFromAmount(nExpectedCommissionAmount)));
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount-nExpectedCommissionAmount)));
		}
		else
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount)));
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(nEscrowFee)));
	}
	else if(role == "buyer")
	{
		// if linked offer send commission to affiliate
		if(!theOffer.vchLinkOffer.empty())
		{
			if(nExpectedCommissionAmount > 0)
				createAddressUniValue.push_back(Pair(resellerAddressPayment.ToString(), ValueFromAmount(nExpectedCommissionAmount)));
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount-nExpectedCommissionAmount)));
		}
		else
			createAddressUniValue.push_back(Pair(sellerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount)));
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nEscrowFee)));
	}

	arrayCreateParams.push_back(createTxInputsArray);
	arrayCreateParams.push_back(createAddressUniValue);
	arrayCreateParams.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams.push_back(rawTx.empty());
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


	// Buyer/Arbiter signs it
	vector<string> strKeys;
	GetPrivateKeysFromScript(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()), strKeys);
	if(strKeys.empty())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4533 - " + _("No private keys found involved in this escrow"));

	string strEscrowScriptPubKey = HexStr(fundingTx.vout[nOutMultiSig].scriptPubKey.begin(), fundingTx.vout[nOutMultiSig].scriptPubKey.end());
 	UniValue arraySignParams(UniValue::VARR);
 	UniValue arraySignInputs(UniValue::VARR);
	UniValue arrayPrivateKeys(UniValue::VARR);

 	UniValue signUniValue(UniValue::VOBJ);
 	signUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
 	signUniValue.push_back(Pair("vout", (int)nOutMultiSig));
 	signUniValue.push_back(Pair("scriptPubKey", strEscrowScriptPubKey));
 	signUniValue.push_back(Pair("redeemScript", HexStr(escrow.vchRedeemScript)));
  	arraySignParams.push_back(createEscrowSpendingTx);
 	arraySignInputs.push_back(signUniValue);
 	arraySignParams.push_back(arraySignInputs);
	BOOST_FOREACH(const string& strKey, strKeys) {
		arrayPrivateKeys.push_back(strKey);
	}
	arraySignParams.push_back(arrayPrivateKeys);

	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", arraySignParams);
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


	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_RELEASE;
	escrow.rawTx = ParseHex(hex_str);
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.bPaymentAck = false;
	escrow.vchLinkAlias = vchLinkAlias;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

    CScript scriptPubKeyOrigSeller, scriptPubKeyOrigArbiter;

    scriptPubKeyOrigSeller << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigSeller += sellerScript;

	scriptPubKeyOrigArbiter << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigArbiter += arbiterScript;

	vector<CRecipient> vecSend;
	CRecipient recipientSeller;
	CreateRecipient(scriptPubKeyOrigSeller, recipientSeller);
	vecSend.push_back(recipientSeller);

	CRecipient recipientArbiter;
	CreateRecipient(scriptPubKeyOrigArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);




	SendMoneySyscoin(vecSend, recipientSeller.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, theAlias.multiSigInfo.vchAliases.size() > 0);
	UniValue res(UniValue::VARR);
	if(theAlias.multiSigInfo.vchAliases.size() > 0)
	{
		UniValue signParams(UniValue::VARR);
		signParams.push_back(EncodeHexTx(wtx));
		const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
		const UniValue& so = resSign.get_obj();
		string hex_str = "";

		const UniValue& hex_value = find_value(so, "hex");
		if (hex_value.isStr())
			hex_str = hex_value.get_str();
		const UniValue& complete_value = find_value(so, "complete");
		bool bComplete = false;
		if (complete_value.isBool())
			bComplete = complete_value.get_bool();
		if(bComplete)
		{
			res.push_back(wtx.GetHash().GetHex());
		}
		else
		{
			res.push_back(hex_str);
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
	}
	return res;
}
UniValue escrowacknowledge(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
		"escrowacknowledge <escrow guid>\n"
                        "Acknowledge escrow payment as seller of offer. Deducts qty of offer and increases number of sold inventory.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);


	EnsureWalletIsUnlocked();
	
    // this is a syscoin transaction
    CWalletTx wtx;
    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4536 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest, resellerAlias, resellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx, buyeraliastx, arbiteraliastx, reselleraliastx;
	bool isExpired;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment, resellerAddressPayment;
	CScript sellerScript;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		sellerAlias.nHeight = vtxPos.front().nHeight;
		sellerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(sellerAlias, &sellerAddressPayment, sellerScript);
	}
	CScript buyerScript;
	if(GetTxAndVtxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, aliasVtxPos, isExpired, true))
	{
		buyerAlias.nHeight = vtxPos.front().nHeight;
		buyerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(buyerAlias, &buyerAddressPayment, buyerScript);
	}
	CScript arbiterScript;
	if(GetTxAndVtxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, aliasVtxPos, isExpired, true))
	{
		arbiterAlias.nHeight = vtxPos.front().nHeight;
		arbiterAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(arbiterAlias, &arbiterAddressPayment, arbiterScript);
	}

	COffer theOffer, linkOffer;
	CTransaction txOffer;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( escrow.vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4537 - " + _("Could not find an offer with this identifier"));
	theOffer.nHeight = vtxPos.front().nAcceptHeight;
	theOffer.GetOfferFromList(offerVtxPos);
	if(!theOffer.vchLinkOffer.empty())
	{
		vector<COffer> offerLinkVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, txOffer, offerLinkVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4538 - " + _("Could not find an offer with this identifier"));
		linkOffer.nHeight = vtxPos.front().nAcceptHeight;
		linkOffer.GetOfferFromList(offerLinkVtxPos);

		if(GetTxAndVtxOfAlias(theOffer.vchAlias, resellerAliasLatest, reselleraliastx, aliasVtxPos, isExpired, true))
		{
			resellerAlias.nHeight = vtxPos.front().nHeight;
			resellerAlias.GetAliasFromList(aliasVtxPos);
			GetAddress(resellerAlias, &resellerAddressPayment);
		}

	}
	
	if(!IsMyAlias(sellerAliasLatest))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4539 - " + _("You must own the seller alias to complete this transaction"));
	COutPoint outPoint;
	int numResults  = aliasunspent(sellerAliasLatest.vchAlias, outPoint);	
	const CWalletTx *wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
	CScript scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += sellerScript;

	escrow.ClearEscrow();
	escrow.bPaymentAck = true;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.vchLinkAlias = sellerAliasLatest.vchAlias;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

    CScript scriptPubKeyOrigBuyer, scriptPubKeyOrigArbiter;

    scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigBuyer += buyerScript;

	scriptPubKeyOrigArbiter << CScript::EncodeOP_N(OP_ESCROW_ACTIVATE) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigArbiter += arbiterScript;

	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient recipientArbiter;
	CreateRecipient(scriptPubKeyOrigArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, sellerAliasLatest.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);




	SendMoneySyscoin(vecSend, recipientBuyer.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, sellerAliasLatest.multiSigInfo.vchAliases.size() > 0);
	UniValue res(UniValue::VARR);
	if(sellerAliasLatest.multiSigInfo.vchAliases.size() > 0)
	{
		UniValue signParams(UniValue::VARR);
		signParams.push_back(EncodeHexTx(wtx));
		const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
		const UniValue& so = resSign.get_obj();
		string hex_str = "";

		const UniValue& hex_value = find_value(so, "hex");
		if (hex_value.isStr())
			hex_str = hex_value.get_str();
		const UniValue& complete_value = find_value(so, "complete");
		bool bComplete = false;
		if (complete_value.isBool())
			bComplete = complete_value.get_bool();
		if(bComplete)
		{
			res.push_back(wtx.GetHash().GetHex());
		}
		else
		{
			res.push_back(hex_str);
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
	}
	return res;

}
UniValue escrowclaimrelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 2 || params.size() < 1)
        throw runtime_error(
		"escrowclaimrelease <escrow guid> [rawTx]\n"
                        "Claim escrow funds released from buyer or arbiter using escrowrelease. Enter in rawTx if this is an external payment release.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx;
	if(params.size() >= 2)
		rawTx = params[1].get_str();

	EnsureWalletIsUnlocked();
	UniValue ret(UniValue::VARR);
    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4540 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias, sellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx;
	bool isExpired;
	CSyscoinAddress sellerAddressPayment;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		sellerAlias.nHeight = vtxPos.front().nHeight;
		sellerAlias.GetAliasFromList(aliasVtxPos);
	}
	
	COffer theOffer, linkOffer;
	CTransaction txOffer;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( escrow.vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4541 - " + _("Could not find an offer with this identifier"));
	theOffer.nHeight = vtxPos.front().nAcceptHeight;
	theOffer.GetOfferFromList(offerVtxPos);
	if(!theOffer.vchLinkOffer.empty())
	{
		vector<COffer> offerLinkVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, txOffer, offerLinkVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4542 - " + _("Could not find an offer with this identifier"));
		linkOffer.nHeight = vtxPos.front().nAcceptHeight;
		linkOffer.GetOfferFromList(offerLinkVtxPos);
	}
	CAmount nCommission;
	COfferLinkWhitelistEntry foundEntry;
	if(theOffer.vchLinkOffer.empty())
	{
		theOffer.linkWhitelist.GetLinkEntryByHash(escrow.vchBuyerAlias, foundEntry);
		nCommission = 0;
	}
	else
	{
		linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);
		nCommission = theOffer.GetPrice() - linkOffer.GetPrice(foundEntry);
	}
	CAmount nExpectedCommissionAmount, nExpectedAmount, nEscrowFee, nEscrowTotal;
	int nFeePerByte;
	int precision = 2;
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		string paymentOptionStr = GetPaymentOptionsString(escrow.nPaymentOption);
		nExpectedCommissionAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nCommission, vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		nExpectedAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), theOffer.GetPrice(foundEntry), vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(theOffer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);	
		nEscrowFee = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nEscrowFee, vtxPos.front().nAcceptHeight, precision);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);	
	}
	else
	{
		nExpectedCommissionAmount = nCommission*escrow.nQty;
		nExpectedAmount = theOffer.GetPrice(foundEntry)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(nExpectedAmount, fEscrowFee);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight,precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);
	}
    CTransaction fundingTx;
	if (!GetSyscoinTransaction(vtxPos.front().nHeight, vtxPos.front().txHash, fundingTx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4543 - " + _("Failed to find escrow transaction"));
	if (!rawTx.empty() && !DecodeHexTx(fundingTx,rawTx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4544 - " + _("Could not decode external payment transaction"));
	unsigned int nOutMultiSig = 0;
	for(unsigned int i=0;i<fundingTx.vout.size();i++)
	{
		if(fundingTx.vout[i].nValue == nEscrowTotal)
		{
			nOutMultiSig = i;
			break;
		}
	}
	CAmount nAmount = fundingTx.vout[nOutMultiSig].nValue;
	if(nAmount != nEscrowTotal)
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4545 - " + _("Expected amount of escrow does not match what is held in escrow. Expected amount: ") +  boost::lexical_cast<string>(nEscrowTotal));
	}
	bool foundSellerPayment = false;
	bool foundCommissionPayment = false;
	bool foundFeePayment = false;
	UniValue arrayDecodeParams(UniValue::VARR);
	arrayDecodeParams.push_back(HexStr(escrow.rawTx));
	UniValue decodeRes;
	try
	{
		decodeRes = tableRPC.execute("decoderawtransaction", arrayDecodeParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!decodeRes.isObject())
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4546 - " + _("Could not decode escrow transaction: Invalid response from decoderawtransaction"));
	}
	const UniValue& decodeo = decodeRes.get_obj();
	const UniValue& vout_value = find_value(decodeo, "vout");
	if (!vout_value.isArray())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4547 - " + _("Could not decode escrow transaction: Cannot find VOUT from transaction"));
	const UniValue &vouts = vout_value.get_array();
    for (unsigned int idx = 0; idx < vouts.size(); idx++) {
        const UniValue& vout = vouts[idx];
		const UniValue &voutObj = vout.get_obj();
		const UniValue &voutValue = find_value(voutObj, "value");
		if(!voutValue.isNum())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4548 - " + _("Could not decode escrow transaction: Invalid VOUT value"));
		int64_t iVout = AmountFromValue(voutValue);
		UniValue scriptPubKeyValue = find_value(voutObj, "scriptPubKey");
		if(!scriptPubKeyValue.isObject())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4549 - " + _("Could not decode escrow transaction: Invalid scriptPubKey value"));
		const UniValue &scriptPubKeyValueObj = scriptPubKeyValue.get_obj();

		const UniValue &typeValue = find_value(scriptPubKeyValueObj, "type");
		const UniValue &addressesValue = find_value(scriptPubKeyValueObj, "addresses");
		if(!typeValue.isStr())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4550 - " + _("Could not decode escrow transaction: Invalid type"));
		if(!addressesValue.isArray())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4551 - " + _("Could not decode escrow transaction: Invalid addresses"));

		const UniValue &addresses = addressesValue.get_array();
		const UniValue& address = addresses[0];
		if(!address.isStr())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4552 - " + _("Could not decode escrow transaction: Invalid address"));
		string strAddress = address.get_str();
		if(typeValue.get_str() == "multisig")
		{
			const UniValue &reqSigsValue = find_value(scriptPubKeyValueObj, "reqSigs");
			if(!reqSigsValue.isNum())
				throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4553 - " + _("Could not decode escrow transaction: Invalid number of signatures"));
			vector<CPubKey> pubKeys;
			for (unsigned int idx = 0; idx < addresses.size(); idx++) {
				const UniValue& address = addresses[idx];
				if(!address.isStr())
					throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4554 - " + _("Could not decode escrow transaction: Invalid address"));
				CSyscoinAddress aliasAddress = CSyscoinAddress(address.get_str());
				if(aliasAddress.vchPubKey.empty())
					throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4555 - " + _("Could not decode escrow transaction: One or more of the multisig addresses do not refer to an alias"));
				CPubKey pubkey(aliasAddress.vchPubKey);
				pubKeys.push_back(pubkey);
			}
			CScript script = GetScriptForMultisig(reqSigsValue.get_int(), pubKeys);
			CScriptID innerID(script);
			CSyscoinAddress aliasAddress(innerID);
			strAddress = aliasAddress.ToString();
		}
		CSyscoinAddress aliasAddress(strAddress);
		// check arb fee is paid to arbiter or buyer
		if(!foundFeePayment)
		{
			if(aliasAddress.aliasName == stringFromVch(escrow.vchArbiterAlias) && iVout >= nEscrowFee)
				foundFeePayment = true;
		}
		if(!foundFeePayment)
		{
			if(aliasAddress.aliasName == stringFromVch(escrow.vchBuyerAlias) && iVout >= nEscrowFee)
				foundFeePayment = true;
		}
		if(!theOffer.vchLinkOffer.empty())
		{
			if(!foundCommissionPayment)
			{
				if(aliasAddress.aliasName == stringFromVch(escrow.vchLinkSellerAlias) && iVout >= nExpectedCommissionAmount)
				{
					foundCommissionPayment = true;
				}
			}
			if(!foundSellerPayment)
			{
				if(aliasAddress.aliasName == stringFromVch(escrow.vchSellerAlias) && iVout >= (nExpectedAmount-nExpectedCommissionAmount))
				{
					foundSellerPayment = true;
				}
			}
		}
		else if(!foundSellerPayment)
		{
			if(aliasAddress.aliasName == stringFromVch(escrow.vchSellerAlias) && iVout >= nExpectedAmount)
			{
				foundSellerPayment = true;
			}
		}
	}
	if(!foundSellerPayment)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4556 - " + _("Expected payment amount not found in escrow"));
	if(!foundFeePayment)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4557 - " + _("Expected fee payment to arbiter or buyer not found in escrow"));
	if(!theOffer.vchLinkOffer.empty() && !foundCommissionPayment && nExpectedCommissionAmount > 0)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4558 - " + _("Expected commission to affiliate not found in escrow"));

    // Seller signs it
	vector<string> strKeys;
	GetPrivateKeysFromScript(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()), strKeys);
	if(strKeys.empty())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4559 - " + _("No private keys found involved in this escrow"));

	string strEscrowScriptPubKey = HexStr(fundingTx.vout[nOutMultiSig].scriptPubKey.begin(), fundingTx.vout[nOutMultiSig].scriptPubKey.end());
 	UniValue arraySignParams(UniValue::VARR);
 	UniValue arraySignInputs(UniValue::VARR);
	UniValue arrayPrivateKeys(UniValue::VARR);

 	UniValue signUniValue(UniValue::VOBJ);
 	signUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
 	signUniValue.push_back(Pair("vout", (int)nOutMultiSig));
 	signUniValue.push_back(Pair("scriptPubKey", strEscrowScriptPubKey));
 	signUniValue.push_back(Pair("redeemScript", HexStr(escrow.vchRedeemScript)));
  	arraySignParams.push_back(HexStr(escrow.rawTx));
 	arraySignInputs.push_back(signUniValue);
 	arraySignParams.push_back(arraySignInputs);
	BOOST_FOREACH(const string& strKey, strKeys) {
		arrayPrivateKeys.push_back(strKey);
	}
	arraySignParams.push_back(arrayPrivateKeys);
	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", arraySignParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
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
	return ret;


}
UniValue escrowcompleterelease(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
		"escrowcompleterelease <escrow guid> <rawtx> \n"
                         "Completes an escrow release by creating the escrow complete release transaction on syscoin blockchain.\n"
						 "<rawtx> Raw syscoin escrow transaction. Enter the raw tx result from escrowclaimrelease.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CTransaction myRawTx;
	DecodeHexTx(myRawTx,rawTx);
    // this is a syscoin transaction
    CWalletTx wtx;

	EnsureWalletIsUnlocked();

    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4562 - " + _("Could not find a escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx, buyeraliastx, arbiteraliastx, reselleraliastx;
	bool isExpired;
	CSyscoinAddress arbiterPaymentAddress;
	CScript arbiterScript;
	if(GetTxAndVtxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript);
	}

	aliasVtxPos.clear();
	CSyscoinAddress buyerPaymentAddress;
	CScript buyerScript;
	if(GetTxAndVtxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript);
	}
	aliasVtxPos.clear();
	CSyscoinAddress sellerPaymentAddress;
	CScript sellerScript;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript);
	}


	const CWalletTx *wtxAliasIn = NULL;
	vector<unsigned char> vchLinkAlias;
	CScript scriptPubKeyAlias;
	if(!IsMyAlias(sellerAliasLatest))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4563 - " + _("You must own the seller alias to complete this transaction"));
	COutPoint outPoint;
	int numResults  = aliasunspent(sellerAliasLatest.vchAlias, outPoint);		
	wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);

	scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += sellerScript;
	vchLinkAlias = sellerAliasLatest.vchAlias;


	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.vchLinkAlias = vchLinkAlias;
	escrow.redeemTxId = myRawTx.GetHash();

    CScript scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyArbiter;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
    scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyBuyer += buyerScript;
    scriptPubKeySeller << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeySeller += sellerScript;
    scriptPubKeyArbiter << CScript::EncodeOP_N(OP_ESCROW_RELEASE) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyArbiter += arbiterScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientSeller, recipientArbiter;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);
	CreateRecipient(scriptPubKeySeller, recipientSeller);
	vecSend.push_back(recipientSeller);
	CreateRecipient(scriptPubKeyArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, sellerAliasLatest.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);



	SendMoneySyscoin(vecSend, recipientBuyer.nAmount+recipientSeller.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, true);
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

	const UniValue& hex_value = find_value(so, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(!bComplete)
	{
		res.push_back(hex_str);
		res.push_back("false");
		return res;
	}
	res.push_back(wtx.GetHash().GetHex());
	return res;
}
UniValue escrowrefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 3 || params.size() < 2)
        throw runtime_error(
		"escrowrefund <escrow guid> <user role> [rawTx]\n"
                        "Refunds escrow funds back to buyer, buyer needs to sign the output transaction and send to the network. User role represents either 'seller' or 'arbiter'. Enter in rawTx if this is an external payment refund.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string role = params[1].get_str();
	string rawTx;
	if(params.size() >= 3)
		rawTx = params[2].get_str();
    // this is a syscoin transaction
    CWalletTx wtx;

	EnsureWalletIsUnlocked();

     // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4564 - " + _("Could not find a escrow with this key"));

 
	CAliasIndex sellerAlias, sellerAliasLatest, buyerAlias, buyerAliasLatest, arbiterAlias, arbiterAliasLatest, resellerAlias, resellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx, buyeraliastx, arbiteraliastx, reselleraliastx;
	bool isExpired;
	CSyscoinAddress arbiterAddressPayment, buyerAddressPayment, sellerAddressPayment, resellerAddressPayment;
	CScript buyerScript, arbiterScript, sellerScript;
	if(GetTxAndVtxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, aliasVtxPos, isExpired, true))
	{
		arbiterAlias.nHeight = vtxPos.front().nHeight;
		arbiterAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(arbiterAliasLatest, &arbiterAddressPayment, arbiterScript, escrow.nPaymentOption);
	}

	aliasVtxPos.clear();
	if(GetTxAndVtxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, aliasVtxPos, isExpired, true))
	{
		buyerAlias.nHeight = vtxPos.front().nHeight;
		buyerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(buyerAliasLatest, &buyerAddressPayment, buyerScript, escrow.nPaymentOption);
	}
	aliasVtxPos.clear();
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		sellerAlias.nHeight = vtxPos.front().nHeight;
		sellerAlias.GetAliasFromList(aliasVtxPos);
		GetAddress(sellerAliasLatest, &sellerAddressPayment, sellerScript, escrow.nPaymentOption);
	}

	COffer theOffer, linkOffer;
	CTransaction txOffer;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( escrow.vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4565 - " + _("Could not find an offer with this identifier"));
	theOffer.nHeight = vtxPos.front().nAcceptHeight;
	theOffer.GetOfferFromList(offerVtxPos);
	if(!theOffer.vchLinkOffer.empty())
	{
		vector<COffer> offerLinkVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, txOffer, offerLinkVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4566 - " + _("Could not find an offer with this identifier"));
		linkOffer.nHeight = vtxPos.front().nAcceptHeight;
		linkOffer.GetOfferFromList(offerLinkVtxPos);
	}
	CAmount nCommission;
	COfferLinkWhitelistEntry foundEntry;
	if(theOffer.vchLinkOffer.empty())
	{
		theOffer.linkWhitelist.GetLinkEntryByHash(buyerAlias.vchAlias, foundEntry);
		nCommission = 0;
	}
	else
	{
		linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);
		nCommission = theOffer.GetPrice() - linkOffer.GetPrice(foundEntry);
	}
	CAmount nExpectedCommissionAmount, nExpectedAmount, nEscrowFee, nEscrowTotal;
	int nFeePerByte;
	int precision = 2;
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		string paymentOptionStr = GetPaymentOptionsString(escrow.nPaymentOption);
		nExpectedCommissionAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nCommission, vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		nExpectedAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), theOffer.GetPrice(foundEntry), vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(theOffer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);	
		nEscrowFee = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nEscrowFee, vtxPos.front().nAcceptHeight, precision);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);	
	}
	else
	{
		nExpectedCommissionAmount = nCommission*escrow.nQty;
		nExpectedAmount = theOffer.GetPrice(foundEntry)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(nExpectedAmount, fEscrowFee);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight,precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);
	}
    CTransaction fundingTx;
	if (!GetSyscoinTransaction(vtxPos.front().nHeight, vtxPos.front().txHash, fundingTx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4567 - " + _("Failed to find escrow transaction"));
	if (!rawTx.empty() && !DecodeHexTx(fundingTx,rawTx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4568 - " + _("Could not decode external payment transaction"));
	unsigned int nOutMultiSig = 0;
	for(unsigned int i=0;i<fundingTx.vout.size();i++)
	{
		if(fundingTx.vout[i].nValue == nEscrowTotal)
		{
			nOutMultiSig = i;
			break;
		}
	}
	CAmount nAmount = fundingTx.vout[nOutMultiSig].nValue;
	if(nAmount != nEscrowTotal)
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4569 - " + _("Expected amount of escrow does not match what is held in escrow. Expected amount: ") +  boost::lexical_cast<string>(nEscrowTotal));
	}
	const CWalletTx *wtxAliasIn = NULL;
	vector<unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	CScript scriptPubKeyAlias;
	COutPoint outPoint;
	int numResults = 0;
	// who is initiating release arbiter or seller?
	if(role == "arbiter")
	{
		if(!IsMyAlias(arbiterAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4570 - " + _("You must own the arbiter alias to complete this transaction"));
		numResults  = aliasunspent(arbiterAliasLatest.vchAlias, outPoint);		
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += arbiterScript;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
	}
	else if(role == "seller")
	{
		if(!IsMyAlias(sellerAlias))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4571 - " + _("You must own the seller alias to complete this transaction"));
	
		numResults  = aliasunspent(sellerAliasLatest.vchAlias, outPoint);		
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += sellerScript;
		vchLinkAlias = sellerAliasLatest.vchAlias;
		theAlias = sellerAliasLatest;
	}

	// refunds buyer from escrow
	UniValue arrayCreateParams(UniValue::VARR);
	UniValue createTxInputsArray(UniValue::VARR);
	UniValue createTxInputUniValue(UniValue::VOBJ);
	UniValue createAddressUniValue(UniValue::VOBJ);
	createTxInputUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
	createTxInputUniValue.push_back(Pair("vout", (int)nOutMultiSig));
	createTxInputsArray.push_back(createTxInputUniValue);
	if(role == "arbiter")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount)));
		createAddressUniValue.push_back(Pair(arbiterAddressPayment.ToString(), ValueFromAmount(nEscrowFee)));
	}
	else if(role == "seller")
	{
		createAddressUniValue.push_back(Pair(buyerAddressPayment.ToString(), ValueFromAmount(nExpectedAmount+nEscrowFee)));
	}
	arrayCreateParams.push_back(createTxInputsArray);
	arrayCreateParams.push_back(createAddressUniValue);
	arrayCreateParams.push_back(NullUniValue);
	// if external blockchain then we dont set the alias payments scriptpubkey
	arrayCreateParams.push_back(rawTx.empty());
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
	vector<string> strKeys;
	GetPrivateKeysFromScript(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()), strKeys);
	if(strKeys.empty())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4573 - " + _("No private keys found involved in this escrow"));

	string strEscrowScriptPubKey = HexStr(fundingTx.vout[nOutMultiSig].scriptPubKey.begin(), fundingTx.vout[nOutMultiSig].scriptPubKey.end());
 	UniValue arraySignParams(UniValue::VARR);
 	UniValue arraySignInputs(UniValue::VARR);
	UniValue arrayPrivateKeys(UniValue::VARR);

 	UniValue signUniValue(UniValue::VOBJ);
 	signUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
 	signUniValue.push_back(Pair("vout", (int)nOutMultiSig));
 	signUniValue.push_back(Pair("scriptPubKey", strEscrowScriptPubKey));
 	signUniValue.push_back(Pair("redeemScript", HexStr(escrow.vchRedeemScript)));
  	arraySignParams.push_back(createEscrowSpendingTx);
 	arraySignInputs.push_back(signUniValue);
 	arraySignParams.push_back(arraySignInputs);
	BOOST_FOREACH(const string& strKey, strKeys) {
		arrayPrivateKeys.push_back(strKey);
	}
	arraySignParams.push_back(arrayPrivateKeys);

	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", arraySignParams);
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

	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_REFUND;
	escrow.bPaymentAck = false;
	escrow.rawTx = ParseHex(hex_str);
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.vchLinkAlias = vchLinkAlias;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());

    CScript scriptPubKeyOrigBuyer, scriptPubKeyOrigArbiter;


    scriptPubKeyOrigBuyer << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigBuyer += buyerScript;

	scriptPubKeyOrigArbiter << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("0") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyOrigArbiter += arbiterScript;

	vector<CRecipient> vecSend;
	CRecipient recipientBuyer;
	CreateRecipient(scriptPubKeyOrigBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);

	CRecipient recipientArbiter;
	CreateRecipient(scriptPubKeyOrigArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);




	SendMoneySyscoin(vecSend, recipientBuyer.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, theAlias.multiSigInfo.vchAliases.size() > 0);
	UniValue res(UniValue::VARR);
	if(theAlias.multiSigInfo.vchAliases.size() > 0)
	{
		UniValue signParams(UniValue::VARR);
		signParams.push_back(EncodeHexTx(wtx));
		const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
		const UniValue& so = resSign.get_obj();
		string hex_str = "";

		const UniValue& hex_value = find_value(so, "hex");
		if (hex_value.isStr())
			hex_str = hex_value.get_str();
		const UniValue& complete_value = find_value(so, "complete");
		bool bComplete = false;
		if (complete_value.isBool())
			bComplete = complete_value.get_bool();
		if(bComplete)
		{
			res.push_back(wtx.GetHash().GetHex());
		}
		else
		{
			res.push_back(hex_str);
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
	}
	return res;
}
UniValue escrowclaimrefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() > 2 || params.size() < 1)
        throw runtime_error(
		"escrowclaimrefund <escrow guid> [rawTx]\n"
                        "Claim escrow funds released from seller or arbiter using escrowrefund. Enter in rawTx if this is an external payment refund.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx;
	if(params.size() >= 2)
		rawTx = params[1].get_str();

	EnsureWalletIsUnlocked();
    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4576 - " + _("Could not find a escrow with this key"));

	CAliasIndex sellerAlias, sellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx;
	CPubKey sellerKey;
	bool isExpired;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		sellerAlias.nHeight = vtxPos.front().nHeight;
		sellerAlias.GetAliasFromList(aliasVtxPos);
	}
 
	COffer theOffer, linkOffer;
	CTransaction txOffer;
	vector<COffer> offerVtxPos;
	if (!GetTxAndVtxOfOffer( escrow.vchOffer, theOffer, txOffer, offerVtxPos, true))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4577 - " + _("Could not find an offer with this identifier"));
	theOffer.nHeight = vtxPos.front().nAcceptHeight;
	theOffer.GetOfferFromList(offerVtxPos);
	if(!theOffer.vchLinkOffer.empty())
	{
		vector<COffer> offerLinkVtxPos;
		if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, txOffer, offerLinkVtxPos, true))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4578 - " + _("Could not find an offer with this identifier"));
		linkOffer.nHeight = vtxPos.front().nAcceptHeight;
		linkOffer.GetOfferFromList(offerLinkVtxPos);
	}

	CAmount nCommission;
	COfferLinkWhitelistEntry foundEntry;
	if(theOffer.vchLinkOffer.empty())
	{
		theOffer.linkWhitelist.GetLinkEntryByHash(escrow.vchBuyerAlias, foundEntry);
		nCommission = 0;
	}
	else
	{
		linkOffer.linkWhitelist.GetLinkEntryByHash(theOffer.vchAlias, foundEntry);
		nCommission = theOffer.GetPrice() - linkOffer.GetPrice(foundEntry);
	}
	CAmount nExpectedCommissionAmount, nExpectedAmount, nEscrowFee, nEscrowTotal;
	int nFeePerByte;
	int precision = 2;
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		string paymentOptionStr = GetPaymentOptionsString(escrow.nPaymentOption);
		nExpectedCommissionAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nCommission, vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		nExpectedAmount = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), theOffer.GetPrice(foundEntry), vtxPos.front().nAcceptHeight, precision)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(theOffer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);	
		nEscrowFee = convertSyscoinToCurrencyCode(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nEscrowFee, vtxPos.front().nAcceptHeight, precision);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), vtxPos.front().nAcceptHeight, precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);	
	}
	else
	{
		nExpectedCommissionAmount = nCommission*escrow.nQty;
		nExpectedAmount = theOffer.GetPrice(foundEntry)*escrow.nQty;
		float fEscrowFee = getEscrowFee(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight, precision);
		nEscrowFee = GetEscrowArbiterFee(nExpectedAmount, fEscrowFee);
		nFeePerByte = getFeePerByte(sellerAlias.vchAliasPeg, vchFromString("SYS"), vtxPos.front().nAcceptHeight,precision);
		nEscrowTotal =  nExpectedAmount + nEscrowFee + (nFeePerByte*400);
	}
    CTransaction fundingTx;
	if (!GetSyscoinTransaction(vtxPos.front().nHeight, vtxPos.front().txHash, fundingTx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4579 - " + _("Failed to find escrow transaction"));
	if (!rawTx.empty() && !DecodeHexTx(fundingTx,rawTx))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4580 - " + _("Could not decode external payment transaction"));
	unsigned int nOutMultiSig = 0;
	for(unsigned int i=0;i<fundingTx.vout.size();i++)
	{
		if(fundingTx.vout[i].nValue == nEscrowTotal)
		{
			nOutMultiSig = i;
			break;
		}
	}
	CAmount nAmount = fundingTx.vout[nOutMultiSig].nValue;
	if(nAmount != nEscrowTotal)
	{
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4581 - " + _("Expected amount of escrow does not match what is held in escrow. Expected amount: ") +  boost::lexical_cast<string>(nEscrowTotal));
	}

	// decode rawTx and check it pays enough and it pays to buyer appropriately
	// check that right amount is going to be sent to buyer
	UniValue arrayDecodeParams(UniValue::VARR);
	arrayDecodeParams.push_back(HexStr(escrow.rawTx));
	UniValue decodeRes;
	try
	{
		decodeRes = tableRPC.execute("decoderawtransaction", arrayDecodeParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!decodeRes.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4582 - " + _("Could not decode escrow transaction: Invalid response from decoderawtransaction"));
	bool foundRefundPayment = false;
	const UniValue& decodeo = decodeRes.get_obj();
	const UniValue& vout_value = find_value(decodeo, "vout");
	if (!vout_value.isArray())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4583 - " + _("Could not decode escrow transaction: Cannot find VOUT from transaction"));
	const UniValue &vouts = vout_value.get_array();
    for (unsigned int idx = 0; idx < vouts.size(); idx++) {
        const UniValue& vout = vouts[idx];
		const UniValue &voutObj = vout.get_obj();
		const UniValue &voutValue = find_value(voutObj, "value");
		if(!voutValue.isNum())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4584 - " + _("Could not decode escrow transaction: Invalid VOUT value"));
		int64_t iVout = AmountFromValue(voutValue);
		UniValue scriptPubKeyValue = find_value(voutObj, "scriptPubKey");
		if(!scriptPubKeyValue.isObject())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4585 - " + _("Could not decode escrow transaction: Invalid scriptPubKey value"));
		const UniValue &scriptPubKeyValueObj = scriptPubKeyValue.get_obj();
		const UniValue &typeValue = find_value(scriptPubKeyValueObj, "type");
		const UniValue &addressesValue = find_value(scriptPubKeyValueObj, "addresses");
		if(!typeValue.isStr())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4586 - " + _("Could not decode escrow transaction: Invalid type"));
		if(!addressesValue.isArray())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4587 - " + _("Could not decode escrow transaction: Invalid addresses"));

		const UniValue &addresses = addressesValue.get_array();
		const UniValue& address = addresses[0];
		if(!address.isStr())
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4588 - " + _("Could not decode escrow transaction: Invalid address"));
		string strAddress = address.get_str();
		if(typeValue.get_str() == "multisig")
		{
			const UniValue &reqSigsValue = find_value(scriptPubKeyValueObj, "reqSigs");
			if(!reqSigsValue.isNum())
				throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4589 - " + _("Could not decode escrow transaction: Invalid number of signatures"));
			vector<CPubKey> pubKeys;
			for (unsigned int idx = 0; idx < addresses.size(); idx++) {
				const UniValue& address = addresses[idx];
				if(!address.isStr())
					throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4590 - " + _("Could not decode escrow transaction: Invalid address"));
				CSyscoinAddress aliasAddress = CSyscoinAddress(address.get_str());
				if(aliasAddress.vchPubKey.empty())
					throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4591 - " + _("Could not decode escrow transaction: One or more of the multisig addresses do not refer to an alias"));
				CPubKey pubkey(aliasAddress.vchPubKey);
				pubKeys.push_back(pubkey);
			}
			CScript script = GetScriptForMultisig(reqSigsValue.get_int(), pubKeys);
			CScriptID innerID(script);
			CSyscoinAddress aliasAddress(innerID);
			strAddress = aliasAddress.ToString();
		}
		if(!foundRefundPayment)
		{
			CSyscoinAddress address(strAddress);
			if(address.aliasName == stringFromVch(escrow.vchBuyerAlias) && iVout >= nExpectedAmount)
				foundRefundPayment = true;
		}

	}
	if(!foundRefundPayment)
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4592 - " + _("Expected refund amount not found"));

    // Buyer signs it
	string strEscrowScriptPubKey = HexStr(fundingTx.vout[nOutMultiSig].scriptPubKey.begin(), fundingTx.vout[nOutMultiSig].scriptPubKey.end());
 	UniValue arraySignParams(UniValue::VARR);
 	UniValue arraySignInputs(UniValue::VARR);
	UniValue arrayPrivateKeys(UniValue::VARR);

	vector<string> strKeys;
	GetPrivateKeysFromScript(CScript(escrow.vchRedeemScript.begin(), escrow.vchRedeemScript.end()), strKeys);
	if(strKeys.empty())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4593 - " + _("No private keys found involved in this escrow"));

 	UniValue signUniValue(UniValue::VOBJ);
 	signUniValue.push_back(Pair("txid", fundingTx.GetHash().ToString()));
 	signUniValue.push_back(Pair("vout", (int)nOutMultiSig));
 	signUniValue.push_back(Pair("scriptPubKey", strEscrowScriptPubKey));
 	signUniValue.push_back(Pair("redeemScript", HexStr(escrow.vchRedeemScript)));
  	arraySignParams.push_back(HexStr(escrow.rawTx));
 	arraySignInputs.push_back(signUniValue);
 	arraySignParams.push_back(arraySignInputs);
	BOOST_FOREACH(const string& strKey, strKeys) {
		arrayPrivateKeys.push_back(strKey);
	}
	arraySignParams.push_back(arrayPrivateKeys);
	UniValue resSign;
	try
	{
		resSign = tableRPC.execute("signrawtransaction", arraySignParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error(find_value(objError, "message").get_str());
	}
	if (!resSign.isObject())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4594 - " + _("Could not sign escrow transaction: Invalid response from signrawtransaction"));

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
	return ret;
}
UniValue escrowcompleterefund(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
		"escrowcompleterefund <escrow guid> <rawtx> \n"
                         "Completes an escrow refund by creating the escrow complete refund transaction on syscoin blockchain.\n"
						 "<rawtx> Raw syscoin escrow transaction. Enter the raw tx result from escrowclaimrefund.\n"
                        + HelpRequiringPassphrase());
    // gather & validate inputs
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	string rawTx = params[1].get_str();
	CTransaction myRawTx;
	DecodeHexTx(myRawTx,rawTx);
    // this is a syscoin transaction
    CWalletTx wtx;

	EnsureWalletIsUnlocked();

    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
	vector<CEscrow> vtxPos;
    if (!GetTxAndVtxOfEscrow( vchEscrow,
		escrow, tx, vtxPos))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4596 - " + _("Could not find a escrow with this key"));

	bool extPayment = false;
	if (escrow.nPaymentOption != PAYMENTOPTION_SYS)
		extPayment = true;

	CAliasIndex sellerAliasLatest, buyerAliasLatest, arbiterAliasLatest, resellerAliasLatest;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction selleraliastx, buyeraliastx, arbiteraliastx, reselleraliastx;
	bool isExpired;
	CSyscoinAddress arbiterPaymentAddress;
	CScript arbiterScript, buyerScript, sellerScript;
	if(GetTxAndVtxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(arbiterAliasLatest, &arbiterPaymentAddress, arbiterScript);
	}

	aliasVtxPos.clear();
	CSyscoinAddress buyerPaymentAddress;
	if(GetTxAndVtxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(buyerAliasLatest, &buyerPaymentAddress, buyerScript);
	}
	aliasVtxPos.clear();
	CSyscoinAddress sellerPaymentAddress;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, aliasVtxPos, isExpired, true))
	{
		GetAddress(sellerAliasLatest, &sellerPaymentAddress, sellerScript);
	}


	string strPrivateKey ;
	const CWalletTx *wtxAliasIn = NULL;
	vector<unsigned char> vchLinkAlias;
	CScript scriptPubKeyAlias;
	if(!IsMyAlias(buyerAliasLatest))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4597 - " + _("You must own the buyer alias to complete this transaction"));
	COutPoint outPoint;
	int numResults  = aliasunspent(buyerAliasLatest.vchAlias, outPoint);		
	wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
	scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += buyerScript;
	vchLinkAlias = buyerAliasLatest.vchAlias;


	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.vchLinkAlias = vchLinkAlias;
	escrow.redeemTxId = myRawTx.GetHash();

    CScript scriptPubKeyBuyer, scriptPubKeySeller, scriptPubKeyArbiter;

	vector<unsigned char> data;
	escrow.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());

    vector<unsigned char> vchHashEscrow = vchFromValue(hash.GetHex());
    scriptPubKeyBuyer << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyBuyer += buyerScript;
    scriptPubKeySeller << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeySeller += sellerScript;
    scriptPubKeyArbiter << CScript::EncodeOP_N(OP_ESCROW_REFUND) << vchEscrow << vchFromString("1") << vchHashEscrow << OP_2DROP << OP_2DROP;
    scriptPubKeyArbiter += arbiterScript;
	vector<CRecipient> vecSend;
	CRecipient recipientBuyer, recipientSeller, recipientArbiter;
	CreateRecipient(scriptPubKeyBuyer, recipientBuyer);
	vecSend.push_back(recipientBuyer);
	CreateRecipient(scriptPubKeySeller, recipientSeller);
	vecSend.push_back(recipientSeller);
	CreateRecipient(scriptPubKeyArbiter, recipientArbiter);
	vecSend.push_back(recipientArbiter);

	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, buyerAliasLatest.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);



	SendMoneySyscoin(vecSend, recipientBuyer.nAmount+recipientSeller.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, true);
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

	const UniValue& hex_value = find_value(so, "hex");
	if (hex_value.isStr())
		hex_str = hex_value.get_str();
	const UniValue& complete_value = find_value(so, "complete");
	bool bComplete = false;
	if (complete_value.isBool())
		bComplete = complete_value.get_bool();
	if(!bComplete)
	{
		res.push_back(hex_str);
		res.push_back("false");
		return res;
	}
	res.push_back(wtx.GetHash().GetHex());
	return res;
}
UniValue escrowfeedback(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 6)
        throw runtime_error(
		"escrowfeedback <escrow guid> <user role> <feedbackprimary> <ratingprimary> <feedbacksecondary> <ratingasecondary>\n"
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
    // this is a syscoin transaction
    CWalletTx wtx;

	EnsureWalletIsUnlocked();

    // look for a transaction with this key
    CTransaction tx;
	CEscrow escrow;
    if (!GetTxOfEscrow( vchEscrow,
		escrow, tx))
        throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4598 - " + _("Could not find a escrow with this key"));

	CAliasIndex arbiterAliasLatest, buyerAliasLatest, sellerAliasLatest, resellerAliasLatest;
	CTransaction arbiteraliastx, selleraliastx, reselleraliastx, buyeraliastx;
	CScript buyerScript, sellerScript, arbiterScript, resellerScript;
	GetTxOfAlias(escrow.vchArbiterAlias, arbiterAliasLatest, arbiteraliastx, true);
	CSyscoinAddress arbiterAddress;
	GetAddress(arbiterAliasLatest, &arbiterAddress, arbiterScript);

	GetTxOfAlias(escrow.vchBuyerAlias, buyerAliasLatest, buyeraliastx, true);
	CSyscoinAddress buyerAddress;
	GetAddress(buyerAliasLatest, &buyerAddress, buyerScript);

	GetTxOfAlias(escrow.vchSellerAlias, sellerAliasLatest, selleraliastx, true);
	CSyscoinAddress sellerAddress;
	GetAddress(sellerAliasLatest, &sellerAddress, sellerScript);
	
	GetTxOfAlias(escrow.vchLinkSellerAlias, resellerAliasLatest, reselleraliastx, true);
	CSyscoinAddress resellerAddress;
	GetAddress(resellerAliasLatest, &resellerAddress, resellerScript);

	vector <unsigned char> vchLinkAlias;
	CAliasIndex theAlias;
	CScript scriptPubKeyAlias;
	COutPoint outPoint;
	int numResults=0;
	const CWalletTx *wtxAliasIn = NULL;
	if(role == "buyer")
	{
		if(!IsMyAlias(buyerAliasLatest))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4599 - " + _("You must own the buyer alias to complete this transaction"));
		
		numResults  = aliasunspent(buyerAliasLatest.vchAlias, outPoint);			
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << buyerAliasLatest.vchAlias << buyerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += buyerScript;
		vchLinkAlias = buyerAliasLatest.vchAlias;
		theAlias = buyerAliasLatest;
		if(!resellerAliasLatest.IsNull())
			sellerAddress = resellerAddress;
	}
	else if(role == "seller")
	{
		if(!IsMyAlias(sellerAliasLatest))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4600 - " + _("You must own the seller alias to complete this transaction"));
		
		numResults  = aliasunspent(sellerAliasLatest.vchAlias, outPoint);		
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << sellerAliasLatest.vchAlias << sellerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += sellerScript;
		vchLinkAlias = sellerAliasLatest.vchAlias;
		theAlias = sellerAliasLatest;
	}
	else if(role == "reseller")
	{
		if(!IsMyAlias(resellerAliasLatest))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4601 - " + _("You must own the reseller alias to complete this transaction"));
		
		numResults  = aliasunspent(resellerAliasLatest.vchAlias, outPoint);		
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << resellerAliasLatest.vchAlias << resellerAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += resellerScript;
		vchLinkAlias = resellerAliasLatest.vchAlias;
		theAlias = resellerAliasLatest;
		sellerAddress = resellerAddress;
	}
	else if(role == "arbiter")
	{
		if(!IsMyAlias(arbiterAliasLatest))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4602 - " + _("You must own the arbiter alias to complete this transaction"));
		
		numResults  = aliasunspent(arbiterAliasLatest.vchAlias, outPoint);			
		wtxAliasIn = pwalletMain->GetWalletTx(outPoint.hash);
		scriptPubKeyAlias  = CScript() << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << arbiterAliasLatest.vchAlias << arbiterAliasLatest.vchGUID << vchFromString("") << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += arbiterScript;
		vchLinkAlias = arbiterAliasLatest.vchAlias;
		theAlias = arbiterAliasLatest;
		if(!resellerAliasLatest.IsNull())
			sellerAddress = resellerAddress;
	}

	escrow.ClearEscrow();
	escrow.op = OP_ESCROW_COMPLETE;
	escrow.bPaymentAck = false;
	escrow.nHeight = chainActive.Tip()->nHeight;
	escrow.vchLinkAlias = vchLinkAlias;
	// buyer
	if(role == "buyer")
	{
		CFeedback sellerFeedback(FEEDBACKBUYER, FEEDBACKSELLER);
		sellerFeedback.vchFeedback = vchFeedbackPrimary;
		sellerFeedback.nRating = nRatingPrimary;
		sellerFeedback.nHeight = chainActive.Tip()->nHeight;
		CFeedback arbiterFeedback(FEEDBACKBUYER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;
		arbiterFeedback.nHeight = chainActive.Tip()->nHeight;
		escrow.feedback.push_back(arbiterFeedback);
		escrow.feedback.push_back(sellerFeedback);
	}
	// seller
	else if(role == "seller")
	{
		CFeedback buyerFeedback(FEEDBACKSELLER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		buyerFeedback.nHeight = chainActive.Tip()->nHeight;
		CFeedback arbiterFeedback(FEEDBACKSELLER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;
		arbiterFeedback.nHeight = chainActive.Tip()->nHeight;
		escrow.feedback.push_back(buyerFeedback);
		escrow.feedback.push_back(arbiterFeedback);
	}
	else if(role == "reseller")
	{
		CFeedback buyerFeedback(FEEDBACKSELLER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		buyerFeedback.nHeight = chainActive.Tip()->nHeight;
		CFeedback arbiterFeedback(FEEDBACKSELLER, FEEDBACKARBITER);
		arbiterFeedback.vchFeedback = vchFeedbackSecondary;
		arbiterFeedback.nRating = nRatingSecondary;
		arbiterFeedback.nHeight = chainActive.Tip()->nHeight;
		escrow.feedback.push_back(buyerFeedback);
		escrow.feedback.push_back(arbiterFeedback);
	}
	// arbiter
	else if(role == "arbiter")
	{
		CFeedback buyerFeedback(FEEDBACKARBITER, FEEDBACKBUYER);
		buyerFeedback.vchFeedback = vchFeedbackPrimary;
		buyerFeedback.nRating = nRatingPrimary;
		buyerFeedback.nHeight = chainActive.Tip()->nHeight;
		CFeedback sellerFeedback(FEEDBACKARBITER, FEEDBACKSELLER);
		sellerFeedback.vchFeedback = vchFeedbackSecondary;
		sellerFeedback.nRating = nRatingSecondary;
		sellerFeedback.nHeight = chainActive.Tip()->nHeight;
		escrow.feedback.push_back(buyerFeedback);
		escrow.feedback.push_back(sellerFeedback);
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
		vecSend.push_back(recipientSeller);
		vecSend.push_back(recipientArbiter);
	}
	// seller
	else if(role == "seller" || role == "reseller")
	{
		vecSend.push_back(recipientBuyer);
		vecSend.push_back(recipientArbiter);
	}
	// arbiter
	else if(role == "arbiter")
	{
		vecSend.push_back(recipientBuyer);
		vecSend.push_back(recipientSeller);
	}
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	vecSend.push_back(fee);




	SendMoneySyscoin(vecSend, recipientBuyer.nAmount+recipientSeller.nAmount+recipientArbiter.nAmount+fee.nAmount+aliasRecipient.nAmount, false, wtx, wtxAliasIn, outPoint.n, theAlias.multiSigInfo.vchAliases.size() > 0);
	UniValue res(UniValue::VARR);
	if(theAlias.multiSigInfo.vchAliases.size() > 0)
	{
		UniValue signParams(UniValue::VARR);
		signParams.push_back(EncodeHexTx(wtx));
		const UniValue &resSign = tableRPC.execute("syscoinsignrawtransaction", signParams);
		const UniValue& so = resSign.get_obj();
		string hex_str = "";

		const UniValue& hex_value = find_value(so, "hex");
		if (hex_value.isStr())
			hex_str = hex_value.get_str();
		const UniValue& complete_value = find_value(so, "complete");
		bool bComplete = false;
		if (complete_value.isBool())
			bComplete = complete_value.get_bool();
		if(bComplete)
		{
			res.push_back(wtx.GetHash().GetHex());
		}
		else
		{
			res.push_back(hex_str);
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
	}
	return res;
}
UniValue escrowinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("escrowinfo <guid>\n"
                "Show stored values of a single escrow and its .\n");

    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
	vector<CEscrow> vtxPos;

    UniValue oEscrow(UniValue::VOBJ);

	if (!pescrowdb->ReadEscrow(vchEscrow, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to read from escrow DB"));

	if(!BuildEscrowJson(vtxPos.back(), vtxPos.front(), oEscrow))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4605 - " + _("Could not find this escrow"));
    return oEscrow;
}
bool BuildEscrowJson(const CEscrow &escrow, const CEscrow &firstEscrow, UniValue& oEscrow, const string &strPrivKey)
{
	vector<CEscrow> vtxPos;
	if (!pescrowdb->ReadEscrow(escrow.vchEscrow, vtxPos) || vtxPos.empty())
		  return false;
	CTransaction tx;
	if (!GetSyscoinTransaction(escrow.nHeight, escrow.txHash, tx, Params().GetConsensus()))
		 return false;
    vector<vector<unsigned char> > vvch;
    int op, nOut;
    if (!DecodeEscrowTx(tx, op, nOut, vvch) )
        return false;
	CTransaction offertx;
	COffer offer, linkOffer;
	vector<COffer> offerVtxPos;
	GetTxAndVtxOfOffer(escrow.vchOffer, offer, offertx, offerVtxPos, true);
	offer.nHeight = firstEscrow.nAcceptHeight;
	offer.GetOfferFromList(offerVtxPos);
    string sHeight = strprintf("%llu", escrow.nHeight);

	string opName = escrowFromOp(escrow.op);
	CEscrow escrowOp(tx);
	if(escrowOp.bPaymentAck)
		opName += "("+_("acknowledged")+")";
	else if(!escrowOp.feedback.empty())
		opName += "("+_("feedback")+")";
	oEscrow.push_back(Pair("escrowtype", opName));

    oEscrow.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));
	string sTime;
	CBlockIndex *pindex = chainActive[escrow.nHeight];
	if (pindex) {
		sTime = strprintf("%llu", pindex->nTime);
	}
	float avgBuyerRating, avgSellerRating, avgArbiterRating;
	vector<CFeedback> buyerFeedBacks, sellerFeedBacks, arbiterFeedBacks;
	GetFeedback(buyerFeedBacks, avgBuyerRating, FEEDBACKBUYER, escrow.feedback);
	GetFeedback(sellerFeedBacks, avgSellerRating, FEEDBACKSELLER, escrow.feedback);
	GetFeedback(arbiterFeedBacks, avgArbiterRating, FEEDBACKARBITER, escrow.feedback);

	CAliasIndex theSellerAlias;
	CTransaction aliastx;
	bool isExpired = false;
	vector<CAliasIndex> aliasVtxPos;
	if(GetTxAndVtxOfAlias(escrow.vchSellerAlias, theSellerAlias, aliastx, aliasVtxPos, isExpired, true))
	{
		theSellerAlias.nHeight = firstEscrow.nHeight;
		theSellerAlias.GetAliasFromList(aliasVtxPos);
	}
	oEscrow.push_back(Pair("time", sTime));
	oEscrow.push_back(Pair("seller", stringFromVch(escrow.vchSellerAlias)));
	oEscrow.push_back(Pair("arbiter", stringFromVch(escrow.vchArbiterAlias)));
	oEscrow.push_back(Pair("buyer", stringFromVch(escrow.vchBuyerAlias)));
	oEscrow.push_back(Pair("offer", stringFromVch(escrow.vchOffer)));
	oEscrow.push_back(Pair("offerlink_seller", stringFromVch(escrow.vchLinkSellerAlias)));
	oEscrow.push_back(Pair("offertitle", stringFromVch(offer.sTitle)));
	oEscrow.push_back(Pair("quantity", strprintf("%d", escrow.nQty)));
	CAmount nExpectedAmount, nExpectedAmountExt, nEscrowFee, nEscrowFeeExt, nEscrowTotal;
	int nFeePerByte;
	int precision = 2;
	int tmpprecision = 2;
	int extprecision = 2;
	// if offer is not linked, look for a discount for the buyer
	COfferLinkWhitelistEntry foundEntry;
	if(offer.vchLinkOffer.empty())
		offer.linkWhitelist.GetLinkEntryByHash(escrow.vchBuyerAlias, foundEntry);

	CAmount nPricePerUnit = convertSyscoinToCurrencyCode(theSellerAlias.vchAliasPeg, offer.sCurrencyCode, offer.GetPrice(foundEntry), firstEscrow.nAcceptHeight, precision);
	nExpectedAmount = nPricePerUnit*escrow.nQty;
	
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		string paymentOptionStr = GetPaymentOptionsString(escrow.nPaymentOption);
		nExpectedAmountExt = convertSyscoinToCurrencyCode(theSellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), offer.GetPrice(foundEntry), firstEscrow.nAcceptHeight, extprecision)*escrow.nQty;
		float fEscrowFee = getEscrowFee(theSellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), firstEscrow.nAcceptHeight, tmpprecision);
		nEscrowFee = GetEscrowArbiterFee(offer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);	
		nEscrowFeeExt = convertSyscoinToCurrencyCode(theSellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), nEscrowFee, firstEscrow.nAcceptHeight, tmpprecision);
		nFeePerByte = getFeePerByte(theSellerAlias.vchAliasPeg, vchFromString(paymentOptionStr), firstEscrow.nAcceptHeight, tmpprecision);
		nEscrowTotal =  nExpectedAmountExt + nEscrowFeeExt + (nFeePerByte*400);	
	}
	else
	{
		float fEscrowFee = getEscrowFee(theSellerAlias.vchAliasPeg, vchFromString("SYS"), firstEscrow.nAcceptHeight, tmpprecision);
		nEscrowFee = GetEscrowArbiterFee(offer.GetPrice(foundEntry)*escrow.nQty, fEscrowFee);
		nFeePerByte = getFeePerByte(theSellerAlias.vchAliasPeg, vchFromString("SYS"), firstEscrow.nAcceptHeight,tmpprecision);
		nEscrowTotal =  (offer.GetPrice(foundEntry)*escrow.nQty) + nEscrowFee + (nFeePerByte*400);
	}

	
	if(nExpectedAmount == 0)
		oEscrow.push_back(Pair("price", "0"));
	else
		oEscrow.push_back(Pair("price", strprintf("%.*f", precision, ValueFromAmount(nPricePerUnit).get_real() )));
	
	oEscrow.push_back(Pair("systotal", (offer.GetPrice(foundEntry) * escrow.nQty)));
	if(escrow.nPaymentOption != PAYMENTOPTION_SYS)
	{
		oEscrow.push_back(Pair("sysfee", nEscrowFeeExt));
		oEscrow.push_back(Pair("fee", strprintf("%.*f", 8, ValueFromAmount(nEscrowFeeExt).get_real() )));
		oEscrow.push_back(Pair("total", strprintf("%.*f", extprecision, ValueFromAmount(nExpectedAmountExt).get_real() )));
		oEscrow.push_back(Pair("totalwithfee", nEscrowTotal));
	}
	else
	{
		oEscrow.push_back(Pair("sysfee", nEscrowFee));
		oEscrow.push_back(Pair("fee", strprintf("%.*f", 8, ValueFromAmount(nEscrowFee).get_real() )));
		oEscrow.push_back(Pair("total", strprintf("%.*f", precision, ValueFromAmount(nExpectedAmount).get_real() )));
		oEscrow.push_back(Pair("totalwithfee", nEscrowTotal));
	}
	

	oEscrow.push_back(Pair("currency", stringFromVch(offer.sCurrencyCode)));


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
    oEscrow.push_back(Pair("paymentoption_display", GetPaymentOptionsString(escrow.nPaymentOption)));
	oEscrow.push_back(Pair("redeem_txid", strRedeemTxId));
    oEscrow.push_back(Pair("txid", escrow.txHash.GetHex()));
    oEscrow.push_back(Pair("height", sHeight));
	string strMessage = string("");
	if(!DecryptMessage(theSellerAlias, escrow.vchPaymentMessage, strMessage, strPrivKey))
		strMessage = _("Encrypted for owner of offer");
	oEscrow.push_back(Pair("pay_message", strMessage));
	int64_t expired_time = GetEscrowExpiration(escrow);
	int expired = 0;
    if(expired_time <= chainActive.Tip()->nTime)
	{
		expired = 1;
	}
	bool escrowRelease = false;
	bool escrowRefund = false;
	if(escrow.op == OP_ESCROW_COMPLETE)
	{
		for(unsigned int i = vtxPos.size() - 1; i >= 0;i--)
		{
			if(vtxPos[i].op == OP_ESCROW_RELEASE)
			{
				escrowRelease = true;
				break;
			}
			else if(vtxPos[i].op == OP_ESCROW_REFUND)
			{
				escrowRefund = true;
				break;
			}
		}
	}
	string status = "unknown";
	if(escrow.op == OP_ESCROW_ACTIVATE)
		status = "in escrow";
	else if(escrow.op == OP_ESCROW_RELEASE && vvch[1] == vchFromString("0"))
		status = "escrow released";
	else if(escrow.op == OP_ESCROW_RELEASE && vvch[1] == vchFromString("1"))
		status = "escrow release complete";
	else if(escrow.op == OP_ESCROW_COMPLETE && escrowRelease)
		status = "escrow release complete";
	else if(escrow.op == OP_ESCROW_REFUND && vvch[1] == vchFromString("0"))
		status = "escrow refunded";
	else if(escrow.op == OP_ESCROW_REFUND && vvch[1] == vchFromString("1"))
		status = "escrow refund complete";
	else if(escrow.op == OP_ESCROW_COMPLETE && escrowRefund)
		status = "escrow refund complete";
	if(escrow.bPaymentAck)
		status += " (acknowledged)";
	oEscrow.push_back(Pair("expired", expired));
	oEscrow.push_back(Pair("status", status));
	UniValue oBuyerFeedBack(UniValue::VARR);
	for(unsigned int i =0;i<buyerFeedBacks.size();i++)
	{
		UniValue oFeedback(UniValue::VOBJ);
		string sFeedbackTime;
		CBlockIndex *pindex = chainActive[buyerFeedBacks[i].nHeight];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->nTime);
		}
		oFeedback.push_back(Pair("txid", buyerFeedBacks[i].txHash.GetHex()));
		oFeedback.push_back(Pair("time", sFeedbackTime));
		oFeedback.push_back(Pair("rating", buyerFeedBacks[i].nRating));
		oFeedback.push_back(Pair("feedbackuser", buyerFeedBacks[i].nFeedbackUserFrom));
		oFeedback.push_back(Pair("feedback", stringFromVch(buyerFeedBacks[i].vchFeedback)));
		oBuyerFeedBack.push_back(oFeedback);
	}
	oEscrow.push_back(Pair("buyer_feedback", oBuyerFeedBack));
	oEscrow.push_back(Pair("avg_buyer_rating", avgBuyerRating));
	UniValue oSellerFeedBack(UniValue::VARR);
	for(unsigned int i =0;i<sellerFeedBacks.size();i++)
	{
		UniValue oFeedback(UniValue::VOBJ);
		string sFeedbackTime;
		CBlockIndex *pindex = chainActive[sellerFeedBacks[i].nHeight];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->nTime);
		}
		oFeedback.push_back(Pair("txid", sellerFeedBacks[i].txHash.GetHex()));
		oFeedback.push_back(Pair("time", sFeedbackTime));
		oFeedback.push_back(Pair("rating", sellerFeedBacks[i].nRating));
		oFeedback.push_back(Pair("feedbackuser", sellerFeedBacks[i].nFeedbackUserFrom));
		oFeedback.push_back(Pair("feedback", stringFromVch(sellerFeedBacks[i].vchFeedback)));
		oSellerFeedBack.push_back(oFeedback);
	}
	oEscrow.push_back(Pair("seller_feedback", oSellerFeedBack));
	oEscrow.push_back(Pair("avg_seller_rating", avgSellerRating));
	UniValue oArbiterFeedBack(UniValue::VARR);
	for(unsigned int i =0;i<arbiterFeedBacks.size();i++)
	{
		UniValue oFeedback(UniValue::VOBJ);
		string sFeedbackTime;
		CBlockIndex *pindex = chainActive[arbiterFeedBacks[i].nHeight];
		if (pindex) {
			sFeedbackTime = strprintf("%llu", pindex->nTime);
		}
		oFeedback.push_back(Pair("txid", arbiterFeedBacks[i].txHash.GetHex()));
		oFeedback.push_back(Pair("time", sFeedbackTime));
		oFeedback.push_back(Pair("rating", arbiterFeedBacks[i].nRating));
		oFeedback.push_back(Pair("feedbackuser", arbiterFeedBacks[i].nFeedbackUserFrom));
		oFeedback.push_back(Pair("feedback", stringFromVch(arbiterFeedBacks[i].vchFeedback)));
		oArbiterFeedBack.push_back(oFeedback);
	}
	oEscrow.push_back(Pair("arbiter_feedback", oArbiterFeedBack));
	oEscrow.push_back(Pair("avg_arbiter_rating", avgArbiterRating));
	unsigned int ratingCount = 0;
	if(avgArbiterRating > 0)
		ratingCount++;
	if(avgSellerRating > 0)
		ratingCount++;
	if(avgBuyerRating > 0)
		ratingCount++;
	oEscrow.push_back(Pair("avg_rating_count", (int)ratingCount));
	float totalAvgRating = 0;
	if(ratingCount > 0)
		 totalAvgRating = (avgArbiterRating+avgSellerRating+avgBuyerRating)/(float)ratingCount;
	totalAvgRating = floor(totalAvgRating * 10) / 10;
	oEscrow.push_back(Pair("avg_rating", totalAvgRating));
	oEscrow.push_back(Pair("avg_rating_display", strprintf("%.1f/5 (%d %s)", totalAvgRating, ratingCount, _("Votes"))));
	return true;
}

UniValue escrowlist(const UniValue& params, bool fHelp) {
   if (fHelp || 3 < params.size())
        throw runtime_error("escrowlist [\"alias\",...] [<escrow>] [<privatekey>]\n"
                "list escrows that an array of aliases are involved in. Set of aliases to look up based on alias, and private key to decrypt any data found in escrow.");
	UniValue aliasesValue(UniValue::VARR);
	vector<string> aliases;
	if(params.size() >= 1)
	{
		if(params[0].isArray())
		{
			aliasesValue = params[0].get_array();
			for(unsigned int aliasIndex =0;aliasIndex<aliasesValue.size();aliasIndex++)
			{
				string lowerStr = aliasesValue[aliasIndex].get_str();
				boost::algorithm::to_lower(lowerStr);
				if(!lowerStr.empty())
					aliases.push_back(lowerStr);
			}
		}
		else
		{
			string aliasName =  params[0].get_str();
			boost::algorithm::to_lower(aliasName);
			if(!aliasName.empty())
				aliases.push_back(aliasName);
		}
	}
	vector<unsigned char> vchNameUniq;
    if (params.size() >= 2 && !params[1].get_str().empty())
        vchNameUniq = vchFromValue(params[1]);

	string strPrivateKey;
	if(params.size() >= 3)
		strPrivateKey = params[2].get_str();

	UniValue oRes(UniValue::VARR);
	map< vector<unsigned char>, int > vNamesI;
	vector<pair<CEscrow, CEscrow> > escrowScan;
	if(aliases.size() > 0)
	{
		if (!pescrowdb->ScanEscrows(vchNameUniq, "", aliases, 1000, escrowScan))
			throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4606 - " + _("Scan failed"));
	
	}
	pair<CEscrow, CEscrow> pairScan;
	BOOST_FOREACH(pairScan, escrowScan) {
		UniValue oEscrow(UniValue::VOBJ);
		if(BuildEscrowJson(pairScan.first, pairScan.second, oEscrow, strPrivateKey))
			oRes.push_back(oEscrow);
	}
    return oRes;
}


UniValue escrowhistory(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("escrowhistory <escrow>\n"
                "List all stored values of an escrow.\n");

    UniValue oRes(UniValue::VARR);
    vector<unsigned char> vchEscrow = vchFromValue(params[0]);
    vector<CEscrow> vtxPos;
    if (!pescrowdb->ReadEscrow(vchEscrow, vtxPos) || vtxPos.empty())
        throw runtime_error("failed to read from escrow DB");

    CEscrow txPos2;
    BOOST_FOREACH(txPos2, vtxPos) {
		UniValue oEscrow(UniValue::VOBJ);
        if(BuildEscrowJson(txPos2, vtxPos.front(), oEscrow))
			oRes.push_back(oEscrow);
    }
    return oRes;
}
UniValue escrowfilter(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() > 2)
		throw runtime_error(
				"escrowfilter [[[[[regexp]] from=0]}\n"
						"scan and filter escrows\n"
						"[regexp] : apply [regexp] on escrows, empty means all escrows\n"
						"[from] : show results from this GUID [from], 0 means first.\n"
						"[escrowfilter] : shows all escrows that are safe to display (not on the ban list)\n"
						"escrowfilter \"\" 5 # list escrows updated in last 5 blocks\n"
						"escrowfilter \"^escrow\" # list all excrows starting with \"escrow\"\n"
						"escrowfilter 36000 0 0 stat # display stats (number of escrows) on active escrows\n");

	vector<unsigned char> vchEscrow;
	string strRegexp;

	if (params.size() > 0)
		strRegexp = params[0].get_str();

	if (params.size() > 1)
		vchEscrow = vchFromValue(params[1]);

	UniValue oRes(UniValue::VARR);

	vector<pair<CEscrow, CEscrow> > escrowScan;
	vector<string> aliases;
	if (!pescrowdb->ScanEscrows(vchEscrow, strRegexp, aliases, 1000, escrowScan))
		throw runtime_error("SYSCOIN_ESCROW_RPC_ERROR: ERRCODE: 4607 - " + _("Scan failed"));

	pair<CEscrow, CEscrow> pairScan;
	BOOST_FOREACH(pairScan, escrowScan) {
		UniValue oEscrow(UniValue::VOBJ);
		if(BuildEscrowJson(pairScan.first, pairScan.second, oEscrow))
			oRes.push_back(oEscrow);
	}

	return oRes;
}
void EscrowTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	
	CEscrow escrow;
	if(!escrow.UnserializeFromData(vchData, vchHash))
		return;

	CTransaction escrowtx;
	CEscrow dbEscrow;
	GetTxOfEscrow(escrow.vchEscrow, dbEscrow, escrowtx);


	string noDifferentStr = _("<No Difference Detected>");
	CEscrow escrowop(escrowtx);
	string opName = escrowFromOp(escrowop.op);
	if(escrowop.bPaymentAck)
		opName += "("+_("acknowledged")+")";
	else if(!escrowop.feedback.empty())
		opName += "("+_("feedback")+")";
	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("escrow", stringFromVch(escrow.vchEscrow)));

	string ackValue = noDifferentStr;
	if(escrow.bPaymentAck && escrow.bPaymentAck != dbEscrow.bPaymentAck)
		ackValue = escrow.bPaymentAck? "true": "false";

	entry.push_back(Pair("paymentacknowledge", ackValue));	

	entry.push_back(Pair("linkalias", stringFromVch(escrow.vchLinkAlias)));

	string feedbackValue = noDifferentStr;
	if(!escrow.feedback.empty())
		feedbackValue = _("Escrow feedback was given");
	entry.push_back(Pair("feedback", feedbackValue));
}
