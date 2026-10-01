// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <compat/compat.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/common.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <algorithm>
#include <cerrno>
#include <concepts>
#include <cstring>
#include <memory>
#include <string>
#include <numeric>

#ifdef HAVE_SOCKADDR_UN
#include <sys/un.h>
#endif

#include <boost/test/unit_test.hpp>

using namespace std::literals;
using namespace util::hex_literals;

BOOST_FIXTURE_TEST_SUITE(netbase_tests, BasicTestingSetup)

static CNetAddr ResolveIP(const std::string& ip)
{
    return LookupHost(ip, false).value_or(CNetAddr{});
}

static CNetAddr CreateInternal(const std::string& host)
{
    CNetAddr addr;
    addr.SetInternal(host);
    return addr;
}

BOOST_AUTO_TEST_CASE(netbase_networks)
{
    BOOST_CHECK(ResolveIP("127.0.0.1").GetNetwork() == NET_UNROUTABLE);
    BOOST_CHECK(ResolveIP("::1").GetNetwork() == NET_UNROUTABLE);
    BOOST_CHECK(ResolveIP("8.8.8.8").GetNetwork() == NET_IPV4);
    BOOST_CHECK(ResolveIP("2001::8888").GetNetwork() == NET_IPV6);
    BOOST_CHECK(ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion").GetNetwork() == NET_ONION);
    BOOST_CHECK(CreateInternal("foo.com").GetNetwork() == NET_INTERNAL);
}

BOOST_AUTO_TEST_CASE(netbase_properties)
{

    BOOST_CHECK(ResolveIP("127.0.0.1").IsIPv4());
    BOOST_CHECK(ResolveIP("::FFFF:192.168.1.1").IsIPv4());
    BOOST_CHECK(ResolveIP("::1").IsIPv6());
    BOOST_CHECK(ResolveIP("10.0.0.1").IsRFC1918());
    BOOST_CHECK(ResolveIP("192.168.1.1").IsRFC1918());
    BOOST_CHECK(ResolveIP("172.31.255.255").IsRFC1918());
    BOOST_CHECK(ResolveIP("198.18.0.0").IsRFC2544());
    BOOST_CHECK(ResolveIP("198.19.255.255").IsRFC2544());
    BOOST_CHECK(ResolveIP("2001:0DB8::").IsRFC3849());
    BOOST_CHECK(ResolveIP("3FFF::").IsRFC9637());
    BOOST_CHECK(ResolveIP("3FFF:0FFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF").IsRFC9637());
    BOOST_CHECK(!ResolveIP("3FFF:1000::").IsRFC9637());
    BOOST_CHECK(ResolveIP("169.254.1.1").IsRFC3927());
    BOOST_CHECK(ResolveIP("2002::1").IsRFC3964());
    BOOST_CHECK(ResolveIP("FC00::").IsRFC4193());
    BOOST_CHECK(ResolveIP("2001::2").IsRFC4380());
    BOOST_CHECK(ResolveIP("2001:10::").IsRFC4843());
    BOOST_CHECK(ResolveIP("2001:20::").IsRFC7343());
    BOOST_CHECK(ResolveIP("FE80::").IsRFC4862());
    BOOST_CHECK(ResolveIP("64:FF9B::").IsRFC6052());
    BOOST_CHECK(ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion").IsTor());
    BOOST_CHECK(ResolveIP("127.0.0.1").IsLocal());
    BOOST_CHECK(ResolveIP("::1").IsLocal());
    BOOST_CHECK(ResolveIP("8.8.8.8").IsRoutable());
    BOOST_CHECK(ResolveIP("2001::1").IsRoutable());
    BOOST_CHECK(ResolveIP("127.0.0.1").IsValid());
    BOOST_CHECK(!ResolveIP("3FFF::").IsValid());
    BOOST_CHECK(CreateInternal("FD6B:88C0:8724:edb1:8e4:3588:e546:35ca").IsInternal());
    BOOST_CHECK(CreateInternal("bar.com").IsInternal());

}

bool static TestSplitHost(const std::string& test, const std::string& host, uint16_t port, bool validPort=true)
{
    std::string hostOut;
    uint16_t portOut{0};
    bool validPortOut = SplitHostPort(test, portOut, hostOut);
    return hostOut == host && portOut == port && validPortOut == validPort;
}

BOOST_AUTO_TEST_CASE(netbase_splithost)
{
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org", "www.bitcoincore.org", 0));
    BOOST_CHECK(TestSplitHost("[www.bitcoincore.org]", "www.bitcoincore.org", 0));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:80", "www.bitcoincore.org", 80));
    BOOST_CHECK(TestSplitHost("[www.bitcoincore.org]:80", "www.bitcoincore.org", 80));
    BOOST_CHECK(TestSplitHost("127.0.0.1", "127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("127.0.0.1:8333", "127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("[127.0.0.1]", "127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("[127.0.0.1]:8333", "127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("::ffff:127.0.0.1", "::ffff:127.0.0.1", 0));
    BOOST_CHECK(TestSplitHost("[::ffff:127.0.0.1]:8333", "::ffff:127.0.0.1", 8333));
    BOOST_CHECK(TestSplitHost("[::]:8333", "::", 8333));
    BOOST_CHECK(TestSplitHost("::8333", "::8333", 0));
    BOOST_CHECK(TestSplitHost(":8333", "", 8333));
    BOOST_CHECK(TestSplitHost("[]:8333", "", 8333));
    BOOST_CHECK(TestSplitHost("", "", 0));
    BOOST_CHECK(TestSplitHost(":65535", "", 65535));
    BOOST_CHECK(TestSplitHost(":65536", ":65536", 0, false));
    BOOST_CHECK(TestSplitHost(":-1", ":-1", 0, false));
    BOOST_CHECK(TestSplitHost("[]:70001", "[]:70001", 0, false));
    BOOST_CHECK(TestSplitHost("[]:-1", "[]:-1", 0, false));
    BOOST_CHECK(TestSplitHost("[]:-0", "[]:-0", 0, false));
    BOOST_CHECK(TestSplitHost("[]:0", "", 0, false));
    BOOST_CHECK(TestSplitHost("[]:1/2", "[]:1/2", 0, false));
    BOOST_CHECK(TestSplitHost("[]:1E2", "[]:1E2", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:65536", "127.0.0.1:65536", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:0", "127.0.0.1", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:", "127.0.0.1:", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:1/2", "127.0.0.1:1/2", 0, false));
    BOOST_CHECK(TestSplitHost("127.0.0.1:1E2", "127.0.0.1:1E2", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:65536", "www.bitcoincore.org:65536", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:0", "www.bitcoincore.org", 0, false));
    BOOST_CHECK(TestSplitHost("www.bitcoincore.org:", "www.bitcoincore.org:", 0, false));
}

bool static TestParse(std::string src, std::string canon)
{
    CService addr(LookupNumeric(src, 65535));
    return canon == addr.ToStringAddrPort();
}

BOOST_AUTO_TEST_CASE(netbase_lookupnumeric)
{
    BOOST_CHECK(TestParse("127.0.0.1", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse("127.0.0.1:8333", "127.0.0.1:8333"));
    BOOST_CHECK(TestParse("::ffff:127.0.0.1", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse("::", "[::]:65535"));
    BOOST_CHECK(TestParse("[::]:8333", "[::]:8333"));
    BOOST_CHECK(TestParse("[127.0.0.1]", "127.0.0.1:65535"));
    BOOST_CHECK(TestParse(":::", "[::]:0"));

    // verify that an internal address fails to resolve
    BOOST_CHECK(TestParse("[fd6b:88c0:8724:1:2:3:4:5]", "[::]:0"));
    // and that a one-off resolves correctly
    BOOST_CHECK(TestParse("[fd6c:88c0:8724:1:2:3:4:5]", "[fd6c:88c0:8724:1:2:3:4:5]:65535"));
}

BOOST_AUTO_TEST_CASE(embedded_test)
{
    CNetAddr addr1(ResolveIP("1.2.3.4"));
    CNetAddr addr2(ResolveIP("::FFFF:0102:0304"));
    BOOST_CHECK(addr2.IsIPv4());
    BOOST_CHECK_EQUAL(addr1.ToStringAddr(), addr2.ToStringAddr());
}

BOOST_AUTO_TEST_CASE(subnet_test)
{
    BOOST_CHECK(LookupSubNet("1.2.3.0/24") == LookupSubNet("1.2.3.0/255.255.255.0"));
    BOOST_CHECK(LookupSubNet("1.2.3.0/24") != LookupSubNet("1.2.4.0/255.255.255.0"));
    BOOST_CHECK(LookupSubNet("1.2.3.0/24").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("1.2.2.0/24").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(LookupSubNet("1.2.3.4").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(LookupSubNet("1.2.3.4/32").Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("1.2.3.4").Match(ResolveIP("5.6.7.8")));
    BOOST_CHECK(!LookupSubNet("1.2.3.4/32").Match(ResolveIP("5.6.7.8")));
    BOOST_CHECK(LookupSubNet("::ffff:127.0.0.1").Match(ResolveIP("127.0.0.1")));
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:8")));
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:9")));
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:0/112").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    BOOST_CHECK(LookupSubNet("192.168.0.1/24").Match(ResolveIP("192.168.0.2")));
    BOOST_CHECK(LookupSubNet("192.168.0.20/29").Match(ResolveIP("192.168.0.18")));
    BOOST_CHECK(LookupSubNet("1.2.2.1/24").Match(ResolveIP("1.2.2.4")));
    BOOST_CHECK(LookupSubNet("1.2.2.110/31").Match(ResolveIP("1.2.2.111")));
    BOOST_CHECK(LookupSubNet("1.2.2.20/26").Match(ResolveIP("1.2.2.63")));
    // All-Matching IPv6 Matches arbitrary IPv6
    BOOST_CHECK(LookupSubNet("::/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    // But not `::` or `0.0.0.0` because they are considered invalid addresses
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("::")));
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("0.0.0.0")));
    // Addresses from one network (IPv4) don't belong to subnets of another network (IPv6)
    BOOST_CHECK(!LookupSubNet("::/0").Match(ResolveIP("1.2.3.4")));
    // All-Matching IPv4 does not Match IPv6
    BOOST_CHECK(!LookupSubNet("0.0.0.0/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    // Invalid subnets Match nothing (not even invalid addresses)
    BOOST_CHECK(!CSubNet().Match(ResolveIP("1.2.3.4")));
    BOOST_CHECK(!LookupSubNet("").Match(ResolveIP("4.5.6.7")));
    BOOST_CHECK(!LookupSubNet("bloop").Match(ResolveIP("0.0.0.0")));
    BOOST_CHECK(!LookupSubNet("bloop").Match(ResolveIP("hab")));
    // Check valid/invalid
    BOOST_CHECK(LookupSubNet("1.2.3.0/0").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/-1").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/+24").IsValid());
    BOOST_CHECK(LookupSubNet("1.2.3.0/32").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/33").IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/300").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/0").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/33").IsValid());
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8/-1").IsValid());
    BOOST_CHECK(LookupSubNet("1:2:3:4:5:6:7:8/128").IsValid());
    BOOST_CHECK(!LookupSubNet("1:2:3:4:5:6:7:8/129").IsValid());
    BOOST_CHECK(!LookupSubNet("fuzzy").IsValid());

    //CNetAddr constructor test
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).IsValid());
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.1")));
    BOOST_CHECK(!CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.2")));
    BOOST_CHECK(CSubNet(ResolveIP("127.0.0.1")).ToString() == "127.0.0.1/32");

    CSubNet subnet = CSubNet(ResolveIP("1.2.3.4"), 32);
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 8);
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 0);
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.255.255.255"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.0.0.0"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("0.0.0.0"));
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).IsValid());
    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:8")));
    BOOST_CHECK(!CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:9")));
    BOOST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).ToString() == "1:2:3:4:5:6:7:8/128");
    // IPv4 address with IPv6 netmask or the other way around.
    BOOST_CHECK(!CSubNet(ResolveIP("1.1.1.1"), ResolveIP("ffff::")).IsValid());
    BOOST_CHECK(!CSubNet(ResolveIP("::1"), ResolveIP("255.0.0.0")).IsValid());

    // Create Non-IP subnets.

    const CNetAddr tor_addr{
        ResolveIP("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion")};

    subnet = CSubNet(tor_addr);
    BOOST_CHECK(subnet.IsValid());
    BOOST_CHECK_EQUAL(subnet.ToString(), tor_addr.ToStringAddr());
    BOOST_CHECK(subnet.Match(tor_addr));
    BOOST_CHECK(
        !subnet.Match(ResolveIP("kpgvmscirrdqpekbqjsvw5teanhatztpp2gl6eee4zkowvwfxwenqaid.onion")));
    BOOST_CHECK(!subnet.Match(ResolveIP("1.2.3.4")));

    BOOST_CHECK(!CSubNet(tor_addr, 200).IsValid());
    BOOST_CHECK(!CSubNet(tor_addr, ResolveIP("255.0.0.0")).IsValid());

    subnet = LookupSubNet("1.2.3.4/255.255.255.255");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = LookupSubNet("1.2.3.4/255.255.255.254");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/31");
    subnet = LookupSubNet("1.2.3.4/255.255.255.252");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/30");
    subnet = LookupSubNet("1.2.3.4/255.255.255.248");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/29");
    subnet = LookupSubNet("1.2.3.4/255.255.255.240");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/28");
    subnet = LookupSubNet("1.2.3.4/255.255.255.224");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/27");
    subnet = LookupSubNet("1.2.3.4/255.255.255.192");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/26");
    subnet = LookupSubNet("1.2.3.4/255.255.255.128");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/25");
    subnet = LookupSubNet("1.2.3.4/255.255.255.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/24");
    subnet = LookupSubNet("1.2.3.4/255.255.254.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.2.0/23");
    subnet = LookupSubNet("1.2.3.4/255.255.252.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/22");
    subnet = LookupSubNet("1.2.3.4/255.255.248.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/21");
    subnet = LookupSubNet("1.2.3.4/255.255.240.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/20");
    subnet = LookupSubNet("1.2.3.4/255.255.224.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/19");
    subnet = LookupSubNet("1.2.3.4/255.255.192.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/18");
    subnet = LookupSubNet("1.2.3.4/255.255.128.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/17");
    subnet = LookupSubNet("1.2.3.4/255.255.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/16");
    subnet = LookupSubNet("1.2.3.4/255.254.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/15");
    subnet = LookupSubNet("1.2.3.4/255.252.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/14");
    subnet = LookupSubNet("1.2.3.4/255.248.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/13");
    subnet = LookupSubNet("1.2.3.4/255.240.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/12");
    subnet = LookupSubNet("1.2.3.4/255.224.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/11");
    subnet = LookupSubNet("1.2.3.4/255.192.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/10");
    subnet = LookupSubNet("1.2.3.4/255.128.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/9");
    subnet = LookupSubNet("1.2.3.4/255.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = LookupSubNet("1.2.3.4/254.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/7");
    subnet = LookupSubNet("1.2.3.4/252.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/6");
    subnet = LookupSubNet("1.2.3.4/248.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/5");
    subnet = LookupSubNet("1.2.3.4/240.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/4");
    subnet = LookupSubNet("1.2.3.4/224.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/3");
    subnet = LookupSubNet("1.2.3.4/192.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/2");
    subnet = LookupSubNet("1.2.3.4/128.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/1");
    subnet = LookupSubNet("1.2.3.4/0.0.0.0");
    BOOST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1:2:3:4:5:6:7:8/128");
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:0000:0000:0000:0000:0000:0000:0000");
    BOOST_CHECK_EQUAL(subnet.ToString(), "1::/16");
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/0000:0000:0000:0000:0000:0000:0000:0000");
    BOOST_CHECK_EQUAL(subnet.ToString(), "::/0");
    // Invalid netmasks (with 1-bits after 0-bits)
    subnet = LookupSubNet("1.2.3.4/255.255.232.0");
    BOOST_CHECK(!subnet.IsValid());
    subnet = LookupSubNet("1.2.3.4/255.0.255.255");
    BOOST_CHECK(!subnet.IsValid());
    subnet = LookupSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");
    BOOST_CHECK(!subnet.IsValid());
}

