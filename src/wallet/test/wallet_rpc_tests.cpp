// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <scheduler.h>
#include <script/solver.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <wallet/context.h>
#include <wallet/rpc/util.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace wallet {
RPCMethod exportaddresskey();
RPCMethod walletpassphrase();
RPCMethod walletlock();

static std::string TestWalletName(const std::string& endpoint, std::optional<std::string> parameter = std::nullopt)
{
    JSONRPCRequest req;
    req.URI = endpoint;
    return EnsureUniqueWalletName(req, parameter);
}

BOOST_FIXTURE_TEST_SUITE(wallet_rpc_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ensure_unique_wallet_name)
{
    // EnsureUniqueWalletName should only return if exactly one unique wallet name is provided
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/bar", "foo"), "foo");

    BOOST_CHECK_THROW(TestWalletName("/"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/foo"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "bar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "foobar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foobar", "foo"), UniValue);
}

BOOST_AUTO_TEST_CASE(export_authorization_lock_and_timeout)
{
    using namespace std::chrono_literals;
    WalletContext context;
    context.args = &m_args;
    context.chain = m_node.chain.get();
    CScheduler scheduler;
    context.scheduler = &scheduler;
    auto wallet = std::make_shared<CWallet>(context.chain, "export", CreateMockableWalletDatabase());
    BOOST_REQUIRE(AddWallet(context, wallet));
    const auto key = GenerateRandomKey();
    auto manager = CreateDescriptor(*wallet, "tr(" + EncodeSecret(key) + ')', /*success=*/true);
    BOOST_REQUIRE(manager);
    const auto scripts = manager->GetScriptPubKeys();
    BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
    BOOST_REQUIRE(wallet->EncryptWallet("synthetic-passphrase"));

    const auto request = [&](UniValue params) {
        JSONRPCRequest req;
        req.context = &context;
        req.URI = "/wallet/export";
        req.params = std::move(params);
        return req;
    };
    const auto await_held = [](auto& mutex) {
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (std::chrono::steady_clock::now() < deadline) {
            TRY_LOCK(mutex, lock);
            if (!lock) return true;
            std::this_thread::yield();
        }
        return false;
    };
    for (const std::string mode : {"wallet", "taproot_output"}) {
        for (bool timeout : {false, true}) {
            UniValue unlock{UniValue::VARR};
            unlock.push_back("synthetic-passphrase");
            unlock.push_back(60);
            walletpassphrase().HandleRequest(request(std::move(unlock)));
            if (timeout) scheduler.MockForward(61s);

            // Hold only the manager lock in a separate thread. An export that
            // reaches this gate must retain cs_wallet across secret extraction.
            std::promise<void> gated;
            auto ready = gated.get_future();
            // On exceptions, release the gate before joining any operation.
            std::future<UniValue> exported;
            std::future<void> locked;
            std::future<void> gate;
            std::promise<void> release_gate;
            gate = std::async(std::launch::async, [&, release = release_gate.get_future()] {
                LOCK(manager->cs_desc_man);
                gated.set_value();
                release.wait();
            });
            const bool gate_ready = ready.wait_for(10s) == std::future_status::ready;
            exported = std::async(std::launch::async, [&] {
                UniValue args{UniValue::VARR};
                args.push_back(EncodeDestination(destination));
                args.push_back(mode);
                return exportaddresskey().HandleRequest(request(std::move(args)));
            });
            const bool authorized = await_held(wallet->cs_wallet);
            locked = std::async(std::launch::async, [&] {
                if (timeout) {
                    scheduler.StopWhenDrained();
                    scheduler.serviceQueue();
                } else {
                    walletlock().HandleRequest(request(UniValue{UniValue::VARR}));
                }
            });
            const bool relock_waiting = await_held(wallet->m_relock_mutex);
            const bool export_waiting = exported.wait_for(0s) == std::future_status::timeout;
            const bool lock_waiting = locked.wait_for(0s) == std::future_status::timeout;
            // Release every injected gate before any assertion can unwind. Both
            // operations must settle; future destruction also joins on failures.
            release_gate.set_value();
            gate.get();
            const bool export_settled = exported.wait_for(10s) == std::future_status::ready;
            const bool lock_settled = locked.wait_for(10s) == std::future_status::ready;
            const auto result = exported.get();
            locked.get();
            BOOST_CHECK(gate_ready && authorized && relock_waiting);
            BOOST_CHECK(export_waiting && lock_waiting);
            BOOST_CHECK(export_settled && lock_settled);
            BOOST_REQUIRE_EQUAL(result["keys"].size(), 1U);
            BOOST_CHECK(wallet->IsLocked());
            UniValue args{UniValue::VARR};
            args.push_back(EncodeDestination(destination));
            args.push_back(mode);
            const auto locked_request = request(std::move(args));
            BOOST_CHECK_EXCEPTION(exportaddresskey().HandleRequest(locked_request), UniValue,
                [](const UniValue& error) {
                    return error["code"].getInt<int>() == -13 && error["message"].get_str().find("walletpassphrase") != std::string::npos;
                });
        }
    }
    scheduler.stop();
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
