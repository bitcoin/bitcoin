///////////////////////////////////////////////////////////////////////////////
/// \file null_regex_traits.hpp
/// Contains the definition of the null_regex_traits\<\> template, which is a
/// stub regex traits implementation that can be used by static and dynamic
/// regexes for searching non-character data.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_TRAITS_NULL_REGEX_TRAITS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_TRAITS_NULL_REGEX_TRAITS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/utility/never_true.hpp>
#include <boost/xpressive/detail/utility/ignore_unused.hpp>

namespace boost { namespace xpressive
{

namespace detail
{
    struct not_a_locale {};
}

struct regex_traits_version_1_tag;

///////////////////////////////////////////////////////////////////////////////
// null_regex_traits
//
/// \brief stub regex_traits for non-char data
///
template<typename Elem>
struct null_regex_traits
{
    typedef Elem char_type;
    typedef std::vector<char_type> string_type;
    typedef detail::not_a_locale locale_type;
    typedef int char_class_type;
    typedef regex_traits_version_1_tag version_tag;

    /// Initialize a null_regex_traits object.
    ///
    null_regex_traits(locale_type = locale_type())
    {
    }

    /// Checks two null_regex_traits objects for equality
    ///
    /// \return true.
    bool operator ==(null_regex_traits<char_type> const &that) const
    {
        detail::ignore_unused(that);
        return true;
    }

    /// Checks two null_regex_traits objects for inequality
    ///
    /// \return false.
    bool operator !=(null_regex_traits<char_type> const &that) const
    {
        detail::ignore_unused(that);
        return false;
    }

    /// Convert a char to a Elem
    ///
    /// \param ch The source character.
    /// \return Elem(ch).
    char_type widen(char ch) const
    {
        return char_type(ch);
    }

    /// Returns a hash value for a Elem in the range [0, UCHAR_MAX]
    ///
    /// \param ch The source character.
    /// \return a value between 0 and UCHAR_MAX, inclusive.
    static unsigned char hash(char_type ch)
    {
        return static_cast<unsigned char>(ch);
    }

    /// No-op
    ///
    /// \param ch The source character.
    /// \return ch
    static char_type translate(char_type ch)
    {
        return ch;
    }

    /// No-op
    ///
    /// \param ch The source character.
    /// \return ch
    static char_type translate_nocase(char_type ch)
    {
        return ch;
    }

    /// Checks to see if a character is within a character range.
    ///
    /// \param first The bottom of the range, inclusive.
    /// \param last The top of the range, inclusive.
    /// \param ch The source character.
    /// \return first <= ch && ch <= last.
    static bool in_range(char_type first, char_type last, char_type ch)
    {
        return first <= ch && ch <= last;
    }

    /// Checks to see if a character is within a character range.
    ///
    /// \param first The bottom of the range, inclusive.
    /// \param last The top of the range, inclusive.
    /// \param ch The source character.
    /// \return first <= ch && ch <= last.
    /// \attention Since the null_regex_traits does not do case-folding,
    /// this function is equivalent to in_range().
    static bool in_range_nocase(char_type first, char_type last, char_type ch)
    {
        return first <= ch && ch <= last;
    }

    /// Returns a sort key for the character sequence designated by the iterator range [F1, F2)
    /// such that if the character sequence [G1, G2) sorts before the character sequence [H1, H2)
    /// then v.transform(G1, G2) < v.transform(H1, H2).
    ///
    /// \attention Not currently used
    template<typename FwdIter>
    static string_type transform(FwdIter begin, FwdIter end)
    {
        return string_type(begin, end);
    }

    /// Returns a sort key for the character sequence designated by the iterator range [F1, F2)
    /// such that if the character sequence [G1, G2) sorts before the character sequence [H1, H2)
    /// when character case is not considered then
    /// v.transform_primary(G1, G2) < v.transform_primary(H1, H2).
    ///
    /// \attention Not currently used
    template<typename FwdIter>
    static string_type transform_primary(FwdIter begin, FwdIter end)
    {
        return string_type(begin, end);
    }

    /// Returns a sequence of characters that represents the collating element
    /// consisting of the character sequence designated by the iterator range [F1, F2).
    /// Returns an empty string if the character sequence is not a valid collating element.
    ///
    /// \attention Not currently used
    template<typename FwdIter>
    static string_type lookup_collatename(FwdIter begin, FwdIter end)
    {
        detail::ignore_unused(begin);
        detail::ignore_unused(end);
        return string_type();
    }

    /// The null_regex_traits does not have character classifications, so lookup_classname()
    /// is unused.
    ///
    /// \param begin not used
    /// \param end not used
    /// \param icase not used
    /// \return static_cast\<char_class_type\>(0)
    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        detail::ignore_unused(begin);
        detail::ignore_unused(end);
        detail::ignore_unused(icase);
        return 0;
    }

    /// The null_regex_traits does not have character classifications, so isctype()
    /// is unused.
    ///
    /// \param ch not used
    /// \param mask not used
    /// \return false
    static bool isctype(char_type ch, char_class_type mask)
    {
        detail::ignore_unused(ch);
        detail::ignore_unused(mask);
        return false;
    }

    /// The null_regex_traits recognizes no elements as digits, so value() is unused.
    ///
    /// \param ch not used
    /// \param radix not used
    /// \return -1
    static int value(char_type ch, int radix)
    {
        detail::ignore_unused(ch);
        detail::ignore_unused(radix);
        return -1;
    }

    /// Not used
    ///
    /// \param loc not used
    /// \return loc
    static locale_type imbue(locale_type loc)
    {
        return loc;
    }

    /// Returns locale_type().
    ///
    /// \return locale_type()
    static locale_type getloc()
    {
        return locale_type();
    }
};

}}

#endif
