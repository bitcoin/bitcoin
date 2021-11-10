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

#ifndef BOOST_RATIO_RATIO_IO_HPP
#define BOOST_RATIO_RATIO_IO_HPP

/*

    ratio_io synopsis

#include <ratio>
#include <string>

namespace boost
{

template <class Ratio, class CharT>
struct ratio_string
{
    static basic_string<CharT> prefix();
    static basic_string<CharT> symbol();
};

}  // boost

*/
#include <boost/ratio/config.hpp>

#ifdef BOOST_RATIO_PROVIDES_DEPRECATED_FEATURES_SINCE_V2_0_0
#include <boost/ratio/detail/ratio_io.hpp>
#else

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
    static std::basic_string<CharT> symbol() {return prefix();}
    static std::basic_string<CharT> prefix();
};

template <class Ratio, class CharT>
std::basic_string<CharT>
ratio_string<Ratio, CharT>::prefix()
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
    static std::string symbol() {
        return std::basic_string<CharT>(
                static_string::c_str<
                        typename ratio_static_string<Ratio, CharT>::symbol
                    >::value);
    }
    static std::string prefix()  {
        return std::basic_string<CharT>(
                static_string::c_str<
                    typename ratio_static_string<Ratio, CharT>::prefix
                >::value);
    }
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
    static std::string symbol() {return std::string(1, 'a');}
    static std::string prefix()  {return std::string("atto");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<atto, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'a');}
    static std::u16string prefix()  {return std::u16string(u"atto");}
};

template <>
struct ratio_string<atto, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'a');}
    static std::u32string prefix()  {return std::u32string(U"atto");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<atto, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'a');}
    static std::wstring prefix()  {return std::wstring(L"atto");}
};
#endif
#endif

// femto

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<femto, CharT> :
    ratio_detail::ratio_string_static<femto,CharT>
{};

#else

template <>
struct ratio_string<femto, char>
{
    static std::string symbol() {return std::string(1, 'f');}
    static std::string prefix()  {return std::string("femto");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<femto, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'f');}
    static std::u16string prefix()  {return std::u16string(u"femto");}
};

template <>
struct ratio_string<femto, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'f');}
    static std::u32string prefix()  {return std::u32string(U"femto");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<femto, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'f');}
    static std::wstring prefix()  {return std::wstring(L"femto");}
};
#endif
#endif

// pico

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<pico, CharT> :
    ratio_detail::ratio_string_static<pico,CharT>
{};

#else
template <>
struct ratio_string<pico, char>
{
    static std::string symbol() {return std::string(1, 'p');}
    static std::string prefix()  {return std::string("pico");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<pico, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'p');}
    static std::u16string prefix()  {return std::u16string(u"pico");}
};

template <>
struct ratio_string<pico, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'p');}
    static std::u32string prefix()  {return std::u32string(U"pico");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<pico, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'p');}
    static std::wstring prefix()  {return std::wstring(L"pico");}
};
#endif
#endif

// nano

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<nano, CharT> :
    ratio_detail::ratio_string_static<nano,CharT>
{};

#else
template <>
struct ratio_string<nano, char>
{
    static std::string symbol() {return std::string(1, 'n');}
    static std::string prefix()  {return std::string("nano");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<nano, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'n');}
    static std::u16string prefix()  {return std::u16string(u"nano");}
};

template <>
struct ratio_string<nano, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'n');}
    static std::u32string prefix()  {return std::u32string(U"nano");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<nano, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'n');}
    static std::wstring prefix()  {return std::wstring(L"nano");}
};
#endif
#endif

// micro

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<micro, CharT> :
    ratio_detail::ratio_string_static<micro,CharT>
{};

#else
template <>
struct ratio_string<micro, char>
{
    static std::string symbol() {return std::string("\xC2\xB5");}
    static std::string prefix()  {return std::string("micro");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<micro, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'\xB5');}
    static std::u16string prefix()  {return std::u16string(u"micro");}
};

template <>
struct ratio_string<micro, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'\xB5');}
    static std::u32string prefix()  {return std::u32string(U"micro");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<micro, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'\xB5');}
    static std::wstring prefix()  {return std::wstring(L"micro");}
};
#endif
#endif

// milli

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<milli, CharT> :
    ratio_detail::ratio_string_static<milli,CharT>
{};

#else
template <>
struct ratio_string<milli, char>
{
    static std::string symbol() {return std::string(1, 'm');}
    static std::string prefix()  {return std::string("milli");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<milli, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'm');}
    static std::u16string prefix()  {return std::u16string(u"milli");}
};

template <>
struct ratio_string<milli, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'm');}
    static std::u32string prefix()  {return std::u32string(U"milli");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<milli, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'm');}
    static std::wstring prefix()  {return std::wstring(L"milli");}
};
#endif
#endif

