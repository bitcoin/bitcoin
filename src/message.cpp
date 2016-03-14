#include "message.h"
#include "alias.h"
#include "cert.h"
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
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
using namespace std;
extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const CWalletTx* wtxInOffer=NULL, const CWalletTx* wtxInCert=NULL, const CWalletTx* wtxInAlias=NULL, const CWalletTx* wtxInEscrow=NULL, bool syscoinTx=true);
void PutToMessageList(std::vector<CMessage> &messageList, CMessage& index) {
	int i = messageList.size() - 1;
	BOOST_REVERSE_FOREACH(CMessage &o, messageList) {
        if(index.nHeight != 0 && o.nHeight == index.nHeight) {
        	messageList[i] = index;
            return;
        }
        else if(!o.txHash.IsNull() && o.txHash == index.txHash) {
        	messageList[i] = index;
            return;
        }
        i--;
	}
    messageList.push_back(index);
}
bool IsMessageOp(int op) {
    return op == OP_MESSAGE_ACTIVATE;
}

// Increase expiration to 36000 gradually starting at block 24000.
// Use for validation purposes and pass the chain height.
int GetMessageExpirationDepth() {
    return 525600;
}


string messageFromOp(int op) {
    switch (op) {
    case OP_MESSAGE_ACTIVATE:
        return "messageactivate";
    default:
        return "<unknown message op>";
    }
}
bool CMessage::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	if(!GetSyscoinData(tx, vchData))
	{
		SetNull();
		return false;
	}
    try {
        CDataStream dsMessage(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsMessage >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	// extra check to ensure data was parsed correctly
	if(!IsCompressedOrUncompressedPubKey(vchPubKeyTo)
		|| !IsCompressedOrUncompressedPubKey(vchPubKeyFrom))
	{
		SetNull();
		return false;
	}

    return true;
}
const vector<unsigned char> CMessage::Serialize() {
    CDataStream dsMessage(SER_NETWORK, PROTOCOL_VERSION);
    dsMessage << *this;
    const vector<unsigned char> vchData(dsMessage.begin(), dsMessage.end());
    return vchData;

}
//TODO implement
bool CMessageDB::ScanMessages(const std::vector<unsigned char>& vchMessage, unsigned int nMax,
        std::vector<std::pair<std::vector<unsigned char>, CMessage> >& messageScan) {

	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->Seek(make_pair(string("messagei"), vchMessage));
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
		pair<string, vector<unsigned char> > key;
        try {
            if (pcursor->GetKey(key) && key.first == "messagei") {
                vector<unsigned char> vchMessage = key.second;
                vector<CMessage> vtxPos;
                pcursor->GetValue(vtxPos);
                CMessage txPos;
                if (!vtxPos.empty())
                    txPos = vtxPos.back();
                messageScan.push_back(make_pair(vchMessage, txPos));
            }
            if (messageScan.size() >= nMax)
                break;

            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    return true;
}

int IndexOfMessageOutput(const CTransaction& tx) {
	if (tx.nVersion != SYSCOIN_TX_VERSION)
		return -1;
    vector<vector<unsigned char> > vvch;
    int op, nOut;
	bool good = DecodeMessageTx(tx, op, nOut, vvch);
	if (!good)
		return -1;
    return nOut;
}


bool GetTxOfMessage(const vector<unsigned char> &vchMessage,
        CMessage& txPos, CTransaction& tx) {
    vector<CMessage> vtxPos;
    if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (nHeight + GetMessageExpirationDepth()
            < chainActive.Tip()->nHeight) {
        string message = stringFromVch(vchMessage);
        LogPrintf("GetTxOfMessage(%s) : expired", message.c_str());
        return false;
    }

    if (!GetSyscoinTransaction(nHeight, txPos.txHash, tx, Params().GetConsensus()))
        return error("GetTxOfMessage() : could not read tx from disk");

    return true;
}
bool DecodeAndParseMessageTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch)
{
	CMessage message;
	bool decode = DecodeMessageTx(tx, op, nOut, vvch);
	bool parse = message.UnserializeFromTx(tx);
	return decode && parse;
}
bool DecodeMessageTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeMessageScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
	if (!found) vvch.clear();
    return found && IsMessageOp(op);
}

bool DecodeMessageScript(const CScript& script, int& op,
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

    if ((op == OP_MESSAGE_ACTIVATE && vvch.size() == 1))
        return true;

    return false;
}

