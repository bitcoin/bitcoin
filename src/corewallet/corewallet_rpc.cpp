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

#include "json/json_spirit_utils.h"

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
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
    
typedef json_spirit::Value(*rpcfn_type)(const json_spirit::Array& params, bool fHelp);

struct RPCDispatchEntry
{
    std::string name;
    rpcfn_type actor;
};

//dispatch compatible rpc function definitions
json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp);
json_spirit::Value listaddresses(const json_spirit::Array& params, bool fHelp);
json_spirit::Value addwallet(const json_spirit::Array& params, bool fHelp);
json_spirit::Value listwallets(const json_spirit::Array& params, bool fHelp);
json_spirit::Value help(const json_spirit::Array& params, bool fHelp);
    
static const RPCDispatchEntry vDispatchEntries[] = {
    { "getnewaddress",                  &getnewaddress },
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
Wallet* WalletFromParams(const json_spirit::Array& params)
{
    std::string walletID = ""; //"" stands for default wallet
    if (params.size() > 0 && params[0].type() == json_spirit::obj_type) //TODO also allow params at index position different the 0
    {
        const json_spirit::Object& o = params[0].get_obj();
        json_spirit::Value possibleValue = find_value(o, "walletid");
        if (!possibleValue.is_null())
            walletID = possibleValue.get_str();
    }
    Wallet* wallet = CoreWallet::GetManager()->GetWalletWithID(walletID);
    if (!wallet)
        throw std::runtime_error("Wallet ("+SanitizeString(walletID)+") not found");
    
    return wallet;
}

//! search in exiting json object for a key/value pair and returns value
json_spirit::Value ValueFromParams(const json_spirit::Array& params, const std::string& key)
{
    if (params.size() < 1 || params[0].type() != json_spirit::obj_type)
        throw std::runtime_error("invalid parameters, always use a json key/value object");
    
    const json_spirit::Object& o = params[0].get_obj();
    return find_value(o, key);
}
    
///////////////////////////
// Keys/Addresses stack
///////////////////////////
json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw RPCHelpException(
                            "getnewaddress\n"
                            "\nReturns a new Bitcoin address for receiving payments.\n"
                            "\nResult:\n"
                            "\"bitcoinaddress\"    (string) The new bitcoin address\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getnewaddress", "")
                            + HelpExampleRpc("getnewaddress", "")
                            );
    

    Wallet *wallet = WalletFromParams(params);
    json_spirit::Value value = ValueFromParams(params, "chainpath");
    json_spirit::Value valueIndex = ValueFromParams(params, "index");
    
    CKeyID keyID;
    
    if(value.is_null())
    {
        int index = -1;
        if (!valueIndex.is_null())
            index = atoi(valueIndex.get_str());
            
        CPubKey newKey = wallet->GenerateNewKey(index);
        keyID = newKey.GetID();
        wallet->SetAddressBook(keyID, "receive");
        return CBitcoinAddress(keyID).ToString();
    }
    else
    {
        //Bip32
        std::string chainpath = value.get_str();
        boost::to_lower(chainpath);
        
        json_spirit::Object obj;
        if (chainpath == "m")
        {
            unsigned char vch[32];
            json_spirit::Value seedHex = ValueFromParams(params, "seed");
            bool useSeed = false;
            if (!seedHex.is_null())
            {
                std::vector<unsigned char> result = ParseHex(seedHex.get_str());
                memcpy((void *)&vch,(const void *)&result.front(),32);
                useSeed = true;
            }
            
            wallet->GenerateBip32Structure("m/44'/0'/0'/", vch, useSeed);
            if (!useSeed)
                obj.push_back(json_spirit::Pair("seed", HexStr(vch, vch+sizeof(vch))));
        }
        else
        {
//            CPubKey newKey = wallet->GenerateBip32Structure(vch, useSeed);
//            keyID = newKey.GetID();
//            wallet->SetAddressBook(keyID, "receive");
//            obj.push_back(json_spirit::Pair("address", CBitcoinAddress(keyID).ToString()));
        }
            
        return obj;
    }
}
    
json_spirit::Value listaddresses(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp)
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
    Wallet *wallet = WalletFromParams(params);
    json_spirit::Array ret;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CAddressBookMetadata)& item, wallet->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const CAddressBookMetadata& metadata = item.second;
        json_spirit::Object obj;
        obj.push_back(json_spirit::Pair(address.ToString(), metadata.purpose));
        ret.push_back(obj);
    }
    
    CKeyID masterKeyID = wallet->masterKeyID;
    if (!masterKeyID.IsNull())
    {
        json_spirit::Object obj;
        obj.push_back(json_spirit::Pair(CBitcoinAddress(masterKeyID).ToString(), "masterkey"));
        ret.push_back(obj);
    }
    
    return ret;
}

    
    
///////////////////////////
// MultiWallet stack
///////////////////////////
json_spirit::Value addwallet(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw RPCHelpException(
                             "addwallet \"walletid\"\n"
                             "\nArguments:\n"
                             "  \"walletid\"    (string, required) allowed characters: A-Za-z0-9._-\n"
                             "\nExamples:\n"
                             + HelpExampleCli("addwallet", "\"anotherwallet\"")
                             + HelpExampleRpc("addwallet", "\"anotherwallet\"")
                             );
    
    std::string walletID = params[0].get_str();
    CoreWallet::GetManager()->AddNewWallet(walletID);
    
    return json_spirit::Value::null;
}
    
json_spirit::Value listwallets(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
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
    
    json_spirit::Array ret;
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
json_spirit::Value help(const json_spirit::Array& params, bool fHelp)
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
    json_spirit::Value value = ValueFromParams(params, "command");
    
    unsigned int i;
    for (i = 0; i < (sizeof(vDispatchEntries) / sizeof(vDispatchEntries[0])); i++)
    {
        try {
            json_spirit::Array params;
            vDispatchEntries[i].actor(params, true);
        } catch (const RPCHelpException& e)
        {
            // Help text is returned in an exception
            std::string strHelp = std::string(e.what());
            if (!value.is_null() && value.get_str() == vDispatchEntries[i].name)
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
void ExecuteRPC(const std::string& strMethod, const json_spirit::Array& params, json_spirit::Value& result, bool& accept)
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