// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet.h"

#include "base58.h"
#include "chainparamsbase.h"
#include "core_io.h"
#include "pubkey.h"
#include "rpcserver.h"
#include "utilstrencodings.h"
#include "utilmoneystr.h"

#include <list>
#include <algorithm>
#include <string>

#include "univalue/univalue.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

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

enum ParamVTypes { VFLEX, VSTR, VNUM, VARR, VOBJ, VBOOL };

struct RPCParamEntry
{
    bool mandatory;
    std::string key;
    enum ParamVTypes vtype;
    std::string helptext;
    std::string defaultvalue;
    std::string example;
};

const char *RPCTypeName(enum ParamVTypes t)
{
    switch (t) {
        case VFLEX: return "not defined";
        case VBOOL: return "bool";
        case VSTR: return "string";
        case VNUM: return "number";
        case VARR: return "array";
        case VOBJ: return "object";

    }
    
    // not reached
    return NULL;
}

std::string HelpExampleCli(const std::string& methodname, const std::string& args)
{
    return "> bitcoin-cli " + methodname + " " + args + "\n";
}

std::string HelpExampleRpc(const std::string& methodname, const std::string& args)
{
    return "> curl --user bitcoinrpc:<password> --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
    "\"method\": \"" + methodname + "\", \"params\": [" + args + "] }' -H 'content-type: text/plain;' http://127.0.0.1:"+boost::lexical_cast<std::string>(BaseParams().RPCPort())+"/corewallet\n";
}

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
UniValue listtransactions(const UniValue& params, bool fHelp);
UniValue listunspent(const UniValue& params, bool fHelp);
UniValue createtx(const UniValue& params, bool fHelp);
UniValue getbalance(const UniValue& params, bool fHelp);
    
