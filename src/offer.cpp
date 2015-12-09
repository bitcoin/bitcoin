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
#include "chainparams.h"
#include <boost/algorithm/hex.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
using namespace std;
extern void SendMoney(const CTxDestination &address, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew);
extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const string& txData="", const CWalletTx* wtxIn=NULL);
// check wallet transactions to see if there was a refund for an accept already
// need this because during a reorg blocks are disconnected (deleted from db) and we can't rely on looking in db to see if refund was made for an accept
bool foundRefundInWallet(const vector<unsigned char> &vchAcceptRand, const vector<unsigned char>& acceptCode)
{
    TRY_LOCK(pwalletMain->cs_wallet, cs_trylock);
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
    {
		vector<vector<unsigned char> > vvchArgs;
		int op, nOut;
        const CWalletTx& wtx = item.second;
        if (wtx.IsCoinBase() || !CheckFinalTx(wtx))
            continue;
		if (DecodeOfferTx(wtx, op, nOut, vvchArgs, -1))
		{
			if(op == OP_OFFER_REFUND)
			{
				if(vchAcceptRand == vvchArgs[1] && vvchArgs[2] == acceptCode)
				{
					return true;
				}
			}
		}
	}
	return false;
}
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
		if (DecodeOfferTx(wtx, op, nOut, vvchArgs, -1))
		{
			if(op == OP_OFFER_ACCEPT)
			{
				if(vvchArgs[0] == vchOffer)
				{
					COffer theOffer(wtx);
					COfferAccept theOfferAccept;
					if (theOffer.IsNull())
						continue;

					if(theOffer.GetAcceptByHash(vvchArgs[1], theOfferAccept))
					{
						if(theOfferAccept.vchLinkOfferAccept == vchAcceptRandLink)
							return true;
					}
				}
			}
		}
	}
	return false;
}
// transfer cert if its linked to offer
string makeTransferCertTX(COffer& theOffer, COfferAccept& theOfferAccept)
{

	string strPubKey = stringFromVch(theOfferAccept.vchBuyerKey);
	string strError;
	string strMethod = string("certtransfer");
	UniValue params(UniValue::VARR);

	
	params.push_back(stringFromVch(theOffer.vchCert));
	params.push_back(strPubKey);
	params.push_back("1");	
    try {
        tableRPC.execute(strMethod, params);
	}
	catch (UniValue& objError)
	{
		return find_value(objError, "message").get_str();
	}
	catch(std::exception& e)
	{
		return string(e.what());
	}
	return "";

}
// refund an offer accept by creating a transaction to send coins to offer accepter, and an offer accept back to the offer owner. 2 Step process in order to use the coins that were sent during initial accept.
string makeOfferLinkAcceptTX(COfferAccept& theOfferAccept, const vector<unsigned char> &vchPubKey, const vector<unsigned char> &vchOffer, const vector<unsigned char> &vchOfferAcceptLink)
{
	string strError;
	string strMethod = string("offeraccept");
	UniValue params(UniValue::VARR);

	CPubKey newDefaultKey;
	
	if(foundOfferLinkInWallet(vchOffer, vchOfferAcceptLink))
	{
		if(fDebug)
			printf("makeOfferLinkAcceptTX() offer linked transaction already exists\n");
		return "";
	}

	int64_t heightToCheckAgainst = theOfferAccept.nHeight;
	
	// if we want to accept an escrow release or we are accepting a linked offer from an escrow release. Override heightToCheckAgainst if using escrow since escrow can take a long time.
	if(!theOfferAccept.vchEscrowLink.empty())
	{
		// get escrow activation UniValue
		vector<CEscrow> escrowVtxPos;
		if (pescrowdb->ExistsEscrow(theOfferAccept.vchEscrowLink)) {
			if (pescrowdb->ReadEscrow(theOfferAccept.vchEscrowLink, escrowVtxPos))
			{	
				// we want the initial funding escrow transaction height as when to calculate this offer accept price from convertCurrencyCodeToSyscoin()
				CEscrow fundingEscrow = escrowVtxPos.front();
				heightToCheckAgainst = fundingEscrow.nHeight;
			}
		}

	}

	pwalletMain->GetKeyFromPool(newDefaultKey);
	CSyscoinAddress refundAddr = CSyscoinAddress(newDefaultKey.GetID());
	const vector<unsigned char> vchRefundAddress = vchFromString(refundAddr.ToString());
	params.push_back(stringFromVch(vchOffer));
	params.push_back(static_cast<ostringstream*>( &(ostringstream() << theOfferAccept.nQty) )->str());
	params.push_back(stringFromVch(vchPubKey));
	params.push_back(stringFromVch(theOfferAccept.vchMessage));
	params.push_back(stringFromVch(vchRefundAddress));
	params.push_back(stringFromVch(vchOfferAcceptLink));
	params.push_back("");
	params.push_back(static_cast<ostringstream*>( &(ostringstream() << heightToCheckAgainst) )->str());
	
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
string makeOfferRefundTX(const CTransaction& prevTx, COffer& theOffer, const COfferAccept& theOfferAccept, const vector<unsigned char> &refundCode)
{
	if(!pwalletMain)
	{
		return string("makeOfferRefundTX(): no wallet found");
	}
	if(theOfferAccept.bRefunded)
	{
		return string("This offer accept has already been refunded");
	}	
	const CWalletTx *wtxPrevIn;
	wtxPrevIn = pwalletMain->GetWalletTx(prevTx.GetHash());
	if (wtxPrevIn == NULL)
	{
		return string("makeOfferRefundTX() : can't find this offer in your wallet");
	}

	const vector<unsigned char> &vchOffer = theOffer.vchRand;
	const vector<unsigned char> &vchAcceptRand = theOfferAccept.vchRand;

	// this is a syscoin txn
	CWalletTx wtx, wtx2;
	int64_t nTotalValue = MIN_AMOUNT;
	CScript scriptPubKeyOrig, scriptPayment;

	if(refundCode == OFFER_REFUND_COMPLETE)
	{
		int precision = 2;
		COfferLinkWhitelistEntry entry;
		theOffer.linkWhitelist.GetLinkEntryByHash(theOfferAccept.vchCertLink, entry);
		// lookup the price of the offer in syscoin based on pegged alias at the block # when accept was made (sets nHeight in offeraccept serialized UniValue in tx)
		nTotalValue = convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, theOffer.GetPrice(entry), theOfferAccept.nHeight, precision)*theOfferAccept.nQty;
	} 


	// create OFFERACCEPT txn keys
	CScript scriptPubKey;
    CPubKey newDefaultKey;
    pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_REFUND)
			<< vchOffer << vchAcceptRand << refundCode
			<< OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;
	
	if (ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		return string("there are pending operations or refunds on that offer");
	}

	if(foundRefundInWallet(vchAcceptRand, refundCode))
	{
		return string("foundRefundInWallet - This offer accept has already been refunded");
	}
    // add a copy of the offer UniValue with just
    // the one accept UniValue to save bandwidth
    COffer offerCopy = theOffer;
    COfferAccept offerAcceptCopy = theOfferAccept;
    offerCopy.accepts.clear();
    offerCopy.PutOfferAccept(offerAcceptCopy);
	string bdata = offerCopy.SerializeToString();
	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxPrevIn);
	
	if(refundCode == OFFER_REFUND_COMPLETE)
	{
		CSyscoinAddress refundAddress(stringFromVch(theOfferAccept.vchRefundAddress));
		SendMoney(refundAddress.Get(), nTotalValue, false, wtx2);
	}	
	return "";

}

bool IsOfferOp(int op) {
	return op == OP_OFFER_ACTIVATE
        || op == OP_OFFER_UPDATE
        || op == OP_OFFER_ACCEPT
		|| op == OP_OFFER_REFUND;
}



int64_t GetOfferNetworkFee(opcodetype seed, unsigned int nHeight) {

	int64_t nFee = 0;
	int64_t nRate = 0;
	const vector<unsigned char> &vchCurrency = vchFromString("USD");
	vector<string> rateList;
	int precision;
	if(getCurrencyToSYSFromAlias(vchCurrency, nRate, nHeight, rateList,precision) != "")
	{
		if(seed==OP_OFFER_ACTIVATE) {
    		nFee = 50 * COIN;
		}
		else if(seed==OP_OFFER_UPDATE) {
    		nFee = 100 * COIN;
		}
	}
	else
	{
		// 10 pips USD, 10k pips = $1USD
		nFee = nRate/1000;
	}
	// Round up to CENT
	nFee += CENT - 1;
	nFee = (nFee / CENT) * CENT;
	return nFee;
}




int GetOfferExpirationDepth() {
    return 525600;
}

string offerFromOp(int op) {
	switch (op) {
	case OP_OFFER_ACTIVATE:
		return "offeractivate";
	case OP_OFFER_UPDATE:
		return "offerupdate";
	case OP_OFFER_ACCEPT:
		return "offeraccept";
	case OP_OFFER_REFUND:
		return "offerrefund";
	default:
		return "<unknown offer op>";
	}
}

bool COffer::UnserializeFromTx(const CTransaction &tx) {
	try {
		CDataStream dsOffer(vchFromString(DecodeBase64(stringFromVch(tx.data))), SER_NETWORK, PROTOCOL_VERSION);
		dsOffer >> *this;
	} catch (std::exception &e) {
		return false;
	}
	return true;
}
string COffer::SerializeToString() {
	// serialize offer UniValue
	CDataStream dsOffer(SER_NETWORK, PROTOCOL_VERSION);
	dsOffer << *this;
	vector<unsigned char> vchData(dsOffer.begin(), dsOffer.end());
	return EncodeBase64(vchData.data(), vchData.size());
}

//TODO implement
bool COfferDB::ScanOffers(const std::vector<unsigned char>& vchOffer, unsigned int nMax,
		std::vector<std::pair<std::vector<unsigned char>, COffer> >& offerScan) {
    CDBIterator *pcursor = pofferdb->NewIterator();

    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(string("offeri"), vchOffer);
    pcursor->Seek(ssKeySet.str());

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
    delete pcursor;
    return true;
}

/**
 * [COfferDB::ReconstructOfferIndex description]
 * @param  pindexRescan [description]
 * @return              [description]
 */
bool COfferDB::ReconstructOfferIndex(CBlockIndex *pindexRescan) {
    CBlockIndex* pindex = pindexRescan;  
	if(!HasReachedMainNetForkB2())
		return true;
    {
	TRY_LOCK(pwalletMain->cs_wallet, cs_trylock);
    while (pindex) {  

        int nHeight = pindex->nHeight;
        CBlock block;
        ReadBlockFromDisk(block, pindex, Params().GetConsensus());
        uint256 txblkhash;
        
        BOOST_FOREACH(CTransaction& tx, block.vtx) {
            if (tx.nVersion != SYSCOIN_TX_VERSION)
                continue;

            vector<vector<unsigned char> > vvchArgs;
            int op, nOut;

            // decode the offer op, params, height
            bool o = DecodeOfferTx(tx, op, nOut, vvchArgs, -1);
            if (!o || !IsOfferOp(op)) continue;         
            vector<unsigned char> vchOffer = vvchArgs[0];
        
            // get the transaction
            if(!GetTransaction(tx.GetHash(), tx, Params().GetConsensus(), txblkhash, true))
                continue;

            // attempt to read offer from txn
            COffer txOffer;
            COfferAccept txCA;
            if(!txOffer.UnserializeFromTx(tx))
				return error("ReconstructOfferIndex() : failed to unserialize offer from tx");

			// save serialized offer
			COffer serializedOffer = txOffer;

            // read offer from DB if it exists
            vector<COffer> vtxPos;
            if (ExistsOffer(vchOffer)) {
                if (!ReadOffer(vchOffer, vtxPos))
                    return error("ReconstructOfferIndex() : failed to read offer from DB");
                if(vtxPos.size()!=0) {
                	txOffer.nHeight = nHeight;
                	txOffer.GetOfferFromList(vtxPos);
                }
            }
			// use the txn offer as master on updates,
			// but grab the accepts from the DB first
			if(op == OP_OFFER_UPDATE || op == OP_OFFER_REFUND) {
				serializedOffer.accepts = txOffer.accepts;
				txOffer = serializedOffer;
			}

			if(op == OP_OFFER_REFUND)
			{
            	bool bReadOffer = false;
            	vector<unsigned char> vchOfferAccept = vvchArgs[1];
	            if (ExistsOfferAccept(vchOfferAccept)) {
	                if (!ReadOfferAccept(vchOfferAccept, vchOffer))
	                    printf("ReconstructOfferIndex() : warning - failed to read offer accept from offer DB\n");
	                else bReadOffer = true;
	            }
				if(!bReadOffer && !txOffer.GetAcceptByHash(vchOfferAccept, txCA))
					 return error("ReconstructOfferIndex() OP_OFFER_REFUND: failed to read offer accept from serializedOffer\n");

				if(vvchArgs[2] == OFFER_REFUND_COMPLETE){
					txCA.bRefunded = true;
					txCA.txRefundId = tx.GetHash();
				}	
				txCA.bPaid = true;
                txCA.vchRand = vvchArgs[1];
		        txCA.txHash = tx.GetHash();
				txOffer.PutOfferAccept(txCA);
				
			}
            // read the offer accept from db if exists
            if(op == OP_OFFER_ACCEPT) {
            	bool bReadOffer = false;
            	vector<unsigned char> vchOfferAccept = vvchArgs[1];
	            if (ExistsOfferAccept(vchOfferAccept)) {
	                if (!ReadOfferAccept(vchOfferAccept, vchOffer))
	                    printf("ReconstructOfferIndex() : warning - failed to read offer accept from offer DB\n");
	                else bReadOffer = true;
	            }
				if(!bReadOffer && !txOffer.GetAcceptByHash(vchOfferAccept, txCA))
					 return error("ReconstructOfferIndex() OP_OFFER_ACCEPT: failed to read offer accept from offer\n");

				// add txn-specific values to offer accept UniValue
				txCA.bPaid = true;
                txCA.vchRand = vvchArgs[1];
		        txCA.txHash = tx.GetHash();
				txOffer.PutOfferAccept(txCA);
			}


			if(op == OP_OFFER_ACTIVATE || op == OP_OFFER_UPDATE || op == OP_OFFER_REFUND) {
				txOffer.txHash = tx.GetHash();
	            txOffer.nHeight = nHeight;
			}


			// txn-specific values to offer UniValue
            txOffer.vchRand = vvchArgs[0];
            txOffer.PutToOfferList(vtxPos);

            if (!WriteOffer(vchOffer, vtxPos))
                return error("ReconstructOfferIndex() : failed to write to offer DB");
            if(op == OP_OFFER_ACCEPT || op == OP_OFFER_REFUND)
	            if (!WriteOfferAccept(vvchArgs[1], vvchArgs[0]))
	                return error("ReconstructOfferIndex() : failed to write to offer DB");
			
			if(fDebug)
				printf( "RECONSTRUCT OFFER: op=%s offer=%s title=%s qty=%u hash=%s height=%d\n",
					offerFromOp(op).c_str(),
					stringFromVch(vvchArgs[0]).c_str(),
					stringFromVch(txOffer.sTitle).c_str(),
					txOffer.nQty,
					tx.GetHash().ToString().c_str(), 
					nHeight);
			
        }
        pindex = chainActive.Next(pindex);
    }
    }
    return true;
}


