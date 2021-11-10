//
//  Copyright (c) 2015 Artyom Beilis (Tonkikh)
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_UTF8_CODECVT_HPP_INCLUDED
#define BOOST_NOWIDE_UTF8_CODECVT_HPP_INCLUDED

#include <boost/nowide/replacement.hpp>
#include <boost/nowide/utf/utf.hpp>
#include <cstdint>
#include <locale>

namespace boost {
namespace nowide {

    static_assert(sizeof(std::mbstate_t) >= 2, "mbstate_t is to small to store an UTF-16 codepoint");
    namespace detail {
        // Avoid including cstring for std::memcpy
        inline void copy_uint16_t(void* dst, const void* src)
        {
            unsigned char* cdst = static_cast<unsigned char*>(dst);
            const unsigned char* csrc = static_cast<const unsigned char*>(src);
            cdst[0] = csrc[0];
            cdst[1] = csrc[1];
        }
        inline std::uint16_t read_state(const std::mbstate_t& src)
        {
            std::uint16_t dst;
            copy_uint16_t(&dst, &src);
            return dst;
        }
        inline void write_state(std::mbstate_t& dst, const std::uint16_t src)
        {
            copy_uint16_t(&dst, &src);
        }
    } // namespace detail

    /// std::codecvt implementation that converts between UTF-8 and UTF-16 or UTF-32
    ///
    /// @tparam CharSize Determines the encoding: 2 for UTF-16, 4 for UTF-32
    ///
    /// Invalid sequences are replaced by #BOOST_NOWIDE_REPLACEMENT_CHARACTER
    /// A trailing incomplete sequence will result in a return value of std::codecvt::partial
    template<typename CharType, int CharSize = sizeof(CharType)>
    class utf8_codecvt;

    /// Specialization for the UTF-8 <-> UTF-16 variant of the std::codecvt implementation
    template<typename CharType>
    class BOOST_SYMBOL_VISIBLE utf8_codecvt<CharType, 2> : public std::codecvt<CharType, char, std::mbstate_t>
    {
    public:
        static_assert(sizeof(CharType) >= 2, "CharType must be able to store UTF16 code point");

        utf8_codecvt(size_t refs = 0) : std::codecvt<CharType, char, std::mbstate_t>(refs)
        {}

    protected:
        using uchar = CharType;

        std::codecvt_base::result do_unshift(std::mbstate_t& s, char* from, char* /*to*/, char*& next) const override
        {
            if(detail::read_state(s) != 0)
                return std::codecvt_base::error;
            next = from;
            return std::codecvt_base::ok;
        }
        int do_encoding() const noexcept override
        {
            return 0;
        }
        int do_max_length() const noexcept override
        {
            return 4;
        }
        bool do_always_noconv() const noexcept override
        {
            return false;
        }

        int do_length(std::mbstate_t& std_state, const char* from, const char* from_end, size_t max) const override
        {
            using utf16_traits = utf::utf_traits<uchar, 2>;
            std::uint16_t state = detail::read_state(std_state);
            const char* save_from = from;
            if(state && max > 0)
            {
                max--;
                state = 0;
            }
            while(max > 0 && from < from_end)
            {
                const char* prev_from = from;
                std::uint32_t ch = utf::utf_traits<char>::decode(from, from_end);
                if(ch == utf::illegal)
                {
                    ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                } else if(ch == utf::incomplete)
                {
                    from = prev_from;
                    break;
                }
                // If we can't write the char, we have to save the low surrogate in state
                if(BOOST_LIKELY(static_cast<size_t>(utf16_traits::width(ch)) <= max))
                {
                    max -= utf16_traits::width(ch);
                } else
                {
                    static_assert(utf16_traits::max_width == 2, "Required for below");
                    std::uint16_t tmpOut[2]{};
                    utf16_traits::encode(ch, tmpOut);
                    state = tmpOut[1];
                    break;
                }
            }
            detail::write_state(std_state, state);
            return static_cast<int>(from - save_from);
        }

        std::codecvt_base::result do_in(std::mbstate_t& std_state,
                                        const char* from,
                                        const char* from_end,
                                        const char*& from_next,
                                        uchar* to,
                                        uchar* to_end,
                                        uchar*& to_next) const override
        {
            std::codecvt_base::result r = std::codecvt_base::ok;
            using utf16_traits = utf::utf_traits<uchar, 2>;

            // mbstate_t is POD type and should be initialized to 0 (i.e. state = stateT())
            // according to standard.
            // We use it to store a low surrogate if it was not yet written, else state is 0
            std::uint16_t state = detail::read_state(std_state);
            // Write low surrogate if present
            if(state && to < to_end)
            {
                *to++ = static_cast<CharType>(state);
                state = 0;
            }
            while(to < to_end && from < from_end)
            {
                const char* from_saved = from;

                uint32_t ch = utf::utf_traits<char>::decode(from, from_end);

                if(ch == utf::illegal)
                {
                    ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                } else if(ch == utf::incomplete)
                {
                    from = from_saved;
                    r = std::codecvt_base::partial;
                    break;
                }
                // If the encoded char fits, write directly, else safe the low surrogate in state
                if(BOOST_LIKELY(utf16_traits::width(ch) <= to_end - to))
                {
                    to = utf16_traits::encode(ch, to);
                } else
                {
                    static_assert(utf16_traits::max_width == 2, "Required for below");
                    std::uint16_t tmpOut[2]{};
                    utf16_traits::encode(ch, tmpOut);
                    *to++ = static_cast<CharType>(tmpOut[0]);
                    state = tmpOut[1];
                    break;
                }
            }
            from_next = from;
            to_next = to;
            if(r == std::codecvt_base::ok && (from != from_end || state != 0))
                r = std::codecvt_base::partial;
            detail::write_state(std_state, state);
            return r;
        }

