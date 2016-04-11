#include "offer.h"
#include "alias.h"
#include "escrow.h"
#include "cert.h"
#include "message.h"
#include "init.h"
#include "main.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "rpcserver.h"
#include "wallet/wallet.h"
#include "consensus/validation.h"
#include "chainparams.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

using namespace std;
extern void SendMoney(const CTxDestination &address, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew);
extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const CWalletTx* wtxInOffer=NULL, const CWalletTx* wtxInCert=NULL, const CWalletTx* wtxInAlias=NULL, const CWalletTx* wtxInEscrow=NULL, bool syscoinTx=true);
bool DisconnectAlias(const CBlockIndex *pindex, const CTransaction &tx, int op, vector<vector<unsigned char> > &vvchArgs );
bool DisconnectOffer(const CBlockIndex *pindex, const CTransaction &tx, int op, vector<vector<unsigned char> > &vvchArgs );
bool DisconnectCertificate(const CBlockIndex *pindex, const CTransaction &tx, int op, vector<vector<unsigned char> > &vvchArgs );
bool DisconnectMessage(const CBlockIndex *pindex, const CTransaction &tx, int op, vector<vector<unsigned char> > &vvchArgs );
bool DisconnectEscrow(const CBlockIndex *pindex, const CTransaction &tx, int op, vector<vector<unsigned char> > &vvchArgs );
static const CBlock *linkedAcceptBlock = NULL;
bool foundOfferLinkInWallet(const vector<unsigned char> &vchOffer, const vector<unsigned char> &vchAcceptRandLink)
{
    TRY_LOCK(pwalletMain->cs_wallet, cs_trylock);
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
    {
		vector<vector<unsigned char> > vvchArgs;
		int op, nOut;
        const CWalletTx& wtx = item.second;
        if (wtx.IsCoinBase() || !CheckFinalTx(wtx))
            continue;
		if(wtx.nVersion != GetSyscoinTxVersion())
			continue;
		if (DecodeOfferTx(wtx, op, nOut, vvchArgs))
		{
			if(op == OP_OFFER_ACCEPT)
			{
				if(vvchArgs[0] == vchOffer)
				{
					vector<unsigned char> vchOfferAcceptLink;
					bool foundOffer = false;
					for (unsigned int i = 0; i < wtx.vin.size(); i++) {
						vector<vector<unsigned char> > vvchIn;
						int opIn;
						const COutPoint *prevOutput = &wtx.vin[i].prevout;
						if(!GetPreviousInput(prevOutput, opIn, vvchIn))
							continue;
						if(foundOffer)
							break;

						if (!foundOffer && opIn == OP_OFFER_ACCEPT) {
							foundOffer = true; 
							vchOfferAcceptLink = vvchIn[1];
						}
					}
					if(vchOfferAcceptLink == vchAcceptRandLink)
						return true;				
				}
			}
		}
	}
	return false;
}
// transfer cert if its linked to offer
string makeTransferCertTX(const COffer& theOffer, const COfferAccept& theOfferAccept)
{

	string strPubKey = HexStr(theOfferAccept.vchBuyerKey);
	string strError;
	string strMethod = string("certtransfer");
	UniValue params(UniValue::VARR);
	params.push_back(stringFromVch(theOffer.vchCert));
	params.push_back(strPubKey);
    try {
        tableRPC.execute(strMethod, params);
	}
	catch (UniValue& objError)
	{
		return find_value(objError, "message").get_str().c_str();
	}
	catch(std::exception& e)
	{
		return string(e.what()).c_str();
	}
	return "";

}
// refund an offer accept by creating a transaction to send coins to offer accepter, and an offer accept back to the offer owner. 2 Step process in order to use the coins that were sent during initial accept.
string makeOfferLinkAcceptTX(const COfferAccept& theOfferAccept, const vector<unsigned char> &vchOffer, const vector<unsigned char> &vchMessage, const vector<unsigned char> &vchLinkOffer, const string &offerAcceptLinkTxHash, const COffer& theOffer, const CBlock* block)
{
	if(!block)
	{
		return "cannot accept linked offer with no linkedAcceptBlock defined";
	}
	string strError;
	string strMethod = string("offeraccept");
	UniValue params(UniValue::VARR);

	CPubKey newDefaultKey;
	linkedAcceptBlock = block;
	vector<vector<unsigned char> > vvchArgs;
	bool foundOfferTx = false;
	for (unsigned int i = 0; i < linkedAcceptBlock->vtx.size(); i++)
    {
		if(foundOfferTx)
			break;
        const CTransaction &tx = linkedAcceptBlock->vtx[i];
		if(tx.nVersion == GetSyscoinTxVersion())
		{
			
			int op, nOut;			
			bool good = true;
			// find first offer accept in this block and make sure it is for the linked offer we are checking
			// the first one is the one that is used to do the offer accept tx, so any subsequent accept tx for the same offer will also check this tx and find that
			// the linked accept tx was already done (grouped all accept's together in this block)
			if(DecodeOfferTx(tx, op, nOut, vvchArgs) && op == OP_OFFER_ACCEPT && vvchArgs[0] == vchOffer)
			{	
				foundOfferTx = true;
				if(foundOfferLinkInWallet(vchLinkOffer, vvchArgs[1]))
				{
					return "offer linked transaction already exists";
				}
				break;
			}
		}
	}
	if(!foundOfferTx)
		return "cannot accept a linked offer accept in linkedAcceptBlock";
	if(!theOfferAccept.txBTCId.IsNull())
	{
		return "cannot accept a linked offer by paying in Bitcoins";
	}
	CPubKey PubKey(theOffer.vchPubKey);
	CSyscoinAddress address(PubKey.GetID());
	address = CSyscoinAddress(address.ToString());
	if(!address.IsValid() || !address.isAlias )
	{
		return "Invalid address or alias";
	}
	params.push_back(address.aliasName);
	params.push_back(stringFromVch(vchLinkOffer));
	params.push_back("99");
	params.push_back(stringFromVch(vchMessage));
	params.push_back("");
	params.push_back(offerAcceptLinkTxHash);
	
    try {
        tableRPC.execute(strMethod, params);
	}
	catch (UniValue& objError)
	{
		linkedAcceptBlock = NULL;
		return find_value(objError, "message").get_str().c_str();
	}
	catch(std::exception& e)
	{
		linkedAcceptBlock = NULL;
		return string(e.what()).c_str();
	}
	linkedAcceptBlock = NULL;
	return "";

}

bool IsOfferOp(int op) {
	return op == OP_OFFER_ACTIVATE
        || op == OP_OFFER_UPDATE
        || op == OP_OFFER_ACCEPT;
}


int GetOfferExpirationDepth() {
	#ifdef ENABLE_DEBUGRPC
    return 100;
  #else
    return 525600;
  #endif
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
bool COffer::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	if(!GetSyscoinData(tx, vchData))
	{
		SetNull();
		return false;
	}
    try {
        CDataStream dsOffer(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsOffer >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	// extra check to ensure data was parsed correctly
	if(!IsCompressedOrUncompressedPubKey(vchPubKey))
	{
		SetNull();
		return false;
	}
    return true;
}
const vector<unsigned char> COffer::Serialize() {
    CDataStream dsOffer(SER_NETWORK, PROTOCOL_VERSION);
    dsOffer << *this;
    const vector<unsigned char> vchData(dsOffer.begin(), dsOffer.end());
    return vchData;

}
//TODO implement
bool COfferDB::ScanOffers(const std::vector<unsigned char>& vchOffer, unsigned int nMax,
		std::vector<std::pair<std::vector<unsigned char>, COffer> >& offerScan) {

	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->Seek(make_pair(string("offeri"), vchOffer));
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
		pair<string, vector<unsigned char> > key;
        try {
			if (pcursor->GetKey(key) && key.first == "offeri") {
            	vector<unsigned char> vchOffer = key.second;
                vector<COffer> vtxPos;
				pcursor->GetValue(vtxPos);
                COffer txPos;
                if (!vtxPos.empty())
                    txPos = vtxPos.back();
                offerScan.push_back(make_pair(vchOffer, txPos));
            }
            if (offerScan.size() >= nMax)
                break;

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
	int op, nOut;
	bool good = DecodeOfferTx(tx, op, nOut, vvch);
	if (!good)
		return -1;
	return nOut;
}

bool GetTxOfOffer(const vector<unsigned char> &vchOffer, 
				  COffer& txPos, CTransaction& tx) {
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		return false;
	int nHeight = vtxPos.back().nHeight;
	txPos.nHeight = nHeight;
	if(!txPos.GetOfferFromList(vtxPos))
	{
		if(fDebug)
			LogPrintf("GetTxOfOffer() : cannot find offer from this offer position");
		return false;
	}
	if (nHeight + GetOfferExpirationDepth()
			< chainActive.Tip()->nHeight) {
		string offer = stringFromVch(vchOffer);
		if(fDebug)
			LogPrintf("GetTxOfOffer(%s) : expired", offer.c_str());
		return false;
	}

	if (!GetSyscoinTransaction(txPos.nHeight, txPos.txHash, tx, Params().GetConsensus()))
		return false;

	return true;
}

bool GetTxOfOfferAccept(const vector<unsigned char> &vchOffer, const vector<unsigned char> &vchOfferAccept,
		COffer &theOffer, COfferAccept &theOfferAccept, CTransaction& tx) {
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty()) return false;
	theOfferAccept.SetNull();
	theOfferAccept.vchAcceptRand = vchOfferAccept;
	GetAcceptByHash(vtxPos, theOfferAccept);
	if(theOfferAccept.IsNull())
		return false;
	int nHeight = theOfferAccept.nHeight;
	theOffer.nHeight = theOfferAccept.nAcceptHeight;
	if(!theOffer.GetOfferFromList(vtxPos))
	{
		if(fDebug)
			LogPrintf("GetTxOfOfferAccept() : cannot find offer from this offer accept position");
		return false;
	}
	if (nHeight + GetOfferExpirationDepth()
			< chainActive.Tip()->nHeight) {
		string offer = stringFromVch(vchOfferAccept);
		if(fDebug)
			LogPrintf("GetTxOfOfferAccept(%s) : expired", offer.c_str());
		return false;
	}

	if (!GetSyscoinTransaction(nHeight, theOfferAccept.txHash, tx, Params().GetConsensus()))
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
		vector<vector<unsigned char> > vvchRead;
		if (DecodeOfferScript(out.scriptPubKey, op, vvch)) {
			nOut = i; found = true;
			break;
		}
	}
	if (!found) vvch.clear();
	return found && IsOfferOp(op);
}


bool DecodeOfferScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
	opcodetype opcode;
	vvch.clear();
	if (!script.GetOp(pc, opcode)) return false;
	if (opcode < OP_1 || opcode > OP_16) return false;
	op = CScript::DecodeOP_N(opcode);

	for (;;) {
		vector<unsigned char> vch;
		if (!script.GetOp(pc, opcode, vch))
			return false;
		if (opcode == OP_DROP || opcode == OP_2DROP || opcode == OP_NOP)
			break;
		if (!(opcode >= 0 && opcode <= OP_PUSHDATA4))
			return false;
		vvch.push_back(vch);
	}

	// move the pc to after any DROP or NOP
	while (opcode == OP_DROP || opcode == OP_2DROP || opcode == OP_NOP) {
		if (!script.GetOp(pc, opcode))
			break;
	}

	pc--;

	if ((op == OP_OFFER_ACTIVATE && vvch.size() == 1)
		|| (op == OP_OFFER_UPDATE && vvch.size() == 1)
		|| (op == OP_OFFER_ACCEPT && vvch.size() == 4))
		return true;
	return false;
}
bool DecodeOfferScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch) {
	CScript::const_iterator pc = script.begin();
	return DecodeOfferScript(script, op, vvch, pc);
}
CScript RemoveOfferScriptPrefix(const CScript& scriptIn) {
	int op;
	vector<vector<unsigned char> > vvch;
	CScript::const_iterator pc = scriptIn.begin();
	
	if (!DecodeOfferScript(scriptIn, op, vvch, pc))
	{
		throw runtime_error(
			"RemoveOfferScriptPrefix() : could not decode offer script");
	}

	return CScript(pc, scriptIn.end());
}