int64_t GetOfferNetFee(const CTransaction& tx) {
	int64_t nFee = 0;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		if (out.scriptPubKey.size() == 1 && out.scriptPubKey[0] == OP_RETURN)
			nFee += out.nValue;
	}
	return nFee;
}

int GetOfferHeight(vector<unsigned char> vchOffer) {
	vector<COffer> vtxPos;
	if (pofferdb->ExistsOffer(vchOffer)) {
		if (!pofferdb->ReadOffer(vchOffer, vtxPos))
			return error("GetOfferHeight() : failed to read from offer DB");
		if (vtxPos.empty()) return -1;
		COffer& txPos = vtxPos.back();
		return txPos.nHeight;
	}
	return -1;
}


int IndexOfOfferOutput(const CTransaction& tx) {
	vector<vector<unsigned char> > vvch;
	int op, nOut;
	if (!DecodeOfferTx(tx, op, nOut, vvch, -1))
		throw runtime_error("IndexOfOfferOutput() : offer output not found");
	return nOut;
}

bool GetNameOfOfferTx(const CTransaction& tx, vector<unsigned char>& offer) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;
	vector<vector<unsigned char> > vvchArgs;
	int op, nOut;
	if (!DecodeOfferTx(tx, op, nOut, vvchArgs, -1))
		return error("GetNameOfOfferTx() : could not decode a syscoin tx");

	switch (op) {
		case OP_OFFER_ACTIVATE:
		case OP_OFFER_UPDATE:
		case OP_OFFER_ACCEPT:
		case OP_OFFER_REFUND:
			offer = vvchArgs[0];
			return true;
	}
	return false;
}


bool GetValueOfOfferTx(const CTransaction& tx, vector<unsigned char>& value) {
	vector<vector<unsigned char> > vvch;
	int op, nOut;

	if (!DecodeOfferTx(tx, op, nOut, vvch, -1))
		return false;

	switch (op) {
	case OP_OFFER_ACTIVATE:
	case OP_OFFER_ACCEPT:
	case OP_OFFER_UPDATE:
		value = vvch[1];
		return true;
	case OP_OFFER_REFUND:
		value = vvch[2];
		return true;
	default:
		return false;
	}
}

bool IsOfferMine(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;

	vector<vector<unsigned char> > vvch;
	int op, nOut;

	bool good = DecodeOfferTx(tx, op, nOut, vvch, -1);
	if (!good) 
		return false;
	
	if(!IsOfferOp(op))
		return false;

	const CTxOut& txout = tx.vout[nOut];
	if (pwalletMain->IsMine(txout)) {
		return true;
	}
	return false;
}

bool GetValueOfOfferTxHash(const uint256 &txHash,
		vector<unsigned char>& vchValue, uint256& hash, int& nHeight) {
	nHeight = GetTxHashHeight(txHash);
	CTransaction tx;
	uint256 blockHash;
	if (!GetTransaction(txHash, tx, Params().GetConsensus(), blockHash, true))
		return error("GetValueOfOfferTxHash() : could not read tx from disk");
	if (!GetValueOfOfferTx(tx, vchValue))
		return error("GetValueOfOfferTxHash() : could not decode value from tx");
	hash = tx.GetHash();
	return true;
}

bool GetValueOfOffer(COfferDB& dbOffer, const vector<unsigned char> &vchOffer,
		vector<unsigned char>& vchValue, int& nHeight) {
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		return false;

	COffer& txPos = vtxPos.back();
	nHeight = txPos.nHeight;
	vchValue = txPos.vchRand;
	return true;
}

bool GetTxOfOffer(COfferDB& dbOffer, const vector<unsigned char> &vchOffer, 
				  COffer& txPos, CTransaction& tx) {
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		return false;
	txPos = vtxPos.back();
	int nHeight = txPos.nHeight;
	if (nHeight + GetOfferExpirationDepth()
			< chainActive.Tip()->nHeight) {
		string offer = stringFromVch(vchOffer);
		if(fDebug)
			printf("GetTxOfOffer(%s) : expired", offer.c_str());
		return false;
	}

	uint256 hashBlock;
	if (!GetTransaction(txPos.txHash, tx, Params().GetConsensus(), hashBlock, true))
		return false;

	return true;
}

bool GetTxOfOfferAccept(COfferDB& dbOffer, const vector<unsigned char> &vchOfferAccept,
		COffer &txPos, CTransaction& tx) {
	vector<COffer> vtxPos;
	vector<unsigned char> vchOffer;
	if (!pofferdb->ReadOfferAccept(vchOfferAccept, vchOffer)) return false;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty()) return false;
	txPos = vtxPos.back();
	int nHeight = txPos.nHeight;
	if (nHeight + GetOfferExpirationDepth()
			< chainActive.Tip()->nHeight) {
		string offer = stringFromVch(vchOfferAccept);
		if(fDebug)
			printf("GetTxOfOfferAccept(%s) : expired", offer.c_str());
		return false;
	}

	uint256 hashBlock;
	if (!GetTransaction(txPos.txHash, tx, Params().GetConsensus(), hashBlock, true))
		return error("GetTxOfOfferAccept() : could not read tx from disk");

	return true;
}

bool DecodeOfferTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, int nHeight) {
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
	return found;
}


bool DecodeOfferScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch) {
	CScript::const_iterator pc = script.begin();
	return DecodeOfferScript(script, op, vvch, pc);
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

	if ((op == OP_OFFER_ACTIVATE && vvch.size() == 2)
		|| (op == OP_OFFER_UPDATE && vvch.size() == 2)
		|| (op == OP_OFFER_ACCEPT && vvch.size() == 2)
		|| (op == OP_OFFER_REFUND && vvch.size() == 3))
		return true;
	return false;
}

