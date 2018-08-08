// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_ENCRYPTION_H
#define BITCOIN_NET_ENCRYPTION_H

#include <memory>

#include <key.h>
#include <net_message.h>
#include <streams.h>
#include <uint256.h>


class EncryptionHandlerInterface
{
public:
    virtual ~EncryptionHandlerInterface() {}

    virtual bool GetLength(CDataStream& data_in, uint32_t& len_out) = 0;
    virtual bool EncryptAppendMAC(std::vector<unsigned char>& data_in_out) = 0;
    virtual bool AuthenticatedAndDecrypt(CDataStream& data_in_out) = 0;
    virtual bool ShouldCryptMsg() = 0;

    virtual unsigned int GetTagLen() const = 0;
    virtual unsigned int GetAADLen() const = 0;
};

class P2PEncryption : public EncryptionHandlerInterface
{
private:
    static constexpr unsigned int TAG_LEN = 16; /* poly1305 128bit MAC tag */
    static constexpr unsigned int AAD_LEN = 3;  /* 24 bit payload length */

public:
    bool GetLength(CDataStream& data_in, uint32_t& len_out) override;
    bool EncryptAppendMAC(std::vector<unsigned char>& data_in_out) override;
    bool AuthenticatedAndDecrypt(CDataStream& data_in_out) override;

    bool ShouldCryptMsg() override;

    unsigned inline int GetTagLen() const override
    {
        return TAG_LEN;
    }

    unsigned inline int GetAADLen() const override
    {
        return AAD_LEN;
    }
};
typedef std::shared_ptr<EncryptionHandlerInterface> EncryptionHandlerRef;

//encrypted network message after v2 p2p message transport protocol
class NetV2Message : public NetMessageBase
{
public:
    bool m_in_data;
    uint32_t m_message_size;
    unsigned int m_hdr_pos;
    uint32_t m_data_pos;
    std::string m_command_name;

    EncryptionHandlerRef m_encryption_handler;

    NetV2Message(EncryptionHandlerRef encryption_handler, const CMessageHeader::MessageStartChars& pchMessageStartIn, int nTypeIn, int nVersionIn)
        : NetMessageBase(nTypeIn, nVersionIn),
          m_encryption_handler(encryption_handler)
    {
        // resize the message buffer to the AADlen (3 byte packet size v2 protocol)
        vRecv.resize(m_encryption_handler->GetAADLen());
        m_message_size = 0;
        m_hdr_pos = 0;
        m_data_pos = 0;
        m_in_data = 0;
        m_type = NetMessageType::ENCRYPTED_MSG;
        m_command_name.clear();
    }

    bool Complete() const override
    {
        if (!m_in_data) {
            return false;
        }
        return (m_message_size + m_encryption_handler->GetTagLen() == m_data_pos);
    }

    uint32_t GetMessageSize() const override
    {
        return m_message_size; //is size(strCommand & playload), where v1 is only size(playload)
    }

    uint32_t GetMessageSizeWithHeader() const override
    {
        return m_message_size + sizeof(m_message_size) + m_encryption_handler->GetAADLen() + m_encryption_handler->GetTagLen();
    }

    std::string GetCommandName() const override
    {
        return m_command_name;
    }

    int Read(const char* pch, unsigned int nBytes) override;

    bool VerifyMessageStart() const override { return true; }
    bool VerifyHeader() const override { return true; }
    bool VerifyChecksum(std::string& error) const override { return true; }
};

#endif // BITCOIN_NET_ENCRYPTION_H
