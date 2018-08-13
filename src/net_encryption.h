// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_ENCRYPTION_H
#define BITCOIN_NET_ENCRYPTION_H

#include <atomic>
#include <memory>

#include <crypto/chacha_poly_aead.h>
#include <key.h>
#include <net_message.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>


class EncryptionHandlerInterface
{
public:
    virtual ~EncryptionHandlerInterface() {}

    virtual bool GetHandshakeRequestData(std::vector<unsigned char>& handshake_data) = 0;
    virtual bool ProcessHandshakeRequestData(const std::vector<unsigned char>& handshake_data) = 0;

    virtual bool GetLength(CDataStream& data_in, uint32_t& len_out) = 0;
    virtual bool EncryptAppendMAC(std::vector<unsigned char>& data_in_out) = 0;
    virtual bool AuthenticatedAndDecrypt(CDataStream& data_in_out) = 0;
    virtual bool ShouldCryptMsg() = 0;

    virtual unsigned int GetTagLen() const = 0;
    virtual unsigned int GetAADLen() const = 0;
    virtual void EnableEncryption(bool inbound) = 0;
    virtual uint256 GetSessionID() = 0;
};

class P2PEncryption : public EncryptionHandlerInterface
{
private:
    static constexpr unsigned int TAG_LEN = 16; /* poly1305 128bit MAC tag */
    static constexpr unsigned int AAD_LEN = 3;  /* 24 bit payload length */
    CKey m_ecdh_key;
    CPrivKey m_raw_ecdh_secret;
    uint256 m_session_id;

    std::atomic_bool handshake_done;

    CCriticalSection cs;
    std::unique_ptr<ChaCha20Poly1305AEAD> m_send_aead_ctx;
    std::unique_ptr<ChaCha20Poly1305AEAD> m_recv_aead_ctx;
    uint32_t m_recv_seq_nr = 0;
    uint32_t m_recv_seq_nr_aad = 0;
    int m_recv_aad_keystream_pos = 0;
    uint32_t m_send_seq_nr = 0;
    uint32_t m_send_seq_nr_aad = 0;
    int m_send_aad_keystream_pos = 0;

public:
    P2PEncryption();
    ~P2PEncryption()
    {
        memory_cleanse(&m_send_aead_ctx, sizeof(m_send_aead_ctx));
        memory_cleanse(&m_recv_aead_ctx, sizeof(m_recv_aead_ctx));
    }
    bool GetHandshakeRequestData(std::vector<unsigned char>& handshake_data) override;
    bool ProcessHandshakeRequestData(const std::vector<unsigned char>& handshake_data) override;

    bool GetLength(CDataStream& data_in, uint32_t& len_out) override;
    bool EncryptAppendMAC(std::vector<unsigned char>& data_in_out) override;
    bool AuthenticatedAndDecrypt(CDataStream& data_in_out) override;

    bool ShouldCryptMsg() override;
    void EnableEncryption(bool inbound) override;
    uint256 GetSessionID() override;

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

//network message for 32byte encryption handshake with fallback option
class NetMessageEncryptionHandshake : public NetMessageBase
{
public:
    unsigned int m_data_pos;

    NetMessageEncryptionHandshake(const CMessageHeader::MessageStartChars& pchMessageStartIn, int nTypeIn, int nVersionIn) : NetMessageBase(nTypeIn, nVersionIn)
    {
        m_data_pos = 0;
        m_type = NetMessageType::PLAINTEXT_ENCRYPTION_HANDSHAKE;
    }

    bool Complete() const override
    {
        return (m_data_pos == 32);
    }

    uint32_t GetMessageSize() const override
    {
        return vRecv.size();
    }

    uint32_t GetMessageSizeWithHeader() const override
    {
        return vRecv.size();
    }

    std::string GetCommandName() const override
    {
        return "";
    }

    int Read(const char* pch, unsigned int nBytes) override;

    bool VerifyMessageStart() const override { return true; }
    bool VerifyHeader() const override;
    bool VerifyChecksum(std::string& error) const override { return true; }
};

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
