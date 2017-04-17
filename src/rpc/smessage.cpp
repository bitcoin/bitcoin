// Copyright (c) 2014 The ShadowCoin developers
// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"

#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <string>

#include "smsg/smessage.h"
#include "wallet/wallet.h"
#include "script/ismine.h"


#include <univalue.h>

UniValue smsgenable(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgenable \n"
            "Enable secure messaging.");

    if (fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is already enabled.");

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("result", (SecureMsgEnable(pwalletMain) ? "Enabled secure messaging." : "Failed to enable secure messaging.")));

    return result;
}

UniValue smsgdisable(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgdisable \n"
            "Disable secure messaging.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is already disabled.");

    UniValue result;
    result.push_back(Pair("result", (SecureMsgDisable() ? "Disabled secure messaging." : "Failed to disable secure messaging.")));

    return result;
}

UniValue smsgoptions(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "smsgoptions [list <with_description>|set <optname> <value>]\n"
            "smsgoptions list 1\n"
            " list possible options with descriptions.\n"
            "List and manage options.");

    std::string mode = "list";
    if (request.params.size() > 0)
        mode = request.params[0].get_str();
    
    UniValue result(UniValue::VOBJ);

    if (mode == "list")
    {
        bool fDescriptions = false;
        if (request.params.size() > 1)
        {
            std::string value = request.params[1].get_str();
            fDescriptions     = part::IsStringBoolPositive(value);
        };

        result.push_back(Pair("option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false")));

        if (fDescriptions)
            result.push_back(Pair("newAddressRecv", "Enable receiving messages for newly created addresses."));
        result.push_back(Pair("option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false")));

        if (fDescriptions)
            result.push_back(Pair("newAddressAnon", "Enable receiving anonymous messages for newly created addresses."));
        result.push_back(Pair("option", std::string("scanIncoming = ") + (smsgOptions.fScanIncoming ? "true" : "false")));

        if (fDescriptions)
            result.push_back(Pair("scanIncoming", "Scan incoming blocks for public keys."));

        result.push_back(Pair("result", "Success."));
    } else
    if (mode == "set")
    {
        if (request.params.size() < 3)
        {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "set <optname> <value>"));
            return result;
        };

        std::string optname = request.params[1].get_str();
        std::string value   = request.params[2].get_str();

        std::transform(optname.begin(), optname.end(), optname.begin(), ::tolower);

        bool fValue;
        if (optname == "newaddressrecv")
        {
            if (part::GetStringBool(value, fValue))
            {
                smsgOptions.fNewAddressRecv = fValue;
            } else
            {
                result.push_back(Pair("result", "Unknown value."));
                return result;
            };
            result.push_back(Pair("set option", std::string("newAddressRecv = ") + (smsgOptions.fNewAddressRecv ? "true" : "false")));
        } else
        if (optname == "newaddressanon")
        {
            if (part::GetStringBool(value, fValue))
            {
                smsgOptions.fNewAddressAnon = fValue;
            } else
            {
                result.push_back(Pair("result", "Unknown value."));
                return result;
            };
            result.push_back(Pair("set option", std::string("newAddressAnon = ") + (smsgOptions.fNewAddressAnon ? "true" : "false")));
        } else
        if (optname == "scanincoming")
        {
            if (part::GetStringBool(value, fValue))
            {
                smsgOptions.fScanIncoming = fValue;
            } else
            {
                result.push_back(Pair("result", "Unknown value."));
                return result;
            };
            result.push_back(Pair("set option", std::string("scanIncoming = ") + (smsgOptions.fScanIncoming ? "true" : "false")));
        } else
        {
            result.push_back(Pair("result", "Option not found."));
            return result;
        };
    } else
    {
        result.push_back(Pair("result", "Unknown Mode."));
        result.push_back(Pair("expected", "smsgoption [list|set <optname> <value>]"));
    };
    
    return result;
}

