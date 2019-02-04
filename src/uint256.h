// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <array>
#include <assert.h>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

/** Template base class for fixed-sized opaque blobs. */
template<unsigned int BITS>
class base_blob
{
    using data_type = std::array<uint8_t, BITS / 8>;
    std::unique_ptr<data_type> data;
public:
    base_blob() : data(new data_type)
    {
        SetNull();
    }

    base_blob(base_blob&&) = default;
    base_blob(const base_blob& other) : data(new data_type)
    {
        std::copy(other.begin(), other.end(), begin());
    }

    explicit base_blob(const std::vector<unsigned char>& vch);

    base_blob& operator=(base_blob&&) = default;
    base_blob& operator=(const base_blob& other)
    {
        if (this != &other)
            std::copy(other.begin(), other.end(), begin());
        return *this;
    }

    bool IsNull() const
    {
        for (auto i: *data)
            if (i != 0)
                return false;
        return true;
    }

    void SetNull()
    {
        data->fill(0);
    }

    inline int Compare(const base_blob& other) const { return memcmp(begin(), other.begin(), size()); }

    friend inline bool operator==(const base_blob& a, const base_blob& b) { return a.Compare(b) == 0; }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) { return a.Compare(b) != 0; }
    friend inline bool operator<(const base_blob& a, const base_blob& b) { return a.Compare(b) < 0; }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;

    uint8_t* begin()
    {
        return data->begin();
    }

    uint8_t* end()
    {
        return data->end();
    }

    const uint8_t* begin() const
    {
        return data->begin();
    }

    const uint8_t* end() const
    {
        return data->end();
    }

    constexpr std::size_t size() const
    {
        return data->size();
    }

    uint64_t GetUint64(int pos) const
    {
        const uint8_t* ptr = begin() + pos * 8;
        return ((uint64_t)ptr[0]) | \
               ((uint64_t)ptr[1]) << 8 | \
               ((uint64_t)ptr[2]) << 16 | \
               ((uint64_t)ptr[3]) << 24 | \
               ((uint64_t)ptr[4]) << 32 | \
               ((uint64_t)ptr[5]) << 40 | \
               ((uint64_t)ptr[6]) << 48 | \
               ((uint64_t)ptr[7]) << 56;
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s.write((const char*)begin(), size());
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s.read((char*)begin(), size());
    }
};

/** 160-bit opaque blob.
 * @note This type is called uint160 for historical reasons only. It is an opaque
 * blob of 160 bits and has no integer operations.
 */
class uint160 : public base_blob<160> {
public:
    uint160() = default;
    using base_blob<160>::base_blob;
    uint160(uint160&&) = default;
    uint160(const uint160&) = default;
    uint160& operator=(uint160&&) = default;
    uint160& operator=(const uint160&) = default;
};

/** 256-bit opaque blob.
 * @note This type is called uint256 for historical reasons only. It is an
 * opaque blob of 256 bits and has no integer operations. Use arith_uint256 if
 * those are required.
 */
class uint256 : public base_blob<256> {
public:
    uint256() = default;
    using base_blob<256>::base_blob;
    uint256(uint256&&) = default;
    uint256(const uint256&) = default;
    uint256& operator=(uint256&&) = default;
    uint256& operator=(const uint256&) = default;
};

/* uint256 from const char *.
 * This is a separate function because the constructor uint256(const char*) can result
 * in dangerously catching uint256(0).
 */
inline uint256 uint256S(const char *str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
/* uint256 from std::string.
 * This is a separate function because the constructor uint256(const std::string &str) can result
 * in dangerously catching uint256(0) via std::string(const char*).
 */
inline uint256 uint256S(const std::string& str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}

#endif // BITCOIN_UINT256_H
