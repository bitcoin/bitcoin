#include "util.h"

#include "data/ecc32_encode_decode.json.h"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

using namespace json_spirit;
extern Array read_json(const std::string& jsondata);

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

BOOST_AUTO_TEST_CASE(ecc32_RemoveErrorCorrectionCode32)
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
        bool fInvalid; std::map<size_t, char> pMapErrors;
        std::string result = RemoveErrorCorrectionCode32(
            ecc32string, &fInvalid, &pMapErrors);
        BOOST_CHECK_MESSAGE(fInvalid == false,
            strTest + " - fInvalid=true");
        BOOST_CHECK_MESSAGE(result == base32string,
            strTest + " - \"" + result + "\"" );
        // FIXME: check pMapErrors
    }
}

BOOST_AUTO_TEST_SUITE_END()
