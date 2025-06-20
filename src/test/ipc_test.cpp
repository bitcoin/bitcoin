// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <ipc/capnp/protocol.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <logging.h>
#include <mp/proxy-types.h>
#include <test/ipc_test.capnp.h>
#include <test/ipc_test.capnp.proxy.h>
#include <test/ipc_test.h>
#include <tinyformat.h>
#include <validation.h>

#include <future>
#include <thread>
#include <kj/common.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

//! Remote init class.
class TestInit : public interfaces::Init
{
public:
    std::unique_ptr<interfaces::Echo> makeEcho() override { return interfaces::MakeEcho(); }
};

//! Generate a temporary path with temp_directory_path and mkstemp
static std::string TempPath(std::string_view pattern)
{
    std::string temp{fs::PathToString(fs::path{fs::temp_directory_path()} / fs::PathFromString(std::string{pattern}))};
    temp.push_back('\0');
    int fd{mkstemp(temp.data())};
    BOOST_CHECK_GE(fd, 0);
    BOOST_CHECK_EQUAL(close(fd), 0);
    temp.resize(temp.size() - 1);
    fs::remove(fs::PathFromString(temp));
    return temp;
}

//! Unit test that tests execution of IPC calls without actually creating a
//! separate process. This test is primarily intended to verify behavior of type
//! conversion code that converts C++ objects to Cap'n Proto messages and vice
//! versa.
//!
//! The test creates a thread which creates a FooImplementation object (defined
//! in ipc_test.h) and a two-way pipe accepting IPC requests which call methods
//! on the object through FooInterface (defined in ipc_test.capnp).
void IpcPipeTest()
{
    // Setup: create FooImplementation object and listen for FooInterface requests
    std::promise<std::unique_ptr<mp::ProxyClient<gen::FooInterface>>> foo_promise;
    std::function<void()> disconnect_client;
    std::thread thread([&]() {
        mp::EventLoop loop("IpcPipeTest", [](bool raise, const std::string& log) { LogPrintf("LOG%i: %s\n", raise, log); });
        auto pipe = loop.m_io_context.provider->newTwoWayPipe();

        auto connection_client = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[0]));
        auto foo_client = std::make_unique<mp::ProxyClient<gen::FooInterface>>(
            connection_client->m_rpc_system->bootstrap(mp::ServerVatId().vat_id).castAs<gen::FooInterface>(),
            connection_client.get(), /* destroy_connection= */ false);
        foo_promise.set_value(std::move(foo_client));
        disconnect_client = [&] { loop.sync([&] { connection_client.reset(); }); };

        auto connection_server = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[1]), [&](mp::Connection& connection) {
            auto foo_server = kj::heap<mp::ProxyServer<gen::FooInterface>>(std::make_shared<FooImplementation>(), connection);
            return capnp::Capability::Client(kj::mv(foo_server));
        });
        connection_server->onDisconnect([&] { connection_server.reset(); });
        loop.loop();
    });
    std::unique_ptr<mp::ProxyClient<gen::FooInterface>> foo{foo_promise.get_future().get()};

    // Test: make sure arguments were sent and return value is received
    BOOST_CHECK_EQUAL(foo->add(1, 2), 3);

    COutPoint txout1{Txid::FromUint256(uint256{100}), 200};
    COutPoint txout2{foo->passOutPoint(txout1)};
    BOOST_CHECK(txout1 == txout2);

    UniValue uni1{UniValue::VOBJ};
    uni1.pushKV("i", 1);
    uni1.pushKV("s", "two");
    UniValue uni2{foo->passUniValue(uni1)};
    BOOST_CHECK_EQUAL(uni1.write(), uni2.write());

    CMutableTransaction mtx;
    mtx.version = 2;
    mtx.nLockTime = 3;
    mtx.vin.emplace_back(txout1);
    mtx.vout.emplace_back(COIN, CScript());
    CTransactionRef tx1{MakeTransactionRef(mtx)};
    CTransactionRef tx2{foo->passTransaction(tx1)};
    BOOST_CHECK(*Assert(tx1) == *Assert(tx2));

    std::vector<char> vec1{'H', 'e', 'l', 'l', 'o'};
    std::vector<char> vec2{foo->passVectorChar(vec1)};
    BOOST_CHECK_EQUAL(std::string_view(vec1.begin(), vec1.end()), std::string_view(vec2.begin(), vec2.end()));

    auto script1{CScript() << OP_11};
    auto script2{foo->passScript(script1)};
    BOOST_CHECK_EQUAL(HexStr(script1), HexStr(script2));

    // Test cleanup: disconnect pipe and join thread
    disconnect_client();
    thread.join();
}