// centi

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<centi, CharT> :
    ratio_detail::ratio_string_static<centi,CharT>
{};

#else
template <>
struct ratio_string<centi, char>
{
    static std::string symbol() {return std::string(1, 'c');}
    static std::string prefix()  {return std::string("centi");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<centi, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'c');}
    static std::u16string prefix()  {return std::u16string(u"centi");}
};

template <>
struct ratio_string<centi, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'c');}
    static std::u32string prefix()  {return std::u32string(U"centi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<centi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'c');}
    static std::wstring prefix()  {return std::wstring(L"centi");}
};
#endif
#endif

// deci

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<deci, CharT> :
    ratio_detail::ratio_string_static<deci,CharT>
{};

#else

template <>
struct ratio_string<deci, char>
{
    static std::string symbol() {return std::string(1, 'd');}
    static std::string prefix()  {return std::string("deci");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<deci, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'd');}
    static std::u16string prefix()  {return std::u16string(u"deci");}
};

template <>
struct ratio_string<deci, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'd');}
    static std::u32string prefix()  {return std::u32string(U"deci");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<deci, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'd');}
    static std::wstring prefix()  {return std::wstring(L"deci");}
};
#endif
#endif

// unit

// deca


#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<deca, CharT> :
    ratio_detail::ratio_string_static<deca,CharT>
{};

#else
template <>
struct ratio_string<deca, char>
{
    static std::string symbol() {return std::string("da");}
    static std::string prefix()  {return std::string("deca");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<deca, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"da");}
    static std::u16string prefix()  {return std::u16string(u"deca");}
};

template <>
struct ratio_string<deca, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"da");}
    static std::u32string prefix()  {return std::u32string(U"deca");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<deca, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"da");}
    static std::wstring prefix()  {return std::wstring(L"deca");}
};
#endif
#endif

// hecto

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<hecto, CharT> :
    ratio_detail::ratio_string_static<hecto,CharT>
{};

#else
template <>
struct ratio_string<hecto, char>
{
    static std::string symbol() {return std::string(1, 'h');}
    static std::string prefix()  {return std::string("hecto");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<hecto, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'h');}
    static std::u16string prefix()  {return std::u16string(u"hecto");}
};

template <>
struct ratio_string<hecto, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'h');}
    static std::u32string prefix()  {return std::u32string(U"hecto");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<hecto, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'h');}
    static std::wstring prefix()  {return std::wstring(L"hecto");}
};
#endif
#endif

// kilo

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<kilo, CharT> :
    ratio_detail::ratio_string_static<kilo,CharT>
{};

#else
template <>
struct ratio_string<kilo, char>
{
    static std::string symbol() {return std::string(1, 'k');}
    static std::string prefix()  {return std::string("kilo");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<kilo, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'k');}
    static std::u16string prefix()  {return std::u16string(u"kilo");}
};

template <>
struct ratio_string<kilo, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'k');}
    static std::u32string prefix()  {return std::u32string(U"kilo");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<kilo, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'k');}
    static std::wstring prefix()  {return std::wstring(L"kilo");}
};
#endif
#endif

// mega

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<mega, CharT> :
    ratio_detail::ratio_string_static<mega,CharT>
{};

#else

template <>
struct ratio_string<mega, char>
{
    static std::string symbol() {return std::string(1, 'M');}
    static std::string prefix()  {return std::string("mega");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<mega, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'M');}
    static std::u16string prefix()  {return std::u16string(u"mega");}
};

template <>
struct ratio_string<mega, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'M');}
    static std::u32string prefix()  {return std::u32string(U"mega");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<mega, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'M');}
    static std::wstring prefix()  {return std::wstring(L"mega");}
};
#endif
#endif

// giga

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<giga, CharT> :
    ratio_detail::ratio_string_static<giga,CharT>
{};

#else

template <>
struct ratio_string<giga, char>
{
    static std::string symbol() {return std::string(1, 'G');}
    static std::string prefix()  {return std::string("giga");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<giga, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'G');}
    static std::u16string prefix()  {return std::u16string(u"giga");}
};

template <>
struct ratio_string<giga, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'G');}
    static std::u32string prefix()  {return std::u32string(U"giga");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<giga, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'G');}
    static std::wstring prefix()  {return std::wstring(L"giga");}
};
#endif
#endif

// tera

//template <>
#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<tera, CharT> :
    ratio_detail::ratio_string_static<tera,CharT>
{};

#else
template <>
struct ratio_string<tera, char>
{
    static std::string symbol() {return std::string(1, 'T');}
    static std::string prefix()  {return std::string("tera");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<tera, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'T');}
    static std::u16string prefix()  {return std::u16string(u"tera");}
};

template <>
struct ratio_string<tera, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'T');}
    static std::u32string prefix()  {return std::u32string(U"tera");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<tera, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'T');}
    static std::wstring prefix()  {return std::wstring(L"tera");}
};
#endif
#endif

