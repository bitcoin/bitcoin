// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h"
#include "key_io.h"
#include "rpc/server.h"
#include "rpc/rawtransaction_util.h"
#include "rpc/util.h"
#include "smessage/smessage.h"
#include "util/strencodings.h"
#include "util/system.h"
#include "util/time.h"
#include "validation.h"
#include <wallet/rpcwallet.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>



//using namespace json_spirit;
//using namespace std;

//extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);


static inline std::string GetTimeString(int64_t time) {
    return DateTimeStrFormat("%Y-%m-%d %H:%M:%S", time);
}

UniValue smsgenable(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgenable \n"
            "Enable secure messaging.");

    if (fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is already enabled.");

    UniValue result(UniValue::VOBJ);
    if (!SecureMsgEnable()) {
        result.pushKV("result", "Failed to enable secure messaging.");
    }
    else {
        result.pushKV("result", "Enabled secure messaging.");
    }
    return result;
}

UniValue smsgdisable(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgdisable \n"
            "Disable secure messaging.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is already disabled.");

    UniValue result(UniValue::VOBJ);
    if (!SecureMsgDisable()) {
        result.pushKV("result", "Failed to disable secure messaging.");
    }
    else {
        result.pushKV("result", "Disabled secure messaging.");
    }
    return result;
}

UniValue smsgoptions(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "smsgoptions [list|set <optname> <value>]\n"
            "List and manage options.");

    std::string mode = "list";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VOBJ);

    if (mode == "list") {
        result.pushKV("option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false"));
        result.pushKV("option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false"));

        result.pushKV("result", "Success.");
    }
    else if (mode == "set") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "set <optname> <value>");
            return result;
        }

        std::string optname = request.params[1].get_str();

        RPCTypeCheckArgument(request.params[2], UniValue::VBOOL);

        bool value = request.params[2].get_bool();

        if (optname == "newAddressRecv") {
            if (value) {
                smsgOptions.fNewAddressRecv = value;
            }
            else {
                result.pushKV("result", "Unknown value.");
                return result;
            }
            result.pushKV("set option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false"));
        }
        else if (optname == "newAddressAnon") {
            if (value) {
                smsgOptions.fNewAddressAnon = value;
            }
            else {
                result.pushKV("result", "Unknown value.");
                return result;
            }
            result.pushKV("set option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false"));
        }
        else {
            result.pushKV("result", "Option not found.");
            return result;
        }
    }
    else {
        result.pushKV("result", "Unknown Mode.");
        result.pushKV("expected", "smsgoption [list|set <optname> <value>]");
    }
    return result;
}

