// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_CRYPTO_BLS_H
#define DASH_CRYPTO_BLS_H

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>

// bls-dash uses relic, which may define DEBUG and ERROR, which leads to many warnings in some build setups
#undef ERROR
#undef DEBUG
#include <bls-dash/bls.hpp>
#include <bls-dash/privatekey.hpp>
#include <bls-dash/elements.hpp>
#include <bls-dash/schemes.hpp>
#include <bls-dash/threshold.hpp>
#undef DOUBLE

#include <array>
#include <mutex>
#include <unistd.h>

static const bool fLegacyDefault{true};

// reversed BLS12-381
#define BLS_CURVE_ID_SIZE 32
#define BLS_CURVE_SECKEY_SIZE 32
#define BLS_CURVE_PUBKEY_SIZE 48
#define BLS_CURVE_SIG_SIZE 96

class CBLSSignature;
class CBLSPublicKey;

template <typename ImplType, size_t _SerSize, typename C>
class CBLSWrapper
{
    friend class CBLSSecretKey;
    friend class CBLSPublicKey;
    friend class CBLSSignature;

    bool fLegacy;

protected:
    ImplType impl;
    bool fValid{false};
    mutable uint256 cachedHash;

    inline constexpr size_t GetSerSize() const { return SerSize; }

public:
    static const size_t SerSize = _SerSize;

    explicit CBLSWrapper(const bool fLegacyIn = fLegacyDefault) : fLegacy(fLegacyIn)
    {
    }
    explicit CBLSWrapper(const std::vector<unsigned char>& vecBytes, const bool fLegacyIn = fLegacyDefault) : CBLSWrapper<ImplType, _SerSize, C>(fLegacyIn)
    {
        SetByteVector(vecBytes);
    }

    CBLSWrapper(const CBLSWrapper& ref) = default;
    CBLSWrapper& operator=(const CBLSWrapper& ref) = default;
    CBLSWrapper(CBLSWrapper&& ref) noexcept
    {
        std::swap(impl, ref.impl);
        std::swap(fValid, ref.fValid);
        std::swap(cachedHash, ref.cachedHash);
        std::swap(fLegacy, ref.fLegacy);
    }
    CBLSWrapper& operator=(CBLSWrapper&& ref) noexcept
    {
        std::swap(impl, ref.impl);
        std::swap(fValid, ref.fValid);
        std::swap(cachedHash, ref.cachedHash);
        std::swap(fLegacy, ref.fLegacy);
        return *this;
    }

    virtual ~CBLSWrapper() {}

    bool operator==(const C& r) const
    {
        return fValid == r.fValid && impl == r.impl;
    }
    bool operator!=(const C& r) const
    {
        return !((*this) == r);
    }

    bool IsValid() const
    {
        return fValid;
    }

    void Reset()
    {
        *((C*)this) = C(fLegacy);
    }

    void SetByteVector(const std::vector<uint8_t>& vecBytes)
    {
        if (vecBytes.size() != SerSize) {
            Reset();
            return;
        }

        if (std::all_of(vecBytes.begin(), vecBytes.end(), [](uint8_t c) { return c == 0; })) {
            Reset();
        } else {
            try {
                impl = ImplType::FromBytes(bls::Bytes(vecBytes), fLegacy);
                fValid = true;
            } catch (...) {
                Reset();
            }
        }
        cachedHash.SetNull();
    }

    std::vector<uint8_t> ToByteVector() const
    {
        if (!fValid) {
            return std::vector<uint8_t>(SerSize, 0);
        }
        return impl.Serialize(fLegacy);
    }

    const uint256& GetHash() const
    {
        if (cachedHash.IsNull()) {
            cachedHash = ::SerializeHash(*this);
        }
        return cachedHash;
    }

    bool SetHexStr(const std::string& str)
    {
        if (!IsHex(str)) {
            Reset();
            return false;
        }
        auto b = ParseHex(str);
        if (b.size() != SerSize) {
            Reset();
            return false;
        }
        SetByteVector(b);
        return IsValid();
    }

public:
    inline void Serialize(CSizeComputer& s) const
    {
        s.seek(SerSize);
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        s.write((const char*)ToByteVector().data(), SerSize);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s, bool checkMalleable = true)
    {
        std::vector<uint8_t> vecBytes(SerSize, 0);
        s.read((char*)vecBytes.data(), SerSize);
        SetByteVector(vecBytes);

        if (checkMalleable && !CheckMalleable(vecBytes)) {
            throw std::ios_base::failure("malleable BLS object");
        }
    }

