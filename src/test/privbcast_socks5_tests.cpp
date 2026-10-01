// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <privbcast/socks5.h>
#include <random.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using privbcast::Socks5Client;
using privbcast::Socks5Credentials;
using Command = Socks5Client::Command;

namespace {

/** Write everything the client offers, and return it. */
std::vector<uint8_t> WriteAll(Socks5Client& client)
{
    const std::span<const uint8_t> bytes{client.BytesToSend()};
    std::vector<uint8_t> written{bytes.begin(), bytes.end()};
    client.MarkSent(written.size());
    return written;
}

/** Take the client through the greeting and the authentication, and return the request it writes. */
std::vector<uint8_t> Request(Socks5Client& client)
{
    BOOST_CHECK(WriteAll(client) == ParseHex("050102"));
    BOOST_CHECK_EQUAL(client.Received(ParseHex("0502")), 2U);
    std::vector<uint8_t> credentials{0x01, static_cast<uint8_t>(client.Username().size())};
    credentials.insert(credentials.end(), client.Username().begin(), client.Username().end());
    credentials.push_back(static_cast<uint8_t>(client.Password().size()));
    credentials.insert(credentials.end(), client.Password().begin(), client.Password().end());
    BOOST_CHECK(WriteAll(client) == credentials);
    BOOST_CHECK_EQUAL(client.Received(ParseHex("0100")), 2U);
    return WriteAll(client);
}

std::vector<uint8_t> Cat(std::vector<uint8_t> a, std::span<const uint8_t> b, std::span<const uint8_t> c = {})
{
    a.insert(a.end(), b.begin(), b.end());
    a.insert(a.end(), c.begin(), c.end());
    return a;
}

std::vector<uint8_t> Bytes(const std::string& text) { return {text.begin(), text.end()}; }

} // namespace

BOOST_AUTO_TEST_SUITE(privbcast_socks5_tests)

// RFC 1928 section 4: a numeric IPv4 or IPv6 address is sent as one, any other host as a name,
// never looked up (B1), and the port in network order. A host SOCKS5 cannot carry fails the
// exchange before anything is sent.
BOOST_AUTO_TEST_CASE(request)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    const std::string name(255, 'a');
    const std::vector<std::pair<Socks5Client, std::vector<uint8_t>>> cases{
        {Socks5Client{Command::Connect, "1.2.3.4", 8333, rng}, ParseHex("0501000101020304208d")},
        {Socks5Client{Command::Resolve, "1.2.3.4", 8333, rng}, ParseHex("05f0000101020304208d")},
        {Socks5Client{Command::Connect, "2001:db8::1", 8333, rng}, ParseHex("0501000420010db8000000000000000000000001208d")},
        {Socks5Client{Command::Connect, "1.2.3.256", 8333, rng}, Cat(ParseHex("0501000309"), Bytes("1.2.3.256"), ParseHex("208d"))},
        {Socks5Client{Command::Resolve, "seed.example", 0, rng}, Cat(ParseHex("05f000030c"), Bytes("seed.example"), ParseHex("0000"))},
        {Socks5Client{Command::Resolve, name, 53, rng}, Cat(ParseHex("05f00003ff"), Bytes(name), ParseHex("0035"))},
    };
    for (auto [client, request] : cases) {
        BOOST_CHECK(Request(client) == request);
        BOOST_CHECK(client.BytesToSend().empty() && !client.Done() && !client.Failed());
    }
    for (const std::string& host : {std::string{}, std::string(256, 'a'), std::string{"a\0b", 3}}) {
        Socks5Client client{Command::Connect, host, 8333, rng};
        BOOST_CHECK(client.Failed() && client.BytesToSend().empty());
    }
}

