// Copyright (c) 2015-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/mempool.h"

#include "tinyformat.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"

MempoolPolicy mempoolPolicy;

MempoolPolicy::MempoolPolicy()
{
    minRelayFeeRate = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);
}

void MempoolPolicy::InitFromArgs()
{
    // Fee-per-kilobyte amount considered the same as "free"
    // If you are mining, be careful setting this:
    // if you set it to zero then
    // a transaction spammer can cheaply fill blocks using
    // 1-satoshi-fee transactions. It should be set above the real
    // cost to you of processing a transaction.
    if (mapArgs.count("-minrelaytxfee"))
    {
        CAmount n = 0;
        if (ParseMoney(mapArgs["-minrelaytxfee"], n) && n > 0) {
            minRelayFeeRate = CFeeRate(n);
        }
        else
            throw std::runtime_error(AmountErrMsg("minrelaytxfee", mapArgs["-minrelaytxfee"]));
    }
}

std::vector<std::pair<std::string, std::string> > MempoolPolicy::GetOptionsHelp(bool showDebug) const
{
    std::vector<std::pair<std::string, std::string> > optionsHelp;
    optionsHelp.push_back(std::make_pair("-minrelaytxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)"),
                                                                           CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE))));
    return optionsHelp;
}

void MempoolPolicy::AppendHelpMessages(std::string& strUsage, bool showDebug)
{
    strUsage += HelpMessageGroup(_("Mempool Policy options:"));
    AppendMessagesOpt(strUsage, GetOptionsHelp(showDebug));
}

CFeeRate MempoolPolicy::GetMinRelayFeeRate()
{
    return minRelayFeeRate;
}

void MempoolPolicy::SetMinRelayFeeRate(CFeeRate newFeeRate)
{
    minRelayFeeRate = newFeeRate;
}
