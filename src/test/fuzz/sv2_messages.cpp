// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/sv2_messages.h>
#include <test/fuzz/fuzz.h>
#include <streams.h>

FUZZ_TARGET(sv2_msgs)
{
    CDataStream ss_setup_conn(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    node::Sv2SetupConnectionMsg setup_conn;

    try {
        ss_setup_conn >> setup_conn;
    } catch (const std::exception &e) {
        return;
    }

    CDataStream ss_submit_solution(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    node::Sv2SubmitSolutionMsg submit_solution;

    try {
        ss_submit_solution >> submit_solution;
    } catch (const std::exception &e) {
        return;
    }

    CDataStream ss_coinbase_output_data(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    node::Sv2CoinbaseOutputDataSizeMsg coinbase_output_data;

    try {
        ss_coinbase_output_data >> coinbase_output_data;
    } catch (const std::exception &e) {
        return;
    }

    CDataStream ss_sv2_header(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    node::Sv2NetHeader sv2_header;

    try {
        ss_sv2_header >> sv2_header;
    } catch (const std::exception &e) {
        return;
    }
}