bool GetOfferAddress(const CTransaction& tx, std::string& strAddress) {
	int op, nOut = 0;
	vector<vector<unsigned char> > vvch;

	if (!DecodeOfferTx(tx, op, nOut, vvch, -1))
		return error("GetOfferAddress() : could not decode offer tx.");

	const CTxOut& txout = tx.vout[nOut];

	const CScript& scriptPubKey = RemoveOfferScriptPrefix(txout.scriptPubKey);
	CTxDestination dest;
	ExtractDestination(scriptPubKey, dest);
	strAddress = CSyscoinAddress(dest).ToString();
	return true;
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

bool CheckOfferInputs(const CTransaction &tx,
		CValidationState &state, const CCoinsViewCache &inputs, bool fBlock, bool fMiner,
		bool fJustCheck, int nHeight) {
	if (!tx.IsCoinBase()) {
		if (fDebug)
			printf("*** %d %d %s %s %s %s\n", nHeight,
				chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
				fBlock ? "BLOCK" : "", fMiner ? "MINER" : "",
				fJustCheck ? "JUSTCHECK" : "");

		bool found = false;
		const COutPoint *prevOutput = NULL;
		CCoins prevCoins;
		int prevOp, prevOp1;
		vector<vector<unsigned char> > vvchPrevArgs, vvchPrevArgs1;
		vvchPrevArgs.clear();
		int nIn = -1;
		// Strict check - bug disallowed, find 2 inputs max
		for (int i = 0; i < (int) tx.vin.size(); i++) {
			prevOutput = &tx.vin[i].prevout;
			inputs.GetCoins(prevOutput->hash, prevCoins);
			vector<vector<unsigned char> > vvch, vvch2, vvch3;
			if(!found)
			{
				if (DecodeOfferScript(prevCoins.vout[prevOutput->n].scriptPubKey, prevOp, vvch)) {
					found = true; 
					vvchPrevArgs = vvch;
				}
				else if (DecodeCertScript(prevCoins.vout[prevOutput->n].scriptPubKey, prevOp, vvch2))
				{
					found = true; 
					vvchPrevArgs = vvch2;
				}
			}
			else
			{
				if (DecodeOfferScript(prevCoins.vout[prevOutput->n].scriptPubKey, prevOp1, vvch)) {
					found = true; 
					vvchPrevArgs1 = vvch;
				}
				else if (DecodeCertScript(prevCoins.vout[prevOutput->n].scriptPubKey, prevOp1, vvch2))
				{
					found = true; 
					vvchPrevArgs1 = vvch2;
				}
			}
			if(!found)vvchPrevArgs.clear();
		}

		// Make sure offer outputs are not spent by a regular transaction, or the offer would be lost
		if (tx.nVersion != SYSCOIN_TX_VERSION) {
			if (found)
				return error(
						"CheckOfferInputs() : a non-syscoin transaction with a syscoin input");
			return true;
		}

		vector<vector<unsigned char> > vvchArgs;
		int op;
		int nOut;
		bool good = DecodeOfferTx(tx, op, nOut, vvchArgs, -1);
		if (!good)
			return error("CheckOfferInputs() : could not decode a syscoin tx");
		int nDepth;
		int64_t nNetFee;

		// unserialize offer UniValue from txn, check for valid
		COffer theOffer(tx);
		COfferAccept theOfferAccept;
		if (theOffer.IsNull())
			return error("CheckOfferInputs() : null offer");
		if(theOffer.sDescription.size() > 1024 * 64)
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
		if(theOffer.vchRand.size() > MAX_ID_LENGTH)
		{
			return error("offer rand too big");
		}
		if(theOffer.vchLinkOffer.size() > MAX_NAME_LENGTH)
		{
			return error("offer link guid too big");
		}
		if(theOffer.vchPubKey.size() > MAX_NAME_LENGTH)
		{
			return error("offer pub key too big");
		}
		if(theOffer.sCurrencyCode.size() > MAX_NAME_LENGTH)
		{
			return error("offer currency code too big");
		}
		if (vvchArgs[0].size() > MAX_NAME_LENGTH)
			return error("offer hex guid too long");

		switch (op) {
		case OP_OFFER_ACTIVATE:
			if (found && !IsCertOp(prevOp) )
				return error("offeractivate previous op is invalid");		
			if (found && vvchPrevArgs[0] != vvchArgs[0])
				return error("CheckOfferInputs() : offernew cert mismatch");
			if (vvchArgs[1].size() > MAX_ID_LENGTH)
				return error("offeractivate tx with guid too big");

			if (fBlock && !fJustCheck) {

					// check for enough fees
				nNetFee = GetOfferNetFee(tx);
				if (nNetFee < GetOfferNetworkFee(OP_OFFER_ACTIVATE, theOffer.nHeight))
					return error(
							"CheckOfferInputs() : OP_OFFER_ACTIVATE got tx %s with fee too low %lu",
							tx.GetHash().GetHex().c_str(),
							(long unsigned int) nNetFee);		
			}
			break;


		case OP_OFFER_UPDATE:
			if ( !found || ( prevOp != OP_OFFER_ACTIVATE && prevOp != OP_OFFER_UPDATE 
				&& prevOp != OP_OFFER_REFUND ) )
				return error("offerupdate previous op %s is invalid", offerFromOp(prevOp).c_str());
			
			if (vvchArgs[1].size() > MAX_VALUE_LENGTH)
				return error("offerupdate tx with value too long");
			
			if (vvchPrevArgs[0] != vvchArgs[0])
				return error("CheckOfferInputs() : offerupdate offer mismatch");
			if (fBlock && !fJustCheck) {
				if(vvchPrevArgs.size() > 0)
				{
					nDepth = CheckTransactionAtRelativeDepth(
							prevCoins, GetCertExpirationDepth());
					if ((fBlock || fMiner) && nDepth < 0)
						return error(
								"CheckOfferInputs() : offerupdate on an expired offer, or there is a pending transaction on the offer");	
	  
				}
				else
				{
					nDepth = CheckTransactionAtRelativeDepth(
							prevCoins, GetOfferExpirationDepth());
					if ((fBlock || fMiner) && nDepth < 0)
						return error(
								"CheckOfferInputs() : offerupdate on an expired cert previous tx, or there is a pending transaction on the cert");		  
				}

				// TODO CPU intensive
				nDepth = CheckTransactionAtRelativeDepth(
						prevCoins, GetOfferExpirationDepth());
				if ((fBlock || fMiner) && nDepth < 0)
					return error(
							"CheckOfferInputs() : offerupdate on an expired offer, or there is a pending transaction on the offer");
					// check for enough fees
				nNetFee = GetOfferNetFee(tx);
				if (nNetFee < GetOfferNetworkFee(OP_OFFER_UPDATE, theOffer.nHeight))
					return error(
							"CheckOfferInputs() : OP_OFFER_UPDATE got tx %s with fee too low %lu",
							tx.GetHash().GetHex().c_str(),
							(long unsigned int) nNetFee);		
			}
			break;
		case OP_OFFER_REFUND:
			int nDepth;
			if ( !found || ( prevOp != OP_OFFER_ACTIVATE && prevOp != OP_OFFER_UPDATE && prevOp != OP_OFFER_REFUND ))
				return error("offerrefund previous op %s is invalid", offerFromOp(prevOp).c_str());		
			if(op == OP_OFFER_REFUND && vvchArgs[2] == OFFER_REFUND_COMPLETE && vvchPrevArgs[2] != OFFER_REFUND_PAYMENT_INPROGRESS)
				return error("offerrefund complete tx must be linked to an inprogress tx");
			
			if (vvchArgs[1].size() > MAX_ID_LENGTH)
				return error("offerrefund tx with guid too big");
			if (vvchArgs[2].size() > MAX_ID_LENGTH)
				return error("offerrefund refund status too long");
			if (vvchPrevArgs[0] != vvchArgs[0])
				return error("CheckOfferInputs() : offerrefund offer mismatch");
			if (fBlock && !fJustCheck) {
				// Check hash
				const vector<unsigned char> &vchAcceptRand = vvchArgs[1];
				
				// check for existence of offeraccept in txn offer obj
				if(!theOffer.GetAcceptByHash(vchAcceptRand, theOfferAccept))
					return error("OP_OFFER_REFUND could not read accept from offer txn");
				// TODO CPU intensive
				nDepth = CheckTransactionAtRelativeDepth(
						prevCoins, GetOfferExpirationDepth());
				if ((fBlock || fMiner) && nDepth < 0)
					return error(
							"CheckOfferInputs() : offerrefund on an expired offer, or there is a pending transaction on the offer");
		
			}
			break;
		case OP_OFFER_ACCEPT:
			// if only cert input
			if (found && !vvchPrevArgs.empty() && !IsCertOp(prevOp))
				return error("CheckOfferInputs() : offeraccept cert/escrow input tx mismatch");
			if (vvchArgs[1].size() > MAX_ID_LENGTH)
				return error("offeraccept tx with guid too big");
			// check for existence of offeraccept in txn offer obj
			if(!theOffer.GetAcceptByHash(vvchArgs[1], theOfferAccept))
				return error("OP_OFFER_ACCEPT could not read accept from offer txn");
			if (theOfferAccept.vchMessage.size() > MAX_VALUE_LENGTH)
				return error("OP_OFFER_ACCEPT message field too big");
			if (theOfferAccept.vchRefundAddress.size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT refund field too big");
			if (theOfferAccept.vchLinkOfferAccept.size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT offer accept link field too big");
			if (theOfferAccept.vchCertLink.size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT cert link field too big");
			if (theOfferAccept.vchEscrowLink.size() > MAX_NAME_LENGTH)
				return error("OP_OFFER_ACCEPT escrow link field too big");
			if (fBlock && !fJustCheck) {
				if(found && IsCertOp(prevOp) && theOfferAccept.vchCertLink != vvchPrevArgs[0])
				{
					return error("theOfferAccept.vchCertlink and vvchPrevArgs[0] don't match");
				}
	   		}
			break;

		default:
			return error( "CheckOfferInputs() : offer transaction has unknown op");
		}

		
		if (fBlock || (!fBlock && !fMiner && !fJustCheck)) {
			// save serialized offer for later use
			COffer serializedOffer = theOffer;
			COffer linkOffer;
			// load the offer data from the DB
			vector<COffer> vtxPos;
			if (pofferdb->ExistsOffer(vvchArgs[0]) && !fJustCheck) {
				if (!pofferdb->ReadOffer(vvchArgs[0], vtxPos))
					return error(
							"CheckOfferInputs() : failed to read from offer DB");
			}

			if (!fMiner && !fJustCheck && chainActive.Tip()->nHeight != nHeight) {
				int nHeight = chainActive.Tip()->nHeight;
				// get the latest offer from the db
            	theOffer.nHeight = nHeight;
            	theOffer.GetOfferFromList(vtxPos);

				// If update, we make the serialized offer the master
				// but first we assign the accepts from the DB since
				// they are not shipped in an update txn to keep size down
				if(op == OP_OFFER_UPDATE) {
					serializedOffer.accepts = theOffer.accepts;
					theOffer = serializedOffer;
				}
				else if(op == OP_OFFER_ACTIVATE)
				{
					// if this is a linked offer activate, then add it to the parent offerLinks list
					if(!theOffer.vchLinkOffer.empty())
					{
						vector<COffer> myVtxPos;
						if (pofferdb->ExistsOffer(theOffer.vchLinkOffer)) {
							if (pofferdb->ReadOffer(theOffer.vchLinkOffer, myVtxPos))
							{
								COffer myParentOffer = myVtxPos.back();
								myParentOffer.offerLinks.push_back(vvchArgs[0]);							
								myParentOffer.PutToOfferList(myVtxPos);
								{
								TRY_LOCK(cs_main, cs_trymain);
								// write parent offer
								if (!pofferdb->WriteOffer(theOffer.vchLinkOffer, myVtxPos))
									return error( "CheckOfferInputs() : failed to write to offer link to DB");
								}
							}
						}
						
					}
				}
				else if(op == OP_OFFER_REFUND)
				{
					serializedOffer.accepts = theOffer.accepts;
					theOffer = serializedOffer;
					vector<unsigned char> vchOfferAccept = vvchArgs[1];
					if (pofferdb->ExistsOfferAccept(vchOfferAccept)) {
						if (!pofferdb->ReadOfferAccept(vchOfferAccept, vvchArgs[0]))
						{
							return error("CheckOfferInputs()- OP_OFFER_REFUND: failed to read offer accept from offer DB\n");
						}

					}

					if(!theOffer.GetAcceptByHash(vchOfferAccept, theOfferAccept))
						return error("CheckOfferInputs()- OP_OFFER_REFUND: could not read accept from serializedOffer txn");	
            		
					if(!fInit && pwalletMain && vvchArgs[2] == OFFER_REFUND_PAYMENT_INPROGRESS){
						CTransaction offerTx;
						COffer tmpOffer;
						if(!GetTxOfOffer(*pofferdb, vvchArgs[0], tmpOffer, offerTx))	
						{
							return error("CheckOfferInputs() - OP_OFFER_REFUND: failed to get offer transaction");
						}
						// we want to refund my offer accepts and any linked offeraccepts of my offer						
						if(IsOfferMine(offerTx))
						{
							string strError = makeOfferRefundTX(tx, theOffer, theOfferAccept, OFFER_REFUND_COMPLETE);
							if (strError != "" && fDebug)
							{
								printf("CheckOfferInputs() - OFFER_REFUND_COMPLETE %s\n", strError.c_str());
							}

						}

						CTransaction myOfferTx;
						COffer activateOffer;
						COfferAccept myAccept;
						// if this accept was done via offer linking (makeOfferLinkAcceptTX) then walk back up and refund
						if(GetTxOfOfferAccept(*pofferdb, theOfferAccept.vchLinkOfferAccept, activateOffer, myOfferTx))
						{
							if(!activateOffer.GetAcceptByHash(theOfferAccept.vchLinkOfferAccept, myAccept))
								return error("CheckOfferInputs()- OFFER_REFUND_PAYMENT_INPROGRESS: could not read accept from offer txn");	
							if(IsOfferMine(myOfferTx))
							{
								string strError = makeOfferRefundTX(myOfferTx, activateOffer, myAccept, OFFER_REFUND_PAYMENT_INPROGRESS);
								if (strError != "" && fDebug)							
									printf("CheckOfferInputs() - OFFER_REFUND_PAYMENT_INPROGRESS %s\n", strError.c_str());
									
							}
						}
					}
					else if(vvchArgs[2] == OFFER_REFUND_COMPLETE){
						theOfferAccept.bRefunded = true;
						theOfferAccept.txRefundId = tx.GetHash();
					}
					theOffer.PutOfferAccept(theOfferAccept);
					{
					TRY_LOCK(cs_main, cs_trymain);
					// write the offer / offer accept mapping to the database
					if (!pofferdb->WriteOfferAccept(vvchArgs[1], vvchArgs[0]))
						return error( "CheckOfferInputs() : failed to write to offer DB");
					}
					
				}
				else if (op == OP_OFFER_ACCEPT) {
					
					COffer myOffer,linkOffer;
					CTransaction offerTx, linkedTx;			
					// find the payment from the tx outputs (make sure right amount of coins were paid for this offer accept), the payment amount found has to be exact
					if(tx.vout.size() > 1)
					{	
						bool foundPayment = false;
						uint64_t heightToCheckAgainst = theOfferAccept.nHeight;
						COfferLinkWhitelistEntry entry;
						if(IsCertOp(prevOp) && found)
						{	
							theOffer.linkWhitelist.GetLinkEntryByHash(theOfferAccept.vchCertLink, entry);						
						}
						// if this accept was done via an escrow release, we get the height from escrow and use that to lookup the price at the time
						if(!theOfferAccept.vchEscrowLink.empty())
						{	
							vector<CEscrow> escrowVtxPos;
							if (pescrowdb->ExistsEscrow(theOfferAccept.vchEscrowLink)) {
								if (pescrowdb->ReadEscrow(theOfferAccept.vchEscrowLink, escrowVtxPos))
								{	
									// we want the initial funding escrow transaction height as when to calculate this offer accept price
									CEscrow fundingEscrow = escrowVtxPos.front();
									heightToCheckAgainst = fundingEscrow.nHeight;
								}
							}
						}

						int precision = 2;
						// lookup the price of the offer in syscoin based on pegged alias at the block # when accept/escrow was made
						int64_t nPrice = convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, theOffer.GetPrice(entry), heightToCheckAgainst, precision)*theOfferAccept.nQty;
						for(unsigned int i=0;i<tx.vout.size();i++)
						{
							if(tx.vout[i].nValue == nPrice)
							{
								foundPayment = true;
								break;
							}
						}
						if(!foundPayment)
						{
							if(fDebug)
								printf("CheckOfferInputs() OP_OFFER_ACCEPT: this offer accept does not pay enough according to the offer price %llu\n", nPrice);
							return true;
						}
					}
					else
					{
						if(fDebug)
							printf("CheckOfferInputs() OP_OFFER_ACCEPT: offer payment not found in accept tx\n");
						return true;
					}
					if (!GetTxOfOffer(*pofferdb, vvchArgs[0], myOffer, offerTx))
						return error("CheckOfferInputs() OP_OFFER_ACCEPT: could not find an offer with this name");

					if(!myOffer.vchLinkOffer.empty())
					{
						if(!GetTxOfOffer(*pofferdb, myOffer.vchLinkOffer, linkOffer, linkedTx))
							linkOffer.SetNull();
					}
											
					// check for existence of offeraccept in txn offer obj
					if(!serializedOffer.GetAcceptByHash(vvchArgs[1], theOfferAccept))
						return error("CheckOfferInputs() OP_OFFER_ACCEPT: could not read accept from offer txn");					


					// 2 step refund: send an offer accept with nRefunded property set to inprogress and then send another with complete later
					// first step is to send inprogress so that next block we can send a complete (also sends coins during second step to original acceptor)
					// this is to ensure that the coins sent during accept are available to spend to refund to avoid having to hold double the balance of an accept amount
					// in order to refund.
					if(theOfferAccept.nQty <= 0 || (theOfferAccept.nQty > theOffer.nQty || (!linkOffer.IsNull() && theOfferAccept.nQty > linkOffer.nQty))) {

						if(pwalletMain && IsOfferMine(offerTx) && theOfferAccept.nQty > 0)
						{	
							string strError = makeOfferRefundTX(offerTx, theOffer, theOfferAccept, OFFER_REFUND_PAYMENT_INPROGRESS);
							if (strError != "" && fDebug)
								printf("CheckOfferInputs() - OP_OFFER_ACCEPT %s\n", strError.c_str());
							
						}
						if(fDebug)
							printf("txn %s accepted but offer not fulfilled because desired"
							" qty %u is more than available qty %u for offer accept %s\n", 
							tx.GetHash().GetHex().c_str(), 
							theOfferAccept.nQty, 
							theOffer.nQty, 
							stringFromVch(theOfferAccept.vchRand).c_str());
						return true;
					}
					if(theOffer.vchLinkOffer.empty())
					{
						theOffer.nQty -= theOfferAccept.nQty;
						// purchased a cert so xfer it
						if(!fInit && pwalletMain && IsOfferMine(offerTx) && !theOffer.vchCert.empty())
						{
							string strError = makeTransferCertTX(theOffer, theOfferAccept);
							if(strError != "")
							{
								if(fDebug)
								{
									printf("CheckOfferInputs() - OP_OFFER_ACCEPT - makeTransferCertTX %s\n", strError.c_str());
								}
							}
						}
						// go through the linked offers, if any, and update the linked offer qty based on the this qty
						for(unsigned int i=0;i<theOffer.offerLinks.size();i++) {
							vector<COffer> myVtxPos;
							if (pofferdb->ExistsOffer(theOffer.offerLinks[i])) {
								if (pofferdb->ReadOffer(theOffer.offerLinks[i], myVtxPos))
								{
									COffer myLinkOffer = myVtxPos.back();
									myLinkOffer.nQty = theOffer.nQty;	
									myLinkOffer.PutToOfferList(myVtxPos);
									{
									TRY_LOCK(cs_main, cs_trymain);
									// write offer
									if (!pofferdb->WriteOffer(theOffer.offerLinks[i], myVtxPos))
										return error( "CheckOfferInputs() : failed to write to offer link to DB");
									}
								}
							}
						}
					}
					if (!fInit && pwalletMain && !linkOffer.IsNull() && IsOfferMine(offerTx))
					{	
						// vchPubKey is for when transfering cert after an offer accept, the pubkey is the transfer-to address and encryption key for cert data
						// myOffer.vchLinkOffer is the linked offer guid
						// vvchArgs[1] is this offer accept rand used to walk back up and refund offers in the linked chain
						// we are now accepting the linked	 offer, up the link offer stack.

						string strError = makeOfferLinkAcceptTX(theOfferAccept, vchFromString(""), myOffer.vchLinkOffer, vvchArgs[1]);
						if(strError != "")
						{
							if(fDebug)
							{
								printf("CheckOfferInputs() - OP_OFFER_ACCEPT - makeOfferLinkAcceptTX %s\n", strError.c_str());
							}
							// if there is a problem refund this accept
							strError = makeOfferRefundTX(offerTx, theOffer, theOfferAccept, OFFER_REFUND_PAYMENT_INPROGRESS);
							if (strError != "" && fDebug)
								printf("CheckOfferInputs() - OP_OFFER_ACCEPT - makeOfferLinkAcceptTX(makeOfferRefundTX) %s\n", strError.c_str());

						}
					}
					
					
					theOfferAccept.bPaid = true;
					
				
					// set the offer accept txn-dependent values and add to the txn
					theOfferAccept.vchRand = vvchArgs[1];
					theOfferAccept.txHash = tx.GetHash();
					theOffer.PutOfferAccept(theOfferAccept);
					{
					TRY_LOCK(cs_main, cs_trymain);
					// write the offer / offer accept mapping to the database
					if (!pofferdb->WriteOfferAccept(vvchArgs[1], vvchArgs[0]))
						return error( "CheckOfferInputs() : failed to write to offer DB");
					}
				}
				
				// only modify the offer's height on an activate or update or refund
				if(op == OP_OFFER_ACTIVATE || op == OP_OFFER_UPDATE ||  op == OP_OFFER_REFUND) {
					theOffer.nHeight = chainActive.Tip()->nHeight;					
					theOffer.txHash = tx.GetHash();
					if(op == OP_OFFER_UPDATE)
					{
						// if this offer is linked to a parent update it with parent information
						if(!theOffer.vchLinkOffer.empty())
						{
							vector<COffer> myVtxPos;
							if (pofferdb->ExistsOffer(theOffer.vchLinkOffer)) {
								if (pofferdb->ReadOffer(theOffer.vchLinkOffer, myVtxPos))
								{
									COffer myLinkOffer = myVtxPos.back();
									theOffer.nQty = myLinkOffer.nQty;	
									theOffer.nHeight = myLinkOffer.nHeight;
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
										myLinkOffer.nHeight = theOffer.nHeight;
										myLinkOffer.SetPrice(theOffer.nPrice);
										myLinkOffer.PutToOfferList(myVtxPos);
										{
										TRY_LOCK(cs_main, cs_trymain);
										// write offer
										if (!pofferdb->WriteOffer(theOffer.offerLinks[i], myVtxPos))
											return error( "CheckOfferInputs() : failed to write to offer link to DB");
										}
									}
								}
							}
							
						}
					}
				}
				// set the offer's txn-dependent values
                theOffer.vchRand = vvchArgs[0];
				theOffer.PutToOfferList(vtxPos);
				{
				TRY_LOCK(cs_main, cs_trymain);
				// write offer
				if (!pofferdb->WriteOffer(vvchArgs[0], vtxPos))
					return error( "CheckOfferInputs() : failed to write to offer DB");

               				
				// debug
				if (fDebug)
					printf( "CONNECTED OFFER: op=%s offer=%s title=%s qty=%u hash=%s height=%d\n",
						offerFromOp(op).c_str(),
						stringFromVch(vvchArgs[0]).c_str(),
						stringFromVch(theOffer.sTitle).c_str(),
						theOffer.nQty,
						tx.GetHash().ToString().c_str(), 
						nHeight);
				}
			}
		}
	}
	return true;
}


void rescanforoffers(CBlockIndex *pindexRescan) {
    printf("Scanning blockchain for offers to create fast index...\n");
    pofferdb->ReconstructOfferIndex(pindexRescan);
}


UniValue getofferfees(const UniValue& params, bool fHelp) {
	if (fHelp || 0 != params.size())
		throw runtime_error(
				"getaliasfees\n"
						"get current service fees for alias transactions\n");
	UniValue oRes(UniValue::VARR);
	oRes.push_back(Pair("height", chainActive.Tip()->nHeight ));
	oRes.push_back(Pair("activate_fee", ValueFromAmount(GetOfferNetworkFee(OP_OFFER_ACTIVATE, chainActive.Tip()->nHeight) )));
	oRes.push_back(Pair("update_fee", ValueFromAmount(GetOfferNetworkFee(OP_OFFER_UPDATE, chainActive.Tip()->nHeight) )));
	return oRes;

}

UniValue offernew(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 7 || params.size() > 10)
		throw runtime_error(
		"offernew <alias> <category> <title> <quantity> <price> <description> <currency> [private=0] [cert. guid] [exclusive resell=1]\n"
						"<alias> An alias you own\n"
						"<category> category, 255 chars max.\n"
						"<title> title, 255 chars max.\n"
						"<quantity> quantity, > 0\n"
						"<price> price in <currency>, > 0\n"
						"<description> description, 64 KB max.\n"
						"<currency> The currency code that you want your offer to be in ie: USD.\n"
						"<private> Is this a private offer. Default 0 for false.\n"
						"<cert. guid> Set this to the guid of a certificate you wish to sell\n"
						"<exclusive resell> set to 1 if you only want those who control the whitelist certificates to be able to resell this offer via offerlink. Defaults to 1.\n"
						+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");
	// gather inputs
	string baSig;
	float nPrice;
	bool bPrivate = false;
	bool bExclusiveResell = true;
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CSyscoinAddress aliasAddress = CSyscoinAddress(stringFromVch(vchAlias));
	if (!aliasAddress.IsValid())
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
				"Invalid syscoin address");
	if (!aliasAddress.isAlias)
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
				"Arbiter must be a valid alias");

	// check for alias existence in DB
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchFromString(aliasAddress.aliasName), vtxPos))
		throw JSONRPCError(RPC_WALLET_ERROR,
				"failed to read alias from alias DB");
	if (vtxPos.size() < 1)
		throw JSONRPCError(RPC_WALLET_ERROR, "no result returned");
	CAliasIndex alias = vtxPos.back();
	vector<unsigned char> vchCat = vchFromValue(params[1]);
	vector<unsigned char> vchTitle = vchFromValue(params[2]);
	vector<unsigned char> vchCurrency = vchFromValue(params[6]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	unsigned int nQty;
	if(atof(params[3].get_str().c_str()) < 0)
		throw runtime_error("invalid quantity value, must be greator than 0");
	try {
		nQty = boost::lexical_cast<unsigned int>(params[3].get_str());
	} catch (std::exception &e) {
		throw runtime_error("invalid quantity value, must be less than 4294967296");
	}
	nPrice = atof(params[4].get_str().c_str());
	if(nPrice <= 0)
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "offer price must be greater than 0!");
	}
	vchDesc = vchFromValue(params[5]);
	if(vchCat.size() < 1)
        throw runtime_error("offer category cannot be empty!");
	if(vchTitle.size() < 1)
        throw runtime_error("offer title cannot be empty!");
	if(vchCat.size() > 255)
        throw runtime_error("offer category cannot exceed 255 bytes!");
	if(vchTitle.size() > 255)
        throw runtime_error("offer title cannot exceed 255 bytes!");
    // 64Kbyte offer desc. maxlen
	if (vchDesc.size() > 1024 * 64)
		throw JSONRPCError(RPC_INVALID_PARAMETER, "offer description cannot exceed 65536 bytes!");
	const CWalletTx *wtxCertIn;
	CCert theCert;
	if(params.size() >= 8)
	{
		bPrivate = atoi(params[7].get_str().c_str()) == 1? true: false;
	}
	if(params.size() >= 9)
	{
		
		vchCert = vchFromValue(params[8]);
		CTransaction txCert;
		
		
		// make sure this cert is still valid
		if (GetTxOfCert(*pcertdb, vchCert, theCert, txCert))
		{
			wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
			// make sure its in your wallet (you control this cert)		
			if (IsCertMine(txCert) && wtxCertIn != NULL) 
				nQty = 1;
			else
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot sell this certificate, it is not yours!");
			// check the offer links in the cert, can't sell a cert thats already linked to another offer
			if(!theCert.vchOfferLink.empty())
			{
				COffer myOffer;
				CTransaction txMyOffer;
				// if offer is still valid then we cannot xfer this cert
				if (GetTxOfOffer(*pofferdb, theCert.vchOfferLink, myOffer, txMyOffer))
				{
					string strError = strprintf("Cannot sell this certificate, it is already linked to offer: %s", stringFromVch(theCert.vchOfferLink).c_str());
					throw JSONRPCError(RPC_INVALID_PARAMETER, strError);
				}
		    }
		}
	}
	if(params.size() >= 10)
	{
		bExclusiveResell = atoi(params[9].get_str().c_str()) == 1? true: false;
	}
	
	int64_t nRate;
	vector<string> rateList;
	int precision;
	if(getCurrencyToSYSFromAlias(vchCurrency, nRate, chainActive.Tip()->nHeight, rateList,precision) != "")
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Could not find this currency code in the SYS_RATES alias!\n");
	}
	float minPrice = 1/pow(10,precision);
	float price = nPrice;
	if(price < minPrice)
		price = minPrice;
	string priceStr = strprintf("%.*f", precision, price);
	nPrice = (float)atof(priceStr.c_str());
	// this is a syscoin transaction
	CWalletTx wtx;



	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));

	EnsureWalletIsUnlocked();


  	CPubKey newDefaultKey;
	CScript scriptPubKeyOrig;
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_ACTIVATE, chainActive.Tip()->nHeight);	
	// unserialize offer UniValue from txn, serialize back
	// build offer UniValue
	COffer newOffer;
	newOffer.vchPubKey = alias.vchPubKey;
	newOffer.aliasName = aliasAddress.aliasName;
	newOffer.vchRand = vchOffer;
	newOffer.sCategory = vchCat;
	newOffer.sTitle = vchTitle;
	newOffer.sDescription = vchDesc;
	newOffer.nQty = nQty;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.SetPrice(nPrice);
	newOffer.vchCert = vchCert;
	newOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;
	newOffer.sCurrencyCode = vchCurrency;
	newOffer.bPrivate = bPrivate;
	string bdata = newOffer.SerializeToString();
	
	scriptPubKeyOrig= GetScriptForDestination(aliasAddress.Get());
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer
			<< vchRand << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);

	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata);
	if(fDebug)
		printf("SENT:OFFERACTIVATE: title=%s, guid=%s, tx=%s\n",
			stringFromVch(newOffer.sTitle).c_str(),
			stringFromVch(vchOffer).c_str(), wtx.GetHash().GetHex().c_str());

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	if(!theCert.IsNull())
	{
		// get a key from our wallet set dest as ourselves
		CPubKey newDefaultKey;
		pwalletMain->GetKeyFromPool(newDefaultKey);
		scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

		// create CERTUPDATE txn keys
		CScript scriptPubKey;
		scriptPubKey << CScript::EncodeOP_N(OP_CERT_UPDATE) << theCert.vchRand << theCert.vchTitle
				<< OP_2DROP << OP_DROP;
		scriptPubKey += scriptPubKeyOrig;

		int64_t nNetFee = GetCertNetworkFee(OP_CERT_UPDATE, chainActive.Tip()->nHeight);
		theCert.nHeight = chainActive.Tip()->nHeight;
		theCert.vchOfferLink = vchOffer;
		vector<CRecipient> vecSend;
		CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
		vecSend.push_back(recipient);

		CScript scriptFee;
		scriptFee << OP_RETURN;
		CRecipient fee = {scriptFee, nNetFee, false};
		vecSend.push_back(fee);
		string bdata = theCert.SerializeToString();
		SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxCertIn);

	}
	return res;
}

