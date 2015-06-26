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
UniValue hdaddchain(const UniValue& params, bool fHelp);
UniValue hdsetchain(const UniValue& params, bool fHelp);
UniValue hdgetinfo(const UniValue& params, bool fHelp);
UniValue hdgetaddress(const UniValue& params, bool fHelp);
UniValue getaddress(const UniValue& params, bool fHelp);
UniValue listaddresses(const UniValue& params, bool fHelp);
UniValue addwallet(const UniValue& params, bool fHelp);
UniValue listwallets(const UniValue& params, bool fHelp);
UniValue help(const UniValue& params, bool fHelp);
    
static const RPCDispatchEntry vDispatchEntries[] = {
    { "getnewaddress",                    &getnewaddress },
    { "listaddresses",                    &listaddresses },

    { "hdaddchain",                       &hdaddchain },
    { "hdsetchain",                       &hdsetchain },
    { "hdgetinfo",                        &hdgetinfo },
    { "hdgetaddress",                     &hdgetaddress },
    
    // Multiwallet
    { "addwallet",                        &addwallet },
    { "listwallets",                      &listwallets },
    
    // Help / Debug
    { "help",                             &help },
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
UniValue getnewaddress(const UniValue& params, bool fHelp)
{
    Wallet *wallet = WalletFromParams(params);
    if (fHelp || !wallet)
        throw RPCHelpException(
                            "getnewaddress\n"
                            "\nReturns a new non-deterministic Bitcoin address for receiving payments.\n"
                            "\nResult:\n"
                            "\"bitcoinaddress\"    (string) The new bitcoin address\n"
                            "\nExamples:\n"
                            + HelpExampleCli("getaddress", "")
                            + HelpExampleRpc("getaddress", "")
                            );
    

    //TODO: non deterministic single key generation

    return NullUniValue;
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
    if (fHelp || params.size() == 1)
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

/*
 
///////////////////////////
// HD/Bip32 stack
///////////////////////////

default chainpath after bip44
m = master key
<num>' or <num>h = hardened key
c stands for internal/external chain switch
c=0 for external addresses
c=1 for internal addresses

example "m/44'/0'/0'/c" will result in m/44'/0'/0'/0/0 for the first external key
example "m/44'/0'/0'/c" will result in m/44'/0'/0'/1/0 for the first internal key
example "m/44'/0'/0'/c" will result in m/44'/0'/0'/0/1 for the second external key
example "m/44'/0'/0'/c" will result in m/44'/0'/0'/1/1 for the second internal key
*/
const std::string hd_default_chainpath = "m/44'/0'/0'/c";

UniValue hdaddchain(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw std::runtime_error(
                            "hdaddchain (<chainpath>|default) (<masterseed_hex>)\n"
                            "\nAdds a HD/Bip32 chain \n"
                            "\nArguments:\n"
                            "1. chainpath        (string, optional, default="+hd_default_chainpath+") chainpath for hd wallet structure\n"
                            "   m stands for master, c for internal/external key-switch, k stands for upcounting child key index"
                            "2. masterseed_hex   (string/hex, optional) use this seed for master key generation\n"
                            "\nResult\n"
                            "{\n"
                            "  \"seed_hex\" : \"<hexstr>\",  (string) seed used during master key generation (only if no masterseed hex was provided\n"
                            "}\n"

                            "\nExamples\n"
                            + HelpExampleCli("hdaddchain", "set")
                            + HelpExampleCli("hdaddchain", "set m/44'/0'/0'/c/k")
                            + HelpExampleRpc("hdaddchain", "set m/44'/0'/0'/c/k")
                            );

    UniValue result(UniValue::VOBJ);

    Wallet *wallet = WalletFromParams(params);
    UniValue chainpathParam = ValueFromParams(params, "chainpath");
    UniValue seedParam = ValueFromParams(params, "seed");

    //EnsureWalletIsUnlocked();

    const unsigned int bip32MasterSeedLength = 32;
    CKeyingMaterial vSeed = CKeyingMaterial(bip32MasterSeedLength);
    bool fGenerateMasterSeed = true;
    HDChainID chainId;
    std::string chainPath = hd_default_chainpath;
    if (!chainpathParam.isNull() && chainpathParam.isStr() && chainpathParam.get_str() != "default")
        chainPath = chainpathParam.get_str(); //todo bip32 chainpath sanity

    if (!seedParam.isNull() && seedParam.isStr())
    {
        if (!IsHex(seedParam.get_str()))
            throw std::runtime_error("HD master seed must encoded in hex");

        std::vector<unsigned char> seed = ParseHex(seedParam.get_str());
        if (seed.size() != bip32MasterSeedLength)
            throw std::runtime_error("HD master seed must be "+itostr(bip32MasterSeedLength*8)+"bit");

        memcpy(&vSeed[0], &seed[0], bip32MasterSeedLength);
        memory_cleanse(&seed[0], bip32MasterSeedLength);
        fGenerateMasterSeed = false;
    }
    
    wallet->HDSetChainPath(chainPath, fGenerateMasterSeed, vSeed, chainId);
    if (fGenerateMasterSeed)
        result.push_back(Pair("seed_hex", HexStr(vSeed)));
    result.push_back(Pair("chainid", chainId.GetHex()));
    result.push_back(Pair("chainpath", chainPath));
    return result;
}

UniValue hdsetchain(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw std::runtime_error(
                            "hdsetchain <chainid>\n"
                            "\nReturns some hd relevant information.\n"
                            "\nArguments:\n"
                            "1. \"chainid\"        (string|hex, required) chainid is a bitcoin hash of the master public key of the corresponding chain.\n"
                            "\nExamples:\n"
                            + HelpExampleCli("hdsetchain", "")
                            + HelpExampleCli("hdgetinfo", "True")
                            + HelpExampleRpc("hdgetinfo", "")
                            );

    Wallet *wallet = WalletFromParams(params);
    UniValue chainIdParam = ValueFromParams(params, "chainid");

    HDChainID chainId;
    if (!chainIdParam.isStr() && !IsHex(chainIdParam.get_str()))
        throw std::runtime_error("Chain id format is invalid");

    chainId.SetHex(chainIdParam.get_str());

    if (!wallet->HDSetActiveChainID(chainId))
        throw std::runtime_error("Could not set active chain");

    return NullUniValue;
}

UniValue hdgetinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                            "hdgetinfo\n"
                            "\nReturns some hd relevant information.\n"
                            "\nArguments:\n"
                            "{\n"
                            "  \"chainid\" : \"<chainid>\",  string) A bitcoinhash of the master public key\n"
                            "  \"creationtime\" : The creation time in seconds since epoch (midnight Jan 1 1970 GMT).\n"
                            "  \"chainpath\" : \"<keyschainpath>\",  string) The chainpath (like m/44'/0'/0'/c)\n"
                            "}\n"
                            "\nExamples:\n"
                            + HelpExampleCli("hdgetinfo", "")
                            + HelpExampleCli("hdgetinfo", "True")
                            + HelpExampleRpc("hdgetinfo", "")
                            );

    Wallet *wallet = WalletFromParams(params);

    std::vector<HDChainID> chainIDs;
    if (!wallet->GetAvailableChainIDs(chainIDs))
        throw std::runtime_error("Could not load chain ids");

    HDChainID activeChain;
    wallet->HDGetActiveChainID(activeChain);

    UniValue result(UniValue::VOBJ);
    UniValue chains(UniValue::VARR);
    BOOST_FOREACH(const HDChainID& chainId, chainIDs)
    {
        CHDChain chain;
        if (!wallet->GetChain(chainId, chain))
            throw std::runtime_error("Could not load chain");

        UniValue chainObject(UniValue::VOBJ);
        chainObject.push_back(Pair("chainid", chainId.GetHex()));
        chainObject.push_back(Pair("creationtime", chain.nCreateTime));
        chainObject.push_back(Pair("chainpath", chain.chainPath));


        chains.push_back(chainObject);
    }

    result.push_back(Pair("availablechains", chains));
    result.push_back(Pair("activechainid", activeChain.GetHex()));
    return result;
}

