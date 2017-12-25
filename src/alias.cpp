// Copyright (c) 2014 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
//
#include "alias.h"
#include "offer.h"
#include "escrow.h"
#include "cert.h"
#include "offer.h"
#include "asset.h"
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
#include <boost/assign/list_of.hpp>
#include <mongoc.h>
#include "instantx.h"
using namespace std;
CAliasDB *paliasdb = NULL;
COfferDB *pofferdb = NULL;
CCertDB *pcertdb = NULL;
CEscrowDB *pescrowdb = NULL;
CAssetDB *passetdb = NULL;
typedef map<vector<unsigned char>, COutPoint > mapAliasRegistrationsType;
typedef map<vector<unsigned char>, vector<unsigned char> > mapAliasRegistrationsDataType;
mapAliasRegistrationsType mapAliasRegistrations;
mapAliasRegistrationsDataType mapAliasRegistrationData;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend, bool transferAlias=false);
extern int nIndexPort;
mongoc_client_t *client = NULL;
mongoc_database_t *database = NULL;
mongoc_collection_t *alias_collection = NULL;
mongoc_collection_t *aliashistory_collection = NULL;
mongoc_collection_t *aliastxhistory_collection = NULL;
mongoc_collection_t *offer_collection = NULL;
mongoc_collection_t *offerhistory_collection = NULL;
mongoc_collection_t *escrow_collection = NULL;
mongoc_collection_t *escrowbid_collection = NULL;
mongoc_collection_t *cert_collection = NULL;
mongoc_collection_t *certhistory_collection = NULL;
mongoc_collection_t *asset_collection = NULL;
mongoc_collection_t *assethistory_collection = NULL;
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
uint64_t GetAliasExpiration(const CAliasIndex& alias) {
	// dont prune by default, set nHeight to future time
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	// if service alias exists in unprunable db (this should always exist for any alias that ever existed) then get the last expire height set for this alias and check against it for pruning
	if (paliasdb && paliasdb->ReadAliasUnprunable(alias.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;
	return nTime;
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
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
			return true;
		}
	}
	else if(offer.UnserializeFromData(vchData, vchHash))
	{
		uint256 txid;
		if (!pofferdb || !pofferdb->ReadOfferLastTXID(offer.vchOffer, txid))
		{			
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
			return true;
		}
		if (!pofferdb->ReadOffer(CNameTXIDTuple(offer.vchOffer, txid), offer))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
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
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
			return true;
		}
		if (!pcertdb->ReadCert(CNameTXIDTuple(cert.vchCert, txid), cert))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
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
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
			return true;
		}
		if (!pescrowdb->ReadEscrow(CNameTXIDTuple(escrow.vchEscrow, txid), escrow))
		{
			// setting to the tip means we don't prune this data, we keep it
			nTime = chainActive.Tip()->GetMedianTimePast() + 1;
			return true;
		}
		nTime = GetEscrowExpiration(escrow);
		return true;
	}
	return false;
}
bool IsSysServiceExpired(const uint64_t &nTime)
{
	if(!chainActive.Tip())
		return false;
	return (chainActive.Tip()->GetMedianTimePast() >= nTime);

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
	else if (DecodeAssetScript(scriptPubKey, op, vvchArgs))
		return true;
	return false;
}
bool RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut)
{
	if (!RemoveAliasScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
		if (!RemoveOfferScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
			if (!RemoveCertScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
				if (!RemoveEscrowScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
					if (!RemoveAssetScriptPrefix(scriptPubKeyIn, scriptPubKeyOut))
						return false;
	return true;
					
}
float getEscrowFee()
{
	// 0.05% escrow fee
	return 0.005;
}
int getFeePerByte(const uint64_t &paymentOptionMask)
{   
	if (IsPaymentOptionInMask(paymentOptionMask, PAYMENTOPTION_BTC))
		return 250;
	else  if (IsPaymentOptionInMask(paymentOptionMask, PAYMENTOPTION_SYS))
		return 25;
	else  if (IsPaymentOptionInMask(paymentOptionMask, PAYMENTOPTION_ZEC))
		return 25;
	return 25;
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
			|| op == OP_ALIAS_UPDATE;
}
string aliasFromOp(int op) {
	switch (op) {
	case OP_ALIAS_UPDATE:
		return "aliasupdate";
	case OP_ALIAS_ACTIVATE:
		return "aliasactivate";
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

bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, bool fJustCheck, int nHeight, string &errorMessage, bool &bDestCheckFailed, bool dontaddtodb) {
	if (tx.IsCoinBase() && !fJustCheck && !dontaddtodb)
	{
		LogPrintf("*Trying to add alias in coinbase transaction, skipping...");
		return true;
	}
	// alias registration has args size of 1 we don't care to validate it until the activation comes in with args size of 4
	if (vvchArgs.size() < 4)
		return true;
	if (fDebug && !dontaddtodb)
		LogPrintf("*** ALIAS %d %d op=%s %s nOut=%d %s\n", nHeight, chainActive.Tip()->nHeight, aliasFromOp(op).c_str(), tx.GetHash().ToString().c_str(), nOut, fJustCheck ? "JUSTCHECK" : "BLOCK");
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
	if(fJustCheck)
	{
		
		if(vvchArgs.size() != 4)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5001 - " + _("Alias arguments incorrect size");
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
	CCoins pprevCoins;
	int prevOutputIndex = 0;
	if(fJustCheck || op != OP_ALIAS_ACTIVATE)
	{
		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int pop;
			CCoins prevCoins;
			// ensure inputs are unspent when doing consensus check to add to block
			if(!GetUTXOCoins(tx.vin[i].prevout, prevCoins) || !IsSyscoinScript(prevCoins.vout[tx.vin[i].prevout.n].scriptPubKey, pop, vvch))
			{
				continue;
			}
			if (IsAliasOp(pop) && (op == OP_ALIAS_ACTIVATE || (vvchArgs.size() > 1 && vvchArgs[0] == vvch[0] && vvchArgs[1] == vvch[1]))) {
				prevOp = pop;
				vvchPrevArgs = vvch;
				pprevCoins = prevCoins;
				prevOutputIndex = tx.vin[i].prevout.n;
				break;
			}
		}
		if(vvchArgs.size() >= 4 && !vvchArgs[3].empty())
		{
			bool bWitnessSigFound = false;
			for (unsigned int i = 0; i < tx.vin.size(); i++) {
				vector<vector<unsigned char> > vvch;
				int pop;
				CCoins prevCoins;
				if(!GetUTXOCoins(tx.vin[i].prevout, prevCoins) || !IsSyscoinScript(prevCoins.vout[tx.vin[i].prevout.n].scriptPubKey, pop, vvch))
				{
					continue;
				}
				// match 4th element in scriptpubkey of alias update with witness input scriptpubkey, if names match then sig is provided
				if (IsAliasOp(pop) && vvchArgs[3] == vvch[0]) {
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
		if(theAlias.vchPublicValue.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5005 - " + _("Alias public value too big");
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
				if (vvchArgs.size() <= 1 || theAlias.vchGUID != vvchArgs[1])
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5010 - " + _("Alias input guid mismatch");
					return error(errorMessage.c_str());
				}
				if(theAlias.vchAddress.empty())
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5011 - " + _("Alias address cannot be empty");
					return error(errorMessage.c_str());
				}
				break;
			case OP_ALIAS_UPDATE:
				if (!IsAliasOp(prevOp))
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5012 - " + _("Alias input to this transaction not found");
					return error(errorMessage.c_str());
				}
				if (!theAlias.IsNull())
				{
					if (theAlias.vchAlias != vvchArgs[0])
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
		// whitelist alias updates don't update expiry date
		if(!vchData.empty() && theAlias.offerWhitelist.entries.empty() && theAlias.nExpireTime > 0)
		{
			CAmount fee = GetDataFee(tx.vout[nDataOut].scriptPubKey);
			float fYears;
			//  get expire time and figure out if alias payload pays enough fees for expiry
			int nHeightTmp = nHeight;
			if(nHeightTmp > chainActive.Height())
				nHeightTmp = chainActive.Height();
			uint64_t nTimeExpiry = theAlias.nExpireTime - chainActive[nHeightTmp]->GetMedianTimePast();
			// ensure aliases are good for atleast an hour
			if (nTimeExpiry < 3600) {
				nTimeExpiry = 3600;
				theAlias.nExpireTime = chainActive[nHeightTmp]->GetMedianTimePast() + 3600;
			}
			fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
			if(fYears < 1)
				fYears = 1;
			fee *= powf(2.88,fYears);
			
			if ((fee-10000) > tx.vout[nDataOut].nValue) 
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5019 - " + _("Transaction does not pay enough fee: ") + ValueFromAmount(tx.vout[nDataOut].nValue).write() + "/" + ValueFromAmount(fee-10000).write() + "/" + boost::lexical_cast<string>(fYears) + " years.";
				return true;
			}
		}
		string newAddress = "";
		bool theAliasNull = theAlias.IsNull();
		if (op == OP_ALIAS_UPDATE)
		{
			CTxDestination aliasDest;
			if (vvchPrevArgs.size() <= 0 || vvchPrevArgs[0] != vvchArgs[0] || vvchPrevArgs[1] != vvchArgs[1] || !pprevCoins.IsAvailable(prevOutputIndex) || !ExtractDestination(pprevCoins.vout[prevOutputIndex].scriptPubKey, aliasDest))
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5020 - " + _("Cannot extract destination of alias input");
				if (!theAliasNull)
					theAlias = dbAlias;
				else
					bDestCheckFailed = true;
			}
			else
			{
				CSyscoinAddress prevaddy(aliasDest);
				if (EncodeBase58(dbAlias.vchAddress) != prevaddy.ToString())
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5021 - " + _("You are not the owner of this alias");
					if (!theAliasNull)
						theAlias = dbAlias;
					else
						bDestCheckFailed = true;
				}
			}

			if (dbAlias.vchGUID != vvchArgs[1] || dbAlias.vchAlias != vvchArgs[0])
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5022 - " + _("Cannot edit this alias, guid mismatch");
				if (!theAliasNull)
					theAlias = dbAlias;
				else
					bDestCheckFailed = true;
			}
			if (!theAliasNull) {
				if (dbAlias.nHeight >= nHeight)
				{
					errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Alias was already updated in this block.");
					return true;
				}
				COfferLinkWhitelist whiteList;
				// if updating whitelist, we dont allow updating any alias details
				if (theAlias.offerWhitelist.entries.size() > 0)
				{
					whiteList = theAlias.offerWhitelist;
					theAlias = dbAlias;
				}
				else
				{
					// can't edit whitelist through aliasupdate
					theAlias.offerWhitelist = dbAlias.offerWhitelist;
					if (theAlias.vchPublicValue.empty())
						theAlias.vchPublicValue = dbAlias.vchPublicValue;
					if (theAlias.vchEncryptionPrivateKey.empty())
						theAlias.vchEncryptionPrivateKey = dbAlias.vchEncryptionPrivateKey;
					if (theAlias.vchEncryptionPublicKey.empty())
						theAlias.vchEncryptionPublicKey = dbAlias.vchEncryptionPublicKey;
					if (theAlias.nExpireTime == 0)
						theAlias.nExpireTime = dbAlias.nExpireTime;
					if (theAlias.vchAddress.empty())
						theAlias.vchAddress = dbAlias.vchAddress;
					else
						newAddress = EncodeBase58(theAlias.vchAddress);
					theAlias.vchGUID = dbAlias.vchGUID;
					theAlias.vchAlias = dbAlias.vchAlias;
					// if transfer
					if (dbAlias.vchAddress != theAlias.vchAddress)
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
						if (dbAlias.nAccessFlags < 2)
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
						if (dbAlias.nAccessFlags < 1)
						{
errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5026 - " + _("Cannot edit this alias. It is view-only.");
theAlias = dbAlias;
						}
					}
					if (theAlias.nAccessFlags > dbAlias.nAccessFlags)
					{
						errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot modify for more lenient access. Only tighter access level can be granted.");
						theAlias = dbAlias;
					}
				}


				// if the txn whitelist entry exists (meaning we want to remove or add)
				if (whiteList.entries.size() >= 1)
				{
					if (whiteList.entries.size() > 20)
					{
						errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 1094 -" + _("Too many affiliates for this whitelist, maximum 20 entries allowed");
						theAlias.offerWhitelist.SetNull();
					}
					// special case we use to remove all entries
					else if (whiteList.entries.size() == 1 && whiteList.entries.begin()->second.nDiscountPct == 127)
					{
						if (theAlias.offerWhitelist.entries.empty())
						{
							errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 1093 - " + _("Whitelist is already empty");
						}
						else
							theAlias.offerWhitelist.SetNull();
					}
					else
					{
						for (auto const &it : whiteList.entries)
						{
							COfferLinkWhitelistEntry entry;
							const COfferLinkWhitelistEntry& newEntry = it.second;
							if (newEntry.nDiscountPct > 99) {
								errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 1094 -" + _("Whitelist discount must be between 0 and 99");
								continue;
							}
							// the stored whitelist has this entry (and its the same) then we want to remove this entry
							if (theAlias.offerWhitelist.GetLinkEntryByHash(newEntry.aliasLinkVchRand, entry) && newEntry == entry)
							{
								theAlias.offerWhitelist.RemoveWhitelistEntry(newEntry.aliasLinkVchRand);
							}
							// we want to add it to the whitelist
							else
							{
								if (theAlias.offerWhitelist.entries.size() < 20)
									theAlias.offerWhitelist.PutWhitelistEntry(newEntry);
								else
								{
									errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 1094 -" + _("Too many affiliates for this whitelist, maximum 20 entries allowed");
								}
							}
						}
					}
				}
			}
		}
		else if (op == OP_ALIAS_ACTIVATE)
		{
			if (!dbAlias.IsNull())
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5028 - " + _("Trying to renew an alias that isn't expired");
				return true;
			}
			if (paliasdb->ExistsAddress(theAlias.vchAddress) && chainActive.Tip()->GetMedianTimePast() < GetAliasExpiration(theAlias))
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5028 - " + _("Trying to create an alias with an address of an alias that isn't expired");
				return true;
			}
		}
		if (!dontaddtodb) {
			int nOutHistory;
			int opHistory;
			char type;
			vector<vector<unsigned char> > vvchHistory;
			if (DecodeAndParseSyscoinTx(tx, opHistory, nOutHistory, vvchHistory, type)) {
				string strResponseEnglish = "";
				string strResponseGUID = "";
				string strResponse = GetSyscoinTransactionDescription(opHistory, vvchHistory, tx, strResponseEnglish, strResponseGUID, type);
				if (strResponse != "") {
					string user1 = stringFromVch(vvchArgs[0]);
					string user2 = "";
					string user3 = "";
					if (type == ALIAS && opHistory == OP_ALIAS_UPDATE) {
						if (!newAddress.empty())
							user2 = newAddress;
					}
					else if (type == CERT && opHistory == OP_CERT_TRANSFER) {
						CCert cert(tx);
						user2 = stringFromVch(cert.linkAliasTuple.first);
					}
					else if (type == ASSET && (opHistory == OP_ASSET_TRANSFER || opHistory == OP_ASSET_SEND)) {
						CAsset asset(tx);
						user2 = stringFromVch(asset.linkAliasTuple.first);
					}
					else if (type == OFFER && opHistory == OP_OFFER_UPDATE)
					{
						COffer offer(tx);
						if (!offer.linkAliasTuple.IsNull())
							user2 = stringFromVch(offer.linkAliasTuple.first);
					}
					else if (type == ESCROW && IsEscrowOp(opHistory)) {
						CEscrow escrow(tx);
						const string &buyer = stringFromVch(escrow.buyerAliasTuple.first);
						const string &seller = stringFromVch(escrow.sellerAliasTuple.first);
						const string &arbiter = stringFromVch(escrow.arbiterAliasTuple.first);
						user1 = buyer;
						user2 = seller;
						user3 = arbiter;
					}
					paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, strResponseGUID);
				}
			}

		}
		if (!theAliasNull)
		{
			theAlias.nHeight = nHeight;
			theAlias.txHash = tx.GetHash();

			CAliasUnprunable aliasUnprunable;
			aliasUnprunable.vchGUID = theAlias.vchGUID;
			aliasUnprunable.nExpireTime = theAlias.nExpireTime;
			if (!dontaddtodb && !paliasdb->WriteAlias(aliasUnprunable, theAlias.vchAddress, theAlias, op))
			{
				errorMessage = "SYSCOIN_ALIAS_CONSENSUS_ERROR: ERRCODE: 5034 - " + _("Failed to write to alias DB");
				return error(errorMessage.c_str());
			}

			if (fDebug)
				LogPrintf(
					"CONNECTED ALIAS: name=%s  op=%s  hash=%s  height=%d\n",
					stringFromVch(vchAlias).c_str(),
					aliasFromOp(op).c_str(),
					tx.GetHash().ToString().c_str(), nHeight);
		}
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
				pcursor->GetValue(txPos);
  				if (chainActive.Tip()->GetMedianTimePast() >= txPos.nExpireTime)
				{
					servicesCleaned++;
					EraseAlias(aliasTuple, true);
				} 
				
            }
			else if (pcursor->GetKey(key) && key.first == "namea") {
				CNameTXIDTuple aliasTuple;
				CAliasIndex alias;
				pcursor->GetValue(aliasTuple);
				if (GetAlias(aliasTuple.first, alias) && chainActive.Tip()->GetMedianTimePast() >= alias.nExpireTime)
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
	if (passetdb != NULL)
		passetdb->CleanupDatabase(numServicesCleaned);
	if (paliasdb != NULL) 
		paliasdb->CleanupDatabase(numServicesCleaned);
	
	if(paliasdb != NULL)
	{
		if (!paliasdb->Flush())
			LogPrintf("Failed to write to alias database!");
	}
	if(pofferdb != NULL)
	{
		if (!pofferdb->Flush())
			LogPrintf("Failed to write to offer database!");
	}
	if(pcertdb != NULL)
	{
		if (!pcertdb->Flush())
			LogPrintf("Failed to write to cert database!");
	}
	if(pescrowdb != NULL)
	{
		if (!pescrowdb->Flush())
			LogPrintf("Failed to write to escrow database!");
	}
	if (passetdb != NULL)
	{
		if (!passetdb->Flush())
			LogPrintf("Failed to write to asset database!");
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
	
	if (chainActive.Tip()->GetMedianTimePast() >= txPos.nExpireTime) {
		txPos.SetNull();
		string alias = stringFromVch(vchAlias);
		return false;
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

bool GetAliasOfTx(const CTransaction& tx, vector<unsigned char>& name) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return false;
	vector<vector<unsigned char> > vvchArgs;
	int op;
	int nOut;

	bool good = DecodeAliasTx(tx, op, nOut, vvchArgs);
	if (!good)
		return error("GetAliasOfTx() : could not decode a syscoin tx");

	switch (op) {
	case OP_ALIAS_ACTIVATE:
	case OP_ALIAS_UPDATE:
		name = vvchArgs[0];
		return true;
	}
	return false;
}
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char& type)
{
	return  
		DecodeAndParseCertTx(tx, op, nOut, vvch, type)
		|| DecodeAndParseOfferTx(tx, op, nOut, vvch, type)
		|| DecodeAndParseEscrowTx(tx, op, nOut, vvch, type)
		|| DecodeAndParseAliasTx(tx, op, nOut, vvch, type)
		|| DecodeAndParseAssetTx(tx, op, nOut, vvch, type);
}
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	CAliasIndex alias;
	bool decode = DecodeAliasTx(tx, op, nOut, vvch);
	if(decode)
	{
		bool parse = alias.UnserializeFromTx(tx);
		if (decode && parse) {
			type = ALIAS;
			return true;
		} 
	}
	return false;
}
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch) {
	bool found = false;


	// Strict check - bug disallowed
	for (unsigned int i = 0; i < tx.vout.size(); i++) {
		const CTxOut& out = tx.vout[i];
		vector<vector<unsigned char> > vvchRead;
		if (DecodeAliasScript(out.scriptPubKey, op, vvchRead)) {
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
	if (op != OP_SYSCOIN_ALIAS)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsAliasOp(op))
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
	return found;
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
	CAmount nFee = 3 * minRelayTxFee.GetFee(nSize);
	recipient.nAmount = nFee;
}
void CreateAliasRecipient(const CScript& scriptPubKeyDest, CRecipient& recipient)
{
	CAmount nFee = 0;
	CRecipient recp = { scriptPubKeyDest, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
	size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	nFee = 3 * minRelayTxFee.GetFee(nSize);
	recipient.nAmount = nFee;
}
void CreateFeeRecipient(CScript& scriptPubKey, const vector<unsigned char>& data, CRecipient& recipient)
{
	CAmount nFee = 0;
	// add hash to data output (must match hash in inputs check with the tx scriptpubkey hash)
    uint256 hash = Hash(data.begin(), data.end());
    vector<unsigned char> vchHashRand = vchFromValue(hash.GetHex());
	scriptPubKey << vchHashRand;
	CRecipient recp = {scriptPubKey, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
	size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	nFee = 3 * minRelayTxFee.GetFee(nSize);
	recipient.nAmount = nFee;
}
CAmount GetDataFee(const CScript& scriptPubKey)
{
	CAmount nFee = 0;
	CRecipient recipient;
	CRecipient recp = {scriptPubKey, 0, false};
	recipient = recp;
	CTxOut txout(0,	recipient.scriptPubKey);
    size_t nSize = txout.GetSerializeSize(SER_DISK,0)+148u;
	nFee = 3 * minRelayTxFee.GetFee(nSize);
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
void setupAliasHistoryIndexes() {
	bson_t keys;
	const char *collection_name = "aliashistory";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "alias", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupAliasTxHistoryIndexes() {
	bson_t keys;
	const char *collection_name = "aliastxhistory";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "user1", 1);
	BSON_APPEND_INT32(&keys, "user2", 1);
	BSON_APPEND_INT32(&keys, "user3", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupOfferIndexes() {
	bson_t keys;
	const char *collection_name = "offer";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "category", 1);
	BSON_APPEND_INT32(&keys, "paymentoptions", 1);
	BSON_APPEND_INT32(&keys, "alias", 1);
	BSON_APPEND_INT32(&keys, "offertype", 1);
	BSON_APPEND_UTF8(&keys, "title", "text");
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupOfferHistoryIndexes() {
	bson_t keys;
	const char *collection_name = "offerhistory";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "offer", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupEscrowIndexes() {
	bson_t keys;
	const char *collection_name = "escrow";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "offer", 1);
	BSON_APPEND_INT32(&keys, "seller", 1);
	BSON_APPEND_INT32(&keys, "arbiter", 1);
	BSON_APPEND_INT32(&keys, "buyer", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupEscrowBidIndexes() {
	bson_t keys;
	const char *collection_name = "escrowbid";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "offer", 1);
	BSON_APPEND_INT32(&keys, "escrow", 1);
	BSON_APPEND_INT32(&keys, "bidder", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupCertIndexes() {
	bson_t keys;
	const char *collection_name = "cert";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "category", 1);
	BSON_APPEND_INT32(&keys, "alias", 1);
	BSON_APPEND_UTF8(&keys, "title", "text");
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupCertHistoryIndexes() {
	bson_t keys;
	const char *collection_name = "certhistory";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "cert", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupAssetIndexes() {
	bson_t keys;
	const char *collection_name = "asset";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "category", 1);
	BSON_APPEND_INT32(&keys, "alias", 1);
	BSON_APPEND_UTF8(&keys, "title", "text");
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupAssetHistoryIndexes() {
	bson_t keys;
	const char *collection_name = "assethistory";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "asset", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void setupFeedbackIndexes() {
	bson_t keys;
	const char *collection_name = "feedback";
	char *index_name;
	bson_t reply;
	bson_error_t error;
	bool r;
	bson_t *create_indexes;

	bson_init(&keys);
	BSON_APPEND_INT32(&keys, "offer", 1);
	BSON_APPEND_INT32(&keys, "escrow", 1);
	BSON_APPEND_INT32(&keys, "feedbackuserfrom", 1);
	BSON_APPEND_INT32(&keys, "feedbackuserto", 1);
	index_name = mongoc_collection_keys_to_index_string(&keys);
	create_indexes = BCON_NEW("createIndexes",
		BCON_UTF8(collection_name),
		"indexes",
		"[",
		"{",
		"key",
		BCON_DOCUMENT(&keys),
		"name",
		BCON_UTF8(index_name),
		"}",
		"]");

	r = mongoc_database_write_command_with_opts(
		database, create_indexes, NULL /* opts */, &reply, &error);

	if (!r) {
		LogPrintf("Error in createIndexes: %s\n", error.message);
	}
	bson_destroy(&keys);
	bson_free(index_name);
	bson_destroy(&reply);
	bson_destroy(create_indexes);
}
void startMongoDB(){
	// SYSCOIN
	nIndexPort = GetArg("-indexport", DEFAULT_INDEXPORT);
	string uri_str = strprintf("mongodb://127.0.0.1:%d", nIndexPort);
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
		aliashistory_collection = mongoc_client_get_collection(client, "syscoindb", "aliashistory");
		aliastxhistory_collection = mongoc_client_get_collection(client, "syscoindb", "aliastxhistory");
		offer_collection = mongoc_client_get_collection(client, "syscoindb", "offer");
		offerhistory_collection = mongoc_client_get_collection(client, "syscoindb", "offerhistory");
		escrow_collection = mongoc_client_get_collection(client, "syscoindb", "escrow");
		escrowbid_collection = mongoc_client_get_collection(client, "syscoindb", "escrowbid");
		cert_collection = mongoc_client_get_collection(client, "syscoindb", "cert");
		certhistory_collection = mongoc_client_get_collection(client, "syscoindb", "certhistory");
		feedback_collection = mongoc_client_get_collection(client, "syscoindb", "feedback");
		asset_collection = mongoc_client_get_collection(client, "syscoindb", "asset");
		assethistory_collection = mongoc_client_get_collection(client, "syscoindb", "assethistory");
		BSON_ASSERT(alias_collection);
		BSON_ASSERT(aliashistory_collection);
		BSON_ASSERT(aliastxhistory_collection);
		BSON_ASSERT(offer_collection);
		BSON_ASSERT(offerhistory_collection);
		BSON_ASSERT(escrow_collection);
		BSON_ASSERT(escrowbid_collection);
		BSON_ASSERT(cert_collection);
		BSON_ASSERT(certhistory_collection);
		BSON_ASSERT(feedback_collection);
		BSON_ASSERT(asset_collection);
		BSON_ASSERT(assethistory_collection);
		setupAliasHistoryIndexes();
		setupOfferIndexes();
		setupOfferHistoryIndexes();
		setupEscrowIndexes();
		setupEscrowBidIndexes();
		setupCertIndexes();
		setupCertHistoryIndexes();
		setupFeedbackIndexes();
		setupAssetIndexes();
		setupAssetHistoryIndexes();
		LogPrintf("Mongo c client loaded!\n");
	}
	else
	{
		if (fDebug)
			LogPrintf("Could not detect mongo db index port, please ensure you specify -indexport in your syscoin.conf file or as a command line argument\n");
	}
}
void CAliasDB::WriteAliasIndex(const CAliasIndex& alias, const int &op) {
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
	oName.push_back(Pair("_id", stringFromVch(alias.vchAlias)));
	oName.push_back(Pair("address", EncodeBase58(alias.vchAddress)));
	update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!update || !mongoc_collection_update(alias_collection, update_flags, selector, update, write_concern, &error)) {
		LogPrintf("MONGODB ALIAS UPDATE ERROR: %s\n", error.message);
	}
	if(update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
	WriteAliasIndexHistory(alias, op);
}
void CAliasDB::WriteAliasIndexHistory(const CAliasIndex& alias, const int &op) {
	if (!aliashistory_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	selector = BCON_NEW("_id", BCON_UTF8(alias.txHash.GetHex().c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	BuildAliasIndexerHistoryJson(alias, oName);
	oName.push_back(Pair("op", aliasFromOp(op)));
	update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!update || !mongoc_collection_update(aliashistory_collection, update_flags, selector, update, write_concern, &error)) {
		LogPrintf("MONGODB ALIAS HISTORY ERROR: %s\n", error.message);
	}
	if (update)
		bson_destroy(update);
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);

}
void CAliasDB::EraseAliasIndexHistory(const std::vector<unsigned char>& vchAlias, bool cleanup) {
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("alias", BCON_UTF8(stringFromVch(vchAlias).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(aliashistory_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB ALIAS HISTORY REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAliasDB::EraseAliasIndex(const std::vector<unsigned char>& vchAlias, bool cleanup) {
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("_id", BCON_UTF8(stringFromVch(vchAlias).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(alias_collection, remove_flags, selector, cleanup? NULL: write_concern, &error)) {
		LogPrintf("MONGODB ALIAS REMOVE ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
	EraseAliasIndexHistory(vchAlias, cleanup);
	EraseAliasIndexTxHistory(vchAlias, cleanup);
}
bool BuildAliasIndexerTxHistoryJson(const string &user1, const string &user2, const string &user3, const uint256 &txHash, const uint64_t& nHeight, const string &type, const string &guid, UniValue& oName)
{
	oName.push_back(Pair("_id", txHash.GetHex()));
	oName.push_back(Pair("user1", user1));
	oName.push_back(Pair("user2", user2));
	oName.push_back(Pair("user3", user3));
	oName.push_back(Pair("type", type));
	oName.push_back(Pair("guid", guid));
	oName.push_back(Pair("height", nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= nHeight) {
		CBlockIndex *pindex = chainActive[nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oName.push_back(Pair("time", nTime));
	return true;
}
void CAliasDB::WriteAliasIndexTxHistory(const string &user1, const string &user2, const string &user3, const uint256 &txHash, const uint64_t& nHeight, const string &type, const string &guid) {
	if (!aliastxhistory_collection)
		return;
	bson_error_t error;
	bson_t *update = NULL;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	UniValue oName(UniValue::VOBJ);
	write_concern = mongoc_write_concern_new();
	selector = BCON_NEW("_id", BCON_UTF8(txHash.GetHex().c_str()));
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	mongoc_update_flags_t update_flags;
	update_flags = (mongoc_update_flags_t)(MONGOC_UPDATE_NO_VALIDATE | MONGOC_UPDATE_UPSERT);
	BuildAliasIndexerTxHistoryJson(user1, user2, user3, txHash, nHeight, type, guid, oName);
	update = bson_new_from_json((unsigned char *)oName.write().c_str(), -1, &error);
	if (!update || !mongoc_collection_update(aliastxhistory_collection, update_flags, selector, update, write_concern, &error)) {
		LogPrintf("MONGODB ALIAS TX HISTORY ERROR: %s\n", error.message);
	}
	if (selector)
		bson_destroy(selector);
	if (update)
		bson_destroy(update);
	if (write_concern)
		mongoc_write_concern_destroy(write_concern);
}
void CAliasDB::EraseAliasIndexTxHistory(const std::vector<unsigned char>& vchAlias, bool cleanup) {
	bson_error_t error;
	bson_t *selector = NULL;
	mongoc_write_concern_t* write_concern = NULL;
	mongoc_remove_flags_t remove_flags;
	remove_flags = (mongoc_remove_flags_t)(MONGOC_REMOVE_NONE);
	selector = BCON_NEW("alias", BCON_UTF8(stringFromVch(vchAlias).c_str()));
	write_concern = mongoc_write_concern_new();
	mongoc_write_concern_set_w(write_concern, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
	if (!mongoc_collection_remove(aliashistory_collection, remove_flags, selector, cleanup ? NULL : write_concern, &error)) {
		LogPrintf("MONGODB ALIAS HISTORY REMOVE ERROR: %s\n", error.message);
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
	if (aliashistory_collection)
		mongoc_collection_destroy(aliashistory_collection);
	if (aliastxhistory_collection)
		mongoc_collection_destroy(aliastxhistory_collection);
	if(offer_collection)
		mongoc_collection_destroy(offer_collection);
	if (offerhistory_collection)
		mongoc_collection_destroy(offerhistory_collection);
	if(escrow_collection)
		mongoc_collection_destroy(escrow_collection);
	if (escrowbid_collection)
		mongoc_collection_destroy(escrowbid_collection);
	if(cert_collection)
		mongoc_collection_destroy(cert_collection);
	if (certhistory_collection)
		mongoc_collection_destroy(certhistory_collection);
	if(feedback_collection)
		mongoc_collection_destroy(feedback_collection);
	if (asset_collection)
		mongoc_collection_destroy(asset_collection);
	if (assethistory_collection)
		mongoc_collection_destroy(assethistory_collection);
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
			"<collection> Collection name, either: 'alias', 'aliashistory', 'aliastxhistory', 'cert', 'certhistory', 'asset', 'assethistory','offer', 'offerhistory', 'feedback', 'escrow', 'escrowbid'.\n"
			"<query> JSON query on the collection to retrieve a set of documents.\n"
			"<options> Optional. JSON option arguments into the query. Based on mongoc_collection_find_with_opts.\n"
			+ HelpRequiringPassphrase());
	string collection = params[0].get_str();
	mongoc_collection_t *selectedCollection;
	if (collection == "alias")
		selectedCollection = alias_collection;
	else if (collection == "aliashistory")
		selectedCollection = aliashistory_collection;
	else if (collection == "aliastxhistory")
		selectedCollection = aliastxhistory_collection;
	else if (collection == "cert")
		selectedCollection = cert_collection;
	else if (collection == "certhistory")
		selectedCollection = certhistory_collection;
	else if (collection == "asset")
		selectedCollection = asset_collection;
	else if (collection == "assethistory")
		selectedCollection = assethistory_collection;
	else if (collection == "offer")
		selectedCollection = offer_collection;
	else if (collection == "offerhistory")
		selectedCollection = offerhistory_collection;
	else if (collection == "escrow")
		selectedCollection = escrow_collection;
	else if (collection == "escrowbid")
		selectedCollection = escrowbid_collection;
	else if (collection == "feedback")
		selectedCollection = feedback_collection;
	else
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("Invalid selection collection name, please specify the collection parameter as either 'alias', 'cert', 'offer', 'feedback' or 'escrow'"));

	string query = params[1].get_str();
	string options = "";
	if (CheckParam(params, 2))
		options = params[2].get_str();
	
	bson_t *filter = NULL;
	bson_t *opts = NULL;
	mongoc_cursor_t *cursor;
	bson_error_t error;
	const bson_t *doc;
	char *str;
	try {
		filter = bson_new_from_json((const uint8_t *)query.c_str(), -1, &error);
		if (!filter) {
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5505 - " + boost::lexical_cast<string>(error.message));
		}
	}
	catch (...) {
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("Cannot convert query into JSON"));
	}
	if (!options.empty()) {
		opts = bson_new_from_json((const uint8_t *)options.c_str(), -1, &error);
		if (!opts) {
			bson_destroy(filter);
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5506 - " + boost::lexical_cast<string>(error.message));
		}
	}
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
	if (fHelp || 8 != params.size())
		throw runtime_error(
			"aliasnew [aliasname] [public value] [accept_transfers_flags=3] [expire_timestamp] [address] [encryption_privatekey] [encryption_publickey] [witness]\n"
						"<aliasname> alias name.\n"
						"<public value> alias public profile data, 256 characters max.\n"
						"<accept_transfers_flags> 0 for none, 1 for accepting certificate transfers, 2 for accepting asset transfers and 3 for all. Default is 3.\n"	
						"<expire_timestamp> Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is FEERATE*(2.88^years). FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 hour.\n"	
						"<address> Address for this alias.\n"		
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. Should be encrypted to publickey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias.\n"						
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"							
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromString(params[0].get_str());
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
	string strPublicValue = "";
	strPublicValue = params[1].get_str();
	vchPublicValue = vchFromString(strPublicValue);

	unsigned char nAcceptTransferFlags = 3;
	nAcceptTransferFlags = params[2].get_int();
	uint64_t nTime = 0;
	nTime = params[3].get_int64();
	// sanity check set to 1 hr
	if(nTime < chainActive.Tip()->GetMedianTimePast() +3600)
		nTime = chainActive.Tip()->GetMedianTimePast() +3600;

	string strAddress = "";
	strAddress = params[4].get_str();
	
	string strEncryptionPrivateKey = "";
	strEncryptionPrivateKey = params[5].get_str();
	string strEncryptionPublicKey = "";
	strEncryptionPublicKey = params[6].get_str();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[7]);

	CWalletTx wtx;

	CAliasIndex oldAlias;
	if(GetAlias(vchAlias, oldAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5508 - " + _("This alias already exists"));


	const vector<unsigned char> &vchRandAlias = vchFromString(GenerateSyscoinGuid());

    // build alias
    CAliasIndex newAlias;
	newAlias.vchGUID = vchRandAlias;
	newAlias.vchAlias = vchAlias;
	newAlias.nHeight = chainActive.Tip()->nHeight;
	if(!strEncryptionPublicKey.empty())
		newAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);
	if(!strEncryptionPrivateKey.empty())
		newAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	newAlias.vchPublicValue = vchPublicValue;
	newAlias.nExpireTime = nTime;
	newAlias.nAcceptTransferFlags = nAcceptTransferFlags;
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
		scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_ACTIVATE) << vchAlias << newAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	else
		scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_ACTIVATE) << vchHashAlias << OP_2DROP << OP_DROP;

	CSyscoinAddress newAddress;
	GetAddress(newAlias, &newAddress, scriptPubKeyOrig);
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, recipientPayment);
	CScript scriptData;
	
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
	uint64_t nTimeExpiry = nTime - chainActive.Tip()->GetMedianTimePast();
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

	SendMoneySyscoin(vchAlias, vchWitness, recipient, recipientPayment, vecSend, wtx, &coinControl, false);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	res.push_back(strAddress);
	return res;
}
UniValue aliasupdate(const UniValue& params, bool fHelp) {
	if (fHelp || 9 != params.size())
		throw runtime_error(
			"aliasupdate [aliasname] [public value] [address] [accept_transfers_flags=3] [expire_timestamp] [encryption_privatekey] [encryption_publickey] [witness] [instantsend]\n"
						"Update and possibly transfer an alias.\n"
						"<aliasname> alias name.\n"
						"<public_value> alias public profile data, 256 characters max.\n"			
						"<address> Address of alias.\n"		
						"<accept_transfers_flags> 0 for none, 1 for accepting certificate transfers, 2 for accepting asset transfers and 3 for all. Default is 3.\n"
						"<expire_timestamp> Time in seconds. Future time when to expire alias. It is exponentially more expensive per year, calculation is 2.88^years. FEERATE is the dynamic satoshi per byte fee set in the rate peg alias used for this alias. Defaults to 1 hour. Set to 0 if not changing expiration.\n"		
						"<encryption_privatekey> Encrypted private key used for encryption/decryption of private data related to this alias. If transferring, the key should be encrypted to alias_pubkey.\n"
						"<encryption_publickey> Public key used for encryption/decryption of private data related to this alias. Useful if you are changing pub/priv keypair for encryption on this alias.\n"						
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						"<instantsend> Set to true to use InstantSend to send this transaction or false otherwise.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromString(params[0].get_str());
	string strPrivateValue = "";
	string strPublicValue = "";
	strPublicValue = params[1].get_str();
	
	CWalletTx wtx;
	CAliasIndex updateAlias;
	string strAddress = "";
	strAddress = params[2].get_str();
	
	unsigned char nAcceptTransferFlags = params[3].get_int();
	
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() +ONE_YEAR_IN_SECONDS;
	nTime = params[4].get_int64();

	string strEncryptionPrivateKey = "";
	strEncryptionPrivateKey = params[5].get_str();
	
	string strEncryptionPublicKey = "";
	strEncryptionPublicKey = params[6].get_str();
	
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[7]);

	bool fUseInstantSend = false;
	fUseInstantSend = params[8].get_bool();

	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5518 - " + _("Could not find an alias with this name"));


	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	theAlias.nHeight = chainActive.Tip()->nHeight;
	if(strPublicValue != stringFromVch(copyAlias.vchPublicValue))
		theAlias.vchPublicValue = vchFromString(strPublicValue);
	if(strEncryptionPrivateKey != HexStr(copyAlias.vchEncryptionPrivateKey))
		theAlias.vchEncryptionPrivateKey = ParseHex(strEncryptionPrivateKey);
	if(strEncryptionPublicKey != HexStr(copyAlias.vchEncryptionPublicKey))
		theAlias.vchEncryptionPublicKey = ParseHex(strEncryptionPublicKey);

	if(strAddress != EncodeBase58(copyAlias.vchAddress))
		DecodeBase58(strAddress, theAlias.vchAddress);
	theAlias.nExpireTime = nTime;
	theAlias.nAccessFlags = copyAlias.nAccessFlags;
	theAlias.nAcceptTransferFlags = nAcceptTransferFlags;
	
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
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

    vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, recipientPayment);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	if (nTime > 0) {
		// calculate a fee if renewal is larger than default.. based on how many years you extend for it will be exponentially more expensive
		uint64_t nTimeExpiry = nTime - chainActive.Tip()->GetMedianTimePast();
		if (nTimeExpiry < 3600)
			nTimeExpiry = 3600;
		float fYears = nTimeExpiry / ONE_YEAR_IN_SECONDS;
		if (fYears < 1)
			fYears = 1;
		fee.nAmount *= powf(2.88, fYears);
	}
	
	vecSend.push_back(fee);
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	bool transferAlias = false;
	if(newAddress.ToString() != EncodeBase58(copyAlias.vchAddress))
		transferAlias = true;
	
	SendMoneySyscoin(vchAlias, vchWitness, recipient, recipientPayment, vecSend, wtx, &coinControl, fUseInstantSend, transferAlias);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
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
	char type;
	if(DecodeAndParseSyscoinTx(rawTx, op, nOut, vvch, type))
		SysTxToJSON(op, vchData, vchHash, output, type);
	
	return output;
}
void SysTxToJSON(const int op, const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash, UniValue &entry, const char& type)
{
	if(type == ALIAS)
		AliasTxToJSON(op, vchData, vchHash, entry);
	else if(type == CERT)
		CertTxToJSON(op, vchData, vchHash, entry);
	else if(type == ESCROW)
		EscrowTxToJSON(op, vchData, vchHash, entry);
	else if(type == OFFER)
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
	if (!alias.offerWhitelist.IsNull())
	{
		if (alias.offerWhitelist.entries.begin()->second.nDiscountPct == 127)
			entry.push_back(Pair("whitelist", _("Whitelist was cleared")));
		else
			entry.push_back(Pair("whitelist", _("Whitelist entries were added or removed")));
	}
	if(!alias.vchPublicValue .empty() && alias.vchPublicValue != dbAlias.vchPublicValue)
		entry.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));

	if(EncodeBase58(alias.vchAddress) != EncodeBase58(dbAlias.vchAddress))
		entry.push_back(Pair("address", EncodeBase58(alias.vchAddress))); 

	if(alias.nExpireTime != dbAlias.nExpireTime)
		entry.push_back(Pair("renewal", alias.nExpireTime));

}
UniValue syscoinsendrawtransaction(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() < 1 || params.size() > 3)
		throw runtime_error("syscoinsendrawtransaction \"hexstring\" ( allowhighfees instantsend )\n"
			"\nSubmits raw transaction (serialized, hex-encoded) to local node and network.\n"
			"\nAlso see createrawtransaction and signrawtransaction calls.\n"
			"\nArguments:\n"
			"1. \"hexstring\"    (string, required) The hex string of the raw transaction)\n"
			"2. allowhighfees  (boolean, optional, default=false) Allow high fees\n"
			"3. instantsend    (boolean, optional, default=false) Use InstantSend to send this transaction\n");
	RPCTypeCheck(params, boost::assign::list_of(UniValue::VSTR)(UniValue::VBOOL)(UniValue::VBOOL));
	const string &hexstring = params[0].get_str();
	bool fOverrideFees = false;
	if (params.size() > 1)
		fOverrideFees = params[1].get_bool();

	bool fInstantSend = false;
	if (params.size() > 2)
		fInstantSend = params[2].get_bool();
	CTransaction tx;
	if (!DecodeHexTx(tx, hexstring))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5534 - " + _("Could not send raw transaction: Cannot decode transaction from hex string"));
	if (tx.vin.size() <= 0)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5534 - " + _("Could not send raw transaction: Inputs are empty"));
	if (tx.vout.size() <= 0)
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5534 - " + _("Could not send raw transaction: Outputs are empty"));
	UniValue arraySendParams(UniValue::VARR);
	arraySendParams.push_back(hexstring);
	arraySendParams.push_back(fOverrideFees);
	arraySendParams.push_back(fInstantSend);
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
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("txid", returnRes.get_str()));
	// check for alias registration, if so save the info in this node for alias activation calls after a block confirmation
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
UniValue prunesyscoinservices(const UniValue& params, bool fHelp)
{
	int servicesCleaned = 0;
	CleanupSyscoinServiceDatabases(servicesCleaned);
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("services_cleaned", servicesCleaned));
	return res;
}
UniValue aliasbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "aliasbalance \"alias\"\n"
            "\nReturns the total amount received by the given alias in transactions.\n"
            "\nArguments:\n"
            "1. \"alias\"  (string, required) The syscoin alias for transactions.\n"
       );
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

  	int op;
	vector<vector<unsigned char> > vvch;
    for (unsigned int i = 0;i<utxoArray.size();i++)
    {
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
		const int& nOut = find_value(utxoObj, "outputIndex").get_int();
		const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "script").get_str()));
		const CScript& scriptPubKey = CScript(data.begin(), data.end());
		const CAmount &nValue = AmountFromValue(find_value(utxoObj, "satoshis"));
		const int& nHeight = find_value(utxoObj, "height").get_int();
		if (DecodeAliasScript(scriptPubKey, op, vvch))
			continue;
		// some smaller sized outputs are reserved to pay for fees only using aliasselectpaymentcoins (with bSelectFeePlacement set to true)
		if (nValue <= std::max(CWallet::GetMinimumFee(1000, nTxConfirmTarget, mempool), CTxLockRequest().GetMinFee() * 2))
			continue;
		{
			LOCK(mempool.cs);
			auto it = mempool.mapNextTx.find(COutPoint(txid, nOut));
			if (it != mempool.mapNextTx.end())
				continue;
		}
		nAmount += nValue;
		
    }
	UniValue res(UniValue::VOBJ);
	res.push_back(Pair("balance", ValueFromAmount(nAmount)));
    return  res;
}
int aliasselectpaymentcoins(const vector<unsigned char> &vchAlias, const CAmount &nAmount, vector<COutPoint>& outPoints, bool& bIsFunded, CAmount &nRequiredAmount, bool bSelectFeePlacement, bool bSelectAll, bool bNoAliasRecipient)
{
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
	vector<vector<unsigned char> > vvch;
	bIsFunded = false;
	for (unsigned int i = 0; i<utxoArray.size(); i++)
	{
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
		const int& nOut = find_value(utxoObj, "outputIndex").get_int();
		const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "script").get_str()));
		const CScript& scriptPubKey = CScript(data.begin(), data.end());
		const CAmount &nValue = AmountFromValue(find_value(utxoObj, "satoshis"));
		// look for non alias inputs, coins that can be used to fund this transaction, aliasunspent is used to get the alias utxo for proof of ownership
		if (DecodeAliasScript(scriptPubKey, op, vvch))
			continue;  
		if (!bSelectAll) {
			// fee placement were ones that were smaller outputs used for subsequent updates
			if (nValue <= std::max(CWallet::GetMinimumFee(1000, nTxConfirmTarget, mempool), CTxLockRequest().GetMinFee() * 2))
			{
				// alias pay doesn't include recipient, no utxo inputs used for aliaspay, for aliaspay we don't care about these small dust amounts
				if (bNoAliasRecipient)
					continue;
				// if this output is a fee placement because its of low value and we aren't selecting fee outputs then continue
				else if (!bSelectFeePlacement)
					continue;
			}
			else if (bSelectFeePlacement)
				continue;
		}
		const COutPoint &outPointToCheck = COutPoint(txid, nOut);
		{
			LOCK(mempool.cs);
			auto it = mempool.mapNextTx.find(outPointToCheck);
			if (it != mempool.mapNextTx.end())
				continue;
		}
		if(!bIsFunded || bSelectAll)
		{
			outPoints.push_back(outPointToCheck);
			nCurrentAmount += nValue;
			if (nCurrentAmount >= nDesiredAmount) {
				bIsFunded = true;
			}
		}	
		numResults++;
    }
	nRequiredAmount = nDesiredAmount - nCurrentAmount;
	if(nRequiredAmount < 0)
		nRequiredAmount = 0;
	numCoinsLeft = numResults - (int)outPoints.size();
	return numCoinsLeft;
}
int aliasunspent(const vector<unsigned char> &vchAlias, COutPoint& outpoint)
{
	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		return 0;
	UniValue paramsUTXO(UniValue::VARR);
	UniValue param(UniValue::VOBJ);
	UniValue utxoParams(UniValue::VARR);
	const string &strAddressFrom = EncodeBase58(theAlias.vchAddress);
	utxoParams.push_back(strAddressFrom);
	param.push_back(Pair("addresses", utxoParams));
	paramsUTXO.push_back(param);
	const UniValue &resUTXOs = tableRPC.execute("getaddressutxos", paramsUTXO);
	UniValue utxoArray(UniValue::VARR);
	if (resUTXOs.isArray())
		utxoArray = resUTXOs.get_array();
	else
		return 0;

	int numResults = 0;
	CAmount nCurrentAmount = 0;
	bool funded = false;
	for (unsigned int i = 0; i<utxoArray.size(); i++)
	{
		const UniValue& utxoObj = utxoArray[i].get_obj();
		const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
		const int& nOut = find_value(utxoObj, "outputIndex").get_int();
		const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "script").get_str()));
		const CScript& scriptPubKey = CScript(data.begin(), data.end());
		int op;
		vector<vector<unsigned char> > vvch;
		if (!DecodeAliasScript(scriptPubKey, op, vvch) || vvch.size() <= 1 || vvch[0] != theAlias.vchAlias || vvch[1] != theAlias.vchGUID)
			continue;
		const COutPoint &outPointToCheck = COutPoint(txid, nOut);
		{
			LOCK(mempool.cs);
			auto it = mempool.mapNextTx.find(outPointToCheck);
			if (it != mempool.mapNextTx.end())
				continue;
		}
		if (!funded)
		{
			outpoint = outPointToCheck;
			funded = true;
		}
		numResults++;
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
	oName.push_back(Pair("encryption_privatekey", HexStr(alias.vchEncryptionPrivateKey)));
	oName.push_back(Pair("encryption_publickey", HexStr(alias.vchEncryptionPublicKey)));
	oName.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));	
	oName.push_back(Pair("txid", alias.txHash.GetHex()));
	int64_t nTime = 0;
	if (chainActive.Height() >= alias.nHeight) {
		CBlockIndex *pindex = chainActive[alias.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oName.push_back(Pair("time", nTime));
	oName.push_back(Pair("address", EncodeBase58(alias.vchAddress)));
	oName.push_back(Pair("accepttransferflags", (int)alias.nAcceptTransferFlags));
	expired_time = alias.nExpireTime;
	if(expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}  
	oName.push_back(Pair("expires_on", expired_time));
	oName.push_back(Pair("expired", expired));
	return true;
}
bool BuildAliasIndexerHistoryJson(const CAliasIndex& alias, UniValue& oName)
{
	oName.push_back(Pair("_id", alias.txHash.GetHex()));
	oName.push_back(Pair("publicvalue", stringFromVch(alias.vchPublicValue)));
	oName.push_back(Pair("alias", stringFromVch(alias.vchAlias)));
	int64_t nTime = 0;
	if (chainActive.Height() >= alias.nHeight) {
		CBlockIndex *pindex = chainActive[alias.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oName.push_back(Pair("time", nTime));
	oName.push_back(Pair("address", EncodeBase58(alias.vchAddress)));
	oName.push_back(Pair("accepttransferflags", (int)alias.nAcceptTransferFlags));
	return true;
}
UniValue aliaspay(const UniValue& params, bool fHelp) {

    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "aliaspay aliasfrom {\"address\":amount,...} (instantsend subtractfeefromamount)\n"
            "\nSend multiple times from an alias. Amounts are double-precision floating point numbers."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
			"1. \"aliasfrom\"			(string, required) Alias to pay from\n"
            "2. \"amounts\"             (string, required) A json object with aliases and amounts\n"
            "    {\n"
            "      \"address\":amount   (numeric or string) The syscoin alias is the key, the numeric amount (can be string) in SYS is the value\n"
            "      ,...\n"
            "    }\n"
			"3. instantsend				(boolean, optional) Set to true to use InstantSend to send this transaction or false otherwise.\n"
			"4. subtractfeefromamount   (string, optional) A json array with addresses.\n"
            "\nResult:\n"
			"\"transaction hex\"          (string) The transaction hex (unsigned) for signing and sending. Only 1 transaction is created regardless of \n"
            "                                    the number of addresses.\n"
            "\nExamples:\n"
            "\nSend two amounts to two different address or aliases, sends 0.01/0.02 SYS representing USD:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"USD\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"alias2\\\":0.02}\"") +
            "\nSend two amounts to two different address or aliases while also adding a comment, sends 0.01/0.02 SYS representing USD:\n"
            + HelpExampleCli("aliaspay", "\"myalias\" \"USD\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\" \"testing\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    string strFrom = params[0].get_str();
	CAliasIndex theAlias;
	if (!GetAlias(vchFromString(strFrom), theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR: ERRCODE: 5509 - " + _("Invalid fromalias"));

    UniValue sendTo = params[1].get_obj();

	bool fUseInstantSend = false;
	if (params.size() > 2)
		fUseInstantSend = params[2].get_bool();

	UniValue subtractFeeFromAmount(UniValue::VARR);
	if (params.size() > 3)
		subtractFeeFromAmount = params[3].get_array();


	CWalletTx wtx;
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
		bool fSubtractFeeFromAmount = false;
		for (unsigned int idx = 0; idx < subtractFeeFromAmount.size(); idx++) {
			const UniValue& addr = subtractFeeFromAmount[idx];
			if (addr.get_str() == name_)
				fSubtractFeeFromAmount = true;
		}
        CRecipient recipient = {scriptPubKey, nAmount, fSubtractFeeFromAmount };
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
	CreateAliasRecipient(scriptPubKeyOrig, recipientPayment);	
	SendMoneySyscoin(theAlias.vchAlias, vchFromString(""), recipient, recipientPayment, vecSend, wtx, &coinControl, fUseInstantSend);
	
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
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
UniValue aliasupdatewhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 4)
		throw runtime_error(
			"aliasupdatewhitelist [owner alias] [{\"alias\":\"aliasname\",\"discount_percentage\":n},...] [witness] [instantsend]\n"
			"Update to the whitelist(controls who can resell). Array of whitelist entries in parameter 1.\n"
			"To add to list, include a new alias/discount percentage that does not exist in the whitelist.\n"
			"To update entry, change the discount percentage of an existing whitelist entry.\n"
			"To remove whitelist entry, pass the whilelist entry without changing discount percentage.\n"
			"<owner alias> owner alias controlling this whitelist.\n"
			"	\"entries\"       (string) A json array of whitelist entries to add/remove/update\n"
			"    [\n"
			"      \"alias\"     (string) Alias that you want to add to the affiliate whitelist. Can be '*' to represent that the offers owned by owner alias can be resold by anybody.\n"
			"	   \"discount_percentage\"     (number) A discount percentage associated with this alias. The reseller can sell your offer at this discount, not accounting for any commissions he/she may set in his own reselling offer. 0 to 99.\n"
			"      ,...\n"
			"    ]\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			"<instantsend> Set to true to use InstantSend to send this transaction or false otherwise.\n"
			+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchOwnerAlias = vchFromValue(params[0]);
	UniValue whitelistEntries = params[1].get_array();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	bool fUseInstantSend = false;
	fUseInstantSend = params[3].get_bool();
	CWalletTx wtx;

	// this is a syscoin txn
	CScript scriptPubKeyOrig;


	CAliasIndex theAlias;
	if (!GetAlias(vchOwnerAlias, theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR ERRCODE: 1518 - " + _("Could not find an alias with this guid"));

	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);
	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	theAlias.nHeight = chainActive.Tip()->nHeight;

	for (unsigned int idx = 0; idx < whitelistEntries.size(); idx++) {
		const UniValue& p = whitelistEntries[idx];
		if (!p.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"alias\",\"discount_percentage\"}");

		UniValue whiteListEntryObj = p.get_obj();
		RPCTypeCheckObj(whiteListEntryObj, boost::assign::map_list_of("alias", UniValue::VSTR)("discount_percentage", UniValue::VNUM));
		string aliasEntryName = find_value(whiteListEntryObj, "alias").get_str();
		int nDiscount = find_value(whiteListEntryObj, "discount_percentage").get_int();

		COfferLinkWhitelistEntry entry;
		entry.aliasLinkVchRand = vchFromString(aliasEntryName);
		entry.nDiscountPct = nDiscount;
		theAlias.offerWhitelist.PutWhitelistEntry(entry);

		if (!theAlias.offerWhitelist.GetLinkEntryByHash(vchFromString(aliasEntryName), entry))
			throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR ERRCODE: 1523 - " + _("This alias entry was not added to affiliate list: ") + aliasEntryName);
	}
	vector<unsigned char> data;
	theAlias.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());
	vector<unsigned char> vchHashAlias = vchFromValue(hash.GetHex());

	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, recipientPayment);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(copyAlias.vchAlias, vchWitness, recipient, recipientPayment, vecSend, wtx, &coinControl, fUseInstantSend);

	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}
UniValue aliasclearwhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 3)
		throw runtime_error(
			"aliasclearwhitelist [owner alias] [witness] [instantsend]\n"
			"Clear your whitelist(controls who can resell).\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[1]);
	bool fUseInstantSend = false;
	fUseInstantSend = params[2].get_bool();
	// this is a syscoind txn
	CWalletTx wtx;
	CScript scriptPubKeyOrig;


	CAliasIndex theAlias;
	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ALIAS_RPC_ERROR ERRCODE: 1529 - " + _("Could not find an alias with this name"));


	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

	COfferLinkWhitelistEntry entry;
	// special case to clear all entries for this offer
	entry.nDiscountPct = 127;
	CAliasIndex copyAlias = theAlias;
	theAlias.ClearAlias();
	theAlias.nHeight = chainActive.Tip()->nHeight;
	theAlias.offerWhitelist.PutWhitelistEntry(entry);
	vector<unsigned char> data;
	theAlias.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());
	vector<unsigned char> vchHashAlias = vchFromValue(hash.GetHex());

	CScript scriptPubKey;
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << copyAlias.vchAlias << copyAlias.vchGUID << vchHashAlias << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	CRecipient recipientPayment;
	CreateAliasRecipient(scriptPubKeyOrig, recipientPayment);
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(copyAlias.vchAlias, vchWitness, recipient, recipientPayment, vecSend, wtx, &coinControl, fUseInstantSend);

	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue aliaswhitelist(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 1)
		throw runtime_error("aliaswhitelist <alias>\n"
			"List all affiliates for this alias.\n");
	UniValue oRes(UniValue::VARR);
	vector<unsigned char> vchAlias = vchFromValue(params[0]);

	CAliasIndex theAlias;

	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("could not find an offer with this guid");

	for (auto const &it : theAlias.offerWhitelist.entries)
	{
		const COfferLinkWhitelistEntry& entry = it.second;
		UniValue oList(UniValue::VOBJ);
		oList.push_back(Pair("alias", stringFromVch(entry.aliasLinkVchRand)));
		oList.push_back(Pair("discount_percentage", entry.nDiscountPct));
		oRes.push_back(oList);
	}
	return oRes;
}
bool COfferLinkWhitelist::GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) const {
	entry.SetNull();
	const vector<unsigned char> allAliases = vchFromString("*");
	if (entries.count(ahash) > 0) {
		entry = entries.at(ahash);
		return true;
	}
	else if(entries.count(allAliases) > 0) {
		entry = entries.at(allAliases);
		return true;
	}
	return false;
}
string GetSyscoinTransactionDescription(const int op, const vector<vector<unsigned char> > &vvchArgs, const CTransaction &tx, string& responseEnglish, string& responseGUID, const char &type)
{
	responseGUID = stringFromVch(vvchArgs[0]);
	string strResponse = "";
	COffer offer;
	CEscrow escrow;
	if (type == ALIAS) {
		if (op == OP_ALIAS_ACTIVATE) {
			strResponse = _("Alias Activated");
			responseEnglish = "Alias Activated";
		}
		else if (op == OP_ALIAS_UPDATE) {
			strResponse = _("Alias Updated");
			responseEnglish = "Alias Updated";
		}
	}
	else if (type == OFFER) {
		if (op == OP_OFFER_ACTIVATE) {
			strResponse = _("Offer Activated");
			responseEnglish = "Offer Activated";
		}
		else if (op == OP_OFFER_UPDATE) {
			strResponse = _("Offer Updated");
			responseEnglish = "Offer Updated";
		}
	}
	else if (type == CERT) {
		if (op == OP_CERT_ACTIVATE) {
			strResponse = _("Certificate Activated");
			responseEnglish = "Certificate Activated";
		}
		else if (op == OP_CERT_UPDATE) {
			strResponse = _("Certificate Updated");
			responseEnglish = "Certificate Updated";
		}
		else if (op == OP_CERT_TRANSFER) {
			strResponse = _("Certificate Transferred");
			responseEnglish = "Certificate Transferred";
		}
	}
	else if (type == ASSET) {
		if (op == OP_ASSET_ACTIVATE) {
			strResponse = _("Asset Activated");
			responseEnglish = "Asset Activated";
		}
		else if (op == OP_ASSET_MINT) {
			strResponse = _("Asset Minted");
			responseEnglish = "Asset Minted";
		}
		else if (op == OP_ASSET_UPDATE) {
			strResponse = _("Asset Updated");
			responseEnglish = "Asset Updated";
		}
		else if (op == OP_ASSET_SEND) {
			strResponse = _("Asset Ownership Transferred");
			responseEnglish = "Asset Ownership Transferred";
		}
		else if (op == OP_ASSET_TRANSFER) {
			strResponse = _("Asset Transferred");
			responseEnglish = "Asset Transferred";
		}
	}
	else if (type == ESCROW) {
		if (op == OP_ESCROW_ACTIVATE) {
			strResponse = _("Escrow Activated");
			responseEnglish = "Escrow Activated";
		}
		else if (op == OP_ESCROW_ACKNOWLEDGE) {
			strResponse = _("Escrow Acknowledged");
			responseEnglish = "Escrow Acknowledged";
		}
		else if (op == OP_ESCROW_RELEASE) {
			strResponse = _("Escrow Released");
			responseEnglish = "Escrow Released";
		}
		else if (op == OP_ESCROW_RELEASE_COMPLETE) {
			strResponse = _("Escrow Release Complete");
			responseEnglish = "Escrow Release Complete";
		}
		else if (op == OP_ESCROW_FEEDBACK) {
			strResponse = _("Escrow Feedback");
			responseEnglish = "Escrow Feedback";
		}
		else if (op == OP_ESCROW_BID) {
			strResponse = _("Escrow Bid");
			responseEnglish = "Escrow Bid";
		}
		else if (op == OP_ESCROW_ADD_SHIPPING) {
			strResponse = _("Escrow Add Shipping");
			responseEnglish = "Escrow Add Shipping";
		}
		else if (op == OP_ESCROW_REFUND) {
			strResponse = _("Escrow Refunded");
			responseEnglish = "Escrow Refunded";
		}
		else if (op == OP_ESCROW_REFUND_COMPLETE) {
			strResponse = _("Escrow Refund Complete");
			responseEnglish = "Escrow Refund Complete";
		}
	}
	else{
		return "";
	}
	return strResponse;
}
