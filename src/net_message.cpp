// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <net_message.h>

#include <chainparams.h>
#include <tinyformat.h>
#include <util/strencodings.h>

int NetMessage::Read(const char* pch, unsigned int nBytes)
{
    return in_data ? ReadData(pch, nBytes) : ReadHeader(pch, nBytes);
}

int NetMessage::ReadHeader(const char* pch, unsigned int nBytes)
{
    // copy data to temporary parsing buffer
    unsigned int nRemaining = 24 - nHdrPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (const std::exception&) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
        return -1;

    // switch state to reading message data
    in_data = true;

    return nCopy;
}

int NetMessage::ReadData(const char* pch, unsigned int nBytes)
{
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    if (vRecv.size() < nDataPos + nCopy) {
        // Allocate up to 256 KiB ahead, but never more than the total message size.
        vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
    }

    hasher.Write((const unsigned char*)pch, nCopy);
    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}

const uint256& NetMessage::GetMessageHash() const
{
    assert(Complete());
    if (data_hash.IsNull())
        hasher.Finalize(data_hash.begin());
    return data_hash;
}

bool NetMessage::VerifyMessageStart() const
{
    return (memcmp(hdr.pchMessageStart, Params().MessageStart(), CMessageHeader::MESSAGE_START_SIZE) == 0);
}

bool NetMessage::VerifyHeader() const
{
    return hdr.IsValid(Params().MessageStart());
}

bool NetMessage::VerifyChecksum(std::string& error) const
{
    const uint256& hash = GetMessageHash();
    if (memcmp(hash.begin(), hdr.pchChecksum, CMessageHeader::CHECKSUM_SIZE) != 0) {
        error = tfm::format("CHECKSUM ERROR expected %s was %s\n",
            HexStr(hash.begin(), hash.begin() + CMessageHeader::CHECKSUM_SIZE),
            HexStr(hdr.pchChecksum, hdr.pchChecksum + CMessageHeader::CHECKSUM_SIZE));
        return false;
    }
    return true;
}