bool CheckOfferInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const CBlock* block) {
		
	if (tx.IsCoinBase())
		return true;
	if (fDebug)
		LogPrintf("*** %d %d %s %s %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			op==OP_OFFER_ACCEPT ? "OFFERACCEPT: ": "", 
			op==OP_OFFER_ACCEPT ? stringFromVch(vvchArgs[1]).c_str(): "", 
			fJustCheck ? "JUSTCHECK" : "BLOCK");
	bool foundOffer = false;
	bool foundCert = false;
	bool foundEscrow = false;
	bool foundAlias = false;
	const COutPoint *prevOutput = NULL;
	CCoins prevCoins;
	uint256 prevOfferHash;
	int prevOp, prevCertOp, prevEscrowOp, prevAliasOp;
	prevOp = prevCertOp = prevEscrowOp = prevAliasOp = 0;
	vector<vector<unsigned char> > vvchPrevArgs, vvchPrevCertArgs, vvchPrevEscrowArgs, vvchPrevAliasArgs;
	if(fJustCheck)
	{
		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int pop;
			prevOutput = &tx.vin[i].prevout;
			if(!prevOutput)
				continue;
			// ensure inputs are unspent when doing consensus check to add to block
			if(!inputs.GetCoins(prevOutput->hash, prevCoins))
				continue;
			if(!IsSyscoinScript(prevCoins.vout[prevOutput->n].scriptPubKey, pop, vvch))
				continue;


			if(foundEscrow && foundOffer && foundCert && foundAlias)
				break;

			if (!foundOffer && IsOfferOp(pop)) {
				foundOffer = true; 
				prevOp = pop;
				vvchPrevArgs = vvch;
				prevOfferHash = prevOutput->hash;
			}
			else if (!foundCert && IsCertOp(pop))
			{
				foundCert = true; 
				prevCertOp = pop;
				vvchPrevCertArgs = vvch;
			}
			else if (!foundEscrow && IsEscrowOp(pop))
			{
				foundEscrow = true; 
				prevEscrowOp = pop;
				vvchPrevEscrowArgs = vvch;
			}
			else if (!foundAlias && IsAliasOp(pop))
			{
				foundAlias = true; 
				prevAliasOp = pop;
				vvchPrevAliasArgs = vvch;
			}
		}
	}
	

	// Make sure offer outputs are not spent by a regular transaction, or the offer would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION) {
		if (foundOffer)
			return error(
					"CheckOfferInputs() : a non-syscoin transaction with a syscoin input");
		return true;
	}

	// unserialize offer from txn, check for valid
	COffer theOffer(tx);
	COfferAccept theOfferAccept;
	if (theOffer.IsNull())
		return error("CheckOfferInputs() : null offer");
	if(theOffer.sDescription.size() > MAX_VALUE_LENGTH)
	{
		return error("offer description too big");
	}
	if(theOffer.sTitle.size() > MAX_NAME_LENGTH)
	{
		return error("offer title too big");
	}
	if(theOffer.sCategory.size() > MAX_NAME_LENGTH)
	{
		return error("offer category too big");
	}
	if(theOffer.vchLinkOffer.size() > MAX_NAME_LENGTH)
	{
		return error("offer link guid too big");
	}
	if(!IsCompressedOrUncompressedPubKey(theOffer.vchPubKey))
	{
		return error("offer pub key too invalid length");
	}
	if(theOffer.sCurrencyCode.size() > MAX_NAME_LENGTH)
	{
		return error("offer currency code too big");
	}
	if(theOffer.vchAliasPeg.size() > MAX_NAME_LENGTH)
	{
		return error("offer alias peg too big");
	}
	if(theOffer.offerLinks.size() > 0)
	{
		return error("offer links are not allowed in tx data");
	}
	if(theOffer.linkWhitelist.entries.size() > 1)
	{
		return error("offer has too many affiliate entries, only one allowed per tx");
	}

	if (vvchArgs[0].size() > MAX_NAME_LENGTH)
		return error("offer hex guid too long");

	if(stringFromVch(theOffer.sCurrencyCode) != "BTC" && theOffer.bOnlyAcceptBTC)
	{
		return error("An offer that only accepts BTC must have BTC specified as its currency");
	}
	vector<CAliasIndex> vtxAliasPos;
	COffer linkOffer;
	COffer myPriceOffer;
	COffer myOffer;
	CTransaction linkedTx;
	uint64_t heightToCheckAgainst;
	COfferLinkWhitelistEntry entry;
	vector<unsigned char> vchWhitelistAlias;
	CCert theCert;
	vector<COffer> vtxPos;
	bool linkAccept = false;
	bool escrowAccept = false;
	vector<string> rateList;
	int precision = 2;
	CAmount nRate;
	// just check is for the memory pool inclusion, here we can stop bad transactions from entering before we get to include them in a block	
	if(fJustCheck)
	{
		switch (op) {
		case OP_OFFER_ACTIVATE:
			if (!theOffer.vchCert.empty() && !IsCertOp(prevCertOp))
				return error("CheckOfferInputs() : you must own a cert you wish to sell");
			if (IsCertOp(prevCertOp) && !theOffer.vchCert.empty() && theOffer.vchCert != vvchPrevCertArgs[0])
				return error("CheckOfferInputs() : cert input and offer cert guid mismatch");		
			// if we are selling a cert ensure it exists and pubkey's match (to ensure it doesnt get transferred prior to accepting by user)
			if(!theOffer.vchCert.empty())
			{
				CTransaction txCert;
				// make sure this cert is still valid
				if (GetTxOfCert( theOffer.vchCert, theCert, txCert))
				{
					if(theCert.vchPubKey != theOffer.vchPubKey)
						return error("CheckOfferInputs() OP_OFFER_ACTIVATE: cert and offer pubkey's must match, this cert may already be linked to another offer");
				}
			}	
			if(!theOffer.vchLinkOffer.empty())
			{

				vector<COffer> myVtxPos;
				if (pofferdb->ExistsOffer(theOffer.vchLinkOffer)) {
					if (pofferdb->ReadOffer(theOffer.vchLinkOffer, myVtxPos))
					{
						COffer myParentOffer = myVtxPos.back();
						if (myParentOffer.bOnlyAcceptBTC)
							return error("CheckOfferInputs() OP_OFFER_ACTIVATE: cannot link to an offer that only accepts Bitcoins as payment");
						if (!IsAliasOp(prevAliasOp) && myParentOffer.linkWhitelist.bExclusiveResell)
							return error("CheckOfferInputs() OP_OFFER_ACTIVATE: parent offer set to exlusive resell but no alias input given to linked offer");			
						if (IsAliasOp(prevAliasOp) && !myParentOffer.linkWhitelist.GetLinkEntryByHash(vvchPrevAliasArgs[0], entry))
							return error("CheckOfferInputs() OP_OFFER_ACTIVATE: can't find this alias in the parent offer affiliate list");
						if (!myParentOffer.vchLinkOffer.empty())
							return error("CheckOfferInputs() OP_OFFER_ACTIVATE: cannot link to an offer that is already linked to another offer");
								
					}
					else
						return error("CheckOfferInputs() OP_OFFER_ACTIVATE: invalid linked offer guid");	
				}
				else
					return error("CheckOfferInputs() OP_OFFER_ACTIVATE: invalid linked offer guid");
			}
			else
			{
				// check for valid alias peg
				if(getCurrencyToSYSFromAlias(theOffer.vchAliasPeg, theOffer.sCurrencyCode, nRate, theOffer.nHeight, rateList,precision) != "")
				{
					return error("CheckOfferInputs() : could not find currency %s in the %s alias!\n", stringFromVch(theOffer.sCurrencyCode).c_str(), stringFromVch(theOffer.vchAliasPeg).c_str());
				}
			}
			
			break;
		case OP_OFFER_UPDATE:
			if (!IsOfferOp(prevOp) )
				return error("CheckOfferInputs() :offerupdate previous op is invalid");	
			if(prevOp == OP_OFFER_ACCEPT)
				return error("CheckOfferInputs(): cannot use offeraccept as input to an update");	
			if (vvchPrevArgs[0] != vvchArgs[0])
				return error("CheckOfferInputs() : offerupdate offer mismatch");	
			// load the offer data from the DB
			if (pofferdb->ExistsOffer(vvchArgs[0])) {
				if (!pofferdb->ReadOffer(vvchArgs[0], vtxPos) || vtxPos.empty())
					return error(
							"CheckOfferInputs() : failed to read from offer DB");
			}
			myOffer = vtxPos.back();

			// check for valid alias peg
			if(!theOffer.vchAliasPeg.empty() && getCurrencyToSYSFromAlias(theOffer.vchAliasPeg, myOffer.sCurrencyCode, nRate, theOffer.nHeight, rateList,precision) != "")
			{
				return error("CheckOfferInputs() : could not find currency %s in the %s alias!\n", stringFromVch(myOffer.sCurrencyCode).c_str(), stringFromVch(theOffer.vchAliasPeg));
			}
			// if we are selling a cert ensure it exists and pubkey's match (to ensure it doesnt get transferred prior to accepting by user)
			// also only do this if whitelist isn't being modified, because if it is, update just falls through and offer is stored as whats in the db, plus any whitelist changes
			if(!theOffer.vchCert.empty())
			{
				if(!myOffer.vchLinkOffer.empty())
					return error("cannot sell a cert as a linked offer");
				
				CTransaction txCert;
				// make sure this cert is still valid
				if (GetTxOfCert( theOffer.vchCert, theCert, txCert))
				{
					if (IsCertOp(prevCertOp) && theOffer.vchCert != vvchPrevCertArgs[0])
						return error("CheckOfferInputs() : cert input and offer cert guid mismatch");
					if (!IsCertOp(prevCertOp))
						return error("CheckOfferInputs() : you must own the cert offer you wish to update");	
					if(theCert.vchPubKey != theOffer.vchPubKey)
						return error("CheckOfferInputs() OP_OFFER_UPDATE: cert and offer pubkey mismatch");
				}
			}
			
			break;
		case OP_OFFER_ACCEPT:
			// check for existence of offeraccept in txn offer obj
			theOfferAccept = theOffer.accept;		
			if(IsOfferOp(prevOp))
				theOfferAccept.nQty = boost::lexical_cast<unsigned int>(stringFromVch(vvchPrevArgs[3]));
			else
				theOfferAccept.nQty = boost::lexical_cast<unsigned int>(stringFromVch(vvchArgs[3]));
			if (theOfferAccept.IsNull())
				return error("OP_OFFER_ACCEPT null accept object");
			if (IsEscrowOp(prevEscrowOp) && IsAliasOp(prevAliasOp))
				return error("OP_OFFER_ACCEPT Cannot have both alias and escrow inputs to an accept");
			if (IsEscrowOp(prevEscrowOp) && !theOfferAccept.txBTCId.IsNull())
				return error("OP_OFFER_ACCEPT can't use BTC for escrow transactions");				
			if (vvchArgs[1].size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT offeraccept tx with guid too big");
			if(prevOp == OP_OFFER_ACCEPT)
			{
				if(!theOfferAccept.txBTCId.IsNull())
					return error("OP_OFFER_ACCEPT can't accept a linked offer with btc");
			}


			if (theOfferAccept.vchAcceptRand.size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT offer accept hex guid too long");
			if (vvchArgs[2].size() > MAX_ENCRYPTED_VALUE_LENGTH)
				return error("OP_OFFER_ACCEPT message field too big");
			if (IsEscrowOp(prevEscrowOp) && IsOfferOp(prevOp))
				return error("OP_OFFER_ACCEPT offer accept cannot attach both escrow and offer inputs at the same time");
			if (IsEscrowOp(prevEscrowOp))
			{	
				vector<CEscrow> escrowVtxPos;
				if (pescrowdb->ExistsEscrow(vvchPrevEscrowArgs[0])) {
					if (pescrowdb->ReadEscrow(vvchPrevEscrowArgs[0], escrowVtxPos) && !escrowVtxPos.empty())
					{	
						// we want the initial funding escrow transaction height as when to calculate this offer accept price
						CEscrow fundingEscrow = escrowVtxPos.front();
						if(fundingEscrow.vchOffer != vvchArgs[0])
							return error("CheckOfferInputs() OP_OFFER_ACCEPT: escrow guid does not match the guid of the offer you are accepting");		
					}
					else
						return error("CheckOfferInputs() OP_OFFER_ACCEPT: invalid escrow guid");		
				}
				else
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: invalid escrow guid");	
			}
			// load the offer data from the DB
			if (pofferdb->ExistsOffer(vvchArgs[0])) {
				if (!pofferdb->ReadOffer(vvchArgs[0], vtxPos) || vtxPos.empty())
					return error(
							"CheckOfferInputs() : failed to read from offer DB");
			}
			theOffer = vtxPos.back();
			if(!theOffer.vchLinkOffer.empty())
			{
				if(!GetTxOfOffer( theOffer.vchLinkOffer, linkOffer, linkedTx))
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: could not get linked offer");
				// make sure that linked offer exists in root offerlinks (offers that are linked to the root offer)
				if(std::find(linkOffer.offerLinks.begin(), linkOffer.offerLinks.end(), vvchArgs[0]) == linkOffer.offerLinks.end())
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: this offer does not exist in the root offerLinks table, are you sure you are allowed to link to the offer and take payments?");
						
			}
			// trying to purchase a cert
			if(!theOffer.vchCert.empty())
			{
				CTransaction txCert;
				// make sure this cert is still valid
				if (GetTxOfCert( theOffer.vchCert, theCert, txCert))
				{
					// if we do an offeraccept based on an escrow release, it's assumed that the cert has already been transferred manually so buyer releases funds which can invalidate this accept
 					// so in that case the escrow is attached to the accept and we skip this check
 					// if the escrow is not attached means the buyer didnt use escrow, so ensure cert didn't get transferred since vendor created the offer in that case.
 					// also ensure its not a linked offer we are accepting, that check is done below
 					if(!IsEscrowOp(prevEscrowOp))
 					{
 						if(theOffer.vchLinkOffer.empty())
 						{
 							if(theCert.vchPubKey != theOffer.vchPubKey)
 								return error("CheckOfferInputs() OP_OFFER_ACCEPT: cannot purchase this offer because the certificate has been transferred since it offer was created or it is linked to another offer. Cert pubkey %s vs Offer pubkey %s", HexStr(theCert.vchPubKey).c_str(), HexStr(theOffer.vchPubKey).c_str());
 						}
 						else
 						{
 							if(theCert.vchPubKey != linkOffer.vchPubKey)
 								return error("CheckOfferInputs() OP_OFFER_ACCEPT: cannot purchase this linked offer because the certificate has been transferred since it offer was created or it is linked to another offer. Cert pubkey %s vs Offer pubkey %s", HexStr(theCert.vchPubKey).c_str(), HexStr(theOffer.vchPubKey).c_str());
 						}
 					}
					theOfferAccept.nQty = 1;
				}
				else
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: purchasing a cert that doesn't exist");
			}
			if(stringFromVch(theOffer.sCurrencyCode) != "BTC" && !theOfferAccept.txBTCId.IsNull())
				return error("CheckOfferInputs() OP_OFFER_ACCEPT: can't accept an offer for BTC that isn't specified in BTC by owner");								
			// find the payment from the tx outputs (make sure right amount of coins were paid for this offer accept), the payment amount found has to be exact	
			heightToCheckAgainst = theOfferAccept.nAcceptHeight;
			// if this is a linked offer accept, set the height to the first height so sys_rates price will match what it was at the time of the original accept
			// we assume previous tx still in mempool because it calls offeraccept within the eheckinputs stage (not entering a block yet)
			if (IsOfferOp(prevOp))
			{
				CTransaction acceptTx;
				COffer theLinkedOfferAccept;
				uint256 hashBlock;
				if(GetTransaction(prevOfferHash, acceptTx, Params().GetConsensus(), hashBlock, true))
				{
					COffer theLinkedOfferAccept(acceptTx);
					if(theLinkedOfferAccept.accept.IsNull())
						return error("CheckOfferInputs() OP_OFFER_ACCEPT: could not find a linked offer accept with this identifier");
					heightToCheckAgainst = theLinkedOfferAccept.accept.nAcceptHeight;
					linkAccept = true;
					if(theOfferAccept.vchBuyerKey != theLinkedOfferAccept.accept.vchBuyerKey)
						return error("CheckOfferInputs() OP_OFFER_ACCEPT: buyer public key of linked accept does not match the public key of the buyer of the linked offer");
				}
				else
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: could not find a linked offer accept from mempool or disk");
			}
			// if this accept was done via an escrow release, we get the height from escrow and use that to lookup the price at the time
			if(IsEscrowOp(prevEscrowOp))
			{		
				vector<CEscrow> escrowVtxPos;
				if (pescrowdb->ExistsEscrow(vvchPrevEscrowArgs[0])) {
					if (pescrowdb->ReadEscrow(vvchPrevEscrowArgs[0], escrowVtxPos) && !escrowVtxPos.empty())
					{	
						// we want the initial funding escrow transaction height as when to calculate this offer accept price
						CEscrow fundingEscrow = escrowVtxPos.front();
						// override height if funding escrow was before the accept that the buyer did, to index into sysrates
						if(heightToCheckAgainst > fundingEscrow.nHeight || !linkAccept){
							escrowAccept = true;
							heightToCheckAgainst = fundingEscrow.nHeight;
						}
						// if an alias is attached to the escrow, meaning the buyer has an alias that may be in the seller's whitelist, get the alias guid here
						if(!escrowVtxPos.back().vchWhitelistAlias.empty())
							vchWhitelistAlias = escrowVtxPos.back().vchWhitelistAlias;

					}
				}
			}
			// the height really shouldnt change cause we set it correctly in offeraccept
			if(heightToCheckAgainst != theOfferAccept.nAcceptHeight)
				return error("CheckOfferInputs() OP_OFFER_ACCEPT: height mismatch with calculated height heightToCheckAgainst %s vs theOfferAccept.nAcceptHeight %s", heightToCheckAgainst, theOfferAccept.nAcceptHeight);
			// if this is a normal accept, make sure height passed into accept is only a few blocks away, giving some buffer to get into mempool for most of network
			// if its a linked accept or escrow, it can be a long time before the time of the buy and time of accept, above check catches that.
			if(!linkAccept && !escrowAccept)
			{
				if(nHeight < heightToCheckAgainst || (nHeight - heightToCheckAgainst) > 10)
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: accept height and current block height differ by too much heightToCheckAgainst %d vs nHeight %d", heightToCheckAgainst, nHeight);

			}
			myPriceOffer.nHeight = heightToCheckAgainst;
			myPriceOffer.GetOfferFromList(vtxPos);
			// check that user pays enough in syscoin if the currency of the offer is not bitcoin or there is no bitcoin transaction ID associated with this accept
			if(stringFromVch(myPriceOffer.sCurrencyCode) != "BTC" || theOfferAccept.txBTCId.IsNull())
			{
	
				// if there is no escrow being used here, vchWhitelistAlias should be empty so if the buyer uses an alias for a discount or a exclusive whitelist buy, then get the guid
				if(IsAliasOp(prevAliasOp) && vchWhitelistAlias.empty())
					vchWhitelistAlias = vvchPrevAliasArgs[0];
				
				// try to get the whitelist entry here from the sellers whitelist, apply the discount with GetPrice()
				myPriceOffer.linkWhitelist.GetLinkEntryByHash(vchWhitelistAlias, entry);	

				float priceAtTimeOfAccept = myPriceOffer.GetPrice(entry);
				if(priceAtTimeOfAccept != theOfferAccept.nPrice)
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: offer accept does not specify the correct payment amount priceAtTimeOfAccept %f%s vs theOfferAccept.nPrice %f%s", priceAtTimeOfAccept, stringFromVch(theOffer.sCurrencyCode).c_str(), theOfferAccept.nPrice, stringFromVch(theOffer.sCurrencyCode).c_str());

				int precision = 2;
				// lookup the price of the offer in syscoin based on pegged alias at the block # when accept/escrow was made
				CAmount nPrice = convertCurrencyCodeToSyscoin(myPriceOffer.vchAliasPeg, myPriceOffer.sCurrencyCode, priceAtTimeOfAccept, heightToCheckAgainst, precision)*theOfferAccept.nQty;
				if(tx.vout[nOut].nValue != nPrice)
					return error("CheckOfferInputs() OP_OFFER_ACCEPT: this offer accept does not pay enough according to the offer price %ld, currency %s, value found %ld\n", nPrice, stringFromVch(theOffer.sCurrencyCode).c_str(), tx.vout[nOut].nValue);											
			}						
		
			if(theOfferAccept.nQty <= 0 || (theOffer.nQty != -1 && theOfferAccept.nQty > theOffer.nQty) || (!linkOffer.IsNull() && theOfferAccept.nQty > linkOffer.nQty && linkOffer.nQty != -1))
				return error("CheckOfferInputs() OP_OFFER_ACCEPT: txn %s rejected because desired qty %u is more than available qty %u\n", tx.GetHash().GetHex().c_str(), theOfferAccept.nQty, theOffer.nQty);
					
			break;

		default:
			return error( "CheckOfferInputs() : offer transaction has unknown op");
		}
	}
	

	if (!fJustCheck ) {
		// save serialized offer for later use
		COffer serializedOffer = theOffer;
		// load the offer data from the DB
		if (pofferdb->ExistsOffer(vvchArgs[0])) {
			if (!pofferdb->ReadOffer(vvchArgs[0], vtxPos))
				return error(
						"CheckOfferInputs() : failed to read from offer DB");
		}
		// get the latest offer from the db
		if(!vtxPos.empty())
			theOffer = vtxPos.back();				

		// If update, we make the serialized offer the master
		// but first we assign fields from the DB since
		// they are not shipped in an update txn to keep size down
		if(op == OP_OFFER_UPDATE) {
			serializedOffer.offerLinks = theOffer.offerLinks;
			serializedOffer.vchLinkOffer = theOffer.vchLinkOffer;
			// btc setting cannot change on update
			serializedOffer.bOnlyAcceptBTC = theOffer.bOnlyAcceptBTC;
			// currency cannot change after creation
			serializedOffer.sCurrencyCode = theOffer.sCurrencyCode;
			serializedOffer.accept.SetNull();
			theOffer = serializedOffer;
			if(!vtxPos.empty())
			{
				const COffer& dbOffer = vtxPos.back();
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
					if(serializedOffer.sDescription.empty())
						theOffer.sDescription = dbOffer.sDescription;
					if(serializedOffer.vchAliasPeg.empty())
						theOffer.vchAliasPeg = dbOffer.vchAliasPeg;
				}
			}
			if(!theOffer.vchCert.empty())						
				theOffer.nQty = 1;
			
		}
		else if(op == OP_OFFER_ACTIVATE)
		{
			// by default offers are private on new, need to update it out of privacy
			theOffer.bPrivate = true;
			if(!theOffer.vchCert.empty())
				theOffer.nQty = 1;
				
			// if this is a linked offer activate, then add it to the parent offerLinks list
			if(!theOffer.vchLinkOffer.empty())
			{
				vector<COffer> myVtxPos;
				if (pofferdb->ExistsOffer(theOffer.vchLinkOffer)) {
					if (pofferdb->ReadOffer(theOffer.vchLinkOffer, myVtxPos))
					{
						COffer myParentOffer = myVtxPos.back();
						// if creating a linked offer we set some mandatory fields to the parent
						theOffer.nQty = myParentOffer.nQty;
						theOffer.sCategory = myParentOffer.sCategory;
						theOffer.sTitle = myParentOffer.sTitle;
						theOffer.linkWhitelist.bExclusiveResell = true;
						theOffer.sCurrencyCode = myParentOffer.sCurrencyCode;
						theOffer.vchCert = myParentOffer.vchCert;
						theOffer.vchAliasPeg = myParentOffer.vchAliasPeg;

						myParentOffer.offerLinks.push_back(vvchArgs[0]);							
						myParentOffer.PutToOfferList(myVtxPos);
						// write parent offer
				
						if (!pofferdb->WriteOffer(theOffer.vchLinkOffer, myVtxPos))
							return error( "CheckOfferInputs() : failed to write to offer link to DB");
						
					}
				}
				
			}
		}
		else if (op == OP_OFFER_ACCEPT) {	
			theOfferAccept = serializedOffer.accept;
			if(IsOfferOp(prevOp))
				theOfferAccept.nQty = boost::lexical_cast<unsigned int>(stringFromVch(vvchPrevArgs[3]));
			else
				theOfferAccept.nQty = boost::lexical_cast<unsigned int>(stringFromVch(vvchArgs[3]));
			if(theOfferAccept.nQty <= 0)
				theOfferAccept.nQty = 1;
			if((theOffer.nQty != -1 && theOfferAccept.nQty > theOffer.nQty)) {
				if(fDebug)
					LogPrintf("CheckOfferInputs() OP_OFFER_ACCEPT: txn %s desired qty %u is more than available qty %u\n", tx.GetHash().GetHex().c_str(), theOfferAccept.nQty, theOffer.nQty);
				return true;
			}
			if(theOffer.nQty != -1)
			{
				theOffer.nQty -= theOfferAccept.nQty;
				if(theOffer.nQty < 0)
					theOffer.nQty = 0;
			}

			theOfferAccept.nHeight = nHeight;
			theOfferAccept.vchAcceptRand = vvchArgs[1];
			theOfferAccept.txHash = tx.GetHash();
			theOffer.accept = theOfferAccept;
		}
		
		if(op == OP_OFFER_ACTIVATE || op == OP_OFFER_UPDATE) {
			if(op == OP_OFFER_UPDATE)
			{
				// if the txn whitelist entry exists (meaning we want to remove or add)
				if(serializedOffer.linkWhitelist.entries.size() == 1)
				{
					// special case we use to remove all entries
					if(serializedOffer.linkWhitelist.entries[0].nDiscountPct == 127)
					{
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
						if(!serializedOffer.linkWhitelist.entries[0].aliasLinkVchRand.empty() && serializedOffer.linkWhitelist.entries[0].nDiscountPct <= 99)
							theOffer.linkWhitelist.PutWhitelistEntry(serializedOffer.linkWhitelist.entries[0]);
					}
				}
				// if this offer is linked to a parent update it with parent information
				if(!theOffer.vchLinkOffer.empty())
				{
					vector<COffer> myVtxPos;
					if (pofferdb->ExistsOffer(theOffer.vchLinkOffer)) {
						if (pofferdb->ReadOffer(theOffer.vchLinkOffer, myVtxPos))
						{
							COffer myLinkOffer = myVtxPos.back();
							theOffer.nQty = myLinkOffer.nQty;	
							theOffer.vchAliasPeg = myLinkOffer.vchAliasPeg;	
							theOffer.SetPrice(myLinkOffer.nPrice);
							
						}
					}
						
				}
				else
				{
					// go through the linked offers, if any, and update the linked offer info based on the this info
					for(unsigned int i=0;i<theOffer.offerLinks.size();i++) {
						vector<COffer> myVtxPos;
						if (pofferdb->ExistsOffer(theOffer.offerLinks[i])) {
							if (pofferdb->ReadOffer(theOffer.offerLinks[i], myVtxPos))
							{
								COffer myLinkOffer = myVtxPos.back();
								myLinkOffer.nQty = theOffer.nQty;	
								myLinkOffer.vchAliasPeg = theOffer.vchAliasPeg;	
								myLinkOffer.SetPrice(theOffer.nPrice);
								myLinkOffer.PutToOfferList(myVtxPos);
								// write offer
							
								if (!pofferdb->WriteOffer(theOffer.offerLinks[i], myVtxPos))
										return error( "CheckOfferInputs() : failed to write to offer link to DB");
								
							}
						}
					}
					
				}
			}
		}
		theOffer.nHeight = nHeight;
		theOffer.txHash = tx.GetHash();
		theOffer.PutToOfferList(vtxPos);
		// write offer
	
		if (!pofferdb->WriteOffer(vvchArgs[0], vtxPos))
			return error( "CheckOfferInputs() : failed to write to offer DB");
		
		if(op == OP_OFFER_ACCEPT)
		{
 			if(theOffer.vchLinkOffer.empty())
			{
				// go through the linked offers, if any, and update the linked offer qty based on the this qty
				for(unsigned int i=0;i<theOffer.offerLinks.size();i++) {
					vector<COffer> myVtxPos;
					if (pofferdb->ExistsOffer(theOffer.offerLinks[i])) {
						if (pofferdb->ReadOffer(theOffer.offerLinks[i], myVtxPos))
						{
							COffer myLinkOffer = myVtxPos.back();
							myLinkOffer.nQty = theOffer.nQty;	
							myLinkOffer.PutToOfferList(myVtxPos);
							// write offer
							
							if (!pofferdb->WriteOffer(theOffer.offerLinks[i], myVtxPos))
									return error( "CheckOfferInputs() : failed to write to offer link to DB");
							
						}
					}
				}
			}	
			if (pwalletMain && !theOffer.vchLinkOffer.empty() && IsSyscoinTxMine(tx, "offer"))
			{	
				// theOffer.vchLinkOffer is the linked offer guid
				// theOffer is this reseller offer used to get pubkey to send to offeraccept as first parameter
				string strError = makeOfferLinkAcceptTX(theOfferAccept, vvchArgs[0], vvchArgs[2], theOffer.vchLinkOffer, tx.GetHash().GetHex(), theOffer, block);
				if(strError != "")					
					LogPrintf("CheckOfferInputs() - OP_OFFER_ACCEPT - makeOfferLinkAcceptTX %s\n", strError.c_str());			
				
			} 
			// only if we are the root offer owner do we even consider xfering a cert					
 			// purchased a cert so xfer it
			// also can't auto xfer offer paid in btc, need to do manually
 			if(pwalletMain && theOfferAccept.txBTCId.IsNull() && IsSyscoinTxMine(tx, "offer") && !theOffer.vchCert.empty() && theOffer.vchLinkOffer.empty())
 			{
 				string strError = makeTransferCertTX(theOffer, theOfferAccept);
 				if(strError != "")
 					LogPrintf("CheckOfferInputs() - OP_OFFER_ACCEPT - makeTransferCert %s\n", strError.c_str());									
 			}
		}
		// debug
		if (fDebug)
			LogPrintf( "CONNECTED OFFER: op=%s offer=%s title=%s qty=%u hash=%s height=%d\n",
				offerFromOp(op).c_str(),
				stringFromVch(vvchArgs[0]).c_str(),
				stringFromVch(theOffer.sTitle).c_str(),
				theOffer.nQty,
				tx.GetHash().ToString().c_str(), 
				nHeight);
	}
	return true;
}

