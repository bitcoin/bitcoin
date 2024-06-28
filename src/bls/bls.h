// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_BLS_BLS_H
#define SYSCOIN_BLS_BLS_H

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/ranges.h>

// bls-dash uses relic, which may define DEBUG and ERROR, which leads to many warnings in some build setups
#undef ERROR
#undef DEBUG
#include <dashbls/bls.hpp>
#include <dashbls/privatekey.hpp>
#include <dashbls/elements.hpp>
#include <dashbls/schemes.hpp>
#include <dashbls/threshold.hpp>
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
    explicit CBLSWrapper(Span<const unsigned char> vecBytes) : CBLSWrapper<ImplType, _SerSize, C>()
    {
        SetByteVector(vecBytes, bls::bls_legacy_scheme.load());
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
    bool operator<(const C& r) const
    {
        return GetHash() < r.GetHash();
    }

    bool IsValid() const
    {
        return fValid;
    }

    void Reset()
    {
        *(static_cast<C*>(this)) = C();
    }

    void SetByteVector(Span<const uint8_t> vecBytes, const bool specificLegacyScheme)
    {
        if (vecBytes.size() != SerSize) {
            Reset();
            return;
        }

        if (ranges::all_of(vecBytes, [](uint8_t c) { return c == 0; })) {
            Reset();
        } else {
            try {
                impl = ImplType::FromBytes(bls::Bytes(vecBytes.data(), vecBytes.size()), specificLegacyScheme);
                fValid = true;
            } catch (...) {
                Reset();
            }
        }
        cachedHash.SetNull();
    }

    std::vector<uint8_t> ToByteVector(const bool specificLegacyScheme) const
    {
        if (!fValid) {
            return std::vector<uint8_t>(SerSize, 0);
        }
        return impl.Serialize(specificLegacyScheme);
    }
    void SetByteVector(const std::vector<uint8_t>& vecBytes)
    {
        SetByteVector(vecBytes, bls::bls_legacy_scheme.load());
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
    bool SetHexStr(const std::string& str)
    {
        return SetHexStr(str, bls::bls_legacy_scheme.load());
    }
    bool SetHexStr(const std::string& str, const bool specificLegacyScheme)
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
        SetByteVector(b, specificLegacyScheme);
        return IsValid();
    }

    inline void Serialize(CSizeComputer& s) const
    {
        s.seek(SerSize);
    }

    template <typename Stream>
    inline void Serialize(Stream& s, const bool specificLegacyScheme) const
    {
        s.write(AsBytes(Span{ToByteVector(specificLegacyScheme).data(), SerSize}));
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        Serialize(s, bls::bls_legacy_scheme.load());
    }

    template <typename Stream>
    inline void Unserialize(Stream& s, const bool specificLegacyScheme)
    {
        std::array<uint8_t, SerSize> vecBytes{};
        s.read(AsWritableBytes(Span{vecBytes.data(), SerSize}));
        SetByteVector(vecBytes, specificLegacyScheme);

        if (!CheckMalleable(vecBytes, specificLegacyScheme)) {
            // If CheckMalleable failed with specificLegacyScheme, we need to try again with the opposite scheme.
            // Probably we received the BLS object sent with legacy scheme, but in the meanwhile the fork activated.
            SetByteVector(vecBytes, !specificLegacyScheme);
            if (!CheckMalleable(vecBytes, !specificLegacyScheme)) {
                // Both attempts failed
                throw std::ios_base::failure("malleable BLS object");
            } else {
                // Indeed the received vecBytes was in opposite scheme. But we can't keep it (mixing with the new scheme will lead to undefined behavior)
                // Therefore, resetting current object (basically marking it as invalid).
                Reset();
            }
        }
    }

    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        Unserialize(s, bls::bls_legacy_scheme.load());
    }

    inline bool CheckMalleable(Span<uint8_t> vecBytes, const bool specificLegacyScheme) const
    {
        if (memcmp(vecBytes.data(), ToByteVector(specificLegacyScheme).data(), SerSize)) {
            // TODO not sure if this is actually possible with the BLS libs. I'm assuming here that somewhere deep inside
            // these libs masking might happen, so that 2 different binary representations could result in the same object
            // representation
            return false;
        }
        return true;
    }

    inline bool CheckMalleable(Span<uint8_t> vecBytes) const
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
    static CBLSSecretKey AggregateInsecure(Span<CBLSSecretKey> sks);

