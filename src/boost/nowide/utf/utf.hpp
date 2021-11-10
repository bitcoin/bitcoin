//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_UTF_HPP_INCLUDED
#define BOOST_NOWIDE_UTF_HPP_INCLUDED

#include <boost/nowide/config.hpp>
#include <cstdint>

namespace boost {
namespace nowide {
    ///
    /// \brief Namespace that holds basic operations on UTF encoded sequences
    ///
    /// All functions defined in this namespace do not require linking with Boost.Nowide library.
    /// Extracted from Boost.Locale
    ///
    namespace utf {

        ///
        /// \brief The integral type that can hold a Unicode code point
        ///
        using code_point = uint32_t;

        ///
        /// \brief Special constant that defines illegal code point
        ///
        static const code_point illegal = 0xFFFFFFFFu;

        ///
        /// \brief Special constant that defines incomplete code point
        ///
        static const code_point incomplete = 0xFFFFFFFEu;

        ///
        /// \brief the function checks if \a v is a valid code point
        ///
        inline bool is_valid_codepoint(code_point v)
        {
            if(v > 0x10FFFF)
                return false;
            if(0xD800 <= v && v <= 0xDFFF) // surrogates
                return false;
            return true;
        }

#ifdef BOOST_NOWIDE_DOXYGEN
        ///
        /// \brief UTF Traits class - functions to convert UTF sequences to and from Unicode code points
        ///
        template<typename CharType, int size = sizeof(CharType)>
        struct utf_traits
        {
            ///
            /// The type of the character
            ///
            using char_type = CharType;
            ///
            /// Read one code point from the range [p,e) and return it.
            ///
            /// - If the sequence that was read is incomplete sequence returns \ref incomplete,
            /// - If illegal sequence detected returns \ref illegal
            ///
            /// Requirements
            ///
            /// - Iterator is valid input iterator
            ///
            /// Postconditions
            ///
            /// - p points to the last consumed character
            ///
            template<typename Iterator>
            static code_point decode(Iterator& p, Iterator e);

            ///
            /// Maximal width of valid sequence in the code units:
            ///
            /// - UTF-8  - 4
            /// - UTF-16 - 2
            /// - UTF-32 - 1
            ///
            static const int max_width;
            ///
            /// The width of specific code point in the code units.
            ///
            /// Requirement: value is a valid Unicode code point
            /// Returns value in range [1..max_width]
            ///
            static int width(code_point value);

            ///
            /// Get the size of the trail part of variable length encoded sequence.
            ///
            /// Returns -1 if C is not valid lead character
            ///
            static int trail_length(char_type c);
            ///
            /// Returns true if c is trail code unit, always false for UTF-32
            ///
            static bool is_trail(char_type c);
            ///
            /// Returns true if c is lead code unit, always true of UTF-32
            ///
            static bool is_lead(char_type c);

            ///
            /// Convert valid Unicode code point \a value to the UTF sequence.
            ///
            /// Requirements:
            ///
            /// - \a value is valid code point
            /// - \a out is an output iterator should be able to accept at least width(value) units
            ///
            /// Returns the iterator past the last written code unit.
            ///
            template<typename Iterator>
            static Iterator encode(code_point value, Iterator out);
            ///
            /// Decodes valid UTF sequence that is pointed by p into code point.
            ///
            /// If the sequence is invalid or points to end the behavior is undefined
            ///
            template<typename Iterator>
            static code_point decode_valid(Iterator& p);
        };

#else

        template<typename CharType, int size = sizeof(CharType)>
        struct utf_traits;

        template<typename CharType>
        struct utf_traits<CharType, 1>
        {
            using char_type = CharType;

            static int trail_length(char_type ci)
            {
                unsigned char c = ci;
                if(c < 128)
                    return 0;
                if(BOOST_UNLIKELY(c < 194))
                    return -1;
                if(c < 224)
                    return 1;
                if(c < 240)
                    return 2;
                if(BOOST_LIKELY(c <= 244))
                    return 3;
                return -1;
            }

            static const int max_width = 4;

            static int width(code_point value)
            {
                if(value <= 0x7F)
                {
                    return 1;
                } else if(value <= 0x7FF)
                {
                    return 2;
                } else if(BOOST_LIKELY(value <= 0xFFFF))
                {
                    return 3;
                } else
                {
                    return 4;
                }
            }

            static bool is_trail(char_type ci)
            {
                unsigned char c = ci;
                return (c & 0xC0) == 0x80;
            }

            static bool is_lead(char_type ci)
            {
                return !is_trail(ci);
            }