UniValue offerlink(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 3)
		throw runtime_error(
		"offerlink <guid> <commission> [description]\n"
						"<alias> An alias you own\n"
						"<guid> offer guid that you are linking to\n"
						"<commission> percentage of profit desired over original offer price, > 0, ie: 5 for 5%\n"
						"<description> description, 64 KB max. Defaults to original description. Leave as '' to use default.\n"
						+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");
	// gather inputs
	string baSig;
	unsigned int nQty;
	CWalletTx *wtxCertIn;
	COfferLinkWhitelistEntry whiteListEntry;

	vector<unsigned char> vchLinkOffer = vchFromValue(params[0]);
	vector<unsigned char> vchDesc;
	// look for a transaction with this key
	CTransaction tx;
	COffer linkOffer;
	if (!GetTxOfOffer(*pofferdb, vchLinkOffer, linkOffer, tx) || vchLinkOffer.empty())
		throw runtime_error("could not find an offer with this name");

	if(!linkOffer.vchLinkOffer.empty())
	{
		throw runtime_error("cannot link to an offer that is already linked to another offer");
	}

	int commissionInteger = atoi(params[1].get_str().c_str());
	if(commissionInteger < 0 || commissionInteger > 255)
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "commission must positive and less than 256!");
	}
	
	if(params.size() >= 3)
	{

		vchDesc = vchFromValue(params[2]);
		if(vchDesc.size() > 0)
		{
			// 64Kbyte offer desc. maxlen
			if (vchDesc.size() > 1024 * 64)
				throw JSONRPCError(RPC_INVALID_PARAMETER, "offer description cannot exceed 65536 bytes!");
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

	// go through the whitelist and see if you own any of the certs to apply to this offer for a discount
	for(unsigned int i=0;i<linkOffer.linkWhitelist.entries.size();i++) {
		CTransaction txCert;
		const CWalletTx *wtxCertIn;
		CCert theCert;
		COfferLinkWhitelistEntry& entry = linkOffer.linkWhitelist.entries[i];
		// make sure this cert is still valid
		if (GetTxOfCert(*pcertdb, entry.certLinkVchRand, theCert, txCert))
		{
			wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
			// make sure its in your wallet (you control this cert)		
			if (IsCertMine(txCert) && wtxCertIn != NULL) 
			{
				foundEntry = entry;
				break;	
			}
			
		}

	}


	// if the whitelist exclusive mode is on and you dont have a cert in the whitelist, you cannot link to this offer
	if(foundEntry.IsNull() && linkOffer.linkWhitelist.bExclusiveResell)
	{
		throw runtime_error("cannot link to this offer because you don't own a cert from its whitelist (the offer is in exclusive mode)");
	}


	// this is a syscoin transaction
	CWalletTx wtx;


	// generate rand identifier
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchOffer = vchFromString(HexStr(vchRand));
	int precision = 2;
	// get precision
	convertCurrencyCodeToSyscoin(linkOffer.sCurrencyCode, linkOffer.GetPrice(), chainActive.Tip()->nHeight, precision);
	float minPrice = 1/pow(10,precision);
	float price = linkOffer.GetPrice();
	if(price < minPrice)
		price = minPrice;
	string priceStr = strprintf("%.*f", precision, price);
	price = (float)atof(priceStr.c_str());

	EnsureWalletIsUnlocked();
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_ACTIVATE, chainActive.Tip()->nHeight);	
	// unserialize offer UniValue from txn, serialize back
	// build offer UniValue
	COffer newOffer;
	newOffer.vchPubKey = linkOffer.vchPubKey;
	newOffer.aliasName = linkOffer.aliasName;
	newOffer.vchRand = vchOffer;
	newOffer.sCategory = linkOffer.sCategory;
	newOffer.sTitle = linkOffer.sTitle;
	newOffer.sDescription = vchDesc;
	newOffer.nQty = linkOffer.nQty;
	newOffer.linkWhitelist.bExclusiveResell = true;
	newOffer.SetPrice(price);
	
	newOffer.nCommission = commissionInteger;
	
	newOffer.vchLinkOffer = vchLinkOffer;
	newOffer.nHeight = chainActive.Tip()->nHeight;
	newOffer.sCurrencyCode = linkOffer.sCurrencyCode;
	string bdata = newOffer.SerializeToString();
	
	//create offeractivate txn keys
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	CScript scriptPubKeyOrig;
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACTIVATE) << vchOffer
			<< vchRand << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;


	string strError;

	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);

	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);

	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata);
	if(fDebug)
		printf("SENT:OFFERACTIVATE: title=%s, guid=%s, tx=%s\n",
			stringFromVch(newOffer.sTitle).c_str(),
			stringFromVch(vchOffer).c_str(), wtx.GetHash().GetHex().c_str());

	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}
