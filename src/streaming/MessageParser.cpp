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
#include "MessageParser.h"
#include "MessageBuilder_p.h"
#include <Message.h>

#include <boost/algorithm/hex.hpp>
#include <assert.h>

#include <util.h>

namespace {
    bool unserialize(const char *data, int dataSize, int &position, uint64_t &result)
    {
        assert(data);
        assert(result == 0);
        assert(position >= 0);
        int pos = position;
        while (pos - position < 8 && pos < dataSize) {
            unsigned char byte = data[pos++];
            result = (result << 7) | (byte & 0x7F);
            if (byte & 0x80)
                result++;
            else {
                position = pos;
                return true;
            }
        }
        return false;
    }
}

Streaming::MessageParser::MessageParser(const Streaming::ConstBuffer &buffer)
    : m_privData(buffer.begin()),
    m_length(buffer.size()),
    m_position(0),
    m_validTagFound(false),
    m_tag(0),
    m_valueState(ValueParsed),
    m_dataStart(-1),
    m_dataLength(-1),
    m_constBuffer(buffer)
{
}

Streaming::MessageParser::MessageParser(const char *buffer, int length)
    : m_privData(buffer),
    m_length(length),
    m_position(0),
    m_validTagFound(false),
    m_tag(0),
    m_valueState(ValueParsed),
    m_dataStart(-1),
    m_dataLength(-1)
{
    assert(buffer);
}

Streaming::ParsedType Streaming::MessageParser::next()
{
    if (m_length <= m_position)
        return EndOfDocument;

    uint8_t byte = m_privData[m_position];
    ValueType type = static_cast<ValueType>(byte & 0x07);
    m_valueState = ValueParsed;

    m_tag = byte >> 3;
    if (m_tag == 31) { // the tag is stored in the next byte(s)
        uint64_t tag = 0;
        bool ok = unserialize(m_privData, m_length, ++m_position, tag);
        if (!ok || tag > 0xFFFFFFFF) {
            --m_position;
            return Error;
        }
        --m_position;
        m_tag = static_cast<uint32_t>(tag);
    }

    uint64_t value = 0;
    switch (type) {
    case PositiveNumber:
    case NegativeNumber: {
        bool ok = unserialize(m_privData, m_length, ++m_position, value);
        if (!ok) {
            --m_position;
            return Error;
        }
        if (type == NegativeNumber)
            m_value = (int32_t) (value * -1);
        else if (value < 0x7FFFFFFF)
            m_value = (int32_t) value;
        else
            m_value = value;
        break;
    }
    case ByteArray:
    case String: {
        int newPos = m_position + 1;
        bool ok = unserialize(m_privData, m_length, newPos, value);
        if (!ok)
            return Error;
        if (newPos + value > (unsigned int) m_length) // need more bytes
            return Error;

        m_valueState = type == ByteArray ? LazyByteArray : LazyString;
        m_dataStart = newPos;
        m_dataLength = value;
        m_position = newPos + value;
        break;
    }
    case BoolTrue:
        m_value = true;
        ++m_position;
        break;
    case BoolFalse:
        m_value = false;
        ++m_position;
        break;
    case Double: {
        double tmp;
        memcpy(&tmp, m_privData + ++m_position, 8);
        m_value = tmp;
        m_position += 8;
        break;
    }
    default:
        return Error;
    }
    return FoundTag;
}

uint32_t Streaming::MessageParser::peekNext(bool *success) const
{
    if (m_length <= m_position) {
        if (success) (*success) = false;
        return 0;
    }

    int pos = m_position;
    uint8_t byte = m_privData[pos];
    uint32_t answer = byte >> 3;
    if (answer == 31) { // the tag is stored in the next byte(s)
        uint64_t tag = 0;
        bool ok = unserialize(m_privData, m_length, ++pos, tag);
        if (!ok || tag > 0xFFFFFFFF) {
            if (success) (*success) = false;
            return 0;
        }
        answer = static_cast<uint32_t>(tag);
    }
    if (success) (*success) = true;
    return answer;
}

uint32_t Streaming::MessageParser::tag() const
{
    return m_tag;
}

Streaming::variant Streaming::MessageParser::data()
{
    if (m_valueState != ValueParsed) {
        // It hurts me that in order to use std::variant this code does two copies, one copy should be enough.
        if (m_valueState == LazyByteArray) {
             std::vector<char> data;
             data.resize(m_dataLength);
             memcpy(&data[0], m_privData + m_dataStart, m_dataLength);
             m_value = data;
        } else {
            m_value = std::string(m_privData + m_dataStart, m_dataLength);
        }
        m_valueState = ValueParsed;
    }

    return m_value;
}

