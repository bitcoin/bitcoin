// Copyright 2014 BitPay Inc.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>

#include <cassert>
#include <cstdio>
#include <string>

#ifndef JSON_TEST_SRC
#error JSON_TEST_SRC must point to test source directory
#endif

std::string srcdir(JSON_TEST_SRC);

static std::string rtrim(std::string s)
{
    s.erase(s.find_last_not_of(" \n\r\t")+1);
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

static void runtest_file(const char *filename_)
{
        std::string basename(filename_);
        std::string filename = srcdir + "/" + basename;
        FILE *f = fopen(filename.c_str(), "r");
        assert(f != nullptr);

        std::string jdata;

        char buf[4096];
        while (!feof(f)) {
                int bread = fread(buf, 1, sizeof(buf), f);
                assert(!ferror(f));

                std::string s(buf, bread);
                jdata += s;
        }

        assert(!ferror(f));
        fclose(f);

        runtest(basename, jdata);
}

static const char *filenames[] = {
        "fail10.json",
        "fail11.json",
        "fail12.json",
        "fail13.json",
        "fail14.json",
        "fail15.json",
        "fail16.json",
        "fail17.json",
        //"fail18.json",             // investigate
        "fail19.json",
        "fail1.json",
        "fail20.json",
        "fail21.json",
        "fail22.json",
        "fail23.json",
        "fail24.json",
        "fail25.json",
        "fail26.json",
        "fail27.json",
        "fail28.json",
        "fail29.json",
        "fail2.json",
        "fail30.json",
        "fail31.json",
        "fail32.json",
        "fail33.json",
        "fail34.json",
        "fail35.json",
        "fail36.json",
        "fail37.json",
        "fail38.json",               // invalid unicode: only first half of surrogate pair
        "fail39.json",               // invalid unicode: only second half of surrogate pair
        "fail40.json",               // invalid unicode: broken UTF-8
        "fail41.json",               // invalid unicode: unfinished UTF-8
        "fail42.json",               // valid json with garbage following a nul byte
        "fail44.json",               // unterminated string
        "fail45.json",               // nested beyond max depth
        "fail3.json",
        "fail4.json",                // extra comma
        "fail5.json",
        "fail6.json",
        "fail7.json",
        "fail8.json",
        "fail9.json",               // extra comma
        "pass1.json",
        "pass2.json",
        "pass3.json",
        "pass4.json",
        "round1.json",              // round-trip test
        "round2.json",              // unicode
        "round3.json",              // bare string
        "round4.json",              // bare number
        "round5.json",              // bare true
        "round6.json",              // bare false
        "round7.json",              // bare null
};

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
    assert(val.read(buf + 3, 7));
}

int main (int argc, char *argv[])
{
    for (const auto& f: filenames) {
        runtest_file(f);
    }

    unescape_unicode_test();
    no_nul_test();

    return 0;
}

