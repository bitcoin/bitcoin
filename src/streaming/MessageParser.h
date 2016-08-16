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
#ifndef MESSAGEPARSER_H
#define MESSAGEPARSER_H

#include "ConstBuffer.h"

#include <vector>
#include <string>
#include <cstdint>
#include <uint256.h>

#include <boost/variant.hpp>
#include <boost/utility/string_ref.hpp>


class Message;

namespace Streaming {

enum ParsedType {
    FoundTag,
    EndOfDocument,
    Error
};

typedef boost::variant<int32_t, bool, uint64_t, std::string, std::vector<char>, double> variant;

class MessageParser
{
public:
    MessageParser(const ConstBuffer &buffer);
    MessageParser(const char *buffer, int length);

    ParsedType next();
    uint32_t peekNext(bool *success = 0) const;

    uint32_t tag() const;
    variant data();

    inline bool isInt() const {
        return m_valueState == ValueParsed && m_value.which() == 0;
    }
    inline bool isLong() const {
        return m_valueState == ValueParsed && (m_value.which() == 2 || m_value.which() == 0);
    }
    inline bool isString() const {
        return m_valueState == LazyString || m_value.which() == 3;
    }
    inline bool isBool() const {
        return m_valueState == ValueParsed && m_value.which() == 1;
    }
    inline bool isByteArray() const {
        return m_valueState == LazyByteArray || m_value.which() == 4;
    }
    inline bool isDouble() const {
        return m_valueState == ValueParsed && m_value.which() == 5;
    }

    int32_t intData() const;
    uint64_t longData() const;
    double doubleData() const;
    std::string stringData();
    boost::string_ref rstringData() const;
    bool boolData() const;
    std::vector<char> bytesData() const;
    /// Backwards compatible unsigned char vector.
    std::vector<unsigned char> unsignedBytesData() const;
    /// Return the amount of bytes a bytesData() would return
    int bytesDataLength() const;
    uint256 uint256Data() const;

    /// return the amount of bytes consumed up-including the latest parsed tag.
    inline int consumed() const {
        return m_position;
    }

    /// consume a number of bytes without parsing.
    void consume(int bytes);

    static void debugMessage(const Message &message);

    static int32_t read32int(const char *buffer);
    static int16_t read16int(const char *buffer);

private:
    const char *m_privData;
    int m_length;
    int m_position;
    bool m_validTagFound;
    variant m_value;
    uint32_t m_tag;

    enum LazyState {
        ValueParsed,
        LazyByteArray,
        LazyString
    };
    LazyState m_valueState;
    int m_dataStart;
    int m_dataLength;

    ConstBuffer m_constBuffer; // we just have this here to make sure its refcounted
};

}
#endif