UniValue smsglocalkeys(const JSONRPCRequest& request) {

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]\n"
            "List and manage keys.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    UniValue result(UniValue::VOBJ);

    std::string mode = "whitelist";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    char cbuf[256];

    if (mode == "whitelist" || mode == "all") {
        uint32_t nKeys = 0;
        int all = mode == "all" ? 1 : 0;

        for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (!all && !it->fReceiveEnabled)
                continue;

            CTxDestination dest = DecodeDestination(it->sAddress);

            if (!IsValidDestination(dest))
                continue;

            CScript scriptPubKey = GetScriptForDestination(dest);
            std::string sPublicKey;
            CKeyID keyID = GetKeyForDestination(*pwallet, dest);

            if (keyID.IsNull())
                continue;

            CPubKey pubKey;
            if (!pwallet->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid() || !pubKey.IsCompressed()) {
                continue;
            };

            sPublicKey = EncodeBase58(&pubKey[0], &pubKey[pubKey.size()]);
            std::string atype="";

            txnouttype txType;
            CTxDestination outputAddress;
            ExtractDestination(scriptPubKey, outputAddress, &txType);
            //if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) && address.type() == typeid(PKHash))
            //  if(PKHash(pubkey) == boost::get<PKHash>(address))
            //      return true;

            std::string sLabel = "";
            if (pwallet->mapAddressBook.count(dest)) {
                sLabel = pwallet->mapAddressBook[dest].name;
            }

            std::string sInfo;
            if (all)
                sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on,  " : "off, ");
            sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
            result.pushKV("key", it->sAddress + " type is " + GetTxnOutputType(txType) + " - " + sPublicKey + " " + sInfo + " - " + sLabel);

            nKeys++;
        }

        snprintf(cbuf, sizeof(cbuf), "%u keys listed.", nKeys);
        result.pushKV("result", std::string(cbuf));
    }
    else if (mode == "recv") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "recv <+/-> <address>");
            return result;
        }

        std::string op      = request.params[1].get_str();
        std::string addr    = request.params[2].get_str();

        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (addr != it->sAddress)
                continue;
            break;
        }

        if (it == smsgAddresses.end()) {
            result.pushKV("result", "Address not found.");
            return result;
        }

        if (op == "+" || op == "on"  || op == "add" || op == "a") {
            it->fReceiveEnabled = true;
        }
        else if (op == "-" || op == "off" || op == "rem" || op == "r") {
            it->fReceiveEnabled = false;
        }
        else {
            result.pushKV("result", "Unknown operation.");
            return result;
        }

        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.pushKV("result", "Success.");
        result.pushKV("key", it->sAddress + " " + sInfo);
        return result;
    }
    else if (mode == "anon") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "anon <+/-> <address>");
            return result;
        }
        std::string op      = request.params[1].get_str();
        std::string addr    = request.params[2].get_str();

        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it) {
            if (addr != it->sAddress)
                continue;
            break;
        }

        if (it == smsgAddresses.end()) {
            result.pushKV("result", "Address not found.");
            return result;
        }

        if (op == "+" || op == "on"  || op == "add" || op == "a") {
            it->fReceiveAnon = true;
        }
        else if (op == "-" || op == "off" || op == "rem" || op == "r") {
            it->fReceiveAnon = false;
        }
        else {
            result.pushKV("result", "Unknown operation.");
            return result;
        }

        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.pushKV("result", "Success.");
        result.pushKV("key", it->sAddress + " " + sInfo);
        return result;
    }
    else if (mode == "wallet") {
        uint32_t nKeys = 0;
        for (const std::pair<CTxDestination, CAddressBookData>& entry : pwallet->mapAddressBook) {


            CTxDestination dest = entry.first;

            if (!IsValidDestination(dest))
                continue;

            CScript scriptPubKey = GetScriptForDestination(dest);
            isminetype mine = IsMine(*pwallet, dest);
            bool ismine =  bool(mine & ISMINE_SPENDABLE);

            if (!ismine)
                continue;

            std::string sPublicKey;
            CKeyID keyID = GetKeyForDestination(*pwallet, dest);

            if (keyID.IsNull())
                continue;

            CPubKey pubKey;
            if (!pwallet->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid() || !pubKey.IsCompressed()) {
                continue;
            };

            sPublicKey = EncodeBase58(&pubKey[0], &pubKey[pubKey.size()]);
            txnouttype txType;
            CTxDestination outputAddress;

            ExtractDestination(scriptPubKey, outputAddress, &txType);
            result.pushKV("key", EncodeDestination(dest) + " type is " + GetTxnOutputType(txType) + " - " + sPublicKey + " - " + entry.second.name);
            nKeys++;
        };

        snprintf(cbuf, sizeof(cbuf), "%u keys listed from wallet.", nKeys);
        result.pushKV("result", std::string(cbuf));
    }
    else {
        result.pushKV("result", "Unknown Mode.");
        result.pushKV("expected", "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]");
    }

    return result;
}

UniValue smsgscanchain(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgscanchain \n"
            "Look for public keys in the block chain.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    UniValue result(UniValue::VOBJ);
    if (!SecureMsgScanBlockChain()) {
        result.pushKV("result", "Scan Chain Failed.");
    }
    else {
        result.pushKV("result", "Scan Chain Completed.");
    }
    return result;
}

UniValue smsgscanbuckets(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgscanbuckets \n"
            "Force rescan of all messages in the bucket store.");

    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");

    UniValue result(UniValue::VOBJ);
    std::string error;
    int ret = SecureMsgScanBuckets(error);
    if (ret) {
        throw JSONRPCError(ret, error);
    }
    return result;
}