BOOST_AUTO_TEST_CASE(netbase_getgroup)
{
    auto netgroupman{NetGroupManager::NoAsmap()}; // use /16
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("127.0.0.1")) == std::vector<unsigned char>({0})); // Local -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("257.0.0.1")) == std::vector<unsigned char>({0})); // !Valid -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("10.0.0.1")) == std::vector<unsigned char>({0})); // RFC1918 -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("169.254.1.1")) == std::vector<unsigned char>({0})); // RFC3927 -> !Routable()
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("1.2.3.4")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // IPv4
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("::FFFF:0:102:304")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC6145
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("64:FF9B::102:304")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC6052
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2002:102:304:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC3964
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:0:9999:9999:9999:9999:FEFD:FCFB")) == std::vector<unsigned char>({(unsigned char)NET_IPV4, 1, 2})); // RFC4380
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:470:abcd:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV6, 32, 1, 4, 112, 175})); //he.net
    BOOST_CHECK(netgroupman.GetGroup(ResolveIP("2001:2001:9999:9999:9999:9999:9999:9999")) == std::vector<unsigned char>({(unsigned char)NET_IPV6, 32, 1, 32, 1})); //IPv6

    // baz.net sha256 hash: 12929400eb4607c4ac075f087167e75286b179c693eb059a01774b864e8fe505
    std::vector<unsigned char> internal_group = {NET_INTERNAL, 0x12, 0x92, 0x94, 0x00, 0xeb, 0x46, 0x07, 0xc4, 0xac, 0x07};
    BOOST_CHECK(netgroupman.GetGroup(CreateInternal("baz.net")) == internal_group);
}

