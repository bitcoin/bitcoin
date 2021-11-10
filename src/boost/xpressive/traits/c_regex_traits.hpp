//////////////////////////////////////////////////////////////////////////////
/// \file c_regex_traits.hpp
/// Contains the definition of the c_regex_traits\<\> template, which is a
/// wrapper for the C locale functions that can be used to customize the
/// behavior of static and dynamic regexes.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_TRAITS_C_REGEX_TRAITS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_TRAITS_C_REGEX_TRAITS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <cstdlib>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/xpressive/traits/detail/c_ctype.hpp>

namespace boost { namespace xpressive
{

namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // empty_locale
    struct empty_locale
    {
    };

    ///////////////////////////////////////////////////////////////////////////////
    // c_regex_traits_base
    template<typename Char, std::size_t SizeOfChar = sizeof(Char)>
    struct c_regex_traits_base
    {
    protected:
        template<typename Traits>
        void imbue(Traits const &tr)
        {
        }
    };

    template<typename Char>
    struct c_regex_traits_base<Char, 1>
    {
    protected:
        template<typename Traits>
        static void imbue(Traits const &)
        {
        }
    };

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    template<std::size_t SizeOfChar>
    struct c_regex_traits_base<wchar_t, SizeOfChar>
    {
    protected:
        template<typename Traits>
        static void imbue(Traits const &)
        {
        }
    };
    #endif

    template<typename Char>
    Char c_tolower(Char);

    template<typename Char>
    Char c_toupper(Char);

