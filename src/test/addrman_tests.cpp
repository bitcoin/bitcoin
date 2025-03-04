// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <addrman_impl.h>
#include <chainparams.h>
#include <clientversion.h>
#include <hash.h>
#include <netbase.h>
#include <random.h>
#include <test/data/asmap.raw.h>
#include <test/util/setup_common.h>
#include <util/asmap.h>
#include <util/string.h>

#include <gtest/gtest.h>

#include <optional>
#include <string>

using namespace std::literals;
using node::NodeContext;
using util::ToString;

static NetGroupManager EMPTY_NETGROUPMAN{std::vector<bool>()};
static const bool DETERMINISTIC{true};

static int32_t GetCheckRatio(const NodeContext& node_ctx)
{
    return std::clamp<int32_t>(node_ctx.args->GetIntArg("-checkaddrman", 100), 0, 1000000);
}

static CNetAddr ResolveIP(const std::string& ip)
{
    const std::optional<CNetAddr> addr{LookupHost(ip, false)};
    EXPECT_TRUE(addr.has_value()) << "failed to resolve: " << ip;
    return addr.value_or(CNetAddr{});
}

static CService ResolveService(const std::string& ip, uint16_t port = 0)
{
    const std::optional<CService> serv{Lookup(ip, port, false)};
    EXPECT_TRUE(serv.has_value()) << "failed to resolve: " << ip << ":" << port;
    return serv.value_or(CService{});
}


static std::vector<bool> FromBytes(std::span<const std::byte> source)
{
    int vector_size(source.size() * 8);
    std::vector<bool> result(vector_size);
    for (int byte_i = 0; byte_i < vector_size / 8; ++byte_i) {
        uint8_t cur_byte{std::to_integer<uint8_t>(source[byte_i])};
        for (int bit_i = 0; bit_i < 8; ++bit_i) {
            result[byte_i * 8 + bit_i] = (cur_byte >> bit_i) & 1;
        }
    }
    return result;
}

class AddrmanTests : public BasicTestingSetup, public testing::Test {};

TEST_F(AddrmanTests, AddrmanSimple)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    CNetAddr source = ResolveIP("252.2.2.2");

    // Test: Does Addrman respond correctly when empty.
    EXPECT_EQ(addrman->Size(), 0U);
    auto addr_null = addrman->Select().first;
    EXPECT_EQ(addr_null.ToStringAddrPort(), "[::]:0");

    // Test: Does Addrman::Add work as expected.
    CService addr1 = ResolveService("250.1.1.1", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), 1U);
    auto addr_ret1 = addrman->Select().first;
    EXPECT_EQ(addr_ret1.ToStringAddrPort(), "250.1.1.1:8333");

    // Test: Does IP address deduplication work correctly.
    //  Expected dup IP should not be added.
    CService addr1_dup = ResolveService("250.1.1.1", 8333);
    EXPECT_FALSE(addrman->Add({CAddress(addr1_dup, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), 1U);


    // Test: New table has one addr and we add a diff addr we should
    //  have at least one addr.
    // Note that addrman's size cannot be tested reliably after insertion, as
    // hash collisions may occur. But we can always be sure of at least one
    // success.

    CService addr2 = ResolveService("250.1.1.2", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr2, NODE_NONE)}, source));
    EXPECT_GE(addrman->Size(), 1);

    // Test: reset addrman and test AddrMan::Add multiple addresses works as expected
    addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    std::vector<CAddress> vAddr;
    vAddr.emplace_back(ResolveService("250.1.1.3", 8333), NODE_NONE);
    vAddr.emplace_back(ResolveService("250.1.1.4", 8333), NODE_NONE);
    EXPECT_TRUE(addrman->Add(vAddr, source));
    EXPECT_GE(addrman->Size(), 1);
}

TEST_F(AddrmanTests, AddrmanPorts)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    CNetAddr source = ResolveIP("252.2.2.2");

    EXPECT_EQ(addrman->Size(), 0U);

    // Test 7; Addr with same IP but diff port does not replace existing addr.
    CService addr1 = ResolveService("250.1.1.1", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), 1U);

    CService addr1_port = ResolveService("250.1.1.1", 8334);
    EXPECT_TRUE(addrman->Add({CAddress(addr1_port, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), 2U);
    auto addr_ret2 = addrman->Select().first;
    EXPECT_TRUE(addr_ret2.ToStringAddrPort() == "250.1.1.1:8333" || addr_ret2.ToStringAddrPort() == "250.1.1.1:8334");

    // Test: Add same IP but diff port to tried table; this converts the entry with
    // the specified port to tried, but not the other.
    addrman->Good(CAddress(addr1_port, NODE_NONE));
    EXPECT_EQ(addrman->Size(), 2U);
    bool new_only = true;
    auto addr_ret3 = addrman->Select(new_only).first;
    EXPECT_EQ(addr_ret3.ToStringAddrPort(), "250.1.1.1:8333");
}

TEST_F(AddrmanTests, AddrmanSelect)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    EXPECT_FALSE(addrman->Select(false).first.IsValid());
    EXPECT_FALSE(addrman->Select(true).first.IsValid());

    CNetAddr source = ResolveIP("252.2.2.2");

    // Add 1 address to the new table
    CService addr1 = ResolveService("250.1.1.1", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), 1U);

    EXPECT_EQ(addrman->Select(/*new_only=*/true).first, addr1);
    EXPECT_EQ(addrman->Select(/*new_only=*/false).first, addr1);

    // Move address to the tried table
    EXPECT_TRUE(addrman->Good(CAddress(addr1, NODE_NONE)));

    EXPECT_EQ(addrman->Size(), 1U);
    EXPECT_FALSE(addrman->Select(/*new_only=*/true).first.IsValid());
    EXPECT_EQ(addrman->Select().first, addr1);
    EXPECT_EQ(addrman->Size(), 1U);

    // Add one address to the new table
    CService addr2 = ResolveService("250.3.1.1", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr2, NODE_NONE)}, addr2));
    EXPECT_EQ(addrman->Select(/*new_only=*/true).first, addr2);

    // Add two more addresses to the new table
    CService addr3 = ResolveService("250.3.2.2", 9999);
    CService addr4 = ResolveService("250.3.3.3", 9999);

    EXPECT_TRUE(addrman->Add({CAddress(addr3, NODE_NONE)}, addr2));
    EXPECT_TRUE(addrman->Add({CAddress(addr4, NODE_NONE)}, ResolveService("250.4.1.1", 8333)));

    // Add three addresses to tried table.
    CService addr5 = ResolveService("250.4.4.4", 8333);
    CService addr6 = ResolveService("250.4.5.5", 7777);
    CService addr7 = ResolveService("250.4.6.6", 8333);

    EXPECT_TRUE(addrman->Add({CAddress(addr5, NODE_NONE)}, addr3));
    EXPECT_TRUE(addrman->Good(CAddress(addr5, NODE_NONE)));
    EXPECT_TRUE(addrman->Add({CAddress(addr6, NODE_NONE)}, addr3));
    EXPECT_TRUE(addrman->Good(CAddress(addr6, NODE_NONE)));
    EXPECT_TRUE(addrman->Add({CAddress(addr7, NODE_NONE)}, ResolveService("250.1.1.3", 8333)));
    EXPECT_TRUE(addrman->Good(CAddress(addr7, NODE_NONE)));

    // 6 addrs + 1 addr from last test = 7.
    EXPECT_EQ(addrman->Size(), 7U);

    // Select pulls from new and tried regardless of port number.
    std::set<uint16_t> ports;
    for (int i = 0; i < 20; ++i) {
        ports.insert(addrman->Select().first.GetPort());
    }
    EXPECT_EQ(ports.size(), 3U);
}

