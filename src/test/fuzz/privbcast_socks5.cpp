// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <netaddress.h>
#include <netbase.h>
#include <privbcast/socks5.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <vector>

using privbcast::Socks5Client;

namespace {

bool SameState(const Socks5Client& a, const Socks5Client& b)
{
    return a.Done() == b.Done() && a.Failed() == b.Failed() && a.Reason() == b.Reason() &&
           a.Answer() == b.Answer() && std::ranges::equal(a.BytesToSend(), b.BytesToSend());
}

void initialize_privbcast_socks5()
{
    // B1: the client looks no name up.
    g_dns_lookup = [](const std::string&, bool) -> std::vector<CNetAddr> {
        assert(false);
        return {};
    };
}

} // namespace

/**
 * The SOCKS5 client (RFC 1928, RFC 1929, Tor's RESOLVE) against a proxy whose replies are split at
 * any byte and wrong in any field, with its own messages written in pieces. The client must offer
 * username and password authentication only, with fresh credentials (B2), take each reply as the
 * RFCs lay it out, fail only for cause, and end the same however the bytes were split. The address
 * bytes of a request are unit cases.
 */
FUZZ_TARGET(privbcast_socks5, .init = initialize_privbcast_socks5)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    FastRandomContext rng{ConsumeUInt256(provider)};
    const auto cmd{provider.ConsumeBool() ? Socks5Client::Command::Connect : Socks5Client::Command::Resolve};
    const std::string host{provider.ConsumeBool() ? ConsumeNetAddr(provider).ToStringAddr() : provider.ConsumeRandomLengthString(300)};
    const uint16_t port{provider.ConsumeIntegral<uint16_t>()};
    Socks5Client client{cmd, host, port, rng};
    const Socks5Client initial{client};
    // B2: credentials RFC 1929 can carry, drawn afresh for each client.
    assert(!client.Username().empty() && client.Username().size() <= 255);
    assert(!client.Password().empty() && client.Password().size() <= 255);
    const Socks5Client other{cmd, host, port, rng};
    assert(other.Username() != client.Username() && other.Password() != client.Password());
    // A host that SOCKS5 cannot carry fails the exchange before anything is written. Otherwise the
    // greeting offers username and password authentication alone (B2).
    const bool carriable{!host.empty() && host.size() <= 255 && host.find('\0') == std::string::npos};
    assert(client.Failed() == !carriable);
    if (carriable) assert(std::ranges::equal(client.BytesToSend(), std::array<uint8_t, 3>{0x05, 0x01, 0x02}));
    // What the client writes before the request's address: the greeting, the credentials
    // (RFC 1929) and the request's head.
    std::vector<uint8_t> prefix{0x05, 0x01, 0x02, 0x01, static_cast<uint8_t>(client.Username().size())};
    prefix.insert(prefix.end(), client.Username().begin(), client.Username().end());
    prefix.push_back(static_cast<uint8_t>(client.Password().size()));
    prefix.insert(prefix.end(), client.Password().begin(), client.Password().end());
    const size_t credentials_end{prefix.size()};
    prefix.insert(prefix.end(), {0x05, static_cast<uint8_t>(cmd), 0x00});

    // The proxy's replies: the method selection, the authentication status and the reply to the
    // request, each field right or any byte, the bound address of any type and length, then the
    // destination's first bytes. They may stop anywhere.
    const auto field{[&](uint8_t right) { return provider.ConsumeBool() ? right : provider.ConsumeIntegral<uint8_t>(); }};
    std::vector<uint8_t> replies{field(0x05), field(0x02), field(0x01), field(0x00), field(0x05), field(0x00), field(0x00)};
    const uint8_t atyp{provider.ConsumeBool() ? provider.PickValueInArray<uint8_t>({0x01, 0x03, 0x04}) : provider.ConsumeIntegral<uint8_t>()};
    replies.push_back(atyp);
    // The bound address, if it is an IPv4 or IPv6 one.
    std::optional<CNetAddr> bound;
    if (atyp == 0x03) {
        const std::string name{provider.ConsumeRandomLengthString(255)};
        replies.push_back(field(static_cast<uint8_t>(name.size())));
        replies.insert(replies.end(), name.begin(), name.end());
    } else {
        const size_t size{atyp == 0x01 ? size_t{4} : atyp == 0x04 ? size_t{16} : provider.ConsumeIntegralInRange<size_t>(0, 32)};
        std::vector<uint8_t> addr{provider.ConsumeBytes<uint8_t>(size)};
        addr.resize(size);
        replies.insert(replies.end(), addr.begin(), addr.end());
        if (atyp == 0x01) {
            in_addr ipv4;
            std::memcpy(&ipv4, addr.data(), sizeof(ipv4));
            bound = CNetAddr{ipv4};
        } else if (atyp == 0x04) {
            in6_addr ipv6;
            std::memcpy(&ipv6, addr.data(), sizeof(ipv6));
            bound = CNetAddr{ipv6};
        }
    }
    replies.push_back(provider.ConsumeIntegral<uint8_t>());
    replies.push_back(provider.ConsumeIntegral<uint8_t>());
    // Where each reply ends.
    const std::array<size_t, 3> ends{2, 4, replies.size()};
    // Where the reply to the request ends, as its address type and, for a name, its length byte
    // say (RFC 1928 section 6). None for another address type.
    std::optional<size_t> reply_end;
    if (atyp == 0x01 || atyp == 0x03 || atyp == 0x04) reply_end = 8 + (atyp == 0x01 ? 4 : atyp == 0x04 ? 16 : 1 + size_t{replies[8]}) + 2;
    const std::vector<uint8_t> after{ConsumeRandomLengthByteVector(provider, 16)};
    replies.insert(replies.end(), after.begin(), after.end());
    if (provider.ConsumeBool()) replies.resize(provider.ConsumeIntegralInRange<size_t>(0, replies.size()));
    // A reply field that is wrong: the version or the method selected, the version or the status
    // of the authentication, the request's version, reply code or reserved byte, an address type
    // other than IPv4, name or IPv6, or an empty name.
    const auto wrong{[&](size_t pos) {
        static constexpr std::array<uint8_t, 7> RIGHT{0x05, 0x02, 0x01, 0x00, 0x05, 0x00, 0x00};
        if (pos < RIGHT.size()) return replies[pos] != RIGHT[pos];
        if (pos == 7) return !reply_end;
        return pos == 8 && atyp == 0x03 && replies[8] == 0;
    }};
    const auto any_wrong{[&](size_t end) {
        for (size_t pos{0}; pos < end; ++pos) {
            if (wrong(pos)) return true;
        }
        return false;
    }};

    // What was delivered, what the client consumed and what it wrote.
    size_t next{0};
    size_t taken{0};
    std::vector<uint8_t> written;
    // A reply byte was delivered before the message it answers was written in full.
    bool early{false};
    const auto write{[&](size_t n) {
        const std::span<const uint8_t> bytes{client.BytesToSend().first(n)};
        written.insert(written.end(), bytes.begin(), bytes.end());
        client.MarkSent(n);
    }};

    LIMITED_WHILE(provider.remaining_bytes() > 0, 10'000) {
        CallOneOf(
            provider,
            [&] {
                write(provider.ConsumeIntegralInRange<size_t>(0, client.BytesToSend().size()));
            },
            [&] {
                write(client.BytesToSend().size());
            },
            [&] {
                // The proxy sends its next bytes: as many as it likes, or the rest of a reply.
                size_t size{provider.ConsumeIntegralInRange<size_t>(0, replies.size() - next)};
                if (provider.ConsumeBool()) {
                    const auto end{std::ranges::find_if(ends, [&](size_t reply_end) { return reply_end > next; })};
                    size = std::min(end == ends.end() ? replies.size() : *end, replies.size()) - next;
                }
                const std::span<const uint8_t> chunk{std::span{replies}.subspan(next, size)};
                const bool ended_before{client.Done() || client.Failed()};
                // Bytes that come while the client has something left to write, or that run past
                // the end of the method selection or of the authentication status, answer a
                // message it has not written in full.
                if (!ended_before && !chunk.empty()) {
                    early = early || !client.BytesToSend().empty() ||
                            std::ranges::any_of(ends.begin(), ends.begin() + 2, [&](size_t end) { return next < end && end < next + chunk.size(); });
                }
                next += chunk.size();
                Socks5Client copy{client};
                const size_t used{client.Received(chunk)};
                taken += used;
                if (ended_before) assert(used == 0);
                // Until the exchange ends, every byte is consumed.
                if (!client.Done() && !client.Failed()) assert(used == chunk.size());
                // Nothing past the consumed bytes was read: they alone lead to the same state.
                assert(copy.Received(chunk.first(used)) == used);
                assert(SameState(copy, client));
            });
        // Nothing is sent once failed, and nothing is left to send once done.
        if (client.Done() || client.Failed()) assert(client.BytesToSend().empty());
        if (client.Failed()) assert(!client.Reason().empty());
        // The client writes the greeting, the credentials and the request's head, in that order,
        // each offered only once the reply to the one before it came in full.
        const size_t n{std::min(written.size(), prefix.size())};
        assert(std::equal(written.begin(), written.begin() + n, prefix.begin()));
        const size_t offered{written.size() + client.BytesToSend().size()};
        if (taken < ends[0]) assert(offered <= 3);
        if (taken < ends[1]) assert(offered <= credentials_end);
        // It is done only once it wrote its request, the destination's port last, and consumed a
        // successful reply to it with every field right, and not a byte past it.
        if (client.Done()) {
            assert(written.size() > prefix.size() + 2 && written.end()[-2] == port >> 8 && written.back() == (port & 0xFF));
            assert(reply_end && taken == *reply_end && !any_wrong(taken));
        }
        // It fails only for cause: a host SOCKS5 cannot carry, a wrong field among the bytes it
        // consumed, or a byte that came early.
        if (client.Failed()) assert(!carriable || early || any_wrong(taken));
        // And it is done once the whole reply came in time with every field right.
        if (carriable && !early && reply_end && next >= *reply_end && !any_wrong(*reply_end)) assert(client.Done());
        // Only a RESOLVE answered with an IPv4 or IPv6 address has an answer: that address. Whether
        // it is usable is for discovery to judge (R2).
        assert(client.Answer() == (client.Done() && cmd == Socks5Client::Command::Resolve ? bound : std::nullopt));
    }

    // How the bytes were split, written or delivered changes nothing: given the same bytes, each
    // reply whole once the message it answers was written in full, the client ends the same way.
    if (!early) {
        Socks5Client whole{initial};
        size_t used{0};
        for (const size_t end : {ends[0], ends[1], next}) {
            whole.MarkSent(whole.BytesToSend().size());
            if (end > used) used += whole.Received(std::span{replies}.subspan(used, std::min(end, next) - used));
        }
        assert(used == taken && whole.Done() == client.Done() && whole.Failed() == client.Failed() &&
               whole.Reason() == client.Reason() && whole.Answer() == client.Answer());
    }
}
