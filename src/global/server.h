// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GLOBALS_SERVER_H
#define BITCOIN_GLOBALS_SERVER_H

#include "consensus/interfaces.h"

// This files contains globals used in the server package (but not on
// lower layer packages like common).

/** A concurrent implementation of Consensus::CVersionBitsCacheInterface. */
class CVersionBitsCacheImplementation : public Consensus::CVersionBitsCacheInterface
{
public:
    CVersionBitsCacheImplementation();
    ~CVersionBitsCacheImplementation();
};

namespace Global {

/** A global cache for versionbits calculation */
extern CVersionBitsCacheImplementation versionBitsStateCache;

} // namespace Global

#endif // BITCOIN_GLOBALS_SERVER_H
