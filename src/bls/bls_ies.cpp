// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls_ies.h>

#include <hash.h>
#include <random.h>

#include <crypto/aes.h>

template <typename Out>
static bool EncryptBlob(const void* in, size_t inSize, Out& out, const void* symKey, const void* iv)
{
    out.resize(inSize);

    AES256CBCEncrypt enc((const unsigned char*)symKey, (const unsigned char*)iv, false);
    int w = enc.Encrypt((const unsigned char*)in, (int)inSize, (unsigned char*)out.data());
    return w == (int)inSize;
}

template <typename Out>
static bool DecryptBlob(const void* in, size_t inSize, Out& out, const void* symKey, const void* iv)
{
    out.resize(inSize);

    AES256CBCDecrypt enc((const unsigned char*)symKey, (const unsigned char*)iv, false);
    int w = enc.Decrypt((const unsigned char*)in, (int)inSize, (unsigned char*)out.data());
    return w == (int)inSize;
}

uint256 CBLSIESEncryptedBlob::GetIV(size_t idx) const
{
    uint256 iv = ivSeed;
    for (size_t i = 0; i < idx; i++) {
        iv = ::SerializeHash(iv);
    }
    return iv;
}

bool CBLSIESEncryptedBlob::Encrypt(size_t idx, const CBLSPublicKey& peerPubKey, const void* plainTextData, size_t dataSize)
{
    CBLSSecretKey ephemeralSecretKey;
    ephemeralSecretKey.MakeNewKey();
    ephemeralPubKey = ephemeralSecretKey.GetPublicKey();

    CBLSPublicKey pk;
    if (!pk.DHKeyExchange(ephemeralSecretKey, peerPubKey)) {
        return false;
    }

    std::vector<unsigned char> symKey = pk.ToByteVector();
    symKey.resize(32);

    uint256 iv = GetIV(idx);
    return EncryptBlob(plainTextData, dataSize, data, symKey.data(), iv.begin());
}

bool CBLSIESEncryptedBlob::Decrypt(size_t idx, const CBLSSecretKey& secretKey, CDataStream& decryptedDataRet) const
{
    CBLSPublicKey pk;
    if (!pk.DHKeyExchange(secretKey, ephemeralPubKey)) {
        return false;
    }

    std::vector<unsigned char> symKey = pk.ToByteVector();
    symKey.resize(32);

    uint256 iv = GetIV(idx);
    return DecryptBlob(data.data(), data.size(), decryptedDataRet, symKey.data(), iv.begin());
}

bool CBLSIESEncryptedBlob::IsValid() const
{
    return ephemeralPubKey.IsValid() && !data.empty() && !ivSeed.IsNull();
}


bool CBLSIESMultiRecipientBlobs::Encrypt(const std::vector<CBLSPublicKey>& recipients, const BlobVector& _blobs)
{
    if (recipients.size() != _blobs.size()) {
        return false;
    }

    InitEncrypt(_blobs.size());

    for (size_t i = 0; i < _blobs.size(); i++) {
        if (!Encrypt(i, recipients[i], _blobs[i])) {
            return false;
        }
    }

    return true;
}

void CBLSIESMultiRecipientBlobs::InitEncrypt(size_t count)
{
    ephemeralSecretKey.MakeNewKey();
    ephemeralPubKey = ephemeralSecretKey.GetPublicKey();
    GetStrongRandBytes(ivSeed.begin(), ivSeed.size());

    uint256 iv = ivSeed;
    ivVector.resize(count);
    blobs.resize(count);
    for (size_t i = 0; i < count; i++) {
        ivVector[i] = iv;
        iv = ::SerializeHash(iv);
    }
}

bool CBLSIESMultiRecipientBlobs::Encrypt(size_t idx, const CBLSPublicKey& recipient, const Blob& blob)
{
    assert(idx < blobs.size());

    CBLSPublicKey pk;
    if (!pk.DHKeyExchange(ephemeralSecretKey, recipient)) {
        return false;
    }

    std::vector<uint8_t> symKey = pk.ToByteVector();
    symKey.resize(32);

    return EncryptBlob(blob.data(), blob.size(), blobs[idx], symKey.data(), ivVector[idx].begin());
}

bool CBLSIESMultiRecipientBlobs::Decrypt(size_t idx, const CBLSSecretKey& sk, Blob& blobRet) const
{
    if (idx >= blobs.size()) {
        return false;
    }

    CBLSPublicKey pk;
    if (!pk.DHKeyExchange(sk, ephemeralPubKey)) {
        return false;
    }

    std::vector<uint8_t> symKey = pk.ToByteVector();
    symKey.resize(32);

    uint256 iv = ivSeed;
    for (size_t i = 0; i < idx; i++) {
        iv = ::SerializeHash(iv);
    }

    return DecryptBlob(blobs[idx].data(), blobs[idx].size(), blobRet, symKey.data(), iv.begin());
}
