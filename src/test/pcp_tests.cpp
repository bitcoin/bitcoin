// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/pcp.h>
#include <netbase.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <deque>

using namespace std::literals;

/// UDP test server operation.
struct TestOp {
    std::chrono::milliseconds delay;
    enum Op {
        SEND, // Expect send (with optional data)
        RECV, // Expect receive (with data)
        NOP,  // Just delay
    } op;
    std::vector<uint8_t> data;

    //! Injected error.
    //! Set this field to a non-zero value to return the errno code from send or receive operation.
    int error;

    TestOp(std::chrono::milliseconds delay_in, Op op_in, const std::vector<uint8_t> &data_in, int error_in):
        delay(delay_in), op(op_in), data(data_in), error(error_in) {}
};

/// Save the value of CreateSock and restore when the test ends.
class PCPTestingSetup : public BasicTestingSetup
{
public:
    explicit PCPTestingSetup(const ChainType chainType = ChainType::MAIN,
                             TestOpts opts = {})
        : BasicTestingSetup{chainType, opts},
          m_create_sock_orig{CreateSock}
    {
        const std::optional<CService> local_ipv4{Lookup("192.168.0.6", 1, false)};
        const std::optional<CService> local_ipv6{Lookup("2a10:1234:5678:9abc:def0:1234:5678:9abc", 1, false)};
        const std::optional<CService> gateway_ipv4{Lookup("192.168.0.1", 1, false)};
        const std::optional<CService> gateway_ipv6{Lookup("2a10:1234:5678:9abc:def0:0000:0000:0000", 1, false)};
        BOOST_REQUIRE(local_ipv4 && local_ipv6 && gateway_ipv4 && gateway_ipv6);
        default_local_ipv4 = *local_ipv4;
        default_local_ipv6 = *local_ipv6;
        default_gateway_ipv4 = *gateway_ipv4;
        default_gateway_ipv6 = *gateway_ipv6;

        struct in_addr inaddr_any;
        inaddr_any.s_addr = htonl(INADDR_ANY);
        bind_any_ipv4 = CNetAddr(inaddr_any);
    }

    ~PCPTestingSetup()
    {
        CreateSock = m_create_sock_orig;
        MockableSteadyClock::ClearMockTime();
    }

    // Default testing nonce.
    static constexpr PCPMappingNonce TEST_NONCE{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    // Default network addresses.
    CNetAddr default_local_ipv4;
    CNetAddr default_local_ipv6;
    CNetAddr default_gateway_ipv4;
    CNetAddr default_gateway_ipv6;
    // IPv4 bind
    CNetAddr bind_any_ipv4;
private:
    const decltype(CreateSock) m_create_sock_orig;
};

/** Simple scripted UDP server emulation for testing.
 */
class PCPTestSock final : public Sock
{
public:
    // Note: we awkwardly mark all methods as const, and properties as mutable,
    // because Sock expects all networking calls to be const.
    explicit PCPTestSock(const CNetAddr &local_ip, const CNetAddr &gateway_ip, const std::vector<TestOp> &script)
        : Sock{INVALID_SOCKET},
          m_script(script),
          m_local_ip(local_ip),
          m_gateway_ip(gateway_ip)
    {
        ElapseTime(std::chrono::seconds(0)); // start mocking steady time
        PrepareOp();
    }

    PCPTestSock& operator=(Sock&& other) override
    {
        assert(false && "Move of Sock into PCPTestSock not allowed.");
        return *this;
    }

    ssize_t Send(const void* data, size_t len, int) const override {
        if (!m_connected) return -1;
        std::span in_pkt = std::span(static_cast<const uint8_t*>(data), len);
        if (AtEndOfScript() || CurOp().op != TestOp::SEND) {
            // Ignore sends after end of script, or sends when we expect a receive.
            FailScript();
            return len;
        }
        if (CurOp().error) return -1; // Inject failure
        if (CurOp().data.empty() || std::ranges::equal(CurOp().data, in_pkt)) {
            AdvanceOp();
        } else {
            // Wrong send, fail script
            FailScript();
        }
        return len;
    }

    ssize_t Recv(void* buf, size_t len, int flags) const override
    {
        if (!m_connected || AtEndOfScript() || CurOp().op != TestOp::RECV || m_time_left != 0s) {
            return -1;
        }
        if (CurOp().error) return -1; // Inject failure
        const auto &recv_pkt = CurOp().data;
        const size_t consume_bytes{std::min(len, recv_pkt.size())};
        std::memcpy(buf, recv_pkt.data(), consume_bytes);
        if ((flags & MSG_PEEK) == 0) {
            AdvanceOp();
        }
        return consume_bytes;
    }