TEST_F(AddrmanTests, AddrmanSelectByNetwork)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    EXPECT_FALSE(addrman->Select(/*new_only=*/true, {NET_IPV4}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_IPV4}).first.IsValid());

    // add ipv4 address to the new table
    CNetAddr source = ResolveIP("252.2.2.2");
    CService addr1 = ResolveService("250.1.1.1", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));

    EXPECT_TRUE(addrman->Select(/*new_only=*/true, {NET_IPV4}).first == addr1);
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, {NET_IPV4}).first == addr1);
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_IPV6}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_ONION}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_I2P}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_CJDNS}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/true, {NET_CJDNS}).first.IsValid());
    EXPECT_TRUE(addrman->Select(/*new_only=*/false).first == addr1);

    // add I2P address to the new table
    CAddress i2p_addr;
    i2p_addr.SetSpecial("udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p");
    EXPECT_TRUE(addrman->Add({i2p_addr}, source));

    EXPECT_TRUE(addrman->Select(/*new_only=*/true, {NET_I2P}).first == i2p_addr);
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, {NET_I2P}).first == i2p_addr);
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, {NET_IPV4}).first == addr1);
    std::unordered_set<Network> nets_with_entries = {NET_IPV4, NET_I2P};
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, nets_with_entries).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_IPV6}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_ONION}).first.IsValid());
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, {NET_CJDNS}).first.IsValid());
    std::unordered_set<Network> nets_without_entries = {NET_IPV6, NET_ONION, NET_CJDNS};
    EXPECT_FALSE(addrman->Select(/*new_only=*/false, nets_without_entries).first.IsValid());

    // bump I2P address to tried table
    EXPECT_TRUE(addrman->Good(i2p_addr));

    EXPECT_FALSE(addrman->Select(/*new_only=*/true, {NET_I2P}).first.IsValid());
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, {NET_I2P}).first == i2p_addr);

    // add another I2P address to the new table
    CAddress i2p_addr2;
    i2p_addr2.SetSpecial("c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p");
    EXPECT_TRUE(addrman->Add({i2p_addr2}, source));

    EXPECT_TRUE(addrman->Select(/*new_only=*/true, {NET_I2P}).first == i2p_addr2);

    // ensure that both new and tried table are selected from
    bool new_selected{false};
    bool tried_selected{false};
    int counter = 256;

    while (--counter > 0 && (!new_selected || !tried_selected)) {
        const CAddress selected{addrman->Select(/*new_only=*/false, {NET_I2P}).first};
        EXPECT_TRUE(selected == i2p_addr || selected == i2p_addr2);
        if (selected == i2p_addr) {
            tried_selected = true;
        } else {
            new_selected = true;
        }
    }

    EXPECT_TRUE(new_selected);
    EXPECT_TRUE(tried_selected);
}

TEST_F(AddrmanTests, AddrmanSelectSpecial)
{
    // use a non-deterministic addrman to ensure a passing test isn't due to setup
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, /*deterministic=*/false, GetCheckRatio(m_node));

    CNetAddr source = ResolveIP("252.2.2.2");

    // add I2P address to the tried table
    CAddress i2p_addr;
    i2p_addr.SetSpecial("udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p");
    EXPECT_TRUE(addrman->Add({i2p_addr}, source));
    EXPECT_TRUE(addrman->Good(i2p_addr));

    // add ipv4 address to the new table
    CService addr1 = ResolveService("250.1.1.3", 8333);
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));

    // since the only ipv4 address is on the new table, ensure that the new
    // table gets selected even if new_only is false. if the table was being
    // selected at random, this test will sporadically fail
    EXPECT_TRUE(addrman->Select(/*new_only=*/false, {NET_IPV4}).first == addr1);
}

TEST_F(AddrmanTests, AddrmanNewCollisions)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    CNetAddr source = ResolveIP("252.2.2.2");

    uint32_t num_addrs{0};

    EXPECT_EQ(addrman->Size(), num_addrs);

    while (num_addrs < 22) { // Magic number! 250.1.1.1 - 250.1.1.22 do not collide with deterministic key = 1
        CService addr = ResolveService("250.1.1." + ToString(++num_addrs));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));

        // Test: No collision in new table yet.
        EXPECT_EQ(addrman->Size(), num_addrs);
    }

    // Test: new table collision!
    CService addr1 = ResolveService("250.1.1." + ToString(++num_addrs));
    uint32_t collisions{1};
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), num_addrs - collisions);

    CService addr2 = ResolveService("250.1.1." + ToString(++num_addrs));
    EXPECT_TRUE(addrman->Add({CAddress(addr2, NODE_NONE)}, source));
    EXPECT_EQ(addrman->Size(), num_addrs - collisions);
}

