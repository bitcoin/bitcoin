// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "main.h"
#include "sync.h"
#include "wallet.h"

#include <fstream>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

void Bitcredit_EnsureWalletIsUnlocked();
void Deposit_EnsureWalletIsUnlocked();
void Bitcoin_EnsureWalletIsUnlocked();

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

Value bitcredit_importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importprivkey \"bitcreditprivkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"bitcreditprivkey\"   (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional) an optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false")
        );

    Bitcredit_EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CPubKey pubkey = key.GetPubKey();
    CKeyID vchAddress = pubkey.GetID();
    {
        LOCK2(credits_mainState.cs_main, bitcredit_pwalletMain->cs_wallet);

        bitcredit_pwalletMain->MarkDirty();
        bitcredit_pwalletMain->SetAddressBook(vchAddress, strLabel, "receive");

        // Don't throw error in case a key is already there
        if (bitcredit_pwalletMain->HaveKey(vchAddress))
            return Value::null;

        bitcredit_pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

        if (!bitcredit_pwalletMain->AddKeyPubKey(key, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        // whenever a key is imported, we need to scan the whole chain
        bitcredit_pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'

        if (fRescan) {
            bitcredit_pwalletMain->ScanForWalletTransactions(bitcoin_pwalletMain, bitcoin_pclaimCoinsTip, credits_chainActive.Genesis(), true);
        }
    }

    return Value::null;
}

Value bitcoin_importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "bitcoin_importprivkey \"bitcoinprivkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by bitcoin_dumpprivkey) to your bitcoin wallet.\n"
            "\nArguments:\n"
            "1. \"bitcoinprivkey\"   (string, required) The private key (see bitcoin_dumpprivkey)\n"
            "2. \"label\"            (string, optional) an optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the bitcoin wallet for transactions\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("bitcoin_dumpprivkey", "\"myaddress\"") +
            "\nImport the private key\n"
            + HelpExampleCli("bitcoin_importprivkey", "\"mykey\"") +
            "\nImport using a label\n"
            + HelpExampleCli("bitcoin_importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("bitcoin_importprivkey", "\"mykey\", \"testing\", false")
        );

    Bitcoin_EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CPubKey pubkey = key.GetPubKey();
    CKeyID vchAddress = pubkey.GetID();
    {
        LOCK2(bitcoin_mainState.cs_main, bitcoin_pwalletMain->cs_wallet);

        bitcoin_pwalletMain->MarkDirty();
        bitcoin_pwalletMain->SetAddressBook(vchAddress, strLabel, "receive");

        // Don't throw error in case a key is already there
        if (bitcoin_pwalletMain->HaveKey(vchAddress))
            return Value::null;

        bitcoin_pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

        if (!bitcoin_pwalletMain->AddKeyPubKey(key, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        // whenever a key is imported, we need to scan the whole chain
        bitcoin_pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'

        if (fRescan) {
            bitcoin_pwalletMain->ScanForWalletTransactions(bitcoin_chainActive.Genesis(), true);
        }
    }

    return Value::null;
}

Value bitcredit_importwallet(const Array& params, bool fHelp)
{
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

    Bitcredit_EnsureWalletIsUnlocked();

    ifstream file;
    file.open(params[0].get_str().c_str(), std::ios::in | std::ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    int64_t nTimeBegin = credits_chainActive.Tip()->nTime;

    bool fGood = true;

    int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
    file.seekg(0, file.beg);

    bitcredit_pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
    while (file.good()) {
        bitcredit_pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
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
        CKeyID keyid = pubkey.GetID();
        if (bitcredit_pwalletMain->HaveKey(keyid)) {
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
        if (!bitcredit_pwalletMain->AddKeyPubKey(key, pubkey)) {
            fGood = false;
            continue;
        }
        bitcredit_pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
        if (fLabel)
            bitcredit_pwalletMain->SetAddressBook(keyid, strLabel, "receive");
        nTimeBegin = std::min(nTimeBegin, nTime);
    }
    file.close();
    bitcredit_pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

    Credits_CBlockIndex *pindex = (Credits_CBlockIndex*)credits_chainActive.Tip();
    while (pindex && pindex->pprev && pindex->nTime > nTimeBegin - 7200)
        pindex = (Credits_CBlockIndex*)pindex->pprev;

    if (!bitcredit_pwalletMain->nTimeFirstKey || nTimeBegin < bitcredit_pwalletMain->nTimeFirstKey)
        bitcredit_pwalletMain->nTimeFirstKey = nTimeBegin;

    LogPrintf("Rescanning last %i blocks\n", credits_chainActive.Height() - pindex->nHeight + 1);
    bitcredit_pwalletMain->ScanForWalletTransactions(bitcoin_pwalletMain, bitcoin_pclaimCoinsTip, pindex);
    bitcredit_pwalletMain->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");

    return Value::null;
}

Value bitcoin_importwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "bitcoin_importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see bitcoin_dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("bitcoin_dumpwallet", "\"test\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("bitcoin_importwallet", "\"test\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("bitcoin_importwallet", "\"test\"")
        );

    Bitcoin_EnsureWalletIsUnlocked();

    ifstream file;
    file.open(params[0].get_str().c_str(), std::ios::in | std::ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    int64_t nTimeBegin = bitcoin_chainActive.Tip()->nTime;

    bool fGood = true;

    int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
    file.seekg(0, file.beg);

    bitcoin_pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
    while (file.good()) {
        bitcoin_pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
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
        CKeyID keyid = pubkey.GetID();
        if (bitcoin_pwalletMain->HaveKey(keyid)) {
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
        if (!bitcoin_pwalletMain->AddKeyPubKey(key, pubkey)) {
            fGood = false;
            continue;
        }
        bitcoin_pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
        if (fLabel)
            bitcoin_pwalletMain->SetAddressBook(keyid, strLabel, "receive");
        nTimeBegin = std::min(nTimeBegin, nTime);
    }
    file.close();
    bitcoin_pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

    Bitcoin_CBlockIndex *pindex = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
    while (pindex && pindex->pprev && pindex->nTime > nTimeBegin - 7200)
        pindex = (Bitcoin_CBlockIndex*)pindex->pprev;

    if (!bitcoin_pwalletMain->nTimeFirstKey || nTimeBegin < bitcoin_pwalletMain->nTimeFirstKey)
        bitcoin_pwalletMain->nTimeFirstKey = nTimeBegin;

    LogPrintf("Rescanning last %i blocks\n", bitcoin_chainActive.Height() - pindex->nHeight + 1);
    bitcoin_pwalletMain->ScanForWalletTransactions(pindex);
    bitcoin_pwalletMain->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");

    return Value::null;
}

Value bitcredit_dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey \"bitcreditaddress\"\n"
            "\nReveals the private key corresponding to 'bitcreditaddress'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"bitcreditaddress\"   (string, required) The credits address for the private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
        );

    Bitcredit_EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Credits address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CKey vchSecret;
    if (!bitcredit_pwalletMain->GetKey(keyID, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret).ToString();
}

Value bitcoin_dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "bitcoin_dumpprivkey \"bitcoinaddress\"\n"
            "\nReveals the private key corresponding to 'bitcoinaddress'.\n"
            "Then the bitcoin_importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"bitcoinaddress\"   (string, required) The bitcoin address for the private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("bitcoin_dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("bitcoin_importprivkey", "\"mykey\"")
            + HelpExampleRpc("bitcoin_dumpprivkey", "\"myaddress\"")
        );

    Bitcoin_EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CKey vchSecret;
    if (!bitcoin_pwalletMain->GetKey(keyID, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret).ToString();
}


