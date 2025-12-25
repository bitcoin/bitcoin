// Copyright 2014 BitPay Inc.
// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>

#include <univalue/test/fail1.json.h>
#include <univalue/test/fail10.json.h>
#include <univalue/test/fail11.json.h>
#include <univalue/test/fail12.json.h>
#include <univalue/test/fail13.json.h>
#include <univalue/test/fail14.json.h>
#include <univalue/test/fail15.json.h>
#include <univalue/test/fail16.json.h>
#include <univalue/test/fail17.json.h>
#include <univalue/test/fail18.json.h>
#include <univalue/test/fail19.json.h>
#include <univalue/test/fail2.json.h>
#include <univalue/test/fail20.json.h>
#include <univalue/test/fail21.json.h>
#include <univalue/test/fail22.json.h>
#include <univalue/test/fail23.json.h>
#include <univalue/test/fail24.json.h>
#include <univalue/test/fail25.json.h>
#include <univalue/test/fail26.json.h>
#include <univalue/test/fail27.json.h>
#include <univalue/test/fail28.json.h>
#include <univalue/test/fail29.json.h>
#include <univalue/test/fail3.json.h>
#include <univalue/test/fail30.json.h>
#include <univalue/test/fail31.json.h>
#include <univalue/test/fail32.json.h>
#include <univalue/test/fail33.json.h>
#include <univalue/test/fail34.json.h>
#include <univalue/test/fail35.json.h>
#include <univalue/test/fail36.json.h>
#include <univalue/test/fail37.json.h>
#include <univalue/test/fail38.json.h>
#include <univalue/test/fail39.json.h>
#include <univalue/test/fail4.json.h>
#include <univalue/test/fail40.json.h>
#include <univalue/test/fail41.json.h>
#include <univalue/test/fail42.json.h>
#include <univalue/test/fail44.json.h>
#include <univalue/test/fail45.json.h>
#include <univalue/test/fail5.json.h>
#include <univalue/test/fail6.json.h>
#include <univalue/test/fail7.json.h>
#include <univalue/test/fail8.json.h>
#include <univalue/test/fail9.json.h>
#include <univalue/test/pass1.json.h>
#include <univalue/test/pass2.json.h>
#include <univalue/test/pass3.json.h>
#include <univalue/test/pass4.json.h>
#include <univalue/test/round1.json.h>
#include <univalue/test/round2.json.h>
#include <univalue/test/round3.json.h>
#include <univalue/test/round4.json.h>
#include <univalue/test/round5.json.h>
#include <univalue/test/round6.json.h>
#include <univalue/test/round7.json.h>

#include <array>
#include <cassert>
#include <cstdio>
#include <string>

static std::string rtrim(std::string s)
{
    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    return s;
}

static void runtest(std::string filename, const std::string& jdata)
{
    std::string prefix = filename.substr(0, 4);

    bool wantPass = (prefix == "pass") || (prefix == "roun");
    bool wantFail = (prefix == "fail");
    bool wantRoundTrip = (prefix == "roun");
    assert(wantPass || wantFail);

    UniValue val;
    bool testResult = val.read(jdata);

    if (wantPass) {
        assert(testResult == true);
    } else {
        assert(testResult == false);
    }

    if (wantRoundTrip) {
        std::string odata = val.write(0, 0);
        assert(odata == rtrim(jdata));
    }
}

#define TEST_FILE(name) {#name, json_tests::name}
inline constexpr std::array tests{std::to_array<std::tuple<std::string_view, std::string_view>>({
    TEST_FILE(fail1),
    TEST_FILE(fail10),
    TEST_FILE(fail11),
    TEST_FILE(fail12),
    TEST_FILE(fail13),
    TEST_FILE(fail14),
    TEST_FILE(fail15),
    TEST_FILE(fail16),
    TEST_FILE(fail17),
    TEST_FILE(fail18),
    TEST_FILE(fail19),
    TEST_FILE(fail2),
    TEST_FILE(fail20),
    TEST_FILE(fail21),
    TEST_FILE(fail22),
    TEST_FILE(fail23),
    TEST_FILE(fail24),
    TEST_FILE(fail25),
    TEST_FILE(fail26),
    TEST_FILE(fail27),
    TEST_FILE(fail28),
    TEST_FILE(fail29),
    TEST_FILE(fail3),
    TEST_FILE(fail30),
    TEST_FILE(fail31),
    TEST_FILE(fail32),
    TEST_FILE(fail33),
    TEST_FILE(fail34),
    TEST_FILE(fail35),
    TEST_FILE(fail36),
    TEST_FILE(fail37),
    TEST_FILE(fail38), // invalid unicode: only first half of surrogate pair
    TEST_FILE(fail39), // invalid unicode: only second half of surrogate pair
    TEST_FILE(fail4),  // extra comma
    TEST_FILE(fail40), // invalid unicode: broken UTF-8
    TEST_FILE(fail41), // invalid unicode: unfinished UTF-8
    TEST_FILE(fail42), // valid json with garbage following a nul byte
    TEST_FILE(fail44), // unterminated string
    TEST_FILE(fail45), // nested beyond max depth
    TEST_FILE(fail5),
    TEST_FILE(fail6),
    TEST_FILE(fail7),
    TEST_FILE(fail8),
    TEST_FILE(fail9), // extra comma
    TEST_FILE(pass1),
    TEST_FILE(pass2),
    TEST_FILE(pass3),
    TEST_FILE(pass4),
    TEST_FILE(round1), // round-trip test
    TEST_FILE(round2), // unicode
    TEST_FILE(round3), // bare string
    TEST_FILE(round4), // bare number
    TEST_FILE(round5), // bare true
    TEST_FILE(round6), // bare false
    TEST_FILE(round7), // bare null
})};

// Test \u handling
void unescape_unicode_test()
{
    UniValue val;
    bool testResult;
    // Escaped ASCII (quote)
    testResult = val.read("[\"\\u0022\"]");
    assert(testResult);
    assert(val[0].get_str() == "\"");
    // Escaped Basic Plane character, two-byte UTF-8
    testResult = val.read("[\"\\u0191\"]");
    assert(testResult);
    assert(val[0].get_str() == "\xc6\x91");
    // Escaped Basic Plane character, three-byte UTF-8
    testResult = val.read("[\"\\u2191\"]");
    assert(testResult);
    assert(val[0].get_str() == "\xe2\x86\x91");
    // Escaped Supplementary Plane character U+1d161
    testResult = val.read("[\"\\ud834\\udd61\"]");
    assert(testResult);
    assert(val[0].get_str() == "\xf0\x9d\x85\xa1");
}

void no_nul_test()
{
    char buf[] = "___[1,2,3]___";
    UniValue val;
    assert(val.read({buf + 3, 7}));
}

int main(int argc, char* argv[])
{
    for (const auto& [file, json] : tests) {
        runtest(std::string{file}, std::string{json});
    }

    unescape_unicode_test();
    no_nul_test();

    return 0;
}
