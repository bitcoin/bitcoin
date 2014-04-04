#include "util.h"

#include "data/ecc32_encode_decode.json.h"
#include "data/ecc32_recover.json.h"

#include <cstdlib>

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

using namespace json_spirit;
extern Array read_json(const std::string& jsondata);

std::string ErrorMapString(const std::map<size_t, char> &mapErrors)
{
    std::ostringstream ss;
    ss << '{';
    std::map<size_t, char>::const_iterator itr;
    for (itr = mapErrors.begin(); itr != mapErrors.end(); ++itr) {
        if (itr != mapErrors.begin())
            ss << ",";
        ss << '"' << itr->first << "\":\"" << itr->second << '\"';
    }
    ss << '}';
    return ss.str();
}

BOOST_AUTO_TEST_SUITE(ecc32_tests)

BOOST_AUTO_TEST_CASE(ecc32_AddErrorCorrectionCode32)
{
    Array tests = read_json(std::string(
        json_tests::ecc32_encode_decode,
        json_tests::ecc32_encode_decode +
            sizeof(json_tests::ecc32_encode_decode)));
    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        std::string strTest = write_string(tv, false);
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string base32string = test[0].get_str();
        std::string  ecc32string = test[1].get_str();
        std::string result = AddErrorCorrectionCode32(base32string);
        BOOST_CHECK_MESSAGE(result == ecc32string, strTest);
    }
}

BOOST_AUTO_TEST_CASE(ecc32_RemoveErrorCorrectionCode32_basic)
{
    Array tests = read_json(std::string(
        json_tests::ecc32_encode_decode,
        json_tests::ecc32_encode_decode +
            sizeof(json_tests::ecc32_encode_decode)));
    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        std::string strTest = write_string(tv, false);
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string base32string = test[0].get_str();
        std::string  ecc32string = test[1].get_str();
        bool fInvalid; std::map<size_t, char> mapErrors;
        std::string result = RemoveErrorCorrectionCode32(
            ecc32string, &fInvalid, &mapErrors);
        BOOST_CHECK_MESSAGE(fInvalid == false,
            strTest + " - fInvalid=true");
        BOOST_CHECK_MESSAGE(result == base32string,
            strTest + " - \"" + result + "\"" );
        BOOST_CHECK_MESSAGE(mapErrors.size() == 0,
            strTest + " - mapErrors not empty");
    }
}

BOOST_AUTO_TEST_CASE(ecc32_RemoveErrorCorrectionCode32_recover)
{
    Array tests = read_json(std::string(
        json_tests::ecc32_recover,
        json_tests::ecc32_recover +
            sizeof(json_tests::ecc32_recover)));
    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        std::string strTest = write_string(tv, false);
        if (test.size() < 3) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string  ecc32string = test[0].get_str();
        std::string base32string = test[1].get_str();
        Object         errordict = test[2].get_obj();
        bool fInvalid; std::map<size_t, char> mapErrors;
        std::string result = RemoveErrorCorrectionCode32(
            ecc32string, &fInvalid, &mapErrors);
        BOOST_CHECK_MESSAGE(fInvalid == false,
            strTest + " - fInvalid=true");
        BOOST_CHECK_MESSAGE(result == base32string,
            strTest + " - \"" + result + "\"" );
        size_t nErrors=0;
        BOOST_FOREACH(const Pair& e, errordict)
        {
            ++nErrors;
            int  key   = std::atoi(e.name_.c_str());
            char value = mapErrors[key];
            BOOST_CHECK_MESSAGE(e.value_.get_str() == std::string(1, value),
                strTest + " - " + "mapErrors=" + ErrorMapString(mapErrors));
        }
        BOOST_CHECK_MESSAGE(nErrors == mapErrors.size(),
            strTest + " - " + "mapErrors=" + ErrorMapString(mapErrors));
    }

    bool fInvalid;
    RemoveErrorCorrectionCode32("invalid", &fInvalid);
    BOOST_CHECK_MESSAGE(fInvalid,
        "decoding of invalid ecc32 message thinks it was successful");
}

BOOST_AUTO_TEST_SUITE_END()