UniValue smsglocalkeys(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 3)
        throw std::runtime_error(
            "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]\n"
            "List and manage keys.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    UniValue result(UniValue::VOBJ);

    std::string mode = "whitelist";
    if (request.params.size() > 0)
    {
        mode = request.params[0].get_str();
    };

    if (mode == "whitelist"
        || mode == "all")
    {
        uint32_t nKeys = 0;
        int all = mode == "all" ? 1 : 0;

        UniValue keys(UniValue::VARR);

        for (std::vector<SecMsgAddress>::iterator it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
        {
            if (!all
                && !it->fReceiveEnabled)
                continue;

            CKeyID &keyID = it->address;
            std::string sPublicKey;
            CPubKey pubKey;
            if (!pwalletSmsg->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid()
                || !pubKey.IsCompressed())
            {
                continue;
            };

            sPublicKey = EncodeBase58(pubKey.begin(), pubKey.end());

            UniValue objM(UniValue::VOBJ);

            std::string sLabel = pwalletSmsg->mapAddressBook[keyID].name;
            std::string sInfo;
            if (all)
                sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on,  " : "off, ");
            sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
            //result.push_back(Pair("key", it->sAddress + " - " + sPublicKey + " " + sInfo + " - " + sLabel));
            objM.push_back(Pair("address", CBitcoinAddress(keyID).ToString()));
            objM.push_back(Pair("public_key", sPublicKey));
            objM.push_back(Pair("receive", (it->fReceiveEnabled ? "1" : "0")));
            objM.push_back(Pair("anon", (it->fReceiveAnon ? "1" : "0")));
            objM.push_back(Pair("label", sLabel));
            keys.push_back(objM);

            nKeys++;
        };
        result.push_back(Pair("keys", keys));
        result.push_back(Pair("result", strprintf("%u", nKeys)));
    } else
    if (mode == "recv")
    {
        if (request.params.size() < 3)
        {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "recv <+/-> <address>"));
            return result;
        };

        std::string op      = request.params[1].get_str();
        std::string addr    = request.params[2].get_str();

        CKeyID keyID;
        CBitcoinAddress coinAddress(addr);
        if (!coinAddress.IsValid())
            throw std::runtime_error("Invalid address.");
        if (!coinAddress.GetKeyID(keyID))
            throw std::runtime_error("Invalid address.");

        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
        {
            if (keyID != it->address)
                continue;
            break;
        };

        if (it == smsgAddresses.end())
        {
            result.push_back(Pair("result", "Address not found."));
            return result;
        };

        if (op == "+" || op == "on"  || op == "add" || op == "a")
        {
            it->fReceiveEnabled = true;
        } else
        if (op == "-" || op == "off" || op == "rem" || op == "r")
        {
            it->fReceiveEnabled = false;
        } else
        {
            result.push_back(Pair("result", "Unknown operation."));
            return result;
        };

        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.push_back(Pair("result", "Success."));
        result.push_back(Pair("key", coinAddress.ToString() + " " + sInfo));
        return result;

    } else
    if (mode == "anon")
    {
        if (request.params.size() < 3)
        {
            result.push_back(Pair("result", "Too few parameters."));
            result.push_back(Pair("expected", "anon <+/-> <address>"));
            return result;
        };

        std::string op      = request.params[1].get_str();
        std::string addr    = request.params[2].get_str();

        CKeyID keyID;
        CBitcoinAddress coinAddress(addr);
        if (!coinAddress.IsValid())
            throw std::runtime_error("Invalid address.");
        if (!coinAddress.GetKeyID(keyID))
            throw std::runtime_error("Invalid address.");

        std::vector<SecMsgAddress>::iterator it;
        for (it = smsgAddresses.begin(); it != smsgAddresses.end(); ++it)
        {
            if (keyID != it->address)
                continue;
            break;
        };

        if (it == smsgAddresses.end())
        {
            result.push_back(Pair("result", "Address not found."));
            return result;
        };

        if (op == "+" || op == "on"  || op == "add" || op == "a")
        {
            it->fReceiveAnon = true;
        } else
        if (op == "-" || op == "off" || op == "rem" || op == "r")
        {
            it->fReceiveAnon = false;
        } else
        {
            result.push_back(Pair("result", "Unknown operation."));
            return result;
        };

        std::string sInfo;
        sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on, " : "off,");
        sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
        result.push_back(Pair("result", "Success."));
        result.push_back(Pair("key", coinAddress.ToString() + " " + sInfo));
        return result;

    } else
    if (mode == "wallet")
    {
        uint32_t nKeys = 0;
        UniValue keys(UniValue::VOBJ);

        BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData) &entry, pwalletSmsg->mapAddressBook)
        {
            if (!IsMine(*pwalletSmsg, entry.first))
                continue;

            CBitcoinAddress coinAddress(entry.first);
            if (!coinAddress.IsValid())
                continue;

            std::string address = coinAddress.ToString();
            std::string sPublicKey;

            CKeyID keyID;
            if (!coinAddress.GetKeyID(keyID))
                continue;

            CPubKey pubKey;
            if (!pwalletSmsg->GetPubKey(keyID, pubKey))
                continue;
            if (!pubKey.IsValid()
                || !pubKey.IsCompressed())
            {
                continue;
            };

            sPublicKey = EncodeBase58(pubKey.begin(), pubKey.end());
            UniValue objM(UniValue::VOBJ);

            objM.push_back(Pair("key", address));
            objM.push_back(Pair("publickey", sPublicKey));
            objM.push_back(Pair("label", entry.second.name));

            keys.push_back(objM);
            nKeys++;
        };
        result.push_back(Pair("keys", keys));
        result.push_back(Pair("result", strprintf("%u", nKeys)));
    } else
    {
        result.push_back(Pair("result", "Unknown Mode."));
        result.push_back(Pair("expected", "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]"));
    };

    return result;
};

