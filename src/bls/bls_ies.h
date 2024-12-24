// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_CRYPTO_BLS_IES_H
#define DASH_CRYPTO_BLS_IES_H

#include <bls/bls.h>
#include <streams.h>

/**
 * All objects in this module working from assumption that basic scheme is
 * available on all masternodes. Serialization of public key for Encrypt and
 * Decrypt by bls_ies.h done using Basic Scheme.
 */
class CBLSIESEncryptedBlob
{
public:
    CBLSPublicKey ephemeralPubKey;
    uint256 ivSeed;
    std::vector<unsigned char> data;

    uint256 GetIV(size_t idx) const;

    SERIALIZE_METHODS(CBLSIESEncryptedBlob, obj)
    {
        READWRITE(obj.ephemeralPubKey, obj.ivSeed, obj.data);
    }

    bool Decrypt(size_t idx, const CBLSSecretKey& secretKey, CDataStream& decryptedDataRet) const;
    bool IsValid() const;
};

template <typename Object>
class CBLSIESEncryptedObject : public CBLSIESEncryptedBlob
{
public:
    CBLSIESEncryptedObject() = default;

    CBLSIESEncryptedObject(const CBLSPublicKey& ephemeralPubKeyIn, const uint256& ivSeedIn, const std::vector<unsigned char>& dataIn)
    {
        ephemeralPubKey = ephemeralPubKeyIn;
        ivSeed = ivSeedIn;
        data = dataIn;
    }

    bool Decrypt(size_t idx, const CBLSSecretKey& secretKey, Object& objRet, int nVersion) const
    {
        CDataStream ds(SER_NETWORK, nVersion);
        if (!CBLSIESEncryptedBlob::Decrypt(idx, secretKey, ds)) {
            return false;
        }
        try {
            ds >> objRet;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
};

class CBLSIESMultiRecipientBlobs
{
public:
    using Blob = std::vector<unsigned char>;
    using BlobVector = std::vector<Blob>;

    CBLSPublicKey ephemeralPubKey;
    uint256 ivSeed;
    BlobVector blobs;

    // Used while encrypting. Temporary and only in-memory
    CBLSSecretKey ephemeralSecretKey;
    std::vector<uint256> ivVector;

    void InitEncrypt(size_t count);
    bool Encrypt(size_t idx, const CBLSPublicKey& recipient, const Blob& blob);
    bool Decrypt(size_t idx, const CBLSSecretKey& sk, Blob& blobRet) const;

    SERIALIZE_METHODS(CBLSIESMultiRecipientBlobs, obj)
    {
        READWRITE(obj.ephemeralPubKey, obj.ivSeed, obj.blobs);
    }
};

template <typename Object>
class CBLSIESMultiRecipientObjects : public CBLSIESMultiRecipientBlobs
{
public:
    bool Encrypt(size_t idx, const CBLSPublicKey& recipient, const Object& obj, int nVersion)
    {
        CDataStream ds(SER_NETWORK, nVersion);
        ds << obj;
        Blob blob(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
        return CBLSIESMultiRecipientBlobs::Encrypt(idx, recipient, blob);
    }

    bool Decrypt(size_t idx, const CBLSSecretKey& sk, Object& objectRet, int nVersion) const
    {
        Blob blob;
        if (!CBLSIESMultiRecipientBlobs::Decrypt(idx, sk, blob)) {
            return false;
        }

        try {
            CDataStream ds(blob, SER_NETWORK, nVersion);
            ds >> objectRet;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    CBLSIESEncryptedObject<Object> Get(const size_t idx)
    {
        return {ephemeralPubKey, ivSeed, blobs[idx]};
    }
};

#endif // DASH_CRYPTO_BLS_IES_H
