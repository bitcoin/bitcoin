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

extern void SendMoneySyscoin(const vector<CRecipient> &vecSend, CAmount nValue, bool fSubtractFeeFromAmount, CWalletTx& wtxNew, const string& txData="", const CWalletTx* wtxIn=NULL);
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

int64_t GetMessageNetworkFee(opcodetype seed, unsigned int nHeight) {

	int64_t nFee = 0;
	int64_t nRate = 0;
	const vector<unsigned char> &vchCurrency = vchFromString("USD");
	vector<string> rateList;
	int precision;
	if(getCurrencyToSYSFromAlias(vchCurrency, nRate, nHeight, rateList, precision) != "")
		{
		if(seed==OP_MESSAGE_ACTIVATE) 
		{
			nFee = 150 * COIN;
		}
	}
	else
	{
		// 1000 pips USD (1 cent), 10k pips = $1USD
		nFee = nRate/10;
	}
	// Round up to CENT
	nFee += CENT - 1;
	nFee = (nFee / CENT) * CENT;
	return nFee;
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
    try {
        CDataStream dsMessage(vchFromString(DecodeBase64(stringFromVch(tx.data))), SER_NETWORK, PROTOCOL_VERSION);
        dsMessage >> *this;
    } catch (std::exception &e) {
        return false;
    }
    return true;
}

string CMessage::SerializeToString() {
    // serialize message UniValue
    CDataStream dsMessage(SER_NETWORK, PROTOCOL_VERSION);
    dsMessage << *this;
    vector<unsigned char> vchData(dsMessage.begin(), dsMessage.end());
    return EncodeBase64(vchData.data(), vchData.size());
}

//TODO implement
bool CMessageDB::ScanMessages(const std::vector<unsigned char>& vchMessage, unsigned int nMax,
        std::vector<std::pair<std::vector<unsigned char>, CMessage> >& messageScan) {

    CDBIterator *pcursor = pmessagedb->NewIterator();

    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(string("messagei"), vchMessage);
    pcursor->Seek(ssKeySet.str());

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
    delete pcursor;
    return true;
}

/**
 * [CMessageDB::ReconstructMessageIndex description]
 * @param  pindexRescan [description]
 * @return              [description]
 */
bool CMessageDB::ReconstructMessageIndex(CBlockIndex *pindexRescan) {
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

            // decode the message op, params, height
            bool o = DecodeMessageTx(tx, op, nOut, vvchArgs, -1);
            if (!o || !IsMessageOp(op)) continue;

            vector<unsigned char> vchMessage = vvchArgs[0];

            // get the transaction
            if(!GetTransaction(tx.GetHash(), tx, Params().GetConsensus(), txblkhash, true))
                continue;

            // attempt to read message from txn
            CMessage txMessage;
            if(!txMessage.UnserializeFromTx(tx))
                return error("ReconstructMessageIndex() : failed to unserialize message from tx");

            // save serialized message
            CMessage serializedMessage = txMessage;

            // read message from DB if it exists
            vector<CMessage> vtxPos;
            if (ExistsMessage(vchMessage)) {
                if (!ReadMessage(vchMessage, vtxPos))
                    return error("ReconstructMessageIndex() : failed to read message from DB");
            }

            txMessage.txHash = tx.GetHash();
            txMessage.nHeight = nHeight;
            // txn-specific values to message UniValue
            txMessage.vchRand = vvchArgs[0];
            PutToMessageList(vtxPos, txMessage);

            if (!WriteMessage(vchMessage, vtxPos))
                return error("ReconstructMessageIndex() : failed to write to message DB");

          
            printf( "RECONSTRUCT MESSAGE: op=%s message=%s hash=%s height=%d\n",
                    messageFromOp(op).c_str(),
                    stringFromVch(vvchArgs[0]).c_str(),
                    tx.GetHash().ToString().c_str(),
                    nHeight);
        }
        pindex = chainActive.Next(pindex);
        
    }
    }
    return true;
}



