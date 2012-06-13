// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinrpc.h"
#include "filter.h"
#include "util.h"
#include "sync.h"
#include "main.h"

using namespace std;
using namespace json_spirit;

static CBloomers bloomers;
static CCriticalSection cs_lock;
static string strNotifyTx;
static string strNotifyBlock;

Value filterclearall(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "filterclearall\n"
            "Clear all filters.");

    LOCK(cs_lock);
    bloomers.clearall();

    return true;
}

Value filterclear(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "filterclear <name>\n"
            "Clear filter named <name>.");

    LOCK(cs_lock);
    bloomers.clear(params[0].get_str());

    return true;
}

Value filteradd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "filteradd <name> <hash hexstring>\n"
            "Add <hash> to filter <name>.");

    LOCK(cs_lock);
    bloomers.add(params[0].get_str(), ParseHex(params[1].get_str()));

    return true;
}

static bool FilterMatchTxOut(const CTxOut& txout)
{
    txnouttype type;
    int nRequired;
    vector<CBitcoinAddress> addresses;

    if (!ExtractAddresses(txout.scriptPubKey, type, addresses, nRequired))
        return false;

    for (unsigned int i = 0; i < addresses.size(); i++) {
        uint160 hash = addresses[i].GetHash160();
        vector<string> rv = bloomers.contains(string(hash.begin(), hash.end()));
        if (rv.size() > 0)
            return true;
    }

    return false;
}

static bool __FilterMatchTx(const CTransaction& tx)
{
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        if (FilterMatchTxOut(txout))
            return true;
    }

    return false;
}

static bool FilterMatchTx(const CTransaction& tx)
{
    LOCK(cs_lock);
    return __FilterMatchTx(tx);
}

static bool FilterMatchBlock(const CBlock& block)
{
    LOCK(cs_lock);

    BOOST_FOREACH(const CTransaction& tx, block.vtx) {
        if (__FilterMatchTx(tx))
            return true;
    }

    return false;
}

void FilterNotifyTx(const CTransaction& tx)
{
    if (strNotifyTx.empty())
        return;

    if (!FilterMatchTx(tx))
        return;

    string strCmd(strNotifyTx);
    boost::replace_all(strCmd, "%t", "tx");
    boost::replace_all(strCmd, "%h", tx.GetHash().GetHex());

    printf("FILTER(tx) exec: %s\n", strCmd.c_str());
    boost::thread t(runCommand, strCmd); // thread runs free
}

void FilterNotifyBlock(const CBlock& block)
{
    if (strNotifyBlock.empty())
        return;

    if (!FilterMatchBlock(block))
        return;

    string strCmd(strNotifyBlock);
    boost::replace_all(strCmd, "%t", "block");
    boost::replace_all(strCmd, "%h", block.GetHash().GetHex());

    printf("FILTER(block) exec: %s\n", strCmd.c_str());
    boost::thread t(runCommand, strCmd); // thread runs free
}

void FilterSetup()
{
    strNotifyTx = GetArg("-filtertx", "");
    if (strNotifyTx.empty())
        strNotifyTx = GetArg("-filter", "");

    strNotifyBlock = GetArg("-filterblock", "");
    if (strNotifyBlock.empty())
        strNotifyBlock = GetArg("-filter", "");
}