UniValue smsgscanchain(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgscanchain \n"
            "Look for public keys in the block chain.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    UniValue result(UniValue::VOBJ);
    if (!SecureMsgScanBlockChain())
    {
        result.push_back(Pair("result", "Scan Chain Failed."));
    } else
    {
        result.push_back(Pair("result", "Scan Chain Completed."));
    };
    return result;
}

UniValue smsgscanbuckets(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "smsgscanbuckets \n"
            "Force rescan of all messages in the bucket store.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    if (pwalletSmsg->IsLocked())
        throw std::runtime_error("Wallet is locked.");

    UniValue result(UniValue::VOBJ);
    if (!SecureMsgScanBuckets())
    {
        result.push_back(Pair("result", "Scan Buckets Failed."));
    } else
    {
        result.push_back(Pair("result", "Scan Buckets Completed."));
    };
    return result;
}

UniValue smsgaddkey(const JSONRPCRequest &request)
{
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
    if (rv != 0)
    {
        result.push_back(Pair("result", "Public key not added to db."));
        switch (rv)
        {
            case 2:     result.push_back(Pair("reason", "publicKey is invalid."));                  break;
            case 3:     result.push_back(Pair("reason", "publicKey does not match address."));      break;
            case 4:     result.push_back(Pair("reason", "address is already in db."));              break;
            case 5:     result.push_back(Pair("reason", "address is invalid."));                    break;
            default:    result.push_back(Pair("reason", "error."));                                 break;
        };
    } else
    {
        result.push_back(Pair("result", "Added public key to db."));
    };

    return result;
}

UniValue smsggetpubkey(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "smsggetpubkey <address>\n"
            "Return the base58 encoded compressed public key for an address.\n"
            "Tests localkeys first, then looks in public key db.\n");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string address = request.params[0].get_str();
    std::string publicKey;

    UniValue result(UniValue::VOBJ);
    int rv = SecureMsgGetLocalPublicKey(address, publicKey);
    switch (rv)
    {
        case 0:
            result.push_back(Pair("result", "Success."));
            result.push_back(Pair("address", address));
            result.push_back(Pair("publickey", publicKey));
            return result; // success, don't check db
        case 2:
        case 3:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Invalid address."));
            return result;
        case 4:
            break; // check db
        //case 1:
        default:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Error."));
            return result;
    };

    CBitcoinAddress coinAddress(address);


    CKeyID keyID;
    if (!coinAddress.GetKeyID(keyID))
    {
        result.push_back(Pair("result", "Failed."));
        result.push_back(Pair("message", "Invalid address."));
        return result;
    };

    CPubKey cpkFromDB;
    rv = SecureMsgGetStoredKey(keyID, cpkFromDB);

    switch (rv)
    {
        case 0:
            if (!cpkFromDB.IsValid()
                || !cpkFromDB.IsCompressed())
            {
                result.push_back(Pair("result", "Failed."));
                result.push_back(Pair("message", "Invalid address."));
            } else
            {
                //cpkFromDB.SetCompressedPubKey(); // make sure key is compressed
                publicKey = EncodeBase58(cpkFromDB.begin(), cpkFromDB.end());

                result.push_back(Pair("result", "Success."));
                result.push_back(Pair("address", address));
                result.push_back(Pair("publickey", publicKey));
            };
            break;
        case 2:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Address not found in wallet or db."));
            return result;
        //case 1:
        default:
            result.push_back(Pair("result", "Failed."));
            result.push_back(Pair("message", "Error, GetStoredKey()."));
            return result;
    };

    return result;
}

