// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/ipc_fuzz.capnp.h>
#include <test/fuzz/ipc_fuzz.capnp.proxy.h>
#include <test/fuzz/ipc_fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <capnp/rpc.h>
#include <capnp/capability.h>
#include <kj/memory.h>
#include <mp/proxy.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <future>
#include <stdexcept>
#include <thread>

namespace {
class IpcFuzzSetup
{
public:
    std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>> client;

    IpcFuzzSetup()
        : m_thread([&] {
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

              m_client_promise.set_value(std::move(client_proxy));
              loop.loop();
          })
    {
        client = m_client_promise.get_future().get();
    }

    ~IpcFuzzSetup()
    {
        client.reset();
        if (m_thread.joinable()) m_thread.join();
    }

private:
    std::promise<std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>> m_client_promise;
    std::thread m_thread;
};

IpcFuzzSetup& GetIpcFuzzSetup()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    static IpcFuzzSetup setup;
    return setup;
}

void initialize_ipc()
{
    (void)GetIpcFuzzSetup();
}

FUZZ_TARGET(ipc, .init = initialize_ipc)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    auto* ipc = GetIpcFuzzSetup().client.get();

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 64) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                static constexpr int MIN_ADD{-1'000'000};
                static constexpr int MAX_ADD{1'000'000};
                const int a = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                const int b = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                assert(ipc->add(a, b) == a + b);
            },
            [&] {
                COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)),
                                   fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                COutPoint expected{outpoint.hash, outpoint.n ^ 0xFFFFFFFFu};
                assert(ipc->passOutPoint(outpoint) == expected);
            },
            [&] {
                std::vector<uint8_t> value = ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512);
                if (value.empty()) value.push_back(0);
                std::vector<uint8_t> expected{value.rbegin(), value.rend()};
                assert(ipc->passVectorUint8(value) == expected);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                CScript expected{script};
                expected << OP_NOP;
                assert(ipc->passScript(script) == expected);
            });
    }
}
} // namespace