UniValue offernew(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 7 || params.size() > 11)
		throw runtime_error(
		"offernew <aliaspeg> <alias> <category> <title> <quantity> <price> <description> <currency> [cert. guid] [exclusive resell=1] [accept btc only=0]\n"
						"<aliaspeg> Alias peg you wish to use, leave blank to use SYS_RATES.\n"	
						"<alias> An alias you own.\n"
						"<category> category, 255 chars max.\n"
						"<title> title, 255 chars max.\n"
						"<quantity> quantity, > 0 or -1 for infinite\n"
						"<price> price in <currency>, > 0\n"
						"<description> description, 1 KB max.\n"
						"<currency> The currency code that you want your offer to be in ie: USD.\n"
						"<cert. guid> Set this to the guid of a certificate you wish to sell\n"
						"<exclusive resell> set to 1 if you only want those who control the affiliate's who are able to resell this offer via offerlink. Defaults to 1.\n"
						"<accept btc only> set to 1 if you only want accept Bitcoins for payment and your currency is set to BTC, note you cannot resell or sell a cert in this mode. Defaults to 0.\n"
						+ HelpRequiringPassphrase());
	// gather inputs
	string baSig;
	float nPrice;
	bool bExclusiveResell = true;
	vector<unsigned char> vchAliasPeg = vchFromValue(params[0]);
	CSyscoinAddress aliasPegAddress = CSyscoinAddress(stringFromVch(vchAliasPeg));
	if (!aliasPegAddress.IsValid() || !aliasPegAddress.isAlias)
		throw runtime_error("Invalid alias peg");
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw runtime_error("Offer must be a valid alias");

	CTransaction aliastx;
	CAliasIndex alias;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");
    if(!IsSyscoinTxMine(aliastx, "alias")) {
		throw runtime_error("This alias is not yours.");
    }
	if (pwalletMain->GetWalletTx(aliastx.GetHash()) == NULL)
		throw runtime_error("this alias is not in your wallet");
	vector<unsigned char> vchCat = vchFromValue(params[2]);
	vector<unsigned char> vchTitle = vchFromValue(params[3]);
	vector<unsigned char> vchCurrency = vchFromValue(params[7]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	bool bOnlyAcceptBTC = false;
	int nQty;

	try {
		nQty = atoi(params[4].get_str());
	} catch (std::exception &e) {
		throw runtime_error("invalid quantity value, must be less than 4294967296 and greater than or equal to -1");
	}
	if(nQty < -1)
		throw runtime_error("qty must be greater than or equal to -1");
	nPrice = atof(params[5].get_str().c_str());
	if(nPrice <= 0)
	{
		throw runtime_error("offer price must be greater than 0!");
	}
	vchDesc = vchFromValue(params[6]);
	if(vchCat.size() < 1)
        throw runtime_error("offer category cannot be empty!");
	if(vchTitle.size() < 1)
        throw runtime_error("offer title cannot be empty!");
	if(vchCat.size() > MAX_NAME_LENGTH)
        throw runtime_error("offer category cannot exceed 255 bytes!");
	if(vchTitle.size() > MAX_NAME_LENGTH)
        throw runtime_error("offer title cannot exceed 255 bytes!");
    // 1Kbyte offer desc. maxlen
	if (vchDesc.size() > MAX_VALUE_LENGTH)
		throw runtime_error("offer description cannot exceed 1023 bytes!");
	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;
	CScript scriptPubKey, scriptPubKeyCert;
	const CWalletTx *wtxCertIn = NULL;
	CCert theCert;
	if(params.size() >= 9)
	{
		
		vchCert = vchFromValue(params[8]);
		CTransaction txCert;
		
		if(nQty <= 0)
			throw runtime_error("qty must be greator than 0 for a cert offer");
		// make sure this cert is still valid
		if (GetTxOfCert( vchCert, theCert, txCert))
		{
      		// check for existing cert 's
			if (ExistsInMempool(vchCert, OP_CERT_UPDATE) || ExistsInMempool(vchCert, OP_CERT_TRANSFER)) {
				throw runtime_error("there are pending operations on that cert");
			}
			wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
			// make sure its in your wallet (you control this cert)		
			if (IsSyscoinTxMine(txCert, "cert") && wtxCertIn != NULL) 
			{
				CPubKey currentCertKey(theCert.vchPubKey);
				scriptPubKeyCertOrig = GetScriptForDestination(currentCertKey.GetID());
			}
			else
				throw runtime_error("You must own this cert to sell it");
			scriptPubKeyCert << CScript::EncodeOP_N(OP_CERT_UPDATE) << vchCert << OP_2DROP;
			scriptPubKeyCert += scriptPubKeyCertOrig;
		}
		else
			vchCert.clear();
	}

	if(params.size() >= 10)
	{
		bExclusiveResell = atoi(params[9].get_str().c_str()) == 1? true: false;
	}
	if(params.size() >= 11)
	{
		bOnlyAcceptBTC = atoi(params[10].get_str().c_str()) == 1? true: false;
		if(bOnlyAcceptBTC && !vchCert.empty())
			throw runtime_error("Cannot sell a certificate accepting only Bitcoins");
		if(bOnlyAcceptBTC && stringFromVch(vchCurrency) != "BTC")
			throw runtime_error("Can only accept Bitcoins for offer's that set their currency to BTC");

	}	
	CAmount nRate;
	vector<string> rateList;
	int precision;
	if(getCurrencyToSYSFromAlias(vchAliasPeg, vchCurrency, nRate, chainActive.Tip()->nHeight, rateList,precision) != "")
	{
		string err = strprintf("Could not find currency %s in the %s alias!\n", stringFromVch(vchCurrency), stringFromVch(vchAliasPeg));
		throw runtime_error(err.c_str());
	}
	double minPrice = pow(10.0,-precision);
	double price = nPrice;
	if(price < minPrice)
		price = minPrice; 
	// this is a syscoin transaction
	CWalletTx wtx;

	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));

	EnsureWalletIsUnlocked();


	// unserialize offer from txn, serialize back
	// build offer
	COffer newOffer;
	// if we sell a cert always use cert pubkey otherwise use alias
	if(wtxCertIn == NULL)
		newOffer.vchPubKey = alias.vchPubKey;
	else
		newOffer.vchPubKey = theCert.vchPubKey;
	newOffer.sCategory = vchCat;
	newOffer.sTitle = vchTitle;
	newOffer.sDescription = vchDesc;
	newOffer.nQty = nQty;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.SetPrice(price);
	newOffer.vchCert = vchCert;
	newOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = true;
	newOffer.bOnlyAcceptBTC = bOnlyAcceptBTC;
	newOffer.vchAliasPeg = vchAliasPeg;

	CPubKey currentOfferKey(newOffer.vchPubKey);
	scriptPubKeyOrig= GetScriptForDestination(currentOfferKey.GetID());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient certRecipient;
	CreateRecipient(scriptPubKeyCert, certRecipient);

	// if we use a cert as input to this offer tx, we need another utxo for further cert transactions on this cert, so we create one here
	if(wtxCertIn != NULL)
		vecSend.push_back(certRecipient);


	const vector<unsigned char> &data = newOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInOffer=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxInOffer, wtxCertIn, wtxInAlias, wtxInEscrow);
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}

UniValue offernew_nocheck(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 7 || params.size() > 11)
		throw runtime_error(
		"offernew_nocheck <alias> <category> <title> <quantity> <price> <description> <currency> [cert. guid] [exclusive resell=1] [accept btc only=0]\n"
						"<alias> An alias you own.\n"
						"<category> category\n"
						"<title> title.\n"
						"<quantity> quantity\n"
						"<price> price in <currency>\n"
						"<description> description, 1 KB max.\n"
						"<currency> The currency code that you want your offer to be in ie: USD.\n"
						"<cert. guid> Set this to the guid of a certificate you wish to sell\n"
						"<exclusive resell> set to 1 if you only want those who control the affiliate's who are able to resell this offer via offerlink. Defaults to 1.\n"
						"<accept btc only> set to 1 if you only want accept Bitcoins for payment and your currency is set to BTC, note you cannot resell or sell a cert in this mode. Defaults to 0.\n"
						+ HelpRequiringPassphrase());
	// gather inputs
	string baSig;
	float nPrice;
	bool bExclusiveResell = true;

	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid()) throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias) throw runtime_error("Offer must be a valid alias");
	CTransaction aliastx;
	CAliasIndex alias;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");

	vector<unsigned char> vchCat = vchFromValue(params[1]);
	vector<unsigned char> vchTitle = vchFromValue(params[2]);
	vector<unsigned char> vchCurrency = vchFromValue(params[6]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	bool bOnlyAcceptBTC = false;
	bool bUseDifferentKeys = false;
	int nQty;

	try {
		nQty = atoi(params[3].get_str());
	} catch (std::exception &e) {
		throw runtime_error("invalid quantity value, must be less than 4294967296 and greater than or equal to -1");
	}

	nPrice = atof(params[4].get_str().c_str());
	vchDesc = vchFromValue(params[5]);

	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;
	CScript scriptPubKey, scriptPubKeyCert;
	const CWalletTx *wtxCertIn = NULL;
	CCert theCert;

	if(params.size() >= 8) {
		vchCert = vchFromValue(params[7]);
		CTransaction txCert;
		if(nQty <= 0)
			throw runtime_error("qty must be greator than 0 for a cert offer");		
		// make sure this cert is still valid
		if (GetTxOfCert( vchCert, theCert, txCert)) {
			CPubKey currentCertKey(theCert.vchPubKey);
			scriptPubKeyCertOrig = GetScriptForDestination(currentCertKey.GetID());
			scriptPubKeyCert << CScript::EncodeOP_N(OP_CERT_UPDATE) << vchCert << OP_2DROP;
			scriptPubKeyCert += scriptPubKeyCertOrig;		
		}
		else vchCert.clear();
	}

	if(params.size() >= 9)
		bExclusiveResell = atoi(params[8].get_str().c_str()) == 1? true: false;

	if(params.size() >= 10) 
		bOnlyAcceptBTC = atoi(params[9].get_str().c_str()) == 1? true: false;

	if(params.size() >= 11) 
		bUseDifferentKeys = atoi(params[10].get_str().c_str()) == 1? true: false;

	CAmount nRate;
	vector<string> rateList;
	int precision=8;
	

	
	double minPrice = pow(10.0,-precision);
	double price = nPrice;
	if(price < minPrice)
		price = minPrice; 
	
	// this is a syscoin transaction
	CWalletTx wtx;

	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));

	EnsureWalletIsUnlocked();

	// unserialize offer from txn, serialize back
	// build offer
	COffer newOffer;
	// if we sell a cert always use cert pubkey otherwise use alias
	if(wtxCertIn == NULL || bUseDifferentKeys)
		newOffer.vchPubKey = alias.vchPubKey;
	else
		newOffer.vchPubKey = theCert.vchPubKey;
	newOffer.sCategory = vchCat;
	newOffer.sTitle = vchTitle;
	newOffer.sDescription = vchDesc;
	newOffer.nQty = nQty;
	newOffer.vchAliasPeg = vchFromString("SYS_RATES");
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.SetPrice(price);
	newOffer.vchCert = vchCert;
	newOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = true;
	newOffer.bOnlyAcceptBTC = bOnlyAcceptBTC;

	CPubKey currentOfferKey(newOffer.vchPubKey);
	scriptPubKeyOrig= GetScriptForDestination(currentOfferKey.GetID());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient certRecipient;
	CreateRecipient(scriptPubKeyCert, certRecipient);

	// if we use a cert as input to this offer tx, we need another utxo for further cert transactions on this cert, so we create one here
	if(wtxCertIn != NULL)
		vecSend.push_back(certRecipient);

	const vector<unsigned char> &data = newOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInOffer=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxInOffer, wtxCertIn, wtxInAlias, wtxInEscrow);
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}

UniValue offerlink(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 3 || params.size() > 4)
		throw runtime_error(
		"offerlink <alias> <guid> <commission> [description]\n"
						"<alias> An alias you own.\n"
						"<guid> offer guid that you are linking to\n"
						"<commission> percentage of profit desired over original offer price, > 0, ie: 5 for 5%\n"
						"<description> description, 1 KB max. Defaults to original description. Leave as '' to use default.\n"
						+ HelpRequiringPassphrase());
	// gather inputs
	string baSig;
	COfferLinkWhitelistEntry whiteListEntry;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw runtime_error("Offer must be a valid alias");

	CTransaction aliastx;
	CAliasIndex alias;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");
    if(!IsSyscoinTxMine(aliastx, "alias")) {
		throw runtime_error("This alias is not yours.");
    }
	if (pwalletMain->GetWalletTx(aliastx.GetHash()) == NULL)
		throw runtime_error("this alias is not in your wallet");
	vector<unsigned char> vchLinkOffer = vchFromValue(params[1]);
	vector<unsigned char> vchDesc;
	// look for a transaction with this key
	CTransaction tx;
	COffer linkOffer;
	if (!GetTxOfOffer( vchLinkOffer, linkOffer, tx) || vchLinkOffer.empty())
		throw runtime_error("could not find an offer with this name");

	if(!linkOffer.vchLinkOffer.empty())
	{
		throw runtime_error("cannot link to an offer that is already linked to another offer");
	}

	int commissionInteger = atoi(params[2].get_str().c_str());
	if(commissionInteger > 255)
	{
		throw runtime_error("markup must be less than 256!");
	}
	
	if(params.size() >= 4)
	{

		vchDesc = vchFromValue(params[3]);
		if(vchDesc.size() > 0)
		{
			// 1kbyte offer desc. maxlen
			if (vchDesc.size() > MAX_VALUE_LENGTH)
				throw runtime_error("offer description cannot exceed 1023 bytes!");
		}
		else
		{
			vchDesc = linkOffer.sDescription;
		}
	}
	else
	{
		vchDesc = linkOffer.sDescription;
	}	


	COfferLinkWhitelistEntry foundEntry;
	CScript scriptPubKeyOrig, scriptPubKeyAliasOrig;
	CScript scriptPubKey, scriptPubKeyAlias;
	const CWalletTx *wtxAliasIn = NULL;

	// go through the whitelist and see if you own any of the aliases to apply to this offer for a discount
	for(unsigned int i=0;i<linkOffer.linkWhitelist.entries.size();i++) {
		CTransaction txAlias;
		CAliasIndex theAlias;
		COfferLinkWhitelistEntry& entry = linkOffer.linkWhitelist.entries[i];
		// make sure this alias is still valid
		if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
		{
			// make sure its in your wallet (you control this alias)		
			if (IsSyscoinTxMine(txAlias, "alias")) 
			{
				wtxAliasIn = pwalletMain->GetWalletTx(txAlias.GetHash());
				foundEntry = entry;
				CPubKey currentAliasKey(theAlias.vchPubKey);
				scriptPubKeyAliasOrig = GetScriptForDestination(currentAliasKey.GetID());
				if(commissionInteger <= -foundEntry.nDiscountPct)
						throw runtime_error(strprintf("You cannot re-sell at a lower price than the discount you received as an affiliate (current discount received: %d%%)", foundEntry.nDiscountPct));

			}
		}
		
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << foundEntry.aliasLinkVchRand << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyAliasOrig;
	}
	// if the whitelist exclusive mode is on and you dont have an alias in the whitelist, you cannot link to this offer
	if(foundEntry.IsNull() && linkOffer.linkWhitelist.bExclusiveResell)
	{
		throw runtime_error("Cannot link to this offer because you don't own an alias from its affiliate list (the offer is in exclusive mode)");
	}
	if(linkOffer.bOnlyAcceptBTC)
	{
		throw runtime_error("Cannot link to an offer that only accepts Bitcoins as payment");
	}
	// this is a syscoin transaction
	CWalletTx wtx;


	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));
	int precision = 2;
	// get precision
	convertCurrencyCodeToSyscoin(linkOffer.vchAliasPeg, linkOffer.sCurrencyCode, linkOffer.GetPrice(), chainActive.Tip()->nHeight, precision);
	double minPrice = pow(10.0,-precision);
	double price = linkOffer.GetPrice();
	if(price < minPrice)
		price = minPrice;

	EnsureWalletIsUnlocked();
	
	// unserialize offer from txn, serialize back
	// build offer
	COffer newOffer;
	newOffer.vchPubKey = alias.vchPubKey;
	newOffer.sDescription = vchDesc;
	newOffer.SetPrice(price);
	newOffer.nCommission = commissionInteger;
	newOffer.vchLinkOffer = vchLinkOffer;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	
	//create offeractivate txn keys
	CPubKey aliasKey(alias.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(aliasKey.GetID());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;


	string strError;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	// if we use a alias as input to this offer tx, we need another utxo for further alias transactions on this alias, so we create one here
	if(wtxAliasIn != NULL)
		vecSend.push_back(aliasRecipient);


	const vector<unsigned char> &data = newOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInCert=NULL;
	const CWalletTx * wtxInOffer=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxInOffer, wtxInCert, wtxAliasIn, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}

