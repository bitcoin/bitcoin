// Copyright (c) 2014-2017 The Dash Core developers

#include "governance-validators.h"
#include "univalue.h"
#include "utilstrencodings.h"

#include "test/test_dash.h"

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <string>

BOOST_FIXTURE_TEST_SUITE(governance_validators_tests, BasicTestingSetup)

UniValue LoadJSON(const std::string& strFilename)
{
    UniValue obj(UniValue::VOBJ);
    std::ifstream istr(strFilename.c_str());

    std::string strData;
    std::string strLine;
    bool fFirstLine = true;
    while(std::getline(istr, strLine)) {
        if(!fFirstLine) {
            strData += "\n";
        }
        strData += strLine;
        fFirstLine = false;
    }
    obj.read(strData);

    return obj;
}

std::string CreateEncodedProposalObject(const UniValue& objJSON)
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
    CProposalValidator validator;
    UniValue obj = LoadJSON("src/test/data/proposals-valid.json");
    for(size_t i = 0; i < obj.size(); ++i) {
        const UniValue& objProposal = obj[i];
        std::string strHexData = CreateEncodedProposalObject(objProposal);
        validator.SetHexData(strHexData);
        BOOST_CHECK(validator.ValidateJSON());
        BOOST_CHECK(validator.ValidateName());
        BOOST_CHECK(validator.ValidateURL());
        BOOST_CHECK(validator.ValidateStartEndEpoch());
        BOOST_CHECK(validator.ValidatePaymentAmount());
        BOOST_CHECK(validator.ValidatePaymentAddress());
        BOOST_CHECK(validator.Validate());
        validator.Clear();
    }
}

BOOST_AUTO_TEST_CASE(invalid_proposals_test)
{
    CProposalValidator validator;
    UniValue obj = LoadJSON("src/test/data/proposals-invalid.json");
    for(size_t i = 0; i < obj.size(); ++i) {
        const UniValue& objProposal = obj[i];
        std::string strHexData = CreateEncodedProposalObject(objProposal);
        validator.SetHexData(strHexData);
        BOOST_CHECK(!validator.Validate());
        validator.Clear();
    }
}

BOOST_AUTO_TEST_SUITE_END()
