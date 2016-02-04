// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "walletdb.h"
#include "rpcserver.h"
#include "init.h"
#include "base58.h"
#include "txdb.h"
#include "stealth.h"
#include "ringsig.h"
#include "smessage.h"
#include <sstream>

using namespace json_spirit;
using namespace std;

int64_t nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

static void accountingDeprecationCheck()
{
    if (!GetBoolArg("-enableaccounts", false))
        throw runtime_error(
            "Accounting API is deprecated and will be removed in future.\n"
            "It can easily result in negative or odd balances if misused or misunderstood, which has happened in the field.\n"
            "If you still want to enable it, add to your config file enableaccounts=1\n");

    if (GetBoolArg("-staking", true))
        throw runtime_error("If you want to use accounting API, staking must be disabled, add to your config file staking=0\n");
}

std::string HelpRequiringPassphrase()
{
    return pwalletMain->IsCrypted()
        ? "\nrequires wallet passphrase to be set with walletpassphrase first"
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (fWalletUnlockStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet is unlocked for staking only.");
}

void WalletTxToJSON(const CWalletTx& wtx, Object& entry)
{
    entry.push_back(Pair("version", wtx.nVersion));
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsCoinBase() || wtx.IsCoinStake())
        entry.push_back(Pair("generated", true));

    if (confirms > 0)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));

        int64_t nTime = 0;
        if (nNodeMode == NT_FULL)
        {
            nTime = mapBlockIndex[wtx.hashBlock]->nTime;
        } else
        {
            std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtx.hashBlock);
            if (mi != mapBlockThinIndex.end())
                nTime = (*mi).second->nTime;
        };

        entry.push_back(Pair("blocktime", nTime));
    };

    entry.push_back(Pair("txid", wtx.GetHash().GetHex()));
    entry.push_back(Pair("time", (int64_t)wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (int64_t)wtx.nTimeReceived));

    BOOST_FOREACH(const PAIRTYPE(string,string)& item, wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

string AccountFromValue(const Value& value)
{
    string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}

Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.");

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj, diff;

    obj.push_back(Pair("version",       FormatFullVersion()));
    obj.push_back(Pair("mode",          std::string(GetNodeModeName(nNodeMode))));
    if (nNodeMode == NT_THIN)
        obj.push_back(Pair("state",          std::string(GetNodeStateName(nNodeState))));

    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    obj.push_back(Pair("gameunitsxbalance", ValueFromAmount(pwalletMain->GetGameunitsXBalance())));
    obj.push_back(Pair("newmint",       ValueFromAmount(pwalletMain->GetNewMint())));
    obj.push_back(Pair("stake",         ValueFromAmount(pwalletMain->GetStake())));
    obj.push_back(Pair("reserve",       ValueFromAmount(nReserveBalance)));

    obj.push_back(Pair("blocks",        (int)nBestHeight));
    if (nNodeMode == NT_THIN)
        obj.push_back(Pair("filteredblocks",   (int)nHeightFilteredNeeded));

    obj.push_back(Pair("timeoffset",    (int64_t)GetTimeOffset()));

    if (nNodeMode == NT_FULL)
        obj.push_back(Pair("moneysupply",   ValueFromAmount(pindexBest->nMoneySupply)));


    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("datareceived",  bytesReadable(CNode::GetTotalBytesRecv())));
    obj.push_back(Pair("datasent",      bytesReadable(CNode::GetTotalBytesSent())));


    obj.push_back(Pair("proxy",         (proxy.IsValid() ? proxy.ToStringIPPort() : string())));
    obj.push_back(Pair("ip",            addrSeenByPeer.ToStringIP()));


    if (nNodeMode == NT_FULL)
    {
        diff.push_back(Pair("proof-of-work",  GetDifficulty()));
        diff.push_back(Pair("proof-of-stake", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    } else
    {
        diff.push_back(Pair("proof-of-work",  GetHeaderDifficulty()));
        diff.push_back(Pair("proof-of-stake", GetHeaderDifficulty(GetLastBlockThinIndex(pindexBestHeader, true))));
    };

    obj.push_back(Pair("difficulty",    diff));

    obj.push_back(Pair("testnet",       fTestNet));
    obj.push_back(Pair("keypoololdest", (int64_t)pwalletMain->GetOldestKeyPoolTime()));
    obj.push_back(Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
    obj.push_back(Pair("mininput",      ValueFromAmount(nMinimumInputValue)));
    if (pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", (int64_t)nWalletUnlockTime / 1000));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}


Value getnewpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewpubkey [account]\n"
            "Returns new public key for coinbase generation.");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return HexStr(newKey.begin(), newKey.end());
}


Value getnewaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddress [account]\n"
            "Returns a new Gameunits address for receiving payments.  "
            "If [account] is specified, it is added to the address book "
            "so payments received with the address will be credited to [account].");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return CBitcoinAddress(keyID).ToString();
}


CBitcoinAddress GetAccountAddress(string strAccount, bool bForceNew=false)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid())
    {
        CScript scriptPubKey;
        scriptPubKey.SetDestination(account.vchPubKey.GetID());
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it)
        {
            const CWalletTx& wtx = (*it).second;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed)
    {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey, false))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBookName(account.vchPubKey.GetID(), strAccount);
        walletdb.WriteAccount(strAccount, account);
    }

    return CBitcoinAddress(account.vchPubKey.GetID());
}

Value getaccountaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccountaddress <account>\n"
            "Returns the current Gameunits address for receiving payments to this account.");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount = AccountFromValue(params[0]);

    Value ret;

    ret = GetAccountAddress(strAccount).ToString();

    return ret;
}



Value setaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setaccount <gameunitsaddress> <account>\n"
            "Sets the account associated with the given address.");

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Gameunits address");


    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Detect when changing the account of an address that is the 'unused current key' of another account:
    if (pwalletMain->mapAddressBook.count(address.Get()))
    {
        string strOldAccount = pwalletMain->mapAddressBook[address.Get()];
        if (address == GetAccountAddress(strOldAccount))
            GetAccountAddress(strOldAccount, true);
    }

    pwalletMain->SetAddressBookName(address.Get(), strAccount);

    return Value::null;
}


Value getaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccount <gameunitsaddress>\n"
            "Returns the account associated with the given address.");

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Gameunits address");

    string strAccount;
    map<CTxDestination, string>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.empty())
        strAccount = (*mi).second;
    return strAccount;
}


Value getaddressesbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressesbyaccount <account>\n"
            "Returns the list of addresses for the given account.");

    string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    Array ret;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, pwalletMain->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const string& strName = item.second;
        if (strName == strAccount)
            ret.push_back(address.ToString());
    }
    return ret;
}

Value sendtoaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "sendtoaddress <gameunitsaddress> <amount> [comment] [comment-to] [narration]\n" // Exchanges use the comments internally...
            "sendtoaddress <gameunitsaddress> <amount> [narration]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    if (params[0].get_str().length() > 75
        && IsStealthAddress(params[0].get_str()))
        return sendtostealthaddress(params, false);

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Gameunits address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);

    CWalletTx wtx;
    std::string sNarr;

    // Wallet comments
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["to"]      = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        sNarr                   = params[4].get_str();
    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, sNarr, wtx);

    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

