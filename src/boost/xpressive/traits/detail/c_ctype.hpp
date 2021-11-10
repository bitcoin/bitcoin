///////////////////////////////////////////////////////////////////////////////
// c_ctype.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_TRAITS_DETAIL_C_CTYPE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_TRAITS_DETAIL_C_CTYPE_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <cctype>
#include <cstring>
#include <boost/mpl/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

#ifndef BOOST_XPRESSIVE_NO_WREGEX
# include <cwchar>
# include <cwctype>
#endif

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// isnewline
//
inline bool isnewline(char ch)
{
    switch(ch)
    {
    case L'\n': case L'\r': case L'\f':
        return true;
    default:
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// iswnewline
//
inline bool iswnewline(wchar_t ch)
{
    switch(ch)
    {
    case L'\n': case L'\r': case L'\f': case 0x2028u: case 0x2029u: case 0x85u:
        return true;
    default:
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// classname_a
//
template<typename FwdIter>
inline std::string classname_a(FwdIter begin, FwdIter end)
{
    std::string name(begin, end);
    for(std::size_t i = 0; i < name.size(); ++i)
    {
        using namespace std;
        name[i] = static_cast<char>(tolower(static_cast<unsigned char>(name[i])));
    }
    return name;
}

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
// classname_w
//
template<typename FwdIter>
inline std::wstring classname_w(FwdIter begin, FwdIter end)
{
    std::wstring name(begin, end);
    for(std::size_t i = 0; i < name.size(); ++i)
    {
        using namespace std;
        name[i] = towlower(name[i]);
    }
    return name;
}
#endif



///////////////////////////////////////////////////////////////////////////////
// char_class_impl
//
template<typename Char>
struct char_class_impl;


#if defined(__QNXNTO__) || defined(__VXWORKS__)

///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef short char_class_type;
    BOOST_MPL_ASSERT_RELATION(0x07FF, ==, (_XB|_XA|_XS|_BB|_CN|_DI|_LO|_PU|_SP|_UP|_XD));
    BOOST_STATIC_CONSTANT(short, char_class_underscore = 0x1000);
    BOOST_STATIC_CONSTANT(short, char_class_newline = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return _DI|_LO|_UP|_XA;
        if(name == "alpha")     return _LO|_UP|_XA;
        if(name == "blank")     return _SP|_XB;
        if(name == "cntrl")     return _BB;
        if(name == "d")         return _DI;
        if(name == "digit")     return _DI;
        if(name == "graph")     return _DI|_LO|_PU|_UP|_XA;
        if(name == "lower")     return icase ? (_LO|_UP) : _LO;
        if(name == "newline")   return char_class_newline;
        if(name == "print")     return _DI|_LO|_PU|_SP|_UP|_XA;
        if(name == "punct")     return _PU;
        if(name == "s")         return _CN|_SP|_XS;
        if(name == "space")     return _CN|_SP|_XS;
        if(name == "upper")     return icase ? (_UP|_LO) : _UP;
        if(name == "w")         return _DI|_LO|_UP|_XA|char_class_underscore;
        if(name == "xdigit")    return _XD;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        using namespace std;
        if(0 != (_Getchrtype((unsigned char)ch) & mask))
        {
            return true;
        }

        switch(ch)
        {
        case '_': return 0 != (mask & char_class_underscore);
        case '\n': case '\r': case '\f': return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef int char_class_type;
    //BOOST_STATIC_CONSTANT(int, char_class_alnum         = 0x0001);
    BOOST_STATIC_CONSTANT(int, char_class_alpha         = 0x0002);
    BOOST_STATIC_CONSTANT(int, char_class_blank         = 0x0004);
    BOOST_STATIC_CONSTANT(int, char_class_cntrl         = 0x0008);
    BOOST_STATIC_CONSTANT(int, char_class_digit         = 0x0010);
    //BOOST_STATIC_CONSTANT(int, char_class_graph         = 0x0020);
    BOOST_STATIC_CONSTANT(int, char_class_lower         = 0x0040);
    //BOOST_STATIC_CONSTANT(int, char_class_print         = 0x0080);
    BOOST_STATIC_CONSTANT(int, char_class_punct         = 0x0100);
    BOOST_STATIC_CONSTANT(int, char_class_space         = 0x0200);
    BOOST_STATIC_CONSTANT(int, char_class_upper         = 0x0400);
    BOOST_STATIC_CONSTANT(int, char_class_underscore    = 0x0800);
    BOOST_STATIC_CONSTANT(int, char_class_xdigit        = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline       = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return char_class_alpha|char_class_digit;
        if(name == L"alpha")    return char_class_alpha;
        if(name == L"blank")    return char_class_blank;
        if(name == L"cntrl")    return char_class_cntrl;
        if(name == L"d")        return char_class_digit;
        if(name == L"digit")    return char_class_digit;
        if(name == L"graph")    return char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"lower")    return icase ? (char_class_lower|char_class_upper) : char_class_lower;
        if(name == L"newline")  return char_class_newline;
        if(name == L"print")    return char_class_blank|char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"punct")    return char_class_punct;
        if(name == L"s")        return char_class_space;
        if(name == L"space")    return char_class_space;
        if(name == L"upper")    return icase ? (char_class_upper|char_class_lower) : char_class_upper;
        if(name == L"w")        return char_class_alpha|char_class_digit|char_class_underscore;
        if(name == L"xdigit")   return char_class_xdigit;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        return ((char_class_alpha & mask) && iswalpha(ch))
            || ((char_class_blank & mask) && (L' ' == ch || L'\t' == ch)) // BUGBUG
            || ((char_class_cntrl & mask) && iswcntrl(ch))
            || ((char_class_digit & mask) && iswdigit(ch))
            || ((char_class_lower & mask) && iswlower(ch))
            || ((char_class_newline & mask) && detail::iswnewline(ch))
            || ((char_class_punct & mask) && iswpunct(ch))
            || ((char_class_space & mask) && iswspace(ch))
            || ((char_class_upper & mask) && iswupper(ch))
            || ((char_class_underscore & mask) && L'_' == ch)
            || ((char_class_xdigit & mask) && iswxdigit(ch))
            ;
    }
};
#endif


#elif defined(__MINGW32_VERSION)


///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef int char_class_type;
    BOOST_MPL_ASSERT_RELATION(0x81FF, ==, (_ALPHA|_UPPER|_LOWER|_DIGIT|_SPACE|_PUNCT|_CONTROL|_BLANK|_HEX|_LEADBYTE));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return _ALPHA|_DIGIT;
        if(name == "alpha")     return _ALPHA;
        if(name == "blank")     return _BLANK; // this is ONLY space!!!
        if(name == "cntrl")     return _CONTROL;
        if(name == "d")         return _DIGIT;
        if(name == "digit")     return _DIGIT;
        if(name == "graph")     return _PUNCT|_ALPHA|_DIGIT;
        if(name == "lower")     return icase ? (_LOWER|_UPPER) : _LOWER;
        if(name == "newline")   return char_class_newline;
        if(name == "print")     return _BLANK|_PUNCT|_ALPHA|_DIGIT;
        if(name == "punct")     return _PUNCT;
        if(name == "s")         return _SPACE;
        if(name == "space")     return _SPACE;
        if(name == "upper")     return icase ? (_UPPER|_LOWER) : _UPPER;
        if(name == "w")         return _ALPHA|_DIGIT|char_class_underscore;
        if(name == "xdigit")    return _HEX;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        using namespace std;
        if(0 != _isctype(static_cast<unsigned char>(ch), mask))
        {
            return true;
        }

        switch(ch)
        {
        case '\t': return 0 != (mask & _BLANK);
        case '_': return 0 != (mask & char_class_underscore);
        case '\n': case '\r': case '\f': return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef wctype_t char_class_type;
    BOOST_MPL_ASSERT_RELATION(0x81FF, ==, (_ALPHA|_UPPER|_LOWER|_DIGIT|_SPACE|_PUNCT|_CONTROL|_BLANK|_HEX|_LEADBYTE));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return _ALPHA|_DIGIT;
        if(name == L"alpha")    return _ALPHA;
        if(name == L"blank")    return _BLANK; // this is ONLY space!!!
        if(name == L"cntrl")    return _CONTROL;
        if(name == L"d")        return _DIGIT;
        if(name == L"digit")    return _DIGIT;
        if(name == L"graph")    return _PUNCT|_ALPHA|_DIGIT;
        if(name == L"lower")    return icase ? (_LOWER|_UPPER) : _LOWER;
        if(name == L"newline")  return char_class_newline;
        if(name == L"print")    return _BLANK|_PUNCT|_ALPHA|_DIGIT;
        if(name == L"punct")    return _PUNCT;
        if(name == L"s")        return _SPACE;
        if(name == L"space")    return _SPACE;
        if(name == L"upper")    return icase ? (_UPPER|_LOWER) : _UPPER;
        if(name == L"w")        return _ALPHA|_DIGIT|char_class_underscore;
        if(name == L"xdigit")   return _HEX;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        if(0 != iswctype(ch, mask))
        {
            return true;
        }

        switch(ch)
        {
        case L'\t': return 0 != (mask & _BLANK);
        case L'_': return 0 != (mask & char_class_underscore);
        case L'\n': case L'\r': case L'\f': case 0x2028u: case 0x2029u: case 0x85u:
            return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};
#endif


#elif defined(__CYGWIN__)



///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef int char_class_type;
    BOOST_MPL_ASSERT_RELATION(0377, ==, (_U|_L|_N|_S|_P|_C|_B|_X));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0400);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 01000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return _U|_L|_N;
        if(name == "alpha")     return _U|_L;
        if(name == "blank")     return _B; // BUGBUG what is this?
        if(name == "cntrl")     return _C;
        if(name == "d")         return _N;
        if(name == "digit")     return _N;
        if(name == "graph")     return _P|_U|_L|_N;
        if(name == "lower")     return icase ? (_L|_U) : _L;
        if(name == "newline")   return char_class_newline;
        if(name == "print")     return _B|_P|_U|_L|_N;
        if(name == "punct")     return _P;
        if(name == "s")         return _S;
        if(name == "space")     return _S;
        if(name == "upper")     return icase ? (_U|_L) : _U;
        if(name == "w")         return _U|_L|_N|char_class_underscore;
        if(name == "xdigit")    return _X;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        if(0 != static_cast<unsigned char>(((_ctype_+1)[(unsigned)(ch)]) & mask))
        {
            return true;
        }

        switch(ch)
        {
        case '_': return 0 != (mask & char_class_underscore);
        case '\n': case '\r': case '\f': return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef int char_class_type;
    //BOOST_STATIC_CONSTANT(int, char_class_alnum         = 0x0001);
    BOOST_STATIC_CONSTANT(int, char_class_alpha         = 0x0002);
    BOOST_STATIC_CONSTANT(int, char_class_blank         = 0x0004);
    BOOST_STATIC_CONSTANT(int, char_class_cntrl         = 0x0008);
    BOOST_STATIC_CONSTANT(int, char_class_digit         = 0x0010);
    //BOOST_STATIC_CONSTANT(int, char_class_graph         = 0x0020);
    BOOST_STATIC_CONSTANT(int, char_class_lower         = 0x0040);
    //BOOST_STATIC_CONSTANT(int, char_class_print         = 0x0080);
    BOOST_STATIC_CONSTANT(int, char_class_punct         = 0x0100);
    BOOST_STATIC_CONSTANT(int, char_class_space         = 0x0200);
    BOOST_STATIC_CONSTANT(int, char_class_upper         = 0x0400);
    BOOST_STATIC_CONSTANT(int, char_class_underscore    = 0x0800);
    BOOST_STATIC_CONSTANT(int, char_class_xdigit        = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline       = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return char_class_alpha|char_class_digit;
        if(name == L"alpha")    return char_class_alpha;
        if(name == L"blank")    return char_class_blank;
        if(name == L"cntrl")    return char_class_cntrl;
        if(name == L"d")        return char_class_digit;
        if(name == L"digit")    return char_class_digit;
        if(name == L"graph")    return char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"lower")    return icase ? (char_class_lower|char_class_upper) : char_class_lower;
        if(name == L"newline")  return char_class_newline;
        if(name == L"print")    return char_class_blank|char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"punct")    return char_class_punct;
        if(name == L"s")        return char_class_space;
        if(name == L"space")    return char_class_space;
        if(name == L"upper")    return icase ? (char_class_upper|char_class_lower) : char_class_upper;
        if(name == L"w")        return char_class_alpha|char_class_digit|char_class_underscore;
        if(name == L"xdigit")   return char_class_xdigit;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        return ((char_class_alpha & mask) && iswalpha(ch))
            || ((char_class_blank & mask) && (L' ' == ch || L'\t' == ch)) // BUGBUG
            || ((char_class_cntrl & mask) && iswcntrl(ch))
            || ((char_class_digit & mask) && iswdigit(ch))
            || ((char_class_lower & mask) && iswlower(ch))
            || ((char_class_newline & mask) && detail::iswnewline(ch))
            || ((char_class_punct & mask) && iswpunct(ch))
            || ((char_class_space & mask) && iswspace(ch))
            || ((char_class_upper & mask) && iswupper(ch))
            || ((char_class_underscore & mask) && L'_' == ch)
            || ((char_class_xdigit & mask) && iswxdigit(ch))
            ;
    }
};
#endif



#elif defined(__GLIBC__)




///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef int char_class_type;
    BOOST_MPL_ASSERT_RELATION(0xffff, ==, (_ISalnum|_ISalpha|_ISblank|_IScntrl|_ISdigit|_ISgraph|
                                           _ISlower|_ISprint|_ISpunct|_ISspace|_ISupper|_ISxdigit|0xffff));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0x00010000);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 0x00020000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return _ISalnum;
        if(name == "alpha")     return _ISalpha;
        if(name == "blank")     return _ISblank;
        if(name == "cntrl")     return _IScntrl;
        if(name == "d")         return _ISdigit;
        if(name == "digit")     return _ISdigit;
        if(name == "graph")     return _ISgraph;
        if(name == "lower")     return icase ? (_ISlower|_ISupper) : _ISlower;
        if(name == "print")     return _ISprint;
        if(name == "newline")   return char_class_newline;
        if(name == "punct")     return _ISpunct;
        if(name == "s")         return _ISspace;
        if(name == "space")     return _ISspace;
        if(name == "upper")     return icase ? (_ISupper|_ISlower) : _ISupper;
        if(name == "w")         return _ISalnum|char_class_underscore;
        if(name == "xdigit")    return _ISxdigit;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        if(glibc_isctype(ch, mask))
        {
            return true;
        }

        switch(ch)
        {
        case '_': return 0 != (mask & char_class_underscore);
        case '\n': case '\r': case '\f': return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }

    static bool glibc_isctype(char ch, char_class_type mask)
    {
        #ifdef __isctype
        return 0 != __isctype(ch, mask);
        #else
        return 0 != ((*__ctype_b_loc())[(int)(ch)] & (unsigned short int)mask);
        #endif
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef int char_class_type;
    //BOOST_STATIC_CONSTANT(int, char_class_alnum         = 0x0001);
    BOOST_STATIC_CONSTANT(int, char_class_alpha         = 0x0002);
    BOOST_STATIC_CONSTANT(int, char_class_blank         = 0x0004);
    BOOST_STATIC_CONSTANT(int, char_class_cntrl         = 0x0008);
    BOOST_STATIC_CONSTANT(int, char_class_digit         = 0x0010);
    //BOOST_STATIC_CONSTANT(int, char_class_graph         = 0x0020);
    BOOST_STATIC_CONSTANT(int, char_class_lower         = 0x0040);
    //BOOST_STATIC_CONSTANT(int, char_class_print         = 0x0080);
    BOOST_STATIC_CONSTANT(int, char_class_punct         = 0x0100);
    BOOST_STATIC_CONSTANT(int, char_class_space         = 0x0200);
    BOOST_STATIC_CONSTANT(int, char_class_upper         = 0x0400);
    BOOST_STATIC_CONSTANT(int, char_class_underscore    = 0x0800);
    BOOST_STATIC_CONSTANT(int, char_class_xdigit        = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline       = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return char_class_alpha|char_class_digit;
        if(name == L"alpha")    return char_class_alpha;
        if(name == L"blank")    return char_class_blank;
        if(name == L"cntrl")    return char_class_cntrl;
        if(name == L"d")        return char_class_digit;
        if(name == L"digit")    return char_class_digit;
        if(name == L"graph")    return char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"lower")    return icase ? (char_class_lower|char_class_upper) : char_class_lower;
        if(name == L"newline")  return char_class_newline;
        if(name == L"print")    return char_class_blank|char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"punct")    return char_class_punct;
        if(name == L"s")        return char_class_space;
        if(name == L"space")    return char_class_space;
        if(name == L"upper")    return icase ? (char_class_upper|char_class_lower) : char_class_upper;
        if(name == L"w")        return char_class_alpha|char_class_digit|char_class_underscore;
        if(name == L"xdigit")   return char_class_xdigit;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        return ((char_class_alpha & mask) && iswalpha(ch))
            || ((char_class_blank & mask) && (L' ' == ch || L'\t' == ch)) // BUGBUG
            || ((char_class_cntrl & mask) && iswcntrl(ch))
            || ((char_class_digit & mask) && iswdigit(ch))
            || ((char_class_lower & mask) && iswlower(ch))
            || ((char_class_newline & mask) && detail::iswnewline(ch))
            || ((char_class_punct & mask) && iswpunct(ch))
            || ((char_class_space & mask) && iswspace(ch))
            || ((char_class_upper & mask) && iswupper(ch))
            || ((char_class_underscore & mask) && L'_' == ch)
            || ((char_class_xdigit & mask) && iswxdigit(ch))
            ;
    }
};
#endif



