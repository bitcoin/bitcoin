// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WARNINGS_H
#define BITCOIN_WARNINGS_H

#include <stdlib.h>
#include <string>

void SetMiscWarning(const std::string& strWarning);
void SetfLargeWorkForkFound(bool flag);
bool GetfLargeWorkForkFound();
void SetfLargeWorkInvalidChainFound(bool flag);
/** Format a string that describes several potential detected problems.
 * If htmlMultiline is set to true, this functiomn returns multiple (and translated) warnings seperated by a <hr> tag.
 * Otherwise only the highest priority warning will be returned.
 */
std::string GetWarnings(bool htmlMultiline = false);

static const bool DEFAULT_TESTSAFEMODE = false;

#endif //  BITCOIN_WARNINGS_H