Value listaddressgroupings(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listaddressgroupings\n"
            "Lists groups of addresses which have had their common ownership\n"
            "made public by common use as inputs or as the resulting change\n"
            "in past transactions");

    Array jsonGroupings;
    map<CTxDestination, int64_t> balances = pwalletMain->GetAddressBalances();
    BOOST_FOREACH(set<CTxDestination> grouping, pwalletMain->GetAddressGroupings())
    {
        Array jsonGrouping;
        BOOST_FOREACH(CTxDestination address, grouping)
        {
            Array addressInfo;
            addressInfo.push_back(CBitcoinAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                    addressInfo.push_back(pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get())->second);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}

Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "signmessage <gameunitsaddress> <message>\n"
            "Sign a message with the private key of an address");

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage <gameunitsaddress> <signature> <message>\n"
            "Verify a signed message");

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}


Value getreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaddress <gameunitsaddress> [minconf=1]\n"
            "Returns the total amount received by <gameunitsaddress> in transactions with at least [minconf] confirmations.");

    // Bitcoin address
    CBitcoinAddress address = CBitcoinAddress(params[0].get_str());
    CScript scriptPubKey;
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Gameunits address");
    scriptPubKey.SetDestination(address.Get());
    if (!IsMine(*pwalletMain,scriptPubKey))
        return (double)0.0;

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return  ValueFromAmount(nAmount);
}


void GetAccountAddresses(string strAccount, set<CTxDestination>& setAddress)
{
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& item, pwalletMain->mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second;
        if (strName == strAccount)
            setAddress.insert(address);
    }
}

Value getreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaccount <account> [minconf=1]\n"
            "Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.");

    accountingDeprecationCheck();

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    string strAccount = AccountFromValue(params[0]);
    set<CTxDestination> setAddress;
    GetAccountAddresses(strAccount, setAddress);

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}


int64_t GetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64_t nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!wtx.IsFinal() || wtx.GetDepthInMainChain() < 0)
            continue;

        int64_t nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

int64_t GetAccountBalance(const string& strAccount, int nMinDepth)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth);
}


Value getbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getbalance [account] [minconf=1]\n"
            "If [account] is not specified, returns the server's total available balance.\n"
            "If [account] is specified, returns the balance in the account.");

    if (params.size() == 0)
        return  ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    if (params[0].get_str() == "*") {
        // Calculate total balance a different way from GetBalance()
        // (GetBalance() sums up all unspent TxOuts)
        // getbalance and getbalance '*' 0 should return the same number.
        int64_t nBalance = 0;
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (!wtx.IsTrusted())
                continue;

            int64_t allFee;
            string strSentAccount;
            list<pair<CTxDestination, int64_t> > listReceived;
            list<pair<CTxDestination, int64_t> > listSent;
            wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount);
            if (wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
            {
                BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listReceived)
                    nBalance += r.second;
            }
            BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listSent)
                nBalance -= r.second;
            nBalance -= allFee;
        }
        return  ValueFromAmount(nBalance);
    }

    accountingDeprecationCheck();

    string strAccount = AccountFromValue(params[0]);

    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);

    return ValueFromAmount(nBalance);
}


Value movecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]\n"
            "Move from one account in your wallet to another.");

    accountingDeprecationCheck();

    string strFrom = AccountFromValue(params[0]);
    string strTo = AccountFromValue(params[1]);
    int64_t nAmount = AmountFromValue(params[2]);

    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    walletdb.WriteAccountingEntry(debit);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    walletdb.WriteAccountingEntry(credit);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}


Value sendfrom(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 7)
        throw runtime_error(
            "sendfrom <fromaccount> <togameunitsaddress> <amount> [minconf=1] [comment] [comment-to] [narration] \n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    string strAccount = AccountFromValue(params[0]);
    CBitcoinAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Gameunits address");
    int64_t nAmount = AmountFromValue(params[2]);

    int nMinDepth = 1;
    if (params.size() > 3)
        nMinDepth = params[3].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;

    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();

    std::string sNarr;
    if (params.size() > 6 && params[6].type() != null_type && !params[6].get_str().empty())
        sNarr = params[6].get_str();

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, sNarr, wtx);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}


Value sendmany(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]\n"
            "amounts are double-precision floating point numbers"
            + HelpRequiringPassphrase());

    string strAccount = AccountFromValue(params[0]);
    Object sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

    set<CBitcoinAddress> setAddress;
    vector<pair<CScript, int64_t> > vecSend;

    int64_t totalAmount = 0;
    BOOST_FOREACH(const Pair& s, sendTo)
    {
        CBitcoinAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Gameunits address: ")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64_t nAmount = AmountFromValue(s.value_);

        totalAmount += nAmount;

        vecSend.push_back(make_pair(scriptPubKey, nAmount));
    }

    EnsureWalletIsUnlocked();

    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    int nChangePos;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, nChangePos);
    if (!fCreated)
    {
        if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value addmultisigaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
    {
        string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]\n"
            "Add a nrequired-to-sign multisignature address to the wallet\"\n"
            "each key is a Gameunits address or hex-encoded public key\n"
            "If [account] is specified, assign address to [account].";
        throw runtime_error(msg);
    }

    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();
    string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();

        // Case 1: Bitcoin address and we have full public key:
        CBitcoinAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner;
    inner.SetMultisig(nRequired, pubkeys);
    CScriptID innerID = inner.GetID();
    if (!pwalletMain->AddCScript(inner))
        throw runtime_error("AddCScript() failed");

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return CBitcoinAddress(innerID).ToString();
}

Value addredeemscript(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
    {
        string msg = "addredeemscript <redeemScript> [account]\n"
            "Add a P2SH address with a specified redeemScript to the wallet.\n"
            "If [account] is specified, assign address to [account].";
        throw runtime_error(msg);
    }

    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Construct using pay-to-script-hash:
    vector<unsigned char> innerData = ParseHexV(params[0], "redeemScript");
    CScript inner(innerData.begin(), innerData.end());
    CScriptID innerID = inner.GetID();
    if (!pwalletMain->AddCScript(inner))
        throw runtime_error("AddCScript() failed");

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return CBitcoinAddress(innerID).ToString();
}

struct tallyitem
{
    int64_t nAmount;
    int nConf;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
    }
};

Value ListReceived(const Array& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    // Tally
    map<CBitcoinAddress, tallyitem> mapTally;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;

        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < nMinDepth)
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address) || !IsMine(*pwalletMain, address))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = min(item.nConf, nDepth);
        }
    }

    // Reply
    Array ret;
    map<string, tallyitem> mapAccountTally;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, pwalletMain->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const string& strAccount = item.second;
        map<CBitcoinAddress, tallyitem>::iterator it = mapTally.find(address);
        if (it == mapTally.end() && !fIncludeEmpty)
            continue;

        int64_t nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        if (it != mapTally.end())
        {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
        }

        if (fByAccounts)
        {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = min(item.nConf, nConf);
        }
        else
        {
            Object obj;
            obj.push_back(Pair("address",       address.ToString()));
            obj.push_back(Pair("account",       strAccount));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    if (fByAccounts)
    {
        for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
        {
            int64_t nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            Object obj;
            obj.push_back(Pair("account",       (*it).first));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}

Value listreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaddress [minconf=1] [includeempty=false]\n"
            "[minconf] is the minimum number of confirmations before payments are included.\n"
            "[includeempty] whether to include addresses that haven't received any payments.\n"
            "Returns an array of objects containing:\n"
            "  \"address\" : receiving address\n"
            "  \"account\" : the account of the receiving address\n"
            "  \"amount\" : total amount received by the address\n"
            "  \"confirmations\" : number of confirmations of the most recent transaction included");

    return ListReceived(params, false);
}

