// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_BLS_H
#define BITGREEN_BLS_H

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>

#undef ERROR // chia BLS uses relic, which defines ERROR, which in turn causes win32/win64 builds to print many warnings
#include <chiabls/bls.hpp>
#include <chiabls/privatekey.hpp>
#include <chiabls/publickey.hpp>
#include <chiabls/signature.hpp>
#undef DOUBLE

#include <array>
#include <mutex>
#include <unistd.h>

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

protected:
    ImplType impl;
    bool fValid{false};
    mutable uint256 cachedHash;

    inline constexpr size_t GetSerSize() const { return SerSize; }

    virtual bool InternalSetBuf(const void* buf) = 0;
    virtual bool InternalGetBuf(void* buf) const = 0;

public:
    static const size_t SerSize = _SerSize;

    CBLSWrapper()
    {
        struct NullHash {
            uint256 hash;
            NullHash() {
                char buf[_SerSize];
                memset(buf, 0, _SerSize);
                CHashWriter ss(SER_GETHASH, 0);
                ss.write(buf, _SerSize);
                hash = ss.GetHash();
            }
        };
        static NullHash nullHash;
        cachedHash = nullHash.hash;
    }

    CBLSWrapper(const CBLSWrapper& ref) = default;
    CBLSWrapper& operator=(const CBLSWrapper& ref) = default;
    CBLSWrapper(CBLSWrapper&& ref)
    {
        std::swap(impl, ref.impl);
        std::swap(fValid, ref.fValid);
        std::swap(cachedHash, ref.cachedHash);
    }
    CBLSWrapper& operator=(CBLSWrapper&& ref)
    {
        std::swap(impl, ref.impl);
        std::swap(fValid, ref.fValid);
        std::swap(cachedHash, ref.cachedHash);
        return *this;
    }

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

    void SetBuf(const void* buf, size_t size)
    {
        if (size != SerSize) {
            Reset();
            return;
        }

        if (std::all_of((const char*)buf, (const char*)buf + SerSize, [](char c) { return c == 0; })) {
            Reset();
        } else {
            fValid = InternalSetBuf(buf);
            if (!fValid) {
                Reset();
            }
        }
        UpdateHash();
    }

    void Reset()
    {
        *((C*)this) = C();
    }

    void GetBuf(void* buf, size_t size) const
    {
        assert(size == SerSize);

        if (!fValid) {
            memset(buf, 0, SerSize);
        } else {
            bool ok = InternalGetBuf(buf);
            assert(ok);
        }
    }

    template <typename T>
    void SetBuf(const T& buf)
    {
        SetBuf(buf.data(), buf.size());
    }

    template <typename T>
    void GetBuf(T& buf) const
    {
        buf.resize(GetSerSize());
        GetBuf(buf.data(), buf.size());
    }

    const uint256& GetHash() const
    {
        return cachedHash;
    }

    void UpdateHash() const
    {
        cachedHash = ::SerializeHash(*this);
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
        SetBuf(b);
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
        char buf[SerSize] = {0};
        GetBuf(buf, SerSize);
        s.write((const char*)buf, SerSize);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s, bool checkMalleable = true)
    {
        char buf[SerSize];
        s.read((char*)buf, SerSize);
        SetBuf(buf, SerSize);

        if (checkMalleable && !CheckMalleable(buf, SerSize)) {
            throw std::ios_base::failure("malleable BLS object");
        }
    }

    inline bool CheckMalleable(void* buf, size_t size) const
    {
        char buf2[SerSize];
        GetBuf(buf2, SerSize);
        if (memcmp(buf, buf2, SerSize)) {
            // TODO not sure if this is actually possible with the BLS libs. I'm assuming here that somewhere deep inside
            // these libs masking might happen, so that 2 different binary representations could result in the same object
            // representation
            return false;
        }
        return true;
    }

    inline std::string ToString() const
    {
        std::vector<unsigned char> buf;
        GetBuf(buf);
        return HexStr(buf.begin(), buf.end());
    }
};

class CBLSId : public CBLSWrapper<uint256, BLS_CURVE_ID_SIZE, CBLSId>
{
public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;

    CBLSId() {}

    void SetInt(int x);
    void SetHash(const uint256& hash);

    static CBLSId FromInt(int64_t i);
    static CBLSId FromHash(const uint256& hash);

protected:
    bool InternalSetBuf(const void* buf);
    bool InternalGetBuf(void* buf) const;
};

class CBLSSecretKey : public CBLSWrapper<bls::PrivateKey, BLS_CURVE_SECKEY_SIZE, CBLSSecretKey>
{
public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;

    CBLSSecretKey() {}

    void AggregateInsecure(const CBLSSecretKey& o);
    static CBLSSecretKey AggregateInsecure(const std::vector<CBLSSecretKey>& sks);

#ifndef BUILD_BITCOIN_INTERNAL
    void MakeNewKey();
#endif
    bool SecretKeyShare(const std::vector<CBLSSecretKey>& msk, const CBLSId& id);

