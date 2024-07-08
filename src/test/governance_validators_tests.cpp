// Copyright (c) 2014-2023 The Dash Core developers

#include <governance/governancevalidators.h>
#include <util/strencodings.h>

#include <test/data/proposals_invalid.json.h>
#include <test/data/proposals_valid.json.h>
#include <test/util/json.h>
#include <test/util/setup_common.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(governance_validators_tests, BasicTestingSetup)

static std::string CreateEncodedProposalObject(const UniValue& objJSON)
{
    UniValue innerArray(UniValue::VARR);
    innerArray.push_back(UniValue("proposal"));
    innerArray.push_back(objJSON);

    UniValue outerArray(UniValue::VARR);
    outerArray.push_back(innerArray);

    std::string strData = outerArray.write();
    std::string strHex = HexStr(strData);
    return strHex;
}

BOOST_AUTO_TEST_CASE(valid_proposals_test)
{
    // all proposals are valid but expired
    UniValue tests = read_json(json_tests::proposals_valid);

    BOOST_CHECK_MESSAGE(tests.size(), "Empty `tests`");
    for(size_t i = 0; i < tests.size(); ++i) {
        const UniValue& objProposal = tests[i][0];
        bool fAllowScript = tests[i][1].get_bool();

        // legacy format
        std::string strHexData1 = CreateEncodedProposalObject(objProposal);
        CProposalValidator validator0(strHexData1, fAllowScript);
        BOOST_CHECK(!validator0.Validate());
        BOOST_CHECK_EQUAL(validator0.GetErrorMessages(), "Proposal must be a JSON object;JSON parsing error;");

        // current format
        std::string strHexData2 = HexStr(objProposal.write());
        CProposalValidator validator2(strHexData2, fAllowScript);
        BOOST_CHECK_MESSAGE(validator2.Validate(false), validator2.GetErrorMessages());
        BOOST_CHECK_MESSAGE(!validator2.Validate(), validator2.GetErrorMessages());
    }
}

BOOST_AUTO_TEST_CASE(invalid_proposals_test)
{
    // all proposals are invalid regardless of being expired or not
    // (i.e. we don't even check for expiration here)
    UniValue tests = read_json(json_tests::proposals_invalid);
    BOOST_CHECK_MESSAGE(tests.size(), "Empty `tests`");
    for(size_t i = 0; i < tests.size(); ++i) {
        const UniValue& objProposal = tests[i];

        // legacy format
        std::string strHexData1 = CreateEncodedProposalObject(objProposal);
        CProposalValidator validator1(strHexData1, false);
        BOOST_CHECK(!validator1.Validate());
        BOOST_CHECK_MESSAGE(!validator1.Validate(false), validator1.GetErrorMessages());

        // current format
        std::string strHexData2 = HexStr(objProposal.write());
        CProposalValidator validator2(strHexData2, false);
        BOOST_CHECK_MESSAGE(!validator2.Validate(false), validator2.GetErrorMessages());
    }
}

BOOST_AUTO_TEST_SUITE_END()
