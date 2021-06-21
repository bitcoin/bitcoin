// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
int default_socks5_recv_timeout;
};

extern int g_socks5_recv_timeout;

void initialize_socks5()
{
    static const auto testing_setup = MakeNoLogFileContext<const BasicTestingSetup>();
    default_socks5_recv_timeout = g_socks5_recv_timeout;
}

FUZZ_TARGET_INIT(socks5, initialize_socks5)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    ProxyCredentials proxy_credentials;
    proxy_credentials.username = fuzzed_data_provider.ConsumeRandomLengthString(512);
    proxy_credentials.password = fuzzed_data_provider.ConsumeRandomLengthString(512);
    InterruptSocks5(fuzzed_data_provider.ConsumeBool());
    // Set FUZZED_SOCKET_FAKE_LATENCY=1 to exercise recv timeout code paths. This
    // will slow down fuzzing.
    g_socks5_recv_timeout = (fuzzed_data_provider.ConsumeBool() && std::getenv("FUZZED_SOCKET_FAKE_LATENCY") != nullptr) ? 1 : default_socks5_recv_timeout;
    FuzzedSock fuzzed_sock = ConsumeSock(fuzzed_data_provider);
    // This Socks5(...) fuzzing harness would have caught CVE-2017-18350 within
    // a few seconds of fuzzing.
    (void)Socks5(fuzzed_data_provider.ConsumeRandomLengthString(512),
                 fuzzed_data_provider.ConsumeIntegral<uint16_t>(),
                 fuzzed_data_provider.ConsumeBool() ? &proxy_credentials : nullptr,
                 fuzzed_sock);
}
