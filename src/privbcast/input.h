// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_INPUT_H
#define BITCOIN_PRIVBCAST_INPUT_H

#include <consensus/amount.h>
#include <netbase.h>
#include <primitives/transaction.h>
#include <privbcast/job.h>
#include <util/result.h>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

class ArgsManager;
class CChainParams;

namespace privbcast {

/**
 * The transactions in text: hex strings separated by whitespace, with nothing but whitespace
 * around them (Interface: bitcoin-privbcast). One, or two for a parent and its child in either
 * order. Each must decode in full.
 */
util::Result<std::vector<CTransactionRef>> ParseTransactions(std::string_view text);

/**
 * Whether tx may be broadcast: it passes the consensus checks that need no coins, is not a
 * coinbase, weighs at most MAX_STANDARD_TX_WEIGHT and sends no more than max_burn to any output
 * whose script is provably unspendable.
 */
util::Result<void> CheckForBroadcast(const CTransaction& tx, CAmount max_burn);

/** A child and the parent it spends. */
struct ParentAndChild {
    CTransactionRef parent;
    CTransactionRef child;
};

/**
 * Whether a and b, in either order, are a parent and its child that may be broadcast together
 * (Extension: Interface): they differ and exactly one spends the other; every input of the child
 * that spends the parent names an output the parent has; they share no input; each may be
 * broadcast (CheckForBroadcast); and together they weigh at most MAX_PACKAGE_WEIGHT.
 *
 * @returns the two, the parent being the one whose output the other spends
 */
util::Result<ParentAndChild> CheckPackage(const CTransactionRef& a, const CTransactionRef& b, CAmount max_burn);

/**
 * Read in to its end, unless it holds more than bound bytes. At most bound + 1 bytes are read, so
 * that an input which goes on is refused without waiting for its end (D3).
 *
 * @returns what was read; an error if there is more than bound bytes, or if reading failed before
 *          the end
 */
util::Result<std::string> ReadBounded(std::istream& in, size_t bound);

/**
 * A job's seed material (A1): the chain's, or on regtest the DNS seed names that the test override
 * seed_arg gives, which other chains refuse (U1). Fails on a DNS seed name that SOCKS5 cannot carry
 * (Socks5CanCarry()): an override's always, the chain's own if check_chain_names.
 */
util::Result<SeedMaterial> GetSeedMaterial(const ArgsManager& args, const CChainParams& chain, const std::string& seed_arg,
                                           bool check_chain_names);

/**
 * The proxy that -tor names: a loopback address, with default_port if it gives none, or a unix
 * socket given as unix:<path>, so that the plaintext SOCKS5 exchange stays on this host (B3).
 * Nothing is resolved (B1). Anything else, an empty path included, is a usage error.
 */
util::Result<Proxy> ParseTor(const std::string& value, uint16_t default_port);

#ifndef WIN32
/**
 * Write text to the descriptor fd if it can take it at once, else drop it, so that a full pipe or a
 * reader that has stopped never holds up the writer (Interface: bitcoin-privbcast). fd is the
 * caller's and is never set non-blocking. A pipe takes a write of PIPE_BUF bytes or fewer in one
 * piece once it has room, so a longer text is dropped rather than cut.
 *
 * @returns whether the text was written
 */
bool WriteIfReady(int fd, std::string_view text);
#endif

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_INPUT_H