TEST_F(AddrmanTests, AddrmanNewMultiplicity)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    CAddress addr{CAddress(ResolveService("253.3.3.3", 8333), NODE_NONE)};
    const auto start_time{Now<NodeSeconds>()};
    addr.nTime = start_time;

    // test that multiplicity stays at 1 if nTime doesn't increase
    for (unsigned int i = 1; i < 20; ++i) {
        std::string addr_ip{ToString(i % 256) + "." + ToString(i >> 8 % 256) + ".1.1"};
        CNetAddr source{ResolveIP(addr_ip)};
        addrman->Add({addr}, source);
    }
    AddressPosition addr_pos = addrman->FindAddressEntry(addr).value();
    EXPECT_EQ(addr_pos.multiplicity, 1U);
    EXPECT_EQ(addrman->Size(), 1U);

    // if nTime increases, an addr can occur in up to 8 buckets
    // The acceptance probability decreases exponentially with existing multiplicity -
    // choose number of iterations such that it gets to 8 with deterministic addrman.
    for (unsigned int i = 1; i < 400; ++i) {
        std::string addr_ip{ToString(i % 256) + "." + ToString(i >> 8 % 256) + ".1.1"};
        CNetAddr source{ResolveIP(addr_ip)};
        addr.nTime = start_time + std::chrono::seconds{i};
        addrman->Add({addr}, source);
    }
    AddressPosition addr_pos_multi = addrman->FindAddressEntry(addr).value();
    EXPECT_EQ(addr_pos_multi.multiplicity, 8U);
    // multiplicity doesn't affect size
    EXPECT_EQ(addrman->Size(), 1U);
}

TEST_F(AddrmanTests, AddrmanTriedCollisions)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    CNetAddr source = ResolveIP("252.2.2.2");

    uint32_t num_addrs{0};

    EXPECT_EQ(addrman->Size(), num_addrs);

    while (num_addrs < 35) { // Magic number! 250.1.1.1 - 250.1.1.35 do not collide in tried with deterministic key = 1
        CService addr = ResolveService("250.1.1." + ToString(++num_addrs));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));

        // Test: Add to tried without collision
        EXPECT_TRUE(addrman->Good(CAddress(addr, NODE_NONE)));

    }

    // Test: Unable to add to tried table due to collision!
    CService addr1 = ResolveService("250.1.1." + ToString(++num_addrs));
    EXPECT_TRUE(addrman->Add({CAddress(addr1, NODE_NONE)}, source));
    EXPECT_TRUE(!addrman->Good(CAddress(addr1, NODE_NONE)));

    // Test: Add the next address to tried without collision
    CService addr2 = ResolveService("250.1.1." + ToString(++num_addrs));
    EXPECT_TRUE(addrman->Add({CAddress(addr2, NODE_NONE)}, source));
    EXPECT_TRUE(addrman->Good(CAddress(addr2, NODE_NONE)));
}

TEST_F(AddrmanTests, AddrmanGetAddr)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    // Test: Sanity check, GetAddr should never return anything if addrman
    //  is empty.
    EXPECT_EQ(addrman->Size(), 0U);
    std::vector<CAddress> vAddr1 = addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt);
    EXPECT_EQ(vAddr1.size(), 0U);

    CAddress addr1 = CAddress(ResolveService("250.250.2.1", 8333), NODE_NONE);
    addr1.nTime = Now<NodeSeconds>(); // Set time so isTerrible = false
    CAddress addr2 = CAddress(ResolveService("250.251.2.2", 9999), NODE_NONE);
    addr2.nTime = Now<NodeSeconds>();
    CAddress addr3 = CAddress(ResolveService("251.252.2.3", 8333), NODE_NONE);
    addr3.nTime = Now<NodeSeconds>();
    CAddress addr4 = CAddress(ResolveService("252.253.3.4", 8333), NODE_NONE);
    addr4.nTime = Now<NodeSeconds>();
    CAddress addr5 = CAddress(ResolveService("252.254.4.5", 8333), NODE_NONE);
    addr5.nTime = Now<NodeSeconds>();
    CNetAddr source1 = ResolveIP("250.1.2.1");
    CNetAddr source2 = ResolveIP("250.2.3.3");

    // Test: Ensure GetAddr works with new addresses.
    EXPECT_TRUE(addrman->Add({addr1, addr3, addr5}, source1));
    EXPECT_TRUE(addrman->Add({addr2, addr4}, source2));

    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt).size(), 5U);
    // Net processing asks for 23% of addresses. 23% of 5 is 1 rounded down.
    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/2500, /*max_pct=*/23, /*network=*/std::nullopt).size(), 1U);

    // Test: Ensure GetAddr works with new and tried addresses.
    EXPECT_TRUE(addrman->Good(CAddress(addr1, NODE_NONE)));
    EXPECT_TRUE(addrman->Good(CAddress(addr2, NODE_NONE)));
    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt).size(), 5U);
    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/2500, /*max_pct=*/23, /*network=*/std::nullopt).size(), 1U);

    // Test: Ensure GetAddr still returns 23% when addrman has many addrs.
    for (unsigned int i = 1; i < (8 * 256); i++) {
        int octet1 = i % 256;
        int octet2 = i >> 8 % 256;
        std::string strAddr = ToString(octet1) + "." + ToString(octet2) + ".1.23";
        CAddress addr = CAddress(ResolveService(strAddr), NODE_NONE);

        // Ensure that for all addrs in addrman, isTerrible == false.
        addr.nTime = Now<NodeSeconds>();
        addrman->Add({addr}, ResolveIP(strAddr));
        if (i % 8 == 0)
            addrman->Good(addr);
    }
    std::vector<CAddress> vAddr = addrman->GetAddr(/*max_addresses=*/2500, /*max_pct=*/23, /*network=*/std::nullopt);

    size_t percent23 = (addrman->Size() * 23) / 100;
    EXPECT_EQ(vAddr.size(), percent23);
    EXPECT_EQ(vAddr.size(), 461U);
    // (addrman.Size() < number of addresses added) due to address collisions.
    EXPECT_EQ(addrman->Size(), 2006U);
}

