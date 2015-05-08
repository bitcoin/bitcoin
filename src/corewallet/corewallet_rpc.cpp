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

json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                            "getnewaddress\n"
                            "\nReturns a new Bitcoin address for receiving payments.\n"
                            "\nResult:\n"
                            "\"bitcoinaddress\"    (string) The new bitcoin address\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getnewaddress", "")
                            + HelpExampleRpc("getnewaddress", "")
                            );
    

    CPubKey newKey = CoreWallet::GetWallet()->GenerateNewKey();
    CKeyID keyID = newKey.GetID();
    
    CoreWallet::GetWallet()->SetAddressBook(keyID, "receive");
    
    return CBitcoinAddress(keyID).ToString();
}
    
json_spirit::Value getaddresses(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                            "getaddressesbyaccount \"account\"\n"
                            "\nResult:\n"
                            "[                     (json array of string)\n"
                            "  \"bitcoinaddress\"  (string) a bitcoin address associated with the given account\n"
                            "  ,...\n"
                            "]\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getaddressesbyaccount", "\"tabby\"")
                            + HelpExampleRpc("getaddressesbyaccount", "\"tabby\"")
                            );
    
    json_spirit::Array ret;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CoreWallet::CAddressBookMetadata)& item, CoreWallet::GetWallet()->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        ret.push_back(address.ToString());
    }
    return ret;
}
    
void ExecuteRPC(const std::string& strMethod, const json_spirit::Array& params, json_spirit::Value& result, bool& accept)
{
    if(strMethod == "getnewaddress")
    {
        result = getnewaddress(params, false);
        accept = true;
    }
    else if(strMethod == "getaddresses")
    {
        result = getaddresses(params, false);
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