bool DecodeMessageScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeMessageScript(script, op, vvch, pc);
}

CScript RemoveMessageScriptPrefix(const CScript& scriptIn) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeMessageScript(scriptIn, op, vvch, pc))
	{
        throw runtime_error("RemoveMessageScriptPrefix() : could not decode message script");
	}
	
    return CScript(pc, scriptIn.end());
}

bool CheckMessageInputs(const CTransaction &tx, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const CBlock& block) {
	
	if (tx.IsCoinBase())
		return true;
	LogPrintf("*** %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");
    const COutPoint *prevOutput = NULL;
    CCoins prevCoins;

	int prevAliasOp = 0;
	
    vector<vector<unsigned char> > vvchPrevAliasArgs;
	if(fJustCheck)
	{
		// Strict check - bug disallowed
		for (unsigned int i = 0; i < tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			int op;
			prevOutput = &tx.vin[i].prevout;
			if(!prevOutput)
				continue;
			// ensure inputs are unspent when doing consensus check to add to block
			if(!inputs.GetCoins(prevOutput->hash, prevCoins))
				continue;
			if(!IsSyscoinScript(prevCoins.vout[prevOutput->n].scriptPubKey, op, vvch))
				continue;
			if (IsAliasOp(op))
			{
				prevAliasOp = op;
				vvchPrevAliasArgs = vvch;
				break;
			}
		}	
	}
    // Make sure message outputs are not spent by a regular transaction, or the message would be lost
    if (tx.nVersion != SYSCOIN_TX_VERSION) {
		LogPrintf("CheckMessageInputs() : non-syscoin transaction\n");
        return true;
    }
    vector<vector<unsigned char> > vvchArgs;
    int op, nOut;
    bool good = DecodeMessageTx(tx, op, nOut, vvchArgs);
    if (!good)
        return error("CheckMessageInputs() : could not decode a syscoin tx");
    // unserialize message UniValue from txn, check for valid
    CMessage theMessage;
    theMessage.UnserializeFromTx(tx);
    if (theMessage.IsNull())
        return error("CheckMessageInputs() : null message");
    if (vvchArgs[0].size() > MAX_NAME_LENGTH)
        return error("message tx GUID too big");
	if(!IsCompressedOrUncompressedPubKey(theMessage.vchPubKeyTo))
	{
		return error("message public key to, invalid length");
	}
	if(!IsCompressedOrUncompressedPubKey(theMessage.vchPubKeyFrom))
	{
		return error("message public key from, invalid length");
	}
	if(theMessage.vchSubject.size() > MAX_NAME_LENGTH)
	{
		return error("message subject too big");
	}
	if(theMessage.vchMessageTo.size() > MAX_ENCRYPTED_VALUE_LENGTH)
	{
		return error("message data to too big");
	}
	if(theMessage.vchMessageFrom.size() > MAX_ENCRYPTED_VALUE_LENGTH)
	{
		return error("message data from too big");
	}
	if(fJustCheck)
	{

	if(op == OP_MESSAGE_ACTIVATE)
		{
			if(!IsAliasOp(prevAliasOp))
				return error("CheckMessageInputs(): alias not provided as input");
			
			vector<CAliasIndex> vtxPos;
			if (!paliasdb->ReadAlias(vvchPrevAliasArgs[0], vtxPos))
				return error("CheckMessageInputs(): failed to read alias from alias DB");
			if (vtxPos.size() < 1)
				return error("CheckMessageInputs(): no alias result returned");
			if(vtxPos.back().vchPubKey != theMessage.vchPubKeyFrom)
				return error("CheckMessageInputs() OP_MESSAGE_ACTIVATE: alias and message from pubkey's must match");
		
		}
		else
			return error( "CheckMessageInputs() : message transaction has unknown op");
	}
	// save serialized message for later use
	CMessage serializedMessage = theMessage;


    if (!fJustCheck ) {
		vector<CMessage> vtxPos;
		if (pmessagedb->ExistsMessage(vvchArgs[0])) {
			if (!pmessagedb->ReadMessage(vvchArgs[0], vtxPos))
				return error(
						"CheckMessageInputs() : failed to read from message DB");
		}      
        // set the message's txn-dependent values
		theMessage.txHash = tx.GetHash();
		theMessage.nHeight = nHeight;
		PutToMessageList(vtxPos, theMessage);
        // write message  
        if (!pmessagedb->WriteMessage(vvchArgs[0], vtxPos))
            return error( "CheckMessageInputs() : failed to write to message DB");
		
      			
        // debug
		if(fDebug)
			LogPrintf( "CONNECTED MESSAGE: op=%s message=%s hash=%s height=%d\n",
                messageFromOp(op).c_str(),
                stringFromVch(vvchArgs[0]).c_str(),
                tx.GetHash().ToString().c_str(),
                nHeight);
	}
    return true;
}