UniValue hdgetaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
                            "hdgetaddress (<childindex>)\n"
                            "\nReturns a Bitcoin address for receiving payments.\n"
                            "\nAutomatically uses the next available childindex if no index is given"
                            "\nArguments:\n"
                            "1. \"childindex\"        (numeric, optional) child key index. ATTENTION: automatic index counting will start at the highes available child key index\n"
                            "{\n"
                            "  \"address\" : \"<address>\",  string) The new bitcoin address\n"
                            "  \"chainpath\" : \"<keyschainpath>\",  string) The used chainpath\n"
                            "}\n"
                            "\nExamples:\n"
                            + HelpExampleCli("hdgetaddress", "")
                            + HelpExampleCli("hdgetaddress", "100")
                            + HelpExampleRpc("hdgetaddress", "")
                            );

    Wallet *wallet = WalletFromParams(params);
    UniValue childIndex = ValueFromParams(params, "index");

    CPubKey newKey;
    std::string keyChainPath;
    if (!childIndex.isNull())
    {
        if (childIndex.isStr())
        {
            childIndex.setNumStr(childIndex.get_str());
        }

        HDChainID emptyId;
        if (!wallet->HDGetChildPubKeyAtIndex(emptyId, newKey, keyChainPath, childIndex.get_int()))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Can't generate HD child key");
    }
    else
    {
        HDChainID emptyId;
        if (!wallet->HDGetNextChildPubKey(emptyId, newKey, keyChainPath))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Can't generate HD child key");
    }
    CKeyID keyID = newKey.GetID();
    
    wallet->SetAddressBook(keyID, "receive");
    
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("address", CBitcoinAddress(keyID).ToString()));
    result.push_back(Pair("chainpath", keyChainPath));
    return result;
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