UniValue offeraddwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 2 || params.size() > 3)
		throw runtime_error(
		"offeraddwhitelist <offer guid> <cert guid> [discount percentage]\n"
		"Add to the whitelist of your offer(controls who can resell).\n"
						"<offer guid> offer guid that you are adding to\n"
						"<cert guid> cert guid representing a certificate that you control (transfer it to reseller after)\n"
						"<discount percentage> percentage of discount given to reseller for this offer. Negative discount adds on top of offer price, acts as an extra commission. -99 to 99.\n"						
						+ HelpRequiringPassphrase());

	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchCert =  vchFromValue(params[1]);
	int nDiscountPctInteger = 0;
	
	if(params.size() >= 3)
		nDiscountPctInteger = atoi(params[2].get_str().c_str());

	if(nDiscountPctInteger < -99 || nDiscountPctInteger > 99)
		throw runtime_error("Invalid discount amount");
	CTransaction txCert;
	CCert theCert;
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	const CWalletTx* wtxCertIn;
	if (!GetTxOfCert(*pcertdb, vchCert, theCert, txCert))
		throw runtime_error("could not find a certificate with this key");

	// check to see if certificate in wallet
	wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
	if (wtxCertIn == NULL)
		throw runtime_error("this certificate is not in your wallet");


	// this is a syscoind txn
	CScript scriptPubKeyOrig;

	// get a key from our wallet set dest as ourselves
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());


	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_REFUND) || ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}
	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");
	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// unserialize offer UniValue from txn
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
		// make sure this cert doesn't already exist
		if (entry.certLinkVchRand == vchCert)
		{
			throw runtime_error("This cert is already added to your whitelist");
		}

	}
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << theOffer.sTitle
			<< OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_UPDATE, chainActive.Tip()->nHeight);
	

	theOffer.accepts.clear();

	COfferLinkWhitelistEntry entry;
	entry.certLinkVchRand = theCert.vchRand;
	entry.nDiscountPct = nDiscountPctInteger;
	theOffer.linkWhitelist.PutWhitelistEntry(entry);
	// serialize offer UniValue
	string bdata = theOffer.SerializeToString();

	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);
	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxIn);

	return wtx.GetHash().GetHex();
}
UniValue offerremovewhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 2)
		throw runtime_error(
		"offerremovewhitelist <offer guid> <cert guid>\n"
		"Remove from the whitelist of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchCert = vchFromValue(params[1]);


	CCert theCert;

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig;

	// get a key from our wallet set dest as ourselves
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

	
	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_REFUND) || ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}
	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << theOffer.sTitle
			<< OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_UPDATE, chainActive.Tip()->nHeight);
	theOffer.accepts.clear();


	if(!theOffer.linkWhitelist.RemoveWhitelistEntry(vchCert))
	{
		throw runtime_error("could not find remove this whitelist entry");
	}

	// serialize offer UniValue
	string bdata = theOffer.SerializeToString();
	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);
	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxIn);

	return wtx.GetHash().GetHex();
}
UniValue offerclearwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 1)
		throw runtime_error(
		"offerclearwhitelist <offer guid>\n"
		"Clear the whitelist of your offer(controls who can resell).\n"
						+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);


	CCert theCert;

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig;

	// get a key from our wallet set dest as ourselves
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());


	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_REFUND) || ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}
	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
	theOffer.nHeight = chainActive.Tip()->nHeight;
	// create OFFERUPDATE txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << theOffer.sTitle
			<< OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_UPDATE, chainActive.Tip()->nHeight);
	
	theOffer.accepts.clear();
	theOffer.linkWhitelist.entries.clear();

	// serialize offer UniValue
	string bdata = theOffer.SerializeToString();

	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);
	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxIn);

	return wtx.GetHash().GetHex();
}

