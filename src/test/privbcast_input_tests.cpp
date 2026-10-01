// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <consensus/amount.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <netbase.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <privbcast/input.h>
#include <privbcast/params.h>
#include <privbcast/socks5.h>
#include <random.h>
#include <script/script.h>
#include <uint256.h>
#include <util/result.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef HAVE_SOCKADDR_UN
#include <sys/un.h>
#endif
#ifndef WIN32
#include <climits>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace privbcast;

namespace {

/** A transaction spending one coin to a witness program, and paying burn to an OP_RETURN output
 *  if burn is set. */
CMutableTransaction MakeTx(std::optional<CAmount> burn = std::nullopt)
{
    CMutableTransaction mtx;
    mtx.version = 2;
    mtx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), 0});
    mtx.vout.emplace_back(10'000, CScript() << OP_0 << std::vector<unsigned char>(20, 0x42));
    if (burn) mtx.vout.emplace_back(*burn, CScript() << OP_RETURN << std::vector<unsigned char>{1, 2, 3});
    return mtx;
}

std::string Hex(const CMutableTransaction& mtx)
{
    return EncodeHexTx(CTransaction{mtx});
}

/** A stream buffer that gives text, then fails as a read error does. */
class FailingBuf : public std::streambuf
{
public:
    explicit FailingBuf(std::string text) : m_text{std::move(text)}
    {
        setg(m_text.data(), m_text.data(), m_text.data() + m_text.size());
    }

protected:
    int_type underflow() override { throw std::ios_base::failure{"read error"}; }

private:
    std::string m_text;
};

} // namespace

BOOST_AUTO_TEST_SUITE(privbcast_input_tests)

BOOST_AUTO_TEST_CASE(parse)
{
    const CMutableTransaction mtx{MakeTx()};
    const std::string hex{Hex(mtx)};

    // One hex string, with whitespace around it.
    for (const std::string& text : {hex, " " + hex, hex + "\n", "\n\t " + hex + " \r\n\n"}) {
        const auto txs{ParseTransactions(text)};
        BOOST_REQUIRE(txs);
        BOOST_REQUIRE_EQUAL(txs->size(), 1U);
        BOOST_CHECK(txs->front()->GetWitnessHash() == CTransaction{mtx}.GetWitnessHash());
    }

    // Upper case hex is hex.
    BOOST_CHECK(ParseTransactions(ToUpper(hex)));

    // Two transactions are refused, and so is one given twice.
    BOOST_CHECK(!ParseTransactions(hex + "\n" + Hex(MakeTx(/*burn=*/0))));
    BOOST_CHECK(!ParseTransactions(hex + " " + hex));

    // Nothing, or nothing but whitespace.
    BOOST_CHECK(!ParseTransactions(""));
    BOOST_CHECK(!ParseTransactions(" \n\t"));

    // Not hex, or not only hex.
    BOOST_CHECK(!ParseTransactions("zz"));
    BOOST_CHECK(!ParseTransactions(hex + "zz"));
    BOOST_CHECK(!ParseTransactions(hex + " zz"));
    BOOST_CHECK(!ParseTransactions("0x" + hex));
    BOOST_CHECK(!ParseTransactions(hex + std::string(1, '\0')));
    // An odd number of digits.
    BOOST_CHECK(!ParseTransactions(hex + "0"));

    // A truncated transaction, and one with bytes after it.
    BOOST_CHECK(!ParseTransactions(hex.substr(0, hex.size() - 2)));
    BOOST_CHECK(!ParseTransactions(hex + "00"));
}

