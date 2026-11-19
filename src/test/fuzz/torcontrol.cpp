// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <crypto/hmac_sha256.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <tinyformat.h>
#include <torcontrol.h>
#include <util/fs.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/threadinterrupt.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

//! Defined in torcontrol.cpp but not exported; used to parse replies as the controller does.
std::pair<std::string, std::string> SplitTorReplyLine(const std::string& s);
std::map<std::string, std::string> ParseTorReplyMapping(const std::string& s);

namespace {
//! Fits every line the controller parses; the longest, an AUTHCHALLENGE reply, is 166 chars.
constexpr size_t MAX_LINE_LENGTH{256};
const std::string TOR_CONTROL_ADDRESS{"127.0.0.1:9051"};
const std::string COOKIE(32, 'c');

constexpr std::string_view COOKIEFILE_KEY{"COOKIEFILE"};

//! `COOKIEFILE="<path>"` for a valid cookie, one of the wrong size and a missing file, created at init.
std::array<std::string, 3> g_cookie_args;

//! Renames every COOKIEFILE key that doesn't name one of our cookie files. Tor input must not make
//! the controller read arbitrary host files: results would vary by host, and some reads block.
void RestrictCookieFiles(std::string& text)
{
    for (size_t pos{0}; (pos = text.find(COOKIEFILE_KEY, pos)) != std::string::npos; ++pos) {
        const std::string_view rest{std::string_view{text}.substr(pos)};
        if (std::ranges::none_of(g_cookie_args, [&](std::string_view arg) { return rest.starts_with(arg); })) {
            text[pos + COOKIEFILE_KEY.size() - 1] = 'X';
        }
    }
}

constexpr std::string_view SERVER_HASH_KEY{"Tor safe cookie authentication server-to-controller hash"};
constexpr std::string_view CLIENT_HASH_KEY{"Tor safe cookie authentication controller-to-server hash"};

//! Independent copy of the SAFECOOKIE HMAC, keyed with the harness cookie.
std::vector<uint8_t> SafeCookieHash(std::string_view key, std::span<const uint8_t> client_nonce, std::span<const uint8_t> server_nonce)
{
    std::vector<uint8_t> hash(CHMAC_SHA256::OUTPUT_SIZE);
    CHMAC_SHA256{UCharCast(key.data()), key.size()}
        .Write(UCharCast(COOKIE.data()), COOKIE.size())
        .Write(client_nonce.data(), client_nonce.size())
        .Write(server_nonce.data(), server_nonce.size())
        .Finalize(hash.data());
    return hash;
}

//! The Tor side of the control connection.
struct FakeTor {
    std::shared_ptr<DynSock::Pipes> pipes{std::make_shared<DynSock::Pipes>()};
    //! Reply bytes queued but not yet written to the socket.
    std::string pending;
    //! End of the bytes already written, too short to hold a whole COOKIEFILE key.
    std::string written_tail;
    //! Bytes the controller sent that don't form a complete command yet.
    std::string sent;
    //! From the last AUTHCHALLENGE command, to answer with a matching server hash and check the
    //! proof. Kept across reconnects, as the controller keeps its own.
    std::vector<uint8_t> client_nonce;
    //! Every client nonce sent, to check none repeats.
    std::set<std::vector<uint8_t>> client_nonces;
    //! The -torpassword. While set, the controller authenticates with it rather than SAFECOOKIE.
    std::string password;
    //! Verbs of the commands not yet answered on this connection, oldest first. Other operations can
    //! also complete replies, so this may drift from the controller's queue; that's still valid input.
    std::deque<std::string> unanswered;
    //! Written bytes after the last line break.
    std::string written_line;
    //! Arguments of AUTH lines written on this connection. Parsing them all is slow, so Offered()
    //! only does it when a command needs it.
    std::vector<std::string> unparsed_auth_args;
    //! Authentication methods listed by the parsed AUTH lines.
    std::set<std::string> offered_methods;
    //! SERVERHASH and SERVERNONCE of every AUTHCHALLENGE line written on this connection that has
    //! both, however it was generated.
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> challenges;

    void NewConnection()
    {
        pipes = std::make_shared<DynSock::Pipes>();
        pending.clear();
        written_tail.clear();
        sent.clear();
        unanswered.clear();
        written_line.clear();
        unparsed_auth_args.clear();
        offered_methods.clear();
        challenges.clear();
    }