BOOST_AUTO_TEST_CASE(netbase_parsenetwork)
{
    BOOST_CHECK_EQUAL(ParseNetwork("ipv4"), NET_IPV4);
    BOOST_CHECK_EQUAL(ParseNetwork("ipv6"), NET_IPV6);
    BOOST_CHECK_EQUAL(ParseNetwork("onion"), NET_ONION);
    BOOST_CHECK_EQUAL(ParseNetwork("cjdns"), NET_CJDNS);

    BOOST_CHECK_EQUAL(ParseNetwork("IPv4"), NET_IPV4);
    BOOST_CHECK_EQUAL(ParseNetwork("IPv6"), NET_IPV6);
    BOOST_CHECK_EQUAL(ParseNetwork("ONION"), NET_ONION);
    BOOST_CHECK_EQUAL(ParseNetwork("CJDNS"), NET_CJDNS);

    // "tor" as a network specification was deprecated in 60dc8e4208 in favor of
    // "onion" and later removed.
    BOOST_CHECK_EQUAL(ParseNetwork("tor"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("TOR"), NET_UNROUTABLE);

    BOOST_CHECK_EQUAL(ParseNetwork(":)"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("oniÖn"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork("\xfe\xff"), NET_UNROUTABLE);
    BOOST_CHECK_EQUAL(ParseNetwork(""), NET_UNROUTABLE);
}

BOOST_AUTO_TEST_CASE(netpermissions_test)
{
    bilingual_str error;
    NetWhitebindPermissions whitebindPermissions;
    NetWhitelistPermissions whitelistPermissions;
    ConnectionDirection connection_direction;

    // Detect invalid white bind
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Cannot resolve -whitebind address") != std::string::npos);
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("127.0.0.1", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Need to specify a port with -whitebind") != std::string::npos);
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("", whitebindPermissions, error));

    // If no permission flags, assume backward compatibility
    BOOST_CHECK(NetWhitebindPermissions::TryParse("1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.empty());
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));
    NetPermissions::ClearFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(!NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);
    NetPermissions::AddFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit);
    BOOST_CHECK(NetPermissions::HasFlag(whitebindPermissions.m_flags, NetPermissionFlags::Implicit));

    // Can set one permission
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);

    NetWhitebindPermissions noban, noban_download, download_noban, download;

    // "noban" implies "download"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("noban@1.2.3.4:32", noban, error));
    BOOST_CHECK_EQUAL(noban.m_flags, NetPermissionFlags::NoBan);
    BOOST_CHECK(NetPermissions::HasFlag(noban.m_flags, NetPermissionFlags::Download));
    BOOST_CHECK(NetPermissions::HasFlag(noban.m_flags, NetPermissionFlags::NoBan));

    // "noban,download" is equivalent to "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("noban,download@1.2.3.4:32", noban_download, error));
    BOOST_CHECK_EQUAL(noban_download.m_flags, noban.m_flags);

    // "download,noban" is equivalent to "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("download,noban@1.2.3.4:32", download_noban, error));
    BOOST_CHECK_EQUAL(download_noban.m_flags, noban.m_flags);

    // "download" excludes (does not imply) "noban"
    BOOST_REQUIRE(NetWhitebindPermissions::TryParse("download@1.2.3.4:32", download, error));
    BOOST_CHECK_EQUAL(download.m_flags, NetPermissionFlags::Download);
    BOOST_CHECK(NetPermissions::HasFlag(download.m_flags, NetPermissionFlags::Download));
    BOOST_CHECK(!NetPermissions::HasFlag(download.m_flags, NetPermissionFlags::NoBan));

    // Happy path, can parse flags
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,forcerelay@1.2.3.4:32", whitebindPermissions, error));
    // forcerelay should also activate the relay permission
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::Relay);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan);
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,forcerelay,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(NetWhitebindPermissions::TryParse("all@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::All);

    // Allow dups
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,noban,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan | NetPermissionFlags::Download); // "noban" implies "download"

    // Allow empty
    BOOST_CHECK(NetWhitebindPermissions::TryParse("bloom,relay,,noban@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::Relay | NetPermissionFlags::NoBan);
    BOOST_CHECK(NetWhitebindPermissions::TryParse(",@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);
    BOOST_CHECK(NetWhitebindPermissions::TryParse(",,@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK_EQUAL(whitebindPermissions.m_flags, NetPermissionFlags::None);

    BOOST_CHECK(!NetWhitebindPermissions::TryParse("out,forcerelay@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("whitebind may only be used for incoming connections (\"out\" was passed)") != std::string::npos);

    // Detect invalid flag
    BOOST_CHECK(!NetWhitebindPermissions::TryParse("bloom,forcerelay,oopsie@1.2.3.4:32", whitebindPermissions, error));
    BOOST_CHECK(error.original.find("Invalid P2P permission") != std::string::npos);

    // Check netmask error
    BOOST_CHECK(!NetWhitelistPermissions::TryParse("bloom,forcerelay,noban@1.2.3.4:32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK(error.original.find("Invalid netmask specified in -whitelist") != std::string::npos);

    // Happy path for whitelist parsing
    BOOST_CHECK(NetWhitelistPermissions::TryParse("noban@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(whitelistPermissions.m_flags, NetPermissionFlags::NoBan);
    BOOST_CHECK(NetPermissions::HasFlag(whitelistPermissions.m_flags, NetPermissionFlags::NoBan));

    BOOST_CHECK(NetWhitelistPermissions::TryParse("bloom,forcerelay,noban,relay@1.2.3.4/32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(whitelistPermissions.m_flags, NetPermissionFlags::BloomFilter | NetPermissionFlags::ForceRelay | NetPermissionFlags::NoBan | NetPermissionFlags::Relay);
    BOOST_CHECK(error.empty());
    BOOST_CHECK_EQUAL(whitelistPermissions.m_subnet.ToString(), "1.2.3.4/32");
    BOOST_CHECK(NetWhitelistPermissions::TryParse("bloom,forcerelay,noban,relay,mempool@1.2.3.4/32", whitelistPermissions, connection_direction, error));
    BOOST_CHECK(NetWhitelistPermissions::TryParse("in,relay@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::In);
    BOOST_CHECK(NetWhitelistPermissions::TryParse("out,bloom@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::Out);
    BOOST_CHECK(NetWhitelistPermissions::TryParse("in,out,bloom@1.2.3.4", whitelistPermissions, connection_direction, error));
    BOOST_CHECK_EQUAL(connection_direction, ConnectionDirection::Both);

    const auto strings = NetPermissions::ToStrings(NetPermissionFlags::All);
    BOOST_CHECK_EQUAL(strings.size(), 7U);
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "bloomfilter") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "forcerelay") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "relay") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "noban") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "mempool") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "download") != strings.end());
    BOOST_CHECK(std::find(strings.begin(), strings.end(), "addr") != strings.end());
}

BOOST_AUTO_TEST_CASE(netbase_dont_resolve_strings_with_embedded_nul_characters)
{
    BOOST_CHECK(LookupHost("127.0.0.1"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0example.com"s, false).has_value());
    BOOST_CHECK(!LookupHost("127.0.0.1\0example.com\0"s, false).has_value());

    BOOST_CHECK(LookupSubNet("1.2.3.0/24"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0example.com"s).IsValid());
    BOOST_CHECK(!LookupSubNet("1.2.3.0/24\0example.com\0"s).IsValid());
    BOOST_CHECK(LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0example.com"s).IsValid());
    BOOST_CHECK(!LookupSubNet("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion\0example.com\0"s).IsValid());
}

// Since CNetAddr (un)ser is tested separately in net_tests.cpp here we only
// try a few edge cases for port, service flags and time.

static const std::vector<CAddress> fixture_addresses({
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0 /* port */),
        NODE_NONE,
        NodeSeconds{0x4966bc61s}, /* Fri Jan  9 02:54:25 UTC 2009 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0x00f1 /* port */),
        NODE_NETWORK,
        NodeSeconds{0x83766279s}, /* Tue Nov 22 11:22:33 UTC 2039 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(COMPAT_IN6ADDR_LOOPBACK_INIT)), 0xf1f2 /* port */),
        static_cast<ServiceFlags>(NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED),
        NodeSeconds{0xffffffffs}, /* Sun Feb  7 06:28:15 UTC 2106 */
    },
});

// fixture_addresses should equal to this when serialized in V1 format.
// When this is unserialized from V1 format it should equal to fixture_addresses.
static constexpr const char* stream_addrv1_hex =
    "03" // number of entries

    "61bc6649"                         // time, Fri Jan  9 02:54:25 UTC 2009
    "0000000000000000"                 // service flags, NODE_NONE
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv4 embedded in IPv6)
    "0000"                             // port

    "79627683"                         // time, Tue Nov 22 11:22:33 UTC 2039
    "0100000000000000"                 // service flags, NODE_NETWORK
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv6)
    "00f1"                             // port

    "ffffffff"                         // time, Sun Feb  7 06:28:15 UTC 2106
    "4804000000000000"                 // service flags, NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED
    "00000000000000000000000000000001" // address, fixed 16 bytes (IPv6)
    "f1f2";                            // port

// fixture_addresses should equal to this when serialized in V2 format.
// When this is unserialized from V2 format it should equal to fixture_addresses.
static constexpr const char* stream_addrv2_hex =
    "03" // number of entries

    "61bc6649"                         // time, Fri Jan  9 02:54:25 UTC 2009
    "00"                               // service flags, COMPACTSIZE(NODE_NONE)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "0000"                             // port

    "79627683"                         // time, Tue Nov 22 11:22:33 UTC 2039
    "01"                               // service flags, COMPACTSIZE(NODE_NETWORK)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "00f1"                             // port

    "ffffffff"                         // time, Sun Feb  7 06:28:15 UTC 2106
    "fd4804"                           // service flags, COMPACTSIZE(NODE_WITNESS | NODE_COMPACT_FILTERS | NODE_NETWORK_LIMITED)
    "02"                               // network id, IPv6
    "10"                               // address length, COMPACTSIZE(16)
    "00000000000000000000000000000001" // address
    "f1f2";                            // port

BOOST_AUTO_TEST_CASE(caddress_serialize_v1)
{
    DataStream s{};

    s << CAddress::V1_NETWORK(fixture_addresses);
    BOOST_CHECK_EQUAL(HexStr(s), stream_addrv1_hex);
}

BOOST_AUTO_TEST_CASE(caddress_unserialize_v1)
{
    std::vector<CAddress> addresses_unserialized;

    SpanReader{ParseHex(stream_addrv1_hex)} >> CAddress::V1_NETWORK(addresses_unserialized);
    BOOST_CHECK(fixture_addresses == addresses_unserialized);
}

BOOST_AUTO_TEST_CASE(caddress_serialize_v2)
{
    DataStream s{};

    s << CAddress::V2_NETWORK(fixture_addresses);
    BOOST_CHECK_EQUAL(HexStr(s), stream_addrv2_hex);
}

BOOST_AUTO_TEST_CASE(caddress_unserialize_v2)
{
    std::vector<CAddress> addresses_unserialized;

    SpanReader{ParseHex(stream_addrv2_hex)} >> CAddress::V2_NETWORK(addresses_unserialized);
    BOOST_CHECK(fixture_addresses == addresses_unserialized);
}

BOOST_AUTO_TEST_CASE(isbadport)
{
    BOOST_CHECK(IsBadPort(1));
    BOOST_CHECK(IsBadPort(22));
    BOOST_CHECK(IsBadPort(6000));

    BOOST_CHECK(!IsBadPort(80));
    BOOST_CHECK(!IsBadPort(443));
    BOOST_CHECK(!IsBadPort(8333));

    // Check all possible ports and ensure we only flag the expected amount as bad
    std::list<int> ports(std::numeric_limits<uint16_t>::max());
    std::iota(ports.begin(), ports.end(), 1);
    BOOST_CHECK_EQUAL(std::ranges::count_if(ports, IsBadPort), 85);
}

/** The socket address API shared by CService, UnixSocketAddr and SocketAddr */
template <typename T>
concept SockAddrLike = requires(T addr, const T caddr, sockaddr* paddr, const sockaddr* cpaddr, socklen_t* paddrlen, socklen_t addrlen) {
    { caddr.IsValid() } -> std::same_as<bool>;
    { caddr.IsIPv4() } -> std::same_as<bool>;
    { caddr.IsIPv6() } -> std::same_as<bool>;
    { caddr.GetSAFamily() } -> std::same_as<sa_family_t>;
    { caddr.ToStringAddrPort() } -> std::same_as<std::string>;
    { caddr.GetSockAddr(paddr, paddrlen) } -> std::same_as<bool>;
    { addr.SetSockAddr(cpaddr, addrlen) } -> std::same_as<bool>;
};
static_assert(SockAddrLike<CService>);
static_assert(SockAddrLike<UnixSocketAddr>);
static_assert(SockAddrLike<SocketAddr>);

template <SockAddrLike T>
static void CheckSockAddrRoundTrip(const T& addr, sa_family_t family, socklen_t expected_len)
{
    BOOST_REQUIRE(addr.IsValid());
    BOOST_CHECK_EQUAL(addr.GetSAFamily(), family);

    sockaddr_storage storage;
    auto* paddr{reinterpret_cast<sockaddr*>(&storage)};

    // Buffer too small
    socklen_t len{expected_len - 1};
    BOOST_CHECK(!addr.GetSockAddr(paddr, &len));

    len = sizeof(storage);
    BOOST_REQUIRE(addr.GetSockAddr(paddr, &len));
    BOOST_CHECK_EQUAL(len, expected_len);
    BOOST_CHECK_EQUAL(paddr->sa_family, family);

    T addr2;
    BOOST_CHECK(!addr2.IsValid());
    BOOST_REQUIRE(addr2.SetSockAddr(paddr, len));
    BOOST_CHECK(addr2.IsValid());
    BOOST_CHECK_EQUAL(addr2.GetSAFamily(), family);
    BOOST_CHECK_EQUAL(addr2.ToStringAddrPort(), addr.ToStringAddrPort());
}

BOOST_AUTO_TEST_CASE(sockaddr_api)
{
    const CService ipv4{LookupNumeric("142.250.217.142", 8333)};
    CheckSockAddrRoundTrip(ipv4, AF_INET, sizeof(sockaddr_in));
    BOOST_CHECK(ipv4.IsIPv4());
    BOOST_CHECK(!ipv4.IsIPv6());
    CheckSockAddrRoundTrip(SocketAddr{ipv4}, AF_INET, sizeof(sockaddr_in));
    BOOST_CHECK(SocketAddr{ipv4}.IsIPv4());
    BOOST_CHECK(!SocketAddr{ipv4}.IsIPv6());

    const CService ipv6{LookupNumeric("2607:f8b0:4006:80f::200e", 8333)};
    CheckSockAddrRoundTrip(ipv6, AF_INET6, sizeof(sockaddr_in6));
    BOOST_CHECK(!ipv6.IsIPv4());
    BOOST_CHECK(ipv6.IsIPv6());
    CheckSockAddrRoundTrip(SocketAddr{ipv6}, AF_INET6, sizeof(sockaddr_in6));
    BOOST_CHECK(!SocketAddr{ipv6}.IsIPv4());
    BOOST_CHECK(SocketAddr{ipv6}.IsIPv6());

#ifdef HAVE_SOCKADDR_UN
    const UnixSocketAddr unix_addr{"unix:/tmp/bitcoin.sock"};
    CheckSockAddrRoundTrip(unix_addr, AF_UNIX, sizeof(sockaddr_un));
    BOOST_CHECK(!unix_addr.IsIPv4());
    BOOST_CHECK(!unix_addr.IsIPv6());
    // Round trip starts from a default SocketAddr, which holds a CService,
    // so this also checks that SetSockAddr() switches the variant alternative.
    CheckSockAddrRoundTrip(SocketAddr{unix_addr}, AF_UNIX, sizeof(sockaddr_un));
    BOOST_CHECK(!SocketAddr{unix_addr}.IsIPv4());
    BOOST_CHECK(!SocketAddr{unix_addr}.IsIPv6());

    // Each type rejects the other's sockaddr
    sockaddr_storage storage;
    auto* paddr{reinterpret_cast<sockaddr*>(&storage)};
    socklen_t len{sizeof(storage)};
    BOOST_REQUIRE(unix_addr.GetSockAddr(paddr, &len));
    CService service;
    BOOST_CHECK(!service.SetSockAddr(paddr, len));

    for (const CService& ip : {ipv4, ipv6}) {
        len = sizeof(storage);
        BOOST_REQUIRE(ip.GetSockAddr(paddr, &len));
        UnixSocketAddr unix_addr2;
        BOOST_CHECK(!unix_addr2.SetSockAddr(paddr, len));
    }
#endif
}

#ifdef HAVE_SOCKADDR_UN
BOOST_AUTO_TEST_CASE(unix_socket_addr)
{
    constexpr socklen_t offset{offsetof(sockaddr_un, sun_path)};
    constexpr size_t max_path_len{sizeof(sockaddr_un::sun_path) - 1};
    const std::string path{"/tmp/bitcoin.sock"};

    // IsUnixSocketPath boundaries
    BOOST_CHECK(IsUnixSocketPath(ADDR_PREFIX_UNIX + path));
    BOOST_CHECK(!IsUnixSocketPath(path));
    BOOST_CHECK(IsUnixSocketPath(ADDR_PREFIX_UNIX + std::string(max_path_len, 'a')));
    BOOST_CHECK(!IsUnixSocketPath(ADDR_PREFIX_UNIX + std::string(max_path_len + 1, 'a')));

    // Accessors
    const UnixSocketAddr addr{ADDR_PREFIX_UNIX + path};
    BOOST_CHECK(addr.IsValid());
    BOOST_CHECK_EQUAL(addr.GetDestString(), path);
    BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), ADDR_PREFIX_UNIX + path);

    sockaddr_un sa_un;
    auto* paddr{reinterpret_cast<sockaddr*>(&sa_un)};
    socklen_t len{sizeof(sa_un)};

    // Default-constructed is invalid and can't be converted
    const UnixSocketAddr empty;
    BOOST_CHECK(!empty.IsValid());
    BOOST_CHECK(!empty.GetSockAddr(paddr, &len));
    BOOST_CHECK_EQUAL(empty.GetDestString(), "");
    BOOST_CHECK_EQUAL(empty.ToStringAddrPort(), "");

    // Constructed from a string without the prefix, or too long, is invalid
    for (const std::string& bad : {path, ADDR_PREFIX_UNIX + std::string(max_path_len + 1, 'a'), std::string{}}) {
        const UnixSocketAddr invalid{bad};
        BOOST_CHECK(!invalid.IsValid());
        BOOST_CHECK(!invalid.GetSockAddr(paddr, &len));
        BOOST_CHECK_EQUAL(invalid.GetDestString(), "");
        // ToStringAddrPort() still echoes the input to aid error messages
        BOOST_CHECK_EQUAL(invalid.ToStringAddrPort(), bad);
    }

    // GetSockAddr zero-pads sun_path after the path
    std::memset(&sa_un, 0xff, sizeof(sa_un));
    BOOST_REQUIRE(addr.GetSockAddr(paddr, &len));
    BOOST_CHECK_EQUAL(len, sizeof(sa_un));
    BOOST_CHECK_EQUAL(sa_un.sun_family, AF_UNIX);
    BOOST_CHECK_EQUAL(std::string(sa_un.sun_path), path);
    BOOST_CHECK(std::all_of(sa_un.sun_path + path.size(), std::end(sa_un.sun_path), [](char c) { return c == '\0'; }));

    // Longest valid path round trips and is still NUL-terminated
    const UnixSocketAddr longest{ADDR_PREFIX_UNIX + std::string(max_path_len, 'a')};
    CheckSockAddrRoundTrip(longest, AF_UNIX, sizeof(sockaddr_un));
    len = sizeof(sa_un);
    BOOST_REQUIRE(longest.GetSockAddr(paddr, &len));
    BOOST_CHECK_EQUAL(sa_un.sun_path[max_path_len], '\0');

    // SetSockAddr from hand-built structs, mimicking what the OS may return
    // from accept(), getpeername() or getsockname()
    const auto make_sa_un{[](std::string_view p) {
        sockaddr_un s{};
        s.sun_family = AF_UNIX;
        std::memcpy(s.sun_path, p.data(), std::min(p.size(), sizeof(s.sun_path)));
        return s;
    }};
    // Returns the resulting ToStringAddrPort(), or "" on failure
    const auto set{[](const sockaddr_un& s, socklen_t addrlen) -> std::string {
        UnixSocketAddr a;
        if (!a.SetSockAddr(reinterpret_cast<const sockaddr*>(&s), addrlen)) return "";
        BOOST_CHECK(a.IsValid());
        return a.ToStringAddrPort();
    }};

    sa_un = make_sa_un(path);
    // Invalid lengths
    BOOST_CHECK_EQUAL(set(sa_un, offset - 1), "");
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un) + 1), "");
    // Length includes the terminator (Linux)
    BOOST_CHECK_EQUAL(set(sa_un, offset + path.size() + 1), ADDR_PREFIX_UNIX + path);
    // Length excludes the terminator
    BOOST_CHECK_EQUAL(set(sa_un, offset + path.size()), ADDR_PREFIX_UNIX + path);
    // Full struct with NUL padding, as produced by GetSockAddr()
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un)), ADDR_PREFIX_UNIX + path);
    // Length shorter than the path truncates it
    BOOST_CHECK_EQUAL(set(sa_un, offset + 4), ADDR_PREFIX_UNIX + "/tmp");

    // Unnamed socket: no path bytes at all, or only NUL bytes
    BOOST_CHECK_EQUAL(set(sa_un, offset), ADDR_PREFIX_UNIX + "unix");
    sa_un = make_sa_un("");
    BOOST_CHECK_EQUAL(set(sa_un, offset + 1), ADDR_PREFIX_UNIX + "unix");
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un)), ADDR_PREFIX_UNIX + "unix");

    // sun_path filled entirely with no terminator can't be represented, fail gracefully
    sa_un = make_sa_un(std::string(max_path_len + 1, 'a'));
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un)), "");
    sa_un = make_sa_un(std::string(max_path_len, 'a'));
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un)), ADDR_PREFIX_UNIX + std::string(max_path_len, 'a'));

    // Wrong address family
    sa_un = make_sa_un(path);
    sa_un.sun_family = AF_INET;
    BOOST_CHECK_EQUAL(set(sa_un, sizeof(sa_un)), "");
}
#endif // HAVE_SOCKADDR_UN