static const RPCDispatchEntry vDispatchEntries[] = {
    { "listaddresses",                    &listaddresses },

    { "hdaddchain",                       &hdaddchain },
    { "hdsetchain",                       &hdsetchain },
    { "hdgetinfo",                        &hdgetinfo },
    { "hdgetaddress",                     &hdgetaddress },

    { "listtransactions",                 &listtransactions },
    { "listunspent",                      &listunspent },

    { "createtx",                         &createtx },
    { "getbalance",                       &getbalance },

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
    std::string walletID = "";
    if (params.size() > 0 && params[0].isObject())
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

//Verifies given and expected parameters and form a help string if required
void VerifyParams(const std::string &callname, const UniValue& params, bool forceHelp, RPCParamEntry *allowedParams, unsigned int aPsize, const std::string &helptext) {
    unsigned int i;

    if (params.size() > 0 && (!params[0].isObject()))
        throw RPCHelpException("Corewallet only supports key=value parameters");

    std::string errorPart = "";
    for (i = 0; i < aPsize; i++)
    {
        RPCParamEntry *param = &allowedParams[i];

        bool found = false;
        bool typeCheck = false;
        UniValue::VType hasType = UniValue::VNULL;

        if (params.size() > 0 && params[0].isObject())
        {
            const UniValue& o = params[0].get_obj();
            BOOST_FOREACH(const std::string &key, o.getKeys())
            {
                UniValue val = find_value(o, key);
                if (boost::to_lower_copy(key) == param->key)
                {
                    found = true;
                    hasType = val.getType();
                    if (param->vtype == VSTR && val.isStr())
                        typeCheck = true;
                    if (param->vtype == VNUM)
                    {
                        UniValue testVal(UniValue::VNUM);
                        testVal.setNumStr(val.get_str()); //also allow VSTR if it can be converted to VNUM
                        if (testVal.isNum())
                            typeCheck = true;
                    }
                    if ( (param->vtype == VBOOL && val.isBool() ) || (val.isStr() && (boost::to_lower_copy(val.get_str()) == "true" || val.get_str() == "1")))
                        typeCheck = true;
                    if (param->vtype == VFLEX)
                        typeCheck = true;

                    break;
                }
            }
        }

        if (param->mandatory && !found)
            errorPart += "Parameter "+SanitizeString(param->key)+" not found but is required\n";

        if (found && !typeCheck)
            errorPart += "Parameter "+SanitizeString(param->key)+" has wrong type ('"+uvTypeName(hasType)+"' but should be '"+RPCTypeName(param->vtype)+"')\n";
    }

    //check if we have a unrecognized parameter
    if (params.size() > 0 && params[0].isObject())
    {
        const UniValue& o = params[0].get_obj();
        BOOST_FOREACH(const std::string &key, o.getKeys())
        {
            bool found = false;
            unsigned int i;
            for (i = 0; i < aPsize; i++)
            {
                RPCParamEntry *param = &allowedParams[i];
                if (param->key == boost::to_lower_copy(key))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                errorPart += "Parameter "+SanitizeString(key)+" is unknown\n";
        }
    }

    if (errorPart.size() > 0 || forceHelp)
    {
        std::string paramText = "";
        std::string keylistText = "";
        std::string exampleCLIText = "";
        std::string exampleRPCText = "";

        unsigned int i;
        for (i = 0; i < aPsize; i++)
        {
            if (paramText == "")
                paramText = "Arguments:\n";

            RPCParamEntry *param = &allowedParams[i];
            paramText += param->key+" ("+(param->mandatory ? "required" : "optional")+", "+RPCTypeName(param->vtype)+") "+param->helptext+"\n";

            keylistText += (param->mandatory ? "" : "(") + param->key + (param->mandatory ? "" : ")");
            exampleCLIText += param->key+"="+param->example+" ";
            exampleRPCText += "\""+param->key+"\":"+(param->vtype == VSTR ? "\"" : "")+param->example+(param->vtype == VSTR ? "\"" : "")+" ";
        }
        boost::trim_right(exampleCLIText);
        boost::trim_right(exampleRPCText);

        //generate the huge help string
        throw RPCHelpException(errorPart+"\ncorewallet/"+callname+" "+keylistText+"\n"+boost::replace_all_copy(boost::replace_all_copy(helptext, "%args%", paramText), "%examples%", "Examples:\n"+HelpExampleCli(callname, exampleCLIText)+HelpExampleRpc(callname, "{"+exampleRPCText+"}")));
    }
}

//! search in exiting json object for a key/value pair and returns value
UniValue ValueFromParams(const UniValue& params, const std::string& key, UniValue::VType forceType = UniValue::VNULL)
{
    if (params.size() > 0 && params[0].isObject())
    {
        const UniValue& o = params[0].get_obj();
        UniValue val = find_value(o, key);
        if (forceType == UniValue::VNUM && val.isStr())
        {
            val.setNumStr(val.get_str()); //convert str to int
            if (!val.isNum())
                throw std::runtime_error("Parameter '"+SanitizeString(key)+"' is not numeric");
        }
        if (forceType == UniValue::VBOOL)
        {
            if (val.isStr() && ( boost::to_lower_copy(val.get_str()) == "true" || val.get_str() == "1" ))
                val.setBool(true);
            else
                val.setBool(false);
        }
        if (forceType == UniValue::VOBJ)
            if (val.isStr())
            {
                std::string jsonStr = val.get_str();
                if (!val.read(jsonStr))
                    throw std::runtime_error("Invalid JSON");
            }

        return val;
    }
    else
    {
        return NullUniValue;
    }
}
    
///////////////////////////
// Keys/Addresses stack
///////////////////////////

UniValue listaddresses(const UniValue& params, bool fHelp)
{
    VerifyParams( "listaddresses", params, fHelp, NULL, 0,
                 "\nList all available addresses.\n"
                 "%args%"
                 "\nResult:\n"
                 "[                     (json array of string)\n"
                 "  \"bitcoinaddress\"  (string) a bitcoin address\n"
                 "  ,...\n"
                 "]\n"
                 "\n%examples%"
                 );

    Wallet *wallet = WalletFromParams(params);

    UniValue ret(UniValue::VARR);

    LOCK(wallet->cs_coreWallet);
    std::vector<CKeyID> alreadyAdded;

    BOOST_FOREACH(const PAIRTYPE(CKeyID, CKeyMetadata)& item, wallet->mapKeyMetadata)
    {
        const CKeyID& keyID = item.first;
        const CBitcoinAddress& address = CBitcoinAddress(keyID);

        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("address", address.ToString()));

        std::map<CTxDestination, CAddressBookMetadata>::const_iterator miAddr = wallet->mapAddressBook.find(keyID);
        if (miAddr != wallet->mapAddressBook.end())
        {
            obj.push_back(Pair("addressbook", true));
            obj.push_back(Pair("purpose", miAddr->second.purpose));
        }
        else
            obj.push_back(Pair("addressbook", false));

        CHDPubKey hdPubKey;
        if (wallet->GetHDPubKey(keyID, hdPubKey))
        {
            obj.push_back(Pair("hd", true));
            obj.push_back(Pair("chainpath", hdPubKey.chainPath));
        }
        else
            obj.push_back(Pair("hd", false));

        std::map<CKeyID, CKeyMetadata>::const_iterator miKMeta = wallet->mapKeyMetadata.find(keyID);
        if (miKMeta != wallet->mapKeyMetadata.end())
        {
            obj.push_back(Pair("createtime", miKMeta->second.nCreateTime));
            obj.push_back(Pair("keyflags", miKMeta->second.KeyflagsAsString()));
        }
        else
            obj.push_back(Pair("missing_key_metadata", true));


        ret.push_back(obj);
        alreadyAdded.push_back(keyID);
    }
    return ret;
}

    
///////////////////////////
// MultiWallet stack
///////////////////////////
UniValue addwallet(const UniValue& params, bool fHelp)
{
    static RPCParamEntry allowedParams[] = {
        { true, "walletid", VSTR, "allowed characters: A-Za-z0-9._-", "", "testwallet"}
    };

    VerifyParams( "addwallet", params, fHelp, allowedParams, 1,
                  "\nCreates a new wallet with given walletid.\n"
                  "%args%"
                  "\n%examples%"
                  );

    UniValue walletIDValue = ValueFromParams(params, "walletid");

    std::string walletID = walletIDValue.get_str();
    CoreWallet::GetManager()->AddNewWallet(walletID);
    return NullUniValue;
}

UniValue listwallets(const UniValue& params, bool fHelp)
{
    VerifyParams( "listwallets", params, fHelp, NULL, 0,
                  "\nList all available wallets.\n"
                  "%args%"
                  "\nResult:\n"
                  "[\n"
                  "  {\n"
                  "      \"walletid\": \"<walletname>\"      (string) a wallet identifier\n"
                  "  }\n"
                  "]\n"
                  "\n%examples%"
                  );
    
    UniValue ret(UniValue::VARR);
    std::vector<std::string> vWalletIDs = CoreWallet::GetManager()->GetWalletIDs();
    BOOST_FOREACH(const std::string& walletID, vWalletIDs)
    {
        UniValue wObj(UniValue::VOBJ);
        wObj.push_back(Pair("wallteid", walletID));
        ret.push_back(wObj);
    }
    
    return ret;
}

///////////////////////////
// Help stack
///////////////////////////
UniValue help(const UniValue& params, bool fHelp)
{
    static RPCParamEntry allowedParams[] = {
        { true, "command", VSTR, "The command to get help on", "", "corewallet/listwallets"}
    };
    VerifyParams( "help", params, fHelp, allowedParams, 1,
                 "\nList all commands, or get help for a specified command.\n"
                 "%args%"
                 "\nResult:\n"
                 "\"text\"     (string) The help text\n"
                 "\n%examples%"
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
    static RPCParamEntry allowedParams[] = {
        { false, "chainpath", VSTR, "chainpath for hd wallet structure\nm stands for master, c for internal/external key-switch, k stands for upcounting child key index", "m/44h/0h/0h/c/k", "m/32h/c/k"},
        { false, "seed", VSTR, "use this seed for master key generation", "", "abcdef..."},
    };
    VerifyParams("hdaddchain", params, fHelp, allowedParams, 2,
                 "\nList all commands, or get help for a specified command.\n"
                 "%args%"
                 "\nResult\n"
                 "{\n"
                 "  \"seed_hex\" : \"<hexstr>\",  (string) seed used during master key generation (only if no masterseed hex was provided\n"
                 "}\n"
                 "\n%examples%"
                 );

    UniValue result(UniValue::VOBJ);
    Wallet *wallet = WalletFromParams(params);
    UniValue chainpathParam = ValueFromParams(params, "chainpath");
    UniValue seedParam = ValueFromParams(params, "seed");

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
    static RPCParamEntry allowedParams[] = {
        { true, "chainid", VSTR, "chainid is a bitcoin hash of the master public key of the corresponding chain.", "", "e2e658234be33a32ab60831d44190d397497aad0f45e6a2d59f69c3a34d4d9b6"}
    };
    VerifyParams("hdsetchain", params, fHelp, allowedParams, 1,
                 "\nSet the active chains of key.\n"
                 "%args%"
                 "\n%examples%"
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
    VerifyParams("hdgetinfo", params, fHelp, NULL, 0,
                 "\nReturns some hd relevant information.\n"
                 "%args%"
                 "\nResult\n"
                 "{\n"
                 "  \"chainid\": \"<chainid>\",           (string) A bitcoinhash of the master public key\n"
                 "  \"creationtime\": <timestamp>,        (numeric) The creation time in seconds since epoch (midnight Jan 1 1970 GMT).\n"
                 "  \"chainpath\": \"<keyschainpath>\",   (string) The chainpath (like m/44'/0'/0'/c)\n"
                 "}\n"
                 "\n%examples%"
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
    static RPCParamEntry allowedParams[] = {
        { false, "index", VNUM, "child key index. ATTENTION: automatic index counting will start at the highes available child key index", "", "100"},
        { false, "internal", VBOOL, "if true, the internal chain will be used for the new key", "", "false"}
    };
    VerifyParams("hdgetaddress", params, fHelp, allowedParams, 2,
                 "\nReturns a Bitcoin address for receiving payments.\n"
                 "Automatically uses the next available childindex if no index is given\n"
                 "\n%args%"
                 "\nResult\n"
                 "{\n"
                 "  \"address\" : \"<address>\",          (string) the new bitcoin address\n"
                 "  \"chainpath\" : \"<keyschainpath>\",  (string) used chainpath\n"
                 "}\n"
                 "\n%examples%"
                 );

    Wallet *wallet = WalletFromParams(params);
    UniValue childIndex = ValueFromParams(params, "index", UniValue::VNUM);

    CPubKey newKey;
    std::string keyChainPath;
    if (!childIndex.isNull())
    {
        LOCK(wallet->cs_coreWallet);
        HDChainID emptyId;
        if (!wallet->HDGetChildPubKeyAtIndex(emptyId, newKey, keyChainPath, childIndex.get_int()))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Can't generate HD child key");
    }
    else
    {
        LOCK(wallet->cs_coreWallet);
        HDChainID emptyId;
        if (!wallet->HDGetNextChildPubKey(emptyId, newKey, keyChainPath))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Can't generate HD child key");
    }
    CKeyID keyID = newKey.GetID();
    
    wallet->SetAndStoreAddressBook(keyID, "receive");
    
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("address", CBitcoinAddress(keyID).ToString()));
    result.push_back(Pair("chainpath", keyChainPath));
    return result;
}

void WalletTxToJSON(const Wallet* wallet, const WalletTx& wtx, UniValue& entry)
{
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsCoinBase())
        entry.push_back(Pair("generated", true));
    if (confirms > 0)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
    }
    uint256 hash = wtx.GetHash();
    entry.push_back(Pair("txid", hash.GetHex()));
    UniValue conflicts(UniValue::VARR);
    BOOST_FOREACH(const uint256& conflict, wallet->GetConflicts(wtx.GetHash()))
    conflicts.push_back(conflict.GetHex());
    entry.push_back(Pair("walletconflicts", conflicts));
    entry.push_back(Pair("time", wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (int64_t)wtx.nTimeReceived));

    isminefilter filter = ISMINE_SPENDABLE; //TODO: make filter configurable over params

    CAmount nCredit = wallet->GetCredit(wtx, filter);
    CAmount nDebit = wallet->GetDebit(wtx, filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wallet->IsFromMe(wtx, filter) ? wtx.GetValueOut() - nDebit : 0);

    entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    if (wallet->IsFromMe(wtx, filter))
        entry.push_back(Pair("fee", ValueFromAmount(nFee)));



    BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item, wtx.mapValue)
    entry.push_back(Pair(item.first, item.second));
}