    //! Records what a written line tells the controller, parsed as
    //! TorControlConnection::ProcessBuffer() and the reply callbacks do. Any line counts, whatever
    //! command it answers.
    void RecordLine(std::string_view line)
    {
        if (line.size() < 4) return;
        const auto [verb, args]{SplitTorReplyLine(std::string{line.substr(4)})};
        if (verb == "AUTH") {
            unparsed_auth_args.push_back(args);
        } else if (verb == "AUTHCHALLENGE") {
            // authchallenge_cb() rejects a challenge missing a key, so skipping it loses nothing.
            const auto mapping{ParseTorReplyMapping(args)};
            const auto hash{mapping.find("SERVERHASH")};
            const auto nonce{mapping.find("SERVERNONCE")};
            if (hash != mapping.end() && nonce != mapping.end()) {
                challenges.emplace_back(ParseHex(hash->second), ParseHex(nonce->second));
            }
        }
    }

    //! Whether a written challenge proved the cookie and `client_hash` is the matching proof.
    bool ProvedCookie(std::string_view client_hash) const
    {
        return std::ranges::any_of(challenges, [&](const auto& challenge) {
            const auto& [server_hash, server_nonce]{challenge};
            return server_hash == SafeCookieHash(SERVER_HASH_KEY, client_nonce, server_nonce) &&
                   client_hash == HexStr(SafeCookieHash(CLIENT_HASH_KEY, client_nonce, server_nonce));
        });
    }

    //! Whether an AUTH line written on this connection, parsed as protocolinfo_cb() does, offered
    //! `method`.
    bool Offered(const std::string& method)
    {
        for (const auto& args : unparsed_auth_args) {
            const auto mapping{ParseTorReplyMapping(args)};
            if (const auto it{mapping.find("METHODS")}; it != mapping.end()) {
                for (auto& offered : util::SplitString(it->second, ',')) {
                    offered_methods.insert(std::move(offered));
                }
            }
        }
        unparsed_auth_args.clear();
        return offered_methods.contains(method);
    }

    //! Writes the first `n` queued bytes to the socket.
    void Write(size_t n)
    {
        // A key may span this write and the previous one; any rename lands past the written tail.
        std::string stream{written_tail + pending};
        RestrictCookieFiles(stream);
        pending = stream.substr(written_tail.size());
        pipes->recv.PushBytes(pending.data(), n);
        written_tail += pending.substr(0, n);
        written_tail.erase(0, written_tail.size() - std::min(written_tail.size(), COOKIEFILE_KEY.size() - 1));
        written_line += pending.substr(0, n);
        // No length limit: lines the controller rejects as too long only loosen the checks.
        util::LineReader reader{written_line, std::numeric_limits<size_t>::max()};
        while (const auto line{reader.ReadLine()}) RecordLine(*line);
        written_line.erase(0, reader.Consumed());
        pending.erase(0, n);
    }

