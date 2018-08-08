// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <net_encryption.h>
#include <net_message.h>

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

        if (!m_encryption_handler->GetLength(vRecv, m_message_size)) {
            return -1;
        }

        // reject messages larger than MAX_SIZE
        if (m_message_size > MAX_SIZE) {
            return -1;
        }

        // switch state to reading message data
        m_in_data = true;

        return copy_bytes;
    } else {
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

        return copy_bytes;
    }
}

bool P2PEncryption::AuthenticatedAndDecrypt(CDataStream& data_in_out)
{
    // remove length
    data_in_out.ignore(4);

    // remove tag
    data_in_out.erase(data_in_out.end() - TAG_LEN, data_in_out.end());
    return true;
}

bool P2PEncryption::EncryptAppendMAC(std::vector<unsigned char>& data)
{
    std::vector<unsigned char> mac_tag;
    for (unsigned int i = 0; i < TAG_LEN; i++) {
        data.push_back(0);
    }
    return true;
}

bool P2PEncryption::GetLength(CDataStream& data_in, uint32_t& len_out)
{
    if (data_in.size() < AAD_LEN) {
        return false;
    }

    //decrypt
    //TODO

    try {
        data_in >> len_out;
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

bool P2PEncryption::ShouldCryptMsg()
{
    return true;
}
