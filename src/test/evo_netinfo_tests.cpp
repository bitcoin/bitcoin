// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/types.h>
#include <interfaces/node.h>
#include <util/helpers.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <clientversion.h>
#include <netbase.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_netinfo_tests, BasicTestingSetup)

struct TestEntry {
    std::pair</*purpose=*/NetInfoPurpose, /*addr=*/std::string> input;
    NetInfoStatus expected_ret_mn{NetInfoStatus::BadInput};
    NetInfoStatus expected_ret_ext{NetInfoStatus::BadInput};
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
    // Internal and private addresses not allowed on mainnet
    {{NetInfoPurpose::CORE_P2P, "127.0.0.1:9999"}, NetInfoStatus::NotRoutable, NetInfoStatus::NotRoutable},
    {{NetInfoPurpose::CORE_P2P, "10.0.0.1:9999"}, NetInfoStatus::NotRoutable, NetInfoStatus::NotRoutable},
    // Valid IPv4 formatting but invalid IPv4 address
    {{NetInfoPurpose::CORE_P2P, "0.0.0.0:9999"}, NetInfoStatus::BadAddress, NetInfoStatus::BadAddress},
    // Port greater than uint16_t max
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:99999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // - Non-IPv4 addresses are prohibited in MnNetInfo
    // - Any valid BIP155 address is allowed in ExtNetInfo
    {{NetInfoPurpose::CORE_P2P, "[2606:4700:4700::1111]:9999"}, NetInfoStatus::BadInput, NetInfoStatus::Success},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - Privacy network domains are allowed in ExtNetInfo but internet domains are not
    {{NetInfoPurpose::CORE_P2P, "example.com:9999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    {{NetInfoPurpose::CORE_P2P, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:9999"}, NetInfoStatus::BadInput, NetInfoStatus::Success},
    {{NetInfoPurpose::PLATFORM_P2P, "example.com:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
    {{NetInfoPurpose::PLATFORM_P2P, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - ExtNetInfo can store Platform HTTPS addresses *as domains* alongside privacy network domains
    {{NetInfoPurpose::PLATFORM_HTTPS, "example.com:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    {{NetInfoPurpose::PLATFORM_HTTPS, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:9999"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // - Port 443 (HTTPS) is below the privileged ports threshold (1023) but allowed for Platform HTTPS
    //   addresses, whether stored as an address or as a domain
    // - Other privileged ports remain disallowed for Platform HTTPS, as does 443 for every other purpose
    {{NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    {{NetInfoPurpose::PLATFORM_HTTPS, "[2606:4700:4700::1111]:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    {{NetInfoPurpose::PLATFORM_HTTPS, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    {{NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:80"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadPort},
    {{NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:443"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadPort},
    {{NetInfoPurpose::CORE_P2P, "1.1.1.1:443"}, NetInfoStatus::BadPort, NetInfoStatus::BadPort},
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
    // Mainnet has fixed rules for the first entry of each purpose (see provider_network_fields_main), so
    // there the table checks the rules for additional entries, added behind valid primaries
    const bool add_primaries{IsNodeOnMainnet()};
    const size_t primaries_count{add_primaries ? 3U : 0U};
    for (const auto& [input, _, expected_ret] : vals) {
        const auto& [purpose, addr] = input;
        ExtNetInfo netInfo;
        if (add_primaries) {
            for (const auto& [primary_purpose, primary] : {std::pair{NetInfoPurpose::CORE_P2P, "1.2.3.4:9999"},
                                                           std::pair{NetInfoPurpose::PLATFORM_P2P, "1.2.3.4:26656"},
                                                           std::pair{NetInfoPurpose::PLATFORM_HTTPS, "1.2.3.4:443"}}) {
                BOOST_REQUIRE_EQUAL(netInfo.AddEntry(primary_purpose, primary), NetInfoStatus::Success);
            }
        }
        BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, addr), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty ExtNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), add_primaries ? NetInfoStatus::Success : NetInfoStatus::Malformed);
            BOOST_CHECK_EQUAL(netInfo.HasEntries(purpose), add_primaries && IsValidPurpose(purpose));
            BOOST_CHECK_EQUAL(netInfo.GetEntries().size(), primaries_count);
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/primaries_count + 1);
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

BOOST_AUTO_TEST_CASE(netinfo_rules_main_allowprivatenet)
{
    // -allowprivatenet widens CNetAddr::IsRoutable() to RFC1918 addresses; a node-local option
    // must not change which masternode addresses pass consensus validation
    fAllowPrivateNet = true;
    TestMnNetInfo(addr_vals_main);
    TestExtNetInfo(addr_vals_main);
    fAllowPrivateNet = DEFAULT_ALLOWPRIVATENET;
}

BOOST_AUTO_TEST_CASE(provider_network_fields_main)
{
    auto node{interfaces::MakeNode(m_node)};
    interfaces::ProviderNetInfo net_info{
        .core_p2p = {strprintf("1.1.1.1:%d", MainParams().GetDefaultPort())},
        .platform_p2p = MainParams().GetDefaultPlatformP2PPort(),
        .platform_https = MainParams().GetDefaultPlatformHTTPPort(),
    };
    BOOST_CHECK(!node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                     /*ipv4_optional=*/false, /*optional=*/false));

    net_info.platform_p2p = static_cast<uint16_t>(MainParams().GetDefaultPlatformP2PPort() + 1);
    auto validation_error{
        node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                            /*ipv4_optional=*/false, /*optional=*/false)};
    BOOST_REQUIRE(validation_error);
    BOOST_CHECK_EQUAL(validation_error->reject_reason, "bad-protx-platform-p2p-port");

    net_info.platform_p2p = MainParams().GetDefaultPlatformP2PPort();
    net_info.platform_https = static_cast<uint16_t>(MainParams().GetDefaultPlatformHTTPPort() + 1);
    validation_error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                           /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(validation_error);
    BOOST_CHECK_EQUAL(validation_error->reject_reason, "bad-protx-platform-http-port");

    net_info.platform_p2p = MainParams().GetDefaultPort();
    net_info.platform_https = MainParams().GetDefaultPlatformHTTPPort();
    validation_error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                           /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(validation_error);
    BOOST_CHECK_EQUAL(validation_error->reject_reason, "bad-protx-platform-p2p-port");

    net_info = {
        .core_p2p = {"127.0.0.1:9999"},
        .platform_p2p = std::vector<std::string>{"1.1.1.2:22200"},
        .platform_https = std::vector<std::string>{"server.example.com:443"},
    };
    validation_error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                           /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(validation_error);
    BOOST_CHECK(validation_error->message.original.find("unroutable address") != std::string::npos);

    // On mainnet the first entry of every purpose must use that purpose's port (9999, 26656, 443) and, as
    // masternodes may not operate without IPv4 there yet, be IPv4, with the first Platform entries on the first
    // Core P2P address. Additional entries may use other addresses, ports and network types. Shared masternodes
    // use the same network fields.
    const std::string a{"1.1.1.1"}, b{"1.1.1.2"}, ipv6{"[2606:4700:4700::1111]"},
        onion{"pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"},
        i2p{"udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p:0"}, domain{"platform.example.com"};
    const auto make = [](std::vector<std::string> core, interfaces::ProviderPlatformEndpoints p2p = {},
                         interfaces::ProviderPlatformEndpoints https = {}) {
        return interfaces::ProviderNetInfo{.core_p2p = std::move(core), .platform_p2p = std::move(p2p),
                                           .platform_https = std::move(https)};
    };
    using Strs = std::vector<std::string>;
    struct Case {
        MnType type{MnType::Regular};
        interfaces::ProviderNetInfo net_info;
        std::string error_field; // Empty if valid, unless reject_reason is set
        std::string error_status{};
        std::string reject_reason{}; // Checked instead of the message if set
    };
    const std::vector<Case> primary_vals{
        // Valid, including additional entries of any type, address and port
        {MnType::Regular, make({a + ":9999"}), ""},
        {MnType::Regular, make({a + ":9999", b + ":9998", ipv6 + ":9999", onion + ":9999"}), ""},
        {MnType::Regular, make({a + ":9999", i2p}), ""},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{a + ":443"}), ""},
        {MnType::Evo,
         make({a + ":9999", ipv6 + ":9999"}, Strs{a + ":26656", b + ":26657", ipv6 + ":26656", onion + ":26656"},
              Strs{a + ":443", domain + ":443", ipv6 + ":8443"}),
         ""},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{a + ":443", ipv6 + ":443", onion + ":443"}), ""},
        // Additional addresses may use the standard ports too, e.g. a second IPv4 address on the same ports
        {MnType::Regular, make({a + ":9999", b + ":9999"}), ""},
        {MnType::Regular, make({a + ":9999", b + ":26656"}), ""},
        {MnType::Evo,
         make({a + ":9999", b + ":9999"}, Strs{a + ":26656", b + ":26656"}, Strs{a + ":443", b + ":443"}), ""},
        // A bare Platform port is placed on the first Core P2P address
        {MnType::Evo, make({a + ":9999"}, uint16_t{26656}, uint16_t{443}), ""},
        {MnType::Evo, make({a + ":9999"}, uint16_t{26657}, uint16_t{443}), "platformP2PAddrs[0]", "invalid port"},
        // The first entry of each purpose must use that purpose's port
        {MnType::Regular, make({a + ":9998"}), "coreP2PAddrs[0]", "invalid port"},
        {MnType::Regular, make({i2p}), "coreP2PAddrs[0]", "invalid port"},
        {MnType::Evo, make({a + ":9998"}, Strs{a + ":26656"}, Strs{a + ":443"}), "coreP2PAddrs[0]", "invalid port"},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26657"}, Strs{a + ":443"}), "platformP2PAddrs[0]", "invalid port"},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{a + ":8443"}), "platformHTTPSAddrs[0]",
         "invalid port"},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{domain + ":8443", a + ":443"}),
         "platformHTTPSAddrs[0]", "invalid port"},
        // The first entry of each purpose must be IPv4, Platform's on the first Core P2P address
        {MnType::Regular, make({ipv6 + ":9999"}), "", "", "bad-protx-netinfo-addr-type"},
        {MnType::Regular, make({onion + ":9999", a + ":9999"}), "", "", "bad-protx-netinfo-addr-type"},
        {MnType::Evo, make({a + ":9999"}, Strs{b + ":26656"}, Strs{a + ":443"}), "", "", "bad-protx-netinfo-addr"},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{b + ":443"}), "", "", "bad-protx-netinfo-addr"},
        {MnType::Evo, make({a + ":9999", ipv6 + ":9999"}, Strs{ipv6 + ":26656"}, Strs{a + ":443"}), "", "",
         "bad-protx-netinfo-addr-type"},
        {MnType::Evo, make({a + ":9999"}, Strs{a + ":26656"}, Strs{domain + ":443"}), "", "",
         "bad-protx-netinfo-addr-type"},
    };
    for (const auto& [type, case_net_info, error_field, error_status, reject_reason] : primary_vals) {
        validation_error = node->evo().validateProviderNetInfo(case_net_info, type, ProTxVersion::ExtAddr,
                                                               /*ipv4_optional=*/false, /*optional=*/false);
        BOOST_CHECK_MESSAGE(!validation_error == (error_field.empty() && reject_reason.empty()),
                            strprintf("%s: %s", Join(case_net_info.core_p2p, ","),
                                      validation_error ? validation_error->message.original : "accepted"));
        if (validation_error && !reject_reason.empty()) {
            BOOST_CHECK_MESSAGE(validation_error->reject_reason == reject_reason, validation_error->message.original);
        } else if (validation_error && !error_field.empty()) {
            const auto& message{validation_error->message.original};
            BOOST_CHECK_MESSAGE(message.find(error_field) != std::string::npos &&
                                    message.find(strprintf("(%s)", error_status)) != std::string::npos,
                                message);
        }
    }

    // Validation needs no chain: the caller supplies whether masternodes may operate without IPv4, which lifts
    // the IPv4 and same-address rules but not the port pins
    for (const auto& [type, case_net_info, _field, _status, reject_reason] : primary_vals) {
        if (reject_reason.empty()) continue;
        BOOST_CHECK(!node->evo().validateProviderNetInfo(case_net_info, type, ProTxVersion::ExtAddr,
                                                         /*ipv4_optional=*/true, /*optional=*/false));
    }
    validation_error = node->evo().validateProviderNetInfo(make({ipv6 + ":9998"}), MnType::Regular,
                                                           ProTxVersion::ExtAddr, /*ipv4_optional=*/true,
                                                           /*optional=*/false);
    BOOST_REQUIRE(validation_error);
    BOOST_CHECK(validation_error->message.original.find("(invalid port)") != std::string::npos);
}