#ifndef BUILD_SYSCOIN_INTERNAL
    void MakeNewKey();
#endif
    bool SecretKeyShare(Span<CBLSSecretKey> msk, const CBLSId& id);

    [[nodiscard]] CBLSPublicKey GetPublicKey() const;
    [[nodiscard]] CBLSSignature Sign(const uint256& hash) const;
    [[nodiscard]] CBLSSignature Sign(const uint256& hash, const bool specificLegacyScheme) const;
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
    static CBLSPublicKey AggregateInsecure(Span<CBLSPublicKey> pks);

    bool PublicKeyShare(Span<CBLSPublicKey> mpk, const CBLSId& id);
    bool DHKeyExchange(const CBLSSecretKey& sk, const CBLSPublicKey& pk);

};

class ConstCBLSPublicKeyVersionWrapper {
private:
    const CBLSPublicKey& obj;
    bool legacy;
public:
    ConstCBLSPublicKeyVersionWrapper(const CBLSPublicKey& obj, bool legacy)
            : obj(obj)
            , legacy(legacy)
    {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        obj.Serialize(s, legacy);
    }
};

class CBLSPublicKeyVersionWrapper {
private:
    CBLSPublicKey& obj;
    bool legacy;
public:
    CBLSPublicKeyVersionWrapper(CBLSPublicKey& obj, bool legacy)
            : obj(obj)
            , legacy(legacy)
    {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        obj.Serialize(s, legacy);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        obj.Unserialize(s, legacy);
    }
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
    static CBLSSignature AggregateInsecure(Span<CBLSSignature> sigs);
    static CBLSSignature AggregateSecure(Span<CBLSSignature> sigs, Span<CBLSPublicKey> pks, const uint256& hash);

    void SubInsecure(const CBLSSignature& o);
    [[nodiscard]] bool VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash, const bool specificLegacyScheme) const;
    [[nodiscard]] bool VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const;
    [[nodiscard]] bool VerifyInsecureAggregated(Span<CBLSPublicKey> pubKeys, Span<uint256> hashes) const;

    [[nodiscard]] bool VerifySecureAggregated(Span<CBLSPublicKey> pks, const uint256& hash) const;

    bool Recover(Span<CBLSSignature> sigs, Span<CBLSId> ids);
};

class CBLSSignatureVersionWrapper {
private:
    CBLSSignature& obj;
    bool legacy;
public:
    CBLSSignatureVersionWrapper(CBLSSignature& obj, bool legacy)
            : obj(obj)
            , legacy(legacy)
    {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        obj.Serialize(s, legacy);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        obj.Unserialize(s, legacy);
    }
};

#ifndef BUILD_SYSCOIN_INTERNAL
template<typename BLSObject>
class CBLSLazyWrapper
{
private:
    mutable std::mutex mutex;

    mutable std::vector<uint8_t> vecBytes;
    mutable bool bufValid{false};
    mutable bool bufLegacyScheme{true};

    mutable BLSObject obj;
    mutable bool objInitialized{false};

    mutable uint256 hash;

public:
    CBLSLazyWrapper() :
            vecBytes(BLSObject::SerSize, 0),
            bufLegacyScheme(bls::bls_legacy_scheme.load())
    {}

    explicit CBLSLazyWrapper(const CBLSLazyWrapper& r)
    {
        *this = r;
    }
    virtual ~CBLSLazyWrapper() = default;