UniValue smsgsend(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "smsgsend <addrFrom> <addrTo> <message>\n"
            "Send an encrypted message from addrFrom to addrTo.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string addrFrom  = request.params[0].get_str();
    std::string addrTo    = request.params[1].get_str();
    std::string msg       = request.params[2].get_str();

    CKeyID kiFrom, kiTo;
    CBitcoinAddress coinAddress(addrFrom);
    if (!coinAddress.IsValid())
        throw std::runtime_error("Invalid from address.");
    if (!coinAddress.GetKeyID(kiFrom))
        throw std::runtime_error("Invalid from address.");
    coinAddress.SetString(addrTo);
    if (!coinAddress.IsValid())
        throw std::runtime_error("Invalid to address.");
    if (!coinAddress.GetKeyID(kiTo))
        throw std::runtime_error("Invalid to address.");


    UniValue result(UniValue::VOBJ);
    std::string sError;
    if (SecureMsgSend(kiFrom, kiTo, msg, sError) != 0)
    {
        result.push_back(Pair("result", "Send failed."));
        result.push_back(Pair("error", sError));
    } else
    {
        result.push_back(Pair("result", "Sent."));
    };

    return result;
}

UniValue smsgsendanon(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "smsgsendanon <addrTo> <message>\n"
            "Send an anonymous encrypted message to addrTo.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string addrTo    = request.params[0].get_str();
    std::string msg       = request.params[1].get_str();
    
    CKeyID kiFrom, kiTo;
    CBitcoinAddress coinAddress(addrTo);
    if (!coinAddress.IsValid())
        throw std::runtime_error("Invalid to address.");
    if (!coinAddress.GetKeyID(kiTo))
        throw std::runtime_error("Invalid to address.");

    UniValue result(UniValue::VOBJ);
    std::string sError;
    if (SecureMsgSend(kiFrom, kiTo, msg, sError) != 0)
    {
        result.push_back(Pair("result", "Send failed."));
        result.push_back(Pair("error", sError));
    } else
    {
        result.push_back(Pair("result", "Sent."));
    };

    return result;
}

UniValue smsginbox(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1) // defaults to read
        throw std::runtime_error(
            "smsginbox [all|unread|clear]\n"
            "Decrypt and display all received messages.\n"
            "Warning: clear will delete all messages.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    if (pwalletSmsg->IsLocked())
        throw std::runtime_error("Wallet is locked.");

    std::string mode = "unread";
    if (request.params.size() > 0)
    {
        mode = request.params[0].get_str();
    };

    UniValue result(UniValue::VOBJ);

    std::vector<unsigned char> vchKey;
    vchKey.resize(16);
    memset(&vchKey[0], 0, 16);

    {
        LOCK(cs_smsgDB);

        SecMsgDB dbInbox;

        if (!dbInbox.Open("cr+"))
            throw std::runtime_error("Could not open DB.");

        uint32_t nMessages = 0;
        char cbuf[256];

        std::string sPrefix("im");
        unsigned char chKey[18];

        if (mode == "clear")
        {
            dbInbox.TxnBegin();

            leveldb::Iterator *it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesgKey(it, sPrefix, chKey))
            {
                dbInbox.EraseSmesg(chKey);
                nMessages++;
            };
            delete it;
            dbInbox.TxnCommit();

            result.push_back(Pair("result", strprintf("Deleted %u messages.", nMessages)));
        } else
        if (mode == "all"
            || mode == "unread")
        {
            int fCheckReadStatus = mode == "unread" ? 1 : 0;

            SecMsgStored smsgStored;
            MessageData msg;

            dbInbox.TxnBegin();

            leveldb::Iterator *it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            UniValue messageList(UniValue::VARR);

            while (dbInbox.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                if (fCheckReadStatus
                    && !(smsgStored.status & SMSG_MASK_UNREAD))
                    continue;

                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
                if (SecureMsgDecrypt(false, smsgStored.addrTo, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    UniValue objM(UniValue::VOBJ);
                    objM.push_back(Pair("success", "1"));
                    objM.push_back(Pair("received", part::GetTimeString(smsgStored.timeReceived, cbuf, sizeof(cbuf))));
                    objM.push_back(Pair("sent", part::GetTimeString(msg.timestamp, cbuf, sizeof(cbuf))));
                    objM.push_back(Pair("from", msg.sFromAddress));
                    objM.push_back(Pair("to", CBitcoinAddress(smsgStored.addrTo).ToString()));
                    objM.push_back(Pair("text", std::string((char*)&msg.vchMessage[0]))); // ugh

                    messageList.push_back(objM);
                } else
                {
                    UniValue objM(UniValue::VOBJ);
                    objM.push_back(Pair("success", "0"));
                    messageList.push_back(objM);
                };

                if (fCheckReadStatus)
                {
                    smsgStored.status &= ~SMSG_MASK_UNREAD;
                    dbInbox.WriteSmesg(chKey, smsgStored);
                };
                nMessages++;
            };
            delete it;
            dbInbox.TxnCommit();

            result.push_back(Pair("messages", messageList));
            result.push_back(Pair("result", strprintf("%u", nMessages)));

        } else
        {
            result.push_back(Pair("result", "Unknown Mode."));
            result.push_back(Pair("expected", "[all|unread|clear]."));
        };
    } // cs_smsgDB

    return result;
};