Value listreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaccount [minconf=1] [includeempty=false]\n"
            "[minconf] is the minimum number of confirmations before payments are included.\n"
            "[includeempty] whether to include accounts that haven't received any payments.\n"
            "Returns an array of objects containing:\n"
            "  \"account\" : the account of the receiving addresses\n"
            "  \"amount\" : total amount received by addresses with this account\n"
            "  \"confirmations\" : number of confirmations of the most recent transaction included");

    accountingDeprecationCheck();

    return ListReceived(params, true);
}

static void MaybePushAddress(Object & entry, const CTxDestination &dest)
{
    CBitcoinAddress addr;
    if (addr.Set(dest))
        entry.push_back(Pair("address", addr.ToString()));
}

void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
{
    int64_t nFee;
    string strSentAccount;
    list<pair<CTxDestination, int64_t> > listReceived;
    list<pair<CTxDestination, int64_t> > listSent;

    wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);

    bool fAllAccounts = (strAccount == string("*"));

    // Sent
    if ((!wtx.IsCoinStake()) && (!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& s, listSent)
        {
            Object entry;
            entry.push_back(Pair("account", strSentAccount));
            MaybePushAddress(entry, s.first);
            entry.push_back(Pair("category", "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        };
    };

    // Received
    if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    {
        bool stop = false;
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& r, listReceived)
        {

            string account;
            if (pwalletMain->mapAddressBook.count(r.first))
                account = pwalletMain->mapAddressBook[r.first];
            if (fAllAccounts || (account == strAccount))
            {
                Object entry;
                entry.push_back(Pair("account", account));
                MaybePushAddress(entry, r.first);
                if (wtx.IsCoinBase() || wtx.IsCoinStake())
                {
                    if (wtx.GetDepthInMainChain() < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else
                    if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                } else
                {
                    entry.push_back(Pair("category", "receive"));
                };

                if (!wtx.IsCoinStake())
                {
                    entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                } else
                {
                    entry.push_back(Pair("amount", ValueFromAmount(-nFee)));
                    stop = true; // only one coinstake output
                };

                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            };
            if (stop)
                break;
        };
    };
}

void AcentryToJSON(const CAccountingEntry& acentry, const string& strAccount, Array& ret)
{
    bool fAllAccounts = (strAccount == string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount)
    {
        Object entry;
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", (int64_t)acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

Value listtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 4)
        throw runtime_error(
            "listtransactions [account] [count=10] [from=0] [show_coinstake=1]\n"
            "Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");

    // listtransactions "*" 20 0 0
    string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();

    bool fShowCoinstake = true;
    if (params.size() > 3)
    {
        std::string value   = params[3].get_str();
        if (IsStringBoolNegative(value))
            fShowCoinstake = false;
    };

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    Array ret;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, strAccount, fShowCoinstake);

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;

        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret);
        CAccountingEntry *const pacentry = (*it).second.second;

        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount+nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;
    Array::iterator first = ret.begin();
    std::advance(first, nFrom);
    Array::iterator last = ret.begin();
    std::advance(last, nFrom+nCount);

    if (last != ret.end()) ret.erase(last, ret.end());
    if (first != ret.begin()) ret.erase(ret.begin(), first);

    std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    return ret;
}

Value listaccounts(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "listaccounts [minconf=1]\n"
            "Returns Object that has account names as keys, account balances as values.");

    accountingDeprecationCheck();

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    map<string, int64_t> mapAccountBalances;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& entry, pwalletMain->mapAddressBook)
    {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
            mapAccountBalances[entry.second] = 0;
    };

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        int64_t nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64_t> > listReceived;
        list<pair<CTxDestination, int64_t> > listSent;
        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0)
            continue;

        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);
        mapAccountBalances[strSentAccount] -= nFee;

        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& s, listSent)
            mapAccountBalances[strSentAccount] -= s.second;

        if (nDepth >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
        {
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& r, listReceived)
                if (pwalletMain->mapAddressBook.count(r.first))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.first]] += r.second;
                else
                    mapAccountBalances[""] += r.second;
        };
    }

    list<CAccountingEntry> acentries;
    CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
    BOOST_FOREACH(const CAccountingEntry& entry, acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    Object ret;
    BOOST_FOREACH(const PAIRTYPE(string, int64_t)& accountBalance, mapAccountBalances)
    {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}

Value listsinceblock(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listsinceblock [blockhash] [target-confirmations]\n"
            "Get all transactions in blocks since block [blockhash], or all transactions if omitted");

    CBlockIndex *pindex = NULL;
    int target_confirms = 1;

    if (params.size() > 0)
    {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        pindex = CBlockLocator(blockId).GetBlockIndex();
    };

    if (params.size() > 1)
    {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    };

    int depth = pindex ? (1 + nBestHeight - pindex->nHeight) : -1;

    Array transactions;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain() < depth)
            ListTransactions(tx, "*", 0, true, transactions);
    };

    uint256 lastblock;

    if (target_confirms == 1)
    {
        lastblock = hashBestChain;
    } else
    {
        int target_height = pindexBest->nHeight + 1 - target_confirms;

        CBlockIndex *block;
        for (block = pindexBest;
             block && block->nHeight > target_height;
             block = block->pprev)  { }

        lastblock = block ? block->GetBlockHash() : 0;
    };

    Object ret;
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}

Value gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction <txid>\n"
            "Get detailed information about <txid>");

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;

    if (pwalletMain->mapWallet.count(hash))
    {
        const CWalletTx& wtx = pwalletMain->mapWallet[hash];

        TxToJSON(wtx, 0, entry);

        int64_t nCredit = wtx.GetCredit();
        int64_t nDebit = wtx.GetDebit();
        int64_t nNet = nCredit - nDebit;
        int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

        entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
        if (wtx.IsFromMe())
            entry.push_back(Pair("fee", ValueFromAmount(nFee)));

        WalletTxToJSON(wtx, entry);

        Array details;
        ListTransactions(pwalletMain->mapWallet[hash], "*", 0, false, details);
        entry.push_back(Pair("details", details));
    } else
    {
        CTransaction tx;
        uint256 hashBlock = 0;
        if (GetTransaction(hash, tx, hashBlock))
        {
            TxToJSON(tx, 0, entry);
            if (hashBlock == 0)
            {
                entry.push_back(Pair("confirmations", 0));
            } else
            {
                entry.push_back(Pair("blockhash", hashBlock.GetHex()));
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
                if (mi != mapBlockIndex.end() && (*mi).second)
                {
                    CBlockIndex* pindex = (*mi).second;
                    if (pindex->IsInMainChain())
                        entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
                    else
                        entry.push_back(Pair("confirmations", 0));
                };
            };
        } else
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
    };

    return entry;
}


Value backupwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "backupwallet <destination>\n"
            "Safely copies wallet.dat to destination, which can be a directory or a path with filename.");

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}


Value keypoolrefill(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "keypoolrefill [new-size]\n"
            "Fills the keypool."
            + HelpRequiringPassphrase());

    unsigned int nSize = max(GetArg("-keypool", 100), (int64_t)0);
    if (params.size() > 0) {
        if (params[0].get_int() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size");
        nSize = (unsigned int) params[0].get_int();
    }

    EnsureWalletIsUnlocked();

    pwalletMain->TopUpKeyPool(nSize);

    if (pwalletMain->GetKeyPoolSize() < nSize)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");

    return Value::null;
}