UniValue offerlink_nocheck(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 3 || params.size() > 4)
		throw runtime_error(
		"offerlink <alias> <guid> <commission> [description]\n"
						"<alias> An alias you own.\n"
						"<guid> offer guid that you are linking to\n"
						"<commission> percentage of profit desired over original offer price, > 0, ie: 5 for 5%\n"
						"<description> description, 1 KB max. Defaults to original description. Leave as '' to use default.\n"
						+ HelpRequiringPassphrase());
	// gather inputs
	string baSig;
	COfferLinkWhitelistEntry whiteListEntry;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw runtime_error("Offer must be a valid alias");

	CTransaction aliastx;
	CAliasIndex alias;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");
  //   if(!IsSyscoinTxMine(aliastx, "alias")) {
		// throw runtime_error("This alias is not yours.");
  //   }
	// if (pwalletMain->GetWalletTx(aliastx.GetHash()) == NULL)
	// 	throw runtime_error("this alias is not in your wallet");
	vector<unsigned char> vchLinkOffer = vchFromValue(params[1]);
	vector<unsigned char> vchDesc;
	// look for a transaction with this key
	CTransaction tx;
	COffer linkOffer;
	if (!GetTxOfOffer( vchLinkOffer, linkOffer, tx) || vchLinkOffer.empty())
		throw runtime_error("could not find an offer with this name");

	// if(!linkOffer.vchLinkOffer.empty())
	// {
	// 	throw runtime_error("cannot link to an offer that is already linked to another offer");
	// }

	int commissionInteger = atoi(params[2].get_str().c_str());
	// if(commissionInteger > 255)
	// {
	// 	throw runtime_error("markup must be less than 256!");
	// }
	
	if(params.size() >= 4)
	{

		vchDesc = vchFromValue(params[3]);
		if(vchDesc.size() > 0)
		{
			// // 1kbyte offer desc. maxlen
			// if (vchDesc.size() > MAX_VALUE_LENGTH)
			// 	throw runtime_error("offer description cannot exceed 1023 bytes!");
		}
		else
		{
			vchDesc = linkOffer.sDescription;
		}
	}
	else
	{
		vchDesc = linkOffer.sDescription;
	}	


	COfferLinkWhitelistEntry foundEntry;
	CScript scriptPubKeyOrig, scriptPubKeyAliasOrig;
	CScript scriptPubKey, scriptPubKeyAlias;
	const CWalletTx *wtxAliasIn = NULL;

	// go through the whitelist and see if you own any of the aliases to apply to this offer for a discount
	// for(unsigned int i=0;i<linkOffer.linkWhitelist.entries.size();i++) {
	// 	CTransaction txAlias;
	// 	CAliasIndex theAlias;
	// 	COfferLinkWhitelistEntry& entry = linkOffer.linkWhitelist.entries[i];
	// 	// make sure this alias is still valid
	// 	if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
	// 	{
	// 		// make sure its in your wallet (you control this alias)		
	// 		if (IsSyscoinTxMine(txAlias, "alias")) 
	// 		{
	// 			wtxAliasIn = pwalletMain->GetWalletTx(txAlias.GetHash());
	// 			foundEntry = entry;
	// 			CPubKey currentAliasKey(theAlias.vchPubKey);
	// 			scriptPubKeyAliasOrig = GetScriptForDestination(currentAliasKey.GetID());
	// 			if(commissionInteger <= -foundEntry.nDiscountPct)
	// 					throw runtime_error(strprintf("You cannot re-sell at a lower price than the discount you received as an affiliate (current discount received: %d%%)", foundEntry.nDiscountPct));

	// 		}
	// 	}
		
	// 	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << foundEntry.aliasLinkVchRand << OP_2DROP;
	// 	scriptPubKeyAlias += scriptPubKeyAliasOrig;
	// }
	// // if the whitelist exclusive mode is on and you dont have an alias in the whitelist, you cannot link to this offer
	// if(foundEntry.IsNull() && linkOffer.linkWhitelist.bExclusiveResell)
	// {
	// 	throw runtime_error("Cannot link to this offer because you don't own an alias from its affiliate list (the offer is in exclusive mode)");
	// }
	// if(linkOffer.bOnlyAcceptBTC)
	// {
	// 	throw runtime_error("Cannot link to an offer that only accepts Bitcoins as payment");
	// }
	// this is a syscoin transaction
	CWalletTx wtx;


	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));
	int precision = 2;
	// get precision
	convertCurrencyCodeToSyscoin(linkOffer.vchAliasPeg, linkOffer.sCurrencyCode, linkOffer.GetPrice(), chainActive.Tip()->nHeight, precision);
	double minPrice = pow(10.0,-precision);
	double price = linkOffer.GetPrice();
	if(price < minPrice)
		price = minPrice;

	EnsureWalletIsUnlocked();
	
	// unserialize offer from txn, serialize back
	// build offer
	COffer newOffer;
	newOffer.vchPubKey = alias.vchPubKey;
	newOffer.sDescription = vchDesc;
	newOffer.SetPrice(price);
	newOffer.nCommission = commissionInteger;
	newOffer.vchLinkOffer = vchLinkOffer;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	
	//create offeractivate txn keys
	CPubKey aliasKey(alias.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(aliasKey.GetID());
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;


	string strError;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	// if we use a alias as input to this offer tx, we need another utxo for further alias transactions on this alias, so we create one here
	if(wtxAliasIn != NULL)
		vecSend.push_back(aliasRecipient);


	const vector<unsigned char> &data = newOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInCert=NULL;
	const CWalletTx * wtxInOffer=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxInOffer, wtxInCert, wtxAliasIn, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}


UniValue offeraddwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 3)
		throw runtime_error(
		"offeraddwhitelist <offer guid> <alias guid> [discount percentage]\n"
		"Add to the affiliate list of your offer(controls who can resell).\n"
						"<offer guid> offer guid that you are adding to\n"
						"<alias guid> alias guid representing an alias that you want to add to the affiliate list\n"
						"<discount percentage> percentage of discount given to reseller for this offer. Negative discount adds on top of offer price, acts as an extra commission. -99 to 99.\n"						
						+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchAlias =  vchFromValue(params[1]);
	int nDiscountPctInteger = 0;
	
	if(params.size() >= 3)
		nDiscountPctInteger = atoi(params[2].get_str().c_str());

	if(nDiscountPctInteger < -99 || nDiscountPctInteger > 99)
		throw runtime_error("Invalid discount amount");
	CTransaction txAlias;
	CAliasIndex theAlias;
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	if (!GetTxOfAlias(vchAlias, theAlias, txAlias))
		throw runtime_error("could not find an alias with this name");

	// this is a syscoin txn
	CScript scriptPubKeyOrig;
	// create OFFERUPDATE txn key
	CScript scriptPubKey;



	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	CPubKey currentKey(theOffer.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(currentKey.GetID());

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations on that offer");
	}
	// unserialize offer from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		// make sure this alias doesn't already exist
		if (entry.aliasLinkVchRand == vchAlias)
		{
			throw runtime_error("This alias is already added to your affiliate list");
		}

	}
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	COfferLinkWhitelistEntry entry;
	entry.aliasLinkVchRand = vchAlias;
	entry.nDiscountPct = nDiscountPctInteger;
	theOffer.ClearOffer();
	theOffer.linkWhitelist.PutWhitelistEntry(entry);

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	
	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInCert=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, wtxInCert, wtxInAlias, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	return res;
}
UniValue offerremovewhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 2)
		throw runtime_error(
		"offerremovewhitelist <offer guid> <alias guid>\n"
		"Remove from the affiliate list of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);

	CTransaction txCert;
	CCert theCert;
	CWalletTx wtx;
	const CWalletTx* wtxIn = NULL;

	// this is a syscoin txn
	CScript scriptPubKeyOrig;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;

	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	CPubKey currentKey(theOffer.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(currentKey.GetID());

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations on that offer");
	}
	// unserialize offer from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
	theOffer.ClearOffer();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// create OFFERUPDATE txn keys

	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	COfferLinkWhitelistEntry entry;
	entry.aliasLinkVchRand = vchAlias;
	theOffer.linkWhitelist.PutWhitelistEntry(entry);

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	const CWalletTx * wtxInCert=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, wtxInCert, wtxInAlias, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	return res;
}
UniValue offerclearwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 1)
		throw runtime_error(
		"offerclearwhitelist <offer guid>\n"
		"Clear the affiliate list of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig;

	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	CPubKey currentKey(theOffer.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(currentKey.GetID());

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations on that offer");
	}
	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
	theOffer.ClearOffer();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	COfferLinkWhitelistEntry entry;
	// special case to clear all entries for this offer
	entry.nDiscountPct = 127;
	theOffer.linkWhitelist.PutWhitelistEntry(entry);

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInCert=NULL;
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, wtxInCert, wtxInAlias, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	return res;
}

UniValue offerwhitelist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("offerwhitelist <offer guid>\n"
                "List all affiliates for this offer.\n");
    UniValue oRes(UniValue::VARR);
    vector<unsigned char> vchOffer = vchFromValue(params[0]);
	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");
	
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CTransaction txAlias;
		CAliasIndex theAlias;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
		{
			UniValue oList(UniValue::VOBJ);
			oList.push_back(Pair("alias", stringFromVch(entry.aliasLinkVchRand)));
			int expires_in = 0;
			uint64_t nHeight = theAlias.nHeight;
			if (!GetSyscoinTransaction(nHeight, txAlias.GetHash(), txAlias, Params().GetConsensus()))
				continue;
			
            if(nHeight + GetAliasExpirationDepth() - chainActive.Tip()->nHeight > 0)
			{
				expires_in = nHeight + GetAliasExpirationDepth() - chainActive.Tip()->nHeight;
			}  
			oList.push_back(Pair("expiresin",expires_in));
			oList.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));
			oRes.push_back(oList);
		}  
    }
    return oRes;
}