    template<>
    inline char c_tolower(char ch)
    {
        using namespace std;
        return static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    template<>
    inline char c_toupper(char ch)
    {
        using namespace std;
        return static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    }

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    template<>
    inline wchar_t c_tolower(wchar_t ch)
    {
        using namespace std;
        return towlower(ch);
    }

    template<>
    inline wchar_t c_toupper(wchar_t ch)
    {
        using namespace std;
        return towupper(ch);
    }
    #endif

} // namespace detail

///////////////////////////////////////////////////////////////////////////////
// regex_traits_version_1_tag
//
struct regex_traits_version_1_tag;

///////////////////////////////////////////////////////////////////////////////
// c_regex_traits
//
/// \brief Encapsaulates the standard C locale functions for use by the
/// \c basic_regex\<\> class template.
template<typename Char>
struct c_regex_traits
  : detail::c_regex_traits_base<Char>
{
    typedef Char char_type;
    typedef std::basic_string<char_type> string_type;
    typedef detail::empty_locale locale_type;
    typedef typename detail::char_class_impl<Char>::char_class_type char_class_type;
    typedef regex_traits_version_2_tag version_tag;
    typedef detail::c_regex_traits_base<Char> base_type;

    /// Initialize a c_regex_traits object to use the global C locale.
    ///
    c_regex_traits(locale_type const &loc = locale_type())
      : base_type()
    {
        this->imbue(loc);
    }

    /// Checks two c_regex_traits objects for equality
    ///
    /// \return true.
    bool operator ==(c_regex_traits<char_type> const &) const
    {
        return true;
    }

    /// Checks two c_regex_traits objects for inequality
    ///
    /// \return false.
    bool operator !=(c_regex_traits<char_type> const &) const
    {
        return false;
    }

    /// Convert a char to a Char
    ///
    /// \param ch The source character.
    /// \return ch if Char is char, std::btowc(ch) if Char is wchar_t.
    static char_type widen(char ch);

    /// Returns a hash value for a Char in the range [0, UCHAR_MAX]
    ///
    /// \param ch The source character.
    /// \return a value between 0 and UCHAR_MAX, inclusive.
    static unsigned char hash(char_type ch)
    {
        return static_cast<unsigned char>(std::char_traits<Char>::to_int_type(ch));
    }

    /// No-op
    ///
    /// \param ch The source character.
    /// \return ch
    static char_type translate(char_type ch)
    {
        return ch;
    }

    /// Converts a character to lower-case using the current global C locale.
    ///
    /// \param ch The source character.
    /// \return std::tolower(ch) if Char is char, std::towlower(ch) if Char is wchar_t.
    static char_type translate_nocase(char_type ch)
    {
        return detail::c_tolower(ch);
    }

    /// Converts a character to lower-case using the current global C locale.
    ///
    /// \param ch The source character.
    /// \return std::tolower(ch) if Char is char, std::towlower(ch) if Char is wchar_t.
    static char_type tolower(char_type ch)
    {
        return detail::c_tolower(ch);
    }

    /// Converts a character to upper-case using the current global C locale.
    ///
    /// \param ch The source character.
    /// \return std::toupper(ch) if Char is char, std::towupper(ch) if Char is wchar_t.
    static char_type toupper(char_type ch)
    {
        return detail::c_toupper(ch);
    }

    /// Returns a \c string_type containing all the characters that compare equal
    /// disregrarding case to the one passed in. This function can only be called
    /// if <tt>has_fold_case\<c_regex_traits\<Char\> \>::value</tt> is \c true.
    ///
    /// \param ch The source character.
    /// \return \c string_type containing all chars which are equal to \c ch when disregarding
    ///     case
    //typedef array<char_type, 2> fold_case_type;
    string_type fold_case(char_type ch) const
    {
        BOOST_MPL_ASSERT((is_same<char_type, char>));
        char_type ntcs[] = {
            detail::c_tolower(ch)
          , detail::c_toupper(ch)
          , 0
        };
        if(ntcs[1] == ntcs[0])
            ntcs[1] = 0;
        return string_type(ntcs);
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

    /// Checks to see if a character is within a character range, irregardless of case.
    ///
    /// \param first The bottom of the range, inclusive.
    /// \param last The top of the range, inclusive.
    /// \param ch The source character.
    /// \return in_range(first, last, ch) || in_range(first, last, tolower(ch)) || in_range(first,
    ///     last, toupper(ch))
    /// \attention The default implementation doesn't do proper Unicode
    ///     case folding, but this is the best we can do with the standard
    ///     C locale functions.
    static bool in_range_nocase(char_type first, char_type last, char_type ch)
    {
        return c_regex_traits::in_range(first, last, ch)
            || c_regex_traits::in_range(first, last, detail::c_tolower(ch))
            || c_regex_traits::in_range(first, last, detail::c_toupper(ch));
    }

    /// Returns a sort key for the character sequence designated by the iterator range [F1, F2)
    /// such that if the character sequence [G1, G2) sorts before the character sequence [H1, H2)
    /// then v.transform(G1, G2) < v.transform(H1, H2).
    ///
    /// \attention Not currently used
    template<typename FwdIter>
    static string_type transform(FwdIter begin, FwdIter end)
    {
        BOOST_ASSERT(false); // BUGBUG implement me
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
        BOOST_ASSERT(false); // BUGBUG implement me
    }

    /// Returns a sequence of characters that represents the collating element
    /// consisting of the character sequence designated by the iterator range [F1, F2).
    /// Returns an empty string if the character sequence is not a valid collating element.
    ///
    /// \attention Not currently used
    template<typename FwdIter>
    static string_type lookup_collatename(FwdIter begin, FwdIter end)
    {
        BOOST_ASSERT(false); // BUGBUG implement me
    }

    /// For the character class name represented by the specified character sequence,
    /// return the corresponding bitmask representation.
    ///
    /// \param begin A forward iterator to the start of the character sequence representing
    ///     the name of the character class.
    /// \param end The end of the character sequence.
    /// \param icase Specifies whether the returned bitmask should represent the case-insensitive
    ///     version of the character class.
    /// \return A bitmask representing the character class.
    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        return detail::char_class_impl<char_type>::lookup_classname(begin, end, icase);
    }

    /// Tests a character against a character class bitmask.
    ///
    /// \param ch The character to test.
    /// \param mask The character class bitmask against which to test.
    /// \pre mask is a bitmask returned by lookup_classname, or is several such masks bit-or'ed
    ///     together.
    /// \return true if the character is a member of any of the specified character classes, false
    ///     otherwise.
    static bool isctype(char_type ch, char_class_type mask)
    {
        return detail::char_class_impl<char_type>::isctype(ch, mask);
    }

    /// Convert a digit character into the integer it represents.
    ///
    /// \param ch The digit character.
    /// \param radix The radix to use for the conversion.
    /// \pre radix is one of 8, 10, or 16.
    /// \return -1 if ch is not a digit character, the integer value of the character otherwise. If
    ///     char_type is char, std::strtol is used for the conversion. If char_type is wchar_t,
    ///     std::wcstol is used.
    static int value(char_type ch, int radix);

    /// No-op
    ///
    locale_type imbue(locale_type loc)
    {
        this->base_type::imbue(*this);
        return loc;
    }

    /// No-op
    ///
    static locale_type getloc()
    {
        locale_type loc;
        return loc;
    }
};

///////////////////////////////////////////////////////////////////////////////
// c_regex_traits<>::widen specializations
/// INTERNAL ONLY
template<>
inline char c_regex_traits<char>::widen(char ch)
{
    return ch;
}

#ifndef BOOST_XPRESSIVE_NO_WREGEX
/// INTERNAL ONLY
template<>
inline wchar_t c_regex_traits<wchar_t>::widen(char ch)
{
    using namespace std;
    return btowc(ch);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// c_regex_traits<>::hash specializations
/// INTERNAL ONLY
template<>
inline unsigned char c_regex_traits<char>::hash(char ch)
{
    return static_cast<unsigned char>(ch);
}

#ifndef BOOST_XPRESSIVE_NO_WREGEX
/// INTERNAL ONLY
template<>
inline unsigned char c_regex_traits<wchar_t>::hash(wchar_t ch)
{
    return static_cast<unsigned char>(ch);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// c_regex_traits<>::value specializations
/// INTERNAL ONLY
template<>
inline int c_regex_traits<char>::value(char ch, int radix)
{
    using namespace std;
    BOOST_ASSERT(8 == radix || 10 == radix || 16 == radix);
    char begin[2] = { ch, '\0' }, *end = 0;
    int val = strtol(begin, &end, radix);
    return begin == end ? -1 : val;
}

#ifndef BOOST_XPRESSIVE_NO_WREGEX
/// INTERNAL ONLY
template<>
inline int c_regex_traits<wchar_t>::value(wchar_t ch, int radix)
{
    using namespace std;
    BOOST_ASSERT(8 == radix || 10 == radix || 16 == radix);
    wchar_t begin[2] = { ch, L'\0' }, *end = 0;
    int val = wcstol(begin, &end, radix);
    return begin == end ? -1 : val;
}
#endif

// Narrow C traits has fold_case() member function.
template<>
struct has_fold_case<c_regex_traits<char> >
  : mpl::true_
{
};

}}

#endif