UniValue offerwhitelist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("offerwhitelist <offer guid>\n"
                "List all whitelist entries for this offer.\n");
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
    UniValue oRes(UniValue::VARR);
    vector<unsigned char> vchOffer = vchFromValue(params[0]);
	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");
	
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CTransaction txCert;
		CCert theCert;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		uint256 hashBlock;
		if (GetTxOfCert(*pcertdb, entry.certLinkVchRand, theCert, txCert))
		{
			UniValue oList(UniValue::VOBJ);
			oList.push_back(Pair("cert_guid", stringFromVch(theCert.vchRand)));
			oList.push_back(Pair("cert_title", stringFromVch(theCert.vchTitle)));
			oList.push_back(Pair("cert_is_mine", IsCertMine(txCert) ? "true" : "false"));
			string strAddress = "";
			GetCertAddress(txCert, strAddress);
			oList.push_back(Pair("cert_address", strAddress));
			int expires_in = 0;
			int64_t nHeight = GetTxHashHeight(txCert.GetHash());
            if(nHeight + GetCertExpirationDepth() - chainActive.Tip()->nHeight > 0)
			{
				expires_in = nHeight + GetCertExpirationDepth() - chainActive.Tip()->nHeight;
			}  
			oList.push_back(Pair("cert_expiresin",expires_in));
			oList.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));
			oRes.push_back(oList);
		}  
    }
    return oRes;
}
UniValue offerupdate(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 5 || params.size() > 9)
		throw runtime_error(
		"offerupdate <guid> <category> <title> <quantity> <price> [description] [private=0] [cert. guid] [exclusive resell=1]\n"
						"Perform an update on an offer you control.\n"
						+ HelpRequiringPassphrase());

	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
	// gather & validate inputs
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchCat = vchFromValue(params[1]);
	vector<unsigned char> vchTitle = vchFromValue(params[2]);
	vector<unsigned char> vchDesc;
	vector<unsigned char> vchCert;
	vector<unsigned char> vchOldCert;
	bool bExclusiveResell = true;
	int bPrivate = false;
	unsigned int nQty;
	float price;
	if (params.size() >= 6) vchDesc = vchFromValue(params[5]);
	if (params.size() >= 7) bPrivate = atoi(params[6].get_str().c_str()) == 1? true: false;
	if (params.size() >= 8) vchCert = vchFromValue(params[7]);
	if(params.size() >= 9) bExclusiveResell = atoi(params[8].get_str().c_str()) == 1? true: false;
	if(atof(params[3].get_str().c_str()) < 0)
		throw runtime_error("invalid quantity value, must be greator than 0");
	
	try {
		nQty = boost::lexical_cast<unsigned int>(params[3].get_str());
		price = atof(params[4].get_str().c_str());

	} catch (std::exception &e) {
		throw runtime_error("invalid price and/or quantity values. Quantity must be less than 4294967296.");
	}
	if (params.size() >= 6) vchDesc = vchFromValue(params[5]);
	if(price <= 0)
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "offer price must be greater than 0!");
	}
	
	if(vchCat.size() < 1)
        throw runtime_error("offer category cannot by empty!");
	if(vchTitle.size() < 1)
        throw runtime_error("offer title cannot be empty!");
	if(vchCat.size() > 255)
        throw runtime_error("offer category cannot exceed 255 bytes!");
	if(vchTitle.size() > 255)
        throw runtime_error("offer title cannot exceed 255 bytes!");
    // 64Kbyte offer desc. maxlen
	if (vchDesc.size() > 1024 * 64)
		throw JSONRPCError(RPC_INVALID_PARAMETER, "offer description cannot exceed 65536 bytes!");

	// this is a syscoind txn
	CWalletTx wtx;
	const CWalletTx* wtxIn;
	CScript scriptPubKeyOrig;

	// get a key from our wallet set dest as ourselves
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

	// create OFFERUPDATE txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_UPDATE) << vchOffer << vchTitle
			<< OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	// check for existing pending offers
	if (ExistsInMempool(vchOffer, OP_OFFER_REFUND) || ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}
	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	CTransaction tx, linktx;
	COffer theOffer, linkOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
		throw runtime_error("could not find an offer with this name");

	wtxIn = pwalletMain->GetWalletTx(tx.GetHash());
	if (wtxIn == NULL)
		throw runtime_error("this offer is not in your wallet");
	// unserialize offer UniValue from txn
	if(!theOffer.UnserializeFromTx(tx))
		throw runtime_error("cannot unserialize offer from txn");

	// get the offer from DB
	vector<COffer> vtxPos;
	if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
		throw runtime_error("could not read offer from DB");

	theOffer = vtxPos.back();
		
	// calculate network fees
	int64_t nNetFee = GetOfferNetworkFee(OP_OFFER_UPDATE, chainActive.Tip()->nHeight);
	int precision = 2;
	// get precision
	convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, theOffer.GetPrice(), chainActive.Tip()->nHeight, precision);
	float minPrice = 1/pow(10,precision);
	if(price < minPrice)
		price = minPrice;
	string priceStr = strprintf("%.*f", precision, price);
	price = (float)atof(priceStr.c_str());
	// update offer values
	theOffer.sCategory = vchCat;
	theOffer.sTitle = vchTitle;
	theOffer.sDescription = vchDesc;
	if (params.size() >= 7)
		theOffer.bPrivate = bPrivate;
	unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	if((nQty-memPoolQty) < 0)
		throw runtime_error("not enough remaining quantity to fulfill this offerupdate"); // SS i think needs better msg
	vchOldCert = theOffer.vchCert;
	if(vchCert.empty())
	{
		theOffer.nQty = nQty;
	}	
	else
	{
		theOffer.nQty = 1;
		theOffer.vchCert = vchCert;
	}
	// update cert if offer has a cert
	if(!theOffer.vchCert.empty())
	{
		CTransaction txCert;
		CWalletTx wtx;
		const CWalletTx *wtxCertIn;
		CCert theCert;
		// make sure this cert is still valid
		if (GetTxOfCert(*pcertdb, theOffer.vchCert, theCert, txCert))
		{
			// make sure its in your wallet (you control this cert)	
			wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());
			if (!IsCertMine(txCert) || wtxCertIn == NULL) 
			{
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot update this offer because this certificate is not yours!");
			}			
		}
		else
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot find this certificate!");
		
		if(!theCert.vchOfferLink.empty() && theCert.vchOfferLink != vchOffer)
		{
			COffer myOffer;
			CTransaction txMyOffer;
			// if offer is still valid then we cannot xfer this cert
			if (GetTxOfOffer(*pofferdb, theCert.vchOfferLink, myOffer, txMyOffer))
			{
				string strError = strprintf("Cannot update this offer because this certificate is linked to another offer: %s", stringFromVch(theCert.vchOfferLink).c_str());
				throw JSONRPCError(RPC_INVALID_PARAMETER, strError);
			}
		}

		// get a key from our wallet set dest as ourselves
		CPubKey newDefaultKey;
		pwalletMain->GetKeyFromPool(newDefaultKey);
		scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

		// create CERTUPDATE txn keys
		CScript scriptPubKey;
		scriptPubKey << CScript::EncodeOP_N(OP_CERT_UPDATE) << theCert.vchRand << theCert.vchTitle
				<< OP_2DROP << OP_DROP;
		scriptPubKey += scriptPubKeyOrig;


		int64_t nNetFee = GetCertNetworkFee(OP_CERT_UPDATE, chainActive.Tip()->nHeight);
		theCert.nHeight = chainActive.Tip()->nHeight;
		if(vchCert.empty())
		{
			theOffer.vchCert.clear();
			theCert.vchOfferLink.clear();
		}
		else
			theCert.vchOfferLink = vchOffer;

		vector<CRecipient> vecSend;
		CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
		vecSend.push_back(recipient);

		CScript scriptFee;
		scriptFee << OP_RETURN;
		CRecipient fee = {scriptFee, nNetFee, false};
		vecSend.push_back(fee);
		string bdata = theCert.SerializeToString();
		SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxCertIn);

		// updating from one cert to another, need to update both certs in this case
		if(!vchOldCert.empty() && !theOffer.vchCert.empty() && theOffer.vchCert != vchOldCert)
		{
			// this offer changed certs so remove offer link from old cert
			CTransaction txOldCert;
			CWalletTx wtxold;
			const CWalletTx* wtxCertOldIn;
			CCert theOldCert;
			// make sure this cert is still valid
			if (GetTxOfCert(*pcertdb, vchOldCert, theOldCert, txOldCert))
			{
				// make sure its in your wallet (you control this cert)		
				wtxCertOldIn = pwalletMain->GetWalletTx(txOldCert.GetHash());
				if (!IsCertMine(txOldCert) || wtxCertOldIn == NULL)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot update this offer because old certificate is not yours!");
				}			
			}
			else
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot find old certificate!");
			// create CERTUPDTE txn keys
			CScript scriptOldPubKey, scriptPubKeyOld;
			pwalletMain->GetKeyFromPool(newDefaultKey);
			scriptPubKeyOld= GetScriptForDestination(newDefaultKey.GetID());
			scriptOldPubKey << CScript::EncodeOP_N(OP_CERT_UPDATE) << theOldCert.vchRand << theOldCert.vchTitle
					<< OP_2DROP << OP_DROP;
			scriptOldPubKey += scriptPubKeyOld;
			// clear the offer link, this is the only change we are making to this cert
			theOldCert.vchOfferLink.clear();

			vector<CRecipient> vecSend;
			CRecipient recipient = {scriptOldPubKey, MIN_AMOUNT, false};
			vecSend.push_back(recipient);

			CScript scriptFee;
			scriptFee << OP_RETURN;
			CRecipient fee = {scriptFee, nNetFee, false};
			vecSend.push_back(fee);
			string bOldCertData = theOldCert.SerializeToString();
			SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtxold, bOldCertData, wtxCertOldIn);
		}
	}
	theOffer.nHeight = chainActive.Tip()->nHeight;
	theOffer.SetPrice(price);
	theOffer.accepts.clear();
	if(params.size() >= 9)
		theOffer.linkWhitelist.bExclusiveResell = bExclusiveResell;
	// serialize offer UniValue
	string bdata = theOffer.SerializeToString();

	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);
	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);
	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata, wtxIn);

	return wtx.GetHash().GetHex();
}