TEST_F(AddrmanTests, GetAddrUnfiltered)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    // Set time on this addr so isTerrible = false
    CAddress addr1 = CAddress(ResolveService("250.250.2.1", 8333), NODE_NONE);
    addr1.nTime = Now<NodeSeconds>();
    // Not setting time so this addr should be isTerrible = true
    CAddress addr2 = CAddress(ResolveService("250.251.2.2", 9999), NODE_NONE);

    CNetAddr source = ResolveIP("250.1.2.1");
    EXPECT_TRUE(addrman->Add({addr1, addr2}, source));

    // Set time on this addr so isTerrible = false
    CAddress addr3 = CAddress(ResolveService("250.251.2.3", 9998), NODE_NONE);
    addr3.nTime = Now<NodeSeconds>();
    addrman->Good(addr3, /*time=*/Now<NodeSeconds>());
    EXPECT_TRUE(addrman->Add({addr3}, source));
    // The time is set, but after ADDRMAN_RETRIES unsuccessful attempts not
    // retried in the last minute, this addr should be isTerrible = true
    for (size_t i = 0; i < 3; ++i) {
        addrman->Attempt(addr3, /*fCountFailure=*/true, /*time=*/Now<NodeSeconds>() - 61s);
    }

    // GetAddr filtered by quality (i.e. not IsTerrible) should only return addr1
    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt).size(), 1U);
    // Unfiltered GetAddr should return all addrs
    EXPECT_EQ(addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt, /*filtered=*/false).size(), 3U);
}

TEST_F(AddrmanTests, CaddrinfoGetTriedBucketLegacy)
{
    CAddress addr1 = CAddress(ResolveService("250.1.1.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.1.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.1.1");


    AddrInfo info1 = AddrInfo(addr1, source1);

    uint256 nKey1 = (HashWriter{} << 1).GetHash();
    uint256 nKey2 = (HashWriter{} << 2).GetHash();

    EXPECT_EQ(info1.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN), 40);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    EXPECT_TRUE(info1.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN) != info1.GetTriedBucket(nKey2, EMPTY_NETGROUPMAN));

    // Test: Two addresses with same IP but different ports can map to
    //  different buckets because they have different keys.
    AddrInfo info2 = AddrInfo(addr2, source1);

    EXPECT_TRUE(info1.GetKey() != info2.GetKey());
    EXPECT_TRUE(info1.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN) != info2.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        AddrInfo infoi = AddrInfo(
            CAddress(ResolveService("250.1.1." + ToString(i)), NODE_NONE),
            ResolveIP("250.1.1." + ToString(i)));
        int bucket = infoi.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same /16 prefix should
    // never get more than 8 buckets with legacy grouping
    EXPECT_EQ(buckets.size(), 8U);

    buckets.clear();
    for (int j = 0; j < 255; j++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("250." + ToString(j) + ".1.1"), NODE_NONE),
            ResolveIP("250." + ToString(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, EMPTY_NETGROUPMAN);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix should map to more than
    // 8 buckets with legacy grouping
    EXPECT_EQ(buckets.size(), 160U);
}