// B2: a client presents the credentials it was given, or by default fresh ones of its own.
BOOST_AUTO_TEST_CASE(credentials)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    const Socks5Credentials given{Socks5Credentials::Draw(rng)};
    Socks5Client a{Command::Resolve, "a.example", 0, given}, b{Command::Resolve, "b.example", 0, given};
    Socks5Client fresh{Command::Resolve, "a.example", 0, rng};
    for (Socks5Client* client : {&a, &b}) {
        BOOST_CHECK_EQUAL(client->Username(), given.username);
        BOOST_CHECK_EQUAL(client->Password(), given.password);
        Request(*client);
    }
    BOOST_CHECK(fresh.Username() != given.username && fresh.Password() != given.password);
    BOOST_CHECK(given.username != given.password);
}

// RFC 1928 section 6: a successful reply with an IPv4 address, a name or an IPv6 address ends the
// exchange, consumed to its last byte and no further. Only a RESOLVE has an answer, and only an
// address: discovery judges whether it is usable (R2).
BOOST_AUTO_TEST_CASE(success)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    const std::string name(255, 'b');
    const std::vector<std::pair<std::vector<uint8_t>, std::optional<CNetAddr>>> replies{
        {ParseHex("050000010a0000010000"), LookupHost("10.0.0.1", /*fAllowLookup=*/false)},
        {ParseHex("0500000420010db80000000000000000000000020000"), LookupHost("2001:db8::2", /*fAllowLookup=*/false)},
        {ParseHex("050000030161208d"), std::nullopt},
        {Cat(ParseHex("05000003ff"), Bytes(name), ParseHex("208d")), std::nullopt},
    };
    for (const Command cmd : {Command::Connect, Command::Resolve}) {
        for (const auto& [reply, address] : replies) {
            Socks5Client client{cmd, "seed.example", 8333, rng};
            Request(client);
            // The destination's first bytes follow.
            const std::vector<uint8_t> bytes{Cat(reply, ParseHex("aabb"))};
            BOOST_CHECK_EQUAL(client.Received(bytes), reply.size());
            BOOST_CHECK(client.Done() && client.BytesToSend().empty());
            BOOST_CHECK(client.Answer() == (cmd == Command::Resolve ? address : std::nullopt));
            BOOST_CHECK_EQUAL(client.Received(bytes), 0U);
        }
    }
}

// The exchange fails at the first byte that is wrong, with nothing more to send: another method
// selected, the credentials or the request refused, a reply RFC 1928 does not allow, or a reply
// byte that comes before the message it answers was written in full.
BOOST_AUTO_TEST_CASE(failure)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    const auto check{[](const Socks5Client& client, size_t used, size_t expected) {
        BOOST_CHECK_EQUAL(used, expected);
        BOOST_CHECK(client.Failed() && client.BytesToSend().empty() && !client.Reason().empty());
    }};
    for (const uint8_t method : {0x00, 0xFF}) {
        Socks5Client client{Command::Connect, "1.2.3.4", 8333, rng};
        WriteAll(client);
        check(client, client.Received(std::vector<uint8_t>{0x05, method, 0x01}), 2);
    }
    {
        Socks5Client client{Command::Connect, "1.2.3.4", 8333, rng};
        WriteAll(client);
        client.Received(ParseHex("0502"));
        WriteAll(client);
        check(client, client.Received(ParseHex("010105")), 2);
    }
    {
        Socks5Client client{Command::Connect, "1.2.3.4", 8333, rng};
        Request(client);
        check(client, client.Received(ParseHex("05050001")), 2);
    }
    // An empty name, and an address type that is none of IPv4, name and IPv6.
    for (const auto& [reply, used] : std::vector<std::pair<std::string, size_t>>{{"050000030000", 5}, {"0500000201020304", 4}}) {
        Socks5Client client{Command::Resolve, "seed.example", 8333, rng};
        Request(client);
        check(client, client.Received(ParseHex(reply)), used);
    }
    // The method selection while the greeting is half written, and the authentication status
    // before the credentials are.
    {
        Socks5Client client{Command::Connect, "1.2.3.4", 8333, rng};
        client.MarkSent(1);
        check(client, client.Received(ParseHex("0502")), 1);
    }
    {
        Socks5Client client{Command::Connect, "1.2.3.4", 8333, rng};
        WriteAll(client);
        check(client, client.Received(ParseHex("05020100")), 3);
    }
}

BOOST_AUTO_TEST_SUITE_END()
