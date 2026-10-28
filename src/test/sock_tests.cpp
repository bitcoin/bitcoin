// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <compat/compat.h>
#include <netbase.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

BOOST_FIXTURE_TEST_SUITE(sock_tests, BasicTestingSetup)

static bool SocketIsClosed(const SOCKET& s)
{
    // Notice that if another thread is running and creates its own socket after `s` has been
    // closed, it may be assigned the same file descriptor number. In this case, our test will
    // wrongly pretend that the socket is not closed.
    int type;
    socklen_t len = sizeof(type);
    return getsockopt(s, SOL_SOCKET, SO_TYPE, reinterpret_cast<char*>(&type), &len) == SOCKET_ERROR;
}

static SOCKET CreateSocket()
{
    const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    BOOST_REQUIRE(s != static_cast<SOCKET>(SOCKET_ERROR));
    return s;
}

BOOST_AUTO_TEST_CASE(constructor_and_destructor)
{
    const SOCKET s = CreateSocket();
    Sock* sock = new Sock(s);
    BOOST_CHECK(*sock == s);
    BOOST_CHECK(!SocketIsClosed(s));
    delete sock;
    BOOST_CHECK(SocketIsClosed(s));
}

BOOST_AUTO_TEST_CASE(move_constructor)
{
    const SOCKET s = CreateSocket();
    Sock* sock1 = new Sock(s);
    Sock* sock2 = new Sock(std::move(*sock1));
    delete sock1;
    BOOST_CHECK(!SocketIsClosed(s));
    BOOST_CHECK(*sock2 == s);
    delete sock2;
    BOOST_CHECK(SocketIsClosed(s));
}

BOOST_AUTO_TEST_CASE(move_assignment)
{
    const SOCKET s1 = CreateSocket();
    const SOCKET s2 = CreateSocket();
    Sock* sock1 = new Sock(s1);
    Sock* sock2 = new Sock(s2);

    BOOST_CHECK(!SocketIsClosed(s1));
    BOOST_CHECK(!SocketIsClosed(s2));

    *sock2 = std::move(*sock1);
    BOOST_CHECK(!SocketIsClosed(s1));
    BOOST_CHECK(SocketIsClosed(s2));
    BOOST_CHECK(*sock2 == s1);

    delete sock1;
    BOOST_CHECK(!SocketIsClosed(s1));
    BOOST_CHECK(SocketIsClosed(s2));
    BOOST_CHECK(*sock2 == s1);

    delete sock2;
    BOOST_CHECK(SocketIsClosed(s1));
    BOOST_CHECK(SocketIsClosed(s2));
}

struct TcpSocketPair {
    Sock sender;
    Sock receiver;

    TcpSocketPair()
        : sender{Sock{CreateSocket()}},
        receiver{Sock{CreateSocket()}}
    {
        connect_pair();
    }

    TcpSocketPair(const TcpSocketPair&) = delete;
    TcpSocketPair& operator= (const TcpSocketPair&) = delete;
    TcpSocketPair(TcpSocketPair&&) = default;
    TcpSocketPair& operator= (TcpSocketPair&&) = default;