UniValue offerupdate(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 5 || params.size() > 10)
		throw runtime_error(
		"offerupdate <aliaspeg> <alias> <guid> <category> <title> <quantity> <price> [description] [private=0] [cert. guid] [exclusive resell=1]\n"
						"Perform an update on an offer you control.\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAliasPeg = vchFromValue(params[0]);
	CSyscoinAddress aliasPegAddress = CSyscoinAddress(stringFromVch(vchAliasPeg));
	if (vchAliasPeg.size() > 0 && (!aliasPegAddress.IsValid() || !aliasPegAddress.isAlias))
		throw runtime_error("Invalid alias peg");
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	vector<unsigned char> vchOffer = vchFromValue(params[2]);
	vector<unsigned char> vchCat = vchFromValue(params[3]);
	vector<unsigned char> vchTitle = vchFromValue(params[4]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	bool bExclusiveResell = true;
	int bPrivate = false;
	int nQty;
	double price;
	if (params.size() >= 8) vchDesc = vchFromValue(params[7]);
	if (params.size() >= 8) bPrivate = atoi(params[8].get_str().c_str()) == 1? true: false;
	if (params.size() >= 10) vchCert = vchFromValue(params[9]);
	if(params.size() >= 11) bExclusiveResell = atoi(params[10].get_str().c_str()) == 1? true: false;

	try {
		nQty = atoi(params[5].get_str());
		price = atof(params[6].get_str().c_str());

	} catch (std::exception &e) {
		throw runtime_error("invalid price and/or quantity values. Quantity must be less than 4294967296 and greater than or equal to -1.");
	}
	if(nQty < -1)
		throw runtime_error("qty must be greater than or equal to -1");
	if (params.size() >= 8) vchDesc = vchFromValue(params[7]);
	if(price <= 0)
	{
		throw runtime_error("offer price must be greater than 0!");
	}
	
	if(vchCat.size() < 1)
        throw runtime_error("offer category cannot by empty!");
	if(vchTitle.size() < 1)
        throw runtime_error("offer title cannot be empty!");
	if(vchCat.size() > 255)
        throw runtime_error("offer category cannot exceed 255 bytes!");
	if(vchTitle.size() > 255)
        throw runtime_error("offer title cannot exceed 255 bytes!");
    // 1kbyte offer desc. maxlen
	if (vchDesc.size() > MAX_VALUE_LENGTH)
		throw runtime_error("offer description cannot exceed 1023 bytes!");
	CAliasIndex alias;
	const CWalletTx *wtxAliasIn = NULL;
	if(!vchAlias.empty())
	{
		CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
		if (!aliasAddress.IsValid())
			throw runtime_error("Invalid syscoin address");
		if (!aliasAddress.isAlias)
			throw runtime_error("Offer must be owned by a valid alias");

		CTransaction aliastx;
		if (!GetTxOfAlias(vchAlias, alias, aliastx))
			throw runtime_error("could not find an alias with this name");

		if(!IsSyscoinTxMine(aliastx, "alias")) {
			throw runtime_error("This alias is not yours.");
		}
		wtxAliasIn = pwalletMain->GetWalletTx(aliastx.GetHash());
		if (wtxAliasIn == NULL)
			throw runtime_error("this alias is not in your wallet");
	}

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;

	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx, linktx;
	COffer theOffer, linkOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	CPubKey currentKey(theOffer.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(currentKey.GetID());

	// create OFFERUPDATE, CERTUPDATE, ALIASUPDATE txn keys
	CScript scriptPubKey, scriptPubKeyCert;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations on that offer");
	}
	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");
	CCert theCert;
	CTransaction txCert;
	const CWalletTx *wtxCertIn = NULL;
	// make sure this cert is still valid
	if (GetTxOfCert( vchCert, theCert, txCert))
	{
		// check for existing cert updates
		if (ExistsInMempool(vchCert, OP_CERT_UPDATE) || ExistsInMempool(vchCert, OP_CERT_TRANSFER)) {
			throw runtime_error("there are pending operations on that cert");
		}
		vector<vector<unsigned char> > vvch;
		wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
		// make sure its in your wallet (you control this cert)		
		if (!IsSyscoinTxMine(txCert, "cert") || wtxCertIn == NULL) 
			throw runtime_error("Cannot sell this certificate, it is not yours!");
		int op, nOut;
		if(DecodeCertTx(txCert, op, nOut, vvch))
			vchCert = vvch[0];

		CPubKey currentCertKey(theCert.vchPubKey);
		scriptPubKeyCertOrig = GetScriptForDestination(currentCertKey.GetID());
		if(!theOffer.vchLinkOffer.empty())
		{
			throw runtime_error("cannot sell a cert as a linked offer");
		}
	}


	scriptPubKeyCert << CScript::EncodeOP_N(OP_CERT_UPDATE) << vchCert << OP_2DROP;
	scriptPubKeyCert += scriptPubKeyCertOrig;

	theOffer = vtxPos.back();
	COffer offerCopy = theOffer;
	theOffer.ClearOffer();	
	CAmount nRate;
	vector<string> rateList;
	// get precision & check for valid alias peg
	int precision = 2;
	if(vchAliasPeg.size() == 0)
		vchAliasPeg = offerCopy.vchAliasPeg;
	if(getCurrencyToSYSFromAlias(vchAliasPeg, offerCopy.sCurrencyCode, nRate, chainActive.Tip()->nHeight, rateList,precision) != "")
	{
		string err = strprintf("Could not find currency %s in the %s alias!\n", stringFromVch(offerCopy.sCurrencyCode), stringFromVch(vchAliasPeg));
		throw runtime_error(err.c_str());
	}

	double minPrice = pow(10.0,-precision);
	if(price < minPrice)
		price = minPrice;
	// update offer values
	if(offerCopy.sCategory != vchCat)
		theOffer.sCategory = vchCat;
	if(offerCopy.vchAliasPeg != vchAliasPeg)
		theOffer.vchAliasPeg = vchAliasPeg;
	if(offerCopy.sTitle != vchTitle)
		theOffer.sTitle = vchTitle;
	if(offerCopy.sDescription != vchDesc)
		theOffer.sDescription = vchDesc;
	// update pubkey to new cert if we change the cert we are selling for this offer or remove it
	if(wtxCertIn != NULL)
	{
		theOffer.vchCert = vchCert;
		theOffer.vchPubKey = theCert.vchPubKey;
	}
	// else if we remove the cert from this offer (don't sell a cert) then clear the cert
	else
	{
		if(!alias.IsNull())
			theOffer.vchPubKey = alias.vchPubKey;
		theOffer.vchCert.clear();
	}
	// ensure pubkey points to an alias

	CPubKey SellerPubKey(theOffer.vchPubKey);
	CSyscoinAddress selleraddy(SellerPubKey.GetID());
	selleraddy = CSyscoinAddress(selleraddy.ToString());
	if(!selleraddy.IsValid() || !selleraddy.isAlias)
		throw runtime_error("Could not detect alias from provided pub key. Check to make sure you are not trying to sell a transferred certificate.");
	
	theOffer.nQty = nQty;
	if (params.size() >= 9)
		theOffer.bPrivate = bPrivate;
	unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	if(nQty != -1 && (nQty-memPoolQty) < 0)
		throw runtime_error("not enough remaining quantity to fulfill this offerupdate");
	theOffer.nHeight = chainActive.Tip()->nHeight;
	theOffer.SetPrice(price);
	if(params.size() >= 11)
		theOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;


	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient certRecipient;
	CreateRecipient(scriptPubKeyCert, certRecipient);

	// if we use a cert as input to this offer tx, we need another utxo for further cert transactions on this cert, so we create one here
	if(wtxCertIn != NULL)
		vecSend.push_back(certRecipient);

	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, wtxCertIn, wtxInAlias, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	return res;
}

UniValue offerupdate_nocheck(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 5 || params.size() > 10)
		throw runtime_error(
		"offerupdate <alias> <guid> <category> <title> <quantity> <price> [description] [private=0] [cert. guid] [exclusive resell=1]\n"
						"Perform an update on an offer you control.\n"
						+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	vector<unsigned char> vchCat = vchFromValue(params[2]);
	vector<unsigned char> vchTitle = vchFromValue(params[3]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	bool bExclusiveResell = true;
	int bPrivate = false;
	int nQty;
	double price;
	if (params.size() >= 7) vchDesc = vchFromValue(params[6]);
	if (params.size() >= 8) bPrivate = atoi(params[7].get_str().c_str()) == 1? true: false;
	if (params.size() >= 9) vchCert = vchFromValue(params[8]);
	if(params.size() >= 10) bExclusiveResell = atoi(params[9].get_str().c_str()) == 1? true: false;

	try {
		nQty = atoi(params[4].get_str());
		price = atof(params[5].get_str().c_str());

	} catch (std::exception &e) {
		throw runtime_error("invalid price and/or quantity values. Quantity must be less than 4294967296 and greater than or equal to -1.");
	}
	// if(nQty < -1)
	// 	throw runtime_error("qty must be greater than or equal to -1");
	// if (params.size() >= 7) vchDesc = vchFromValue(params[6]);
	// if(price <= 0)
	// {
	// 	throw runtime_error("offer price must be greater than 0!");
	// }
	
	// if(vchCat.size() < 1)
 //        throw runtime_error("offer category cannot by empty!");
	// if(vchTitle.size() < 1)
 //        throw runtime_error("offer title cannot be empty!");
	// if(vchCat.size() > 255)
 //        throw runtime_error("offer category cannot exceed 255 bytes!");
	// if(vchTitle.size() > 255)
 //        throw runtime_error("offer title cannot exceed 255 bytes!");
 //    // 1kbyte offer desc. maxlen
	// if (vchDesc.size() > MAX_VALUE_LENGTH)
	// 	throw runtime_error("offer description cannot exceed 1023 bytes!");
	CAliasIndex alias;
	if(!vchAlias.empty())
	{
		const CWalletTx *wtxAliasIn = NULL;
		CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
		if (!aliasAddress.IsValid())
			throw runtime_error("Invalid syscoin address");
		if (!aliasAddress.isAlias)
			throw runtime_error("Offer must be owned by a valid alias");

		CTransaction aliastx;
		if (!GetTxOfAlias(vchAlias, alias, aliastx))
			throw runtime_error("could not find an alias with this name");

		// if(!IsSyscoinTxMine(aliastx, "alias")) {
		// 	throw runtime_error("This alias is not yours.");

		// wtxAliasIn = pwalletMain->GetWalletTx(aliastx.GetHash());
		// if (wtxAliasIn == NULL)
		// 	throw runtime_error("this alias is not in your wallet");
	}

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig, scriptPubKeyCertOrig;

	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx, linktx;
	COffer theOffer, linkOffer;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	CPubKey currentKey(theOffer.vchPubKey);
	scriptPubKeyOrig = GetScriptForDestination(currentKey.GetID());

	// create OFFERUPDATE, CERTUPDATE, ALIASUPDATE txn keys
	CScript scriptPubKey, scriptPubKeyCert;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");

	// check for existing pending offers
	// if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
	// 	throw runtime_error("there are pending operations on that offer");
	// }

	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	CCert theCert;
	CTransaction txCert;
	const CWalletTx *wtxCertIn = NULL;

	// make sure this cert is still valid
	if (GetTxOfCert( vchCert, theCert, txCert))
	{
		// check for existing cert updates
		// if (ExistsInMempool(vchCert, OP_CERT_UPDATE) || ExistsInMempool(vchCert, OP_CERT_TRANSFER)) {
		// 	throw runtime_error("there are pending operations on that cert");
		// }

		vector<vector<unsigned char> > vvch;
		wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
		// make sure its in your wallet (you control this cert)		
		// if (!IsSyscoinTxMine(txCert, "cert") || wtxCertIn == NULL) 
		// 	throw runtime_error("Cannot sell this certificate, it is not yours!");
		int op, nOut;
		if(DecodeCertTx(txCert, op, nOut, vvch))
			vchCert = vvch[0];

		CPubKey currentCertKey(theCert.vchPubKey);
		scriptPubKeyCertOrig = GetScriptForDestination(currentCertKey.GetID());
		// if(!theOffer.vchLinkOffer.empty())
		// {
		// 	throw runtime_error("cannot sell a cert as a linked offer");
		// }
	}

	scriptPubKeyCert << CScript::EncodeOP_N(OP_CERT_UPDATE) << vchCert << OP_2DROP;
	scriptPubKeyCert += scriptPubKeyCertOrig;

	theOffer = vtxPos.back();
	COffer offerCopy = theOffer;
	theOffer.ClearOffer();	

	int precision = 2;
	// get precision
	convertCurrencyCodeToSyscoin(offerCopy.vchAliasPeg, offerCopy.sCurrencyCode, offerCopy.GetPrice(), chainActive.Tip()->nHeight, precision);
	double minPrice = pow(10.0,-precision);
	if(price < minPrice)
		price = minPrice;
	// update offer values
	if(offerCopy.sCategory != vchCat)
		theOffer.sCategory = vchCat;
	if(offerCopy.sTitle != vchTitle)
		theOffer.sTitle = vchTitle;
	if(offerCopy.sDescription != vchDesc)
		theOffer.sDescription = vchDesc;
	// update pubkey to new cert if we change the cert we are selling for this offer or remove it
	if(wtxCertIn != NULL)
	{
		theOffer.vchCert = vchCert;
		theOffer.vchPubKey = theCert.vchPubKey;
	}
	// else if we remove the cert from this offer (don't sell a cert) then clear the cert
	else
	{
		if(!alias.IsNull())
			theOffer.vchPubKey = alias.vchPubKey;
		theOffer.vchCert.clear();
	}
	// ensure pubkey points to an alias

	CPubKey SellerPubKey(theOffer.vchPubKey);
	CSyscoinAddress selleraddy(SellerPubKey.GetID());
	selleraddy = CSyscoinAddress(selleraddy.ToString());
	if(!selleraddy.IsValid() || !selleraddy.isAlias)
		throw runtime_error("Could not detect alias from provided pub key. Check to make sure you are not trying to sell a transferred certificate.");
	
	theOffer.nQty = nQty;
	if (params.size() >= 8)
		theOffer.bPrivate = bPrivate;
	unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	// if(nQty != -1 && (nQty-memPoolQty) < 0)
	// 	throw runtime_error("not enough remaining quantity to fulfill this offerupdate");
	theOffer.nHeight = chainActive.Tip()->nHeight;
	theOffer.SetPrice(price);
	if(params.size() >= 10)
		theOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;


	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient certRecipient;
	CreateRecipient(scriptPubKeyCert, certRecipient);

	// if we use a cert as input to this offer tx, we need another utxo for further cert transactions on this cert, so we create one here
	if(wtxCertIn != NULL)
		vecSend.push_back(certRecipient);

	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInAlias=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, wtxCertIn, wtxInAlias, wtxInEscrow);

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	return res;
}

bool CreateLinkedOfferAcceptRecipients(vector<CRecipient> &vecSend, const CAmount &nPrice, const CWalletTx* acceptTx, const vector<unsigned char>& linkedOfferGUID, const CScript& scriptPubKeyDestination)
{
	unsigned int size = vecSend.size();
	vector<unsigned char> offerGUID;
	int op, nOut;
	
	vector<vector<unsigned char> > vvch;
	vector<vector<unsigned char> > vvchOffer;
	if(!linkedAcceptBlock)
		return false;
	if(!acceptTx)
		return false;
	if(!DecodeAndParseSyscoinTx(*acceptTx, op, nOut, vvch))
		return false;;
	if(op != OP_OFFER_ACCEPT)
		return false;
	offerGUID = vvch[0];
	// add recipients to vecSend if we find any linked accepts that are trying to accept linkedOfferGUID (they can be grouped into one accept for root offer owner)
	for (unsigned int i = 0; i < linkedAcceptBlock->vtx.size(); i++)
    {
		CScript scriptPubKey;
        const CTransaction &tx = linkedAcceptBlock->vtx[i];
		if(tx.nVersion != GetSyscoinTxVersion())
			continue;
		if(!DecodeAndParseSyscoinTx(tx, op, nOut, vvchOffer))
			continue;
		if(op != OP_OFFER_ACCEPT)
			continue;
		if(vvchOffer[0] != offerGUID)
			continue;
		// generate offer accept identifier and hash
		int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
		vector<unsigned char> vchAcceptRand = CScriptNum(rand).getvch();
		vector<unsigned char> vchAccept = vchFromString(HexStr(vchAcceptRand));
		unsigned int nQty = boost::lexical_cast<unsigned int>(stringFromVch(vvchOffer[3]));
		CAmount nTotalValue = ( nPrice * nQty );
		scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << linkedOfferGUID << vchAccept << vvchOffer[2] << vvchOffer[3] << OP_2DROP << OP_2DROP << OP_DROP; 
		scriptPubKey += scriptPubKeyDestination;
		CRecipient paymentRecipient = {scriptPubKey, nTotalValue, false};
		vecSend.push_back(paymentRecipient);
	}
	return vecSend.size() != size;
}

UniValue offeraccept(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size() || params.size() > 7)
		throw runtime_error("offeraccept <alias> <guid> [quantity] [message] [BTC TxId] [linkedacceptguidtxhash] [escrowTxHash]\n"
				"Accept&Pay for a confirmed offer.\n"
				"<alias> An alias of the buyer.\n"
				"<guid> guidkey from offer.\n"
				"<quantity> quantity to buy. Defaults to 1.\n"
				"<message> payment message to seller, 1KB max.\n"
				"<BTC TxId> If you have paid in Bitcoin and the offer is in Bitcoin, enter the transaction ID here. Default is empty.\n"
				"<linkedacceptguidtxhash> transaction id of the linking offer accept. For internal use only, leave blank\n"
				"<escrowTxHash> If this offer accept is done by an escrow release. For internal use only, leave blank\n"
				+ HelpRequiringPassphrase());

	CSyscoinAddress refundAddr;	
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	vector<unsigned char> vchPubKey;
	vector<unsigned char> vchBTCTxId = vchFromValue(params.size()>=5?params[4]:"");
	vector<unsigned char> vchLinkOfferAcceptTxHash = vchFromValue(params.size()>= 6? params[5]:"");
	vector<unsigned char> vchMessage = vchFromValue(params.size()>=4?params[3]:"");
	vector<unsigned char> vchEscrowTxHash = vchFromValue(params.size()>=7?params[6]:"");
	int64_t nHeight = chainActive.Tip()->nHeight;
	unsigned int nQty = 1;
	if (params.size() >= 3) {
		if(atof(params[2].get_str().c_str()) <= 0)
			throw runtime_error("invalid quantity value, must be greator than 0");
	
		try {
			nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
		} catch (std::exception &e) {
			throw runtime_error("invalid quantity value. Quantity must be less than 4294967296.");
		}
	}
	if(vchAlias.empty())
		throw runtime_error("You must specify an alias!");
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw runtime_error("Invalid syscoin alias");

	CAliasIndex alias;
	CTransaction aliastx;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");
    if (vchEscrowTxHash.empty())
	{
		if(!IsSyscoinTxMine(aliastx, "alias")) {
			throw runtime_error("This alias is not yours.");
		}
		if (pwalletMain->GetWalletTx(aliastx.GetHash()) == NULL)
			throw runtime_error("this alias is not in your wallet");
	}
	vchPubKey = alias.vchPubKey;
	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig, scriptPubKeyAliasOrig, scriptPubKeyEscrowOrig;
	// generate offer accept identifier and hash
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchAcceptRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchAccept = vchFromString(HexStr(vchAcceptRand));

	// create OFFERACCEPT txn keys
	CScript scriptPubKey;
	CScript scriptPubKeyEscrow, scriptPubKeyAlias;
	EnsureWalletIsUnlocked();
	CTransaction acceptTx;
	COffer theOffer;
	const CWalletTx *wtxOfferIn = NULL;
	// if this is a linked offer accept, set the height to the first height so sys_rates price will match what it was at the time of the original accept
	CTransaction tx;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
	{
		throw runtime_error("could not find an offer with this identifier");
	}
	if (!vchLinkOfferAcceptTxHash.empty())
	{
		uint256 linkTxHash(uint256S(stringFromVch(vchLinkOfferAcceptTxHash)));
		wtxOfferIn = pwalletMain->GetWalletTx(linkTxHash);
		if (wtxOfferIn == NULL)
			throw runtime_error("this offer accept is not in your wallet");	
		COffer linkOffer(*wtxOfferIn);
		if(linkOffer.accept.IsNull())
			throw runtime_error("offer accept passed into the function is not actually an offer accept");	
		vchPubKey = linkOffer.accept.vchBuyerKey;
		nHeight = linkOffer.accept.nAcceptHeight;
		nQty = linkOffer.accept.nQty;
	}
	const CWalletTx *wtxEscrowIn = NULL;
	CEscrow escrow;
	vector<vector<unsigned char> > escrowVvch;
	vector<unsigned char> vchEscrowWhitelistAlias;
	if(!vchEscrowTxHash.empty())
	{
		if(!vchBTCTxId.empty())
			throw runtime_error("Cannot release an escrow transaction by paying in Bitcoins");
		uint256 escrowTxHash(uint256S(stringFromVch(vchEscrowTxHash)));
		// make sure escrow is in wallet
		wtxEscrowIn = pwalletMain->GetWalletTx(escrowTxHash);
		if (wtxEscrowIn == NULL) 
			throw runtime_error("release escrow transaction is not in your wallet");;
		if(!escrow.UnserializeFromTx(*wtxEscrowIn))
			throw runtime_error("release escrow transaction cannot unserialize escrow value");
		
		int op, nOut;
		if (!DecodeEscrowTx(*wtxEscrowIn, op, nOut, escrowVvch))
			throw runtime_error("Cannot decode escrow tx hash");
		
		// if we want to accept an escrow release or we are accepting a linked offer from an escrow release. Override heightToCheckAgainst if using escrow since escrow can take a long time.
		// get escrow activation
		vector<CEscrow> escrowVtxPos;
		if (pescrowdb->ExistsEscrow(escrowVvch[0])) {
			if (pescrowdb->ReadEscrow(escrowVvch[0], escrowVtxPos) && !escrowVtxPos.empty())
			{	
				// we want the initial funding escrow transaction height as when to calculate this offer accept price from convertCurrencyCodeToSyscoin()
				CEscrow fundingEscrow = escrowVtxPos.front();
				vchEscrowWhitelistAlias = fundingEscrow.vchWhitelistAlias;
				// update height if it is bigger than escrow creation height, we want earlier of two, linked heifht or escrow creation to index into sysrates check
				if(nHeight > fundingEscrow.nHeight)
					nHeight = fundingEscrow.nHeight;
				CPubKey currentEscrowKey(fundingEscrow.vchSellerKey);
				scriptPubKeyEscrowOrig = GetScriptForDestination(currentEscrowKey.GetID());
				scriptPubKeyEscrow << CScript::EncodeOP_N(OP_ESCROW_COMPLETE) << escrowVvch[0] << OP_2DROP;
				scriptPubKeyEscrow += scriptPubKeyEscrowOrig;
			}
		}	
	}
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE)) {
		throw runtime_error("there are pending operations on that offer");
	}

	// load the offer data from the DB
	vector<COffer> vtxPos;
	if (pofferdb->ExistsOffer(vchOffer)) {
		if (!pofferdb->ReadOffer(vchOffer, vtxPos))
			return error(
					"failed to read from offer DB");
	}
	// get offer price at the time of accept from buyer
	theOffer.nHeight = nHeight;
	theOffer.GetOfferFromList(vtxPos);

	COffer linkedOffer;
	CTransaction tmpTx;
	// check if parent to linked offer is still valid
	if (!theOffer.vchLinkOffer.empty())
	{
		if(!vchBTCTxId.empty())
			throw runtime_error("Cannot accept a linked offer by paying in Bitcoins");

		if(pofferdb->ExistsOffer(theOffer.vchLinkOffer))
		{
			if (!GetTxOfOffer( theOffer.vchLinkOffer, linkedOffer, tmpTx))
				throw runtime_error("Trying to accept a linked offer but could not find parent offer, perhaps it is expired");
			if (linkedOffer.bOnlyAcceptBTC)
				throw runtime_error("Linked offer only accepts Bitcoins, linked offers currently only work with Syscoin payments");
		}
	}
	COfferLinkWhitelistEntry foundAlias;
	const CWalletTx *wtxAliasIn = NULL;
	vector<unsigned char> vchWhitelistAlias;
	// go through the whitelist and see if you own any of the aliases to apply to this offer for a discount
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CTransaction txAlias;
		
		CAliasIndex theAlias;
		vector<vector<unsigned char> > vvch;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		// make sure this alias is still valid
		if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
		{
			// make sure its in your wallet (you control this alias)
			// if escrow has a whitelist alias attached, use that to get the offerlinkwhitelist entry, else check the seller's whitelist to see if we own any aliases from his whitelist
			if (IsSyscoinTxMine(txAlias, "alias") || vchEscrowWhitelistAlias == entry.aliasLinkVchRand) 
			{
				// find the entry with the biggest discount, for buyers convenience
				if(entry.nDiscountPct >= foundAlias.nDiscountPct || foundAlias.nDiscountPct == 0)
				{
					wtxAliasIn = pwalletMain->GetWalletTx(txAlias.GetHash());		
					foundAlias = entry;		
					vchWhitelistAlias = entry.aliasLinkVchRand;
					CPubKey currentKey(theAlias.vchPubKey);
					scriptPubKeyAliasOrig = GetScriptForDestination(currentKey.GetID());
				}				
			}		
		}
	}

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << vchWhitelistAlias << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;

	// if this is an accept for a linked offer, the offer is set to exclusive mode and you dont have an alias in the whitelist, you cannot accept this offer
	if(wtxOfferIn != NULL && foundAlias.IsNull() && theOffer.linkWhitelist.bExclusiveResell)
	{
		throw runtime_error("cannot pay for this linked offer because you don't own an alias from its affiliate list");
	}

	unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	if(vtxPos.back().nQty != -1 && vtxPos.back().nQty < (nQty+memPoolQty))
		throw runtime_error(strprintf("not enough remaining quantity to fulfill this orderaccept, qty remaining %u, qty desired %u,  qty waiting to be accepted by the network %d", vtxPos.back().nQty, nQty, memPoolQty));

	int precision = 2;
	CAmount nPrice = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, theOffer.GetPrice(foundAlias), nHeight, precision);
	if(nPrice == 0)
		throw runtime_error(strprintf("%s currency not found in offer's alias peg %s", stringFromVch(theOffer.sCurrencyCode), stringFromVch(theOffer.vchAliasPeg)));
	string strCipherText = "";
	// encryption should only happen once even when not a resell or not an escrow accept. It is already encrypted in both cases.
	if(wtxOfferIn == NULL && vchEscrowTxHash.empty())
	{
		if(!theOffer.vchLinkOffer.empty())
		{
			// encrypt to root offer owner if this is a linked offer you are accepting
			if(!EncryptMessage(linkedOffer.vchPubKey, vchMessage, strCipherText))
				throw runtime_error("could not encrypt message to seller");
		}
		else
		{
			// encrypt to offer owner
			if(!EncryptMessage(theOffer.vchPubKey, vchMessage, strCipherText))
				throw runtime_error("could not encrypt message to seller");
		}
	}
	vector<unsigned char> vchPaymentMessage;
	if(strCipherText.size() > 0)
		vchPaymentMessage = vchFromString(strCipherText);
	else
		vchPaymentMessage = vchMessage;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << vchOffer << vchAccept << vchPaymentMessage << vchFromString(boost::lexical_cast<std::string>(nQty)) << OP_2DROP << OP_2DROP << OP_DROP;
	if(wtxOfferIn != NULL && !vchBTCTxId.empty())
		throw runtime_error("Cannot accept a linked offer by paying in Bitcoins");

	if (strCipherText.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		throw runtime_error("offeraccept message length cannot exceed 1023 bytes!");

	if(!theOffer.vchCert.empty())
	{
		// check for existing cert transfer
		if (ExistsInMempool(theOffer.vchCert, OP_CERT_TRANSFER)) {
			throw runtime_error("there is a pending transfer operation on that cert");
		}
		if(!vchBTCTxId.empty())
			throw runtime_error("Cannot purchase certificates with Bitcoins!");
		CTransaction txCert;
		CCert theCert;
		// make sure this cert is still valid
		if (!GetTxOfCert( theOffer.vchCert, theCert, txCert))
			throw runtime_error("Cannot purchase with this certificate, it may be expired!");
	}
	else{
		if (vchMessage.size() <= 0)
			throw runtime_error("offeraccept message data cannot be empty!");
	}
	// create accept
	COfferAccept txAccept;
	txAccept.vchAcceptRand = vchAccept;
	txAccept.nQty = nQty;
	txAccept.nPrice = theOffer.GetPrice(foundAlias);
	// if we have a linked offer accept then use height from linked accept (the one buyer makes, not the reseller). We need to do this to make sure we convert price at the time of initial buyer's accept.
	// in checkescrowinput we override this if its from an escrow release, just like above.
	txAccept.nAcceptHeight = nHeight;
	txAccept.vchBuyerKey = vchPubKey;
    CAmount nTotalValue = ( nPrice * nQty );
    

    CScript scriptPayment;
	CPubKey currentKey(theOffer.vchPubKey);
	scriptPayment = GetScriptForDestination(currentKey.GetID());
	scriptPubKey += scriptPayment;

	vector<CRecipient> vecSend;
	CRecipient paymentRecipient = {scriptPubKey, nTotalValue, false};
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient escrowRecipient;
	CreateRecipient(scriptPubKeyEscrow, escrowRecipient);
	// if we are accepting an escrow transaction then create another escrow utxo for escrowcomplete to be able to do its thing
	if (wtxEscrowIn != NULL) 
	{
		wtxAliasIn = NULL;
		vecSend.push_back(escrowRecipient);
	}
	// if not accepting an escrow accept, if we use a alias as input to this offer tx, we need another utxo for further alias transactions on this alias, so we create one here
	else if(wtxAliasIn != NULL)
		vecSend.push_back(aliasRecipient);

	// check for Bitcoin payment on the bitcoin network, otherwise pay in syscoin
	if(!vchBTCTxId.empty() && stringFromVch(theOffer.sCurrencyCode) == "BTC")
	{
		uint256 txBTCId(uint256S(stringFromVch(vchBTCTxId)));
		txAccept.txBTCId = txBTCId;
	}
	else if(!theOffer.bOnlyAcceptBTC)
	{
		// linked accept will go through the linkedAcceptBlock and find all linked accepts to same offer and group them together into vecSend so it can go into one tx (inputs can be shared, mainly the whitelist alias inputs)
		if(!CreateLinkedOfferAcceptRecipients(vecSend, nPrice, wtxOfferIn, vchOffer, scriptPayment))
			vecSend.push_back(paymentRecipient);
	}
	else
	{
		throw runtime_error("This offer must be paid with Bitcoins as per requirements of the seller");
	}
	theOffer.ClearOffer();
	theOffer.accept = txAccept;

	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInCert=NULL;
	// if making a purchase and we are using an alias from the whitelist of the offer, we may need to prove that we own that alias so in that case we attach an input from the alias
	// if purchasing an escrow, we adjust the height to figure out pricing of the accept so we may also attach escrow inputs to the tx
	SendMoneySyscoin(vecSend, paymentRecipient.nAmount+fee.nAmount, false, wtx, wtxOfferIn, wtxInCert, wtxAliasIn, wtxEscrowIn);
	
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(stringFromVch(vchAccept));
	return res;
}

