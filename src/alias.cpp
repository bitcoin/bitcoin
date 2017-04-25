// Copyright (c) 2014 Syscoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
//
#include "alias.h"
#include "offer.h"
#include "escrow.h"
#include "message.h"
#include "cert.h"
#include "offer.h"
#include "init.h"
#include "main.h"
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
using namespace std;
CAliasDB *paliasdb = NULL;
COfferDB *pofferdb = NULL;
CCertDB *pcertdb = NULL;
CEscrowDB *pescrowdb = NULL;
CMessageDB *pmessagedb = NULL;
extern CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);
extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const CWalletTx* wtxInAlias=NULL, int nTxOutAlias = 0, bool syscoinMultiSigTx=false, const CCoinControl* coinControl=NULL, const CWalletTx* wtxInLinkAlias=NULL,  int nTxOutLinkAlias = 0);
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
	CMessage message;
	CEscrow escrow;
	CCert cert;
	nTime = 0;
	if(alias.UnserializeFromData(vchData, vchHash))
	{
		if(alias.vchAlias == vchFromString("sysrates.peg") || alias.vchAlias == vchFromString("sysban") || alias.vchAlias == vchFromString("syscategory"))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->nTime + 1;
			return true;
		}
		CAliasUnprunable aliasUnprunable;
		// we only prune things that we have in our db and that we can verify the last tx is expired
		// nHeight is set to the height at which data is pruned, if the tip is newer than nHeight it won't send data to other nodes
		if (paliasdb->ReadAliasUnprunable(alias.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		{	
			// if we are renewing alias then prune based on expiry of alias in tx
			if(!alias.vchGUID.empty() && aliasUnprunable.vchGUID != alias.vchGUID)
				nTime = alias.nExpireTime;
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
		nTime = GetOfferExpiration(offer);
		return true; 
	}
	else if(cert.UnserializeFromData(vchData, vchHash))
	{
		nTime = GetCertExpiration(cert);
		return true; 
	}
	else if(escrow.UnserializeFromData(vchData, vchHash))
	{
		nTime = GetEscrowExpiration(escrow);
		return true;
	}
	else if(message.UnserializeFromData(vchData, vchHash))
	{
		nTime = GetMessageExpiration(message);
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
	else if(DecodeMessageScript(scriptPubKey, op, vvchArgs))
		return true;
	else if(DecodeEscrowScript(scriptPubKey, op, vvchArgs))
		return true;
	return false;
}
void RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut)
{
	if (!RemoveAliasScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
		if(!RemoveOfferScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
			if(!RemoveCertScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
				if(!RemoveEscrowScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
					RemoveMessageScriptPrefix(scriptPubKeyIn, scriptPubKeyOut);					
}

// how much is 1.1 BTC in syscoin? 1 BTC = 110000 SYS for example, nPrice would be 1.1, sysPrice would be 110000
CAmount convertCurrencyCodeToSyscoin(const vector<unsigned char> &vchAliasPeg, const vector<unsigned char> &vchCurrencyCode, const double &nPrice, const unsigned int &nHeight, int &precision)
{
	CAmount sysPrice = 0;
	double nRate = 1;
	float fEscrowFee = 0.005;
	int nFeePerByte;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(vchAliasPeg, vchCurrencyCode, nRate, nHeight, rateList, precision, nFeePerByte, fEscrowFee) == "")
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
float getEscrowFee(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const unsigned int &nHeight, int &precision)
{
	double nRate;
	int nFeePerByte =0;
	// 0.05% escrow fee by default it not provided
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(vchAliasPeg, vchCurrencyCode, nRate, nHeight, rateList, precision, nFeePerByte, fEscrowFee) == "")
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
int getFeePerByte(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const unsigned int &nHeight, int &precision)
{
	double nRate;
	int nFeePerByte = 25;
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(vchAliasPeg, vchCurrencyCode, nRate, nHeight, rateList, precision, nFeePerByte, fEscrowFee) == "")
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
CAmount convertSyscoinToCurrencyCode(const vector<unsigned char> &vchAliasPeg, const vector<unsigned char> &vchCurrencyCode, const CAmount &nPrice, const unsigned int &nHeight, int &precision)
{
	CAmount currencyPrice = 0;
	double nRate = 1;
	int nFeePerByte;
	float fEscrowFee = 0.005;
	vector<string> rateList;
	try
	{
		if(getCurrencyToSYSFromAlias(vchAliasPeg, vchCurrencyCode, nRate, nHeight, rateList, precision, nFeePerByte, fEscrowFee) == "")
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
string getCurrencyToSYSFromAlias(const vector<unsigned char> &vchAliasPeg, const vector<unsigned char> &vchCurrency, double &nFee, const unsigned int &nHeightToFind, vector<string>& rateList, int &precision, int &nFeePerByte, float &fEscrowFee)
{
	string currencyCodeToFind = stringFromVch(vchCurrency);
	// check for alias existence in DB
	vector<CAliasIndex> vtxPos;
	CAliasIndex tmpAlias;
	bool isExpired;
	if (!GetVtxOfAlias(vchAliasPeg, tmpAlias, vtxPos, isExpired))
	{
		if(fDebug)
			LogPrintf("getCurrencyToSYSFromAlias() Could not find %s alias\n", stringFromVch(vchAliasPeg).c_str());
		return "1";
	}
	CAliasIndex foundAlias;
	for(unsigned int i=0;i<vtxPos.size();i++) {
        CAliasIndex a = vtxPos[i];
        if(a.nHeight <= nHeightToFind) {
            foundAlias = a;
        }
		else
			break;
    }
	if(foundAlias.IsNull())
		foundAlias = vtxPos.back();


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
			LogPrintf("getCurrencyToSYSFromAlias() currency %s not found in %s alias\n", stringFromVch(vchCurrency).c_str(), stringFromVch(vchAliasPeg).c_str());
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
bool getBanListFromValue(map<string, unsigned char>& banAliasList,  map<string, unsigned char>& banCertList,  map<string, unsigned char>& banOfferList,const UniValue& outerValue)
{
	try
		{
		UniValue outerObj = outerValue.get_obj();
		UniValue objOfferValue = find_value(outerObj, "offers");
		if (objOfferValue.isArray())
		{
			UniValue codes = objOfferValue.get_array();
			for (unsigned int idx = 0; idx < codes.size(); idx++) {
				const UniValue& code = codes[idx];					
				UniValue codeObj = code.get_obj();					
				UniValue idValue = find_value(codeObj, "id");
				UniValue severityValue = find_value(codeObj, "severity");
				if (idValue.isStr() && severityValue.isNum())
				{		
					string idStr = idValue.get_str();
					int severityNum = severityValue.get_int();
					banOfferList.insert(make_pair(idStr, severityNum));
				}
			}
		}

		UniValue objCertValue = find_value(outerObj, "certs");
		if (objCertValue.isArray())
		{
			UniValue codes = objCertValue.get_array();
			for (unsigned int idx = 0; idx < codes.size(); idx++) {
				const UniValue& code = codes[idx];					
				UniValue codeObj = code.get_obj();					
				UniValue idValue = find_value(codeObj, "id");
				UniValue severityValue = find_value(codeObj, "severity");
				if (idValue.isStr() && severityValue.isNum())
				{		
					string idStr = idValue.get_str();
					int severityNum = severityValue.get_int();
					banCertList.insert(make_pair(idStr, severityNum));
				}
			}
		}
			
		

		UniValue objAliasValue = find_value(outerObj, "aliases");
		if (objAliasValue.isArray())
		{
			UniValue codes = objAliasValue.get_array();
			for (unsigned int idx = 0; idx < codes.size(); idx++) {
				const UniValue& code = codes[idx];					
				UniValue codeObj = code.get_obj();					
				UniValue idValue = find_value(codeObj, "id");
				UniValue severityValue = find_value(codeObj, "severity");
				if (idValue.isStr() && severityValue.isNum())
				{		
					string idStr = idValue.get_str();
					int severityNum = severityValue.get_int();
					banAliasList.insert(make_pair(idStr, severityNum));
				}
			}
		}
	}
	catch(std::runtime_error& err)
	{	
		if(fDebug)
			LogPrintf("getBanListFromValue(): Failed to get ban list from value\n");
		return false;
	}
	return true;
}
bool getBanList(const vector<unsigned char>& banData, map<string, unsigned char>& banAliasList,  map<string, unsigned char>& banCertList,  map<string, unsigned char>& banOfferList)
{
	string value = stringFromVch(banData);
	
	UniValue outerValue(UniValue::VSTR);
	bool read = outerValue.read(value);
	if (read)
	{
		return getBanListFromValue(banAliasList, banCertList, banOfferList, outerValue);
	}
	else
	{
		if(fDebug)
			LogPrintf("getBanList() Failed to get value from alias\n");
		return false;
	}
	return false;

}
bool getCategoryList(vector<string>& categoryList)
{
	// check for alias existence in DB
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchFromString("syscategory"), vtxPos) || vtxPos.empty())
	{
		if(fDebug)
			LogPrintf("getCategoryList() Could not find syscategory alias\n");
		return false;
	}
	
	if (vtxPos.size() < 1)
	{
		if(fDebug)
			LogPrintf("getCategoryList() Could not find syscategory alias (vtxPos.size() == 0)\n");
		return false;
	}

	CAliasIndex categoryAlias = vtxPos.back();

	UniValue outerValue(UniValue::VSTR);
	bool read = outerValue.read(stringFromVch(categoryAlias.vchPublicValue));
	if (read)
	{
		try{
		
			getCategoryListFromValue(categoryList, outerValue);
			return true;
		}
		catch(std::runtime_error& err)
		{
			
			if(fDebug)
				LogPrintf("getCategoryListFromValue(): Failed to get category list from value\n");
			return false;
		}
	}
	else
	{
		if(fDebug)
			LogPrintf("getCategoryList() Failed to get value from alias\n");
		return false;
	}
	return false;

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

bool IsAliasOp(int op) {
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
	else if ((type == "message" || type == "any"))
		myNout = IndexOfMessageOutput(tx);
	else if ((type == "escrow" || type == "any"))
		myNout = IndexOfEscrowOutput(tx);
	else
		return false;
	return myNout >= 0;
}
void updateBans(const vector<unsigned char> &banData)
{
	map<string, unsigned char> banAliasList;
	map<string, unsigned char> banCertList;
	map<string, unsigned char> banOfferList;
	if(getBanList(banData, banAliasList, banCertList, banOfferList))
	{
		// update alias bans
		for (map<string, unsigned char>::iterator it = banAliasList.begin(); it != banAliasList.end(); it++) {
			vector<unsigned char> vchGUID = vchFromString((*it).first);
			unsigned char severity = (*it).second;
			if(paliasdb->ExistsAlias(vchGUID))
			{
				vector<CAliasIndex> vtxAliasPos;
				if (paliasdb->ReadAlias(vchGUID, vtxAliasPos) && !vtxAliasPos.empty())
				{
					CAliasIndex aliasBan = vtxAliasPos.back();
					aliasBan.safetyLevel = severity;
					PutToAliasList(vtxAliasPos, aliasBan);
					paliasdb->WriteAlias(aliasBan.vchAlias, vtxAliasPos);
					
				}		
			}
		}
		// update cert bans
		for (map<string, unsigned char>::iterator it = banCertList.begin(); it != banCertList.end(); it++) {
			vector<unsigned char> vchGUID = vchFromString((*it).first);
			unsigned char severity = (*it).second;
			if(pcertdb->ExistsCert(vchGUID))
			{
				vector<CCert> vtxCertPos;
				if (pcertdb->ReadCert(vchGUID, vtxCertPos) && !vtxCertPos.empty())
				{
					CCert certBan = vtxCertPos.back();
					certBan.safetyLevel = severity;
					PutToCertList(vtxCertPos, certBan);
					pcertdb->WriteCert(vchGUID, vtxCertPos);
					
				}		
			}
		}
		// update offer bans
		for (map<string, unsigned char>::iterator it = banOfferList.begin(); it != banOfferList.end(); it++) {
			vector<unsigned char> vchGUID = vchFromString((*it).first);
			unsigned char severity = (*it).second;
			if(pofferdb->ExistsOffer(vchGUID))
			{
				vector<COffer> vtxOfferPos, myLinkVtxPos;
				if (pofferdb->ReadOffer(vchGUID, vtxOfferPos) && !vtxOfferPos.empty())
				{
					COffer offerBan = vtxOfferPos.back();
					offerBan.safetyLevel = severity;
					offerBan.PutToOfferList(vtxOfferPos);
					pofferdb->WriteOffer(vchGUID, vtxOfferPos);	
				}		
			}
		}
	}
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
	if(op != OP_ALIAS_PAYMENT)
	{
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
	}
	else
		theAlias.SetNull();
	if(fJustCheck)
	{
		if(op != OP_ALIAS_PAYMENT)
		{
			if(vvchArgs.size() != 3)
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5001 - " + _("Alias arguments incorrect size");
				return error(errorMessage.c_str());
			}
		}
		else if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5002 - " + _("Alias arguments incorrect size");
			return error(errorMessage.c_str());
		}
		if(op != OP_ALIAS_PAYMENT)
		{
			if(!theAlias.IsNull())
			{
				if(vvchArgs.size() <= 2 || vchHash != vvchArgs[2])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5003 - " + _("Hash provided doesn't match the calculated hash of the data");
					return true;
				}
			}					
		}
	}
	if(fJustCheck || op == OP_ALIAS_UPDATE)
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
			if(prevCoins->vout.size() <= prevOutput->n || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch) || pop == OP_ALIAS_PAYMENT)
				continue;

			if (IsAliasOp(pop) && vvchArgs[0] == vvch[0]) {
				prevOp = pop;
				vvchPrevArgs = vvch;
				break;
			}
		}
	}
	vector<CAliasIndex> vtxPos;
	CRecipient fee;
	string retError = "";
	if(fJustCheck)
	{
		if(vvchArgs.empty() || !IsValidAliasName(vvchArgs[0]))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5004 - " + _("Alias name does not follow the domain name specification");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPublicValue.size() > MAX_VALUE_LENGTH && vvchArgs[0] != vchFromString("sysrates.peg") && vvchArgs[0] != vchFromString("syscategory"))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5005 - " + _("Alias public value too big");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPrivateValue.size() > MAX_ENCRYPTED_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5006 - " + _("Alias private value too big");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchAliasPeg.size() > MAX_GUID_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5007 - " + _("Alias peg too long");
			return error(errorMessage.c_str());
		}
		if(theAlias.vchPassword.size() > MAX_ENCRYPTED_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5008 - " + _("Alias password too long");
			return error(errorMessage.c_str());
		}
		if(theAlias.nHeight > nHeight)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5009 - " + _("Bad alias height");
			return error(errorMessage.c_str());
		}

		switch (op) {
			case OP_ALIAS_PAYMENT:
				break;
			case OP_ALIAS_ACTIVATE:
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
				break;
			case OP_ALIAS_UPDATE:
				if (!IsAliasOp(prevOp))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5012 - " + _("Alias input to this transaction not found");
					return error(errorMessage.c_str());
				}
				// Check GUID
				if (vvchArgs.size() <= 1 || vvchPrevArgs.size() <= 1 || vvchPrevArgs[1] != vvchArgs[1])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5013 - " + _("Alias Guid input mismatch");
					return error(errorMessage.c_str());
				}
				// Check name
				if (vvchPrevArgs[0] != vvchArgs[0])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5014 - " + _("Alias input mismatch");
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
		bool isExpired = false;
		CAliasIndex dbAlias;
		string strName = stringFromVch(vvchArgs[0]);
		boost::algorithm::to_lower(strName);
		vchAlias = vchFromString(strName);
		// get the alias from the DB
		if(!GetVtxOfAlias(vchAlias, dbAlias, vtxPos, isExpired))	
		{
			if(op == OP_ALIAS_ACTIVATE)
			{
				if(!isExpired && !vtxPos.empty())
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5017 - " + _("Trying to renew an alias that isn't expired");
					return true;
				}
			}
			else if(op == OP_ALIAS_UPDATE)
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5018 - " + _("Failed to read from alias DB");
				return true;
			}
			else if(op == OP_ALIAS_PAYMENT && vtxPos.empty())
				return true;
		}
		if(!vchData.empty())
		{
			CAmount fee = GetDataFee(tx.vout[nDataOut].scriptPubKey,  (op == OP_ALIAS_ACTIVATE)? theAlias.vchAliasPeg:dbAlias.vchAliasPeg, nHeight);	
			// if this is an alias update get expire time and figure out if alias update pays enough fees for updating expiry
			if(!theAlias.IsNull())
			{
				int nHeightTmp = nHeight;
				if(nHeightTmp > chainActive.Height())
					nHeightTmp = chainActive.Height();
				uint64_t nTimeExpiry = theAlias.nExpireTime - chainActive[nHeightTmp]->nTime;
				float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
				if(fYears < 1)
					fYears = 1;
				fee *= powf(2.88,fYears);

				// ensure aliases are good for atleast an hour
				if(nTimeExpiry < 3600)
					theAlias.nExpireTime = chainActive[nHeightTmp]->nTime+3600;
			}
			if ((fee-10000) > tx.vout[nDataOut].nValue) 
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5019 - " + _("Transaction does not pay enough fees");
				return true;
			}
		}
				
		if(op == OP_ALIAS_UPDATE)
		{
			if(!vtxPos.empty())
			{
				CTxDestination aliasDest;
				if (vvchPrevArgs.size() <= 0 || vvchPrevArgs[0] != vvchArgs[0] || !prevCoins || prevOutput->n >= prevCoins->vout.size() || !ExtractDestination(prevCoins->vout[prevOutput->n].scriptPubKey, aliasDest))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5020 - " + _("Cannot extract destination of alias input");
					theAlias = dbAlias;
				}
				else
				{
					const CAliasIndex& destAlias = vtxPos.back();
					CSyscoinAddress prevaddy(aliasDest);	
					CSyscoinAddress destaddy;
					GetAddress(destAlias, &destaddy);
					if(destaddy.ToString() != prevaddy.ToString())
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
					theAlias = vtxPos.back();
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
					if(theAlias.vchPassword.empty())
						theAlias.vchPassword = dbAlias.vchPassword;
					else
						pwChange = true;
					// user can't update safety level or rating after creation
					theAlias.safetyLevel = dbAlias.safetyLevel;
					theAlias.nRatingAsBuyer = dbAlias.nRatingAsBuyer;
					theAlias.nRatingCountAsBuyer = dbAlias.nRatingCountAsBuyer;
					theAlias.nRatingAsSeller = dbAlias.nRatingAsSeller;
					theAlias.nRatingCountAsSeller = dbAlias.nRatingCountAsSeller;
					theAlias.nRatingAsArbiter = dbAlias.nRatingAsArbiter;
					theAlias.nRatingCountAsArbiter= dbAlias.nRatingCountAsArbiter;
					theAlias.vchGUID = dbAlias.vchGUID;
					theAlias.vchAlias = dbAlias.vchAlias;
					if(!theAlias.multiSigInfo.IsNull())
					{
						if(theAlias.multiSigInfo.vchAliases.size() > 3 || theAlias.multiSigInfo.nRequiredSigs > 3)
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5023 - " + _("Alias multisig too big, reduce the number of signatures required for this alias and try again");
							theAlias.multiSigInfo.SetNull();
						}
						std::vector<CPubKey> pubkeys; 
						CPubKey pubkey(theAlias.vchPubKey);
						pubkeys.push_back(pubkey);
						for(int i =0;i<theAlias.multiSigInfo.vchAliases.size();i++)
						{
							CAliasIndex multiSigAlias;
							CTransaction txMultiSigAlias;
							if (!GetTxOfAlias(vchFromString(theAlias.multiSigInfo.vchAliases[i]), multiSigAlias, txMultiSigAlias))
								continue;
							CPubKey pubkey(multiSigAlias.vchPubKey);
							pubkeys.push_back(pubkey);

						}	
						if(theAlias.multiSigInfo.nRequiredSigs > pubkeys.size())
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5024 - " + _("Cannot update multisig alias because required signatures is greator than the amount of signatures provided");
							theAlias.multiSigInfo.SetNull();
						}	
						CScript inner = GetScriptForMultisig(theAlias.multiSigInfo.nRequiredSigs, pubkeys);
						CScript redeemScript(theAlias.multiSigInfo.vchRedeemScript.begin(), theAlias.multiSigInfo.vchRedeemScript.end());
						if(redeemScript != inner)
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5025 - " + _("Invalid redeem script provided in transaction");
							theAlias.multiSigInfo.SetNull();
						}
					}
					// if transfer or change pw
					if(dbAlias.vchPubKey != theAlias.vchPubKey)
					{
						// if transfer clear pw
						if(!pwChange)
							theAlias.vchPassword.clear();
						CSyscoinAddress myAddress;
						GetAddress(theAlias, &myAddress);
						const vector<unsigned char> &vchAddress = vchFromString(myAddress.ToString());
						// make sure xfer to pubkey doesn't point to an alias already, otherwise don't assign pubkey to alias
						// we want to avoid aliases with duplicate public keys (addresses)
						if (paliasdb->ExistsAddress(vchAddress))
						{
							vector<unsigned char> vchMyAlias;
							if (paliasdb->ReadAddress(vchAddress, vchMyAlias) && !vchMyAlias.empty() && vchMyAlias != dbAlias.vchAlias)
							{
								errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("An alias already exists with that address, try another public key");
								theAlias = dbAlias;
							}
						}					
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
			if(!isExpired && !vtxPos.empty())
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5028 - " + _("Trying to renew an alias that isn't expired");
				return true;
			}
			theAlias.nRatingAsBuyer = 0;
			theAlias.nRatingCountAsBuyer = 0;
			theAlias.nRatingAsSeller = 0;
			theAlias.nRatingCountAsSeller = 0;
			theAlias.nRatingAsArbiter = 0;
			theAlias.nRatingCountAsArbiter = 0;
			if(theAlias.multiSigInfo.vchAliases.size() > 0)
			{
				if(theAlias.multiSigInfo.vchAliases.size() > 5 || theAlias.multiSigInfo.nRequiredSigs > 5)
				{
					theAlias.multiSigInfo.SetNull();
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5029 - " + _("Alias multisig too big, reduce the number of signatures required for this alias and try again");
				}
				std::vector<CPubKey> pubkeys; 
				CPubKey pubkey(theAlias.vchPubKey);
				pubkeys.push_back(pubkey);
				for(int i =0;i<theAlias.multiSigInfo.vchAliases.size();i++)
				{
					CAliasIndex multiSigAlias;
					CTransaction txMultiSigAlias;
					if (!GetTxOfAlias(vchFromString(theAlias.multiSigInfo.vchAliases[i]), multiSigAlias, txMultiSigAlias))
						continue;

					CPubKey pubkey(multiSigAlias.vchPubKey);
					pubkeys.push_back(pubkey);

				}
				if(theAlias.multiSigInfo.nRequiredSigs > pubkeys.size())
				{
					theAlias.multiSigInfo.SetNull();
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5030 - " + _("Cannot update multisig alias because required signatures is greator than the amount of signatures provided");
				}
				CScript inner = GetScriptForMultisig(theAlias.multiSigInfo.nRequiredSigs, pubkeys);
				CScript redeemScript(theAlias.multiSigInfo.vchRedeemScript.begin(), theAlias.multiSigInfo.vchRedeemScript.end());
				if(redeemScript != inner)
				{
					theAlias.multiSigInfo.SetNull();
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5031 - " + _("Invalid redeem script provided in transaction");
				}
			}
		}
		else if(op == OP_ALIAS_PAYMENT)
		{
			const uint256 &txHash = tx.GetHash();
			vector<CAliasPayment> vtxPaymentPos;
			if(paliasdb->ExistsAliasPayment(vchAlias))
			{
				if(!paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5032 - " + _("Cannot read payments from alias DB");
					return true;
				}
			}
			CAliasPayment payment;
			payment.txHash = txHash;
			payment.nOut = nOut;
			vtxPaymentPos.push_back(payment);
			if (!dontaddtodb && !paliasdb->WriteAliasPayment(vchAlias, vtxPaymentPos))
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5034 - " + _("Failed to write payment to alias DB");
				return error(errorMessage.c_str());
			}
			if(fDebug)
				LogPrintf(
					"CONNECTED ALIAS: name=%s  op=%s  hash=%s  height=%d\n",
					stringFromVch(vchAlias).c_str(),
					aliasFromOp(op).c_str(),
					tx.GetHash().ToString().c_str(), nHeight);
			return true;
		}
		theAlias.nHeight = nHeight;
		theAlias.txHash = tx.GetHash();
		PutToAliasList(vtxPos, theAlias);
		CSyscoinAddress address;
		GetAddress(theAlias, &address);
		CAliasUnprunable aliasUnprunable;
		aliasUnprunable.vchGUID = theAlias.vchGUID;
		aliasUnprunable.nExpireTime = theAlias.nExpireTime;
		if (!dontaddtodb && !paliasdb->WriteAlias(vchAlias, aliasUnprunable, vchFromString(address.ToString()), vtxPos))
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5034 - " + _("Failed to write to alias DB");
			return error(errorMessage.c_str());
		}

		if(!dontaddtodb && vchAlias == vchFromString("sysban"))
		{
			updateBans(theAlias.vchPublicValue);
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
void GetAddress(const CAliasIndex& alias, CSyscoinAddress* address,const uint32_t nPaymentOption)
{
	if(!address)
		return;
	CPubKey aliasPubKey(alias.vchPubKey);
	CChainParams::AddressType myAddressType = PaymentOptionToAddressType(nPaymentOption);
	address[0] = CSyscoinAddress(aliasPubKey.GetID(), myAddressType);
	if(alias.multiSigInfo.vchAliases.size() > 0)
	{
		CScript inner(alias.multiSigInfo.vchRedeemScript.begin(), alias.multiSigInfo.vchRedeemScript.end());
		CScriptID innerID(inner);
		address[0] = CSyscoinAddress(innerID, myAddressType);
	}
}
void GetAddress(const CAliasIndex& alias, CSyscoinAddress* address,CScript& script,const uint32_t nPaymentOption)
{
	if(!address)
		return;
	CPubKey aliasPubKey(alias.vchPubKey);
	CChainParams::AddressType myAddressType = PaymentOptionToAddressType(nPaymentOption);
	address[0] = CSyscoinAddress(aliasPubKey.GetID(), myAddressType);
	script = GetScriptForDestination(address[0].Get());
	if(alias.multiSigInfo.vchAliases.size() > 0)
	{
		script = CScript(alias.multiSigInfo.vchRedeemScript.begin(), alias.multiSigInfo.vchRedeemScript.end());
		CScriptID innerID(script);
		address[0] = CSyscoinAddress(innerID, myAddressType);
	}
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
bool CAliasDB::ScanNames(const std::vector<unsigned char>& vchAlias, const string& strRegexp, bool safeSearch, 
		unsigned int nMax,
		vector<CAliasIndex>& nameScan) {
	// regexp
	using namespace boost::xpressive;
	smatch nameparts;
	string strRegexpLower = strRegexp;
	boost::algorithm::to_lower(strRegexpLower);
	sregex cregex = sregex::compile(strRegexpLower);
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	if(!vchAlias.empty())
		pcursor->Seek(make_pair(string("namei"), vchAlias));
	else
		pcursor->SeekToFirst();
	vector<CAliasIndex> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "namei") {
            	const vector<unsigned char> &vchMyAlias = key.second;
				
                
				pcursor->GetValue(vtxPos);
				
				if (vtxPos.empty()){
					pcursor->Next();
					continue;
				}
				const CAliasIndex &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= txPos.nExpireTime)
				{
					pcursor->Next();
					continue;
				} 
				if(txPos.safetyLevel >= SAFETY_LEVEL1)
				{
					if(safeSearch)
					{
						pcursor->Next();
						continue;
					}
					if(txPos.safetyLevel > SAFETY_LEVEL1)
					{
						pcursor->Next();
						continue;
					}
				}
				if(!txPos.safeSearch && safeSearch)
				{
					pcursor->Next();
					continue;
				}
				const string &name = stringFromVch(vchMyAlias);
				if (strRegexp != "" && !regex_search(name, nameparts, cregex) && strRegexp != name)
				{
					pcursor->Next();
					continue;
				}
                nameScan.push_back(txPos);
            }
            if (nameScan.size() >= nMax)
                break;

            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    return true;
}

