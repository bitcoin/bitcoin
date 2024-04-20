// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_URL_H
#define BITCOIN_COMMON_URL_H

#include <string>
#include <string_view>

using UrlDecodeFn = std::string(std::string_view url_encoded);
UrlDecodeFn urlDecode;
extern UrlDecodeFn* const URL_DECODE;

#endif // BITCOIN_COMMON_URL_H