BOOST_AUTO_TEST_CASE(unix_socket_value)
{
    // Classification is purely syntactic
    BOOST_CHECK(IsUnixSocketValue("unix:/tmp/a.sock", /*allow_default=*/false));
    BOOST_CHECK(IsUnixSocketValue("unix:8080", /*allow_default=*/false));
    BOOST_CHECK(IsUnixSocketValue("unix:/tmp/a:b.sock", /*allow_default=*/false));
    BOOST_CHECK(IsUnixSocketValue("unix:", /*allow_default=*/false));
    BOOST_CHECK(!IsUnixSocketValue("unix", /*allow_default=*/false));
    BOOST_CHECK(IsUnixSocketValue("unix", /*allow_default=*/true));
    BOOST_CHECK(!IsUnixSocketValue("unixfoo", /*allow_default=*/true));
    BOOST_CHECK(!IsUnixSocketValue("127.0.0.1:8332", /*allow_default=*/true));
    BOOST_CHECK(!IsUnixSocketValue("", /*allow_default=*/true));

    const fs::path datadir{fs::PathFromString("/data/regtest")};
    {
        const auto not_unix{ResolveUnixSocketAddr("127.0.0.1", datadir, "http.sock")};
        BOOST_REQUIRE(!not_unix);
        BOOST_CHECK_EQUAL(not_unix.error(), "'127.0.0.1' is not a unix socket address");
    }
#ifdef HAVE_SOCKADDR_UN
    // Returns the resolved ToStringAddrPort(), or "" on failure
    const auto resolve{[&](std::string_view value) {
        const auto addr{ResolveUnixSocketAddr(value, datadir, "http.sock")};
        return addr ? addr->ToStringAddrPort() : std::string{};
    }};
    // The keyword selects the default name in datadir, as for -ipcbind
    BOOST_CHECK_EQUAL(resolve("unix"), "unix:/data/regtest/http.sock");
    BOOST_CHECK_EQUAL(resolve("unix:"), "unix:/data/regtest/http.sock");
    // Relative paths are interpreted relative to datadir
    BOOST_CHECK_EQUAL(resolve("unix:rpc.sock"), "unix:/data/regtest/rpc.sock");
    BOOST_CHECK_EQUAL(resolve("unix:sub/rpc.sock"), "unix:/data/regtest/sub/rpc.sock");
    BOOST_CHECK_EQUAL(resolve("unix:8080"), "unix:/data/regtest/8080");
    // Absolute paths are used as is
    BOOST_CHECK_EQUAL(resolve("unix:/tmp/rpc.sock"), "unix:/tmp/rpc.sock");

    // The datadir prefix counts toward the path length limit...
    const size_t max_path_len{sizeof(sockaddr_un::sun_path) - 1};
    const std::string longest_name(max_path_len - fs::PathToString(datadir).size() - 1, 'a');
    BOOST_CHECK_EQUAL(resolve("unix:" + longest_name), "unix:/data/regtest/" + longest_name);
    BOOST_CHECK_EQUAL(resolve("unix:" + longest_name + "a"), "");
    {
        const auto too_long{ResolveUnixSocketAddr("unix:" + longest_name + "a", datadir, "http.sock")};
        BOOST_REQUIRE(!too_long);
        BOOST_CHECK(too_long.error().find("exceeds the maximum unix socket path length") != std::string::npos);
        // The error names the resolved path, including the datadir prefix
        BOOST_CHECK(too_long.error().find("/data/regtest/" + longest_name + "a") != std::string::npos);
    }
    // ...but not for an absolute path
    const std::string longest_abs{"/" + std::string(max_path_len - 1, 'a')};
    BOOST_CHECK_EQUAL(resolve("unix:" + longest_abs), "unix:" + longest_abs);
    BOOST_CHECK_EQUAL(resolve("unix:" + longest_abs + "a"), "");
#else
    const auto unsupported{ResolveUnixSocketAddr("unix", datadir, "http.sock")};
    BOOST_REQUIRE(!unsupported);
    BOOST_CHECK_EQUAL(unsupported.error(), "unix sockets are not supported on this platform");
#endif
}