// peta

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<peta, CharT> :
    ratio_detail::ratio_string_static<peta,CharT>
{};

#else
template <>
struct ratio_string<peta, char>
{
    static std::string symbol() {return std::string(1, 'P');}
    static std::string prefix()  {return std::string("peta");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<peta, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'P');}
    static std::u16string prefix()  {return std::u16string(u"peta");}
};

template <>
struct ratio_string<peta, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'P');}
    static std::u32string prefix()  {return std::u32string(U"peta");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<peta, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'P');}
    static std::wstring prefix()  {return std::wstring(L"peta");}
};
#endif
#endif

// exa

#ifdef BOOST_RATIO_HAS_STATIC_STRING
template <typename CharT>
struct ratio_string<exa, CharT> :
    ratio_detail::ratio_string_static<exa,CharT>
{};

#else
template <>
struct ratio_string<exa, char>
{
    static std::string symbol() {return std::string(1, 'E');}
    static std::string prefix()  {return std::string("exa");}
};

#if defined BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<exa, char16_t>
{
    static std::u16string symbol() {return std::u16string(1, u'E');}
    static std::u16string prefix()  {return std::u16string(u"exa");}
};

template <>
struct ratio_string<exa, char32_t>
{
    static std::u32string symbol() {return std::u32string(1, U'E');}
    static std::u32string prefix()  {return std::u32string(U"exa");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<exa, wchar_t>
{
    static std::wstring symbol() {return std::wstring(1, L'E');}
    static std::wstring prefix()  {return std::wstring(L"exa");}
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
    static std::string symbol() {return std::string("Ki");}
    static std::string prefix()  {return std::string("kibi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<kibi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Ki");}
    static std::u16string prefix()  {return std::u16string(u"kibi");}
};

template <>
struct ratio_string<kibi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Ki");}
    static std::u32string prefix()  {return std::u32string(U"kibi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<kibi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Ki");}
    static std::wstring prefix()  {return std::wstring(L"kibi");}
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
    static std::string symbol() {return std::string("Mi");}
    static std::string prefix()  {return std::string("mebi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<mebi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Mi");}
    static std::u16string prefix()  {return std::u16string(u"mebi");}
};

template <>
struct ratio_string<mebi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Mi");}
    static std::u32string prefix()  {return std::u32string(U"mebi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<mebi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Mi");}
    static std::wstring prefix()  {return std::wstring(L"mebi");}
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
    static std::string symbol() {return std::string("Gi");}
    static std::string prefix()  {return std::string("gibi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<gibi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Gi");}
    static std::u16string prefix()  {return std::u16string(u"gibi");}
};

template <>
struct ratio_string<gibi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Gi");}
    static std::u32string prefix()  {return std::u32string(U"gibi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<gibi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Gi");}
    static std::wstring prefix()  {return std::wstring(L"gibi");}
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
    static std::string symbol() {return std::string("Ti");}
    static std::string prefix()  {return std::string("tebi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<tebi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Ti");}
    static std::u16string prefix()  {return std::u16string(u"tebi");}
};

template <>
struct ratio_string<tebi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Ti");}
    static std::u32string prefix()  {return std::u32string(U"tebi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<tebi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Ti");}
    static std::wstring prefix()  {return std::wstring(L"tebi");}
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
    static std::string symbol() {return std::string("Pi");}
    static std::string prefix()  {return std::string("pebi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<pebi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Pi");}
    static std::u16string prefix()  {return std::u16string(u"pebi");}
};

template <>
struct ratio_string<pebi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Pi");}
    static std::u32string prefix()  {return std::u32string(U"pebi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<pebi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Pi");}
    static std::wstring prefix()  {return std::wstring(L"pebi");}
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
    static std::string symbol() {return std::string("Ei");}
    static std::string prefix()  {return std::string("exbi");}
};

#if BOOST_RATIO_HAS_UNICODE_SUPPORT

template <>
struct ratio_string<exbi, char16_t>
{
    static std::u16string symbol() {return std::u16string(u"Ei");}
    static std::u16string prefix()  {return std::u16string(u"exbi");}
};

template <>
struct ratio_string<exbi, char32_t>
{
    static std::u32string symbol() {return std::u32string(U"Ei");}
    static std::u32string prefix()  {return std::u32string(U"exbi");}
};

#endif

#ifndef BOOST_NO_STD_WSTRING
template <>
struct ratio_string<exbi, wchar_t>
{
    static std::wstring symbol() {return std::wstring(L"Ei");}
    static std::wstring prefix()  {return std::wstring(L"exbi");}
};
#endif
#endif
#endif

}

#endif  // BOOST_RATIO_PROVIDES_DEPRECATED_FEATURES_SINCE_V2_0_0
#endif  // BOOST_RATIO_RATIO_IO_HPP
