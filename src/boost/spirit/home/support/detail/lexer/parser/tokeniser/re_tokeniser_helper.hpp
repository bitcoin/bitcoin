// tokeniser_helper.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_RE_TOKENISER_HELPER_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_RE_TOKENISER_HELPER_HPP

#include "../../char_traits.hpp"
// strlen()
#include <cstring>
#include "../../size_t.hpp"
#include "re_tokeniser_state.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT, typename Traits = char_traits<CharT> >
class basic_re_tokeniser_helper
{
public:
    typedef basic_re_tokeniser_state<CharT> state;
    typedef std::basic_string<CharT> string;

    static const CharT *escape_sequence (state &state_, CharT &ch_,
        std::size_t &str_len_)
    {
        bool eos_ = state_.eos ();

        if (eos_)
        {
            throw runtime_error ("Unexpected end of regex "
                "following '\\'.");
        }

        const CharT *str_ = charset_shortcut (*state_._curr, str_len_);

        if (str_)
        {
            state_.increment ();
        }
        else
        {
            ch_ = chr (state_);
        }

        return str_;
    }

    // This function can call itself.
    static void charset (state &state_, string &chars_, bool &negated_)
    {
        CharT ch_ = 0;
        bool eos_ = state_.next (ch_);

        if (eos_)
        {
            // Pointless returning index if at end of string
            throw runtime_error ("Unexpected end of regex "
                "following '['.");
        }

        negated_ = ch_ == '^';

        if (negated_)
        {
            eos_ = state_.next (ch_);

            if (eos_)
            {
                // Pointless returning index if at end of string
                throw runtime_error ("Unexpected end of regex "
                    "following '^'.");
            }
        }

        bool chset_ = false;
        CharT prev_ = 0;

        while (ch_ != ']')
        {
            if (ch_ == '\\')
            {
                std::size_t str_len_ = 0;
                const CharT *str_ = escape_sequence (state_, prev_, str_len_);

                chset_ = str_ != 0;

                if (chset_)
                {
                    state temp_state_ (str_ + 1, str_ + str_len_,
                        state_._flags, state_._locale);
                    string temp_chars_;
                    bool temp_negated_ = false;

                    charset (temp_state_, temp_chars_, temp_negated_);

                    if (negated_ != temp_negated_)
                    {
                        std::ostringstream ss_;

                        ss_ << "Mismatch in charset negation preceding "
                            "index " << state_.index () << '.';
                        throw runtime_error (ss_.str ().c_str ());
                    }

                    chars_ += temp_chars_;
                }
            }
/*
            else if (ch_ == '[' && !state_.eos () && *state_._curr == ':')
            {
                // TODO: POSIX charsets
            }
*/
            else
            {
                chset_ = false;
                prev_ = ch_;
            }

            eos_ = state_.next (ch_);

            // Covers preceding if, else if and else
            if (eos_)
            {
                // Pointless returning index if at end of string
                throw runtime_error ("Unexpected end of regex "
                    "(missing ']').");
            }

            if (ch_ == '-')
            {
                charset_range (chset_, state_, eos_, ch_, prev_, chars_);
            }
            else if (!chset_)
            {
                if ((state_._flags & icase) &&
                    (std::isupper (prev_, state_._locale) ||
                    std::islower (prev_, state_._locale)))
                {
                    CharT upper_ = std::toupper (prev_, state_._locale);
                    CharT lower_ = std::tolower (prev_, state_._locale);

                    chars_ += upper_;
                    chars_ += lower_;
                }
                else
                {
                    chars_ += prev_;
                }
            }
        }

        if (!negated_ && chars_.empty ())
        {
            throw runtime_error ("Empty charsets not allowed.");
        }
    }