    void connect_pair()
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        BOOST_REQUIRE_EQUAL(receiver.Bind(reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
        BOOST_REQUIRE_EQUAL(receiver.Listen(1), 0);

        // Get the address of the listener.
        sockaddr_in bound{};
        socklen_t blen = sizeof(bound);
        BOOST_REQUIRE_EQUAL(receiver.GetSockName(reinterpret_cast<sockaddr*>(&bound), &blen), 0);
        BOOST_REQUIRE_EQUAL(blen, sizeof(bound));

        BOOST_REQUIRE_EQUAL(sender.Connect(reinterpret_cast<sockaddr*>(&bound), sizeof(bound)), 0);

        std::unique_ptr<Sock> accepted = receiver.Accept(nullptr, nullptr);
        BOOST_REQUIRE(accepted != nullptr);

        receiver = std::move(*accepted);
    }

    void send_and_receive()
    {
        const char* msg = "abcd";
        constexpr ssize_t msg_len = 4;
        char recv_buf[10];

        BOOST_CHECK_EQUAL(sender.Send(msg, msg_len, 0), msg_len);
        BOOST_CHECK_EQUAL(receiver.Recv(recv_buf, sizeof(recv_buf), 0), msg_len);
        BOOST_CHECK_EQUAL(strncmp(msg, recv_buf, msg_len), 0);
    }
};

BOOST_AUTO_TEST_CASE(send_and_receive)
{
    TcpSocketPair socks{};
    socks.send_and_receive();

    // Sockets are still connected after being moved.
    TcpSocketPair socks_moved = std::move(socks);
    socks_moved.send_and_receive();
}

BOOST_AUTO_TEST_CASE(wait)
{
    TcpSocketPair socks = TcpSocketPair{};

    std::thread waiter([&socks]() { (void)socks.receiver.Wait(24h, Sock::RecvEvent); });

    BOOST_REQUIRE_EQUAL(socks.sender.Send("a", 1, 0), 1);

    waiter.join();
}

BOOST_AUTO_TEST_CASE(recv_until_terminator_limit)
{
    constexpr auto timeout = 1min; // High enough so that it is never hit.
    CThreadInterrupt interrupt;

    TcpSocketPair socks = TcpSocketPair{};

    std::thread receiver([&socks, &timeout, &interrupt]() {
        constexpr size_t max_data{10};
        bool threw_as_expected{false};
        // BOOST_CHECK_EXCEPTION() writes to some variables shared with the main thread which
        // creates a data race. So mimic it manually.
        try {
            (void)socks.receiver.RecvUntilTerminator('\n', timeout, interrupt, max_data);
        } catch (const std::runtime_error& e) {
            threw_as_expected = HasReason("too many bytes without a terminator")(e);
        }
        assert(threw_as_expected);
    });

    BOOST_REQUIRE_NO_THROW(socks.sender.SendComplete("1234567", timeout, interrupt));
    BOOST_REQUIRE_NO_THROW(socks.sender.SendComplete("89a\n", timeout, interrupt));

    receiver.join();
}

/**
 * Various helpers to use in the following tests, which pin down some invariants about the socket
 * API against the real OS over loopback TCP. In these tests, we exercise a non-blocking Sock ("our"
 * end of the connection) against a remote peer represented as a bare OS socket.
 */
namespace {

#ifdef USE_POLL
constexpr bool POLL{true};
#else
constexpr bool POLL{false};
#endif
#ifdef WIN32
constexpr bool WINDOWS{true};
#else
constexpr bool WINDOWS{false};
#endif
// poll() reports a reset connection (POLLERR, POLLHUP) whatever was requested, even nothing.
// select() reports it only through the read and write sets.
constexpr bool RESET_REPORTS_ERROR{POLL};
// Windows' select() fails if no socket requests any event, as all three sets are then empty.
constexpr bool NOTHING_REQUESTED_FAILS{!POLL && WINDOWS};

// Timeout for an expected event. Would only be reached on test failure. Note we never wait for
// something *not* to happen, all tests are level-triggered.
constexpr auto LONG_WAIT{60s};

#ifdef WIN32
constexpr int SHUT_WRITE{SD_SEND};
#else
constexpr int SHUT_WRITE{SHUT_WR};
#endif

constexpr int SOCK_OPTION_ON{1};

std::string EventsStr(Sock::Event events)
{
    std::string s;
    if (events & Sock::RecvEvent) s += "|Recv";
    if (events & Sock::SendEvent) s += "|Send";
    if (events & Sock::ErrorEvent) s += "|Error";
    if (events & ~(Sock::RecvEvent | Sock::SendEvent | Sock::ErrorEvent)) s += "|garbage";
    return s.empty() ? "0" : s.substr(1);
}

void CloseSocket(SOCKET s)
{
#ifdef WIN32
    closesocket(s);
#else
    close(s);
#endif
}

/**
 * Wait on `sock` for `requested` until all of `expected` occurred, or poll once if nothing is
 * expected.
 * @return what occurred in the last wait.
 */
Sock::Event AwaitEvents(const Sock& sock, Sock::Event requested, Sock::Event expected)
{
    const auto deadline{Now<SteadyMilliseconds>() + LONG_WAIT};
    for (;;) {
        const auto remaining{deadline - Now<SteadyMilliseconds>()};
        Sock::Event occurred{0xff};
        const auto sock_timeout{expected == 0 ? 0ms : std::max(remaining, 0ms)};
        BOOST_REQUIRE(sock.Wait(sock_timeout, requested, &occurred));
        if ((occurred & expected) == expected || remaining <= 0ms) return occurred;
        // Something else is ready already. Give the rest time to arrive.
        std::this_thread::sleep_for(1ms);
    }
}

/** recv() or send() made progress or failed for good. */
bool Progressed(ssize_t ret)
{
    return ret >= 0 || IOErrorIsPermanent(WSAGetLastError());
}

bool WouldBlock(ssize_t ret)
{
    return !Progressed(ret);
}

/** A socket of ours, with the options we set in production. */
std::unique_ptr<Sock> CreateOurSock()
{
    // Avoid CreateSock(), which could be silently overriden.
    auto sock{CreateSockOS(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
    BOOST_REQUIRE(sock);
    return sock;
}

/** Our listening socket. */
struct Listener {
    const std::unique_ptr<Sock> sock_ptr{CreateOurSock()};
    const Sock& sock{*sock_ptr};
    sockaddr_in addr{};

    Listener()
    {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        BOOST_REQUIRE_EQUAL(sock.Bind(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)), 0);
        BOOST_REQUIRE_EQUAL(sock.Listen(SOMAXCONN), 0);
        socklen_t len{sizeof(addr)};
        BOOST_REQUIRE_EQUAL(sock.GetSockName(reinterpret_cast<sockaddr*>(&addr), &len), 0);
    }
};

/**
 * A loopback TCP connection between our non-blocking Sock, which is accepted like an inbound
 * peer's, and a bare OS socket that plays the remote peer.
 */
struct Connection {
    // A fixed size disables buffer autotuning, which on macOS would grow a full send buffer and
    // make room again.
    static constexpr int SMALL_BUFFER{4096};

    SOCKET peer;
    std::shared_ptr<Sock> sock;

    explicit Connection(const Listener& listener) : peer{CreateSocket()}
    {
        // Set before connecting: the receive window is negotiated in the handshake.
        BOOST_REQUIRE_EQUAL(setsockopt(peer, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&SMALL_BUFFER), sizeof(SMALL_BUFFER)), 0);
        BOOST_REQUIRE_EQUAL(connect(peer, reinterpret_cast<const sockaddr*>(&listener.addr), sizeof(listener.addr)), 0);

        // The connect() above is blocking, but make sure our side did process the ACK.
        Sock::Event occurred{0xff};
        BOOST_REQUIRE(listener.sock.Wait(LONG_WAIT, Sock::RecvEvent, &occurred));
        BOOST_REQUIRE_EQUAL(occurred, Sock::RecvEvent);

        auto accepted{listener.sock.Accept(nullptr, nullptr)};
        BOOST_REQUIRE(accepted);
        sock = std::move(accepted);
        // Other options are inherited from the listening socket, but not O_NONBLOCK on Linux, nor
        // TCP_NODELAY everywhere. In production we set TCP_NODELAY again but not O_NONBLOCK (!) and
        // instead rely on passing MSG_DONTWAIT for all recv() and send(). Set it here so a test failure
        // raises instead of hanging.
        BOOST_REQUIRE(sock->SetNonBlocking());
        BOOST_REQUIRE_EQUAL(sock->SetSockOpt(IPPROTO_TCP, TCP_NODELAY, &SOCK_OPTION_ON, sizeof(SOCK_OPTION_ON)), 0);
        BOOST_REQUIRE_EQUAL(sock->SetSockOpt(SOL_SOCKET, SO_SNDBUF, &SMALL_BUFFER, sizeof(SMALL_BUFFER)), 0);
    }

    ~Connection() { ClosePeer(); }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void PeerSend(std::string_view data, int flags = 0) const
    {
        BOOST_REQUIRE_EQUAL(send(peer, data.data(), data.size(), flags), static_cast<ssize_t>(data.size()));
    }

    void PeerShutdown() const { BOOST_REQUIRE_EQUAL(shutdown(peer, SHUT_WRITE), 0); }

    void ClosePeer()
    {
        if (peer != INVALID_SOCKET) {
            CloseSocket(peer);
        }
        peer = INVALID_SOCKET;
    }

    /** Abort the connection: close the peer's socket so that it sends RST. */
    void ResetPeer()
    {
        linger lin{};
        lin.l_onoff = 1;
        lin.l_linger = 0;
        BOOST_REQUIRE_EQUAL(setsockopt(peer, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lin), sizeof(lin)), 0);
        ClosePeer();
    }