    CBLSLazyWrapper& operator=(const CBLSLazyWrapper& r)
    {
        std::unique_lock<std::mutex> l(r.mutex);
        bufValid = r.bufValid;
        bufLegacyScheme = r.bufLegacyScheme;
        if (r.bufValid) {
            vecBytes = r.vecBytes;
        } else {
            vecBytes.resize(BLSObject::SerSize);
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
    inline void Serialize(Stream& s, const bool specificLegacyScheme) const
    {
        std::unique_lock<std::mutex> l(mutex);
        if (!objInitialized && !bufValid) {
            vecBytes.resize(BLSObject::SerSize);
            std::fill(vecBytes.begin(), vecBytes.end(), 0);
        } else if (!bufValid || (bufLegacyScheme != specificLegacyScheme)) {
            vecBytes = obj.ToByteVector(specificLegacyScheme);
            bufValid = true;
            bufLegacyScheme = specificLegacyScheme;
            hash.SetNull();
        }
        s.write(MakeByteSpan(vecBytes));
    }

    template<typename Stream>
    inline void Serialize(Stream& s) const
    {
        Serialize(s, bufLegacyScheme);
    }

    template<typename Stream>
    inline void Unserialize(Stream& s, const bool specificLegacyScheme) const
    {
        std::unique_lock<std::mutex> l(mutex);
        s.read(AsWritableBytes(Span{vecBytes.data(), BLSObject::SerSize}));
        bufValid = true;
        bufLegacyScheme = specificLegacyScheme;
        objInitialized = false;
        hash.SetNull();
    }

    template<typename Stream>
    inline void Unserialize(Stream& s) const
    {
        Unserialize(s, bufLegacyScheme);
    }

    void Set(const BLSObject& _obj, const bool specificLegacyScheme)
    {
        std::unique_lock<std::mutex> l(mutex);
        bufValid = false;
        bufLegacyScheme = specificLegacyScheme;
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
            obj.SetByteVector(vecBytes, bufLegacyScheme);
            if (!obj.IsValid()) {
                bufValid = false;
                return invalidObj;
            }
            if (!obj.CheckMalleable(vecBytes, bufLegacyScheme)) {
                bufValid = false;
                return invalidObj;
            }
            objInitialized = true;
        }
        return obj;
    }

    bool operator==(const CBLSLazyWrapper& r) const
    {
        if (bufValid && r.bufValid && bufLegacyScheme == r.bufLegacyScheme) {
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
        if (!objInitialized && !bufValid) {
            vecBytes.resize(BLSObject::SerSize);
            std::fill(vecBytes.begin(), vecBytes.end(), 0);
            hash.SetNull();
        } else if (!bufValid) {
            vecBytes = obj.ToByteVector(bufLegacyScheme);
            bufValid = true;
            hash.SetNull();
        }
        if (hash.IsNull()) {
            CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
            ss.write(MakeByteSpan(vecBytes));
            hash = ss.GetHash();
        }
        return hash;
    }

    bool IsLegacy() const
    {
        return bufLegacyScheme;
    }

    void SetLegacy(bool specificLegacyScheme)
    {
        bufLegacyScheme = specificLegacyScheme;
    }

    std::string ToString() const
    {
        return Get().ToString(bufLegacyScheme);
    }
};
using CBLSLazySignature = CBLSLazyWrapper<CBLSSignature>;
using CBLSLazyPublicKey = CBLSLazyWrapper<CBLSPublicKey>;

class CBLSLazyPublicKeyVersionWrapper {
private:
    CBLSLazyPublicKey& obj;
    bool legacy;
public:
    CBLSLazyPublicKeyVersionWrapper(CBLSLazyPublicKey& obj, bool legacy)
            : obj(obj)
            , legacy(legacy)
    {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        obj.Serialize(s, legacy);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        obj.Unserialize(s, legacy);
    }
};
#endif

using BLSVerificationVectorPtr = std::shared_ptr<std::vector<CBLSPublicKey>>;

bool BLSInit();

#endif // SYSCOIN_BLS_BLS_H