TEST_F(AddrmanTests, CaddrinfoGetNewBucketLegacy)
{
    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.2.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.2.1");

    AddrInfo info1 = AddrInfo(addr1, source1);

    uint256 nKey1 = (HashWriter{} << 1).GetHash();
    uint256 nKey2 = (HashWriter{} << 2).GetHash();

    // Test: Make sure the buckets are what we expect
    EXPECT_EQ(info1.GetNewBucket(nKey1, EMPTY_NETGROUPMAN), 786);
    EXPECT_EQ(info1.GetNewBucket(nKey1, source1, EMPTY_NETGROUPMAN), 786);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    EXPECT_NE(info1.GetNewBucket(nKey1, EMPTY_NETGROUPMAN), info1.GetNewBucket(nKey2, EMPTY_NETGROUPMAN));

    // Test: Ports should not affect bucket placement in the addr
    AddrInfo info2 = AddrInfo(addr2, source1);
    EXPECT_NE(info1.GetKey(), info2.GetKey());
    EXPECT_EQ(info1.GetNewBucket(nKey1, EMPTY_NETGROUPMAN), info2.GetNewBucket(nKey1, EMPTY_NETGROUPMAN));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        AddrInfo infoi = AddrInfo(
            CAddress(ResolveService("250.1.1." + ToString(i)), NODE_NONE),
            ResolveIP("250.1.1." + ToString(i)));
        int bucket = infoi.GetNewBucket(nKey1, EMPTY_NETGROUPMAN);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same group (\16 prefix for IPv4) should
    //  always map to the same bucket.
    EXPECT_EQ(buckets.size(), 1U);

    buckets.clear();
    for (int j = 0; j < 4 * 255; j++) {
        AddrInfo infoj = AddrInfo(CAddress(
                                        ResolveService(
                                            ToString(250 + (j / 255)) + "." + ToString(j % 256) + ".1.1"), NODE_NONE),
            ResolveIP("251.4.1.1"));
        int bucket = infoj.GetNewBucket(nKey1, EMPTY_NETGROUPMAN);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same source groups should map to NO MORE
    //  than 64 buckets.
    EXPECT_LE(buckets.size(), 64);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("250." + ToString(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, EMPTY_NETGROUPMAN);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source groups should map to MORE
    //  than 64 buckets.
    EXPECT_GT(buckets.size(), 64);
}

// The following three test cases use asmap.raw
// We use an artificial minimal mock mapping
// 250.0.0.0/8 AS1000
// 101.1.0.0/16 AS1
// 101.2.0.0/16 AS2
// 101.3.0.0/16 AS3
// 101.4.0.0/16 AS4
// 101.5.0.0/16 AS5
// 101.6.0.0/16 AS6
// 101.7.0.0/16 AS7
// 101.8.0.0/16 AS8

TEST_F(AddrmanTests, CaddrinfoGetTriedBucket)
{
    std::vector<bool> asmap = FromBytes(test::data::asmap);
    NetGroupManager ngm_asmap{asmap};

    CAddress addr1 = CAddress(ResolveService("250.1.1.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.1.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.1.1");


    AddrInfo info1 = AddrInfo(addr1, source1);

    uint256 nKey1 = (HashWriter{} << 1).GetHash();
    uint256 nKey2 = (HashWriter{} << 2).GetHash();

    EXPECT_EQ(info1.GetTriedBucket(nKey1, ngm_asmap), 236);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    EXPECT_NE(info1.GetTriedBucket(nKey1, ngm_asmap), info1.GetTriedBucket(nKey2, ngm_asmap));

    // Test: Two addresses with same IP but different ports can map to
    //  different buckets because they have different keys.
    AddrInfo info2 = AddrInfo(addr2, source1);

    EXPECT_NE(info1.GetKey(), info2.GetKey());
    EXPECT_NE(info1.GetTriedBucket(nKey1, ngm_asmap), info2.GetTriedBucket(nKey1, ngm_asmap));

    std::set<int> buckets;
    for (int j = 0; j < 255; j++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("101." + ToString(j) + ".1.1"), NODE_NONE),
            ResolveIP("101." + ToString(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix MAY map to more than
    // 8 buckets.
    EXPECT_GT(buckets.size(), 8);

    buckets.clear();
    for (int j = 0; j < 255; j++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("250." + ToString(j) + ".1.1"), NODE_NONE),
            ResolveIP("250." + ToString(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix MAY NOT map to more than
    // 8 buckets.
    EXPECT_EQ(buckets.size(), 8);
}

TEST_F(AddrmanTests, CaddrinfoGetNewBucket)
{
    std::vector<bool> asmap = FromBytes(test::data::asmap);
    NetGroupManager ngm_asmap{asmap};

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.2.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.2.1");

    AddrInfo info1 = AddrInfo(addr1, source1);

    uint256 nKey1 = (HashWriter{} << 1).GetHash();
    uint256 nKey2 = (HashWriter{} << 2).GetHash();

    // Test: Make sure the buckets are what we expect
    EXPECT_EQ(info1.GetNewBucket(nKey1, ngm_asmap), 795);
    EXPECT_EQ(info1.GetNewBucket(nKey1, source1, ngm_asmap), 795);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    EXPECT_NE(info1.GetNewBucket(nKey1, ngm_asmap), info1.GetNewBucket(nKey2, ngm_asmap));

    // Test: Ports should not affect bucket placement in the addr
    AddrInfo info2 = AddrInfo(addr2, source1);
    EXPECT_NE(info1.GetKey(), info2.GetKey());
    EXPECT_EQ(info1.GetNewBucket(nKey1, ngm_asmap), info2.GetNewBucket(nKey1, ngm_asmap));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        AddrInfo infoi = AddrInfo(
            CAddress(ResolveService("250.1.1." + ToString(i)), NODE_NONE),
            ResolveIP("250.1.1." + ToString(i)));
        int bucket = infoi.GetNewBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same /16 prefix
    // usually map to the same bucket.
    EXPECT_EQ(buckets.size(), 1U);

    buckets.clear();
    for (int j = 0; j < 4 * 255; j++) {
        AddrInfo infoj = AddrInfo(CAddress(
                                        ResolveService(
                                            ToString(250 + (j / 255)) + "." + ToString(j % 256) + ".1.1"), NODE_NONE),
            ResolveIP("251.4.1.1"));
        int bucket = infoj.GetNewBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same source /16 prefix should not map to more
    // than 64 buckets.
    EXPECT_LE(buckets.size(), 64);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("101." + ToString(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source /16 prefixes usually map to MORE
    // than 1 bucket.
    EXPECT_GT(buckets.size(), 1);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        AddrInfo infoj = AddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("250." + ToString(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, ngm_asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source /16 prefixes sometimes map to NO MORE
    // than 1 bucket.
    EXPECT_EQ(buckets.size(), 1);
}

TEST_F(AddrmanTests, AddrmanSerialization)
{
    std::vector<bool> asmap1 = FromBytes(test::data::asmap);
    NetGroupManager netgroupman{asmap1};

    const auto ratio = GetCheckRatio(m_node);
    auto addrman_asmap1 = std::make_unique<AddrMan>(netgroupman, DETERMINISTIC, ratio);
    auto addrman_asmap1_dup = std::make_unique<AddrMan>(netgroupman, DETERMINISTIC, ratio);
    auto addrman_noasmap = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, ratio);

    DataStream stream{};

    CAddress addr = CAddress(ResolveService("250.1.1.1"), NODE_NONE);
    CNetAddr default_source;

    addrman_asmap1->Add({addr}, default_source);

    stream << *addrman_asmap1;
    // serizalizing/deserializing addrman with the same asmap
    stream >> *addrman_asmap1_dup;

    AddressPosition addr_pos1 = addrman_asmap1->FindAddressEntry(addr).value();
    AddressPosition addr_pos2 = addrman_asmap1_dup->FindAddressEntry(addr).value();
    EXPECT_NE(addr_pos1.multiplicity, 0);
    EXPECT_NE(addr_pos2.multiplicity, 0);

    EXPECT_TRUE(addr_pos1 == addr_pos2);

    // deserializing asmaped peers.dat to non-asmaped addrman
    stream << *addrman_asmap1;
    stream >> *addrman_noasmap;
    AddressPosition addr_pos3 = addrman_noasmap->FindAddressEntry(addr).value();
    EXPECT_NE(addr_pos3.multiplicity, 0);
    EXPECT_NE(addr_pos1.bucket, addr_pos3.bucket);
    EXPECT_NE(addr_pos1.position, addr_pos3.position);

    // deserializing non-asmaped peers.dat to asmaped addrman
    addrman_asmap1 = std::make_unique<AddrMan>(netgroupman, DETERMINISTIC, ratio);
    addrman_noasmap = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, ratio);
    addrman_noasmap->Add({addr}, default_source);
    stream << *addrman_noasmap;
    stream >> *addrman_asmap1;

    AddressPosition addr_pos4 = addrman_asmap1->FindAddressEntry(addr).value();
    EXPECT_NE(addr_pos4.multiplicity, 0);
    EXPECT_NE(addr_pos4.bucket, addr_pos3.bucket);
    EXPECT_TRUE(addr_pos4 == addr_pos2);

    // used to map to different buckets, now maps to the same bucket.
    addrman_asmap1 = std::make_unique<AddrMan>(netgroupman, DETERMINISTIC, ratio);
    addrman_noasmap = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, ratio);
    CAddress addr1 = CAddress(ResolveService("250.1.1.1"), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.2.1.1"), NODE_NONE);
    addrman_noasmap->Add({addr, addr2}, default_source);
    AddressPosition addr_pos5 = addrman_noasmap->FindAddressEntry(addr1).value();
    AddressPosition addr_pos6 = addrman_noasmap->FindAddressEntry(addr2).value();
    EXPECT_NE(addr_pos5.bucket, addr_pos6.bucket);
    stream << *addrman_noasmap;
    stream >> *addrman_asmap1;
    AddressPosition addr_pos7 = addrman_asmap1->FindAddressEntry(addr1).value();
    AddressPosition addr_pos8 = addrman_asmap1->FindAddressEntry(addr2).value();
    EXPECT_EQ(addr_pos7.bucket, addr_pos8.bucket);
    EXPECT_NE(addr_pos7.position, addr_pos8.position);
}

TEST_F(AddrmanTests, RemoveInvalid)
{
    // Confirm that invalid addresses are ignored in unserialization.

    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    DataStream stream{};

    const CAddress new1{ResolveService("5.5.5.5"), NODE_NONE};
    const CAddress new2{ResolveService("6.6.6.6"), NODE_NONE};
    const CAddress tried1{ResolveService("7.7.7.7"), NODE_NONE};
    const CAddress tried2{ResolveService("8.8.8.8"), NODE_NONE};

    addrman->Add({new1, tried1, new2, tried2}, CNetAddr{});
    addrman->Good(tried1);
    addrman->Good(tried2);
    ASSERT_EQ(addrman->Size(), 4);

    stream << *addrman;

    const std::string str{stream.str()};
    size_t pos;

    const char new2_raw[]{6, 6, 6, 6};
    const uint8_t new2_raw_replacement[]{0, 0, 0, 0}; // 0.0.0.0 is !IsValid()
    pos = str.find(new2_raw, 0, sizeof(new2_raw));
    ASSERT_NE(pos, std::string::npos);
    ASSERT_LE(pos + sizeof(new2_raw_replacement), stream.size());
    memcpy(stream.data() + pos, new2_raw_replacement, sizeof(new2_raw_replacement));

    const char tried2_raw[]{8, 8, 8, 8};
    const uint8_t tried2_raw_replacement[]{255, 255, 255, 255}; // 255.255.255.255 is !IsValid()
    pos = str.find(tried2_raw, 0, sizeof(tried2_raw));
    ASSERT_NE(pos, std::string::npos);
    ASSERT_LE(pos + sizeof(tried2_raw_replacement), stream.size());
    memcpy(stream.data() + pos, tried2_raw_replacement, sizeof(tried2_raw_replacement));

    addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    stream >> *addrman;
    EXPECT_EQ(addrman->Size(), 2);
}

TEST_F(AddrmanTests, AddrmanSelectTriedCollision)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    EXPECT_EQ(addrman->Size(), 0);

    // Empty addrman should return blank addrman info.
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");

    // Add twenty two addresses.
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1." + ToString(i));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));

        // No collisions in tried.
        EXPECT_TRUE(addrman->Good(addr));
        EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");
    }

    // Ensure Good handles duplicates well.
    // If an address is a duplicate, Good will return false but will not count it as a collision.
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1." + ToString(i));

        // Unable to add duplicate address to tried table.
        EXPECT_FALSE(addrman->Good(addr));

        // Verify duplicate address not marked as a collision.
        EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");
    }
}

TEST_F(AddrmanTests, AddrmanNoEvict)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    // Add 35 addresses.
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 36; i++) {
        CService addr = ResolveService("250.1.1." + ToString(i));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));

        // No collision yet.
        EXPECT_TRUE(addrman->Good(addr));
    }

    // Collision in tried table between 36 and 19.
    CService addr36 = ResolveService("250.1.1.36");
    EXPECT_TRUE(addrman->Add({CAddress(addr36, NODE_NONE)}, source));
    EXPECT_FALSE(addrman->Good(addr36));
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "250.1.1.19:0");

    // 36 should be discarded and 19 not evicted.
    // This means we keep 19 in the tried table and
    // 36 stays in the new table.
    addrman->ResolveCollisions();
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");

    // Lets create two collisions.
    for (unsigned int i = 37; i < 59; i++) {
        CService addr = ResolveService("250.1.1." + ToString(i));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));
        EXPECT_TRUE(addrman->Good(addr));
    }

    // Cause a collision in the tried table.
    CService addr59 = ResolveService("250.1.1.59");
    EXPECT_TRUE(addrman->Add({CAddress(addr59, NODE_NONE)}, source));
    EXPECT_TRUE(!addrman->Good(addr59));

    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "250.1.1.10:0");

    // Cause a second collision in the new table.
    EXPECT_TRUE(!addrman->Add({CAddress(addr36, NODE_NONE)}, source));

    // 36 still cannot be moved from new to tried due to colliding with 19
    EXPECT_FALSE(addrman->Good(addr36));
    EXPECT_NE(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");

    // Resolve all collisions.
    addrman->ResolveCollisions();
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");
}