struct TestNetSetup : public BasicTestingSetup {
    TestNetSetup() : BasicTestingSetup{CBaseChainParams::TESTNET} {}
};

BOOST_FIXTURE_TEST_CASE(extnetinfo_ports_mainnet_only, TestNetSetup)
{
    struct Case {
        MnType type{MnType::Regular};
        std::vector<std::pair<NetInfoPurpose, std::string>> entries;
        bool valid_on_mainnet{false};
    };
    const std::string ipv6{"[2606:4700:4700::1111]"};
    const std::vector<Case> vals{
        {MnType::Regular, {{NetInfoPurpose::CORE_P2P, "1.1.1.1:19999"}}},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:22000"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"}}},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:8443"}}},
        // Satisfies the mainnet port rules, with additional entries on any port, including the standard ones
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::CORE_P2P, ipv6 + ":19999"},
          {NetInfoPurpose::CORE_P2P, "1.1.1.2:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:26656"},
          {NetInfoPurpose::PLATFORM_P2P, "1.1.1.2:22000"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.3:26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"}, {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.2:443"},
          {NetInfoPurpose::PLATFORM_HTTPS, "platform.example.com:443"}},
         /*valid_on_mainnet=*/true},
    };
    const auto check_fields = [](const std::shared_ptr<ExtNetInfo>& net_info, MnType type, TxValidationState& state) {
        return CheckProviderNetworkFields(net_info, type, ProTxVersion::ExtAddr, /*platform_node_id=*/nullptr,
                                          /*platform_p2p_port=*/0, /*platform_http_port=*/0, /*allow_empty=*/false,
                                          /*ipv4_optional=*/false, state);
    };

    // Outside mainnet, these port rules don't apply
    std::vector<CDataStream> serialized;
    for (const auto& [type, entries, _] : vals) {
        ExtNetInfo net_info;
        for (const auto& [purpose, entry] : entries) {
            BOOST_CHECK_EQUAL(net_info.AddEntry(purpose, entry), NetInfoStatus::Success);
        }
        BOOST_CHECK_EQUAL(net_info.Validate(), NetInfoStatus::Success);
        TxValidationState state;
        BOOST_CHECK(check_fields(std::make_shared<ExtNetInfo>(net_info), type, state));
        serialized.emplace_back(SER_DISK, CLIENT_VERSION) << net_info;
    }

    // A serialized ProTx carries its entries as-is, so consensus on mainnet must apply the rules to the
    // same objects even though they never passed through AddEntry() there
    SelectParams(CBaseChainParams::MAIN);
    for (size_t idx{0}; idx < vals.size(); ++idx) {
        const auto& [type, _, valid_on_mainnet] = vals[idx];
        const auto net_info{std::make_shared<ExtNetInfo>(deserialize, serialized[idx])};
        BOOST_CHECK_EQUAL(net_info->Validate(), valid_on_mainnet ? NetInfoStatus::Success : NetInfoStatus::BadPort);
        TxValidationState state;
        BOOST_CHECK_EQUAL(check_fields(net_info, type, state), valid_on_mainnet);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), valid_on_mainnet ? "" : "bad-protx-netinfo-port");
    }
    SelectParams(CBaseChainParams::TESTNET);
}

