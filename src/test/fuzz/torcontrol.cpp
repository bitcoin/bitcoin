// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <torcontrol.h>

#include <cstdint>
#include <string>
#include <vector>

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(torcontrol, .init = initialize_torcontrol)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    TorController tor_controller;
    CThreadInterrupt interrupt;
    TorControlConnection conn{interrupt};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        TorControlReply tor_control_reply;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_control_reply.code = 250;
            },
            [&] {
                tor_control_reply.code = 510;
            },
            [&] {
                tor_control_reply.code = fuzzed_data_provider.ConsumeIntegral<int>();
            });
        tor_control_reply.lines = ConsumeRandomLengthStringVector(fuzzed_data_provider);

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_controller.add_onion_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.auth_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.authchallenge_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.protocolinfo_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.get_socks_cb(conn, tor_control_reply);
            });
    }
}