// TODO: need to cleanout CTxOuts (transactions stored on disk) which have data stored in them after expiry, erase at same time on startup so pruning can happen properly
bool CAliasDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	vector<CAliasIndex> vtxPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "namei") {
            	const vector<unsigned char> &vchMyAlias = key.second;  
				if(vchMyAlias == vchFromString("sysrates.peg") || vchMyAlias == vchFromString("sysban") || vchMyAlias == vchFromString("syscategory"))
				{
					pcursor->Next();
					continue;
				}
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty()){
					servicesCleaned++;
					EraseAlias(vchMyAlias);
					pcursor->Next();
					continue;
				}
				const CAliasIndex &txPos = vtxPos.back();
  				if (chainActive.Tip()->nTime >= txPos.nExpireTime)
				{
					servicesCleaned++;
					EraseAlias(vchMyAlias);
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
	if(pmessagedb!= NULL)
		pmessagedb->CleanupDatabase(numServicesCleaned);
	if(pcertdb!= NULL)
		pcertdb->CleanupDatabase(numServicesCleaned);
	if(paliasdb!= NULL)
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
	if(pmessagedb != NULL)
	{
		if (!pmessagedb->Flush())
			LogPrintf("Failed to write to message database!");
		delete pmessagedb;
		pmessagedb = NULL;
	}

}
bool GetTxOfAlias(const vector<unsigned char> &vchAlias, 
				  CAliasIndex& txPos, CTransaction& tx, bool skipExpiresCheck) {
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		return false;
	txPos = vtxPos.back();
	int nHeight = txPos.nHeight;
	if(vchAlias != vchFromString("sysrates.peg") && vchAlias != vchFromString("sysban") && vchAlias != vchFromString("syscategory"))
	{
		if (!skipExpiresCheck && chainActive.Tip()->nTime >= txPos.nExpireTime) {
			string name = stringFromVch(vchAlias);
			LogPrintf("GetTxOfAlias(%s) : expired", name.c_str());
			return false;
		}
	}

	if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
		return error("GetTxOfAlias() : could not read tx from disk");

	return true;
}
bool GetTxAndVtxOfAlias(const vector<unsigned char> &vchAlias, 
						CAliasIndex& txPos, CTransaction& tx, std::vector<CAliasIndex> &vtxPos, bool &isExpired, bool skipExpiresCheck) {
	isExpired = false;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		return false;
	txPos = vtxPos.back();
	int nHeight = txPos.nHeight;
	if(vchAlias != vchFromString("sysrates.peg") && vchAlias != vchFromString("sysban") && vchAlias != vchFromString("syscategory"))
	{
		if (!skipExpiresCheck && chainActive.Tip()->nTime >= txPos.nExpireTime) {
			string name = stringFromVch(vchAlias);
			LogPrintf("GetTxOfAlias(%s) : expired", name.c_str());
			isExpired = true;
			return false;
		}
	}

	if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
		return error("GetTxOfAlias() : could not read tx from disk");
	return true;
}
bool GetVtxOfAlias(const vector<unsigned char> &vchAlias, 
						CAliasIndex& txPos, std::vector<CAliasIndex> &vtxPos, bool &isExpired, bool skipExpiresCheck) {
	isExpired = false;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		return false;
	txPos = vtxPos.back();
	int nHeight = txPos.nHeight;
	if(vchAlias != vchFromString("sysrates.peg") && vchAlias != vchFromString("sysban") && vchAlias != vchFromString("syscategory"))
	{
		if (!skipExpiresCheck && chainActive.Tip()->nTime >= txPos.nExpireTime) {
			string name = stringFromVch(vchAlias);
			LogPrintf("GetTxOfAlias(%s) : expired", name.c_str());
			isExpired = true;
			return false;
		}
	}
	return true;
}
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, unsigned char& safetyLevel, bool& safeSearch, std::vector<unsigned char> &vchRedeemScript, std::vector<unsigned char> &vchPubKey) {

	string strLowerAlias = strAlias;
	boost::algorithm::to_lower(strLowerAlias);
	const vector<unsigned char> &vchAlias = vchFromValue(strLowerAlias);
	if (!paliasdb || !paliasdb->ExistsAlias(vchAlias))
		return false;

	// check for alias existence in DB
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos))
		return false;
	if (vtxPos.size() < 1)
		return false;

	const CAliasIndex &alias = vtxPos.back();
	CSyscoinAddress address;
	GetAddress(alias, &address);
	strAddress = address.ToString();
	safetyLevel = alias.safetyLevel;
	safeSearch = alias.safeSearch;
	vchRedeemScript = alias.multiSigInfo.vchRedeemScript;
	vchPubKey = alias.vchPubKey;
	return true;
}

bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, unsigned char& safetyLevel, bool& safeSearch,  std::vector<unsigned char> &vchRedeemScript, std::vector<unsigned char> &vchPubKey) {

	const vector<unsigned char> &vchAddress = vchFromValue(strAddress);
	if (!paliasdb || !paliasdb->ExistsAddress(vchAddress))
		return false;

	// check for alias address mapping existence in DB
	vector<unsigned char> vchAlias;
	if (!paliasdb->ReadAddress(vchAddress, vchAlias))
		return false;
	if (vchAlias.empty())
		return false;
	
	strAlias = stringFromVch(vchAlias);
	vector<CAliasIndex> vtxPos;
	if (paliasdb && !paliasdb->ReadAlias(vchAlias, vtxPos))
		return false;
	if (vtxPos.size() < 1)
		return false;
	const CAliasIndex &alias = vtxPos.back();
	safetyLevel = alias.safetyLevel;
	safeSearch = alias.safeSearch;
	vchRedeemScript = alias.multiSigInfo.vchRedeemScript;
	vchPubKey = alias.vchPubKey;
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
	return DecodeAndParseAliasTx(tx, op, nOut, vvch) 
		|| DecodeAndParseCertTx(tx, op, nOut, vvch)
		|| DecodeAndParseOfferTx(tx, op, nOut, vvch)
		|| DecodeAndParseEscrowTx(tx, op, nOut, vvch)
		|| DecodeAndParseMessageTx(tx, op, nOut, vvch);
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
			nOut = i;
			found = true;
			vvch = vvchRead;
			break;
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
	CAmount fee = 3*minRelayTxFee.GetFee(nSize);
	recipient.nAmount = fee;
}
void CreateFeeRecipient(CScript& scriptPubKey, const vector<unsigned char>& vchAliasPeg, const uint64_t& nHeight, const vector<unsigned char>& data, CRecipient& recipient)
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
	int nFeePerByte = getFeePerByte(vchAliasPeg, vchFromString("SYS"), nHeight, precision);
	if(nFeePerByte <= 0)
		nFee = 3*minRelayTxFee.GetFee(nSize);
	else
		nFee = nFeePerByte * nSize;

	recipient.nAmount = nFee;
}
CAmount GetDataFee(const CScript& scriptPubKey, const vector<unsigned char>& vchAliasPeg, const uint64_t& nHeight)
{
	int precision = 0;
	CAmount nFee = 0;
	CRecipient recipient;
	CRecipient recp = {scriptPubKey, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
    size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	int nFeePerByte = getFeePerByte(vchAliasPeg, vchFromString("SYS"), nHeight, precision);
	if(nFeePerByte <= 0)
		nFee = 3*minRelayTxFee.GetFee(nSize);
	else
		nFee = nFeePerByte * nSize;
	
	recipient.nAmount = nFee;
	return recipient.nAmount;
}
UniValue aliasauthenticate(const UniValue& params, bool fHelp) {
	if (fHelp || 2 != params.size())
		throw runtime_error("aliasauthenticate <alias> <password>\n"
		"Authenticates an alias with a provided password and returns the private key if successful. Warning: Calling this function over a public network can lead to someone reading your password/private key in plain text.\n");
	vector<unsigned char> vchAlias = vchFromString(params[0].get_str());
	const SecureString &strPassword = params[1].get_str().c_str();
	
	CTransaction tx;
	CAliasIndex theAlias;
	if (!GetTxOfAlias(vchAlias, theAlias, tx, true))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5500 - " + _("Could not find an alias with this name"));

	CPubKey aliasPubKey(theAlias.vchPubKey);
	CCrypter crypt;
	uint256 hashAliasNum = Hash(vchAlias.begin(), vchAlias.end());
	vector<unsigned char> vchAliasHash = vchFromString(hashAliasNum.GetHex());
	vchAliasHash.resize(WALLET_CRYPTO_SALT_SIZE);
	if(strPassword.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5501 - " + _("Password cannot be empty"));

    if(!crypt.SetKeyFromPassphrase(strPassword, vchAliasHash, 25000, 0))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5502 - " + _("Could not determine key from password"));

	CKey key;
	key.Set(crypt.chKey, crypt.chKey + (sizeof crypt.chKey), true);
	CPubKey defaultKey = key.GetPubKey();
	if(!defaultKey.IsFullyValid())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5503 - " + _("Generated public key not fully valid"));

	if(aliasPubKey != defaultKey)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5504 - " + _("Password is incorrect"));
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("privatekey", CSyscoinSecret(key).ToString()));
	return res;

}
void TransferAliasBalances(const vector<unsigned char> &vchAlias, const CScript& scriptPubKeyTo, vector<CRecipient> &vecSend, CCoinControl& coinControl){

	LOCK(cs_main);
	CAmount nAmount = 0;
	std::vector<CAliasPayment> vtxPaymentPos;
	if(!paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos))
		return;
	
	CAliasIndex theAlias;
	CTransaction aliasTx;
	if (!GetTxOfAlias(vchAlias, theAlias, aliasTx, true))
		return;

	CSyscoinAddress addressFrom;
	GetAddress(theAlias, &addressFrom);

	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
	// get all alias inputs and transfer them to the new alias destination
    for (unsigned int i = 0;i<vtxPaymentPos.size();i++)
    {
		const CAliasPayment& aliasPayment = vtxPaymentPos[i];
		coins = view.AccessCoins(aliasPayment.txHash);
		if(coins == NULL)
			continue;
     
		if(!coins->IsAvailable(aliasPayment.nOut))
			continue;
		if (!ExtractDestination(coins->vout[aliasPayment.nOut].scriptPubKey, payDest)) 
			continue;
		destaddy = CSyscoinAddress(payDest);
        if (destaddy.ToString() == addressFrom.ToString())
		{  
			nAmount += coins->vout[aliasPayment.nOut].nValue;
			COutPoint outpt(aliasPayment.txHash, aliasPayment.nOut);
			coinControl.Select(outpt);
		}	
		
    }
	if(nAmount > 0)
	{
		CAmount nFee = 0;
		for(unsigned int i=0;i<vecSend.size();i++)
			nFee += vecSend[i].nAmount;

		CScript scriptChangeOrig;
		scriptChangeOrig << CScript::EncodeOP_N(OP_ALIAS_PAYMENT) << vchAlias << OP_2DROP;
		scriptChangeOrig += scriptPubKeyTo;
		
		CRecipient recipient  = {scriptChangeOrig, nAmount-(nFee*2), false};
		vecSend.push_back(recipient);
	}
}
UniValue aliasnew(const UniValue& params, bool fHelp) {
	if (fHelp || 4 > params.size() || 10 < params.size())
		throw runtime_error(
		"aliasnew <aliaspeg> <aliasname> <password> <public value> [private value] [safe search=Yes] [accept transfers=Yes] [expire=31536000] [nrequired=0] [\"alias\",...]\n"
						"<aliasname> alias name.\n"
						"<password> used to generate your public/private key that controls this alias. Warning: Calling this function over a public network can lead to someone reading your password in plain text.\n"
						"<public value> alias public profile data, 1024 chars max.\n"
						"<private value> alias private profile data, 1024 chars max. Will be private and readable by owner only.\n"
						"<safe search> set to No if this alias should only show in the search when safe search is not selected. Defaults to Yes (alias shows with or without safe search selected in search lists).\n"	
						"<accept transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"	
						"<expire> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is FEERATE*(1.5^years). FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"	
						"<nrequired> For multisig aliases only. The number of required signatures out of the n aliases for a multisig alias update.\n"
						"<aliases>     For multisig aliases only. A json array of aliases which are used to sign on an update to this alias.\n"
						"     [\n"
						"       \"alias\"    Existing syscoin alias name\n"
						"       ,...\n"
						"     ]\n"						
						
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
	string strPassword = params[2].get_str().c_str();
	if(strPassword.size() < 4 && strPassword.size() > 0)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5507 - " + _("Invalid Syscoin Identity. Please enter a password atleast 4 characters long"));
	string strPublicValue = params[3].get_str();
	vchPublicValue = vchFromString(strPublicValue);

	string strPrivateValue = params.size()>=5?params[4].get_str():"";
	string strSafeSearch = "Yes";
	string strAcceptCertTransfers = "Yes";

	if(params.size() >= 6)
	{
		strSafeSearch = params[5].get_str();
	}
	if(params.size() >= 7)
	{
		strAcceptCertTransfers = params[6].get_str();
	}
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	if(params.size() >= 8)
		nTime = boost::lexical_cast<uint64_t>(params[7].get_str());
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;
    int nMultiSig = 1;
	if(params.size() >= 9)
		nMultiSig = boost::lexical_cast<int>(params[8].get_str());
    UniValue aliasNames;
	if(params.size() >= 10)
		aliasNames = params[9].get_array();
	
	vchPrivateValue = vchFromString(strPrivateValue);

	CWalletTx wtx;

	EnsureWalletIsUnlocked();
	CPubKey defaultKey, encryptionKey;
	encryptionKey = pwalletMain->GenerateNewKey();
	CKey privateEncryptionKey;
	pwalletMain->GetKey(encryptionKey.GetID(), privateEncryptionKey);
	std::vector<unsigned char> vchEncryptionPublicKey(encryptionKey.begin(), encryptionKey.end());
	std::vector<unsigned char> vchEncryptionPrivateKey(privateEncryptionKey.begin(), privateEncryptionKey.end());

	CAliasIndex oldAlias;
	vector<CAliasIndex> vtxPos;
	bool isExpired;
	bool aliasExists = GetVtxOfAlias(vchAlias, oldAlias, vtxPos, isExpired);
	if(aliasExists && !isExpired)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("This alias already exists"));
	if(IsMyAlias(oldAlias))
	{
		defaultKey = CPubKey(oldAlias.vchPubKey);	
	}
	else if(strPassword.empty())
		defaultKey = pwalletMain->GenerateNewKey();

	CSyscoinAddress oldAddress(defaultKey.GetID());
	if(!strPassword.empty())
	{
		CCrypter crypt;
		uint256 hashAliasNum = Hash(vchAlias.begin(), vchAlias.end());
		vector<unsigned char> vchAliasHash = vchFromString(hashAliasNum.GetHex());
		vchAliasHash.resize(WALLET_CRYPTO_SALT_SIZE);
		string pwStr = strPassword;
		SecureString password = pwStr.c_str();
		if(!crypt.SetKeyFromPassphrase(password, vchAliasHash, 25000, 0))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Could not determine key from password"));
		CKey key;
		key.Set(crypt.chKey, crypt.chKey + (sizeof crypt.chKey), true);
		defaultKey = key.GetPubKey();
		if(!defaultKey.IsFullyValid())
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5510 - " + _("Generated public key not fully valid"));
		CKey keyTmp;
		if(!pwalletMain->GetKey(defaultKey.GetID(), keyTmp) && !pwalletMain->AddKeyPubKey(key, defaultKey))	
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5511 - " + _("Please choose a different password"));
	}
	CScript scriptPubKeyOrig;
	CMultiSigAliasInfo multiSigInfo;
	string strCipherText;
	if(aliasNames.size() > 0)
	{
		multiSigInfo.nRequiredSigs = nMultiSig;
		std::vector<CPubKey> pubkeys; 
		pubkeys.push_back(defaultKey);
		for(int i =0;i<aliasNames.size();i++)
		{
			CAliasIndex multiSigAlias;
			CTransaction txMultiSigAlias;
			if (!GetTxOfAlias( vchFromString(aliasNames[i].get_str()), multiSigAlias, txMultiSigAlias))
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5512 - " + _("Could not find multisig alias with the name: ") + aliasNames[i].get_str());

			CPubKey pubkey(multiSigAlias.vchPubKey);
			pubkeys.push_back(pubkey);
			multiSigInfo.vchAliases.push_back(aliasNames[i].get_str());
			vector<unsigned char> vchMSPubKey(pubkey.begin(), pubkey.end());
			if(!EncryptMessage(vchMSPubKey, vchEncryptionPrivateKey, strCipherText))
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5513 - " + _("Could not encrypt private encryption key!"));
			multiSigInfo.vchEncryptionPrivateKeys.push_back(strCipherText);
		}
		scriptPubKeyOrig = GetScriptForMultisig(nMultiSig, pubkeys);
		std::vector<unsigned char> vchRedeemScript(scriptPubKeyOrig.begin(), scriptPubKeyOrig.end());
		multiSigInfo.vchRedeemScript = vchRedeemScript;

	}	
	else
		scriptPubKeyOrig = GetScriptForDestination(defaultKey.GetID());

	CSyscoinAddress newAddress = CSyscoinAddress(CScriptID(scriptPubKeyOrig));	

	std::vector<unsigned char> vchPubKey(defaultKey.begin(), defaultKey.end());
	
	if(!EncryptMessage(vchEncryptionPublicKey, vchPrivateValue, strCipherText))
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5514 - " + _("Could not encrypt private alias value!"));
	}
	vchPrivateValue = vchFromString(strCipherText);

	if(!EncryptMessage(vchPubKey, vchEncryptionPrivateKey, strCipherText))
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5515 - " + _("Could not encrypt private encryption key!"));
	}
	vchEncryptionPrivateKey = vchFromString(strCipherText);
	
	if(!strPassword.empty())
	{
		if(!EncryptMessage(vchPubKey, vchFromString(strPassword), strCipherText))
		{
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5516 - " + _("Could not encrypt alias password"));
		}
		strPassword = strCipherText;
	}
	vector<unsigned char> vchRandAlias = vchFromString(GenerateSyscoinGuid());

    // build alias
    CAliasIndex newAlias;
	newAlias.vchGUID = vchRandAlias;
	newAlias.vchAliasPeg = vchAliasPeg;
	newAlias.vchAlias = vchAlias;
	newAlias.nHeight = chainActive.Tip()->nHeight;
	newAlias.vchPubKey = vchPubKey;
	newAlias.vchEncryptionPublicKey = vchEncryptionPublicKey;
	newAlias.vchEncryptionPrivateKey = vchEncryptionPrivateKey;
	newAlias.vchPublicValue = vchPublicValue;
	newAlias.vchPrivateValue = vchPrivateValue;
	newAlias.nExpireTime = nTime;
	newAlias.vchPassword = vchFromString(strPassword);
	newAlias.safetyLevel = 0;
	newAlias.safeSearch = strSafeSearch == "Yes"? true: false;
	newAlias.acceptCertTransfers = strAcceptCertTransfers == "Yes"? true: false;
	newAlias.multiSigInfo = multiSigInfo;
	
	vector<unsigned char> data;
	newAlias.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
    vector<unsigned char> vchHashAlias = vchFromValue(hash.GetHex());

	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_ACTIVATE) << vchAlias << vchRandAlias << vchHashAlias << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	for(unsigned int i =0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(recipient);
	CScript scriptData;
	
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
	float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
	if(fYears < 1)
		fYears = 1;
	fee.nAmount *= powf(2.88,fYears);


	vecSend.push_back(fee);
	CCoinControl coinControl;
	// if renewing your own alias and address changed, transfer balances
	if(!oldAlias.IsNull() && newAddress.ToString() != oldAddress.ToString() && IsMyAlias(oldAlias))
	{
		coinControl.fAllowOtherInputs = true;
		coinControl.fAllowWatchOnly = true;
		TransferAliasBalances(vchAlias, scriptPubKeyOrig, vecSend, coinControl);
	}
	SendMoneySyscoin(vecSend, recipient.nAmount + fee.nAmount, false, wtx, NULL, 0, oldAlias.multiSigInfo.vchAliases.size() > 0, coinControl.HasSelected()? &coinControl: NULL);
	UniValue res(UniValue::VARR);
	if(oldAlias.multiSigInfo.vchAliases.size() > 0)
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
			res.push_back(HexStr(vchPubKey));
		}
		else
		{
			res.push_back(hex_str);
			res.push_back(HexStr(vchPubKey));
			res.push_back("false");
		}
	}
	else
	{
		res.push_back(wtx.GetHash().GetHex());
		res.push_back(HexStr(vchPubKey));
	}
	return res;
}
UniValue aliasupdate(const UniValue& params, bool fHelp) {
	if (fHelp || 3 > params.size() || 11 < params.size())
		throw runtime_error(
		"aliasupdate <aliaspeg> <aliasname> <public value> [private value=''] [safesearch=Yes] [toalias_pubkey=''] [password=''] [accept transfers=Yes] [expire=31536000] [nrequired=0] [\"alias\",...]\n"
						"Update and possibly transfer an alias.\n"
						"<aliasname> alias name.\n"
						"<public value> alias public profile data, 1024 chars max.\n"
						"<private value> alias private profile data, 1024 chars max. Will be private and readable by owner only.\n"				
						"<password> used to generate your public/private key that controls this alias. Warning: Calling this function over a public network can lead to someone reading your password in plain text. Leave empty to leave current password unchanged.\n"
						"<safesearch> is this alias safe to search. Defaults to Yes, No for not safe and to hide in GUI search queries\n"
						"<toalias_pubkey> receiver syscoin alias pub key, if transferring alias.\n"
						"<accept transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"		
						"<expire> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is 1.5^years. FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"		
						"<nrequired> For multisig aliases only. The number of required signatures out of the n aliases for a multisig alias update.\n"
						"<aliases>     For multisig aliases only. A json array of aliases which are used to sign on an update to this alias.\n"
						"     [\n"
						"       \"alias\"    Existing syscoin alias name\n"
						"       ,...\n"
						"     ]\n"							
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAliasPeg = vchFromString(params[0].get_str());
	if(vchAliasPeg.empty())
		vchAliasPeg = vchFromString("sysrates.peg");
	vector<unsigned char> vchAlias = vchFromString(params[1].get_str());
	vector<unsigned char> vchPublicValue;
	vector<unsigned char> vchPrivateValue;
	string strPublicValue = params[2].get_str();
	vchPublicValue = vchFromString(strPublicValue);
	string strPrivateValue = params.size()>=4 && params[3].get_str().size() > 0?params[3].get_str():"";
	vchPrivateValue = vchFromString(strPrivateValue);
	vector<unsigned char> vchPubKeyByte;
	
	CWalletTx wtx;
	CAliasIndex updateAlias;

	string strSafeSearch = "Yes";
	if(params.size() >= 5)
	{
		strSafeSearch = params[4].get_str();
	}
	string strPubKey;
	bool transferAlias = false;
    if (params.size() >= 6 && params[5].get_str().size() > 1) {
		transferAlias = true;
		vector<unsigned char> vchPubKey;
		vchPubKey = vchFromString(params[5].get_str());
		boost::algorithm::unhex(vchPubKey.begin(), vchPubKey.end(), std::back_inserter(vchPubKeyByte));
	}
	string strPassword;
	if(params.size() >= 7 && params[6].get_str().size() > 0 && vchPubKeyByte.empty())
		strPassword = params[6].get_str();

	if(strPassword.size() < 4 && strPassword.size() > 0)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5517 - " + _("Invalid Syscoin Identity. Please enter a password atleast 4 characters long"));

	string strAcceptCertTransfers = "Yes";
	if(params.size() >= 8)
	{
		strAcceptCertTransfers = params[7].get_str();
	}
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	if(params.size() >= 9)
		nTime = boost::lexical_cast<uint64_t>(params[8].get_str());
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;
    int nMultiSig = 1;
	if(params.size() >= 10)
		nMultiSig = boost::lexical_cast<int>(params[9].get_str());
    UniValue aliasNames;
	if(params.size() >= 11)
		aliasNames = params[10].get_array();
	EnsureWalletIsUnlocked();
	CTransaction tx;
	CAliasIndex theAlias;
	if (!GetTxOfAlias(vchAlias, theAlias, tx, true))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5518 - " + _("Could not find an alias with this name"));

	COutPoint outPoint;
	int numResults  = aliasunspent(vchAlias, outPoint);
	const CWalletTx* wtxIn = pwalletMain->GetWalletTx(outPoint.hash);
	if (wtxIn == NULL)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5519 - " + _("This alias is not in your wallet"));

	CSyscoinAddress oldAddress;
	GetAddress(theAlias, &oldAddress);
	CPubKey pubKey(theAlias.vchPubKey);	
	if(!strPassword.empty())
	{
		CCrypter crypt;
		uint256 hashAliasNum = Hash(vchAlias.begin(), vchAlias.end());
		vector<unsigned char> vchAliasHash = vchFromString(hashAliasNum.GetHex());
		vchAliasHash.resize(WALLET_CRYPTO_SALT_SIZE);
		string pwStr = strPassword;
		SecureString password = pwStr.c_str();
		if(!crypt.SetKeyFromPassphrase(password, vchAliasHash, 25000, 0))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5520 - " + _("Could not determine key from password"));
		CKey key;
		key.Set(crypt.chKey, crypt.chKey + (sizeof crypt.chKey), true);
		pubKey = key.GetPubKey();
		
		if(!pubKey.IsFullyValid())
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5521 - " + _("Generated public key not fully valid"));	
		CKey keyTmp;
		if(!pwalletMain->GetKey(pubKey.GetID(), keyTmp) && !pwalletMain->AddKeyPubKey(key, pubKey))	
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5522 - " + _("Please choose a different password"));	
	}
	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	CKey vchSecret;
	if(vchPubKeyByte.empty())
	{
		vchPubKeyByte = vector<unsigned char>(pubKey.begin(), pubKey.end());
	}
	pubKey = CPubKey(vchPubKeyByte);
	string strCipherText;
	vector<unsigned char> vchEncryptionPrivateKey = copyAlias.vchEncryptionPrivateKey;
	vector<unsigned char> vchEncryptionPublicKey = copyAlias.vchEncryptionPublicKey;
	string strDecryptedText = "";
	if(!DecryptPrivateKey(copyAlias.vchPubKey, vchEncryptionPrivateKey, strDecryptedText))
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5523 - " + _("Could not decrypt alias encryption private key"));
	}
	vchEncryptionPrivateKey = vchFromString(strDecryptedText);
	if(!vchPrivateValue.empty())
	{
		// see if data changed, if not dont send payload
		if(!copyAlias.vchPrivateValue.empty() && !transferAlias)
		{
			if(!DecryptMessage(copyAlias, copyAlias.vchPrivateValue, strDecryptedText))
			{
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5524 - " + _("Could not decrypt alias private data"));
			}
			if(vchPrivateValue == vchFromString(strDecryptedText))
				vchPrivateValue = copyAlias.vchPrivateValue;
			else
			{
				if(!EncryptMessage(vchEncryptionPublicKey, vchPrivateValue, strCipherText))
				{
					throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5525 - " + _("Could not encrypt alias private data"));
				}
				vchPrivateValue = vchFromString(strCipherText);
			}
		}
		else
		{
			if(!EncryptMessage(vchEncryptionPublicKey, vchPrivateValue, strCipherText))
			{
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5526 - " + _("Could not encrypt alias private data"));
			}
			vchPrivateValue = vchFromString(strCipherText);
		}
	}
	if(!strPassword.empty())
	{
		// encrypt using new key
		if(!EncryptMessage(vchPubKeyByte, vchFromString(strPassword), strCipherText))
		{
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5527 - " + _("Could not encrypt alias password"));
		}
		strPassword = strCipherText;
	}
	CMultiSigAliasInfo multiSigInfo;
	if(aliasNames.size() > 0)
	{
		multiSigInfo.nRequiredSigs = nMultiSig;
		std::vector<CPubKey> pubkeys; 
		pubkeys.push_back(pubKey);
		for(int i =0;i<aliasNames.size();i++)
		{
			CAliasIndex multiSigAlias;
			CTransaction txMultiSigAlias;
			if (!GetTxOfAlias( vchFromString(aliasNames[i].get_str()), multiSigAlias, txMultiSigAlias))
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5528 - " + _("Could not find multisig alias with the name: ") + aliasNames[i].get_str());

			CPubKey pubkey(multiSigAlias.vchPubKey);
			pubkeys.push_back(pubkey);
			multiSigInfo.vchAliases.push_back(aliasNames[i].get_str());
			vector<unsigned char> vchMSPubKey(pubkey.begin(), pubkey.end());
			if(!EncryptMessage(vchMSPubKey, vchEncryptionPrivateKey, strCipherText))
				throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5529 - " + _("Could not encrypt private encryption key!"));
			multiSigInfo.vchEncryptionPrivateKeys.push_back(strCipherText);
		}	
		CScript script = GetScriptForMultisig(nMultiSig, pubkeys);
		std::vector<unsigned char> vchRedeemScript(script.begin(), script.end());
		multiSigInfo.vchRedeemScript = vchRedeemScript;
	}

	if(!EncryptMessage(vchPubKeyByte, vchEncryptionPrivateKey, strCipherText))
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5530 - " + _("Could not encrypt private encryption key!"));
	}
	vchEncryptionPrivateKey = vchFromString(strCipherText);

	theAlias.nHeight = chainActive.Tip()->nHeight;
	if(copyAlias.vchPublicValue != vchPublicValue)
		theAlias.vchPublicValue = vchPublicValue;
	if(copyAlias.vchPrivateValue != vchPrivateValue)
		theAlias.vchPrivateValue = vchPrivateValue;
	if(copyAlias.vchEncryptionPrivateKey != vchEncryptionPrivateKey)
		theAlias.vchEncryptionPrivateKey = vchEncryptionPrivateKey;
	if(copyAlias.vchEncryptionPublicKey != vchEncryptionPublicKey)
		theAlias.vchEncryptionPublicKey = vchEncryptionPublicKey;
	if(copyAlias.vchPassword != vchFromString(strPassword))
		theAlias.vchPassword = vchFromString(strPassword);

	theAlias.vchAliasPeg = vchAliasPeg;
	theAlias.multiSigInfo = multiSigInfo;
	theAlias.vchPubKey = vchPubKeyByte;
	theAlias.nExpireTime = nTime;
	theAlias.safeSearch = strSafeSearch == "Yes"? true: false;
	theAlias.acceptCertTransfers = strAcceptCertTransfers == "Yes"? true: false;
	
	CSyscoinAddress newAddress;
	CScript scriptPubKeyOrig;
	GetAddress(theAlias, &newAddress, scriptPubKeyOrig);
	vector<unsigned char> data;
	theAlias.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
    vector<unsigned char> vchHashAlias = vchFromValue(hash.GetHex());

	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient); 
	for(unsigned int i =numResults;i<=MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		vecSend.push_back(recipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, copyAlias.vchAliasPeg,  chainActive.Tip()->nHeight, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
	float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
	if(fYears < 1)
		fYears = 1;
	fee.nAmount *= powf(2.88,fYears);
	
	vecSend.push_back(fee);
	CCoinControl coinControl;
	// for now dont transfer balances on an alias transfer (TODO add option to transfer balances)
	if(!transferAlias && newAddress.ToString() != oldAddress.ToString())
	{
		coinControl.fAllowOtherInputs = true;
		coinControl.fAllowWatchOnly = true;
		TransferAliasBalances(vchAlias, scriptPubKeyOrig, vecSend, coinControl);
	}
	
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxIn, outPoint.n, copyAlias.multiSigInfo.vchAliases.size() > 0, coinControl.HasSelected()? &coinControl: NULL);
	UniValue res(UniValue::VARR);
	if(copyAlias.multiSigInfo.vchAliases.size() > 0)
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
			res.push_back(wtx.GetHash().GetHex());
		else
		{
			res.push_back(hex_str);
			res.push_back("false");
		}
	}
	else
		res.push_back(wtx.GetHash().GetHex());
	return res;
}
UniValue syscoindecoderawtransaction(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("syscoindecoderawtransaction <alias> <hexstring>\n"
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
	return output;
}
void SysTxToJSON(const int op, const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash, UniValue &entry)
{
	if(IsAliasOp(op))
		AliasTxToJSON(op, vchData, vchHash, entry);
	if(IsCertOp(op))
		CertTxToJSON(op, vchData, vchHash, entry);
	if(IsMessageOp(op))
		MessageTxToJSON(op,vchData, vchHash, entry);
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
	bool isExpired = false;
	vector<CAliasIndex> aliasVtxPos;
	CTransaction aliastx;
	CAliasIndex dbAlias;
	if(GetTxAndVtxOfAlias(alias.vchAlias, dbAlias, aliastx, aliasVtxPos, isExpired, true))
	{
		dbAlias.nHeight = alias.nHeight;
		dbAlias.GetAliasFromList(aliasVtxPos);
	}
	string noDifferentStr = _("<No Difference Detected>");

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("name", stringFromVch(alias.vchAlias)));

	string aliasPegValue = noDifferentStr;
	if(!alias.vchAliasPeg.empty() && alias.vchAliasPeg != dbAlias.vchAliasPeg)
		aliasPegValue = stringFromVch(alias.vchAliasPeg);

	entry.push_back(Pair("aliaspeg", aliasPegValue));

	string publicValue = noDifferentStr;
	if(!alias.vchPublicValue .empty() && alias.vchPublicValue != dbAlias.vchPublicValue)
		publicValue = stringFromVch(alias.vchPublicValue);
	entry.push_back(Pair("publicvalue", publicValue));

	string strPrivateValue = "";
	if(!alias.vchPrivateValue.empty())
		strPrivateValue = _("Encrypted for alias owner");
	string strDecrypted = "";
	if(DecryptMessage(alias, alias.vchPrivateValue, strDecrypted))
		strPrivateValue = strDecrypted;		

	string privateValue = noDifferentStr;
	if(!alias.vchPrivateValue.empty() && alias.vchPrivateValue != dbAlias.vchPrivateValue)
		privateValue = strPrivateValue;

	entry.push_back(Pair("privatevalue", privateValue));

	string strPassword = "";
	if(!alias.vchPassword.empty())
		strPassword = _("Encrypted for alias owner");
	strDecrypted = "";
	if(DecryptPrivateKey(alias.vchPubKey, alias.vchPassword, strDecrypted))
		strPassword = strDecrypted;		

	string password = noDifferentStr;
	if(!alias.vchPassword.empty() && alias.vchPassword != dbAlias.vchPassword)
		password = strPassword;

	entry.push_back(Pair("password", password));


	CSyscoinAddress address;
	GetAddress(alias, &address);
	CSyscoinAddress dbaddress;
	GetAddress(dbAlias, &dbaddress);

	string addressValue = noDifferentStr;
	if(address.ToString() != dbaddress.ToString())
		addressValue = address.ToString();

	entry.push_back(Pair("address", addressValue));


	string safeSearchValue = noDifferentStr;
	if(alias.safeSearch != dbAlias.safeSearch)
		safeSearchValue = alias.safeSearch? "Yes": "No";

	entry.push_back(Pair("safesearch", safeSearchValue));
	
	string acceptTransfersValue = noDifferentStr;
	if(alias.acceptCertTransfers != dbAlias.acceptCertTransfers)
		acceptTransfersValue = alias.acceptCertTransfers? "Yes": "No";

	entry.push_back(Pair("acceptcerttransfers", acceptTransfersValue));

	string expireValue = noDifferentStr;
	if(alias.nExpireTime != dbAlias.nExpireTime)
		expireValue = strprintf("%d", alias.nExpireTime);

	entry.push_back(Pair("renewal", expireValue));

	string safetyLevelValue = noDifferentStr;
	if(alias.safetyLevel != dbAlias.safetyLevel)
		safetyLevelValue = alias.safetyLevel;

	entry.push_back(Pair("safetylevel", safetyLevelValue ));

	UniValue msInfo(UniValue::VOBJ);

	string reqsigsValue = noDifferentStr;
	if(alias.multiSigInfo != dbAlias.multiSigInfo)
	{
		msInfo.push_back(Pair("reqsigs", (int)alias.multiSigInfo.nRequiredSigs));
		UniValue msAliases(UniValue::VARR);
		for(int i =0;i<alias.multiSigInfo.vchAliases.size();i++)
		{
			msAliases.push_back(alias.multiSigInfo.vchAliases[i]);
		}
		msInfo.push_back(Pair("reqsigners", msAliases));
		
	}
	else
	{
		msInfo.push_back(Pair("reqsigs", noDifferentStr));
		msInfo.push_back(Pair("reqsigners", noDifferentStr));
	}
	entry.push_back(Pair("multisiginfo", msInfo));

}
UniValue syscoinsignrawtransaction(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("syscoinsignrawtransaction <hexstring>\n"
				"Sign inputs for raw transaction (serialized, hex-encoded) and sends them out to the network if signing is complete\n"
				"<hexstring> The transaction hex string.\n");
	string hexstring = params[0].get_str();
	string doNotSend = params.size() >= 2? params[1].get_str(): "0";
	UniValue res;
	UniValue arraySignParams(UniValue::VARR);
	arraySignParams.push_back(hexstring);
	try
	{
		res = tableRPC.execute("signrawtransaction", arraySignParams);
	}
	catch (UniValue& objError)
	{
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5532 - " + _("Could not sign multisig transaction: ") + find_value(objError, "message").get_str());
	}	
	if (!res.isObject())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5533 - " + _("Could not sign multisig transaction: Invalid response from signrawtransaction"));
	
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
	}
	return res;
}
bool IsMyAlias(const CAliasIndex& alias)
{

	CPubKey aliasPubKey(alias.vchPubKey);
	CSyscoinAddress address(aliasPubKey.GetID());
	if(alias.multiSigInfo.vchAliases.size() > 0)
	{
		CScript inner(alias.multiSigInfo.vchRedeemScript.begin(), alias.multiSigInfo.vchRedeemScript.end());
		return IsMine(*pwalletMain, inner);
	}
	else
		return IsMine(*pwalletMain, address.Get());
}
UniValue aliaslist(const UniValue& params, bool fHelp) {
	if (fHelp || 2 < params.size())
		throw runtime_error("aliaslist [<aliasname>] [<privatekey>]\n"
				"list my own aliases.\n"
				"<aliasname> alias name to use as filter.\n");

	vector<unsigned char> vchAlias;
	if (params.size() >= 1)
		vchAlias = vchFromValue(params[0]);

	string strPrivateKey;
	if(params.size() >= 2)
		strPrivateKey = params[1].get_str();	

	UniValue oRes(UniValue::VARR);
	map<vector<unsigned char>, int> vNamesI;
	map<vector<unsigned char>, UniValue> vNamesO;

	uint256 hash;
	CTransaction tx;
	int pending = 0;
	BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet) {
		pending = 0;
		// get txn hash, read txn index
		hash = item.second.GetHash();
		const CWalletTx &wtx = item.second;
		// skip non-syscoin txns
		if (wtx.nVersion != SYSCOIN_TX_VERSION)
			continue;

		vector<CAliasIndex> vtxPos;
		CAliasIndex alias(wtx);
		if(alias.IsNull())
			continue;
		// skip this alias if it doesn't match the given filter value
		if (vchAlias.size() > 0 && alias.vchAlias != vchAlias)
			continue;
		if (!paliasdb->ReadAlias(alias.vchAlias, vtxPos) || vtxPos.empty())
		{
			pending = 1;
			if(!IsSyscoinTxMine(wtx, "alias"))
				continue;
		}
		else
		{
			alias = vtxPos.back();
			CTransaction tx;
			if (!GetSyscoinTransaction(alias.nHeight, alias.txHash, tx, Params().GetConsensus()))
			{
				pending = 1;
				if(!IsSyscoinTxMine(wtx, "alias"))
					continue;
			}
			else{
				if(!IsMyAlias(alias))
					continue;
			}
		}
		// get last active name only
		if (vNamesI.find(alias.vchAlias) != vNamesI.end() && (alias.nHeight <= vNamesI[alias.vchAlias] || vNamesI[alias.vchAlias] < 0))
			continue;	
		UniValue oName(UniValue::VOBJ);
		if(BuildAliasJson(alias, pending, oName, strPrivateKey))
		{
			vNamesI[alias.vchAlias] = alias.nHeight;
			vNamesO[alias.vchAlias] = oName;	
		}
	}
	BOOST_FOREACH(const PAIRTYPE(vector<unsigned char>, UniValue)& item, vNamesO)
		oRes.push_back(item.second);
	return oRes;
}

