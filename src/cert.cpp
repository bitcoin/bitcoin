// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cert.h"
#include "alias.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include "coincontrol.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <chrono>

using namespace std::chrono;
using namespace std;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend=false, bool transferAlias=false);
bool IsCertOp(int op) {
    return op == OP_CERT_ACTIVATE
        || op == OP_CERT_UPDATE
        || op == OP_CERT_TRANSFER;
}

uint64_t GetCertExpiration(const CCert& cert) {
	uint64_t nTime = chainActive.Tip()->GetMedianTimePast() + 1;
	CAliasUnprunable aliasUnprunable;
	if (paliasdb && paliasdb->ReadAliasUnprunable(cert.vchAlias, aliasUnprunable) && !aliasUnprunable.IsNull())
		nTime = aliasUnprunable.nExpireTime;
	
	return nTime;
}


string certFromOp(int op) {
    switch (op) {
    case OP_CERT_ACTIVATE:
        return "certactivate";
    case OP_CERT_UPDATE:
        return "certupdate";
    case OP_CERT_TRANSFER:
        return "certtransfer";
    default:
        return "<unknown cert op>";
    }
}
bool CCert::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsCert(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsCert >> *this;

		vector<unsigned char> vchCertData;
		Serialize(vchCertData);
		const uint256 &calculatedHash = Hash(vchCertData.begin(), vchCertData.end());
		const vector<unsigned char> &vchRandCert = vchFromValue(calculatedHash.GetHex());
		if(vchRandCert != vchHash)
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
bool CCert::UnserializeFromTx(const CTransaction &tx) {
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
void CCert::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsCert(SER_NETWORK, PROTOCOL_VERSION);
    dsCert << *this;
	vchData = vector<unsigned char>(dsCert.begin(), dsCert.end());

}
void CCertDB::WriteCertIndex(const CCert& cert, const int& op) {
	UniValue oName(UniValue::VOBJ);
	if (BuildCertIndexerJson(cert, oName)) {
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "cert");
	}

	WriteCertIndexHistory(cert, op);
}
void CCertDB::WriteCertIndexHistory(const CCert& cert, const int &op) {
	UniValue oName(UniValue::VOBJ);
	if (BuildCertIndexerHistoryJson(cert, oName)) {
		oName.push_back(Pair("op", certFromOp(op)));
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "certhistory");
	}
}
	