UniValue listtransactions(const UniValue& params, bool fHelp)
{
    Wallet *wallet = WalletFromParams(params);
    
    LOCK(wallet->cs_coreWallet);
    Wallet::WtxItems txOrdered = wallet->OrderedTxItems();

    UniValue result(UniValue::VARR);

    // iterate backwards until we have nCount items to return:
    for (Wallet::WtxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        WalletTx *const pwtx = (*it).second;
        UniValue entry(UniValue::VOBJ);
        WalletTxToJSON(wallet, *pwtx, entry);
        result.push_back(entry);
    }

    return result;
}

UniValue listunspent(const UniValue& params, bool fHelp)
{
    static RPCParamEntry allowedParams[] = {
        { false, "minconf", VNUM, "The minimum confirmations to filter", "1", "2"},
        { false, "maxconf", VNUM, "The maximum confirmations to filter", "9999999", "10"},
        { false, "addresses", VARR, "A json array of bitcoin addresses to filter", "", "[\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\"]"},
    };

    VerifyParams("listunspent", params, fHelp, allowedParams, 3,
                 "\nReturns array of unspent transaction outputs\n"
                 "with between minconf and maxconf (inclusive) confirmations.\n"
                 "Optionally filter to only include txouts paid to specified addresses.\n"
                 "Results are an array of Objects, each of which has:\n"
                 "{txid, vout, scriptPubKey, amount, confirmations}\n"
                 "%args%"
                 "\nResult\n"
                 "[                   (array of json object)\n"
                 "  {\n"
                 "    \"txid\" : \"txid\",        (string) the transaction id \n"
                 "    \"vout\" : n,               (numeric) the vout value\n"
                 "    \"address\" : \"address\",  (string) the bitcoin address\n"
                 "    \"account\" : \"account\",  (string) DEPRECATED. The associated account, or \"\" for the default account\n"
                 "    \"scriptPubKey\" : \"key\", (string) the script key\n"
                 "    \"amount\" : x.xxx,         (numeric) the transaction amount in btc\n"
                 "    \"confirmations\" : n       (numeric) The number of confirmations\n"
                 "  }\n"
                 "  ,...\n"
                 "]\n"
                 "\n%examples%"
                 );

    Wallet *wallet = WalletFromParams(params);

    int nMinDepth = 1;
    UniValue minconf = ValueFromParams(params, "minconf", UniValue::VNUM);
    if (!minconf.isNull())
        nMinDepth = minconf.get_int();

    UniValue maxconf = ValueFromParams(params, "maxconf", UniValue::VNUM);
    int nMaxDepth = 9999999;
    if (!maxconf.isNull())
        nMaxDepth = maxconf.get_int();

    UniValue addresses = ValueFromParams(params, "addresses");
    std::set<CBitcoinAddress> setAddress;
    if (!addresses.isNull())
    {
        UniValue inputs = addresses.get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CBitcoinAddress address(input.get_str());
            if (!address.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address: "+input.get_str());
            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, duplicated address: "+input.get_str());
            setAddress.insert(address);
        }
    }

    UniValue results(UniValue::VARR);
    std::vector<COutput> vecOutputs;

    LOCK(wallet->cs_coreWallet);
    wallet->AvailableCoins(vecOutputs, false, NULL, true);
    BOOST_FOREACH(const COutput& out, vecOutputs) {
        if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
            continue;

        if (setAddress.size()) {
            CTxDestination address;
            if (!ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
                continue;

            if (!setAddress.count(address))
                continue;
        }

        CAmount nValue = out.tx->vout[out.i].nValue;
        const CScript& pk = out.tx->vout[out.i].scriptPubKey;
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("txid", out.tx->GetHash().GetHex()));
        entry.push_back(Pair("vout", out.i));
        CTxDestination address;
        if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, address)) {
            entry.push_back(Pair("address", CBitcoinAddress(address).ToString()));
        }
        entry.push_back(Pair("scriptPubKey", HexStr(pk.begin(), pk.end())));
        if (pk.IsPayToScriptHash()) {
            CTxDestination address;
            if (ExtractDestination(pk, address)) {
                const CScriptID& hash = boost::get<CScriptID>(address);
                CScript redeemScript;
                if (wallet->GetCScript(hash, redeemScript))
                    entry.push_back(Pair("redeemScript", HexStr(redeemScript.begin(), redeemScript.end())));
            }
        }
        entry.push_back(Pair("amount",ValueFromAmount(nValue)));
        entry.push_back(Pair("confirmations",out.nDepth));
        entry.push_back(Pair("spendable", out.fSpendable));
        results.push_back(entry);
    }
    
    return results;
}

