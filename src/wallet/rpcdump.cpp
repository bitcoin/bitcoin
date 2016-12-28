// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "chain.h"
#include "rpcserver.h"
#include "init.h"
#include "main.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"
#include "util.h"
#include "utiltime.h"
#include "wallet.h"

#include <fstream>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <univalue.h>

#include <boost/foreach.hpp>

using namespace std;

void EnsureWalletIsUnlocked();
bool EnsureWalletIsAvailable(bool avoidException);

std::string static EncodeDumpTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime);
}

int64_t static DecodeDumpTime(const std::string &str) {
    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
    static const std::locale loc(std::locale::classic(),
        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
    std::istringstream iss(str);
    iss.imbue(loc);
    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
    iss >> ptime;
    if (ptime.is_not_a_date_time())
        return 0;
    return (ptime - epoch).total_seconds();
}

std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    BOOST_FOREACH(unsigned char c, str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
        } else {
            ret << c;
        }
    }
    return ret.str();
}

std::string DecodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        unsigned char c = str[pos];
        if (c == '%' && pos+2 < str.length()) {
            c = (((str[pos+1]>>6)*9+((str[pos+1]-'0')&15)) << 4) | 
                ((str[pos+2]>>6)*9+((str[pos+2]-'0')&15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

UniValue importprivkey(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;
    
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importprivkey \"bitcoinprivkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"bitcoinprivkey\"   (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key with rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label and without rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false")
        );


    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    if (fRescan && fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CPubKey pubkey = key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));
    CKeyID vchAddress = pubkey.GetID();
    {
        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBook(vchAddress, strLabel, "receive");

        // Don't throw error in case a key is already there
        if (pwalletMain->HaveKey(vchAddress))
            return NullUniValue;

        pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

        if (!pwalletMain->AddKeyPubKey(key, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        // whenever a key is imported, we need to scan the whole chain
        pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'

        if (fRescan) {
            pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true);
        }
    }

    return NullUniValue;
}

UniValue deleteprivkey(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "deleteprivkey \"pubkey\"\n"
            "\nRemove the private key corresponding to the provided pubkey, making that address become watch-only (as if it was imported with importaddress). For security reasons the wallet must be unlocked.\n"
            "\nArguments:\n"
            "1. \"pubkey\"        (string, required) The pubkey corresponding to the private key to be removed.\n"
            "\nExamples:\n"
            "\nConvert a spendable address to watch-only\n"
            + HelpExampleCli("deleteprivkey", "\"mypubkey\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("deleteprivkey", "\"mypubkey\"")
        );

    std::vector<unsigned char> data(ParseHex(params[0].get_str()));
    CPubKey pubKey(data.begin(), data.end());
    if (!pubKey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");
    CKeyID keyID = pubKey.GetID();
    CScript script = GetScriptForDestination(keyID);

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Wallet must be unlocked not because we need access to key material, but
    // because otherwise some joker that finds an unattended Bitcoin Core could
    // delete the keys.
    EnsureWalletIsUnlocked();

    if (!pwalletMain->HaveKey(keyID))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not in the wallet");

    // Erase the key material
    pwalletMain->EraseKey(pubKey);

    // Re-add the pubkey to the wallet as a watch-only address
    pwalletMain->AddWatchOnly(script);

    return NullUniValue;
}

void ImportAddress(const CBitcoinAddress& address, const string& strLabel);
void ImportScript(const CScript& script, const string& strLabel, bool isRedeemScript)
{
    if (!isRedeemScript && ::IsMine(*pwalletMain, script) == ISMINE_SPENDABLE)
        throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");

    pwalletMain->MarkDirty();

    if (!pwalletMain->HaveWatchOnly(script) && !pwalletMain->AddWatchOnly(script))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");

    if (isRedeemScript) {
        if (!pwalletMain->HaveCScript(script) && !pwalletMain->AddCScript(script))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding p2sh redeemScript to wallet");
        ImportAddress(CBitcoinAddress(CScriptID(script)), strLabel);
    }
}

void ImportAddress(const CBitcoinAddress& address, const string& strLabel)
{
    CScript script = GetScriptForDestination(address.Get());
    ImportScript(script, strLabel, false);
    // add to address book or update label
    if (address.IsValid())
        pwalletMain->SetAddressBook(address.Get(), strLabel, "receive");
}

UniValue importaddress(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;
    
    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "importaddress \"address\" ( \"label\" rescan p2sh )\n"
            "\nAdds a script (in hex) or address that can be watched as if it were in your wallet but cannot be used to spend.\n"
            "\nArguments:\n"
            "1. \"script\"           (string, required) The hex-encoded script (or address)\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "4. p2sh                 (boolean, optional, default=false) Add the P2SH version of the script as well\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "If you have the full public key, you should call importpublickey instead of this.\n"
            "\nExamples:\n"
            "\nImport a script with rescan\n"
            + HelpExampleCli("importaddress", "\"myscript\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importaddress", "\"myscript\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importaddress", "\"myscript\", \"testing\", false")
        );


    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    if (fRescan && fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

    // Whether to import a p2sh version, too
    bool fP2SH = false;
    if (params.size() > 3)
        fP2SH = params[3].get_bool();

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBitcoinAddress address(params[0].get_str());
    if (address.IsValid()) {
        if (fP2SH)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Cannot use the p2sh flag with an address - use a script instead");
        ImportAddress(address, strLabel);
    } else if (IsHex(params[0].get_str())) {
        std::vector<unsigned char> data(ParseHex(params[0].get_str()));
        ImportScript(CScript(data.begin(), data.end()), strLabel, fP2SH);
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address or script");
    }

    if (fRescan)
    {
        pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true);
        pwalletMain->ReacceptWalletTransactions();
    }

    return NullUniValue;
}

