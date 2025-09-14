// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <clientversion.h>
#include <evo/netinfo.h>
#include <netbase.h>
#include <streams.h>
#include <util/pointer.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_netinfo_tests, BasicTestingSetup)

using TestVectors =
    std::vector<std::tuple</*input=*/std::string, /*expected_ret_mn=*/NetInfoStatus, /*expected_ret_ext=*/NetInfoStatus>>;

static const TestVectors vals_main{
    // Address and port specified
    {"1.1.1.1:9999", NetInfoStatus::Success, NetInfoStatus::Success},
    // - Port should default to default P2P core with MnNetInfo
    // - Ports are no longer implied with ExtNetInfo
    {"1.1.1.1", NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Non-mainnet port on mainnet causes failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which 9998 isn't
    {"1.1.1.1:9998", NetInfoStatus::BadPort, NetInfoStatus::Success},
    // Internal addresses not allowed on mainnet
    {"127.0.0.1:9999", NetInfoStatus::NotRoutable, NetInfoStatus::NotRoutable},
    // Valid IPv4 formatting but invalid IPv4 address
    {"0.0.0.0:9999", NetInfoStatus::BadAddress, NetInfoStatus::BadAddress},
    // Port greater than uint16_t max
    {"1.1.1.1:99999", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // - Non-IPv4 addresses are prohibited in MnNetInfo
    // - Any valid BIP155 address is allowed in ExtNetInfo
    {"[2606:4700:4700::1111]:9999", NetInfoStatus::BadInput, NetInfoStatus::Success},
    // Domains are not allowed
    {"example.com:9999", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Incorrect IPv4 address
    {"1.1.1.256:9999", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Missing address
    {":9999", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
};

void ValidateGetEntries(const NetInfoList& entries, const size_t expected_size)
{
    BOOST_CHECK_EQUAL(entries.size(), expected_size);
    for (const NetInfoEntry& entry : entries) {
        BOOST_CHECK(entry.IsTriviallyValid());
    }
}

void TestMnNetInfo(const TestVectors& vals)
{
    for (const auto& [input, expected_ret, _] : vals) {
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(input), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty MnNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }
}

void TestExtNetInfo(const TestVectors& vals)
{
    for (const auto& [input, _, expected_ret] : vals) {
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(input), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty ExtNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }
}

BOOST_AUTO_TEST_CASE(mnnetinfo_rules_main)
{
    TestMnNetInfo(vals_main);

    {
        // MnNetInfo only stores one value, overwriting prohibited
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:9999"), NetInfoStatus::Success);
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.2:9999"), NetInfoStatus::MaxLimit);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
    }
}

BOOST_AUTO_TEST_CASE(extnetinfo_rules_main) { TestExtNetInfo(vals_main); }

static const TestVectors vals_reg{
    // - MnNetInfo doesn't mind using port 0
    // - ExtNetInfo requires non-zero ports
    {"1.1.1.1:0", NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Mainnet P2P port on non-mainnet cause failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which 9999 isn't
    {"1.1.1.1:9999", NetInfoStatus::BadPort, NetInfoStatus::Success},
    // - Non-mainnet P2P port is allowed in MnNetInfo regardless of bad port status
    // - Port 22 (SSH) is below the privileged ports threshold (1023) and is therefore a bad port, disallowed in ExtNetInfo
    {"1.1.1.1:22", NetInfoStatus::Success, NetInfoStatus::BadPort},
};

BOOST_FIXTURE_TEST_CASE(mnnetinfo_rules_reg, RegTestingSetup) { TestMnNetInfo(vals_reg); }

BOOST_FIXTURE_TEST_CASE(extnetinfo_rules_reg, RegTestingSetup)
{
    TestExtNetInfo(vals_reg);

    {
        // ExtNetInfo can store up to 4 entries, check limit enforcement
        ExtNetInfo netInfo;
        for (size_t idx{1}; idx <= MAX_ENTRIES_EXTNETINFO; idx++) {
            BOOST_CHECK_EQUAL(netInfo.AddEntry(strprintf("1.1.1.%d:9998", idx)), NetInfoStatus::Success);
        }
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.5:9998"), NetInfoStatus::MaxLimit);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/MAX_ENTRIES_EXTNETINFO);
    }

    {
        // ExtNetInfo does not allow storing duplicates
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:9998"), NetInfoStatus::Success);
        // Exact duplicates are prohibited
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:9998"), NetInfoStatus::Duplicate);
        // Partial duplicates (same address, different port) are also prohibited
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:9997"), NetInfoStatus::Duplicate);
    }
}

BOOST_AUTO_TEST_CASE(netinfo_ser)
{
    {
        // An empty object should only store one byte to denote it is invalid
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        NetInfoEntry entry{};
        ds << entry;
        BOOST_CHECK_EQUAL(ds.size(), sizeof(uint8_t));
    }

    {
        // Reading a nonsense byte should return an empty object
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        NetInfoEntry entry{};
        ds << 0xfe;
        ds >> entry;
        BOOST_CHECK(entry.IsEmpty() && !entry.IsTriviallyValid());
    }

    {
        // Reading an invalid CService should fail trivial validation and return an empty object
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        NetInfoEntry entry{};
        ds << NetInfoEntry::NetInfoType::Service << CService{};
        ds >> entry;
        BOOST_CHECK(entry.IsEmpty() && !entry.IsTriviallyValid());
    }

    {
        // Reading an unrecognized type should fail trivial validation and return an empty object
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        NetInfoEntry entry{};
        ds << NetInfoEntry::NetInfoType::Service << uint256{};
        ds >> entry;
        BOOST_CHECK(entry.IsEmpty() && !entry.IsTriviallyValid());
    }

    {
        // A valid CService should be constructable, readable and pass validation
        CDataStream ds(SER_DISK, CLIENT_VERSION | ADDRV2_FORMAT);
        CService service{LookupNumeric("1.1.1.1", Params().GetDefaultPort())};
        BOOST_CHECK(service.IsValid());
        NetInfoEntry entry{service}, entry2{};
        ds << NetInfoEntry::NetInfoType::Service << service;
        ds >> entry2;
        BOOST_CHECK(entry == entry2);
        BOOST_CHECK(!entry.IsEmpty() && entry.IsTriviallyValid());
        BOOST_CHECK(entry.GetAddrPort().value() == service);
    }

    {
        // NetInfoEntry should be able to read and write ADDRV2 addresses
        CService service{};
        service.SetSpecial("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion");
        BOOST_CHECK(service.IsValid() && service.IsTor());

        CDataStream ds(SER_DISK, CLIENT_VERSION | ADDRV2_FORMAT);
        ds << NetInfoEntry::NetInfoType::Service << service;
        ds.SetVersion(CLIENT_VERSION); // Drop the explicit format flag

        NetInfoEntry entry{};
        ds >> entry;
        BOOST_CHECK(!entry.IsEmpty() && entry.IsTriviallyValid());
        BOOST_CHECK(entry.GetAddrPort().value() == service);
        ds.clear();

        NetInfoEntry entry2{};
        ds << entry;
        ds >> entry2;
        BOOST_CHECK(entry == entry2 && entry2.GetAddrPort().value() == service);
    }
}

BOOST_AUTO_TEST_CASE(netinfo_retvals)
{
    uint16_t p2p_port{Params().GetDefaultPort()};
    CService service{LookupNumeric("1.1.1.1", p2p_port)}, service2{LookupNumeric("1.1.1.2", p2p_port)};
    NetInfoEntry entry{service}, entry2{service2}, entry_empty{};

    // Check that values are correctly recorded and pass trivial validation
    BOOST_CHECK(service.IsValid());
    BOOST_CHECK(!entry.IsEmpty() && entry.IsTriviallyValid());
    BOOST_CHECK(entry.GetAddrPort().value() == service);
    BOOST_CHECK(!entry2.IsEmpty() && entry2.IsTriviallyValid());
    BOOST_CHECK(entry2.GetAddrPort().value() == service2);

    // Check that dispatch returns the expected values
    BOOST_CHECK_EQUAL(entry.GetPort(), service.GetPort());
    BOOST_CHECK_EQUAL(entry.ToString(), strprintf("CService(addr=%s, port=%u)", service.ToStringAddr(), service.GetPort()));
    BOOST_CHECK_EQUAL(entry.ToStringAddr(), service.ToStringAddr());
    BOOST_CHECK_EQUAL(entry.ToStringAddrPort(), service.ToStringAddrPort());
    BOOST_CHECK_EQUAL(service < service2, entry < entry2);

    // Check that empty/invalid entries return error messages
    BOOST_CHECK_EQUAL(entry_empty.GetPort(), 0);
    BOOST_CHECK_EQUAL(entry_empty.ToString(), "[invalid entry]");
    BOOST_CHECK_EQUAL(entry_empty.ToStringAddr(), "[invalid entry]");
    BOOST_CHECK_EQUAL(entry_empty.ToStringAddrPort(), "[invalid entry]");

    // The invalid entry type code is 0xff (highest possible value) and therefore will return as greater
    // in comparison to any valid entry
    BOOST_CHECK(entry < entry_empty);
}

bool CheckIfSerSame(const CService& lhs, const MnNetInfo& rhs)
{
    CHashWriter ss_lhs(SER_GETHASH, 0), ss_rhs(SER_GETHASH, 0);
    ss_lhs << lhs;
    ss_rhs << rhs;
    return ss_lhs.GetSHA256() == ss_rhs.GetSHA256();
}

BOOST_AUTO_TEST_CASE(cservice_compatible)
{
    // Empty values should be the same
    CService service;
    MnNetInfo netInfo;
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Valid IPv4 address, valid port
    service = LookupNumeric("1.1.1.1", 9999);
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:9999"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Valid IPv4 address, default P2P port implied
    service = LookupNumeric("1.1.1.1", Params().GetDefaultPort());
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Lookup() failure (domains not allowed), MnNetInfo should remain empty if Lookup() failed
    service = CService();
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("example.com"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Validation failure (non-IPv4 not allowed), MnNetInfo should remain empty if ValidateService() failed
    service = CService();
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("[2606:4700:4700::1111]:9999"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
}

BOOST_AUTO_TEST_CASE(interface_equality)
{
    // We also check for symmetry as NetInfoInterface, ExtNetInfo, MnNetInfo and NetInfoEntry
    // define their operator!= as the inverse of operator==
    std::shared_ptr<NetInfoInterface> ptr_lhs{nullptr}, ptr_rhs{nullptr};

    // Equal initialization state (uninitialized)
    BOOST_CHECK(util::shared_ptr_equal(ptr_lhs, ptr_rhs) && !util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Unequal initialization state (lhs initialized, rhs unchanged)
    ptr_lhs = std::make_shared<MnNetInfo>();
    BOOST_CHECK(!util::shared_ptr_equal(ptr_lhs, ptr_rhs) && util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Equal initialization state (lhs unchanged, rhs initialized), same values
    ptr_rhs = std::make_shared<MnNetInfo>();
    BOOST_CHECK(ptr_lhs->IsEmpty() && ptr_rhs->IsEmpty());
    BOOST_CHECK(util::shared_ptr_equal(ptr_lhs, ptr_rhs) && !util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Equal initialization state, same type, differing values
    BOOST_CHECK_EQUAL(ptr_rhs->AddEntry("1.1.1.1:9999"), NetInfoStatus::Success);
    BOOST_CHECK(!util::shared_ptr_equal(ptr_lhs, ptr_rhs) && util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Equal initialization state, different type, same values
    ptr_rhs = std::make_shared<ExtNetInfo>();
    BOOST_CHECK(ptr_lhs->IsEmpty() && ptr_rhs->IsEmpty());
    BOOST_CHECK(!util::shared_ptr_equal(ptr_lhs, ptr_rhs) && util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Equal initialization state, same type, same values
    ptr_lhs = std::make_shared<ExtNetInfo>();
    BOOST_CHECK(ptr_lhs->IsEmpty() && ptr_rhs->IsEmpty());
    BOOST_CHECK(util::shared_ptr_equal(ptr_lhs, ptr_rhs) && !util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));

    // Equal initialization state, same type, differing values
    BOOST_CHECK_EQUAL(ptr_rhs->AddEntry("1.1.1.1:9999"), NetInfoStatus::Success);
    BOOST_CHECK(!util::shared_ptr_equal(ptr_lhs, ptr_rhs) && util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));
}

BOOST_AUTO_TEST_SUITE_END()
