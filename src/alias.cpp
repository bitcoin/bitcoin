// Copyright (c) 2014 Syscoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
//
#include "alias.h"
#include "offer.h"
#include "escrow.h"
#include "cert.h"
#include "offer.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "random.h"
#include "wallet/wallet.h"
#include "rpc/server.h"
#include "base58.h"
#include "txmempool.h"
#include "txdb.h"
#include "chainparams.h"
#include "core_io.h"
#include "policy/policy.h"
#include "utiltime.h"
#include "coincontrol.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/find.hpp>
#include <mongoc.h>
using namespace std;
CAliasDB *paliasdb = NULL;
COfferDB *pofferdb = NULL;
CCertDB *pcertdb = NULL;
CEscrowDB *pescrowdb = NULL;
typedef map<vector<unsigned char>, COutPoint > mapAliasRegistrationsType;
typedef map<vector<unsigned char>, vector<unsigned char> > mapAliasRegistrationsDataType;
mapAliasRegistrationsType mapAliasRegistrations;
mapAliasRegistrationsDataType mapAliasRegistrationData;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
extern int nIndexPort;
mongoc_client_t *client = NULL;
mongoc_database_t *database = NULL;
mongoc_collection_t *alias_collection = NULL;
mongoc_collection_t *offer_collection = NULL;
mongoc_collection_t *escrow_collection = NULL;
mongoc_collection_t *cert_collection = NULL;
mongoc_collection_t *feedback_collection = NULL;
unsigned int MAX_ALIAS_UPDATES_PER_BLOCK = 5;
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, const Consensus::Params& consensusParams)
{
	if(nHeight < 0 || nHeight > chainActive.Height())
		return false;
	CBlockIndex *pindexSlow = NULL; 
	LOCK(cs_main);
	pindexSlow = chainActive[nHeight];
    if (pindexSlow) {
        CBlock block;
        if (ReadBlockFromDisk(block, pindexSlow, consensusParams)) {
            BOOST_FOREACH(const CTransaction &tx, block.vtx) {
                if (tx.GetHash() == hash) {
                    txOut = tx;
                    return true;
                }
            }
        }
    }
	return false;
}
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, uint256 &hashBlock, const Consensus::Params& consensusParams)
{
	if(nHeight < 0 || nHeight > chainActive.Height())
		return false;
	CBlockIndex *pindexSlow = NULL; 
	LOCK(cs_main);
	pindexSlow = chainActive[nHeight];
    if (pindexSlow) {
        CBlock block;
        if (ReadBlockFromDisk(block, pindexSlow, consensusParams)) {
            BOOST_FOREACH(const CTransaction &tx, block.vtx) {
                if (tx.GetHash() == hash) {
                    txOut = tx;
					hashBlock = pindexSlow->GetBlockHash();
                    return true;
                }
            }
        }
    }
	return false;
}
bool GetTimeToPrune(const CScript& scriptPubKey, uint64_t &nTime)
{
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	if(!GetSyscoinData(scriptPubKey, vchData, vchHash))
		return false;
	if(!chainActive.Tip())
		return false;
	CAliasIndex alias;
	COffer offer;
	CEscrow escrow;
	CCert cert;
	nTime = 0;
	if(alias.UnserializeFromData(vchData, vchHash))
	{
		if(alias.vchAlias == vchFromString("sysrates.peg"))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		CAliasUnprunable aliasUnprunable;
		// we only prune things that we have in our db and that we can verify the last tx is expired
		// nHeight is set to the height at which data is pruned, if the tip is newer than nHeight it won't send data to other nodes
		// we want to keep history of all of the old tx's related to aliases that were renewed, we can't delete its history otherwise we won't know 
		// to tell nodes that aliases were renewed and to update their info pertaining to that alias.
		if (paliasdb->ReadAliasUnprunable(alias.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		{
			// if we are renewing alias then prune based on max of expiry of alias in tx vs the stored alias expiry time of latest alias tx
			if (!alias.vchGUID.empty() && aliasUnprunable.vchGUID != alias.vchGUID)
				nTime = max(alias.nExpireTime, aliasUnprunable.nExpireTime);
			else
				nTime = aliasUnprunable.nExpireTime;

			return true;
		}
		// this is a new service, either sent to us because it's not supposed to be expired yet or sent to ourselves as a new service, either way we keep the data and validate it into the service db
		else
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
	}
	else if(offer.UnserializeFromData(vchData, vchHash))
	{
		uint256 txid;
		if (!pofferdb || !pofferdb->ReadOfferLastTXID(offer.vchOffer, txid))
		{			
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		if (!pofferdb->ReadOffer(CNameTXIDTuple(offer.vchOffer, txid), offer))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		nTime = GetOfferExpiration(offer);
		return true; 
	}
	else if(cert.UnserializeFromData(vchData, vchHash))
	{
		uint256 txid;
		if (!pcertdb || !pcertdb->ReadCertLastTXID(cert.vchCert, txid))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		if (!pcertdb->ReadCert(CNameTXIDTuple(cert.vchCert, txid), cert))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		nTime = GetCertExpiration(cert);
		return true; 
	}
	else if(escrow.UnserializeFromData(vchData, vchHash))
	{
		uint256 txid;
		if (!pescrowdb || !pescrowdb->ReadEscrowLastTXID(escrow.vchEscrow, txid))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		if (!pescrowdb->ReadEscrow(CNameTXIDTuple(escrow.vchEscrow, txid), escrow))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		nTime = GetEscrowExpiration(escrow);
		return true;
	}
	return false;
}
bool IsSysServiceExpired(const uint64_t &nTime)
{
	if(!chainActive.Tip() || fTxIndex)
		return false;
	return (chainActive.Tip()->nTime >= nTime);

}
bool IsSyscoinScript(const CScript& scriptPubKey, int &op, vector<vector<unsigned char> > &vvchArgs)
{
	if (DecodeAliasScript(scriptPubKey, op, vvchArgs))
		return true;
	else if(DecodeOfferScript(scriptPubKey, op, vvchArgs))
		return true;
	else if(DecodeCertScript(scriptPubKey, op, vvchArgs))
		return true;
	else if(DecodeEscrowScript(scriptPubKey, op, vvchArgs))
		return true;
	return false;
}
bool RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut)
{
	if (!RemoveAliasScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
		if (!RemoveOfferScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
			if (!RemoveCertScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
				if (!RemoveEscrowScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
					return false;
	return true;
					
}
// how much is 1.1 BTC in syscoin? 1 BTC = 110000 SYS for example, nPrice would be 1.1, sysPrice would be 110000
CAmount convertCurrencyCodeToSyscoin(const CNameTXIDTuple &aliasPegTuple, const vector<unsigned char> &vchCurrencyCode, const double &nPrice, int &precision)
{
	CAmount sysPrice = 0;
	double nRate = 1;
	float fEscrowFee = 0.005;
	int nFeePerByte;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(aliasPegTuple, vchCurrencyCode, nRate, rateList, precision, nFeePerByte, fEscrowFee) == "")
		{
			float fTotal = nPrice*nRate;
			CAmount nTotal = fTotal;
			int myprecision = precision;
			if(myprecision < 8)
				myprecision += 1;
			if(nTotal != fTotal)
				sysPrice = AmountFromValue(strprintf("%.*f", myprecision, fTotal)); 
			else
				sysPrice = nTotal*COIN;

		}
	}
	catch(...)
	{
		if(fDebug)
			LogPrintf("convertCurrencyCodeToSyscoin() Exception caught getting rate alias information\n");
	}
	if(precision > 8)
		sysPrice = 0;
	return sysPrice;
}
float getEscrowFee(const CNameTXIDTuple &aliasPegTuple, const std::vector<unsigned char> &vchCurrencyCode, int &precision)
{
	double nRate;
	int nFeePerByte =0;
	// 0.05% escrow fee by default it not provided
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(aliasPegTuple, vchCurrencyCode, nRate, rateList, precision, nFeePerByte, fEscrowFee) == "")
		{		
			return fEscrowFee;
		}
	}
	catch(...)
	{
		if(fDebug)
			LogPrintf("getEscrowFee() Exception caught getting rate alias information\n");
	}
	return fEscrowFee;
}
int getFeePerByte(const CNameTXIDTuple &aliasPegTuple, const std::vector<unsigned char> &vchCurrencyCode, int &precision)
{
	double nRate;
	int nFeePerByte = 25;
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(aliasPegTuple, vchCurrencyCode, nRate, rateList, precision, nFeePerByte, fEscrowFee) == "")
		{
			return nFeePerByte;
		}
	}
	catch(...)
	{
		if(fDebug)
			LogPrintf("getBTCFeePerByte() Exception caught getting rate alias information\n");
	}
	return nFeePerByte;
}
// convert 110000*COIN SYS into 1.1*COIN BTC
CAmount convertSyscoinToCurrencyCode(const CNameTXIDTuple &aliasPegTuple, const vector<unsigned char> &vchCurrencyCode, const CAmount &nPrice, int &precision)
{
	CAmount currencyPrice = 0;
	double nRate = 1;
	int nFeePerByte;
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(aliasPegTuple, vchCurrencyCode, nRate, rateList, precision, nFeePerByte, fEscrowFee) == "")
		{
			currencyPrice = CAmount(nPrice/nRate);
		}
	}
	catch(...)
	{
		if(fDebug)
			LogPrintf("convertSyscoinToCurrencyCode() Exception caught getting rate alias information\n");
	}
	if(precision > 8)
		currencyPrice = 0;
	return currencyPrice;
}
string getCurrencyToSYSFromAlias(const CNameTXIDTuple &aliasPegTuple, const vector<unsigned char> &vchCurrency, double &nFee, vector<string>& rateList, int &precision, int &nFeePerByte, float &fEscrowFee)
{
	string currencyCodeToFind = stringFromVch(vchCurrency);
	// check for alias existence in DB
	CAliasIndex foundAlias;
	if (!GetAlias(CNameTXIDTuple(aliasPegTuple.first, aliasPegTuple.second), foundAlias))
	{
		if(fDebug)
			LogPrintf("getCurrencyToSYSFromAlias() Could not find %s alias\n", stringFromVch(aliasPegTuple.first).c_str());
		return "1";
	}

	bool found = false;
	string value = stringFromVch(foundAlias.vchPublicValue);
	
	UniValue outerValue(UniValue::VSTR);
	bool read = outerValue.read(value);
	if (read)
	{
		UniValue outerObj = outerValue.get_obj();
		UniValue ratesValue = find_value(outerObj, "rates");
		if (ratesValue.isArray())
		{
			UniValue codes = ratesValue.get_array();
			for (unsigned int idx = 0; idx < codes.size(); idx++) {
				const UniValue& code = codes[idx];					
				UniValue codeObj = code.get_obj();					
				UniValue currencyNameValue = find_value(codeObj, "currency");
				UniValue currencyAmountValue = find_value(codeObj, "rate");
				if (currencyNameValue.isStr())
				{		
					string currencyCode = currencyNameValue.get_str();
					rateList.push_back(currencyCode);
					if(currencyCodeToFind == currencyCode)
					{		
						UniValue feePerByteValue = find_value(codeObj, "fee");
						if(feePerByteValue.isNum())
						{
							nFeePerByte = feePerByteValue.get_int();
						}
						UniValue escrowFeeValue = find_value(codeObj, "escrowfee");
						if(escrowFeeValue.isNum())
						{
							fEscrowFee = escrowFeeValue.get_real();
						}
						UniValue precisionValue = find_value(codeObj, "precision");
						if(precisionValue.isNum())
						{
							precision = precisionValue.get_int();
						}
						if(currencyAmountValue.isNum())
						{
							found = true;
							try{
							
								nFee = currencyAmountValue.get_real();
							}
							catch(std::runtime_error& err)
							{
								try
								{
									nFee = currencyAmountValue.get_int();
								}
								catch(std::runtime_error& err)
								{
									if(fDebug)
										LogPrintf("getCurrencyToSYSFromAlias() Failed to get currency amount from value\n");
									return "1";
								}
							}
							
						}
					}
				}
			}
		}
		
	}
	else
	{
		if(fDebug)
			LogPrintf("getCurrencyToSYSFromAlias() Failed to get value from alias\n");
		return "1";
	}
	if(!found)
	{
		if(fDebug)
			LogPrintf("getCurrencyToSYSFromAlias() currency %s not found in %s alias\n", stringFromVch(vchCurrency).c_str(), stringFromVch(aliasPegTuple.first).c_str());
		return "0";
	}
	return "";
}
void getCategoryListFromValue(vector<string>& categoryList,const UniValue& outerValue)
{
	UniValue outerObj = outerValue.get_obj();
	UniValue objCategoriesValue = find_value(outerObj, "categories");
	UniValue categories = objCategoriesValue.get_array();
	for (unsigned int idx = 0; idx < categories.size(); idx++) {
		const UniValue& category = categories[idx];
		const UniValue& categoryObj = category.get_obj();	
		const UniValue categoryValue = find_value(categoryObj, "cat");
		categoryList.push_back(categoryValue.get_str());
	}
}

