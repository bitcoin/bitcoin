#include <boost/test/unit_test.hpp>
#include <common/sv2_messages.h>
#include <node/sv2_template_provider.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/sock.h>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(sv2_template_provider_tests, TestChain100Setup)

static std::vector<uint8_t> get_setup_conn_bytes()
{
    return {
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
}

static void connect(Sv2TemplateProvider& template_provider, Sv2Client& client, std::vector<uint8_t> setup_conn_bytes)
{
    // Check that sending a valid setup connection message can be handled successfully.
    auto bytes_copy = setup_conn_bytes;
    node::Sv2NetHeader setup_conn_header = node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, static_cast<uint32_t>(bytes_copy.size())};
    node::Sv2NetMsg sv2_msg = node::Sv2NetMsg{std::move(setup_conn_header), std::move(bytes_copy)};

    template_provider.ProcessSv2Message(sv2_msg,
                                        client);

    BOOST_CHECK(client.m_setup_connection_confirmed);
    BOOST_CHECK(!client.m_disconnect_flag);
}

BOOST_AUTO_TEST_CASE(Sv2TemplateProvider_Connection_test)
{
    Sv2TemplateProvider template_provider{*m_node.chainman};

    auto sock = std::make_shared<StaticContentsSock>(std::string(1, 'a'));
    auto static_key{GenerateRandomKey()};
    auto authority_key{GenerateRandomKey()};

    // Create certificates
    auto epoch_now = std::chrono::system_clock::now().time_since_epoch();
    uint16_t version = 0;
    uint32_t valid_from = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(epoch_now).count());
    uint32_t valid_to =  std::numeric_limits<unsigned int>::max();

    // TODO: Stratum v2 spec requires signing the static key using the authority key,
    //       but SRI currently implements this incorrectly.
    authority_key = static_key;
    auto certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to,
                                XOnlyPubKey(static_key.GetPubKey()), authority_key);

    std::unique_ptr transport = std::make_unique<Sv2Transport>(static_key, certificate);
    std::unique_ptr<Sv2Client> client = std::make_unique<Sv2Client>(0, std::move(sock), std::move(transport));
    client->m_disconnect_flag = false;
    client->m_setup_connection_confirmed = false;
    BOOST_CHECK(!client->m_disconnect_flag);
    BOOST_CHECK(!client->m_setup_connection_confirmed);

    {
        // Check that failure to deserialize the body of the message into a SetupConnection
        // results in a disconnection for the client->
        node::Sv2NetHeader setup_conn_header{node::Sv2MsgType::SETUP_CONNECTION, 0};
        std::vector<uint8_t> empty_body;

        node::Sv2NetMsg sv2_msg{std::move(setup_conn_header), std::move(empty_body)};

        template_provider.ProcessSv2Message(sv2_msg, *client);
        BOOST_CHECK(client->m_disconnect_flag);

        // Check that if a client is already connected and sends a SetupConnection message twice,
        // they do not get marked for disconnection.
        client->m_setup_connection_confirmed = true;
        client->m_disconnect_flag = false;

        template_provider.ProcessSv2Message(sv2_msg, *client);
        BOOST_CHECK(!client->m_disconnect_flag);
    }

    {
        // Make a successful connection
        std::vector<uint8_t> setup_conn_bytes = get_setup_conn_bytes();
        connect(template_provider, *client, setup_conn_bytes);

        // Check that sending an invalid sub protocol results in the client
        // being set for disconnection.
        client->m_setup_connection_confirmed = false;
        setup_conn_bytes[0] = 0x03;
        node::Sv2NetHeader setup_conn_header = node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, static_cast<uint32_t>(setup_conn_bytes.size())};
        node::Sv2NetMsg sv2_msg = node::Sv2NetMsg{std::move(setup_conn_header), std::move(setup_conn_bytes)};

        // TODO: give ProcessSv2Message a callback where it can pass replies,
        //       so we can avoid dealing with or mocking the noise handshake
        // template_provider.ProcessSv2Message(sv2_msg, client);

        // BOOST_CHECK(!client->m_setup_connection_confirmed);
        // BOOST_CHECK(client->m_disconnect_flag);
    }

    // {
        // Check that incompatible versions of the current sv2 protocol means the client is
        // set for disconnection.
        client->m_disconnect_flag = false;
        BOOST_CHECK(!client->m_disconnect_flag);

        std::vector<uint8_t> setup_conn_bytes = get_setup_conn_bytes();
        setup_conn_bytes[1] = 0x05;
        setup_conn_bytes[3] = 0x1a;

        node::Sv2NetHeader setup_conn_header = node::Sv2NetHeader{node::Sv2MsgType::SETUP_CONNECTION, static_cast<uint32_t>(setup_conn_bytes.size())};
        node::Sv2NetMsg sv2_msg = node::Sv2NetMsg{std::move(setup_conn_header), std::move(setup_conn_bytes)};

        // BOOST_CHECK_THROW(template_provider.ProcessSv2Message(sv2_msg,
        //                                                       client),
        //                   std::runtime_error);

        // BOOST_CHECK(!client->m_setup_connection_confirmed);
        // TODO: why doesn't this work?
        // BOOST_CHECK(client->m_disconnect_flag);
    // }
}

BOOST_AUTO_TEST_CASE(Sv2TemplateProvider_Message_CoinbaseOutputDataSize_test)
{
    // Sv2TemplateProvider template_provider{*m_node.chainman};

    // auto sock = std::make_shared<StaticContentsSock>(std::string(1, 'a'));
    // Sv2Client client{sock};

    // std::vector<uint8_t> setup_conn_bytes = get_setup_conn_bytes();
    // connect(template_provider, client, setup_conn_bytes);

    // Check that receiving a valid coinbase_output_max_additional_size_bytes results
    // in a new block in the block cache matched with an incremented template id.
    // client->m_setup_connection_confirmed = true;
    // uint8_t coinbase_output_max_additional_size_bytes[]{
    //     0x01, 0x00, 0x00, 0x00, // additional coinbase output size
    // };
    // DataStream ss_coinbase_output_max_additional_size(coinbase_output_max_additional_size_bytes);

    // TODO: fix test: error: no viable conversion from 'node::Sv2NetHeader' to 'const node::Sv2NetMsg'
    // template_provider.ProcessSv2Message(node::Sv2NetHeader{node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE, sizeof(coinbase_output_max_additional_size_bytes)},
    //                                     // ss_coinbase_output_max_additional_size,
    //                                     client,
    //                                     best_new_template,
    //                                     best_prev_hash,
    //                                     block_cache,
    //                                     template_id);
    // BOOST_CHECK_EQUAL(template_id, 1);
    // BOOST_CHECK(block_cache.count(template_id));
}
BOOST_AUTO_TEST_SUITE_END()