int64_t GetMessageNetFee(const CTransaction& tx) {
    int64_t nFee = 0;
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        if (out.scriptPubKey.size() == 1 && out.scriptPubKey[0] == OP_RETURN)
            nFee += out.nValue;
    }
    return nFee;
}

int GetMessageHeight(vector<unsigned char> vchMessage) {
    vector<CMessage> vtxPos;
    if (pmessagedb->ExistsMessage(vchMessage)) {
        if (!pmessagedb->ReadMessage(vchMessage, vtxPos))
            return error("GetMessageHeight() : failed to read from message DB");
        if (vtxPos.empty()) return -1;
        CMessage& txPos = vtxPos.back();
        return txPos.nHeight;
    }
    return -1;
}


int IndexOfMessageOutput(const CTransaction& tx) {
    vector<vector<unsigned char> > vvch;
    int op, nOut;
    if (!DecodeMessageTx(tx, op, nOut, vvch, -1))
        throw runtime_error("IndexOfMessageOutput() : message output not found");
    return nOut;
}

bool GetNameOfMessageTx(const CTransaction& tx, vector<unsigned char>& message) {
    if (tx.nVersion != SYSCOIN_TX_VERSION)
        return false;
    vector<vector<unsigned char> > vvchArgs;
    int op, nOut;
    if (!DecodeMessageTx(tx, op, nOut, vvchArgs, -1))
        return error("GetNameOfMessageTx() : could not decode a syscoin tx");

    switch (op) {
        case OP_MESSAGE_ACTIVATE:
            message = vvchArgs[0];
            return true;
    }
    return false;
}

bool GetValueOfMessageTx(const CTransaction& tx, vector<unsigned char>& value) {
    vector<vector<unsigned char> > vvch;
    int op, nOut;

    if (!DecodeMessageTx(tx, op, nOut, vvch, -1))
        return false;

    switch (op) {
    case OP_MESSAGE_ACTIVATE:
        value = vvch[1];
        return true;
    default:
        return false;
    }
}

bool IsMessageMine(const CTransaction& tx) {
    if (tx.nVersion != SYSCOIN_TX_VERSION)
        return false;

    vector<vector<unsigned char> > vvch;
    int op, nOut;

    bool good = DecodeMessageTx(tx, op, nOut, vvch, -1);
    if (!good) 
        return false;
    
    if(!IsMessageOp(op))
        return false;

    const CTxOut& txout = tx.vout[nOut];
    if (pwalletMain->IsMine(txout)) {     
        return true;
    }
    return false;
}


bool GetValueOfMessageTxHash(const uint256 &txHash,
        vector<unsigned char>& vchValue, uint256& hash, int& nHeight) {
    nHeight = GetTxHashHeight(txHash);
    CTransaction tx;
    uint256 blockHash;
    if (!GetTransaction(txHash, tx, Params().GetConsensus(), blockHash, true))
        return error("GetValueOfMessageTxHash() : could not read tx from disk");
    if (!GetValueOfMessageTx(tx, vchValue))
        return error("GetValueOfMessageTxHash() : could not decode value from tx");
    hash = tx.GetHash();
    return true;
}

bool GetValueOfMessage(CMessageDB& dbMessage, const vector<unsigned char> &vchMessage,
        vector<unsigned char>& vchValue, int& nHeight) {
    vector<CMessage> vtxPos;
    if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
        return false;

    CMessage& txPos = vtxPos.back();
    nHeight = txPos.nHeight;
    vchValue = txPos.vchRand;
    return true;
}

bool GetTxOfMessage(CMessageDB& dbMessage, const vector<unsigned char> &vchMessage,
        CMessage& txPos, CTransaction& tx) {
    vector<CMessage> vtxPos;
    if (!pmessagedb->ReadMessage(vchMessage, vtxPos) || vtxPos.empty())
        return false;
    txPos = vtxPos.back();
    int nHeight = txPos.nHeight;
    if (nHeight + GetMessageExpirationDepth()
            < chainActive.Tip()->nHeight) {
        string message = stringFromVch(vchMessage);
        printf("GetTxOfMessage(%s) : expired", message.c_str());
        return false;
    }

    uint256 hashBlock;
    if (!GetTransaction(txPos.txHash, tx, Params().GetConsensus(), hashBlock, true))
        return error("GetTxOfMessage() : could not read tx from disk");

    return true;
}