UniValue smsgaddkey(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "smsgaddkey <address> <pubkey>\n"
            "Add address, pubkey pair to database.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string addr = request.params[0].get_str();
    std::string pubk = request.params[1].get_str();

    UniValue result(UniValue::VOBJ);
    int rv = SecureMsgAddAddress(addr, pubk);
    if (rv != 0) {
        result.pushKV("result", "Public key not added to db.");
        switch (rv) {
            case 2:     result.pushKV("reason", "publicKey is invalid.");                  break;
            case 3:     result.pushKV("reason", "publicKey does not match address.");      break;
            case 4:     result.pushKV("reason", "address is already in db.");              break;
            case 5:     result.pushKV("reason", "address is invalid.");                    break;
            default:    result.pushKV("reason", "error.");                                 break;
        }
    }
    else {
        result.pushKV("result", "Added public key to db.");
    }

    return result;
}

UniValue smsggetpubkey(const JSONRPCRequest& request) {

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "smsggetpubkey <address>\n"
            "Return the base58 encoded compressed public key for an address.\n"
            "Tests localkeys first, then looks in public key db.\n");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");


    std::string address   = request.params[0].get_str();
    std::string publicKey;

    UniValue result(UniValue::VOBJ);
    int rv = SecureMsgGetLocalPublicKey(address, publicKey);
    switch (rv) {
        case 0:
            result.pushKV("result", "Success.");
            result.pushKV("address in wallet", address);
            result.pushKV("compressed public key", publicKey);
            return result; // success, don't check db
        case 2:
        case 3:
            result.pushKV("result", "Failed.");
            result.pushKV("message", "Invalid address.");
            return result;
        case 4:
            break; // check db
        //case 1:
        default:
            result.pushKV("result", "Failed.");
            result.pushKV("message", "Error.");
            return result;
    }

    CTxDestination dest = DecodeDestination(address);
    CKeyID keyID = GetKeyForDestination(*pwallet, dest);

    if (keyID.IsNull()) {
        result.pushKV("result", "Failed.");
        result.pushKV("message", "Invalid address.");
        return result;
    }

    CPubKey cpkFromDB;

    rv = SecureMsgGetStoredKey(keyID, cpkFromDB);

    switch (rv) {
        case 0:
            if (!cpkFromDB.IsValid() || !cpkFromDB.IsCompressed()) {
                result.pushKV("result", "Failed.");
                result.pushKV("message", "Invalid address.");
            }
            else {
                //cpkFromDB.SetCompressedPubKey(); // make sure key is compressed
                publicKey = EncodeBase58(&cpkFromDB[0], &cpkFromDB[cpkFromDB.size()]);

                result.pushKV("result", "Success.");
                result.pushKV("peer address in DB", address);
                result.pushKV("compressed public key", publicKey);
            }
            break;
        case 2:
            result.pushKV("result", "Failed.");
            result.pushKV("message", "Address not found in wallet or db.");
            return result;
        //case 1:
        default:
            result.pushKV("result", "Failed.");
            result.pushKV("message", "Error, GetStoredKey().");
            return result;
    }

    return result;
}

UniValue smsgsend(const JSONRPCRequest& request) {

    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "smsgsend <addrFrom> <addrTo> <message>\n"
            "Send an encrypted message from addrFrom to addrTo.");

    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Secure messaging is disabled.");

    std::string addrFrom  = request.params[0].get_str();
    std::string addrTo    = request.params[1].get_str();
    std::string msg       = request.params[2].get_str();

    UniValue result(UniValue::VOBJ);

    std::string sError;
    int error = SecureMsgSend(addrFrom, addrTo, msg, sError);
    sError = "Sending secure message failed - " + sError;
    if (error > 0) {
        throw std::runtime_error(sError);
    }
    else if (error < 0) {
        throw JSONRPCError(error, sError);
    }

    return NullUniValue;
}

UniValue smsgsendanon(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "smsgsendanon <addrTo> <message>\n"
            "Send an anonymous encrypted message to addrTo.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string addrFrom  = "anon";
    std::string addrTo    = request.params[0].get_str();
    std::string msg       = request.params[1].get_str();


    UniValue result(UniValue::VOBJ);
    std::string sError;
    int ret = SecureMsgSend(addrFrom, addrTo, msg, sError);
    sError = "Sending anonymous massage failed - " + sError;
    if (ret > 0) {
        throw std::runtime_error(sError);
    }
    else if (ret < 0) {
        throw JSONRPCError(ret, sError);
    }

    return NullUniValue;
}

