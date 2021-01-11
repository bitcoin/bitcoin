#include <boost/test/unit_test.hpp>
#include <test/data/ethspv_valid.json.h>
#include <test/data/ethspv_invalid.json.h>

#include <uint256.h>
#include <util/strencodings.h>
#include <ethereum/ethereum.h>
#include <ethereum/common.h>
#include <ethereum/rlp.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <policy/policy.h>
#include <services/asset.h>
#include <univalue.h>
#include <key_io.h>
#include <util/system.h>
#include <test/util/setup_common.h>
extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(ethereum_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ethereum_parseabidata)
{
    tfm::format(std::cout,"Running ethereum_parseabidata...\n");
    CAmount outputAmount;
    uint32_t nAsset = 0;
    const std::vector<unsigned char> &expectedMethodHash = ParseHex("54c988ff");
    const std::vector<unsigned char> &rlpBytes = ParseHex("54c988ff00000000000000000000000000000000000000000000000000000002540be400000000000000000000000000000000000000000000000000000000009be8894b0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000002c62637274317130667265323430737939326d716b386b6b377073616561366b74366d3537323570377964636a0000000000000000000000000000000000000000");
    std::string expectedAddress = "bcrt1q0fre240sy92mqk8kk7psaea6kt6m5725p7ydcj";
    std::string address;
    BOOST_CHECK(parseEthMethodInputData(expectedMethodHash, 8, 8, rlpBytes, outputAmount, nAsset, address));
    BOOST_CHECK_EQUAL(outputAmount, 100*COIN);
    BOOST_CHECK_EQUAL(nAsset, 2615707979);
    BOOST_CHECK(address == expectedAddress);

}

BOOST_AUTO_TEST_CASE(ethspv_valid)
{
    tfm::format(std::cout,"Running ethspv_valid...\n");
    // Read tests from test/data/ethspv_valid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // [[spv_root, spv_parent_node, spv_value, spv_path]]

    UniValue tests = read_json(std::string(json_tests::ethspv_valid, json_tests::ethspv_valid + sizeof(json_tests::ethspv_valid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() != 4) {
				// ignore comments
				continue;
		} else {
        if ( !test[0].isStr() || !test[1].isStr() || !test[2].isStr() || !test[3].isStr()) {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }

      	std::string spv_tx_root = test[0].get_str();
			  std::string spv_parent_nodes = test[1].get_str();
			  std::string spv_value = test[2].get_str();
			  std::string spv_path = test[3].get_str();

        const std::vector<unsigned char> &vchTxRoot = ParseHex(spv_tx_root);
        dev::RLP rlpTxRoot(&vchTxRoot);
        const std::vector<unsigned char> &vchTxParentNodes = ParseHex(spv_parent_nodes);
        dev::RLP rlpTxParentNodes(&vchTxParentNodes);
        const std::vector<unsigned char> &vchTxValue = ParseHex(spv_value);
        dev::RLP rlpTxValue(&vchTxValue);
        const std::vector<unsigned char> &vchTxPath = ParseHex(spv_path);
        BOOST_CHECK(VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot));
        }
    }
}

BOOST_AUTO_TEST_CASE(ethspv_invalid)
{
    tfm::format(std::cout,"Running ethspv_invalid...\n");
    // Read tests from test/data/ethspv_invalid.json
    // Format is an array of arrays
    // Inner arrays are either [ "comment" ]
    // [[spv_root, spv_parent_node, spv_value, spv_path]]

    UniValue tests = read_json(std::string(json_tests::ethspv_invalid, json_tests::ethspv_invalid + sizeof(json_tests::ethspv_invalid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() != 4) {
				// ignore comments
				continue;
		    } else {
            if ( !test[0].isStr() || !test[1].isStr() || !test[2].isStr() || !test[3].isStr()) {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }
			      std::string spv_tx_root = test[0].get_str();
			      std::string spv_parent_nodes = test[1].get_str();
			      std::string spv_value = test[2].get_str();
			      std::string spv_path = test[3].get_str();

            const std::vector<unsigned char> &vchTxRoot = ParseHex(spv_tx_root);
            dev::RLP rlpTxRoot(&vchTxRoot);
            const std::vector<unsigned char> &vchTxParentNodes = ParseHex(spv_parent_nodes);
            dev::RLP rlpTxParentNodes(&vchTxParentNodes);
            const std::vector<unsigned char> &vchTxValue = ParseHex(spv_value);
            dev::RLP rlpTxValue(&vchTxValue);
            const std::vector<unsigned char> &vchTxPath = ParseHex(spv_path);
            BOOST_CHECK(!VerifyProof(&vchTxPath, rlpTxValue, rlpTxParentNodes, rlpTxRoot));
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