UniValue messagenew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 4 )
        throw runtime_error(
		"messagenew <subject> <message> <fromalias> <toalias>\n"
						"<subject> Subject of message.\n"
						"<message> Message to send to alias.\n"
						"<fromalias> Alias to send message from.\n"
						"<toalias> Alias to send message to.\n"					
                        + HelpRequiringPassphrase());
	vector<unsigned char> vchMySubject = vchFromValue(params[0]);
    if (vchMySubject.size() > MAX_NAME_LENGTH)
        throw runtime_error("Subject cannot exceed 255 bytes!");

	vector<unsigned char> vchMyMessage = vchFromValue(params[1]);
	string strFromAddress = params[2].get_str();
	string strToAddress = params[3].get_str();
	std::vector<unsigned char> vchFromPubKey;
	std::vector<unsigned char> vchToPubKey;
	CSyscoinAddress fromAddress, toAddress;
	EnsureWalletIsUnlocked();


	fromAddress = CSyscoinAddress(strFromAddress);
	if (!fromAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!fromAddress.isAlias)
		throw runtime_error("Invalid alias");

	CAliasIndex alias;
	CTransaction aliastx;
	if (!GetTxOfAlias(vchFromString(strFromAddress), alias, aliastx))
		throw runtime_error("could not find an alias with this name");
	// check for existing pending alias updates
	if (ExistsInMempool(vchFromString(strFromAddress), OP_ALIAS_UPDATE)) {
		throw runtime_error("there are pending operations on that alias");
	}
    if(!IsSyscoinTxMine(aliastx, "alias")) {
		throw runtime_error("This alias is not yours.");
    }
	const CWalletTx *wtxAliasIn = pwalletMain->GetWalletTx(aliastx.GetHash());
	if (wtxAliasIn == NULL)
		throw runtime_error("this alias is not in your wallet");
	vchFromPubKey = alias.vchPubKey;
	CScript scriptPubKeyOrig, scriptPubKeyAliasOrig, scriptPubKey, scriptPubKeyAlias;

	CPubKey FromPubKey = CPubKey(vchFromPubKey);
	if(!FromPubKey.IsValid())
	{
		throw runtime_error("Invalid sending public key");
	}
	scriptPubKeyAliasOrig = GetScriptForDestination(FromPubKey.GetID());
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << vchFromString(strFromAddress) << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyAliasOrig;		


	toAddress = CSyscoinAddress(strToAddress);
	if (!toAddress.IsValid())
		throw runtime_error("Invalid syscoin address");
	if (!toAddress.isAlias)
		throw runtime_error("Invalid alias");

	vector<CAliasIndex> vtxAliasPos;
	if (!paliasdb->ReadAlias(vchFromString(toAddress.aliasName), vtxAliasPos))
		throw runtime_error("failed to read alias from alias DB");
	if (vtxAliasPos.size() < 1)
		throw runtime_error("no result returned");
	alias = vtxAliasPos.back();
	vchToPubKey = alias.vchPubKey;
	CPubKey ToPubKey = CPubKey(vchToPubKey);
	if(!ToPubKey.IsValid())
	{
		throw runtime_error("Invalid recv public key");
	}
	

	if (!IsMine(*pwalletMain, fromAddress.Get()))
		throw runtime_error("fromalias must be yours");


    // gather inputs
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
    vector<unsigned char> vchMessage = vchFromValue(HexStr(vchRand));

    // this is a syscoin transaction
    CWalletTx wtx;
	scriptPubKeyOrig= GetScriptForDestination(ToPubKey.GetID());
	scriptPubKey << CScript::EncodeOP_N(OP_MESSAGE_ACTIVATE) << vchMessage << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;


	string strCipherTextTo;
	if(!EncryptMessage(vchToPubKey, vchMyMessage, strCipherTextTo))
	{
		throw runtime_error("Could not encrypt message data for receiver!");
	}
	string strCipherTextFrom;
	if(!EncryptMessage(vchFromPubKey, vchMyMessage, strCipherTextFrom))
	{
		throw runtime_error("Could not encrypt message data for sender!");
	}

    if (strCipherTextFrom.size() > MAX_ENCRYPTED_VALUE_LENGTH)
        throw runtime_error("Message data cannot exceed 1023 bytes!");
    if (strCipherTextTo.size() > MAX_ENCRYPTED_VALUE_LENGTH)
        throw runtime_error("Message data cannot exceed 1023 bytes!");

    // build message
    CMessage newMessage;
	newMessage.vchMessageFrom = vchFromString(strCipherTextFrom);
	newMessage.vchMessageTo = vchFromString(strCipherTextTo);
	newMessage.vchSubject = vchMySubject;
	newMessage.vchPubKeyFrom = vchFromPubKey;
	newMessage.vchPubKeyTo = vchToPubKey;
	newMessage.nHeight = chainActive.Tip()->nHeight;
	
	// send the tranasction
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	// we attach alias input here so noone can create an offer under anyone elses alias/pubkey
	if(wtxAliasIn != NULL)
		vecSend.push_back(aliasRecipient);

	const vector<unsigned char> &data = newMessage.Serialize();
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	const CWalletTx * wtxInOffer=NULL;
	const CWalletTx * wtxInEscrow=NULL;
	const CWalletTx * wtxInCert=NULL;
	SendMoneySyscoin(vecSend, recipient.nAmount+fee.nAmount, false, wtx, wtxInOffer, wtxInCert, wtxAliasIn, wtxInEscrow);
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}

