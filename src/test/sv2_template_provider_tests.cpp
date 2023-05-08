#include <boost/test/unit_test.hpp>
#include <node/sv2_messages.h>
#include <sv2_template_provider.h>
#include <util/strencodings.h>
#include <util/sock.h>
#include <test/util/setup_common.h>
#include <test/util/net.h>

BOOST_FIXTURE_TEST_SUITE(sv2_template_provider_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(Sv2TemplateProvider_ProcessSv2Message_test)
{
    Sv2TemplateProvider template_provider{*m_node.chainman, *m_node.mempool};

    auto sock = std::make_shared<StaticContentsSock>(std::string(1, 'a'));
    Sv2Client client{sock};
    client.m_disconnect_flag = false;
    client.m_setup_connection_confirmed = false;
    BOOST_CHECK(!client.m_disconnect_flag);
    BOOST_CHECK(!client.m_setup_connection_confirmed);

    std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>> block_cache;
    uint64_t template_id{0};

    node::Sv2NewTemplateMsg best_new_template;
    node::Sv2SetNewPrevHashMsg best_prev_hash;

    // Check that failure to deserialize the body of the message into a SetupConnection
    // results in a disconnection for the client.
    node::Sv2NetHeader sv2_net_header{node::Sv2MsgType::SETUP_CONNECTION, 0};
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

    template_provider.ProcessSv2Message(sv2_net_header,
                                        ss,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);
    BOOST_CHECK(client.m_disconnect_flag);

    // Check that if a client is already connected and sends a SetupConnection message twice,
    // they do not get marked for disconnection.
    client.m_setup_connection_confirmed = true;
    client.m_disconnect_flag = false;

    template_provider.ProcessSv2Message(sv2_net_header,
                                        ss,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);
    BOOST_CHECK(!client.m_disconnect_flag);

    // Check that sending a valid setup connection message can be handled successfully.
    uint8_t setup_conn_bytes[]{
        0x02,                                                 // protocol
        0x02, 0x00,                                           // min_version
        0x02, 0x00,                                           // max_version
        0x01, 0x00, 0x00, 0x00,                               // flags
        0x07, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30,       // endpoint_host
        0x61, 0x21,                                           // endpoint_port
        0x07, 0x42, 0x69, 0x74, 0x6d, 0x61, 0x69, 0x6e,       // vendor
        0x08, 0x53, 0x39, 0x69, 0x20, 0x31, 0x33, 0x2e, 0x35, // hardware_version
        0x1c, 0x62, 0x72, 0x61, 0x69, 0x69, 0x6e, 0x73, 0x2d, 0x6f, 0x73, 0x2d, 0x32, 0x30,
        0x31, 0x38, 0x2d, 0x30, 0x39, 0x2d, 0x32, 0x32, 0x2d, 0x31, 0x2d, 0x68, 0x61, 0x73,
        0x68, // firmware
        0x10, 0x73, 0x6f, 0x6d, 0x65, 0x2d, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2d, 0x75,
        0x75, 0x69, 0x64, // device_id
    };
    CDataStream ss_setup_conn(setup_conn_bytes, SER_NETWORK, PROTOCOL_VERSION);

    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, sizeof(setup_conn_bytes)},
                                        ss_setup_conn,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);

    BOOST_CHECK(client.m_setup_connection_confirmed);
    BOOST_CHECK(!client.m_disconnect_flag);

    // Check that sending an invalid sub protocol results in the client
    // being set for disconnection.
    client.m_setup_connection_confirmed = false;
    setup_conn_bytes[0] = 0x03;
    ss_setup_conn.clear();
    ss_setup_conn = CDataStream(setup_conn_bytes, SER_NETWORK, PROTOCOL_VERSION);

    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, sizeof(setup_conn_bytes)},
                                        ss_setup_conn,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);

    BOOST_CHECK(!client.m_setup_connection_confirmed);
    BOOST_CHECK(client.m_disconnect_flag);

    // Check that incompatible versions of the current sv2 protocol means the client is
    // set for disconnection.
    client.m_disconnect_flag = false;
    BOOST_CHECK(!client.m_disconnect_flag);
    setup_conn_bytes[1] = 0x05;
    setup_conn_bytes[3] = 0x1a;
    ss_setup_conn.clear();
    ss_setup_conn = CDataStream(setup_conn_bytes, SER_NETWORK, PROTOCOL_VERSION);

    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, sizeof(setup_conn_bytes)},
                                        ss_setup_conn,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);

    BOOST_CHECK(!client.m_setup_connection_confirmed);
    BOOST_CHECK(client.m_disconnect_flag);

    // Check that receiving a valid coinbase_output_max_additional_size_bytes results
    // in a new block in the block cache matched with an incremented template id.
    client.m_setup_connection_confirmed = true;
    uint8_t coinbase_output_max_additional_size_bytes[]{
        0x01, 0x00, 0x00, 0x00, // additional coinbase output size
    };
    CDataStream ss_coinbase_output_max_additional_size(coinbase_output_max_additional_size_bytes, SER_NETWORK, PROTOCOL_VERSION);

    template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, sizeof(coinbase_output_max_additional_size_bytes)},
                                        ss_coinbase_output_max_additional_size,
                                        client,
                                        best_new_template,
                                        best_prev_hash,
                                        block_cache,
                                        template_id);
    BOOST_CHECK_EQUAL(template_id, 1);
    BOOST_CHECK(block_cache.count(template_id));
}
BOOST_AUTO_TEST_SUITE_END()