    //! Reads the commands the controller sent, checks them and queues them for answers.
    void ProcessCommands()
    {
        static constexpr std::array<std::string_view, 5> VERBS{"PROTOCOLINFO", "AUTHENTICATE", "AUTHCHALLENGE", "GETINFO", "ADD_ONION"};
        std::array<char, 4096> buf;
        for (ssize_t n; (n = pipes->send.GetBytes(buf.data(), buf.size())) > 0;) {
            sent.append(buf.data(), n);
        }
        for (size_t end; (end = sent.find("\r\n")) != std::string::npos; sent.erase(0, end + 2)) {
            const std::string_view cmd{std::string_view{sent}.substr(0, end)};
            // Reply data must never break a command apart or inject a new one.
            assert(cmd.find_first_of("\r\n") == std::string_view::npos);
            assert(std::ranges::any_of(VERBS, [&](std::string_view verb) { return cmd.starts_with(verb); }));
            unanswered.emplace_back(cmd.substr(0, cmd.find(' ')));
            // Each authentication method may only be used once Tor offered it on this connection.
            if (cmd == "AUTHENTICATE") {
                assert(Offered("NULL"));
            } else if (cmd.starts_with("AUTHENTICATE ")) {
                const std::string_view arg{cmd.substr(std::string_view{"AUTHENTICATE "}.size())};
                if (password.empty()) {
                    // SAFECOOKIE: send the proof only after Tor proved it knows the cookie.
                    assert(ProvedCookie(arg));
                } else {
                    assert(Offered("HASHEDPASSWORD"));
                }
            } else if (cmd.starts_with("AUTHCHALLENGE SAFECOOKIE ")) {
                assert(Offered("SAFECOOKIE"));
                client_nonce = ParseHex(cmd.substr(std::string_view{"AUTHCHALLENGE SAFECOOKIE "}.size()));
                // A repeated nonce would let a recorded server hash prove the cookie again.
                assert(client_nonce.size() == 32 && client_nonces.insert(client_nonce).second);
            }
        }
    }
};

std::string QuoteTorString(std::string_view s)
{
    std::string quoted{'"'};
    for (const char c : s) {
        if (c == '\\' || c == '"') quoted += '\\';
        quoted += c;
    }
    return quoted + '"';
}

std::string ConsumeAuthLine(FuzzedDataProvider& provider)
{
    // The controller must never use COOKIE; offering it lets the checks catch a fallback that would
    // hand the cookie to whatever listens on the control port.
    static constexpr std::array<std::string_view, 4> METHODS{"NULL", "COOKIE", "SAFECOOKIE", "HASHEDPASSWORD"};
    std::string methods;
    LIMITED_WHILE(provider.ConsumeBool(), 4) {
        if (!methods.empty()) methods += ',';
        methods += provider.PickValueInArray(METHODS);
    }
    return "AUTH METHODS=" + methods + ' ' + provider.PickValueInArray(g_cookie_args);
}

std::string ConsumeServiceIdLine(FuzzedDataProvider& provider)
{
    // The fuzzer can't produce the SHA3 checksum of a v3 onion address on its own.
    const std::string onion{OnionToString(ConsumeFixedLengthByteVector(provider, ADDR_TORV3_SIZE))};
    return "ServiceID=" + onion.substr(0, onion.size() - std::string_view{".onion"}.size());
}

std::string ConsumePrivateKeyLine(FuzzedDataProvider& provider)
{
    return "PrivateKey=ED25519-V3:" + EncodeBase64(ConsumeFixedLengthByteVector(provider, 64));
}

std::string ConsumeSocksLine(FuzzedDataProvider& provider)
{
    std::string line{"net/listeners/socks="};
    LIMITED_WHILE(provider.ConsumeBool(), 4) {
        std::string addr{provider.ConsumeBool() ? strprintf("127.0.0.1:%u", provider.ConsumeIntegral<uint16_t>()) : provider.ConsumeRandomLengthString(32)};
        if (provider.ConsumeBool()) addr = '"' + addr + '"';
        line += addr + ' ';
    }
    return line;
}

std::string ConsumeAuthChallengeLine(FuzzedDataProvider& provider, const FakeTor& tor)
{
    const auto server_nonce{ConsumeFixedLengthByteVector(provider, 32)};
    const auto server_hash{provider.ConsumeBool() ?
                               SafeCookieHash(SERVER_HASH_KEY, tor.client_nonce, server_nonce) :
                               ConsumeFixedLengthByteVector(provider, 32)};
    return "AUTHCHALLENGE SERVERHASH=" + HexStr(server_hash) + " SERVERNONCE=" + HexStr(server_nonce);
}

std::string ConsumeReplyLine(FuzzedDataProvider& provider, const FakeTor& tor)
{
    std::string line;
    CallOneOf(
        provider,
        [&] { line = provider.ConsumeRandomLengthString(MAX_LINE_LENGTH); },
        [&] { line = ConsumeAuthLine(provider); },
        [&] { line = ConsumeServiceIdLine(provider); },
        [&] { line = ConsumePrivateKeyLine(provider); },
        [&] { line = ConsumeSocksLine(provider); },
        [&] { line = ConsumeAuthChallengeLine(provider, tor); });
    return line;
}

//! A reply with any code and lines, whatever command it ends up answering.
TorControlReply ConsumeReply(FuzzedDataProvider& provider, const FakeTor& tor)
{
    static constexpr std::array<int, 4> CODES{TOR_REPLY_OK, TOR_REPLY_UNRECOGNIZED, TOR_REPLY_SYNTAX_ERROR, /*async event=*/650};
    TorControlReply reply;
    reply.code = provider.ConsumeBool() ? provider.PickValueInArray(CODES) : provider.ConsumeIntegralInRange<int>(0, 999);
    LIMITED_WHILE(provider.ConsumeBool(), 16) {
        reply.lines.push_back(ConsumeReplyLine(provider, tor));
    }
    return reply;
}

//! The reply Tor would send to a `verb` command. Usually 250, but other codes keep error paths
//! (like the 512 that triggers the ADD_ONION retry without PoW) reachable.
TorControlReply ConsumeAnswer(FuzzedDataProvider& provider, const FakeTor& tor, std::string_view verb)
{
    // No 6xx codes: ProcessBuffer() treats them as async events and keeps waiting for the real
    // answer, but the caller has already dropped the command from `unanswered`.
    static constexpr std::array<int, 4> CODES{TOR_REPLY_OK, TOR_REPLY_UNRECOGNIZED, TOR_REPLY_SYNTAX_ERROR, /*unspecified error=*/550};
    TorControlReply reply;
    reply.code = provider.ConsumeBool() ? provider.PickValueInArray(CODES) : TOR_REPLY_OK;
    if (verb == "PROTOCOLINFO") {
        reply.lines = {"PROTOCOLINFO 1", ConsumeAuthLine(provider), R"(VERSION Tor="0.4.8.10")", "OK"};
    } else if (verb == "AUTHCHALLENGE") {
        reply.lines = {ConsumeAuthChallengeLine(provider, tor)};
    } else if (verb == "GETINFO") {
        reply.lines = {ConsumeSocksLine(provider), "OK"};
    } else if (verb == "ADD_ONION") {
        reply.lines = {ConsumeServiceIdLine(provider)};
        if (provider.ConsumeBool()) reply.lines.push_back(ConsumePrivateKeyLine(provider));
        reply.lines.emplace_back("OK");
    } else {
        reply.lines = {"OK"};
    }
    return reply;
}

//! Wire format: "<code>-<line>" for all lines but the last, which uses "<code> <line>".
std::string ConsumeWireReply(FuzzedDataProvider& provider, const TorControlReply& reply)
{
    std::string wire;
    for (size_t i{0}; i < reply.lines.size(); ++i) {
        const char separator{i + 1 == reply.lines.size() ? ' ' : provider.ConsumeBool() ? '-' : '+'};
        wire += strprintf("%03d%c%s%s", reply.code, separator, reply.lines[i], provider.ConsumeBool() ? "\r\n" : "\n");
    }
    return wire;
}
} // namespace

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    // Connect() resolves the numeric control address; real DNS lookups abort the fuzzer.
    fNameLookup = false;
    const fs::path datadir{gArgs.GetDataDirNet()};
    const std::array<fs::path, 3> cookie_files{datadir / "cookie_valid", datadir / "cookie_short", datadir / "cookie_missing"};
    Assert(WriteBinaryFile(cookie_files[0], COOKIE));
    Assert(WriteBinaryFile(cookie_files[1], COOKIE.substr(1)));
    for (size_t i{0}; i < cookie_files.size(); ++i) {
        g_cookie_args[i] = std::string{COOKIEFILE_KEY} + '=' + QuoteTorString(fs::PathToString(cookie_files[i]));
    }
}

