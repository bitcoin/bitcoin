#include <boost/test/unit_test.hpp>
#include <common/sv2_messages.h>
#include <util/strencodings.h>

BOOST_AUTO_TEST_SUITE(sv2_messages_tests)

BOOST_AUTO_TEST_CASE(Sv2SetupConnection_test)
{
    uint8_t input[]{
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
    BOOST_CHECK_EQUAL(sizeof(input), 82);

    DataStream ss(input);
    BOOST_CHECK_EQUAL(ss.size(), sizeof(input));

    node::Sv2SetupConnectionMsg setup_conn;
    ss >> setup_conn;

    BOOST_CHECK_EQUAL(setup_conn.m_protocol, 2);
    BOOST_CHECK_EQUAL(setup_conn.m_min_version, 2);
    BOOST_CHECK_EQUAL(setup_conn.m_max_version, 2);
    BOOST_CHECK_EQUAL(setup_conn.m_flags, 1);
    BOOST_CHECK_EQUAL(setup_conn.m_endpoint_host, "0.0.0.0");
    BOOST_CHECK_EQUAL(setup_conn.m_endpoint_port, 8545);
    BOOST_CHECK_EQUAL(setup_conn.m_vendor, "Bitmain");
    BOOST_CHECK_EQUAL(setup_conn.m_hardware_version, "S9i 13.5");
    BOOST_CHECK_EQUAL(setup_conn.m_firmware, "braiins-os-2018-09-22-1-hash");
    BOOST_CHECK_EQUAL(setup_conn.m_device_id, "some-device-uuid");
}

BOOST_AUTO_TEST_CASE(Sv2NetHeader_SetupConnection_test)
{
    uint8_t input[]{
        0x00, 0x00,       // extension type
        0x00,             // msg type (SetupConnection)
        0x52, 0x00, 0x00, // msg length
    };

    DataStream ss(input);
    node::Sv2NetHeader sv2_header;
    ss >> sv2_header;

    BOOST_CHECK(sv2_header.m_msg_type == node::Sv2MsgType::SETUP_CONNECTION);
    BOOST_CHECK_EQUAL(sv2_header.m_msg_len, 82);
}

BOOST_AUTO_TEST_CASE(Sv2SetupConnectionSuccess_test)
{
    // 0200 - used_version
    // 03000000 - flags
    std::string expected{"020003000000"};
    node::Sv2SetupConnectionSuccessMsg setup_conn_success{2, 3};

    DataStream ss{};
    ss << setup_conn_success;

    BOOST_CHECK_EQUAL(HexStr(ss), expected);
}

BOOST_AUTO_TEST_CASE(Sv2NetMsg_SetupConnectionSuccess_test)
{
    // 0200     - used_version
    // 03000000 - flags
    std::string expected{"01020003000000"};
    node::Sv2NetMsg sv2_net_msg{node::Sv2SetupConnectionSuccessMsg{2, 3}};

    DataStream ss{};
    ss << sv2_net_msg;

    BOOST_CHECK_EQUAL(HexStr(ss), expected);
}

BOOST_AUTO_TEST_CASE(Sv2SetupConnectionError_test)
{
    // 00 00 00 00     - flags
    // 19 - error_code length
    // 70726f746f636f6c2d76657273696f6e2d6d69736d61746368 - error_code
    std::string expected{"000000001970726f746f636f6c2d76657273696f6e2d6d69736d61746368"};
    node::Sv2SetupConnectionErrorMsg setup_conn_err{0, std::string{"protocol-version-mismatch"}};

    DataStream ss{};
    ss << setup_conn_err;

    BOOST_CHECK_EQUAL(HexStr(ss), expected);
}

BOOST_AUTO_TEST_SUITE_END()