UniValue smsgoutbox(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1) // defaults to read
        throw std::runtime_error(
            "smsgoutbox [all|clear]\n"
            "Decrypt and display all sent messages.\n"
            "Warning: clear will delete all sent messages.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    if (pwalletSmsg->IsLocked())
        throw std::runtime_error("Wallet is locked.");

    std::string mode = "all";
    if (request.params.size() > 0)
    {
        mode = request.params[0].get_str();
    };


    UniValue result(UniValue::VOBJ);

    std::string sPrefix("sm");
    unsigned char chKey[18];
    memset(&chKey[0], 0, 18);

    {
        LOCK(cs_smsgDB);

        SecMsgDB dbOutbox;

        if (!dbOutbox.Open("cr+"))
            throw std::runtime_error("Could not open DB.");

        uint32_t nMessages = 0;
        char cbuf[256];

        if (mode == "clear")
        {
            dbOutbox.TxnBegin();

            leveldb::Iterator *it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesgKey(it, sPrefix, chKey))
            {
                dbOutbox.EraseSmesg(chKey);
                nMessages++;
            };
            delete it;
            dbOutbox.TxnCommit();


            result.push_back(Pair("result", strprintf("Deleted %u messages.", nMessages)));
        } else
        if (mode == "all")
        {
            SecMsgStored smsgStored;
            MessageData msg;
            leveldb::Iterator *it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());

            UniValue messageList(UniValue::VARR);

            while (dbOutbox.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;

                if (SecureMsgDecrypt(false, smsgStored.addrOutbox, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    UniValue objM(UniValue::VOBJ);
                    objM.push_back(Pair("success", "1"));
                    objM.push_back(Pair("sent", part::GetTimeString(msg.timestamp, cbuf, sizeof(cbuf))));
                    objM.push_back(Pair("from", msg.sFromAddress));
                    objM.push_back(Pair("to", CBitcoinAddress(smsgStored.addrTo).ToString()));
                    objM.push_back(Pair("text", std::string((char*)&msg.vchMessage[0]))); // ugh

                    messageList.push_back(objM);
                } else
                {
                    UniValue objM(UniValue::VOBJ);
                    objM.push_back(Pair("success", "0"));
                    messageList.push_back(objM);
                };
                nMessages++;
            };
            delete it;

            result.push_back(Pair("messages" ,messageList));
            result.push_back(Pair("result", strprintf("%u", nMessages)));
        } else
        {
            result.push_back(Pair("result", "Unknown Mode."));
            result.push_back(Pair("expected", "[all|clear]."));
        };
    }

    return result;
};


UniValue smsgbuckets(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "smsgbuckets [stats|dump]\n"
            "Display some statistics.");

    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    std::string mode = "stats";
    if (request.params.size() > 0)
    {
        mode = request.params[0].get_str();
    };

    UniValue result(UniValue::VOBJ);

    char cbuf[256];
    if (mode == "stats")
    {
        uint32_t nBuckets = 0;
        uint32_t nMessages = 0;
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

                std::string sHash = boost::lexical_cast<std::string>(it->second.hash);

                nBuckets++;
                nMessages += tokenSet.size();

                UniValue objM(UniValue::VOBJ);
                objM.push_back(Pair("bucket", sBucket));
                objM.push_back(Pair("time", part::GetTimeString(it->first, cbuf, sizeof(cbuf))));
                objM.push_back(Pair("no. messages", strprintf("%u", tokenSet.size())));
                objM.push_back(Pair("hash", sHash));
                objM.push_back(Pair("last changed", part::GetTimeString(it->second.timeChanged, cbuf, sizeof(cbuf))));

                boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;


                if (!boost::filesystem::exists(fullPath))
                {
                    // -- If there is a file for an empty bucket something is wrong.
                    if (tokenSet.size() == 0)
                        objM.push_back(Pair("file size", "Empty bucket."));
                    else
                        objM.push_back(Pair("file size, error", "File not found."));
                } else
                {
                    try {

                        uint64_t nFBytes = 0;
                        nFBytes = boost::filesystem::file_size(fullPath);
                        nBytes += nFBytes;
                        objM.push_back(Pair("file size", part::BytesReadable(nFBytes)));
                    } catch (const boost::filesystem::filesystem_error& ex)
                    {
                        objM.push_back(Pair("file size, error", ex.what()));
                    };
                };

                result.push_back(Pair("bucket", objM));
            };
        }; // LOCK(cs_smsg);


        std::string snBuckets = boost::lexical_cast<std::string>(nBuckets);
        std::string snMessages = boost::lexical_cast<std::string>(nMessages);

        UniValue objM(UniValue::VOBJ);
        objM.push_back(Pair("buckets", snBuckets));
        objM.push_back(Pair("messages", snMessages));
        objM.push_back(Pair("size", part::BytesReadable(nBytes)));
        result.push_back(Pair("total", objM));

    } else
    if (mode == "dump")
    {
        {
            LOCK(cs_smsg);
            std::map<int64_t, SecMsgBucket>::iterator it;
            it = smsgBuckets.begin();

            for (it = smsgBuckets.begin(); it != smsgBuckets.end(); ++it)
            {
                std::string sFile = boost::lexical_cast<std::string>(it->first) + "_01.dat";

                try {
                    boost::filesystem::path fullPath = GetDataDir() / "smsgStore" / sFile;
                    boost::filesystem::remove(fullPath);
                } catch (const boost::filesystem::filesystem_error& ex)
                {
                    //objM.push_back(Pair("file size, error", ex.what()));
                    LogPrintf("Error removing bucket file %s.\n", ex.what());
                };
            };
            smsgBuckets.clear();
        }; // cs_smsg

        result.push_back(Pair("result", "Removed all buckets."));

    } else
    {
        result.push_back(Pair("result", "Unknown Mode."));
        result.push_back(Pair("expected", "[stats|dump]."));
    };


    return result;
};

