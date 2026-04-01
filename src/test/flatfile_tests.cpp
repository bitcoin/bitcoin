// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <common/args.h>
#include <flatfile.h>
#include <streams.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(flatfile_open_readonly_missing_dir)
{
    // Open file with read_only=true on a directory that does not exist.
    // File should be null instead of throwing an exception.
    const auto data_dir = m_args.GetDataDirBase() / "nonexistent";
    const FlatFileSeq seq(data_dir, "a", 16 * 1024);

    const AutoFile file{seq.Open(FlatFilePos(0, 0), /*read_only=*/true)};
    BOOST_CHECK(file.IsNull());
}

BOOST_AUTO_TEST_CASE(flatfile_open_write_creates_dir)
{
    // Open with read_only=false on a directory that does not yet exist
    // should create it and succeed.
    const auto data_dir = m_args.GetDataDirBase() / "new_write_dir";
    const FlatFileSeq seq(data_dir, "a", 16 * 1024);

    const AutoFile file{seq.Open(FlatFilePos(0, 0), /*read_only=*/false)};
    BOOST_CHECK(!file.IsNull());
    BOOST_CHECK(fs::exists(data_dir));
}

BOOST_AUTO_TEST_CASE(flatfile_open_write_missing_unwritable_parent)
{
    // Open with read_only=false when the parent directory cannot be created
    const auto data_dir = m_args.GetDataDirBase();
    const auto dir = data_dir / "parent_dir";
    fs::create_directories(dir);

    // Make it read-only so creating subdirectories fails.
    SimulateFileSystemError(m_path_root, dir, [&dir]() {
        const FlatFileSeq seq(dir / "blocks", "a", 16 * 1024);
        const AutoFile file{seq.Open(FlatFilePos(0, 0), /*read_only=*/false)};
        BOOST_CHECK(file.IsNull());
    });
}

BOOST_AUTO_TEST_CASE(flatfile_flush_unwritable_dir)
{
    // Flush returns false when the file cannot be opened for writing.
    const auto data_dir = m_args.GetDataDirBase() / "flush_dir";
    fs::create_directories(data_dir);
    const FlatFileSeq seq(data_dir, "a", 100);

    SimulateFileSystemError(m_path_root, data_dir, [&seq]() {
        BOOST_CHECK(!seq.Flush(FlatFilePos(0, 1)));
    });
}

BOOST_AUTO_TEST_CASE(flatfile_allocate_unwritable_dir)
{
    // Allocate should return 0 when the file cannot be opened for writing.
    // Note that out_of_space stays false. Callers that only check
    // out_of_space will miss this failure.
    const auto data_dir = m_args.GetDataDirBase() / "alloc_dir";
    fs::create_directories(data_dir);
    const FlatFileSeq seq(data_dir, "a", 100);

    SimulateFileSystemError(m_path_root, data_dir, [&seq]() {
        bool out_of_space;
        // Note: this throws due to 'CheckDiskSpace'. In the future, this shouldn't throw either.
        BOOST_CHECK_THROW(seq.Allocate(FlatFilePos(0, 0), 1, out_of_space), fs::filesystem_error);
        BOOST_CHECK(!out_of_space);
    });
}

BOOST_AUTO_TEST_SUITE_END()