void PutToAliasList(std::vector<CAliasIndex> &aliasList, CAliasIndex& index) {
	int i = aliasList.size() - 1;
	BOOST_REVERSE_FOREACH(CAliasIndex &o, aliasList) {
        if(!o.txHash.IsNull() && o.txHash == index.txHash) {
        	aliasList[i] = index;
            return;
        }
        i--;
	}
    aliasList.push_back(index);
}

bool IsAliasOp(int op, bool ismine) {
	if(ismine)
		return op == OP_ALIAS_ACTIVATE
			|| op == OP_ALIAS_UPDATE;
	else
		return op == OP_ALIAS_ACTIVATE
			|| op == OP_ALIAS_UPDATE
			|| op == OP_ALIAS_PAYMENT;
}
string aliasFromOp(int op) {
	switch (op) {
	case OP_ALIAS_UPDATE:
		return "aliasupdate";
	case OP_ALIAS_ACTIVATE:
		return "aliasactivate";
	case OP_ALIAS_PAYMENT:
		return "aliaspayment";
	default:
		return "<unknown alias op>";
	}
}
int GetSyscoinDataOutput(const CTransaction& tx) {
   for(unsigned int i = 0; i<tx.vout.size();i++) {
	   if(IsSyscoinDataOutput(tx.vout[i]))
		   return i;
	}
   return -1;
}
bool IsSyscoinDataOutput(const CTxOut& out) {
   txnouttype whichType;
	if (!IsStandard(out.scriptPubKey, whichType))
		return false;
	if (whichType == TX_NULL_DATA)
		return true;
   return false;
}
int GetSyscoinTxVersion()
{
	return SYSCOIN_TX_VERSION;
}

/**
 * [IsSyscoinTxMine check if this transaction is mine or not, must contain a syscoin service vout]
 * @param  tx [syscoin based transaction]
 * @param  type [the type of syscoin service you expect in this transaction]
 * @return    [if syscoin transaction is yours based on type passed in]
 */