static void LockWallet(CWallet* pWallet)
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pWallet->Lock();
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 3))
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout> [stakingonly]\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.\n"
            "if [stakingonly] is true sending functions are disabled.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    if (!pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_ALREADY_UNLOCKED, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");
    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() > 0)
    {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    } else
    {
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");
    };
    
    pwalletMain->TopUpKeyPool();
    int64_t nSleepTime = params[1].get_int64();
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = GetTime() + nSleepTime;
    RPCRunLater("lockwallet", boost::bind(LockWallet, pwalletMain), nSleepTime);

    // ppcoin: if user OS account compromised prevent trivial sendmoney commands
    if (params.size() > 2)
        fWalletUnlockStakingOnly = params[2].get_bool();
    else
        fWalletUnlockStakingOnly = false;

    return Value::null;
}


Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    return Value::null;
}


Value walletlock(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
        throw runtime_error(
            "walletlock\n"
            "Removes the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return Value::null;
}


Value encryptwallet(const Array& params, bool fHelp)
{
    if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");
    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; Gameunits server stopping, restart to run with encrypted wallet.  The keypool has been flushed, you need to make a new backup.";
}

class DescribeAddressVisitor : public boost::static_visitor<Object>
{
public:
    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        pwalletMain->GetPubKey(keyID, vchPubKey);
        obj.push_back(Pair("isscript", false));
        obj.push_back(Pair("pubkey", HexStr(vchPubKey)));
        obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        CScript subscript;
        pwalletMain->GetCScript(scriptID, subscript);
        std::vector<CTxDestination> addresses;
        txnouttype whichType;
        int nRequired;
        ExtractDestinations(subscript, whichType, addresses, nRequired);
        obj.push_back(Pair("script", GetTxnOutputType(whichType)));
        obj.push_back(Pair("hex", HexStr(subscript.begin(), subscript.end())));
        Array a;
        BOOST_FOREACH(const CTxDestination& addr, addresses)
            a.push_back(CBitcoinAddress(addr).ToString());
        obj.push_back(Pair("addresses", a));
        if (whichType == TX_MULTISIG)
            obj.push_back(Pair("sigsrequired", nRequired));
        return obj;
    }

    Object operator()(const CStealthAddress &stxAddr) const {
        Object obj;
        obj.push_back(Pair("todo", true));
        return obj;
    }
};

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress <gameunitsaddress>\n"
            "Return information about <gameunitsaddress>.");

    CBitcoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = IsMine(*pwalletMain, dest);
        ret.push_back(Pair("ismine", fMine));
        if (fMine)
        {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        };

        if (pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return ret;
}

Value validatepubkey(const Array& params, bool fHelp)
{
    if (fHelp || !params.size() || params.size() > 2)
        throw runtime_error(
            "validatepubkey <gameunitspubkey>\n"
            "Return information about <gameunitspubkey>.");

    std::vector<unsigned char> vchPubKey = ParseHex(params[0].get_str());
    CPubKey pubKey(vchPubKey);

    bool isValid = pubKey.IsValid();
    bool isCompressed = pubKey.IsCompressed();
    CKeyID keyID = pubKey.GetID();

    CBitcoinAddress address;
    address.Set(keyID);

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = IsMine(*pwalletMain, dest);
        ret.push_back(Pair("ismine", fMine));
        ret.push_back(Pair("iscompressed", isCompressed));
        if (fMine)
        {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return ret;
}

// ppcoin: reserve balance from being staked for network protection
Value reservebalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "reservebalance [<reserve> [amount]]\n"
            "<reserve> is true or false to turn balance reserve on or off.\n"
            "<amount> is a real and rounded to cent.\n"
            "Set reserve amount not participating in network protection.\n"
            "If no parameters provided current setting is printed.\n");

    if (params.size() > 0)
    {
        bool fReserve = params[0].get_bool();
        if (fReserve)
        {
            if (params.size() == 1)
                throw runtime_error("must provide amount to reserve balance.\n");
            int64_t nAmount = AmountFromValue(params[1]);
            nAmount = (nAmount / CENT) * CENT;  // round to cent
            if (nAmount < 0)
                throw runtime_error("amount cannot be negative.\n");
            nReserveBalance = nAmount;
        } else
        {
            if (params.size() > 1)
                throw runtime_error("cannot specify amount to turn off reserve.\n");
            nReserveBalance = 0;
        }
    }

    Object result;
    result.push_back(Pair("reserve", (nReserveBalance > 0)));
    result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
    return result;
}


// ppcoin: check wallet integrity
Value checkwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "checkwallet\n"
            "Check wallet for integrity.\n");

    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion, true);
    Object result;
    if (nMismatchSpent == 0)
    {
        result.push_back(Pair("wallet check passed", true));
    } else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
    };
    return result;
}


// ppcoin: repair wallet
Value repairwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "repairwallet\n"
            "Repair wallet if checkwallet reports any problem.\n");

    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion);
    Object result;
    if (nMismatchSpent == 0)
    {
        result.push_back(Pair("wallet check passed", true));
    } else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
    }
    return result;
}

// NovaCoin: resend unconfirmed wallet transactions
Value resendtx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "resendtx\n"
            "Re-send unconfirmed transactions.\n"
        );

    ResendWalletTransactions(true);

    return Value::null;
}

// ppcoin: make a public-private key pair
Value makekeypair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "makekeypair [prefix]\n"
            "Make a public/private key pair.\n"
            "[prefix] is optional preferred prefix for the public key.\n");

    string strPrefix = "";
    if (params.size() > 0)
        strPrefix = params[0].get_str();

    CKey key;
    key.MakeNewKey(false);

    CPrivKey vchPrivKey = key.GetPrivKey();
    Object result;
    result.push_back(Pair("PrivateKey", HexStr<CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
    result.push_back(Pair("PublicKey", HexStr(key.GetPubKey())));
    return result;
}



Value getnewstealthaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewstealthaddress [label]\n"
            "Returns a new Gameunits stealth address for receiving payments anonymously."
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw runtime_error("Failed: Wallet must be unlocked.");

    std::string sLabel;
    if (params.size() > 0)
        sLabel = params[0].get_str();

    CStealthAddress sxAddr;
    std::string sError;
    if (!pwalletMain->NewStealthAddress(sError, sLabel, sxAddr))
        throw runtime_error(std::string("Could get new stealth address: ") + sError);

    if (!pwalletMain->AddStealthAddress(sxAddr))
        throw runtime_error("Could not save to wallet.");

    return sxAddr.Encoded();
}

Value liststealthaddresses(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "liststealthaddresses [show_secrets=0]\n"
            "List owned stealth addresses.");

    bool fShowSecrets = false;

    if (params.size() > 0)
    {
        std::string str = params[0].get_str();

        if (IsStringBoolNegative(str))
            fShowSecrets = false;
        else
            fShowSecrets = true;
    };

    if (fShowSecrets)
        EnsureWalletIsUnlocked();

    Object result;

    std::set<CStealthAddress>::iterator it;
    for (it = pwalletMain->stealthAddresses.begin(); it != pwalletMain->stealthAddresses.end(); ++it)
    {
        if (it->scan_secret.size() < 1)
            continue; // stealth address is not owned

        if (fShowSecrets)
        {
            Object objA;
            objA.push_back(Pair("Label        ", it->label));
            objA.push_back(Pair("Address      ", it->Encoded()));
            objA.push_back(Pair("Scan Secret  ", HexStr(it->scan_secret.begin(), it->scan_secret.end())));
            objA.push_back(Pair("Spend Secret ", HexStr(it->spend_secret.begin(), it->spend_secret.end())));
            result.push_back(Pair("Stealth Address", objA));
        } else
        {
            result.push_back(Pair("Stealth Address", it->Encoded() + " - " + it->label));
        };
    };

    return result;
}