BOOST_AUTO_TEST_CASE(socket_addr)
{
    const CService ipv4{LookupNumeric("142.250.217.142", 8333)};
    const CService ipv6{LookupNumeric("2607:f8b0:4006:80f::200e", 8333)};

    sockaddr_storage storage;
    auto* paddr{reinterpret_cast<sockaddr*>(&storage)};
    socklen_t len{sizeof(storage)};

    // Default-constructed holds an invalid CService
    const SocketAddr empty;
    BOOST_CHECK(!empty.IsValid());
    BOOST_CHECK(!empty.IsUnix());
    // CService default values
    BOOST_CHECK_EQUAL(empty.GetSAFamily(), AF_INET6);
    BOOST_CHECK(empty.GetSockAddr(paddr, &len));

    // IP addresses delegate to CService
    for (const CService& ip : {ipv4, ipv6}) {
        const SocketAddr addr{ip};
        BOOST_CHECK(addr.IsValid());
        BOOST_CHECK(!addr.IsUnix());
        BOOST_CHECK_EQUAL(addr.GetSAFamily(), ip.GetSAFamily());
        BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), ip.ToStringAddrPort());
        BOOST_CHECK_EQUAL(addr.GetHost(), ip.ToStringAddr());
        BOOST_CHECK(addr.GetCNetAddr() == static_cast<CNetAddr>(ip));
    }
    BOOST_CHECK_EQUAL(SocketAddr{ipv4}.GetHost(), "142.250.217.142");
    BOOST_CHECK_EQUAL(SocketAddr{ipv6}.GetHost(), "2607:f8b0:4006:80f::200e");
    BOOST_CHECK_EQUAL(SocketAddr{ipv6}.ToStringAddrPort(), "[2607:f8b0:4006:80f::200e]:8333");