bool CCertDB::CleanupDatabase(int &servicesCleaned)
{
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CCert txPos;
	pair<string, vector<unsigned char> > key;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
			if (pcursor->GetKey(key) && key.first == "certi") {
  				if (!GetCert(key.second, txPos) || chainActive.Tip()->GetMedianTimePast() >= GetCertExpiration(txPos))
				{
					servicesCleaned++;
					EraseCert(key.second, true);
				} 
				
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
	return true;
}
bool GetCert(const vector<unsigned char> &vchCert,
        CCert& txPos) {
	uint256 txid;
    if (!pcertdb || !pcertdb->ReadCert(vchCert, txPos))
        return false;
    if (chainActive.Tip()->GetMedianTimePast() >= GetCertExpiration(txPos)) {
		txPos.SetNull();
        return false;
    }

    return true;
}
bool GetFirstCert(const vector<unsigned char> &vchCert,
	CCert& txPos) {
	if (!pcertdb || !pcertdb->ReadFirstCert(vchCert, txPos))
		return false;
	if (chainActive.Tip()->GetMedianTimePast() >= GetCertExpiration(txPos)) {
		txPos.SetNull();
		return false;
	}

	return true;
}
bool DecodeAndParseCertTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	CCert cert;
	bool decode = DecodeCertTx(tx, op, nOut, vvch);
	bool parse = cert.UnserializeFromTx(tx);
	if (decode&&parse) {
		type = CERT;
		return true;
	}
	return false;
}
bool DecodeCertTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeCertScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DecodeCertScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
    if (!script.GetOp(pc, opcode)) return false;
    if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);
	if (op != OP_SYSCOIN_CERT)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsCertOp(op))
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
bool DecodeCertScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeCertScript(script, op, vvch, pc);
}
bool RemoveCertScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeCertScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}
bool RevertCert(const std::vector<unsigned char>& vchCert, const int op, const uint256 &txHash, sorted_vector<std::vector<unsigned char> > &revertedCerts) {
	// only revert cert once
	if (revertedCerts.find(vchCert) != revertedCerts.end())
		return true;

	string errorMessage = "";
	CCert dbCert;
	LogPrintf("RevertCert %s\n", stringFromVch(vchCert).c_str());
	// if prev state doesn't exist, probably certactivate, in which case delete the cert and let pow create it again.
	if (!pcertdb->ReadLastCert(vchCert, dbCert)) {
		if (!pcertdb->EraseCert(vchCert))
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to erase cert");
			return error(errorMessage.c_str());
		}
	}
	// write the state back to previous state
	else if (!pcertdb->WriteCert(dbCert, op, INT64_MAX, false))
	{
		errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to write to cert DB");
		return error(errorMessage.c_str());
	}
	pcertdb->EraseISArrivalTimes(vchCert);
	revertedCerts.insert(vchCert);
	return true;

}
bool CheckCertInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const std::vector<unsigned char> &vvchAlias,
        bool fJustCheck, int nHeight, sorted_vector<std::vector<unsigned char> > &revertedCerts, string &errorMessage, bool bSanityCheck) {
	if (!pcertdb || !paliasdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !bSanityCheck)
	{
		LogPrintf("*Trying to add cert in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !bSanityCheck)
		LogPrintf("*** CERT %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

	// unserialize cert from txn, check for valid
	CCert theCert;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || !theCert.UnserializeFromData(vchData, vchHash))
	{
		errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a certificate");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Certificate arguments incorrect size");
			return error(errorMessage.c_str());
		}			
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}	
	}
	
	CAliasIndex alias;
	string retError = "";
	if(fJustCheck)
	{
		if(theCert.sCategory.size() > MAX_NAME_LENGTH)
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Certificate category too big");
			return error(errorMessage.c_str());
		}
		if(theCert.vchPubData.size() > MAX_VALUE_LENGTH)
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Certificate public data too big");
			return error(errorMessage.c_str());
		}
		switch (op) {
		case OP_CERT_ACTIVATE:
			if (theCert.vchCert.size() > MAX_GUID_LENGTH)
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Certificate hex guid too long");
				return error(errorMessage.c_str());
			}
			if((theCert.vchTitle.size() > MAX_NAME_LENGTH || theCert.vchTitle.empty()))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2012 - " + _("Certificate title too big or is empty");
				return error(errorMessage.c_str());
			}
			if(!boost::algorithm::starts_with(stringFromVch(theCert.sCategory), "certificates"))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Must use a certificate category");
				return true;
			}
			break;

		case OP_CERT_UPDATE:
			if(theCert.vchTitle.size() > MAX_NAME_LENGTH)
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Certificate title too big");
				return error(errorMessage.c_str());
			}
			if(theCert.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theCert.sCategory), "certificates"))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Must use a certificate category");
				return true;
			}
			break;

		case OP_CERT_TRANSFER:
			if(theCert.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theCert.sCategory), "certificates"))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2020 - " + _("Must use a certificate category");
				return true;
			}
			break;

		default:
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Certificate transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	if (!fJustCheck && !bSanityCheck) {
		if (!RevertCert(theCert.vchCert, op, tx.GetHash(), revertedCerts))
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to revert cert");
			return error(errorMessage.c_str());
		}
	}
	const string &user1 = stringFromVch(vvchAlias);
	string user2 = "";
	string user3 = "";
	if (op == OP_CERT_TRANSFER) {
		user2 = stringFromVch(theCert.vchAlias);
	}
	string strResponseEnglish = "";
	string strResponse = GetSyscoinTransactionDescription(op, strResponseEnglish, CERT);
	// if not an certnew, load the cert data from the DB
	CCert dbCert;
	if (!GetCert(theCert.vchCert, dbCert))
	{
		if (op != OP_CERT_ACTIVATE) {
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to read from certificate DB");
			return true;
		}
	}
	if(op != OP_CERT_ACTIVATE) 
	{
		if (dbCert.vchAlias != vvchAlias)
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot update this certificate. Certificate owner must sign off on this change.");
			return true;
		}
		if (theCert.vchAlias.empty())
			theCert.vchAlias = dbCert.vchAlias;
		if(theCert.vchPubData.empty())
			theCert.vchPubData = dbCert.vchPubData;
		if(theCert.vchTitle.empty())
			theCert.vchTitle = dbCert.vchTitle;
		if(theCert.sCategory.empty())
			theCert.sCategory = dbCert.sCategory;

		CCert firstCert;
		if (!GetFirstCert(dbCert.vchCert, firstCert)) {
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Cannot read first cert from cert DB");
			return true;
		}
		if(op == OP_CERT_TRANSFER)
		{
			// check toalias
			if(!GetAlias(theCert.vchAlias, alias))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Cannot find alias you are transferring to. It may be expired");	
				return true;
			}
			if(!(alias.nAcceptTransferFlags & ACCEPT_TRANSFER_CERTIFICATES))
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("The alias you are transferring to does not accept certificate transfers");
				return true;
			}
				
			// the original owner can modify certificate regardless of access flags, new owners must adhere to access flags
			if(dbCert.nAccessFlags < 2 && dbCert.vchAlias != firstCert.vchAlias)
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot transfer this certificate. Insufficient privileges.");
				return true;
			}
		}
		else if(op == OP_CERT_UPDATE)
		{
			if(dbCert.nAccessFlags < 1 && dbCert.vchAlias != firstCert.vchAlias)
			{
				errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this certificate. It is view-only.");
				return true;
			}
		}
		if(theCert.nAccessFlags > dbCert.nAccessFlags && dbCert.vchAlias != firstCert.vchAlias)
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot modify for more lenient access. Only tighter access level can be granted.");
			return true;
		}
	}
	else
	{
		if (fJustCheck && GetCert(theCert.vchCert, theCert))
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Certificate already exists");
			return true;
		}
	}
	if(!bSanityCheck) {
		if (strResponse != "") {
			paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theCert.vchCert));
		}
	}
    // set the cert's txn-dependent values
	theCert.nHeight = nHeight;
	theCert.txHash = tx.GetHash();
    // write cert  
	if (!bSanityCheck) {
		int64_t ms = INT64_MAX;
		if (fJustCheck) {
			ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}
		if (!pcertdb->WriteCert(theCert, op, ms, fJustCheck))
		{
			errorMessage = "SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to certifcate DB");
			return error(errorMessage.c_str());
		}
		// debug
		if (fDebug)
			LogPrintf("CONNECTED CERT: op=%s cert=%s hash=%s height=%d fJustCheck=%d\n",
				certFromOp(op).c_str(),
				stringFromVch(theCert.vchCert).c_str(),
				tx.GetHash().ToString().c_str(),
				nHeight,
				fJustCheck ? 1 : -1);
	}

    return true;
}