int32_t Streaming::MessageParser::intData() const
{
    if (isInt())
        return boost::get<int32_t>(m_value);
    if (isLong())
        return static_cast<int32_t>(boost::get<uint64_t>(m_value));
    return 0;
}

uint64_t Streaming::MessageParser::longData() const
{
    if (m_value.which() == 0)
        return boost::get<int32_t>(m_value);
    if (m_value.which() == 2)
        return boost::get<uint64_t>(m_value);
    return 0;
}

double Streaming::MessageParser::doubleData() const
{
    if (m_value.which() == 5)
        return boost::get<double>(m_value);
    return 0.0;
}

std::string Streaming::MessageParser::stringData()
{
    if (!isString())
        return std::string();
    if (m_valueState == LazyString)
        return std::string(m_privData + m_dataStart, m_dataLength);
    return boost::get<std::string>(m_value);
}

boost::string_ref Streaming::MessageParser::rstringData() const
{
    if (!isString() && !isByteArray())
        return boost::string_ref();
    return boost::string_ref(m_privData + m_dataStart, m_dataLength);
}

bool Streaming::MessageParser::boolData() const
{
    if (isBool())
        return boost::get<bool>(m_value);
    return false;
}

std::vector<char> Streaming::MessageParser::bytesData() const
{
    if (!isByteArray())
        return std::vector<char>();
    if (m_valueState == LazyByteArray) {
        std::vector<char> data;
        data.resize(m_dataLength);
        memcpy(&data[0], m_privData + m_dataStart, m_dataLength);
        return data;
    }
    return boost::get<std::vector<char> >(m_value);
}

std::vector<unsigned char> Streaming::MessageParser::unsignedBytesData() const
{
    if (!isByteArray())
        return std::vector<unsigned char>();
    std::vector<unsigned char> data;
    data.resize(m_dataLength);
    memcpy(&data[0], m_privData + m_dataStart, m_dataLength);
    return data;
}

int Streaming::MessageParser::bytesDataLength() const
{
    if (!isByteArray())
        return 0;
    return m_dataLength;
}

uint256 Streaming::MessageParser::uint256Data() const
{
    if (!isByteArray() || m_dataLength < 32)
        return uint256();
    assert(m_valueState == LazyByteArray);
    return uint256(m_privData + m_dataStart);
}

void Streaming::MessageParser::consume(int bytes)
{
    assert(bytes >= 0);
    m_position += bytes;
}

void Streaming::MessageParser::debugMessage(const Message &message)
{
    MessageParser parser(message.body());
    Streaming::ParsedType type;
    while(true) {
        type = parser.next();
        if (type != Streaming::FoundTag)
            break;
        if (parser.isInt())
            LogPrintf(" + %d = %d\n", parser.tag(), parser.intData());
        else if (parser.isLong())
            LogPrintf(" + %d = %ld\n", parser.tag(), parser.longData());
        else if (parser.isString())
            LogPrintf(" + %d = \"%s\"\n", parser.tag(), parser.stringData().c_str());
        else if (parser.isBool())
            LogPrintf(" + %d = %s\n", parser.tag(), parser.boolData() ? "true" : "false");
        else if (parser.isByteArray()) {
            std::string output;
            boost::algorithm::hex(parser.bytesData(), back_inserter(output));
            if (output.size() > 400)
                output = output.substr(0, 400);
            LogPrintf(" + %d = %s\n", parser.tag(), output.c_str());
        }
        else if (parser.isDouble())
            LogPrintf(" + %d = %E\n", parser.tag(), parser.doubleData());
        else
            LogPrintf(" + %d = [unknown]\n", parser.tag());
    }
}

int32_t Streaming::MessageParser::read32int(const char *buffer)
{
    unsigned int answer = buffer[3] & 0xFF;
    answer = answer << 8;
    answer += buffer[2] & 0xFF;
    answer = answer << 8;
    answer += buffer[1] & 0xFF;
    answer = answer << 8;
    answer += buffer[0] & 0xFF;
    return answer;
}

int16_t Streaming::MessageParser::read16int(const char *buffer)
{
    unsigned int answer = buffer[1] & 0xFF;
    answer = answer << 8;
    answer += buffer[0] & 0xFF;
    return answer;
}