bool IsSyscoinTxMine(const CTransaction& tx, const string &type) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;
	int myNout;
	vector<vector<unsigned char> > vvch;
	if ((type == "alias" || type == "any"))
		myNout = IndexOfAliasOutput(tx);
	else if ((type == "offer" || type == "any"))
		myNout = IndexOfOfferOutput(tx);
	else if ((type == "cert" || type == "any"))
		myNout = IndexOfCertOutput(tx);
	else if ((type == "escrow" || type == "any"))
		myNout = IndexOfEscrowOutput(tx);
	else
		return false;
	return myNout >= 0;
}
bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, string &errorMessage, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add alias in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug)
		LogPrintf("*** ALIAS %d %d op=%s %s nOut=%d %s\n", nHeight, chainActive.Tip()->nHeight, aliasFromOp(op).c_str(), tx.GetHash().ToString().c_str(), nOut, fJustCheck ? "JUSTCHECK" : "BLOCK");
	const COutPoint *prevOutput = NULL;
	const CCoins *prevCoins;
	int prevOp = 0;
	vector<vector<unsigned char> > vvchPrevArgs;
	// Make sure alias outputs are not spent by a regular transaction, or the alias would be lost
	if (tx.nVersion != SYSCOIN_TX_VERSION) 
	{
		errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5000 - " + _("Non-Syscoin transaction found");
		return true;
	}
	// unserialize alias from txn, check for valid
	CAliasIndex theAlias;
	vector<unsigned char> vchData;
	vector<unsigned char> vchAlias;
	vector<unsigned char> vchHash;
	int nDataOut;
	
	bool bData = GetSyscoinData(tx, vchData, vchHash, nDataOut);
	if(bData && !theAlias.UnserializeFromData(vchData, vchHash))
	{
		theAlias.SetNull();
	}
	// we need to check for cert update specially because an alias update without data is sent along with offers linked with the alias
	else if (!bData)
	{
		if(fDebug)
			LogPrintf("CheckAliasInputs(): Null alias, skipping...\n");	
		return true;
	}
	

	if(fJustCheck)
	{
		
		if(vvchArgs.size() != 4)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5001 - " + _("Alias arguments incorrect size");
			return error(errorMessage.c_str());
		}
		
		else if(vvchArgs.size() <= 0 || vvchArgs.size() > 4)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5002 - " + _("Alias payment arguments incorrect size");
			return error(errorMessage.c_str());
		}
		
		if(!theAlias.IsNull())
		{
			if(vvchArgs.size() <= 2 || vchHash != vvchArgs[2])
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5003 - " + _("Hash provided doesn't match the calculated hash of the data");
				return true;
			}
		}					
		
	}
	if(fJustCheck || op != OP_ALIAS_ACTIVATE)
	{
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
			{
				prevCoins = NULL;
				continue;
			}
			if (IsAliasOp(pop, true) && ((vvchArgs[0] == vvch[0] && vvchArgs[1] == vvch[1]) || op == OP_ALIAS_ACTIVATE)) {
				prevOp = pop;
				vvchPrevArgs = vvch;
				break;
			}
		}
		if(vvchArgs.size() >= 4 && !vvchArgs[3].empty())
		{
			bool bWitnessSigFound = false;
			for (unsigned int i = 0; i < tx.vin.size(); i++) {
				vector<vector<unsigned char> > vvch;
				int pop;
				const COutPoint* prevOutputWitness = &tx.vin[i].prevout;
				if(!prevOutputWitness)
					continue;
				// ensure inputs are unspent when doing consensus check to add to block
				const CCoins* prevCoinsWitness = inputs.AccessCoins(prevOutputWitness->hash);
				if(prevCoinsWitness == NULL)
					continue;
				if(!prevCoinsWitness->IsAvailable(prevOutputWitness->n) || !IsSyscoinScript(prevCoinsWitness->vout[prevOutputWitness->n].scriptPubKey, pop, vvch))
				{
					continue;
				}
				// match 4th element in scriptpubkey of alias update with witness input scriptpubkey, if names match then sig is provided
				if (IsAliasOp(pop, true) && vvchArgs[3] == vvch[0]) {
					bWitnessSigFound = true;
					break;
				}
			}
			if(!bWitnessSigFound)
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5002 - " + _("Witness signature not found");
				return error(errorMessage.c_str());
			}
		}
	}
	CRecipient fee;
	string retError = "";
	if(fJustCheck)
	{
		if(vvchArgs.empty() || !IsValidAliasName(vvchArgs[0]))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5004 - " + _("Alias name does not follow the domain name specification");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPublicValue.size() > MAX_VALUE_LENGTH && vvchArgs[0] != vchFromString("sysrates.peg"))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5005 - " + _("Alias public value too big");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPrivateValue.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5006 - " + _("Alias private value too big");
			return error(errorMessage.c_str());
		}
		if(theAlias.aliasPegTuple.first.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5007 - " + _("Alias peg too long");
			return error(errorMessage.c_str());
		}

		if(theAlias.nHeight > nHeight)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5009 - " + _("Bad alias height");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchEncryptionPrivateKey.size() > MAX_ENCRYPTED_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Encryption private key too long");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchEncryptionPublicKey.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Encryption public key too long");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPasswordSalt.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 3006 - " + _("Password salt too long");
			return error(errorMessage.c_str());
		}
		switch (op) {
			case OP_ALIAS_ACTIVATE:
				if (prevOp != OP_ALIAS_ACTIVATE)
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5009 - " + _("Alias input to this transaction not found");
					return error(errorMessage.c_str());
				}
				// Check new/activate hash
				if (vvchPrevArgs.size() <= 0 || vvchPrevArgs[0] != vchHash)
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5010 - " + _("Alias new and activate hash mismatch");
					return error(errorMessage.c_str());
				}
				// Check GUID
				if (vvchArgs.size() <=  1 || theAlias.vchGUID != vvchArgs[1])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5010 - " + _("Alias input guid mismatch");
					return error(errorMessage.c_str());
				}
				if(theAlias.vchAlias != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5011 - " + _("Guid in data output doesn't match guid in tx");
					return error(errorMessage.c_str());
				}
				if(theAlias.vchAddress.empty())
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5011 - " + _("Alias address cannot be empty");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ALIAS_UPDATE:
				if (!IsAliasOp(prevOp, true))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5012 - " + _("Alias input to this transaction not found");
					return error(errorMessage.c_str());
				}
				if(!theAlias.IsNull())
				{
					if(theAlias.vchAlias != vvchArgs[0])
					{
						errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5015 - " + _("Guid in data output doesn't match guid in transaction");
						return error(errorMessage.c_str());
					}
				}
				break;
		default:
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5016 - " + _("Alias transaction has unknown op");
				return error(errorMessage.c_str());
		}

	}
	
	if (!fJustCheck ) {
		bool pwChange = false;
		CAliasIndex dbAlias;
		string strName = stringFromVch(vvchArgs[0]);
		boost::algorithm::to_lower(strName);
		vchAlias = vchFromString(strName);
		// get the alias from the DB
		if(!GetAlias(vchAlias, dbAlias))	
		{
			if(op == OP_ALIAS_UPDATE)
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5018 - " + _("Failed to read from alias DB");
				return true;
			}
		}
		if(!vchData.empty())
		{
			CAmount fee = GetDataFee(tx.vout[nDataOut].scriptPubKey,  (op == OP_ALIAS_ACTIVATE)? theAlias.aliasPegTuple:dbAlias.aliasPegTuple);
			float fYears;
			// if this is an alias payload get expire time and figure out if alias payload pays enough fees for expiry
			if(!theAlias.IsNull())
			{
				int nHeightTmp = nHeight;
				if(nHeightTmp > chainActive.Height())
					nHeightTmp = chainActive.Height();
				uint64_t nTimeExpiry = theAlias.nExpireTime - chainActive[nHeightTmp]->nTime;
				// ensure aliases are good for atleast an hour
				if (nTimeExpiry < 3600) {
					nTimeExpiry = 3600;
					theAlias.nExpireTime = chainActive[nHeightTmp]->nTime + 3600;
				}
				fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
				if(fYears < 1)
					fYears = 1;
				fee *= powf(2.88,fYears);
			}
			if ((fee-10000) > tx.vout[nDataOut].nValue) 
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5019 - " + _("Transaction does not pay enough fee: ") + ValueFromAmount(tx.vout[nDataOut].nValue).write() + "/" + ValueFromAmount(fee-10000).write() + "/" + boost::lexical_cast<string>(fYears) + " years.";
				return true;
			}
		}
				
		if(op == OP_ALIAS_UPDATE)
		{
			if(!dbAlias.IsNull())
			{
				CTxDestination aliasDest;
				if (vvchPrevArgs.size() <= 0 || vvchPrevArgs[0] != vvchArgs[0] || !prevCoins || !prevOutput || prevOutput->n >= prevCoins->vout.size() || !ExtractDestination(prevCoins->vout[prevOutput->n].scriptPubKey, aliasDest))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5020 - " + _("Cannot extract destination of alias input");
					theAlias = dbAlias;
				}
				else
				{
					CSyscoinAddress prevaddy(aliasDest);	
					if(EncodeBase58(dbAlias.vchAddress) != prevaddy.ToString())
					{
						errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5021 - " + _("You are not the owner of this alias");
						theAlias = dbAlias;
					}
				}
				if(dbAlias.vchGUID != vvchArgs[1])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5022 - " + _("Cannot edit this alias, guid mismatch");
					theAlias = dbAlias;
				}
				if(theAlias.IsNull())
					theAlias = dbAlias;
				else
				{
					if(theAlias.vchPublicValue.empty())
						theAlias.vchPublicValue = dbAlias.vchPublicValue;	
					if(theAlias.vchPrivateValue.empty())
						theAlias.vchPrivateValue = dbAlias.vchPrivateValue;	
					if(theAlias.vchEncryptionPrivateKey.empty())
						theAlias.vchEncryptionPrivateKey = dbAlias.vchEncryptionPrivateKey;
					if(theAlias.vchEncryptionPublicKey.empty())
						theAlias.vchEncryptionPublicKey = dbAlias.vchEncryptionPublicKey;
					if(theAlias.vchPasswordSalt.empty())
						theAlias.vchPasswordSalt = dbAlias.vchPasswordSalt;
					// can't change alias peg name once it is set, only update it to latest txid
					if (!theAlias.aliasPegTuple.IsNull() && theAlias.aliasPegTuple.first != dbAlias.aliasPegTuple.first)
						theAlias.aliasPegTuple = dbAlias.aliasPegTuple;

					if(theAlias.vchAddress.empty())
						theAlias.vchAddress = dbAlias.vchAddress;
					theAlias.vchGUID = dbAlias.vchGUID;
					theAlias.vchAlias = dbAlias.vchAlias;
					// if transfer
					if(dbAlias.vchAddress != theAlias.vchAddress)
					{
						// make sure xfer to pubkey doesn't point to an alias already, otherwise don't assign pubkey to alias
						// we want to avoid aliases with duplicate addresses
						if (paliasdb->ExistsAddress(theAlias.vchAddress))
						{
							vector<unsigned char> vchMyAlias;
							if (paliasdb->ReadAddress(theAlias.vchAddress, vchMyAlias) && !vchMyAlias.empty() && vchMyAlias != dbAlias.vchAlias)
							{
								CAliasIndex dbReadAlias;
								// ensure that you block transferring only if the recv address has an active alias associated with it
								if (GetAlias(vchMyAlias, dbReadAlias)) {
									errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("An alias already exists with that address, try another public key");
									theAlias = dbAlias;
								}
							}
						}
						if(dbAlias.nAccessFlags < 2)
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("Cannot edit this alias. Insufficient privileges.");
							theAlias = dbAlias;
						}
						// let old address be re-occupied by a new alias
						if (!dontaddtodb && errorMessage.empty())
						{
							paliasdb->EraseAddress(dbAlias.vchAddress);
						}
					}
					else
					{
						if(dbAlias.nAccessFlags < 1)
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("Cannot edit this alias. It is view-only.");
							theAlias = dbAlias;
						}
					}
					if(theAlias.nAccessFlags > dbAlias.nAccessFlags)
					{
						errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot modify for more lenient access. Only tighter access level can be granted.");
						theAlias = dbAlias;
					}
				}
			}
			else
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5027 -" + _("Alias not found when trying to update");
				return true;
			}
		}
		else if(op == OP_ALIAS_ACTIVATE)
		{
			if(!dbAlias.IsNull())
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5028 - " + _("Trying to renew an alias that isn't expired");
				return true;
			}
		}
		theAlias.nHeight = nHeight;
		theAlias.txHash = tx.GetHash();
	
		CAliasUnprunable aliasUnprunable;
		aliasUnprunable.vchGUID = theAlias.vchGUID;
		aliasUnprunable.nExpireTime = theAlias.nExpireTime;
		if (!dontaddtodb && !paliasdb->WriteAlias(aliasUnprunable, theAlias.vchAddress, theAlias))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5034 - " + _("Failed to write to alias DB");
			return error(errorMessage.c_str());
		}
	
		if(fDebug)
			LogPrintf(
				"CONNECTED ALIAS: name=%s  op=%s  hash=%s  height=%d\n",
				stringFromVch(vchAlias).c_str(),
				aliasFromOp(op).c_str(),
				tx.GetHash().ToString().c_str(), nHeight);
	}

	return true;
}

string stringFromValue(const UniValue& value) {
	string strName = value.get_str();
	return strName;
}
vector<unsigned char> vchFromValue(const UniValue& value) {
	string strName = value.get_str();
	unsigned char *strbeg = (unsigned char*) strName.c_str();
	return vector<unsigned char>(strbeg, strbeg + strName.size());
}

std::vector<unsigned char> vchFromString(const std::string &str) {
	unsigned char *strbeg = (unsigned char*) str.c_str();
	return vector<unsigned char>(strbeg, strbeg + str.size());
}

