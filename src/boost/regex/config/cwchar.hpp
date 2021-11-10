/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         boost/regex/config/cwchar.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex wide character string fixes.
  */

#ifndef BOOST_REGEX_CONFIG_CWCHAR_HPP
#define BOOST_REGEX_CONFIG_CWCHAR_HPP

#include <cwchar>
#include <cwctype>
#include <boost/config.hpp>

#if defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)
// apparently this is required for the RW STL on Linux:
#undef iswalnum
#undef iswalpha
#undef iswblank
#undef iswcntrl
#undef iswdigit
#undef iswgraph
#undef iswlower
#undef iswprint
#undef iswprint
#undef iswpunct
#undef iswspace
#undef iswupper
#undef iswxdigit
#undef iswctype
#undef towlower
#undef towupper
#undef towctrans
#undef wctrans
#undef wctype
#endif

namespace std{

#ifndef BOOST_NO_STDC_NAMESPACE
extern "C"{
#endif

#ifdef iswalnum
inline int (iswalnum)(wint_t i)
{ return iswalnum(i); }
#undef iswalnum
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswalnum;
#endif

#ifdef iswalpha
inline int (iswalpha)(wint_t i)
{ return iswalpha(i); }
#undef iswalpha
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswalpha;
#endif

#ifdef iswcntrl
inline int (iswcntrl)(wint_t i)
{ return iswcntrl(i); }
#undef iswcntrl
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswcntrl;
#endif

#ifdef iswdigit
inline int (iswdigit)(wint_t i)
{ return iswdigit(i); }
#undef iswdigit
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswdigit;
#endif

#ifdef iswgraph
inline int (iswgraph)(wint_t i)
{ return iswgraph(i); }
#undef iswgraph
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswgraph;
#endif

#ifdef iswlower
inline int (iswlower)(wint_t i)
{ return iswlower(i); }
#undef iswlower
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswlower;
#endif

#ifdef iswprint
inline int (iswprint)(wint_t i)
{ return iswprint(i); }
#undef iswprint
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswprint;
#endif

#ifdef iswpunct
inline int (iswpunct)(wint_t i)
{ return iswpunct(i); }
#undef iswpunct
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswpunct;
#endif

#ifdef iswspace
inline int (iswspace)(wint_t i)
{ return iswspace(i); }
#undef iswspace
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswspace;
#endif

#ifdef iswupper
inline int (iswupper)(wint_t i)
{ return iswupper(i); }
#undef iswupper
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswupper;
#endif

#ifdef iswxdigit
inline int (iswxdigit)(wint_t i)
{ return iswxdigit(i); }
#undef iswxdigit
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::iswxdigit;
#endif

#ifdef towlower
inline wint_t (towlower)(wint_t i)
{ return towlower(i); }
#undef towlower
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::towlower;
#endif

#ifdef towupper
inline wint_t (towupper)(wint_t i)
{ return towupper(i); }
#undef towupper
#elif defined(BOOST_NO_STDC_NAMESPACE)
using :: towupper;
#endif

#ifdef wcscmp
inline int (wcscmp)(const wchar_t *p1, const wchar_t *p2)
{ return wcscmp(p1,p2); }
#undef wcscmp
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::wcscmp;
#endif

#ifdef wcscoll
inline int (wcscoll)(const wchar_t *p1, const wchar_t *p2)
{ return wcscoll(p1,p2); }
#undef wcscoll
#elif defined(BOOST_NO_STDC_NAMESPACE) && !defined(UNDER_CE)
using ::wcscoll;
#endif

#ifdef wcscpy
inline wchar_t *(wcscpy)(wchar_t *p1, const wchar_t *p2)
{ return wcscpy(p1,p2); }
#undef wcscpy
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::wcscpy;
#endif

#ifdef wcslen
inline size_t (wcslen)(const wchar_t *p)
{ return wcslen(p); }
#undef wcslen
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::wcslen;
#endif

#ifdef wcsxfrm
size_t wcsxfrm(wchar_t *p1, const wchar_t *p2, size_t s)
{ return wcsxfrm(p1,p2,s); }
#undef wcsxfrm
#elif defined(BOOST_NO_STDC_NAMESPACE)
using ::wcsxfrm;
#endif


#ifndef BOOST_NO_STDC_NAMESPACE
} // extern "C"
#endif

} // namespace std

#endif

