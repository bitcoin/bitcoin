#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include <version.h>

// Forward Declarations
template <size_t NUM_BYTES, class ALLOC>
class BigInt;

class Commitment;
namespace mw { 
using Hash = BigInt<32, std::allocator<uint8_t>>;
}

#define IMPL_SERIALIZED(T)                                                  \
    std::vector<uint8_t> Serialized() const noexcept final                  \
    {                                                                       \
        std::vector<uint8_t> serialized;                                    \
        CVectorWriter stream(SER_NETWORK, PROTOCOL_VERSION, serialized, 0); \
        ::Serialize(stream, *this);                                         \
        return serialized;                                                  \
    }                                                                       \
    static T Deserialize(const std::vector<uint8_t>& serialized)            \
    {                                                                       \
        T obj;                                                              \
        VectorReader stream(SER_NETWORK, PROTOCOL_VERSION, serialized, 0);  \
        stream >> obj;                                                      \
        return obj;                                                         \
    }

#define IMPL_SERIALIZABLE(T, obj) \
    IMPL_SERIALIZED(T)            \
    SERIALIZE_METHODS(T, obj)
    

namespace Traits
{
    class ISerializable
    {
    public:
        virtual ~ISerializable() = default;

        //
        // Serializes object into a byte vector.
        //
        virtual std::vector<uint8_t> Serialized() const noexcept = 0;
    };

    class IPrintable
    {
    public:
        virtual ~IPrintable() = default;

        virtual std::string Format() const = 0;
    };

    class ICommitted
    {
    public:
        virtual ~ICommitted() = default;

        virtual const Commitment& GetCommitment() const noexcept = 0;
    };
    
    class IHashable
    {
    public:
        virtual ~IHashable() = default;

        virtual const mw::Hash& GetHash() const noexcept = 0;
    };
}