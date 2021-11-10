// string_token.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_STRING_TOKEN_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_STRING_TOKEN_HPP

#include <algorithm>
#include "size_t.hpp"
#include "consts.hpp" // num_chars, num_wchar_ts
#include <string>
#include <limits>

namespace boost
{
namespace lexer
{
template<typename CharT>
struct basic_string_token
{
    typedef std::basic_string<CharT> string;

    bool _negated;
    string _charset;

    basic_string_token () :
        _negated (false)
    {
    }

    basic_string_token (const bool negated_, const string &charset_) :
        _negated (negated_),
        _charset (charset_)
    {
    }

    void remove_duplicates ()
    {
        const CharT *start_ = _charset.c_str ();
        const CharT *end_ = start_ + _charset.size ();

        // Optimisation for very large charsets:
        // sorting via pointers is much quicker than
        // via iterators...
        std::sort (const_cast<CharT *> (start_), const_cast<CharT *> (end_));
        _charset.erase (std::unique (_charset.begin (), _charset.end ()),
            _charset.end ());
    }

    void normalise ()
    {
        const std::size_t max_chars_ = sizeof (CharT) == 1 ?
            num_chars : num_wchar_ts;

        if (_charset.length () == max_chars_)
        {
            _negated = !_negated;
            _charset.clear ();
        }
        else if (_charset.length () > max_chars_ / 2)
        {
            negate ();
        }
    }

    void negate ()
    {
        const std::size_t max_chars_ = sizeof (CharT) == 1 ?
            num_chars : num_wchar_ts;
        CharT curr_char_ = (std::numeric_limits<CharT>::min)();
        string temp_;
        const CharT *curr_ = _charset.c_str ();
        const CharT *chars_end_ = curr_ + _charset.size ();

        _negated = !_negated;
        temp_.resize (max_chars_ - _charset.size ());

        CharT *ptr_ = const_cast<CharT *> (temp_.c_str ());
        std::size_t i_ = 0;

        while (curr_ < chars_end_)
        {
            while (*curr_ > curr_char_)
            {
                *ptr_ = curr_char_;
                ++ptr_;
                ++curr_char_;
                ++i_;
            }

            ++curr_char_;
            ++curr_;
            ++i_;
        }

        for (; i_ < max_chars_; ++i_)
        {
            *ptr_ = curr_char_;
            ++ptr_;
            ++curr_char_;
        }

        _charset = temp_;
    }

    bool operator < (const basic_string_token &rhs_) const
    {
        return _negated < rhs_._negated ||
            (_negated == rhs_._negated && _charset < rhs_._charset);
    }

    bool empty () const
    {
        return _charset.empty () && !_negated;
    }

    bool any () const
    {
        return _charset.empty () && _negated;
    }

    void clear ()
    {
        _negated = false;
        _charset.clear ();
    }

    void intersect (basic_string_token &rhs_, basic_string_token &overlap_)
    {
        if ((any () && rhs_.any ()) || (_negated == rhs_._negated &&
            !any () && !rhs_.any ()))
        {
            intersect_same_types (rhs_, overlap_);
        }
        else
        {
            intersect_diff_types (rhs_, overlap_);
        }
    }

    static void escape_char (const CharT ch_, string &out_)
    {
        switch (ch_)
        {
            case '\0':
                out_ += '\\';
                out_ += '0';
                break;
            case '\a':
                out_ += '\\';
                out_ += 'a';
                break;
            case '\b':
                out_ += '\\';
                out_ += 'b';
                break;
            case 27:
                out_ += '\\';
                out_ += 'x';
                out_ += '1';
                out_ += 'b';
                break;
            case '\f':
                out_ += '\\';
                out_ += 'f';
                break;
            case '\n':
                out_ += '\\';
                out_ += 'n';
                break;
            case '\r':
                out_ += '\\';
                out_ += 'r';
                break;
            case '\t':
                out_ += '\\';
                out_ += 't';
                break;
            case '\v':
                out_ += '\\';
                out_ += 'v';
                break;
            case '\\':
                out_ += '\\';
                out_ += '\\';
                break;
            case '"':
                out_ += '\\';
                out_ += '"';
                break;
            case '\'':
                out_ += '\\';
                out_ += '\'';
                break;
            default:
            {
                if (ch_ < 32 && ch_ >= 0)
                {
                    std::basic_stringstream<CharT> ss_;

                    out_ += '\\';
                    out_ += 'x';
                    ss_ << std::hex <<
                        static_cast<std::size_t> (ch_);
                    out_ += ss_.str ();
                }
                else
                {
                    out_ += ch_;
                }

                break;
            }
        }
    }

private:
    void intersect_same_types (basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (any ())
        {
            clear ();
            overlap_._negated = true;
            rhs_.clear ();
        }
        else
        {
            typename string::iterator iter_ = _charset.begin ();
            typename string::iterator end_ = _charset.end ();
            typename string::iterator rhs_iter_ = rhs_._charset.begin ();
            typename string::iterator rhs_end_ = rhs_._charset.end ();

            overlap_._negated = _negated;

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    ++iter_;
                }
                else if (*iter_ > *rhs_iter_)
                {
                    ++rhs_iter_;
                }
                else
                {
                    overlap_._charset += *iter_;
                    iter_ = _charset.erase (iter_);
                    end_ = _charset.end ();
                    rhs_iter_ = rhs_._charset.erase (rhs_iter_);
                    rhs_end_ = rhs_._charset.end ();
                }
            }