    inline bool CheckMalleable(const std::vector<uint8_t>& vecBytes) const
    {
        if (memcmp(vecBytes.data(), ToByteVector().data(), SerSize)) {
            // TODO not sure if this is actually possible with the BLS libs. I'm assuming here that somewhere deep inside
            // these libs masking might happen, so that 2 different binary representations could result in the same object
            // representation
            return false;
        }
        return true;
    }

    inline std::string ToString() const
    {
        std::vector<uint8_t> buf = ToByteVector();
        return HexStr(buf);
    }
};

struct CBLSIdImplicit : public uint256
{
    CBLSIdImplicit() = default;
    CBLSIdImplicit(const uint256& id)
    {
        memcpy(begin(), id.begin(), sizeof(uint256));
    }
    static CBLSIdImplicit FromBytes(const uint8_t* buffer, const bool fLegacy = false)
    {
        CBLSIdImplicit instance;
        memcpy(instance.begin(), buffer, sizeof(CBLSIdImplicit));
        return instance;
    }
    std::vector<uint8_t> Serialize(const bool fLegacy = false) const
    {
        return {begin(), end()};
    }
};

class CBLSId : public CBLSWrapper<CBLSIdImplicit, BLS_CURVE_ID_SIZE, CBLSId>
{
public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;
    using CBLSWrapper::CBLSWrapper;

    CBLSId() = default;
    explicit CBLSId(const uint256& nHash);
};

class CBLSSecretKey : public CBLSWrapper<bls::PrivateKey, BLS_CURVE_SECKEY_SIZE, CBLSSecretKey>
{
public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;
    using CBLSWrapper::CBLSWrapper;

    CBLSSecretKey() = default;
    CBLSSecretKey(const CBLSSecretKey&) = default;
    CBLSSecretKey& operator=(const CBLSSecretKey&) = default;

    void AggregateInsecure(const CBLSSecretKey& o);
    static CBLSSecretKey AggregateInsecure(const std::vector<CBLSSecretKey>& sks);

#ifndef BUILD_BITCOIN_INTERNAL
    void MakeNewKey();
#endif
    bool SecretKeyShare(const std::vector<CBLSSecretKey>& msk, const CBLSId& id);

    CBLSPublicKey GetPublicKey() const;
    CBLSSignature Sign(const uint256& hash) const;
};

class CBLSPublicKey : public CBLSWrapper<bls::G1Element, BLS_CURVE_PUBKEY_SIZE, CBLSPublicKey>
{
    friend class CBLSSecretKey;
    friend class CBLSSignature;

public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;
    using CBLSWrapper::CBLSWrapper;

    CBLSPublicKey() = default;

    void AggregateInsecure(const CBLSPublicKey& o);
    static CBLSPublicKey AggregateInsecure(const std::vector<CBLSPublicKey>& pks, bool fLegacy = fLegacyDefault);

    bool PublicKeyShare(const std::vector<CBLSPublicKey>& mpk, const CBLSId& id);
    bool DHKeyExchange(const CBLSSecretKey& sk, const CBLSPublicKey& pk);

};

class CBLSSignature : public CBLSWrapper<bls::G2Element, BLS_CURVE_SIG_SIZE, CBLSSignature>
{
    friend class CBLSSecretKey;

public:
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;
    using CBLSWrapper::CBLSWrapper;

    CBLSSignature() = default;
    CBLSSignature(const CBLSSignature&) = default;
    CBLSSignature& operator=(const CBLSSignature&) = default;

    void AggregateInsecure(const CBLSSignature& o);
    static CBLSSignature AggregateInsecure(const std::vector<CBLSSignature>& sigs, bool fLegacy = fLegacyDefault);
    static CBLSSignature AggregateSecure(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSPublicKey>& pks, const uint256& hash, bool fLegacy = fLegacyDefault);

    void SubInsecure(const CBLSSignature& o);

    bool VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const;
    bool VerifyInsecureAggregated(const std::vector<CBLSPublicKey>& pubKeys, const std::vector<uint256>& hashes) const;

    bool VerifySecureAggregated(const std::vector<CBLSPublicKey>& pks, const uint256& hash) const;

    bool Recover(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSId>& ids);
};

#ifndef BUILD_BITCOIN_INTERNAL
template<typename BLSObject>
class CBLSLazyWrapper
{
private:
    mutable std::mutex mutex;

    mutable std::vector<uint8_t> vecBytes;
    mutable bool bufValid{false};

    mutable BLSObject obj;
    mutable bool objInitialized{false};