#ifdef HAVE_SOCKADDR_UN
    // Unix socket addresses delegate to UnixSocketAddr
    const UnixSocketAddr unix_addr{"unix:/tmp/bitcoin.sock"};
    {
        const SocketAddr addr{unix_addr};
        BOOST_CHECK(addr.IsValid());
        BOOST_CHECK(addr.IsUnix());
        BOOST_CHECK_EQUAL(addr.GetSAFamily(), AF_UNIX);
        BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), "unix:/tmp/bitcoin.sock");
        BOOST_CHECK_EQUAL(addr.GetHost(), "localhost");
        BOOST_CHECK(!addr.GetCNetAddr().IsValid());
        BOOST_CHECK(addr.GetCNetAddr() == CNetAddr{});
    }

    // SetSockAddr() switches between alternatives in both directions
    SocketAddr addr{ipv4};
    len = sizeof(storage);
    BOOST_REQUIRE(unix_addr.GetSockAddr(paddr, &len));
    BOOST_REQUIRE(addr.SetSockAddr(paddr, len));
    BOOST_CHECK(addr.IsUnix());
    BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), unix_addr.ToStringAddrPort());

    len = sizeof(storage);
    BOOST_REQUIRE(ipv6.GetSockAddr(paddr, &len));
    BOOST_REQUIRE(addr.SetSockAddr(paddr, len));
    BOOST_CHECK(!addr.IsUnix());
    BOOST_CHECK(addr.IsIPv6());
    BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), ipv6.ToStringAddrPort());

    // A failed SetSockAddr() leaves the unix address unchanged
    addr = SocketAddr{unix_addr};
    len = sizeof(storage);
    BOOST_REQUIRE(unix_addr.GetSockAddr(paddr, &len));
    BOOST_CHECK(!addr.SetSockAddr(paddr, len + 1));
    BOOST_CHECK_EQUAL(addr.ToStringAddrPort(), unix_addr.ToStringAddrPort());
