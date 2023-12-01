// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_messages.h>
#include <test/fuzz/fuzz.h>
#include <streams.h>

FUZZ_TARGET(sv2_msgs)
{
    DataStream ss_setup_conn(buffer);
    node::Sv2SetupConnectionMsg setup_conn;

    try {
        ss_setup_conn >> setup_conn;
    } catch (const std::exception &e) {
        return;
    }

    DataStream ss_submit_solution(buffer);
    node::Sv2SubmitSolutionMsg submit_solution;

    try {
        ss_submit_solution >> submit_solution;
    } catch (const std::exception &e) {
        return;
    }

    DataStream ss_coinbase_output_data(buffer);
    node::Sv2CoinbaseOutputDataSizeMsg coinbase_output_data;

    try {
        ss_coinbase_output_data >> coinbase_output_data;
    } catch (const std::exception &e) {
        return;
    }

    DataStream ss_sv2_header(buffer);
    node::Sv2NetHeader sv2_header;

    try {
        ss_sv2_header >> sv2_header;
    } catch (const std::exception &e) {
        return;
    }
}