string stringFromVch(const vector<unsigned char> &vch) {
	string res;
	vector<unsigned char>::const_iterator vi = vch.begin();
	while (vi != vch.end()) {
		res += (char) (*vi);
		vi++;
	}
	return res;
}
bool GetSyscoinData(const CTransaction &tx, vector<unsigned char> &vchData, vector<unsigned char> &vchHash, int& nOut)
{
	nOut = GetSyscoinDataOutput(tx);
    if(nOut == -1)
	   return false;

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData, vchHash);
}
bool IsValidAliasName(const std::vector<unsigned char> &vchAlias)
{
	return (vchAlias.size() <= MAX_GUID_LENGTH && vchAlias.size() >= 3);
}
bool GetSyscoinData(const CScript &scriptPubKey, vector<unsigned char> &vchData, vector<unsigned char> &vchHash)
{
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if(opcode != OP_RETURN)
		return false;
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
	if (!scriptPubKey.GetOp(pc, opcode, vchHash))
		return false;
	return true;
}
void GetAddress(const CAliasIndex& alias, CSyscoinAddress* address,CScript& script,const uint32_t nPaymentOption)
{
	if(!address)
		return;
	CChainParams::AddressType myAddressType = PaymentOptionToAddressType(nPaymentOption);
	CSyscoinAddress addrTmp = CSyscoinAddress(EncodeBase58(alias.vchAddress));
	address[0] = CSyscoinAddress(addrTmp.Get(), myAddressType);
	script = GetScriptForDestination(address[0].Get());
}
bool CAliasIndex::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsAlias(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsAlias >> *this;

		vector<unsigned char> vchAliasData;
		Serialize(vchAliasData);
		const uint256 &calculatedHash = Hash(vchAliasData.begin(), vchAliasData.end());
		const vector<unsigned char> &vchRandAlias = vchFromValue(calculatedHash.GetHex());
		if(vchRandAlias != vchHash)
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
bool CAliasIndex::UnserializeFromTx(const CTransaction &tx) {
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
void CAliasIndex::Serialize(vector<unsigned char>& vchData) {
    CDataStream dsAlias(SER_NETWORK, PROTOCOL_VERSION);
    dsAlias << *this;
    vchData = vector<unsigned char>(dsAlias.begin(), dsAlias.end());

}

// TODO: need to cleanout CTxOuts (transactions stored on disk) which have data stored in them after expiry, erase at same time on startup so pruning can happen properly
bool CAliasDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAliasIndex txPos;
	pair<string, CNameTXIDTuple > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "namei") {
				const CNameTXIDTuple &aliasTuple = key.second;
				if(aliasTuple.first == vchFromString("sysrates.peg"))
				{
					pcursor->Next();
					continue;
				}
				pcursor->GetValue(txPos);
  				if (chainActive.Tip()->nTime >= txPos.nExpireTime)
				{
					servicesCleaned++;
					EraseAlias(aliasTuple);
				} 
				
            }
			else if (pcursor->GetKey(key) && key.first == "namea") {
				CNameTXIDTuple aliasTuple;
				CAliasIndex alias;
				pcursor->GetValue(aliasTuple);
				if (aliasTuple.first == vchFromString("sysrates.peg"))
				{
					pcursor->Next();
					continue;
				}
				if (GetAlias(aliasTuple.first, alias) && chainActive.Tip()->nTime >= alias.nExpireTime)
				{
					servicesCleaned++;
					EraseAddress(alias.vchAddress);
				}

			}
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
void CleanupSyscoinServiceDatabases(int &numServicesCleaned)
{
	if(pofferdb != NULL)
		pofferdb->CleanupDatabase(numServicesCleaned);
	if(pescrowdb!= NULL)
		pescrowdb->CleanupDatabase(numServicesCleaned);
	if(pcertdb!= NULL)
		pcertdb->CleanupDatabase(numServicesCleaned);
	if (paliasdb != NULL) 
		paliasdb->CleanupDatabase(numServicesCleaned);
	
	if(paliasdb != NULL)
	{
		if (!paliasdb->Flush())
			LogPrintf("Failed to write to alias database!");
		delete paliasdb;
		paliasdb = NULL;
	}
	if(pofferdb != NULL)
	{
		if (!pofferdb->Flush())
			LogPrintf("Failed to write to offer database!");
		delete pofferdb;
		pofferdb = NULL;
	}
	if(pcertdb != NULL)
	{
		if (!pcertdb->Flush())
			LogPrintf("Failed to write to cert database!");
		delete pcertdb;
		pcertdb = NULL;
	}
	if(pescrowdb != NULL)
	{
		if (!pescrowdb->Flush())
			LogPrintf("Failed to write to escrow database!");
		delete pescrowdb;
		pescrowdb = NULL;
	}
}
bool GetAlias(const CNameTXIDTuple &aliasTuple,
	CAliasIndex& txPos) {
	if (!paliasdb || !paliasdb->ReadAlias(CNameTXIDTuple(aliasTuple.first, aliasTuple.second), txPos))
		return false;
	return true;
}
bool GetAlias(const vector<unsigned char> &vchAlias,
	CAliasIndex& txPos) {
	uint256 txid;
	if (!paliasdb || !paliasdb->ReadAliasLastTXID(vchAlias, txid))
		return false;
	if (!paliasdb->ReadAlias(CNameTXIDTuple(vchAlias, txid), txPos))
		return false;
	if (vchAlias != vchFromString("sysrates.peg")) {
		if (chainActive.Tip()->nTime >= txPos.nExpireTime) {
			txPos.SetNull();
			string alias = stringFromVch(vchAlias);
			LogPrintf("GetAlias(%s) : expired", alias.c_str());
			return false;
		}
	}
	return true;
}
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, std::vector<unsigned char> &vchPubKey) {

	string strLowerAlias = strAlias;
	boost::algorithm::to_lower(strLowerAlias);
	const vector<unsigned char> &vchAlias = vchFromValue(strLowerAlias);

	CAliasIndex alias;
	// check for alias existence in DB
	if (!GetAlias(vchAlias, alias))
		return false;

	strAddress = EncodeBase58(alias.vchAddress);
	vchPubKey = alias.vchEncryptionPublicKey;
	return true;
}

bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, std::vector<unsigned char> &vchPubKey) {

	vector<unsigned char> vchAddress;
	DecodeBase58(strAddress, vchAddress);
	if (!paliasdb || !paliasdb->ExistsAddress(vchAddress))
		return false;

	// check for alias address mapping existence in DB
	vector<unsigned char> vchAlias;
	if (!paliasdb->ReadAddress(vchAddress, vchAlias))
		return false;
	if (vchAlias.empty())
		return false;
	
	strAlias = stringFromVch(vchAlias);
	CAliasIndex alias;
	// check for alias existence in DB
	if (!GetAlias(vchAlias, alias))
		return false;
	vchPubKey = alias.vchEncryptionPublicKey;
	return true;
}
int IndexOfAliasOutput(const CTransaction& tx) {
	vector<vector<unsigned char> > vvch;
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
	int op;
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		// find an output you own
		if (pwalletMain->IsMine(out) && DecodeAliasScript(out.scriptPubKey, op, vvch) && op != OP_ALIAS_PAYMENT) {
			return i;
		}
	}
	return -1;
}

bool GetAliasOfTx(const CTransaction& tx, vector<unsigned char>& name) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;
	vector<vector<unsigned char> > vvchArgs;
	int op;
	int nOut;

	bool good = DecodeAliasTx(tx, op, nOut, vvchArgs, false) || DecodeAliasTx(tx, op, nOut, vvchArgs, true);
	if (!good)
		return error("GetAliasOfTx() : could not decode a syscoin tx");

	switch (op) {
	case OP_ALIAS_ACTIVATE:
	case OP_ALIAS_UPDATE:
	case OP_ALIAS_PAYMENT:
		name = vvchArgs[0];
		return true;
	}
	return false;
}
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	return  
		DecodeAndParseCertTx(tx, op, nOut, vvch)
		|| DecodeAndParseOfferTx(tx, op, nOut, vvch)
		|| DecodeAndParseEscrowTx(tx, op, nOut, vvch)
		|| DecodeAndParseAliasTx(tx, op, nOut, vvch);
}
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	CAliasIndex alias;
	bool decode = DecodeAliasTx(tx, op, nOut, vvch, false);
	if(decode)
	{
		bool parse = alias.UnserializeFromTx(tx);
		return decode && parse;
	}
	return DecodeAliasTx(tx, op, nOut, vvch, true);
}
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, bool payment) {
	bool found = false;


	// Strict check - bug disallowed
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		vector<vector<unsigned char> > vvchRead;
		if (DecodeAliasScript(out.scriptPubKey, op, vvchRead) && ((op == OP_ALIAS_PAYMENT && payment) || (op != OP_ALIAS_PAYMENT && !payment))) {
			if(op == OP_ALIAS_PAYMENT || vvchRead.size() >= 3)
			{
				nOut = i;
				found = true;
				vvch = vvchRead;
				break;
			}
		}
	}
	if (!found)
		vvch.clear();

	return found;
}


