#include <mw/crypto/Hasher.h>

#define BLAKE3_NO_AVX512 1
#define BLAKE3_NO_AVX2 1
#define BLAKE3_NO_SSE41 1
#define BLAKE3_NO_SSE2 1
extern "C" {
#include <crypto/blake3/blake3.c>
#include <crypto/blake3/blake3_dispatch.c>
#include <crypto/blake3/blake3_portable.c>
}

mw::Hash Hashed(const std::vector<uint8_t>& serialized)
{
    Hasher hasher;
    hasher.write((char*)serialized.data(), serialized.size());
    return hasher.hash();
}

mw::Hash Hashed(const Traits::ISerializable& serializable)
{
    return Hashed(serializable.Serialized());
}