UniValue certnew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 5)
        throw runtime_error(
			"certnew [alias] [title] [public] [category=certificates] [witness]\n"
						"<alias> An alias you own.\n"
						"<title> title, 256 characters max.\n"
                        "<public> public data, 256 characters max.\n"
						"<category> category, 256 characters max. Defaults to certificates\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAlias = vchFromValue(params[0]);
    vector<unsigned char> vchTitle = vchFromString(params[1].get_str());
	vector<unsigned char> vchPubData = vchFromString(params[2].get_str());
	string strCategory = "certificates";
	strCategory = params[3].get_str();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);

	// check for alias existence in DB
	CAliasIndex theAlias;

	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2500 - " + _("failed to read alias from alias DB"));

	
    // gather inputs
	vector<unsigned char> vchCert = vchFromString(GenerateSyscoinGuid());
    // this is a syscoin transaction
    CWalletTx wtx;

    CScript scriptPubKeyOrig;
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);


    CScript scriptPubKey,scriptPubKeyAlias;

	// calculate net
    // build cert object
    CCert newCert;
	newCert.vchCert = vchCert;
	newCert.sCategory = vchFromString(strCategory);
	newCert.vchTitle = vchTitle;
	newCert.vchPubData = vchPubData;
	newCert.vchAlias = vchAlias;

	vector<unsigned char> data;
	newCert.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashCert = vchFromValue(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_CERT) << CScript::EncodeOP_N(OP_CERT_ACTIVATE) << vchHashCert << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(vchAlias, vchWitness, aliasRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	res.push_back(stringFromVch(vchCert));
	return res;
}

