// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <test/util/setup_common.h>
#include <util/readwritefile.h>

#include <fstream>
#include <optional>
#include <stdint.h>
#include <string.h>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(readwritefile_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(util_ReadBinaryFile)
{
    fs::path tmpfolder = m_args.GetDataDirBase();
    fs::path tmpfile = tmpfolder / "read_binary.dat";
    std::string expected_text;
    for (int i = 0; i < 30; i++) {
        expected_text += "0123456789";
    }
    {
        std::ofstream file{tmpfile};
        file << expected_text;
    }
    {
        // read all contents in file
        auto text{ReadBinaryFile<std::string>(tmpfile)};
        BOOST_REQUIRE(text);
        BOOST_CHECK_EQUAL(text.value(), expected_text);
    }
    {
        // read half contents in file
        auto text{ReadBinaryFile<std::string>(tmpfile, expected_text.size() / 2)};
        BOOST_REQUIRE(text);
        BOOST_CHECK_EQUAL(text.value(), expected_text.substr(0, expected_text.size() / 2));
    }
    {
        // read from non-existent file
        fs::path invalid_file = tmpfolder / "invalid_binary.dat";
        auto text{ReadBinaryFile<std::string>(invalid_file)};
        BOOST_REQUIRE(!text);
    }
    {
        std::vector<unsigned char> expected_data;
        for (int i = 0; i < 30; i++) {
            // store result as bytes (ASCII "0" is character 48)
            expected_data.insert(expected_data.end(), {48,49,50,51,52,53,54,55,56,57});
        }
        auto data{ReadBinaryFile<std::vector<unsigned char>>(tmpfile)};
        BOOST_REQUIRE(data);
        BOOST_REQUIRE(data.value() == expected_data);
    }
}

BOOST_AUTO_TEST_CASE(util_WriteBinaryFile)
{
    fs::path tmpfolder = m_args.GetDataDirBase();
    fs::path tmpfile = tmpfolder / "write_binary.dat";
    std::string expected_text = "bitcoin";
    {
        auto valid = WriteBinaryFile(tmpfile, expected_text);
        BOOST_CHECK(valid);
        std::string actual_text;
        std::ifstream file{tmpfile};
        file >> actual_text;
        BOOST_CHECK_EQUAL(actual_text, expected_text);
    }
    {
        std::vector<unsigned char> bytes{98, 105, 116, 99, 111, 105, 110}; // "bitcoin"
        auto valid = WriteBinaryFile(tmpfile, bytes);
        BOOST_CHECK(valid);
        std::string actual_text;
        std::ifstream file{tmpfile};
        file >> actual_text;
        BOOST_CHECK_EQUAL(actual_text, expected_text);
    }

}

BOOST_AUTO_TEST_SUITE_END()