    /** Write to our socket until it would block. */
    void FillSendBuffer() const
    {
        const std::vector<char> data(SMALL_BUFFER, 'x');
        size_t sent{0};
        for (;;) {
            const ssize_t ret{sock->Send(data.data(), data.size(), MSG_NOSIGNAL)};
            if (ret < 0) {
                BOOST_REQUIRE(WouldBlock(ret));
                return;
            }
            sent += ret;
            // 64 MiB is far more than any socket buffer.
            BOOST_REQUIRE_LT(sent, 64U << 20);
        }
    }

    /**
     * Wait until normal data, the end of the stream or a reset has arrived. Any event will do, so
     * that a backend that misreports it fails the checks of the test rather than this wait.
     */
    void AwaitReadable() const
    {
        Sock::Event occurred{0};
        BOOST_REQUIRE(sock->Wait(LONG_WAIT, Sock::RecvEvent, &occurred));
        BOOST_REQUIRE(occurred != 0);
    }
};

/** What the peer did last. */
enum class Last {
    NOTHING,
    SHUTDOWN, //!< shut down its sending side
    CLOSE,    //!< closed its socket
    RESET,    //!< aborted the connection
};

struct Scenario {
    // State of the socket prior to the peer's last action.
    bool data;   //!< the peer sent normal data
    bool full;   //!< our send buffer is full
    //! Peer's last action, happening after the above.
    Last last;
    //! This scenario's connection from the remote peer to our Sock.
    std::unique_ptr<Connection> conn{};

