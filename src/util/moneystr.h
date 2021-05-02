// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Money parsing/formatting utilities.
 */
#ifndef XBIT_UTIL_MONEYSTR_H
#define XBIT_UTIL_MONEYSTR_H

#include <amount.h>
#include <attributes.h>

#include <string>

/* Do not use these functions to represent or parse monetary amounts to or from
 * JSON but use AmountFromValue and ValueFromAmount for that.
 */
std::string FormatMoney(const CAmount& n);
/** Parse an amount denoted in full coins. E.g. "0.0034" supplied on the command line. **/
NODISCARD bool ParseMoney(const std::string& str, CAmount& nRet);

#endif // XBIT_UTIL_MONEYSTR_H