BOOST_AUTO_TEST_CASE(check)
{
    BOOST_CHECK(CheckForBroadcast(CTransaction{MakeTx()}, /*max_burn=*/0));

    // Consensus checks that need no coins.
    CMutableTransaction no_outputs{MakeTx()};
    no_outputs.vout.clear();
    BOOST_CHECK(!CheckForBroadcast(CTransaction{no_outputs}, 0));
    CMutableTransaction duplicate_inputs{MakeTx()};
    duplicate_inputs.vin.push_back(duplicate_inputs.vin.front());
    BOOST_CHECK(!CheckForBroadcast(CTransaction{duplicate_inputs}, 0));
    CMutableTransaction negative_output{MakeTx()};
    negative_output.vout.front().nValue = -1;
    BOOST_CHECK(!CheckForBroadcast(CTransaction{negative_output}, 0));

    // A coinbase, which passes those.
    CMutableTransaction coinbase{MakeTx()};
    coinbase.vin.front().prevout.SetNull();
    coinbase.vin.front().scriptSig = CScript() << OP_1 << OP_1;
    TxValidationState state;
    BOOST_CHECK(CheckTransaction(CTransaction{coinbase}, state));
    BOOST_CHECK(!CheckForBroadcast(CTransaction{coinbase}, 0));

    // The coinbase check runs on what ParseTransactions decoded.
    const auto parsed{ParseTransactions(Hex(coinbase))};
    BOOST_REQUIRE(parsed);
    BOOST_CHECK(!CheckForBroadcast(*parsed->front(), 0));
}