    //! Collection of events reported by Wait() calls for this scenario, for the progress checks.
    Sock::Event occurred_any{0};

    /** Whether the peer's last action led to a connection reset. */
    bool Reset() const
    {
        // A peer that closes its socket with unread data resets the connection.
        return last == Last::RESET || (last == Last::CLOSE && full);
    }

    /** Whether the peer's last action means it cleanly closed its end of the connection. */
    bool EndOfStream() const
    {
        return last == Last::SHUTDOWN || (last == Last::CLOSE && !full);
    }

    std::string Name() const
    {
        static constexpr const char* LAST[]{"nothing", "shutdown", "close", "reset"};
        return strprintf("peer: %s%s, our send buffer: %s", data ? "data then " : "", LAST[static_cast<int>(last)], full ? "full" : "empty");
    }

    std::string Name(Sock::Event requested) const { return Name() + ", requested: " + EventsStr(requested); }

    /** The events that must occur, and those that may in addition. */
    struct Expected {
        Sock::Event must{0};
        Sock::Event may{0};
    };

    Expected Expect(Sock::Event requested) const
    {
        Expected e;
        if (requested & Sock::RecvEvent) {
            // Data, the end of the stream or a reset: the next recv() does not block.
            if (data || EndOfStream() || Reset()) e.must |= Sock::RecvEvent;
        }
        if (requested & Sock::SendEvent) {
            // A send() to a reset connection fails at once, so it is reported. When the send buffer is
            // full, the OS may still find room (Windows does after a while), which is fine as long as
            // the next send() then makes progress.
            if (!full || Reset()) {
                e.must |= Sock::SendEvent;
            } else {
                e.may |= Sock::SendEvent;
            }
        }
        if (Reset() && RESET_REPORTS_ERROR) {
            e.must |= Sock::ErrorEvent;
        }
        return e;
    }