BOOST_FIXTURE_TEST_CASE(ipv4_primaries_follow_activation, TestNetSetup)
{
    // Masternodes may operate without IPv4 on testnet (and devnets), which already has such registrations, but
    // not yet on mainnet
    BOOST_CHECK(DeploymentActiveAfter(nullptr, Params().GetConsensus(), Consensus::DEPLOYMENT_IPV4_OPTIONAL));
    BOOST_CHECK(!DeploymentEnabled(MainParams().GetConsensus(), Consensus::DEPLOYMENT_IPV4_OPTIONAL));

    struct Case {
        MnType type{MnType::Regular};
        std::vector<std::pair<NetInfoPurpose, std::string>> entries;
        std::string reject_reason{}; // Before activation, empty if valid
    };
    const std::string ipv6{"[2606:4700:4700::1111]"};
    const std::string onion{"pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"};
    const std::vector<Case> vals{
        {MnType::Regular, {{NetInfoPurpose::CORE_P2P, ipv6 + ":9999"}, {NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}},
         "bad-protx-netinfo-addr-type"},
        {MnType::Regular, {{NetInfoPurpose::CORE_P2P, onion + ":9999"}}, "bad-protx-netinfo-addr-type"},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.2:26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"}},
         "bad-protx-netinfo-addr"},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.2:443"}},
         "bad-protx-netinfo-addr"},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::CORE_P2P, ipv6 + ":9999"},
          {NetInfoPurpose::PLATFORM_P2P, ipv6 + ":26656"}, {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"}},
         "bad-protx-netinfo-addr-type"},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "platform.example.com:443"}},
         "bad-protx-netinfo-addr-type"},
        {MnType::Evo,
         {{NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"}, {NetInfoPurpose::CORE_P2P, ipv6 + ":9999"},
          {NetInfoPurpose::PLATFORM_P2P, "1.1.1.1:26656"}, {NetInfoPurpose::PLATFORM_P2P, onion + ":26656"},
          {NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"},
          {NetInfoPurpose::PLATFORM_HTTPS, "platform.example.com:443"}}},
    };
    const auto check = [](const std::shared_ptr<ExtNetInfo>& net_info, const Case& val) {
        BOOST_CHECK_EQUAL(net_info->Validate(), NetInfoStatus::Success);
        TxValidationState state;
        BOOST_CHECK(CheckProviderNetworkFields(net_info, val.type, ProTxVersion::ExtAddr, nullptr, 0, 0,
                                               /*allow_empty=*/false, /*ipv4_optional=*/true, state));
        BOOST_CHECK_EQUAL(CheckProviderNetworkFields(net_info, val.type, ProTxVersion::ExtAddr, nullptr, 0, 0,
                                                     /*allow_empty=*/false, /*ipv4_optional=*/false, state),
                          val.reject_reason.empty());
        BOOST_CHECK_EQUAL(state.GetRejectReason(), val.reject_reason);
    };

    std::vector<CDataStream> serialized;
    for (const auto& val : vals) {
        auto net_info{std::make_shared<ExtNetInfo>()};
        for (const auto& [purpose, entry] : val.entries) {
            BOOST_CHECK_EQUAL(net_info->AddEntry(purpose, entry), NetInfoStatus::Success);
        }
        check(net_info, val);
        serialized.emplace_back(SER_DISK, CLIENT_VERSION) << *net_info;
    }

    // The rule depends on the activation, not the network. Validate(), which also checks stored masternode states
    // without a height, doesn't apply it.
    SelectParams(CBaseChainParams::MAIN);
    for (size_t idx{0}; idx < vals.size(); ++idx) {
        check(std::make_shared<ExtNetInfo>(deserialize, serialized[idx]), vals[idx]);
    }
    SelectParams(CBaseChainParams::TESTNET);
}

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