BOOST_AUTO_TEST_CASE(weight)
{
    // A witness item sized so that the transaction weighs exactly MAX_STANDARD_TX_WEIGHT: each
    // witness byte weighs one unit.
    CMutableTransaction mtx{MakeTx()};
    mtx.vin.front().scriptWitness.stack.emplace_back(100'000, 0x01);
    const int32_t base{GetTransactionWeight(CTransaction{mtx})};
    mtx.vin.front().scriptWitness.stack.front().resize(100'000 + MAX_STANDARD_TX_WEIGHT - base);
    BOOST_REQUIRE_EQUAL(GetTransactionWeight(CTransaction{mtx}), MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK(CheckForBroadcast(CTransaction{mtx}, 0));

    mtx.vin.front().scriptWitness.stack.front().push_back(0x01);
    BOOST_REQUIRE_EQUAL(GetTransactionWeight(CTransaction{mtx}), MAX_STANDARD_TX_WEIGHT + 1);
    BOOST_CHECK(!CheckForBroadcast(CTransaction{mtx}, 0));

    // As read from hex.
    const auto parsed{ParseTransactions(Hex(mtx))};
    BOOST_REQUIRE(parsed);
    BOOST_CHECK(!CheckForBroadcast(*parsed->front(), 0));
}

BOOST_AUTO_TEST_CASE(burn)
{
    // An OP_RETURN output: refused above max_burn, accepted at or below it.
    const CTransaction burning{MakeTx(/*burn=*/1'000)};
    BOOST_CHECK(!CheckForBroadcast(burning, 0));
    BOOST_CHECK(!CheckForBroadcast(burning, 999));
    BOOST_CHECK(CheckForBroadcast(burning, 1'000));
    BOOST_CHECK(CheckForBroadcast(burning, MAX_MONEY));
    // An OP_RETURN output of no value burns nothing.
    BOOST_CHECK(CheckForBroadcast(CTransaction{MakeTx(/*burn=*/0)}, 0));

    // A script with an invalid opcode is unspendable too, and so is one longer than
    // MAX_SCRIPT_SIZE.
    CMutableTransaction invalid_op{MakeTx()};
    invalid_op.vout.front().scriptPubKey = CScript() << OP_INVALIDOPCODE;
    BOOST_CHECK(!CheckForBroadcast(CTransaction{invalid_op}, 9'999));
    BOOST_CHECK(CheckForBroadcast(CTransaction{invalid_op}, 10'000));
    CMutableTransaction too_long{MakeTx()};
    const std::vector<unsigned char> ones(MAX_SCRIPT_SIZE + 1, OP_1);
    too_long.vout.front().scriptPubKey = CScript(ones.begin(), ones.end());
    BOOST_CHECK(!CheckForBroadcast(CTransaction{too_long}, 0));
    BOOST_CHECK(CheckForBroadcast(CTransaction{too_long}, 10'000));
}

BOOST_AUTO_TEST_CASE(read_bounded)
{
    // Input that ends within the bound is returned whole.
    {
        std::istringstream in{"abc"};
        BOOST_CHECK_EQUAL(ReadBounded(in, 3).value(), "abc");
    }
    {
        std::istringstream in{"abc"};
        BOOST_CHECK_EQUAL(ReadBounded(in, 1000).value(), "abc");
    }
    {
        std::istringstream in{""};
        BOOST_CHECK_EQUAL(ReadBounded(in, 0).value(), "");
    }
    // More than the bound: refused, with bound + 1 bytes read and the rest left in the stream.
    {
        std::istringstream in{"abcdef"};
        BOOST_CHECK(!ReadBounded(in, 2));
        std::string rest;
        in >> rest;
        BOOST_CHECK_EQUAL(rest, "def");
    }
    {
        std::istringstream in{"a"};
        BOOST_CHECK(!ReadBounded(in, 0));
    }
    // Over more than one read.
    const size_t bound{MAX_STDIN_BYTES};
    {
        std::istringstream in{std::string(bound, 'x')};
        BOOST_CHECK_EQUAL(ReadBounded(in, bound).value().size(), bound);
    }
    {
        std::istringstream in{std::string(bound + 10, 'x')};
        BOOST_CHECK(!ReadBounded(in, bound));
        std::string rest;
        in >> rest;
        BOOST_CHECK_EQUAL(rest.size(), 9U);
    }
}

BOOST_AUTO_TEST_CASE(read_failure)
{
    // A failing stream is refused, even after a valid transaction and a megabyte of whitespace.
    const std::string text{Hex(MakeTx()) + std::string(1'000'000, ' ')};
    BOOST_REQUIRE(ParseTransactions(text));
    FailingBuf buf{text};
    std::istream in{&buf};
    BOOST_CHECK(!ReadBounded(in, MAX_STDIN_BYTES));
    // Failing at once, and a stream that had failed already.
    FailingBuf nothing{""};
    std::istream at_once{&nothing};
    BOOST_CHECK(!ReadBounded(at_once, MAX_STDIN_BYTES));
    std::istringstream failed{Hex(MakeTx())};
    failed.setstate(std::ios_base::failbit);
    BOOST_CHECK(!ReadBounded(failed, MAX_STDIN_BYTES));
}

BOOST_AUTO_TEST_CASE(tor)
{
    // B1, B3: the proxy is a loopback address, with the default port unless one is given, or
    // unix:<path>; never a name, another address or an empty path.
    const auto parse{[](const std::string& value) { return ParseTor(value, 9050); }};
    for (const auto& [value, proxy] : {std::pair{"127.0.0.1", "127.0.0.1:9050"}, std::pair{"127.1.2.3:9150", "127.1.2.3:9150"},
                                       std::pair{"[::1]:9150", "[::1]:9150"}}) {
        const auto parsed{parse(value)};
        BOOST_REQUIRE(parsed);
        BOOST_CHECK(!parsed->m_is_unix_socket);
        BOOST_CHECK_EQUAL(parsed->ToString(), proxy);
    }
    for (const std::string value : {"", "localhost", "localhost:9050", "10.1.2.3:9050", "0.0.0.0:9050", "[::]:9050",
                                    "[2001:db8::1]:9050", "127.0.0.1:0", "unix:"}) {
        BOOST_CHECK_MESSAGE(!parse(value), value);
    }
#ifdef HAVE_SOCKADDR_UN
    const auto parsed{parse("unix:/run/tor/socks")};
    BOOST_REQUIRE(parsed);
    BOOST_CHECK(parsed->m_is_unix_socket);
    BOOST_CHECK_EQUAL(parsed->ToString(), "unix:/run/tor/socks");
    // The path and its terminating NUL fill sun_path at most.
    BOOST_CHECK(parse("unix:/" + std::string(sizeof(sockaddr_un::sun_path) - 2, 'a')));
    BOOST_CHECK(!parse("unix:/" + std::string(sizeof(sockaddr_un::sun_path) - 1, 'a')));
#else
    BOOST_CHECK(!parse("unix:/run/tor/socks"));
#endif
}

#ifndef WIN32
BOOST_AUTO_TEST_CASE(write_if_ready)
{
    // Interface: bitcoin-privbcast. A pipe with room takes a text of up to PIPE_BUF bytes whole and
    // drops a longer one; a full pipe gets nothing, at once, the descriptor left blocking, until
    // the reader catches up.
    int fds[2];
    BOOST_REQUIRE_EQUAL(pipe(fds), 0);
    const int read_end{fds[0]}, write_end{fds[1]};
    // Everything the pipe holds, without waiting for more.
    const auto drain{[&] {
        std::string held;
        const int flags{fcntl(read_end, F_GETFL)};
        fcntl(read_end, F_SETFL, flags | O_NONBLOCK);
        std::array<char, 4096> buf;
        for (ssize_t n; (n = read(read_end, buf.data(), buf.size())) > 0;) held.append(buf.data(), n);
        fcntl(read_end, F_SETFL, flags);
        return held;
    }};
    BOOST_CHECK(WriteIfReady(write_end, "a line\n"));
    BOOST_CHECK_EQUAL(drain(), "a line\n");
    BOOST_CHECK(WriteIfReady(write_end, std::string(PIPE_BUF, 'a')));
    BOOST_CHECK(!WriteIfReady(write_end, std::string(PIPE_BUF + 1, 'b')));
    BOOST_CHECK_EQUAL(drain(), std::string(PIPE_BUF, 'a'));

    // Full, as a reader that has stopped leaves it.
    const int flags{fcntl(write_end, F_GETFL)};
    BOOST_REQUIRE(!(flags & O_NONBLOCK));
    fcntl(write_end, F_SETFL, flags | O_NONBLOCK);
    const std::string chunk(PIPE_BUF, 'c');
    while (write(write_end, chunk.data(), chunk.size()) > 0) {}
    while (write(write_end, chunk.data(), 1) > 0) {}
    fcntl(write_end, F_SETFL, flags);
    // On another thread, so that a write that blocks fails the test rather than hang it: the pipe is
    // drained once the wait is over.
    std::atomic<bool> returned{false};
    bool written{true};
    std::thread writer{[&] {
        written = WriteIfReady(write_end, "a line\n");
        returned = true;
    }};
    for (int waited{0}; waited < 10'000 && !returned; ++waited) std::this_thread::sleep_for(std::chrono::milliseconds{1});
    BOOST_CHECK(returned);
    const std::string held{drain()};
    writer.join();
    BOOST_CHECK(!written);
    BOOST_CHECK_EQUAL(held.find('\n'), std::string::npos);
    BOOST_CHECK_EQUAL(fcntl(write_end, F_GETFL), flags);
    BOOST_CHECK(WriteIfReady(write_end, "a line\n"));
    BOOST_CHECK_EQUAL(drain(), "a line\n");
    close(read_end);
    close(write_end);
}

BOOST_AUTO_TEST_CASE(write_if_ready_no_reader)
{
    // main() ignores SIGPIPE before its first write, which this case relies on: the write fails.
    int fds[2];
    BOOST_REQUIRE_EQUAL(pipe(fds), 0);
    close(fds[0]);
    struct sigaction ignore{}, previous{};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    BOOST_REQUIRE_EQUAL(sigaction(SIGPIPE, &ignore, &previous), 0);
    const bool written{WriteIfReady(fds[1], "a line\n")};
    BOOST_CHECK_EQUAL(sigaction(SIGPIPE, &previous, nullptr), 0);
    close(fds[1]);
    BOOST_CHECK(!written);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
