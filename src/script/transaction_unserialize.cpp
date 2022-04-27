// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/transaction_unserialize.h>

#include <span.h>
#include <version.h>

#include <cstddef>
#include <cstring>
#include <ios>
#include <string>

namespace {

/** A class that deserializes a single CTransaction one time. */
class TxInputStream
{
public:
    TxInputStream(int nVersionIn, const unsigned char* txTo, size_t txToLen)
        : m_version(nVersionIn),
          m_data(txTo),
          m_remaining(txToLen)
    {
    }

    void read(Span<std::byte> dst)
    {
        if (dst.size() > m_remaining) {
            throw std::ios_base::failure(std::string(__func__) + ": end of data");
        }

        if (dst.data() == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");
        }

        if (m_data == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");
        }

        memcpy(dst.data(), m_data, dst.size());
        m_remaining -= dst.size();
        m_data += dst.size();
    }

    template <typename T>
    TxInputStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

    int GetVersion() const { return m_version; }

private:
    const int m_version;
    const unsigned char* m_data;
    size_t m_remaining;
};
} // namespace

CTransaction bitcoinconsensus::UnserializeTx(const unsigned char* txTo, unsigned int txToLen)
{
    TxInputStream stream{PROTOCOL_VERSION, txTo, txToLen};
    return {deserialize, stream};
}

size_t bitcoinconsensus::TxSize(const CTransaction& tx)
{
    return GetSerializeSize(tx, PROTOCOL_VERSION);
}
