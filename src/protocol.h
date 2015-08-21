// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __cplusplus
#error This header can only be compiled as C++.
#endif

#ifndef BITCOIN_PROTOCOL_H
#define BITCOIN_PROTOCOL_H

#include "netbase.h"
#include "serialize.h"
#include "uint256.h"
#include "version.h"

#include <stdint.h>
#include <string>

#define MESSAGE_START_SIZE 4

/** Message header.
 * (4) message start.
 * (12) command.
 * (4) size.
 * (4) checksum.
 */
class CMessageHeader
{
public:
    typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

    CMessageHeader(const MessageStartChars& pchMessageStartIn);
    CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn);

    std::string GetCommand() const;
    bool IsValid(const MessageStartChars& messageStart) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(FLATDATA(pchMessageStart));
        READWRITE(FLATDATA(pchCommand));
        READWRITE(nMessageSize);
        READWRITE(nChecksum);
    }

    // TODO: make private (improves encapsulation)
public:
    enum {
        COMMAND_SIZE = 12,
        MESSAGE_SIZE_SIZE = sizeof(int),
        CHECKSUM_SIZE = sizeof(int),

        MESSAGE_SIZE_OFFSET = MESSAGE_START_SIZE + COMMAND_SIZE,
        CHECKSUM_OFFSET = MESSAGE_SIZE_OFFSET + MESSAGE_SIZE_SIZE,
        HEADER_SIZE = MESSAGE_START_SIZE + COMMAND_SIZE + MESSAGE_SIZE_SIZE + CHECKSUM_SIZE
    };
    char pchMessageStart[MESSAGE_START_SIZE];
    char pchCommand[COMMAND_SIZE];
    unsigned int nMessageSize;
    unsigned int nChecksum;
};

/** nServices flags */
enum {
    // NODE_NETWORK means that the node is capable of serving the block chain. It is currently
    // set by all Bitcoin Core nodes, and is unset by SPV clients or other peers that just want
    // network services but don't provide them.
    NODE_NETWORK = (1 << 0),
    // NODE_GETUTXO means the node is capable of responding to the getutxo protocol request.
    // Bitcoin Core does not support this but a patch set called Bitcoin XT does.
    // See BIP 64 for details on how this is implemented.
    NODE_GETUTXO = (1 << 1),
    // NODE_BLOOM means the node is capable and willing to handle bloom-filtered connections.
    // Bitcoin Core nodes used to support this by default, without advertising this bit,
    // but no longer do as of protocol version 70011 (= NO_BLOOM_VERSION)
    NODE_BLOOM = (1 << 2),

    // Bits 24-31 are reserved for temporary experiments. Just pick a bit that
    // isn't getting used, or one not being used much, and notify the
    // bitcoin-development mailing list. Remember that service bits are just
    // unauthenticated advertisements, so your code must be robust against
    // collisions and other cases where nodes may be advertising a service they
    // do not actually support. Other service bits should be allocated via the
    // BIP process.
};

/** A CService with information about it as peer */
class CAddress : public CService
{
public:
    CAddress();
    explicit CAddress(CService ipIn, uint64_t nServicesIn = NODE_NETWORK);

    void Init();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (ser_action.ForRead())
            Init();
        if (nType & SER_DISK)
            READWRITE(nVersion);
        if ((nType & SER_DISK) ||
            (nVersion >= CADDR_TIME_VERSION && !(nType & SER_GETHASH)))
            READWRITE(nTime);
        READWRITE(nServices);
        READWRITE(*(CService*)this);
    }

    // TODO: make private (improves encapsulation)
public:
    uint64_t nServices;

    // disk and network only
    unsigned int nTime;
};

/** inv message data */
class CInv
{
public:
    CInv();
    CInv(int typeIn, const uint256& hashIn);
    CInv(const std::string& strType, const uint256& hashIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(type);
        READWRITE(hash);
    }

    friend bool operator<(const CInv& a, const CInv& b);

    bool IsKnownType() const;
    const char* GetCommand() const;
    std::string ToString() const;

    // TODO: make private (improves encapsulation)
public:
    int type;
    uint256 hash;
};

enum {
    MSG_TX = 1,
    MSG_BLOCK,
    // Nodes may always request a MSG_FILTERED_BLOCK in a getdata, however,
    // MSG_FILTERED_BLOCK should not appear in any invs except as a part of getdata.
    MSG_FILTERED_BLOCK,
};

#endif // BITCOIN_PROTOCOL_H
