// Copyright (c) 2011-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <fs.h>
#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(fs_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(fsbridge_fstream)
{
    fs::path tmpfolder = SetDataDir("fsbridge_fstream");
    // tmpfile1 should be the same as tmpfile2
    fs::path tmpfile1 = tmpfolder / "fs_tests_₿_🏃";
    fs::path tmpfile2 = tmpfolder / L"fs_tests_₿_🏃";
    {
        fsbridge::ofstream file(tmpfile1);
        file << "syscoin";
    }
    {
        fsbridge::ifstream file(tmpfile2);
        std::string input_buffer;
        file >> input_buffer;
        BOOST_CHECK_EQUAL(input_buffer, "syscoin");
    }
    {
        fsbridge::ifstream file(tmpfile1, std::ios_base::in | std::ios_base::ate);
        std::string input_buffer;
        file >> input_buffer;
        BOOST_CHECK_EQUAL(input_buffer, "");
    }
    {
        fsbridge::ofstream file(tmpfile2, std::ios_base::out | std::ios_base::app);
        file << "tests";
    }
    {
        fsbridge::ifstream file(tmpfile1);
        std::string input_buffer;
        file >> input_buffer;
        BOOST_CHECK_EQUAL(input_buffer, "syscointests");
    }
    {
        fsbridge::ofstream file(tmpfile2, std::ios_base::out | std::ios_base::trunc);
        file << "syscoin";
    }
    {
        fsbridge::ifstream file(tmpfile1);
        std::string input_buffer;
        file >> input_buffer;
        BOOST_CHECK_EQUAL(input_buffer, "syscoin");
    }
}

BOOST_AUTO_TEST_SUITE_END()
