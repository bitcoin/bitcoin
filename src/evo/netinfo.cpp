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

bool MatchCharsFilter(std::string_view input, std::string_view filter)
{
    return std::all_of(input.begin(), input.end(), [&filter](char c) { return filter.find(c) != std::string_view::npos; });
}
} // anonymous namespace

bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }

const CChainParams& MainParams()
{
    std::call_once(g_main_params_flag, [&]() { g_main_params = CreateChainParams(::gArgs, CBaseChainParams::MAIN); });
    return *Assert(g_main_params);
}

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

NetInfoStatus MnNetInfo::AddEntry(const NetInfoPurpose purpose, const std::string& input)
{
    if (purpose != NetInfoPurpose::CORE_P2P || !IsEmpty()) {
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
    UniValue ret{UniValue::VOBJ};
    if (!IsEmpty()) {
        ret.pushKV(PurposeToString(NetInfoPurpose::CORE_P2P).data(), ArrFromService(GetPrimary()));
    }
    return ret;
}

std::string MnNetInfo::ToString() const
{
    return IsEmpty() ? "MnNetInfo()"
                     : strprintf("MnNetInfo(NetInfo(purpose=%s, [%s]))", PurposeToString(NetInfoPurpose::CORE_P2P),
                                 m_addr.ToString());
}

bool ExtNetInfo::HasAddrPortDuplicates() const
{
    std::set<NetInfoEntry> known{};
    for (const auto& entry : m_all_entries) {
        if (auto [_, inserted] = known.insert(entry); !inserted) {
            return true;
        }
    }
    ASSERT_IF_DEBUG(known.size() == m_all_entries.size());
    return false;
}

bool ExtNetInfo::IsAddrPortDuplicate(const NetInfoEntry& candidate) const
{
    return std::any_of(m_all_entries.begin(), m_all_entries.end(),
                       [&candidate](const auto& entry) { return candidate == entry; });
}

bool ExtNetInfo::HasAddrDuplicates(const NetInfoList& entries) const
{
    std::unordered_set<std::string> known{};
    for (const auto& entry : entries) {
        if (auto [_, inserted] = known.insert(entry.ToStringAddr()); !inserted) {
            return true;
        }
    }
    ASSERT_IF_DEBUG(known.size() == entries.size());
    return false;
}

bool ExtNetInfo::IsAddrDuplicate(const NetInfoEntry& candidate, const NetInfoList& entries) const
{
    const std::string& candidate_str{candidate.ToStringAddr()};
    return std::any_of(entries.begin(), entries.end(),
                       [&candidate_str](const auto& entry) { return candidate_str == entry.ToStringAddr(); });
}

NetInfoStatus ExtNetInfo::ProcessCandidate(const NetInfoPurpose purpose, const NetInfoEntry& candidate)
{
    assert(candidate.IsTriviallyValid());

    if (IsAddrPortDuplicate(candidate)) {
        return NetInfoStatus::Duplicate;
    }
    if (auto it{m_data.find(purpose)}; it != m_data.end()) {
        // Existing entries list found, check limit
        auto& [_, entries] = *it;
        if (entries.size() >= MAX_ENTRIES_EXTNETINFO) {
            return NetInfoStatus::MaxLimit;
        }
        if (IsAddrDuplicate(candidate, entries)) {
            return NetInfoStatus::Duplicate;
        }
        entries.push_back(candidate);
    } else {
        // First entry for purpose code, create new entries list
        auto [_, status] = m_data.try_emplace(purpose, std::vector<NetInfoEntry>({candidate}));
        assert(status); // We did just check to see if our value already existed, try_emplace shouldn't fail
    }

    // Candidate succesfully added, update cache
    m_all_entries.push_back(candidate);
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

NetInfoStatus ExtNetInfo::AddEntry(const NetInfoPurpose purpose, const std::string& input)
{
    if (!IsValidPurpose(purpose)) {
        return NetInfoStatus::MaxLimit;
    }

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
            return ProcessCandidate(purpose, NetInfoEntry{*service_opt});
        }
        return ret; /* ValidateService() failed */
    }
    return NetInfoStatus::BadInput; /* Lookup() failed */
}

NetInfoList ExtNetInfo::GetEntries() const
{
    // ExtNetInfo is an append-only structure, we can avoid re-calculating
    // a list of all entries by maintaining a small cache.
    return m_all_entries;
}

CService ExtNetInfo::GetPrimary() const
{
    if (const auto& it{m_data.find(NetInfoPurpose::CORE_P2P)}; it != m_data.end()) {
        const auto& [_, entries] = *it;
        // If a purpose code is in the map, there should be at least one entry
        ASSERT_IF_DEBUG(!entries.empty());
        if (entries.size() >= 1) {
            if (const auto& service_opt{entries[0].GetAddrPort()}) {
                return *service_opt;
            }
        }
    }
    return CService{};
}

bool ExtNetInfo::HasEntries(NetInfoPurpose purpose) const
{
    if (!IsValidPurpose(purpose)) return false;
    const auto& it{m_data.find(purpose)};
    return it != m_data.end() && !it->second.empty();
}

NetInfoStatus ExtNetInfo::Validate() const
{
    if (m_version == 0 || m_version > CURRENT_VERSION || m_data.empty()) {
        return NetInfoStatus::Malformed;
    }
    if (HasAddrPortDuplicates()) {
        return NetInfoStatus::Duplicate;
    }
    for (const auto& [purpose, entries] : m_data) {
        if (!IsValidPurpose(purpose)) {
            return NetInfoStatus::Malformed;
        }
        if (entries.empty()) {
            // Purpose if present in map must have at least one entry
            return NetInfoStatus::Malformed;
        }
        if (HasAddrDuplicates(entries)) {
            return NetInfoStatus::Duplicate;
        }
        for (const auto& entry : entries) {
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
    }
    return NetInfoStatus::Success;
}

UniValue ExtNetInfo::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    for (const auto& [purpose, entries] : m_data) {
        UniValue arr(UniValue::VARR);
        for (const auto& entry : entries) {
            arr.push_back(entry.ToStringAddrPort());
        }
        ret.pushKV(PurposeToString(purpose).data(), arr);
    }
    return ret;
}

std::string ExtNetInfo::ToString() const
{
    return IsEmpty() ? "ExtNetInfo()" : strprintf("ExtNetInfo(%s)", [&]() -> std::string {
        std::string ret{};
        bool first{true};
        for (const auto& [purpose, entries] : m_data) {
            if (!first) { ret += ", "; } else { first = false; }
            ret += strprintf("NetInfo(purpose=%s, [%s])", PurposeToString(purpose), [&]() -> std::string {
                if (entries.empty()) {
                    return "invalid list";
                }
                return Join(entries, ", ", [](const auto& entry) { return entry.ToString(); });
            }());
        }
        return ret;
    }());
}