#endif

    // A failed SetSockAddr() leaves the IP address unchanged
    SocketAddr ip_addr{ipv4};
    len = sizeof(storage);
    BOOST_REQUIRE(ipv4.GetSockAddr(paddr, &len));
    BOOST_CHECK(!ip_addr.SetSockAddr(paddr, len - 1));
    std::memset(&storage, 0, sizeof(storage));
    paddr->sa_family = AF_UNSPEC;
    BOOST_CHECK(!ip_addr.SetSockAddr(paddr, sizeof(storage)));
    BOOST_CHECK_EQUAL(ip_addr.ToStringAddrPort(), ipv4.ToStringAddrPort());
}

/** A Sock whose Connect() always fails with ECONNREFUSED */
class ConnectFailingSock : public ZeroSock
{
public:
    int Connect(const sockaddr*, socklen_t) const override
    {
        errno = ECONNREFUSED;
        return SOCKET_ERROR;
    }
private:
    using Sock::operator=;
};

/** A ZeroSock that records how it was created and the address passed to Connect() */
class ConnectRecordingSock : public ZeroSock
{
public:
    ConnectRecordingSock(int domain, int type, int protocol)
        : m_domain{domain}, m_type{type}, m_protocol{protocol} {}

    int Connect(const sockaddr* addr, socklen_t len) const override
    {
        m_connected_len = std::min<socklen_t>(len, sizeof(m_connected_addr));
        std::memcpy(&m_connected_addr, addr, m_connected_len);
        return 0;
    }

    const int m_domain;
    const int m_type;
    const int m_protocol;
    // Written by Connect(), which is const
    mutable sockaddr_storage m_connected_addr{};
    mutable socklen_t m_connected_len{0};
private:
    using Sock::operator=;
};

struct ConnectRecordingSockTestingSetup : public SocketTestingSetup {
    ConnectRecordingSockTestingSetup()
    {
        CreateSock = [](int d, int t, int p) -> std::unique_ptr<Sock> {
            return std::make_unique<ConnectRecordingSock>(d, t, p);
        };
    }
};

/**
 * Checks that Connect() creates the expected kind of socket and connects it
 * to the sockaddr described by expected_addr. Requires CreateSock to return
 * a ConnectRecordingSock.
 */
template <typename T>
static void CheckConnect(const T& connectable, int expected_domain, int expected_protocol, const std::string& expected_addr)
{
    const auto sock{connectable.Connect()};
    BOOST_REQUIRE(sock != nullptr);
    const auto* recording_sock{dynamic_cast<const ConnectRecordingSock*>(sock.get())};
    BOOST_REQUIRE(recording_sock != nullptr);
    BOOST_CHECK_EQUAL(recording_sock->m_domain, expected_domain);
    BOOST_CHECK_EQUAL(recording_sock->m_type, SOCK_STREAM);
    BOOST_CHECK_EQUAL(recording_sock->m_protocol, expected_protocol);
    SocketAddr connected;
    BOOST_REQUIRE(connected.SetSockAddr(reinterpret_cast<const sockaddr*>(&recording_sock->m_connected_addr), recording_sock->m_connected_len));
    BOOST_CHECK_EQUAL(connected.ToStringAddrPort(), expected_addr);
}

BOOST_FIXTURE_TEST_CASE(socket_addr_connect, ConnectRecordingSockTestingSetup)
{
    const SocketAddr ipv4{LookupNumeric("142.250.217.142", 8333)};
    CheckConnect(ipv4, AF_INET, IPPROTO_TCP, ipv4.ToStringAddrPort());
    const SocketAddr ipv6{LookupNumeric("2607:f8b0:4006:80f::200e", 8333)};
    CheckConnect(ipv6, AF_INET6, IPPROTO_TCP, ipv6.ToStringAddrPort());
#ifdef HAVE_SOCKADDR_UN
    const SocketAddr unix_addr{UnixSocketAddr{"unix:/tmp/bitcoin.sock"}};
    CheckConnect(unix_addr, AF_UNIX, 0, unix_addr.ToStringAddrPort());
#endif

    // Invalid address doesn't even create a socket
    BOOST_CHECK(SocketAddr{}.Connect() == nullptr);

    // Failure to create a socket is handled
    CreateSock = [](int, int, int) -> std::unique_ptr<Sock> { return nullptr; };
    BOOST_CHECK(SocketAddr{LookupNumeric("142.250.217.142", 8333)}.Connect() == nullptr);
#ifdef HAVE_SOCKADDR_UN
    BOOST_CHECK(SocketAddr{UnixSocketAddr{"unix:/tmp/bitcoin.sock"}}.Connect() == nullptr);
#endif
}