static bool sortMsgAsc(const std::pair<int64_t, UniValue> &a, const std::pair<int64_t, UniValue> &b)
{
    return a.first < b.first;
};

static bool sortMsgDesc(const std::pair<int64_t, UniValue> &a, const std::pair<int64_t, UniValue> &b)
{
    return a.first > b.first;
};

static int64_t strToEpoch(const char *input)
{
    // strptime is posix only, boost::posix_time needs an extra lib linked
    
    int year, month, day, hours, minutes, seconds;
    int n = sscanf(input, "%d-%d-%dT%d:%d:%d", 
        &year, &month, &day, &hours, &minutes, &seconds);
    
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    
    if (n > 0
        && year >= 1970 && year <= 9999)
        tm.tm_year = year - 1900;
    
    if (n > 1
        && month > 0 && month < 13)
        tm.tm_mon = month - 1;
    
    if (n > 2
        && day > 0 && day < 32)
        tm.tm_mday = day;
    
    if (n > 3
        && hours >= 0 && hours < 24)
        tm.tm_hour = hours;
    
    if (n > 4
        && minutes >= 0 && minutes < 60)
        tm.tm_min = minutes;
    
    if (n > 5
        && seconds >= 0 && seconds < 60)
        tm.tm_sec = seconds;
    
    return (int64_t) mktime(&tm);
};

