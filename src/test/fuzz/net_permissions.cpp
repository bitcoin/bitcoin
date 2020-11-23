// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <optional.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(32);
    const NetPermissionFlags net_permission_flags = fuzzed_data_provider.ConsumeBool() ? fuzzed_data_provider.PickValueInArray<NetPermissionFlags>({
                                                                                             NetPermissionFlags::PF_NONE,
                                                                                             NetPermissionFlags::PF_BLOOMFILTER,
                                                                                             NetPermissionFlags::PF_RELAY,
                                                                                             NetPermissionFlags::PF_FORCERELAY,
                                                                                             NetPermissionFlags::PF_NOBAN,
                                                                                             NetPermissionFlags::PF_MEMPOOL,
                                                                                             NetPermissionFlags::PF_ISIMPLICIT,
                                                                                             NetPermissionFlags::PF_ALL,
                                                                                         }) :
                                                                                         static_cast<NetPermissionFlags>(fuzzed_data_provider.ConsumeIntegral<uint32_t>());

    NetWhitebindPermissions net_whitebind_permissions;
    std::string error_net_whitebind_permissions;
    if (NetWhitebindPermissions::TryParse(s, net_whitebind_permissions, error_net_whitebind_permissions)) {
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitebind_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitebind_permissions.m_flags, net_permission_flags));
        (void)NetPermissions::ClearFlag(net_whitebind_permissions.m_flags, net_permission_flags);
        (void)NetPermissions::ToStrings(net_whitebind_permissions.m_flags);
    }

    NetWhitelistPermissions net_whitelist_permissions;
    std::string error_net_whitelist_permissions;
    if (NetWhitelistPermissions::TryParse(s, net_whitelist_permissions, error_net_whitelist_permissions)) {
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
        (void)NetPermissions::AddFlag(net_whitelist_permissions.m_flags, net_permission_flags);
        assert(NetPermissions::HasFlag(net_whitelist_permissions.m_flags, net_permission_flags));
        (void)NetPermissions::ClearFlag(net_whitelist_permissions.m_flags, net_permission_flags);
        (void)NetPermissions::ToStrings(net_whitelist_permissions.m_flags);
    }
}