#elif defined(_CPPLIB_VER) // Dinkumware STL




///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef int char_class_type;
    BOOST_MPL_ASSERT_RELATION(0x81FF, ==, (_ALPHA|_UPPER|_LOWER|_DIGIT|_SPACE|_PUNCT|_CONTROL|_BLANK|_HEX|_LEADBYTE));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return _ALPHA|_DIGIT;
        if(name == "alpha")     return _ALPHA;
        if(name == "blank")     return _BLANK; // this is ONLY space!!!
        if(name == "cntrl")     return _CONTROL;
        if(name == "d")         return _DIGIT;
        if(name == "digit")     return _DIGIT;
        if(name == "graph")     return _PUNCT|_ALPHA|_DIGIT;
        if(name == "lower")     return icase ? (_LOWER|_UPPER) : _LOWER;
        if(name == "newline")   return char_class_newline;
        if(name == "print")     return _BLANK|_PUNCT|_ALPHA|_DIGIT;
        if(name == "punct")     return _PUNCT;
        if(name == "s")         return _SPACE;
        if(name == "space")     return _SPACE;
        if(name == "upper")     return icase ? (_UPPER|_LOWER) : _UPPER;
        if(name == "w")         return _ALPHA|_DIGIT|char_class_underscore;
        if(name == "xdigit")    return _HEX;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        using namespace std;
        if(0 != _isctype(static_cast<unsigned char>(ch), mask))
        {
            return true;
        }

        switch(ch)
        {
        case '\t': return 0 != (mask & _BLANK);
        case '_': return 0 != (mask & char_class_underscore);
        case '\n': case '\r': case '\f': return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef wctype_t char_class_type;
    BOOST_MPL_ASSERT_RELATION(0x81FF, ==, (_ALPHA|_UPPER|_LOWER|_DIGIT|_SPACE|_PUNCT|_CONTROL|_BLANK|_HEX|_LEADBYTE));
    BOOST_STATIC_CONSTANT(int, char_class_underscore = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return _ALPHA|_DIGIT;
        if(name == L"alpha")    return _ALPHA;
        if(name == L"blank")    return _BLANK; // this is ONLY space!!!
        if(name == L"cntrl")    return _CONTROL;
        if(name == L"d")        return _DIGIT;
        if(name == L"digit")    return _DIGIT;
        if(name == L"graph")    return _PUNCT|_ALPHA|_DIGIT;
        if(name == L"lower")    return icase ? _LOWER|_UPPER : _LOWER;
        if(name == L"newline")  return char_class_newline;
        if(name == L"print")    return _BLANK|_PUNCT|_ALPHA|_DIGIT;
        if(name == L"punct")    return _PUNCT;
        if(name == L"s")        return _SPACE;
        if(name == L"space")    return _SPACE;
        if(name == L"upper")    return icase ? _UPPER|_LOWER : _UPPER;
        if(name == L"w")        return _ALPHA|_DIGIT|char_class_underscore;
        if(name == L"xdigit")   return _HEX;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        if(0 != iswctype(ch, mask))
        {
            return true;
        }

        switch(ch)
        {
        case L'\t': return 0 != (mask & _BLANK);
        case L'_': return 0 != (mask & char_class_underscore);
        case L'\n': case L'\r': case L'\f': case 0x2028u: case 0x2029u: case 0x85u:
            return 0 != (mask & char_class_newline);
        default:;
        }

        return false;
    }
};
#endif