UniValue smsginbox(const JSONRPCRequest& request)
{

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() > 1) // defaults to read
        throw std::runtime_error("smsginbox ( \"all|unread|clear\" )\n"
                            "\nDecrypt and display received secure messages in the inbox.\n"
                            "Warning: clear will delete all messages.");

    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Secure messaging is disabled.");

    if (pwallet->IsLocked())
        throw std::runtime_error("Wallet is locked.");

    std::string mode = "unread";
    if (request.params.size() > 0)
    {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VARR);

    {
        LOCK(cs_smsgDB);

        SecMsgDB dbInbox;

        if (!dbInbox.Open("cr+"))
            throw std::runtime_error("Could not open DB.");

        uint32_t nMessages = 0;

        std::string sPrefix("im");
        unsigned char chKey[18];

        if (mode == "clear")
        {
            dbInbox.TxnBegin();

            leveldb::Iterator* it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesgKey(it, sPrefix, chKey))
            {
                dbInbox.EraseSmesg(chKey);
                nMessages++;
            };
            delete it;
            dbInbox.TxnCommit();

            std::ostringstream oss;
            oss << nMessages << " messages deleted";
            return oss.str();
        }
        else if (mode == "all" || mode == "unread") {
            int fCheckReadStatus = mode == "unread" ? 1 : 0;

            SecMsgStored smsgStored;
            MessageData msg;

            dbInbox.TxnBegin();

            leveldb::Iterator* it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                if (fCheckReadStatus
                    && !(smsgStored.status & SMSG_MASK_UNREAD))
                    continue;

                UniValue objM(UniValue::VOBJ);
                std::string errorMsg;
                int error = SecureMsgDecrypt(smsgStored, msg, errorMsg);
                if (!error) {
                    objM.pushKV("received", GetTimeString(smsgStored.timeReceived));
                    objM.pushKV("sent", GetTimeString(msg.timestamp));
                    objM.pushKV("from", msg.sFromAddress.c_str());
                    objM.pushKV("to", msg.sToAddress.c_str());
                    objM.pushKV("text", msg.sMessage.c_str());
                }
                else {
                    std::ostringstream oss;
                    oss << error;
                    objM.pushKV("error", "Could not decrypt (" + oss.str() + ")");
                }
                result.push_back(objM);

                if (fCheckReadStatus) {
                    smsgStored.status &= ~SMSG_MASK_UNREAD;
                    dbInbox.WriteSmesg(chKey, smsgStored);
                }
                nMessages++;
            }
            delete it;
            dbInbox.TxnCommit();

            return result;
        }
    }
    return smsginbox(request);
};

UniValue smsgoutbox(const JSONRPCRequest& request)
{

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() > 1) // defaults to read
        throw std::runtime_error(
            "smsgoutbox ( \"all|clear\" )\n"
            "Decrypt and display all sent messages.\n"
            "Warning: clear will delete all sent messages.");

    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");

    if (pwallet->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Wallet is locked, but secure messaging needs an unlocked wallet.");

    std::string mode = "all";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VARR);

    std::string sPrefix("sm");
    unsigned char chKey[18];
    memset(&chKey[0], 0, 18);

    {
        LOCK(cs_smsgDB);

        SecMsgDB dbOutbox;

        if (!dbOutbox.Open("cr+"))
            throw std::runtime_error("Could not open DB.");

        uint32_t nMessages = 0;

        if (mode == "clear") {
            dbOutbox.TxnBegin();

            leveldb::Iterator* it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesgKey(it, sPrefix, chKey)) {
                dbOutbox.EraseSmesg(chKey);
                nMessages++;
            }
            delete it;
            dbOutbox.TxnCommit();

            return NullUniValue;
        }
        else if (mode == "all") {
            SecMsgStored smsgStored;
            MessageData msg;
            leveldb::Iterator* it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesg(it, sPrefix, chKey, smsgStored)) {
                const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
                SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
                memcpy(smsg.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);
                int error = SecureMsgDecrypt(false, smsgStored.sAddrOutbox, smsg, pPayload, msg);
                if (!error) {
                    UniValue objM(UniValue::VOBJ);
                    objM.pushKV("sent", GetTimeString(msg.timestamp));
                    objM.pushKV("from", msg.sFromAddress.c_str());
                    objM.pushKV("to", smsgStored.sAddrTo.c_str());
                    objM.pushKV("text", msg.sMessage.c_str());

                    result.push_back(objM);
                }
                else if (error < 0) {
                    throw JSONRPCError(error, "Could not decrypt.");
                }
                else
                    throw std::runtime_error("Could not decrypt.");
                nMessages++;
            }
            delete it;

            return result;
        }
    }

    return smsgoutbox(request);
};


