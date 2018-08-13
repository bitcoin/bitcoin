// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <crypto/hkdf_sha256_32.h>
#include <net_encryption.h>

#include <logging.h>
#include <net_message.h>
#include <util.h>

int NetV2Message::Read(const char* pch, unsigned bytes)
{
    if (!m_in_data) {
        // copy data to temporary parsing buffer
        unsigned int remaining = m_encryption_handler->GetAADLen() - m_hdr_pos;
        unsigned int copy_bytes = std::min(remaining, bytes);

        memcpy(&vRecv[m_hdr_pos], pch, copy_bytes);
        m_hdr_pos += copy_bytes;

        // if AAD incomplete, exit
        if (m_hdr_pos < m_encryption_handler->GetAADLen()) {
            return copy_bytes;
        }

        // decrypt the length from the AAD
        if (!m_encryption_handler->GetLength(vRecv, m_message_size)) {
            LogPrint(BCLog::NET, "Failed to read length\n");
            return -1;
        }

        // reject messages larger than MAX_SIZE
        if (m_message_size > MAX_SIZE) {
            LogPrint(BCLog::NET, "Max size exceeded\n");
            return -1;
        }

        // switch state to reading message data
        m_in_data = true;

        return copy_bytes;
    } else {
        // copy the message payload plus the MAC tag
        const unsigned int AAD_LEN = m_encryption_handler->GetAADLen();
        const unsigned int TAG_LEN = m_encryption_handler->GetTagLen();
        unsigned int remaining = m_message_size + TAG_LEN - m_data_pos;
        unsigned int copy_bytes = std::min(remaining, bytes);

        // extend buffer, respect previous copied AAD part
        if (vRecv.size() < AAD_LEN + m_data_pos + copy_bytes) {
            // Allocate up to 256 KiB ahead, but never more than the total message size (incl. AAD & TAG).
            vRecv.resize(std::min(AAD_LEN + m_message_size + TAG_LEN, AAD_LEN + m_data_pos + copy_bytes + 256 * 1024 + TAG_LEN));
        }

        memcpy(&vRecv[AAD_LEN + m_data_pos], pch, copy_bytes);
        m_data_pos += copy_bytes;

        if (Complete()) {
            // authenticate and decrypt if the message is complete
            if (!m_encryption_handler->AuthenticatedAndDecrypt(vRecv)) {
                LogPrint(BCLog::NET, "Authentication or decryption failed\n");
                return false;
            }

            // vRecv holds now the plaintext message excluding the AAD and MAC
            // m_message_size holds the packet size excluding the MAC

            // initially check the message
            try {
                vRecv >> m_command_name;
            } catch (const std::exception&) {
                LogPrint(BCLog::NET, "Invalid command name\n");
                return false;
            }
            // vRecv points now to the plaintext message payload (MAC is removed)
        }

        return copy_bytes;
    }
}

int NetMessageEncryptionHandshake::Read(const char* pch, unsigned int bytes)
{
    // copy data to temporary parsing buffer
    unsigned int remaining = 32 - m_data_pos;
    unsigned int copy_bytes = std::min(remaining, bytes);
    if (vRecv.size() < 32) {
        vRecv.resize(32);
    }
    memcpy(&vRecv[m_data_pos], pch, copy_bytes);
    m_data_pos += copy_bytes;

    return copy_bytes;
}

bool NetMessageEncryptionHandshake::VerifyHeader() const
{
    CMessageHeader hdr(Params().MessageStart());
    CDataStream str = vRecv; //copy stream to keep function const
    try {
        str >> hdr;
    } catch (const std::exception&) {
        LogPrint(BCLog::NET, "Invalid header\n");
        return false;
    }
    if (memcmp(hdr.pchMessageStart, Params().MessageStart(), CMessageHeader::MESSAGE_START_SIZE) == 0 || hdr.GetCommand() == NetMsgType::VERSION) {
        LogPrint(BCLog::NET, "Invalid msg start\n");
        return false;
    }
    return true;
}

bool P2PEncryption::AuthenticatedAndDecrypt(CDataStream& data_in_out)
{
    // create a buffer for the decrypted payload
    std::vector<unsigned char> buf_dec;
    buf_dec.resize(data_in_out.size());

    // keep the original payload size
    size_t vsize = data_in_out.size();

    // authenticate and decrypt the message
    LOCK(cs);
    if (!m_recv_aead_ctx->Crypt(m_recv_seq_nr, m_recv_seq_nr_aad, m_recv_aad_keystream_pos, buf_dec.data(), buf_dec.size(), (const unsigned char *)&data_in_out.data()[0],
            data_in_out.size(), false)) {
        memory_cleanse(data_in_out.data(), data_in_out.size());
        LogPrint(BCLog::NET, "Crypt failed\n");
        return false;
    }

    // increase main sequence number and eventually increase the AAD sequence number
    // always increase (or reset) the keystream position
    m_recv_seq_nr++;
    m_recv_aad_keystream_pos += CHACHA20_POLY1305_AEAD_AAD_LEN;
    if (m_recv_aad_keystream_pos + CHACHA20_POLY1305_AEAD_AAD_LEN > CHACHA20_ROUND_OUTPUT) {
        m_recv_aad_keystream_pos = 0;
        m_recv_seq_nr_aad++;
    }

    data_in_out.clear();
    // write payload (avoid the 3byte AAD length and the MAC)
    data_in_out.write((const char*)&buf_dec.begin()[AAD_LEN], vsize - TAG_LEN - AAD_LEN);
    return true;
}