#else // unknown, use portable implementation




///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<char>
{
    typedef int char_class_type;
    //BOOST_STATIC_CONSTANT(int, char_class_alnum         = 0x0001);
    BOOST_STATIC_CONSTANT(int, char_class_alpha         = 0x0002);
    BOOST_STATIC_CONSTANT(int, char_class_blank         = 0x0004);
    BOOST_STATIC_CONSTANT(int, char_class_cntrl         = 0x0008);
    BOOST_STATIC_CONSTANT(int, char_class_digit         = 0x0010);
    //BOOST_STATIC_CONSTANT(int, char_class_graph         = 0x0020);
    BOOST_STATIC_CONSTANT(int, char_class_lower         = 0x0040);
    //BOOST_STATIC_CONSTANT(int, char_class_print         = 0x0080);
    BOOST_STATIC_CONSTANT(int, char_class_punct         = 0x0100);
    BOOST_STATIC_CONSTANT(int, char_class_space         = 0x0200);
    BOOST_STATIC_CONSTANT(int, char_class_upper         = 0x0400);
    BOOST_STATIC_CONSTANT(int, char_class_underscore    = 0x0800);
    BOOST_STATIC_CONSTANT(int, char_class_xdigit        = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline       = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        string const name = classname_a(begin, end);
        if(name == "alnum")     return char_class_alpha|char_class_digit;
        if(name == "alpha")     return char_class_alpha;
        if(name == "blank")     return char_class_blank;
        if(name == "cntrl")     return char_class_cntrl;
        if(name == "d")         return char_class_digit;
        if(name == "digit")     return char_class_digit;
        if(name == "graph")     return char_class_punct|char_class_alpha|char_class_digit;
        if(name == "lower")     return icase ? (char_class_lower|char_class_upper) : char_class_lower;
        if(name == "newline")   return char_class_newline;
        if(name == "print")     return char_class_blank|char_class_punct|char_class_alpha|char_class_digit;
        if(name == "punct")     return char_class_punct;
        if(name == "s")         return char_class_space;
        if(name == "space")     return char_class_space;
        if(name == "upper")     return icase ? (char_class_upper|char_class_lower) : char_class_upper;
        if(name == "w")         return char_class_alpha|char_class_digit|char_class_underscore;
        if(name == "xdigit")    return char_class_xdigit;
        return 0;
    }

    static bool isctype(char ch, char_class_type mask)
    {
        using namespace std;
        unsigned char uch = static_cast<unsigned char>(ch);
        return ((char_class_alpha & mask) && isalpha(uch))
            || ((char_class_blank & mask) && (' ' == ch || '\t' == ch)) // BUGBUG
            || ((char_class_cntrl & mask) && iscntrl(uch))
            || ((char_class_digit & mask) && isdigit(uch))
            || ((char_class_lower & mask) && islower(uch))
            || ((char_class_newline & mask) && detail::isnewline(ch))
            || ((char_class_punct & mask) && ispunct(uch))
            || ((char_class_space & mask) && isspace(uch))
            || ((char_class_upper & mask) && isupper(uch))
            || ((char_class_underscore & mask) && '_' == ch)
            || ((char_class_xdigit & mask) && isxdigit(uch))
            ;
    }
};