bool DecodeAliasScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
	opcodetype opcode;
	vvch.clear();
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;

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
	return found && IsAliasOp(op);
}
bool DecodeAliasScript(const CScript& script, int& op,
		vector<vector<unsigned char> > &vvch) {
	CScript::const_iterator pc = script.begin();
	return DecodeAliasScript(script, op, vvch, pc);
}
bool RemoveAliasScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
	int op;
	vector<vector<unsigned char> > vvch;
	CScript::const_iterator pc = scriptIn.begin();

	if (!DecodeAliasScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient)
{
	CRecipient recp = {scriptPubKey, recipient.nAmount, false};
	recipient = recp;
	CTxOut txout(recipient.nAmount,	recipient.scriptPubKey);
    size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	CAmount nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	recipient.nAmount = nFee;
}
void CreateAliasRecipient(const CScript& scriptPubKeyDest, const vector<unsigned char>& vchAlias, const CNameTXIDTuple& aliasPegTuple, CRecipient& recipient)
{
	int precision = 0;
	CAmount nFee = 0;
	CScript scriptChangeOrig;
	scriptChangeOrig << CScript::EncodeOP_N(OP_ALIAS_PAYMENT) << vchAlias << vchFromString("1") << OP_DROP << OP_2DROP;
	scriptChangeOrig += scriptPubKeyDest;
	CRecipient recp = {scriptChangeOrig, 0, false};
	recipient = recp;
	int nFeePerByte = getFeePerByte(aliasPegTuple, vchFromString("SYS"), precision);
	CTxOut txout(0,	recipient.scriptPubKey);
	size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	// create alias payment utxo max 1500 bytes worth of fees
	// create utxo min 1500 bytes worth of fees
	if(nSize < 1500)
		nSize = 1500;
	if(nFeePerByte <= 0)
		nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	else
		nFee = nFeePerByte * nSize;
	recipient.nAmount = nFee;
}
void CreateFeeRecipient(CScript& scriptPubKey, const CNameTXIDTuple& aliasPegTuple, const vector<unsigned char>& data, CRecipient& recipient)
{
	int precision = 0;
	CAmount nFee = 0;
	// add hash to data output (must match hash in inputs check with the tx scriptpubkey hash)
    uint256 hash = Hash(data.begin(), data.end());
    vector<unsigned char> vchHashRand = vchFromValue(hash.GetHex());
	scriptPubKey << vchHashRand;
	CRecipient recp = {scriptPubKey, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
	size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	int nFeePerByte = getFeePerByte(aliasPegTuple, vchFromString("SYS"), precision);
	if(nFeePerByte <= 0)
		nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	else
		nFee = nFeePerByte * nSize;

	recipient.nAmount = nFee;
}
CAmount GetDataFee(const CScript& scriptPubKey, const CNameTXIDTuple& aliasPegTuple)
{
	int precision = 0;
	CAmount nFee = 0;
	CRecipient recipient;
	CRecipient recp = {scriptPubKey, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
    size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	int nFeePerByte = getFeePerByte(aliasPegTuple, vchFromString("SYS"), precision);
	if(nFeePerByte <= 0)
		nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	else
		nFee = nFeePerByte * nSize;

	recipient.nAmount = nFee;
	return recipient.nAmount;
}
bool CheckParam(const UniValue& params, const unsigned int index)
{
	if(params.size() > index)
	{
		if(params[index].isStr())
		{
			if( params[index].get_str().size() > 0 && params[index].get_str() != "\"\"")
				return true;
		}
		else if(params[index].isArray())
			return params[index].get_array().size() > 0;
	}
	return false;
}
void startMongoDB(){
	// SYSCOIN
	nIndexPort = GetArg("-indexport", DEFAULT_INDEXPORT);
	string uri_str = strprintf("mongodb://localhost:%d", nIndexPort);
	if (fDebug)
		LogPrintf("Detecting the start of mongo indexer: ");
	if (nIndexPort > 0) {
		if (fDebug)
			LogPrintf("Starting mongo c client\n");
		mongoc_init();
		client = mongoc_client_new(uri_str.c_str());
		mongoc_client_set_appname(client, "syscoin-indexer");

		database = mongoc_client_get_database(client, "syscoindb");
		alias_collection = mongoc_client_get_collection(client, "syscoindb", "alias");
		offer_collection = mongoc_client_get_collection(client, "syscoindb", "offer");
		escrow_collection = mongoc_client_get_collection(client, "syscoindb", "escrow");
		cert_collection = mongoc_client_get_collection(client, "syscoindb", "cert");
		feedback_collection = mongoc_client_get_collection(client, "syscoindb", "feedback");
		BSON_ASSERT(alias_collection);
		BSON_ASSERT(offer_collection);
		BSON_ASSERT(escrow_collection);
		BSON_ASSERT(cert_collection);
		BSON_ASSERT(feedback_collection);
		LogPrintf("Mongo c client loaded!\n");
	}
	else
	{
		if (fDebug)
			LogPrintf("Could not detect mongo db index port, please ensure you specify -indexport in your syscoin.conf file or as a command line argument\n");
	}
}
void CAliasDB::WriteAliasIndex(const CAliasIndex& alias) {
	if (!alias_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(alias.vchAlias).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (BuildAliasJson(alias, oName)) {
		update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
		if (!update || !mongoc_collection_update(alias_collection, update_flags, selector, update, write_concern, &error)) {
			LogPrintf("MONGODB ALIAS UPDATE ERROR: %s\n", error.message);
		}
	}
	if(update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAliasDB::EraseAliasIndex(const std::vector<unsigned char>& vchAlias) {
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(vchAlias).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(alias_collection, remove_flags, selector, write_concern, &error)) {
		LogPrintf("MONGODB ALIAS REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void stopMongoDB() {
	/*
	* Release our handles and clean up libmongoc
	*/
	if (fDebug)
		LogPrintf("Shutting down mongo indexer...\n");
	if(alias_collection)
		mongoc_collection_destroy(alias_collection);
	if(offer_collection)
		mongoc_collection_destroy(offer_collection);
	if(escrow_collection)
		mongoc_collection_destroy(escrow_collection);
	if(cert_collection)
		mongoc_collection_destroy(cert_collection);
	if(feedback_collection)
		mongoc_collection_destroy(feedback_collection);
	if(database)
		mongoc_database_destroy(database);
	if (client) {
		mongoc_client_destroy(client);
		mongoc_cleanup();
	}
}
UniValue syscoinquery(const UniValue& params, bool fHelp) {
	if (fHelp || 2 > params.size() || 3 < params.size())
		throw runtime_error(
			"syscoinquery <collection> <query> [options]\n"
			"<collection> Collection name, either: 'alias', 'cert', 'offer', 'feedback', 'escrow'.\n"
			"<query> JSON query on the collection to retrieve a set of documents.\n"
			"<options> JSON option arguments into the query. Based on mongoc_collection_find_with_opts.\n"
			+ HelpRequiringPassphrase());
	string collection = params[0].get_str();
	string query = params[1].get_str();
	string options;
	if (CheckParam(params, 2))
		options = params[2].get_str();
	
	bson_t *filter = NULL;
	bson_t *opts = NULL;
	mongoc_cursor_t *cursor;
	bson_error_t error;
	const bson_t *doc;
	char *str;
	mongoc_collection_t *selectedCollection;
	filter = bson_new_from_json((const uint8_t *)query.c_str(), -1, &error);
	if (!filter) {
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5505 - " + boost::lexical_cast<string>(error.message));
	}
	if (options.size() > 0) {
		opts = bson_new_from_json((const uint8_t *)options.c_str(), -1, &error);
		if (!opts) {
			bson_destroy(filter);
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5506 - " + boost::lexical_cast<string>(error.message));
		}
	}
	if (collection == "alias")
		selectedCollection = alias_collection;
	else if(collection == "cert")
		selectedCollection = cert_collection;
	else if (collection == "offer")
		selectedCollection = offer_collection;
	else if (collection == "escrow")
		selectedCollection = escrow_collection;
	else if (collection == "feedback")
		selectedCollection = feedback_collection;
	else
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("Invalid selection collection name, please specify the collection parameter as either 'alias', 'cert', 'offer', 'feedback' or 'escrow'"));

	cursor = mongoc_collection_find_with_opts(selectedCollection, filter, opts, NULL);
	UniValue res(UniValue::VARR);
	while (mongoc_cursor_next(cursor, &doc)) {
		str = bson_as_relaxed_extended_json(doc, NULL);
		res.push_back(str);
		bson_free(str);
	}

	if (mongoc_cursor_error(cursor, &error)) {
		mongoc_cursor_destroy(cursor);
		bson_destroy(filter);
		if(opts)
			bson_destroy(opts);
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5506 - " + boost::lexical_cast<string>(error.message));
	}

	mongoc_cursor_destroy(cursor);
	bson_destroy(filter);
	if (opts)
		bson_destroy(opts);
	return res;
}
UniValue aliasnew(const UniValue& params, bool fHelp) {
	if (fHelp || 2 > params.size() || 11 < params.size())
		throw runtime_error(
		"aliasnew <aliaspeg> <aliasname> <public value> [private value] [accept transfers=true] [expire_timestamp] [address] [password_salt] [encryption_privatekey] [encryption_publickey] [witness]\n"
						"<aliasname> alias name.\n"
						"<public value> alias public profile data, 1024 chars max.\n"
						"<private value> alias private profile data, 1024 chars max. Will be private and readable by anyone with encryption_privatekey. Should be encrypted to encryption_publickey.\n"
						"<accept transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"	
						"<expire_timestamp> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is FEERATE*(2.88^years). FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"	
						"<address> Address for this alias.\n"		
						"<password_salt> Salt used for key derivation if password is set.\n"
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. Should be encrypted to publickey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias.\n"						
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"							
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAliasPeg = vchFromString(params[0].get_str());
	if(vchAliasPeg.empty())
		vchAliasPeg = vchFromString("sysrates.peg");
	vector<unsigned char> vchAlias = vchFromString(params[1].get_str());
	string strName = stringFromVch(vchAlias);
	/*Above pattern makes sure domain name matches the following criteria :

	The domain name should be a-z | 0-9 and hyphen(-)
	The domain name should between 3 and 63 characters long
	Last Tld can be 2 to a maximum of 6 characters
	The domain name should not start or end with hyphen (-) (e.g. -syscoin.org or syscoin-.org)
	The domain name can be a subdomain (e.g. sys.blogspot.com)*/

	using namespace boost::xpressive;
	using namespace boost::algorithm;
	to_lower(strName);
	smatch nameparts;
	sregex domainwithtldregex = sregex::compile("^((?!-)[a-z0-9-]{3,64}(?<!-)\\.)+[a-z]{2,6}$");
	sregex domainwithouttldregex = sregex::compile("^((?!-)[a-z0-9-]{3,64}(?<!-))");

	if(find_first(strName, "."))
	{
		if (!regex_search(strName, nameparts, domainwithtldregex) || string(nameparts[0]) != strName)
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5505 - " + _("Invalid Syscoin Identity. Must follow the domain name spec of 3 to 64 characters with no preceding or trailing dashes and a TLD of 2 to 6 characters"));	
	}
	else
	{
		if (!regex_search(strName, nameparts, domainwithouttldregex)  || string(nameparts[0]) != strName)
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5506 - " + _("Invalid Syscoin Identity. Must follow the domain name spec of 3 to 64 characters with no preceding or trailing dashes"));
	}
	


	vchAlias = vchFromString(strName);

	vector<unsigned char> vchPublicValue;
	vector<unsigned char> vchPrivateValue;
	string strPublicValue = "";
	if (CheckParam(params, 2))
		strPublicValue = params[2].get_str();
	vchPublicValue = vchFromString(strPublicValue);

	string strPrivateValue = "";
	if(CheckParam(params, 3))
		strPrivateValue = params[3].get_str();
	string strAcceptCertTransfers = "true";

	if(CheckParam(params, 4))
		strAcceptCertTransfers = params[4].get_str();
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	if(CheckParam(params, 5))
		nTime = boost::lexical_cast<uint64_t>(params[5].get_str());
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;

	string strAddress = "";
	if(CheckParam(params, 6))
		strAddress = params[6].get_str();
	string strPasswordSalt = "";

	if(CheckParam(params, 7))
		strPasswordSalt = params[7].get_str();
	
	string strEncryptionPrivateKey = "";
	if(CheckParam(params, 8))
		strEncryptionPrivateKey = params[8].get_str();
	string strEncryptionPublicKey = "";
	if(CheckParam(params, 9))
		strEncryptionPublicKey = params[9].get_str();
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 10))
		vchWitness = vchFromValue(params[10]);

	CWalletTx wtx;

	CAliasIndex oldAlias;
	if(GetAlias(vchAlias, oldAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("This alias already exists"));

	CAliasIndex pegAlias;
	if (strName != "sysrates.peg" && !GetAlias(vchAliasPeg, pegAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("The alias peg does not exist"));

	const vector<unsigned char> &vchRandAlias = vchFromString(GenerateSyscoinGuid());

    // build alias
    CAliasIndex newAlias;
	newAlias.vchGUID = vchRandAlias;
	newAlias.aliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
	newAlias.vchAlias = vchAlias;
	newAlias.nHeight = chainActive.Tip()->nHeight;
	if(!strEncryptionPublicKey.empty())
		newAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);
	if(!strEncryptionPrivateKey.empty())
		newAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	newAlias.vchPublicValue = vchPublicValue;
	if(!strPrivateValue.empty())
		newAlias.vchPrivateValue = vchFromString(strPrivateValue);
	newAlias.nExpireTime = nTime;

	if(!strPasswordSalt.empty())
		newAlias.vchPasswordSalt = ParseHex(strPasswordSalt);
	newAlias.acceptCertTransfers = strAcceptCertTransfers == "true"? true: false;
	if(strAddress.empty())
	{
		// generate new address in this wallet if not passed in
		CKey privKey;
		privKey.MakeNewKey(true);
		CPubKey pubKey = privKey.GetPubKey();
		vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
		CSyscoinAddress addressAlias(pubKey.GetID());
		strAddress = addressAlias.ToString();
		if (pwalletMain && !pwalletMain->AddKeyPubKey(privKey, pubKey))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("Error adding key to wallet"));
	}
	DecodeBase58(strAddress, newAlias.vchAddress);
	CScript scriptPubKeyOrig;
	

	vector<unsigned char> data;
	vector<unsigned char> vchHashAlias;
	uint256 hash;
	bool bActivation = false;
	if(mapAliasRegistrationData.count(vchAlias) > 0)
	{
		data = mapAliasRegistrationData[vchAlias];
		hash = Hash(data.begin(), data.end());
		vchHashAlias = vchFromValue(hash.GetHex());
		mapAliasRegistrationData.erase(vchAlias);
		if(!newAlias.UnserializeFromData(data, vchHashAlias))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("Cannot unserialize alias registration transaction"));
		bActivation = true;
	}
	else
	{
		newAlias.Serialize(data);
		hash = Hash(data.begin(), data.end());
		vchHashAlias = vchFromValue(hash.GetHex());
		mapAliasRegistrationData.insert(make_pair(vchAlias, data));
	}

	
	CScript scriptPubKey;
	if(bActivation)
		scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_ACTIVATE) << vchAlias << newAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	else
		scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_ACTIVATE) << vchHashAlias << OP_2DROP;

	CSyscoinAddress newAddress;
	GetAddress(newAlias, &newAddress, scriptPubKeyOrig);
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, vchAlias, newAlias.aliasPegTuple, recipientPayment);
	CScript scriptData;
	
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, newAlias.aliasPegTuple, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
	if (nTimeExpiry < 3600)
		nTimeExpiry = 3600;
	float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
	if(fYears < 1)
		fYears = 1;
	fee.nAmount *= powf(2.88,fYears);
	CCoinControl coinControl;
	if(bActivation && mapAliasRegistrations.count(vchHashAlias) > 0)
	{
		vecSend.push_back(fee);
		// add the registration input to the alias activation transaction
		coinControl.Select(mapAliasRegistrations[vchHashAlias]);
	}
	coinControl.fAllowOtherInputs = true;
	coinControl.fAllowWatchOnly = true;
	bool useOnlyAliasPaymentToFund = false;

	SendMoneySyscoin(vchAlias, vchWitness, newAlias.aliasPegTuple.first, "", recipient, recipientPayment, vecSend, wtx, &coinControl, useOnlyAliasPaymentToFund);
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
		res.push_back(strAddress);
	}
	else
	{
		res.push_back(hex_str);
		res.push_back(strAddress);
		res.push_back("false");
	}
	return res;
}
UniValue aliasupdate(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size() || 10 < params.size())
		throw runtime_error(
		"aliasupdate <aliasname> [public value] [private value] [address] [accept_transfers=true] [expire_timestamp] [password_salt] [encryption_privatekey] [encryption_publickey] [witness]\n"
						"Update and possibly transfer an alias.\n"
						"<aliasname> alias name.\n"
						"<public_value> alias public profile data, 1024 chars max.\n"
						"<private_value> alias private profile data, 1024 chars max. Will be private and readable by anyone with encryption_privatekey.\n"				
						"<address> Address of alias.\n"		
						"<accept_transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"		
						"<expire_timestamp> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is 2.88^years. FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"		
						"<password_salt> Salt used for key derivation if password is set.\n"
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. If transferring, the key should be encrypted to alias_pubkey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias. Useful if you are changing pub/priv keypair for encryption on this alias.\n"						
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromString(params[0].get_str());
	string strPrivateValue = "";
	string strPublicValue = "";
	if(CheckParam(params, 1))
		strPublicValue = params[1].get_str();
	
	if(CheckParam(params, 2))
		strPrivateValue = params[2].get_str();
	CWalletTx wtx;
	CAliasIndex updateAlias;
	string strAddress = "";
	if(CheckParam(params,3))
		strAddress = params[3].get_str();
	
	string strAcceptCertTransfers = "";
	if(CheckParam(params, 4))
		strAcceptCertTransfers = params[4].get_str();
	
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	bool timeSet = false;
	if(CheckParam(params, 5))
	{
		nTime = boost::lexical_cast<uint64_t>(params[5].get_str());
		timeSet = true;
	}
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;

	string strPasswordSalt = "";
	if(CheckParam(params, 6))
		strPasswordSalt = params[6].get_str();
	

	string strEncryptionPrivateKey = "";
	if(CheckParam(params, 7))
		strEncryptionPrivateKey = params[7].get_str();
	
	string strEncryptionPublicKey = "";
	if(CheckParam(params, 8))
		strEncryptionPublicKey = params[8].get_str();
	
	vector<unsigned char> vchWitness;
	if(CheckParam(params, 9))
		vchWitness = vchFromValue(params[9]);

	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5518 - " + _("Could not find an alias with this name"));

	CAliasIndex pegAlias;
	if (vchAlias != vchFromString("sysrates.peg") && !GetAlias(theAlias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("The alias peg does not exist"));

	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	theAlias.nHeight = chainActive.Tip()->nHeight;
	theAlias.aliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
	if(!strPublicValue.empty())
		theAlias.vchPublicValue = vchFromString(strPublicValue);
	if(!strPrivateValue.empty())
		theAlias.vchPrivateValue = vchFromString(strPrivateValue);
	if(!strEncryptionPrivateKey.empty())
		theAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	if(!strEncryptionPublicKey.empty())
		theAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);

	if(!strPasswordSalt.empty())
		theAlias.vchPasswordSalt = ParseHex(strPasswordSalt);
	if(!strAddress.empty())
		DecodeBase58(strAddress, theAlias.vchAddress);
	if(timeSet || copyAlias.nExpireTime <= chainActive.Tip()->nTime)
		theAlias.nExpireTime = nTime;
	else
		theAlias.nExpireTime = copyAlias.nExpireTime;
	theAlias.nAccessFlags = copyAlias.nAccessFlags;
	if(strAcceptCertTransfers.empty())
		theAlias.acceptCertTransfers = copyAlias.acceptCertTransfers;
	else
		theAlias.acceptCertTransfers = strAcceptCertTransfers == "true"? true: false;
	
	CSyscoinAddress newAddress;
	CScript scriptPubKeyOrig;
	if(theAlias.vchAddress.empty())
		GetAddress(copyAlias, &newAddress, scriptPubKeyOrig);
	else
		GetAddress(theAlias, &newAddress, scriptPubKeyOrig);

	vector<unsigned char> data;
	theAlias.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
    vector<unsigned char> vchHashAlias = vchFromValue(hash.GetHex());

	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, copyAlias.vchAlias, copyAlias.aliasPegTuple, recipientPayment);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, copyAlias.aliasPegTuple, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
	if (nTimeExpiry < 3600)
		nTimeExpiry = 3600;
	float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
	if(fYears < 1)
		fYears = 1;
	fee.nAmount *= powf(2.88,fYears);
	
	vecSend.push_back(fee);
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	bool useOnlyAliasPaymentToFund = true;
	bool transferAlias = false;
	if(newAddress.ToString() != EncodeBase58(copyAlias.vchAddress))
		transferAlias = true;
	
	SendMoneySyscoin(vchAlias, vchWitness, copyAlias.aliasPegTuple.first, "", recipient, recipientPayment, vecSend, wtx, &coinControl, useOnlyAliasPaymentToFund, transferAlias);
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
UniValue syscoindecoderawtransaction(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("syscoindecoderawtransaction <hexstring>\n"
		"Decode raw syscoin transaction (serialized, hex-encoded) and display information pertaining to the service that is included in the transactiion data output(OP_RETURN)\n"
				"<hexstring> The transaction hex string.\n");
	string hexstring = params[0].get_str();
	CTransaction rawTx;
	DecodeHexTx(rawTx,hexstring);
	if(rawTx.IsNull())
	{
		throw runtime_error("SYSCOIN_RPC_ERROR: ERRCODE: 5531 - " + _("Could not decode transaction"));
	}
	vector<unsigned char> vchData;
	int nOut;
	int op;
	vector<vector<unsigned char> > vvch;
	vector<unsigned char> vchHash;
	GetSyscoinData(rawTx, vchData, vchHash, nOut);	
	UniValue output(UniValue::VOBJ);
	if(DecodeAndParseSyscoinTx(rawTx, op, nOut, vvch))
		SysTxToJSON(op, vchData, vchHash, output);
	
	bool sendCoin = false;
	for (unsigned int i = 0; i < rawTx.vout.size(); i++) {
		int tmpOp;
		vector<vector<unsigned char> > tmpvvch;	
		if(!IsSyscoinDataOutput(rawTx.vout[i]) && (!IsSyscoinScript(rawTx.vout[i].scriptPubKey, tmpOp, tmpvvch) || tmpOp == OP_ALIAS_PAYMENT))
		{
			if(!pwalletMain->IsMine(rawTx.vout[i]))
			{
				sendCoin = true;
				break;
			}
		}

	}
	if(sendCoin)
		output.push_back(Pair("warning", _("Warning: This transaction sends coins to an address or alias you do not own")));
	else
		output.push_back(Pair("warning", ""));
	return output;
}
void SysTxToJSON(const int op, const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash, UniValue &entry)
{
	if(IsAliasOp(op))
		AliasTxToJSON(op, vchData, vchHash, entry);
	if(IsCertOp(op))
		CertTxToJSON(op, vchData, vchHash, entry);
	if(IsEscrowOp(op))
		EscrowTxToJSON(op, vchData, vchHash, entry);
	if(IsOfferOp(op))
		OfferTxToJSON(op, vchData, vchHash, entry);
}
void AliasTxToJSON(const int op, const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = aliasFromOp(op);
	CAliasIndex alias;
	if(!alias.UnserializeFromData(vchData, vchHash))
		return;
	CAliasIndex dbAlias;
	GetAlias(CNameTXIDTuple(alias.vchAlias, alias.txHash), dbAlias);

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(alias.vchAlias)));

	if(!alias.aliasPegTuple.first.empty() && alias.aliasPegTuple != dbAlias.aliasPegTuple)
		entry.push_back(Pair("aliaspeg", stringFromVch(alias.aliasPegTuple.first)));

	if(!alias.vchPublicValue .empty() && alias.vchPublicValue != dbAlias.vchPublicValue)
		entry.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));
	
	if(!alias.vchPrivateValue.empty() && alias.vchPrivateValue != dbAlias.vchPrivateValue)
		entry.push_back(Pair("privatevalue", stringFromVch(alias.vchPrivateValue)));

	if(EncodeBase58(alias.vchAddress) != EncodeBase58(dbAlias.vchAddress))
		entry.push_back(Pair("address", EncodeBase58(alias.vchAddress))); 

	if(alias.nExpireTime != dbAlias.nExpireTime)
		entry.push_back(Pair("renewal", alias.nExpireTime));

}
UniValue syscoinsignrawtransaction(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("syscoinsignrawtransaction <hexstring>\n"
				"Sign inputs for raw transaction (serialized, hex-encoded) and sends them out to the network if signing is complete\n"
				"<hexstring> The transaction hex string.\n");
	const string &hexstring = params[0].get_str();
	UniValue res;
	UniValue arraySignParams(UniValue::VARR);
	arraySignParams.push_back(hexstring);
	try
	{
		res = tableRPC.execute("signrawtransaction", arraySignParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5532 - " + _("Could not sign raw transaction: ") + find_value(objError, "message").get_str());
	}	
	if (!res.isObject())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5533 - " + _("Could not sign raw transaction: Invalid response from signrawtransaction"));
	
	const UniValue& so = res.get_obj();
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
		UniValue arraySendParams(UniValue::VARR);
		arraySendParams.push_back(hex_str);
		UniValue returnRes;
		try
		{
			returnRes = tableRPC.execute("sendrawtransaction", arraySendParams);
		}
		catch (UniValue& objError)
		{
			throw runtime_error(find_value(objError, "message").get_str());
		}
		if (!returnRes.isStr())
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5534 - " + _("Could not send raw transaction: Invalid response from sendrawtransaction"));
		res.push_back(Pair("txid", returnRes.get_str()));
		// check for alias registration, if so save the info in this node for alias activation calls after a block confirmation
		CTransaction tx;
		if(!DecodeHexTx(tx,hex_str))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5534 - " + _("Could not send raw transaction: Cannot decode transaction from hex string"));
		vector<vector<unsigned char> > vvch;
		int op;
		for (unsigned int i = 0; i < tx.vout.size(); i++) {
			const CTxOut& out = tx.vout[i];
			if (DecodeAliasScript(out.scriptPubKey, op, vvch) && op == OP_ALIAS_ACTIVATE) 
			{
				if(vvch.size() == 1)
				{
					if(!mapAliasRegistrations.count(vvch[0]))
						mapAliasRegistrations.insert(make_pair(vvch[0], COutPoint(tx.GetHash(), i)));
				}
				else if(vvch.size() >= 3)
				{
					if(mapAliasRegistrations.count(vvch[2]) > 0)
						mapAliasRegistrations.erase(vvch[2]);
					if(mapAliasRegistrationData.count(vvch[0]) > 0)
						mapAliasRegistrationData.erase(vvch[0]);
				}
				break;
			}
		}
	}
	return res;
}
bool IsMyAlias(const CAliasIndex& alias)
{
	CSyscoinAddress address(EncodeBase58(alias.vchAddress));
	return IsMine(*pwalletMain, address.Get());
}
string GenerateSyscoinGuid()
{
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchGuidRand = CScriptNum(rand).getvch();
	return HexStr(vchGuidRand);
}
UniValue aliasbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "aliasbalance \"alias\"\n"
            "\nReturns the total amount received by the given alias in transactions with at least 1 confirmation.\n"
            "\nArguments:\n"
            "1. \"alias\"  (string, required) The syscoin alias for transactions.\n"
       );
	LOCK(cs_main);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CAmount nAmount = 0;
	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
	{
		UniValue res(UniValue::VOBJ);
		res.push_back(Pair("balance", ValueFromAmount(nAmount)));
		return  res;
	}

	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);
	UniValue paramsUTXO(UniValue::VARR);
	UniValue param(UniValue::VOBJ);
	UniValue utxoParams(UniValue::VARR);
	utxoParams.push_back(strAddressFrom);
	param.push_back(Pair("addresses", utxoParams));
	paramsUTXO.push_back(param);
	const UniValue &resUTXOs = tableRPC.execute("getaddressutxos", paramsUTXO);
	UniValue utxoArray(UniValue::VARR);
	if (resUTXOs.isArray())
		utxoArray = resUTXOs.get_array();
	else
	{
		UniValue res(UniValue::VOBJ);
		res.push_back(Pair("balance", ValueFromAmount(nAmount)));
		return  res;
	}

	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
  	int op;
	vector<vector<unsigned char> > vvch;
    for (unsigned int i = 0;i<utxoArray.size();i++)
    {
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj.get_obj(), "txid").get_str());
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		coins = view.AccessCoins(txid);
		if(coins == NULL)
			continue; 
		if(!coins->IsAvailable(nOut))
			continue;
		if (!ExtractDestination(coins->vout[nOut].scriptPubKey, payDest)) 
			continue;
		if(DecodeAliasScript(coins->vout[nOut].scriptPubKey, op, vvch) && vvch[0] != theAlias.vchAlias)
			continue;  
		// some outputs are reserved to pay for fees only
		if(vvch.size() > 1 && vvch[1] == vchFromString("1"))
			continue;
		destaddy = CSyscoinAddress(payDest);
		if (destaddy.ToString() == strAddressFrom)
		{  
			COutPoint outp(txid, nOut);
			auto it = mempool.mapNextTx.find(outp);
			if (it != mempool.mapNextTx.end())
				continue;
			nAmount += coins->vout[nOut].nValue;
		}		
		
    }
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("balance", ValueFromAmount(nAmount)));
    return  res;
}
int aliasselectpaymentcoins(const vector<unsigned char> &vchAlias, const CAmount &nAmount, vector<COutPoint>& outPoints, bool& bIsFunded, CAmount &nRequiredAmount, bool bSelectFeePlacementOnly, bool bSelectAll, bool bNoAliasRecipient)
{
	LOCK2(cs_main, mempool.cs);
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	int numCoinsLeft = 0;
	int numResults = 0;
	CAmount nCurrentAmount = 0;
	CAmount nDesiredAmount = nAmount;
	outPoints.clear();
	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		return -1;

	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);
	UniValue paramsUTXO(UniValue::VARR);
	UniValue param(UniValue::VOBJ);
	UniValue utxoParams(UniValue::VARR);
	utxoParams.push_back(strAddressFrom);
	param.push_back(Pair("addresses", utxoParams));
	paramsUTXO.push_back(param);
	const UniValue &resUTXOs = tableRPC.execute("getaddressutxos", paramsUTXO);
	UniValue utxoArray(UniValue::VARR);
	if (resUTXOs.isArray())
		utxoArray = resUTXOs.get_array();
	else
		return -1;
	
  	int op;
	int unspentUTXOs = 0;
	vector<vector<unsigned char> > vvch;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
	bIsFunded = false;
	for (unsigned int i = 0; i<utxoArray.size(); i++)
	{
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj.get_obj(), "txid").get_str());
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		coins = view.AccessCoins(txid);
		if(coins == NULL)
			continue;
   
		if(!coins->IsAvailable(nOut))
			continue;
		if (DecodeAliasScript(coins->vout[nOut].scriptPubKey, op, vvch) && vvch[0] != theAlias.vchAlias)
			continue;  
		if(vvch.size() > 1)
		{
			// if no alias recipient was used to create another UTXO
			// and if not selecting all inputs, ensure you leave atleast one alias UTXO for next alias update
			if (bNoAliasRecipient && !bSelectAll && IsAliasOp(op, true)) {
				unspentUTXOs++;
				if (unspentUTXOs >= MAX_ALIAS_UPDATES_PER_BLOCK) {
					continue;
				}
			}
			if(vvch[1] == vchFromString("1"))
			{
				if(!bSelectFeePlacementOnly)
					continue;
			}
			else if(bSelectFeePlacementOnly)
				continue;
		}
		else if(bSelectFeePlacementOnly)
			continue;
		if (!ExtractDestination(coins->vout[nOut].scriptPubKey, payDest)) 
			continue;
		destaddy = CSyscoinAddress(payDest);
		if (destaddy.ToString() == strAddressFrom)
		{  
			auto it = mempool.mapNextTx.find(COutPoint(txid, nOut));
			if (it != mempool.mapNextTx.end())
				continue;
			numResults++;
			if(!bIsFunded || bSelectAll)
			{
				outPoints.push_back(COutPoint(txid, nOut));
				nCurrentAmount += coins->vout[nOut].nValue;
				if(nCurrentAmount >= nDesiredAmount)
					bIsFunded = true;
			}
		}		
    }
	nRequiredAmount = nDesiredAmount - nCurrentAmount;
	if(nRequiredAmount < 0)
		nRequiredAmount = 0;
	numCoinsLeft = numResults - (int)outPoints.size();
	return numCoinsLeft;
}
int aliasunspent(const vector<unsigned char> &vchAlias, COutPoint& outpoint)
{
	LOCK2(cs_main, mempool.cs);
	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		return 0;
	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);
	UniValue paramsUTXO(UniValue::VARR);
	UniValue param(UniValue::VOBJ);
	UniValue utxoParams(UniValue::VARR);
	utxoParams.push_back(strAddressFrom);
	param.push_back(Pair("addresses", utxoParams));
	paramsUTXO.push_back(param);
	const UniValue &resUTXOs = tableRPC.execute("getaddressutxos", paramsUTXO);
	UniValue utxoArray(UniValue::VARR);
	if (resUTXOs.isArray())
		utxoArray = resUTXOs.get_array();
	else
		return 0;
	CTxDestination aliasDest;
	CSyscoinAddress prevaddy;
	int numResults = 0;
	CAmount nCurrentAmount = 0;
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	bool funded = false;
	for (unsigned int i = 0; i<utxoArray.size(); i++)
	{
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj.get_obj(), "txid").get_str());
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		coins = view.AccessCoins(txid);

		if(coins == NULL)
			continue;
		int op;
		vector<vector<unsigned char> > vvch;

		if (!coins->IsAvailable(nOut))
			continue;
		if (!DecodeAliasScript(coins->vout[nOut].scriptPubKey, op, vvch) || vvch[0] != theAlias.vchAlias || vvch[1] != theAlias.vchGUID || !IsAliasOp(op, true))
			continue;
		if (!ExtractDestination(coins->vout[nOut].scriptPubKey, aliasDest))
			continue;
		prevaddy = CSyscoinAddress(aliasDest);
		if (strAddressFrom != prevaddy.ToString())
			continue;

		numResults++;
		if (!funded)
		{
			auto it = mempool.mapNextTx.find(COutPoint(txid, nOut));
			if (it != mempool.mapNextTx.end())
				continue;
			outpoint = COutPoint(txid, nOut);
			funded = true;
		}
    }
	return numResults;
}
/**
 * [aliasinfo description]
 * @param  params [description]
 * @param  fHelp  [description]
 * @return        [description]
 */