    int Connect(const sockaddr* sa, socklen_t sa_len) const override {
        CService service;
        if (service.SetSockAddr(sa, sa_len) && service == CService(m_gateway_ip, 5351)) {
            if (m_bound.IsBindAny()) { // If bind-any, bind to local ip.
                m_bound = CService(m_local_ip, 0);
            }
            if (m_bound.GetPort() == 0) { // If no port assigned, assign port 1.
                m_bound = CService(m_bound, 1);
            }
            m_connected = true;
            return 0;
        }
        return -1;
    }

    int Bind(const sockaddr* sa, socklen_t sa_len) const override {
        CService service;
        if (service.SetSockAddr(sa, sa_len)) {
            // Can only bind to one of our local ips
            if (!service.IsBindAny() && service != m_local_ip) {
                return -1;
            }
            m_bound = service;
            return 0;
        }
        return -1;
    }

    int Listen(int) const override { return -1; }

    std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const override
    {
        return nullptr;
    };

    int GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const override
    {
        return -1;
    }

    int SetSockOpt(int, int, const void*, socklen_t) const override { return 0; }

    int GetSockName(sockaddr* name, socklen_t* name_len) const override
    {
        // Return the address we've been bound to.
        return m_bound.GetSockAddr(name, name_len) ? 0 : -1;
    }

    bool SetNonBlocking() const override { return true; }

    bool IsSelectable() const override { return true; }

    bool Wait(std::chrono::milliseconds timeout,
              Event requested,
              Event* occurred = nullptr) const override
    {
        // Only handles receive events.
        if (AtEndOfScript() || requested != Sock::RECV) {
            ElapseTime(timeout);
        } else {
            std::chrono::milliseconds delay = std::min(m_time_left, timeout);
            ElapseTime(delay);
            m_time_left -= delay;
            if (CurOp().op == TestOp::RECV && m_time_left == 0s && occurred != nullptr) {
                *occurred = Sock::RECV;
            }
            if (CurOp().op == TestOp::NOP) {
                // This was a pure delay operation, move to the next op.
                AdvanceOp();
            }
        }
        return true;
    }

    bool WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const override
    {
        return false;
    }

    bool IsConnected(std::string&) const override
    {
        return true;
    }

private:
    const std::vector<TestOp> m_script;
    mutable size_t m_script_ptr = 0;
    mutable std::chrono::milliseconds m_time_left;
    mutable std::chrono::milliseconds m_time{MockableSteadyClock::INITIAL_MOCK_TIME};
    mutable bool m_connected{false};
    mutable CService m_bound;
    mutable CNetAddr m_local_ip;
    mutable CNetAddr m_gateway_ip;

    void ElapseTime(std::chrono::milliseconds duration) const
    {
        m_time += duration;
        MockableSteadyClock::SetMockTime(m_time);
    }

    bool AtEndOfScript() const { return m_script_ptr == m_script.size(); }
    const TestOp &CurOp() const {
        BOOST_REQUIRE(m_script_ptr < m_script.size());
        return m_script[m_script_ptr];
    }

    void PrepareOp() const {
        if (AtEndOfScript()) return;
        m_time_left = CurOp().delay;
    }

    void AdvanceOp() const
    {
        m_script_ptr += 1;
        PrepareOp();
    }

