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

template <typename T1>
void ProcessNetInfoCore(T1& ptx, const UniValue& input, const bool optional)
{
    CHECK_NONFATAL(ptx.netInfo);

    if (input.isStr()) {
        const std::string& entry = input.get_str();
        if (entry.empty()) {
            if (!optional) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Empty param for coreP2PAddrs not allowed");
            }
            return; // Nothing to do
        }
        if (auto entryRet = ptx.netInfo->AddEntry(entry); entryRet != NetInfoStatus::Success) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Error setting coreP2PAddrs[0] to '%s' (%s)", entry, NISToString(entryRet)));
        }
        return; // Parsing complete
    }

    if (input.isArray()) {
        const UniValue& entries = input.get_array();
        if (entries.empty()) {
            if (!optional) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Empty params for coreP2PAddrs not allowed");
            }
            return; // Nothing to do
        }
        for (size_t idx{0}; idx < entries.size(); idx++) {
            const UniValue& entry_uv{entries[idx]};
            if (!entry_uv.isStr()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   strprintf("Invalid param for coreP2PAddrs[%d], must be string", idx));
            }
            const std::string& entry = entry_uv.get_str();
            if (entry.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   strprintf("Invalid param for coreP2PAddrs[%d], cannot be empty string", idx));
            }
            if (auto entryRet = ptx.netInfo->AddEntry(entry); entryRet != NetInfoStatus::Success) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting coreP2PAddrs[%d] to '%s' (%s)", idx,
                                                                    entry, NISToString(entryRet)));
            }
        }
        return; // Parsing complete
    }

    // Invalid input
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, must be string or array");
}
template void ProcessNetInfoCore(CProRegTx& ptx, const UniValue& input, const bool optional);
template void ProcessNetInfoCore(CProUpServTx& ptx, const UniValue& input, const bool optional);

template <typename T1>
void ProcessNetInfoPlatform(T1& ptx, const UniValue& input_p2p, const UniValue& input_http)
{
    CHECK_NONFATAL(ptx.netInfo);

    auto process_field = [](uint16_t& target, const UniValue& input, const std::string& field_name) {
        if (!input.isNum() && !input.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for %s, must be number", field_name));
        }
        if (int32_t port{ParseInt32V(input, field_name)}; port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
            target = static_cast<uint16_t>(port);
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid port [1-65535]", field_name));
        }
    };
    process_field(ptx.platformP2PPort, input_p2p, "platformP2PPort");
    process_field(ptx.platformHTTPPort, input_http, "platformHTTPPort");
}
template void ProcessNetInfoPlatform(CProRegTx& ptx, const UniValue& input_p2p, const UniValue& input_http);
template void ProcessNetInfoPlatform(CProUpServTx& ptx, const UniValue& input_p2p, const UniValue& input_http);