    void Check(Sock::Event requested, Sock::Event occurred, std::string_view via = "") const
    {
        const Expected expected{Expect(requested)};
        const bool all_expected{(occurred & expected.must) == expected.must};
        const bool no_unexpected{(occurred & ~(expected.must | expected.may)) == 0};
        BOOST_CHECK_MESSAGE(all_expected && no_unexpected,
                            Name(requested) << via << ": occurred " << EventsStr(occurred) << ", expected " << EventsStr(expected.must)
                                            << (expected.may ? " and maybe " + EventsStr(expected.may) : ""));
    }
};

} // namespace

/**
 * Test the events reported by Wait() / WaitMany() for a matrix of scenarii (what the peer did prior
 * to the poll, what event is requested). This exhaustively checks all states covered by the Scenario
 * model. Some states were deliberately left out of the model, for instance because they require a fix
 * (URG data on macOS / Windows), or because they can't be reliably triggered in our unit tests.
 */
BOOST_AUTO_TEST_CASE(readiness)
{
    Listener listener;

    // Create a scenario for each combination of last state, whether the peer sent data before that
    // last state, and whether our send buffer is full (i.e. a write would block).
    std::vector<Scenario> scenarios;
    for (const bool data : {false, true}) {
        for (const Last last : {Last::NOTHING, Last::SHUTDOWN, Last::CLOSE, Last::RESET}) {
            for (const bool full : {false, true}) {
                scenarios.push_back({.data = data, .full = full, .last = last});
            }
        }
    }

    // Now for each scenario, setup the connection according to the scenario's instructions.
    for (auto& s : scenarios) {
        s.conn = std::make_unique<Connection>(listener);
        if (s.full) s.conn->FillSendBuffer();
        if (s.data) s.conn->PeerSend("data");
        switch (s.last) {
        case Last::NOTHING: break;
        case Last::SHUTDOWN: s.conn->PeerShutdown(); break;
        case Last::CLOSE: s.conn->ClosePeer(); break;
        case Last::RESET: s.conn->ResetPeer(); break;
        }
    }

    // Make sure that what the peers just did has arrived. What arrives after that, like a reset
    // that follows a FIN, is waited for below.
    for (const auto& s : scenarios) {
        if (s.data || s.EndOfStream() || s.Reset()) {
            s.conn->AwaitReadable();
        }
    }

    // Go through all combinations of requestable events, and for each scenario determine what
    // reported events should be expected, then perform the wait and check expectations.
    // Wait on each scenario's socket individually, and then on all scenarios' sockets at once
    // using WaitMany(). Events are level-triggered, so each must report the same.
    for (const Sock::Event requested : {Sock::Event{0}, Sock::RecvEvent, Sock::SendEvent, Sock::Event{Sock::RecvEvent | Sock::SendEvent}}) {
        Sock::EventsPerSock events_per_sock;
        for (auto& s : scenarios) {
            const auto& sock{*s.conn->sock};
            if (requested == 0 && NOTHING_REQUESTED_FAILS) {
                BOOST_CHECK_MESSAGE(!sock.Wait(0ms, requested), s.Name(requested));
            } else {
                const Sock::Event occurred{AwaitEvents(sock, requested, s.Expect(requested).must)};
                s.Check(requested, occurred);
                s.occurred_any |= occurred;
            }
            Sock::Events events{requested};
            events.occurred = 0xff; // Make sure WaitMany overwrites it, rather than add to it.
            events_per_sock.emplace(s.conn->sock, events);
        }

        // All of them in a single WaitMany(). Everything that must occur is ready already.
        const bool ok{listener.sock.WaitMany(0ms, events_per_sock)};
        if (requested == 0 && NOTHING_REQUESTED_FAILS) {
            BOOST_CHECK(!ok);
            continue;
        }
        BOOST_REQUIRE(ok);
        for (const auto& s : scenarios) {
            const Sock::Event occurred{events_per_sock.at(s.conn->sock).occurred};
            s.Check(requested, occurred, " (WaitMany)");
        }
    }

    for (const auto& s : scenarios) {
        const std::string name{s.Name()};
        const auto& sock{*s.conn->sock};

        char c;
        const ssize_t received{sock.Recv(&c, 1, MSG_PEEK)};
        if (s.occurred_any & Sock::RecvEvent) {
            BOOST_CHECK_MESSAGE(Progressed(received), name);
        }
        if (!s.data && s.EndOfStream()) {
            BOOST_CHECK_MESSAGE(received == 0, name << ": recv() returned " << received << " errno " << WSAGetLastError());
        } else if (s.Reset()) {
            // Data received before a reset may or may not be returned first, depending on the OS.
            BOOST_CHECK_MESSAGE(Progressed(received), name);
        } else if (s.data) {
            BOOST_CHECK_MESSAGE(received == 1 && c == 'd', name);
        } else {
            BOOST_CHECK_MESSAGE(WouldBlock(received), name);
        }

        if (s.occurred_any & Sock::SendEvent) {
            BOOST_CHECK_MESSAGE(Progressed(sock.Send("x", 1, MSG_NOSIGNAL)), name);
        }
    }

    // Not tested, as the timing of signals is racy: a wait interrupted by a signal (EINTR).
}

