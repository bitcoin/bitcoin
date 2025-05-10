// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <netbase.h>
#include <span.h>
#include <util/check.h>
#include <util/system.h>

namespace {
static std::unique_ptr<const CChainParams> g_main_params{nullptr};
static std::once_flag g_main_params_flag;

bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }
const CChainParams& MainParams()
{
    // TODO: use real args here
    std::call_once(g_main_params_flag,
                   [&]() { g_main_params = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN); });
    return *Assert(g_main_params);
}
} // anonymous namespace

NetInfoStatus MnNetInfo::ValidateService(const CService& service)
{
    if (!service.IsValid()) {
        return NetInfoStatus::BadInput;
    }
    if (!service.IsIPv4()) {
        return NetInfoStatus::BadInput;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return NetInfoStatus::BadInput;
    }

    const auto default_port_main = MainParams().GetDefaultPort();
    if (IsNodeOnMainnet()) {
        if (service.GetPort() != default_port_main) {
            // Must use mainnet port on mainnet
            return NetInfoStatus::BadPort;
        }
    } else if (service.GetPort() == default_port_main) {
        // Using mainnet port prohibited outside of mainnet
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus MnNetInfo::AddEntry(const std::string& input)
{
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false);
        service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            m_addr = service.value();
            ASSERT_IF_DEBUG(m_addr != CService());
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

CServiceList MnNetInfo::GetEntries() const
{
    CServiceList ret;
    if (!IsEmpty()) {
        ASSERT_IF_DEBUG(m_addr != CService());
        ret.push_back(m_addr);
    }
    // If MnNetInfo is empty, we probably don't expect any entries to show up, so
    // we return a blank set instead.
    return ret;
}

std::string MnNetInfo::ToString() const
{
    // Extra padding to account for padding done by the calling function.
    return strprintf("MnNetInfo()\n"
                     "    CService(addr=%s, port=%u)\n",
                     m_addr.ToStringAddr(), m_addr.GetPort());
}
