// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/sv2_messages.h>
#include <sv2_template_provider.h>
#include <test/fuzz/fuzz.h>
#include <streams.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>

FUZZ_TARGET(sv2_template_provider)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    const TestingSetup* setup = testing_setup.get();

    CDataStream random_bytes(buffer, SER_NETWORK, INIT_PROTO_VERSION);

    Sv2TemplateProvider template_provider{*setup->m_node.chainman, *setup->m_node.mempool};

    node::Sv2NewTemplateMsg best_new_template;
    node::Sv2SetNewPrevHashMsg best_prev_hash;
    std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>> block_cache;
    uint64_t template_id;

    auto sock = std::make_shared<StaticContentsSock>(std::string(1, 'a'));
    Sv2Client client{sock};

    client.m_setup_connection_confirmed = false;
    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, static_cast<uint32_t>(buffer.size())},
                                        random_bytes,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);

    client.m_setup_connection_confirmed = true;
    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, static_cast<uint32_t>(buffer.size())},
                                        random_bytes,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);

    client.m_coinbase_output_data_size_recv = true;
    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::SUBMIT_SOLUTION, static_cast<uint32_t>(buffer.size())},
                                        random_bytes,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);
};