/** Check that WaitMany reports each socket's own events, for many sockets in random states. */
BOOST_AUTO_TEST_CASE(wait_many_attribution)
{
    enum class State { IDLE, DATA, END_OF_STREAM };
    struct Entry {
        State state;
        Sock::Event requested;
        std::unique_ptr<Connection> conn;
    };

    // Create a hundred connections to a single Sock listener, in various states.
    Listener listener;
    std::vector<Entry> entries;
    bool nothing_requested{true};
    for (int i{0}; i < 100; ++i) {
        const auto state{static_cast<State>(m_rng.randrange(3))};
        const auto requested{static_cast<Sock::Event>(m_rng.randrange(4))};
        if (nothing_requested && requested != 0) nothing_requested = false;
        entries.push_back({state, requested, std::make_unique<Connection>(listener)});
        if (state == State::DATA) entries.back().conn->PeerSend("data");
        if (state == State::END_OF_STREAM) entries.back().conn->PeerShutdown();
    }
    // At least one socket must request something, see NOTHING_REQUESTED_FAILS.
    if (nothing_requested) entries.back().requested = Sock::SendEvent;

    // Make sure all events setting up the state have arrived.
    for (const auto& e : entries) {
        if (e.state != State::IDLE) e.conn->AwaitReadable();
    }

    Sock::EventsPerSock events_per_sock;
    for (const auto& e : entries) {
        events_per_sock.emplace(e.conn->sock, Sock::Events{e.requested});
    }
    BOOST_REQUIRE(listener.sock.WaitMany(LONG_WAIT, events_per_sock));
    for (const auto& e : entries) {
        Sock::Event possible{Sock::SendEvent};
        if (e.state != State::IDLE) possible |= Sock::RecvEvent;
        const auto expected{static_cast<Sock::Event>(e.requested & possible)};
        BOOST_CHECK_EQUAL(EventsStr(events_per_sock.at(e.conn->sock).occurred), EventsStr(expected));
    }
}

BOOST_AUTO_TEST_SUITE_END()