TEST_F(AddrmanTests, AddrmanEvictionWorks)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));

    EXPECT_EQ(addrman->Size(), 0);

    // Empty addrman should return blank addrman info.
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "[::]:0");

    // Add 35 addresses
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 36; i++) {
        CService addr = ResolveService("250.1.1." + ToString(i));
        EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));

        // No collision yet.
        EXPECT_TRUE(addrman->Good(addr));
    }

    // Collision between 36 and 19.
    CService addr = ResolveService("250.1.1.36");
    EXPECT_TRUE(addrman->Add({CAddress(addr, NODE_NONE)}, source));
    EXPECT_TRUE(!addrman->Good(addr));

    auto info = addrman->SelectTriedCollision().first;
    EXPECT_EQ(info.ToStringAddrPort(), "250.1.1.19:0");

    // Ensure test of address fails, so that it is evicted.
    // Update entry in tried by setting last good connection in the deep past.
    EXPECT_FALSE(addrman->Good(info, NodeSeconds{1s}));
    addrman->Attempt(info, /*fCountFailure=*/false, Now<NodeSeconds>() - 61s);

    // Should swap 36 for 19.
    addrman->ResolveCollisions();
    EXPECT_TRUE(addrman->SelectTriedCollision().first.ToStringAddrPort() == "[::]:0");
    AddressPosition addr_pos{addrman->FindAddressEntry(CAddress(addr, NODE_NONE)).value()};
    EXPECT_TRUE(addr_pos.tried);

    // If 36 was swapped for 19, then adding 36 to tried should fail because we
    // are attempting to add a duplicate.
    // We check this by verifying Good() returns false and also verifying that
    // we have no collisions.
    EXPECT_TRUE(!addrman->Good(addr));
    EXPECT_TRUE(addrman->SelectTriedCollision().first.ToStringAddrPort() == "[::]:0");

    // 19 should fail as a collision (not a duplicate) if we now attempt to move
    // it to the tried table.
    CService addr19 = ResolveService("250.1.1.19");
    EXPECT_TRUE(!addrman->Good(addr19));
    EXPECT_EQ(addrman->SelectTriedCollision().first.ToStringAddrPort(), "250.1.1.36:0");

    // Eviction is also successful if too much time has passed since last try
    SetMockTime(GetTime() + 4 * 60 *60);
    addrman->ResolveCollisions();
    EXPECT_TRUE(addrman->SelectTriedCollision().first.ToStringAddrPort() == "[::]:0");
    //Now 19 is in tried again, and 36 back to new
    AddressPosition addr_pos19{addrman->FindAddressEntry(CAddress(addr19, NODE_NONE)).value()};
    EXPECT_TRUE(addr_pos19.tried);
    AddressPosition addr_pos36{addrman->FindAddressEntry(CAddress(addr, NODE_NONE)).value()};
    EXPECT_TRUE(!addr_pos36.tried);
}

