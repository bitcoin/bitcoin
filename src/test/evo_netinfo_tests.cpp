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
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_netinfo_tests, BasicTestingSetup)

struct TestEntry {
    std::pair</*purpose=*/NetInfoPurpose, /*addr=*/std::string> input;
    NetInfoStatus expected_ret_mn;
    NetInfoStatus expected_ret_ext;
};

static const std::vector<TestEntry> addr_vals_main{
    // Address and port specified
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, NetInfoStatus::Success, NetInfoStatus::Success},
    // - Port should default to default P2P core with MnNetInfo
    // - Ports are no longer implied with ExtNetInfo
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Non-mainnet port on mainnet causes failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which 9998 isn't
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9998"}, NetInfoStatus::BadPort, NetInfoStatus::Success},
    // Internal addresses not allowed on mainnet
    {{NetInfoPurpose::CORE_P2P, "127.0.0.1:9999"}, NetInfoStatus::NotRoutable, NetInfoStatus::NotRoutable},
    // Valid IPv4 formatting but invalid IPv4 address
    {{NetInfoPurpose::CORE_P2P, "0.0.0.0:9999"}, NetInfoStatus::BadAddress, NetInfoStatus::BadAddress},
    // Port greater than uint16_t max
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:99999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // - Non-IPv4 addresses are prohibited in MnNetInfo
    // - Any valid BIP155 address is allowed in ExtNetInfo
    {{NetInfoPurpose::CORE_P2P, "[2606:4700:4700::1111]:9999"}, NetInfoStatus::BadInput, NetInfoStatus::Success},
    // Domains are not allowed for Core P2P or Platform P2P
    {{NetInfoPurpose::CORE_P2P, "example.com:9999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    {{NetInfoPurpose::PLATFORM_P2P, "example.com:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - ExtNetInfo can store Platform HTTPS addresses *as domains*
    {{NetInfoPurpose::PLATFORM_HTTPS, "example.com:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // Incorrect IPv4 address
    {{NetInfoPurpose::CORE_P2P, "1.1.1.256:9999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Missing address
    {{NetInfoPurpose::CORE_P2P, ":9999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Bad purpose code
    {{static_cast<NetInfoPurpose>(64), "1.1.1.1:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::MaxLimit},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - ExtNetInfo allows storing Platform P2P addresses
    {{NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
};

void ValidateGetEntries(const NetInfoList& entries, const size_t expected_size)
{
    BOOST_CHECK_EQUAL(entries.size(), expected_size);
    for (const NetInfoEntry& entry : entries) {
        BOOST_CHECK(entry.IsTriviallyValid());
    }
}

void TestMnNetInfo(const std::vector<TestEntry>& vals)
{
    for (const auto& [input, expected_ret, _] : vals) {
        const auto& [purpose, addr] = input;
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, addr), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty MnNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }
}

void TestExtNetInfo(const std::vector<TestEntry>& vals)
{
    for (const auto& [input, _, expected_ret] : vals) {
        const auto& [purpose, addr] = input;
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, addr), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty ExtNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }
}

BOOST_AUTO_TEST_CASE(mnnetinfo_rules_main)
{
    TestMnNetInfo(addr_vals_main);

    {
        // MnNetInfo only stores one value, overwriting prohibited
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.2:9999"), NetInfoStatus::MaxLimit);
        BOOST_CHECK(netInfo.HasEntries(NetInfoPurpose::CORE_P2P));
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
    }

    {
        // MnNetInfo only allows storing a Core P2P address
        MnNetInfo netInfo;
        for (const auto purpose : {NetInfoPurpose::PLATFORM_HTTPS, NetInfoPurpose::PLATFORM_P2P}) {
            BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, "1.1.1.1:9999"), NetInfoStatus::MaxLimit);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
        }
        BOOST_CHECK(netInfo.GetEntries().empty());
    }
}

BOOST_AUTO_TEST_CASE(extnetinfo_rules_main) { TestExtNetInfo(addr_vals_main); }

static const std::vector<TestEntry> addr_vals_reg{
    // - MnNetInfo doesn't mind using port 0
    // - ExtNetInfo requires non-zero ports
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:0"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Mainnet P2P port on non-mainnet cause failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which 9999 isn't
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, NetInfoStatus::BadPort, NetInfoStatus::Success},
    // - Non-mainnet P2P port is allowed in MnNetInfo regardless of bad port status
    // - Port 22 (SSH) is below the privileged ports threshold (1023) and is therefore a bad port, disallowed in ExtNetInfo
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:22"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
};

BOOST_FIXTURE_TEST_CASE(mnnetinfo_rules_reg, RegTestingSetup) { TestMnNetInfo(addr_vals_reg); }

BOOST_FIXTURE_TEST_CASE(extnetinfo_rules_reg, RegTestingSetup)
{
    TestExtNetInfo(addr_vals_reg);

    {
        // ExtNetInfo can store up to 4 entries per purpose code, check limit enforcement
        ExtNetInfo netInfo;
        for (size_t idx{1}; idx <= MAX_ENTRIES_EXTNETINFO; idx++) {
            BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.%d:9998", idx)),
                              NetInfoStatus::Success);
        }
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.5:9998"), NetInfoStatus::MaxLimit);
        BOOST_CHECK(netInfo.HasEntries(NetInfoPurpose::CORE_P2P));
        // The limit applies *per purpose code* and therefore wouldn't error if the address was for a different purpose
        BOOST_CHECK(!netInfo.HasEntries(NetInfoPurpose::PLATFORM_P2P));
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_P2P, "1.1.1.5:9998"), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(NetInfoPurpose::PLATFORM_P2P));
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        // GetEntries() is a tally of all entries across all purpose codes
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/MAX_ENTRIES_EXTNETINFO + 1);
    }

    {
        // ExtNetInfo has restrictions on duplicates
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9998"), NetInfoStatus::Success);

        // Exact (i.e. addr:port) duplicates are prohibited *within* a list
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9998"), NetInfoStatus::Duplicate);
        // Partial (i.e. different port) duplicates are prohibited *within* a list
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9997"), NetInfoStatus::Duplicate);

        // Exact (i.e. addr:port) duplicates are prohibited *across* lists
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:9998"), NetInfoStatus::Duplicate);
        // Partial (i.e. different port) duplicates are allowed *across* a list
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:9997"), NetInfoStatus::Success);

        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(NetInfoPurpose::CORE_P2P));
        BOOST_CHECK(netInfo.HasEntries(NetInfoPurpose::PLATFORM_P2P));
        BOOST_CHECK(!netInfo.HasEntries(NetInfoPurpose::PLATFORM_HTTPS));
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/2);
    }

    {
        // ExtNetInfo has additional rules for domains
        const std::vector<TestEntry> domain_vals{
            // Port 80 (HTTP) is below the privileged ports threshold (1023), not allowed
            {{NetInfoPurpose::PLATFORM_HTTPS, "example.com:80"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadPort},
            // Port 443 (HTTPS) is below the privileged ports threshold (1023) but still allowed
            {{NetInfoPurpose::PLATFORM_HTTPS, "example.com:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
            // TLDs must be alphabetic to avoid ambiguation with IP addresses (per ICANN guidelines)
            {{NetInfoPurpose::PLATFORM_HTTPS, "example.123:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
            // .local is a prohibited TLD
            {{NetInfoPurpose::PLATFORM_HTTPS, "somebodys-macbook-pro.local:9998"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
        };
        TestExtNetInfo(domain_vals);
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
    BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Valid IPv4 address, default P2P port implied
    service = LookupNumeric("1.1.1.1", Params().GetDefaultPort());
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Lookup() failure (domains not allowed), MnNetInfo should remain empty if Lookup() failed
    service = CService();
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "example.com"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Validation failure (non-IPv4 not allowed), MnNetInfo should remain empty if ValidateService() failed
    service = CService();
    netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "[2606:4700:4700::1111]:9999"), NetInfoStatus::BadInput);
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
    BOOST_CHECK_EQUAL(ptr_rhs->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);
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
    BOOST_CHECK_EQUAL(ptr_rhs->AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);
    BOOST_CHECK(!util::shared_ptr_equal(ptr_lhs, ptr_rhs) && util::shared_ptr_not_equal(ptr_lhs, ptr_rhs));
}

BOOST_AUTO_TEST_CASE(domainport_rules)
{
    static const std::vector<std::pair</*addr=*/std::string, /*retval=*/DomainPort::Status>> domain_vals{
        // Domain name labels can be as small as one character long and remain valid
        {"r.server-1.ab.cd", DomainPort::Status::Success},
        // Domain names labels can trail with numbers or consist entirely of numbers due to RFC 1123
        {"9998.9example7.ab", DomainPort::Status::Success},
        // dotless domains prohibited
        {"abcd", DomainPort::Status::BadDotless},
        // no empty label (trailing delimiter)
        {"abc.", DomainPort::Status::BadCharPos},
        // no empty label (leading delimiter)
        {".abc", DomainPort::Status::BadCharPos},
        // no empty label (extra delimiters)
        {"a..dot..b", DomainPort::Status::BadLabelLen},
        // ' is not a valid character in domains
        {"somebody's macbook pro.local", DomainPort::Status::BadChar},
        // spaces are not a valid character in domains
        {"somebodys macbook pro.local", DomainPort::Status::BadChar},
        // trailing hyphens are not allowed
        {"-a-.bc.de", DomainPort::Status::BadLabelCharPos},
        // 2 (characters in domain) < 3 (minimum length)
        {"ac", DomainPort::Status::BadLen},
        // 278 (characters in domain) > 253 (maximum limit)
        {"Loremipsumdolorsitametconsecteturadipiscingelitseddoeiusmodtempor"
         "incididuntutlaboreetdoloremagnaaliquaUtenimadminimveniamquisnostrud"
         "exercitationullamcolaborisnisiutaliquipexeacommodoconsequatDuisaute"
         "iruredolorinreprehenderitinvoluptatevelitessecillumdoloreeufugiatnullapariat.ur", DomainPort::Status::BadLen},
        // 64 (characters in label) > 63 (maximum limit)
        {"loremipsumdolorsitametconsecteturadipiscingelitseddoeiusmodtempo.ri.nc", DomainPort::Status::BadLabelLen},
    };

    for (const auto& [addr, retval] : domain_vals) {
        DomainPort domain;
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(domain.Set(addr, 443), retval);
        if (retval != DomainPort::Status::Success) {
            BOOST_CHECK_EQUAL(domain.Validate(), DomainPort::Status::Malformed); // Empty values report as Malformed
            BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_HTTPS, domain.ToStringAddrPort()),
                              NetInfoStatus::BadInput);
        } else {
            BOOST_CHECK_EQUAL(domain.Validate(), DomainPort::Status::Success);
            BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_HTTPS, domain.ToStringAddrPort()), NetInfoStatus::Success);
        }
    }

    {
        // DomainPort requires non-zero ports
        DomainPort domain;
        BOOST_CHECK_EQUAL(domain.Set("example.com", 0), DomainPort::Status::BadPort);
        BOOST_CHECK_EQUAL(domain.Validate(), DomainPort::Status::Malformed);
    }

    {
        // DomainPort stores the domain in lower-case
        DomainPort lhs, rhs;
        BOOST_CHECK_EQUAL(lhs.Set("example.com", 9999), DomainPort::Status::Success);
        BOOST_CHECK_EQUAL(rhs.Set(ToUpper("example.com"), 9999), DomainPort::Status::Success);
        BOOST_CHECK_EQUAL(lhs.ToStringAddr(), rhs.ToStringAddr());
        BOOST_CHECK(lhs == rhs);
    }
}

BOOST_AUTO_TEST_SUITE_END()
