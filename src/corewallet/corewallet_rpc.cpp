// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet.h"
#include "pubkey.h"
#include "rpcserver.h"
#include "utilstrencodings.h"

#include <string>

#include "univalue/univalue.h"

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet
{

//!rpc help exception
class RPCHelpException: public std::exception
{
public:
    explicit RPCHelpException(const std::string& message): msg_(message) {}
    virtual ~RPCHelpException() throw (){}
    virtual const char* what() const throw (){
        return msg_.c_str();
    }
protected:
    std::string msg_;
};
    
typedef UniValue(*rpcfn_type)(const UniValue& params, bool fHelp);

struct RPCDispatchEntry
{
    std::string name;
    rpcfn_type actor;
};

//dispatch compatible rpc function definitions
UniValue getaddress(const UniValue& params, bool fHelp);
UniValue listaddresses(const UniValue& params, bool fHelp);
UniValue addwallet(const UniValue& params, bool fHelp);
UniValue listwallets(const UniValue& params, bool fHelp);
UniValue help(const UniValue& params, bool fHelp);
    
static const RPCDispatchEntry vDispatchEntries[] = {
    { "getaddress",                  &getaddress },
    { "listaddresses",                  &listaddresses },
    
    // Multiwallet
    { "addwallet",                      &addwallet },
    { "listwallets",                    &listwallets },
    
    // Help / Debug
    { "help",                           &help },
};

    
///////////////////////////
// helpers
///////////////////////////

//! try to filter out the desired wallet instance via given params
Wallet* WalletFromParams(const UniValue& params)
{
    std::string walletID = ""; //"" stands for default wallet
    if (params.size() > 0 && params[0].isObject()) //TODO also allow params at index position different the 0
    {
        const UniValue& o = params[0].get_obj();
        UniValue possibleValue = find_value(o, "walletid");
        if (!possibleValue.isNull())
            walletID = possibleValue.get_str();
    }
    Wallet* wallet = CoreWallet::GetManager()->GetWalletWithID(walletID);
    if (!wallet)
        throw std::runtime_error("Wallet ("+SanitizeString(walletID)+") not found");
    
    return wallet;
}

//! search in exiting json object for a key/value pair and returns value
UniValue ValueFromParams(const UniValue& params, const std::string& key)
{
    if (params.size() > 0 && params[0].isObject())
    {
        const UniValue& o = params[0].get_obj();
        return find_value(o, key);
    }
    else
    {
        return NullUniValue;
    }
}
    
///////////////////////////
// Keys/Addresses stack
///////////////////////////
UniValue getaddress(const UniValue& params, bool fHelp)
{
    Wallet *wallet = WalletFromParams(params);
    if (fHelp || !wallet)
        throw RPCHelpException(
                            "getaddress\n"
                            "\nReturns a new Bitcoin address for receiving payments.\n"
                            "\nResult:\n"
                            "\"bitcoinaddress\"    (string) The new bitcoin address\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getaddress", "")
                            + HelpExampleRpc("getaddress", "")
                            );
    

    UniValue valueIndex = ValueFromParams(params, "index");
    
    CKeyID keyID;
    CPubKey newKey;
    unsigned int index;

    if (!valueIndex.isNull())
    {
        if (valueIndex.isStr())
            index = atoi(valueIndex.get_str());
        else
            index = valueIndex.get_int();
    }

    if (valueIndex.isNull())
    {
        newKey = wallet->GetNextUnusedKey(index);
    }
    else
    {
        newKey = wallet->GetKeyAtIndex(index);
    }

    keyID = newKey.GetID();
    std::stringstream ss; ss << index;
    UniValue obj(UniValue::VOBJ);
    std::string chainPath = wallet->strChainPath;
    boost::replace_all(chainPath, "c", "0");
    boost::replace_all(chainPath, "k", ss.str());
    obj.push_back(Pair(chainPath, CBitcoinAddress(keyID).ToString()));


    return obj;
}

UniValue listaddresses(const UniValue& params, bool fHelp)
{
    Wallet *wallet = WalletFromParams(params);
    if (fHelp || !wallet)
        throw RPCHelpException(
                            "listaddresses\n"
                            "\nResult:\n"
                            "[                     (json array of string)\n"
                            "  \"bitcoinaddress\"  (string) a bitcoin address associated with the given account\n"
                            "  ,...\n"
                            "]\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getaddressesbyaccount", "\"tabby\"")
                            + HelpExampleRpc("getaddressesbyaccount", "\"tabby\"")
                            );
    UniValue ret(UniValue::VARR);
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CAddressBookMetadata)& item, wallet->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const CAddressBookMetadata& metadata = item.second;
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair(address.ToString(), metadata.purpose));
        ret.push_back(obj);
    }
    
    CKeyID masterKeyID = wallet->masterKeyID;
    if (!masterKeyID.IsNull())
    {
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair(CBitcoinAddress(masterKeyID).ToString(), "masterkey"));
        ret.push_back(obj);
    }
    
    return ret;
}

    
    