static auto AddrmanToStream(const AddrMan& addrman)
{
    DataStream ssPeersIn{};
    ssPeersIn << Params().MessageStart();
    ssPeersIn << addrman;
    return ssPeersIn;
}

TEST_F(AddrmanTests, LoadAddrman)
{
    AddrMan addrman{EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node)};

    std::optional<CService> addr1, addr2, addr3, addr4;
    addr1 = Lookup("250.7.1.1", 8333, false);
    EXPECT_TRUE(addr1.has_value());
    addr2 = Lookup("250.7.2.2", 9999, false);
    EXPECT_TRUE(addr2.has_value());
    addr3 = Lookup("250.7.3.3", 9999, false);
    EXPECT_TRUE(addr3.has_value());
    addr3 = Lookup("250.7.3.3"s, 9999, false);
    EXPECT_TRUE(addr3.has_value());
    addr4 = Lookup("250.7.3.3\0example.com"s, 9999, false);
    EXPECT_TRUE(!addr4.has_value());

    // Add three addresses to new table.
    const std::optional<CService> source{Lookup("252.5.1.1", 8333, false)};
    EXPECT_TRUE(source.has_value());
    std::vector<CAddress> addresses{CAddress(addr1.value(), NODE_NONE), CAddress(addr2.value(), NODE_NONE), CAddress(addr3.value(), NODE_NONE)};
    EXPECT_TRUE(addrman.Add(addresses, source.value()));
    EXPECT_TRUE(addrman.Size() == 3);

    // Test that the de-serialization does not throw an exception.
    auto ssPeers1{AddrmanToStream(addrman)};
    bool exceptionThrown = false;
    AddrMan addrman1{EMPTY_NETGROUPMAN, !DETERMINISTIC, GetCheckRatio(m_node)};

    EXPECT_TRUE(addrman1.Size() == 0);
    try {
        unsigned char pchMsgTmp[4];
        ssPeers1 >> pchMsgTmp;
        ssPeers1 >> addrman1;
    } catch (const std::exception&) {
        exceptionThrown = true;
    }

    EXPECT_TRUE(addrman1.Size() == 3);
    EXPECT_TRUE(exceptionThrown == false);

    // Test that ReadFromStream creates an addrman with the correct number of addrs.
    DataStream ssPeers2 = AddrmanToStream(addrman);

    AddrMan addrman2{EMPTY_NETGROUPMAN, !DETERMINISTIC, GetCheckRatio(m_node)};
    EXPECT_TRUE(addrman2.Size() == 0);
    ReadFromStream(addrman2, ssPeers2);
    EXPECT_TRUE(addrman2.Size() == 3);
}

// Produce a corrupt peers.dat that claims 20 addrs when it only has one addr.
static void MakeCorruptPeersDat(DataStream s)
{
    s << ::Params().MessageStart();

    unsigned char nVersion = 1;
    s << nVersion;
    s << ((unsigned char)32);
    s << uint256::ONE;
    s << 10; // nNew
    s << 10; // nTried

    int nUBuckets = ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30);
    s << nUBuckets;

    const std::optional<CService> serv{Lookup("252.1.1.1", 7777, false)};
    EXPECT_TRUE(serv.has_value());
    CAddress addr = CAddress(serv.value(), NODE_NONE);
    std::optional<CNetAddr> resolved{LookupHost("252.2.2.2", false)};
    ASSERT_TRUE(resolved.has_value());
    AddrInfo info = AddrInfo(addr, resolved.value());
    s << CAddress::V1_DISK(info);
}