UniValue offerrefund(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("offerrefund <acceptguid>\n"
				"Refund an offer accept of an offer you control.\n"
				"<guid> guidkey of offer accept to refund.\n"
				+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");
	vector<unsigned char> vchAcceptRand = vchFromString(params[0].get_str());

	EnsureWalletIsUnlocked();

	// look for a transaction with this key
	COffer activateOffer, theOffer;
	CTransaction tmpTx, txOffer, txAccept;
	COfferAccept theOfferAccept;
	uint256 hashBlock;

	if (!GetTxOfOfferAccept(*pofferdb, vchAcceptRand, theOffer, txOffer))
		throw runtime_error("could not find an offer accept with this identifier");

	const CWalletTx *wtxIn;
	wtxIn = pwalletMain->GetWalletTx(txOffer.GetHash());
	if (wtxIn == NULL)
	{
		throw runtime_error("can't find this offer in your wallet");
	}

	if (ExistsInMempool(theOffer.vchRand, OP_OFFER_REFUND) || ExistsInMempool(theOffer.vchRand, OP_OFFER_ACTIVATE) || ExistsInMempool(theOffer.vchRand, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}
	// check for existence of offeraccept in txn offer obj
	if(!theOffer.GetAcceptByHash(vchAcceptRand, theOfferAccept))
		throw runtime_error("could not read accept from offer txn");
	// check if this offer is linked to another offer
	if (!theOffer.vchLinkOffer.empty())
		throw runtime_error("You cannot refund an offer that is linked to another offer, only the owner of the original offer can issue a refund.");
	
	string strError = makeOfferRefundTX(txOffer, theOffer, theOfferAccept, OFFER_REFUND_PAYMENT_INPROGRESS);
	if (strError != "")
	{
		throw runtime_error(strError);
	}
	return "Success";
}

UniValue offeraccept(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size() || params.size() > 8)
		throw runtime_error("offeraccept <guid> [quantity] [pubkey] [message] [refund address] [linkedguid] [escrowTxHash] [height]\n"
				"Accept&Pay for a confirmed offer.\n"
				"<guid> guidkey from offer.\n"
				"<pubkey> Public key of buyer address (to transfer certificate).\n"
				"<quantity> quantity to buy. Defaults to 1.\n"
				"<message> payment message to seller, 1KB max.\n"
				"<refund address> In case offer not accepted refund to this address. Leave empty to use a new address from your wallet. \n"
				"<linkedguid> guidkey from offer accept linking to this offer accept. For internal use only, leave blank\n"
				"<escrowTxHash> If this offer accept is done by an escrow release. For internal use only, leave blank\n"
				"<height> Height to index into price calculation function. For internal use only, leave blank\n"
				+ HelpRequiringPassphrase());
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");	
	vector<unsigned char> vchRefundAddress;	
	CSyscoinAddress refundAddr;	
	vector<unsigned char> vchOffer = vchFromValue(params[0]);
	vector<unsigned char> vchPubKey = vchFromValue(params.size()>=3?params[2]:"");
	vector<unsigned char> vchLinkOfferAccept = vchFromValue(params.size()>= 6? params[5]:"");
	vector<unsigned char> vchMessage = vchFromValue(params.size()>=4?params[3]:"");
	vector<unsigned char> vchEscrowTxHash = vchFromValue(params.size()>=7?params[6]:"");
	int64_t nHeight = params.size()>=8?atoi64(params[7].get_str()):0;
	unsigned int nQty = 1;
	if (params.size() >= 2) {
		if(atof(params[1].get_str().c_str()) < 0)
			throw runtime_error("invalid quantity value, must be greator than 0");
	
		try {
			nQty = boost::lexical_cast<unsigned int>(params[1].get_str());
		} catch (std::exception &e) {
			throw runtime_error("invalid quantity value. Quantity must be less than 4294967296.");
		}
	}
	if(params.size() < 5)
	{
		CPubKey newDefaultKey;
		pwalletMain->GetKeyFromPool(newDefaultKey);
		refundAddr = CSyscoinAddress(newDefaultKey.GetID());
		vchRefundAddress = vchFromString(refundAddr.ToString());
	}
	else
	{
		vchRefundAddress = vchFromValue(params[4]);
		refundAddr = CSyscoinAddress(stringFromVch(vchRefundAddress));
	}
    if (vchMessage.size() <= 0 && vchPubKey.empty())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "offeraccept message data cannot be empty!");


	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig;

	// generate offer accept identifier and hash
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchAcceptRand = CScriptNum(rand).getvch();
	vector<unsigned char> vchAccept = vchFromString(HexStr(vchAcceptRand));

	// get a key from our wallet set dest as ourselves
	CPubKey newDefaultKey;
	pwalletMain->GetKeyFromPool(newDefaultKey);
	scriptPubKeyOrig= GetScriptForDestination(newDefaultKey.GetID());

	// create OFFERACCEPT txn keys
	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_OFFER_ACCEPT)
			<< vchOffer << vchAccept
			<< OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

	if (ExistsInMempool(vchOffer, OP_OFFER_REFUND) || ExistsInMempool(vchOffer, OP_OFFER_ACTIVATE) || ExistsInMempool(vchOffer, OP_OFFER_UPDATE)) {
		throw runtime_error("there are pending operations or refunds on that offer");
	}

	EnsureWalletIsUnlocked();
	const CWalletTx *wtxEscrowIn;
	uint256 escrowTxHash(vchEscrowTxHash);
	// make sure cert is in wallet
	wtxEscrowIn = pwalletMain->GetWalletTx(escrowTxHash);
	if (!vchEscrowTxHash.empty() && wtxEscrowIn == NULL) 
		throw runtime_error("release escrow transaction is not in your wallet");

	CEscrow escrow(*wtxEscrowIn);
	if(!vchEscrowTxHash.empty() && escrow.IsNull())
		throw runtime_error("release escrow transaction cannot unserialize escrow UniValue");
	// if we want to accept an escrow release or we are accepting a linked offer from an escrow release. Override heightToCheckAgainst if using escrow since escrow can take a long time.
	if(!escrow.IsNull())
	{
		// get escrow activation UniValue
		vector<CEscrow> escrowVtxPos;
		if (pescrowdb->ExistsEscrow(escrow.vchRand)) {
			if (pescrowdb->ReadEscrow(escrow.vchRand, escrowVtxPos))
			{	
				// we want the initial funding escrow transaction height as when to calculate this offer accept price from convertCurrencyCodeToSyscoin()
				CEscrow fundingEscrow = escrowVtxPos.front();
				nHeight = fundingEscrow.nHeight;
			}
		}

	}
	// look for a transaction with this key
	CTransaction tx;
	COffer theOffer;
	if (!GetTxOfOffer(*pofferdb, vchOffer, theOffer, tx))
	{
		throw runtime_error("could not find an offer with this identifier");
	}

	COffer linkedOffer;
	CTransaction tmpTx;
	// check if parent to linked offer is still valid
	if (!theOffer.vchLinkOffer.empty())
	{
		if(pofferdb->ExistsOffer(theOffer.vchLinkOffer))
		{
			if (!GetTxOfOffer(*pofferdb, theOffer.vchLinkOffer, linkedOffer, tmpTx))
				throw runtime_error("Trying to accept a linked offer but could not find parent offer, perhaps it is expired");
		}
	}
	COfferLinkWhitelistEntry foundCert;
	const CWalletTx *wtxCertIn;
	// go through the whitelist and see if you own any of the certs to apply to this offer for a discount
	for(unsigned int i=0;i<theOffer.linkWhitelist.entries.size();i++) {
		CTransaction txCert;
		
		CCert theCert;
		COfferLinkWhitelistEntry& entry = theOffer.linkWhitelist.entries[i];
		// make sure this cert is still valid
		if (GetTxOfCert(*pcertdb, entry.certLinkVchRand, theCert, txCert))
		{
			// make sure its in your wallet (you control this cert)
			wtxCertIn = pwalletMain->GetWalletTx(txCert.GetHash());		
			if (IsCertMine(txCert) && wtxCertIn != NULL) 
			{
				foundCert = entry;
				break;
			}
			
		}

	}
	// if this is an accept for a linked offer, the offer is set to exclusive mode and you dont have a cert in the whitelist, you cannot accept this offer
	if(!vchLinkOfferAccept.empty() && foundCert.IsNull() && theOffer.linkWhitelist.bExclusiveResell)
	{
		throw runtime_error("cannot pay for this linked offer because you don't own a cert from its whitelist");
	}

	unsigned int memPoolQty = QtyOfPendingAcceptsInMempool(vchOffer);
	if(theOffer.nQty < (nQty+memPoolQty))
		throw runtime_error("not enough remaining quantity to fulfill this orderaccept");
	int precision = 2;
	int64_t nPrice = convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, theOffer.GetPrice(foundCert), nHeight>0?nHeight:chainActive.Tip()->nHeight, precision);
	string strCipherText = "";
	// encryption should only happen once even if the offer accept is from a reseller.
	// if this is an accept to the root of the offer owner, not reseller accept
	if(vchLinkOfferAccept.empty())
	{
		// encrypt to offer owner
		if(!EncryptMessage(theOffer.vchPubKey, vchMessage, strCipherText))
			throw runtime_error("could not encrypt message to seller");
	}


	if (vchMessage.size() > MAX_VALUE_LENGTH)
		throw JSONRPCError(RPC_INVALID_PARAMETER, "offeraccept message length cannot exceed 1023 bytes!");
	if(!theOffer.vchCert.empty())
	{
		CTransaction txCert;
		CCert theCert;
		// make sure this cert is still valid
		if (!GetTxOfCert(*pcertdb, theOffer.vchCert, theCert, txCert))
		{
			// make sure its in your wallet (you control this cert)		
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot purchase this certificate, it may be expired!");
			
		}
		nQty = 1;
	}
	
	// create accept UniValue
	COfferAccept txAccept;
	txAccept.vchRand = vchAccept;
	if(strCipherText.size() > 0)
		txAccept.vchMessage = vchFromString(strCipherText);
	else
		txAccept.vchMessage = vchMessage;
	txAccept.nQty = nQty;
	txAccept.nPrice = theOffer.GetPrice(foundCert);
	txAccept.vchLinkOfferAccept = vchLinkOfferAccept;
	txAccept.vchRefundAddress = vchRefundAddress;
	// if we have a linked offer accept then use height from linked accept (the one buyer makes, not the reseller). We need to do this to make sure we convert price at the time of initial buyer's accept.
	// in checkescrowinput we override this if its from an escrow release, just like above.
	txAccept.nHeight = nHeight>0?nHeight:chainActive.Tip()->nHeight;


	txAccept.bPaid = true;
	txAccept.vchCertLink = foundCert.certLinkVchRand;
	txAccept.vchBuyerKey = vchPubKey;
	txAccept.vchEscrowLink = escrow.vchRand;
	theOffer.accepts.clear();
	theOffer.PutOfferAccept(txAccept);


    int64_t nTotalValue = ( nPrice * nQty );
    

    CScript scriptPayment;
	string strAddress = "";
    GetOfferAddress(tx, strAddress);
	CSyscoinAddress address(strAddress);
	if(!address.IsValid())
	{
		throw runtime_error("payment to invalid address");
	}
    scriptPayment= GetScriptForDestination(address.Get());

	vector<CRecipient> vecSend;
	CRecipient paymentRecipient = {scriptPayment, nTotalValue, false};
	vecSend.push_back(paymentRecipient);

	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);

	// serialize offer UniValue
	string bdata = theOffer.SerializeToString();
	if(!foundCert.IsNull())
	{
		if(escrow.IsNull())
		{
			SendMoneySyscoin(vecSend, MIN_AMOUNT+nTotalValue, false, wtx, bdata, wtxCertIn);
		}
	
		// create a certupdate passing in wtx (offeraccept) as input to keep chain of inputs going for next cert transaction (since we used the last cert tx as input to sendmoneysysoin)
		CWalletTx wtxCert;
		CScript scriptPubKeyOrig;
		CCert cert;
		CTransaction tx;
		if (!GetTxOfCert(*pcertdb, foundCert.certLinkVchRand, cert, tx))
			throw runtime_error("could not find a certificate with this key");


		// get a key from our wallet set dest as ourselves
		CPubKey newDefaultKey;
		pwalletMain->GetKeyFromPool(newDefaultKey);
		scriptPubKeyOrig = GetScriptForDestination(newDefaultKey.GetID());

		// create CERTUPDATE txn keys
		CScript scriptPubKey;
		scriptPubKey << CScript::EncodeOP_N(OP_CERT_UPDATE) << cert.vchRand << cert.vchTitle
				<< OP_2DROP << OP_DROP;
		scriptPubKey += scriptPubKeyOrig;
		int64_t nNetFee = GetCertNetworkFee(OP_CERT_UPDATE, chainActive.Tip()->nHeight);
		cert.nHeight = chainActive.Tip()->nHeight;

		vector<CRecipient> vecSend;
		CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
		vecSend.push_back(recipient);

		CScript scriptFee;
		scriptFee << OP_RETURN;
		CRecipient fee = {scriptFee, nNetFee, false};
		vecSend.push_back(fee);
		const CWalletTx* wtxIn = &wtx;
		string bdata = cert.SerializeToString();
		SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtxCert, bdata, wtxIn);
	}
	else
	{
		SendMoneySyscoin(vecSend, MIN_AMOUNT+nTotalValue, false, wtx, bdata);
	}
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
	{
		vector<COffer> vtxPos;
		if (!pofferdb->ReadOffer(vchOffer, vtxPos))
			throw JSONRPCError(RPC_WALLET_ERROR,
					"failed to read from offer DB");
		if (vtxPos.size() < 1)
			throw JSONRPCError(RPC_WALLET_ERROR, "no result returned");

        // get transaction pointed to by offer
        CTransaction tx;
        uint256 blockHash;
        uint256 txHash = vtxPos.back().txHash;
        if (!GetTransaction(txHash, tx, Params().GetConsensus(), blockHash, true))
            throw JSONRPCError(RPC_WALLET_ERROR, "failed to read transaction from disk");

        COffer theOffer = vtxPos.back();

		UniValue oOffer(UniValue::VOBJ);
		vector<unsigned char> vchValue;
		UniValue aoOfferAccepts(UniValue::VARR);
		for(unsigned int i=0;i<theOffer.accepts.size();i++) {
			COfferAccept ca = theOffer.accepts[i];
			UniValue oOfferAccept(UniValue::VOBJ);

	        // get transaction pointed to by offer

	        CTransaction txA;
	        uint256 blockHashA;
	        uint256 txHashA= ca.txHash;
	        if (!GetTransaction(txHashA, txA, Params().GetConsensus(), blockHashA, true))
	            throw JSONRPCError(RPC_WALLET_ERROR, "failed to read transaction from disk");
			string sTime;
			CBlockIndex *pindex = chainActive[ca.nHeight];
			if (pindex) {
				sTime = strprintf("%llu", pindex->nTime);
			}
            string sHeight = strprintf("%llu", ca.nHeight);
			oOfferAccept.push_back(Pair("id", stringFromVch(ca.vchRand)));
			oOfferAccept.push_back(Pair("txid", ca.txHash.GetHex()));
			oOfferAccept.push_back(Pair("height", sHeight));
			oOfferAccept.push_back(Pair("time", sTime));
			oOfferAccept.push_back(Pair("quantity", strprintf("%u", ca.nQty)));
			oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, ca.nPrice ))); 
			oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, ca.nPrice * ca.nQty )));
			COfferLinkWhitelistEntry entry;
			if(IsOfferMine(tx)) 
				theOffer.linkWhitelist.GetLinkEntryByHash(ca.vchCertLink, entry);
			oOfferAccept.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));
			oOfferAccept.push_back(Pair("is_mine", IsOfferMine(txA) ? "true" : "false"));
			if(ca.bPaid) {
				oOfferAccept.push_back(Pair("paid","true"));
				string strMessage = string("");
				if(!DecryptMessage(theOffer.vchPubKey, ca.vchMessage, strMessage))
					strMessage = string("Encrypted for owner of offer");
				oOfferAccept.push_back(Pair("pay_message", strMessage));

			}
			else
			{
				oOfferAccept.push_back(Pair("paid","false"));
				oOfferAccept.push_back(Pair("pay_message",""));
			}
			if(ca.bRefunded) { 
				oOfferAccept.push_back(Pair("refunded", "true"));
				oOfferAccept.push_back(Pair("refund_txid", ca.txRefundId.GetHex()));
			}
			else
			{
				oOfferAccept.push_back(Pair("refunded", "false"));
				oOfferAccept.push_back(Pair("refund_txid", ""));
			}
			
			

			aoOfferAccepts.push_back(oOfferAccept);
		}
		int nHeight;
		uint256 offerHash;
		int expired;
		int expires_in;
		int expired_block;
  
		expired = 0;	
		expires_in = 0;
		expired_block = 0;
        if (GetValueOfOfferTxHash(txHash, vchValue, offerHash, nHeight)) {
			oOffer.push_back(Pair("offer", offer));
			oOffer.push_back(Pair("cert", stringFromVch(theOffer.vchCert)));
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

			string strAddress = "";
            GetOfferAddress(tx, strAddress);
			CSyscoinAddress address(strAddress);
			if(address.IsValid() && address.isAlias)
				oOffer.push_back(Pair("address", address.aliasName));
			else
				oOffer.push_back(Pair("address", address.ToString()));
			oOffer.push_back(Pair("category", stringFromVch(theOffer.sCategory)));
			oOffer.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
			oOffer.push_back(Pair("quantity", strprintf("%u", theOffer.nQty)));
			oOffer.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
			
			
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oOffer.push_back(Pair("price", strprintf("%.*f", precision, theOffer.GetPrice() ))); 
			
			oOffer.push_back(Pair("is_mine", IsOfferMine(tx) ? "true" : "false"));
			if(!theOffer.vchLinkOffer.empty() && IsOfferMine(tx)) {
				oOffer.push_back(Pair("commission", strprintf("%d%%", theOffer.nCommission)));
				oOffer.push_back(Pair("offerlink", "true"));
				oOffer.push_back(Pair("offerlink_guid", stringFromVch(theOffer.vchLinkOffer)));
			}
			else
			{
				oOffer.push_back(Pair("commission", "0"));
				oOffer.push_back(Pair("offerlink", "false"));
			}
			oOffer.push_back(Pair("exclusive_resell", theOffer.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
			oOffer.push_back(Pair("private", theOffer.bPrivate ? "Yes" : "No"));
			oOffer.push_back(Pair("description", stringFromVch(theOffer.sDescription)));
			oOffer.push_back(Pair("alias", theOffer.aliasName));
			oOffer.push_back(Pair("accepts", aoOfferAccepts));
			oLastOffer = oOffer;
		}
	}
	return oLastOffer;

}
UniValue offeracceptlist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size())
		throw runtime_error("offeracceptlist [offer]\n"
				"list my offer accepts");

    vector<unsigned char> vchName;

    if (params.size() == 1)
        vchName = vchFromValue(params[0]);
    map< vector<unsigned char>, int > vNamesI;
    UniValue oRes(UniValue::VARR);
    {

        uint256 blockHash;
        uint256 hash;
        CTransaction tx;
        int nHeight;
		
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
        {
			UniValue oOfferAccept(UniValue::VOBJ);
            // get txn hash, read txn index
            hash = item.second.GetHash();

 			const CWalletTx &wtx = item.second;

            // skip non-syscoin txns
            if (wtx.nVersion != SYSCOIN_TX_VERSION)
                continue;

            // decode txn, skip non-alias txns
            vector<vector<unsigned char> > vvch;
            int op, nOut;
            if (!DecodeOfferTx(wtx, op, nOut, vvch, -1) 
            	|| !IsOfferOp(op) 
            	|| (op != OP_OFFER_ACCEPT))
                continue;

            // get the txn height
            nHeight = GetTxHashHeight(hash);

            // get the txn alias name
            if(!GetNameOfOfferTx(wtx, vchName))
                continue;
			
			COfferAccept theOfferAccept;

			// Check hash
			const vector<unsigned char> &vchAcceptRand = vvch[1];			

			CTransaction offerTx;
			COffer theOffer;
			if(!GetTxOfOffer(*pofferdb, vchName, theOffer, offerTx))	
				continue;
			// check for existence of offeraccept in txn offer obj
			if(!theOffer.GetAcceptByHash(vchAcceptRand, theOfferAccept))
				continue;					

			string offer = stringFromVch(vchName);
			string sHeight = strprintf("%u", theOfferAccept.nHeight);
			oOfferAccept.push_back(Pair("offer", offer));
			oOfferAccept.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
			oOfferAccept.push_back(Pair("id", stringFromVch(theOfferAccept.vchRand)));
			oOfferAccept.push_back(Pair("alias", theOffer.aliasName));
			oOfferAccept.push_back(Pair("buyerkey", stringFromVch(theOfferAccept.vchBuyerKey)));
			oOfferAccept.push_back(Pair("height", sHeight));
			oOfferAccept.push_back(Pair("quantity", strprintf("%u", theOfferAccept.nQty)));
			oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
			oOfferAccept.push_back(Pair("escrowlink", stringFromVch(theOfferAccept.vchEscrowLink)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, theOfferAccept.nPrice ))); 
			oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, theOfferAccept.nPrice * theOfferAccept.nQty ))); 
			// this accept is for me(something ive sold) if this offer is mine
			oOfferAccept.push_back(Pair("is_mine", IsOfferMine(offerTx)? "true" : "false"));
			if(theOfferAccept.bPaid && !theOfferAccept.bRefunded) {
				oOfferAccept.push_back(Pair("status","paid"));
			}
			else if(!theOfferAccept.bRefunded)
			{
				oOfferAccept.push_back(Pair("status","not paid"));
			}
			else if(theOfferAccept.bRefunded) { 
				oOfferAccept.push_back(Pair("status", "refunded"));
			}

			oRes.push_back(oOfferAccept);	
        }
       BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
        {
			UniValue oOfferAccept(UniValue::VOBJ);
            // get txn hash, read txn index
            hash = item.second.GetHash();

            if (!GetTransaction(hash, tx, Params().GetConsensus(), blockHash, true))
                continue;
            if (tx.nVersion != SYSCOIN_TX_VERSION)
                continue;

            // decode txn
            vector<vector<unsigned char> > vvch;
            int op, nOut;
            if (!DecodeEscrowTx(tx, op, nOut, vvch, -1) 
            	|| !IsEscrowOp(op))
                continue;

            // get the txn height
            nHeight = GetTxHashHeight(hash);

            // get the txn alias name
            if(!GetNameOfEscrowTx(tx, vchName))
                continue;
		
			COfferAccept theOfferAccept;		

			CTransaction escrowTx;
			CEscrow theEscrow;
			COffer theOffer;
			CTransaction offerTx;
			if(!GetTxOfEscrow(*pescrowdb, vchName, theEscrow, escrowTx))	
				continue;
			std::vector<unsigned char> vchBuyerKeyByte;
			boost::algorithm::unhex(theEscrow.vchBuyerKey.begin(), theEscrow.vchBuyerKey.end(), std::back_inserter(vchBuyerKeyByte));
			CPubKey buyerKey(vchBuyerKeyByte);
			CSyscoinAddress buyerAddress(buyerKey.GetID());
			if(!buyerAddress.IsValid() || !IsMine(*pwalletMain, buyerAddress.Get()))
				continue;
			if (!GetTxOfOfferAccept(*pofferdb, theEscrow.vchOfferAcceptLink, theOffer, offerTx))
				continue;


			// check for existence of offeraccept in txn offer obj
			if(!theOffer.GetAcceptByHash(theEscrow.vchOfferAcceptLink, theOfferAccept))
				continue;		
			// get last active accept only
			if (vNamesI.find(theOfferAccept.vchRand) != vNamesI.end() && (theOfferAccept.nHeight <= vNamesI[theOfferAccept.vchRand] || vNamesI[theOfferAccept.vchRand] < 0))
				continue;
			vNamesI[theOfferAccept.vchRand] = theOfferAccept.nHeight;
			string offer = stringFromVch(theOffer.vchRand);
			string sHeight = strprintf("%u", theOfferAccept.nHeight);
			oOfferAccept.push_back(Pair("offer", offer));
			oOfferAccept.push_back(Pair("title", stringFromVch(theOffer.sTitle)));
			oOfferAccept.push_back(Pair("id", stringFromVch(theOfferAccept.vchRand)));
			oOfferAccept.push_back(Pair("alias", theOffer.aliasName));
			oOfferAccept.push_back(Pair("buyerkey", stringFromVch(theOfferAccept.vchBuyerKey)));
			oOfferAccept.push_back(Pair("height", sHeight));
			oOfferAccept.push_back(Pair("quantity", strprintf("%u", theOfferAccept.nQty)));
			oOfferAccept.push_back(Pair("currency", stringFromVch(theOffer.sCurrencyCode)));
			oOfferAccept.push_back(Pair("escrowlink", stringFromVch(theOfferAccept.vchEscrowLink)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oOfferAccept.push_back(Pair("price", strprintf("%.*f", precision, theOfferAccept.nPrice ))); 
			oOfferAccept.push_back(Pair("total", strprintf("%.*f", precision, theOfferAccept.nPrice * theOfferAccept.nQty ))); 
			// this accept is for me(something ive sold) if this offer is mine
			oOfferAccept.push_back(Pair("is_mine", IsOfferMine(offerTx)? "true" : "false"));
			if(theOfferAccept.bPaid && !theOfferAccept.bRefunded) {
				oOfferAccept.push_back(Pair("status","paid"));
			}
			else if(!theOfferAccept.bRefunded)
			{
				oOfferAccept.push_back(Pair("status","not paid"));
			}
			else if(theOfferAccept.bRefunded) { 
				oOfferAccept.push_back(Pair("status", "refunded"));
			}
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
        int nHeight;

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
            if (!DecodeOfferTx(wtx, op, nOut, vvch, -1) 
            	|| !IsOfferOp(op) 
            	|| (op == OP_OFFER_ACCEPT))
                continue;

            // get the txn height
            nHeight = GetTxHashHeight(hash);

            // get the txn alias name
            if(!GetNameOfOfferTx(wtx, vchName))
                continue;

			// skip this offer if it doesn't match the given filter value
			if (vchNameUniq.size() > 0 && vchNameUniq != vchName)
				continue;
			// get last active name only
			if (vNamesI.find(vchName) != vNamesI.end() && (nHeight < vNamesI[vchName] || vNamesI[vchName] < 0))
				continue;
			
			vector<COffer> vtxPos;
			COffer theOfferA;
			if (!pofferdb->ReadOffer(vchName, vtxPos))
			{
				pending = 1;
				theOfferA = COffer(tx);
			}
			if (vtxPos.size() < 1)
			{
				pending = 1;
				theOfferA = COffer(tx);
			}	
			if(pending != 1)
			{
				theOfferA = vtxPos.back();
			}
			
            // build the output UniValue
            UniValue oName(UniValue::VOBJ);
            oName.push_back(Pair("offer", stringFromVch(vchName)));
			oName.push_back(Pair("cert", stringFromVch(theOfferA.vchCert)));
            oName.push_back(Pair("title", stringFromVch(theOfferA.sTitle)));
            oName.push_back(Pair("category", stringFromVch(theOfferA.sCategory)));
            oName.push_back(Pair("description", stringFromVch(theOfferA.sDescription)));
			int precision = 2;
			convertCurrencyCodeToSyscoin(theOfferA.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
			oName.push_back(Pair("price", strprintf("%.*f", precision, theOfferA.GetPrice() ))); 	

			oName.push_back(Pair("currency", stringFromVch(theOfferA.sCurrencyCode) ) );
			oName.push_back(Pair("commission", strprintf("%d%%", theOfferA.nCommission)));
            oName.push_back(Pair("quantity", strprintf("%u", theOfferA.nQty)));
			string strAddress = "";
            GetOfferAddress(tx, strAddress);
			CSyscoinAddress address(strAddress);
			if(address.IsValid() && address.isAlias)
				oName.push_back(Pair("address", address.aliasName));
			else
				oName.push_back(Pair("address", address.ToString()));
			oName.push_back(Pair("exclusive_resell", theOfferA.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
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
			oName.push_back(Pair("alias", theOfferA.aliasName));
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
			throw JSONRPCError(RPC_WALLET_ERROR,
					"failed to read from offer DB");

		COffer txPos2;
		uint256 txHash;
		uint256 blockHash;
		BOOST_FOREACH(txPos2, vtxPos) {
			txHash = txPos2.txHash;
			CTransaction tx;
			if (!GetTransaction(txHash, tx, Params().GetConsensus(), blockHash, true)) {
				error("could not read txpos");
				continue;
			}
			int expired = 0;
			int expires_in = 0;
			int expired_block = 0;
			UniValue oOffer(UniValue::VOBJ);
			vector<unsigned char> vchValue;
			int nHeight;
			uint256 hash;
			if (GetValueOfOfferTxHash(txHash, vchValue, hash, nHeight)) {
				oOffer.push_back(Pair("offer", offer));
				string value = stringFromVch(vchValue);
				oOffer.push_back(Pair("value", value));
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
				oOffer.push_back(Pair("alias", txPos2.aliasName));
				oOffer.push_back(Pair("expires_in", expires_in));
				oOffer.push_back(Pair("expires_on", expired_block));
				oOffer.push_back(Pair("expired", expired));
				oRes.push_back(oOffer);
			}
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
	if (!pofferdb->ScanOffers(vchOffer, 100000000, offerScan))
		throw JSONRPCError(RPC_WALLET_ERROR, "scan failed");

    // regexp
    using namespace boost::xpressive;
    smatch offerparts;
    sregex cregex = sregex::compile(strRegexp);

	pair<vector<unsigned char>, COffer> pairScan;
	BOOST_FOREACH(pairScan, offerScan) {
		COffer txOffer = pairScan.second;
		string offer = stringFromVch(txOffer.vchRand);
		string title = stringFromVch(txOffer.sTitle);
		string alias = txOffer.aliasName;
        if (strRegexp != "" && !regex_search(title, offerparts, cregex) && strRegexp != offer && strRegexp != alias)
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
        uint256 blockHash;
		if (!GetTransaction(txOffer.txHash, tx, Params().GetConsensus(), blockHash, true))
			continue;
		// dont return sold out offers
		if(txOffer.nQty <= 0)
			continue;
		if(txOffer.bPrivate)
			continue;
		UniValue oOffer(UniValue::VOBJ);
		oOffer.push_back(Pair("offer", offer));
		oOffer.push_back(Pair("cert", stringFromVch(txOffer.vchCert)));
        oOffer.push_back(Pair("title", stringFromVch(txOffer.sTitle)));
		oOffer.push_back(Pair("description", stringFromVch(txOffer.sDescription)));
        oOffer.push_back(Pair("category", stringFromVch(txOffer.sCategory)));
		int precision = 2;
		convertCurrencyCodeToSyscoin(txOffer.sCurrencyCode, 0, chainActive.Tip()->nHeight, precision);
		oOffer.push_back(Pair("price", strprintf("%.*f", precision, txOffer.GetPrice() ))); 	
		oOffer.push_back(Pair("currency", stringFromVch(txOffer.sCurrencyCode)));
		oOffer.push_back(Pair("commission", strprintf("%d%%", txOffer.nCommission)));
        oOffer.push_back(Pair("quantity", strprintf("%u", txOffer.nQty)));
		oOffer.push_back(Pair("exclusive_resell", txOffer.linkWhitelist.bExclusiveResell ? "ON" : "OFF"));
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
		oOffer.push_back(Pair("alias", txOffer.aliasName));
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
		throw JSONRPCError(RPC_WALLET_ERROR, "scan failed");

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
		if(txOffer.nQty <= 0)
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
		oOffer.push_back(Pair("alias", txOffer.aliasName));
		oOffer.push_back(Pair("expires_in", expires_in));
		oOffer.push_back(Pair("expires_on", expired_block));
		oOffer.push_back(Pair("expired", expired));
		oRes.push_back(oOffer);
	}

	return oRes;
}