UniValue smsgview(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 6)
        throw std::runtime_error(
            "smsgview [address/label|(asc/desc|-from yyyy-mm-dd|-to yyyy-mm-dd)]\n"
            "View messages by address."
            "Setting address to '*' will match all addresses"
            "'abc*' will match addresses with labels beginning 'abc'"
            "'*abc' will match addresses with labels ending 'abc'"
            "Full date/time format for from and to is yyyy-mm-ddThh:mm:ss"
            "From and to will accept incomplete inputs like: -from 2016");
    
    if (!fSecMsgEnabled)
        throw std::runtime_error("Secure messaging is disabled.");

    if (pwalletSmsg->IsLocked())
        throw std::runtime_error("Wallet is locked.");

    char cbuf[256];
    bool fMatchAll = false;
    bool fDesc = false;
    int64_t tFrom = 0, tTo = 0;
    std::vector<CKeyID> vMatchAddress;
    std::string sTemp;

    if (request.params.size() > 0)
    {
        sTemp = request.params[0].get_str();

        // Blank address or "*" will match all
        if (sTemp.length() < 1) // Error instead?
            fMatchAll = true;
        else
        if (sTemp.length() == 1 && sTemp[0] == '*')
            fMatchAll = true;

        if (!fMatchAll)
        {
            CBitcoinAddress checkValid(sTemp);

            if (checkValid.IsValid())
            {
                CKeyID ki;
                checkValid.GetKeyID(ki);
                vMatchAddress.push_back(ki);
            } else
            {
                // - Lookup address by label, can match multiple addresses

                // TODO: Use Boost.Regex?
                int matchType = 0; // 0 full match, 1 startswith, 2 endswith
                if (sTemp[0] == '*')
                {
                    matchType = 1;
                    sTemp.erase(0, 1);
                } else
                if (sTemp[sTemp.length()-1] == '*')
                {
                    matchType = 2;
                    sTemp.erase(sTemp.length()-1, 1);
                };
                
                std::map<CTxDestination, CAddressBookData>::iterator itl;
                
                for (itl = pwalletSmsg->mapAddressBook.begin(); itl != pwalletSmsg->mapAddressBook.end(); ++itl)
                {
                    if (part::stringsMatchI(itl->second.name, sTemp, matchType))
                    {
                        CBitcoinAddress checkValid(itl->first);
                        if (checkValid.IsValid())
                        {
                            CKeyID ki;
                            checkValid.GetKeyID(ki);
                            vMatchAddress.push_back(ki);
                            //LogPrintf("[rm] matched address: %s\n", checkValid.ToString().c_str());
                        } else
                        {
                            LogPrintf("Warning: matched invalid address: %s\n", checkValid.ToString().c_str());
                        };
                    };
                };
            };
        };
    } else
    {
        fMatchAll = true;
    };
    
    size_t i = 1;
    while (i < request.params.size())
    {
        sTemp = request.params[i].get_str();
        if (sTemp == "-from")
        {
            if (i >= request.params.size()-1)
                throw std::runtime_error("Argument required for: " + sTemp);
            i++;
            sTemp = request.params[i].get_str();
            tFrom = strToEpoch(sTemp.c_str());
            if (tFrom < 0)
                throw std::runtime_error("from format error: " + std::string(strerror(errno)));
        } else
        if (sTemp == "-to")
        {
            if (i >= request.params.size()-1)
                throw std::runtime_error("Argument required for: " + sTemp);
            i++;
            sTemp = request.params[i].get_str();
            tTo = strToEpoch(sTemp.c_str());
            if (tTo < 0)
                throw std::runtime_error("to format error: " + std::string(strerror(errno)));
        } else
        if (sTemp == "asc")
        {
            fDesc = false;
        } else
        if (sTemp == "desc")
        {
            fDesc = true;
        } else
        {
            throw std::runtime_error("Unknown parameter: " + sTemp);
        };

        i++;
    };

    if (!fMatchAll && vMatchAddress.size() < 1)
        throw std::runtime_error("No address found.");

    UniValue result(UniValue::VOBJ);

    std::map<CKeyID, std::string> mLabelCache;
    std::vector<std::pair<int64_t, UniValue> > vMessages;

    std::vector<std::string> vPrefixes;
    vPrefixes.push_back("im");
    vPrefixes.push_back("sm");

    unsigned char chKey[18];
    size_t nMessages = 0;
    UniValue messageList(UniValue::VARR);

    size_t debugEmptySent = 0;

    {
        LOCK(cs_smsgDB);
        SecMsgDB dbMsg;
        if (!dbMsg.Open("cr"))
            throw std::runtime_error("Could not open DB.");

        std::vector<std::string>::iterator itp;
        std::vector<CKeyID>::iterator its;
        for (itp = vPrefixes.begin(); itp < vPrefixes.end(); ++itp)
        {
            bool fInbox = *itp == std::string("im");

            dbMsg.TxnBegin();

            leveldb::Iterator *it = dbMsg.pdb->NewIterator(leveldb::ReadOptions());
            SecMsgStored smsgStored;
            MessageData msg;

            while (dbMsg.NextSmesg(it, *itp, chKey, smsgStored))
            {
                if (!fInbox && smsgStored.addrOutbox.IsNull())
                {
                    if (fDebugSmsg)
                        debugEmptySent++;
                    continue;
                };

                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;

                int rv;
                if ((rv = SecureMsgDecrypt(false, fInbox ? smsgStored.addrTo : smsgStored.addrOutbox,
                    &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg)) == 0)
                {
                    if ((tFrom > 0 && msg.timestamp < tFrom)
                        || (tTo > 0 && msg.timestamp > tTo))
                        continue;

                    CKeyID kiFrom;
                    CBitcoinAddress addrFrom(msg.sFromAddress);
                    if (addrFrom.IsValid())
                        addrFrom.GetKeyID(kiFrom);
                    
                    
                    if (!fMatchAll)
                    {
                        bool fSkip = true;

                        for (its = vMatchAddress.begin(); its < vMatchAddress.end(); ++its)
                        {
                            if (*its == kiFrom
                                || *its == smsgStored.addrTo)
                            {
                                fSkip = false;
                                break;
                            };
                        };

                        if (fSkip)
                            continue;
                    };

                    // - Get labels for addresses, cache found labels.
                    std::string lblFrom, lblTo;
                    std::map<CKeyID, std::string>::iterator itl;

                    if ((itl = mLabelCache.find(kiFrom)) != mLabelCache.end())
                    {
                        lblFrom = itl->second;
                    } else
                    {
                        CBitcoinAddress address_parsed(kiFrom);
                        std::map<CTxDestination, CAddressBookData>::iterator
                            mi(pwalletSmsg->mapAddressBook.find(address_parsed.Get()));
                        if (mi != pwalletSmsg->mapAddressBook.end())
                            lblFrom = mi->second.name;
                        mLabelCache[kiFrom] = lblFrom;
                    };

                    if ((itl = mLabelCache.find(smsgStored.addrTo)) != mLabelCache.end())
                    {
                        lblTo = itl->second;
                    } else
                    {
                        CBitcoinAddress address_parsed(smsgStored.addrTo);
                        std::map<CTxDestination, CAddressBookData>::iterator
                            mi(pwalletSmsg->mapAddressBook.find(address_parsed.Get()));
                        if (mi != pwalletSmsg->mapAddressBook.end())
                            lblTo = mi->second.name;
                        mLabelCache[smsgStored.addrTo] = lblTo;
                    };

                    std::string sFrom = kiFrom.IsNull() ? "anon" : CBitcoinAddress(kiFrom).ToString();
                    std::string sTo = CBitcoinAddress(smsgStored.addrTo).ToString();
                    if (lblFrom.length() != 0)
                        sFrom += " (" + lblFrom + ")";
                    if (lblTo.length() != 0)
                        sTo += " (" + lblTo + ")";

                    UniValue objM(UniValue::VOBJ);
                    objM.push_back(Pair("sent", part::GetTimeString(msg.timestamp, cbuf, sizeof(cbuf))));
                    objM.push_back(Pair("from", sFrom));
                    objM.push_back(Pair("to", sTo));
                    objM.push_back(Pair("text", std::string((char*)&msg.vchMessage[0])));
                    
                    vMessages.push_back(std::make_pair(msg.timestamp, objM));
                } else
                {
                    LogPrintf("%s: SecureMsgDecrypt failed, %s.", __func__, HexStr(chKey, chKey+18).c_str());
                };
            };
            delete it;
            
            dbMsg.TxnCommit();
        };
    } // cs_smsgDB


    std::sort(vMessages.begin(), vMessages.end(), fDesc ? sortMsgDesc : sortMsgAsc);

    std::vector<std::pair<int64_t, UniValue> >::iterator itm;
    for (itm = vMessages.begin(); itm < vMessages.end(); ++itm)
    {
        messageList.push_back(itm->second);
        nMessages++;
    };

    result.push_back(Pair("messages", messageList));
    
    if (fDebugSmsg)
        result.push_back(Pair("debug empty sent", (int)debugEmptySent));

    result.push_back(Pair("result", strprintf("Displayed %u messages.", nMessages)));
    if (tFrom > 0)
        result.push_back(Pair("from", part::GetTimeString(tFrom, cbuf, sizeof(cbuf))));
    if (tTo > 0)
        result.push_back(Pair("to", part::GetTimeString(tTo, cbuf, sizeof(cbuf))));

    return result;
};