TEST_F(AddrmanTests, LoadAddrmanCorrupted)
{
    // Test that the de-serialization of corrupted peers.dat throws an exception.
    DataStream ssPeers1{};
    ASSERT_NO_FATAL_FAILURE(MakeCorruptPeersDat(ssPeers1));
    bool exceptionThrown = false;
    AddrMan addrman1{EMPTY_NETGROUPMAN, !DETERMINISTIC, GetCheckRatio(m_node)};
    EXPECT_TRUE(addrman1.Size() == 0);
    try {
        unsigned char pchMsgTmp[4];
        ssPeers1 >> pchMsgTmp;
        ssPeers1 >> addrman1;
    } catch (const std::exception&) {
        exceptionThrown = true;
    }
    EXPECT_TRUE(exceptionThrown);

    // Test that ReadFromStream fails if peers.dat is corrupt
    DataStream ssPeers2{};
    ASSERT_NO_FATAL_FAILURE(MakeCorruptPeersDat(ssPeers2));

    AddrMan addrman2{EMPTY_NETGROUPMAN, !DETERMINISTIC, GetCheckRatio(m_node)};
    EXPECT_TRUE(addrman2.Size() == 0);
    EXPECT_THROW(ReadFromStream(addrman2, ssPeers2), std::ios_base::failure);
}

TEST_F(AddrmanTests, AddrmanUpdateAddress)
{
    // Tests updating nTime via Connected() and nServices via SetServices() and Add()
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    CNetAddr source{ResolveIP("252.2.2.2")};
    CAddress addr{CAddress(ResolveService("250.1.1.1", 8333), NODE_NONE)};

    const auto start_time{Now<NodeSeconds>() - 10000s};
    addr.nTime = start_time;
    EXPECT_TRUE(addrman->Add({addr}, source));
    EXPECT_EQ(addrman->Size(), 1U);

    // Updating an addrman entry with a different port doesn't change it
    CAddress addr_diff_port{CAddress(ResolveService("250.1.1.1", 8334), NODE_NONE)};
    addr_diff_port.nTime = start_time;
    addrman->Connected(addr_diff_port);
    addrman->SetServices(addr_diff_port, NODE_NETWORK_LIMITED);
    std::vector<CAddress> vAddr1{addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt)};
    EXPECT_EQ(vAddr1.size(), 1U);
    EXPECT_EQ(vAddr1.at(0).nTime, start_time);
    EXPECT_EQ(vAddr1.at(0).nServices, NODE_NONE);

    // Updating an addrman entry with the correct port is successful
    addrman->Connected(addr);
    addrman->SetServices(addr, NODE_NETWORK_LIMITED);
    std::vector<CAddress> vAddr2 = addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt);
    EXPECT_EQ(vAddr2.size(), 1U);
    EXPECT_GE(vAddr2.at(0).nTime, start_time + 10000s);
    EXPECT_EQ(vAddr2.at(0).nServices, NODE_NETWORK_LIMITED);

    // Updating an existing addr through Add() (used in gossip relay) can add additional services but can't remove existing ones.
    CAddress addr_v2{CAddress(ResolveService("250.1.1.1", 8333), NODE_P2P_V2)};
    addr_v2.nTime = start_time;
    EXPECT_FALSE(addrman->Add({addr_v2}, source));
    std::vector<CAddress> vAddr3{addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt)};
    EXPECT_EQ(vAddr3.size(), 1U);
    EXPECT_EQ(vAddr3.at(0).nServices, NODE_P2P_V2 | NODE_NETWORK_LIMITED);

    // SetServices() (used when we connected to them) overwrites existing service flags
    addrman->SetServices(addr, NODE_NETWORK);
    std::vector<CAddress> vAddr4{addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt)};
    EXPECT_EQ(vAddr4.size(), 1U);
    EXPECT_EQ(vAddr4.at(0).nServices, NODE_NETWORK);

    // Promoting to Tried does not affect the service flags
    EXPECT_TRUE(addrman->Good(addr)); // addr has NODE_NONE
    std::vector<CAddress> vAddr5{addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt)};
    EXPECT_EQ(vAddr5.size(), 1U);
    EXPECT_EQ(vAddr5.at(0).nServices, NODE_NETWORK);

    // Adding service flags even works when the addr is in Tried
    EXPECT_TRUE(!addrman->Add({addr_v2}, source));
    std::vector<CAddress> vAddr6{addrman->GetAddr(/*max_addresses=*/0, /*max_pct=*/0, /*network=*/std::nullopt)};
    EXPECT_EQ(vAddr6.size(), 1U);
    EXPECT_EQ(vAddr6.at(0).nServices, NODE_NETWORK | NODE_P2P_V2);
}

TEST_F(AddrmanTests, AddrmanSize)
{
    auto addrman = std::make_unique<AddrMan>(EMPTY_NETGROUPMAN, DETERMINISTIC, GetCheckRatio(m_node));
    const CNetAddr source = ResolveIP("252.2.2.2");

    // empty addrman
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/std::nullopt), 0U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/std::nullopt), 0U);
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/true), 0U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/false), 0U);

    // add two ipv4 addresses, one to tried and new
    const CAddress addr1{ResolveService("250.1.1.1", 8333), NODE_NONE};
    EXPECT_TRUE(addrman->Add({addr1}, source));
    EXPECT_TRUE(addrman->Good(addr1));
    const CAddress addr2{ResolveService("250.1.1.2", 8333), NODE_NONE};
    EXPECT_TRUE(addrman->Add({addr2}, source));

    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/std::nullopt), 2U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/std::nullopt), 2U);
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/true), 1U);
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/false), 1U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/true), 1U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/false), 1U);

    // add one i2p address to new
    CService i2p_addr;
    i2p_addr.SetSpecial("UDHDrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.I2P");
    const CAddress addr3{i2p_addr, NODE_NONE};
    EXPECT_TRUE(addrman->Add({addr3}, source));
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/std::nullopt), 3U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_IPV4, /*in_new=*/std::nullopt), 2U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_I2P, /*in_new=*/std::nullopt), 1U);
    EXPECT_EQ(addrman->Size(/*net=*/NET_I2P, /*in_new=*/true), 1U);
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/true), 2U);
    EXPECT_EQ(addrman->Size(/*net=*/std::nullopt, /*in_new=*/false), 1U);
}