UniValue aliasaffiliates(const UniValue& params, bool fHelp) {
	if (fHelp || 1 < params.size())
		throw runtime_error("aliasaffiliates \n"
				"list my own affiliations with merchant offers.\n");
	

	vector<unsigned char> vchOffer;
	UniValue oRes(UniValue::VARR);
	map<vector<unsigned char>, int> vOfferI;
	map<vector<unsigned char>, UniValue> vOfferO;
	{
		uint256 hash;
		CTransaction tx;
		uint64_t nHeight;
		BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet) {
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
			if(!IsSyscoinTxMine(wtx, "offer"))
					continue;
            vchOffer = vvch[0];

			vector<COffer> vtxPos;
			COffer theOffer;
			if (!pofferdb->ReadOffer(vchOffer, vtxPos) || vtxPos.empty())
				continue;
			
			theOffer = vtxPos.back();
			nHeight = theOffer.nHeight;
			// get last active name only
			if (vOfferI.find(vchOffer) != vOfferI.end() && (nHeight < vOfferI[vchOffer] || vOfferI[vchOffer] < 0))
				continue;
			vOfferI[vchOffer] = nHeight;
			// if this is my offer and it is linked go through else skip
			if(theOffer.vchLinkOffer.empty())
				continue;
			// get parent offer
			CTransaction tx;
			COffer linkOffer;
			vector<COffer> offerVtxPos;
			if (!GetTxAndVtxOfOffer( theOffer.vchLinkOffer, linkOffer, tx, offerVtxPos))
				continue;

			for(unsigned int i=0;i<linkOffer.linkWhitelist.entries.size();i++) {
				CTransaction txAlias;
				CAliasIndex theAlias;
				COfferLinkWhitelistEntry& entry = linkOffer.linkWhitelist.entries[i];
				if (GetTxOfAlias(entry.aliasLinkVchRand, theAlias, txAlias))
				{
					if (!IsMyAlias(theAlias))
						continue;
					UniValue oList(UniValue::VOBJ);
					oList.push_back(Pair("offer", stringFromVch(vchOffer)));
					oList.push_back(Pair("alias", stringFromVch(entry.aliasLinkVchRand)));
					oList.push_back(Pair("expires_on",theAlias.nExpireTime));
					oList.push_back(Pair("offer_discount_percentage", strprintf("%d%%", entry.nDiscountPct)));
					vOfferO[vchOffer] = oList;	
				}  
			}
		}
	}

	BOOST_FOREACH(const PAIRTYPE(vector<unsigned char>, UniValue)& item, vOfferO)
		oRes.push_back(item.second);

	return oRes;
}
string GenerateSyscoinGuid()
{
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchGuidRand = CScriptNum(rand).getvch();
	return HexStr(vchGuidRand);
}
UniValue aliasbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "aliasbalance \"alias\" ( minconf )\n"
            "\nReturns the total amount received by the given alias in transactions with at least minconf confirmations.\n"
            "\nArguments:\n"
            "1. \"alias\"  (string, required) The syscoin alias for transactions.\n"
            "2. minconf             (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
       );
	LOCK(cs_main);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAmount nAmount = 0;
	vector<CAliasPayment> vtxPaymentPos;
	CAliasIndex theAlias;
	CTransaction aliasTx;
	if (!GetTxOfAlias(vchAlias, theAlias, aliasTx, true))
		return ValueFromAmount(nAmount);

	CSyscoinAddress addressFrom;
	GetAddress(theAlias, &addressFrom);

	if(!paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos))
		return ValueFromAmount(nAmount);
	
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
	// get all alias inputs and transfer them to the new alias destination
    for (unsigned int i = 0;i<vtxPaymentPos.size();i++)
    {
		const CAliasPayment& aliasPayment = vtxPaymentPos[i];
		coins = view.AccessCoins(aliasPayment.txHash);
		if(coins == NULL)
			continue;
       
		if(!coins->IsAvailable(aliasPayment.nOut))
			continue;
		if (!ExtractDestination(coins->vout[aliasPayment.nOut].scriptPubKey, payDest)) 
			continue;
		destaddy = CSyscoinAddress(payDest);
		if (destaddy.ToString() == addressFrom.ToString())
		{  
			nAmount += coins->vout[aliasPayment.nOut].nValue;
		}		
		
    }
    return  ValueFromAmount(nAmount);
}
int aliasunspent(const vector<unsigned char> &vchAlias, COutPoint& outpoint)
{
	LOCK2(cs_main, mempool.cs);
	vector<CAliasIndex> vtxPos;
	CAliasIndex theAlias;
	CTransaction aliasTx;
	bool isExpired = false;
	if (!GetTxAndVtxOfAlias(vchAlias, theAlias, aliasTx, vtxPos, isExpired, true))
		return 0;
	CSyscoinAddress destaddy;
	GetAddress(theAlias, &destaddy);
	CTxDestination aliasDest;
	CSyscoinAddress prevaddy;
	int numResults = 0;
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	bool found = false;
    for (unsigned int i = 0;i<vtxPos.size();i++)
    {
		const CAliasIndex& alias = vtxPos[i];
		coins = view.AccessCoins(alias.txHash);

		if(coins == NULL)
			continue;
         for (unsigned int j = 0;j<coins->vout.size();j++)
		 {
			int op;
			vector<vector<unsigned char> > vvch;

			if(!coins->IsAvailable(j))
				continue;
			if(!pwalletMain->IsMine(coins->vout[j]))
				continue;
			if(pwalletMain->IsLockedCoin(alias.txHash, j))
				continue;
			
			if(!DecodeAliasScript(coins->vout[j].scriptPubKey, op, vvch) || vvch[0] != theAlias.vchAlias || vvch[1] != theAlias.vchGUID)
				continue;
			if (!ExtractDestination(coins->vout[j].scriptPubKey, aliasDest))
				continue;
			prevaddy = CSyscoinAddress(aliasDest);
			if(destaddy.ToString() != prevaddy.ToString())
				continue;

			numResults++;
			if(!found)
			{
				auto it = mempool.mapNextTx.find(COutPoint(alias.txHash, j));
				if (it != mempool.mapNextTx.end())
					continue;

				outpoint = COutPoint(alias.txHash, j);
				found = true;
			}
			
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
	if (fHelp || 2 < params.size())
		throw runtime_error("aliasinfo <aliasname> [<privatekey>]\n"
				"Show values of an alias.\n");
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	string strPrivateKey;
	if(params.size() >= 2)
		strPrivateKey = params[1].get_str();

	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from alias DB"));

	UniValue oName(UniValue::VOBJ);
	if(!BuildAliasJson(vtxPos.back(), 0, oName, strPrivateKey))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5536 - " + _("Could not find this alias"));
		
	return oName;
}
bool BuildAliasJson(const CAliasIndex& alias, const int pending, UniValue& oName, const string &strPrivKey)
{
	uint64_t nHeight;
	int expired = 0;
	int64_t expires_in = 0;
	int64_t expired_time = 0;
	nHeight = alias.nHeight;
	oName.push_back(Pair("name", stringFromVch(alias.vchAlias)));

	if(alias.safetyLevel >= SAFETY_LEVEL2)
		return false;
	oName.push_back(Pair("value", stringFromVch(alias.vchPublicValue)));
	string strPrivateValue = "";
	if(!alias.vchPrivateValue.empty())
		strPrivateValue = _("Encrypted for alias owner");
	string strDecrypted = "";
	if(DecryptMessage(alias, alias.vchPrivateValue, strDecrypted, strPrivKey))
		strPrivateValue = strDecrypted;		
	oName.push_back(Pair("privatevalue", strPrivateValue));

	string strPassword = "";
	if(!alias.vchPassword.empty())
		strPassword = _("Encrypted for alias owner");
	strDecrypted = "";
	if(DecryptPrivateKey(alias.vchPubKey, alias.vchPassword, strDecrypted, strPrivKey))
		strPassword = strDecrypted;		
	oName.push_back(Pair("password", strPassword));


	oName.push_back(Pair("txid", alias.txHash.GetHex()));
	CSyscoinAddress address;
	GetAddress(alias, &address);
	if(!address.IsValid())
		return false;

	oName.push_back(Pair("address", address.ToString()));

	oName.push_back(Pair("alias_peg", stringFromVch(alias.vchAliasPeg)));

	UniValue balanceParams(UniValue::VARR);
	balanceParams.push_back(stringFromVch(alias.vchAlias));
	const UniValue &resBalance = tableRPC.execute("aliasbalance", balanceParams);
	CAmount nAliasBalance = AmountFromValue(resBalance);
	oName.push_back(Pair("balance", ValueFromAmount(nAliasBalance)));

	oName.push_back(Pair("ismine", IsMyAlias(alias)? true:  false));
	oName.push_back(Pair("safesearch", alias.safeSearch ? "Yes" : "No"));
	oName.push_back(Pair("acceptcerttransfers", alias.acceptCertTransfers ? "Yes" : "No"));
	oName.push_back(Pair("safetylevel", alias.safetyLevel ));
	float ratingAsBuyer = 0;
	if(alias.nRatingCountAsBuyer > 0)
	{
		ratingAsBuyer = alias.nRatingAsBuyer/(float)alias.nRatingCountAsBuyer;
		ratingAsBuyer = floor(ratingAsBuyer * 10) / 10;
	}
	float ratingAsSeller = 0;
	if(alias.nRatingCountAsSeller > 0)
	{
		ratingAsSeller = alias.nRatingAsSeller/(float)alias.nRatingCountAsSeller;
		ratingAsSeller = floor(ratingAsSeller * 10) / 10;
	}
	float ratingAsArbiter = 0;
	if(alias.nRatingCountAsArbiter > 0)
	{
		ratingAsArbiter = alias.nRatingAsArbiter/(float)alias.nRatingCountAsArbiter;
		ratingAsArbiter = floor(ratingAsArbiter * 10) / 10;
	}
	oName.push_back(Pair("buyer_rating", ratingAsBuyer));
	oName.push_back(Pair("buyer_ratingcount", (int)alias.nRatingCountAsBuyer));
	oName.push_back(Pair("buyer_rating_display", strprintf("%.1f/5 (%d %s)", ratingAsBuyer, alias.nRatingCountAsBuyer, _("Votes"))));
	oName.push_back(Pair("seller_rating", ratingAsSeller));
	oName.push_back(Pair("seller_ratingcount", (int)alias.nRatingCountAsSeller));
	oName.push_back(Pair("seller_rating_display", strprintf("%.1f/5 (%d %s)", ratingAsSeller, alias.nRatingCountAsSeller, _("Votes"))));
	oName.push_back(Pair("arbiter_rating", ratingAsArbiter));
	oName.push_back(Pair("arbiter_ratingcount", (int)alias.nRatingCountAsArbiter));
	oName.push_back(Pair("arbiter_rating_display", strprintf("%.1f/5 (%d %s)", ratingAsArbiter, alias.nRatingCountAsArbiter, _("Votes"))));
	if(chainActive[nHeight])
		oName.push_back(Pair("timereceived", chainActive[nHeight]->nTime));
	else
		oName.push_back(Pair("timereceived", 0));
	if(alias.vchAlias != vchFromString("sysrates.peg") && alias.vchAlias != vchFromString("sysban") && alias.vchAlias != vchFromString("syscategory"))
	{
		expired_time = alias.nExpireTime;
		if(expired_time <= chainActive.Tip()->nTime)
		{
			expired = 1;
		}  
		expires_in = expired_time - chainActive.Tip()->nTime;
		if(expires_in < -1)
			expires_in = -1;
	}
	else
	{
		expires_in = -1;
		expired = 0;
		expired_time = -1;
	}
	oName.push_back(Pair("expires_in", expires_in));
	oName.push_back(Pair("expires_on", expired_time));
	oName.push_back(Pair("expired", expired));
	oName.push_back(Pair("pending", pending));
	UniValue msInfo(UniValue::VOBJ);
	msInfo.push_back(Pair("reqsigs", (int)alias.multiSigInfo.nRequiredSigs));
	UniValue msAliases(UniValue::VARR);
	for(int i =0;i<alias.multiSigInfo.vchAliases.size();i++)
	{
		msAliases.push_back(alias.multiSigInfo.vchAliases[i]);
	}
	msInfo.push_back(Pair("reqsigners", msAliases));
	msInfo.push_back(Pair("redeemscript", HexStr(alias.multiSigInfo.vchRedeemScript)));
	oName.push_back(Pair("multisiginfo", msInfo));
	return true;
}
/**
 * [aliashistory description]
 * @param  params [description]
 * @param  fHelp  [description]
 * @return        [description]
 */
UniValue aliashistory(const UniValue& params, bool fHelp) {
	if (fHelp || 1 != params.size())
		throw runtime_error("aliashistory <aliasname>\n"
				"List all stored values of an alias.\n");
	UniValue oRes(UniValue::VARR);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5537 - " + _("Failed to read from alias DB"));

	CAliasIndex txPos2;
	CTransaction tx;
    vector<vector<unsigned char> > vvch;
    int op, nOut;
	string opName;
	BOOST_FOREACH(txPos2, vtxPos) {
		if (!GetSyscoinTransaction(txPos2.nHeight, txPos2.txHash, tx, Params().GetConsensus()))
			continue;

		if(DecodeOfferTx(tx, op, nOut, vvch) )
		{
			opName = offerFromOp(op);
			COffer offer(tx);
			if(offer.accept.bPaymentAck)
				opName += "("+_("acknowledged")+")";
			else if(!offer.accept.feedback.empty())
				opName += "("+_("feedback")+")";

		}
		else if(DecodeMessageTx(tx, op, nOut, vvch) )
			opName = messageFromOp(op);
		else if(DecodeEscrowTx(tx, op, nOut, vvch) )
		{
			CEscrow escrow(tx);
			opName = escrowFromOp(escrow.op);
			if(escrow.bPaymentAck)
				opName += "("+_("acknowledged")+")";
			else if(!escrow.feedback.empty())
				opName += "("+_("feedback")+")";
		}
		else if(DecodeCertTx(tx, op, nOut, vvch) )
			opName = certFromOp(op);
		else if(DecodeAliasTx(tx, op, nOut, vvch) )
			opName = stringFromVch(vvch[0]);
		else
			continue;
		UniValue oName(UniValue::VOBJ);
		oName.push_back(Pair("type", opName));
		if(BuildAliasJson(txPos2, 0, oName))
			oRes.push_back(oName);
	}
	
	return oRes;
}
UniValue generatepublickey(const UniValue& params, bool fHelp) {
	if(!pwalletMain)
		throw runtime_error("No wallet defined!");
	EnsureWalletIsUnlocked();
	CPubKey PubKey = pwalletMain->GenerateNewKey();
	std::vector<unsigned char> vchPubKey(PubKey.begin(), PubKey.end());
	UniValue res(UniValue::VARR);
	res.push_back(HexStr(vchPubKey));
	return res;
}
/**
 * [aliasfilter description]
 * @param  params [description]
 * @param  fHelp  [description]
 * @return        [description]
 */
UniValue aliasfilter(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() > 3)
		throw runtime_error(
				"aliasfilter [[[[[regexp]] from='']] safesearch='Yes']\n"
						"scan and filter aliases\n"
						"[regexp] : apply [regexp] on aliases, empty means all aliases\n"
						"[from] : show results from this GUID [from], empty means first.\n"
						"[aliasfilter] : shows all aliases that are safe to display (not on the ban list)\n"
						"aliasfilter \"\" 5 # list aliases updated in last 5 blocks\n"
						"aliasfilter \"^alias\" # list all aliases starting with \"alias\"\n"
						"aliasfilter 36000 0 0 stat # display stats (number of aliases) on active aliases\n");

	vector<unsigned char> vchAlias;
	string strRegexp;
	string strName;
	bool safeSearch = true;


	if (params.size() > 0)
		strRegexp = params[0].get_str();

	if (params.size() > 1)
	{
		vchAlias = vchFromValue(params[1]);
		strName = params[1].get_str();
	}

	if (params.size() > 2)
		safeSearch = params[2].get_str()=="On"? true: false;

	UniValue oRes(UniValue::VARR);

	
	vector<CAliasIndex> nameScan;
	boost::algorithm::to_lower(strName);
	vchAlias = vchFromString(strName);
	CTransaction aliastx;
	if (!paliasdb->ScanNames(vchAlias, strRegexp, safeSearch, 25, nameScan))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5538 - " + _("Scan failed"));

	BOOST_FOREACH(const CAliasIndex &alias, nameScan) {
		UniValue oName(UniValue::VOBJ);
		if(BuildAliasJson(alias, 0, oName))
			oRes.push_back(oName);
	}


	return oRes;
}
void GetPrivateKeysFromScript(const CScript& script, vector<string> &strKeys)
{
    vector<CTxDestination> addrs;
    int nRequired;
	txnouttype whichType;
    ExtractDestinations(script, whichType, addrs, nRequired);
	BOOST_FOREACH(const CTxDestination& txDest, addrs) {
		CSyscoinAddress address(txDest);
		CKeyID keyID;
		if (!address.GetKeyID(keyID))
			continue;
		CKey vchSecret;
		if (!pwalletMain->GetKey(keyID, vchSecret))
			continue;
		strKeys.push_back(CSyscoinSecret(vchSecret).ToString());
	}
}
UniValue aliaspay(const UniValue& params, bool fHelp) {

    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "aliaspay aliasfrom {\"address\":amount,...} ( minconf \"comment\")\n"
            "\nSend multiple times from an alias. Amounts are double-precision floating point numbers."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
			"1. \"alias\"				(string, required) alias to pay from\n"
            "2. \"amounts\"             (string, required) A json object with aliases and amounts\n"
            "    {\n"
            "      \"address\":amount   (numeric or string) The syscoin alias is the key, the numeric amount (can be string) in " + CURRENCY_UNIT + " is the value\n"
            "      ,...\n"
            "    }\n"
			"3. minconf                 (numeric, optional, default=1) Only use the balance confirmed at least this many times.\n"
            "4. \"comment\"             (string, optional) A comment\n"
            "\nResult:\n"
            "\"transactionid\"          (string) The transaction id for the send. Only 1 transaction is created regardless of \n"
            "                                    the number of addresses.\n"
            "\nExamples:\n"
            "\nSend two amounts to two different addresses\aliases:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"alias2\\\":0.02}\"") +
            "\nSend two amounts to two different addresses setting the comment:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\" \"testing\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    string strFromAlias = params[0].get_str();
	CAliasIndex theAlias;
	CTransaction aliasTx;
	if (!GetTxOfAlias(vchFromString(strFromAlias), theAlias, aliasTx, true))
		throw JSONRPCError(RPC_TYPE_ERROR, "Invalid alias");
    UniValue sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();
    CWalletTx wtx;
    if (params.size() > 3 && !params[3].isNull() && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

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
        CAmount nAmount = AmountFromValue(sendTo[name_]);
        if (nAmount <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
        totalAmount += nAmount;
        CRecipient recipient = {scriptPubKey, nAmount, false};
        vecSend.push_back(recipient);
    }

    EnsureWalletIsUnlocked();
    // Check funds
	UniValue balanceParams(UniValue::VARR);
	balanceParams.push_back(strFromAlias);
	const UniValue &resBalance = tableRPC.execute("aliasbalance", balanceParams);
	CAmount nBalance = AmountFromValue(resBalance);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Alias has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    CAmount nFeeRequired = 0;
    int nChangePosRet = -1;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, nChangePosRet, strFailReason, NULL, true, NULL, 0, true, NULL, 0, true);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}