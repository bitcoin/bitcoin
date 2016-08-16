/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <streaming/BufferPool.h>
#include <streaming/MessageBuilder.h>
#include <streaming/MessageParser.h>

#include <boost/test/auto_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Buffers)

using namespace Streaming;

BOOST_AUTO_TEST_CASE(Basic)
{
    Streaming::BufferPool pool;
    pool.reserve(1000);
    const int maxCapacity = pool.capacity();
    BOOST_CHECK(pool.capacity() >= 1000);
    pool.markUsed(101);
    BOOST_CHECK(pool.capacity() == maxCapacity - 101);
    pool.markUsed(122);
    const int newCapacity = maxCapacity - 101 - 122;
    BOOST_CHECK(pool.capacity() == newCapacity);
    Streaming::ConstBuffer buf1 = pool.commit();
    BOOST_CHECK(pool.capacity() == newCapacity);
    BOOST_CHECK(buf1.size() == 223);

    Streaming::ConstBuffer buf2 = pool.commit(pool.capacity());
    BOOST_CHECK(buf1.size() == 223);
    BOOST_CHECK(buf2.size() == newCapacity);
    BOOST_CHECK(pool.capacity() == 0);
}

BOOST_AUTO_TEST_CASE(MultiBuffer)
{
    Streaming::BufferPool pool(500); // small :)
    BOOST_CHECK(pool.capacity() == 500);
    pool.reserve(1000); // bigger!
    BOOST_CHECK(pool.capacity() >= 1000);

    Streaming::ConstBuffer buf1 = pool.commit(800);
    BOOST_CHECK(pool.capacity() == 200);
    pool.reserve(1000); // won't fit. It should create a new buf.
    BOOST_CHECK(pool.capacity() >= 1000);
    Streaming::ConstBuffer buf2 = pool.commit(800);
    BOOST_CHECK(pool.capacity() >= 200);

    BOOST_CHECK(buf1.internal_buffer() != buf2.internal_buffer());
}

BOOST_AUTO_TEST_CASE(Builder)
{
    MessageBuilder builder(NoHeader);
    builder.add(1, "bla");
    ConstBuffer buf1 = builder.buffer();
    BOOST_CHECK(buf1.size() == 5);
    const char * data = buf1.begin();
    // for (int i = 0; i < 5; ++i) { printf(" %d: %d\n", i, data[i]); }
    BOOST_CHECK(data[0] == 10); // my 1 + 010 (for string) is binary 1010, is decimal 10
    BOOST_CHECK(data[1] == 3); // length of 'bla'
    BOOST_CHECK(data[2] == 'b');
    BOOST_CHECK(data[3] == 'l');
    BOOST_CHECK(data[4] == 'a');
}

BOOST_AUTO_TEST_CASE(Parser)
{
    MessageBuilder builder(NoHeader);
    builder.add(1, "bla");
    builder.add(3, 100);
    builder.add(5, true);
    builder.add(100, false);
    std::vector<char> data;
    data.push_back(5);
    data.push_back(0);
    data.push_back(8);
    data.push_back(254);
    builder.add(6, data);
    builder.add(9, 15.5);
    uint256 origHash;
    origHash.SetHex("1298709234abd981729817291a8798172f871982a798195278312095a7982348");
    builder.add(10, origHash);
    ConstBuffer buf = builder.buffer();
    // printf("size %d\n", buf.size());
    BOOST_CHECK(buf.size() == 59);

    MessageParser parser(buf);
    ParsedType type = parser.next();
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.tag() == 1);
    variant v = parser.data();
    BOOST_CHECK(boost::get<std::string>(v) == std::string("bla"));

    type = parser.next();
    BOOST_CHECK(parser.tag() == 3);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isLong() || parser.isInt());
    BOOST_CHECK(parser.intData() == 100);

    type = parser.next();
    BOOST_CHECK(parser.tag() == 5);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isBool());
    BOOST_CHECK(parser.boolData() == true);

    type = parser.next();
    BOOST_CHECK(parser.tag() == 100);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isBool());
    BOOST_CHECK(parser.boolData() == false);

    type = parser.next();
    BOOST_CHECK(parser.tag() == 6);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isByteArray());
    v = parser.data();
    std::vector<char> byteArray = boost::get<std::vector<char> >(v);
    BOOST_CHECK(byteArray == data);

    type = parser.next();
    BOOST_CHECK(parser.tag() == 9);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isDouble());
    v = parser.data();
    double doubleData = boost::get<double>(v);
    BOOST_CHECK(doubleData == 15.5);
    BOOST_CHECK(parser.doubleData() == 15.5);

    type = parser.next();
    BOOST_CHECK(parser.tag() == 10);
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.isByteArray());
    uint256 hash(parser.unsignedBytesData());
    BOOST_CHECK(origHash == hash);

    type = parser.next();
    BOOST_CHECK(type == EndOfDocument);
}

BOOST_AUTO_TEST_CASE(StringRefInParser)
{
    MessageBuilder builder(NoHeader);
    builder.add(1, "bla");
    builder.add(5, "String");
    ConstBuffer buf = builder.buffer();
    BOOST_CHECK(buf.size() == 13);

    MessageParser parser(buf);
    ParsedType type = parser.next();
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.tag() == 1);
    BOOST_CHECK(parser.isString());
    boost::string_ref ref = parser.rstringData();
    BOOST_CHECK(ref.length() == 3);
    BOOST_CHECK(ref == std::string("bla"));

    type = parser.next();
    BOOST_CHECK(type == FoundTag);
    BOOST_CHECK(parser.tag() == 5);
    ref = parser.rstringData();
    BOOST_CHECK(ref.length() == 6);
    BOOST_CHECK(ref == std::string("String"));
    BOOST_CHECK(parser.isString());

    type = parser.next();
    BOOST_CHECK(type == EndOfDocument);
}

BOOST_AUTO_TEST_CASE(Clear)
{
    BufferPool pool(30000);
    pool.reserve(40000);
    int maxCapacity = pool.capacity();
    BOOST_CHECK(maxCapacity >= 40000);
    pool.markUsed(1000);
    BOOST_CHECK(pool.capacity() == maxCapacity - 1000);

    pool.commit(1000);
    BOOST_CHECK(pool.capacity() == maxCapacity - 2000);

    pool.clear();
    BOOST_CHECK(pool.capacity() == 30000);
    BOOST_CHECK(pool.begin() == nullptr);
    BOOST_CHECK(pool.end() == nullptr);

    pool.reserve(1000);
    BOOST_CHECK(pool.capacity() == 30000);
    BOOST_CHECK(pool.begin() != nullptr);
    BOOST_CHECK(pool.end() != nullptr);

    strcpy(pool.begin(), "bla");
    ConstBuffer buf = pool.commit(4);
    BOOST_CHECK(strncmp(buf.begin(), "bla", 3) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
