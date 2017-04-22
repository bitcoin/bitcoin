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
#include "messagecrypter.h"
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
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchAliasPeg, const string &currencyCode, const CRecipient &aliasRecipient, const CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool useOnlyAliasPaymentToFund=true, bool transferAlias=false);
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
		else if(vvchArgs.size() <= 0 || vvchArgs.size() > 4)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5002 - " + _("Alias payment arguments incorrect size");
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
			if(prevCoins->vout.size() <= prevOutput->n || !IsSyscoinScript(prevCoins->vout[prevOutput->n].scriptPubKey, pop, vvch))
			{
				prevCoins = NULL;
				continue;
			}

			if (IsAliasOp(pop, true) && vvchArgs[0] == vvch[0]) {
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
				const int64_t &nTimeExpiry = theAlias.nExpireTime - chainActive[nHeightTmp]->nTime;
				// ensure aliases are good for atleast an hour
				if(nTimeExpiry < 3600)
					theAlias.nExpireTime = chainActive[nHeightTmp]->nTime+3600;
				float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
				if(fYears < 1)
					fYears = 1;
				fee *= powf(2.88,fYears);
			}
			if ((fee-10000) > tx.vout[nDataOut].nValue) 
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5019 - " + _("Transaction does not pay enough fee: ") + ValueFromAmount(tx.vout[nDataOut].nValue).write() + "/" + ValueFromAmount(fee-10000).write();
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
					if(theAlias.vchPasswordSalt.empty())
						theAlias.vchPasswordSalt = dbAlias.vchPasswordSalt;
					if(theAlias.vchPassword.empty())
						theAlias.vchPassword = dbAlias.vchPassword;
					else
						pwChange = true;
					if(theAlias.vchAddress.empty())
						theAlias.vchAddress = dbAlias.vchAddress;
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
					// if transfer
					if(dbAlias.vchAddress != theAlias.vchAddress)
					{
						// if transfer clear pw
						if(!pwChange)
						{
							theAlias.vchPassword.clear();
							theAlias.vchPasswordSalt.clear();
						}
						// make sure xfer to pubkey doesn't point to an alias already, otherwise don't assign pubkey to alias
						// we want to avoid aliases with duplicate addresses
						if (paliasdb->ExistsAddress(theAlias.vchAddress))
						{
							vector<unsigned char> vchMyAlias;
							if (paliasdb->ReadAddress(theAlias.vchAddress, vchMyAlias) && !vchMyAlias.empty() && vchMyAlias != dbAlias.vchAlias)
							{
								errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("An alias already exists with that address, try another public key");
								theAlias = dbAlias;
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
			payment.nHeight = nHeight;
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
	
		CAliasUnprunable aliasUnprunable;
		aliasUnprunable.vchGUID = theAlias.vchGUID;
		aliasUnprunable.nExpireTime = theAlias.nExpireTime;
		if (!dontaddtodb && !paliasdb->WriteAlias(vchAlias, aliasUnprunable, theAlias.vchAddress, vtxPos))
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
					EraseAliasAndAddress(vchMyAlias, txPos.vchAddress);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
bool CAliasDB::GetDBAliases(std::vector<CAliasIndex>& aliases, const uint64_t &nExpireFilter)
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
				pcursor->GetValue(vtxPos);	
				if (vtxPos.empty())
				{
					pcursor->Next();
					continue;
				}
				const CAliasIndex &txPos = vtxPos.back();
				if(chainActive.Height() <= txPos.nHeight || chainActive[txPos.nHeight]->nTime < nExpireFilter)
				{
					pcursor->Next();
					continue;
				}
  				if (chainActive.Tip()->nTime >= txPos.nExpireTime)
				{
					if(vchMyAlias != vchFromString("sysrates.peg") && vchMyAlias != vchFromString("sysban") && vchMyAlias != vchFromString("syscategory"))
					{
						pcursor->Next();
						continue;
					}
				}
				aliases.push_back(txPos);	
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
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, unsigned char& safetyLevel, bool& safeSearch, std::vector<unsigned char> &vchPubKey) {

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
	strAddress = EncodeBase58(alias.vchAddress);
	safetyLevel = alias.safetyLevel;
	safeSearch = alias.safeSearch;
	vchPubKey = alias.vchEncryptionPublicKey;
	return true;
}

bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, unsigned char& safetyLevel, bool& safeSearch, std::vector<unsigned char> &vchPubKey) {

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
	vector<CAliasIndex> vtxPos;
	if (paliasdb && !paliasdb->ReadAlias(vchAlias, vtxPos))
		return false;
	if (vtxPos.size() < 1)
		return false;
	const CAliasIndex &alias = vtxPos.back();
	safetyLevel = alias.safetyLevel;
	safeSearch = alias.safeSearch;
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
	CAmount nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	recipient.nAmount = nFee;
}
void CreateAliasRecipient(const CScript& scriptPubKeyDest, const vector<unsigned char>& vchAlias, const vector<unsigned char>& vchAliasPeg, const uint64_t& nHeight, CRecipient& recipient)
{
	int precision = 0;
	CAmount nFee = 0;
	CScript scriptChangeOrig;
	scriptChangeOrig << CScript::EncodeOP_N(OP_ALIAS_PAYMENT) << vchAlias << vchFromString("1") << OP_DROP << OP_2DROP;
	scriptChangeOrig += scriptPubKeyDest;
	CRecipient recp = {scriptChangeOrig, 0, false};
	recipient = recp;
	int nFeePerByte = getFeePerByte(vchAliasPeg, vchFromString("SYS"), nHeight, precision);
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
		nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
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
		nFee = CWallet::GetMinimumFee(nSize, nTxConfirmTarget, mempool);
	else
		nFee = nFeePerByte * nSize;

	recipient.nAmount = nFee;
	return recipient.nAmount;
}
UniValue aliasauthenticate(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 3 || params.size() > 4)
		throw runtime_error("aliasauthenticate <alias> <password> <salt> [unittest]\n"
		"Authenticates an alias with a provided password/salt combination and returns the private key if successful. Warning: Calling this function over a network can lead to an external party reading your salt/password/private key in plain text.\n");
	vector<unsigned char> vchAlias = vchFromString(params[0].get_str());
	const SecureString &strPassword = params[1].get_str().c_str();
	string strSalt = params[2].get_str();
    string strUnitTest = "";
	if(CheckParam(params, 3))
		strUnitTest = params[3].get_str();	
	CTransaction tx;
	CAliasIndex theAlias;
	if (!GetTxOfAlias(vchAlias, theAlias, tx, true))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5500 - " + _("Could not find an alias with this name"));

	CCrypter crypt;
	if(strPassword.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5501 - " + _("Password cannot be empty"));

	if(!crypt.SetKeyFromPassphrase(strPassword, ParseHex(strSalt), 1, strUnitTest == "Yes"? 2: 1))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5502 - " + _("Could not determine key from password"));

	CKey key;
	key.Set(crypt.chKey, crypt.chKey + (sizeof crypt.chKey), true);
	CPubKey defaultKey = key.GetPubKey();
	if(!defaultKey.IsFullyValid())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5503 - " + _("Generated public key not fully valid"));

	CSyscoinAddress defaultAddress(defaultKey.GetID());
	CPubKey encryptionPubKey(theAlias.vchEncryptionPublicKey);
	if(!encryptionPubKey.IsFullyValid())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5503 - " + _("Encryption public key not fully valid"));

	CSyscoinAddress encryptionAddress(encryptionPubKey.GetID());
	CSyscoinSecret Secret(key);

	bool readonly = false;
	if(EncodeBase58(theAlias.vchAddress) != defaultAddress.ToString())
	{
		CKey PrivateKey = Secret.GetKey();
		const vector<unsigned char> vchPrivateKey(PrivateKey.begin(), PrivateKey.end());
		string strPrivateKey = "";
		CMessageCrypter crypter;
		if(!crypter.Decrypt(stringFromVch(vchPrivateKey), stringFromVch(theAlias.vchEncryptionPrivateKey), strPrivateKey))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5504 - " + _("Password is incorrect"));	
		readonly = true;
	}


	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("privatekey", Secret.ToString()));
	res.push_back(Pair("readonly", readonly));
	return res;

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
UniValue aliasnew(const UniValue& params, bool fHelp) {
	if (fHelp || 4 > params.size() || 12 < params.size())
		throw runtime_error(
		"aliasnew <aliaspeg> <aliasname> <password> <public value> [private value] [safe search=Yes] [accept transfers=Yes] [expire_timestamp] [address] [password_salt] [encryption_privatekey] [encryption_publickey]\n"
						"<aliasname> alias name.\n"
						"<password> used to generate your public/private key that controls this alias. Should be encrypted to publickey.\n"
						"<public value> alias public profile data, 1024 chars max.\n"
						"<private value> alias private profile data, 1024 chars max. Will be private and readable by anyone with encryption_privatekey. Should be encrypted to encryption_publickey.\n"
						"<safe search> set to No if this alias should only show in the search when safe search is not selected. Defaults to Yes (alias shows with or without safe search selected in search lists).\n"
						"<accept transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"	
						"<expire_timestamp> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is FEERATE*(2.88^years). FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"	
						"<address> Address for this alias.\n"		
						"<password_salt> Salt used for key derivation if password is set.\n"
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. Should be encrypted to publickey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias.\n"							
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
	string strPassword = "";
	if(CheckParam(params, 2))
		strPassword = params[2].get_str();
	if(strPassword.size() < 4 && strPassword.size() > 0)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5507 - " + _("Invalid Syscoin Identity. Please enter a password atleast 4 characters long"));
	string strPublicValue = params[3].get_str();
	vchPublicValue = vchFromString(strPublicValue);

	string strPrivateValue = "";
	string strSafeSearch = "Yes";
	if(CheckParam(params, 4))
		strPrivateValue = params[4].get_str();
	if(CheckParam(params, 5))
		strSafeSearch = params[5].get_str();
	string strAcceptCertTransfers = "Yes";

	if(CheckParam(params, 6))
		strAcceptCertTransfers = params[6].get_str();
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	if(CheckParam(params, 7))
		nTime = boost::lexical_cast<uint64_t>(params[7].get_str());
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;

	string strAddress = "";
	if(CheckParam(params, 8))
		strAddress = params[8].get_str();
	string strPasswordSalt = "";

	if(CheckParam(params, 9))
		strPasswordSalt = params[9].get_str();
	
	string strEncryptionPrivateKey = "";
	if(CheckParam(params, 10))
		strEncryptionPrivateKey = params[10].get_str();
	string strEncryptionPublicKey = "";
	if(CheckParam(params, 11))
		strEncryptionPublicKey = params[11].get_str();


	CWalletTx wtx;

	CAliasIndex oldAlias;
	vector<CAliasIndex> vtxPos;
	bool isExpired;
	bool aliasExists = GetVtxOfAlias(vchAlias, oldAlias, vtxPos, isExpired);
	if(aliasExists && !isExpired)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("This alias already exists"));

	const vector<unsigned char> &vchRandAlias = vchFromString(GenerateSyscoinGuid());

    // build alias
    CAliasIndex newAlias;
	newAlias.vchGUID = vchRandAlias;
	newAlias.vchAliasPeg = vchAliasPeg;
	newAlias.vchAlias = vchAlias;
	newAlias.nHeight = chainActive.Tip()->nHeight;
	if(!strEncryptionPublicKey.empty())
		newAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);
	if(!strEncryptionPrivateKey.empty())
		newAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	newAlias.vchPublicValue = vchPublicValue;
	if(!strPrivateValue.empty())
		newAlias.vchPrivateValue = ParseHex(strPrivateValue);
	newAlias.nExpireTime = nTime;
	if(!strPassword.empty())
		newAlias.vchPassword = ParseHex(strPassword);
	if(!strPasswordSalt.empty())
		newAlias.vchPasswordSalt = ParseHex(strPasswordSalt);
	newAlias.safeSearch = strSafeSearch == "Yes"? true: false;
	newAlias.acceptCertTransfers = strAcceptCertTransfers == "Yes"? true: false;
	if(strAddress.empty())
	{
		// generate new address in this wallet if not passed in
		CKey privKey;
		privKey.MakeNewKey(true);
		CPubKey pubKey = privKey.GetPubKey();
		vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
		CSyscoinAddress addressAlias(pubKey.GetID());
		strAddress = addressAlias.ToString();
	}
	DecodeBase58(strAddress, newAlias.vchAddress);
	CSyscoinAddress newAddress;
	CScript scriptPubKeyOrig;
	GetAddress(newAlias, &newAddress, scriptPubKeyOrig);

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
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, vchAlias, newAlias.vchAliasPeg, chainActive.Tip()->nHeight, recipientPayment);
	CScript scriptData;
	
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, newAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
	float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
	if(fYears < 1)
		fYears = 1;
	fee.nAmount *= powf(2.88,fYears);


	vecSend.push_back(fee);
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = true;
	coinControl.fAllowWatchOnly = true;
	bool useOnlyAliasPaymentToFund = false;

	SendMoneySyscoin(vchAlias, vchAliasPeg, "", recipient, recipientPayment, vecSend, wtx, &coinControl, useOnlyAliasPaymentToFund);
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
	if (fHelp || 2 > params.size() || 12 < params.size())
		throw runtime_error(
		"aliasupdate <aliaspeg> <aliasname> [public value] [private value] [safesearch=Yes] [address] [password] [accept_transfers=Yes] [expire_timestamp] [password_salt] [encryption_privatekey] [encryption_publickey]\n"
						"Update and possibly transfer an alias.\n"
						"<aliasname> alias name.\n"
						"<public_value> alias public profile data, 1024 chars max.\n"
						"<private_value> alias private profile data, 1024 chars max. Will be private and readable by anyone with encryption_privatekey.\n"				
						"<address> Address of alias.\n"
						"<password> used to generate your public/private key that controls this alias.\n"						
						"<safesearch> is this alias safe to search. Defaults to Yes, No for not safe and to hide in GUI search queries\n"
						"<accept_transfers> set to No if this alias should not allow a certificate to be transferred to it. Defaults to Yes.\n"		
						"<expire_timestamp> String. Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is 2.88^years. FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 year.\n"		
						"<password_salt> Salt used for key derivation if password is set.\n"
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. If transferring, the key should be encrypted to alias_pubkey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias. Useful if you are changing pub/priv keypair for encryption on this alias.\n"						
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAliasPeg = vchFromString(params[0].get_str());
	if(vchAliasPeg.empty())
		vchAliasPeg = vchFromString("sysrates.peg");
	vector<unsigned char> vchAlias = vchFromString(params[1].get_str());
	string strPrivateValue = "";
	string strPublicValue = "";
	if(CheckParam(params, 2))
		strPublicValue = params[2].get_str();
	
	if(CheckParam(params, 3))
		strPrivateValue = params[3].get_str();
	string strSafeSearch = "";
	if(CheckParam(params, 4))
		strSafeSearch = params[4].get_str();
	CWalletTx wtx;
	CAliasIndex updateAlias;
	string strAddress = "";
	if(CheckParam(params, 5))
		strAddress = params[5].get_str();
	
	string strPassword = "";
	
	if(CheckParam(params, 6))
		strPassword = params[6].get_str();
	string strAcceptCertTransfers = "";
	if(CheckParam(params, 7))
		strAcceptCertTransfers = params[7].get_str();
	
	uint64_t nTime = chainActive.Tip()->nTime+ONE_YEAR_IN_SECONDS;
	bool timeSet = false;
	if(CheckParam(params, 8))
	{
		nTime = boost::lexical_cast<uint64_t>(params[8].get_str());
		timeSet = true;
	}
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->nTime+3600)
		nTime = chainActive.Tip()->nTime+3600;

	string strPasswordSalt = "";
	if(CheckParam(params, 9))
		strPasswordSalt = params[9].get_str();
	

	string strEncryptionPrivateKey = "";
	if(CheckParam(params, 10))
		strEncryptionPrivateKey = params[10].get_str();
	
	string strEncryptionPublicKey = "";
	if(CheckParam(params, 11))
		strEncryptionPublicKey = params[11].get_str();
	

	CTransaction tx;
	CAliasIndex theAlias;
	if (!GetTxOfAlias(vchAlias, theAlias, tx, true))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5518 - " + _("Could not find an alias with this name"));



	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	theAlias.nHeight = chainActive.Tip()->nHeight;
	if(!strPublicValue.empty())
		theAlias.vchPublicValue = vchFromString(strPublicValue);
	if(!strPrivateValue.empty())
		theAlias.vchPrivateValue = ParseHex(strPrivateValue);
	if(!strEncryptionPrivateKey.empty())
		theAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	if(!strEncryptionPublicKey.empty())
		theAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);
	if(!strPassword.empty())
		theAlias.vchPassword = ParseHex(strPassword);
	if(!strPasswordSalt.empty())
		theAlias.vchPasswordSalt = ParseHex(strPasswordSalt);
	if(!strSafeSearch.empty())
		theAlias.safeSearch = strSafeSearch == "Yes"? true: false;
	else
		theAlias.safeSearch = copyAlias.safeSearch;
	if(!strAddress.empty())
		DecodeBase58(strAddress, theAlias.vchAddress);
	theAlias.vchAliasPeg = vchAliasPeg;
	if(timeSet || copyAlias.nExpireTime <= chainActive.Tip()->nTime)
		theAlias.nExpireTime = nTime;
	else
		theAlias.nExpireTime = copyAlias.nExpireTime;
	theAlias.nAccessFlags = copyAlias.nAccessFlags;
	if(strAcceptCertTransfers.empty())
		theAlias.acceptCertTransfers = copyAlias.acceptCertTransfers;
	else
		theAlias.acceptCertTransfers = strAcceptCertTransfers == "Yes"? true: false;
	
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
	scriptPubKey << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, copyAlias.vchAlias, copyAlias.vchAliasPeg, chainActive.Tip()->nHeight, recipientPayment);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, copyAlias.vchAliasPeg, chainActive.Tip()->nHeight, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->nTime;
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
	
	SendMoneySyscoin(vchAlias, copyAlias.vchAliasPeg, "", recipient, recipientPayment, vecSend, wtx, &coinControl, useOnlyAliasPaymentToFund, transferAlias);
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
	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("alias", stringFromVch(alias.vchAlias)));

	if(!alias.vchAliasPeg.empty() && alias.vchAliasPeg != dbAlias.vchAliasPeg)
		entry.push_back(Pair("aliaspeg", stringFromVch(alias.vchAliasPeg)));

	if(!alias.vchPublicValue .empty() && alias.vchPublicValue != dbAlias.vchPublicValue)
		entry.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));
	
	if(!alias.vchPrivateValue.empty() && alias.vchPrivateValue != dbAlias.vchPrivateValue)
		entry.push_back(Pair("privatevalue", HexStr(alias.vchPrivateValue)));
	
	if(!alias.vchPassword.empty() && alias.vchPassword != dbAlias.vchPassword)
		entry.push_back(Pair("password", HexStr(alias.vchPassword)));

	if(EncodeBase58(alias.vchAddress) != EncodeBase58(dbAlias.vchAddress))
		entry.push_back(Pair("address", EncodeBase58(alias.vchAddress))); 

	if(alias.nExpireTime != dbAlias.nExpireTime)
		entry.push_back(Pair("renewal", alias.nExpireTime));

	if(alias.safeSearch != dbAlias.safeSearch)
		entry.push_back(Pair("safesearch", alias.safeSearch));

	if(alias.safetyLevel != dbAlias.safetyLevel)
		entry.push_back(Pair("safetylevel", alias.safetyLevel ));

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
	}
	return res;
}
bool IsMyAlias(const CAliasIndex& alias)
{
	CSyscoinAddress address(EncodeBase58(alias.vchAddress));
	return IsMine(*pwalletMain, address.Get());
}
UniValue aliaslist(const UniValue& params, bool fHelp) {
	if (fHelp || 1 < params.size())
		throw runtime_error("aliaslist [<aliasname>]\n"
				"list my own aliases.\n"
				"<aliasname> alias name to use as filter.\n");

	vector<unsigned char> vchAlias;
	if(CheckParam(params, 0))
		vchAlias = vchFromValue(params[0]);


	UniValue oRes(UniValue::VARR);
	map<vector<unsigned char>, int> vNamesI;
	map<vector<unsigned char>, UniValue> vNamesO;

	uint256 hash;
	CTransaction tx;
	bool pending = false;
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
			pending = true;
			if(!IsSyscoinTxMine(wtx, "alias"))
				continue;
		}
		else
		{
			alias = vtxPos.back();
			CTransaction tx;
			if (!GetSyscoinTransaction(alias.nHeight, alias.txHash, tx, Params().GetConsensus()))
			{
				pending = true;
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
		if(BuildAliasJson(alias, pending, oName))
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
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "aliasbalance \"alias\" ( minconf )\n"
            "\nReturns the total amount received by the given alias in transactions with at least 1 confirmation.\n"
            "\nArguments:\n"
            "1. \"alias\"  (string, required) The syscoin alias for transactions.\n"
       );
	LOCK(cs_main);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	CAmount nAmount = 0;
	vector<CAliasPayment> vtxPaymentPos;
	CAliasIndex theAlias;
	CTransaction aliasTx;
	if (!GetTxOfAlias(vchAlias, theAlias, aliasTx))
		return ValueFromAmount(nAmount);

	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);
	if(!paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos))
		return ValueFromAmount(nAmount);
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
  	int op;
	vector<vector<unsigned char> > vvch;
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
		if(!DecodeAliasScript(coins->vout[aliasPayment.nOut].scriptPubKey, op, vvch) || vvch[0] != theAlias.vchAlias)
			continue;  
		// some outputs are reserved to pay for fees only
		if(vvch.size() > 1 && vvch[1] == vchFromString("1"))
			continue;
		destaddy = CSyscoinAddress(payDest);
		if (destaddy.ToString() == strAddressFrom)
		{  
			COutPoint outp(aliasPayment.txHash, aliasPayment.nOut);
			auto it = mempool.mapNextTx.find(outp);
			if (it != mempool.mapNextTx.end())
				continue;
			nAmount += coins->vout[aliasPayment.nOut].nValue;
		}		
		
    }
    return  ValueFromAmount(nAmount);
}
int aliasselectpaymentcoins(const vector<unsigned char> &vchAlias, const CAmount &nAmount, vector<COutPoint>& outPoints, bool& bIsFunded, CAmount &nRequiredAmount, bool bSelectFeePlacementOnly, bool bSelectAll)
{
	LOCK2(cs_main, mempool.cs);
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	int numCoinsLeft = 0;
	int numResults = 0;
	CAmount nCurrentAmount = 0;
	CAmount nDesiredAmount = nAmount;
	outPoints.clear();
	vector<CAliasPayment> vtxPaymentPos;
	CAliasIndex theAlias;
	CTransaction aliasTx;
	if (!GetTxOfAlias(vchAlias, theAlias, aliasTx))
		return -1;

	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);

	if(!paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos))
		return -1;
	
  	int op;
	vector<vector<unsigned char> > vvch;
	CTxDestination payDest;
	CSyscoinAddress destaddy;
	bIsFunded = false;
	// get all alias inputs and transfer them to the new alias destination
    for (unsigned int i = 0;i<vtxPaymentPos.size();i++)
    {
		const CAliasPayment& aliasPayment = vtxPaymentPos[i];
		coins = view.AccessCoins(aliasPayment.txHash);
		if(coins == NULL)
			continue;
   
		if(!coins->IsAvailable(aliasPayment.nOut))
			continue;
		if(!DecodeAliasScript(coins->vout[aliasPayment.nOut].scriptPubKey, op, vvch) || vvch[0] != theAlias.vchAlias)
			continue;  
		if(vvch.size() > 1)
		{
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
		if (!ExtractDestination(coins->vout[aliasPayment.nOut].scriptPubKey, payDest)) 
			continue;
		destaddy = CSyscoinAddress(payDest);
		if (destaddy.ToString() == strAddressFrom)
		{  
			auto it = mempool.mapNextTx.find(COutPoint(aliasPayment.txHash, aliasPayment.nOut));
			if (it != mempool.mapNextTx.end())
				continue;
			numResults++;
			if(!bIsFunded || bSelectAll)
			{
				outPoints.push_back(COutPoint(aliasPayment.txHash, aliasPayment.nOut));
				nCurrentAmount += coins->vout[aliasPayment.nOut].nValue;
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
	vector<CAliasIndex> vtxPos;
	CAliasIndex theAlias;
	CTransaction aliasTx;
	bool isExpired = false;
	if (!GetTxAndVtxOfAlias(vchAlias, theAlias, aliasTx, vtxPos, isExpired))
		return 0;
	const string &strAddressDest = EncodeBase58(theAlias.vchAddress);
	CTxDestination aliasDest;
	CSyscoinAddress prevaddy;
	int numResults = 0;
	CAmount nCurrentAmount = 0;
	CCoinsViewCache view(pcoinsTip);
	const CCoins *coins;
	bool funded = false;
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
			if(!DecodeAliasScript(coins->vout[j].scriptPubKey, op, vvch) || vvch[0] != theAlias.vchAlias || vvch[1] != theAlias.vchGUID || op == OP_ALIAS_PAYMENT)
				continue;
			if (!ExtractDestination(coins->vout[j].scriptPubKey, aliasDest))
				continue;
			prevaddy = CSyscoinAddress(aliasDest);
			if(strAddressDest != prevaddy.ToString())
				continue;

			numResults++;
			if(!funded)
			{
				auto it = mempool.mapNextTx.find(COutPoint(alias.txHash, j));
				if (it != mempool.mapNextTx.end())
					continue;
				outpoint = COutPoint(alias.txHash, j);
				funded = true;
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
	if (fHelp || 1 > params.size() || 2 < params.size())
		throw runtime_error("aliasinfo <aliasname> [walletless=No]\n"
				"Show values of an alias.\n");
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	string strWalletless = "No";
	if(CheckParam(params, 1))
		strWalletless = params[1].get_str();

	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5535 - " + _("Failed to read from alias DB"));

	UniValue oName(UniValue::VOBJ);
	if(!BuildAliasJson(vtxPos.back(), false, oName, strWalletless))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5536 - " + _("Could not find this alias"));
		
	return oName;
}
bool BuildAliasJson(const CAliasIndex& alias, const bool pending, UniValue& oName, const string &strWalletless)
{
	uint64_t nHeight;
	bool expired = false;
	int64_t expires_in = 0;
	int64_t expired_time = 0;
	nHeight = alias.nHeight;
	oName.push_back(Pair("alias", stringFromVch(alias.vchAlias)));
	
	if(alias.safetyLevel >= SAFETY_LEVEL2)
		return false;
	string strEncryptionPrivateKey = "";
	string strData = "";
	string strPassword = "";
	string strDecrypted = "";
	if(strWalletless == "Yes")
		strEncryptionPrivateKey = HexStr(alias.vchEncryptionPrivateKey);
	else
	{
		DecryptPrivateKey(alias, strEncryptionPrivateKey);
	}
	if(!alias.vchPrivateValue.empty() || !alias.vchPassword.empty())
	{
		if(strWalletless == "Yes")
		{
			strData = HexStr(alias.vchPrivateValue);
			strPassword = HexStr(alias.vchPassword);
		}
		else
		{
			CMessageCrypter crypter;
			if(!strEncryptionPrivateKey.empty())
			{
				if(!alias.vchPrivateValue.empty() && crypter.Decrypt(strEncryptionPrivateKey, stringFromVch(alias.vchPrivateValue), strDecrypted))
					strData = strDecrypted;
				strDecrypted = "";
				if(!alias.vchPassword.empty() && crypter.Decrypt(strEncryptionPrivateKey, stringFromVch(alias.vchPassword), strDecrypted))
					strPassword = strDecrypted;
			}
		}
	}

	oName.push_back(Pair("password", strPassword));
	oName.push_back(Pair("passwordsalt", HexStr(alias.vchPasswordSalt)));
	oName.push_back(Pair("encryption_privatekey", HexStr(strEncryptionPrivateKey)));
	oName.push_back(Pair("encryption_publickey", HexStr(alias.vchEncryptionPublicKey)));
	oName.push_back(Pair("alias_peg", stringFromVch(alias.vchAliasPeg)));
	oName.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));	
	oName.push_back(Pair("privatevalue", strData));
	oName.push_back(Pair("txid", alias.txHash.GetHex()));
	oName.push_back(Pair("address", EncodeBase58(alias.vchAddress)));
	UniValue balanceParams(UniValue::VARR);
	balanceParams.push_back(stringFromVch(alias.vchAlias));
	const UniValue &resBalance = aliasbalance(balanceParams, false);
	oName.push_back(Pair("balance", resBalance));
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
	oName.push_back(Pair("lastupdate_height", nHeight));
	if(alias.vchAlias != vchFromString("sysrates.peg") && alias.vchAlias != vchFromString("sysban") && alias.vchAlias != vchFromString("syscategory"))
	{
		expired_time = alias.nExpireTime;
		if(expired_time <= chainActive.Tip()->nTime)
		{
			expired = true;
		}  
		expires_in = expired_time - chainActive.Tip()->nTime;
		if(expires_in < -1)
			expires_in = -1;
	}
	else
	{
		expires_in = -1;
		expired = false;
		expired_time = -1;
	}
	oName.push_back(Pair("expires_in", expires_in));
	oName.push_back(Pair("expires_on", expired_time));
	oName.push_back(Pair("expired", expired));
	oName.push_back(Pair("pending", pending));
	return true;
}
/**
 * [aliashistory description]
 * @param  params [description]
 * @param  fHelp  [description]
 * @return        [description]
 */
UniValue aliashistory(const UniValue& params, bool fHelp) {
	if (fHelp || 1 > params.size() || 2 < params.size())
		throw runtime_error("aliashistory <aliasname> [walletless=No]\n"
				"List all stored values of an alias.\n");
	UniValue oRes(UniValue::VARR);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	string strWalletless = "No";
	if(CheckParam(params, 1))
		strWalletless = params[1].get_str();	
	vector<CAliasIndex> vtxPos;
	if (!paliasdb->ReadAlias(vchAlias, vtxPos) || vtxPos.empty())
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5537 - " + _("Failed to read from alias DB"));
	vector<CAliasPayment> vtxPaymentPos;
	paliasdb->ReadAliasPayment(vchAlias, vtxPaymentPos);
	
	CAliasIndex txPos;
	CAliasPayment txPaymentPos;
    vector<vector<unsigned char> > vvch;
    int op, nOut;
	string opName;
	UniValue oUpdates(UniValue::VARR);
	UniValue oPayments(UniValue::VARR);
	map<uint256, int> oPaymentDetails;
	BOOST_FOREACH(txPos, vtxPos) {
		CTransaction tx;
		if (!GetSyscoinTransaction(txPos.nHeight, txPos.txHash, tx, Params().GetConsensus()))
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
		else if(DecodeAliasTx(tx, op, nOut, vvch, false) )
		{
			opName = aliasFromOp(op);
			UniValue oName(UniValue::VOBJ);
			oName.push_back(Pair("type", opName));
			CAliasIndex alias(tx);
			if(!alias.IsNull())
			{
				alias.txHash = tx.GetHash();
				if(BuildAliasJson(alias, false, oName, strWalletless))
					oUpdates.push_back(oName);
			}
		}
		else
			continue;
	}
	BOOST_FOREACH(txPaymentPos, vtxPaymentPos) {
		CTransaction tx;		
		if (!GetSyscoinTransaction(txPaymentPos.nHeight, txPaymentPos.txHash, tx, Params().GetConsensus()))
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
		if(DecodeAliasTx(tx, op, nOut, vvch, true) )
		{
			if(oPaymentDetails[tx.GetHash()] == 1 || (vvch.size() >= 2 && vvch[1] == vchFromString("1")))
				continue;
			const string &currencyStr = vvch.size() >= 4? vvch[3]: "";
			const vector<unsigned char> &vchAliasPeg = vvch.size() >= 3? vvch[2]: vchFromString("");
			opName += + " - " + aliasFromOp(op);
			UniValue oName(UniValue::VOBJ);
			oName.push_back(Pair("type", opName));
			CAliasIndex alias(tx);
			if(!alias.IsNull())
			{
				oPaymentDetails[tx.GetHash()] = 1;
				oPayments.push_back(oName);	
				oPayments.push_back(Pair("txid", tx.GetHash().GetHex()));
				oPayments.push_back(Pair("sysamount", ValueFromAmount(tx.GetValueOut()).write()));
				oPayments.push_back(Pair("currency", currencyStr));
				int precision = 2;
				CAmount nPricePerUnit = convertSyscoinToCurrencyCode(vchAliasPeg, currencyStr, tx.GetValueOut(), txPaymentPos.nHeight, precision);
				if(nPricePerUnit == 0)
					oPayments.push_back(Pair("amount", "0"));
				else
					oPayments.push_back(Pair("amount", strprintf("%.*f", precision, ValueFromAmount(nPricePerUnit).get_real())));

				
			}
		}
		else
			continue;
	}
	oRes.push_back(oUpdates);	
	oRes.push_back(oPayments);	
	return oRes;
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


	if(CheckParam(params, 0))
		strRegexp = params[0].get_str();

	if(CheckParam(params, 1))
	{
		vchAlias = vchFromValue(params[1]);
		strName = params[1].get_str();
	}

	if(CheckParam(params, 2))
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
		if(BuildAliasJson(alias, false, oName))
			oRes.push_back(oName);
	}


	return oRes;
}
/* Output some stats about aliases
	- Total number of aliases
	- Last aliases since some arbritrary time
*/
UniValue aliasstats(const UniValue& params, bool fHelp) {
	if (fHelp || 1 < params.size())
		throw runtime_error("aliasstats unixtime=0\n"
				"Show statistics for all non-expired aliases. Only aliases created or updated after unixtime are returned.\n");
	uint64_t nExpireFilter = 0;
	if(CheckParam(params, 0))
		nExpireFilter = params[0].get_int64();
	
	UniValue oAliasStats(UniValue::VOBJ);
	std::vector<CAliasIndex> aliases;
	if (!paliasdb->GetDBAliases(aliases, nExpireFilter))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR ERRCODE: 5539 - " + _("Scan failed"));	
	if(!BuildAliasStatsJson(aliases, oAliasStats))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR ERRCODE: 5540 - " + _("Could not find this alias"));

	return oAliasStats;

}
/* Output some stats about aliases
	- Total number of aliases
*/
bool BuildAliasStatsJson(const std::vector<CAliasIndex> &aliases, UniValue& oAliasStats)
{
	uint32_t totalAliases = aliases.size();
	oAliasStats.push_back(Pair("totalaliases", (int)totalAliases));
	UniValue oAliases(UniValue::VARR);
	BOOST_REVERSE_FOREACH(const CAliasIndex& alias, aliases) {
		UniValue oAlias(UniValue::VOBJ);
		if(!BuildAliasJson(alias, false, oAlias, "Yes"))
			continue;
		oAliases.push_back(oAlias);
	}
	oAliasStats.push_back(Pair("aliases", oAliases)); 
	return true;
}
UniValue aliaspay(const UniValue& params, bool fHelp) {

    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "aliaspay aliasfrom currency {\"address\":amount,...} ( minconf \"comment\")\n"
            "\nSend multiple times from an alias. Amounts are double-precision floating point numbers."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
			"1. \"aliasfrom\"			(string, required) alias to pay from\n"
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

	CRecipient recipient, recipientPayment;
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	CScript scriptPubKeyOrig;
	CSyscoinAddress addressAlias;
	GetAddress(theAlias, &addressAlias, scriptPubKeyOrig);
	CreateAliasRecipient(scriptPubKeyOrig, theAlias.vchAlias, theAlias.vchAliasPeg, chainActive.Tip()->nHeight, recipientPayment);	
	SendMoneySyscoin(theAlias.vchAlias, theAlias.vchAliasPeg, strCurrency, recipient, recipientPayment, vecSend, wtx, &coinControl);
	
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
	