BOOST_FIXTURE_TEST_CASE(proxy_api, ConnectRecordingSockTestingSetup)
{
    // Default-constructed proxy is invalid and can't connect
    const Proxy empty;
    BOOST_CHECK(!empty.IsValid());
    BOOST_CHECK(!empty.m_tor_stream_isolation);
    BOOST_CHECK(empty.Connect() == nullptr);
    // An invalid CService makes an invalid proxy
    BOOST_CHECK(!Proxy{CService{}}.IsValid());
    BOOST_CHECK(Proxy{CService{}}.Connect() == nullptr);

    // IP proxies
    const CService ipv4{LookupNumeric("127.0.0.1", 9050)};
    const CService ipv6{LookupNumeric("::1", 9050)};
    const Proxy proxy4{ipv4};
    BOOST_CHECK(proxy4.IsValid());
    BOOST_CHECK_EQUAL(proxy4.GetSAFamily(), AF_INET);
    BOOST_CHECK_EQUAL(proxy4.ToString(), "127.0.0.1:9050");
    CheckConnect(proxy4, AF_INET, IPPROTO_TCP, "127.0.0.1:9050");

    const Proxy proxy6{ipv6};
    BOOST_CHECK(proxy6.IsValid());
    BOOST_CHECK_EQUAL(proxy6.GetSAFamily(), AF_INET6);
    BOOST_CHECK_EQUAL(proxy6.ToString(), "[::1]:9050");
    CheckConnect(proxy6, AF_INET6, IPPROTO_TCP, "[::1]:9050");

    const std::string path{"unix:/tmp/tor/socks.sock"};
#ifdef HAVE_SOCKADDR_UN
    // Unix socket proxies
    const Proxy proxy_unix{UnixSocketAddr(path)};
    BOOST_CHECK(proxy_unix.IsValid());
    BOOST_CHECK_EQUAL(proxy_unix.GetSAFamily(), AF_UNIX);
    BOOST_CHECK_EQUAL(proxy_unix.ToString(), path);
    CheckConnect(proxy_unix, AF_UNIX, 0, path);

    // A path without the "unix:" prefix, or too long for sun_path, is invalid
    for (const std::string& bad_path : {std::string{"/tmp/tor/socks.sock"},
                                        ADDR_PREFIX_UNIX + std::string(sizeof(sockaddr_un::sun_path), 'a')}) {
        const Proxy bad{UnixSocketAddr(bad_path)};
        BOOST_CHECK(!bad.IsValid());
        BOOST_CHECK_EQUAL(bad.GetSAFamily(), AF_UNIX);
        BOOST_CHECK_EQUAL(bad.ToString(), bad_path);
        BOOST_CHECK(bad.Connect() == nullptr);
    }
#else
    // Without unix socket support every unix path is invalid
    const Proxy proxy_unix{UnixSocketAddr(path)};
    BOOST_CHECK(!proxy_unix.IsValid());
    BOOST_CHECK(proxy_unix.Connect() == nullptr);
#endif

    // The isolation flag is stored and doesn't affect the address
    for (const bool isolation : {false, true}) {
        BOOST_CHECK_EQUAL(Proxy(ipv4, isolation).m_tor_stream_isolation, isolation);
        BOOST_CHECK_EQUAL(Proxy(ipv6, isolation).m_tor_stream_isolation, isolation);
        BOOST_CHECK_EQUAL(Proxy(UnixSocketAddr(path), isolation).m_tor_stream_isolation, isolation);
        BOOST_CHECK_EQUAL(Proxy(ipv4, isolation).ToString(), proxy4.ToString());
        BOOST_CHECK_EQUAL(Proxy(UnixSocketAddr(path), isolation).ToString(), proxy_unix.ToString());
    }

    // Failure to connect is handled: no half-initialized socket is returned
    CreateSock = [](int, int, int) -> std::unique_ptr<Sock> { return std::make_unique<ConnectFailingSock>(); };
    BOOST_CHECK(proxy4.Connect() == nullptr);
    BOOST_CHECK(proxy6.Connect() == nullptr);
#ifdef HAVE_SOCKADDR_UN
    BOOST_CHECK(proxy_unix.Connect() == nullptr);
#endif

    // Failure to create a socket is handled
    CreateSock = [](int, int, int) -> std::unique_ptr<Sock> { return nullptr; };
    BOOST_CHECK(proxy4.Connect() == nullptr);
#ifdef HAVE_SOCKADDR_UN
    BOOST_CHECK(proxy_unix.Connect() == nullptr);
#endif
}

BOOST_AUTO_TEST_CASE(asmap_test_vectors)
{
    // Randomly generated encoded ASMap with 128 ranges, up to 20-bit AS numbers.
    constexpr auto ASMAP_DATA{
        "fd38d50f7d5d665357f64bba6bfc190d6078a7e68e5d3ac032edf47f8b5755f87881bfd3633d9aa7c1fa279b3"
        "6fe26c63bbc9de44e0f04e5a382d8e1cddbe1c26653bc939d4327f287e8b4d1f8aff33176787cb0ff7cb28e3f"
        "daef0f8f47357f801c9f7ff7a99f7f9c9f99de7f3156ae00f23eb27a303bc486aa3ccc31ec19394c2f8a53ddd"
        "ea3cc56257f3b7e9b1f488be9c1137db823759aa4e071eef2e984aaf97b52d5f88d0f373dd190fe45e06efef1"
        "df7278be680a73a74c76db4dd910f1d30752c57fe2bc9f079f1a1e1b036c2a69219f11c5e11980a3fa51f4f82"
        "d36373de73b1863a8c27e36ae0e4f705be3d76ecff038a75bc0f92ba7e7f6f4080f1c47c34d095367ecf4406c"
        "1e3bbc17ba4d6f79ea3f031b876799ac268b1e0ea9babf0f9a8e5f6c55e363c6363df46afc696d7afceaf49b6"
        "e62df9e9dc27e70664cafe5c53df66dd0b8237678ada90e73f05ec60e6f6e96c3cbb1ea2f9dece115d5bdba10"
        "33e53662a7d72a29477b5beb35710591d3e23e5f0379baea62ffdee535bcdf879cbf69b88d7ea37c8015381cf"
        "63dc33d28f757a4a5e15d6a08"_hex};

    // Construct NetGroupManager with this data.
    auto netgroup{NetGroupManager::WithEmbeddedAsmap(ASMAP_DATA)};
    BOOST_CHECK(netgroup.UsingASMap());

    // Check some randomly-generated IPv6 addresses in it (biased towards the very beginning and
    // very end of the 128-bit range).
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("0:1559:183:3728:224c:65a5:62e6:e991", false)), 961340);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("d0:d493:faa0:8609:e927:8b75:293c:f5a4", false)), 961340);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("2a0:26f:8b2c:2ee7:c7d1:3b24:4705:3f7f", false)), 693761);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("a77:7cd4:4be5:a449:89f2:3212:78c6:ee38", false)), 0);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("1336:1ad6:2f26:4fe3:d809:7321:6e0d:4615", false)), 672176);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("1d56:abd0:a52f:a8d5:d5a7:a610:581d:d792", false)), 499880);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("378e:7290:54e5:bd36:4760:971c:e9b9:570d", false)), 0);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("406c:820b:272a:c045:b74e:fc0a:9ef2:cecc", false)), 248495);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("46c2:ae07:9d08:2d56:d473:2bc7:57e3:20ac", false)), 248495);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("50d2:3db6:52fa:2e7:12ec:5bc4:1bd1:49f9", false)), 124471);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("53e1:1812:ffa:dccf:f9f2:64be:75fa:795", false)), 539993);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("544d:eeba:3990:35d1:ad66:f9a3:576d:8617", false)), 374443);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("6a53:40dc:8f1d:3ffa:efeb:3aa3:df88:b94b", false)), 435070);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("87aa:d1c9:9edb:91e7:aab1:9eb9:baa0:de18", false)), 244121);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("9f00:48fa:88e3:4b67:a6f3:e6d2:5cc1:5be2", false)), 862116);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("c49f:9cc6:86ad:ba08:4580:315e:dbd1:8a62", false)), 969411);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("dff5:8021:61d:b17d:406d:7888:fdac:4a20", false)), 969411);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("e888:6791:2960:d723:bcfd:47e1:2d8c:599f", false)), 824019);
    BOOST_CHECK_EQUAL(netgroup.GetMappedAS(*LookupHost("ffff:d499:8c4b:4941:bc81:d5b9:b51e:85a8", false)), 824019);
}

BOOST_AUTO_TEST_SUITE_END()