static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "smsg",               "smsgenable",             &smsgenable,             true, {} },
    { "smsg",               "smsgdisable",            &smsgdisable,            true, {} },
    { "smsg",               "smsgoptions",            &smsgoptions,            true, {} },
    { "smsg",               "smsglocalkeys",          &smsglocalkeys,          true, {} },
    { "smsg",               "smsgscanchain",          &smsgscanchain,          true, {} },
    { "smsg",               "smsgscanbuckets",        &smsgscanbuckets,        true, {} },
    { "smsg",               "smsgaddkey",             &smsgaddkey,             true, {} },
    { "smsg",               "smsggetpubkey",          &smsggetpubkey,          true, {} },
    { "smsg",               "smsgsend",               &smsgsend,               true, {} },
    { "smsg",               "smsgsendanon",           &smsgsendanon,           true, {} },
    { "smsg",               "smsginbox",              &smsginbox,              true, {} },
    { "smsg",               "smsgoutbox",             &smsgoutbox,             true, {} },
    { "smsg",               "smsgbuckets",            &smsgbuckets,            true, {} },
    { "smsg",               "smsgview",               &smsgview,               true, {} },
    
    


    /* Not shown in help */
    //{ "hidden",             "setmocktime",            &setmocktime,            true  },
};

void RegisterSmsgRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