UniValue offeraccept_nocheck(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size() || params.size() > 7)
		throw runtime_error("offeraccept <alias> <guid> [quantity] [message] [BTC TxId] [linkedacceptguidtxhash] [escrowTxHash]\n"
				"Accept&Pay for a confirmed offer.\n"
				"<alias> An alias of the buyer.\n"
				"<guid> guidkey from offer.\n"
				"<quantity> quantity to buy. Defaults to 1.\n"
				"<message> payment message to seller, 1KB max.\n"
				"<BTC TxId> If you have paid in Bitcoin and the offer is in Bitcoin, enter the transaction ID here. Default is empty.\n"
				"<linkedacceptguidtxhash> transaction id of the linking offer accept. For internal use only, leave blank\n"
				"<escrowTxHash> If this offer accept is done by an escrow release. For internal use only, leave blank\n"
				+ HelpRequiringPassphrase());

	CSyscoinAddress refundAddr;	
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchOffer = vchFromValue(params[1]);
	vector<unsigned char> vchPubKey;
	vector<unsigned char> vchBTCTxId = vchFromValue(params.size()>=5?params[4]:"");
	vector<unsigned char> vchLinkOfferAcceptTxHash = vchFromValue(params.size()>= 6? params[5]:"");
	vector<unsigned char> vchMessage = vchFromValue(params.size()>=4?params[3]:"");
	vector<unsigned char> vchEscrowTxHash = vchFromValue(params.size()>=7?params[6]:"");
	int64_t nHeight = chainActive.Tip()->nHeight;
	unsigned int nQty = 1;
	if (params.size() >= 3) {
		// if(atof(params[2].get_str().c_str()) <= 0)
		// 	throw runtime_error("invalid quantity value, must be greator than 0");
	
		try {
			nQty = boost::lexical_cast<unsigned int>(params[2].get_str());
		} catch (std::exception &e) {
			throw runtime_error("invalid quantity value. Quantity must be less than 4294967296.");
		}
	}
	if(vchAlias.empty())
		throw runtime_error("You must specify an alias!");

	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw runtime_error("Invalid syscoin alias");

	CAliasIndex alias;
	CTransaction aliastx;
	if (!GetTxOfAlias(vchAlias, alias, aliastx))
		throw runtime_error("could not find an alias with this name");
    if (vchEscrowTxHash.empty())
	{
		// if(!IsSyscoinTxMine(aliastx, "alias")) {
		// 	throw runtime_error("This alias is not yours.");
		// }
		// if (pwalletMain->GetWalletTx(aliastx.GetHash()) == NULL)
		// 	throw runtime_error("this alias is not in your wallet");
	}
	vchPubKey = alias.vchPubKey;
	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig, scriptPubKeyAliasOrig, scriptPubKeyEscrowOrig;
	// generate offer accept identifier and hash
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchAcceptRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchAccept = vchFromString(HexStr(vchAcceptRand));

	// create OFFERACCEPT txn keys
	CScript scriptPubKey;
	CScript scriptPubKeyEscrow, scriptPubKeyAlias;
	EnsureWalletIsUnlocked();
	CTransaction acceptTx;
	COffer theOffer;
	const CWalletTx *wtxOfferIn = NULL;

	// if this is a linked offer accept, set the height to the first height so sys_rates price will match what it was at the time of the original accept
	CTransaction tx;
	if (!GetTxOfOffer( vchOffer, theOffer, tx))
	{
		throw runtime_error("could not find an offer with this identifier");
	}

	if (!vchLinkOfferAcceptTxHash.empty())
	{
		uint256 linkTxHash(uint256S(stringFromVch(vchLinkOfferAcceptTxHash)));
		wtxOfferIn = pwalletMain->GetWalletTx(linkTxHash);
		if (wtxOfferIn == NULL)
			throw runtime_error("this offer accept is not in your wallet");	
		COffer linkOffer(*wtxOfferIn);
		if(linkOffer.accept.IsNull())
			throw runtime_error("offer accept passed into the function is not actually an offer accept");	
		vchPubKey = linkOffer.accept.vchBuyerKey;
		nHeight = linkOffer.accept.nAcceptHeight;
		nQty = linkOffer.accept.nQty;
	}

	const CWalletTx *wtxEscrowIn = NULL;
	CEscrow escrow;
	vector<vector<unsigned char> > escrowVvch;
	vector<unsigned char> vchEscrowWhitelistAlias;
	if(!vchEscrowTxHash.empty())
	{
		if(!vchBTCTxId.empty())
			throw runtime_error("Cannot release an escrow transaction by paying in Bitcoins");
		uint256 escrowTxHash(uint256S(stringFromVch(vchEscrowTxHash)));
		// make sure escrow is in wallet
		wtxEscrowIn = pwalletMain->GetWalletTx(escrowTxHash);
		if (wtxEscrowIn == NULL) 
			throw runtime_error("release escrow transaction is not in your wallet");;
		if(!escrow.UnserializeFromTx(*wtxEscrowIn))
			throw runtime_error("release escrow transaction cannot unserialize escrow value");
		
		int op, nOut;
		if (!DecodeEscrowTx(*wtxEscrowIn, op, nOut, escrowVvch))
			throw runtime_error("Cannot decode escrow tx hash");
		
		// if we want to accept an escrow release or we are accepting a linked offer from an escrow release. Override heightToCheckAgainst if using escrow since escrow can take a long time.
		// get escrow activation
		vector<CEscrow> escrowVtxPos;
		if (pescrowdb->ExistsEscrow(escrowVvch[0])) {
			if (pescrowdb->ReadEscrow(escrowVvch[0], escrowVtxPos) && !escrowVtxPos.empty())
			{	
				// we want the initial funding escrow transaction height as when to calculate this offer accept price from convertCurrencyCodeToSyscoin()
				CEscrow fundingEscrow = escrowVtxPos.front();
				vchEscrowWhitelistAlias = fundingEscrow.vchWhitelistAlias;
				// update height if it is bigger than escrow creation height, we want earlier of two, linked heifht or escrow creation to index into sysrates check
				if(nHeight > fundingEscrow.nHeight)
					nHeight = fundingEscrow.nHeight;
				CPubKey currentEscrowKey(fundingEscrow.vchSellerKey);
				scriptPubKeyEscrowOrig = GetScriptForDestination(currentEscrowKey.GetID());
				scriptPubKeyEscrow << CScript::EncodeOP_N(OP_ESCROW_COMPLETE) << escrowVvch[0] << OP_2DROP;
				scriptPubKeyEscrow += scriptPubKeyEscrowOrig;
			}
		}	
	}
	// if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE)) {
	// 	throw runtime_error("there are pending operations on that offer");
	//}

	// load the offer data from the DB
	vector<COffer> vtxPos;
	if (pofferdb->ExistsOffer(vchOffer)) {
		if (!pofferdb->ReadOffer(vchOffer, vtxPos))
			return error(
					"failed to read from offer DB");
	}
	// get offer price at the time of accept from buyer
	theOffer.nHeight = nHeight;
	theOffer.GetOfferFromList(vtxPos);

	COffer linkedOffer;
	CTransaction tmpTx;
	// check if parent to linked offer is still valid
	if (!theOffer.vchLinkOffer.empty())
	{
		// if(!vchBTCTxId.empty())
		// 	throw runtime_error("Cannot accept a linked offer by paying in Bitcoins");

		if(pofferdb->ExistsOffer(theOffer.vchLinkOffer))
		{
			if (!GetTxOfOffer( theOffer.vchLinkOffer, linkedOffer, tmpTx))
				throw runtime_error("Trying to accept a linked offer but could not find parent offer, perhaps it is expired");
			// if (linkedOffer.bOnlyAcceptBTC)
			// 	throw runtime_error("Linked offer only accepts Bitcoins, linked offers currently only work with Syscoin payments");
		}
	}
	COfferLinkWhitelistEntry foundAlias;
	const CWalletTx *wtxAliasIn = NULL;
	vector<unsigned char> vchWhitelistAlias;
	// go through the whitelist and see if you own any of the aliases to apply to this offer for a discount
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CTransaction txAlias;
		
		CAliasIndex theAlias;
		vector<vector<unsigned char> > vvch;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		// make sure this alias is still valid
		if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
		{
			// make sure its in your wallet (you control this alias)
			// if escrow has a whitelist alias attached, use that to get the offerlinkwhitelist entry, else check the seller's whitelist to see if we own any aliases from his whitelist
			if (IsSyscoinTxMine(txAlias, "alias") || vchEscrowWhitelistAlias == entry.aliasLinkVchRand) 
			{
				// find the entry with the biggest discount, for buyers convenience
				if(entry.nDiscountPct >= foundAlias.nDiscountPct || foundAlias.nDiscountPct == 0)
				{
					wtxAliasIn = pwalletMain->GetWalletTx(txAlias.GetHash());		
					foundAlias = entry;		
					vchWhitelistAlias = entry.aliasLinkVchRand;
					CPubKey currentKey(theAlias.vchPubKey);
					scriptPubKeyAliasOrig = GetScriptForDestination(currentKey.GetID());
				}				
			}		
		}
	}

	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << vchWhitelistAlias << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;

	// if this is an accept for a linked offer, the offer is set to exclusive mode and you dont have an alias in the whitelist, you cannot accept this offer
	// if(wtxOfferIn != NULL && foundAlias.IsNull() && theOffer.linkWhitelist.bExclusiveResell)
	// {
	// 	throw runtime_error("cannot pay for this linked offer because you don't own an alias from its affiliate list");
	// }

	// unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	// if(vtxPos.back().nQty != -1 && vtxPos.back().nQty < (nQty+memPoolQty))
	// 	throw runtime_error(strprintf("not enough remaining quantity to fulfill this orderaccept, qty remaining %u, qty desired %u,  qty waiting to be accepted by the network %d", vtxPos.back().nQty, nQty, memPoolQty));

	int precision = 2;
	CAmount nPrice = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, theOffer.GetPrice(foundAlias), nHeight, precision);
	string strCipherText = "";
	// encryption should only happen once even when not a resell or not an escrow accept. It is already encrypted in both cases.
	if(wtxOfferIn == NULL && vchEscrowTxHash.empty())
	{
		if(!theOffer.vchLinkOffer.empty())
		{
			// encrypt to root offer owner if this is a linked offer you are accepting
			if(!EncryptMessage(linkedOffer.vchPubKey, vchMessage, strCipherText))
				throw runtime_error("could not encrypt message to seller");
		}
		else
		{
			// encrypt to offer owner
			if(!EncryptMessage(theOffer.vchPubKey, vchMessage, strCipherText))
				throw runtime_error("could not encrypt message to seller");
		}
	}
	vector<unsigned char> vchPaymentMessage;
	if(strCipherText.size() > 0)
		vchPaymentMessage = vchFromString(strCipherText);
	else
		vchPaymentMessage = vchMessage;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACCEPT) << vchOffer << vchAccept << vchPaymentMessage << vchFromString(boost::lexical_cast<std::string>(nQty)) << OP_2DROP << OP_2DROP << OP_DROP;
	
	// if(wtxOfferIn != NULL && !vchBTCTxId.empty())
	// 	throw runtime_error("Cannot accept a linked offer by paying in Bitcoins");

	// if (strCipherText.size() > MAX_ENCRYPTED_VALUE_LENGTH)
	// 	throw runtime_error("offeraccept message length cannot exceed 1023 bytes!");

	// if(!theOffer.vchCert.empty())
	// {
		// check for existing cert transfer
		// if (ExistsInMempool(theOffer.vchCert, OP_CERT_TRANSFER)) {
		// 	throw runtime_error("there is a pending transfer operation on that cert");
		// }
		// if(!vchBTCTxId.empty())
		// 	throw runtime_error("Cannot purchase certificates with Bitcoins!");
		// CTransaction txCert;
		// CCert theCert;
		// make sure this cert is still valid
		// if (!GetTxOfCert( theOffer.vchCert, theCert, txCert))
		// 	throw runtime_error("Cannot purchase with this certificate, it may be expired!");
	// }
	//else{
		// if (vchMessage.size() <= 0)
		// 	throw runtime_error("offeraccept message data cannot be empty!");
//	}
	
	// create accept
	COfferAccept txAccept;
	txAccept.vchAcceptRand = vchAccept;
	txAccept.nQty = nQty;
	txAccept.nPrice = theOffer.GetPrice(foundAlias);
	// if we have a linked offer accept then use height from linked accept (the one buyer makes, not the reseller). We need to do this to make sure we convert price at the time of initial buyer's accept.
	// in checkescrowinput we override this if its from an escrow release, just like above.
	txAccept.nAcceptHeight = nHeight;
	txAccept.vchBuyerKey = vchPubKey;
    CAmount nTotalValue = ( nPrice * nQty );
    

    CScript scriptPayment;
	CPubKey currentKey(theOffer.vchPubKey);
	scriptPayment = GetScriptForDestination(currentKey.GetID());
	scriptPubKey += scriptPayment;

	vector<CRecipient> vecSend;
	CRecipient paymentRecipient = {scriptPubKey, nTotalValue, false};
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient escrowRecipient;
	CreateRecipient(scriptPubKeyEscrow, escrowRecipient);
	// if we are accepting an escrow transaction then create another escrow utxo for escrowcomplete to be able to do its thing
	if (wtxEscrowIn != NULL) 
	{
		wtxAliasIn = NULL;
		vecSend.push_back(escrowRecipient);
	}
	// if not accepting an escrow accept, if we use a alias as input to this offer tx, we need another utxo for further alias transactions on this alias, so we create one here
	else if(wtxAliasIn != NULL)
		vecSend.push_back(aliasRecipient);

	// check for Bitcoin payment on the bitcoin network, otherwise pay in syscoin
	if(!vchBTCTxId.empty() && stringFromVch(theOffer.sCurrencyCode) == "BTC")
	{
		uint256 txBTCId(uint256S(stringFromVch(vchBTCTxId)));
		txAccept.txBTCId = txBTCId;
	}
	else if(!theOffer.bOnlyAcceptBTC)
	{
		// linked accept will go through the linkedAcceptBlock and find all linked accepts to same offer and group them together into vecSend so it can go into one tx (inputs can be shared, mainly the whitelist alias inputs)
		if(!CreateLinkedOfferAcceptRecipients(vecSend, nPrice, wtxOfferIn, vchOffer, scriptPayment))
			vecSend.push_back(paymentRecipient);
	}
	else
	{
		throw runtime_error("This offer must be paid with Bitcoins as per requirements of the seller");
	}
	theOffer.ClearOffer();
	theOffer.accept = txAccept;

	const vector<unsigned char> &data = theOffer.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInCert=NULL;
	// if making a purchase and we are using an alias from the whitelist of the offer, we may need to prove that we own that alias so in that case we attach an input from the alias
	// if purchasing an escrow, we adjust the height to figure out pricing of the accept so we may also attach escrow inputs to the tx
	SendMoneySyscoin(vecSend, paymentRecipient.nAmount+fee.nAmount, false, wtx, wtxOfferIn, wtxInCert, wtxAliasIn, wtxEscrowIn);
	
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(stringFromVch(vchAccept));
	return res;
}