UniValue getbalance(const UniValue& params, bool fHelp)
{
    static RPCParamEntry allowedParams[] = {
        { false, "type", VSTR, "define the desired balance type (confirmed, unconfirmed, immature, all)", "confirmed", "all"},
        { false, "includewatchonly", VBOOL, "Also include balance in watchonly addresses (see 'importaddress')", "false", "true"},
    };

    VerifyParams("listunspent", params, fHelp, allowedParams, 2,
                 "\nReturns the total available balance.\n"
                 "%args%"
                 "\nResult:\n"
                 "amount              (numeric) The total amount in btc received for this account (or a object in case of type=all).\n"
                 "\n%examples%"
                 );

    Wallet *wallet = WalletFromParams(params);
    LOCK(wallet->cs_coreWallet);

    isminefilter filter = ISMINE_SPENDABLE;
    UniValue includewatchonly = ValueFromParams(params, "includewatchonly", UniValue::VBOOL);
    if (!includewatchonly.isNull() && includewatchonly.isBool() && includewatchonly.isTrue())
        filter = filter | ISMINE_WATCH_ONLY;

    UniValue balanceType = ValueFromParams(params, "type");
    if (!balanceType.isNull() && balanceType.get_str() == "immature")
        return ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_IMMATURE, filter));
    if (!balanceType.isNull() && balanceType.get_str() == "unconfirmed")
        return ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_UNCONFIRMED, filter));
    if (!balanceType.isNull() && balanceType.get_str() == "all")
    {
        UniValue balanceArray = UniValue(UniValue::VOBJ);
        balanceArray.push_back(Pair("available", ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_AVAILABLE, filter))));
        balanceArray.push_back(Pair("unconfirmed", ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_UNCONFIRMED, filter))));
        balanceArray.push_back(Pair("immature", ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_IMMATURE, filter))));
        return balanceArray;
    }
    else
        return ValueFromAmount(wallet->GetBalance(CREDIT_DEBIT_TYPE_AVAILABLE, filter));
}