bool P2PEncryption::EncryptAppendMAC(std::vector<unsigned char>& data_in_out)
{
    // create a buffer for the encrypted payload
    std::vector<unsigned char> buf_enc;
    buf_enc.resize(data_in_out.size() + TAG_LEN);

    // encrypt and add MAC tag
    LOCK(cs);
    m_send_aead_ctx->Crypt(m_send_seq_nr, m_send_seq_nr_aad, m_send_aad_keystream_pos, &buf_enc[0], buf_enc.size(), &data_in_out[0],
        data_in_out.size(), true);

    // increase main sequence number and eventually increase the AAD sequence number
    // always increase (or reset) the keystream position
    m_send_seq_nr++;
    m_send_aad_keystream_pos += CHACHA20_POLY1305_AEAD_AAD_LEN;
    if (m_send_aad_keystream_pos + CHACHA20_POLY1305_AEAD_AAD_LEN > CHACHA20_ROUND_OUTPUT) {
        m_send_aad_keystream_pos = 0;
        m_send_seq_nr_aad++;
    }

    // clear data_in and append the decrypted data
    data_in_out.clear();
    // append encrypted message (AAD & payload & MAC)
    data_in_out.insert(data_in_out.begin(), buf_enc.begin(), buf_enc.end());
    return true;
}

bool P2PEncryption::GetLength(CDataStream& data_in, uint32_t& len_out)
{
    if (data_in.size() < AAD_LEN) {
        LogPrint(BCLog::NET, "AAD len to short\n");
        return false;
    }

    LOCK(cs);
    if (!m_recv_aead_ctx->GetLength(&len_out, m_recv_seq_nr_aad, m_recv_aad_keystream_pos, (const uint8_t*)&data_in.data()[0])) {
        LogPrint(BCLog::NET, "GetLength failed\n");
        return false;
    }

    return true;
}

bool P2PEncryption::ShouldCryptMsg()
{
    return handshake_done;
}

uint256 P2PEncryption::GetSessionID()
{
    LOCK(cs);
    return m_session_id;
}

void P2PEncryption::EnableEncryption(bool inbound)
{
    unsigned char aead_k_1_a[32];
    unsigned char aead_k_2_a[32];
    unsigned char aead_k_1_b[32];
    unsigned char aead_k_2_b[32];

    LOCK(cs);
    if (m_raw_ecdh_secret.size() != 32) {
        return;
    }
    // extract 2 keys for each direction with HKDF HMAC_SHA256 with length 32
    CHKDF_HMAC_SHA256_L32 hkdf_32(&m_raw_ecdh_secret[0], 32, "BitcoinSharedSecret");
    hkdf_32.Expand32("BitcoinK1A", aead_k_1_a);
    hkdf_32.Expand32("BitcoinK2A", aead_k_2_a);
    hkdf_32.Expand32("BitcoinK1B", aead_k_1_b);
    hkdf_32.Expand32("BitcoinK2B", aead_k_2_b);
    hkdf_32.Expand32("BitcoinSessionID", m_session_id.begin());

    // enabling k1 for send channel on requesting peer and for recv channel on responding peer
    // enabling k2 for recv channel on requesting peer and for send channel on responding peer
    m_send_aead_ctx.reset(new ChaCha20Poly1305AEAD(inbound ? aead_k_1_b : aead_k_1_a, sizeof(aead_k_1_a), inbound ? aead_k_2_b : aead_k_2_a, sizeof(aead_k_1_a)));
    m_recv_aead_ctx.reset(new ChaCha20Poly1305AEAD(inbound ? aead_k_1_a : aead_k_1_b, sizeof(aead_k_1_a), inbound ? aead_k_2_a : aead_k_2_b, sizeof(aead_k_1_a)));
    handshake_done = true;
}

bool P2PEncryption::GetHandshakeRequestData(std::vector<unsigned char>& handshake_data)
{
    LOCK(cs);
    CPubKey pubkey = m_ecdh_key.GetPubKey();
    m_ecdh_key.VerifyPubKey(pubkey); //verify the pubkey
    assert(pubkey[0] == 2);

    handshake_data.insert(handshake_data.begin(), pubkey.begin() + 1, pubkey.end());
    return true;
}

bool P2PEncryption::ProcessHandshakeRequestData(const std::vector<unsigned char>& handshake_data)
{
    CPubKey pubkey;
    if (handshake_data.size() != 32) {
        LogPrint(BCLog::NET, "invalid handshake size\n");
        return false;
    }
    std::vector<unsigned char> handshake_data_even_pubkey;
    handshake_data_even_pubkey.push_back(2);
    handshake_data_even_pubkey.insert(handshake_data_even_pubkey.begin() + 1, handshake_data.begin(), handshake_data.end());
    pubkey.Set(handshake_data_even_pubkey.begin(), handshake_data_even_pubkey.end());
    if (!pubkey.IsFullyValid()) {
        LogPrint(BCLog::NET, "invalid pubkey\n");
        return false;
    }

    // calculate ECDH secret
    LOCK(cs);
    bool ret = m_ecdh_key.ComputeECDHSecret(pubkey, m_raw_ecdh_secret);

    // After calculating the ECDH secret, the ephemeral key can be cleansed from memory
    m_ecdh_key.SetNull();
    return ret;
}

P2PEncryption::P2PEncryption() : handshake_done(false)
{
    m_ecdh_key.MakeNewKey(true);
    if (m_ecdh_key.GetPubKey()[0] == 3) {
        // the encryption handshake will only use 32byte pubkeys
        // force EVEN (0x02) pubkey be negating the private key in case of ODD (0x03) pubkeys
        m_ecdh_key.Negate();
    }
    assert(m_ecdh_key.IsValid());
}