UniValue messageinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("messageinfo <guid>\n"
                "Show stored values of a single message.\n");

    vector<unsigned char> vchMessage = vchFromValue(params[0]);

    // look for a transaction with this key, also returns
    // an message UniValue if it is found
    CTransaction tx;

	vector<CMessage> vtxPos;

    UniValue oMessage(UniValue::VOBJ);
    vector<unsigned char> vchValue;

	if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
		 throw runtime_error("failed to read from message DB");
	CMessage ca = vtxPos.back();

    string sHeight = strprintf("%llu", ca.nHeight);
	oMessage.push_back(Pair("GUID", stringFromVch(vchMessage)));
	string sTime;
	CBlockIndex *pindex = chainActive[ca.nHeight];
	if (pindex) {
		sTime = strprintf("%llu", pindex->nTime);
	}
	CPubKey FromPubKey(ca.vchPubKeyFrom);
	CSyscoinAddress fromaddress(FromPubKey.GetID());
	fromaddress = CSyscoinAddress(fromaddress.ToString());
	oMessage.push_back(Pair("time", sTime));
	if(fromaddress.isAlias)
		oMessage.push_back(Pair("from", fromaddress.aliasName));
	else
		oMessage.push_back(Pair("from", fromaddress.ToString()));

	CPubKey ToPubKey(ca.vchPubKeyTo);
	CSyscoinAddress toaddress(ToPubKey.GetID());
	toaddress = CSyscoinAddress(toaddress.ToString());
	if(toaddress.isAlias)
		oMessage.push_back(Pair("to", toaddress.aliasName));
	else
		oMessage.push_back(Pair("to", toaddress.ToString()));

	oMessage.push_back(Pair("subject", stringFromVch(ca.vchSubject)));
	string strDecrypted = "";
	if(DecryptMessage(ca.vchPubKeyTo, ca.vchMessageTo, strDecrypted))
		oMessage.push_back(Pair("message", strDecrypted));
	else if(DecryptMessage(ca.vchPubKeyFrom, ca.vchMessageFrom, strDecrypted))
		oMessage.push_back(Pair("message", strDecrypted));
	else
		oMessage.push_back(Pair("message", stringFromVch(ca.vchMessageTo)));

    oMessage.push_back(Pair("txid", ca.txHash.GetHex()));
    oMessage.push_back(Pair("height", sHeight));
	
    return oMessage;
}