void RemoveAddress(const CBitcoinAddress& address);
void RemoveScript(const CScript& script, bool isRedeemScript)
{
    pwalletMain->MarkDirty();

    if (pwalletMain->HaveWatchOnly(script) && !pwalletMain->RemoveWatchOnly(script))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error removing address from wallet");

    if (isRedeemScript) {
        //if (pwalletMain->HaveCScript(script) && !pwalletMain->RemoveCScript(script))
        //    throw JSONRPCError(RPC_WALLET_ERROR, "Error removing p2sh redeemScript to wallet");
        RemoveAddress(CBitcoinAddress(CScriptID(script)));
    }
}

void RemoveAddress(const CBitcoinAddress& address)
{
    CScript script = GetScriptForDestination(address.Get());
    RemoveScript(script, false);
    // add to address book or update label
    if (address.IsValid())
        pwalletMain->DelAddressBook(address.Get());
}

UniValue forgetaddress(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "forgetaddress \"address\" ( purge p2sh )\n"
            "\nRemoves a script (in hex) or address from the wallet and (optionally) purges transactions which as a result are no longer considered as belonging to the wallet.\n"
            "\nArguments:\n"
            "1. \"address\"          (string, required) The hex-encoded script (or address)\n"
            "2. purge              (boolean, optional, default=true) Remove transactions whose last remaining link to the wallet was this script/address\n"
            "3. p2sh               (boolean, optional, default=false) Remove the P2SH version of the script as well\n"
            "\nNote: This call only works on watch-only scripts or addresses. To forget a private key known to the wallet, first call deleteprivkey.\n"
            "\nNote: This call will take two full passes through the wallet and compact the wallet database if purge is true.\n"
            "\nExamples:\n"
            "\nRemove a script with purge\n"
            + HelpExampleCli("forgetaddress", "\"myscript\"") +
            "\nRemove an address without purge\n"
            + HelpExampleCli("forgetaddress", "\"myaddress\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("forgetaddress", "\"myaddress\", false")
        );

    // Whether to perform purge after removal
    bool fPurge = true;
    if (params.size() > 1)
        fPurge = params[1].get_bool();

    // Whether to remove a p2sh version, too
    bool fP2SH = false;
    if (params.size() > 2)
        fP2SH = params[2].get_bool();

    LOCK2(cs_main, pwalletMain->cs_wallet);

    set<uint256> setNotMine;

    if (fPurge)
    {
        for (map<uint256, CWalletTx>::const_iterator i = pwalletMain->mapWallet.begin(); i != pwalletMain->mapWallet.end(); ++i)
        {
            if (!pwalletMain->IsMine(i->second)
             && !pwalletMain->IsFromMe(i->second))
                setNotMine.insert(i->first);
        }
    }

    CBitcoinAddress address(params[0].get_str());
    if (address.IsValid()) {
        if (fP2SH)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Cannot use the p2sh flag with an address - use a script instead");
        CKeyID keyID;
        CPubKey vchPubKey;
        if (address.GetKeyID(keyID) && pwalletMain->GetPubKey(keyID, vchPubKey))
        {
            //throw JSONRPCError(RPC_WALLET_HAVE_KEY, "Refusing to forget address associated with a known key. Make sure you know what you are doing, then call deleteprivkey first.");
            UniValue args(UniValue::VARR);
            args.push_back(HexStr(vchPubKey));
            deleteprivkey(args, false);
        }
        RemoveAddress(address);
    } else if (IsHex(params[0].get_str())) {
        std::vector<unsigned char> data(ParseHex(params[0].get_str()));
        RemoveScript(CScript(data.begin(), data.end()), fP2SH);
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address or script");
    }

    vector<uint256> vDeletes;

    if (fPurge)
    {
        // Sort the array of transaction hashes so that the order of operations
        // and output are deterministic, for testing purposes.
        sort(vDeletes.begin(), vDeletes.end());

        for (map<uint256, CWalletTx>::const_iterator i = pwalletMain->mapWallet.begin(); i != pwalletMain->mapWallet.end(); ++i)
        {
            if (!pwalletMain->IsMine(i->second)
             && !pwalletMain->IsFromMe(i->second)
             && !setNotMine.count(i->first))
            {
                vDeletes.push_back(i->first);
            }
        }
    }

    if (!vDeletes.empty())
    {
        CWalletDB walletdb(pwalletMain->strWalletFile);
        for (vector<uint256>::const_iterator pTxID = vDeletes.begin(); pTxID != vDeletes.end(); ++pTxID)
            pwalletMain->EraseFromWallet(*pTxID, &walletdb);
        walletdb.Flush();
        walletdb.Compact();
    }

    if (fPurge)
    {
        UniValue ret(UniValue::VARR);
        BOOST_FOREACH(const uint256& hash, vDeletes)
            ret.push_back(hash.ToString());
        return ret;
    }

    return NullUniValue;
}