Value importstealthaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2)
        throw runtime_error(
            "importstealthaddress <scan_secret> <spend_secret> [label]\n"
            "Import an owned stealth addresses."
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw runtime_error("Failed: Wallet must be unlocked.");

    std::string sScanSecret  = params[0].get_str();
    std::string sSpendSecret = params[1].get_str();
    std::string sLabel;


    if (params.size() > 2)
    {
        sLabel = params[2].get_str();
    };

    std::vector<uint8_t> vchScanSecret;
    std::vector<uint8_t> vchSpendSecret;

    if (IsHex(sScanSecret))
    {
        vchScanSecret = ParseHex(sScanSecret);
    } else
    {
        if (!DecodeBase58(sScanSecret, vchScanSecret))
            throw runtime_error("Could not decode scan secret as hex or base58.");
    };

    if (IsHex(sSpendSecret))
    {
        vchSpendSecret = ParseHex(sSpendSecret);
    } else
    {
        if (!DecodeBase58(sSpendSecret, vchSpendSecret))
            throw runtime_error("Could not decode spend secret as hex or base58.");
    };

    if (vchScanSecret.size() != 32)
        throw runtime_error("Scan secret is not 32 bytes.");
    if (vchSpendSecret.size() != 32)
        throw runtime_error("Spend secret is not 32 bytes.");


    ec_secret scan_secret;
    ec_secret spend_secret;

    memcpy(&scan_secret.e[0], &vchScanSecret[0], 32);
    memcpy(&spend_secret.e[0], &vchSpendSecret[0], 32);

    ec_point scan_pubkey, spend_pubkey;
    if (SecretToPublicKey(scan_secret, scan_pubkey) != 0)
        throw runtime_error("Could not get scan public key.");

    if (SecretToPublicKey(spend_secret, spend_pubkey) != 0)
        throw runtime_error("Could not get spend public key.");


    CStealthAddress sxAddr;
    sxAddr.label = sLabel;
    sxAddr.scan_pubkey = scan_pubkey;
    sxAddr.spend_pubkey = spend_pubkey;

    sxAddr.scan_secret = vchScanSecret;
    sxAddr.spend_secret = vchSpendSecret;

    Object result;
    bool fFound = false;
    // -- find if address already exists
    std::set<CStealthAddress>::iterator it;
    for (it = pwalletMain->stealthAddresses.begin(); it != pwalletMain->stealthAddresses.end(); ++it)
    {
        CStealthAddress &sxAddrIt = const_cast<CStealthAddress&>(*it);
        if (sxAddrIt.scan_pubkey == sxAddr.scan_pubkey
            && sxAddrIt.spend_pubkey == sxAddr.spend_pubkey)
        {
            if (sxAddrIt.scan_secret.size() < 1)
            {
                sxAddrIt.scan_secret = sxAddr.scan_secret;
                sxAddrIt.spend_secret = sxAddr.spend_secret;
                fFound = true; // update stealth address with secrets
                break;
            };

            result.push_back(Pair("result", "Import failed - stealth address exists."));
            return result;
        };
    };

    if (fFound)
    {
        result.push_back(Pair("result", "Success, updated " + sxAddr.Encoded()));
    } else
    {
        pwalletMain->stealthAddresses.insert(sxAddr);
        result.push_back(Pair("result", "Success, imported " + sxAddr.Encoded()));
    };


    if (!pwalletMain->AddStealthAddress(sxAddr))
        throw runtime_error("Could not save to wallet.");

    return result;
}


Value sendtostealthaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
        "sendtostealthaddress <stealth_address> <amount> [comment] [comment-to] [narration]\n"
        "sendtostealthaddress <stealth_address> <amount> [narration]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    std::string sEncoded = params[0].get_str();
    int64_t nAmount = AmountFromValue(params[1]);

    std::string sNarr;
    if (params.size() == 3 || params.size() == 5)
    {
        int nNarr = params.size() - 1;
        if(params[nNarr].type() != null_type && !params[nNarr].get_str().empty())
            sNarr = params[nNarr].get_str();
    }

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(sEncoded))
        throw runtime_error("Invalid Gameunits stealth address.");

    CWalletTx wtx;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["to"]      = params[4].get_str();

    std::string sError;
    if (!pwalletMain->SendStealthMoneyToDestination(sxAddr, nAmount, sNarr, wtx, sError))
        throw JSONRPCError(RPC_WALLET_ERROR, sError);

    return wtx.GetHash().GetHex();
}

Value clearwallettransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "clearwallettransactions \n"
            "delete all transactions from wallet - reload with reloadanondata\n"
            "Warning: Backup your wallet first!");



    Object result;

    uint32_t nTransactions = 0;

    char cbuf[256];

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CWalletDB walletdb(pwalletMain->strWalletFile);
        walletdb.TxnBegin();
        Dbc* pcursor = walletdb.GetTxnCursor();
        if (!pcursor)
            throw runtime_error("Cannot get wallet DB cursor");

        Dbt datKey;
        Dbt datValue;

        datKey.set_flags(DB_DBT_USERMEM);
        datValue.set_flags(DB_DBT_USERMEM);

        std::vector<unsigned char> vchKey;
        std::vector<unsigned char> vchType;
        std::vector<unsigned char> vchKeyData;
        std::vector<unsigned char> vchValueData;

        vchKeyData.resize(100);
        vchValueData.resize(100);

        datKey.set_ulen(vchKeyData.size());
        datKey.set_data(&vchKeyData[0]);

        datValue.set_ulen(vchValueData.size());
        datValue.set_data(&vchValueData[0]);

        unsigned int fFlags = DB_NEXT; // same as using DB_FIRST for new cursor
        while (true)
        {
            int ret = pcursor->get(&datKey, &datValue, fFlags);

            if (ret == ENOMEM
                || ret == DB_BUFFER_SMALL)
            {
                if (datKey.get_size() > datKey.get_ulen())
                {
                    vchKeyData.resize(datKey.get_size());
                    datKey.set_ulen(vchKeyData.size());
                    datKey.set_data(&vchKeyData[0]);
                };

                if (datValue.get_size() > datValue.get_ulen())
                {
                    vchValueData.resize(datValue.get_size());
                    datValue.set_ulen(vchValueData.size());
                    datValue.set_data(&vchValueData[0]);
                };
                // -- try once more, when DB_BUFFER_SMALL cursor is not expected to move
                ret = pcursor->get(&datKey, &datValue, fFlags);
            };

            if (ret == DB_NOTFOUND)
                break;
            else
            if (datKey.get_data() == NULL || datValue.get_data() == NULL
                || ret != 0)
            {
                snprintf(cbuf, sizeof(cbuf), "wallet DB error %d, %s", ret, db_strerror(ret));
                throw runtime_error(cbuf);
            };

            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            ssValue.SetType(SER_DISK);
            ssValue.clear();
            ssValue.write((char*)datKey.get_data(), datKey.get_size());

            ssValue >> vchType;


            std::string strType(vchType.begin(), vchType.end());

            //LogPrintf("strType %s\n", strType.c_str());

            if (strType == "tx")
            {
                uint256 hash;
                ssValue >> hash;

                if ((ret = pcursor->del(0)) != 0)
                {
                    LogPrintf("Delete transaction failed %d, %s\n", ret, db_strerror(ret));
                    continue;
                };

                pwalletMain->mapWallet.erase(hash);
                pwalletMain->NotifyTransactionChanged(pwalletMain, hash, CT_DELETED);

                nTransactions++;
            };
        };
        pcursor->close();
        walletdb.TxnCommit();

        //pwalletMain->mapWallet.clear();

        if (nNodeMode == NT_THIN)
        {
            // reset LastFilteredHeight
            walletdb.WriteLastFilteredHeight(0);
        }
    }



    snprintf(cbuf, sizeof(cbuf), "Removed %u transactions.", nTransactions);
    result.push_back(Pair("complete", std::string(cbuf)));
    result.push_back(Pair("", "Reload with reloadanondata, reindex or re-download blockchain."));


    return result;
}