enum class ExpectedType : uint8_t {
    CJDNS,
    I2P,
    Tor,
};

static const std::vector<std::tuple</*type=*/ExpectedType, /*input=*/std::string, /*expected_ret=*/NetInfoStatus>> privacy_addr_vals{
    {ExpectedType::CJDNS, "[fc00:3344:5566:7788:9900:aabb:ccdd:eeff]:9998", NetInfoStatus::Success},
    // ExtNetInfo can store I2P addresses as long as it uses port 0
    {ExpectedType::I2P, "udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p:0", NetInfoStatus::Success},
    // ExtNetInfo can store onion addresses
    {ExpectedType::Tor, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:9998", NetInfoStatus::Success},
    // ExtNetInfo can store I2P addresses but non-zero ports are not allowed
    {ExpectedType::I2P, "udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p:9998", NetInfoStatus::BadPort},
    // ExtNetInfo can store onion addresses but zero ports are not allowed
    {ExpectedType::Tor, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:0", NetInfoStatus::BadPort},
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
            // DomainPort isn't used for storing privacy network TLDs like .onion
            {{NetInfoPurpose::PLATFORM_HTTPS, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd:9998"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
        };
        TestExtNetInfo(domain_vals);
    }

    // Privacy network entry checks
    for (const auto& [type, input, expected_ret] : privacy_addr_vals) {
        const bool expected_success{expected_ret == NetInfoStatus::Success};

        ExtNetInfo netInfo{};
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, input), expected_ret);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/expected_success ? 1 : 0);
        if (!expected_success) continue;

        // Type registration check
        const CService service{netInfo.GetEntries().at(0).GetAddrPort().value()};
        BOOST_CHECK(service.IsValid());
        switch (type) {
        case ExpectedType::CJDNS:
            BOOST_CHECK(service.IsCJDNS());
            break;
        case ExpectedType::I2P:
            BOOST_CHECK(service.IsI2P());
            break;
        case ExpectedType::Tor:
            BOOST_CHECK(service.IsTor());
            break;
        } // no default case, so the compiler can warn about missing cases
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

    {
        // GetEntries() order does not depend on whether the object was built or deserialized
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::PLATFORM_HTTPS, "1.1.1.1:443"), NetInfoStatus::Success);
        BOOST_CHECK_EQUAL(netInfo.AddEntry(NetInfoPurpose::CORE_P2P, "1.1.1.1:9999"), NetInfoStatus::Success);

        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << netInfo;
        ExtNetInfo netInfo2;
        ds >> netInfo2;
        BOOST_CHECK(netInfo == netInfo2);
        BOOST_CHECK(netInfo.GetEntries() == netInfo2.GetEntries());
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

