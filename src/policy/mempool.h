// Copyright (c) 2015-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMPOOLPOLICY_H
#define BITCOIN_MEMPOOLPOLICY_H

#include <string>
#include <vector>

#include "amount.h"

//Mempool policy defaults
static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000;

/**
 * \class MempoolPolicy
 * Encapsulate parameters needed for mempool policy
 */
class MempoolPolicy
{
public:
    MempoolPolicy();
    void InitFromArgs();
    /**
     * @return a formatted HelpMessage string with the mempool policy options
     */
    std::vector<std::pair<std::string, std::string> > GetOptionsHelp(bool showDebug) const;
    /**
     * Append a help string for the options of the selected policy.
     * @param strUsage a formatted HelpMessage string with mempool policy
     * options appended to the string
     */
    void AppendHelpMessages(std::string& strUsage, bool showDebug);

    /** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
    CFeeRate GetMinRelayFeeRate();
    /** Setter for testing */
    void SetMinRelayFeeRate(CFeeRate newFeeRate);
private:
    CFeeRate minRelayFeeRate;
};

extern MempoolPolicy mempoolPolicy;

#endif // BITCOIN_MEMPOOLPOLICY_H
