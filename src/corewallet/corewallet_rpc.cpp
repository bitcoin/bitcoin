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
#include <boost/foreach.hpp>

typedef json_spirit::Value(*rpcfn_type)(const json_spirit::Array& params, bool fHelp);

namespace CoreWallet
{

///////////////////////////
// helpers
///////////////////////////

Wallet* WalletFromParams(const json_spirit::Array& params)
{
    if (params.size() < 1 || params[0].type() != json_spirit::obj_type)
        throw std::runtime_error("invalid parameters, always use a json key/value object");
    std::string walletID = ""; //"" stands for default wallet
    
    const json_spirit::Object& o = params[0].get_obj();
    walletID = find_value(o, "walletid").get_str();
    if (params.size() > 0 && params[0].type() == json_spirit::obj_type) //TODO also allow params at index position different the 0
    {
        const json_spirit::Object& o = params[0].get_obj();
        json_spirit::Value possibleValue = find_value(o, "walletid");
        if (!possibleValue.is_null())
            walletID = possibleValue.get_str();
    }
    Wallet* wallet = CoreWallet::GetWalletWithID(walletID);
    if (!wallet)
        throw std::runtime_error("Wallet ("+SanitizeString(walletID)+") not found");
    
    return wallet;
}

///////////////////////////
// Keys/Addresses stack
///////////////////////////
json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
                            "getnewaddress\n"
                            "\nReturns a new Bitcoin address for receiving payments.\n"
                            "\nResult:\n"
                            "\"bitcoinaddress\"    (string) The new bitcoin address\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getnewaddress", "")
                            + HelpExampleRpc("getnewaddress", "")
                            );
    

    Wallet *wallet = WalletFromParams(params);
    CPubKey newKey = wallet->GenerateNewKey();
    CKeyID keyID = newKey.GetID();
    
    wallet->SetAddressBook(keyID, "receive");
    
    return CBitcoinAddress(keyID).ToString();
}
    
json_spirit::Value listaddresses(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
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
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CoreWallet::CAddressBookMetadata)& item, wallet->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        ret.push_back(address.ToString());
    }
    return ret;
}

    
    
///////////////////////////
// MultiWallet stack
///////////////////////////
json_spirit::Value addwallet(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
                             "addwallet \"walletid\"\n"
                             "\nArguments:\n"
                             "  \"walletid\"    (string, required) allowed characters: A-Za-z0-9._-\n"
                             "\nExamples:\n"
                             + HelpExampleCli("addwallet", "\"anotherwallet\"")
                             + HelpExampleRpc("addwallet", "\"anotherwallet\"")
                             );
    
    std::string walletID = params[0].get_str();
    CoreWallet::AddNewWallet(walletID);
    
    return json_spirit::Value::null;
}
    
json_spirit::Value listwallets(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
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
    std::vector<std::string> vWalletIDs = CoreWallet::GetWalletIDs();
    BOOST_FOREACH(const std::string& walletID, vWalletIDs)
    {
        ret.push_back(walletID);
    }
    
    return ret;
}

    
///////////////////////////
// Dispatching/signaling stack
///////////////////////////
void ExecuteRPC(const std::string& strMethod, const json_spirit::Array& params, json_spirit::Value& result, bool& accept)
{
    if(strMethod == "getnewaddress")
    {
        result = getnewaddress(params, false);
        accept = true;
    }
    else if(strMethod == "listaddresses")
    {
        result = listaddresses(params, false);
        accept = true;
    }
    else if(strMethod == "addwallet")
    {
        result = addwallet(params, false);
        accept = true;
    }
    else if(strMethod == "listwallets")
    {
        result = listwallets(params, false);
        accept = true;
    }
}
    
void AddRPCHelp(std::string& helpString)
{
    helpString += "\n== CoreWallet ==\n";
    try {
        json_spirit::Array params;
        getnewaddress(params, true);
    } catch (const std::exception& e)
    {
        // Help text is returned in an exception
        std::string strHelp = std::string(e.what());
        if (strHelp.find('\n') != std::string::npos)
            strHelp = strHelp.substr(0, strHelp.find('\n'));
        
        helpString+=strHelp;
        
    }
}
    

}; //end namespace