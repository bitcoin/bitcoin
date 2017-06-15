// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETADDRESS_H
#define BITCOIN_NETADDRESS_H

#include <stdint.h>

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "netbase.h" // for CNetAddr
#include "serialize.h"

class CSubNet
{
protected:
    /// Network (base) address
    CNetAddr network;
    /// Netmask, in network byte order
    uint8_t netmask[16];
    /// Is this value valid? (only used to signal parse errors)
    bool valid;

public:
    CSubNet();
    explicit CSubNet(const std::string &strSubnet);

    // constructor for single ip subnet (<ipv4>/32 or <ipv6>/128)
    explicit CSubNet(const CNetAddr &addr);

    bool Match(const CNetAddr &addr) const;

    std::string ToString() const;
    bool IsValid() const;

    friend bool operator==(const CSubNet &a, const CSubNet &b);
    friend bool operator!=(const CSubNet &a, const CSubNet &b);
    friend bool operator<(const CSubNet &a, const CSubNet &b);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(network);
        READWRITE(FLATDATA(netmask));
        READWRITE(FLATDATA(valid));
    }
};


#endif