    static CharT chr (state &state_)
    {
        CharT ch_ = 0;

        // eos_ has already been checked for.
        switch (*state_._curr)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                ch_ = decode_octal (state_);
                break;
            case 'a':
                ch_ = '\a';
                state_.increment ();
                break;
            case 'b':
                ch_ = '\b';
                state_.increment ();
                break;
            case 'c':
                ch_ = decode_control_char (state_);
                break;
            case 'e':
                ch_ = 27; // '\e' not recognised by compiler
                state_.increment ();
                break;
            case 'f':
                ch_ = '\f';
                state_.increment ();
                break;
            case 'n':
                ch_ = '\n';
                state_.increment ();
                break;
            case 'r':
                ch_ = '\r';
                state_.increment ();
                break;
            case 't':
                ch_ = '\t';
                state_.increment ();
                break;
            case 'v':
                ch_ = '\v';
                state_.increment ();
                break;
            case 'x':
                ch_ = decode_hex (state_);
                break;
            default:
                ch_ = *state_._curr;
                state_.increment ();
                break;
        }

        return ch_;
    }

private:
    static const char *charset_shortcut (const char ch_,
        std::size_t &str_len_)
    {
        const char *str_ = 0;

        switch (ch_)
        {
        case 'd':
            str_ = "[0-9]";
            break;
        case 'D':
            str_ = "[^0-9]";
            break;
        case 's':
            str_ = "[ \t\n\r\f\v]";
            break;
        case 'S':
            str_ = "[^ \t\n\r\f\v]";
            break;
        case 'w':
            str_ = "[_0-9A-Za-z]";
            break;
        case 'W':
            str_ = "[^_0-9A-Za-z]";
            break;
        }

        if (str_)
        {
            // Some systems have strlen in namespace std.
            using namespace std;

            str_len_ = strlen (str_);
        }
        else
        {
            str_len_ = 0;
        }

        return str_;
    }

    static const wchar_t *charset_shortcut (const wchar_t ch_,
        std::size_t &str_len_)
    {
        const wchar_t *str_ = 0;

        switch (ch_)
        {
        case 'd':
            str_ = L"[0-9]";
            break;
        case 'D':
            str_ = L"[^0-9]";
            break;
        case 's':
            str_ = L"[ \t\n\r\f\v]";
            break;
        case 'S':
            str_ = L"[^ \t\n\r\f\v]";
            break;
        case 'w':
            str_ = L"[_0-9A-Za-z]";
            break;
        case 'W':
            str_ = L"[^_0-9A-Za-z]";
            break;
        }

        if (str_)
        {
            // Some systems have wcslen in namespace std.
            using namespace std;

            str_len_ = wcslen (str_);
        }
        else
        {
            str_len_ = 0;
        }

        return str_;
    }

    static CharT decode_octal (state &state_)
    {
        std::size_t accumulator_ = 0;
        CharT ch_ = *state_._curr;
        unsigned short count_ = 3;
        bool eos_ = false;

        for (;;)
        {
            accumulator_ *= 8;
            accumulator_ += ch_ - '0';
            --count_;
            state_.increment ();
            eos_ = state_.eos ();

            if (!count_ || eos_) break;

            ch_ = *state_._curr;

            // Don't consume invalid chars!
            if (ch_ < '0' || ch_ > '7')
            {
                break;
            }
        }

        return static_cast<CharT> (accumulator_);
    }

    static CharT decode_control_char (state &state_)
    {
        // Skip over 'c'
        state_.increment ();

        CharT ch_ = 0;
        bool eos_ = state_.next (ch_);

        if (eos_)
        {
            // Pointless returning index if at end of string
            throw runtime_error ("Unexpected end of regex following \\c.");
        }
        else
        {
            if (ch_ >= 'a' && ch_ <= 'z')
            {
                ch_ -= 'a' - 1;
            }
            else if (ch_ >= 'A' && ch_ <= 'Z')
            {
                ch_ -= 'A' - 1;
            }
            else if (ch_ == '@')
            {
                // Apparently...
                ch_ = 0;
            }
            else
            {
                std::ostringstream ss_;

                ss_ << "Invalid control char at index " <<
                    state_.index () - 1 << '.';
                throw runtime_error (ss_.str ().c_str ());
            }
        }

        return ch_;
    }

    static CharT decode_hex (state &state_)
    {
        // Skip over 'x'
        state_.increment ();

        CharT ch_ = 0;
        bool eos_ = state_.next (ch_);

        if (eos_)
        {
            // Pointless returning index if at end of string
            throw runtime_error ("Unexpected end of regex following \\x.");
        }

        if (!((ch_ >= '0' && ch_ <= '9') || (ch_ >= 'a' && ch_ <= 'f') ||
            (ch_ >= 'A' && ch_ <= 'F')))
        {
            std::ostringstream ss_;

            ss_ << "Illegal char following \\x at index " <<
                state_.index () - 1 << '.';
            throw runtime_error (ss_.str ().c_str ());
        }

        std::size_t hex_ = 0;

        do
        {
            hex_ *= 16;

            if (ch_ >= '0' && ch_ <= '9')
            {
                hex_ += ch_ - '0';
            }
            else if (ch_ >= 'a' && ch_ <= 'f')
            {
                hex_ += 10 + (ch_ - 'a');
            }
            else
            {
                hex_ += 10 + (ch_ - 'A');
            }

            eos_ = state_.eos ();

            if (!eos_)
            {
                ch_ = *state_._curr;

                // Don't consume invalid chars!
                if (((ch_ >= '0' && ch_ <= '9') ||
                    (ch_ >= 'a' && ch_ <= 'f') || (ch_ >= 'A' && ch_ <= 'F')))
                {
                    state_.increment ();
                }
                else
                {
                    eos_ = true;
                }
            }
        } while (!eos_);

        return static_cast<CharT> (hex_);
    }

    static void charset_range (const bool chset_, state &state_, bool &eos_,
        CharT &ch_, const CharT prev_, string &chars_)
    {
        if (chset_)
        {
            std::ostringstream ss_;

            ss_ << "Charset cannot form start of range preceding "
                "index " << state_.index () - 1 << '.';
            throw runtime_error (ss_.str ().c_str ());
        }

        eos_ = state_.next (ch_);

        if (eos_)
        {
            // Pointless returning index if at end of string
            throw runtime_error ("Unexpected end of regex "
                "following '-'.");
        }

        CharT curr_ = 0;

        if (ch_ == '\\')
        {
            std::size_t str_len_ = 0;

            if (escape_sequence (state_, curr_, str_len_))
            {
                std::ostringstream ss_;

                ss_ << "Charset cannot form end of range preceding index "
                    << state_.index () << '.';
                throw runtime_error (ss_.str ().c_str ());
            }
        }
/*
        else if (ch_ == '[' && !state_.eos () && *state_._curr == ':')
        {
            std::ostringstream ss_;

            ss_ << "POSIX char class cannot form end of range at "
                "index " << state_.index () - 1 << '.';
            throw runtime_error (ss_.str ().c_str ());
        }
*/
        else
        {
            curr_ = ch_;
        }

        eos_ = state_.next (ch_);

        // Covers preceding if and else
        if (eos_)
        {
            // Pointless returning index if at end of string
            throw runtime_error ("Unexpected end of regex "
                "(missing ']').");
        }

        std::size_t start_ = static_cast<typename Traits::index_type> (prev_);
        std::size_t end_ = static_cast<typename Traits::index_type> (curr_);

        // Semanic check
        if (end_ < start_)
        {
            std::ostringstream ss_;

            ss_ << "Invalid range in charset preceding index " <<
                state_.index () - 1 << '.';
            throw runtime_error (ss_.str ().c_str ());
        }

        chars_.reserve (chars_.size () + (end_ + 1 - start_));

        for (; start_ <= end_; ++start_)
        {
            CharT ch_i = static_cast<CharT> (start_);

            if ((state_._flags & icase) &&
                (std::isupper (ch_i, state_._locale) ||
                std::islower (ch_i, state_._locale)))
            {
                CharT upper_ = std::toupper (ch_i, state_._locale);
                CharT lower_ = std::tolower (ch_i, state_._locale);

                chars_ += (upper_);
                chars_ += (lower_);
            }
            else
            {
                chars_ += (ch_i);
            }
        }
    }
};
}
}
}

#endif
