// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <netinet/in.h>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const CNetAddr net_addr = ConsumeNetAddr(fuzzed_data_provider);
    (void)net_addr.GetHash();
    (void)net_addr.GetNetClass();
    if (net_addr.GetNetwork() == Network::NET_IPV4) {
        assert(net_addr.IsIPv4());
    }
    if (net_addr.GetNetwork() == Network::NET_IPV6) {
        assert(net_addr.IsIPv6());
    }
    if (net_addr.GetNetwork() == Network::NET_ONION) {
        assert(net_addr.IsTor());
    }
    if (net_addr.GetNetwork() == Network::NET_INTERNAL) {
        assert(net_addr.IsInternal());
    }
    if (net_addr.GetNetwork() == Network::NET_UNROUTABLE) {
        assert(!net_addr.IsRoutable());
    }
    (void)net_addr.IsBindAny();
    if (net_addr.IsInternal()) {
        assert(net_addr.GetNetwork() == Network::NET_INTERNAL);
    }
    if (net_addr.IsIPv4()) {
        assert(net_addr.GetNetwork() == Network::NET_IPV4 || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    if (net_addr.IsIPv6()) {
        assert(net_addr.GetNetwork() == Network::NET_IPV6 || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    (void)net_addr.IsLocal();
    if (net_addr.IsRFC1918() || net_addr.IsRFC2544() || net_addr.IsRFC6598() || net_addr.IsRFC5737() || net_addr.IsRFC3927()) {
        assert(net_addr.IsIPv4());
    }
    (void)net_addr.IsRFC2544();
    if (net_addr.IsRFC3849() || net_addr.IsRFC3964() || net_addr.IsRFC4380() || net_addr.IsRFC4843() || net_addr.IsRFC7343() || net_addr.IsRFC4862() || net_addr.IsRFC6052() || net_addr.IsRFC6145()) {
        assert(net_addr.IsIPv6());
    }
    (void)net_addr.IsRFC3927();
    (void)net_addr.IsRFC3964();
    if (net_addr.IsRFC4193()) {
        assert(net_addr.GetNetwork() == Network::NET_ONION || net_addr.GetNetwork() == Network::NET_INTERNAL || net_addr.GetNetwork() == Network::NET_UNROUTABLE);
    }
    (void)net_addr.IsRFC4380();
    (void)net_addr.IsRFC4843();
    (void)net_addr.IsRFC4862();
    (void)net_addr.IsRFC5737();
    (void)net_addr.IsRFC6052();
    (void)net_addr.IsRFC6145();
    (void)net_addr.IsRFC6598();
    (void)net_addr.IsRFC7343();
    if (!net_addr.IsRoutable()) {
        assert(net_addr.GetNetwork() == Network::NET_UNROUTABLE || net_addr.GetNetwork() == Network::NET_INTERNAL);
    }
    if (net_addr.IsTor()) {
        assert(net_addr.GetNetwork() == Network::NET_ONION);
    }
    (void)net_addr.IsValid();
    (void)net_addr.ToString();
    (void)net_addr.ToStringIP();

    const CSubNet sub_net{net_addr, fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
    (void)sub_net.IsValid();
    (void)sub_net.ToString();

    const CService service{net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    (void)service.GetKey();
    (void)service.GetPort();
    (void)service.ToString();
    (void)service.ToStringIPPort();
    (void)service.ToStringPort();

    const CNetAddr other_net_addr = ConsumeNetAddr(fuzzed_data_provider);
    (void)net_addr.GetReachabilityFrom(&other_net_addr);
    (void)sub_net.Match(other_net_addr);

    const CService other_service{net_addr, fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
    assert((service == other_service) != (service != other_service));
    (void)(service < other_service);

    const CSubNet sub_net_copy_1{net_addr, other_net_addr};
    const CSubNet sub_net_copy_2{net_addr};

    CNetAddr mutable_net_addr;
    mutable_net_addr.SetIP(net_addr);
    assert(net_addr == mutable_net_addr);
}
