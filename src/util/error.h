// Copyright (c) 2010-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_ERROR_H
#define BITCOIN_UTIL_ERROR_H

/**
 * util/error.h is a common place for definitions of simple error types and
 * string functions. Types and functions defined here should not require any
 * outside dependencies.
 *
 * Error types defined here can be used in different parts of the
 * codebase, to avoid the need to write boilerplate code catching and
 * translating errors passed across wallet/node/rpc/gui code boundaries.
 */

#include <node/types.h>
#include <string>

struct bilingual_str;

bilingual_str TransactionErrorString(const TransactionError error);

bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind);

bilingual_str InvalidPortErrMsg(const std::string& optname, const std::string& strPort);

bilingual_str AmountHighWarn(const std::string& optname);

bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue);

#endif // BITCOIN_UTIL_ERROR_H
