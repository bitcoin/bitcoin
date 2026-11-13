// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <common/args.h>
#include <flatfile.h>
#include <streams.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>

BOOST_FIXTURE_TEST_SUITE(flatfile_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(flatfile_filename)
{
    const auto data_dir = m_args.GetDataDirBase();

    FlatFilePos pos(456, 789);

    FlatFileSeq seq1(data_dir, "a", 16 * 1024);
    BOOST_CHECK_EQUAL(seq1.FileName(pos), data_dir / "a00456.dat");

    FlatFileSeq seq2(data_dir / "a", "b", 16 * 1024);
    BOOST_CHECK_EQUAL(seq2.FileName(pos), data_dir / "a" / "b00456.dat");

    // Check default constructor IsNull
    assert(FlatFilePos{}.IsNull());
}

BOOST_AUTO_TEST_CASE(flatfile_open)
{
    const auto data_dir = m_args.GetDataDirBase();
    FlatFileSeq seq(data_dir, "a", 16 * 1024);

    std::string line1("A purely peer-to-peer version of electronic cash would allow online "
                      "payments to be sent directly from one party to another without going "
                      "through a financial institution.");
    std::string line2("Digital signatures provide part of the solution, but the main benefits are "
                      "lost if a trusted third party is still required to prevent double-spending.");

    uint64_t pos1{0};
    uint64_t pos2{pos1 + GetSerializeSize(line1)};

    // Write first line to file.
    {
        AutoFile file{seq.Open(FlatFilePos(0, pos1))};
        file << LIMITED_STRING(line1, 256);
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    // Attempt to append to file opened in read-only mode.
    {
        AutoFile file{seq.Open(FlatFilePos(0, pos2), true)};
        BOOST_CHECK_THROW(file << LIMITED_STRING(line2, 256), std::ios_base::failure);
    }

    // Append second line to file.
    {
        AutoFile file{seq.Open(FlatFilePos(0, pos2))};
        file << LIMITED_STRING(line2, 256);
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    // Read text from file in read-only mode.
    {
        std::string text;
        AutoFile file{seq.Open(FlatFilePos(0, pos1), true)};

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line1);

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line2);
    }

    // Read text from file with position offset.
    {
        std::string text;
        AutoFile file{seq.Open(FlatFilePos(0, pos2))};

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line2);
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    // Ensure another file in the sequence has no data.
    {
        std::string text;
        AutoFile file{seq.Open(FlatFilePos(1, pos2))};
        BOOST_CHECK_THROW(file >> LIMITED_STRING(text, 256), std::ios_base::failure);
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }
}

BOOST_AUTO_TEST_CASE(flatfile_pos_serialization)
{
    for (const uint64_t offset : {uint64_t{0},
                                  (uint64_t{1} << 31) - 1, uint64_t{1} << 31, (uint64_t{1} << 31) + 1,
                                  (uint64_t{1} << 32) - 1, uint64_t{1} << 32, (uint64_t{1} << 32) + 1,
                                  std::numeric_limits<uint64_t>::max()}) {
        BOOST_TEST_CONTEXT("offset=" << offset)
        {
            // Decode an independently encoded 64-bit offset, not one already narrowed by FlatFilePos.
            DataStream encoded{};
            const int32_t file_number{3};
            encoded << VARINT_MODE(file_number, VarIntMode::NONNEGATIVE_SIGNED) << VARINT(offset);
            FlatFilePos decoded;
            encoded >> decoded;
            BOOST_CHECK_EQUAL(decoded.nFile, file_number);
            BOOST_CHECK_EQUAL(decoded.nPos, offset);
            BOOST_CHECK(encoded.empty());

            DataStream serialized{};
            serialized << FlatFilePos(file_number, offset);
            int32_t decoded_file{0};
            uint64_t decoded_offset{0};
            serialized >> VARINT_MODE(decoded_file, VarIntMode::NONNEGATIVE_SIGNED) >> VARINT(decoded_offset);
            BOOST_CHECK_EQUAL(decoded_file, file_number);
            BOOST_CHECK_EQUAL(decoded_offset, offset);
            BOOST_CHECK(serialized.empty());
        }
    }
}

BOOST_AUTO_TEST_CASE(flatfile_large_position)
{
#ifndef WIN32
    // Non-Windows FileSeek64 still uses fseek, whose offset is a long.
    if constexpr (sizeof(long) < sizeof(int64_t)) {
        BOOST_WARN_MESSAGE(false, "Skipping large file seeks: fseek has a 32-bit offset");
        return;
    }
#endif
    FlatFileSeq seq{m_args.GetDataDirBase(), "large", 100};
    {
        AutoFile file{seq.Open(FlatFilePos(0, 0))};
        BOOST_REQUIRE(!file.IsNull());
        file << uint8_t{0x5a};
        BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    }

    for (const uint64_t offset : {(uint64_t{1} << 31) - 1, uint64_t{1} << 31, (uint64_t{1} << 31) + 1,
                                  (uint64_t{1} << 32) - 1, uint64_t{1} << 32, (uint64_t{1} << 32) + 1}) {
        for (const bool read_only : {false, true}) {
            BOOST_TEST_CONTEXT("offset=" << offset << ", read_only=" << read_only)
            {
                // Seeking past EOF does not allocate a large file. Never write at this position.
                AutoFile file{seq.Open(FlatFilePos(0, offset), read_only)};
                BOOST_REQUIRE(!file.IsNull());
                BOOST_CHECK_EQUAL(file.tell(), offset);
                BOOST_CHECK_EQUAL(file.size(), 1);
                BOOST_CHECK_EQUAL(file.tell(), offset);
                file.seek(-1, SEEK_CUR);
                BOOST_CHECK_EQUAL(file.tell(), offset - 1);
                file.seek(offset, SEEK_SET);
                BOOST_CHECK_EQUAL(file.tell(), offset);
                file.seek(0, SEEK_END);
                BOOST_CHECK_EQUAL(file.tell(), 1);
                file.seek(0, SEEK_SET);
                uint8_t marker{0};
                file >> marker;
                BOOST_CHECK_EQUAL(marker, 0x5a);
            }
        }
    }
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 0))), 1);
}

BOOST_AUTO_TEST_CASE(flatfile_allocate)
{
    const auto data_dir = m_args.GetDataDirBase();
    FlatFileSeq seq(data_dir, "a", 100);

    bool out_of_space;

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 0), 1, out_of_space), 100U);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 0))), 100U);
    BOOST_CHECK(!out_of_space);

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 99), 1, out_of_space), 0U);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 99))), 100U);
    BOOST_CHECK(!out_of_space);

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 99), 2, out_of_space), 101U);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 99))), 200U);
    BOOST_CHECK(!out_of_space);
}

BOOST_AUTO_TEST_CASE(flatfile_flush)
{
    const auto data_dir = m_args.GetDataDirBase();
    FlatFileSeq seq(data_dir, "a", 100);

    bool out_of_space;
    seq.Allocate(FlatFilePos(0, 0), 1, out_of_space);

    // Flush without finalize should not truncate file.
    seq.Flush(FlatFilePos(0, 1));
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 1))), 100U);

    // Flush with finalize should truncate file.
    seq.Flush(FlatFilePos(0, 1), true);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 1))), 1U);
}

BOOST_AUTO_TEST_SUITE_END()
