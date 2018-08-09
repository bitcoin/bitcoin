// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_MESSAGE_H
#define BITCOIN_NET_MESSAGE_H

#include <hash.h>
#include <protocol.h>
#include <streams.h>
#include <uint256.h>

/** Maximum length of incoming protocol messages (no message over 4 MB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 4 * 1000 * 1000;

// base class for format agnostic network messages
class NetMessageBase
{
public:
    CDataStream vRecv; // received message data
    int64_t nTime;     // time (in microseconds) of message receipt.

    NetMessageBase(int nTypeIn, int nVersionIn) : vRecv(nTypeIn, nVersionIn)
    {
        nTime = 0;
    }
    virtual ~NetMessageBase() {}

    virtual bool Complete() const = 0;
    virtual uint32_t GetMessageSize() const = 0;           //returns 0 when message has not yet been parsed
    virtual uint32_t GetMessageSizeWithHeader() const = 0; //return complete message size including header

    virtual std::string GetCommandName() const = 0; //returns the command name. Returns an empty string when no command name is supported

    virtual void SetVersion(int nVersionIn)
    {
        vRecv.SetVersion(nVersionIn);
    }

    virtual int Read(const char* pch, unsigned int nBytes) = 0; //parse bytes

    virtual bool VerifyMessageStart() const = 0;
    virtual bool VerifyHeader() const = 0;
    virtual bool VerifyChecksum(std::string& error) const = 0;
};

//basic network message for the currently used unencrypted p2p communication
class NetMessage : public NetMessageBase
{
private:
    mutable CHash256 hasher;
    mutable uint256 data_hash;
public:
    bool in_data;                   // parsing header (false) or data (true)

    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    unsigned int nHdrPos;

    unsigned int nDataPos;

    NetMessage(const CMessageHeader::MessageStartChars& pchMessageStartIn, int nTypeIn, int nVersionIn) : NetMessageBase(nTypeIn, nVersionIn), hdrbuf(nTypeIn, nVersionIn), hdr(pchMessageStartIn)
    {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
    }

    bool Complete() const override
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }

    uint32_t GetMessageSize() const override
    {
        if (!in_data) {
            return 0;
        }
        return hdr.nMessageSize;
    }

    uint32_t GetMessageSizeWithHeader() const override
    {
        return hdr.nMessageSize + CMessageHeader::HEADER_SIZE;
    }

    std::string GetCommandName() const override
    {
        return hdr.pchCommand;
    }

    const uint256& GetMessageHash() const;

    void SetVersion(int nVersionIn) override
    {
        hdrbuf.SetVersion(nVersionIn);
        NetMessageBase::SetVersion(nVersionIn);
    }

    int Read(const char* pch, unsigned int nBytes) override;
    int ReadHeader(const char *pch, unsigned int nBytes);
    int ReadData(const char *pch, unsigned int nBytes);

    bool VerifyMessageStart() const override;
    bool VerifyHeader() const override;
    bool VerifyChecksum(std::string& error) const override;
};

using NetMessageBaseRef = std::unique_ptr<NetMessageBase>;

#endif // BITCOIN_NET_MESSAGE_H
