// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_BLS_BLS_IES_H
#define SYSCOIN_BLS_BLS_IES_H

#include <bls/bls.h>
#include <streams.h>

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

    bool Encrypt(size_t idx, const CBLSPublicKey& peerPubKey, const void* data, size_t dataSize);
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

    bool Encrypt(size_t idx, const CBLSPublicKey& peerPubKey, const Object& obj, int nVersion)
    {
        try {
            CDataStream ds(SER_NETWORK, nVersion);
            ds << obj;
            return CBLSIESEncryptedBlob::Encrypt(idx, peerPubKey, ds.data(), ds.size());
        } catch (const std::exception&) {
            return false;
        }
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
    // SYSCOIN
    using Blob = std::vector<uint8_t>;
    using BlobVector = std::vector<Blob>;

    CBLSPublicKey ephemeralPubKey;
    uint256 ivSeed;
    BlobVector blobs;

    // Used while encrypting. Temporary and only in-memory
    CBLSSecretKey ephemeralSecretKey;
    std::vector<uint256> ivVector;

    bool Encrypt(const std::vector<CBLSPublicKey>& recipients, const BlobVector& _blobs);

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
    using ObjectVector = std::vector<Object>;

    bool Encrypt(const std::vector<CBLSPublicKey>& recipients, const ObjectVector& _objects, int nVersion)
    {
        BlobVector blobs;
        blobs.resize(_objects.size());

        try {
            CDataStream ds(SER_NETWORK, nVersion);
            for (size_t i = 0; i < _objects.size(); i++) {
                ds.clear();

                ds << _objects[i];
                // SYSCOIN
                const auto bytesVec = MakeUCharSpan(ds);
                blobs[i].assign(bytesVec.begin(), bytesVec.end());
            }
        } catch (const std::exception&) {
            return false;
        }

        return CBLSIESMultiRecipientBlobs::Encrypt(recipients, blobs);
    }

    bool Encrypt(size_t idx, const CBLSPublicKey& recipient, const Object& obj, int nVersion)
    {
        CDataStream ds(SER_NETWORK, nVersion);
        ds << obj;
        // SYSCOIN
        const auto bytesVec = MakeUCharSpan(ds);
        Blob blob(bytesVec.begin(), bytesVec.end());
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

#endif // SYSCOIN_BLS_BLS_IES_H