        std::codecvt_base::result do_out(std::mbstate_t& std_state,
                                         const uchar* from,
                                         const uchar* from_end,
                                         const uchar*& from_next,
                                         char* to,
                                         char* to_end,
                                         char*& to_next) const override
        {
            std::codecvt_base::result r = std::codecvt_base::ok;
            using utf16_traits = utf::utf_traits<uchar, 2>;
            // mbstate_t is POD type and should be initialized to 0
            // (i.e. state = stateT()) according to standard.
            // We use it to store the first observed surrogate pair, or 0 if there is none yet
            std::uint16_t state = detail::read_state(std_state);
            for(; to < to_end && from < from_end; ++from)
            {
                std::uint32_t ch = 0;
                if(state != 0)
                {
                    // We have a high surrogate, so now there should be a low surrogate
                    std::uint16_t w1 = state;
                    std::uint16_t w2 = *from;
                    if(BOOST_LIKELY(utf16_traits::is_trail(w2)))
                    {
                        ch = utf16_traits::combine_surrogate(w1, w2);
                    } else
                    {
                        ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                    }
                } else
                {
                    std::uint16_t w1 = *from;
                    if(BOOST_LIKELY(utf16_traits::is_single_codepoint(w1)))
                    {
                        ch = w1;
                    } else if(BOOST_LIKELY(utf16_traits::is_first_surrogate(w1)))
                    {
                        // Store into state and continue at next character
                        state = w1;
                        continue;
                    } else
                    {
                        // Neither a single codepoint nor a high surrogate so must be low surrogate.
                        // This is an error -> Replace character
                        ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                    }
                }
                if(!utf::is_valid_codepoint(ch))
                {
                    r = std::codecvt_base::error;
                    break;
                }
                int len = utf::utf_traits<char>::width(ch);
                if(to_end - to < len)
                {
                    r = std::codecvt_base::partial;
                    break;
                }
                to = utf::utf_traits<char>::encode(ch, to);
                state = 0;
            }
            from_next = from;
            to_next = to;
            if(r == std::codecvt_base::ok && (from != from_end || state != 0))
                r = std::codecvt_base::partial;
            detail::write_state(std_state, state);
            return r;
        }
    };

    /// Specialization for the UTF-8 <-> UTF-32 variant of the std::codecvt implementation
    template<typename CharType>
    class BOOST_SYMBOL_VISIBLE utf8_codecvt<CharType, 4> : public std::codecvt<CharType, char, std::mbstate_t>
    {
    public:
        utf8_codecvt(size_t refs = 0) : std::codecvt<CharType, char, std::mbstate_t>(refs)
        {}

    protected:
        using uchar = CharType;

        std::codecvt_base::result
        do_unshift(std::mbstate_t& /*s*/, char* from, char* /*to*/, char*& next) const override
        {
            next = from;
            return std::codecvt_base::ok;
        }
        int do_encoding() const noexcept override
        {
            return 0;
        }
        int do_max_length() const noexcept override
        {
            return 4;
        }
        bool do_always_noconv() const noexcept override
        {
            return false;
        }

        int do_length(std::mbstate_t& /*state*/, const char* from, const char* from_end, size_t max) const override
        {
            const char* start_from = from;

            while(max > 0 && from < from_end)
            {
                const char* save_from = from;
                std::uint32_t ch = utf::utf_traits<char>::decode(from, from_end);
                if(ch == utf::incomplete)
                {
                    from = save_from;
                    break;
                } else if(ch == utf::illegal)
                {
                    ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                }
                max--;
            }
            return from - start_from;
        }

        std::codecvt_base::result do_in(std::mbstate_t& /*state*/,
                                        const char* from,
                                        const char* from_end,
                                        const char*& from_next,
                                        uchar* to,
                                        uchar* to_end,
                                        uchar*& to_next) const override
        {
            std::codecvt_base::result r = std::codecvt_base::ok;

            while(to < to_end && from < from_end)
            {
                const char* from_saved = from;

                uint32_t ch = utf::utf_traits<char>::decode(from, from_end);

                if(ch == utf::illegal)
                {
                    ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                } else if(ch == utf::incomplete)
                {
                    r = std::codecvt_base::partial;
                    from = from_saved;
                    break;
                }
                *to++ = ch;
            }
            from_next = from;
            to_next = to;
            if(r == std::codecvt_base::ok && from != from_end)
                r = std::codecvt_base::partial;
            return r;
        }

        std::codecvt_base::result do_out(std::mbstate_t& /*std_state*/,
                                         const uchar* from,
                                         const uchar* from_end,
                                         const uchar*& from_next,
                                         char* to,
                                         char* to_end,
                                         char*& to_next) const override
        {
            std::codecvt_base::result r = std::codecvt_base::ok;
            while(to < to_end && from < from_end)
            {
                std::uint32_t ch = 0;
                ch = *from;
                if(!utf::is_valid_codepoint(ch))
                {
                    ch = BOOST_NOWIDE_REPLACEMENT_CHARACTER;
                }
                int len = utf::utf_traits<char>::width(ch);
                if(to_end - to < len)
                {
                    r = std::codecvt_base::partial;
                    break;
                }
                to = utf::utf_traits<char>::encode(ch, to);
                from++;
            }
            from_next = from;
            to_next = to;
            if(r == std::codecvt_base::ok && from != from_end)
                r = std::codecvt_base::partial;
            return r;
        }
    };

} // namespace nowide
} // namespace boost

#endif