Value scanforalltxns(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "scanforalltxns [fromHeight]\n"
            "Scan blockchain for owned transactions.");


    if (nNodeMode != NT_FULL)
        throw runtime_error("Can't run in thin mode.");

    Object result;
    int32_t nFromHeight = 0;

    CBlockIndex *pindex = pindexGenesisBlock;


    if (params.size() > 0)
        nFromHeight = params[0].get_int();


    if (nFromHeight > 0)
    {
        pindex = mapBlockIndex[hashBestChain];
        while (pindex->nHeight > nFromHeight
            && pindex->pprev)
            pindex = pindex->pprev;
    };

    if (pindex == NULL)
        throw runtime_error("Genesis Block is not set.");

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        pwalletMain->MarkDirty();

        pwalletMain->ScanForWalletTransactions(pindex, true);
        pwalletMain->ReacceptWalletTransactions();
    }

    result.push_back(Pair("result", "Scan complete."));

    return result;
}

Value scanforstealthtxns(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "scanforstealthtxns [fromHeight]\n"
            "Scan blockchain for owned stealth transactions.");

    Object result;
    uint32_t nBlocks = 0;
    uint32_t nTransactions = 0;
    int32_t nFromHeight = 0;

    CBlockIndex *pindex = pindexGenesisBlock;


    if (params.size() > 0)
        nFromHeight = params[0].get_int();


    if (nFromHeight > 0)
    {
        pindex = mapBlockIndex[hashBestChain];
        while (pindex->nHeight > nFromHeight
            && pindex->pprev)
            pindex = pindex->pprev;
    };

    if (pindex == NULL)
        throw runtime_error("Genesis Block is not set.");

    // -- locks in AddToWalletIfInvolvingMe

    bool fUpdate = true; // todo: option?

    pwalletMain->nStealth = 0;
    pwalletMain->nFoundStealth = 0;

    while (pindex)
    {
        nBlocks++;
        CBlock block;
        block.ReadFromDisk(pindex, true);

        BOOST_FOREACH(CTransaction& tx, block.vtx)
        {
            if (!tx.IsStandard())
                continue; // leave out coinbase and others
            nTransactions++;

            uint256 hash = tx.GetHash();
            pwalletMain->AddToWalletIfInvolvingMe(tx, hash, &block, fUpdate);
        };

        pindex = pindex->pnext;
    };

    LogPrintf("Scanned %u blocks, %u transactions\n", nBlocks, nTransactions);
    LogPrintf("Found %u stealth transactions in blockchain.\n", pwalletMain->nStealth);
    LogPrintf("Found %u new owned stealth transactions.\n", pwalletMain->nFoundStealth);

    char cbuf[256];
    snprintf(cbuf, sizeof(cbuf), "%u new stealth transactions.", pwalletMain->nFoundStealth);

    result.push_back(Pair("result", "Scan complete."));
    result.push_back(Pair("found", std::string(cbuf)));

    return result;
}


Value sendgameunitstoanon(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "sendgameunitstoanon <stealth_address> <amount> [narration] [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            "<ring_size> is a number of outputs of the same amount to include in the signature"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    std::string sEncoded = params[0].get_str();

    int64_t nAmount = AmountFromValue(params[1]);

    std::string sNarr;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        sNarr = params[2].get_str();

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(sEncoded))
        throw runtime_error("Invalid Gameunits stealth address.");

    CWalletTx wtx;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["to"]      = params[4].get_str();

    std::string sError;
    if (!pwalletMain->SendGameunitsToAnon(sxAddr, nAmount, sNarr, wtx, sError))
    {
        LogPrintf("SendGameunitsToAnon failed %s\n", sError.c_str());
        throw JSONRPCError(RPC_WALLET_ERROR, sError);
    };
    return wtx.GetHash().GetHex();
}

Value sendanontoanon(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
            "sendanontoanon <stealth_address> <amount> <ring_size> [narration] [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            "<ring_size> is a number of outputs of the same amount to include in the signature"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    std::string sEncoded = params[0].get_str();
    int64_t nAmount = AmountFromValue(params[1]);

    uint32_t nRingSize = (uint32_t)params[2].get_int();

    std::ostringstream ssThrow;
    if (nRingSize < MIN_RING_SIZE || nRingSize > MAX_RING_SIZE)
        ssThrow << "Ring size must be >= " << MIN_RING_SIZE << " and <= " << MAX_RING_SIZE << ".", throw runtime_error(ssThrow.str());


    std::string sNarr;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        sNarr = params[3].get_str();

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(sEncoded))
        throw runtime_error("Invalid Gameunits stealth address.");

    CWalletTx wtx;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();


    std::string sError;
    if (!pwalletMain->SendAnonToAnon(sxAddr, nAmount, nRingSize, sNarr, wtx, sError))
    {
        LogPrintf("SendAnonToAnon failed %s\n", sError.c_str());
        throw JSONRPCError(RPC_WALLET_ERROR, sError);
    };
    return wtx.GetHash().GetHex();
}

Value sendanontogameunits(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
            "sendanontogameunits <stealth_address> <amount> <ring_size> [narration] [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    std::string sEncoded = params[0].get_str();
    int64_t nAmount = AmountFromValue(params[1]);

    uint32_t nRingSize = (uint32_t)params[2].get_int();

    std::ostringstream ssThrow;
    if (nRingSize < 1 || nRingSize > MAX_RING_SIZE)
        ssThrow << "Ring size must be >= 1 and <= " << MAX_RING_SIZE << ".", throw runtime_error(ssThrow.str());


    std::string sNarr;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        sNarr = params[3].get_str();

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");

    CStealthAddress sxAddr;

    if (!sxAddr.SetEncoded(sEncoded))
        throw runtime_error("Invalid Gameunits stealth address.");

    CWalletTx wtx;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();


    std::string sError;
    if (!pwalletMain->SendAnonToGameunits(sxAddr, nAmount, nRingSize, sNarr, wtx, sError))
    {
        LogPrintf("SendAnonToGameunits failed %s\n", sError.c_str());
        throw JSONRPCError(RPC_WALLET_ERROR, sError);
    };
    return wtx.GetHash().GetHex();
}

Value estimateanonfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "estimateanonfee <amount> <ring_size> [narration]\n"
            "<amount>is a real and is rounded to the nearest 0.000001");

    int64_t nAmount = AmountFromValue(params[0]);

    uint32_t nRingSize = (uint32_t)params[1].get_int();

    std::ostringstream ssThrow;
    if (nRingSize < MIN_RING_SIZE || nRingSize > MAX_RING_SIZE)
        ssThrow << "Ring size must be >= " << MIN_RING_SIZE << " and <= " << MAX_RING_SIZE << ".", throw runtime_error(ssThrow.str());


    std::string sNarr;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        sNarr = params[2].get_str();

    if (sNarr.length() > 24)
        throw runtime_error("Narration must be 24 characters or less.");


    CWalletTx wtx;
    int64_t nFee = 0;
    std::string sError;
    if (!pwalletMain->EstimateAnonFee(nAmount, nRingSize, sNarr, wtx, nFee, sError))
    {
        LogPrintf("EstimateAnonFee failed %s\n", sError.c_str());
        throw JSONRPCError(RPC_WALLET_ERROR, sError);
    };

    uint32_t nBytes = ::GetSerializeSize(*(CTransaction*)&wtx, SER_NETWORK, PROTOCOL_VERSION);

    Object result;

    result.push_back(Pair("Estimated bytes", (int)nBytes));
    result.push_back(Pair("Estimated inputs", (int)wtx.vin.size()));
    result.push_back(Pair("Estimated outputs", (int)wtx.vout.size()));
    result.push_back(Pair("Estimated fee", ValueFromAmount(nFee)));

    return result;
}

