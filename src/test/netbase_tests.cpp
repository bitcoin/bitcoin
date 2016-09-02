// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netbase.h"
#include "test/test_bitcoin.h"

#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(netbase_tests, BasicTestingSetup)

static CNetAddr ResolveIP(const char* ip)
{
    CNetAddr addr;
    LookupHost(ip, addr, false);
    return addr;
}

static CSubNet ResolveSubNet(const char* subnet)
{
    CSubNet ret;
    LookupSubNet(subnet, ret);
    return ret;
}

BOOST_AUTO_TEST_CASE(netbase_networks)
{
    FAST_CHECK(ResolveIP("127.0.0.1").GetNetwork()                              == NET_UNROUTABLE);
    FAST_CHECK(ResolveIP("::1").GetNetwork()                                    == NET_UNROUTABLE);
    FAST_CHECK(ResolveIP("8.8.8.8").GetNetwork()                                == NET_IPV4);
    FAST_CHECK(ResolveIP("2001::8888").GetNetwork()                             == NET_IPV6);
    FAST_CHECK(ResolveIP("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetNetwork() == NET_TOR);

}

BOOST_AUTO_TEST_CASE(netbase_properties)
{

    FAST_CHECK(ResolveIP("127.0.0.1").IsIPv4());
    FAST_CHECK(ResolveIP("::FFFF:192.168.1.1").IsIPv4());
    FAST_CHECK(ResolveIP("::1").IsIPv6());
    FAST_CHECK(ResolveIP("10.0.0.1").IsRFC1918());
    FAST_CHECK(ResolveIP("192.168.1.1").IsRFC1918());
    FAST_CHECK(ResolveIP("172.31.255.255").IsRFC1918());
    FAST_CHECK(ResolveIP("2001:0DB8::").IsRFC3849());
    FAST_CHECK(ResolveIP("169.254.1.1").IsRFC3927());
    FAST_CHECK(ResolveIP("2002::1").IsRFC3964());
    FAST_CHECK(ResolveIP("FC00::").IsRFC4193());
    FAST_CHECK(ResolveIP("2001::2").IsRFC4380());
    FAST_CHECK(ResolveIP("2001:10::").IsRFC4843());
    FAST_CHECK(ResolveIP("FE80::").IsRFC4862());
    FAST_CHECK(ResolveIP("64:FF9B::").IsRFC6052());
    FAST_CHECK(ResolveIP("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").IsTor());
    FAST_CHECK(ResolveIP("127.0.0.1").IsLocal());
    FAST_CHECK(ResolveIP("::1").IsLocal());
    FAST_CHECK(ResolveIP("8.8.8.8").IsRoutable());
    FAST_CHECK(ResolveIP("2001::1").IsRoutable());
    FAST_CHECK(ResolveIP("127.0.0.1").IsValid());

}

bool static TestSplitHost(string test, string host, int port)
{
    string hostOut;
    int portOut = -1;
    SplitHostPort(test, portOut, hostOut);
    return hostOut == host && port == portOut;
}

BOOST_AUTO_TEST_CASE(netbase_splithost)
{
    FAST_CHECK(TestSplitHost("www.bitcoin.org", "www.bitcoin.org", -1));
    FAST_CHECK(TestSplitHost("[www.bitcoin.org]", "www.bitcoin.org", -1));
    FAST_CHECK(TestSplitHost("www.bitcoin.org:80", "www.bitcoin.org", 80));
    FAST_CHECK(TestSplitHost("[www.bitcoin.org]:80", "www.bitcoin.org", 80));
    FAST_CHECK(TestSplitHost("127.0.0.1", "127.0.0.1", -1));
    FAST_CHECK(TestSplitHost("127.0.0.1:8333", "127.0.0.1", 8333));
    FAST_CHECK(TestSplitHost("[127.0.0.1]", "127.0.0.1", -1));
    FAST_CHECK(TestSplitHost("[127.0.0.1]:8333", "127.0.0.1", 8333));
    FAST_CHECK(TestSplitHost("::ffff:127.0.0.1", "::ffff:127.0.0.1", -1));
    FAST_CHECK(TestSplitHost("[::ffff:127.0.0.1]:8333", "::ffff:127.0.0.1", 8333));
    FAST_CHECK(TestSplitHost("[::]:8333", "::", 8333));
    FAST_CHECK(TestSplitHost("::8333", "::8333", -1));
    FAST_CHECK(TestSplitHost(":8333", "", 8333));
    FAST_CHECK(TestSplitHost("[]:8333", "", 8333));
    FAST_CHECK(TestSplitHost("", "", -1));
}

bool static TestParse(string src, string canon)
{
    CService addr(LookupNumeric(src.c_str(), 65535));
    return canon == addr.ToString();
}

BOOST_AUTO_TEST_CASE(netbase_lookupnumeric)
{
    FAST_CHECK(TestParse("127.0.0.1", "127.0.0.1:65535"));
    FAST_CHECK(TestParse("127.0.0.1:8333", "127.0.0.1:8333"));
    FAST_CHECK(TestParse("::ffff:127.0.0.1", "127.0.0.1:65535"));
    FAST_CHECK(TestParse("::", "[::]:65535"));
    FAST_CHECK(TestParse("[::]:8333", "[::]:8333"));
    FAST_CHECK(TestParse("[127.0.0.1]", "127.0.0.1:65535"));
    FAST_CHECK(TestParse(":::", "[::]:0"));
}

BOOST_AUTO_TEST_CASE(onioncat_test)
{

    // values from https://web.archive.org/web/20121122003543/http://www.cypherpunk.at/onioncat/wiki/OnionCat
    CNetAddr addr1(ResolveIP("5wyqrzbvrdsumnok.onion"));
    CNetAddr addr2(ResolveIP("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca"));
    FAST_CHECK(addr1 == addr2);
    FAST_CHECK(addr1.IsTor());
    FAST_CHECK(addr1.ToStringIP() == "5wyqrzbvrdsumnok.onion");
    FAST_CHECK(addr1.IsRoutable());

}

BOOST_AUTO_TEST_CASE(subnet_test)
{

    FAST_CHECK(ResolveSubNet("1.2.3.0/24") == ResolveSubNet("1.2.3.0/255.255.255.0"));
    FAST_CHECK(ResolveSubNet("1.2.3.0/24") != ResolveSubNet("1.2.4.0/255.255.255.0"));
    FAST_CHECK(ResolveSubNet("1.2.3.0/24").Match(ResolveIP("1.2.3.4")));
    FAST_CHECK(!ResolveSubNet("1.2.2.0/24").Match(ResolveIP("1.2.3.4")));
    FAST_CHECK(ResolveSubNet("1.2.3.4").Match(ResolveIP("1.2.3.4")));
    FAST_CHECK(ResolveSubNet("1.2.3.4/32").Match(ResolveIP("1.2.3.4")));
    FAST_CHECK(!ResolveSubNet("1.2.3.4").Match(ResolveIP("5.6.7.8")));
    FAST_CHECK(!ResolveSubNet("1.2.3.4/32").Match(ResolveIP("5.6.7.8")));
    FAST_CHECK(ResolveSubNet("::ffff:127.0.0.1").Match(ResolveIP("127.0.0.1")));
    FAST_CHECK(ResolveSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:8")));
    FAST_CHECK(!ResolveSubNet("1:2:3:4:5:6:7:8").Match(ResolveIP("1:2:3:4:5:6:7:9")));
    FAST_CHECK(ResolveSubNet("1:2:3:4:5:6:7:0/112").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    FAST_CHECK(ResolveSubNet("192.168.0.1/24").Match(ResolveIP("192.168.0.2")));
    FAST_CHECK(ResolveSubNet("192.168.0.20/29").Match(ResolveIP("192.168.0.18")));
    FAST_CHECK(ResolveSubNet("1.2.2.1/24").Match(ResolveIP("1.2.2.4")));
    FAST_CHECK(ResolveSubNet("1.2.2.110/31").Match(ResolveIP("1.2.2.111")));
    FAST_CHECK(ResolveSubNet("1.2.2.20/26").Match(ResolveIP("1.2.2.63")));
    // All-Matching IPv6 Matches arbitrary IPv4 and IPv6
    FAST_CHECK(ResolveSubNet("::/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    FAST_CHECK(ResolveSubNet("::/0").Match(ResolveIP("1.2.3.4")));
    // All-Matching IPv4 does not Match IPv6
    FAST_CHECK(!ResolveSubNet("0.0.0.0/0").Match(ResolveIP("1:2:3:4:5:6:7:1234")));
    // Invalid subnets Match nothing (not even invalid addresses)
    FAST_CHECK(!CSubNet().Match(ResolveIP("1.2.3.4")));
    FAST_CHECK(!ResolveSubNet("").Match(ResolveIP("4.5.6.7")));
    FAST_CHECK(!ResolveSubNet("bloop").Match(ResolveIP("0.0.0.0")));
    FAST_CHECK(!ResolveSubNet("bloop").Match(ResolveIP("hab")));
    // Check valid/invalid
    FAST_CHECK(ResolveSubNet("1.2.3.0/0").IsValid());
    FAST_CHECK(!ResolveSubNet("1.2.3.0/-1").IsValid());
    FAST_CHECK(ResolveSubNet("1.2.3.0/32").IsValid());
    FAST_CHECK(!ResolveSubNet("1.2.3.0/33").IsValid());
    FAST_CHECK(ResolveSubNet("1:2:3:4:5:6:7:8/0").IsValid());
    FAST_CHECK(ResolveSubNet("1:2:3:4:5:6:7:8/33").IsValid());
    FAST_CHECK(!ResolveSubNet("1:2:3:4:5:6:7:8/-1").IsValid());
    FAST_CHECK(ResolveSubNet("1:2:3:4:5:6:7:8/128").IsValid());
    FAST_CHECK(!ResolveSubNet("1:2:3:4:5:6:7:8/129").IsValid());
    FAST_CHECK(!ResolveSubNet("fuzzy").IsValid());

    //CNetAddr constructor test
    FAST_CHECK(CSubNet(ResolveIP("127.0.0.1")).IsValid());
    FAST_CHECK(CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.1")));
    FAST_CHECK(!CSubNet(ResolveIP("127.0.0.1")).Match(ResolveIP("127.0.0.2")));
    FAST_CHECK(CSubNet(ResolveIP("127.0.0.1")).ToString() == "127.0.0.1/32");

    CSubNet subnet = CSubNet(ResolveIP("1.2.3.4"), 32);
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 8);
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), 0);
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.255.255.255"));
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("255.0.0.0"));
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = CSubNet(ResolveIP("1.2.3.4"), ResolveIP("0.0.0.0"));
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    FAST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).IsValid());
    FAST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:8")));
    FAST_CHECK(!CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).Match(ResolveIP("1:2:3:4:5:6:7:9")));
    FAST_CHECK(CSubNet(ResolveIP("1:2:3:4:5:6:7:8")).ToString() == "1:2:3:4:5:6:7:8/128");

    subnet = ResolveSubNet("1.2.3.4/255.255.255.255");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/32");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.254");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/31");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.252");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.4/30");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.248");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/29");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.240");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/28");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.224");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/27");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.192");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/26");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.128");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/25");
    subnet = ResolveSubNet("1.2.3.4/255.255.255.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.3.0/24");
    subnet = ResolveSubNet("1.2.3.4/255.255.254.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.2.0/23");
    subnet = ResolveSubNet("1.2.3.4/255.255.252.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/22");
    subnet = ResolveSubNet("1.2.3.4/255.255.248.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/21");
    subnet = ResolveSubNet("1.2.3.4/255.255.240.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/20");
    subnet = ResolveSubNet("1.2.3.4/255.255.224.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/19");
    subnet = ResolveSubNet("1.2.3.4/255.255.192.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/18");
    subnet = ResolveSubNet("1.2.3.4/255.255.128.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/17");
    subnet = ResolveSubNet("1.2.3.4/255.255.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/16");
    subnet = ResolveSubNet("1.2.3.4/255.254.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/15");
    subnet = ResolveSubNet("1.2.3.4/255.252.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/14");
    subnet = ResolveSubNet("1.2.3.4/255.248.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/13");
    subnet = ResolveSubNet("1.2.3.4/255.240.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/12");
    subnet = ResolveSubNet("1.2.3.4/255.224.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/11");
    subnet = ResolveSubNet("1.2.3.4/255.192.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/10");
    subnet = ResolveSubNet("1.2.3.4/255.128.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/9");
    subnet = ResolveSubNet("1.2.3.4/255.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.0.0.0/8");
    subnet = ResolveSubNet("1.2.3.4/254.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/7");
    subnet = ResolveSubNet("1.2.3.4/252.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/6");
    subnet = ResolveSubNet("1.2.3.4/248.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/5");
    subnet = ResolveSubNet("1.2.3.4/240.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/4");
    subnet = ResolveSubNet("1.2.3.4/224.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/3");
    subnet = ResolveSubNet("1.2.3.4/192.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/2");
    subnet = ResolveSubNet("1.2.3.4/128.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/1");
    subnet = ResolveSubNet("1.2.3.4/0.0.0.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "0.0.0.0/0");

    subnet = ResolveSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    FAST_CHECK_EQUAL(subnet.ToString(), "1:2:3:4:5:6:7:8/128");
    subnet = ResolveSubNet("1:2:3:4:5:6:7:8/ffff:0000:0000:0000:0000:0000:0000:0000");
    FAST_CHECK_EQUAL(subnet.ToString(), "1::/16");
    subnet = ResolveSubNet("1:2:3:4:5:6:7:8/0000:0000:0000:0000:0000:0000:0000:0000");
    FAST_CHECK_EQUAL(subnet.ToString(), "::/0");
    subnet = ResolveSubNet("1.2.3.4/255.255.232.0");
    FAST_CHECK_EQUAL(subnet.ToString(), "1.2.0.0/255.255.232.0");
    subnet = ResolveSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");
    FAST_CHECK_EQUAL(subnet.ToString(), "1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");

}

