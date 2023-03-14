// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace std::string_literals;

BOOST_FIXTURE_TEST_SUITE(streams_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(streams_vector_writer)
{
    unsigned char a(1);
    unsigned char b(2);
    unsigned char bytes[] = { 3, 4, 5, 6 };
    std::vector<unsigned char> vch;

    // Each test runs twice. Serializing a second time at the same starting
    // point should yield the same results, even if the first test grew the
    // vector.

    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 0, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{1, 2}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 0, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{1, 2}}));
    vch.clear();

    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 1, 2}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 1, 2}}));
    vch.clear();

    vch.resize(5, 0);
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 1, 2, 0}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 1, 2, 0}}));
    vch.clear();

    vch.resize(4, 0);
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 3, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 0, 1, 2}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 3, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 0, 1, 2}}));
    vch.clear();

    vch.resize(4, 0);
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 4, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 0, 0, 1, 2}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 4, a, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{0, 0, 0, 0, 1, 2}}));
    vch.clear();

    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 0, bytes);
    BOOST_CHECK((vch == std::vector<unsigned char>{{3, 4, 5, 6}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 0, bytes);
    BOOST_CHECK((vch == std::vector<unsigned char>{{3, 4, 5, 6}}));
    vch.clear();

    vch.resize(4, 8);
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, bytes, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{8, 8, 1, 3, 4, 5, 6, 2}}));
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, vch, 2, a, bytes, b);
    BOOST_CHECK((vch == std::vector<unsigned char>{{8, 8, 1, 3, 4, 5, 6, 2}}));
    vch.clear();
}

