// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <capnp/capability.h>
#include <capnp/rpc.h>
#include <kj/memory.h>
#include <mp/proxy-io.h>
#include <mp/proxy.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.proxy.h>
#include <ipc/test/fuzz/ipc_fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <future>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
class IpcFuzzSetup
{
public:
    IpcFuzzSetup()
    {
        std::promise<std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>> client_promise;
        auto client_future{client_promise.get_future()};
        m_loop_thread = std::thread([&client_promise] {
            mp::EventLoop loop("ipc-fuzz", [](mp::LogMessage message) {
                if (message.level == mp::Log::Raise) throw std::runtime_error(message.message);
            });
            auto pipe = loop.m_io_context.provider->newTwoWayPipe();

            auto server_connection = std::make_unique<mp::Connection>(
                loop,
                kj::mv(pipe.ends[0]),
                [&](mp::Connection& connection) {
                    auto server_proxy = kj::heap<mp::ProxyServer<test::fuzz::messages::IpcFuzzInterface>>(
                        std::make_shared<IpcFuzzImplementation>(), connection);
                    return capnp::Capability::Client(kj::mv(server_proxy));
                });
            server_connection->onDisconnect([&] { server_connection.reset(); });

            auto client_connection = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[1]));
            auto client_proxy = std::make_unique<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>(
                client_connection->m_rpc_system->bootstrap(mp::ServerVatId().vat_id)
                    .castAs<test::fuzz::messages::IpcFuzzInterface>(),
                client_connection.get(),
                /* destroy_connection= */ true);
            (void)client_connection.release();

            client_promise.set_value(std::move(client_proxy));
            loop.loop();
        });
        m_client = client_future.get();
    }

    ~IpcFuzzSetup()
    {
        m_client.reset();
        if (m_loop_thread.joinable()) m_loop_thread.join();
    }

    std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>> m_client;

private:
    std::thread m_loop_thread;
};

static IpcFuzzSetup* g_ipc;

static void initialize_ipc()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    (void)testing_setup;

    // Ensure g_thread_context is destroyed after the IPC setup, since C++
    // destroys thread_local objects in reverse construction order.
    mp::ThreadContext& thread_context{mp::g_thread_context};
    (void)thread_context;

    thread_local static IpcFuzzSetup ipc; // NOLINT(bitcoin-nontrivial-threadlocal)
    g_ipc = &ipc;
}

FUZZ_TARGET(ipc, .init = initialize_ipc)
{
    auto& ipc = *g_ipc;
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 64) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                static constexpr int MIN_ADD{-1'000'000};
                static constexpr int MAX_ADD{1'000'000};
                const int a = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                const int b = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                assert(ipc.m_client->add(a, b) == a + b);
            },
            [&] {
                COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)),
                                   fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                COutPoint expected{outpoint.hash, outpoint.n ^ 0xFFFFFFFFu};
                assert(ipc.m_client->passOutPoint(outpoint) == expected);
            },
            [&] {
                std::vector<uint8_t> value = ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512);
                std::vector<uint8_t> expected{value.rbegin(), value.rend()};
                assert(ipc.m_client->passVectorUint8(value) == expected);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                CScript expected{script};
                expected << OP_NOP;
                assert(ipc.m_client->passScript(script) == expected);
            },
            [&] {
                UniValue value;
                if (!value.read(fuzzed_data_provider.ConsumeRandomLengthString(512))) return;
                assert(ipc.m_client->passUniValue(value).write() == value.write());
            },
            [&] {
                const CMutableTransaction mutable_tx = ConsumeTransaction(fuzzed_data_provider, std::nullopt);
                if (mutable_tx.vin.empty()) return;
                const CTransactionRef tx = MakeTransactionRef(mutable_tx);
                assert(*ipc.m_client->passTransaction(tx) == *tx);
            });
    }
}
} // namespace
