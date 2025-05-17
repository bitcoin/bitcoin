// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

class CService;

enum class NetInfoStatus : uint8_t {
    // Managing entries
    BadInput,

    // Validation
    BadAddress,
    BadPort,
    BadType,
    NotRoutable,

    Success
};

constexpr std::string_view NISToString(const NetInfoStatus code)
{
    switch (code) {
    case NetInfoStatus::BadAddress:
        return "invalid address";
    case NetInfoStatus::BadInput:
        return "invalid input";
    case NetInfoStatus::BadPort:
        return "invalid port";
    case NetInfoStatus::BadType:
        return "invalid address type";
    case NetInfoStatus::NotRoutable:
        return "unroutable address";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

using CServiceList = std::vector<std::reference_wrapper<const CService>>;

class MnNetInfo
{
private:
    CService m_addr{};

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return m_addr == rhs.m_addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.m_addr);
    }

    NetInfoStatus AddEntry(const std::string& service);
    CServiceList GetEntries() const;

    const CService& GetPrimary() const { return m_addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const { return ValidateService(m_addr); }
    std::string ToString() const;

    void Clear() { m_addr = CService(); }
};

#endif // BITCOIN_EVO_NETINFO_H