BOOST_AUTO_TEST_CASE(streams_vector_reader)
{
    std::vector<unsigned char> vch = {1, 255, 3, 4, 5, 6};

    SpanReader reader{SER_NETWORK, INIT_PROTO_VERSION, vch};
    BOOST_CHECK_EQUAL(reader.size(), 6U);
    BOOST_CHECK(!reader.empty());

    // Read a single byte as an unsigned char.
    unsigned char a;
    reader >> a;
    BOOST_CHECK_EQUAL(a, 1);
    BOOST_CHECK_EQUAL(reader.size(), 5U);
    BOOST_CHECK(!reader.empty());

    // Read a single byte as a signed char.
    signed char b;
    reader >> b;
    BOOST_CHECK_EQUAL(b, -1);
    BOOST_CHECK_EQUAL(reader.size(), 4U);
    BOOST_CHECK(!reader.empty());

    // Read a 4 bytes as an unsigned int.
    unsigned int c;
    reader >> c;
    BOOST_CHECK_EQUAL(c, 100992003U); // 3,4,5,6 in little-endian base-256
    BOOST_CHECK_EQUAL(reader.size(), 0U);
    BOOST_CHECK(reader.empty());

    // Reading after end of byte vector throws an error.
    signed int d;
    BOOST_CHECK_THROW(reader >> d, std::ios_base::failure);

    // Read a 4 bytes as a signed int from the beginning of the buffer.
    SpanReader new_reader{SER_NETWORK, INIT_PROTO_VERSION, vch};
    new_reader >> d;
    BOOST_CHECK_EQUAL(d, 67370753); // 1,255,3,4 in little-endian base-256
    BOOST_CHECK_EQUAL(new_reader.size(), 2U);
    BOOST_CHECK(!new_reader.empty());

    // Reading after end of byte vector throws an error even if the reader is
    // not totally empty.
    BOOST_CHECK_THROW(new_reader >> d, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(streams_vector_reader_rvalue)
{
    std::vector<uint8_t> data{0x82, 0xa7, 0x31};
    SpanReader reader{SER_NETWORK, INIT_PROTO_VERSION, data};
    uint32_t varint = 0;
    // Deserialize into r-value
    reader >> VARINT(varint);
    BOOST_CHECK_EQUAL(varint, 54321U);
    BOOST_CHECK(reader.empty());
}

BOOST_AUTO_TEST_CASE(bitstream_reader_writer)
{
    DataStream data{};

    BitStreamWriter bit_writer{data};
    bit_writer.Write(0, 1);
    bit_writer.Write(2, 2);
    bit_writer.Write(6, 3);
    bit_writer.Write(11, 4);
    bit_writer.Write(1, 5);
    bit_writer.Write(32, 6);
    bit_writer.Write(7, 7);
    bit_writer.Write(30497, 16);
    bit_writer.Flush();

    DataStream data_copy{data};
    uint32_t serialized_int1;
    data >> serialized_int1;
    BOOST_CHECK_EQUAL(serialized_int1, uint32_t{0x7700C35A}); // NOTE: Serialized as LE
    uint16_t serialized_int2;
    data >> serialized_int2;
    BOOST_CHECK_EQUAL(serialized_int2, uint16_t{0x1072}); // NOTE: Serialized as LE

    BitStreamReader bit_reader{data_copy};
    BOOST_CHECK_EQUAL(bit_reader.Read(1), 0U);
    BOOST_CHECK_EQUAL(bit_reader.Read(2), 2U);
    BOOST_CHECK_EQUAL(bit_reader.Read(3), 6U);
    BOOST_CHECK_EQUAL(bit_reader.Read(4), 11U);
    BOOST_CHECK_EQUAL(bit_reader.Read(5), 1U);
    BOOST_CHECK_EQUAL(bit_reader.Read(6), 32U);
    BOOST_CHECK_EQUAL(bit_reader.Read(7), 7U);
    BOOST_CHECK_EQUAL(bit_reader.Read(16), 30497U);
    BOOST_CHECK_THROW(bit_reader.Read(8), std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(streams_serializedata_xor)
{
    std::vector<std::byte> in;

    // Degenerate case
    {
        DataStream ds{in};
        ds.Xor({0x00, 0x00});
        BOOST_CHECK_EQUAL(""s, ds.str());
    }

    in.push_back(std::byte{0x0f});
    in.push_back(std::byte{0xf0});

    // Single character key
    {
        DataStream ds{in};
        ds.Xor({0xff});
        BOOST_CHECK_EQUAL("\xf0\x0f"s, ds.str());
    }

    // Multi character key

    in.clear();
    in.push_back(std::byte{0xf0});
    in.push_back(std::byte{0x0f});

    {
        DataStream ds{in};
        ds.Xor({0xff, 0x0f});
        BOOST_CHECK_EQUAL("\x0f\x00"s, ds.str());
    }
}

BOOST_AUTO_TEST_CASE(streams_buffered_file)
{
    fs::path streams_test_filename = m_args.GetDataDirBase() / "streams_test_tmp";
    FILE* file = fsbridge::fopen(streams_test_filename, "w+b");

    // The value at each offset is the offset.
    for (uint8_t j = 0; j < 40; ++j) {
        fwrite(&j, 1, 1, file);
    }
    rewind(file);

    // The buffer size (second arg) must be greater than the rewind
    // amount (third arg).
    try {
        CBufferedFile bfbad(file, 25, 25, 222, 333);
        BOOST_CHECK(false);
    } catch (const std::exception& e) {
        BOOST_CHECK(strstr(e.what(),
                        "Rewind limit must be less than buffer size") != nullptr);
    }

    // The buffer is 25 bytes, allow rewinding 10 bytes.
    CBufferedFile bf(file, 25, 10, 222, 333);
    BOOST_CHECK(!bf.eof());

    // These two members have no functional effect.
    BOOST_CHECK_EQUAL(bf.GetType(), 222);
    BOOST_CHECK_EQUAL(bf.GetVersion(), 333);

    uint8_t i;
    bf >> i;
    BOOST_CHECK_EQUAL(i, 0);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 1);

    // After reading bytes 0 and 1, we're positioned at 2.
    BOOST_CHECK_EQUAL(bf.GetPos(), 2U);

    // Rewind to offset 0, ok (within the 10 byte window).
    BOOST_CHECK(bf.SetPos(0));
    bf >> i;
    BOOST_CHECK_EQUAL(i, 0);

    // We can go forward to where we've been, but beyond may fail.
    BOOST_CHECK(bf.SetPos(2));
    bf >> i;
    BOOST_CHECK_EQUAL(i, 2);

    // If you know the maximum number of bytes that should be
    // read to deserialize the variable, you can limit the read
    // extent. The current file offset is 3, so the following
    // SetLimit() allows zero bytes to be read.
    BOOST_CHECK(bf.SetLimit(3));
    try {
        bf >> i;
        BOOST_CHECK(false);
    } catch (const std::exception& e) {
        BOOST_CHECK(strstr(e.what(),
                           "Attempt to position past buffer limit") != nullptr);
    }
    // The default argument removes the limit completely.
    BOOST_CHECK(bf.SetLimit());
    // The read position should still be at 3 (no change).
    BOOST_CHECK_EQUAL(bf.GetPos(), 3U);

    // Read from current offset, 3, forward until position 10.
    for (uint8_t j = 3; j < 10; ++j) {
        bf >> i;
        BOOST_CHECK_EQUAL(i, j);
    }
    BOOST_CHECK_EQUAL(bf.GetPos(), 10U);

    // We're guaranteed (just barely) to be able to rewind to zero.
    BOOST_CHECK(bf.SetPos(0));
    BOOST_CHECK_EQUAL(bf.GetPos(), 0U);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 0);

    // We can set the position forward again up to the farthest
    // into the stream we've been, but no farther. (Attempting
    // to go farther may succeed, but it's not guaranteed.)
    BOOST_CHECK(bf.SetPos(10));
    bf >> i;
    BOOST_CHECK_EQUAL(i, 10);
    BOOST_CHECK_EQUAL(bf.GetPos(), 11U);

    // Now it's only guaranteed that we can rewind to offset 1
    // (current read position, 11, minus rewind amount, 10).
    BOOST_CHECK(bf.SetPos(1));
    BOOST_CHECK_EQUAL(bf.GetPos(), 1U);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 1);

    // We can stream into large variables, even larger than
    // the buffer size.
    BOOST_CHECK(bf.SetPos(11));
    {
        uint8_t a[40 - 11];
        bf >> a;
        for (uint8_t j = 0; j < sizeof(a); ++j) {
            BOOST_CHECK_EQUAL(a[j], 11 + j);
        }
    }
    BOOST_CHECK_EQUAL(bf.GetPos(), 40U);

    // We've read the entire file, the next read should throw.
    try {
        bf >> i;
        BOOST_CHECK(false);
    } catch (const std::exception& e) {
        BOOST_CHECK(strstr(e.what(),
                        "CBufferedFile::Fill: end of file") != nullptr);
    }
    // Attempting to read beyond the end sets the EOF indicator.
    BOOST_CHECK(bf.eof());

    // Still at offset 40, we can go back 10, to 30.
    BOOST_CHECK_EQUAL(bf.GetPos(), 40U);
    BOOST_CHECK(bf.SetPos(30));
    bf >> i;
    BOOST_CHECK_EQUAL(i, 30);
    BOOST_CHECK_EQUAL(bf.GetPos(), 31U);

    // We're too far to rewind to position zero.
    BOOST_CHECK(!bf.SetPos(0));
    // But we should now be positioned at least as far back as allowed
    // by the rewind window (relative to our farthest read position, 40).
    BOOST_CHECK(bf.GetPos() <= 30U);

    // We can explicitly close the file, or the destructor will do it.
    bf.fclose();

    fs::remove(streams_test_filename);
}

BOOST_AUTO_TEST_CASE(streams_buffered_file_skip)
{
    fs::path streams_test_filename = m_args.GetDataDirBase() / "streams_test_tmp";
    FILE* file = fsbridge::fopen(streams_test_filename, "w+b");
    // The value at each offset is the byte offset (e.g. byte 1 in the file has the value 0x01).
    for (uint8_t j = 0; j < 40; ++j) {
        fwrite(&j, 1, 1, file);
    }
    rewind(file);

    // The buffer is 25 bytes, allow rewinding 10 bytes.
    CBufferedFile bf(file, 25, 10, 222, 333);

    uint8_t i;
    // This is like bf >> (7-byte-variable), in that it will cause data
    // to be read from the file into memory, but it's not copied to us.
    bf.SkipTo(7);
    BOOST_CHECK_EQUAL(bf.GetPos(), 7U);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 7);

    // The bytes in the buffer up to offset 7 are valid and can be read.
    BOOST_CHECK(bf.SetPos(0));
    bf >> i;
    BOOST_CHECK_EQUAL(i, 0);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 1);

    bf.SkipTo(11);
    bf >> i;
    BOOST_CHECK_EQUAL(i, 11);

    // SkipTo() honors the transfer limit; we can't position beyond the limit.
    bf.SetLimit(13);
    try {
        bf.SkipTo(14);
        BOOST_CHECK(false);
    } catch (const std::exception& e) {
        BOOST_CHECK(strstr(e.what(), "Attempt to position past buffer limit") != nullptr);
    }

    // We can position exactly to the transfer limit.
    bf.SkipTo(13);
    BOOST_CHECK_EQUAL(bf.GetPos(), 13U);

    bf.fclose();
    fs::remove(streams_test_filename);
}

