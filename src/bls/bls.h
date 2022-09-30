// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_CRYPTO_BLS_H
#define DASH_CRYPTO_BLS_H

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/ranges.h>

// bls-dash uses relic, which may define DEBUG and ERROR, which leads to many warnings in some build setups
#undef ERROR
#undef DEBUG
#include <bls-dash/bls.hpp>
#include <bls-dash/privatekey.hpp>
#include <bls-dash/elements.hpp>
#include <bls-dash/schemes.hpp>
#include <bls-dash/threshold.hpp>
#undef DOUBLE
#undef SEED

#include <array>
#include <mutex>
#include <unistd.h>

#include <atomic>

namespace bls {
    extern std::atomic<bool> bls_legacy_scheme;
}

// reversed BLS12-381
constexpr int BLS_CURVE_ID_SIZE{32};
constexpr int BLS_CURVE_SECKEY_SIZE{32};
constexpr int BLS_CURVE_PUBKEY_SIZE{48};
constexpr int BLS_CURVE_SIG_SIZE{96};

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

public:
    static constexpr size_t SerSize = _SerSize;

    explicit CBLSWrapper() = default;
    explicit CBLSWrapper(const std::vector<unsigned char>& vecBytes) : CBLSWrapper<ImplType, _SerSize, C>()
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
    }
    CBLSWrapper& operator=(CBLSWrapper&& ref) noexcept
    {
        std::swap(impl, ref.impl);
        std::swap(fValid, ref.fValid);
        std::swap(cachedHash, ref.cachedHash);
        return *this;
    }

    virtual ~CBLSWrapper() = default;

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
        *(static_cast<C*>(this)) = C();
    }

    //TODO: Overwrite function with default value of specificLegacyScheme
    void SetByteVector(const std::vector<uint8_t>& vecBytes, const bool specificLegacyScheme)
    {
        if (vecBytes.size() != SerSize) {
            Reset();
            return;
        }

        if (ranges::all_of(vecBytes, [](uint8_t c) { return c == 0; })) {
            Reset();
        } else {
            try {
                impl = ImplType::FromBytes(bls::Bytes(vecBytes), specificLegacyScheme);
                fValid = true;
            } catch (...) {
                Reset();
            }
        }
        cachedHash.SetNull();
    }

    void SetByteVector(const std::vector<uint8_t>& vecBytes)
    {
        SetByteVector(vecBytes, bls::bls_legacy_scheme.load());
    }

    //TODO: Overwrite function with default value of specificLegacyScheme
    std::vector<uint8_t> ToByteVector(const bool specificLegacyScheme) const
    {
        if (!fValid) {
            return std::vector<uint8_t>(SerSize, 0);
        }
        return impl.Serialize(specificLegacyScheme);
    }

    std::vector<uint8_t> ToByteVector() const
    {
        return ToByteVector(bls::bls_legacy_scheme.load());
    }

    const uint256& GetHash() const
    {
        if (cachedHash.IsNull()) {
            cachedHash = ::SerializeHash(*this);
        }
        return cachedHash;
    }

    //TODO: Add version with argument (bool specificLegacyScheme)
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

    inline void Serialize(CSizeComputer& s) const
    {
        s.seek(SerSize);
    }

    //TODO: Overwrite function with default value of specificLegacyScheme
    template <typename Stream>
    inline void Serialize(Stream& s, const bool specificLegacyScheme) const
    {
        s.write(reinterpret_cast<const char*>(ToByteVector(specificLegacyScheme).data()), SerSize);
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        Serialize(s, bls::bls_legacy_scheme.load());
    }

    //TODO: Overwrite function with default value of specificLegacyScheme
    template <typename Stream>
    inline void Unserialize(Stream& s, const bool specificLegacyScheme, bool checkMalleable = true)
    {
        std::vector<uint8_t> vecBytes(SerSize, 0);
        s.read(reinterpret_cast<char*>(vecBytes.data()), SerSize);
        SetByteVector(vecBytes, specificLegacyScheme);

        if (checkMalleable && !CheckMalleable(vecBytes)) {
            throw std::ios_base::failure("malleable BLS object");
        }
    }

    template <typename Stream>
    inline void Unserialize(Stream& s, bool checkMalleable = true)
    {
        Unserialize(s, bls::bls_legacy_scheme.load(), checkMalleable);
    }

    //TODO: Overwrite function with default value of specificLegacyScheme
    inline bool CheckMalleable(const std::vector<uint8_t>& vecBytes, const bool specificLegacyScheme) const
    {
        if (memcmp(vecBytes.data(), ToByteVector(specificLegacyScheme).data(), SerSize)) {
            // TODO not sure if this is actually possible with the BLS libs. I'm assuming here that somewhere deep inside
            // these libs masking might happen, so that 2 different binary representations could result in the same object
            // representation
            return false;
        }
        return true;
    }

    inline bool CheckMalleable(const std::vector<uint8_t>& vecBytes) const
    {
        return CheckMalleable(vecBytes, bls::bls_legacy_scheme.load());
    }

    inline std::string ToString(const bool specificLegacyScheme) const
    {
        std::vector<uint8_t> buf = ToByteVector(specificLegacyScheme);
        return HexStr(buf);
    }

    inline std::string ToString() const
    {
        return ToString(bls::bls_legacy_scheme.load());
    }
};

