// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <evo/providertx.h>
#include <netbase.h>
#include <span.h>
#include <util/check.h>
#include <util/string.h>
#include <util/system.h>

#include <univalue.h>

namespace {
static std::unique_ptr<const CChainParams> g_main_params{nullptr};
static std::once_flag g_main_params_flag;

static constexpr std::string_view SAFE_CHARS_IPV4{"1234567890."};
static constexpr std::string_view SAFE_CHARS_IPV4_6{"abcdefABCDEF1234567890.:[]"};

bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }
const CChainParams& MainParams()
{
    // TODO: use real args here
    std::call_once(g_main_params_flag,
                   [&]() { g_main_params = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN); });
    return *Assert(g_main_params);
}

bool MatchCharsFilter(std::string_view input, std::string_view filter)
{
    return std::all_of(input.begin(), input.end(), [&filter](char c) { return filter.find(c) != std::string_view::npos; });
}
} // anonymous namespace

UniValue ArrFromService(const CService& addr)
{
    UniValue obj(UniValue::VARR);
    obj.push_back(addr.ToStringAddrPort());
    return obj;
}

bool IsServiceDeprecatedRPCEnabled()
{
    const auto args = gArgs.GetArgs("-deprecatedrpc");
    return std::find(args.begin(), args.end(), "service") != args.end();
}

bool NetInfoEntry::operator==(const NetInfoEntry& rhs) const
{
    if (m_type != rhs.m_type) return false;
    return std::visit(
        [](auto&& lhs, auto&& rhs) -> bool {
            if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
                return lhs == rhs;
            }
            return false;
        },
        m_data, rhs.m_data);
}

bool NetInfoEntry::operator<(const NetInfoEntry& rhs) const
{
    if (m_type != rhs.m_type) return m_type < rhs.m_type;
    return std::visit(
        [](auto&& lhs, auto&& rhs) -> bool {
            using T1 = std::decay_t<decltype(lhs)>;
            using T2 = std::decay_t<decltype(rhs)>;
            if constexpr (std::is_same_v<T1, T2>) {
                // Both the same type, compare as usual
                return lhs < rhs;
            }
            // If lhs is monostate, it less than rhs; otherwise rhs is greater
            return std::is_same_v<T1, std::monostate>;
        },
        m_data, rhs.m_data);
}

std::optional<CService> NetInfoEntry::GetAddrPort() const
{
    if (const auto* data_ptr{std::get_if<CService>(&m_data)}; m_type == NetInfoType::Service && data_ptr) {
        ASSERT_IF_DEBUG(data_ptr->IsValid());
        return *data_ptr;
    }
    return std::nullopt;
}

uint16_t NetInfoEntry::GetPort() const
{
    return std::visit(
        [](auto&& input) -> uint16_t {
            using T1 = std::decay_t<decltype(input)>;
            if constexpr (std::is_same_v<T1, CService>) {
                return input.GetPort();
            }
            return 0;
        },
        m_data);
}

// NetInfoEntry is a dumb object that doesn't enforce validation rules, that is the responsibility of
// types that utilize NetInfoEntry (MnNetInfo and others). IsTriviallyValid() is there to check if a
// NetInfoEntry object is properly constructed.
bool NetInfoEntry::IsTriviallyValid() const
{
    if (m_type == NetInfoType::Invalid) return false;
    return std::visit(
        [this](auto&& input) -> bool {
            using T1 = std::decay_t<decltype(input)>;
            static_assert(std::is_same_v<T1, std::monostate> || std::is_same_v<T1, CService>, "Unexpected type");
            if constexpr (std::is_same_v<T1, std::monostate>) {
                // Empty underlying data isn't a valid entry
                return false;
            } else if constexpr (std::is_same_v<T1, CService>) {
                // Type code should be truthful as it decides what underlying type is used when (de)serializing
                if (m_type != NetInfoType::Service) return false;
                // Underlying data must meet surface-level validity checks for its type
                if (!input.IsValid()) return false;
            }
            return true;
        },
        m_data);
}

std::string NetInfoEntry::ToString() const
{
    return std::visit(
        [](auto&& input) -> std::string {
            using T1 = std::decay_t<decltype(input)>;
            if constexpr (std::is_same_v<T1, CService>) {
                return strprintf("CService(addr=%s, port=%u)", input.ToStringAddr(), input.GetPort());
            }
            return "[invalid entry]";
        },
        m_data);
}

std::string NetInfoEntry::ToStringAddr() const
{
    return std::visit(
        [](auto&& input) -> std::string {
            using T1 = std::decay_t<decltype(input)>;
            if constexpr (std::is_same_v<T1, CService>) {
                return input.ToStringAddr();
            }
            return "[invalid entry]";
        },
        m_data);
}

std::string NetInfoEntry::ToStringAddrPort() const
{
    return std::visit(
        [](auto&& input) -> std::string {
            using T1 = std::decay_t<decltype(input)>;
            if constexpr (std::is_same_v<T1, CService>) {
                return input.ToStringAddrPort();
            }
            return "[invalid entry]";
        },
        m_data);
}

std::shared_ptr<NetInfoInterface> NetInfoInterface::MakeNetInfo(const uint16_t nVersion)
{
    assert(nVersion > 0);
    if (nVersion >= ProTxVersion::ExtAddr) {
        return std::make_shared<ExtNetInfo>();
    }
    return std::make_shared<MnNetInfo>();
}

