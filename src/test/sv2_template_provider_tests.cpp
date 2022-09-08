#include <sv2_template_provider.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sv2_template_provider_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(SetupConnection_test)
{
    uint8_t expected[]{
        0x03, // protocol
        0x02, 0x00, // min_version
        0x02, 0x00, // max_version
        0x01, 0x00, 0x00, 0x00, // flags
        0x07, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30, // endpoint_host
        0x61, 0x21, // endpoint_port
        0x07, 0x42, 0x69, 0x74, 0x6d, 0x61, 0x69, 0x6e, // vendor
        0x08, 0x53, 0x39, 0x69, 0x20, 0x31, 0x33, 0x2e, 0x35, // hardware_version 
        0x1c, 0x62, 0x72, 0x61, 0x69, 0x69, 0x6e, 0x73, 0x2d, 0x6f, 0x73, 0x2d, 0x32, 0x30,
        0x31, 0x38, 0x2d, 0x30, 0x39, 0x2d, 0x32, 0x32, 0x2d, 0x31, 0x2d, 0x68, 0x61, 0x73,
        0x68, //firmware
        0x10, 0x73, 0x6f, 0x6d, 0x65, 0x2d, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2d, 0x75,
        0x75, 0x69, 0x64, // device_id
    };
    BOOST_CHECK_EQUAL(sizeof(expected), 82);

    CDataStream ss(expected, SER_NETWORK, PROTOCOL_VERSION);
    BOOST_CHECK_EQUAL(ss.size(), 82);

    SetupConnection setup_conn;
    ss >> setup_conn;

    BOOST_CHECK_EQUAL(setup_conn.m_protocol, 3);
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

BOOST_AUTO_TEST_CASE(Sv2Header_SetupConnection_test)
{
    uint8_t input[]{
        0x00, 0x00, // extension type
        0x00, // msg type (SetupConnection)
        0x52, 0x00, 0x00, // msg length
    };

    CDataStream ss(input, SER_NETWORK, PROTOCOL_VERSION);
    Sv2Header sv2_header;
    ss >> sv2_header;

    BOOST_CHECK_EQUAL(sv2_header.m_msg_type, Sv2MsgType::SETUP_CONNECTION);
    BOOST_CHECK_EQUAL(sv2_header.m_msg_len, 82);
}

BOOST_AUTO_TEST_CASE(SetupConnectionSuccess_test)
{
    uint8_t expected[]{
       0x02, 0x00, // used_version
       0x03, 0x00, 0x00, 0x00, // flags
    };

    SetupConnectionSuccess setup_conn_success{2, 3};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << setup_conn_success;
    BOOST_CHECK_EQUAL(ss.size(), 6);

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 6);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_CASE(Sv2Header_SetupConnectionSuccess_test)
{
    uint8_t expected[]{
       0x00, 0x00, // extension type
       0x01, // msg type (SetupConnectionSuccess)
       0x06, 0x00, 0x00, // msg length
       0x02, 0x00, // used_version
       0x03, 0x00, 0x00, 0x00, // flags
    };

    SetupConnectionSuccess setup_conn_success{2, 3};
    Sv2Header sv2_header{Sv2MsgType::SETUP_CONNECTION_SUCCESS, setup_conn_success.GetMsgLen()};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << sv2_header << setup_conn_success;

    BOOST_CHECK_EQUAL(ss.size(), 12);

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;

        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 12);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_CASE(NewTemplate_test)
{
    uint8_t expected[]{
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // template id
        0x00, // future template
        0x00, 0x00, 0x00, 0x30, // version
        0x02, 0x00, 0x00, 0x00, // coinbase tx version
        0x05, // coinbase_prefix len
        0x04, 0x03, 0x01, 0x21, 0x00,  // coinbase_prefix
        0xff, 0xff, 0xff, 0xff, // coinbase_tx_input_sequence
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // coinbase_tx_value remaining
        0x00, 0x00, 0x00, 0x00, // cointbase_tx_outputs count
        0x00, 0x00, // coinbase_tx_ouputs
        0x00, 0x00, 0x00, 0x00, // coinbase_tx_locktime
        0x01, 0x01, // merkle_path length
        0x1a, 0x62, 0x40, 0x82, 0x3d, 0xe4, 0xc8, 0xd6, 0xaa, 0xf8, 0x26, 0x85, // merkle path
        0x1b, 0xdf, 0x2b, 0x0e, 0x8d, 0x5a, 0xcf, 0x7c, 0x31, 0xe8, 0x57, 0x8c,
        0xff, 0x4c, 0x39, 0x4b, 0x5a, 0x32, 0xbd, 0x4e,
    };

    NewTemplate new_template;
    new_template.m_template_id = 1;
    new_template.m_future_template = false;
    new_template.m_version = 805306368;
    new_template.m_coinbase_tx_version = 2;

    std::vector<uint8_t> coinbase_prefix{0x03, 0x01, 0x21, 0x00};
    CScript prefix(coinbase_prefix.begin(), coinbase_prefix.end());
    new_template.m_coinbase_prefix = prefix;

    new_template.m_coinbase_tx_input_sequence = 4294967295;
    new_template.m_coinbase_tx_value_remaining = 0;
    new_template.m_coinbase_tx_outputs_count = 0;

    std::vector<CTxOut> coinbase_tx_ouputs;
    new_template.m_coinbase_tx_outputs = coinbase_tx_ouputs;

    new_template.m_coinbase_tx_locktime = 0;

    std::vector<uint256> merkle_path;
    CMutableTransaction mtx_tx;
    CTransaction tx{mtx_tx};
    merkle_path.push_back(tx.GetHash());

    new_template.m_merkle_path = merkle_path;

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << new_template;

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_CASE(Sv2Header_NewTemplate_test)
{
    uint8_t expected[] = {
        0x00, 0x00, // extension type
        0x71, // msg type (NewTemplate)
        0x00, 0x00, 0x00, // msg length
    };

    Sv2Header sv2_header{Sv2MsgType::NEW_TEMPLATE, 0};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << sv2_header;
    BOOST_CHECK_EQUAL(ss.size(), 6);

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 6);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}


BOOST_AUTO_TEST_CASE(SetNewPrevHash_test)
{
    uint8_t expected[]{
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // template_id
        0xe5, 0x9a, 0x2e, 0xf8, 0xd4, 0x82, 0x6b, 0x89, 0xfe, 0x63, 0x7f, 0x3b, // prev hash
        0x53, 0x22, 0xc0, 0xf1, 0x67, 0xfd, 0x2d, 0x3d, 0xc8, 0x8e, 0xc5, 0x68,
        0xa7, 0xc2, 0xc8, 0xff, 0xd8, 0xeb, 0x88, 0x73,
        0x97, 0x3c, 0x0e, 0x63, // header_timestamp
        0xff, 0xff, 0x7f, 0x03, // nbits
        0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // targets
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    std::vector<uint8_t> prev_hash_input{
        0xe5, 0x9a, 0x2e, 0xf8, 0xd4, 0x82, 0x6b, 0x89, 0xfe, 0x63, 0x7f, 0x3b,
        0x53, 0x22, 0xc0, 0xf1, 0x67, 0xfd, 0x2d, 0x3d, 0xc8, 0x8e, 0xc5, 0x68,
        0xa7, 0xc2, 0xc8, 0xff, 0xd8, 0xeb, 0x88, 0x73
    };
    uint256 prev_hash = uint256(prev_hash_input);

    CBlock block;
    block.hashPrevBlock = prev_hash;
    block.nTime = 1661877399;
    block.nBits = 58720255;

    SetNewPrevHash new_prev_hash{block, 2};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << new_prev_hash;
    BOOST_CHECK_EQUAL(ss.size(), 80);
    BOOST_CHECK_EQUAL(new_prev_hash.GetMsgLen(), 80);

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 80);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_CASE(Sv2Header_SetNewPrevHash_test)
{
    uint8_t expected[]{
        0x00, 0x00, // extension type
        0x72, // msg type (SetNewPrevHash)
        0x00, 0x00, 0x00, // msg length
    };

    Sv2Header sv2_header{Sv2MsgType::SET_NEW_PREV_HASH, 0};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << sv2_header;
    BOOST_CHECK_EQUAL(ss.size(), 6);

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 6);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_CASE(SubmitSolution_test)
{
    uint8_t input[]{
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // template_id
        0x02, 0x00, 0x00, 0x00, // version
        0x97, 0x3c, 0x0e, 0x63, // header_timestamp
        0xff, 0xff, 0x7f, 0x03, // header_nonce
        0x5d, 0x00, // 2 byte length of coinbase_tx
        2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // coinbase_tx
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 34, 1, 24, 0,
        0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 1, 0, 0, 0, 0, 0, 0, 0, 0, 22, 0, 20,
        83, 18, 96, 170, 42, 25, 158, 34, 140, 83, 125, 250, 66, 200, 43, 234, 44,
        124, 31, 77, 0, 0, 0, 0,
    };
    CDataStream ss(input, SER_NETWORK, PROTOCOL_VERSION);

    SubmitSolution submit_solution;
    ss >> submit_solution;
    BOOST_CHECK_EQUAL(submit_solution.m_template_id, 2);
    BOOST_CHECK_EQUAL(submit_solution.m_version, 2);
}

BOOST_AUTO_TEST_CASE(Sv2Header_SubmitSolution_test)
{
    uint8_t expected[]{
        0x00, 0x00, // extension type
        0x76, // msg type (SubmitSolution)
        0x00, 0x00, 0x00, // msg length
    };

    Sv2Header sv2_header{Sv2MsgType::SUBMIT_SOLUTION, 0};

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << sv2_header;

    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < sizeof(expected); ++i) {
        uint8_t b;
        ss >> b;
        bytes.push_back(b);
    }
    BOOST_CHECK_EQUAL(bytes.size(), 6);
    BOOST_CHECK(std::equal(bytes.begin(), bytes.end(), expected));
}

BOOST_AUTO_TEST_SUITE_END()