struct CBLSIdImplicit : public uint256
{
    CBLSIdImplicit() = default;
    CBLSIdImplicit(const uint256& id)
    {
        memcpy(begin(), id.begin(), sizeof(uint256));
    }
    static CBLSIdImplicit FromBytes(const uint8_t* buffer, const bool fLegacy)
    {
        CBLSIdImplicit instance;
        memcpy(instance.begin(), buffer, sizeof(CBLSIdImplicit));
        return instance;
    }
    [[nodiscard]] std::vector<uint8_t> Serialize(const bool fLegacy) const
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

    [[nodiscard]] CBLSPublicKey GetPublicKey() const;
    [[nodiscard]] CBLSSignature Sign(const uint256& hash) const;
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
    static CBLSPublicKey AggregateInsecure(const std::vector<CBLSPublicKey>& pks);

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
    static CBLSSignature AggregateInsecure(const std::vector<CBLSSignature>& sigs);
    static CBLSSignature AggregateSecure(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSPublicKey>& pks, const uint256& hash);

    void SubInsecure(const CBLSSignature& o);

    [[nodiscard]] bool VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const;
    [[nodiscard]] bool VerifyInsecureAggregated(const std::vector<CBLSPublicKey>& pubKeys, const std::vector<uint256>& hashes) const;

    [[nodiscard]] bool VerifySecureAggregated(const std::vector<CBLSPublicKey>& pks, const uint256& hash) const;

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

    explicit CBLSLazyWrapper(const CBLSLazyWrapper& r)
    {
        *this = r;
    }
    virtual ~CBLSLazyWrapper() = default;

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
        s.write(reinterpret_cast<const char*>(vecBytes.data()), vecBytes.size());
    }

    template<typename Stream>
    inline void Unserialize(Stream& s)
    {
        std::unique_lock<std::mutex> l(mutex);
        s.read(reinterpret_cast<char*>(vecBytes.data()), BLSObject::SerSize);
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
            ss.write(reinterpret_cast<const char*>(vecBytes.data()), vecBytes.size());
            hash = ss.GetHash();
        }
        return hash;
    }
};
using CBLSLazySignature = CBLSLazyWrapper<CBLSSignature>;
using CBLSLazyPublicKey = CBLSLazyWrapper<CBLSPublicKey>;
#endif

using BLSIdVector = std::vector<CBLSId>;
using BLSVerificationVector = std::vector<CBLSPublicKey>;
using BLSPublicKeyVector = std::vector<CBLSPublicKey>;
using BLSSecretKeyVector = std::vector<CBLSSecretKey>;
using BLSSignatureVector = std::vector<CBLSSignature>;

using BLSVerificationVectorPtr = std::shared_ptr<BLSVerificationVector>;

bool BLSInit();

#endif // DASH_CRYPTO_BLS_H