UniValue aliasinfo(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size())
		throw runtime_error("aliasinfo <aliasname>\n"
				"Show values of an alias.\n");
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CAliasIndex txPos;
	uint256 txid;
	if (!paliasdb || !paliasdb->ReadAliasLastTXID(vchAlias, txid))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from alias DB"));
	if (!paliasdb->ReadAlias(CNameTXIDTuple(vchAlias, txid), txPos))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from alias DB"));

	CTransaction tx;
	if (!GetSyscoinTransaction(txPos.nHeight, txPos.txHash, tx, Params().GetConsensus()))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to read from alias tx"));
	vector<vector<unsigned char> > vvch;
	int op, nOut;
	if (!DecodeAliasTx(tx, op, nOut, vvch))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 4604 - " + _("Failed to decode alias"));

	UniValue oName(UniValue::VOBJ);
	if(!BuildAliasJson(txPos, oName))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5536 - " + _("Could not find this alias"));
		
	return oName;
}
bool BuildAliasJson(const CAliasIndex& alias, UniValue& oName)
{
	bool expired = false;
	int64_t expired_time = 0;
	oName.push_back(Pair("_id", stringFromVch(alias.vchAlias)));
	oName.push_back(Pair("passwordsalt", HexStr(alias.vchPasswordSalt)));
	oName.push_back(Pair("encryption_privatekey", HexStr(alias.vchEncryptionPrivateKey)));
	oName.push_back(Pair("encryption_publickey", HexStr(alias.vchEncryptionPublicKey)));
	oName.push_back(Pair("alias_peg", stringFromVch(alias.aliasPegTuple.first)));
	oName.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));	
	oName.push_back(Pair("privatevalue", stringFromVch(alias.vchPrivateValue)));
	oName.push_back(Pair("txid", alias.txHash.GetHex()));
	int64_t nTime = 0;
	if (chainActive.Height() >= alias.nHeight) {
		CBlockIndex *pindex = chainActive[alias.nHeight];
		if (pindex) {
			nTime = pindex->nTime;
		}
	}
	oName.push_back(Pair("time", nTime));
	oName.push_back(Pair("address", EncodeBase58(alias.vchAddress)));
	oName.push_back(Pair("acceptcerttransfers", alias.acceptCertTransfers));
	if(alias.vchAlias != vchFromString("sysrates.peg"))
	{
		expired_time = alias.nExpireTime;
		if(expired_time <= chainActive.Tip()->nTime)
		{
			expired = true;
		}  
	}
	else
	{
		expired = false;
		expired_time = -1;
	}
	oName.push_back(Pair("expires_on", expired_time));
	oName.push_back(Pair("expired", expired));
	return true;
}


