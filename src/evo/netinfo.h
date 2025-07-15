// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>
#include <streams.h>

#include <variant>

class CService;

class UniValue;

enum class NetInfoStatus : uint8_t {
    // Managing entries
    BadInput,
    MaxLimit,

    // Validation
    BadAddress,
    BadPort,
    BadType,
    NotRoutable,
    Malformed,

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
    case NetInfoStatus::Malformed:
        return "malformed";
    case NetInfoStatus::MaxLimit:
        return "too many entries";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

/* Identical to IsDeprecatedRPCEnabled("service"). For use outside of RPC code. */
bool IsServiceDeprecatedRPCEnabled();

class NetInfoEntry
{
public:
    enum NetInfoType : uint8_t {
        Service = 0x01,
        Invalid = 0xff
    };

private:
    uint8_t m_type{NetInfoType::Invalid};
    std::variant<std::monostate, CService> m_data{std::monostate{}};

public:
    NetInfoEntry() = default;
    NetInfoEntry(const CService& service)
    {
        if (!service.IsValid()) return;
        m_type = NetInfoType::Service;
        m_data = service;
    }
    template <typename Stream>
    NetInfoEntry(deserialize_type, Stream& s) { s >> *this; }

    ~NetInfoEntry() = default;

    bool operator<(const NetInfoEntry& rhs) const;
    bool operator==(const NetInfoEntry& rhs) const;
    bool operator!=(const NetInfoEntry& rhs) const { return !(*this == rhs); }

    template <typename Stream>
    void Serialize(Stream& s_) const
    {
        OverrideStream<Stream> s(&s_, /*nType=*/0, s_.GetVersion() | ADDRV2_FORMAT);
        if (const auto* data_ptr{std::get_if<CService>(&m_data)};
            m_type == NetInfoType::Service && data_ptr && data_ptr->IsValid()) {
            s << m_type << *data_ptr;
        } else {
            s << NetInfoType::Invalid;
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s_)
    {
        OverrideStream<Stream> s(&s_, /*nType=*/0, s_.GetVersion() | ADDRV2_FORMAT);
        s >> m_type;
        if (m_type == NetInfoType::Service) {
            m_data = CService{};
            try {
                CService& service{std::get<CService>(m_data)};
                s >> service;
                if (!service.IsValid()) { Clear(); } // Invalid CService, mark as invalid
            } catch (const std::ios_base::failure&) { Clear(); } // Deser failed, mark as invalid
        } else { Clear(); } // Invalid type code, mark as invalid
    }

    void Clear()
    {
        m_type = NetInfoType::Invalid;
        m_data = std::monostate{};
    }

    std::optional<std::reference_wrapper<const CService>> GetAddrPort() const;
    uint16_t GetPort() const;
    bool IsEmpty() const { return *this == NetInfoEntry{}; }
    bool IsTriviallyValid() const;
    std::string ToString() const;
    std::string ToStringAddr() const;
    std::string ToStringAddrPort() const;
};

template<> struct is_serializable_enum<NetInfoEntry::NetInfoType> : std::true_type {};

using NetInfoList = std::vector<std::reference_wrapper<const NetInfoEntry>>;

class NetInfoInterface
{
public:
    static std::shared_ptr<NetInfoInterface> MakeNetInfo(const uint16_t nVersion);

public:
    virtual ~NetInfoInterface() = default;

    virtual NetInfoStatus AddEntry(const std::string& service) = 0;
    virtual NetInfoList GetEntries() const = 0;

    virtual const CService& GetPrimary() const = 0;
    virtual bool CanStorePlatform() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual NetInfoStatus Validate() const = 0;
    virtual UniValue ToJson() const = 0;
    virtual std::string ToString() const = 0;

    virtual void Clear() = 0;

    bool operator==(const NetInfoInterface& rhs) const { return typeid(*this) == typeid(rhs) && this->IsEqual(rhs); }
    bool operator!=(const NetInfoInterface& rhs) const { return !(*this == rhs); }

private:
    virtual bool IsEqual(const NetInfoInterface& rhs) const = 0;
};

class MnNetInfo final : public NetInfoInterface
{
private:
    NetInfoEntry m_addr{};

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    template <typename Stream>
    MnNetInfo(deserialize_type, Stream& s) { s >> *this; }

    ~MnNetInfo() = default;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (const auto& service{m_addr.GetAddrPort()}; service.has_value()) {
            s << service->get();
        } else {
            s << CService{};
        }
    }

    void Serialize(CSizeComputer& s) const
    {
        s.seek(::GetSerializeSize(CService{}, s.GetVersion()));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        CService service;
        s >> service;
        m_addr = NetInfoEntry{service};
    }

    NetInfoStatus AddEntry(const std::string& service) override;
    NetInfoList GetEntries() const override;

    const CService& GetPrimary() const override;
    bool IsEmpty() const override { return m_addr.IsEmpty(); }
    bool CanStorePlatform() const override { return false; }
    NetInfoStatus Validate() const override;
    UniValue ToJson() const override;
    std::string ToString() const override;

    void Clear() override { m_addr.Clear(); }

private:
    // operator== and operator!= are defined by the parent which then leverage the child's IsEqual() override
    // IsEqual() should only be called by NetInfoInterface::operator== otherwise static_cast assumption could fail
    bool IsEqual(const NetInfoInterface& rhs) const override
    {
        ASSERT_IF_DEBUG(typeid(*this) == typeid(rhs));
        const auto& rhs_obj{static_cast<const MnNetInfo&>(rhs)};
        return m_addr == rhs_obj.m_addr;
    }
};

class NetInfoSerWrapper
{
private:
    std::shared_ptr<NetInfoInterface>& m_data;
    const bool m_is_extended{false};

public:
    NetInfoSerWrapper() = delete;
    NetInfoSerWrapper(const NetInfoSerWrapper&) = delete;
    NetInfoSerWrapper(std::shared_ptr<NetInfoInterface>& data, const bool is_extended) :
        m_data{data},
        m_is_extended{is_extended}
    {
        // TODO: Remove when extended addresses implementation is added in
        assert(!m_is_extended);
    }

    ~NetInfoSerWrapper() = default;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (const auto ptr{std::dynamic_pointer_cast<MnNetInfo>(m_data)}) {
            s << *ptr;
        } else {
            // NetInfoInterface::MakeNetInfo() supplied an unexpected implementation or we didn't call it and
            // are left with a nullptr. Neither should happen.
            assert(false);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        m_data = std::make_shared<MnNetInfo>(deserialize, s);
    }
};

#endif // BITCOIN_EVO_NETINFO_H