    CBLSPublicKey GetPublicKey() const;
    CBLSSignature Sign(const uint256& hash) const;

protected:
    bool InternalSetBuf(const void* buf);
    bool InternalGetBuf(void* buf) const;
};

class CBLSPublicKey : public CBLSWrapper<bls::PublicKey, BLS_CURVE_PUBKEY_SIZE, CBLSPublicKey>
{
    friend class CBLSSecretKey;
    friend class CBLSSignature;

public:
    using CBLSWrapper::operator=;
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;

    CBLSPublicKey() {}

    void AggregateInsecure(const CBLSPublicKey& o);
    static CBLSPublicKey AggregateInsecure(const std::vector<CBLSPublicKey>& pks);

    bool PublicKeyShare(const std::vector<CBLSPublicKey>& mpk, const CBLSId& id);
    bool DHKeyExchange(const CBLSSecretKey& sk, const CBLSPublicKey& pk);

protected:
    bool InternalSetBuf(const void* buf);
    bool InternalGetBuf(void* buf) const;
};

class CBLSSignature : public CBLSWrapper<bls::InsecureSignature, BLS_CURVE_SIG_SIZE, CBLSSignature>
{
    friend class CBLSSecretKey;

public:
    using CBLSWrapper::operator==;
    using CBLSWrapper::operator!=;
    using CBLSWrapper::CBLSWrapper;

    CBLSSignature() {}
    CBLSSignature(const CBLSSignature&) = default;
    CBLSSignature& operator=(const CBLSSignature&) = default;

    void AggregateInsecure(const CBLSSignature& o);
    static CBLSSignature AggregateInsecure(const std::vector<CBLSSignature>& sigs);
    static CBLSSignature AggregateSecure(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSPublicKey>& pks, const uint256& hash);

    void SubInsecure(const CBLSSignature& o);

    bool VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const;
    bool VerifyInsecureAggregated(const std::vector<CBLSPublicKey>& pubKeys, const std::vector<uint256>& hashes) const;

    bool VerifySecureAggregated(const std::vector<CBLSPublicKey>& pks, const uint256& hash) const;

    bool Recover(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSId>& ids);

protected:
    bool InternalSetBuf(const void* buf);
    bool InternalGetBuf(void* buf) const;
};

#ifndef BUILD_BITCOIN_INTERNAL
template<typename BLSObject>
class CBLSLazyWrapper
{
private:
    mutable std::mutex mutex;

    mutable char buf[BLSObject::SerSize];
    mutable bool bufValid{false};

    mutable BLSObject obj;
    mutable bool objInitialized{false};

    mutable uint256 hash;

public:
    CBLSLazyWrapper()
    {
        memset(buf, 0, sizeof(buf));
        // the all-zero buf is considered a valid buf, but the resulting object will return false for IsValid
        bufValid = true;
    }

    CBLSLazyWrapper(const CBLSLazyWrapper& r)
    {
        *this = r;
    }

    CBLSLazyWrapper& operator=(const CBLSLazyWrapper& r)
    {
        std::unique_lock<std::mutex> l(r.mutex);
        bufValid = r.bufValid;
        if (r.bufValid) {
            memcpy(buf, r.buf, sizeof(buf));
        } else {
            memset(buf, 0, sizeof(buf));
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
            obj.GetBuf(buf, sizeof(buf));
            bufValid = true;
            hash = uint256();
        }
        s.write(buf, sizeof(buf));
    }

    template<typename Stream>
    inline void Unserialize(Stream& s)
    {
        std::unique_lock<std::mutex> l(mutex);
        s.read(buf, sizeof(buf));
        bufValid = true;
        objInitialized = false;
        hash = uint256();
    }

    void Set(const BLSObject& _obj)
    {
        std::unique_lock<std::mutex> l(mutex);
        bufValid = false;
        objInitialized = true;
        obj = _obj;
        hash = uint256();
    }
    const BLSObject& Get() const
    {
        std::unique_lock<std::mutex> l(mutex);
        static BLSObject invalidObj;
        if (!bufValid && !objInitialized) {
            return invalidObj;
        }
        if (!objInitialized) {
            obj.SetBuf(buf, sizeof(buf));
            if (!obj.CheckMalleable(buf, sizeof(buf))) {
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
            return memcmp(buf, r.buf, sizeof(buf)) == 0;
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
            obj.GetBuf(buf, sizeof(buf));
            bufValid = true;
            hash = uint256();
        }
        if (hash.IsNull()) {
            UpdateHash();
        }
        return hash;
    }
private:
    void UpdateHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss.write(buf, sizeof(buf));
        hash = ss.GetHash();
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

#endif // BITGREEN_BLS_H