    void FailScript() const { m_script_ptr = m_script.size(); }
};

BOOST_FIXTURE_TEST_SUITE(pcp_tests, PCPTestingSetup)

// NAT-PMP IPv4 good-weather scenario.
BOOST_AUTO_TEST_CASE(natpmp_ipv4)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x00, 0x00, // version, opcode (request external IP)
            }, 0
        },
        {
            2ms, TestOp::RECV,
            {
                0x00, 0x80, 0x00, 0x00, // version, opcode (external IP), result code (success)
                0x66, 0xfd, 0xa1, 0xee, // seconds sinds start of epoch
                0x01, 0x02, 0x03, 0x04, // external IP address
            }, 0
        },
        {
            0ms, TestOp::SEND,
            {
                0x00, 0x02, 0x00, 0x00, // version, opcode (request map TCP)
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x03, 0xe8, // requested mapping lifetime in seconds
            }, 0
        },
        {
            2ms, TestOp::RECV,
            {
                0x00, 0x82, 0x00, 0x00, // version, opcode (mapped TCP)
                0x66, 0xfd, 0xa1, 0xee, // seconds sinds start of epoch
                0x04, 0xd2, 0x04, 0xd2, // internal port, mapped external port
                0x00, 0x00, 0x01, 0xf4, // mapping lifetime in seconds
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = NATPMPRequestPortMap(default_gateway_ipv4, 1234, 1000, 1, 200ms);

    MappingResult* mapping = std::get_if<MappingResult>(&res);
    BOOST_REQUIRE(mapping);
    BOOST_CHECK_EQUAL(mapping->version, 0);
    BOOST_CHECK_EQUAL(mapping->internal.ToStringAddrPort(), "192.168.0.6:1234");
    BOOST_CHECK_EQUAL(mapping->external.ToStringAddrPort(), "1.2.3.4:1234");
    BOOST_CHECK_EQUAL(mapping->lifetime, 500);
}

// PCP IPv4 good-weather scenario.
BOOST_AUTO_TEST_CASE(pcp_ipv4)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0xa8, 0x00, 0x06, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, // suggested external IP
            }, 0
        },
        {
            250ms, TestOp::RECV, // 250ms delay before answer
            {
                0x02, 0x81, 0x00, 0x00, // version, opcode, result success
                0x00, 0x00, 0x01, 0xf4, // granted lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, assigned external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0x02, 0x03, 0x04, // assigned external IP
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 1, 1000ms);

    MappingResult* mapping = std::get_if<MappingResult>(&res);
    BOOST_REQUIRE(mapping);
    BOOST_CHECK_EQUAL(mapping->version, 2);
    BOOST_CHECK_EQUAL(mapping->internal.ToStringAddrPort(), "192.168.0.6:1234");
    BOOST_CHECK_EQUAL(mapping->external.ToStringAddrPort(), "1.2.3.4:1234");
    BOOST_CHECK_EQUAL(mapping->lifetime, 500);
}

// PCP IPv6 good-weather scenario.
BOOST_AUTO_TEST_CASE(pcp_ipv6)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // suggested external IP
            }, 0
        },
        {
            500ms, TestOp::RECV, // 500ms delay before answer
            {
                0x02, 0x81, 0x00, 0x00, // version, opcode, result success
                0x00, 0x00, 0x01, 0xf4, // granted lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, assigned external port
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // suggested external IP
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET6 && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv6, default_gateway_ipv6, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv6, default_local_ipv6, 1234, 1000, 1, 1000ms);

    MappingResult* mapping = std::get_if<MappingResult>(&res);
    BOOST_REQUIRE(mapping);
    BOOST_CHECK_EQUAL(mapping->version, 2);
    BOOST_CHECK_EQUAL(mapping->internal.ToStringAddrPort(), "[2a10:1234:5678:9abc:def0:1234:5678:9abc]:1234");
    BOOST_CHECK_EQUAL(mapping->external.ToStringAddrPort(), "[2a10:1234:5678:9abc:def0:1234:5678:9abc]:1234");
    BOOST_CHECK_EQUAL(mapping->lifetime, 500);
}

// PCP timeout.
BOOST_AUTO_TEST_CASE(pcp_timeout)
{
    const std::vector<TestOp> script{};
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    ASSERT_DEBUG_LOG("pcp: Retrying (1)");
    ASSERT_DEBUG_LOG("pcp: Retrying (2)");
    ASSERT_DEBUG_LOG("pcp: Timeout");

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 3, 2000ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::NETWORK_ERROR);
}

// PCP failure receiving (router sends ICMP port closed).
BOOST_AUTO_TEST_CASE(pcp_connrefused)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            { // May send anything.
            }, 0
        },
        {
            0ms, TestOp::RECV,
            {
            }, ECONNREFUSED
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    ASSERT_DEBUG_LOG("pcp: Could not receive response");

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 3, 2000ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::NETWORK_ERROR);
}

// PCP IPv6 success after one timeout.
BOOST_AUTO_TEST_CASE(pcp_ipv6_timeout_success)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // suggested external IP
            }, 0
        },
        {
            2001ms, TestOp::NOP, // Takes longer to respond than timeout of 2000ms
            {}, 0
        },
        {
            0ms, TestOp::SEND, // Repeated send (try 2)
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // suggested external IP
            }, 0
        },
        {
            200ms, TestOp::RECV, // This time we're in time
            {
                0x02, 0x81, 0x00, 0x00, // version, opcode, result success
                0x00, 0x00, 0x01, 0xf4, // granted lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, assigned external port
                0x2a, 0x10, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // suggested external IP
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET6 && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv6, default_gateway_ipv6, script);
        return std::unique_ptr<PCPTestSock>();
    };

    ASSERT_DEBUG_LOG("pcp: Retrying (1)");
    ASSERT_DEBUG_LOG("pcp: Timeout");

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv6, default_local_ipv6, 1234, 1000, 2, 2000ms);

    BOOST_CHECK(std::get_if<MappingResult>(&res));
}

