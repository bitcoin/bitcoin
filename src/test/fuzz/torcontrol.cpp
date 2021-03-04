// Copyright (c) 2020-2021 The Bitcoin Core developers
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

class DummyTorControlConnection : public TorControlConnection
{
public:
    DummyTorControlConnection() : TorControlConnection{nullptr}
    {
    }

    bool Connect(const std::string&, const ConnectionCB&, const ConnectionCB&)
    {
        return true;
    }

    void Disconnect()
    {
    }

    bool Command(const std::string&, const ReplyHandlerCB&)
    {
        return true;
    }
};

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET_INIT(torcontrol, initialize_torcontrol)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    TorController tor_controller;
    while (fuzzed_data_provider.ConsumeBool()) {
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
        if (tor_control_reply.lines.empty()) {
            break;
        }
        DummyTorControlConnection dummy_tor_control_connection;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_controller.add_onion_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.auth_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.authchallenge_cb(dummy_tor_control_connection, tor_control_reply);
            },
            [&] {
                tor_controller.protocolinfo_cb(dummy_tor_control_connection, tor_control_reply);
            });
    }
}
