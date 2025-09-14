// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/evo_util.h>

#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <util/check.h>

#include <univalue.h>

namespace {
template <typename ProTx>
void ParseInput(ProTx& ptx, std::string_view field_name, const std::string& input_str, NetInfoPurpose purpose,
                size_t idx, bool optional)
{
    if (input_str.empty()) {
        if (!optional) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Invalid param for %s[%zu], cannot be empty", field_name, idx));
        }
        return; // Nothing to do
    }
    if (auto ret = ptx.netInfo->AddEntry(purpose, input_str); ret != NetInfoStatus::Success) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Error setting %s[%zu] to '%s' (%s)", field_name, idx, input_str, NISToString(ret)));
    }
}
} // anonymous namespace

template <typename ProTx>
void ProcessNetInfoCore(ProTx& ptx, const UniValue& input, const bool optional)
{
    CHECK_NONFATAL(ptx.netInfo);

    if (input.isStr()) {
        ParseInput(ptx, /*field_name=*/"coreP2PAddrs", input.get_str(), NetInfoPurpose::CORE_P2P, /*idx=*/0, optional);
    } else if (input.isArray()) {
        const UniValue& entries = input.get_array();
        if (!optional && entries.empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, cannot be empty");
        }
        for (size_t idx{0}; idx < entries.size(); idx++) {
            const UniValue& entry_uv{entries[idx]};
            if (!entry_uv.isStr()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   strprintf("Invalid param for coreP2PAddrs[%zu], must be string", idx));
            }
            ParseInput(ptx, /*field_name=*/"coreP2PAddrs", entry_uv.get_str(), NetInfoPurpose::CORE_P2P, idx,
                       /*optional=*/false);
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, must be string or array");
    }
}
template void ProcessNetInfoCore(CProRegTx& ptx, const UniValue& input, const bool optional);
template void ProcessNetInfoCore(CProUpServTx& ptx, const UniValue& input, const bool optional);

template <typename ProTx>
void ProcessNetInfoPlatform(ProTx& ptx, const UniValue& input_p2p, const UniValue& input_http)
{
    CHECK_NONFATAL(ptx.netInfo);

    auto process_field = [](uint16_t& target, const UniValue& input, const std::string& field_name) {
        if (!input.isNum() && !input.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for %s, must be number", field_name));
        }
        if (int32_t port{ParseInt32V(input, field_name)}; port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
            target = static_cast<uint16_t>(port);
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Invalid param for %s, must be a valid port [1-65535]", field_name));
        }
    };
    process_field(ptx.platformP2PPort, input_p2p, "platformP2PPort");
    process_field(ptx.platformHTTPPort, input_http, "platformHTTPPort");
}
template void ProcessNetInfoPlatform(CProRegTx& ptx, const UniValue& input_p2p, const UniValue& input_http);
template void ProcessNetInfoPlatform(CProUpServTx& ptx, const UniValue& input_p2p, const UniValue& input_http);
