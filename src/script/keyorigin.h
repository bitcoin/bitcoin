// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_KEYORIGIN_H
#define BITCOIN_SCRIPT_KEYORIGIN_H

#include <serialize.h>
#include <vector>

struct KeyOriginInfo
{
    unsigned char fingerprint[4]; //!< First 32 bits of the Hash160 of the public key at the root of the path
    std::vector<uint32_t> path;

    friend bool operator==(const KeyOriginInfo& a, const KeyOriginInfo& b)
    {
        return std::equal(std::begin(a.fingerprint), std::end(a.fingerprint), std::begin(b.fingerprint)) && a.path == b.path;
    }

    SERIALIZE_METHODS(KeyOriginInfo, obj) { READWRITE(obj.fingerprint, obj.path); }

    void clear()
    {
        memset(fingerprint, 0, 4);
        path.clear();
    }
};

#endif // BITCOIN_SCRIPT_KEYORIGIN_H