UniValue purgetransactions(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() > 1)
        throw runtime_error(
            "purgetransactions compact\n"
            "\nRemoves transactions from the wallet which are not known to correspond to any keys or watch-only scripts in to the wallet. It is not normally the case that a wallet ends up with transactions unrelated to its keys, but this can happen if forgetaddress is called with purge=false. A common use case is to forget a large number of transactions and then purge them all in one pass.\n"
            "\nArguments:\n"
            "1. compact        (boolean, optional, default=true) Compact the wallet database if any transactions are removed\n"
            "\nNote: This call will take a single full passes through the wallet and then compact the wallet database if and only if transactions are removed and compact=true.\n"
            "\nExamples:\n"
            "\nRemove unlinked transactions from the wallet\n"
            + HelpExampleCli("purgetransactions", "") +
            "\nRemove unlinked transactions without compacting the wallet\n"
            + HelpExampleCli("purgetransactions", "false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("purgetransactions", "false")
        );

    // Whether to perform compaction after purge
    bool fCompact = true;
    if (params.size() > 0)
        fCompact = params[0].get_bool();

    LOCK2(cs_main, pwalletMain->cs_wallet);

    vector<uint256> vDeletes;

    for (map<uint256, CWalletTx>::const_iterator i = pwalletMain->mapWallet.begin(); i != pwalletMain->mapWallet.end(); ++i)
    {
        if (!pwalletMain->IsMine(i->second)
         && !pwalletMain->IsFromMe(i->second))
        {
            vDeletes.push_back(i->first);
        }
    }

    if (!vDeletes.empty())
    {
        // Sort the array of transaction hashes so that the order of operations
        // and output are deterministic, for testing purposes.
        sort(vDeletes.begin(), vDeletes.end());

        // Open the wallet database before entering the loop..
        CWalletDB walletdb(pwalletMain->strWalletFile);

        // Remove each transaction.
        for (vector<uint256>::const_iterator pTxID = vDeletes.begin(); pTxID != vDeletes.end(); ++pTxID)
            pwalletMain->EraseFromWallet(*pTxID, &walletdb);

        // Write to the database and the perform a compaction operation --
        // hopefully shrinking the size of the database files to reduce the
        // size of the wallet on disk.
        walletdb.Flush();
        if (fCompact)
            walletdb.Compact();
    }

    UniValue ret(UniValue::VARR);
    BOOST_FOREACH(const uint256& hash, vDeletes)
        ret.push_back(hash.ToString());
    return ret;
}

UniValue importpubkey(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "importpubkey \"pubkey\" ( \"label\" rescan )\n"
            "\nAdds a public key (in hex) that can be watched as if it were in your wallet but cannot be used to spend.\n"
            "\nArguments:\n"
            "1. \"pubkey\"           (string, required) The hex-encoded public key\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nNote: This call can take minutes to complete if rescan is true.\n"
            "\nExamples:\n"
            "\nImport a public key with rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importpubkey", "\"mypubkey\", \"testing\", false")
        );


    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    if (fRescan && fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

    if (!IsHex(params[0].get_str()))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
    std::vector<unsigned char> data(ParseHex(params[0].get_str()));
    CPubKey pubKey(data.begin(), data.end());
    if (!pubKey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");

    LOCK2(cs_main, pwalletMain->cs_wallet);

    ImportAddress(CBitcoinAddress(pubKey.GetID()), strLabel);
    ImportScript(GetScriptForRawPubKey(pubKey), strLabel, false);

    if (fRescan)
    {
        pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true);
        pwalletMain->ReacceptWalletTransactions();
    }

    return NullUniValue;
}


