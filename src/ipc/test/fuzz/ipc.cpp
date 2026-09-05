// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/capability.h>
#include <capnp/rpc.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.proxy.h>
#include <ipc/test/fuzz/ipc_fuzz.h>
#include <ipc/util.h>
#include <kj/async.h>
#include <kj/memory.h>
#include <mp/proxy-io.h>
#include <mp/proxy.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <exception>
#include <future>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {
// Callback invoked by the server to exercise IPC communication in the
// server to client direction.
class FuzzCallback final : public IpcFuzzCallback
{
public:
    FuzzCallback(int expected_arg, int result) : m_expected_arg{expected_arg}, m_result{result} {}

    int call(int arg) override
    {
        assert(arg == m_expected_arg);
        return m_result;
    }

private:
    const int m_expected_arg;
    const int m_result;
};

struct MalformedResponseState
{
    std::vector<uint8_t> transaction;
    std::string univalue;
};

// Return arbitrary payloads to exercise deserialization by ProxyClient.
class MalformedResponseServer final : public test::fuzz::messages::IpcFuzzInterface::Server
{
public:
    explicit MalformedResponseServer(std::shared_ptr<MalformedResponseState> state) : m_state{std::move(state)} {}

    kj::Promise<void> passTransaction(PassTransactionContext context) override
    {
        context.getResults().setResult(kj::arrayPtr(m_state->transaction.data(), m_state->transaction.size()));
        return kj::READY_NOW;
    }

    kj::Promise<void> passUniValue(PassUniValueContext context) override
    {
        context.getResults().setResult(kj::StringPtr(m_state->univalue.data(), m_state->univalue.size()));
        return kj::READY_NOW;
    }

private:
    std::shared_ptr<MalformedResponseState> m_state;
};