NetInfoStatus MnNetInfo::ValidateService(const CService& service)
{
    if (!service.IsValid()) {
        return NetInfoStatus::BadAddress;
    }
    if (!service.IsIPv4()) {
        return NetInfoStatus::BadType;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return NetInfoStatus::NotRoutable;
    }

    if (IsNodeOnMainnet() != (service.GetPort() == MainParams().GetDefaultPort())) {
        // Must use mainnet port on mainnet.
        // Must NOT use mainnet port on other networks.
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus MnNetInfo::AddEntry(const std::string& input)
{
    if (!IsEmpty()) {
        return NetInfoStatus::MaxLimit;
    }

    std::string addr;
    uint16_t port{Params().GetDefaultPort()};
    SplitHostPort(input, port, addr);
    // Contains invalid characters, unlikely to pass Lookup(), fast-fail
    if (!MatchCharsFilter(addr, SAFE_CHARS_IPV4)) {
        return NetInfoStatus::BadInput;
    }

    if (auto service_opt{Lookup(addr, /*portDefault=*/port, /*fAllowLookup=*/false)}) {
        const auto ret{ValidateService(*service_opt)};
        if (ret == NetInfoStatus::Success) {
            m_addr = NetInfoEntry{*service_opt};
            ASSERT_IF_DEBUG(m_addr.GetAddrPort().has_value());
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

NetInfoList MnNetInfo::GetEntries() const
{
    if (!IsEmpty()) {
        ASSERT_IF_DEBUG(m_addr.GetAddrPort().has_value());
        return {m_addr};
    }
    // If MnNetInfo is empty, we probably don't expect any entries to show up, so
    // we return a blank set instead.
    return {};
}

CService MnNetInfo::GetPrimary() const
{
    if (const auto service_opt{m_addr.GetAddrPort()}) {
        return *service_opt;
    }
    return CService{};
}

NetInfoStatus MnNetInfo::Validate() const
{
    if (!m_addr.IsTriviallyValid()) {
        return NetInfoStatus::Malformed;
    }
    return ValidateService(GetPrimary());
}

UniValue MnNetInfo::ToJson() const
{
    if (IsEmpty()) {
        return UniValue{UniValue::VARR};
    }
    return ArrFromService(GetPrimary());
}

std::string MnNetInfo::ToString() const
{
    return IsEmpty() ? "MnNetInfo()" : strprintf("MnNetInfo([%s])", m_addr.ToString());
}

NetInfoStatus ExtNetInfo::ProcessCandidate(const NetInfoEntry& candidate)
{
    assert(candidate.IsTriviallyValid());

    if (m_data.size() >= MAX_ENTRIES_EXTNETINFO) {
        return NetInfoStatus::MaxLimit;
    }
    m_data.push_back(candidate);

    return NetInfoStatus::Success;
}

NetInfoStatus ExtNetInfo::ValidateService(const CService& service)
{
    if (!service.IsValid()) {
        return NetInfoStatus::BadAddress;
    }
    if (!service.IsIPv4() && !service.IsIPv6()) {
        return NetInfoStatus::BadType;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return NetInfoStatus::NotRoutable;
    }
    if (IsBadPort(service.GetPort()) || service.GetPort() == 0) {
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus ExtNetInfo::AddEntry(const std::string& input)
{
    // We don't allow assuming ports, so we set the default value to 0 so that if no port is specified
    // it uses a fallback value of 0, which will return a NetInfoStatus::BadPort
    std::string addr;
    uint16_t port{0};
    SplitHostPort(input, port, addr);
    // Contains invalid characters, unlikely to pass Lookup(), fast-fail
    if (!MatchCharsFilter(addr, SAFE_CHARS_IPV4_6)) {
        return NetInfoStatus::BadInput;
    }

    if (auto service_opt{Lookup(addr, /*portDefault=*/port, /*fAllowLookup=*/false)}) {
        const auto ret{ValidateService(*service_opt)};
        if (ret == NetInfoStatus::Success) {
            return ProcessCandidate(NetInfoEntry{*service_opt});
        }
        return ret; /* ValidateService() failed */
    }
    return NetInfoStatus::BadInput; /* Lookup() failed */
}

NetInfoList ExtNetInfo::GetEntries() const
{
    return m_data;
}

CService ExtNetInfo::GetPrimary() const
{
    if (m_data.size() >= 1) {
        if (const auto& service_opt{m_data[0].GetAddrPort()}) {
            return *service_opt;
        }
    }
    return CService{};
}

NetInfoStatus ExtNetInfo::Validate() const
{
    if (m_version == 0 || m_version > CURRENT_VERSION || m_data.empty()) {
        return NetInfoStatus::Malformed;
    }
    for (const auto& entry : m_data) {
        if (!entry.IsTriviallyValid()) {
            // Trivially invalid NetInfoEntry, no point checking against consensus rules
            return NetInfoStatus::Malformed;
        }
        if (const auto& service_opt{entry.GetAddrPort()}) {
            if (auto ret{ValidateService(*service_opt)}; ret != NetInfoStatus::Success) {
                // Stores CService underneath but doesn't pass validation rules
                return ret;
            }
        } else {
            // Doesn't store valid type underneath
            return NetInfoStatus::Malformed;
        }
    }
    return NetInfoStatus::Success;
}

UniValue ExtNetInfo::ToJson() const
{
    UniValue ret(UniValue::VARR);
    for (const auto& entry : m_data) {
        ret.push_back(entry.ToStringAddrPort());
    }
    return ret;
}

std::string ExtNetInfo::ToString() const
{
    return IsEmpty()
               ? "ExtNetInfo()"
               : strprintf("ExtNetInfo([%s])", Join(m_data, ", ", [](const auto& entry) { return entry.ToString(); }));
}