            if (_negated)
            {
                // duplicates already merged, so safe to merge
                // using std lib.

                // src, dest
                merge (_charset, overlap_._charset);
                // duplicates already merged, so safe to merge
                // using std lib.

                // src, dest
                merge (rhs_._charset, overlap_._charset);
                _negated = false;
                rhs_._negated = false;
                std::swap (_charset, rhs_._charset);
                normalise ();
                overlap_.normalise ();
                rhs_.normalise ();
            }
            else if (!overlap_._charset.empty ())
            {
                normalise ();
                overlap_.normalise ();
                rhs_.normalise ();
            }
        }
    }

    void intersect_diff_types (basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (any ())
        {
            intersect_any (rhs_, overlap_);
        }
        else if (_negated)
        {
            intersect_negated (rhs_, overlap_);
        }
        else // _negated == false
        {
            intersect_charset (rhs_, overlap_);
        }
    }

    void intersect_any (basic_string_token &rhs_, basic_string_token &overlap_)
    {
        if (rhs_._negated)
        {
            rhs_.intersect_negated (*this, overlap_);
        }
        else // rhs._negated == false
        {
            rhs_.intersect_charset (*this, overlap_);
        }
    }

    void intersect_negated (basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (rhs_.any ())
        {
            overlap_._negated = true;
            overlap_._charset = _charset;
            rhs_._negated = false;
            rhs_._charset = _charset;
            clear ();
        }
        else // rhs._negated == false
        {
            rhs_.intersect_charset (*this, overlap_);
        }
    }

    void intersect_charset (basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (rhs_.any ())
        {
            overlap_._charset = _charset;
            rhs_._negated = true;
            rhs_._charset = _charset;
            clear ();
        }
        else // rhs_._negated == true
        {
            typename string::iterator iter_ = _charset.begin ();
            typename string::iterator end_ = _charset.end ();
            typename string::iterator rhs_iter_ = rhs_._charset.begin ();
            typename string::iterator rhs_end_ = rhs_._charset.end ();

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    overlap_._charset += *iter_;
                    rhs_iter_ = rhs_._charset.insert (rhs_iter_, *iter_);
                    ++rhs_iter_;
                    rhs_end_ = rhs_._charset.end ();
                    iter_ = _charset.erase (iter_);
                    end_ = _charset.end ();
                }
                else if (*iter_ > *rhs_iter_)
                {
                    ++rhs_iter_;
                }
                else
                {
                    ++iter_;
                    ++rhs_iter_;
                }
            }

            if (iter_ != end_)
            {
                // nothing bigger in rhs_ than iter_,
                // so safe to merge using std lib.
                string temp_ (iter_, end_);

                // src, dest
                merge (temp_, overlap_._charset);
                _charset.erase (iter_, end_);
            }

            if (!overlap_._charset.empty ())
            {
                merge (overlap_._charset, rhs_._charset);
                // possible duplicates, so check for any and erase.
                rhs_._charset.erase (std::unique (rhs_._charset.begin (),
                    rhs_._charset.end ()), rhs_._charset.end ());
                normalise ();
                overlap_.normalise ();
                rhs_.normalise ();
            }
        }
    }

    void merge (string &src_, string &dest_)
    {
        string tmp_ (src_.size () + dest_.size (), 0);

        std::merge (src_.begin (), src_.end (), dest_.begin (), dest_.end (),
            tmp_.begin ());
        dest_ = tmp_;
    }
};
}
}

#endif