FUZZ_TARGET(torcontrol, .init = initialize_torcontrol)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    // Sock::SendComplete() reads the node clock, which must be mocked under fuzzing.
    FakeNodeClock clock{1231006505s};

    // A successful add_onion_cb() advertises the service through the global AddLocal().
    ClearLocal();
    std::string torpassword{fuzzed_data_provider.ConsumeBool() ? fuzzed_data_provider.ConsumeRandomLengthString(32) : ""};
    // The password is local config, not Tor input; a line break in it would trip the command check.
    std::erase_if(torpassword, [](char c) { return c == '\r' || c == '\n'; });
    gArgs.ForceSetArg("-torpassword", torpassword);

    FakeTor tor;
    tor.password = torpassword;
    struct RestoreCreateSock {
        ~RestoreCreateSock() { CreateSock = original; }

    private:
        decltype(CreateSock) original{CreateSock};
    } restore_create_sock;
    CreateSock = [&tor](int, int, int) { return std::make_unique<DynSock>(tor.pipes); };

    TorController tor_controller;
    // Keeps disconnected_cb() from sleeping before a reconnect.
    tor_controller.Interrupt();
    CThreadInterrupt interrupt;
    TorControlConnection conn{interrupt};

    // Mirrors TorController::ThreadControl(), with the harness playing Tor.
    const auto connect{[&] {
        Assert(conn.Connect(TOR_CONTROL_ADDRESS));
        tor_controller.connected_cb(conn);
    }};
    const auto receive{[&] {
        if (conn.ReceiveAndProcess()) return;
        tor_controller.disconnected_cb(conn);
        tor.ProcessCommands();
        tor.NewConnection();
        connect();
    }};
    connect();

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Answer the oldest unanswered command with a reply of the matching type.
                if (tor.unanswered.empty()) return;
                const std::string verb{std::move(tor.unanswered.front())};
                tor.unanswered.pop_front();
                tor.pending += ConsumeWireReply(fuzzed_data_provider, ConsumeAnswer(fuzzed_data_provider, tor, verb));
                tor.Write(tor.pending.size());
                receive();
            },
            [&] {
                tor.pending += ConsumeWireReply(fuzzed_data_provider, ConsumeReply(fuzzed_data_provider, tor));
            },
            [&] {
                tor.pending += fuzzed_data_provider.ConsumeRandomLengthString(MAX_LINE_LENGTH);
            },
            [&] {
                // Writing only a prefix can split a line across reads.
                tor.Write(fuzzed_data_provider.ConsumeBool() ? tor.pending.size() : fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, tor.pending.size()));
                receive();
            },
            [&] {
                tor.pipes->recv.Eof();
                receive();
            },
            [&] {
                (void)conn.WaitForData(0ms);
            });
        tor.ProcessCommands();
    }
}
