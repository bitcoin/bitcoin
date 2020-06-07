// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

void initialize_socks5()
{
    static const auto testing_setup = MakeNoLogFileContext<const BasicTestingSetup>();
}

FUZZ_TARGET_INIT(socks5, initialize_socks5)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    ProxyCredentials proxy_credentials;
    proxy_credentials.username = fuzzed_data_provider.ConsumeRandomLengthString(512);
    proxy_credentials.password = fuzzed_data_provider.ConsumeRandomLengthString(512);
    InterruptSocks5(fuzzed_data_provider.ConsumeBool());
    FuzzedSock fuzzed_sock = ConsumeSock(fuzzed_data_provider);
    // This Socks5(...) fuzzing harness would have caught CVE-2017-18350 within
    // a few seconds of fuzzing.
    (void)Socks5(fuzzed_data_provider.ConsumeRandomLengthString(512),
                 fuzzed_data_provider.ConsumeIntegral<int>(),
                 fuzzed_data_provider.ConsumeBool() ? &proxy_credentials : nullptr,
                 fuzzed_sock);
}