Value bitcredit_dumpwallet(const Array& params, bool fHelp)
{
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

    Bitcredit_EnsureWalletIsUnlocked();

    ofstream file;
    file.open(params[0].get_str().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    bitcredit_pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    bitcredit_pwalletMain->GetAllReserveKeys(setKeyPool);

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by Credits %s (%s)\n", CLIENT_BUILD, CLIENT_DATE);
    file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", credits_chainActive.Height(), credits_chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", EncodeDumpTime(credits_chainActive.Tip()->nTime));
    file << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = CBitcoinAddress(keyid).ToString();
        CKey key;
        if (bitcredit_pwalletMain->GetKey(keyid, key)) {
            if (bitcredit_pwalletMain->mapAddressBook.count(keyid)) {
                file << strprintf("%s %s label=%s # addr=%s\n", CBitcoinSecret(key).ToString(), strTime, EncodeDumpString(bitcredit_pwalletMain->mapAddressBook[keyid].name), strAddr);
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
    return Value::null;
}

Value bitcoin_dumpwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "bitcoin_dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename\n"
            "\nExamples:\n"
            + HelpExampleCli("bitcoin_dumpwallet", "\"test\"")
            + HelpExampleRpc("bitcoin_dumpwallet", "\"test\"")
        );

    Bitcoin_EnsureWalletIsUnlocked();

    ofstream file;
    file.open(params[0].get_str().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    bitcoin_pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    bitcoin_pwalletMain->GetAllReserveKeys(setKeyPool);

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by Credits %s (%s)\n", CLIENT_BUILD, CLIENT_DATE);
    file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", bitcoin_chainActive.Height(), bitcoin_chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", EncodeDumpTime(bitcoin_chainActive.Tip()->nTime));
    file << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = CBitcoinAddress(keyid).ToString();
        CKey key;
        if (bitcoin_pwalletMain->GetKey(keyid, key)) {
            if (bitcoin_pwalletMain->mapAddressBook.count(keyid)) {
                file << strprintf("%s %s label=%s # addr=%s\n", CBitcoinSecret(key).ToString(), strTime, EncodeDumpString(bitcoin_pwalletMain->mapAddressBook[keyid].name), strAddr);
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
    return Value::null;
}
