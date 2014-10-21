// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EC_CRYPTO_VERIFY_H
#define BITCOIN_EC_CRYPTO_VERIFY_H

#include <vector>
#include <cstdlib>
class uint256;

namespace eccrypto {

bool Check(const unsigned char *vch);
bool CheckSignatureElement(const unsigned char *vch, int len, bool half);

} // eccrypto namespace
#endif