UniValue offerinfo(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("offerinfo <guid>\n"
				"Show values of an offer.\n");

	UniValue oLastOffer(UniValue::VOBJ);
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	string offer = stringFromVch(vchOffer);
	
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos))
		throw runtime_error("failed to read from offer DB");
	if (vtxPos.size() < 1)
		throw runtime_error("no result returned");

    // get transaction pointed to by offer
    CTransaction tx;
    uint256 txHash = vtxPos.back().txHash;
    if (!GetSyscoinTransaction(vtxPos.back().nHeight, txHash, tx, Params().GetConsensus()))
        throw runtime_error("failed to read offer transaction from disk");
    COffer theOffer;
	vector<vector<unsigned char> > vvch;
    int op, nOut;
	UniValue oOffer(UniValue::VOBJ);
	vector<unsigned char> vchValue;
	UniValue aoOfferAccepts(UniValue::VARR);
	for(int i=vtxPos.size()-1;i>=0;i--) {
		COfferAccept ca = vtxPos[i].accept;
		theOffer.nHeight = ca.nAcceptHeight;
		if(!theOffer.GetOfferFromList(vtxPos))
			continue;
		if(ca.IsNull())
			continue;
		UniValue oOfferAccept(UniValue::VOBJ);

        // get transaction pointed to by offer

        CTransaction txA;
        uint256 txHashA= ca.txHash;
        if (!GetSyscoinTransaction(ca.nHeight, txHashA, txA, Params().GetConsensus()))
		{
			error(strprintf("failed to accept read transaction from disk: %s", txHashA.GetHex()).c_str());
			continue;
		}
		for (unsigned int j = 0; j < txA.vout.size(); j++)
		{
			if (!IsSyscoinScript(txA.vout[j].scriptPubKey, op, vvch))
				continue;
			if(op != OP_OFFER_ACCEPT)
				continue;
			if(ca.vchAcceptRand == vvch[1])
				break;
		}
		if(op != OP_OFFER_ACCEPT)
			continue;
		const vector<unsigned char> &vchAcceptRand = vvch[1];	
		const vector<unsigned char> &vchMessage = vvch[2];	
		string sTime;
		CBlockIndex *pindex = chainActive[ca.nHeight];
		if (pindex) {
			sTime = strprintf("%llu", pindex->nTime);
		}
        string sHeight = strprintf("%llu", ca.nHeight);
		oOfferAccept.push_back(Pair("id", stringFromVch(vchAcceptRand)));
		oOfferAccept.push_back(Pair("txid", ca.txHash.GetHex()));
		string strBTCId = "";
		if(!ca.txBTCId.IsNull())
			strBTCId = ca.txBTCId.GetHex();
		oOfferAccept.push_back(Pair("btctxid", strBTCId));
		oOfferAccept.push_back(Pair("height", sHeight));
		oOfferAccept.push_back(Pair("time", sTime));
		oOfferAccept.push_back(Pair("quantity", strprintf("%d", ca.nQty)));
		oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
		vector<unsigned char> vchEscrowLink;
		COfferLinkWhitelistEntry entry;	
		vector<unsigned char> vchAliasLink;
		vector<unsigned char> vchOfferLink;
		vector<unsigned char> vchOfferAcceptLink;
		vector<unsigned char> vchCertLink;
		bool foundOffer = false;
		bool foundCert = false;
		bool foundEscrow = false;
		bool foundAlias = false;
		for (unsigned int i = 0; i < txA.vin.size(); i++) {
			vector<vector<unsigned char> > vvchIn;
			int opIn;
			const COutPoint *prevOutput = &txA.vin[i].prevout;
			if(!GetPreviousInput(prevOutput, opIn, vvchIn))
				continue;
			if(foundOffer && foundCert && foundEscrow && foundAlias)
				break;

			if (!foundOffer && IsOfferOp(opIn)) {
				foundOffer = true; 
				vchOfferLink = vvchIn[0];
				if(opIn == OP_OFFER_ACCEPT)
					vchOfferAcceptLink = vvchIn[1];
			}
			if (!foundEscrow && IsEscrowOp(opIn)) {
				foundEscrow = true; 
				vchEscrowLink = vvchIn[0];
			}
			if (!foundCert && IsCertOp(opIn)) {
				foundCert = true; 
				vchCertLink = vvchIn[0];
			}
			if (!foundAlias && IsAliasOp(opIn)) {
				foundAlias = true; 
				vchAliasLink = vvchIn[0];
			}
		}
		if(foundAlias)
			theOffer.linkWhitelist.GetLinkEntryByHash(vchAliasLink, entry);	
		if(foundEscrow)
		{
			vector<CEscrow> vtxEscrowPos;
			pescrowdb->ReadEscrow(vchEscrowLink, vtxEscrowPos);
			if(!vtxEscrowPos.back().vchWhitelistAlias.empty())
				theOffer.linkWhitelist.GetLinkEntryByHash(vtxEscrowPos.back().vchWhitelistAlias, entry);	
				
		}
		
		theOffer = vtxPos.back();
		string linkAccept = "";
		if(!vchOfferAcceptLink.empty())
			linkAccept = stringFromVch(vchOfferAcceptLink);
		oOfferAccept.push_back(Pair("linkofferaccept", linkAccept));
		oOfferAccept.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));			
		oOfferAccept.push_back(Pair("escrowlink", stringFromVch(vchEscrowLink)));
		int precision = 2;
		CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, ca.nPrice, ca.nAcceptHeight, precision);
		oOfferAccept.push_back(Pair("systotal", ValueFromAmount(nPricePerUnit * ca.nQty)));
		oOfferAccept.push_back(Pair("sysprice", ValueFromAmount(nPricePerUnit)));
		oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, ca.nPrice ))); 	
		oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, ca.nPrice * ca.nQty )));

		oOfferAccept.push_back(Pair("ismine", IsSyscoinTxMine(txA, "offer") ? "true" : "false"));

		if(!ca.txBTCId.IsNull())
			oOfferAccept.push_back(Pair("paid","check payment"));
		else
			oOfferAccept.push_back(Pair("paid","true"));
		string strMessage = string("");
		if(!DecryptMessage(theOffer.vchPubKey, vchMessage, strMessage))
			strMessage = string("Encrypted for owner of offer");
		oOfferAccept.push_back(Pair("pay_message", strMessage));
		aoOfferAccepts.push_back(oOfferAccept);
	}

	uint64_t nHeight;
	int expired;
	int expires_in;
	int expired_block;

	expired = 0;	
	expires_in = 0;
	expired_block = 0;
	theOffer = vtxPos.back();
    nHeight = theOffer.nHeight;
	vector<unsigned char> vchCert;
	if(!theOffer.vchCert.empty())
		vchCert = theOffer.vchCert;
	oOffer.push_back(Pair("offer", offer));
	oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
	oOffer.push_back(Pair("txid", tx.GetHash().GetHex()));
	expired_block = nHeight + GetOfferExpirationDepth();
    if(nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight <= 0)
	{
		expired = 1;
	}  
	if(expired == 0)
	{
		expires_in = nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight;
	}
	oOffer.push_back(Pair("expires_in", expires_in));
	oOffer.push_back(Pair("expired_block", expired_block));
	oOffer.push_back(Pair("expired", expired));
	oOffer.push_back(Pair("height", strprintf("%llu", nHeight)));
	CPubKey SellerPubKey(theOffer.vchPubKey);
	CSyscoinAddress selleraddy(SellerPubKey.GetID());
	selleraddy = CSyscoinAddress(selleraddy.ToString());
	oOffer.push_back(Pair("address", selleraddy.ToString()));
	oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
	oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
	if(theOffer.nQty == -1)
		oOffer.push_back(Pair("quantity", "unlimited"));
	else
		oOffer.push_back(Pair("quantity", strprintf("%d", theOffer.nQty)));
	oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
	
	
	int precision = 2;
	CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, theOffer.GetPrice(), nHeight, precision);
	oOffer.push_back(Pair("sysprice", ValueFromAmount(nPricePerUnit)));
	oOffer.push_back(Pair("price", strprintf("%.*f", precision, theOffer.GetPrice() ))); 
	
	oOffer.push_back(Pair("ismine", IsSyscoinTxMine(tx, "offer") ? "true" : "false"));
	if(!theOffer.vchLinkOffer.empty() && IsSyscoinTxMine(tx, "offer")) {
		oOffer.push_back(Pair("commission", strprintf("%d%%", theOffer.nCommission)));
		oOffer.push_back(Pair("offerlink", "true"));
		oOffer.push_back(Pair("offerlink_guid", stringFromVch(theOffer.vchLinkOffer)));
	}
	else
	{
		oOffer.push_back(Pair("commission", "0"));
		oOffer.push_back(Pair("offerlink", "false"));
		oOffer.push_back(Pair("offerlink_guid", ""));
	}
	oOffer.push_back(Pair("exclusive_resell", theOffer.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
	oOffer.push_back(Pair("private", theOffer.bPrivate ? "Yes" : "No"));
	oOffer.push_back(Pair("btconly", theOffer.bOnlyAcceptBTC ? "Yes" : "No"));
	oOffer.push_back(Pair("alias_peg", stringFromVch(theOffer.vchAliasPeg)));
	oOffer.push_back(Pair("description", stringFromVch(theOffer.sDescription)));
	oOffer.push_back(Pair("alias", selleraddy.aliasName));
	oOffer.push_back(Pair("accepts", aoOfferAccepts));
	oLastOffer = oOffer;
	
	return oLastOffer;

}
UniValue offeracceptlist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size())
		throw runtime_error("offeracceptlist [offer]\n"
				"list my offer accepts");

    vector<unsigned char> vchOffer;
	vector<unsigned char> vchOfferToFind;
    if (params.size() == 1)
        vchOfferToFind = vchFromValue(params[0]);	
	vector<unsigned char> vchEscrow;	
    UniValue oRes(UniValue::VARR);
    {

        uint256 blockHash;
        uint256 hash;
        CTransaction tx;
		
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
        {
            // get txn hash, read txn index
            hash = item.second.GetHash();

 			const CWalletTx &wtx = item.second;
            // skip non-syscoin txns
            if (wtx.nVersion != SYSCOIN_TX_VERSION)
                continue;
			for (unsigned int j = 0; j < wtx.vout.size(); j++)
			{
				UniValue oOfferAccept(UniValue::VOBJ);
				// decode txn, skip non-alias txns
				vector<vector<unsigned char> > vvch;
				int op;
				if (!IsSyscoinScript(wtx.vout[j].scriptPubKey, op, vvch))
					continue;
				if(op != OP_OFFER_ACCEPT)
					continue;


				if(vvch[0] != vchOfferToFind && !vchOfferToFind.empty())
					continue;
				const vector<unsigned char> &vchMessage = vvch[2];	
				vchOffer = vvch[0];
				
				COfferAccept theOfferAccept;

				// Check hash
				const vector<unsigned char> &vchAcceptRand = vvch[1];			
				CTransaction offerTx, acceptTx;
				COffer theOffer;
				if (!GetTxOfOfferAccept(vchOffer, vchAcceptRand, theOffer, theOfferAccept, acceptTx))
					continue;
				if(theOfferAccept.vchAcceptRand != vchAcceptRand)
					continue;

				string offer = stringFromVch(vchOffer);
				string sHeight = strprintf("%llu", theOfferAccept.nHeight);
				oOfferAccept.push_back(Pair("offer", offer));
				oOfferAccept.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
				oOfferAccept.push_back(Pair("id", stringFromVch(vchAcceptRand)));
				string strBTCId = "";
				if(!theOfferAccept.txBTCId.IsNull())
					strBTCId = theOfferAccept.txBTCId.GetHex();
				oOfferAccept.push_back(Pair("btctxid", strBTCId));
				CPubKey SellerPubKey(theOffer.vchPubKey);
				CSyscoinAddress selleraddy(SellerPubKey.GetID());
				selleraddy = CSyscoinAddress(selleraddy.ToString());
				CPubKey BuyerPubKey(theOfferAccept.vchBuyerKey);
				CSyscoinAddress buyeraddy(BuyerPubKey.GetID());
				buyeraddy = CSyscoinAddress(buyeraddy.ToString());
				oOfferAccept.push_back(Pair("alias", selleraddy.aliasName));
				oOfferAccept.push_back(Pair("buyer", buyeraddy.aliasName));
				oOfferAccept.push_back(Pair("height", sHeight));
				oOfferAccept.push_back(Pair("quantity", strprintf("%d", theOfferAccept.nQty)));
				oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
				COfferLinkWhitelistEntry entry;
				vector<unsigned char> vchEscrowLink;
				vector<unsigned char> vchCertLink;
				vector<unsigned char> vchOfferLink;
				vector<unsigned char> vchOfferAcceptLink;
				vector<unsigned char> vchAliasLink;
				bool foundOffer = false;
				bool foundEscrow = false;
				bool foundCert = false;
				bool foundAlias = false;
				for (unsigned int i = 0; i < acceptTx.vin.size(); i++) {
					vector<vector<unsigned char> > vvchIn;
					int opIn;
					const COutPoint *prevOutput = &acceptTx.vin[i].prevout;
					if(!GetPreviousInput(prevOutput, opIn, vvchIn))
						continue;
					if(foundOffer && foundEscrow && foundCert && foundAlias)
						break;

					if (!foundOffer && IsOfferOp(opIn)) {
						foundOffer = true; 
						vchOfferLink = vvchIn[0];
						if(opIn == OP_OFFER_ACCEPT)
							vchOfferAcceptLink = vvchIn[1];
					}
					if (!foundEscrow && IsEscrowOp(opIn)) {
						foundEscrow = true; 
						vchEscrowLink = vvchIn[0];
					}
					if (!foundCert && IsCertOp(opIn)) {
						foundCert = true; 
						vchCertLink = vvchIn[0];
					}
					if (!foundAlias && IsAliasOp(opIn)) {
						foundAlias = true; 
						vchAliasLink = vvchIn[0];
					}
				}
				if(foundAlias)
					theOffer.linkWhitelist.GetLinkEntryByHash(vchAliasLink, entry);
				if(foundEscrow)
				{
					vector<CEscrow> vtxEscrowPos;
					pescrowdb->ReadEscrow(vchEscrowLink, vtxEscrowPos);
					if(!vtxEscrowPos.back().vchWhitelistAlias.empty())
						theOffer.linkWhitelist.GetLinkEntryByHash(vtxEscrowPos.back().vchWhitelistAlias, entry);	
						
				}		
				string linkAccept = "";
				if(!vchOfferAcceptLink.empty())
					linkAccept = stringFromVch(vchOfferAcceptLink);
				oOfferAccept.push_back(Pair("linkofferaccept", linkAccept));
				oOfferAccept.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));
				oOfferAccept.push_back(Pair("escrowlink", stringFromVch(vchEscrowLink)));
				int precision = 2;
				CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, theOfferAccept.nPrice, theOfferAccept.nAcceptHeight, precision);
				oOfferAccept.push_back(Pair("systotal", ValueFromAmount(nPricePerUnit * theOfferAccept.nQty)));
				
				oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, theOffer.GetPrice() ))); 
				oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, theOfferAccept.nPrice * theOfferAccept.nQty ))); 
				// this accept is for me(something ive sold) if this offer is mine
				oOfferAccept.push_back(Pair("ismine", IsSyscoinTxMine(acceptTx, "offer")? "true" : "false"));

				if(!theOfferAccept.txBTCId.IsNull())
					oOfferAccept.push_back(Pair("status","check payment"));
				else
					oOfferAccept.push_back(Pair("status","paid"));

				
				string strMessage = string("");
				if(!DecryptMessage(theOffer.vchPubKey, vchMessage, strMessage))
					strMessage = string("Encrypted for owner of offer");
				oOfferAccept.push_back(Pair("pay_message", strMessage));

				oRes.push_back(oOfferAccept);
			}
        }

       BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
        {
			UniValue oOfferAccept(UniValue::VOBJ);
            if (item.second.nVersion != SYSCOIN_TX_VERSION)
                continue;

            // decode txn
            vector<vector<unsigned char> > vvch;
            int op, nOut;
            if (!DecodeEscrowTx(item.second, op, nOut, vvch) 
            	|| !IsEscrowOp(op))
                continue;
			
            vchEscrow = vvch[0];
		
			COfferAccept theOfferAccept;		

			CTransaction escrowTx;
			CEscrow theEscrow;
			COffer theOffer;
			CTransaction acceptTx;
			
			if(!GetTxOfEscrow( vchEscrow, theEscrow, escrowTx))	
				continue;
			CPubKey buyerKey(theEscrow.vchBuyerKey);
			CSyscoinAddress buyerAddress(buyerKey.GetID());
			if(!buyerAddress.IsValid() || !IsMine(*pwalletMain, buyerAddress.Get()))
				continue;
			if (!GetTxOfOfferAccept(theEscrow.vchOffer, theEscrow.vchOfferAcceptLink, theOffer, theOfferAccept, acceptTx))
				continue;

			// check for existence of offeraccept in txn offer obj
			if(theOfferAccept.vchAcceptRand != theEscrow.vchOfferAcceptLink)
				continue;	
 
            // decode txn, skip non-alias txns
            vector<vector<unsigned char> > offerVvch;
            if (!DecodeOfferTx(acceptTx, op, nOut, offerVvch) 
            	|| !IsOfferOp(op) 
            	|| (op != OP_OFFER_ACCEPT))
                continue;
			if(offerVvch[0] != vchOfferToFind && !vchOfferToFind.empty())
				continue;
			const vector<unsigned char> &vchAcceptRand = offerVvch[1];
			const vector<unsigned char> &vchMessage = offerVvch[2];
	
			string offer = stringFromVch(theEscrow.vchOffer);
			string sHeight = strprintf("%llu", theOfferAccept.nHeight);
			oOfferAccept.push_back(Pair("offer", offer));
			oOfferAccept.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
			oOfferAccept.push_back(Pair("id", stringFromVch(vchAcceptRand)));
			string strBTCId = "";
			if(!theOfferAccept.txBTCId.IsNull())
				strBTCId = theOfferAccept.txBTCId.GetHex();
			oOfferAccept.push_back(Pair("btctxid", strBTCId));
			CPubKey SellerPubKey(theOffer.vchPubKey);
			CSyscoinAddress selleraddy(SellerPubKey.GetID());
			selleraddy = CSyscoinAddress(selleraddy.ToString());
			CPubKey BuyerPubKey(theOfferAccept.vchBuyerKey);
			CSyscoinAddress buyeraddy(BuyerPubKey.GetID());
			buyeraddy = CSyscoinAddress(buyeraddy.ToString());

			oOfferAccept.push_back(Pair("alias", selleraddy.aliasName));
			oOfferAccept.push_back(Pair("buyer", buyeraddy.aliasName));
			oOfferAccept.push_back(Pair("height", sHeight));
			oOfferAccept.push_back(Pair("quantity", strprintf("%d", theOfferAccept.nQty)));
			oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
			vector<unsigned char> vchEscrowLink;
			vector<unsigned char> vchCertLink;
			vector<unsigned char> vchOfferLink;
			vector<unsigned char> vchOfferAcceptLink;
			vector<unsigned char> vchAliasLink;
			bool foundOffer = false;
			bool foundEscrow = false;
			bool foundCert = false;
			bool foundAlias = false;
			COfferLinkWhitelistEntry entry;
			for (unsigned int i = 0; i < acceptTx.vin.size(); i++) {
				vector<vector<unsigned char> > vvchIn;
				int opIn;
				const COutPoint *prevOutput = &acceptTx.vin[i].prevout;
				if(!GetPreviousInput(prevOutput, opIn, vvchIn))
					continue;
				if(foundOffer && foundEscrow && foundCert && foundAlias)
					break;

				if (!foundOffer && IsOfferOp(opIn)) {
					foundOffer = true; 
					vchOfferLink = vvchIn[0];
					if(opIn == OP_OFFER_ACCEPT)
						vchOfferAcceptLink = vvchIn[1];
				}
				if (!foundEscrow && IsEscrowOp(opIn)) {
					foundEscrow = true; 
					vchEscrowLink = vvchIn[0];
				}
				if (!foundCert && IsCertOp(opIn)) {
					foundCert = true; 
					vchCertLink = vvchIn[0];
				}
				if (!foundAlias && IsAliasOp(opIn)) {
					foundAlias = true; 
					vchAliasLink = vvchIn[0];
				}
			}
			if(foundAlias)
				theOffer.linkWhitelist.GetLinkEntryByHash(vchAliasLink, entry);
			if(foundEscrow)
			{
				vector<CEscrow> vtxEscrowPos;
				pescrowdb->ReadEscrow(vchEscrowLink, vtxEscrowPos);
				if(!vtxEscrowPos.back().vchWhitelistAlias.empty())
					theOffer.linkWhitelist.GetLinkEntryByHash(vtxEscrowPos.back().vchWhitelistAlias, entry);	
					
			}
			string linkAccept = "";
			if(!vchOfferAcceptLink.empty())
				linkAccept = stringFromVch(vchOfferAcceptLink);
			oOfferAccept.push_back(Pair("linkofferaccept", linkAccept));
			oOfferAccept.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));			
			oOfferAccept.push_back(Pair("escrowlink", stringFromVch(vchEscrowLink)));
			int precision = 2;
			CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(theOffer.vchAliasPeg, theOffer.sCurrencyCode, theOfferAccept.nPrice, theOfferAccept.nAcceptHeight, precision);
			oOfferAccept.push_back(Pair("systotal", ValueFromAmount(nPricePerUnit * theOfferAccept.nQty)));
			oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, theOfferAccept.nPrice ))); 
			oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, theOfferAccept.nPrice * theOfferAccept.nQty ))); 
			// this accept is for me(something ive sold) if this offer is mine
			oOfferAccept.push_back(Pair("ismine", IsSyscoinTxMine(acceptTx, "offer")? "true" : "false"));
			if(!theOfferAccept.txBTCId.IsNull())
				oOfferAccept.push_back(Pair("status","check payment"));
			else
				oOfferAccept.push_back(Pair("status","paid"));

			string strMessage = string("");
			if(!DecryptMessage(theOffer.vchPubKey, vchMessage, strMessage))
				strMessage = string("Encrypted for owner of offer");
			oOfferAccept.push_back(Pair("pay_message", strMessage));
			oRes.push_back(oOfferAccept);	
        }
    }

    return oRes;
}
UniValue offerlist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size())
		throw runtime_error("offerlist [offer]\n"
				"list my own offers");

    vector<unsigned char> vchName;

    if (params.size() == 1)
        vchName = vchFromValue(params[0]);

    vector<unsigned char> vchNameUniq;
    if (params.size() == 1)
        vchNameUniq = vchFromValue(params[0]);

    UniValue oRes(UniValue::VARR);
    map< vector<unsigned char>, int > vNamesI;
    map< vector<unsigned char>, UniValue > vNamesO;

    {

        uint256 blockHash;
        uint256 hash;
        CTransaction tx;
		int expired;
		int pending;
		int expires_in;
		int expired_block;
        uint64_t nHeight;

        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
        {
			expired = 0;
			pending = 0;
			expires_in = 0;
			expired_block = 0;
            // get txn hash, read txn index
            hash = item.second.GetHash();

 			const CWalletTx &wtx = item.second;

            // skip non-syscoin txns
            if (wtx.nVersion != SYSCOIN_TX_VERSION)
                continue;

            // decode txn, skip non-alias txns
            vector<vector<unsigned char> > vvch;
            int op, nOut;
            if (!DecodeOfferTx(wtx, op, nOut, vvch) 
            	|| !IsOfferOp(op) 
            	|| (op == OP_OFFER_ACCEPT))
                continue;

            // get the txn name
            vchName = vvch[0];

			// skip this offer if it doesn't match the given filter value
			if (vchNameUniq.size() > 0 && vchNameUniq != vchName)
				continue;
	
			vector<COffer> vtxPos;
			COffer theOfferA;
			if (!pofferdb->ReadOffer(vchName, vtxPos) || vtxPos.empty())
			{
				pending = 1;
				theOfferA = COffer(wtx);
				if(!IsSyscoinTxMine(wtx, "offer"))
					continue;
			}	
			else
			{
				theOfferA = vtxPos.back();
				CTransaction tx;
				if (!GetSyscoinTransaction(theOfferA.nHeight, theOfferA.txHash, tx, Params().GetConsensus()))
					continue;
				if (!DecodeOfferTx(tx, op, nOut, vvch) || !IsOfferOp(op))
					continue;
				if(!IsSyscoinTxMine(tx, "offer"))
					continue;
			}

			// get last active name only
			if (vNamesI.find(vchName) != vNamesI.end() && (theOfferA.nHeight < vNamesI[vchName] || vNamesI[vchName] < 0))
				continue;	
			nHeight = theOfferA.nHeight;
            // build the output UniValue
            UniValue oName(UniValue::VOBJ);
            oName.push_back(Pair("offer", stringFromVch(vchName)));
			vector<unsigned char> vchCert;
			if(!theOfferA.vchCert.empty())
				vchCert = theOfferA.vchCert;
			oName.push_back(Pair("cert", stringFromVch(vchCert)));
            oName.push_back(Pair("title", stringFromVch(theOfferA.sTitle)));
            oName.push_back(Pair("category", stringFromVch(theOfferA.sCategory)));
            oName.push_back(Pair("description", stringFromVch(theOfferA.sDescription)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOfferA.vchAliasPeg, theOfferA.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oName.push_back(Pair("price", strprintf("%.*f", precision, theOfferA.GetPrice() ))); 	

			oName.push_back(Pair("currency", stringFromVch(theOfferA.sCurrencyCode) ) );
			oName.push_back(Pair("commission", strprintf("%d%%", theOfferA.nCommission)));
			if(theOfferA.nQty == -1)
				oName.push_back(Pair("quantity", "unlimited"));
			else
				oName.push_back(Pair("quantity", strprintf("%d", theOfferA.nQty)));
			CPubKey SellerPubKey(theOfferA.vchPubKey);
			CSyscoinAddress selleraddy(SellerPubKey.GetID());
			selleraddy = CSyscoinAddress(selleraddy.ToString());
			oName.push_back(Pair("address", selleraddy.ToString()));
			oName.push_back(Pair("exclusive_resell", theOfferA.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
			oName.push_back(Pair("btconly", theOfferA.bOnlyAcceptBTC ? "Yes" : "No"));
			oName.push_back(Pair("alias_peg", stringFromVch(theOfferA.vchAliasPeg)));
			oName.push_back(Pair("private", theOfferA.bPrivate ? "Yes" : "No"));
			expired_block = nHeight + GetOfferExpirationDepth();
            if(pending == 0 && (nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight <= 0))
			{
				expired = 1;
			}  
			if(pending == 0 && expired == 0)
			{
				expires_in = nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight;
			}
			oName.push_back(Pair("alias", selleraddy.aliasName));
			oName.push_back(Pair("expires_in", expires_in));
			oName.push_back(Pair("expires_on", expired_block));
			oName.push_back(Pair("expired", expired));

			oName.push_back(Pair("pending", pending));

            vNamesI[vchName] = nHeight;
            vNamesO[vchName] = oName;
        }
    }

    BOOST_FOREACH(const PAIRTYPE(vector<unsigned char>, UniValue)& item, vNamesO)
        oRes.push_back(item.second);

    return oRes;
}


UniValue offerhistory(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("offerhistory <offer>\n"
				"List all stored values of an offer.\n");

	UniValue oRes(UniValue::VARR);
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	string offer = stringFromVch(vchOffer);

	{

		//vector<CDiskTxPos> vtxPos;
		vector<COffer> vtxPos;
		//COfferDB dbOffer("r");
		if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
			throw runtime_error("failed to read from offer DB");

		COffer txPos2;
		uint256 txHash;
		BOOST_FOREACH(txPos2, vtxPos) {
			txHash = txPos2.txHash;
			CTransaction tx;
			if (!GetSyscoinTransaction(txPos2.nHeight, txHash, tx, Params().GetConsensus())) {
				error("could not read txpos");
				continue;
			}
            // decode txn, skip non-alias txns
            vector<vector<unsigned char> > vvch;
            int op, nOut;
            if (!DecodeOfferTx(tx, op, nOut, vvch) 
            	|| !IsOfferOp(op) )
                continue;

			int expired = 0;
			int expires_in = 0;
			int expired_block = 0;
			UniValue oOffer(UniValue::VOBJ);
			vector<unsigned char> vchValue;
			uint64_t nHeight;
			nHeight = txPos2.nHeight;
			COffer theOfferA = txPos2;
			oOffer.push_back(Pair("offer", offer));
			string opName = offerFromOp(op);
			oOffer.push_back(Pair("offertype", opName));
			vector<unsigned char> vchCert;
			if(!theOfferA.vchCert.empty())
				vchCert = theOfferA.vchCert;
			oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
            oOffer.push_back(Pair("title", stringFromVch(theOfferA.sTitle)));
            oOffer.push_back(Pair("category", stringFromVch(theOfferA.sCategory)));
            oOffer.push_back(Pair("description", stringFromVch(theOfferA.sDescription)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOfferA.vchAliasPeg, theOfferA.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oOffer.push_back(Pair("price", strprintf("%.*f", precision, theOfferA.GetPrice() ))); 	

			oOffer.push_back(Pair("currency", stringFromVch(theOfferA.sCurrencyCode) ) );
			oOffer.push_back(Pair("commission", strprintf("%d%%", theOfferA.nCommission)));
			if(theOfferA.nQty == -1)
				oOffer.push_back(Pair("quantity", "unlimited"));
			else
				oOffer.push_back(Pair("quantity", strprintf("%d", theOfferA.nQty)));

			oOffer.push_back(Pair("txid", tx.GetHash().GetHex()));
			expired_block = nHeight + GetOfferExpirationDepth();
			if(nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight <= 0)
			{
				expired = 1;
			}  
			if(expired == 0)
			{
				expires_in = nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight;
			}
			CPubKey SellerPubKey(txPos2.vchPubKey);
			CSyscoinAddress selleraddy(SellerPubKey.GetID());
			selleraddy = CSyscoinAddress(selleraddy.ToString());
			oOffer.push_back(Pair("alias", selleraddy.aliasName));
			oOffer.push_back(Pair("expires_in", expires_in));
			oOffer.push_back(Pair("expires_on", expired_block));
			oOffer.push_back(Pair("expired", expired));
			oOffer.push_back(Pair("height", strprintf("%d", theOfferA.nHeight)));
			oRes.push_back(oOffer);
		}
	}
	return oRes;
}

UniValue offerfilter(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() > 5)
		throw runtime_error(
				"offerfilter [[[[[regexp] maxage=36000] from=0] nb=0] stat]\n"
						"scan and filter offeres\n"
						"[regexp] : apply [regexp] on offeres, empty means all offeres\n"
						"[maxage] : look in last [maxage] blocks\n"
						"[from] : show results from number [from]\n"
						"[nb] : show [nb] results, 0 means all\n"
						"[stats] : show some stats instead of results\n"
						"offerfilter \"\" 5 # list offeres updated in last 5 blocks\n"
						"offerfilter \"^offer\" # list all offeres starting with \"offer\"\n"
						"offerfilter 36000 0 0 stat # display stats (number of offers) on active offeres\n");

	string strRegexp;
	int nFrom = 0;
	int nNb = 0;
	int nMaxAge = GetOfferExpirationDepth();
	bool fStat = false;
	int nCountFrom = 0;
	int nCountNb = 0;

	if (params.size() > 0)
		strRegexp = params[0].get_str();

	if (params.size() > 1)
		nMaxAge = params[1].get_int();

	if (params.size() > 2)
		nFrom = params[2].get_int();

	if (params.size() > 3)
		nNb = params[3].get_int();

	if (params.size() > 4)
		fStat = (params[4].get_str() == "stat" ? true : false);

	//COfferDB dbOffer("r");
	UniValue oRes(UniValue::VARR);

	vector<unsigned char> vchOffer;
	vector<pair<vector<unsigned char>, COffer> > offerScan;
	if (!pofferdb->ScanOffers(vchOffer, GetOfferExpirationDepth(), offerScan))
		throw runtime_error("scan failed");

    // regexp
    using namespace boost::xpressive;
    smatch offerparts;
	smatch nameparts;
	string strRegexpLower = strRegexp;
	boost::algorithm::to_lower(strRegexpLower);
	sregex cregex = sregex::compile(strRegexpLower);
	
	pair<vector<unsigned char>, COffer> pairScan;
	BOOST_FOREACH(pairScan, offerScan) {
		const COffer &txOffer = pairScan.second;
		const string &offer = stringFromVch(pairScan.first);
		string title = stringFromVch(txOffer.sTitle);
		boost::algorithm::to_lower(title);
		string description = stringFromVch(txOffer.sDescription);
		boost::algorithm::to_lower(description);
		string category = stringFromVch(txOffer.sCategory);
		boost::algorithm::to_lower(category);
		CPubKey SellerPubKey(txOffer.vchPubKey);
		CSyscoinAddress selleraddy(SellerPubKey.GetID());
		selleraddy = CSyscoinAddress(selleraddy.ToString());
		if(!selleraddy.IsValid() || !selleraddy.isAlias)
			continue;
		string alias = selleraddy.aliasName;
		boost::algorithm::to_lower(alias);
        if (strRegexp != "" && !regex_search(title, offerparts, cregex) && !regex_search(category, offerparts, cregex) && !regex_search(description, offerparts, cregex) && strRegexp != offer && strRegexpLower != alias)
            continue;
		if(txOffer.bPrivate && strRegexp != offer)
			continue;
		int expired = 0;
		int expires_in = 0;
		int expired_block = 0;		
		int nHeight = txOffer.nHeight;

		// max age
		if (nMaxAge != 0 && chainActive.Tip()->nHeight - nHeight >= nMaxAge)
			continue;

		// from limits
		nCountFrom++;
		if (nCountFrom < nFrom + 1)
			continue;
        CTransaction tx;
		if (!GetSyscoinTransaction(txOffer.nHeight, txOffer.txHash, tx, Params().GetConsensus()))
			continue;
		// dont return sold out offers
		if(txOffer.nQty <= 0 && txOffer.nQty != -1)
			continue;
		UniValue oOffer(UniValue::VOBJ);
		oOffer.push_back(Pair("offer", offer));
			vector<unsigned char> vchCert;
			if(!txOffer.vchCert.empty())
				vchCert = txOffer.vchCert;
		oOffer.push_back(Pair("cert", stringFromVch(vchCert)));
        oOffer.push_back(Pair("title", stringFromVch(txOffer.sTitle)));
		oOffer.push_back(Pair("description", stringFromVch(txOffer.sDescription)));
        oOffer.push_back(Pair("category", stringFromVch(txOffer.sCategory)));
		int precision = 2;
		convertCurrencyCodeToSyscoin(txOffer.vchAliasPeg, txOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
		COffer foundOffer = txOffer;	
		oOffer.push_back(Pair("price", strprintf("%.*f", precision, foundOffer.GetPrice() ))); 	
		oOffer.push_back(Pair("currency", stringFromVch(txOffer.sCurrencyCode)));
		oOffer.push_back(Pair("commission", strprintf("%d%%", txOffer.nCommission)));
		if(txOffer.nQty == -1)
			oOffer.push_back(Pair("quantity", "unlimited"));
		else
			oOffer.push_back(Pair("quantity", strprintf("%d", txOffer.nQty)));
		oOffer.push_back(Pair("exclusive_resell", txOffer.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
		oOffer.push_back(Pair("btconly", txOffer.bOnlyAcceptBTC ? "Yes" : "No"));
		oOffer.push_back(Pair("alias_peg", stringFromVch(txOffer.vchAliasPeg)));
		expired_block = nHeight + GetOfferExpirationDepth();
		if(nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight <= 0)
		{
			expired = 1;
		}  
		if(expired == 0)
		{
			expires_in = nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight;
		}
		oOffer.push_back(Pair("private", txOffer.bPrivate ? "Yes" : "No"));
		oOffer.push_back(Pair("alias", selleraddy.aliasName));
		oOffer.push_back(Pair("expires_in", expires_in));
		oOffer.push_back(Pair("expires_on", expired_block));
		oOffer.push_back(Pair("expired", expired));
		oRes.push_back(oOffer);

		nCountNb++;
		// nb limits
		if (nNb > 0 && nCountNb >= nNb)
			break;
	}

	if (fStat) {
		UniValue oStat(UniValue::VOBJ);
		oStat.push_back(Pair("blocks", (int) chainActive.Tip()->nHeight));
		oStat.push_back(Pair("count", (int) oRes.size()));
		//oStat.push_back(Pair("sha256sum", SHA256(oRes), true));
		return oStat;
	}

	return oRes;
}

UniValue offerscan(const UniValue& params, bool fHelp) {
	if (fHelp || 2 > params.size())
		throw runtime_error(
				"offerscan [<start-offer>] [<max-returned>]\n"
						"scan all offers, starting at start-offer and returning a maximum number of entries (default 500)\n");

	vector<unsigned char> vchOffer;
	int nMax = 500;
	if (params.size() > 0) {
		vchOffer = vchFromValue(params[0]);
	}

	if (params.size() > 1) {
		nMax = params[1].get_int();
	}

	//COfferDB dbOffer("r");
	UniValue oRes(UniValue::VARR);

	vector<pair<vector<unsigned char>, COffer> > offerScan;
	if (!pofferdb->ScanOffers(vchOffer, nMax, offerScan))
		throw runtime_error("scan failed");

	pair<vector<unsigned char>, COffer> pairScan;
	BOOST_FOREACH(pairScan, offerScan) {
		int expired = 0;
		int expires_in = 0;
		int expired_block = 0;
		UniValue oOffer(UniValue::VOBJ);
		string offer = stringFromVch(pairScan.first);
		oOffer.push_back(Pair("offer", offer));
		CTransaction tx;
		COffer txOffer = pairScan.second;
		// dont return sold out offers
		if(txOffer.nQty <= 0 && txOffer.nQty != -1)
			continue;
		if(txOffer.bPrivate)
			continue;
		uint256 blockHash;

		int nHeight = txOffer.nHeight;
		expired_block = nHeight + GetOfferExpirationDepth();
		if(nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight <= 0)
		{
			expired = 1;
		}  
		if(expired == 0)
		{
			expires_in = nHeight + GetOfferExpirationDepth() - chainActive.Tip()->nHeight;
		}
		CPubKey SellerPubKey(txOffer.vchPubKey);
		CSyscoinAddress selleraddy(SellerPubKey.GetID());
		selleraddy = CSyscoinAddress(selleraddy.ToString());
		oOffer.push_back(Pair("alias", selleraddy.aliasName));
		oOffer.push_back(Pair("expires_in", expires_in));
		oOffer.push_back(Pair("expires_on", expired_block));
		oOffer.push_back(Pair("expired", expired));
		oRes.push_back(oOffer);
	}

	return oRes;
}
bool GetAcceptByHash(std::vector<COffer> &offerList, COfferAccept &ca) {
	if(offerList.empty())
		return false;
	for(std::vector<COffer>::reverse_iterator it = offerList.rbegin(); it != offerList.rend(); ++it) {
		const COffer& myoffer = *it;
		if(myoffer.accept.IsNull())
			continue;
        if(myoffer.accept.vchAcceptRand == ca.vchAcceptRand) {
            ca = myoffer.accept;
			return true;
        }
    }
    ca = offerList.back().accept;
	return false;
}