            template<typename Iterator>
            static code_point decode(Iterator& p, Iterator e)
            {
                if(BOOST_UNLIKELY(p == e))
                    return incomplete;

                unsigned char lead = *p++;

                // First byte is fully validated here
                int trail_size = trail_length(lead);

                if(BOOST_UNLIKELY(trail_size < 0))
                    return illegal;

                // OK as only ASCII may be of size = 0
                // also optimize for ASCII text
                if(trail_size == 0)
                    return lead;

                code_point c = lead & ((1 << (6 - trail_size)) - 1);

                // Read the rest
                unsigned char tmp;
                switch(trail_size)
                {
                case 3:
                    if(BOOST_UNLIKELY(p == e))
                        return incomplete;
                    tmp = *p++;
                    if(!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
                    BOOST_NOWIDE_FALLTHROUGH;
                case 2:
                    if(BOOST_UNLIKELY(p == e))
                        return incomplete;
                    tmp = *p++;
                    if(!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
                    BOOST_NOWIDE_FALLTHROUGH;
                case 1:
                    if(BOOST_UNLIKELY(p == e))
                        return incomplete;
                    tmp = *p++;
                    if(!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
                }

                // Check code point validity:
                // - no surrogates and valid range
                // - most compact representation
                if(BOOST_UNLIKELY(!is_valid_codepoint(c)) || BOOST_UNLIKELY(width(c) != trail_size + 1))
                {
                    p -= trail_size;
                    return illegal;
                }

                return c;
            }

            template<typename Iterator>
            static code_point decode_valid(Iterator& p)
            {
                unsigned char lead = *p++;
                if(lead < 192)
                    return lead;

                int trail_size;

                if(lead < 224)
                    trail_size = 1;
                else if(BOOST_LIKELY(lead < 240)) // non-BMP rare
                    trail_size = 2;
                else
                    trail_size = 3;

                code_point c = lead & ((1 << (6 - trail_size)) - 1);

                switch(trail_size)
                {
                case 3: c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F); BOOST_NOWIDE_FALLTHROUGH;
                case 2: c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F); BOOST_NOWIDE_FALLTHROUGH;
                case 1: c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
                }

                return c;
            }

            template<typename Iterator>
            static Iterator encode(code_point value, Iterator out)
            {
                if(value <= 0x7F)
                {
                    *out++ = static_cast<char_type>(value);
                } else if(value <= 0x7FF)
                {
                    *out++ = static_cast<char_type>((value >> 6) | 0xC0);
                    *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
                } else if(BOOST_LIKELY(value <= 0xFFFF))
                {
                    *out++ = static_cast<char_type>((value >> 12) | 0xE0);
                    *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
                    *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
                } else
                {
                    *out++ = static_cast<char_type>((value >> 18) | 0xF0);
                    *out++ = static_cast<char_type>(((value >> 12) & 0x3F) | 0x80);
                    *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
                    *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
                }
                return out;
            }
        }; // utf8

        template<typename CharType>
        struct utf_traits<CharType, 2>
        {
            using char_type = CharType;

            // See RFC 2781
            static bool is_single_codepoint(uint16_t x)
            {
                // Ranges [U+0000, 0+D7FF], [U+E000, U+FFFF] are numerically equal in UTF-16
                return x <= 0xD7FF || x >= 0xE000;
            }
            static bool is_first_surrogate(uint16_t x)
            {
                // Range [U+D800, 0+DBFF]: High surrogate
                return 0xD800 <= x && x <= 0xDBFF;
            }
            static bool is_second_surrogate(uint16_t x)
            {
                // Range [U+DC00, 0+DFFF]: Low surrogate
                return 0xDC00 <= x && x <= 0xDFFF;
            }
            static code_point combine_surrogate(uint16_t w1, uint16_t w2)
            {
                return ((code_point(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
            }
            static int trail_length(char_type c)
            {
                if(is_first_surrogate(c))
                    return 1;
                if(is_second_surrogate(c))
                    return -1;
                return 0;
            }
            /// Return true if c is trail code unit, always false for UTF-32
            static bool is_trail(char_type c)
            {
                return is_second_surrogate(c);
            }
            /// Return true if c is lead code unit, always true of UTF-32
            static bool is_lead(char_type c)
            {
                return !is_second_surrogate(c);
            }

            template<typename It>
            static code_point decode(It& current, It last)
            {
                if(BOOST_UNLIKELY(current == last))
                    return incomplete;
                uint16_t w1 = *current++;
                if(BOOST_LIKELY(is_single_codepoint(w1)))
                {
                    return w1;
                }
                // Now it's either a high or a low surrogate, the latter is invalid
                if(w1 >= 0xDC00)
                    return illegal;
                if(current == last)
                    return incomplete;
                uint16_t w2 = *current++;
                if(!is_second_surrogate(w2))
                    return illegal;
                return combine_surrogate(w1, w2);
            }
            template<typename It>
            static code_point decode_valid(It& current)
            {
                uint16_t w1 = *current++;
                if(BOOST_LIKELY(is_single_codepoint(w1)))
                {
                    return w1;
                }
                uint16_t w2 = *current++;
                return combine_surrogate(w1, w2);
            }

            static const int max_width = 2;
            static int width(code_point u)
            {
                return u >= 0x10000 ? 2 : 1;
            }
            template<typename It>
            static It encode(code_point u, It out)
            {
                if(BOOST_LIKELY(u <= 0xFFFF))
                {
                    *out++ = static_cast<char_type>(u);
                } else
                {
                    u -= 0x10000;
                    *out++ = static_cast<char_type>(0xD800 | (u >> 10));
                    *out++ = static_cast<char_type>(0xDC00 | (u & 0x3FF));
                }
                return out;
            }
        }; // utf16;

        template<typename CharType>
        struct utf_traits<CharType, 4>
        {
            using char_type = CharType;
            static int trail_length(char_type c)
            {
                if(is_valid_codepoint(c))
                    return 0;
                return -1;
            }
            static bool is_trail(char_type /*c*/)
            {
                return false;
            }
            static bool is_lead(char_type /*c*/)
            {
                return true;
            }

            template<typename It>
            static code_point decode_valid(It& current)
            {
                return *current++;
            }

            template<typename It>
            static code_point decode(It& current, It last)
            {
                if(BOOST_UNLIKELY(current == last))
                    return incomplete;
                code_point c = *current++;
                if(BOOST_UNLIKELY(!is_valid_codepoint(c)))
                    return illegal;
                return c;
            }
            static const int max_width = 1;
            static int width(code_point /*u*/)
            {
                return 1;
            }
            template<typename It>
            static It encode(code_point u, It out)
            {
                *out++ = static_cast<char_type>(u);
                return out;
            }
        }; // utf32

#endif

    } // namespace utf
} // namespace nowide
} // namespace boost

#endif
