// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOB256_H
#define BITCOIN_BLOB256_H

#include <assert.h>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

/** Template base class for fixed-sized opaque blobs. */
template<unsigned int BITS>
class base_blob
{
protected:
    enum { WIDTH=BITS/8 };
    uint8_t data[WIDTH];
public:
    base_blob()
    {
        for (int i = 0; i < WIDTH; i++)
            data[i] = 0;
    }

    base_blob(const base_blob& b)
    {
        for (int i = 0; i < WIDTH; i++)
            data[i] = b.data[i];
    }

    base_blob& operator=(const base_blob& b)
    {
        for (int i = 0; i < WIDTH; i++)
            data[i] = b.data[i];
        return *this;
    }
    explicit base_blob(const std::string& str);
    explicit base_blob(const std::vector<unsigned char>& vch);

    bool IsNull() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (data[i] != 0)
                return false;
        return true;
    }
    void SetNull()
    {
        memset(data, 0, sizeof(data));
    }

    friend inline bool operator==(const base_blob& a, const base_blob& b) { return memcmp(a.data, b.data, sizeof(a.data)) == 0; }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) { return memcmp(a.data, b.data, sizeof(a.data)) != 0; }
    friend inline bool operator<(const base_blob& a, const base_blob& b) {
        for (int i = 0; i < WIDTH; i++)
        {
            if (a.data[i] < b.data[i])
                return true;
            else if (a.data[i] > b.data[i])
                return false;
        }
        return false;
    }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;

    unsigned char* begin()
    {
        return &data[0];
    }

    unsigned char* end()
    {
        return &data[WIDTH];
    }

    const unsigned char* begin() const
    {
        return &data[0];
    }

    const unsigned char* end() const
    {
        return &data[WIDTH];
    }

    unsigned int size() const
    {
        return sizeof(data);
    }
    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(data);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        s.write((char*)data, sizeof(data));
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        s.read((char*)data, sizeof(data));
    }

    /** A cheap hash function that just returns 64 bits from the result, it can be
     * used when the contents are considered uniformly random. It is not appropriate
     * when the value can easily be influenced from outside as e.g. a network adversary could
     * provide values to trigger worst-case behavior.
     * @note The result of this function is not stable between little and big endian.
     */
    uint64_t GetCheapHash() const
    {
        uint64_t result;
        memcpy((void*)&result, (void*)data, 8);
        return result;
    }
};

/** 160-bit opaque blob. */
class blob160 : public base_blob<160> {
public:
    blob160() {}
    blob160(const base_blob<160>& b) : base_blob<160>(b) {}
    explicit blob160(const std::string& str) : base_blob<160>(str) {}
    explicit blob160(const std::vector<unsigned char>& vch) : base_blob<160>(vch) {}
};

/** 256-bit opaque blob. */
class blob256 : public base_blob<256> {
public:
    blob256() {}
    blob256(const base_blob<256>& b) : base_blob<256>(b) {}
    explicit blob256(const std::string& str) : base_blob<256>(str) {}
    explicit blob256(const std::vector<unsigned char>& vch) : base_blob<256>(vch) {}

    /** A more secure, salted hash function.
     * @note This hash is not stable between little and big endian.
     */
    uint64_t GetHash(const blob256& salt) const;
};

#endif // BITCOIN_BLOB256_H