    mutable uint256 hash;

public:
    CBLSLazyWrapper() :
        vecBytes(BLSObject::SerSize, 0)
    {
        // the all-zero buf is considered a valid buf, but the resulting object will return false for IsValid
        bufValid = true;
    }

    CBLSLazyWrapper(const CBLSLazyWrapper& r)
    {
        *this = r;
    }
    virtual ~CBLSLazyWrapper() {}

    CBLSLazyWrapper& operator=(const CBLSLazyWrapper& r)
    {
        std::unique_lock<std::mutex> l(r.mutex);
        bufValid = r.bufValid;
        if (r.bufValid) {
            vecBytes = r.vecBytes;
        } else {
            std::fill(vecBytes.begin(), vecBytes.end(), 0);
        }
        objInitialized = r.objInitialized;
        if (r.objInitialized) {
            obj = r.obj;
        } else {
            obj.Reset();
        }
        hash = r.hash;
        return *this;
    }

    inline void Serialize(CSizeComputer& s) const
    {
        s.seek(BLSObject::SerSize);
    }

    template<typename Stream>
    inline void Serialize(Stream& s) const
    {
        std::unique_lock<std::mutex> l(mutex);
        if (!objInitialized && !bufValid) {
            throw std::ios_base::failure("obj and buf not initialized");
        }
        if (!bufValid) {
            vecBytes = obj.ToByteVector();
            bufValid = true;
            hash.SetNull();
        }
        s.write((const char*)vecBytes.data(), vecBytes.size());
    }

    template<typename Stream>
    inline void Unserialize(Stream& s)
    {
        std::unique_lock<std::mutex> l(mutex);
        s.read((char*)vecBytes.data(), BLSObject::SerSize);
        bufValid = true;
        objInitialized = false;
        hash.SetNull();
    }

    void Set(const BLSObject& _obj)
    {
        std::unique_lock<std::mutex> l(mutex);
        bufValid = false;
        objInitialized = true;
        obj = _obj;
        hash.SetNull();
    }
    const BLSObject& Get() const
    {
        std::unique_lock<std::mutex> l(mutex);
        static BLSObject invalidObj;
        if (!bufValid && !objInitialized) {
            return invalidObj;
        }
        if (!objInitialized) {
            obj.SetByteVector(vecBytes);
            if (!obj.CheckMalleable(vecBytes)) {
                bufValid = false;
                objInitialized = false;
                obj = invalidObj;
            } else {
                objInitialized = true;
            }
        }
        return obj;
    }

    bool operator==(const CBLSLazyWrapper& r) const
    {
        if (bufValid && r.bufValid) {
            return vecBytes == r.vecBytes;
        }
        if (objInitialized && r.objInitialized) {
            return obj == r.obj;
        }
        return Get() == r.Get();
    }

    bool operator!=(const CBLSLazyWrapper& r) const
    {
        return !(*this == r);
    }

    uint256 GetHash() const
    {
        std::unique_lock<std::mutex> l(mutex);
        if (!bufValid) {
            vecBytes = obj.ToByteVector();
            bufValid = true;
            hash.SetNull();
        }
        if (hash.IsNull()) {
            CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
            ss.write((const char*)vecBytes.data(), vecBytes.size());
            hash = ss.GetHash();
        }
        return hash;
    }
};
typedef CBLSLazyWrapper<CBLSSignature> CBLSLazySignature;
typedef CBLSLazyWrapper<CBLSPublicKey> CBLSLazyPublicKey;
typedef CBLSLazyWrapper<CBLSSecretKey> CBLSLazySecretKey;
#endif

typedef std::vector<CBLSId> BLSIdVector;
typedef std::vector<CBLSPublicKey> BLSVerificationVector;
typedef std::vector<CBLSPublicKey> BLSPublicKeyVector;
typedef std::vector<CBLSSecretKey> BLSSecretKeyVector;
typedef std::vector<CBLSSignature> BLSSignatureVector;

typedef std::shared_ptr<BLSIdVector> BLSIdVectorPtr;
typedef std::shared_ptr<BLSVerificationVector> BLSVerificationVectorPtr;
typedef std::shared_ptr<BLSPublicKeyVector> BLSPublicKeyVectorPtr;
typedef std::shared_ptr<BLSSecretKeyVector> BLSSecretKeyVectorPtr;
typedef std::shared_ptr<BLSSignatureVector> BLSSignatureVectorPtr;

bool BLSInit();

#endif // DASH_CRYPTO_BLS_H