Value anonoutputs(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "anonoutputs [systemTotals] [show_immature_outputs]\n"
            "[systemTotals] if true displays the total no. of coins in the system.");

    if (nNodeMode != NT_FULL)
        throw runtime_error("Must be in full mode.");

    bool fSystemTotals = false;
    if (params.size() > 0)
    {
        std::string value   = params[0].get_str();
        if (IsStringBoolPositive(value))
            fSystemTotals = true;
    };

    bool fMatureOnly = true;
    if (params.size() > 1)
    {
        std::string value   = params[1].get_str();
        if (IsStringBoolPositive(value))
            fMatureOnly = false;
    };

    std::list<COwnedAnonOutput> lAvailableCoins;
    if (pwalletMain->ListUnspentAnonOutputs(lAvailableCoins, fMatureOnly) != 0)
        throw runtime_error("ListUnspentAnonOutputs() failed.");


    Object result;

    if (!fSystemTotals)
    {
        result.push_back(Pair("No. of coins", "amount"));

        // -- mAvailableCoins is ordered by value
        char cbuf[256];
        int64_t nTotal = 0;
        int64_t nLast = 0;
        int nCount = 0;
        for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
        {
            if (nLast > 0 && it->nValue != nLast)
            {
                snprintf(cbuf, sizeof(cbuf), "%03d", nCount);
                result.push_back(Pair(cbuf, ValueFromAmount(nLast)));
                nCount = 0;
            };
            nCount++;
            nLast = it->nValue;
            nTotal += it->nValue;
        };

        if (nCount > 0)
        {
            snprintf(cbuf, sizeof(cbuf), "%03d", nCount);
            result.push_back(Pair(cbuf, ValueFromAmount(nLast)));
        };
        result.push_back(Pair("total", ValueFromAmount(nTotal)));
    } else
    {
        std::map<int64_t, int> mOutputCounts;
        for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
            mOutputCounts[it->nValue] = 0;

        if (pwalletMain->CountAnonOutputs(mOutputCounts, fMatureOnly) != 0)
            throw runtime_error("CountAnonOutputs() failed.");

        result.push_back(Pair("No. of coins owned, No. of system coins", "amount"));

        // -- lAvailableCoins is ordered by value
        char cbuf[256];
        int64_t nTotal = 0;
        int64_t nLast = 0;
        int64_t nCount = 0;
        int64_t nSystemCount;
        for (std::list<COwnedAnonOutput>::iterator it = lAvailableCoins.begin(); it != lAvailableCoins.end(); ++it)
        {
            if (nLast > 0 && it->nValue != nLast)
            {
                nSystemCount = mOutputCounts[nLast];
                std::string str = strprintf(cbuf, sizeof(cbuf), "%04d, %04d", nCount, nSystemCount);
                result.push_back(Pair(str, ValueFromAmount(nLast)));
                nCount = 0;
            };
            nCount++;
            nLast = it->nValue;
            nTotal += it->nValue;
        };

        if (nCount > 0)
        {
            nSystemCount = mOutputCounts[nLast];
            std::string str = strprintf(cbuf, sizeof(cbuf), "%04d, %04d", nCount, nSystemCount);
            result.push_back(Pair(str, ValueFromAmount(nLast)));
        };
        result.push_back(Pair("total currency owned", ValueFromAmount(nTotal)));
    }

    return result;
}

Value anoninfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "anoninfo [recalculate]\n"
            "list outputs in system.");

    if (nNodeMode != NT_FULL)
        throw runtime_error("Must be in full mode.");

    bool fMatureOnly = false; // TODO: add parameter

    bool fRecalculate = false;

    if (params.size() > 0)
    {
        std::string value   = params[0].get_str();
        if (IsStringBoolPositive(value))
            fRecalculate = true;
    };

    Object result;

    std::list<CAnonOutputCount> lOutputCounts;

    if (fRecalculate)
    {
        if (pwalletMain->CountAllAnonOutputs(lOutputCounts, fMatureOnly) != 0)
            throw runtime_error("CountAllAnonOutputs() failed.");
    } else
    {
        // TODO: make mapAnonOutputStats a vector preinitialised with all possible coin values?
        for (std::map<int64_t, CAnonOutputCount>::iterator mi = mapAnonOutputStats.begin(); mi != mapAnonOutputStats.end(); ++mi)
        {
            bool fProcessed = false;
            CAnonOutputCount aoc = mi->second;
            if (aoc.nLeastDepth > 0)
                aoc.nLeastDepth = nBestHeight - aoc.nLeastDepth;
            for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
            {
                if (aoc.nValue > it->nValue)
                    continue;
                lOutputCounts.insert(it, aoc);
                fProcessed = true;
                break;
            };
            if (!fProcessed)
                lOutputCounts.push_back(aoc);
        };
    };

    result.push_back(Pair("No. Exists, No. Spends, Least Depth", "value"));


    // -- lOutputCounts is ordered by value
    char cbuf[256];
    int64_t nTotalIn = 0;
    int64_t nTotalOut = 0;
    int64_t nTotalCoins = 0;
    for (std::list<CAnonOutputCount>::iterator it = lOutputCounts.begin(); it != lOutputCounts.end(); ++it)
    {
        snprintf(cbuf, sizeof(cbuf), "%05d, %05d, %05d", it->nExists, it->nSpends, it->nLeastDepth);
        result.push_back(Pair(cbuf, ValueFromAmount(it->nValue)));


        nTotalIn += it->nValue * it->nExists;
        nTotalOut += it->nValue * it->nSpends;
        nTotalCoins += it->nExists;
    };

    result.push_back(Pair("total anon value in", ValueFromAmount(nTotalIn)));
    result.push_back(Pair("total anon value out", ValueFromAmount(nTotalOut)));
    result.push_back(Pair("total anon outputs", nTotalCoins));

    return result;
}

Value reloadanondata(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "reloadanondata \n"
            "clears all anon txn data from system, and runs scanforalltxns.\n"
            "WARNING: Intended for development use only."
            + HelpRequiringPassphrase());

    if (nNodeMode != NT_FULL)
        throw runtime_error("Must be in full mode.");


    CBlockIndex *pindex = pindexGenesisBlock;
    
    
    Object result;
    if (pindex)
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        if (!pwalletMain->EraseAllAnonData())
            throw runtime_error("EraseAllAnonData() failed.");

        pwalletMain->MarkDirty();
        pwalletMain->ScanForWalletTransactions(pindex, true);
        pwalletMain->ReacceptWalletTransactions();

        pwalletMain->CacheAnonStats();
        result.push_back(Pair("result", "reloadanondata complete."));
    } else
    {
        result.push_back(Pair("result", "reloadanondata failed - !pindex."));
    }
    
    return result;
}


static bool compareTxnTime(const CWalletTx* pa, const CWalletTx* pb)
{
    return pa->nTime < pb->nTime;
};