///////////////////////////
// MultiWallet stack
///////////////////////////
UniValue addwallet(const UniValue& params, bool fHelp)
{
    UniValue walletIDValue = ValueFromParams(params, "walletid");
    if (fHelp || walletIDValue.isNull())
        throw RPCHelpException(
                             "addwallet walletid=<walletid>\n"
                             "\nArguments:\n"
                             "  \"walletid\"    (string, required) allowed characters: A-Za-z0-9._-\n"
                             "\nExamples:\n"
                             + HelpExampleCli("addwallet", "\"anotherwallet\"")
                             + HelpExampleRpc("addwallet", "\"anotherwallet\"")
                             );

    UniValue obj(UniValue::VOBJ);

    std::string walletID = walletIDValue.get_str();
    Wallet *wallet = CoreWallet::GetManager()->AddNewWallet(walletID);

    unsigned char vch[32];
    UniValue seedHex = ValueFromParams(params, "seed");
    bool useSeed = false;
    if (!seedHex.isNull())
    {
        std::vector<unsigned char> result = ParseHex(seedHex.get_str());
        memcpy((void *)&vch,(const void *)&result.front(),32);
        useSeed = true;
    }


    bool success = wallet->GenerateBip32Structure("m/44'/0'/0'/c/k", vch, useSeed);
    if (!useSeed)
        obj.push_back(Pair("seed", HexStr(vch, vch+sizeof(vch))));


    return obj;
}

UniValue listwallets(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw RPCHelpException(
                                 "listwallets\n"
                                 "\nResult:\n"
                                 "[                     (json array of string)\n"
                                 "  \"mainWallet\"  (string) a wallet identifier\n"
                                 "  \"anotherWallet\"\n"
                                 "]\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("listwallets", "")
                                 + HelpExampleRpc("listwallets", "")
                                 );
    
    UniValue ret(UniValue::VARR);
    std::vector<std::string> vWalletIDs = CoreWallet::GetManager()->GetWalletIDs();
    BOOST_FOREACH(const std::string& walletID, vWalletIDs)
    {
        ret.push_back(walletID);
    }
    
    return ret;
}

///////////////////////////
// Help stack
///////////////////////////
UniValue help(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw RPCHelpException(
                            "help ( \"command\" )\n"
                            "\nList all commands, or get help for a specified command.\n"
                            "\nArguments:\n"
                            "1. \"command\"     (string, optional) The command to get help on\n"
                            "\nResult:\n"
                            "\"text\"     (string) The help text\n"
                            );
    
    std::string helpString = "\n== CoreWallet ==\n";
    UniValue value = ValueFromParams(params, "command");
    
    unsigned int i;
    for (i = 0; i < (sizeof(vDispatchEntries) / sizeof(vDispatchEntries[0])); i++)
    {
        try {
            UniValue params(UniValue::VARR);
            vDispatchEntries[i].actor(params, true);
        } catch (const RPCHelpException& e)
        {
            // Help text is returned in an exception
            std::string strHelp = std::string(e.what());
            if (!value.isNull() && value.get_str() == vDispatchEntries[i].name)
            {
                //shortcut if uses likes help of a single command
                helpString = strHelp;
                return helpString;
            }
            
            if (strHelp.find('\n') != std::string::npos)
                strHelp = strHelp.substr(0, strHelp.find('\n'));
               
            helpString+=strHelp+"\n";
        }
    }
    
    return helpString;
}
    
///////////////////////////
// Dispatching/signaling stack
///////////////////////////
    
//! search for command in dispatch table and executes
void ExecuteRPC(const std::string& strMethod, const UniValue& params, UniValue& result, bool& accept)
{
    unsigned int i;
    for (i = 0; i < (sizeof(vDispatchEntries) / sizeof(vDispatchEntries[0])); i++)
    {
        if(vDispatchEntries[i].name == strMethod)
        {
            try
            {
                result = vDispatchEntries[i].actor(params, false);
            }
            catch (const RPCHelpException& e)
            {
                throw JSONRPCError(RPC_MISC_ERROR, e.what());
            }
            
            accept = true;
        }
    }
}

//! Create helpstring by catching "help"-exceptions over dispatching table.
void AddRPCHelp(std::string& helpString)
{
    
}
    

}; //end namespace