#ifndef BOOST_XPRESSIVE_NO_WREGEX
///////////////////////////////////////////////////////////////////////////////
//
template<>
struct char_class_impl<wchar_t>
{
    typedef int char_class_type;
    //BOOST_STATIC_CONSTANT(int, char_class_alnum         = 0x0001);
    BOOST_STATIC_CONSTANT(int, char_class_alpha         = 0x0002);
    BOOST_STATIC_CONSTANT(int, char_class_blank         = 0x0004);
    BOOST_STATIC_CONSTANT(int, char_class_cntrl         = 0x0008);
    BOOST_STATIC_CONSTANT(int, char_class_digit         = 0x0010);
    //BOOST_STATIC_CONSTANT(int, char_class_graph         = 0x0020);
    BOOST_STATIC_CONSTANT(int, char_class_lower         = 0x0040);
    //BOOST_STATIC_CONSTANT(int, char_class_print         = 0x0080);
    BOOST_STATIC_CONSTANT(int, char_class_punct         = 0x0100);
    BOOST_STATIC_CONSTANT(int, char_class_space         = 0x0200);
    BOOST_STATIC_CONSTANT(int, char_class_upper         = 0x0400);
    BOOST_STATIC_CONSTANT(int, char_class_underscore    = 0x0800);
    BOOST_STATIC_CONSTANT(int, char_class_xdigit        = 0x1000);
    BOOST_STATIC_CONSTANT(int, char_class_newline       = 0x2000);

