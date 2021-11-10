//  ratio_io
//
//  (C) Copyright Howard Hinnant
//  (C) Copyright 2010 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
// This code was adapted by Vicente from Howard Hinnant's experimental work
// on chrono i/o under lvm/libc++ to Boost

#ifndef BOOST_RATIO_DETAIL_RATIO_IO_HPP
#define BOOST_RATIO_DETAIL_RATIO_IO_HPP

/*

    ratio_io synopsis

#include <ratio>
#include <string>

namespace boost
{

template <class Ratio, class CharT>
struct ratio_string
{
    static basic_string<CharT> short_name();
    static basic_string<CharT> long_name();
};

}  // boost

*/

#include <boost/config.hpp>
#include <boost/ratio/ratio.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <string>
#include <sstream>

#if defined(BOOST_NO_CXX11_UNICODE_LITERALS) || defined(BOOST_NO_CXX11_CHAR16_T) || defined(BOOST_NO_CXX11_CHAR32_T) || defined(BOOST_NO_CXX11_U16STRING) || defined(BOOST_NO_CXX11_U32STRING)
#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT
#undef BOOST_RATIO_HAS_UNICODE_SUPPORT
#endif
#else
#define BOOST_RATIO_HAS_UNICODE_SUPPORT 1
#endif

namespace boost {

//template <class Ratio>
//struct ratio_string_is_localizable : false_type {};
//template <class Ratio>
//struct ratio_string_id : integral_constant<int,0> {};

template <class Ratio, class CharT>
struct ratio_string
{
    static std::basic_string<CharT> short_name() {return long_name();}
    static std::basic_string<CharT> long_name();
    static std::basic_string<CharT> symbol() {return short_name();}
    static std::basic_string<CharT> prefix() {return long_name();}
};

template <class Ratio, class CharT>
std::basic_string<CharT>
ratio_string<Ratio, CharT>::long_name()
{
    std::basic_ostringstream<CharT> os;
    os << CharT('[') << Ratio::num << CharT('/')
                        << Ratio::den << CharT(']');
    return os.str();
}

#ifdef BOOST_RATIO_HAS_STATIC_STRING
namespace ratio_detail {
template <class Ratio, class CharT>
struct ratio_string_static
{
    static std::string short_name() {
        return std::basic_string<CharT>(
                static_string::c_str<
                        typename ratio_static_string<Ratio, CharT>::short_name
                    >::value);
    }
    static std::string long_name()  {
        return std::basic_string<CharT>(
                static_string::c_str<
                    typename ratio_static_string<Ratio, CharT>::long_name
                >::value);
    }
    static std::basic_string<CharT> symbol() {return short_name();}
    static std::basic_string<CharT> prefix() {return long_name();}
};
}
#endif
// atto
//template <>
//struct ratio_string_is_localizable<atto> : true_type {};
//
//template <>
//struct ratio_string_id<atto> : integral_constant<int,-18> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<atto, CharT> :
    ratio_detail::ratio_string_static<atto,CharT>
{};

#else
template <>
struct ratio_string<atto, char>
{
    static std::string short_name() {return std::string(1, 'a');}
    static std::string long_name()  {return std::string("atto");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<atto, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'a');}
    static std::u16string long_name()  {return std::u16string(u"atto");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<atto, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'a');}
    static std::u32string long_name()  {return std::u32string(U"atto");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<atto, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'a');}
    static std::wstring long_name()  {return std::wstring(L"atto");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// femto

//template <>
//struct ratio_string_is_localizable<femto> : true_type {};
//
//template <>
//struct ratio_string_id<femto> : integral_constant<int,-15> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<femto, CharT> :
    ratio_detail::ratio_string_static<femto,CharT>
{};

#else

template <>
struct ratio_string<femto, char>
{
    static std::string short_name() {return std::string(1, 'f');}
    static std::string long_name()  {return std::string("femto");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<femto, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'f');}
    static std::u16string long_name()  {return std::u16string(u"femto");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<femto, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'f');}
    static std::u32string long_name()  {return std::u32string(U"femto");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<femto, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'f');}
    static std::wstring long_name()  {return std::wstring(L"femto");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// pico

//template <>
//struct ratio_string_is_localizable<pico> : true_type {};
//
//template <>
//struct ratio_string_id<pico> : integral_constant<int,-12> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<pico, CharT> :
    ratio_detail::ratio_string_static<pico,CharT>
{};

#else
template <>
struct ratio_string<pico, char>
{
    static std::string short_name() {return std::string(1, 'p');}
    static std::string long_name()  {return std::string("pico");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<pico, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'p');}
    static std::u16string long_name()  {return std::u16string(u"pico");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<pico, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'p');}
    static std::u32string long_name()  {return std::u32string(U"pico");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<pico, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'p');}
    static std::wstring long_name()  {return std::wstring(L"pico");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// nano

//template <>
//struct ratio_string_is_localizable<nano> : true_type {};
//
//template <>
//struct ratio_string_id<nano> : integral_constant<int,-9> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<nano, CharT> :
    ratio_detail::ratio_string_static<nano,CharT>
{};

#else
template <>
struct ratio_string<nano, char>
{
    static std::string short_name() {return std::string(1, 'n');}
    static std::string long_name()  {return std::string("nano");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<nano, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'n');}
    static std::u16string long_name()  {return std::u16string(u"nano");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<nano, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'n');}
    static std::u32string long_name()  {return std::u32string(U"nano");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<nano, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'n');}
    static std::wstring long_name()  {return std::wstring(L"nano");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// micro

//template <>
//struct ratio_string_is_localizable<micro> : true_type {};
//
//template <>
//struct ratio_string_id<micro> : integral_constant<int,-6> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<micro, CharT> :
    ratio_detail::ratio_string_static<micro,CharT>
{};

#else
template <>
struct ratio_string<micro, char>
{
    static std::string short_name() {return std::string("\xC2\xB5");}
    static std::string long_name()  {return std::string("micro");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<micro, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'\xB5');}
    static std::u16string long_name()  {return std::u16string(u"micro");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<micro, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'\xB5');}
    static std::u32string long_name()  {return std::u32string(U"micro");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<micro, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'\xB5');}
    static std::wstring long_name()  {return std::wstring(L"micro");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// milli

//template <>
//struct ratio_string_is_localizable<milli> : true_type {};
//
//template <>
//struct ratio_string_id<milli> : integral_constant<int,-3> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<milli, CharT> :
    ratio_detail::ratio_string_static<milli,CharT>
{};

#else
template <>
struct ratio_string<milli, char>
{
    static std::string short_name() {return std::string(1, 'm');}
    static std::string long_name()  {return std::string("milli");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<milli, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'm');}
    static std::u16string long_name()  {return std::u16string(u"milli");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<milli, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'm');}
    static std::u32string long_name()  {return std::u32string(U"milli");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<milli, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'm');}
    static std::wstring long_name()  {return std::wstring(L"milli");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// centi

//template <>
//struct ratio_string_is_localizable<centi> : true_type {};
//
//template <>
//struct ratio_string_id<centi> : integral_constant<int,-2> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<centi, CharT> :
    ratio_detail::ratio_string_static<centi,CharT>
{};

#else
template <>
struct ratio_string<centi, char>
{
    static std::string short_name() {return std::string(1, 'c');}
    static std::string long_name()  {return std::string("centi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<centi, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'c');}
    static std::u16string long_name()  {return std::u16string(u"centi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<centi, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'c');}
    static std::u32string long_name()  {return std::u32string(U"centi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<centi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'c');}
    static std::wstring long_name()  {return std::wstring(L"centi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// deci

//template <>
//struct ratio_string_is_localizable<deci> : true_type {};
//
//template <>
//struct ratio_string_id<deci> : integral_constant<int,-1> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<deci, CharT> :
    ratio_detail::ratio_string_static<deci,CharT>
{};

#else

template <>
struct ratio_string<deci, char>
{
    static std::string short_name() {return std::string(1, 'd');}
    static std::string long_name()  {return std::string("deci");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<deci, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'd');}
    static std::u16string long_name()  {return std::u16string(u"deci");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<deci, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'd');}
    static std::u32string long_name()  {return std::u32string(U"deci");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<deci, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'd');}
    static std::wstring long_name()  {return std::wstring(L"deci");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// unit

//template <>
//struct ratio_string_is_localizable<ratio<1> > : true_type {};
//
//template <>
//struct ratio_string_id<ratio<1> > : integral_constant<int,0> {};
// deca

//template <>
//struct ratio_string_is_localizable<deca> : true_type {};
//
//template <>
//struct ratio_string_id<deca> : integral_constant<int,1> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<deca, CharT> :
    ratio_detail::ratio_string_static<deca,CharT>
{};

#else
template <>
struct ratio_string<deca, char>
{
    static std::string short_name() {return std::string("da");}
    static std::string long_name()  {return std::string("deca");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<deca, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"da");}
    static std::u16string long_name()  {return std::u16string(u"deca");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<deca, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"da");}
    static std::u32string long_name()  {return std::u32string(U"deca");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<deca, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"da");}
    static std::wstring long_name()  {return std::wstring(L"deca");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// hecto

//template <>
//struct ratio_string_is_localizable<hecto> : true_type {};
//
//template <>
//struct ratio_string_id<hecto> : integral_constant<int,2> {};


#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<hecto, CharT> :
    ratio_detail::ratio_string_static<hecto,CharT>
{};

#else
template <>
struct ratio_string<hecto, char>
{
    static std::string short_name() {return std::string(1, 'h');}
    static std::string long_name()  {return std::string("hecto");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<hecto, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'h');}
    static std::u16string long_name()  {return std::u16string(u"hecto");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<hecto, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'h');}
    static std::u32string long_name()  {return std::u32string(U"hecto");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<hecto, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'h');}
    static std::wstring long_name()  {return std::wstring(L"hecto");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// kilo

//template <>
//struct ratio_string_is_localizable<kilo> : true_type {};
//
//template <>
//struct ratio_string_id<kilo> : integral_constant<int,3> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<kilo, CharT> :
    ratio_detail::ratio_string_static<kilo,CharT>
{};

#else
template <>
struct ratio_string<kilo, char>
{
    static std::string short_name() {return std::string(1, 'k');}
    static std::string long_name()  {return std::string("kilo");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<kilo, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'k');}
    static std::u16string long_name()  {return std::u16string(u"kilo");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<kilo, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'k');}
    static std::u32string long_name()  {return std::u32string(U"kilo");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<kilo, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'k');}
    static std::wstring long_name()  {return std::wstring(L"kilo");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// mega

//template <>
//struct ratio_string_is_localizable<mega> : true_type {};
//
//template <>
//struct ratio_string_id<mega> : integral_constant<int,6> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<mega, CharT> :
    ratio_detail::ratio_string_static<mega,CharT>
{};

#else

template <>
struct ratio_string<mega, char>
{
    static std::string short_name() {return std::string(1, 'M');}
    static std::string long_name()  {return std::string("mega");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<mega, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'M');}
    static std::u16string long_name()  {return std::u16string(u"mega");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<mega, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'M');}
    static std::u32string long_name()  {return std::u32string(U"mega");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<mega, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'M');}
    static std::wstring long_name()  {return std::wstring(L"mega");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// giga

//template <>
//struct ratio_string_is_localizable<giga> : true_type {};
//
//template <>
//struct ratio_string_id<giga> : integral_constant<int,9> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<giga, CharT> :
    ratio_detail::ratio_string_static<giga,CharT>
{};

#else

template <>
struct ratio_string<giga, char>
{
    static std::string short_name() {return std::string(1, 'G');}
    static std::string long_name()  {return std::string("giga");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<giga, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'G');}
    static std::u16string long_name()  {return std::u16string(u"giga");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<giga, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'G');}
    static std::u32string long_name()  {return std::u32string(U"giga");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<giga, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'G');}
    static std::wstring long_name()  {return std::wstring(L"giga");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// tera

//template <>
//struct ratio_string_is_localizable<tera> : true_type {};
//
//template <>
//struct ratio_string_id<tera> : integral_constant<int,12> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<tera, CharT> :
    ratio_detail::ratio_string_static<tera,CharT>
{};

#else
template <>
struct ratio_string<tera, char>
{
    static std::string short_name() {return std::string(1, 'T');}
    static std::string long_name()  {return std::string("tera");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<tera, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'T');}
    static std::u16string long_name()  {return std::u16string(u"tera");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<tera, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'T');}
    static std::u32string long_name()  {return std::u32string(U"tera");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<tera, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'T');}
    static std::wstring long_name()  {return std::wstring(L"tera");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// peta

//template <>
//struct ratio_string_is_localizable<peta> : true_type {};
//
//template <>
//struct ratio_string_id<peta> : integral_constant<int,15> {};


#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<peta, CharT> :
    ratio_detail::ratio_string_static<peta,CharT>
{};

#else
template <>
struct ratio_string<peta, char>
{
    static std::string short_name() {return std::string(1, 'P');}
    static std::string long_name()  {return std::string("peta");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<peta, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'P');}
    static std::u16string long_name()  {return std::u16string(u"peta");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<peta, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'P');}
    static std::u32string long_name()  {return std::u32string(U"peta");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<peta, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'P');}
    static std::wstring long_name()  {return std::wstring(L"peta");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

// exa

//template <>
//struct ratio_string_is_localizable<exa> : true_type {};
//
//template <>
//struct ratio_string_id<exa> : integral_constant<int,18> {};

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<exa, CharT> :
    ratio_detail::ratio_string_static<exa,CharT>
{};

#else
template <>
struct ratio_string<exa, char>
{
    static std::string short_name() {return std::string(1, 'E');}
    static std::string long_name()  {return std::string("exa");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<exa, char16_t>
{
    static std::u16string short_name() {return std::u16string(1, u'E');}
    static std::u16string long_name()  {return std::u16string(u"exa");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<exa, char32_t>
{
    static std::u32string short_name() {return std::u32string(1, U'E');}
    static std::u32string long_name()  {return std::u32string(U"exa");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<exa, wchar_t>
{
    static std::wstring short_name() {return std::wstring(1, L'E');}
    static std::wstring long_name()  {return std::wstring(L"exa");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_EXTENSIONS

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<kibi, CharT> :
    ratio_detail::ratio_string_static<kibi,CharT>
{};

#else
template <>
struct ratio_string<kibi, char>
{
    static std::string short_name() {return std::string("Ki");}
    static std::string long_name()  {return std::string("kibi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<kibi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Ki");}
    static std::u16string long_name()  {return std::u16string(u"kibi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<kibi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Ki");}
    static std::u32string long_name()  {return std::u32string(U"kibi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<kibi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Ki");}
    static std::wstring long_name()  {return std::wstring(L"kibi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<mebi, CharT> :
    ratio_detail::ratio_string_static<mebi,CharT>
{};

#else
template <>
struct ratio_string<mebi, char>
{
    static std::string short_name() {return std::string("Mi");}
    static std::string long_name()  {return std::string("mebi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<mebi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Mi");}
    static std::u16string long_name()  {return std::u16string(u"mebi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<mebi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Mi");}
    static std::u32string long_name()  {return std::u32string(U"mebi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<mebi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Mi");}
    static std::wstring long_name()  {return std::wstring(L"mebi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<gibi, CharT> :
    ratio_detail::ratio_string_static<gibi,CharT>
{};

#else
template <>
struct ratio_string<gibi, char>
{
    static std::string short_name() {return std::string("Gi");}
    static std::string long_name()  {return std::string("gibi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<gibi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Gi");}
    static std::u16string long_name()  {return std::u16string(u"gibi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<gibi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Gi");}
    static std::u32string long_name()  {return std::u32string(U"gibi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<gibi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Gi");}
    static std::wstring long_name()  {return std::wstring(L"gibi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<tebi, CharT> :
    ratio_detail::ratio_string_static<tebi,CharT>
{};

#else
template <>
struct ratio_string<tebi, char>
{
    static std::string short_name() {return std::string("Ti");}
    static std::string long_name()  {return std::string("tebi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<tebi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Ti");}
    static std::u16string long_name()  {return std::u16string(u"tebi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<tebi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Ti");}
    static std::u32string long_name()  {return std::u32string(U"tebi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<tebi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Ti");}
    static std::wstring long_name()  {return std::wstring(L"tebi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<pebi, CharT> :
    ratio_detail::ratio_string_static<pebi,CharT>
{};

#else
template <>
struct ratio_string<pebi, char>
{
    static std::string short_name() {return std::string("Pi");}
    static std::string long_name()  {return std::string("pebi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<pebi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Pi");}
    static std::u16string long_name()  {return std::u16string(u"pebi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<pebi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Pi");}
    static std::u32string long_name()  {return std::u32string(U"pebi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<pebi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Pi");}
    static std::wstring long_name()  {return std::wstring(L"pebi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<exbi, CharT> :
    ratio_detail::ratio_string_static<exbi,CharT>
{};

#else
template <>
struct ratio_string<exbi, char>
{
    static std::string short_name() {return std::string("Ei");}
    static std::string long_name()  {return std::string("exbi");}
    static std::string symbol() {return short_name();}
    static std::string prefix() {return long_name();}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<exbi, char16_t>
{
    static std::u16string short_name() {return std::u16string(u"Ei");}
    static std::u16string long_name()  {return std::u16string(u"exbi");}
    static std::u16string symbol() {return short_name();}
    static std::u16string prefix() {return long_name();}
};

template <>
struct ratio_string<exbi, char32_t>
{
    static std::u32string short_name() {return std::u32string(U"Ei");}
    static std::u32string long_name()  {return std::u32string(U"exbi");}
    static std::u32string symbol() {return short_name();}
    static std::u32string prefix() {return long_name();}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<exbi, wchar_t>
{
    static std::wstring short_name() {return std::wstring(L"Ei");}
    static std::wstring long_name()  {return std::wstring(L"exbi");}
    static std::wstring symbol() {return short_name();}
    static std::wstring prefix() {return long_name();}
};
#endif
#endif
#endif
}

#endif  // BOOST_RATIO_RATIO_IO_HPP
