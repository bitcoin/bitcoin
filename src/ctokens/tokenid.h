// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CTOKENS_TOKENID_H
#define BITCOIN_CTOKENS_TOKENID_H

#include <uint256.h>
#include <streams.h>

class TokenId
{
public:
    uint256 token;
    uint64_t subid;

    TokenId(const uint256& t = uint256(), const uint64_t& i = std::numeric_limits<uint64_t>::max()) : token(t), subid(i){}

    void SetNull() { token = uint256(); subid = -1; }

    bool IsNull() const { return token == uint256() && subid == std::numeric_limits<uint64_t>::max(); }

    friend bool operator==(const TokenId& a, const TokenId& b) { return a.token == b.token && a.subid == b.subid; }

    friend bool operator<(const TokenId& a, const TokenId& b) {
        if (a.token == b.token)
        {
            return a.subid < b.subid;
        }
        return a.token < b.token;
    }

    friend bool operator!=(const TokenId& a, const TokenId& b) { return !(a == b); }

    // TODO add definition of this and uncomment
    // ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(token);
        READWRITE(subid);
    }
};

#endif // BITCOIN_CTOKENS_TOKENID_H