//! Test ipc::Protocol connect() and serve() methods connecting over a socketpair.
void IpcSocketPairTest()
{
    int fds[2];
    BOOST_CHECK_EQUAL(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    std::unique_ptr<interfaces::Init> init{std::make_unique<TestInit>()};
    std::unique_ptr<ipc::Protocol> protocol{ipc::capnp::MakeCapnpProtocol()};
    std::promise<void> promise;
    std::thread thread([&]() {
        protocol->serve(fds[0], "test-serve", *init, [&] { promise.set_value(); });
    });
    promise.get_future().wait();
    std::unique_ptr<interfaces::Init> remote_init{protocol->connect(fds[1], "test-connect")};
    std::unique_ptr<interfaces::Echo> remote_echo{remote_init->makeEcho()};
    BOOST_CHECK_EQUAL(remote_echo->echo("echo test"), "echo test");
    remote_echo.reset();
    remote_init.reset();
    thread.join();
}

//! Test ipc::Process bind() and connect() methods connecting over a unix socket.
void IpcSocketTest(const fs::path& datadir)
{
    std::unique_ptr<interfaces::Init> init{std::make_unique<TestInit>()};
    std::unique_ptr<ipc::Protocol> protocol{ipc::capnp::MakeCapnpProtocol()};
    std::unique_ptr<ipc::Process> process{ipc::MakeProcess()};

    std::string invalid_bind{"invalid:"};
    BOOST_CHECK_THROW(process->bind(datadir, "test_bitcoin", invalid_bind), std::invalid_argument);
    BOOST_CHECK_THROW(process->connect(datadir, "test_bitcoin", invalid_bind), std::invalid_argument);

    auto bind_and_listen{[&](const std::string& bind_address) {
        std::string address{bind_address};
        int serve_fd = process->bind(datadir, "test_bitcoin", address);
        BOOST_CHECK_GE(serve_fd, 0);
        BOOST_CHECK_EQUAL(address, bind_address);
        protocol->listen(serve_fd, "test-serve", *init);
    }};

    auto connect_and_test{[&](const std::string& connect_address) {
        std::string address{connect_address};
        int connect_fd{process->connect(datadir, "test_bitcoin", address)};
        BOOST_CHECK_EQUAL(address, connect_address);
        std::unique_ptr<interfaces::Init> remote_init{protocol->connect(connect_fd, "test-connect")};
        std::unique_ptr<interfaces::Echo> remote_echo{remote_init->makeEcho()};
        BOOST_CHECK_EQUAL(remote_echo->echo("echo test"), "echo test");
    }};

    // Need to specify explicit socket addresses outside the data directory, because the data
    // directory path is so long that the default socket address and any other
    // addresses in the data directory would fail with errors like:
    //   Address 'unix' path '"/tmp/test_common_Bitcoin Core/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff/test_bitcoin.sock"' exceeded maximum socket path length
    std::vector<std::string> addresses{
        strprintf("unix:%s", TempPath("bitcoin_sock0_XXXXXX")),
        strprintf("unix:%s", TempPath("bitcoin_sock1_XXXXXX")),
    };

    // Bind and listen on multiple addresses
    for (const auto& address : addresses) {
        bind_and_listen(address);
    }

    // Connect and test each address multiple times.
    for (int i : {0, 1, 0, 0, 1}) {
        connect_and_test(addresses[i]);
    }
}
