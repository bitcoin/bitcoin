// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet.h"

#include "base58.h"
#include "core_io.h"
#include "pubkey.h"
#include "rpcserver.h"
#include "utilstrencodings.h"
#include "utilmoneystr.h"

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
UniValue listtransactions(const UniValue& params, bool fHelp);
UniValue listunspent(const UniValue& params, bool fHelp);
UniValue createtx(const UniValue& params, bool fHelp);
UniValue getbalance(const UniValue& params, bool fHelp);
    
static const RPCDispatchEntry vDispatchEntries[] = {
    { "getnewaddress",                    &getnewaddress },
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
UniValue ValueFromParams(const UniValue& params, const std::string& key, UniValue::VType forceType = UniValue::VNULL)
{
    if (params.size() > 0 && params[0].isObject())
    {
        const UniValue& o = params[0].get_obj();
        UniValue val = find_value(o, key);
        if (forceType == UniValue::VNUM && val.isStr())
            val.setNumStr(val.get_str()); //convert str to int
        if (forceType == UniValue::VBOOL)
        {
            if (val.isStr() && ( val.get_str() == "true" || val.get_str() == "1" ))
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
                            "  \"bitcoinaddress\"  (string) a bitcoin address\n"
                            "  ,...\n"
                            "]\n"
                            "\nExamples:\n"
                            + HelpExampleCli("listaddresses", "")
                            + HelpExampleRpc("listaddresses", "")
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

    std::string walletID = walletIDValue.get_str();
    CoreWallet::GetManager()->AddNewWallet(walletID);
    return NullUniValue;
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
    
    wallet->SetAddressBook(keyID, "receive");
    
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
    CAmount nDebit = wtx.GetDebit(filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wtx.IsFromMe(filter) ? wtx.GetValueOut() - nDebit : 0);

    entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    if (wtx.IsFromMe(filter))
        entry.push_back(Pair("fee", ValueFromAmount(nFee)));



    BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item, wtx.mapValue)
    entry.push_back(Pair(item.first, item.second));
}

UniValue listtransactions(const UniValue& params, bool fHelp)
{

    Wallet *wallet = WalletFromParams(params);
    
    LOCK(wallet->cs_coreWallet);
    Wallet::TxItems txOrdered = wallet->OrderedTxItems();

    UniValue result(UniValue::VARR);

    // iterate backwards until we have nCount items to return:
    for (Wallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        WalletTx *const pwtx = (*it).second.first;
        UniValue entry(UniValue::VOBJ);
        WalletTxToJSON(wallet, *pwtx, entry);
        result.push_back(entry);
    }

    return result;
}

UniValue listunspent(const UniValue& params, bool fHelp)
{
    Wallet *wallet = WalletFromParams(params);

    if (fHelp)
        throw std::runtime_error(
                            "listunspent ( minconf maxconf  [\"address\",...] )\n"
                            "\nReturns array of unspent transaction outputs\n"
                            "with between minconf and maxconf (inclusive) confirmations.\n"
                            "Optionally filter to only include txouts paid to specified addresses.\n"
                            "Results are an array of Objects, each of which has:\n"
                            "{txid, vout, scriptPubKey, amount, confirmations}\n"
                            "\nArguments:\n"
                            "1. minconf          (numeric, optional, default=1) The minimum confirmations to filter\n"
                            "2. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter\n"
                            "3. \"addresses\"    (string) A json array of bitcoin addresses to filter\n"
                            "    [\n"
                            "      \"address\"   (string) bitcoin address\n"
                            "      ,...\n"
                            "    ]\n"
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

                            "\nExamples\n"
                            + HelpExampleCli("listunspent", "")
                            + HelpExampleCli("listunspent", "6 9999999 \"[\\\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\\\",\\\"1LtvqCaApEdUGFkpKMM4MstjcaL4dKg8SP\\\"]\"")
                            + HelpExampleRpc("listunspent", "6, 9999999 \"[\\\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\\\",\\\"1LtvqCaApEdUGFkpKMM4MstjcaL4dKg8SP\\\"]\"")
                            );

    //RPCTypeCheck(params, boost::assign::list_of(UniValue::VNUM)(UniValue::VNUM)(UniValue::VARR));

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
    if (fHelp)
        throw std::runtime_error(
                            "getbalance type=(all|available|unconfirmed|immature) includewatchonly=true\n"
                            "\nIf account is not specified, returns the server's total available balance.\n"
                            "If account is specified (DEPRECATED), returns the balance in the account.\n"
                            "Note that the account \"\" is not the same as leaving the parameter out.\n"
                            "The server total may be different to the balance in the default \"\" account.\n"
                            "\nArguments:\n"
                            "1. \"type\"      (string, optional) define the desired balance type \"\".\n"
                            "1. includewatchonly (bool, optional, default=false) Also include balance in watchonly addresses (see 'importaddress')\n"
                            "\nResult:\n"
                            "amount              (numeric) The total amount in btc received for this account (or a object in case of type=all).\n"
                            "\nExamples:\n"
                            "\nThe total amount in the wallet\n"
                            + HelpExampleCli("getbalance", "") +
                            "\nThe total amount in the wallet at least 5 blocks confirmed\n"
                            + HelpExampleCli("getbalance", "type=all includewatchonly=true") +
                            "\nAs a json rpc call\n"
                            + HelpExampleRpc("getbalance", "type=all")
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
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
                                 "createtx sendto=([{address0:amount}, {address1:amount}]) send=(1|0)\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("createtx", "{\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\":10.0}")
                                 + HelpExampleRpc("createtx", "")
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