UniValue importwallet(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;
    
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("dumpwallet", "\"test\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"test\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"test\"")
        );

    if (fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Importing wallets is disabled in pruned mode");

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    ifstream file;
    file.open(params[0].get_str().c_str(), std::ios::in | std::ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    int64_t nTimeBegin = chainActive.Tip()->GetBlockTime();

    bool fGood = true;

    int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
    file.seekg(0, file.beg);

    pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
    while (file.good()) {
        pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
        std::string line;
        std::getline(file, line);
        if (line.empty() || line[0] == '#')
            continue;

        std::vector<std::string> vstr;
        boost::split(vstr, line, boost::is_any_of(" "));
        if (vstr.size() < 2)
            continue;
        CBitcoinSecret vchSecret;
        if (!vchSecret.SetString(vstr[0]))
            continue;
        CKey key = vchSecret.GetKey();
        CPubKey pubkey = key.GetPubKey();
        assert(key.VerifyPubKey(pubkey));
        CKeyID keyid = pubkey.GetID();
        if (pwalletMain->HaveKey(keyid)) {
            LogPrintf("Skipping import of %s (key already present)\n", CBitcoinAddress(keyid).ToString());
            continue;
        }
        int64_t nTime = DecodeDumpTime(vstr[1]);
        std::string strLabel;
        bool fLabel = true;
        for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
            if (boost::algorithm::starts_with(vstr[nStr], "#"))
                break;
            if (vstr[nStr] == "change=1")
                fLabel = false;
            if (vstr[nStr] == "reserve=1")
                fLabel = false;
            if (boost::algorithm::starts_with(vstr[nStr], "label=")) {
                strLabel = DecodeDumpString(vstr[nStr].substr(6));
                fLabel = true;
            }
        }
        LogPrintf("Importing %s...\n", CBitcoinAddress(keyid).ToString());
        if (!pwalletMain->AddKeyPubKey(key, pubkey)) {
            fGood = false;
            continue;
        }
        pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
        if (fLabel)
            pwalletMain->SetAddressBook(keyid, strLabel, "receive");
        nTimeBegin = std::min(nTimeBegin, nTime);
    }
    file.close();
    pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

    CBlockIndex *pindex = chainActive.Tip();
    while (pindex && pindex->pprev && pindex->GetBlockTime() > nTimeBegin - 7200)
        pindex = pindex->pprev;

    if (!pwalletMain->nTimeFirstKey || nTimeBegin < pwalletMain->nTimeFirstKey)
        pwalletMain->nTimeFirstKey = nTimeBegin;

    LogPrintf("Rescanning last %i blocks\n", chainActive.Height() - pindex->nHeight + 1);
    pwalletMain->ScanForWalletTransactions(pindex);
    pwalletMain->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");

    return NullUniValue;
}

UniValue dumpprivkey(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;
    
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey \"bitcoinaddress\"\n"
            "\nReveals the private key corresponding to 'bitcoinaddress'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"bitcoinaddress\"   (string, required) The bitcoin address for the private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CKey vchSecret;
    if (!pwalletMain->GetKey(keyID, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret).ToString();
}


UniValue dumpwallet(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;
    
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpwallet", "\"test\"")
            + HelpExampleRpc("dumpwallet", "\"test\"")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    ofstream file;
    file.open(params[0].get_str().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    pwalletMain->GetAllReserveKeys(setKeyPool);

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by Bitcoin %s (%s)\n", CLIENT_BUILD, CLIENT_DATE);
    file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", EncodeDumpTime(chainActive.Tip()->GetBlockTime()));
    file << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = CBitcoinAddress(keyid).ToString();
        CKey key;
        if (pwalletMain->GetKey(keyid, key)) {
            if (pwalletMain->mapAddressBook.count(keyid)) {
                file << strprintf("%s %s label=%s # addr=%s\n", CBitcoinSecret(key).ToString(), strTime, EncodeDumpString(pwalletMain->mapAddressBook[keyid].name), strAddr);
            } else if (setKeyPool.count(keyid)) {
                file << strprintf("%s %s reserve=1 # addr=%s\n", CBitcoinSecret(key).ToString(), strTime, strAddr);
            } else {
                file << strprintf("%s %s change=1 # addr=%s\n", CBitcoinSecret(key).ToString(), strTime, strAddr);
            }
        }
    }
    file << "\n";
    file << "# End of dump\n";
    file.close();
    return NullUniValue;
}