UniValue certupdate(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 5)
        throw runtime_error(
			"certupdate [guid] [title] [public] [category=certificates] [witness]\n"
						"Perform an update on an certificate you control.\n"
						"<guid> Certificate guidkey.\n"
						"<title> Certificate title, 256 characters max.\n"
                        "<public> Public data, 256 characters max.\n"                
						"<category> Category, 256 characters max. Defaults to certificates\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchCert = vchFromValue(params[0]);

	string strData = "";
	string strTitle = "";
	string strPubData = "";
	string strCategory = "";
	strTitle = params[1].get_str();
	strPubData = params[2].get_str();
	strCategory = params[3].get_str();

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);
    // this is a syscoind txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig;
	CCert theCert;
	
    if (!GetCert( vchCert, theCert))
        throw runtime_error("SYSCOIN_CERTIFICATE_RPC_ERROR: ERRCODE: 2504 - " + _("Could not find a certificate with this key"));

	if (!GetBoolArg("-unittest", false)) {
		ArrivalTimesMap arrivalTimes;
		pcertdb->ReadISArrivalTimes(vchCert, arrivalTimes);
		const int64_t & nNow = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
		for (auto& arrivalTime : arrivalTimes) {
			// if this tx arrived within the minimum latency period flag it as potentially conflicting
			if ((nNow - (arrivalTime.second / 1000)) < ZDAG_MINIMUM_LATENCY_SECONDS) {
				throw runtime_error("SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2510 - " + _("Please wait a few more seconds and try again..."));
			}
		}
	}

	CAliasIndex theAlias;

	if (!GetAlias(theCert.vchAlias, theAlias))
		throw runtime_error("SYSCOIN_CERTIFICATE_CONSENSUS_ERROR: ERRCODE: 2505 - " + _("Failed to read alias from alias DB"));

	CCert copyCert = theCert;
	theCert.ClearCert();
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

    // create CERTUPDATE txn keys
    CScript scriptPubKey;

	if(strPubData != stringFromVch(theCert.vchPubData))
		theCert.vchPubData = vchFromString(strPubData);
	if(strCategory != stringFromVch(theCert.sCategory))
		theCert.sCategory = vchFromString(strCategory);
	if(strTitle != stringFromVch(theCert.vchTitle))
		theCert.vchTitle = vchFromString(strTitle);
	vector<unsigned char> data;
	theCert.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashCert = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_CERT) << CScript::EncodeOP_N(OP_CERT_UPDATE) << vchHashCert << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, aliasRecipient, vecSend, wtx, &coinControl);
 	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}


UniValue certtransfer(const UniValue& params, bool fHelp) {
 if (fHelp || params.size() != 5)
        throw runtime_error(
			"certtransfer [guid] [alias] [public] [accessflags=2] [witness]\n"
						"Transfer a certificate you own to another alias.\n"
						"<guid> certificate guidkey.\n"
						"<alias> alias to transfer to.\n"
                        "<public> public data, 256 characters max.\n"	
						"<accessflags> Set new access flags for new owner for this certificate, 0 for read-only, 1 for edit, 2 for edit and transfer access. Default is 2.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());

    // gather & validate inputs
	vector<unsigned char> vchCert = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);

	string strPubData = "";
	int nAccessFlags = 2;
	strPubData = params[2].get_str();
	nAccessFlags = params[3].get_int();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[4]);

	// check for alias existence in DB
	CAliasIndex toAlias;
	if (!GetAlias(vchAlias, toAlias))
		throw runtime_error("SYSCOIN_CERTIFICATE_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

    // this is a syscoin txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

	CCert theCert;
    if (!GetCert( vchCert, theCert))
        throw runtime_error("SYSCOIN_CERTIFICATE_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a certificate with this key"));

	if (!GetBoolArg("-unittest", false)) {
		ArrivalTimesMap arrivalTimes;
		pcertdb->ReadISArrivalTimes(vchCert, arrivalTimes);
		const int64_t & nNow = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
		for (auto& arrivalTime : arrivalTimes) {
			// if this tx arrived within the minimum latency period flag it as potentially conflicting
			if ((nNow - (arrivalTime.second / 1000)) < ZDAG_MINIMUM_LATENCY_SECONDS) {
				throw runtime_error("SYSCOIN_OFFER_RPC_ERROR: ERRCODE: 2510 - " + _("Please wait a few more seconds and try again..."));
			}
		}
	}

	CAliasIndex fromAlias;
	if(!GetAlias(theCert.vchAlias, fromAlias))
	{
		 throw runtime_error("SYSCOIN_CERTIFICATE_RPC_ERROR: ERRCODE: 2511 - " + _("Could not find the certificate alias"));
	}

	CSyscoinAddress sendAddr;
	GetAddress(toAlias, &sendAddr, scriptPubKeyOrig);
	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CCert copyCert = theCert;
	theCert.ClearCert();
    CScript scriptPubKey;
	theCert.vchAlias = toAlias.vchAlias;

	if(strPubData != stringFromVch(theCert.vchPubData))
		theCert.vchPubData = vchFromString(strPubData);

	theCert.nAccessFlags = nAccessFlags;

	vector<unsigned char> data;
	theCert.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashCert = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_CERT) << CScript::EncodeOP_N(OP_CERT_TRANSFER) << vchHashCert << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
    // send the cert pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyFromOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(fromAlias.vchAlias, vchWitness, aliasRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}


