#pragma once

#include <mw/common/Traits.h>
#include <mw/models/crypto/BigInteger.h>
#include <mw/models/crypto/SecretKey.h>
#include <pubkey.h>

#include <boost/functional/hash.hpp>

class PublicKey :
    public Traits::IPrintable,
    public Traits::ISerializable
{
public:
    //
    // Constructors
    //
    PublicKey() = default;
    PublicKey(BigInt<33>&& compressed) : m_compressed(std::move(compressed)) {}
    PublicKey(const BigInt<33>& compressed) : m_compressed(compressed) {}
    PublicKey(const uint8_t* compressed) : m_compressed(compressed) {}
    PublicKey(const PublicKey&) = default;
    PublicKey(PublicKey&&) noexcept = default;

    //
    // Destructor
    //
    virtual ~PublicKey() = default;

    //
    // Operators
    //
    PublicKey& operator=(const PublicKey& rhs) = default;
    PublicKey& operator=(PublicKey&& other) noexcept = default;
    bool operator==(const PublicKey& rhs) const { return m_compressed == rhs.m_compressed; }
    bool operator!=(const PublicKey& rhs) const { return m_compressed != rhs.m_compressed; }
    bool operator<=(const PublicKey& rhs) const { return m_compressed <= rhs.m_compressed; }
    bool operator<(const PublicKey& rhs) const { return m_compressed < rhs.m_compressed; }
    bool operator>=(const PublicKey& rhs) const { return m_compressed >= rhs.m_compressed; }
    bool operator>(const PublicKey& rhs) const { return m_compressed > rhs.m_compressed; }

    //
    // Factory
    //
    static PublicKey From(const SecretKey& key);
    static PublicKey From(const Commitment& commitment);
    static PublicKey Random();

    const BigInt<33>& GetBigInt() const { return m_compressed; }
    CKeyID GetID() const { return CPubKey(vec()).GetID(); }
    std::array<uint8_t, 33> array() const { return m_compressed.ToArray(); }
    const std::vector<uint8_t>& vec() const { return m_compressed.vec(); }
    const uint8_t& operator[](const size_t x) const { return m_compressed[x]; }
    const uint8_t* data() const { return m_compressed.data(); }
    uint8_t* data() { return m_compressed.data(); }
    size_t size() const { return m_compressed.size(); }

    //
    // Point Arithmetic
    //
    PublicKey Mul(const SecretKey& mul) const;
    PublicKey Div(const SecretKey& div) const;
    PublicKey Add(const SecretKey& add) const;
    PublicKey Add(const PublicKey& add) const;
    PublicKey Sub(const SecretKey& sub) const;
    PublicKey Sub(const PublicKey& sub) const;

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(PublicKey, obj)
    {
        READWRITE(obj.m_compressed);
    }

    std::string ToHex() const { return m_compressed.ToHex(); }
    std::string Format() const final { return m_compressed.ToHex(); }

private:
    BigInt<33> m_compressed;
};

namespace std
{
    template<>
    struct hash<PublicKey>
    {
        size_t operator()(const PublicKey& pubkey) const
        {
            return boost::hash_value(pubkey.vec());
        }
    };
}