// PCP IPv4 failure (no resources).
BOOST_AUTO_TEST_CASE(pcp_ipv4_fail_no_resources)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0xa8, 0x00, 0x06, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, // suggested external IP
            }, 0
        },
        {
            500ms, TestOp::RECV,
            {
                0x02, 0x81, 0x00, 0x08, // version, opcode, result 0x08: no resources
                0x00, 0x00, 0x00, 0x00, // granted lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x00, 0x00, // internal port, assigned external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // assigned external IP
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 3, 1000ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::NO_RESOURCES);
}

// PCP IPv4 failure (test NATPMP downgrade scenario).
BOOST_AUTO_TEST_CASE(pcp_ipv4_fail_unsupported_version)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0xa8, 0x00, 0x06, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, // suggested external IP
            }, 0
        },
        {
            500ms, TestOp::RECV,
            {
                0x00, 0x81, 0x00, 0x01, // version, opcode, result 0x01: unsupported version
                0x00, 0x00, 0x00, 0x00,
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 3, 1000ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::UNSUPP_VERSION);
}

// NAT-PMP IPv4 protocol error scenarii.
BOOST_AUTO_TEST_CASE(natpmp_protocol_error)
{
    // First scenario: non-0 result code when requesting external IP.
    std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x00, 0x00, // version, opcode (request external IP)
            }, 0
        },
        {
            2ms, TestOp::RECV,
            {
                0x00, 0x80, 0x00, 0x42, // version, opcode (external IP), result code (*NOT* success)
                0x66, 0xfd, 0xa1, 0xee, // seconds sinds start of epoch
                0x01, 0x02, 0x03, 0x04, // external IP address
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = NATPMPRequestPortMap(default_gateway_ipv4, 1234, 1000, 1, 200ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::PROTOCOL_ERROR);

    // First scenario: non-0 result code when requesting port mapping.
    script = {
        {
            0ms, TestOp::SEND,
            {
                0x00, 0x00, // version, opcode (request external IP)
            }, 0
        },
        {
            2ms, TestOp::RECV,
            {
                0x00, 0x80, 0x00, 0x00, // version, opcode (external IP), result code (success)
                0x66, 0xfd, 0xa1, 0xee, // seconds sinds start of epoch
                0x01, 0x02, 0x03, 0x04, // external IP address
            }, 0
        },
        {
            0ms, TestOp::SEND,
            {
                0x00, 0x02, 0x00, 0x00, // version, opcode (request map TCP)
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x03, 0xe8, // requested mapping lifetime in seconds
            }, 0
        },
        {
            2ms, TestOp::RECV,
            {
                0x00, 0x82, 0x00, 0x43, // version, opcode (mapped TCP)
                0x66, 0xfd, 0xa1, 0xee, // seconds sinds start of epoch
                0x04, 0xd2, 0x04, 0xd2, // internal port, mapped external port
                0x00, 0x00, 0x01, 0xf4, // mapping lifetime in seconds
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    res = NATPMPRequestPortMap(default_gateway_ipv4, 1234, 1000, 1, 200ms);

    err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::PROTOCOL_ERROR);
}

// PCP IPv4 protocol error scenario.
BOOST_AUTO_TEST_CASE(pcp_protocol_error)
{
    const std::vector<TestOp> script{
        {
            0ms, TestOp::SEND,
            {
                0x02, 0x01, 0x00, 0x00, // version, opcode
                0x00, 0x00, 0x03, 0xe8, // lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0xa8, 0x00, 0x06, // internal IP
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, suggested external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, // suggested external IP
            }, 0
        },
        {
            250ms, TestOp::RECV, // 250ms delay before answer
            {
                0x02, 0x81, 0x00, 0x42, // version, opcode, result error
                0x00, 0x00, 0x01, 0xf4, // granted lifetime
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, // nonce
                0x06, 0x00, 0x00, 0x00, // protocol (TCP), reserved
                0x04, 0xd2, 0x04, 0xd2, // internal port, assigned external port
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0x02, 0x03, 0x04, // assigned external IP
            }, 0
        },
    };
    CreateSock = [this, &script](int domain, int type, int protocol) {
        if (domain == AF_INET && type == SOCK_DGRAM && protocol == IPPROTO_UDP) return std::make_unique<PCPTestSock>(default_local_ipv4, default_gateway_ipv4, script);
        return std::unique_ptr<PCPTestSock>();
    };

    auto res = PCPRequestPortMap(TEST_NONCE, default_gateway_ipv4, bind_any_ipv4, 1234, 1000, 1, 1000ms);

    MappingError* err = std::get_if<MappingError>(&res);
    BOOST_REQUIRE(err);
    BOOST_CHECK_EQUAL(*err, MappingError::PROTOCOL_ERROR);
}

BOOST_AUTO_TEST_SUITE_END()

