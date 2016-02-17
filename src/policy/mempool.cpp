// Copyright (c) 2015-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/mempool.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

MempoolPolicy mempoolPolicy;

void MempoolPolicy::InitFromArgs()
{
}

std::vector<std::pair<std::string, std::string> > MempoolPolicy::GetOptionsHelp(bool showDebug) const
{
    std::vector<std::pair<std::string, std::string> > optionsHelp;
    return optionsHelp;
}

void MempoolPolicy::AppendHelpMessages(std::string& strUsage, bool showDebug)
{
    strUsage += HelpMessageGroup(_("Mempool Policy options:"));
    AppendMessagesOpt(strUsage, GetOptionsHelp(showDebug));
}
