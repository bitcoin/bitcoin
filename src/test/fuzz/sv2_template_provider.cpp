// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_messages.h>
#include <node/sv2_template_provider.h>
#include <test/fuzz/fuzz.h>
#include <streams.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>

FUZZ_TARGET(sv2_template_provider)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    const TestingSetup* setup = testing_setup.get();

    std::vector<uint8_t> random_bytes{buffer.begin(), buffer.end()};

    Sv2TemplateProvider template_provider{*setup->m_node.chainman, *setup->m_node.mempool};

    // node::Sv2NewTemplateMsg best_new_template;
    // node::Sv2SetNewPrevHashMsg best_prev_hash;
    // std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>> block_cache;

    // auto sock = std::make_shared<StaticContentsSock>(std::string(1, 'a'));
    // Sv2Client client{sock};

    // client.m_setup_connection_confirmed = false;
    // auto header = node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, static_cast<uint32_t>(random_bytes.size())};
    // auto setup_conn = node::Sv2NetMsg(std::move(header), std::vector<uint8_t>(random_bytes));

    // TODO: pass callback to ProcessSv2Message to it can return unencrypted
    // responses.

    // template_provider.ProcessSv2Message(setup_conn,
    //                                     client);

    // client.m_setup_connection_confirmed = true;
    // header = node::Sv2NetHeader{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, static_cast<uint32_t>(random_bytes.size())};
    // auto coinbase_output_size = node::Sv2NetMsg(std::move(header), std::vector<uint8_t>(random_bytes));
    // template_provider.ProcessSv2Message(coinbase_output_size,
    //                                     client);

    // client.m_coinbase_output_data_size_recv = true;
    // header = node::Sv2NetHeader{node::Sv2MsgType::SUBMIT_SOLUTION, static_cast<uint32_t>(random_bytes.size())};
    // auto submit_solution = node::Sv2NetMsg(std::move(header), std::vector<uint8_t>(random_bytes));
    // template_provider.ProcessSv2Message(submit_solution,
    //                                     client);
};