class IpcFuzzSetup
{
public:
    IpcFuzzSetup()
    {
        std::promise<std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>> client_promise;
        std::promise<std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>> response_client_promise;
        auto client_future{client_promise.get_future()};
        auto response_client_future{response_client_promise.get_future()};
        m_loop_thread = std::thread([&client_promise, &response_client_promise, impl = m_impl, response_state = m_response_state] {
            mp::EventLoop loop("ipc-fuzz", [](mp::LogMessage message) {
                if (message.level == mp::Log::Raise) throw std::runtime_error(message.message);
            });
            auto pipe = loop.m_io_context.provider->newTwoWayPipe();

            auto server_connection = std::make_unique<mp::Connection>(
                loop,
                kj::mv(pipe.ends[0]),
                [&](mp::Connection& connection) {
                    auto server_proxy = kj::heap<mp::ProxyServer<test::fuzz::messages::IpcFuzzInterface>>(impl, connection);
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

            auto response_pipe = loop.m_io_context.provider->newTwoWayPipe();
            auto response_server_connection = std::make_unique<mp::Connection>(
                loop,
                kj::mv(response_pipe.ends[0]),
                [response_state](mp::Connection&) {
                    auto response_proxy = kj::heap<MalformedResponseServer>(response_state);
                    return capnp::Capability::Client(kj::mv(response_proxy));
                });
            response_server_connection->onDisconnect([&] { response_server_connection.reset(); });

            auto response_client_connection = std::make_unique<mp::Connection>(loop, kj::mv(response_pipe.ends[1]));
            auto response_client = std::make_unique<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>(
                response_client_connection->m_rpc_system->bootstrap(mp::ServerVatId().vat_id).castAs<test::fuzz::messages::IpcFuzzInterface>(),
                response_client_connection.get(),
                /* destroy_connection= */ true);
            (void)response_client_connection.release();
            response_client_promise.set_value(std::move(response_client));
            loop.loop();
        });
        m_client = client_future.get();
        m_response_client = response_client_future.get();

        // Exchange thread maps so the server can invoke callbacks on the fuzzing thread.
        m_client->initThreadMap();
    }

    // Bypass ProxyClient serialization to send arbitrary data directly to the server.
    void sendTransactionPayload(std::vector<uint8_t> payload)
    {
        std::promise<void> done;
        auto future{done.get_future()};

        m_client->m_context.loop->sync([&] {
            auto request{m_client->m_client.consumeTransactionRequest()};
            request.setArg(kj::arrayPtr(payload.data(), payload.size()));

            m_client->m_context.loop->m_task_set->add(request.send().then(
                [&](auto&&) {
                    done.set_value();
                },
                [&](kj::Exception&& exception) {
                    // Arbitrary transaction bytes may fail deserialization.
                    if (exception.getType() == kj::Exception::Type::FAILED) {
                        done.set_value();
                        return;
                    }
                    done.set_exception(std::make_exception_ptr(
                        std::runtime_error{exception.getDescription().cStr()}));
                }));
        });

        future.get();
    }

    // Bypass ProxyClient serialization to send arbitrary text directly to the server.
    void sendUniValuePayload(std::string payload)
    {
        std::promise<void> done;
        auto future{done.get_future()};

        m_client->m_context.loop->sync([&] {
            auto request{m_client->m_client.consumeUniValueRequest()};
            request.setArg(kj::StringPtr{payload.data(), payload.size()});

            m_client->m_context.loop->m_task_set->add(request.send().then(
                [&](auto&&) {
                    done.set_value();
                },
                [&](kj::Exception&& exception) {
                    // UniValue::read() reports invalid JSON by returning false, so any KJ error is unexpected.
                    done.set_exception(std::make_exception_ptr(
                        std::runtime_error{exception.getDescription().cStr()}));
                }));
        });

        future.get();
    }

    // Return arbitrary transaction data from the raw server to ProxyClient.
    void receiveTransactionPayload(std::vector<uint8_t> payload)
    {
        m_response_state->transaction = std::move(payload);

        try {
            const auto request{MakeTransactionRef(CMutableTransaction{})};
            (void)m_response_client->passTransaction(request);
        } catch (const std::ios_base::failure&) {
            // Arbitrary transaction bytes may fail deserialization.
        }
    }

    // Return arbitrary text data from the raw server to ProxyClient.
    void receiveUniValuePayload(std::string payload)
    {
        m_response_state->univalue = std::move(payload);

        UniValue request;
        (void)m_response_client->passUniValue(request);
    }

    ~IpcFuzzSetup()
    {
        m_client.reset();
        m_response_client.reset();
        if (m_loop_thread.joinable()) m_loop_thread.join();
    }

    std::shared_ptr<IpcFuzzImplementation> m_impl{std::make_shared<IpcFuzzImplementation>()};
    std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>> m_client;
    std::shared_ptr<MalformedResponseState> m_response_state{std::make_shared<MalformedResponseState>()};
    std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>> m_response_client;

private:
    std::thread m_loop_thread;
};

static IpcFuzzSetup* g_ipc;

static void initialize_ipc()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    (void)testing_setup;

    // Ensure the thread's ThreadContext is created before the IPC setup, so
    // it is destroyed after it, since C++ destroys thread_local objects in
    // reverse construction order.
    mp::CurrentThread();

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
                ipc.m_impl->m_expected_a = a;
                ipc.m_impl->m_expected_b = b;
                assert(ipc.m_client->add(a, b) == a + b);
            },
            [&] {
                COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)),
                                   fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                COutPoint expected{outpoint.hash, outpoint.n ^ 0xFFFFFFFFu};
                ipc.m_impl->m_expected_outpoint = outpoint;
                assert(ipc.m_client->passOutPoint(outpoint) == expected);
            },
            [&] {
                std::vector<uint8_t> value = ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512);
                std::vector<uint8_t> expected{value.rbegin(), value.rend()};
                ipc.m_impl->m_expected_vector = value;
                assert(ipc.m_client->passVectorUint8(value) == expected);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                CScript expected{script};
                expected << OP_NOP;
                ipc.m_impl->m_expected_script = script;
                assert(ipc.m_client->passScript(script) == expected);
            },
            [&] {
                UniValue value;
                if (!value.read(fuzzed_data_provider.ConsumeRandomLengthString(512))) return;
                ipc.m_impl->m_expected_univalue = value.write();
                assert(ipc.m_client->passUniValue(value).write() == value.write());
            },
            [&] {
                const CMutableTransaction mutable_tx = ConsumeTransaction(fuzzed_data_provider, std::nullopt);
                if (mutable_tx.vin.empty()) return;
                const CTransactionRef tx = MakeTransactionRef(mutable_tx);
                ipc.m_impl->m_expected_transaction = tx;
                assert(*ipc.m_client->passTransaction(tx) == *tx);
            },
            [&] {
                const int arg = fuzzed_data_provider.ConsumeIntegral<int>();
                const int result = fuzzed_data_provider.ConsumeIntegral<int>();

                ipc.m_impl->m_expected_callback_arg = arg;
                ipc.m_impl->m_expected_callback_result = result;

                FuzzCallback callback{arg, result};
                assert(ipc.m_client->callCallback(callback, arg) == result);
            },
            [&] {
                ipc.sendTransactionPayload(
                    ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512));
            },
            [&] {
                ipc.sendUniValuePayload(
                    fuzzed_data_provider.ConsumeRandomLengthString(512));
            },
            [&] {
                ipc.receiveTransactionPayload(ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512));
            },
            [&] {
                ipc.receiveUniValuePayload(fuzzed_data_provider.ConsumeRandomLengthString(512));
            });
    }
}
} // namespace