UniValue createtx(const UniValue& params, bool fHelp)
{
    static RPCParamEntry allowedParams[] = {
        { false, "sendto", VSTR, "object containing n bitcoin/value pairs", "", "{\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\":10.0}"},
        { false, "send", VBOOL, "is set, the transaction will be broadcasted", "false", "true"},
    };

    VerifyParams("createtx", params, fHelp, allowedParams, 2,
                 "\nCreates a new transaction with the options to directly broadcast the transaction (send=true).\n"
                 "%args%"

                 "\nResult\n"
                 "[                   (array of json object)\n"
                 "  {\n"
                 "    \"hex\" : \"hex\",          (string) hex string of the raw transaction\n"
                 "    \"fee\" : n,                (numeric) the fee used in the transaction\n"
                 "    \"sent\" : true|false,      (boolean) will be set to true if transaction has been successfully broadcastet\n"
                 "    \"txid\" : \"txid\",        (optional, string) the transaction id if the transaction has been broadcasted\n"
                 "  }\n"
                 "  ,...\n"
                 "]\n"
                 "\n%examples%"
                 );
    
    Wallet *wallet = WalletFromParams(params);
    std::vector<std::pair<CBitcoinAddress, CAmount> > sendToList;

    UniValue sendToArray = ValueFromParams(params, "sendto", UniValue::VOBJ);
    UniValue sendTx = ValueFromParams(params, "send", UniValue::VBOOL);

    CAmount curBalance = wallet->GetBalance(CREDIT_DEBIT_TYPE_AVAILABLE, ISMINE_SPENDABLE);

    CBitcoinAddress address(sendToArray.getKeys()[0]);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");

    CAmount nValue = AmountFromValue(sendToArray.getValues()[0]);
    // Check amount
    if (nValue <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nValue > curBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    bool shouldSend = false;
    if (!sendTx.isNull() && sendTx.isTrue())
        shouldSend = true;

    // Parse Bitcoin address
    CScript scriptPubKey = GetScriptForDestination(address.Get());

    bool fSubtractFeeFromAmount = false;
    WalletTx wtxNew;

    // Create and send the transaction
    CHDReserveKey reservekey(wallet);
    CAmount nFeeRequired;
    std::string strError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);
    if (!wallet->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strError)) {
        if (!fSubtractFeeFromAmount && nValue + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    if (shouldSend && !wallet->CommitTransaction(wtxNew, reservekey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", EncodeHexTx(wtxNew)));
    result.push_back(Pair("fee", ValueFromAmount(nFeeRequired)));
    result.push_back(Pair("sent", shouldSend));
    if (shouldSend)
        result.push_back(Pair("txid", wtxNew.GetHash().GetHex()));

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