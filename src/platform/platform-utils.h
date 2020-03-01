// Copyright (c) 2014-2020 Crown Core developers
// Copyright (c) 2018 EOS developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_UTILS_H
#define CROWN_PLATFORM_UTILS_H

#include <cstdint>
#include <string>

namespace Platform
{
    static bool IsValidSymbol(char c)
    {
        return (( c >= 'a' && c <= 'z' ) || ( c >= '1' && c <= '5' ) || c == '.');
    }

    /**
    *  Converts a base32 symbol into its binary representation, used by StringToProtocolName()
    *
    *  @brief Converts a base32 symbol into its binary representation, used by StringToProtocolName()
    *  @param c - Character to be converted
    *  @return constexpr char - Converted character
    */
    static /*TODO: cpp14 constexpr*/ char CharToSymbol(char c)
    {
        if( c >= 'a' && c <= 'z' )
            return static_cast<char>((c - 'a') + 6);
        if( c >= '1' && c <= '5' )
            return static_cast<char>((c - '1') + 1);
        return 0;
    }

    /**
    *  Converts a base32 string to a uint64_t. This is a constexpr so that
    *  this method can be used in template arguments as well.
    *
    *  @brief Converts a base32 string to a uint64_t.
    *  @param str - String representation of the protocol name
    *  @return constexpr uint64_t - 64-bit unsigned integer representation of the name
    */
    static /*TODO: cpp14 constexpr*/ uint64_t StringToProtocolName( const char* str )
    {
        uint32_t len = 0;
        while( str[len] ) ++len;

        if (str[0] == '.' || (len > 1 && str[len - 1] == '.'))
            return static_cast<uint64_t >(-1);

        uint64_t value = 0;

        for( uint32_t i = 0; i <= 12; ++i )
        {
            uint64_t c = 0;
            if( i < len && i <= 12 )
            {
                if (!IsValidSymbol(str[i]))
                    return static_cast<uint64_t >(-1);
                c = uint64_t(CharToSymbol(str[i]));
            }

            if( i < 12 )
            {
                c &= 0x1f;
                c <<= 64-5*(i+1);
            }
            else
            {
                c &= 0x0f;
            }

            value |= c;
        }

        return value;
    }

    /**
    *  Wraps a uint64_t to ensure it is only passed to methods that expect a Name and
    *  that no mathematical operations occur.  It also enables specialization of print
    *  so that it is printed as a base32 string.
    *
    *  @brief wraps a uint64_t to ensure it is only passed to methods that expect a Name
    */
    class ProtocolName
    {
    public:
        // TODO: remove constructors with cpp14.
        // In cpp14 this class stays an aggregate even with the default member initialization
        // and brace initialization can be used without constructors implementation
        ProtocolName() = default;
        explicit ProtocolName(const uint64_t & protocolId) : value(protocolId) {}

        /**
         * Conversion Operator to convert name to uint64_t
         *
         * @brief Conversion Operator
         * @return uint64_t - Converted result
         */
        operator uint64_t()const { return value; }

        // keep in sync with name::operator string() in eosio source code definition for name
        std::string ToString() const
        {
            static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

            std::string str(13,'.');

            uint64_t tmp = value;
            for( uint32_t i = 0; i <= 12; ++i )
            {
                char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
                str[12-i] = c;
                tmp >>= (i == 0 ? 4 : 5);
            }

            trim_right_dots( str );
            return str;
        }

        /**
         * Equality Operator for name
         *
         * @brief Equality Operator for name
         * @param a - First data to be compared
         * @param b - Second data to be compared
         * @return true - if equal
         * @return false - if unequal
         */
        friend bool operator==( const ProtocolName& a, const ProtocolName& b ) { return a.value == b.value; }

        /**
         * Internal Representation of the protocol name
         *
         * @brief Internal Representation of the account name
         */
        uint64_t value = 0;

    private:
        static void trim_right_dots(std::string& str )
        {
            const auto last = str.find_last_not_of('.');
            if (last != std::string::npos)
                str = str.substr(0, last + 1);
        }
    };

}

#endif //CROWN_PLATFORM_UTILS_H