UniValue certinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 > params.size())
        throw runtime_error("certinfo <guid>\n"
                "Show stored values of a single certificate and its .\n");

    vector<unsigned char> vchCert = vchFromValue(params[0]);
	UniValue oCert(UniValue::VOBJ);

	CCert txPos;
	if (!pcertdb || !pcertdb->ReadCert(vchCert, txPos))
		throw runtime_error("SYSCOIN_CERT_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from cert DB"));

	if(!BuildCertJson(txPos, oCert))
		oCert.clear();
    return oCert;
}
bool BuildCertJson(const CCert& cert, UniValue& oCert)
{
    oCert.push_back(Pair("_id", stringFromVch(cert.vchCert)));
    oCert.push_back(Pair("txid", cert.txHash.GetHex()));
    oCert.push_back(Pair("height", (int)cert.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= cert.nHeight) {
		CBlockIndex *pindex = chainActive[cert.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oCert.push_back(Pair("time", nTime));
	oCert.push_back(Pair("title", stringFromVch(cert.vchTitle)));
	oCert.push_back(Pair("publicvalue", stringFromVch(cert.vchPubData)));
	oCert.push_back(Pair("category", stringFromVch(cert.sCategory)));
	oCert.push_back(Pair("alias", stringFromVch(cert.vchAlias)));
	oCert.push_back(Pair("access_flags", cert.nAccessFlags));
	int64_t expired_time = GetCertExpiration(cert);
	bool expired = false;
    if(expired_time <= chainActive.Tip()->GetMedianTimePast())
	{
		expired = true;
	}  


	oCert.push_back(Pair("expires_on", expired_time));
	oCert.push_back(Pair("expired", expired));
	return true;
}
bool BuildCertIndexerHistoryJson(const CCert& cert, UniValue& oCert)
{
	oCert.push_back(Pair("_id", cert.txHash.GetHex()));
	oCert.push_back(Pair("cert", stringFromVch(cert.vchCert)));
	oCert.push_back(Pair("height", (int)cert.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= cert.nHeight) {
		CBlockIndex *pindex = chainActive[cert.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oCert.push_back(Pair("time", nTime));
	oCert.push_back(Pair("title", stringFromVch(cert.vchTitle)));
	oCert.push_back(Pair("publicvalue", stringFromVch(cert.vchPubData)));
	oCert.push_back(Pair("category", stringFromVch(cert.sCategory)));
	oCert.push_back(Pair("alias", stringFromVch(cert.vchAlias)));
	oCert.push_back(Pair("access_flags", cert.nAccessFlags));
	return true;
}
bool BuildCertIndexerJson(const CCert& cert, UniValue& oCert)
{
	oCert.push_back(Pair("_id", stringFromVch(cert.vchCert)));
	oCert.push_back(Pair("title", stringFromVch(cert.vchTitle)));
	oCert.push_back(Pair("height", (int)cert.nHeight));
	oCert.push_back(Pair("category", stringFromVch(cert.sCategory)));
	oCert.push_back(Pair("alias", stringFromVch(cert.vchAlias)));
	oCert.push_back(Pair("expires_on", GetCertExpiration(cert)));
	return true;
}
void CertTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = certFromOp(op);
	CCert cert;
	if(!cert.UnserializeFromData(vchData, vchHash))
		return;

	CCert dbCert;
	GetCert(cert.vchCert, dbCert);
	

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(cert.vchCert)));

	if(!cert.vchTitle.empty() && cert.vchTitle != dbCert.vchTitle)
		entry.push_back(Pair("title", stringFromVch(cert.vchTitle)));

	if(!cert.vchPubData.empty() && cert.vchPubData != dbCert.vchPubData)
		entry.push_back(Pair("publicdata", stringFromVch(cert.vchPubData)));

	if(!cert.vchAlias.empty() && cert.vchAlias != dbCert.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(cert.vchAlias)));

	if(cert.nAccessFlags != dbCert.nAccessFlags)
		entry.push_back(Pair("access_flags", cert.nAccessFlags));




}