UniValue aliaspay(const UniValue& params, bool fHelp) {

    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "aliaspay aliasfrom currency {\"address\":amount,...} ( minconf \"comment\")\n"
            "\nSend multiple times from an alias. Amounts are double-precision floating point numbers."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
			"1. \"aliasfrom\"			(string, required) Alias to pay from\n"
			"2. \"currency\"			(string, required) Currency to pay from, must be valid in the rates peg in aliasfrom\n"
            "3. \"amounts\"             (string, required) A json object with aliases and amounts\n"
            "    {\n"
            "      \"address\":amount   (numeric or string) The syscoin alias is the key, the numeric amount (can be string) in " + CURRENCY_UNIT + " is the value\n"
            "      ,...\n"
            "    }\n"
			"4. minconf                 (numeric, optional, default=1) Only use the balance confirmed at least this many times.\n"
            "5. \"comment\"             (string, optional) A comment\n"
            "\nResult:\n"
            "\"transactionid\"          (string) The transaction id for the send. Only 1 transaction is created regardless of \n"
            "                                    the number of addresses.\n"
            "\nExamples:\n"
            "\nSend two amounts to two different address or aliases:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"USD\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"alias2\\\":0.02}\"") +
            "\nSend two amounts to two different address or aliases while also adding a comment:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"USD\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\" \"testing\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    string strFrom = params[0].get_str();
	CAliasIndex theAlias;
	if (!GetAlias(vchFromString(strFrom), theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Invalid fromalias"));
	CAliasIndex pegAlias;
	if (!GetAlias(theAlias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("The alias peg does not exist"));
	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
    string strCurrency = params[1].get_str();
    UniValue sendTo = params[2].get_obj();
    int nMinDepth = 1;
    if(CheckParam(params, 3))
        nMinDepth = params[3].get_int();
    CWalletTx wtx;
    if(CheckParam(params, 4))
        wtx.mapValue["comment"] = params[4].get_str();

    set<CSyscoinAddress> setAddress;
    vector<CRecipient> vecSend;

    CAmount totalAmount = 0;
    vector<string> keys = sendTo.getKeys();
    BOOST_FOREACH(const string& name_, keys)
    {
        CSyscoinAddress address(name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Syscoin address: ")+name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+name_);
        setAddress.insert(address);
        CScript scriptPubKey = GetScriptForDestination(address.Get());
        if (sendTo[name_].get_real() <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
		int precision = 2;
		CAmount nPricePerUnit = convertCurrencyCodeToSyscoin(latestAliasPegTuple, vchFromString(strCurrency), sendTo[name_].get_real(), precision);
        totalAmount += nPricePerUnit;
        CRecipient recipient = {scriptPubKey, nPricePerUnit, false};
        vecSend.push_back(recipient);
    }

    EnsureWalletIsUnlocked();
    // Check funds
	UniValue balanceParams(UniValue::VARR);
	balanceParams.push_back(strFrom);
	const UniValue &resBalance = tableRPC.execute("aliasbalance", balanceParams);
	CAmount nBalance = AmountFromValue(find_value(resBalance.get_obj(), "balance"));
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Alias has insufficient funds");

	CRecipient recipient, recipientPayment;
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	CScript scriptPubKeyOrig;
	CSyscoinAddress addressAlias;
	GetAddress(theAlias, &addressAlias, scriptPubKeyOrig);
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.aliasPegTuple, recipientPayment);	
	SendMoneySyscoin(theAlias.vchAlias, vchFromString(""), theAlias.aliasPegTuple.first, strCurrency, recipient, recipientPayment, vecSend, wtx, &coinControl);
	
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
UniValue aliasdecodemultisigredeemscript(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("decodealiasmultisigredeemscript redeemscript\n"
				"decodes an alias redeemscript and returns required signatures and aliases in the multisig redeem script if the type of redeemscript is TX_MULTISIG.\n");
	string strRedeemScript = params[0].get_str();
	
	vector<vector<unsigned char> > vSolutions;
	txnouttype whichType;
	const vector<unsigned char> &vchRedeemScript = ParseHex(strRedeemScript); 
	CScript redeemScript(vchRedeemScript.begin(), vchRedeemScript.end());
	if (!Solver(redeemScript, whichType, vSolutions) || whichType != TX_MULTISIG)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Invalid alias redeem script"));

	UniValue oRedeemScript(UniValue::VOBJ);
	UniValue oRedeemKeys(UniValue::VARR);
    int nRequired = vSolutions.front()[0];
    for (unsigned int i = 1; i < vSolutions.size()-1; i++)
    {
        CPubKey pubKey(vSolutions[i]);
        if (!pubKey.IsValid())
            continue;
		CSyscoinAddress address(pubKey.GetID());
		address = CSyscoinAddress(address.ToString());
		if(address.IsValid() && address.isAlias)
			oRedeemKeys.push_back(address.aliasName);		
    }
	oRedeemScript.push_back(Pair("reqsigs", (int)nRequired));
	oRedeemScript.push_back(Pair("reqsigners", oRedeemKeys));
	return oRedeemScript;
}
UniValue aliasaddscript(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("aliasaddscript redeemscript\n"
				"Add redeemscript to local wallet for signing smart contract based alias transactions.\n");
	std::vector<unsigned char> data(ParseHex(params[0].get_str()));
	if(pwalletMain)
		pwalletMain->AddCScript(CScript(data.begin(), data.end()));
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("result", "success"));
	return res;
}
UniValue aliasconvertcurrency(const UniValue& params, bool fHelp) {
	if (fHelp || 4 != params.size())
		throw runtime_error("aliasconvertcurrency alias currencyFrom currencyTo amountCurrencyFrom\n"
				"Convert from a currency to another currency amount using the rates peg of an alias. Both currencies need to be defined in the rates peg.\n");
	string alias = params[0].get_str();
	vector<unsigned char> vchCurrencyFrom = vchFromValue(params[1]);
	vector<unsigned char> vchCurrencyTo = vchFromValue(params[2]);
	double fCurrencyValue =  boost::lexical_cast<double>(params[3].get_str());
	CAliasIndex theAlias;
	if (!GetAlias(vchFromString(alias), theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Invalid alias"));
	CAliasIndex pegAlias;
	if (!GetAlias(theAlias.aliasPegTuple.first, pegAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Invalid peg rates alias"));
	const CNameTXIDTuple &latestAliasPegTuple = CNameTXIDTuple(pegAlias.vchAlias, pegAlias.txHash);
	int precision;
	
	CAmount nTotalFrom = convertCurrencyCodeToSyscoin(latestAliasPegTuple, vchCurrencyFrom, fCurrencyValue, precision);
	CAmount nTotalTo = nTotalFrom;
	// don't need extra conversion if attempting to convert to SYS
	if(vchCurrencyTo != vchFromString("SYS"))
		nTotalTo = convertSyscoinToCurrencyCode(latestAliasPegTuple, vchCurrencyTo, nTotalFrom, precision);
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("convertedrate", strprintf("%.*f", precision, ValueFromAmount(nTotalTo).get_real())));
	return res;
}
