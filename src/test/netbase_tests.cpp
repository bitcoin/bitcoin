// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <string>
#include <numeric>

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
    NetGroupManager netgroupman{std::vector<bool>()}; // use /16
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
        CService(CNetAddr(in6_addr(IN6ADDR_LOOPBACK_INIT)), 0 /* port */),
        NODE_NONE,
        NodeSeconds{0x4966bc61s}, /* Fri Jan  9 02:54:25 UTC 2009 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(IN6ADDR_LOOPBACK_INIT)), 0x00f1 /* port */),
        NODE_NETWORK,
        NodeSeconds{0x83766279s}, /* Tue Nov 22 11:22:33 UTC 2039 */
    },
    CAddress{
        CService(CNetAddr(in6_addr(IN6ADDR_LOOPBACK_INIT)), 0xf1f2 /* port */),
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
    DataStream s{ParseHex(stream_addrv1_hex)};
    std::vector<CAddress> addresses_unserialized;

    s >> CAddress::V1_NETWORK(addresses_unserialized);
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
    DataStream s{ParseHex(stream_addrv2_hex)};
    std::vector<CAddress> addresses_unserialized;

    s >> CAddress::V2_NETWORK(addresses_unserialized);
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

    // Convert to std::vector<bool> format that the ASMap interpreter uses.
    std::vector<bool> asmap_bits;
    asmap_bits.reserve(ASMAP_DATA.size() * 8);
    for (auto byte : ASMAP_DATA) {
        for (int bit = 0; bit < 8; ++bit) {
            asmap_bits.push_back((std::to_integer<uint8_t>(byte) >> bit) & 1);
        }
    }

    // Construct NetGroupManager with this data.
    NetGroupManager netgroup{std::move(asmap_bits)};
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