UniValue messagelist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size())
        throw runtime_error("messagelist [<message>]\n"
                "list my own messages");
	vector<unsigned char> vchName;

	if (params.size() == 1)
		vchName = vchFromValue(params[0]);

    UniValue oRes(UniValue::VARR);
    uint256 hash;
    CTransaction tx, dbtx;

    vector<unsigned char> vchValue;
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
    {
        // get txn hash, read txn index
        hash = item.second.GetHash();
		const CWalletTx &wtx = item.second;

        // skip non-syscoin txns
        if (wtx.nVersion != SYSCOIN_TX_VERSION)
            continue;
		// decode txn, skip non-alias txns
		vector<vector<unsigned char> > vvch;
		int op, nOut;
		if (!DecodeMessageTx(wtx, op, nOut, vvch) || !IsMessageOp(op))
			continue;
		vchName = vvch[0];
		vector<CMessage> vtxPos;
		CMessage message;
		int pending = 0;
		if (!pmessagedb->ReadMessage(vchName, vtxPos) || vtxPos.empty())
		{
			pending = 1;
			message = CMessage(wtx);
			if(!IsSyscoinTxMine(wtx, "message"))
				continue;
		}
		else
		{
			message = vtxPos.back();
			CTransaction tx;
			if (!GetSyscoinTransaction(message.nHeight, message.txHash, tx, Params().GetConsensus()))
				continue;
			if (!DecodeMessageTx(tx, op, nOut, vvch) || !IsMessageOp(op))
				continue;
			if(!IsSyscoinTxMine(tx, "message"))
				continue;
		}

        // build the output
        UniValue oName(UniValue::VOBJ);
        oName.push_back(Pair("GUID", stringFromVch(vchName)));

		string sTime;
		CBlockIndex *pindex = chainActive[message.nHeight];
		if (pindex) {
			sTime = strprintf("%llu", pindex->nTime);
		}
        string strAddress = "";
		oName.push_back(Pair("time", sTime));

		CPubKey FromPubKey(message.vchPubKeyFrom);
		CSyscoinAddress fromaddress(FromPubKey.GetID());
		fromaddress = CSyscoinAddress(fromaddress.ToString());
		if(fromaddress.isAlias)
			oName.push_back(Pair("from", fromaddress.aliasName));
		else
			oName.push_back(Pair("from", fromaddress.ToString()));

		CPubKey ToPubKey(message.vchPubKeyTo);
		CSyscoinAddress toaddress(ToPubKey.GetID());
		toaddress = CSyscoinAddress(toaddress.ToString());
		if(toaddress.isAlias)
			oName.push_back(Pair("to", toaddress.aliasName));
		else
			oName.push_back(Pair("to", toaddress.ToString()));


		oName.push_back(Pair("subject", stringFromVch(message.vchSubject)));
		string strDecrypted = "";
		string strData = string("Encrypted for owner of message");
		if(DecryptMessage(message.vchPubKeyTo, message.vchMessageTo, strDecrypted))
			strData = strDecrypted;
		else if(DecryptMessage(message.vchPubKeyFrom, message.vchMessageFrom, strDecrypted))
			strData = strDecrypted;
		oName.push_back(Pair("message", strData));
		oRes.push_back(oName);
	}

    return oRes;
}