UniValue smsgbuckets(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "smsgbuckets ( \"stats|dump\" )\n"
            "Display some statistics.");

    if (!fSecMsgEnabled)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Secure messaging is disabled.");

    std::string mode = "stats";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VARR);

    if (mode == "stats")
    {
        uint64_t nBuckets = 0;
        uint64_t nMessages = 0;
        uint64_t nBytes = 0;
        {
            LOCK(cs_smsg);
            std::map<int64_t, SecMsgBucket>::iterator it;
            it = smsgBuckets.begin();

            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it)
            {
                std::set<SecMsgToken>& tokenSet = it->second.setTokens;

                std::string sBucket = boost::lexical_cast<std::string>(it->first);
                std::string sFile = sBucket + "_01.dat";

                nBuckets++;
                nMessages += tokenSet.size();

                UniValue objM(UniValue::VOBJ);
                objM.pushKV("bucket", (uint64_t) it->first);
                objM.pushKV("time", GetTimeString(it->first));
                objM.pushKV("no. messages", (uint64_t)tokenSet.size());
                objM.pushKV("hash", (uint64_t) it->second.hash);
                objM.pushKV("last changed", GetTimeString(it->second.timeChanged));

                boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;


                if (!boost::filesystem::exists(fullPath))
                {
                    // -- If there is a file for an empty bucket something is wrong.
                    if (tokenSet.size() == 0)
                        objM.pushKV("file size", "Empty bucket.");
                    else
                        objM.pushKV("file size, error", "File not found.");
                } else
                {
                    try {

                        uint64_t nFBytes = 0;
                        nFBytes = boost::filesystem::file_size(fullPath);
                        nBytes += nFBytes;
                        objM.pushKV("file size", nFBytes);
                    } catch (const boost::filesystem::filesystem_error& ex)
                    {
                        objM.pushKV("file size, error", ex.what());
                    };
                };

                result.push_back(objM);
            };
        }; // LOCK(cs_smsg);

        UniValue objM(UniValue::VOBJ);
        objM.pushKV("buckets", result);
        objM.pushKV("no. buckets", nBuckets);
        objM.pushKV("no. messages", nMessages);
        objM.pushKV("size", nBytes);

        return objM;
    }
    else if (mode == "dump") {
        {
            LOCK(cs_smsg);
            std::map<int64_t, SecMsgBucket>::iterator it;
            it = smsgBuckets.begin();

            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it) {
                std::string sFile = boost::lexical_cast<std::string>(it->first) + "_01.dat";

                try {
                    boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;
                    boost::filesystem::remove(fullPath);
                }
                catch (const boost::filesystem::filesystem_error& ex) {
                    //objM.pushKV("file size, error", ex.what()));
                    printf("Error removing bucket file %s.\n", ex.what());
                }
            }
            smsgBuckets.clear();
        } // LOCK(cs_smsg);

        return NullUniValue;
    }

    return smsgbuckets(request);
};

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "messaging",             "smsgaddkey",           &smsgaddkey,       {"nblocks","height"} },
    { "messaging",             "smsgbuckets",          &smsgbuckets,          {} },
    { "messaging",             "smsgdisable",          &smsgdisable,  {"txid","dummy","fee_delta"} },
    { "messaging",             "smsgenable",           &smsgenable,       {"template_request"} },
    { "messaging",             "smsggetpubkey",        &smsggetpubkey,            {"hexdata","dummy"} },
    { "messaging",             "smsginbox",            &smsginbox,           {"hexdata"} },
    { "messaging",             "smsglocalkeys",        &smsglocalkeys,             {"height"} },
    { "messaging",             "smsgoptions",          &smsgoptions,            {"hexdata","dummy"} },
    { "messaging",             "smsgoutbox",           &smsgoutbox,           {"hexdata"} },
    { "messaging",             "smsgscanbuckets",      &smsgscanbuckets,             {"height"} },
    { "messaging",             "smsgscanchain",        &smsgscanchain,            {"hexdata","dummy"} },
    { "messaging",             "smsgsend",             &smsgsend,             {"height"} },
    { "messaging",             "smsgsendanon",         &smsgsendanon,             {"height"} },
};
// clang-format on

void RegisterMessagingRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