Value txnreport(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "txnreport [collate_amounts] [show_key_images]\n"
            "List transactions at output level.\n");
    
    bool fCollateAmounts = false;
    bool fShowKeyImage = false;
    
    // TODO: trust CWalletTx::vfSpent?
    
    if (params.size() > 0)
    {
        std::string value = params[0].get_str();
        if (IsStringBoolPositive(value))
            fCollateAmounts = true;
    };
    
    if (params.size() > 1)
    {
        std::string value = params[1].get_str();
        if (IsStringBoolPositive(value))
            fShowKeyImage = true;
    };
    
    int64_t nWalletIn = 0;      // total inputs from owned addresses
    int64_t nWalletOut = 0;     // total outputs from owned addresses
    
    Object result;
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        
        std::list<CWalletTx*> listOrdered;
        for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            if (it->second.GetDepthInMainChain() > 0) // exclude txns not in the chain
                listOrdered.push_back(&it->second);
        };
        
        listOrdered.sort(compareTxnTime);
        
        std::list<CWalletTx*>::iterator it;
        
        Array headings;
        headings.push_back("When");
        headings.push_back("Txn Hash");
        headings.push_back("In/Output type");
        
        headings.push_back("Address");
        headings.push_back("Ring Size");
        
        if (fShowKeyImage)
            headings.push_back("Key Image");
        
        headings.push_back("Owned");
        headings.push_back("Spent");
        
        headings.push_back("Value In");
        headings.push_back("Value Out");
        
        if (fCollateAmounts)
        {
            headings.push_back("Wallet In");
            headings.push_back("Wallet Out");
        };
        
        result.push_back(Pair("headings", headings));
        
        if (pwalletMain->IsLocked())
        {
            result.push_back(Pair("warning", "Wallet is locked - owned inputs may not be detected correctly."));
        };
        
        Array lines;
        
        CTxDB txdb("r");
        CWalletDB walletdb(pwalletMain->strWalletFile, "r");
        
        char cbuf[256];
        for (it = listOrdered.begin(); it != listOrdered.end(); ++it)
        {
            CWalletTx* pwtx = (*it);
            
            Array entryTxn;
            entryTxn.push_back(getTimeString(pwtx->nTime, cbuf, sizeof(cbuf)));
            entryTxn.push_back(pwtx->GetHash().GetHex());
            
            for (uint32_t i = 0; i < pwtx->vin.size(); ++i)
            {
                const CTxIn& txin = pwtx->vin[i];
                
                int64_t nInputValue = 0;
                
                Array entry = entryTxn;
                
                std::string sAddr = "";
                std::string sKeyImage = "";
                bool fOwnCoin = false;
                int nRingSize = 0;
                if (pwtx->nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput())
                {
                    entry.push_back("gameunitsx in");
                    std::vector<uint8_t> vchImage;
                    txin.ExtractKeyImage(vchImage);
                    nRingSize = txin.ExtractRingSize();
                    
                    sKeyImage = HexStr(vchImage);
                    
                    CKeyImageSpent ski;
                    bool fInMemPool;
                    if (GetKeyImage(&txdb, vchImage, ski, fInMemPool))
                        nInputValue = ski.nValue;
                    
                    COwnedAnonOutput oao;
                    if (walletdb.ReadOwnedAnonOutput(vchImage, oao))
                    {
                        fOwnCoin = true;
                    } else
                    if (pwalletMain->IsCrypted())
                    {
                        // - tokens received with locked wallet won't have oao until wallet unlocked
                        //   No way to tell if locked input is owned
                        //   need vchImage
                        
                        // TODO, closest would be to tell if it's possible for the input to be owned
                        sKeyImage = "locked?";
                    };
                    
                } else
                {
                    entry.push_back("gameunits in");
                    
                    if (pwalletMain->IsMine(txin))
                        fOwnCoin = true;
                    
                    CTransaction prevTx;
                    if (txdb.ReadDiskTx(txin.prevout.hash, prevTx))
                    {
                        if (txin.prevout.n < prevTx.vout.size())
                        {
                            const CTxOut &vout = prevTx.vout[txin.prevout.n];
                            nInputValue = vout.nValue;
                            
                            CTxDestination address;
                            if (ExtractDestination(vout.scriptPubKey, address))
                                sAddr = CBitcoinAddress(address).ToString();
                        } else
                        {
                            nInputValue = 0;
                        };
                    };
                };
                
                if (fOwnCoin)
                    nWalletIn += nInputValue;
                
                
                entry.push_back(sAddr);
                entry.push_back(nRingSize == 0 ? "" : strprintf("%d", nRingSize));
                
                if (fShowKeyImage)
                    entry.push_back(sKeyImage);
                
                entry.push_back(fOwnCoin);
                entry.push_back(""); // spent
                entry.push_back(strprintf("%f", (double)nInputValue / (double)COIN));
                entry.push_back(""); // out
                
                if (fCollateAmounts)
                {
                    entry.push_back(strprintf("%f", (double)nWalletIn / (double)COIN));
                    entry.push_back(strprintf("%f", (double)nWalletOut / (double)COIN));
                };
                
                lines.push_back(entry);
            };
            
            for (unsigned int i = 0; i < pwtx->vout.size(); i++)
            {
                const CTxOut& txout = pwtx->vout[i];
                
                if (txout.nValue < 1) // metadata output, narration or stealth
                    continue;
                
                Array entry = entryTxn;
                
                
                std::string sAddr = "";
                std::string sKeyImage = "";
                bool fOwnCoin = false;
                bool fSpent = false;
                
                if (pwtx->nVersion == ANON_TXN_VERSION
                    && txout.IsAnonOutput())
                {
                    entry.push_back("gameunitsx out");
                    
                    CPubKey pkCoin    = txout.ExtractAnonPk();
                    
                    std::vector<uint8_t> vchImage;
                    COwnedAnonOutput oao;
                    
                    if (walletdb.ReadOwnedAnonOutputLink(pkCoin, vchImage)
                        && walletdb.ReadOwnedAnonOutput(vchImage, oao))
                    {
                        sKeyImage = HexStr(vchImage);
                        fOwnCoin = true;
                    } else
                    if (pwalletMain->IsCrypted())
                    {
                        // - tokens received with locked wallet won't have oao until wallet unlocked
                        CKeyID ckCoinId = pkCoin.GetID();
                        
                        CLockedAnonOutput lockedAo;
                        if (walletdb.ReadLockedAnonOutput(ckCoinId, lockedAo))
                            fOwnCoin = true;
                        
                        sKeyImage = "locked?";
                    };
                } else
                {
                    entry.push_back("gameunits out");
                    
                    CTxDestination address;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        sAddr = CBitcoinAddress(address).ToString();
                    
                    if (pwalletMain->IsMine(txout))
                        fOwnCoin = true;
                };
                
                if (fOwnCoin)
                {
                    nWalletOut += txout.nValue;
                    fSpent = pwtx->IsSpent(i);
                };
                
                entry.push_back(sAddr);
                
                entry.push_back(""); // ring size (only for inputs)
                
                if (fShowKeyImage)
                    entry.push_back(sKeyImage);
                
                entry.push_back(fOwnCoin);
                entry.push_back(fSpent);
                
                entry.push_back(""); // in
                entry.push_back(ValueFromAmount(txout.nValue));
                
                if (fCollateAmounts)
                {
                    entry.push_back(strprintf("%f", (double)nWalletIn / (double)COIN));
                    entry.push_back(strprintf("%f", (double)nWalletOut / (double)COIN));
                };
                
                lines.push_back(entry);
            };
        };
        result.push_back(Pair("data", lines));
    }
    
    
    result.push_back(Pair("result", "txnreport complete."));
    return result;
}