UniValue messagesentlist(const UniValue& params, bool fHelp) {
    if (fHelp || 1 < params.size())
        throw runtime_error("messagesentlist [<message>]\n"
                "list my sent messages");
	vector<unsigned char> vchName;

	if (params.size() == 1)
		vchName = vchFromValue(params[0]);

    UniValue oRes(UniValue::VARR);
    uint256 hash;
    CTransaction tx, dbtx;

    vector<unsigned char> vchValue;
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, pwalletMain->mapWallet)
    {
        // get txn hash, read txn index
        hash = item.second.GetHash();
		const CWalletTx &wtx = item.second;        // skip non-syscoin txns
        if (wtx.nVersion != SYSCOIN_TX_VERSION)
            continue;
		// decode txn, skip non-alias txns
		vector<vector<unsigned char> > vvch;
		int op, nOut;
		if (!DecodeMessageTx(wtx, op, nOut, vvch) || !IsMessageOp(op))
			continue;
		vchName = vvch[0];

		vector<CMessage> vtxPos;
		CMessage message;
		int pending = 0;
		if (!pmessagedb->ReadMessage(vchName, vtxPos) || vtxPos.empty())
		{
			pending = 1;
			message = CMessage(wtx);
			if(IsSyscoinTxMine(wtx, "message"))
				continue;
		}
		else
		{
			message = vtxPos.back();
			CTransaction tx;
			if (!GetSyscoinTransaction(message.nHeight, message.txHash, tx, Params().GetConsensus()))
				continue;
			if (!DecodeMessageTx(tx, op, nOut, vvch) || !IsMessageOp(op))
				continue;
			if(IsSyscoinTxMine(tx, "message"))
				continue;
		}

        // build the output
        UniValue oName(UniValue::VOBJ);
        oName.push_back(Pair("GUID", stringFromVch(vchName)));

		string sTime;
		CBlockIndex *pindex = chainActive[message.nHeight];
		if (pindex) {
			sTime = strprintf("%llu", pindex->nTime);
		}
        string strAddress = "";
		oName.push_back(Pair("time", sTime));
		CPubKey FromPubKey(message.vchPubKeyFrom);
		CSyscoinAddress fromaddress(FromPubKey.GetID());
		fromaddress = CSyscoinAddress(fromaddress.ToString());
		if(fromaddress.isAlias)
			oName.push_back(Pair("from", fromaddress.aliasName));
		else
			oName.push_back(Pair("from", fromaddress.ToString()));

		CPubKey ToPubKey(message.vchPubKeyTo);
		CSyscoinAddress toaddress(ToPubKey.GetID());
		toaddress = CSyscoinAddress(toaddress.ToString());
		if(toaddress.isAlias)
			oName.push_back(Pair("to", toaddress.aliasName));
		else
			oName.push_back(Pair("to", toaddress.ToString()));

		oName.push_back(Pair("subject", stringFromVch(message.vchSubject)));
		string strDecrypted = "";
		string strData = string("Encrypted for owner of message");
		if(DecryptMessage(message.vchPubKeyTo, message.vchMessageTo, strDecrypted))
			strData = strDecrypted;
		else if(DecryptMessage(message.vchPubKeyFrom, message.vchMessageFrom, strDecrypted))
			strData = strDecrypted;
		oName.push_back(Pair("message", strData));
		oRes.push_back(oName);
	}

    return oRes;
}
UniValue messagehistory(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("messagehistory <message>\n"
                "List all stored values of a message.\n");

    UniValue oRes(UniValue::VARR);
    vector<unsigned char> vchMessage = vchFromValue(params[0]);
    string message = stringFromVch(vchMessage);

    {
        vector<CMessage> vtxPos;
        if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
            throw runtime_error("failed to read from message DB");

        CMessage txPos2;
        uint256 txHash;
        uint256 blockHash;
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
            if (!DecodeMessageTx(tx, op, nOut, vvch) 
            	|| !IsMessageOp(op) )
                continue;
            UniValue oMessage(UniValue::VOBJ);
            vector<unsigned char> vchValue;
            uint64_t nHeight;
            nHeight = txPos2.nHeight;
            oMessage.push_back(Pair("GUID", message));
			string opName = messageFromOp(op);
			oMessage.push_back(Pair("messagetype", opName));
			string sTime;
			CBlockIndex *pindex = chainActive[nHeight];
			if (pindex) {
				sTime = strprintf("%llu", pindex->nTime);
			}
			oMessage.push_back(Pair("time", sTime));

			CPubKey FromPubKey(txPos2.vchPubKeyFrom);
			CSyscoinAddress fromaddress(FromPubKey.GetID());
			fromaddress = CSyscoinAddress(fromaddress.ToString());
			if(fromaddress.isAlias)
				oMessage.push_back(Pair("from", fromaddress.aliasName));
			else
				oMessage.push_back(Pair("from", fromaddress.ToString()));

			CPubKey ToPubKey(txPos2.vchPubKeyTo);
			CSyscoinAddress toaddress(ToPubKey.GetID());
			toaddress = CSyscoinAddress(toaddress.ToString());
			if(toaddress.isAlias)
				oMessage.push_back(Pair("to", toaddress.aliasName));
			else
				oMessage.push_back(Pair("to", toaddress.ToString()));

			oMessage.push_back(Pair("subject", stringFromVch(txPos2.vchSubject)));
			string strDecrypted = "";
			string strData = string("Encrypted for owner of message");
			if(DecryptMessage(txPos2.vchPubKeyTo, txPos2.vchMessageTo, strDecrypted))
				strData = strDecrypted;
			else if(DecryptMessage(txPos2.vchPubKeyFrom, txPos2.vchMessageFrom, strDecrypted))
				strData = strDecrypted;

			oMessage.push_back(Pair("message", strData));
            oRes.push_back(oMessage);
		}
	}
    return oRes;
}