BOOST_AUTO_TEST_CASE(netbase_getgroup)
{

    FAST_CHECK(ResolveIP("127.0.0.1").GetGroup() == boost::assign::list_of(0)); // Local -> !Routable()
    FAST_CHECK(ResolveIP("257.0.0.1").GetGroup() == boost::assign::list_of(0)); // !Valid -> !Routable()
    FAST_CHECK(ResolveIP("10.0.0.1").GetGroup() == boost::assign::list_of(0)); // RFC1918 -> !Routable()
    FAST_CHECK(ResolveIP("169.254.1.1").GetGroup() == boost::assign::list_of(0)); // RFC3927 -> !Routable()
    FAST_CHECK(ResolveIP("1.2.3.4").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // IPv4
    FAST_CHECK(ResolveIP("::FFFF:0:102:304").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC6145
    FAST_CHECK(ResolveIP("64:FF9B::102:304").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC6052
    FAST_CHECK(ResolveIP("2002:102:304:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC3964
    FAST_CHECK(ResolveIP("2001:0:9999:9999:9999:9999:FEFD:FCFB").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC4380
    FAST_CHECK(ResolveIP("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetGroup() == boost::assign::list_of((unsigned char)NET_TOR)(239)); // Tor
    FAST_CHECK(ResolveIP("2001:470:abcd:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV6)(32)(1)(4)(112)(175)); //he.net
    FAST_CHECK(ResolveIP("2001:2001:9999:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV6)(32)(1)(32)(1)); //IPv6

}

BOOST_AUTO_TEST_SUITE_END()
