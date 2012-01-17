// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef __cplusplus
# error This header can only be compiled as C++.
#endif

#ifndef __INCLUDED_PROTOCOL_H__
#define __INCLUDED_PROTOCOL_H__

#include "serialize.h"
#include "netbase.h"
#include "util.h"
#include <string>
#include "uint256.h"

extern bool fTestNet;
static inline unsigned short GetDefaultPort(const bool testnet = fTestNet)
{
    return testnet ? 18333 : 8333;
}

//
// Message header
//  (4) message start
//  (12) command
//  (4) size
//  (4) checksum

extern unsigned char pchMessageStart[4];

class CMessageHeader
{
    public:
        CMessageHeader();
        CMessageHeader(const char* pszCommand, unsigned int nMessageSizeIn);

        std::string GetCommand() const;
        bool IsValid() const;

        IMPLEMENT_SERIALIZE
            (
             READWRITE(FLATDATA(pchMessageStart));
             READWRITE(FLATDATA(pchCommand));
             READWRITE(nMessageSize);
             if (nVersion >= 209)
             READWRITE(nChecksum);
            )

    // TODO: make private (improves encapsulation)
    public:
        enum { COMMAND_SIZE=12 };
        char pchMessageStart[sizeof(::pchMessageStart)];
        char pchCommand[COMMAND_SIZE];
        unsigned int nMessageSize;
        unsigned int nChecksum;
};

enum
{
    NODE_NETWORK = (1 << 0),
};

class CAddress : public CService
{
    public:
        CAddress();
        explicit CAddress(CService ipIn, uint64 nServicesIn=NODE_NETWORK);

        void Init();

        IMPLEMENT_SERIALIZE
            (
             CAddress* pthis = const_cast<CAddress*>(this);
             CService* pip = (CService*)pthis;
             if (fRead)
                 pthis->Init();
             if (nType & SER_DISK)
             READWRITE(nVersion);
             if ((nType & SER_DISK) || (nVersion >= 31402 && !(nType & SER_GETHASH)))
             READWRITE(nTime);
             READWRITE(nServices);
             READWRITE(*pip);
            )

        void print() const;

    // TODO: make private (improves encapsulation)
    public:
        uint64 nServices;

        // disk and network only
        unsigned int nTime;

        // memory only
        unsigned int nLastTry;
};

class CInv
{
    public:
        CInv();
        CInv(int typeIn, const uint256& hashIn);
        CInv(const std::string& strType, const uint256& hashIn);

        IMPLEMENT_SERIALIZE
        (
            READWRITE(type);
            READWRITE(hash);
        )

        friend bool operator<(const CInv& a, const CInv& b);

        bool IsKnownType() const;
        const char* GetCommand() const;
        std::string ToString() const;
        void print() const;

    // TODO: make private (improves encapsulation)
    public:
        int type;
        uint256 hash;
};

#endif // __INCLUDED_PROTOCOL_H__