    template<typename FwdIter>
    static char_class_type lookup_classname(FwdIter begin, FwdIter end, bool icase)
    {
        using namespace std;
        wstring const name = classname_w(begin, end);
        if(name == L"alnum")    return char_class_alpha|char_class_digit;
        if(name == L"alpha")    return char_class_alpha;
        if(name == L"blank")    return char_class_blank;
        if(name == L"cntrl")    return char_class_cntrl;
        if(name == L"d")        return char_class_digit;
        if(name == L"digit")    return char_class_digit;
        if(name == L"graph")    return char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"newline")  return char_class_newline;
        if(name == L"lower")    return icase ? (char_class_lower|char_class_upper) : char_class_lower;
        if(name == L"print")    return char_class_blank|char_class_punct|char_class_alpha|char_class_digit;
        if(name == L"punct")    return char_class_punct;
        if(name == L"s")        return char_class_space;
        if(name == L"space")    return char_class_space;
        if(name == L"upper")    return icase ? (char_class_upper|char_class_lower) : char_class_upper;
        if(name == L"w")        return char_class_alpha|char_class_digit|char_class_underscore;
        if(name == L"xdigit")   return char_class_xdigit;
        return 0;
    }

    static bool isctype(wchar_t ch, char_class_type mask)
    {
        using namespace std;
        return ((char_class_alpha & mask) && iswalpha(ch))
            || ((char_class_blank & mask) && (L' ' == ch || L'\t' == ch)) // BUGBUG
            || ((char_class_cntrl & mask) && iswcntrl(ch))
            || ((char_class_digit & mask) && iswdigit(ch))
            || ((char_class_lower & mask) && iswlower(ch))
            || ((char_class_newline & mask) && detail::iswnewline(ch))
            || ((char_class_punct & mask) && iswpunct(ch))
            || ((char_class_space & mask) && iswspace(ch))
            || ((char_class_upper & mask) && iswupper(ch))
            || ((char_class_underscore & mask) && L'_' == ch)
            || ((char_class_xdigit & mask) && iswxdigit(ch))
            ;
    }
};
#endif


#endif


}}} // namespace boost::xpressive::detail

#endif
