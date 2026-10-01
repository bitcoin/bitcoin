// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/input.h>

#include <common/args.h>
#include <consensus/amount.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <kernel/chainparams.h>
#include <netaddress.h>
#include <netbase.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <privbcast/socks5.h>
#include <script/script.h>
#include <tinyformat.h>
#include <util/chaintype.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/string.h>
#include <util/translation.h>

#include <array>
#include <cstddef>
#include <ios>
#include <istream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef WIN32
#include <cerrno>
#include <climits>
#include <poll.h>
#include <unistd.h>
#endif

namespace privbcast {
namespace {

/** 127.0.0.0/8 or ::1. */
bool IsLoopback(const CService& addr)
{
    if (addr.IsIPv4()) return (addr.GetLinkedIPv4() >> 24) == 127;
    return addr.IsIPv6() && addr.IsLocal();
}

} // namespace

util::Result<std::vector<CTransactionRef>> ParseTransactions(std::string_view text)
{
    std::vector<std::string_view> tokens{util::Split<std::string_view>(text, " \f\n\r\t\v")};
    std::erase(tokens, std::string_view{});
    if (tokens.empty()) return util::Error{Untranslated("no transaction given")};
    if (tokens.size() > 1) return util::Error{Untranslated(strprintf("%d transactions given, expected one", tokens.size()))};

    std::vector<CTransactionRef> txs;
    for (const std::string_view token : tokens) {
        CMutableTransaction mtx;
        if (!DecodeHexTx(mtx, std::string{token})) {
            return util::Error{Untranslated("the transaction is not hex or does not decode")};
        }
        txs.push_back(MakeTransactionRef(std::move(mtx)));
    }
    return txs;
}

util::Result<void> CheckForBroadcast(const CTransaction& tx, CAmount max_burn)
{
    TxValidationState state;
    if (!CheckTransaction(tx, state)) {
        return util::Error{Untranslated(strprintf("the transaction is invalid: %s", state.ToString()))};
    }
    if (tx.IsCoinBase()) return util::Error{Untranslated("the transaction is a coinbase")};
    const int32_t weight{GetTransactionWeight(tx)};
    if (weight > MAX_STANDARD_TX_WEIGHT) {
        return util::Error{Untranslated(strprintf("the transaction weighs %d, more than %d", weight, MAX_STANDARD_TX_WEIGHT))};
    }
    for (size_t i{0}; i < tx.vout.size(); ++i) {
        const CTxOut& out{tx.vout[i]};
        if ((out.scriptPubKey.IsUnspendable() || !out.scriptPubKey.HasValidOps()) && out.nValue > max_burn) {
            return util::Error{Untranslated(strprintf("output %d burns %s, more than the %s allowed",
                                                      i, FormatMoney(out.nValue), FormatMoney(max_burn)))};
        }
    }
    return {};
}

util::Result<std::string> ReadBounded(std::istream& in, size_t bound)
{
    std::string text;
    std::array<char, 64 * 1024> chunk;
    while (text.size() <= bound) {
        // Up to one byte past the bound, to tell whether there is more.
        const size_t room{bound - text.size()};
        in.read(chunk.data(), static_cast<std::streamsize>(room < chunk.size() ? room + 1 : chunk.size()));
        text.append(chunk.data(), static_cast<size_t>(in.gcount()));
        if (!in) break;
    }
    if (text.size() > bound) return util::Error{Untranslated(strprintf("the input is longer than %d bytes", bound))};
    // A stream that failed before its end: what was read is not the whole input.
    if (in.bad() || (in.fail() && !in.eof())) return util::Error{Untranslated("reading the input failed")};
    return text;
}

util::Result<SeedMaterial> GetSeedMaterial(const ArgsManager& args, const CChainParams& chain, const std::string& seed_arg,
                                           bool check_chain_names)
{
    if (args.IsArgSet(seed_arg) && chain.GetChainType() != ChainType::REGTEST) {
        return util::Error{Untranslated(strprintf("%s is only available on regtest", seed_arg))};
    }
    SeedMaterial seeds{
        .dns_seeds = chain.DNSSeeds(),
        .default_port = chain.GetDefaultPort(),
        .chain = chain.GetChainTypeString(),
    };
    const bool overridden{args.IsArgSet(seed_arg)};
    if (overridden) seeds.dns_seeds = args.GetArgs(seed_arg);
    // A name that the proxy cannot carry is a usage error, not a query that fails.
    if (overridden || check_chain_names) {
        for (const std::string& name : seeds.dns_seeds) {
            if (!Socks5CanCarry(name)) {
                return util::Error{Untranslated(strprintf("cannot query a DNS seed name of %d bytes: SOCKS5 carries 1 to 255 bytes, none of them NUL", name.size()))};
            }
        }
    }
    return seeds;
}

util::Result<Proxy> ParseTor(const std::string& value, uint16_t default_port)
{
    if (value.starts_with(ADDR_PREFIX_UNIX)) {
        if (value.size() == ADDR_PREFIX_UNIX.size() || !IsUnixSocketPath(value)) {
            return util::Error{Untranslated(strprintf("-tor=%s is not a usable unix socket path", value))};
        }
        return Proxy{value};
    }
    const CService service{LookupNumeric(value, default_port)};
    if (!service.IsValid() || service.GetPort() == 0) {
        return util::Error{Untranslated(strprintf("-tor=%s is neither an ip:port nor a unix:<path>", value))};
    }
    if (!IsLoopback(service)) {
        return util::Error{Untranslated(strprintf("-tor=%s is not a loopback address: a remote proxy would see every destination in plaintext", value))};
    }
    return Proxy{service};
}

#ifndef WIN32
bool WriteIfReady(int fd, std::string_view text)
{
    if (text.size() > PIPE_BUF) return false;
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLOUT;
    if (poll(&pfd, 1, /*timeout=*/0) != 1 || !(pfd.revents & POLLOUT)) return false;
    ssize_t written;
    do {
        written = write(fd, text.data(), text.size());
    } while (written < 0 && errno == EINTR);
    return written == static_cast<ssize_t>(text.size());
}
#endif

} // namespace privbcast