BOOST_AUTO_TEST_CASE(streams_buffered_file_rand)
{
    // Make this test deterministic.
    SeedInsecureRand(SeedRand::ZEROS);

    fs::path streams_test_filename = m_args.GetDataDirBase() / "streams_test_tmp";
    for (int rep = 0; rep < 50; ++rep) {
        FILE* file = fsbridge::fopen(streams_test_filename, "w+b");
        size_t fileSize = InsecureRandRange(256);
        for (uint8_t i = 0; i < fileSize; ++i) {
            fwrite(&i, 1, 1, file);
        }
        rewind(file);

        size_t bufSize = InsecureRandRange(300) + 1;
        size_t rewindSize = InsecureRandRange(bufSize);
        CBufferedFile bf(file, bufSize, rewindSize, 222, 333);
        size_t currentPos = 0;
        size_t maxPos = 0;
        for (int step = 0; step < 100; ++step) {
            if (currentPos >= fileSize)
                break;

            // We haven't read to the end of the file yet.
            BOOST_CHECK(!bf.eof());
            BOOST_CHECK_EQUAL(bf.GetPos(), currentPos);

            // Pretend the file consists of a series of objects of varying
            // sizes; the boundaries of the objects can interact arbitrarily
            // with the CBufferFile's internal buffer. These first three
            // cases simulate objects of various sizes (1, 2, 5 bytes).
            switch (InsecureRandRange(6)) {
            case 0: {
                uint8_t a[1];
                if (currentPos + 1 > fileSize)
                    continue;
                bf.SetLimit(currentPos + 1);
                bf >> a;
                for (uint8_t i = 0; i < 1; ++i) {
                    BOOST_CHECK_EQUAL(a[i], currentPos);
                    currentPos++;
                }
                break;
            }
            case 1: {
                uint8_t a[2];
                if (currentPos + 2 > fileSize)
                    continue;
                bf.SetLimit(currentPos + 2);
                bf >> a;
                for (uint8_t i = 0; i < 2; ++i) {
                    BOOST_CHECK_EQUAL(a[i], currentPos);
                    currentPos++;
                }
                break;
            }
            case 2: {
                uint8_t a[5];
                if (currentPos + 5 > fileSize)
                    continue;
                bf.SetLimit(currentPos + 5);
                bf >> a;
                for (uint8_t i = 0; i < 5; ++i) {
                    BOOST_CHECK_EQUAL(a[i], currentPos);
                    currentPos++;
                }
                break;
            }
            case 3: {
                // SkipTo is similar to the "read" cases above, except
                // we don't receive the data.
                size_t skip_length{static_cast<size_t>(InsecureRandRange(5))};
                if (currentPos + skip_length > fileSize) continue;
                bf.SetLimit(currentPos + skip_length);
                bf.SkipTo(currentPos + skip_length);
                currentPos += skip_length;
                break;
            }
            case 4: {
                // Find a byte value (that is at or ahead of the current position).
                size_t find = currentPos + InsecureRandRange(8);
                if (find >= fileSize)
                    find = fileSize - 1;
                bf.FindByte(uint8_t(find));
                // The value at each offset is the offset.
                BOOST_CHECK_EQUAL(bf.GetPos(), find);
                currentPos = find;

                bf.SetLimit(currentPos + 1);
                uint8_t i;
                bf >> i;
                BOOST_CHECK_EQUAL(i, currentPos);
                currentPos++;
                break;
            }
            case 5: {
                size_t requestPos = InsecureRandRange(maxPos + 4);
                bool okay = bf.SetPos(requestPos);
                // The new position may differ from the requested position
                // because we may not be able to rewind beyond the rewind
                // window, and we may not be able to move forward beyond the
                // farthest position we've reached so far.
                currentPos = bf.GetPos();
                BOOST_CHECK_EQUAL(okay, currentPos == requestPos);
                // Check that we can position within the rewind window.
                if (requestPos <= maxPos &&
                    maxPos > rewindSize &&
                    requestPos >= maxPos - rewindSize) {
                    // We requested a position within the rewind window.
                    BOOST_CHECK(okay);
                }
                break;
            }
            }
            if (maxPos < currentPos)
                maxPos = currentPos;
        }
    }
    fs::remove(streams_test_filename);
}

BOOST_AUTO_TEST_CASE(streams_hashed)
{
    CDataStream stream(SER_NETWORK, INIT_PROTO_VERSION);
    HashedSourceWriter hash_writer{stream};
    const std::string data{"bitcoin"};
    hash_writer << data;

    CHashVerifier hash_verifier{&stream};
    std::string result;
    hash_verifier >> result;
    BOOST_CHECK_EQUAL(data, result);
    BOOST_CHECK_EQUAL(hash_writer.GetHash(), hash_verifier.GetHash());
}

BOOST_AUTO_TEST_SUITE_END()