// DomainPort used a bare std::string unserialize path (bounded only by
// MAX_SIZE == 32 MiB) and NetInfoEntry::Unserialize swallowed ios_base::failure.
// That combination allowed a tiny ProTx ExtNetInfo payload to force repeated
// allocate-and-zero of multi-megabyte strings. These tests pin the serialization
// layer to reject oversized domain claims without performing the large allocation.
static CDataStream MakeOversizedDomainPortStream(const size_t claimed_len)
{
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    WriteCompactSize(ds, claimed_len);
    // Provide a full body so pre-fix code can complete the string read; the
    // bound must reject based on the CompactSize alone, not on a short read.
    const std::string body(claimed_len, 'a');
    if (!body.empty()) {
        ds.write(MakeByteSpan(body));
    }
    ser_writedata16be(ds, 443);
    return ds;
}

BOOST_AUTO_TEST_CASE(domainport_deser_rejects_oversized_string)
{
    // DOMAIN_MAX_LEN is 253; a fully-formed 1000-byte domain must be rejected at
    // deserialization time (LIMITED_STRING), not accepted then failed later in
    // ValidateDomain.
    constexpr size_t kOversized{1000};
    CDataStream ds{MakeOversizedDomainPortStream(kOversized)};

    DomainPort domain;
    BOOST_CHECK_THROW(ds >> domain, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(netinfoentry_domain_deser_rejects_oversized_string)
{
    // NetInfoEntry must not swallow the length-limit failure: rethrow so the
    // enclosing ProTx payload deserialization aborts and the peer is penalised.
    constexpr size_t kOversized{1000};
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << uint8_t{NetInfoEntry::NetInfoType::Domain};
    {
        CDataStream body{MakeOversizedDomainPortStream(kOversized)};
        ds.write(MakeByteSpan(body));
    }

    NetInfoEntry entry;
    BOOST_CHECK_THROW(ds >> entry, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(domainport_deser_rejects_huge_claimed_length)
{
    // CompactSize claims 1 MiB with no body. Pre-fix this would allocate 1 MiB
    // then throw on short-read; post-fix LIMITED_STRING rejects before resize.
    // Either way the operation must throw; the important property is that a
    // declared length above DOMAIN_MAX_LEN never produces a successful DomainPort.
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    WriteCompactSize(ds, size_t{1} << 20);

    DomainPort domain;
    BOOST_CHECK_THROW(ds >> domain, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(extnetinfo_domain_amplification_rejected)
{
    // Simulate the attack shape: ExtNetInfo vector of Domain entries each
    // claiming a large string length from a small stream. Pre-fix,
    // NetInfoEntry swallows the short-read and the vector loop continues
    // (stream position does not advance on the failed body read), producing
    // repeated large allocations. Post-fix the first oversized claim throws
    // and aborts the whole ExtNetInfo unserialize.
    constexpr size_t kClaimed{1 << 16}; // 64 KiB — large enough to prove, small enough to not OOM pre-fix
    constexpr size_t kEntries{8};

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << uint8_t{1}; // ExtNetInfo version
    WriteCompactSize(ds, 1); // one purpose
    ds << NetInfoPurpose::PLATFORM_HTTPS;
    WriteCompactSize(ds, kEntries);
    for (size_t i = 0; i < kEntries; ++i) {
        ds << uint8_t{NetInfoEntry::NetInfoType::Domain};
        // Claim kClaimed bytes but supply none — CompactSize only.
        WriteCompactSize(ds, kClaimed);
    }

    ExtNetInfo netInfo;
    BOOST_CHECK_THROW(ds >> netInfo, std::ios_base::failure);
}

BOOST_FIXTURE_TEST_CASE(extnetinfo_validate_deser, RegTestingSetup)
{
    // The in-memory builder (AddEntry/ProcessCandidate) refuses duplicates, domains
    // on non-HTTPS purposes, and lists exceeding MAX_ENTRIES_EXTNETINFO. None of those
    // checks run on deserialization, so Validate() must catch them itself.
    const uint16_t port{9998};

    auto write_header = [](CDataStream& ds, size_t n_purposes) {
        ds << uint8_t{1}; // current version
        WriteCompactSize(ds, n_purposes);
    };

    {
        // Two identical entries under the same purpose
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 1);
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, 2);
        const NetInfoEntry entry{LookupNumeric("1.1.1.1", port)};
        BOOST_CHECK(entry.IsTriviallyValid());
        ds << entry << entry;

        ExtNetInfo netInfo;
        ds >> netInfo;
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Duplicate);
    }

    {
        // List exceeds MAX_ENTRIES_EXTNETINFO per purpose
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 1);
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, MAX_ENTRIES_EXTNETINFO + 1);
        for (size_t i = 1; i <= MAX_ENTRIES_EXTNETINFO + 1; ++i) {
            const NetInfoEntry entry{LookupNumeric(strprintf("1.1.1.%d", i), port)};
            BOOST_CHECK(entry.IsTriviallyValid());
            ds << entry;
        }

        ExtNetInfo netInfo;
        ds >> netInfo;
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::MaxLimit);
    }

    {
        // Domain entry under a non-PLATFORM_HTTPS purpose
        DomainPort domain;
        BOOST_CHECK_EQUAL(domain.Set("example.com", 443), DomainPort::Status::Success);
        const NetInfoEntry entry{domain};
        BOOST_CHECK(entry.IsTriviallyValid());

        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 1);
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, 1);
        ds << entry;

        ExtNetInfo netInfo;
        ds >> netInfo;
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::BadInput);
    }

    {
        // A repeated purpose is rejected at deserialization, even if its entries would
        // otherwise be silently cleared instead of failing
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 2);
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, 1);
        ds << NetInfoEntry{LookupNumeric("1.1.1.1", port)};
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, 1);
        ds << uint8_t{0x03}; // unknown NetInfoType

        ExtNetInfo netInfo;
        BOOST_CHECK_THROW(ds >> netInfo, std::ios_base::failure);
    }

    // Purposes must be ascending and known, so that one object has exactly one encoding
    for (const auto& purposes : std::vector<std::vector<NetInfoPurpose>>{
             {NetInfoPurpose::PLATFORM_HTTPS, NetInfoPurpose::CORE_P2P},
             {static_cast<NetInfoPurpose>(3)},
         }) {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, purposes.size());
        for (const auto purpose : purposes) {
            ds << purpose;
            WriteCompactSize(ds, 1);
            ds << NetInfoEntry{LookupNumeric("1.1.1.1", port)};
        }

        ExtNetInfo netInfo;
        BOOST_CHECK_THROW(ds >> netInfo, std::ios_base::failure);
    }

    // A cleared entry under a known purpose decodes but must not validate
    const auto with_entry = [&](auto&& write_entry) {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 1);
        ds << NetInfoPurpose::CORE_P2P;
        WriteCompactSize(ds, 1);
        write_entry(ds);
        return ds;
    };
    for (auto ds : {
             with_entry([](CDataStream& s) { s << uint8_t{0x03}; }),                                                  // unknown type
             with_entry([](CDataStream& s) { s << NetInfoEntry::NetInfoType::Invalid; }),                             // explicit invalid marker
             with_entry([](CDataStream& s) {
                 s << NetInfoEntry::NetInfoType::Service;
                 OverrideStream<CDataStream> s_v2(&s, /*nType=*/0, s.GetVersion() | ADDRV2_FORMAT);
                 s_v2 << CService{};
             }), // unspecified address
             with_entry([](CDataStream& s) { s << NetInfoEntry::NetInfoType::Service << uint8_t{2} << uint8_t{16}; }), // truncated service
         }) {
        ExtNetInfo netInfo;
        ds >> netInfo;
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
    }

    // Port 443 is only permitted for Platform HTTPS addresses
    for (const auto& [purpose, expected] : std::vector<std::pair<NetInfoPurpose, NetInfoStatus>>{
             {NetInfoPurpose::PLATFORM_HTTPS, NetInfoStatus::Success},
             {NetInfoPurpose::PLATFORM_P2P, NetInfoStatus::BadPort},
             {NetInfoPurpose::CORE_P2P, NetInfoStatus::BadPort},
         }) {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        write_header(ds, 1);
        ds << purpose;
        WriteCompactSize(ds, 1);
        ds << NetInfoEntry{LookupNumeric("1.1.1.1", 443)};

        ExtNetInfo netInfo;
        ds >> netInfo;
        BOOST_CHECK_EQUAL(netInfo.Validate(), expected);
    }
}

