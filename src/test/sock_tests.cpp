// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <compat/compat.h>
#include <test/util/setup_common.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <thread>

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

struct tcp_socket_pair {
    std::unique_ptr<Sock> sender{};
    std::unique_ptr<Sock> receiver{};

    static tcp_socket_pair create(bool connect = true)
    {
        tcp_socket_pair socks{};
        socks.sender = std::make_unique<Sock>(CreateSocket());
        socks.receiver = std::make_unique<Sock>(CreateSocket());
        if (connect) socks.connect();

        return socks;
    }

    void connect()
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        BOOST_REQUIRE_EQUAL(receiver->Bind(reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
        BOOST_REQUIRE_EQUAL(receiver->Listen(1), 0);

        // Get the address of the listener.
        sockaddr_in bound{};
        socklen_t blen = sizeof(bound);
        BOOST_REQUIRE_EQUAL(receiver->GetSockName(reinterpret_cast<sockaddr*>(&bound), &blen), 0);
        BOOST_REQUIRE_EQUAL(blen, sizeof(bound));

        // Sender attempts to initiate connection to listener.
        BOOST_REQUIRE_EQUAL(sender->Connect(reinterpret_cast<sockaddr*>(&bound), sizeof(bound)), 0);

        // Listener accepts connection.
        std::unique_ptr<Sock> accepted = receiver->Accept(nullptr, nullptr);
        // The call to accept(2) succeeded.
        BOOST_REQUIRE(accepted != nullptr);

        receiver = std::move(accepted);
    }

    void send_and_receive()
    {
        const char* msg = "abcd";
        constexpr ssize_t msg_len = 4;
        char recv_buf[10];

        BOOST_CHECK_EQUAL(sender->Send(msg, msg_len, 0), msg_len);
        BOOST_CHECK_EQUAL(receiver->Recv(recv_buf, sizeof(recv_buf), 0), msg_len);
        BOOST_CHECK_EQUAL(strncmp(msg, recv_buf, msg_len), 0);
    }
};

BOOST_AUTO_TEST_CASE(send_and_receive)
{
    tcp_socket_pair socks = tcp_socket_pair::create(/*connect=*/true);
    socks.send_and_receive();

    // Sockets are still connected after being moved.
    tcp_socket_pair socks_moved = std::move(socks);
    socks_moved.send_and_receive();
}

BOOST_AUTO_TEST_CASE(wait)
{
    tcp_socket_pair socks = tcp_socket_pair::create(/*connect=*/true);

    std::thread waiter([&socks]() { (void)socks.receiver->Wait(24h, Sock::RECV); });

    BOOST_REQUIRE_EQUAL(socks.sender->Send("a", 1, 0), 1);

    waiter.join();
}

BOOST_AUTO_TEST_CASE(recv_until_terminator_limit)
{
    constexpr auto timeout = 1min; // High enough so that it is never hit.
    CThreadInterrupt interrupt;

    tcp_socket_pair socks = tcp_socket_pair::create(/*connect=*/true);

    std::thread receiver([&socks, &timeout, &interrupt]() {
        constexpr size_t max_data{10};
        bool threw_as_expected{false};
        // BOOST_CHECK_EXCEPTION() writes to some variables shared with the main thread which
        // creates a data race. So mimic it manually.
        try {
            (void)socks.receiver->RecvUntilTerminator('\n', timeout, interrupt, max_data);
        } catch (const std::runtime_error& e) {
            threw_as_expected = HasReason("too many bytes without a terminator")(e);
        }
        assert(threw_as_expected);
    });

    BOOST_REQUIRE_NO_THROW(socks.sender->SendComplete("1234567", timeout, interrupt));
    BOOST_REQUIRE_NO_THROW(socks.sender->SendComplete("89a\n", timeout, interrupt));

    receiver.join();
}

BOOST_AUTO_TEST_SUITE_END()
