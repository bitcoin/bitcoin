// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/common.h>

#include <core_io.h>
#include <univalue.h>

namespace CoinJoin
{
std::string DenominationToString(int nDenom)
{
    switch (CAmount nDenomAmount = DenominationToAmount(nDenom)) {
        case  0: return "N/A";
        case -1: return "out-of-bounds";
        case -2: return "non-denom";
        case -3: return "to-amount-error";
        default: return ValueFromAmount(nDenomAmount).getValStr();
    }

    // shouldn't happen
    return "to-string-error";
}

} // namespace CoinJoin