bool DecodeMessageTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch, int nHeight) {
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
    return found;
}

bool DecodeMessageScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeMessageScript(script, op, vvch, pc);
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

    if ((op == OP_MESSAGE_ACTIVATE && vvch.size() == 2))
        return true;

    return false;
}

bool GetMessageAddress(const CTransaction& tx, std::string& strAddress) {
    int op, nOut = 0;
    vector<vector<unsigned char> > vvch;
    if (!DecodeMessageTx(tx, op, nOut, vvch, -1))
        return error("GetMessageAddress() : could not decode message tx.");

    const CTxOut& txout = tx.vout[nOut];

    const CScript& scriptPubKey = RemoveMessageScriptPrefix(txout.scriptPubKey);
	CTxDestination dest;
	ExtractDestination(scriptPubKey, dest);
	strAddress = CSyscoinAddress(dest).ToString();
    return true;
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

bool CheckMessageInputs(const CTransaction &tx,
        CValidationState &state, const CCoinsViewCache &inputs, bool fBlock, bool fMiner,
        bool fJustCheck, int nHeight) {

    if (!tx.IsCoinBase()) {
			printf("*** %d %d %s %s %s %s\n", nHeight,
				chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
				fBlock ? "BLOCK" : "", fMiner ? "MINER" : "",
				fJustCheck ? "JUSTCHECK" : "");

        bool found = false;
        const COutPoint *prevOutput = NULL;
        CCoins prevCoins;

        int prevOp;
        vector<vector<unsigned char> > vvchPrevArgs;
		vvchPrevArgs.clear();
        // Strict check - bug disallowed
		for (int i = 0; i < (int) tx.vin.size(); i++) {
			vector<vector<unsigned char> > vvch;
			prevOutput = &tx.vin[i].prevout;
			inputs.GetCoins(prevOutput->hash, prevCoins);
			if(DecodeMessageScript(prevCoins.vout[prevOutput->n].scriptPubKey, prevOp, vvch))
			{
				vvchPrevArgs = vvch;
				found = true;
				break;
			}
			if(!found)vvchPrevArgs.clear();
			
		}
		
        // Make sure message outputs are not spent by a regular transaction, or the message would be lost
        if (tx.nVersion != SYSCOIN_TX_VERSION) {
            if (found)
                return error(
                        "CheckMessageInputs() : a non-syscoin transaction with a syscoin input");
			printf("CheckMessageInputs() : non-syscoin transaction\n");
            return true;
        }
        vector<vector<unsigned char> > vvchArgs;
        int op, nOut;
        bool good = DecodeMessageTx(tx, op, nOut, vvchArgs, -1);
        if (!good)
            return error("CheckMessageInputs() : could not decode a syscoin tx");
        int nDepth;
        int64_t nNetFee;
        // unserialize message UniValue from txn, check for valid
        CMessage theMessage;
        theMessage.UnserializeFromTx(tx);
        if (theMessage.IsNull())
            return error("CheckMessageInputs() : null message");
		if(theMessage.vchRand.size() > MAX_ID_LENGTH)
		{
			return error("message rand too big");
		}
        if (vvchArgs[0].size() > MAX_NAME_LENGTH)
            return error("message tx GUID too big");
		if (vvchArgs[1].size() > MAX_ID_LENGTH)
			return error("message tx rand too big");
		if(theMessage.vchPubKeyTo.size() > MAX_NAME_LENGTH)
		{
			return error("message public key to, too big");
		}
		if(theMessage.vchPubKeyFrom.size() > MAX_NAME_LENGTH)
		{
			return error("message public key from, too big");
		}
		if(theMessage.vchSubject.size() > MAX_NAME_LENGTH)
		{
			return error("message subject too big");
		}
		if(theMessage.vchFrom.size() > MAX_NAME_LENGTH)
		{
			return error("message from too big");
		}
		if(theMessage.vchTo.size() > MAX_NAME_LENGTH)
		{
			return error("message to too big");
		}
		if(theMessage.vchMessageTo.size() > MAX_MSG_LENGTH)
		{
			return error("message data to too big");
		}
		if(theMessage.vchMessageFrom.size() > MAX_MSG_LENGTH)
		{
			return error("message data from too big");
		}
        switch (op) {
        case OP_MESSAGE_ACTIVATE:
			if (fBlock && !fJustCheck) {

					// check for enough fees
				nNetFee = GetMessageNetFee(tx);
				if (nNetFee < GetMessageNetworkFee(OP_MESSAGE_ACTIVATE, theMessage.nHeight))
					return error(
							"CheckMessageInputs() : OP_MESSAGE_ACTIVATE got tx %s with fee too low %lu",
							tx.GetHash().GetHex().c_str(),
							(long unsigned int) nNetFee);		
			}
            break;
        default:
            return error( "CheckMessageInputs() : message transaction has unknown op");
        }



        // these ifs are problably total bullshit except for the messagenew
        if (fBlock || (!fBlock && !fMiner && !fJustCheck)) {
			// save serialized message for later use
			CMessage serializedMessage = theMessage;

			// if not an messagenew, load the message data from the DB
			vector<CMessage> vtxPos;
			if (pmessagedb->ExistsMessage(vvchArgs[0]) && !fJustCheck) {
				if (!pmessagedb->ReadMessage(vvchArgs[0], vtxPos))
					return error(
							"CheckMessageInputs() : failed to read from message DB");
			}
            if (!fMiner && !fJustCheck && chainActive.Tip()->nHeight != nHeight) {
                int nHeight = chainActive.Tip()->nHeight;
                // set the message's txn-dependent values
				theMessage.txHash = tx.GetHash();
				theMessage.nHeight = nHeight;
                theMessage.vchRand = vvchArgs[0];
				PutToMessageList(vtxPos, theMessage);
				{
				TRY_LOCK(cs_main, cs_trymain);
                // write message  
                if (!pmessagedb->WriteMessage(vvchArgs[0], vtxPos))
                    return error( "CheckMessageInputs() : failed to write to message DB");

              			
                // debug
				if(fDebug)
					printf( "CONNECTED MESSAGE: op=%s message=%s hash=%s height=%d\n",
                        messageFromOp(op).c_str(),
                        stringFromVch(vvchArgs[0]).c_str(),
                        tx.GetHash().ToString().c_str(),
                        nHeight);
				}
            }
            
        }
    }
    return true;
}

void rescanformessages(CBlockIndex *pindexRescan) {
    printf("Scanning blockchain for messages to create fast index...\n");
    pmessagedb->ReconstructMessageIndex(pindexRescan);
}


UniValue getmessagefees(const UniValue& params, bool fHelp) {
    if (fHelp || 0 != params.size())
        throw runtime_error(
                "getmessagefees\n"
                        "get current service fees for message transactions\n");
    UniValue oRes(UniValue::VARR);
    oRes.push_back(Pair("height", chainActive.Tip()->nHeight ));
    oRes.push_back(Pair("activate_fee", ValueFromAmount(GetMessageNetworkFee(OP_MESSAGE_ACTIVATE, chainActive.Tip()->nHeight) )));
    return oRes;

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
	if(!HasReachedMainNetForkB2())
		throw runtime_error("Please wait until B2 hardfork starts in before executing this command.");
	vector<unsigned char> vchMySubject = vchFromValue(params[0]);
    if (vchMySubject.size() > MAX_NAME_LENGTH)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Subject cannot exceed 255 bytes!");

	vector<unsigned char> vchMyMessage = vchFromValue(params[1]);
	vector<unsigned char> vchFromAliasOrPubKey = vchFromValue(params[2]);
	vector<unsigned char> vchToAliasOrPubKey = vchFromValue(params[3]);

	string strFromAddress = params[2].get_str();
	string strToAddress = params[3].get_str();
	CSyscoinAddress fromAddress;
	CSyscoinAddress toAddress;
	std::vector<unsigned char> vchFromPubKey;
	std::vector<unsigned char> vchToPubKey;
	EnsureWalletIsUnlocked();
	// strFromAddress is a pubkey otherwise it's an alias
	if(strFromAddress.size() == 66)
	{
		std::vector<unsigned char> vchPubKeyByte;
		boost::algorithm::unhex(vchFromAliasOrPubKey.begin(), vchFromAliasOrPubKey.end(), std::back_inserter(vchPubKeyByte));
		CPubKey PubKey(vchPubKeyByte);
		CSyscoinAddress PublicAddress(PubKey.GetID());
		if(!PublicAddress.IsValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Invalid alias, no public key found! (fromalias)");	
		fromAddress = PublicAddress;
		vchFromPubKey = vchFromAliasOrPubKey;
	}
	else
	{
		fromAddress = CSyscoinAddress(strFromAddress);
		if (!fromAddress.IsValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Invalid syscoin address of fromalias");
		if (!fromAddress.isAlias)
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Must be a valid alias (fromalias)");
		// check for alias existence in DB
		vector<CAliasIndex> vtxPosFrom;
		if (!paliasdb->ReadAlias(vchFromString(fromAddress.aliasName), vtxPosFrom))
			throw JSONRPCError(RPC_WALLET_ERROR,
					"failed to read alias from alias DB (fromalias)");
		if (vtxPosFrom.size() < 1)
			throw JSONRPCError(RPC_WALLET_ERROR, "no result returned (fromalias)");
		CAliasIndex msgFromAlias = vtxPosFrom.back();
		vchFromPubKey = msgFromAlias.vchPubKey;

	}
	// strToAddress is a pubkey otherwise it's an alias
	if(strToAddress.size() == 66)
	{
		std::vector<unsigned char> vchPubKeyByte;
		boost::algorithm::unhex(vchToAliasOrPubKey.begin(), vchToAliasOrPubKey.end(), std::back_inserter(vchPubKeyByte));
		CPubKey PubKey(vchPubKeyByte);
		CSyscoinAddress PublicAddress(PubKey.GetID());
		if(!PublicAddress.IsValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Invalid alias, no public key found! (toalias)");	
		toAddress = PublicAddress;
		vchToPubKey = vchToAliasOrPubKey;
	}
	else
	{
		toAddress = CSyscoinAddress(strToAddress);
		if (!toAddress.IsValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Invalid syscoin address of toalias");
		if (!toAddress.isAlias)
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
					"Must be a valid alias (toalias)");
		// check for alias existence in DB
		vector<CAliasIndex> vtxPosTo;
		if (!paliasdb->ReadAlias(vchFromString(toAddress.aliasName), vtxPosTo))
			throw JSONRPCError(RPC_WALLET_ERROR,
					"failed to read alias from alias DB (toalias)");
		if (vtxPosTo.size() < 1)
			throw JSONRPCError(RPC_WALLET_ERROR, "no result returned (toalias)");
		CAliasIndex msgToAlias = vtxPosTo.back();
		vchToPubKey = msgToAlias.vchPubKey;

	}

	

	if (!IsMine(*pwalletMain, fromAddress.Get()))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
				"fromalias must be yours");


    // gather inputs
	int64_t rand = GetRand(std::numeric_limits<int64_t>::max());
	vector<unsigned char> vchRand = CScriptNum(rand).getvch();
    vector<unsigned char> vchMessage = vchFromValue(HexStr(vchRand));

    // this is a syscoin transaction
    CWalletTx wtx;

	CScript scriptPubKeyOrig, scriptPubKey;
	scriptPubKeyOrig= GetScriptForDestination(toAddress.Get());
	scriptPubKey << CScript::EncodeOP_N(OP_MESSAGE_ACTIVATE) << vchMessage
			<< vchRand << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;


	string strCipherTextTo;
	if(!EncryptMessage(vchToPubKey, vchMyMessage, strCipherTextTo))
	{
		throw JSONRPCError(RPC_WALLET_ERROR, "Could not encrypt message data for receiver!");
	}
	string strCipherTextFrom;
	if(!EncryptMessage(vchFromPubKey, vchMyMessage, strCipherTextFrom))
	{
		throw JSONRPCError(RPC_WALLET_ERROR, "Could not encrypt message data for sender!");
	}
    if (strCipherTextFrom.size() > MAX_MSG_LENGTH)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Message data cannot exceed 16383 bytes!");
    if (strCipherTextTo.size() > MAX_MSG_LENGTH)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Message data cannot exceed 16383 bytes!");

	int64_t nNetFee = GetMessageNetworkFee(OP_MESSAGE_ACTIVATE, chainActive.Tip()->nHeight);
 
    // build message UniValue
    CMessage newMessage;
    newMessage.vchRand = vchMessage;
	if(fromAddress.isAlias)
		newMessage.vchFrom = vchFromString(fromAddress.aliasName);
	else
		newMessage.vchFrom = vchFromString(fromAddress.ToString());
	if(toAddress.isAlias)
		newMessage.vchTo = vchFromString(toAddress.aliasName);
	else
		newMessage.vchTo = vchFromString(toAddress.ToString());

	newMessage.vchMessageFrom = vchFromString(strCipherTextFrom);
	newMessage.vchMessageTo = vchFromString(strCipherTextTo);
	newMessage.vchSubject = vchMySubject;
	newMessage.vchPubKeyFrom = vchFromPubKey;
	newMessage.vchPubKeyTo = vchToPubKey;
	
    string bdata = newMessage.SerializeToString();
	// send the tranasction
	vector<CRecipient> vecSend;
	CRecipient recipient = {scriptPubKey, MIN_AMOUNT, false};
	vecSend.push_back(recipient);

	CScript scriptFee;
	scriptFee << OP_RETURN;
	CRecipient fee = {scriptFee, nNetFee, false};
	vecSend.push_back(fee);

	SendMoneySyscoin(vecSend, MIN_AMOUNT, false, wtx, bdata);
	UniValue res(UniValue::VARR);
	res.push_back(wtx.GetHash().GetHex());
	res.push_back(HexStr(vchRand));
	return res;
}

UniValue messageinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 1 != params.size())
        throw runtime_error("messageinfo <guid>\n"
                "Show stored values of a single message.\n");

    vector<unsigned char> vchRand = vchFromValue(params[0]);

    // look for a transaction with this key, also returns
    // an message UniValue if it is found
    CTransaction tx;

	vector<CMessage> vtxPos;

	int expired = 0;
	int expires_in = 0;
	int expired_block = 0;
    UniValue oMessage(UniValue::VOBJ);
    vector<unsigned char> vchValue;

	if (!pmessagedb->ReadMessage(vchRand, vtxPos) || vtxPos.empty())
		  throw JSONRPCError(RPC_WALLET_ERROR, "failed to read from message DB");
	CMessage ca = vtxPos.back();

    string sHeight = strprintf("%llu", ca.nHeight);
	oMessage.push_back(Pair("GUID", stringFromVch(vchRand)));
	string sTime;
	CBlockIndex *pindex = chainActive[ca.nHeight];
	if (pindex) {
		sTime = strprintf("%llu", pindex->nTime);
	}
    string strAddress = "";
	oMessage.push_back(Pair("time", sTime));
	oMessage.push_back(Pair("from", stringFromVch(ca.vchFrom)));
	oMessage.push_back(Pair("to", stringFromVch(ca.vchTo)));
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
    uint256 blockHash;
    uint256 hash;
    CTransaction tx, dbtx;

    vector<unsigned char> vchValue;
    int nHeight;
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
		if (!DecodeMessageTx(wtx, op, nOut, vvch, -1) || !IsMessageOp(op))
			continue;
		// get the txn height
		nHeight = GetTxHashHeight(hash);

		// get the txn message name
		if (!GetNameOfMessageTx(wtx, vchName))
			continue;

		vector<CMessage> vtxPos;
		if (!pmessagedb->ReadMessage(vchName, vtxPos))
			continue;
		CMessage message = vtxPos.back();
		if (!GetTransaction(message.txHash, tx, Params().GetConsensus(), blockHash, true))
			continue;
		if(!IsMessageMine(tx))
			continue;
        // build the output UniValue
        UniValue oName(UniValue::VOBJ);
        oName.push_back(Pair("GUID", stringFromVch(vchName)));

		string sTime;
		CBlockIndex *pindex = chainActive[message.nHeight];
		if (pindex) {
			sTime = strprintf("%llu", pindex->nTime);
		}
        string strAddress = "";
		oName.push_back(Pair("time", sTime));

		oName.push_back(Pair("from", stringFromVch(message.vchFrom)));
		oName.push_back(Pair("to", stringFromVch(message.vchTo)));
		oName.push_back(Pair("subject", stringFromVch(message.vchSubject)));
		string strDecrypted = "";
		string strData = string("Encrypted for receiver of message");
		if(DecryptMessage(message.vchPubKeyTo, message.vchMessageTo, strDecrypted))
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
    uint256 blockHash;
    uint256 hash;
    CTransaction tx, dbtx;

    vector<unsigned char> vchValue;
    int nHeight;
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
		if (!DecodeMessageTx(wtx, op, nOut, vvch, -1) || !IsMessageOp(op))
			continue;
		// get the txn height
		nHeight = GetTxHashHeight(hash);

		// get the txn message name
		if (!GetNameOfMessageTx(wtx, vchName))
			continue;

		vector<CMessage> vtxPos;
		if (!pmessagedb->ReadMessage(vchName, vtxPos) || vtxPos.empty())
			continue;
		CMessage message = vtxPos.back();
		if (!GetTransaction(message.txHash, tx, Params().GetConsensus(), blockHash, true))
			continue;
		if(IsMessageMine(tx))
			continue;
        // build the output UniValue
        UniValue oName(UniValue::VOBJ);
        oName.push_back(Pair("GUID", stringFromVch(vchName)));

		string sTime;
		CBlockIndex *pindex = chainActive[message.nHeight];
		if (pindex) {
			sTime = strprintf("%llu", pindex->nTime);
		}
        string strAddress = "";
		oName.push_back(Pair("time", sTime));
		oName.push_back(Pair("from", stringFromVch(message.vchFrom)));
		oName.push_back(Pair("to", stringFromVch(message.vchTo)));
		oName.push_back(Pair("subject", stringFromVch(message.vchSubject)));
		string strDecrypted = "";
		string strData = string("Encrypted for sender of message");
		if(DecryptMessage(message.vchPubKeyFrom, message.vchMessageFrom, strDecrypted))
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
            throw JSONRPCError(RPC_WALLET_ERROR,
                    "failed to read from message DB");

        CMessage txPos2;
        uint256 txHash;
        uint256 blockHash;
        BOOST_FOREACH(txPos2, vtxPos) {
            txHash = txPos2.txHash;
			CTransaction tx;
			if (!GetTransaction(txHash, tx, Params().GetConsensus(), blockHash, true)) {
				error("could not read txpos");
				continue;
			}
            UniValue oMessage(UniValue::VOBJ);
            vector<unsigned char> vchValue;
            int nHeight;
            uint256 hash;
            if (GetValueOfMessageTxHash(txHash, vchValue, hash, nHeight)) {
                oMessage.push_back(Pair("GUID", message));
				string sTime;
				CBlockIndex *pindex = chainActive[txPos2.nHeight];
				if (pindex) {
					sTime = strprintf("%llu", pindex->nTime);
				}
				oMessage.push_back(Pair("time", sTime));

				oMessage.push_back(Pair("from", stringFromVch(txPos2.vchFrom)));
				oMessage.push_back(Pair("to", stringFromVch(txPos2.vchTo)));
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
    }
    return oRes;
}