BOOST_AUTO_TEST_CASE(domain_port_wire_compatibility)
{
    DomainPort domain;
    BOOST_REQUIRE_EQUAL(domain.Set("example.com", 443), DomainPort::Status::Success);

    CDataStream encoded{SER_NETWORK, CLIENT_VERSION};
    encoded << domain;
    CDataStream expected{SER_NETWORK, CLIENT_VERSION};
    expected << std::string{"example.com"} << Using<BigEndianFormatter<2>>(uint16_t{443});
    BOOST_CHECK_EQUAL_COLLECTIONS(encoded.begin(), encoded.end(), expected.begin(), expected.end());

    CDataStream oversized{SER_NETWORK, CLIENT_VERSION};
    oversized << NetInfoEntry::NetInfoType::Domain;
    constexpr size_t MAX_DOMAIN_LENGTH{253};
    WriteCompactSize(oversized, MAX_DOMAIN_LENGTH + 1);
    const std::string oversized_addr(MAX_DOMAIN_LENGTH + 1, 'a');
    oversized.write(MakeByteSpan(oversized_addr));
    oversized << Using<BigEndianFormatter<2>>(uint16_t{443});

    NetInfoEntry entry;
    BOOST_CHECK_EXCEPTION(oversized >> entry, std::ios_base::failure,
                          [](const auto& e) { return std::string{e.what()}.find("String length limit exceeded") != std::string::npos; });
}

BOOST_AUTO_TEST_SUITE_END()
