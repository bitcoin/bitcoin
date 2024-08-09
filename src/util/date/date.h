#ifndef DATE_H
#define DATE_H

// The MIT License (MIT)
//
// Copyright (c) 2015, 2016, 2017 Howard Hinnant
// Copyright (c) 2016 Adrian Colomitchi
// Copyright (c) 2017 Florian Dang
// Copyright (c) 2017 Paul Thompson
// Copyright (c) 2018, 2019 Tomasz Kamiński
// Copyright (c) 2019 Jiangang Zhuang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Our apologies.  When the previous paragraph was written, lowercase had not yet
// been invented (that would involve another several millennia of evolution).
// We did not mean to shout.

#ifndef HAS_STRING_VIEW
#  if __cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define HAS_STRING_VIEW 1
#  else
#    define HAS_STRING_VIEW 0
#  endif
#endif  // HAS_STRING_VIEW

#include <cassert>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <ios>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <ostream>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#if HAS_STRING_VIEW
# include <string_view>
#endif
#include <utility>
#include <type_traits>

#ifdef __GNUC__
# pragma GCC diagnostic push
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7)
#  pragma GCC diagnostic ignored "-Wpedantic"
# endif
# if __GNUC__ < 5
   // GCC 4.9 Bug 61489 Wrong warning with -Wmissing-field-initializers
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# endif
#endif

#ifdef _MSC_VER
#   pragma warning(push)
// warning C4127: conditional expression is constant
#   pragma warning(disable : 4127)
#endif

namespace date
{

//---------------+
// Configuration |
//---------------+

#ifndef ONLY_C_LOCALE
#  define ONLY_C_LOCALE 0
#endif

#if defined(_MSC_VER) && (!defined(__clang__) || (_MSC_VER < 1910))
// MSVC
#  ifndef _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
#    define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
#  endif
#  if _MSC_VER < 1910
//   before VS2017
#    define CONSTDATA const
#    define CONSTCD11
#    define CONSTCD14
#    define NOEXCEPT _NOEXCEPT
#  else
//   VS2017 and later
#    define CONSTDATA constexpr const
#    define CONSTCD11 constexpr
#    define CONSTCD14 constexpr
#    define NOEXCEPT noexcept
#  endif

#elif defined(__SUNPRO_CC) && __SUNPRO_CC <= 0x5150
// Oracle Developer Studio 12.6 and earlier
#  define CONSTDATA constexpr const
#  define CONSTCD11 constexpr
#  define CONSTCD14
#  define NOEXCEPT noexcept

#elif __cplusplus >= 201402
// C++14
#  define CONSTDATA constexpr const
#  define CONSTCD11 constexpr
#  define CONSTCD14 constexpr
#  define NOEXCEPT noexcept
#else
// C++11
#  define CONSTDATA constexpr const
#  define CONSTCD11 constexpr
#  define CONSTCD14
#  define NOEXCEPT noexcept
#endif

#ifndef HAS_UNCAUGHT_EXCEPTIONS
#  if __cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define HAS_UNCAUGHT_EXCEPTIONS 1
#  else
#    define HAS_UNCAUGHT_EXCEPTIONS 0
#  endif
#endif  // HAS_UNCAUGHT_EXCEPTIONS

#ifndef HAS_VOID_T
#  if __cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define HAS_VOID_T 1
#  else
#    define HAS_VOID_T 0
#  endif
#endif  // HAS_VOID_T

// Protect from Oracle sun macro
#ifdef sun
#  undef sun
#endif

// Work around for a NVCC compiler bug which causes it to fail
// to compile std::ratio_{multiply,divide} when used directly
// in the std::chrono::duration template instantiations below
namespace detail {
template <typename R1, typename R2>
using ratio_multiply = decltype(std::ratio_multiply<R1, R2>{});

template <typename R1, typename R2>
using ratio_divide = decltype(std::ratio_divide<R1, R2>{});
}  // namespace detail

//-----------+
// Interface |
//-----------+

// durations

using days = std::chrono::duration
    <int, detail::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

using weeks = std::chrono::duration
    <int, detail::ratio_multiply<std::ratio<7>, days::period>>;

using years = std::chrono::duration
    <int, detail::ratio_multiply<std::ratio<146097, 400>, days::period>>;

using months = std::chrono::duration
    <int, detail::ratio_divide<years::period, std::ratio<12>>>;

// time_point

template <class Duration>
    using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;

using sys_days    = sys_time<days>;
using sys_seconds = sys_time<std::chrono::seconds>;

struct local_t {};

template <class Duration>
    using local_time = std::chrono::time_point<local_t, Duration>;

using local_seconds = local_time<std::chrono::seconds>;
using local_days    = local_time<days>;

// types

struct last_spec
{
    explicit last_spec() = default;
};

class day;
class month;
class year;

class weekday;
class weekday_indexed;
class weekday_last;

class month_day;
class month_day_last;
class month_weekday;
class month_weekday_last;

class year_month;

class year_month_day;
class year_month_day_last;
class year_month_weekday;
class year_month_weekday_last;

// date composition operators

CONSTCD11 year_month operator/(const year& y, const month& m) NOEXCEPT;
CONSTCD11 year_month operator/(const year& y, int          m) NOEXCEPT;

CONSTCD11 month_day operator/(const day& d, const month& m) NOEXCEPT;
CONSTCD11 month_day operator/(const day& d, int          m) NOEXCEPT;
CONSTCD11 month_day operator/(const month& m, const day& d) NOEXCEPT;
CONSTCD11 month_day operator/(const month& m, int        d) NOEXCEPT;
CONSTCD11 month_day operator/(int          m, const day& d) NOEXCEPT;

CONSTCD11 month_day_last operator/(const month& m, last_spec) NOEXCEPT;
CONSTCD11 month_day_last operator/(int          m, last_spec) NOEXCEPT;
CONSTCD11 month_day_last operator/(last_spec, const month& m) NOEXCEPT;
CONSTCD11 month_day_last operator/(last_spec, int          m) NOEXCEPT;

CONSTCD11 month_weekday operator/(const month& m, const weekday_indexed& wdi) NOEXCEPT;
CONSTCD11 month_weekday operator/(int          m, const weekday_indexed& wdi) NOEXCEPT;
CONSTCD11 month_weekday operator/(const weekday_indexed& wdi, const month& m) NOEXCEPT;
CONSTCD11 month_weekday operator/(const weekday_indexed& wdi, int          m) NOEXCEPT;

CONSTCD11 month_weekday_last operator/(const month& m, const weekday_last& wdl) NOEXCEPT;
CONSTCD11 month_weekday_last operator/(int          m, const weekday_last& wdl) NOEXCEPT;
CONSTCD11 month_weekday_last operator/(const weekday_last& wdl, const month& m) NOEXCEPT;
CONSTCD11 month_weekday_last operator/(const weekday_last& wdl, int          m) NOEXCEPT;

CONSTCD11 year_month_day operator/(const year_month& ym, const day& d) NOEXCEPT;
CONSTCD11 year_month_day operator/(const year_month& ym, int        d) NOEXCEPT;
CONSTCD11 year_month_day operator/(const year& y, const month_day& md) NOEXCEPT;
CONSTCD11 year_month_day operator/(int         y, const month_day& md) NOEXCEPT;
CONSTCD11 year_month_day operator/(const month_day& md, const year& y) NOEXCEPT;
CONSTCD11 year_month_day operator/(const month_day& md, int         y) NOEXCEPT;

CONSTCD11
    year_month_day_last operator/(const year_month& ym,   last_spec) NOEXCEPT;
CONSTCD11
    year_month_day_last operator/(const year& y, const month_day_last& mdl) NOEXCEPT;
CONSTCD11
    year_month_day_last operator/(int         y, const month_day_last& mdl) NOEXCEPT;
CONSTCD11
    year_month_day_last operator/(const month_day_last& mdl, const year& y) NOEXCEPT;
CONSTCD11
    year_month_day_last operator/(const month_day_last& mdl, int         y) NOEXCEPT;

CONSTCD11
year_month_weekday
operator/(const year_month& ym, const weekday_indexed& wdi) NOEXCEPT;

CONSTCD11
year_month_weekday
operator/(const year&        y, const month_weekday&   mwd) NOEXCEPT;

CONSTCD11
year_month_weekday
operator/(int                y, const month_weekday&   mwd) NOEXCEPT;

CONSTCD11
year_month_weekday
operator/(const month_weekday& mwd, const year&          y) NOEXCEPT;

CONSTCD11
year_month_weekday
operator/(const month_weekday& mwd, int                  y) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator/(const year_month& ym, const weekday_last& wdl) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator/(const year& y, const month_weekday_last& mwdl) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator/(int         y, const month_weekday_last& mwdl) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator/(const month_weekday_last& mwdl, const year& y) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator/(const month_weekday_last& mwdl, int         y) NOEXCEPT;

// Detailed interface

// day

class day
{
    unsigned char d_;

public:
    day() = default;
    explicit CONSTCD11 day(unsigned d) NOEXCEPT;

    CONSTCD14 day& operator++()    NOEXCEPT;
    CONSTCD14 day  operator++(int) NOEXCEPT;
    CONSTCD14 day& operator--()    NOEXCEPT;
    CONSTCD14 day  operator--(int) NOEXCEPT;

    CONSTCD14 day& operator+=(const days& d) NOEXCEPT;
    CONSTCD14 day& operator-=(const days& d) NOEXCEPT;

    CONSTCD11 explicit operator unsigned() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const day& x, const day& y) NOEXCEPT;
CONSTCD11 bool operator!=(const day& x, const day& y) NOEXCEPT;
CONSTCD11 bool operator< (const day& x, const day& y) NOEXCEPT;
CONSTCD11 bool operator> (const day& x, const day& y) NOEXCEPT;
CONSTCD11 bool operator<=(const day& x, const day& y) NOEXCEPT;
CONSTCD11 bool operator>=(const day& x, const day& y) NOEXCEPT;

CONSTCD11 day  operator+(const day&  x, const days& y) NOEXCEPT;
CONSTCD11 day  operator+(const days& x, const day&  y) NOEXCEPT;
CONSTCD11 day  operator-(const day&  x, const days& y) NOEXCEPT;
CONSTCD11 days operator-(const day&  x, const day&  y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const day& d);

// month

class month
{
    unsigned char m_;

public:
    month() = default;
    explicit CONSTCD11 month(unsigned m) NOEXCEPT;

    CONSTCD14 month& operator++()    NOEXCEPT;
    CONSTCD14 month  operator++(int) NOEXCEPT;
    CONSTCD14 month& operator--()    NOEXCEPT;
    CONSTCD14 month  operator--(int) NOEXCEPT;

    CONSTCD14 month& operator+=(const months& m) NOEXCEPT;
    CONSTCD14 month& operator-=(const months& m) NOEXCEPT;

    CONSTCD11 explicit operator unsigned() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const month& x, const month& y) NOEXCEPT;
CONSTCD11 bool operator!=(const month& x, const month& y) NOEXCEPT;
CONSTCD11 bool operator< (const month& x, const month& y) NOEXCEPT;
CONSTCD11 bool operator> (const month& x, const month& y) NOEXCEPT;
CONSTCD11 bool operator<=(const month& x, const month& y) NOEXCEPT;
CONSTCD11 bool operator>=(const month& x, const month& y) NOEXCEPT;

CONSTCD14 month  operator+(const month&  x, const months& y) NOEXCEPT;
CONSTCD14 month  operator+(const months& x,  const month& y) NOEXCEPT;
CONSTCD14 month  operator-(const month&  x, const months& y) NOEXCEPT;
CONSTCD14 months operator-(const month&  x,  const month& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month& m);

// year

class year
{
    short y_;

public:
    year() = default;
    explicit CONSTCD11 year(int y) NOEXCEPT;

    CONSTCD14 year& operator++()    NOEXCEPT;
    CONSTCD14 year  operator++(int) NOEXCEPT;
    CONSTCD14 year& operator--()    NOEXCEPT;
    CONSTCD14 year  operator--(int) NOEXCEPT;

    CONSTCD14 year& operator+=(const years& y) NOEXCEPT;
    CONSTCD14 year& operator-=(const years& y) NOEXCEPT;

    CONSTCD11 year operator-() const NOEXCEPT;
    CONSTCD11 year operator+() const NOEXCEPT;

    CONSTCD11 bool is_leap() const NOEXCEPT;

    CONSTCD11 explicit operator int() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;

    static CONSTCD11 year min() NOEXCEPT { return year{-32767}; }
    static CONSTCD11 year max() NOEXCEPT { return year{32767}; }
};

CONSTCD11 bool operator==(const year& x, const year& y) NOEXCEPT;
CONSTCD11 bool operator!=(const year& x, const year& y) NOEXCEPT;
CONSTCD11 bool operator< (const year& x, const year& y) NOEXCEPT;
CONSTCD11 bool operator> (const year& x, const year& y) NOEXCEPT;
CONSTCD11 bool operator<=(const year& x, const year& y) NOEXCEPT;
CONSTCD11 bool operator>=(const year& x, const year& y) NOEXCEPT;

CONSTCD11 year  operator+(const year&  x, const years& y) NOEXCEPT;
CONSTCD11 year  operator+(const years& x, const year&  y) NOEXCEPT;
CONSTCD11 year  operator-(const year&  x, const years& y) NOEXCEPT;
CONSTCD11 years operator-(const year&  x, const year&  y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year& y);

// weekday

class weekday
{
    unsigned char wd_;
public:
    weekday() = default;
    explicit CONSTCD11 weekday(unsigned wd) NOEXCEPT;
    CONSTCD14 weekday(const sys_days& dp) NOEXCEPT;
    CONSTCD14 explicit weekday(const local_days& dp) NOEXCEPT;

    CONSTCD14 weekday& operator++()    NOEXCEPT;
    CONSTCD14 weekday  operator++(int) NOEXCEPT;
    CONSTCD14 weekday& operator--()    NOEXCEPT;
    CONSTCD14 weekday  operator--(int) NOEXCEPT;

    CONSTCD14 weekday& operator+=(const days& d) NOEXCEPT;
    CONSTCD14 weekday& operator-=(const days& d) NOEXCEPT;

    CONSTCD11 bool ok() const NOEXCEPT;

    CONSTCD11 unsigned c_encoding() const NOEXCEPT;
    CONSTCD11 unsigned iso_encoding() const NOEXCEPT;

    CONSTCD11 weekday_indexed operator[](unsigned index) const NOEXCEPT;
    CONSTCD11 weekday_last    operator[](last_spec)      const NOEXCEPT;

private:
    static CONSTCD14 unsigned char weekday_from_days(int z) NOEXCEPT;

    friend CONSTCD11 bool operator==(const weekday& x, const weekday& y) NOEXCEPT;
    friend CONSTCD14 days operator-(const weekday& x, const weekday& y) NOEXCEPT;
    friend CONSTCD14 weekday operator+(const weekday& x, const days& y) NOEXCEPT;
    template<class CharT, class Traits>
        friend std::basic_ostream<CharT, Traits>&
            operator<<(std::basic_ostream<CharT, Traits>& os, const weekday& wd);
    friend class weekday_indexed;
};

CONSTCD11 bool operator==(const weekday& x, const weekday& y) NOEXCEPT;
CONSTCD11 bool operator!=(const weekday& x, const weekday& y) NOEXCEPT;

CONSTCD14 weekday operator+(const weekday& x, const days&    y) NOEXCEPT;
CONSTCD14 weekday operator+(const days&    x, const weekday& y) NOEXCEPT;
CONSTCD14 weekday operator-(const weekday& x, const days&    y) NOEXCEPT;
CONSTCD14 days    operator-(const weekday& x, const weekday& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday& wd);

// weekday_indexed

class weekday_indexed
{
    unsigned char wd_    : 4;
    unsigned char index_ : 4;

public:
    weekday_indexed() = default;
    CONSTCD11 weekday_indexed(const date::weekday& wd, unsigned index) NOEXCEPT;

    CONSTCD11 date::weekday weekday() const NOEXCEPT;
    CONSTCD11 unsigned index() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const weekday_indexed& x, const weekday_indexed& y) NOEXCEPT;
CONSTCD11 bool operator!=(const weekday_indexed& x, const weekday_indexed& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday_indexed& wdi);

// weekday_last

class weekday_last
{
    date::weekday wd_;

public:
    explicit CONSTCD11 weekday_last(const date::weekday& wd) NOEXCEPT;

    CONSTCD11 date::weekday weekday() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const weekday_last& x, const weekday_last& y) NOEXCEPT;
CONSTCD11 bool operator!=(const weekday_last& x, const weekday_last& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday_last& wdl);

namespace detail
{

struct unspecified_month_disambiguator {};

}  // namespace detail

// year_month

class year_month
{
    date::year  y_;
    date::month m_;

public:
    year_month() = default;
    CONSTCD11 year_month(const date::year& y, const date::month& m) NOEXCEPT;

    CONSTCD11 date::year  year()  const NOEXCEPT;
    CONSTCD11 date::month month() const NOEXCEPT;

    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month& operator+=(const months& dm) NOEXCEPT;
    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month& operator-=(const months& dm) NOEXCEPT;
    CONSTCD14 year_month& operator+=(const years& dy) NOEXCEPT;
    CONSTCD14 year_month& operator-=(const years& dy) NOEXCEPT;

    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 bool operator!=(const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 bool operator< (const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 bool operator> (const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 bool operator<=(const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 bool operator>=(const year_month& x, const year_month& y) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month operator+(const year_month& ym, const months& dm) NOEXCEPT;
template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month operator+(const months& dm, const year_month& ym) NOEXCEPT;
template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month operator-(const year_month& ym, const months& dm) NOEXCEPT;

CONSTCD11 months operator-(const year_month& x, const year_month& y) NOEXCEPT;
CONSTCD11 year_month operator+(const year_month& ym, const years& dy) NOEXCEPT;
CONSTCD11 year_month operator+(const years& dy, const year_month& ym) NOEXCEPT;
CONSTCD11 year_month operator-(const year_month& ym, const years& dy) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month& ym);

// month_day

class month_day
{
    date::month m_;
    date::day   d_;

public:
    month_day() = default;
    CONSTCD11 month_day(const date::month& m, const date::day& d) NOEXCEPT;

    CONSTCD11 date::month month() const NOEXCEPT;
    CONSTCD11 date::day   day() const NOEXCEPT;

    CONSTCD14 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const month_day& x, const month_day& y) NOEXCEPT;
CONSTCD11 bool operator!=(const month_day& x, const month_day& y) NOEXCEPT;
CONSTCD11 bool operator< (const month_day& x, const month_day& y) NOEXCEPT;
CONSTCD11 bool operator> (const month_day& x, const month_day& y) NOEXCEPT;
CONSTCD11 bool operator<=(const month_day& x, const month_day& y) NOEXCEPT;
CONSTCD11 bool operator>=(const month_day& x, const month_day& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_day& md);

// month_day_last

class month_day_last
{
    date::month m_;

public:
    CONSTCD11 explicit month_day_last(const date::month& m) NOEXCEPT;

    CONSTCD11 date::month month() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const month_day_last& x, const month_day_last& y) NOEXCEPT;
CONSTCD11 bool operator!=(const month_day_last& x, const month_day_last& y) NOEXCEPT;
CONSTCD11 bool operator< (const month_day_last& x, const month_day_last& y) NOEXCEPT;
CONSTCD11 bool operator> (const month_day_last& x, const month_day_last& y) NOEXCEPT;
CONSTCD11 bool operator<=(const month_day_last& x, const month_day_last& y) NOEXCEPT;
CONSTCD11 bool operator>=(const month_day_last& x, const month_day_last& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_day_last& mdl);

// month_weekday

class month_weekday
{
    date::month           m_;
    date::weekday_indexed wdi_;
public:
    CONSTCD11 month_weekday(const date::month& m,
                            const date::weekday_indexed& wdi) NOEXCEPT;

    CONSTCD11 date::month           month()           const NOEXCEPT;
    CONSTCD11 date::weekday_indexed weekday_indexed() const NOEXCEPT;

    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11 bool operator==(const month_weekday& x, const month_weekday& y) NOEXCEPT;
CONSTCD11 bool operator!=(const month_weekday& x, const month_weekday& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_weekday& mwd);

// month_weekday_last

class month_weekday_last
{
    date::month        m_;
    date::weekday_last wdl_;

public:
    CONSTCD11 month_weekday_last(const date::month& m,
                                 const date::weekday_last& wd) NOEXCEPT;

    CONSTCD11 date::month        month()        const NOEXCEPT;
    CONSTCD11 date::weekday_last weekday_last() const NOEXCEPT;

    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11
    bool operator==(const month_weekday_last& x, const month_weekday_last& y) NOEXCEPT;
CONSTCD11
    bool operator!=(const month_weekday_last& x, const month_weekday_last& y) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_weekday_last& mwdl);

// class year_month_day

class year_month_day
{
    date::year  y_;
    date::month m_;
    date::day   d_;

public:
    year_month_day() = default;
    CONSTCD11 year_month_day(const date::year& y, const date::month& m,
                             const date::day& d) NOEXCEPT;
    CONSTCD14 year_month_day(const year_month_day_last& ymdl) NOEXCEPT;

    CONSTCD14 year_month_day(sys_days dp) NOEXCEPT;
    CONSTCD14 explicit year_month_day(local_days dp) NOEXCEPT;

    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_day& operator+=(const months& m) NOEXCEPT;
    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_day& operator-=(const months& m) NOEXCEPT;
    CONSTCD14 year_month_day& operator+=(const years& y)  NOEXCEPT;
    CONSTCD14 year_month_day& operator-=(const years& y)  NOEXCEPT;

    CONSTCD11 date::year  year()  const NOEXCEPT;
    CONSTCD11 date::month month() const NOEXCEPT;
    CONSTCD11 date::day   day()   const NOEXCEPT;

    CONSTCD14 operator sys_days() const NOEXCEPT;
    CONSTCD14 explicit operator local_days() const NOEXCEPT;
    CONSTCD14 bool ok() const NOEXCEPT;

private:
    static CONSTCD14 year_month_day from_days(days dp) NOEXCEPT;
    CONSTCD14 days to_days() const NOEXCEPT;
};

CONSTCD11 bool operator==(const year_month_day& x, const year_month_day& y) NOEXCEPT;
CONSTCD11 bool operator!=(const year_month_day& x, const year_month_day& y) NOEXCEPT;
CONSTCD11 bool operator< (const year_month_day& x, const year_month_day& y) NOEXCEPT;
CONSTCD11 bool operator> (const year_month_day& x, const year_month_day& y) NOEXCEPT;
CONSTCD11 bool operator<=(const year_month_day& x, const year_month_day& y) NOEXCEPT;
CONSTCD11 bool operator>=(const year_month_day& x, const year_month_day& y) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month_day operator+(const year_month_day& ymd, const months& dm) NOEXCEPT;
template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month_day operator+(const months& dm, const year_month_day& ymd) NOEXCEPT;
template<class = detail::unspecified_month_disambiguator>
CONSTCD14 year_month_day operator-(const year_month_day& ymd, const months& dm) NOEXCEPT;
CONSTCD11 year_month_day operator+(const year_month_day& ymd, const years& dy)  NOEXCEPT;
CONSTCD11 year_month_day operator+(const years& dy, const year_month_day& ymd)  NOEXCEPT;
CONSTCD11 year_month_day operator-(const year_month_day& ymd, const years& dy)  NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_day& ymd);

// year_month_day_last

class year_month_day_last
{
    date::year           y_;
    date::month_day_last mdl_;

public:
    CONSTCD11 year_month_day_last(const date::year& y,
                                  const date::month_day_last& mdl) NOEXCEPT;

    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_day_last& operator+=(const months& m) NOEXCEPT;
    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_day_last& operator-=(const months& m) NOEXCEPT;
    CONSTCD14 year_month_day_last& operator+=(const years& y)  NOEXCEPT;
    CONSTCD14 year_month_day_last& operator-=(const years& y)  NOEXCEPT;

    CONSTCD11 date::year           year()           const NOEXCEPT;
    CONSTCD11 date::month          month()          const NOEXCEPT;
    CONSTCD11 date::month_day_last month_day_last() const NOEXCEPT;
    CONSTCD14 date::day            day()            const NOEXCEPT;

    CONSTCD14 operator sys_days() const NOEXCEPT;
    CONSTCD14 explicit operator local_days() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;
};

CONSTCD11
    bool operator==(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;
CONSTCD11
    bool operator!=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;
CONSTCD11
    bool operator< (const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;
CONSTCD11
    bool operator> (const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;
CONSTCD11
    bool operator<=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;
CONSTCD11
    bool operator>=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_day_last
operator+(const year_month_day_last& ymdl, const months& dm) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_day_last
operator+(const months& dm, const year_month_day_last& ymdl) NOEXCEPT;

CONSTCD11
year_month_day_last
operator+(const year_month_day_last& ymdl, const years& dy) NOEXCEPT;

CONSTCD11
year_month_day_last
operator+(const years& dy, const year_month_day_last& ymdl) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_day_last
operator-(const year_month_day_last& ymdl, const months& dm) NOEXCEPT;

CONSTCD11
year_month_day_last
operator-(const year_month_day_last& ymdl, const years& dy) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_day_last& ymdl);

// year_month_weekday

class year_month_weekday
{
    date::year            y_;
    date::month           m_;
    date::weekday_indexed wdi_;

public:
    year_month_weekday() = default;
    CONSTCD11 year_month_weekday(const date::year& y, const date::month& m,
                                   const date::weekday_indexed& wdi) NOEXCEPT;
    CONSTCD14 year_month_weekday(const sys_days& dp) NOEXCEPT;
    CONSTCD14 explicit year_month_weekday(const local_days& dp) NOEXCEPT;

    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_weekday& operator+=(const months& m) NOEXCEPT;
    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_weekday& operator-=(const months& m) NOEXCEPT;
    CONSTCD14 year_month_weekday& operator+=(const years& y)  NOEXCEPT;
    CONSTCD14 year_month_weekday& operator-=(const years& y)  NOEXCEPT;

    CONSTCD11 date::year year() const NOEXCEPT;
    CONSTCD11 date::month month() const NOEXCEPT;
    CONSTCD11 date::weekday weekday() const NOEXCEPT;
    CONSTCD11 unsigned index() const NOEXCEPT;
    CONSTCD11 date::weekday_indexed weekday_indexed() const NOEXCEPT;

    CONSTCD14 operator sys_days() const NOEXCEPT;
    CONSTCD14 explicit operator local_days() const NOEXCEPT;
    CONSTCD14 bool ok() const NOEXCEPT;

private:
    static CONSTCD14 year_month_weekday from_days(days dp) NOEXCEPT;
    CONSTCD14 days to_days() const NOEXCEPT;
};

CONSTCD11
    bool operator==(const year_month_weekday& x, const year_month_weekday& y) NOEXCEPT;
CONSTCD11
    bool operator!=(const year_month_weekday& x, const year_month_weekday& y) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday
operator+(const year_month_weekday& ymwd, const months& dm) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday
operator+(const months& dm, const year_month_weekday& ymwd) NOEXCEPT;

CONSTCD11
year_month_weekday
operator+(const year_month_weekday& ymwd, const years& dy) NOEXCEPT;

CONSTCD11
year_month_weekday
operator+(const years& dy, const year_month_weekday& ymwd) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday
operator-(const year_month_weekday& ymwd, const months& dm) NOEXCEPT;

CONSTCD11
year_month_weekday
operator-(const year_month_weekday& ymwd, const years& dy) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_weekday& ymwdi);

// year_month_weekday_last

class year_month_weekday_last
{
    date::year y_;
    date::month m_;
    date::weekday_last wdl_;

public:
    CONSTCD11 year_month_weekday_last(const date::year& y, const date::month& m,
                                      const date::weekday_last& wdl) NOEXCEPT;

    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_weekday_last& operator+=(const months& m) NOEXCEPT;
    template<class = detail::unspecified_month_disambiguator>
    CONSTCD14 year_month_weekday_last& operator-=(const months& m) NOEXCEPT;
    CONSTCD14 year_month_weekday_last& operator+=(const years& y) NOEXCEPT;
    CONSTCD14 year_month_weekday_last& operator-=(const years& y) NOEXCEPT;

    CONSTCD11 date::year year() const NOEXCEPT;
    CONSTCD11 date::month month() const NOEXCEPT;
    CONSTCD11 date::weekday weekday() const NOEXCEPT;
    CONSTCD11 date::weekday_last weekday_last() const NOEXCEPT;

    CONSTCD14 operator sys_days() const NOEXCEPT;
    CONSTCD14 explicit operator local_days() const NOEXCEPT;
    CONSTCD11 bool ok() const NOEXCEPT;

private:
    CONSTCD14 days to_days() const NOEXCEPT;
};

CONSTCD11
bool
operator==(const year_month_weekday_last& x, const year_month_weekday_last& y) NOEXCEPT;

CONSTCD11
bool
operator!=(const year_month_weekday_last& x, const year_month_weekday_last& y) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday_last
operator+(const year_month_weekday_last& ymwdl, const months& dm) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday_last
operator+(const months& dm, const year_month_weekday_last& ymwdl) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator+(const year_month_weekday_last& ymwdl, const years& dy) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator+(const years& dy, const year_month_weekday_last& ymwdl) NOEXCEPT;

template<class = detail::unspecified_month_disambiguator>
CONSTCD14
year_month_weekday_last
operator-(const year_month_weekday_last& ymwdl, const months& dm) NOEXCEPT;

CONSTCD11
year_month_weekday_last
operator-(const year_month_weekday_last& ymwdl, const years& dy) NOEXCEPT;

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_weekday_last& ymwdl);

#if !defined(_MSC_VER) || (_MSC_VER >= 1900)
inline namespace literals
{

CONSTCD11 date::day  operator "" _d(unsigned long long d) NOEXCEPT;
CONSTCD11 date::year operator "" _y(unsigned long long y) NOEXCEPT;

}  // inline namespace literals
#endif // !defined(_MSC_VER) || (_MSC_VER >= 1900)

// CONSTDATA date::month January{1};
// CONSTDATA date::month February{2};
// CONSTDATA date::month March{3};
// CONSTDATA date::month April{4};
// CONSTDATA date::month May{5};
// CONSTDATA date::month June{6};
// CONSTDATA date::month July{7};
// CONSTDATA date::month August{8};
// CONSTDATA date::month September{9};
// CONSTDATA date::month October{10};
// CONSTDATA date::month November{11};
// CONSTDATA date::month December{12};
//
// CONSTDATA date::weekday Sunday{0u};
// CONSTDATA date::weekday Monday{1u};
// CONSTDATA date::weekday Tuesday{2u};
// CONSTDATA date::weekday Wednesday{3u};
// CONSTDATA date::weekday Thursday{4u};
// CONSTDATA date::weekday Friday{5u};
// CONSTDATA date::weekday Saturday{6u};

#if HAS_VOID_T

template <class T, class = std::void_t<>>
struct is_clock
    : std::false_type
{};

template <class T>
struct is_clock<T, std::void_t<decltype(T::now()), typename T::rep, typename T::period,
                               typename T::duration, typename T::time_point,
                               decltype(T::is_steady)>>
    : std::true_type
{};

template<class T> inline constexpr bool is_clock_v = is_clock<T>::value;

#endif  // HAS_VOID_T

//----------------+
// Implementation |
//----------------+

// utilities
namespace detail {

template<class CharT, class Traits = std::char_traits<CharT>>
class save_istream
{
protected:
    std::basic_ios<CharT, Traits>& is_;
    CharT fill_;
    std::ios::fmtflags flags_;
    std::streamsize precision_;
    std::streamsize width_;
    std::basic_ostream<CharT, Traits>* tie_;
    std::locale loc_;

public:
    ~save_istream()
    {
        is_.fill(fill_);
        is_.flags(flags_);
        is_.precision(precision_);
        is_.width(width_);
        is_.imbue(loc_);
        is_.tie(tie_);
    }

    save_istream(const save_istream&) = delete;
    save_istream& operator=(const save_istream&) = delete;

    explicit save_istream(std::basic_ios<CharT, Traits>& is)
        : is_(is)
        , fill_(is.fill())
        , flags_(is.flags())
        , precision_(is.precision())
        , width_(is.width(0))
        , tie_(is.tie(nullptr))
        , loc_(is.getloc())
        {
            if (tie_ != nullptr)
                tie_->flush();
        }
};

template<class CharT, class Traits = std::char_traits<CharT>>
class save_ostream
    : private save_istream<CharT, Traits>
{
public:
    ~save_ostream()
    {
        if ((this->flags_ & std::ios::unitbuf) &&
#if HAS_UNCAUGHT_EXCEPTIONS
                std::uncaught_exceptions() == 0 &&
#else
                !std::uncaught_exception() &&
#endif
                this->is_.good())
            this->is_.rdbuf()->pubsync();
    }

    save_ostream(const save_ostream&) = delete;
    save_ostream& operator=(const save_ostream&) = delete;

    explicit save_ostream(std::basic_ios<CharT, Traits>& os)
        : save_istream<CharT, Traits>(os)
        {
        }
};

template <class T>
struct choose_trunc_type
{
    static const int digits = std::numeric_limits<T>::digits;
    using type = typename std::conditional
                 <
                     digits < 32,
                     std::int32_t,
                     typename std::conditional
                     <
                         digits < 64,
                         std::int64_t,
#ifdef __SIZEOF_INT128__
                         __int128
#else
                         std::int64_t
#endif
                     >::type
                 >::type;
};

template <class T>
CONSTCD11
inline
typename std::enable_if
<
    !std::chrono::treat_as_floating_point<T>::value,
    T
>::type
trunc(T t) NOEXCEPT
{
    return t;
}

template <class T>
CONSTCD14
inline
typename std::enable_if
<
    std::chrono::treat_as_floating_point<T>::value,
    T
>::type
trunc(T t) NOEXCEPT
{
    using std::numeric_limits;
    using I = typename choose_trunc_type<T>::type;
    CONSTDATA auto digits = numeric_limits<T>::digits;
    static_assert(digits < numeric_limits<I>::digits, "");
    CONSTDATA auto max = I{1} << (digits-1);
    CONSTDATA auto min = -max;
    const auto negative = t < T{0};
    if (min <= t && t <= max && t != 0 && t == t)
    {
        t = static_cast<T>(static_cast<I>(t));
        if (t == 0 && negative)
            t = -t;
    }
    return t;
}

template <std::intmax_t Xp, std::intmax_t Yp>
struct static_gcd
{
    static const std::intmax_t value = static_gcd<Yp, Xp % Yp>::value;
};

template <std::intmax_t Xp>
struct static_gcd<Xp, 0>
{
    static const std::intmax_t value = Xp;
};

template <>
struct static_gcd<0, 0>
{
    static const std::intmax_t value = 1;
};

template <class R1, class R2>
struct no_overflow
{
private:
    static const std::intmax_t gcd_n1_n2 = static_gcd<R1::num, R2::num>::value;
    static const std::intmax_t gcd_d1_d2 = static_gcd<R1::den, R2::den>::value;
    static const std::intmax_t n1 = R1::num / gcd_n1_n2;
    static const std::intmax_t d1 = R1::den / gcd_d1_d2;
    static const std::intmax_t n2 = R2::num / gcd_n1_n2;
    static const std::intmax_t d2 = R2::den / gcd_d1_d2;
#ifdef __cpp_constexpr
    static const std::intmax_t max = std::numeric_limits<std::intmax_t>::max();
#else
    static const std::intmax_t max = LLONG_MAX;
#endif

    template <std::intmax_t Xp, std::intmax_t Yp, bool overflow>
    struct mul    // overflow == false
    {
        static const std::intmax_t value = Xp * Yp;
    };

    template <std::intmax_t Xp, std::intmax_t Yp>
    struct mul<Xp, Yp, true>
    {
        static const std::intmax_t value = 1;
    };

public:
    static const bool value = (n1 <= max / d2) && (n2 <= max / d1);
    typedef std::ratio<mul<n1, d2, !value>::value,
                       mul<n2, d1, !value>::value> type;
};

}  // detail

// trunc towards zero
template <class To, class Rep, class Period>
CONSTCD11
inline
typename std::enable_if
<
    detail::no_overflow<Period, typename To::period>::value,
    To
>::type
trunc(const std::chrono::duration<Rep, Period>& d)
{
    return To{detail::trunc(std::chrono::duration_cast<To>(d).count())};
}

template <class To, class Rep, class Period>
CONSTCD11
inline
typename std::enable_if
<
    !detail::no_overflow<Period, typename To::period>::value,
    To
>::type
trunc(const std::chrono::duration<Rep, Period>& d)
{
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using rep = typename std::common_type<Rep, typename To::rep>::type;
    return To{detail::trunc(duration_cast<To>(duration_cast<duration<rep>>(d)).count())};
}

#ifndef HAS_CHRONO_ROUNDING
#  if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 190023918 || (_MSC_FULL_VER >= 190000000 && defined (__clang__)))
#    define HAS_CHRONO_ROUNDING 1
#  elif defined(__cpp_lib_chrono) && __cplusplus > 201402 && __cpp_lib_chrono >= 201510
#    define HAS_CHRONO_ROUNDING 1
#  elif defined(_LIBCPP_VERSION) && __cplusplus > 201402 && _LIBCPP_VERSION >= 3800
#    define HAS_CHRONO_ROUNDING 1
#  else
#    define HAS_CHRONO_ROUNDING 0
#  endif
#endif  // HAS_CHRONO_ROUNDING

#if HAS_CHRONO_ROUNDING == 0

// round down
template <class To, class Rep, class Period>
CONSTCD14
inline
typename std::enable_if
<
    detail::no_overflow<Period, typename To::period>::value,
    To
>::type
floor(const std::chrono::duration<Rep, Period>& d)
{
    auto t = trunc<To>(d);
    if (t > d)
        return t - To{1};
    return t;
}

template <class To, class Rep, class Period>
CONSTCD14
inline
typename std::enable_if
<
    !detail::no_overflow<Period, typename To::period>::value,
    To
>::type
floor(const std::chrono::duration<Rep, Period>& d)
{
    using rep = typename std::common_type<Rep, typename To::rep>::type;
    return floor<To>(floor<std::chrono::duration<rep>>(d));
}

// round to nearest, to even on tie
template <class To, class Rep, class Period>
CONSTCD14
inline
To
round(const std::chrono::duration<Rep, Period>& d)
{
    auto t0 = floor<To>(d);
    auto t1 = t0 + To{1};
    if (t1 == To{0} && t0 < To{0})
        t1 = -t1;
    auto diff0 = d - t0;
    auto diff1 = t1 - d;
    if (diff0 == diff1)
    {
        if (t0 - trunc<To>(t0/2)*2 == To{0})
            return t0;
        return t1;
    }
    if (diff0 < diff1)
        return t0;
    return t1;
}

// round up
template <class To, class Rep, class Period>
CONSTCD14
inline
To
ceil(const std::chrono::duration<Rep, Period>& d)
{
    auto t = trunc<To>(d);
    if (t < d)
        return t + To{1};
    return t;
}

template <class Rep, class Period,
          class = typename std::enable_if
          <
              std::numeric_limits<Rep>::is_signed
          >::type>
CONSTCD11
std::chrono::duration<Rep, Period>
abs(std::chrono::duration<Rep, Period> d)
{
    return d >= d.zero() ? d : static_cast<decltype(d)>(-d);
}

// round down
template <class To, class Clock, class FromDuration>
CONSTCD11
inline
std::chrono::time_point<Clock, To>
floor(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{date::floor<To>(tp.time_since_epoch())};
}

// round to nearest, to even on tie
template <class To, class Clock, class FromDuration>
CONSTCD11
inline
std::chrono::time_point<Clock, To>
round(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{round<To>(tp.time_since_epoch())};
}

// round up
template <class To, class Clock, class FromDuration>
CONSTCD11
inline
std::chrono::time_point<Clock, To>
ceil(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{ceil<To>(tp.time_since_epoch())};
}

#else  // HAS_CHRONO_ROUNDING == 1

using std::chrono::floor;
using std::chrono::ceil;
using std::chrono::round;
using std::chrono::abs;

#endif  // HAS_CHRONO_ROUNDING

namespace detail
{

template <class To, class Rep, class Period>
CONSTCD14
inline
typename std::enable_if
<
    !std::chrono::treat_as_floating_point<typename To::rep>::value,
    To
>::type
round_i(const std::chrono::duration<Rep, Period>& d)
{
    return round<To>(d);
}

template <class To, class Rep, class Period>
CONSTCD14
inline
typename std::enable_if
<
    std::chrono::treat_as_floating_point<typename To::rep>::value,
    To
>::type
round_i(const std::chrono::duration<Rep, Period>& d)
{
    return d;
}

template <class To, class Clock, class FromDuration>
CONSTCD11
inline
std::chrono::time_point<Clock, To>
round_i(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{round_i<To>(tp.time_since_epoch())};
}

}  // detail

// trunc towards zero
template <class To, class Clock, class FromDuration>
CONSTCD11
inline
std::chrono::time_point<Clock, To>
trunc(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{trunc<To>(tp.time_since_epoch())};
}

// day

CONSTCD11 inline day::day(unsigned d) NOEXCEPT : d_(static_cast<decltype(d_)>(d)) {}
CONSTCD14 inline day& day::operator++() NOEXCEPT {++d_; return *this;}
CONSTCD14 inline day day::operator++(int) NOEXCEPT {auto tmp(*this); ++(*this); return tmp;}
CONSTCD14 inline day& day::operator--() NOEXCEPT {--d_; return *this;}
CONSTCD14 inline day day::operator--(int) NOEXCEPT {auto tmp(*this); --(*this); return tmp;}
CONSTCD14 inline day& day::operator+=(const days& d) NOEXCEPT {*this = *this + d; return *this;}
CONSTCD14 inline day& day::operator-=(const days& d) NOEXCEPT {*this = *this - d; return *this;}
CONSTCD11 inline day::operator unsigned() const NOEXCEPT {return d_;}
CONSTCD11 inline bool day::ok() const NOEXCEPT {return 1 <= d_ && d_ <= 31;}

CONSTCD11
inline
bool
operator==(const day& x, const day& y) NOEXCEPT
{
    return static_cast<unsigned>(x) == static_cast<unsigned>(y);
}

CONSTCD11
inline
bool
operator!=(const day& x, const day& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const day& x, const day& y) NOEXCEPT
{
    return static_cast<unsigned>(x) < static_cast<unsigned>(y);
}

CONSTCD11
inline
bool
operator>(const day& x, const day& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const day& x, const day& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const day& x, const day& y) NOEXCEPT
{
    return !(x < y);
}

CONSTCD11
inline
days
operator-(const day& x, const day& y) NOEXCEPT
{
    return days{static_cast<days::rep>(static_cast<unsigned>(x)
                                     - static_cast<unsigned>(y))};
}

CONSTCD11
inline
day
operator+(const day& x, const days& y) NOEXCEPT
{
    return day{static_cast<unsigned>(x) + static_cast<unsigned>(y.count())};
}

CONSTCD11
inline
day
operator+(const days& x, const day& y) NOEXCEPT
{
    return y + x;
}

CONSTCD11
inline
day
operator-(const day& x, const days& y) NOEXCEPT
{
    return x + -y;
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const day& d)
{
    detail::save_ostream<CharT, Traits> _(os);
    os.fill('0');
    os.flags(std::ios::dec | std::ios::right);
    os.width(2);
    os << static_cast<unsigned>(d);
    return os;
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const day& d)
{
    detail::low_level_fmt(os, d);
    if (!d.ok())
        os << " is not a valid day";
    return os;
}

// month

CONSTCD11 inline month::month(unsigned m) NOEXCEPT : m_(static_cast<decltype(m_)>(m)) {}
CONSTCD14 inline month& month::operator++() NOEXCEPT {*this += months{1}; return *this;}
CONSTCD14 inline month month::operator++(int) NOEXCEPT {auto tmp(*this); ++(*this); return tmp;}
CONSTCD14 inline month& month::operator--() NOEXCEPT {*this -= months{1}; return *this;}
CONSTCD14 inline month month::operator--(int) NOEXCEPT {auto tmp(*this); --(*this); return tmp;}

CONSTCD14
inline
month&
month::operator+=(const months& m) NOEXCEPT
{
    *this = *this + m;
    return *this;
}

CONSTCD14
inline
month&
month::operator-=(const months& m) NOEXCEPT
{
    *this = *this - m;
    return *this;
}

CONSTCD11 inline month::operator unsigned() const NOEXCEPT {return m_;}
CONSTCD11 inline bool month::ok() const NOEXCEPT {return 1 <= m_ && m_ <= 12;}

CONSTCD11
inline
bool
operator==(const month& x, const month& y) NOEXCEPT
{
    return static_cast<unsigned>(x) == static_cast<unsigned>(y);
}

CONSTCD11
inline
bool
operator!=(const month& x, const month& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const month& x, const month& y) NOEXCEPT
{
    return static_cast<unsigned>(x) < static_cast<unsigned>(y);
}

CONSTCD11
inline
bool
operator>(const month& x, const month& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const month& x, const month& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const month& x, const month& y) NOEXCEPT
{
    return !(x < y);
}

CONSTCD14
inline
months
operator-(const month& x, const month& y) NOEXCEPT
{
    auto const d = static_cast<unsigned>(x) - static_cast<unsigned>(y);
    return months(d <= 11 ? d : d + 12);
}

CONSTCD14
inline
month
operator+(const month& x, const months& y) NOEXCEPT
{
    auto const mu = static_cast<long long>(static_cast<unsigned>(x)) + y.count() - 1;
    auto const yr = (mu >= 0 ? mu : mu-11) / 12;
    return month{static_cast<unsigned>(mu - yr * 12 + 1)};
}

CONSTCD14
inline
month
operator+(const months& x, const month& y) NOEXCEPT
{
    return y + x;
}

CONSTCD14
inline
month
operator-(const month& x, const months& y) NOEXCEPT
{
    return x + -y;
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const month& m)
{
    if (m.ok())
    {
        CharT fmt[] = {'%', 'b', 0};
        os << format(os.getloc(), fmt, m);
    }
    else
        os << static_cast<unsigned>(m);
    return os;
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month& m)
{
    detail::low_level_fmt(os, m);
    if (!m.ok())
        os << " is not a valid month";
    return os;
}

// year

CONSTCD11 inline year::year(int y) NOEXCEPT : y_(static_cast<decltype(y_)>(y)) {}
CONSTCD14 inline year& year::operator++() NOEXCEPT {++y_; return *this;}
CONSTCD14 inline year year::operator++(int) NOEXCEPT {auto tmp(*this); ++(*this); return tmp;}
CONSTCD14 inline year& year::operator--() NOEXCEPT {--y_; return *this;}
CONSTCD14 inline year year::operator--(int) NOEXCEPT {auto tmp(*this); --(*this); return tmp;}
CONSTCD14 inline year& year::operator+=(const years& y) NOEXCEPT {*this = *this + y; return *this;}
CONSTCD14 inline year& year::operator-=(const years& y) NOEXCEPT {*this = *this - y; return *this;}
CONSTCD11 inline year year::operator-() const NOEXCEPT {return year{-y_};}
CONSTCD11 inline year year::operator+() const NOEXCEPT {return *this;}

CONSTCD11
inline
bool
year::is_leap() const NOEXCEPT
{
    return y_ % 4 == 0 && (y_ % 100 != 0 || y_ % 400 == 0);
}

CONSTCD11 inline year::operator int() const NOEXCEPT {return y_;}

CONSTCD11
inline
bool
year::ok() const NOEXCEPT
{
    return y_ != std::numeric_limits<short>::min();
}

CONSTCD11
inline
bool
operator==(const year& x, const year& y) NOEXCEPT
{
    return static_cast<int>(x) == static_cast<int>(y);
}

CONSTCD11
inline
bool
operator!=(const year& x, const year& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const year& x, const year& y) NOEXCEPT
{
    return static_cast<int>(x) < static_cast<int>(y);
}

CONSTCD11
inline
bool
operator>(const year& x, const year& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const year& x, const year& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const year& x, const year& y) NOEXCEPT
{
    return !(x < y);
}

CONSTCD11
inline
years
operator-(const year& x, const year& y) NOEXCEPT
{
    return years{static_cast<int>(x) - static_cast<int>(y)};
}

CONSTCD11
inline
year
operator+(const year& x, const years& y) NOEXCEPT
{
    return year{static_cast<int>(x) + y.count()};
}

CONSTCD11
inline
year
operator+(const years& x, const year& y) NOEXCEPT
{
    return y + x;
}

CONSTCD11
inline
year
operator-(const year& x, const years& y) NOEXCEPT
{
    return year{static_cast<int>(x) - y.count()};
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const year& y)
{
    detail::save_ostream<CharT, Traits> _(os);
    os.fill('0');
    os.flags(std::ios::dec | std::ios::internal);
    os.width(4 + (y < year{0}));
    os.imbue(std::locale::classic());
    os << static_cast<int>(y);
    return os;
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year& y)
{
    detail::low_level_fmt(os, y);
    if (!y.ok())
        os << " is not a valid year";
    return os;
}

// weekday

CONSTCD14
inline
unsigned char
weekday::weekday_from_days(int z) NOEXCEPT
{
    auto u = static_cast<unsigned>(z);
    return static_cast<unsigned char>(z >= -4 ? (u+4) % 7 : u % 7);
}

CONSTCD11
inline
weekday::weekday(unsigned wd) NOEXCEPT
    : wd_(static_cast<decltype(wd_)>(wd != 7 ? wd : 0))
    {}

CONSTCD14
inline
weekday::weekday(const sys_days& dp) NOEXCEPT
    : wd_(weekday_from_days(dp.time_since_epoch().count()))
    {}

CONSTCD14
inline
weekday::weekday(const local_days& dp) NOEXCEPT
    : wd_(weekday_from_days(dp.time_since_epoch().count()))
    {}

CONSTCD14 inline weekday& weekday::operator++() NOEXCEPT {*this += days{1}; return *this;}
CONSTCD14 inline weekday weekday::operator++(int) NOEXCEPT {auto tmp(*this); ++(*this); return tmp;}
CONSTCD14 inline weekday& weekday::operator--() NOEXCEPT {*this -= days{1}; return *this;}
CONSTCD14 inline weekday weekday::operator--(int) NOEXCEPT {auto tmp(*this); --(*this); return tmp;}

CONSTCD14
inline
weekday&
weekday::operator+=(const days& d) NOEXCEPT
{
    *this = *this + d;
    return *this;
}

CONSTCD14
inline
weekday&
weekday::operator-=(const days& d) NOEXCEPT
{
    *this = *this - d;
    return *this;
}

CONSTCD11 inline bool weekday::ok() const NOEXCEPT {return wd_ <= 6;}

CONSTCD11
inline
unsigned weekday::c_encoding() const NOEXCEPT
{
    return unsigned{wd_};
}

CONSTCD11
inline
unsigned weekday::iso_encoding() const NOEXCEPT
{
    return unsigned{((wd_ == 0u) ? 7u : wd_)};
}

CONSTCD11
inline
bool
operator==(const weekday& x, const weekday& y) NOEXCEPT
{
    return x.wd_ == y.wd_;
}

CONSTCD11
inline
bool
operator!=(const weekday& x, const weekday& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD14
inline
days
operator-(const weekday& x, const weekday& y) NOEXCEPT
{
    auto const wdu = x.wd_ - y.wd_;
    auto const wk = (wdu >= 0 ? wdu : wdu-6) / 7;
    return days{wdu - wk * 7};
}

CONSTCD14
inline
weekday
operator+(const weekday& x, const days& y) NOEXCEPT
{
    auto const wdu = static_cast<long long>(static_cast<unsigned>(x.wd_)) + y.count();
    auto const wk = (wdu >= 0 ? wdu : wdu-6) / 7;
    return weekday{static_cast<unsigned>(wdu - wk * 7)};
}

CONSTCD14
inline
weekday
operator+(const days& x, const weekday& y) NOEXCEPT
{
    return y + x;
}

CONSTCD14
inline
weekday
operator-(const weekday& x, const days& y) NOEXCEPT
{
    return x + -y;
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const weekday& wd)
{
    if (wd.ok())
    {
        CharT fmt[] = {'%', 'a', 0};
        os << format(fmt, wd);
    }
    else
        os << wd.c_encoding();
    return os;
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday& wd)
{
    detail::low_level_fmt(os, wd);
    if (!wd.ok())
        os << " is not a valid weekday";
    return os;
}

#if !defined(_MSC_VER) || (_MSC_VER >= 1900)
inline namespace literals
{

CONSTCD11
inline
date::day
operator "" _d(unsigned long long d) NOEXCEPT
{
    return date::day{static_cast<unsigned>(d)};
}

CONSTCD11
inline
date::year
operator "" _y(unsigned long long y) NOEXCEPT
{
    return date::year(static_cast<int>(y));
}
#endif  // !defined(_MSC_VER) || (_MSC_VER >= 1900)

CONSTDATA date::last_spec last{};

CONSTDATA date::month jan{1};
CONSTDATA date::month feb{2};
CONSTDATA date::month mar{3};
CONSTDATA date::month apr{4};
CONSTDATA date::month may{5};
CONSTDATA date::month jun{6};
CONSTDATA date::month jul{7};
CONSTDATA date::month aug{8};
CONSTDATA date::month sep{9};
CONSTDATA date::month oct{10};
CONSTDATA date::month nov{11};
CONSTDATA date::month dec{12};

CONSTDATA date::weekday sun{0u};
CONSTDATA date::weekday mon{1u};
CONSTDATA date::weekday tue{2u};
CONSTDATA date::weekday wed{3u};
CONSTDATA date::weekday thu{4u};
CONSTDATA date::weekday fri{5u};
CONSTDATA date::weekday sat{6u};

#if !defined(_MSC_VER) || (_MSC_VER >= 1900)
}  // inline namespace literals
#endif

CONSTDATA date::month January{1};
CONSTDATA date::month February{2};
CONSTDATA date::month March{3};
CONSTDATA date::month April{4};
CONSTDATA date::month May{5};
CONSTDATA date::month June{6};
CONSTDATA date::month July{7};
CONSTDATA date::month August{8};
CONSTDATA date::month September{9};
CONSTDATA date::month October{10};
CONSTDATA date::month November{11};
CONSTDATA date::month December{12};

CONSTDATA date::weekday Monday{1};
CONSTDATA date::weekday Tuesday{2};
CONSTDATA date::weekday Wednesday{3};
CONSTDATA date::weekday Thursday{4};
CONSTDATA date::weekday Friday{5};
CONSTDATA date::weekday Saturday{6};
CONSTDATA date::weekday Sunday{7};

// weekday_indexed

CONSTCD11
inline
weekday
weekday_indexed::weekday() const NOEXCEPT
{
    return date::weekday{static_cast<unsigned>(wd_)};
}

CONSTCD11 inline unsigned weekday_indexed::index() const NOEXCEPT {return index_;}

CONSTCD11
inline
bool
weekday_indexed::ok() const NOEXCEPT
{
    return weekday().ok() && 1 <= index_ && index_ <= 5;
}

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#endif  // __GNUC__

CONSTCD11
inline
weekday_indexed::weekday_indexed(const date::weekday& wd, unsigned index) NOEXCEPT
    : wd_(static_cast<decltype(wd_)>(static_cast<unsigned>(wd.wd_)))
    , index_(static_cast<decltype(index_)>(index))
    {}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif  // __GNUC__

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const weekday_indexed& wdi)
{
    return low_level_fmt(os, wdi.weekday()) << '[' << wdi.index() << ']';
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday_indexed& wdi)
{
    detail::low_level_fmt(os, wdi);
    if (!wdi.ok())
        os << " is not a valid weekday_indexed";
    return os;
}

CONSTCD11
inline
weekday_indexed
weekday::operator[](unsigned index) const NOEXCEPT
{
    return {*this, index};
}

CONSTCD11
inline
bool
operator==(const weekday_indexed& x, const weekday_indexed& y) NOEXCEPT
{
    return x.weekday() == y.weekday() && x.index() == y.index();
}

CONSTCD11
inline
bool
operator!=(const weekday_indexed& x, const weekday_indexed& y) NOEXCEPT
{
    return !(x == y);
}

// weekday_last

CONSTCD11 inline date::weekday weekday_last::weekday() const NOEXCEPT {return wd_;}
CONSTCD11 inline bool weekday_last::ok() const NOEXCEPT {return wd_.ok();}
CONSTCD11 inline weekday_last::weekday_last(const date::weekday& wd) NOEXCEPT : wd_(wd) {}

CONSTCD11
inline
bool
operator==(const weekday_last& x, const weekday_last& y) NOEXCEPT
{
    return x.weekday() == y.weekday();
}

CONSTCD11
inline
bool
operator!=(const weekday_last& x, const weekday_last& y) NOEXCEPT
{
    return !(x == y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const weekday_last& wdl)
{
    return low_level_fmt(os, wdl.weekday()) << "[last]";
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const weekday_last& wdl)
{
    detail::low_level_fmt(os, wdl);
    if (!wdl.ok())
        os << " is not a valid weekday_last";
    return os;
}

CONSTCD11
inline
weekday_last
weekday::operator[](last_spec) const NOEXCEPT
{
    return weekday_last{*this};
}

// year_month

CONSTCD11
inline
year_month::year_month(const date::year& y, const date::month& m) NOEXCEPT
    : y_(y)
    , m_(m)
    {}

CONSTCD11 inline year year_month::year() const NOEXCEPT {return y_;}
CONSTCD11 inline month year_month::month() const NOEXCEPT {return m_;}
CONSTCD11 inline bool year_month::ok() const NOEXCEPT {return y_.ok() && m_.ok();}

template<class>
CONSTCD14
inline
year_month&
year_month::operator+=(const months& dm) NOEXCEPT
{
    *this = *this + dm;
    return *this;
}

template<class>
CONSTCD14
inline
year_month&
year_month::operator-=(const months& dm) NOEXCEPT
{
    *this = *this - dm;
    return *this;
}

CONSTCD14
inline
year_month&
year_month::operator+=(const years& dy) NOEXCEPT
{
    *this = *this + dy;
    return *this;
}

CONSTCD14
inline
year_month&
year_month::operator-=(const years& dy) NOEXCEPT
{
    *this = *this - dy;
    return *this;
}

CONSTCD11
inline
bool
operator==(const year_month& x, const year_month& y) NOEXCEPT
{
    return x.year() == y.year() && x.month() == y.month();
}

CONSTCD11
inline
bool
operator!=(const year_month& x, const year_month& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const year_month& x, const year_month& y) NOEXCEPT
{
    return x.year() < y.year() ? true
        : (x.year() > y.year() ? false
        : (x.month() < y.month()));
}

CONSTCD11
inline
bool
operator>(const year_month& x, const year_month& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const year_month& x, const year_month& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const year_month& x, const year_month& y) NOEXCEPT
{
    return !(x < y);
}

template<class>
CONSTCD14
inline
year_month
operator+(const year_month& ym, const months& dm) NOEXCEPT
{
    auto dmi = static_cast<int>(static_cast<unsigned>(ym.month())) - 1 + dm.count();
    auto dy = (dmi >= 0 ? dmi : dmi-11) / 12;
    dmi = dmi - dy * 12 + 1;
    return (ym.year() + years(dy)) / month(static_cast<unsigned>(dmi));
}

template<class>
CONSTCD14
inline
year_month
operator+(const months& dm, const year_month& ym) NOEXCEPT
{
    return ym + dm;
}

template<class>
CONSTCD14
inline
year_month
operator-(const year_month& ym, const months& dm) NOEXCEPT
{
    return ym + -dm;
}

CONSTCD11
inline
months
operator-(const year_month& x, const year_month& y) NOEXCEPT
{
    return (x.year() - y.year()) +
            months(static_cast<unsigned>(x.month()) - static_cast<unsigned>(y.month()));
}

CONSTCD11
inline
year_month
operator+(const year_month& ym, const years& dy) NOEXCEPT
{
    return (ym.year() + dy) / ym.month();
}

CONSTCD11
inline
year_month
operator+(const years& dy, const year_month& ym) NOEXCEPT
{
    return ym + dy;
}

CONSTCD11
inline
year_month
operator-(const year_month& ym, const years& dy) NOEXCEPT
{
    return ym + -dy;
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const year_month& ym)
{
    low_level_fmt(os, ym.year()) << '/';
    return low_level_fmt(os, ym.month());
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month& ym)
{
    detail::low_level_fmt(os, ym);
    if (!ym.ok())
        os << " is not a valid year_month";
    return os;
}

// month_day

CONSTCD11
inline
month_day::month_day(const date::month& m, const date::day& d) NOEXCEPT
    : m_(m)
    , d_(d)
    {}

CONSTCD11 inline date::month month_day::month() const NOEXCEPT {return m_;}
CONSTCD11 inline date::day month_day::day() const NOEXCEPT {return d_;}

CONSTCD14
inline
bool
month_day::ok() const NOEXCEPT
{
    CONSTDATA date::day d[] =
    {
        date::day(31), date::day(29), date::day(31),
        date::day(30), date::day(31), date::day(30),
        date::day(31), date::day(31), date::day(30),
        date::day(31), date::day(30), date::day(31)
    };
    return m_.ok() && date::day{1} <= d_ && d_ <= d[static_cast<unsigned>(m_)-1];
}

CONSTCD11
inline
bool
operator==(const month_day& x, const month_day& y) NOEXCEPT
{
    return x.month() == y.month() && x.day() == y.day();
}

CONSTCD11
inline
bool
operator!=(const month_day& x, const month_day& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const month_day& x, const month_day& y) NOEXCEPT
{
    return x.month() < y.month() ? true
        : (x.month() > y.month() ? false
        : (x.day() < y.day()));
}

CONSTCD11
inline
bool
operator>(const month_day& x, const month_day& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const month_day& x, const month_day& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const month_day& x, const month_day& y) NOEXCEPT
{
    return !(x < y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const month_day& md)
{
    low_level_fmt(os, md.month()) << '/';
    return low_level_fmt(os, md.day());
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_day& md)
{
    detail::low_level_fmt(os, md);
    if (!md.ok())
        os << " is not a valid month_day";
    return os;
}

// month_day_last

CONSTCD11 inline month month_day_last::month() const NOEXCEPT {return m_;}
CONSTCD11 inline bool month_day_last::ok() const NOEXCEPT {return m_.ok();}
CONSTCD11 inline month_day_last::month_day_last(const date::month& m) NOEXCEPT : m_(m) {}

CONSTCD11
inline
bool
operator==(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return x.month() == y.month();
}

CONSTCD11
inline
bool
operator!=(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return x.month() < y.month();
}

CONSTCD11
inline
bool
operator>(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const month_day_last& x, const month_day_last& y) NOEXCEPT
{
    return !(x < y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const month_day_last& mdl)
{
    return low_level_fmt(os, mdl.month()) << "/last";
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_day_last& mdl)
{
    detail::low_level_fmt(os, mdl);
    if (!mdl.ok())
        os << " is not a valid month_day_last";
    return os;
}

// month_weekday

CONSTCD11
inline
month_weekday::month_weekday(const date::month& m,
                             const date::weekday_indexed& wdi) NOEXCEPT
    : m_(m)
    , wdi_(wdi)
    {}

CONSTCD11 inline month month_weekday::month() const NOEXCEPT {return m_;}

CONSTCD11
inline
weekday_indexed
month_weekday::weekday_indexed() const NOEXCEPT
{
    return wdi_;
}

CONSTCD11
inline
bool
month_weekday::ok() const NOEXCEPT
{
    return m_.ok() && wdi_.ok();
}

CONSTCD11
inline
bool
operator==(const month_weekday& x, const month_weekday& y) NOEXCEPT
{
    return x.month() == y.month() && x.weekday_indexed() == y.weekday_indexed();
}

CONSTCD11
inline
bool
operator!=(const month_weekday& x, const month_weekday& y) NOEXCEPT
{
    return !(x == y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const month_weekday& mwd)
{
    low_level_fmt(os, mwd.month()) << '/';
    return low_level_fmt(os, mwd.weekday_indexed());
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_weekday& mwd)
{
    detail::low_level_fmt(os, mwd);
    if (!mwd.ok())
        os << " is not a valid month_weekday";
    return os;
}

// month_weekday_last

CONSTCD11
inline
month_weekday_last::month_weekday_last(const date::month& m,
                                       const date::weekday_last& wdl) NOEXCEPT
    : m_(m)
    , wdl_(wdl)
    {}

CONSTCD11 inline month month_weekday_last::month() const NOEXCEPT {return m_;}

CONSTCD11
inline
weekday_last
month_weekday_last::weekday_last() const NOEXCEPT
{
    return wdl_;
}

CONSTCD11
inline
bool
month_weekday_last::ok() const NOEXCEPT
{
    return m_.ok() && wdl_.ok();
}

CONSTCD11
inline
bool
operator==(const month_weekday_last& x, const month_weekday_last& y) NOEXCEPT
{
    return x.month() == y.month() && x.weekday_last() == y.weekday_last();
}

CONSTCD11
inline
bool
operator!=(const month_weekday_last& x, const month_weekday_last& y) NOEXCEPT
{
    return !(x == y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const month_weekday_last& mwdl)
{
    low_level_fmt(os, mwdl.month()) << '/';
    return low_level_fmt(os, mwdl.weekday_last());
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const month_weekday_last& mwdl)
{
    detail::low_level_fmt(os, mwdl);
    if (!mwdl.ok())
        os << " is not a valid month_weekday_last";
    return os;
}

// year_month_day_last

CONSTCD11
inline
year_month_day_last::year_month_day_last(const date::year& y,
                                         const date::month_day_last& mdl) NOEXCEPT
    : y_(y)
    , mdl_(mdl)
    {}

template<class>
CONSTCD14
inline
year_month_day_last&
year_month_day_last::operator+=(const months& m) NOEXCEPT
{
    *this = *this + m;
    return *this;
}

template<class>
CONSTCD14
inline
year_month_day_last&
year_month_day_last::operator-=(const months& m) NOEXCEPT
{
    *this = *this - m;
    return *this;
}

CONSTCD14
inline
year_month_day_last&
year_month_day_last::operator+=(const years& y) NOEXCEPT
{
    *this = *this + y;
    return *this;
}

CONSTCD14
inline
year_month_day_last&
year_month_day_last::operator-=(const years& y) NOEXCEPT
{
    *this = *this - y;
    return *this;
}

CONSTCD11 inline year year_month_day_last::year() const NOEXCEPT {return y_;}
CONSTCD11 inline month year_month_day_last::month() const NOEXCEPT {return mdl_.month();}

CONSTCD11
inline
month_day_last
year_month_day_last::month_day_last() const NOEXCEPT
{
    return mdl_;
}

CONSTCD14
inline
day
year_month_day_last::day() const NOEXCEPT
{
    CONSTDATA date::day d[] =
    {
        date::day(31), date::day(28), date::day(31),
        date::day(30), date::day(31), date::day(30),
        date::day(31), date::day(31), date::day(30),
        date::day(31), date::day(30), date::day(31)
    };
    return (month() != February || !y_.is_leap()) && mdl_.ok() ?
        d[static_cast<unsigned>(month()) - 1] : date::day{29};
}

CONSTCD14
inline
year_month_day_last::operator sys_days() const NOEXCEPT
{
    return sys_days(year()/month()/day());
}

CONSTCD14
inline
year_month_day_last::operator local_days() const NOEXCEPT
{
    return local_days(year()/month()/day());
}

CONSTCD11
inline
bool
year_month_day_last::ok() const NOEXCEPT
{
    return y_.ok() && mdl_.ok();
}

CONSTCD11
inline
bool
operator==(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return x.year() == y.year() && x.month_day_last() == y.month_day_last();
}

CONSTCD11
inline
bool
operator!=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return x.year() < y.year() ? true
        : (x.year() > y.year() ? false
        : (x.month_day_last() < y.month_day_last()));
}

CONSTCD11
inline
bool
operator>(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const year_month_day_last& x, const year_month_day_last& y) NOEXCEPT
{
    return !(x < y);
}

namespace detail
{

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
low_level_fmt(std::basic_ostream<CharT, Traits>& os, const year_month_day_last& ymdl)
{
    low_level_fmt(os, ymdl.year()) << '/';
    return low_level_fmt(os, ymdl.month_day_last());
}

}  // namespace detail

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_day_last& ymdl)
{
    detail::low_level_fmt(os, ymdl);
    if (!ymdl.ok())
        os << " is not a valid year_month_day_last";
    return os;
}

template<class>
CONSTCD14
inline
year_month_day_last
operator+(const year_month_day_last& ymdl, const months& dm) NOEXCEPT
{
    return (ymdl.year() / ymdl.month() + dm) / last;
}

template<class>
CONSTCD14
inline
year_month_day_last
operator+(const months& dm, const year_month_day_last& ymdl) NOEXCEPT
{
    return ymdl + dm;
}

template<class>
CONSTCD14
inline
year_month_day_last
operator-(const year_month_day_last& ymdl, const months& dm) NOEXCEPT
{
    return ymdl + (-dm);
}

CONSTCD11
inline
year_month_day_last
operator+(const year_month_day_last& ymdl, const years& dy) NOEXCEPT
{
    return {ymdl.year()+dy, ymdl.month_day_last()};
}

CONSTCD11
inline
year_month_day_last
operator+(const years& dy, const year_month_day_last& ymdl) NOEXCEPT
{
    return ymdl + dy;
}

CONSTCD11
inline
year_month_day_last
operator-(const year_month_day_last& ymdl, const years& dy) NOEXCEPT
{
    return ymdl + (-dy);
}

// year_month_day

CONSTCD11
inline
year_month_day::year_month_day(const date::year& y, const date::month& m,
                               const date::day& d) NOEXCEPT
    : y_(y)
    , m_(m)
    , d_(d)
    {}

CONSTCD14
inline
year_month_day::year_month_day(const year_month_day_last& ymdl) NOEXCEPT
    : y_(ymdl.year())
    , m_(ymdl.month())
    , d_(ymdl.day())
    {}

CONSTCD14
inline
year_month_day::year_month_day(sys_days dp) NOEXCEPT
    : year_month_day(from_days(dp.time_since_epoch()))
    {}

CONSTCD14
inline
year_month_day::year_month_day(local_days dp) NOEXCEPT
    : year_month_day(from_days(dp.time_since_epoch()))
    {}

CONSTCD11 inline year year_month_day::year() const NOEXCEPT {return y_;}
CONSTCD11 inline month year_month_day::month() const NOEXCEPT {return m_;}
CONSTCD11 inline day year_month_day::day() const NOEXCEPT {return d_;}

template<class>
CONSTCD14
inline
year_month_day&
year_month_day::operator+=(const months& m) NOEXCEPT
{
    *this = *this + m;
    return *this;
}

template<class>
CONSTCD14
inline
year_month_day&
year_month_day::operator-=(const months& m) NOEXCEPT
{
    *this = *this - m;
    return *this;
}

CONSTCD14
inline
year_month_day&
year_month_day::operator+=(const years& y) NOEXCEPT
{
    *this = *this + y;
    return *this;
}

CONSTCD14
inline
year_month_day&
year_month_day::operator-=(const years& y) NOEXCEPT
{
    *this = *this - y;
    return *this;
}

CONSTCD14
inline
days
year_month_day::to_days() const NOEXCEPT
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    auto const y = static_cast<int>(y_) - (m_ <= February);
    auto const m = static_cast<unsigned>(m_);
    auto const d = static_cast<unsigned>(d_);
    auto const era = (y >= 0 ? y : y-399) / 400;
    auto const yoe = static_cast<unsigned>(y - era * 400);       // [0, 399]
    auto const doy = (153*(m > 2 ? m-3 : m+9) + 2)/5 + d-1;      // [0, 365]
    auto const doe = yoe * 365 + yoe/4 - yoe/100 + doy;          // [0, 146096]
    return days{era * 146097 + static_cast<int>(doe) - 719468};
}

CONSTCD14
inline
year_month_day::operator sys_days() const NOEXCEPT
{
    return sys_days{to_days()};
}

CONSTCD14
inline
year_month_day::operator local_days() const NOEXCEPT
{
    return local_days{to_days()};
}

CONSTCD14
inline
bool
year_month_day::ok() const NOEXCEPT
{
    if (!(y_.ok() && m_.ok()))
        return false;
    return date::day{1} <= d_ && d_ <= (y_ / m_ / last).day();
}

CONSTCD11
inline
bool
operator==(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return x.year() == y.year() && x.month() == y.month() && x.day() == y.day();
}

CONSTCD11
inline
bool
operator!=(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return !(x == y);
}

CONSTCD11
inline
bool
operator<(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return x.year() < y.year() ? true
        : (x.year() > y.year() ? false
        : (x.month() < y.month() ? true
        : (x.month() > y.month() ? false
        : (x.day() < y.day()))));
}

CONSTCD11
inline
bool
operator>(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return y < x;
}

CONSTCD11
inline
bool
operator<=(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return !(y < x);
}

CONSTCD11
inline
bool
operator>=(const year_month_day& x, const year_month_day& y) NOEXCEPT
{
    return !(x < y);
}

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_day& ymd)
{
    detail::save_ostream<CharT, Traits> _(os);
    os.fill('0');
    os.flags(std::ios::dec | std::ios::right);
    os.imbue(std::locale::classic());
    os << static_cast<int>(ymd.year()) << '-';
    os.width(2);
    os << static_cast<unsigned>(ymd.month()) << '-';
    os.width(2);
    os << static_cast<unsigned>(ymd.day());
    if (!ymd.ok())
        os << " is not a valid year_month_day";
    return os;
}

CONSTCD14
inline
year_month_day
year_month_day::from_days(days dp) NOEXCEPT
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    auto const z = dp.count() + 719468;
    auto const era = (z >= 0 ? z : z - 146096) / 146097;
    auto const doe = static_cast<unsigned>(z - era * 146097);          // [0, 146096]
    auto const yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;  // [0, 399]
    auto const y = static_cast<days::rep>(yoe) + era * 400;
    auto const doy = doe - (365*yoe + yoe/4 - yoe/100);                // [0, 365]
    auto const mp = (5*doy + 2)/153;                                   // [0, 11]
    auto const d = doy - (153*mp+2)/5 + 1;                             // [1, 31]
    auto const m = mp < 10 ? mp+3 : mp-9;                              // [1, 12]
    return year_month_day{date::year{y + (m <= 2)}, date::month(m), date::day(d)};
}

template<class>
CONSTCD14
inline
year_month_day
operator+(const year_month_day& ymd, const months& dm) NOEXCEPT
{
    return (ymd.year() / ymd.month() + dm) / ymd.day();
}

template<class>
CONSTCD14
inline
year_month_day
operator+(const months& dm, const year_month_day& ymd) NOEXCEPT
{
    return ymd + dm;
}

template<class>
CONSTCD14
inline
year_month_day
operator-(const year_month_day& ymd, const months& dm) NOEXCEPT
{
    return ymd + (-dm);
}

CONSTCD11
inline
year_month_day
operator+(const year_month_day& ymd, const years& dy) NOEXCEPT
{
    return (ymd.year() + dy) / ymd.month() / ymd.day();
}

CONSTCD11
inline
year_month_day
operator+(const years& dy, const year_month_day& ymd) NOEXCEPT
{
    return ymd + dy;
}

CONSTCD11
inline
year_month_day
operator-(const year_month_day& ymd, const years& dy) NOEXCEPT
{
    return ymd + (-dy);
}

// year_month_weekday

CONSTCD11
inline
year_month_weekday::year_month_weekday(const date::year& y, const date::month& m,
                                       const date::weekday_indexed& wdi)
        NOEXCEPT
    : y_(y)
    , m_(m)
    , wdi_(wdi)
    {}

CONSTCD14
inline
year_month_weekday::year_month_weekday(const sys_days& dp) NOEXCEPT
    : year_month_weekday(from_days(dp.time_since_epoch()))
    {}

CONSTCD14
inline
year_month_weekday::year_month_weekday(const local_days& dp) NOEXCEPT
    : year_month_weekday(from_days(dp.time_since_epoch()))
    {}

template<class>
CONSTCD14
inline
year_month_weekday&
year_month_weekday::operator+=(const months& m) NOEXCEPT
{
    *this = *this + m;
    return *this;
}

template<class>
CONSTCD14
inline
year_month_weekday&
year_month_weekday::operator-=(const months& m) NOEXCEPT
{
    *this = *this - m;
    return *this;
}

CONSTCD14
inline
year_month_weekday&
year_month_weekday::operator+=(const years& y) NOEXCEPT
{
    *this = *this + y;
    return *this;
}

CONSTCD14
inline
year_month_weekday&
year_month_weekday::operator-=(const years& y) NOEXCEPT
{
    *this = *this - y;
    return *this;
}

CONSTCD11 inline year year_month_weekday::year() const NOEXCEPT {return y_;}
CONSTCD11 inline month year_month_weekday::month() const NOEXCEPT {return m_;}

CONSTCD11
inline
weekday
year_month_weekday::weekday() const NOEXCEPT
{
    return wdi_.weekday();
}

CONSTCD11
inline
unsigned
year_month_weekday::index() const NOEXCEPT
{
    return wdi_.index();
}

CONSTCD11
inline
weekday_indexed
year_month_weekday::weekday_indexed() const NOEXCEPT
{
    return wdi_;
}

CONSTCD14
inline
year_month_weekday::operator sys_days() const NOEXCEPT
{
    return sys_days{to_days()};
}

CONSTCD14
inline
year_month_weekday::operator local_days() const NOEXCEPT
{
    return local_days{to_days()};
}

CONSTCD14
inline
bool
year_month_weekday::ok() const NOEXCEPT
{
    if (!y_.ok() || !m_.ok() || !wdi_.weekday().ok() || wdi_.index() < 1)
        return false;
    if (wdi_.index() <= 4)
        return true;
    auto d2 = wdi_.weekday() - date::weekday(static_cast<sys_days>(y_/m_/1)) +
                  days((wdi_.index()-1)*7 + 1);
    return static_cast<unsigned>(d2.count()) <= static_cast<unsigned>((y_/m_/last).day());
}

CONSTCD14
inline
year_month_weekday
year_month_weekday::from_days(days d) NOEXCEPT
{
    sys_days dp{d};
    auto const wd = date::weekday(dp);
    auto const ymd = year_month_day(dp);
    return {ymd.year(), ymd.month(), wd[(static_cast<unsigned>(ymd.day())-1)/7+1]};
}

CONSTCD14
inline
days
year_month_weekday::to_days() const NOEXCEPT
{
    auto d = sys_days(y_/m_/1);
    return (d + (wdi_.weekday() - date::weekday(d) + days{(wdi_.index()-1)*7})
           ).time_since_epoch();
}

CONSTCD11
inline
bool
operator==(const year_month_weekday& x, const year_month_weekday& y) NOEXCEPT
{
    return x.year() == y.year() && x.month() == y.month() &&
           x.weekday_indexed() == y.weekday_indexed();
}

CONSTCD11
inline
bool
operator!=(const year_month_weekday& x, const year_month_weekday& y) NOEXCEPT
{
    return !(x == y);
}

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_weekday& ymwdi)
{
    detail::low_level_fmt(os, ymwdi.year()) << '/';
    detail::low_level_fmt(os, ymwdi.month()) << '/';
    detail::low_level_fmt(os, ymwdi.weekday_indexed());
    if (!ymwdi.ok())
        os << " is not a valid year_month_weekday";
    return os;
}

template<class>
CONSTCD14
inline
year_month_weekday
operator+(const year_month_weekday& ymwd, const months& dm) NOEXCEPT
{
    return (ymwd.year() / ymwd.month() + dm) / ymwd.weekday_indexed();
}

template<class>
CONSTCD14
inline
year_month_weekday
operator+(const months& dm, const year_month_weekday& ymwd) NOEXCEPT
{
    return ymwd + dm;
}

template<class>
CONSTCD14
inline
year_month_weekday
operator-(const year_month_weekday& ymwd, const months& dm) NOEXCEPT
{
    return ymwd + (-dm);
}

CONSTCD11
inline
year_month_weekday
operator+(const year_month_weekday& ymwd, const years& dy) NOEXCEPT
{
    return {ymwd.year()+dy, ymwd.month(), ymwd.weekday_indexed()};
}

CONSTCD11
inline
year_month_weekday
operator+(const years& dy, const year_month_weekday& ymwd) NOEXCEPT
{
    return ymwd + dy;
}

CONSTCD11
inline
year_month_weekday
operator-(const year_month_weekday& ymwd, const years& dy) NOEXCEPT
{
    return ymwd + (-dy);
}

// year_month_weekday_last

CONSTCD11
inline
year_month_weekday_last::year_month_weekday_last(const date::year& y,
                                                 const date::month& m,
                                                 const date::weekday_last& wdl) NOEXCEPT
    : y_(y)
    , m_(m)
    , wdl_(wdl)
    {}

template<class>
CONSTCD14
inline
year_month_weekday_last&
year_month_weekday_last::operator+=(const months& m) NOEXCEPT
{
    *this = *this + m;
    return *this;
}

template<class>
CONSTCD14
inline
year_month_weekday_last&
year_month_weekday_last::operator-=(const months& m) NOEXCEPT
{
    *this = *this - m;
    return *this;
}

CONSTCD14
inline
year_month_weekday_last&
year_month_weekday_last::operator+=(const years& y) NOEXCEPT
{
    *this = *this + y;
    return *this;
}

CONSTCD14
inline
year_month_weekday_last&
year_month_weekday_last::operator-=(const years& y) NOEXCEPT
{
    *this = *this - y;
    return *this;
}

CONSTCD11 inline year year_month_weekday_last::year() const NOEXCEPT {return y_;}
CONSTCD11 inline month year_month_weekday_last::month() const NOEXCEPT {return m_;}

CONSTCD11
inline
weekday
year_month_weekday_last::weekday() const NOEXCEPT
{
    return wdl_.weekday();
}

CONSTCD11
inline
weekday_last
year_month_weekday_last::weekday_last() const NOEXCEPT
{
    return wdl_;
}

CONSTCD14
inline
year_month_weekday_last::operator sys_days() const NOEXCEPT
{
    return sys_days{to_days()};
}

CONSTCD14
inline
year_month_weekday_last::operator local_days() const NOEXCEPT
{
    return local_days{to_days()};
}

CONSTCD11
inline
bool
year_month_weekday_last::ok() const NOEXCEPT
{
    return y_.ok() && m_.ok() && wdl_.ok();
}

CONSTCD14
inline
days
year_month_weekday_last::to_days() const NOEXCEPT
{
    auto const d = sys_days(y_/m_/last);
    return (d - (date::weekday{d} - wdl_.weekday())).time_since_epoch();
}

CONSTCD11
inline
bool
operator==(const year_month_weekday_last& x, const year_month_weekday_last& y) NOEXCEPT
{
    return x.year() == y.year() && x.month() == y.month() &&
           x.weekday_last() == y.weekday_last();
}

CONSTCD11
inline
bool
operator!=(const year_month_weekday_last& x, const year_month_weekday_last& y) NOEXCEPT
{
    return !(x == y);
}

template<class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const year_month_weekday_last& ymwdl)
{
    detail::low_level_fmt(os, ymwdl.year()) << '/';
    detail::low_level_fmt(os, ymwdl.month()) << '/';
    detail::low_level_fmt(os, ymwdl.weekday_last());
    if (!ymwdl.ok())
        os << " is not a valid year_month_weekday_last";
    return os;
}

template<class>
CONSTCD14
inline
year_month_weekday_last
operator+(const year_month_weekday_last& ymwdl, const months& dm) NOEXCEPT
{
    return (ymwdl.year() / ymwdl.month() + dm) / ymwdl.weekday_last();
}

template<class>
CONSTCD14
inline
year_month_weekday_last
operator+(const months& dm, const year_month_weekday_last& ymwdl) NOEXCEPT
{
    return ymwdl + dm;
}

template<class>
CONSTCD14
inline
year_month_weekday_last
operator-(const year_month_weekday_last& ymwdl, const months& dm) NOEXCEPT
{
    return ymwdl + (-dm);
}

CONSTCD11
inline
year_month_weekday_last
operator+(const year_month_weekday_last& ymwdl, const years& dy) NOEXCEPT
{
    return {ymwdl.year()+dy, ymwdl.month(), ymwdl.weekday_last()};
}

CONSTCD11
inline
year_month_weekday_last
operator+(const years& dy, const year_month_weekday_last& ymwdl) NOEXCEPT
{
    return ymwdl + dy;
}

CONSTCD11
inline
year_month_weekday_last
operator-(const year_month_weekday_last& ymwdl, const years& dy) NOEXCEPT
{
    return ymwdl + (-dy);
}

// year_month from operator/()

CONSTCD11
inline
year_month
operator/(const year& y, const month& m) NOEXCEPT
{
    return {y, m};
}

CONSTCD11
inline
year_month
operator/(const year& y, int   m) NOEXCEPT
{
    return y / month(static_cast<unsigned>(m));
}

// month_day from operator/()

CONSTCD11
inline
month_day
operator/(const month& m, const day& d) NOEXCEPT
{
    return {m, d};
}

CONSTCD11
inline
month_day
operator/(const day& d, const month& m) NOEXCEPT
{
    return m / d;
}

CONSTCD11
inline
month_day
operator/(const month& m, int d) NOEXCEPT
{
    return m / day(static_cast<unsigned>(d));
}

CONSTCD11
inline
month_day
operator/(int m, const day& d) NOEXCEPT
{
    return month(static_cast<unsigned>(m)) / d;
}

CONSTCD11 inline month_day operator/(const day& d, int m) NOEXCEPT {return m / d;}

// month_day_last from operator/()

CONSTCD11
inline
month_day_last
operator/(const month& m, last_spec) NOEXCEPT
{
    return month_day_last{m};
}

CONSTCD11
inline
month_day_last
operator/(last_spec, const month& m) NOEXCEPT
{
    return m/last;
}

CONSTCD11
inline
month_day_last
operator/(int m, last_spec) NOEXCEPT
{
    return month(static_cast<unsigned>(m))/last;
}

CONSTCD11
inline
month_day_last
operator/(last_spec, int m) NOEXCEPT
{
    return m/last;
}

// month_weekday from operator/()

CONSTCD11
inline
month_weekday
operator/(const month& m, const weekday_indexed& wdi) NOEXCEPT
{
    return {m, wdi};
}

CONSTCD11
inline
month_weekday
operator/(const weekday_indexed& wdi, const month& m) NOEXCEPT
{
    return m / wdi;
}

CONSTCD11
inline
month_weekday
operator/(int m, const weekday_indexed& wdi) NOEXCEPT
{
    return month(static_cast<unsigned>(m)) / wdi;
}

CONSTCD11
inline
month_weekday
operator/(const weekday_indexed& wdi, int m) NOEXCEPT
{
    return m / wdi;
}

// month_weekday_last from operator/()

CONSTCD11
inline
month_weekday_last
operator/(const month& m, const weekday_last& wdl) NOEXCEPT
{
    return {m, wdl};
}

CONSTCD11
inline
month_weekday_last
operator/(const weekday_last& wdl, const month& m) NOEXCEPT
{
    return m / wdl;
}

CONSTCD11
inline
month_weekday_last
operator/(int m, const weekday_last& wdl) NOEXCEPT
{
    return month(static_cast<unsigned>(m)) / wdl;
}

CONSTCD11
inline
month_weekday_last
operator/(const weekday_last& wdl, int m) NOEXCEPT
{
    return m / wdl;
}

// year_month_day from operator/()

CONSTCD11
inline
year_month_day
operator/(const year_month& ym, const day& d) NOEXCEPT
{
    return {ym.year(), ym.month(), d};
}

CONSTCD11
inline
year_month_day
operator/(const year_month& ym, int d)  NOEXCEPT
{
    return ym / day(static_cast<unsigned>(d));
}

CONSTCD11
inline
year_month_day
operator/(const year& y, const month_day& md) NOEXCEPT
{
    return y / md.month() / md.day();
}

CONSTCD11
inline
year_month_day
operator/(int y, const month_day& md) NOEXCEPT
{
    return year(y) / md;
}

CONSTCD11
inline
year_month_day
operator/(const month_day& md, const year& y)  NOEXCEPT
{
    return y / md;
}

CONSTCD11
inline
year_month_day
operator/(const month_day& md, int y) NOEXCEPT
{
    return year(y) / md;
}

// year_month_day_last from operator/()

CONSTCD11
inline
year_month_day_last
operator/(const year_month& ym, last_spec) NOEXCEPT
{
    return {ym.year(), month_day_last{ym.month()}};
}

CONSTCD11
inline
year_month_day_last
operator/(const year& y, const month_day_last& mdl) NOEXCEPT
{
    return {y, mdl};
}

CONSTCD11
inline
year_month_day_last
operator/(int y, const month_day_last& mdl) NOEXCEPT
{
    return year(y) / mdl;
}

CONSTCD11
inline
year_month_day_last
operator/(const month_day_last& mdl, const year& y) NOEXCEPT
{
    return y / mdl;
}

CONSTCD11
inline
year_month_day_last
operator/(const month_day_last& mdl, int y) NOEXCEPT
{
    return year(y) / mdl;
}

// year_month_weekday from operator/()

CONSTCD11
inline
year_month_weekday
operator/(const year_month& ym, const weekday_indexed& wdi) NOEXCEPT
{
    return {ym.year(), ym.month(), wdi};
}

CONSTCD11
inline
year_month_weekday
operator/(const year& y, const month_weekday& mwd) NOEXCEPT
{
    return {y, mwd.month(), mwd.weekday_indexed()};
}

CONSTCD11
inline
year_month_weekday
operator/(int y, const month_weekday& mwd) NOEXCEPT
{
    return year(y) / mwd;
}

CONSTCD11
inline
year_month_weekday
operator/(const month_weekday& mwd, const year& y) NOEXCEPT
{
    return y / mwd;
}

CONSTCD11
inline
year_month_weekday
operator/(const month_weekday& mwd, int y) NOEXCEPT
{
    return year(y) / mwd;
}

// year_month_weekday_last from operator/()

CONSTCD11
inline
year_month_weekday_last
operator/(const year_month& ym, const weekday_last& wdl) NOEXCEPT
{
    return {ym.year(), ym.month(), wdl};
}

CONSTCD11
inline
year_month_weekday_last
operator/(const year& y, const month_weekday_last& mwdl) NOEXCEPT
{
    return {y, mwdl.month(), mwdl.weekday_last()};
}

CONSTCD11
inline
year_month_weekday_last
operator/(int y, const month_weekday_last& mwdl) NOEXCEPT
{
    return year(y) / mwdl;
}

CONSTCD11
inline
year_month_weekday_last
operator/(const month_weekday_last& mwdl, const year& y) NOEXCEPT
{
    return y / mwdl;
}

CONSTCD11
inline
year_month_weekday_last
operator/(const month_weekday_last& mwdl, int y) NOEXCEPT
{
    return year(y) / mwdl;
}

template <class Duration>
struct fields;

template <class CharT, class Traits, class Duration>
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const fields<Duration>& fds, const std::string* abbrev = nullptr,
          const std::chrono::seconds* offset_sec = nullptr);

template <class CharT, class Traits, class Duration, class Alloc>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            fields<Duration>& fds, std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr);

// hh_mm_ss

namespace detail
{

struct undocumented {explicit undocumented() = default;};

// width<n>::value is the number of fractional decimal digits in 1/n
// width<0>::value and width<1>::value are defined to be 0
// If 1/n takes more than 18 fractional decimal digits,
//   the result is truncated to 19.
// Example:  width<2>::value    ==  1
// Example:  width<3>::value    == 19
// Example:  width<4>::value    ==  2
// Example:  width<10>::value   ==  1
// Example:  width<1000>::value ==  3
template <std::uint64_t n, std::uint64_t d, unsigned w = 0,
          bool should_continue = n%d != 0 && (w < 19)>
struct width
{
    static_assert(d > 0, "width called with zero denominator");
    static CONSTDATA unsigned value = 1 + width<n%d*10, d, w+1>::value;
};

template <std::uint64_t n, std::uint64_t d, unsigned w>
struct width<n, d, w, false>
{
    static CONSTDATA unsigned value = 0;
};

template <unsigned exp>
struct static_pow10
{
private:
    static CONSTDATA std::uint64_t h = static_pow10<exp/2>::value;
public:
    static CONSTDATA std::uint64_t value = h * h * (exp % 2 ? 10 : 1);
};

template <>
struct static_pow10<0>
{
    static CONSTDATA std::uint64_t value = 1;
};

template <class Duration>
class decimal_format_seconds
{
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    using rep = typename CT::rep;
    static unsigned CONSTDATA trial_width =
        detail::width<CT::period::num, CT::period::den>::value;
public:
    static unsigned CONSTDATA width = trial_width < 19 ? trial_width : 6u;
    using precision = std::chrono::duration<rep,
                                            std::ratio<1, static_pow10<width>::value>>;

private:
    std::chrono::seconds s_;
    precision            sub_s_;

public:
    CONSTCD11 decimal_format_seconds()
        : s_()
        , sub_s_()
        {}

    CONSTCD11 explicit decimal_format_seconds(const Duration& d) NOEXCEPT
        : s_(std::chrono::duration_cast<std::chrono::seconds>(d))
        , sub_s_(std::chrono::duration_cast<precision>(d - s_))
        {}

    CONSTCD14 std::chrono::seconds& seconds() NOEXCEPT {return s_;}
    CONSTCD11 std::chrono::seconds seconds() const NOEXCEPT {return s_;}
    CONSTCD11 precision subseconds() const NOEXCEPT {return sub_s_;}

    CONSTCD14 precision to_duration() const NOEXCEPT
    {
        return s_ + sub_s_;
    }

    CONSTCD11 bool in_conventional_range() const NOEXCEPT
    {
        return sub_s_ < std::chrono::seconds{1} && s_ < std::chrono::minutes{1};
    }

    template <class CharT, class Traits>
    friend
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const decimal_format_seconds& x)
    {
        return x.print(os, std::chrono::treat_as_floating_point<rep>{});
    }

    template <class CharT, class Traits>
    std::basic_ostream<CharT, Traits>&
    print(std::basic_ostream<CharT, Traits>& os, std::true_type) const
    {
        date::detail::save_ostream<CharT, Traits> _(os);
        std::chrono::duration<rep> d = s_ + sub_s_;
        if (d < std::chrono::seconds{10})
            os << '0';
        os.precision(width+6);
        os << std::fixed << d.count();
        return os;
    }

    template <class CharT, class Traits>
    std::basic_ostream<CharT, Traits>&
    print(std::basic_ostream<CharT, Traits>& os, std::false_type) const
    {
        date::detail::save_ostream<CharT, Traits> _(os);
        os.fill('0');
        os.flags(std::ios::dec | std::ios::right);
        os.width(2);
        os << s_.count();
        if (width > 0)
        {
#if !ONLY_C_LOCALE
            os << std::use_facet<std::numpunct<CharT>>(os.getloc()).decimal_point();
#else
            os << '.';
#endif
            date::detail::save_ostream<CharT, Traits> _s(os);
            os.imbue(std::locale::classic());
            os.width(width);
            os << sub_s_.count();
        }
        return os;
    }
};

template <class Rep, class Period>
inline
CONSTCD11
typename std::enable_if
         <
            std::numeric_limits<Rep>::is_signed,
            std::chrono::duration<Rep, Period>
         >::type
abs(std::chrono::duration<Rep, Period> d)
{
    return d >= d.zero() ? +d : -d;
}

template <class Rep, class Period>
inline
CONSTCD11
typename std::enable_if
         <
            !std::numeric_limits<Rep>::is_signed,
            std::chrono::duration<Rep, Period>
         >::type
abs(std::chrono::duration<Rep, Period> d)
{
    return d;
}

}  // namespace detail

template <class Duration>
class hh_mm_ss
{
    using dfs = detail::decimal_format_seconds<typename std::common_type<Duration,
                                               std::chrono::seconds>::type>;

    std::chrono::hours h_;
    std::chrono::minutes m_;
    dfs s_;
    bool neg_;

public:
    static unsigned CONSTDATA fractional_width = dfs::width;
    using precision = typename dfs::precision;

    CONSTCD11 hh_mm_ss() NOEXCEPT
        : hh_mm_ss(Duration::zero())
        {}

    CONSTCD11 explicit hh_mm_ss(Duration d) NOEXCEPT
        : h_(std::chrono::duration_cast<std::chrono::hours>(detail::abs(d)))
        , m_(std::chrono::duration_cast<std::chrono::minutes>(detail::abs(d)) - h_)
        , s_(detail::abs(d) - h_ - m_)
        , neg_(d < Duration::zero())
        {}

    CONSTCD11 std::chrono::hours hours() const NOEXCEPT {return h_;}
    CONSTCD11 std::chrono::minutes minutes() const NOEXCEPT {return m_;}
    CONSTCD11 std::chrono::seconds seconds() const NOEXCEPT {return s_.seconds();}
    CONSTCD14 std::chrono::seconds&
        seconds(detail::undocumented) NOEXCEPT {return s_.seconds();}
    CONSTCD11 precision subseconds() const NOEXCEPT {return s_.subseconds();}
    CONSTCD11 bool is_negative() const NOEXCEPT {return neg_;}

    CONSTCD11 explicit operator  precision()   const NOEXCEPT {return to_duration();}
    CONSTCD11          precision to_duration() const NOEXCEPT
        {return (s_.to_duration() + m_ + h_) * (1-2*neg_);}

    CONSTCD11 bool in_conventional_range() const NOEXCEPT
    {
        return !neg_ && h_ < days{1} && m_ < std::chrono::hours{1} &&
               s_.in_conventional_range();
    }

private:

    template <class charT, class traits>
    friend
    std::basic_ostream<charT, traits>&
    operator<<(std::basic_ostream<charT, traits>& os, hh_mm_ss const& tod)
    {
        if (tod.is_negative())
            os << '-';
        if (tod.h_ < std::chrono::hours{10})
            os << '0';
        os << tod.h_.count() << ':';
        if (tod.m_ < std::chrono::minutes{10})
            os << '0';
        os << tod.m_.count() << ':' << tod.s_;
        return os;
    }

    template <class CharT, class Traits, class Duration2>
    friend
    std::basic_ostream<CharT, Traits>&
    date::to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const fields<Duration2>& fds, const std::string* abbrev,
          const std::chrono::seconds* offset_sec);

    template <class CharT, class Traits, class Duration2, class Alloc>
    friend
    std::basic_istream<CharT, Traits>&
    date::from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
          fields<Duration2>& fds,
          std::basic_string<CharT, Traits, Alloc>* abbrev, std::chrono::minutes* offset);
};

inline
CONSTCD14
bool
is_am(std::chrono::hours const& h) NOEXCEPT
{
    using std::chrono::hours;
    return hours{0} <= h && h < hours{12};
}

inline
CONSTCD14
bool
is_pm(std::chrono::hours const& h) NOEXCEPT
{
    using std::chrono::hours;
    return hours{12} <= h && h < hours{24};
}

inline
CONSTCD14
std::chrono::hours
make12(std::chrono::hours h) NOEXCEPT
{
    using std::chrono::hours;
    if (h < hours{12})
    {
        if (h == hours{0})
            h = hours{12};
    }
    else
    {
        if (h != hours{12})
            h = h - hours{12};
    }
    return h;
}

inline
CONSTCD14
std::chrono::hours
make24(std::chrono::hours h, bool is_pm) NOEXCEPT
{
    using std::chrono::hours;
    if (is_pm)
    {
        if (h != hours{12})
            h = h + hours{12};
    }
    else if (h == hours{12})
        h = hours{0};
    return h;
}

template <class Duration>
using time_of_day = hh_mm_ss<Duration>;

template <class Rep, class Period>
CONSTCD11
inline
hh_mm_ss<std::chrono::duration<Rep, Period>>
make_time(const std::chrono::duration<Rep, Period>& d)
{
    return hh_mm_ss<std::chrono::duration<Rep, Period>>(d);
}

template <class CharT, class Traits, class Duration>
inline
typename std::enable_if
<
    !std::is_convertible<Duration, days>::value,
    std::basic_ostream<CharT, Traits>&
>::type
operator<<(std::basic_ostream<CharT, Traits>& os, const sys_time<Duration>& tp)
{
    auto const dp = date::floor<days>(tp);
    return os << year_month_day(dp) << ' ' << make_time(tp-dp);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const sys_days& dp)
{
    return os << year_month_day(dp);
}

template <class CharT, class Traits, class Duration>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const local_time<Duration>& ut)
{
    return (date::operator<<(os, sys_time<Duration>{ut.time_since_epoch()}));
}

namespace detail
{

template <class CharT, std::size_t N>
class string_literal;

template <class CharT1, class CharT2, std::size_t N1, std::size_t N2>
inline
CONSTCD14
string_literal<typename std::conditional<sizeof(CharT2) <= sizeof(CharT1), CharT1, CharT2>::type,
               N1 + N2 - 1>
operator+(const string_literal<CharT1, N1>& x, const string_literal<CharT2, N2>& y) NOEXCEPT;

template <class CharT, std::size_t N>
class string_literal
{
    CharT p_[N];

    CONSTCD11 string_literal() NOEXCEPT
      : p_{}
    {}

public:
    using const_iterator = const CharT*;

    string_literal(string_literal const&) = default;
    string_literal& operator=(string_literal const&) = delete;

    template <std::size_t N1 = 2,
              class = typename std::enable_if<N1 == N>::type>
    CONSTCD11 string_literal(CharT c) NOEXCEPT
        : p_{c}
    {
    }

    template <std::size_t N1 = 3,
              class = typename std::enable_if<N1 == N>::type>
    CONSTCD11 string_literal(CharT c1, CharT c2) NOEXCEPT
        : p_{c1, c2}
    {
    }

    template <std::size_t N1 = 4,
              class = typename std::enable_if<N1 == N>::type>
    CONSTCD11 string_literal(CharT c1, CharT c2, CharT c3) NOEXCEPT
        : p_{c1, c2, c3}
    {
    }

    CONSTCD14 string_literal(const CharT(&a)[N]) NOEXCEPT
        : p_{}
    {
        for (std::size_t i = 0; i < N; ++i)
            p_[i] = a[i];
    }

    template <class U = CharT,
              class = typename std::enable_if<(1 < sizeof(U))>::type>
    CONSTCD14 string_literal(const char(&a)[N]) NOEXCEPT
        : p_{}
    {
        for (std::size_t i = 0; i < N; ++i)
            p_[i] = a[i];
    }

    template <class CharT2,
              class = typename std::enable_if<!std::is_same<CharT2, CharT>::value>::type>
    CONSTCD14 string_literal(string_literal<CharT2, N> const& a) NOEXCEPT
        : p_{}
    {
        for (std::size_t i = 0; i < N; ++i)
            p_[i] = a[i];
    }

    CONSTCD11 const CharT* data() const NOEXCEPT {return p_;}
    CONSTCD11 std::size_t size() const NOEXCEPT {return N-1;}

    CONSTCD11 const_iterator begin() const NOEXCEPT {return p_;}
    CONSTCD11 const_iterator end()   const NOEXCEPT {return p_ + N-1;}

    CONSTCD11 CharT const& operator[](std::size_t n) const NOEXCEPT
    {
        return p_[n];
    }

    template <class Traits>
    friend
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const string_literal& s)
    {
        return os << s.p_;
    }

    template <class CharT1, class CharT2, std::size_t N1, std::size_t N2>
    friend
    CONSTCD14
    string_literal<typename std::conditional<sizeof(CharT2) <= sizeof(CharT1), CharT1, CharT2>::type,
                   N1 + N2 - 1>
    operator+(const string_literal<CharT1, N1>& x, const string_literal<CharT2, N2>& y) NOEXCEPT;
};

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 3>
operator+(const string_literal<CharT, 2>& x, const string_literal<CharT, 2>& y) NOEXCEPT
{
  return string_literal<CharT, 3>(x[0], y[0]);
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 4>
operator+(const string_literal<CharT, 3>& x, const string_literal<CharT, 2>& y) NOEXCEPT
{
  return string_literal<CharT, 4>(x[0], x[1], y[0]);
}

template <class CharT1, class CharT2, std::size_t N1, std::size_t N2>
CONSTCD14
inline
string_literal<typename std::conditional<sizeof(CharT2) <= sizeof(CharT1), CharT1, CharT2>::type,
               N1 + N2 - 1>
operator+(const string_literal<CharT1, N1>& x, const string_literal<CharT2, N2>& y) NOEXCEPT
{
    using CT = typename std::conditional<sizeof(CharT2) <= sizeof(CharT1), CharT1, CharT2>::type;

    string_literal<CT, N1 + N2 - 1> r;
    std::size_t i = 0;
    for (; i < N1-1; ++i)
       r.p_[i] = CT(x.p_[i]);
    for (std::size_t j = 0; j < N2; ++j, ++i)
       r.p_[i] = CT(y.p_[j]);

    return r;
}


template <class CharT, class Traits, class Alloc, std::size_t N>
inline
std::basic_string<CharT, Traits, Alloc>
operator+(std::basic_string<CharT, Traits, Alloc> x, const string_literal<CharT, N>& y)
{
    x.append(y.data(), y.size());
    return x;
}

#if __cplusplus >= 201402  && (!defined(__EDG_VERSION__) || __EDG_VERSION__ > 411) \
                           && (!defined(__SUNPRO_CC) || __SUNPRO_CC > 0x5150)

template <class CharT,
          class = std::enable_if_t<std::is_same<CharT, char>::value ||
                                   std::is_same<CharT, wchar_t>::value ||
                                   std::is_same<CharT, char16_t>::value ||
                                   std::is_same<CharT, char32_t>::value>>
CONSTCD14
inline
string_literal<CharT, 2>
msl(CharT c) NOEXCEPT
{
    return string_literal<CharT, 2>{c};
}

CONSTCD14
inline
std::size_t
to_string_len(std::intmax_t i)
{
    std::size_t r = 0;
    do
    {
        i /= 10;
        ++r;
    } while (i > 0);
    return r;
}

template <std::intmax_t N>
CONSTCD14
inline
std::enable_if_t
<
    N < 10,
    string_literal<char, to_string_len(N)+1>
>
msl() NOEXCEPT
{
    return msl(char(N % 10 + '0'));
}

template <std::intmax_t N>
CONSTCD14
inline
std::enable_if_t
<
    10 <= N,
    string_literal<char, to_string_len(N)+1>
>
msl() NOEXCEPT
{
    return msl<N/10>() + msl(char(N % 10 + '0'));
}

template <class CharT, std::intmax_t N, std::intmax_t D>
CONSTCD14
inline
std::enable_if_t
<
    std::ratio<N, D>::type::den != 1,
    string_literal<CharT, to_string_len(std::ratio<N, D>::type::num) +
                          to_string_len(std::ratio<N, D>::type::den) + 4>
>
msl(std::ratio<N, D>) NOEXCEPT
{
    using R = typename std::ratio<N, D>::type;
    return msl(CharT{'['}) + msl<R::num>() + msl(CharT{'/'}) +
                             msl<R::den>() + msl(CharT{']'});
}

template <class CharT, std::intmax_t N, std::intmax_t D>
CONSTCD14
inline
std::enable_if_t
<
    std::ratio<N, D>::type::den == 1,
    string_literal<CharT, to_string_len(std::ratio<N, D>::type::num) + 3>
>
msl(std::ratio<N, D>) NOEXCEPT
{
    using R = typename std::ratio<N, D>::type;
    return msl(CharT{'['}) + msl<R::num>() + msl(CharT{']'});
}


#else  // __cplusplus < 201402 || (defined(__EDG_VERSION__) && __EDG_VERSION__ <= 411)

inline
std::string
to_string(std::uint64_t x)
{
    return std::to_string(x);
}

template <class CharT>
inline
std::basic_string<CharT>
to_string(std::uint64_t x)
{
    auto y = std::to_string(x);
    return std::basic_string<CharT>(y.begin(), y.end());
}

template <class CharT, std::intmax_t N, std::intmax_t D>
inline
typename std::enable_if
<
    std::ratio<N, D>::type::den != 1,
    std::basic_string<CharT>
>::type
msl(std::ratio<N, D>)
{
    using R = typename std::ratio<N, D>::type;
    return std::basic_string<CharT>(1, '[') + to_string<CharT>(R::num) + CharT{'/'} +
                                              to_string<CharT>(R::den) + CharT{']'};
}

template <class CharT, std::intmax_t N, std::intmax_t D>
inline
typename std::enable_if
<
    std::ratio<N, D>::type::den == 1,
    std::basic_string<CharT>
>::type
msl(std::ratio<N, D>)
{
    using R = typename std::ratio<N, D>::type;
    return std::basic_string<CharT>(1, '[') + to_string<CharT>(R::num) + CharT{']'};
}

#endif  // __cplusplus < 201402 || (defined(__EDG_VERSION__) && __EDG_VERSION__ <= 411)

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::atto) NOEXCEPT
{
    return string_literal<CharT, 2>{'a'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::femto) NOEXCEPT
{
    return string_literal<CharT, 2>{'f'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::pico) NOEXCEPT
{
    return string_literal<CharT, 2>{'p'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::nano) NOEXCEPT
{
    return string_literal<CharT, 2>{'n'};
}

template <class CharT>
CONSTCD11
inline
typename std::enable_if
<
    std::is_same<CharT, char>::value,
    string_literal<char, 3>
>::type
msl(std::micro) NOEXCEPT
{
    return string_literal<char, 3>{'\xC2', '\xB5'};
}

template <class CharT>
CONSTCD11
inline
typename std::enable_if
<
    !std::is_same<CharT, char>::value,
    string_literal<CharT, 2>
>::type
msl(std::micro) NOEXCEPT
{
    return string_literal<CharT, 2>{CharT{static_cast<unsigned char>('\xB5')}};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::milli) NOEXCEPT
{
    return string_literal<CharT, 2>{'m'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::centi) NOEXCEPT
{
    return string_literal<CharT, 2>{'c'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 3>
msl(std::deca) NOEXCEPT
{
    return string_literal<CharT, 3>{'d', 'a'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::deci) NOEXCEPT
{
    return string_literal<CharT, 2>{'d'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::hecto) NOEXCEPT
{
    return string_literal<CharT, 2>{'h'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::kilo) NOEXCEPT
{
    return string_literal<CharT, 2>{'k'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::mega) NOEXCEPT
{
    return string_literal<CharT, 2>{'M'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::giga) NOEXCEPT
{
    return string_literal<CharT, 2>{'G'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::tera) NOEXCEPT
{
    return string_literal<CharT, 2>{'T'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::peta) NOEXCEPT
{
    return string_literal<CharT, 2>{'P'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
msl(std::exa) NOEXCEPT
{
    return string_literal<CharT, 2>{'E'};
}

template <class CharT, class Period>
CONSTCD11
inline
auto
get_units(Period p)
 -> decltype(msl<CharT>(p) + string_literal<CharT, 2>{'s'})
{
    return msl<CharT>(p) + string_literal<CharT, 2>{'s'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
get_units(std::ratio<1>)
{
    return string_literal<CharT, 2>{'s'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
get_units(std::ratio<3600>)
{
    return string_literal<CharT, 2>{'h'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 4>
get_units(std::ratio<60>)
{
    return string_literal<CharT, 4>{'m', 'i', 'n'};
}

template <class CharT>
CONSTCD11
inline
string_literal<CharT, 2>
get_units(std::ratio<86400>)
{
    return string_literal<CharT, 2>{'d'};
}

template <class CharT, class Traits = std::char_traits<CharT>>
struct make_string;

template <>
struct make_string<char>
{
    template <class Rep>
    static
    std::string
    from(Rep n)
    {
        return std::to_string(n);
    }
};

template <class Traits>
struct make_string<char, Traits>
{
    template <class Rep>
    static
    std::basic_string<char, Traits>
    from(Rep n)
    {
        auto s = std::to_string(n);
        return std::basic_string<char, Traits>(s.begin(), s.end());
    }
};

template <>
struct make_string<wchar_t>
{
    template <class Rep>
    static
    std::wstring
    from(Rep n)
    {
        return std::to_wstring(n);
    }
};

template <class Traits>
struct make_string<wchar_t, Traits>
{
    template <class Rep>
    static
    std::basic_string<wchar_t, Traits>
    from(Rep n)
    {
        auto s = std::to_wstring(n);
        return std::basic_string<wchar_t, Traits>(s.begin(), s.end());
    }
};

}  // namespace detail

// to_stream

CONSTDATA year nanyear{-32768};

template <class Duration>
struct fields
{
    year_month_day        ymd{nanyear/0/0};
    weekday               wd{8u};
    hh_mm_ss<Duration>    tod{};
    bool                  has_tod = false;

#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ <= 409)
    fields() : ymd{nanyear/0/0}, wd{8u}, tod{}, has_tod{false} {}
#else
    fields() = default;
#endif

    fields(year_month_day ymd_) : ymd(ymd_) {}
    fields(weekday wd_) : wd(wd_) {}
    fields(hh_mm_ss<Duration> tod_) : tod(tod_), has_tod(true) {}

    fields(year_month_day ymd_, weekday wd_) : ymd(ymd_), wd(wd_) {}
    fields(year_month_day ymd_, hh_mm_ss<Duration> tod_) : ymd(ymd_), tod(tod_),
                                                           has_tod(true) {}

    fields(weekday wd_, hh_mm_ss<Duration> tod_) : wd(wd_), tod(tod_), has_tod(true) {}

    fields(year_month_day ymd_, weekday wd_, hh_mm_ss<Duration> tod_)
        : ymd(ymd_)
        , wd(wd_)
        , tod(tod_)
        , has_tod(true)
        {}
};

namespace detail
{

template <class CharT, class Traits, class Duration>
unsigned
extract_weekday(std::basic_ostream<CharT, Traits>& os, const fields<Duration>& fds)
{
    if (!fds.ymd.ok() && !fds.wd.ok())
    {
        // fds does not contain a valid weekday
        os.setstate(std::ios::failbit);
        return 8;
    }
    weekday wd;
    if (fds.ymd.ok())
    {
        wd = weekday{sys_days(fds.ymd)};
        if (fds.wd.ok() && wd != fds.wd)
        {
            // fds.ymd and fds.wd are inconsistent
            os.setstate(std::ios::failbit);
            return 8;
        }
    }
    else
        wd = fds.wd;
    return static_cast<unsigned>((wd - Sunday).count());
}

template <class CharT, class Traits, class Duration>
unsigned
extract_month(std::basic_ostream<CharT, Traits>& os, const fields<Duration>& fds)
{
    if (!fds.ymd.month().ok())
    {
        // fds does not contain a valid month
        os.setstate(std::ios::failbit);
        return 0;
    }
    return static_cast<unsigned>(fds.ymd.month());
}

}  // namespace detail

#if ONLY_C_LOCALE

namespace detail
{

inline
std::pair<const std::string*, const std::string*>
weekday_names()
{
    static const std::string nm[] =
    {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
        "Sun",
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat"
    };
    return std::make_pair(nm, nm+sizeof(nm)/sizeof(nm[0]));
}

inline
std::pair<const std::string*, const std::string*>
month_names()
{
    static const std::string nm[] =
    {
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December",
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
    };
    return std::make_pair(nm, nm+sizeof(nm)/sizeof(nm[0]));
}

inline
std::pair<const std::string*, const std::string*>
ampm_names()
{
    static const std::string nm[] =
    {
        "AM",
        "PM"
    };
    return std::make_pair(nm, nm+sizeof(nm)/sizeof(nm[0]));
}

template <class CharT, class Traits, class FwdIter>
FwdIter
scan_keyword(std::basic_istream<CharT, Traits>& is, FwdIter kb, FwdIter ke)
{
    size_t nkw = static_cast<size_t>(std::distance(kb, ke));
    const unsigned char doesnt_match = '\0';
    const unsigned char might_match = '\1';
    const unsigned char does_match = '\2';
    unsigned char statbuf[100];
    unsigned char* status = statbuf;
    std::unique_ptr<unsigned char, void(*)(void*)> stat_hold(0, free);
    if (nkw > sizeof(statbuf))
    {
        status = (unsigned char*)std::malloc(nkw);
        if (status == nullptr)
            throw std::bad_alloc();
        stat_hold.reset(status);
    }
    size_t n_might_match = nkw;  // At this point, any keyword might match
    size_t n_does_match = 0;     // but none of them definitely do
    // Initialize all statuses to might_match, except for "" keywords are does_match
    unsigned char* st = status;
    for (auto ky = kb; ky != ke; ++ky, ++st)
    {
        if (!ky->empty())
            *st = might_match;
        else
        {
            *st = does_match;
            --n_might_match;
            ++n_does_match;
        }
    }
    // While there might be a match, test keywords against the next CharT
    for (size_t indx = 0; is && n_might_match > 0; ++indx)
    {
        // Peek at the next CharT but don't consume it
        auto ic = is.peek();
        if (ic == EOF)
        {
            is.setstate(std::ios::eofbit);
            break;
        }
        auto c = static_cast<char>(toupper(static_cast<unsigned char>(ic)));
        bool consume = false;
        // For each keyword which might match, see if the indx character is c
        // If a match if found, consume c
        // If a match is found, and that is the last character in the keyword,
        //    then that keyword matches.
        // If the keyword doesn't match this character, then change the keyword
        //    to doesn't match
        st = status;
        for (auto ky = kb; ky != ke; ++ky, ++st)
        {
            if (*st == might_match)
            {
                if (c == static_cast<char>(toupper(static_cast<unsigned char>((*ky)[indx]))))
                {
                    consume = true;
                    if (ky->size() == indx+1)
                    {
                        *st = does_match;
                        --n_might_match;
                        ++n_does_match;
                    }
                }
                else
                {
                    *st = doesnt_match;
                    --n_might_match;
                }
            }
        }
        // consume if we matched a character
        if (consume)
        {
            (void)is.get();
            // If we consumed a character and there might be a matched keyword that
            //   was marked matched on a previous iteration, then such keywords
            //   are now marked as not matching.
            if (n_might_match + n_does_match > 1)
            {
                st = status;
                for (auto ky = kb; ky != ke; ++ky, ++st)
                {
                    if (*st == does_match && ky->size() != indx+1)
                    {
                        *st = doesnt_match;
                        --n_does_match;
                    }
                }
            }
        }
    }
    // We've exited the loop because we hit eof and/or we have no more "might matches".
    // Return the first matching result
    for (st = status; kb != ke; ++kb, ++st)
        if (*st == does_match)
            break;
    if (kb == ke)
        is.setstate(std::ios::failbit);
    return kb;
}

}  // namespace detail

#endif  // ONLY_C_LOCALE

template <class CharT, class Traits, class Duration>
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const fields<Duration>& fds, const std::string* abbrev,
          const std::chrono::seconds* offset_sec)
{
#if ONLY_C_LOCALE
    using detail::weekday_names;
    using detail::month_names;
    using detail::ampm_names;
#endif
    using detail::save_ostream;
    using detail::get_units;
    using detail::extract_weekday;
    using detail::extract_month;
    using std::ios;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::minutes;
    using std::chrono::hours;
    date::detail::save_ostream<CharT, Traits> ss(os);
    os.fill(' ');
    os.flags(std::ios::skipws | std::ios::dec);
    os.width(0);
    tm tm{};
    bool insert_negative = fds.has_tod && fds.tod.to_duration() < Duration::zero();
#if !ONLY_C_LOCALE
    auto& facet = std::use_facet<std::time_put<CharT>>(os.getloc());
#endif
    const CharT* command = nullptr;
    CharT modified = CharT{};
    for (; *fmt; ++fmt)
    {
        switch (*fmt)
        {
        case 'a':
        case 'A':
            if (command)
            {
                if (modified == CharT{})
                {
                    tm.tm_wday = static_cast<int>(extract_weekday(os, fds));
                    if (os.fail())
                        return os;
#if !ONLY_C_LOCALE
                    const CharT f[] = {'%', *fmt};
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
#else  // ONLY_C_LOCALE
                    os << weekday_names().first[tm.tm_wday+7*(*fmt == 'a')];
#endif  // ONLY_C_LOCALE
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'b':
        case 'B':
        case 'h':
            if (command)
            {
                if (modified == CharT{})
                {
                    tm.tm_mon = static_cast<int>(extract_month(os, fds)) - 1;
#if !ONLY_C_LOCALE
                    const CharT f[] = {'%', *fmt};
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
#else  // ONLY_C_LOCALE
                    os << month_names().first[tm.tm_mon+12*(*fmt != 'B')];
#endif  // ONLY_C_LOCALE
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'c':
        case 'x':
            if (command)
            {
                if (modified == CharT{'O'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.ok())
                        os.setstate(std::ios::failbit);
                    if (*fmt == 'c' && !fds.has_tod)
                        os.setstate(std::ios::failbit);
#if !ONLY_C_LOCALE
                    tm = std::tm{};
                    auto const& ymd = fds.ymd;
                    auto ld = local_days(ymd);
                    if (*fmt == 'c')
                    {
                        tm.tm_sec = static_cast<int>(fds.tod.seconds().count());
                        tm.tm_min = static_cast<int>(fds.tod.minutes().count());
                        tm.tm_hour = static_cast<int>(fds.tod.hours().count());
                    }
                    tm.tm_mday = static_cast<int>(static_cast<unsigned>(ymd.day()));
                    tm.tm_mon = static_cast<int>(extract_month(os, fds) - 1);
                    tm.tm_year = static_cast<int>(ymd.year()) - 1900;
                    tm.tm_wday = static_cast<int>(extract_weekday(os, fds));
                    if (os.fail())
                        return os;
                    tm.tm_yday = static_cast<int>((ld - local_days(ymd.year()/1/1)).count());
                    CharT f[3] = {'%'};
                    auto fe = std::begin(f) + 1;
                    if (modified == CharT{'E'})
                        *fe++ = modified;
                    *fe++ = *fmt;
                    facet.put(os, os, os.fill(), &tm, std::begin(f), fe);
#else  // ONLY_C_LOCALE
                    if (*fmt == 'c')
                    {
                        auto wd = static_cast<int>(extract_weekday(os, fds));
                        os << weekday_names().first[static_cast<unsigned>(wd)+7]
                           << ' ';
                        os << month_names().first[extract_month(os, fds)-1+12] << ' ';
                        auto d = static_cast<int>(static_cast<unsigned>(fds.ymd.day()));
                        if (d < 10)
                            os << ' ';
                        os << d << ' '
                           << make_time(duration_cast<seconds>(fds.tod.to_duration()))
                           << ' ' << fds.ymd.year();

                    }
                    else  // *fmt == 'x'
                    {
                        auto const& ymd = fds.ymd;
                        save_ostream<CharT, Traits> _(os);
                        os.fill('0');
                        os.flags(std::ios::dec | std::ios::right);
                        os.width(2);
                        os << static_cast<unsigned>(ymd.month()) << CharT{'/'};
                        os.width(2);
                        os << static_cast<unsigned>(ymd.day()) << CharT{'/'};
                        os.width(2);
                        os << static_cast<int>(ymd.year()) % 100;
                    }
#endif  // ONLY_C_LOCALE
                }
                command = nullptr;
                modified = CharT{};
            }
            else
                os << *fmt;
            break;
        case 'C':
            if (command)
            {
                if (modified == CharT{'O'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.year().ok())
                        os.setstate(std::ios::failbit);
                    auto y = static_cast<int>(fds.ymd.year());
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        save_ostream<CharT, Traits> _(os);
                        os.fill('0');
                        os.flags(std::ios::dec | std::ios::right);
                        if (y >= 0)
                        {
                            os.width(2);
                            os << y/100;
                        }
                        else
                        {
                            os << CharT{'-'};
                            os.width(2);
                            os << -(y-99)/100;
                        }
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'E'})
                    {
                        tm.tm_year = y - 1900;
                        CharT f[3] = {'%', 'E', 'C'};
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                command = nullptr;
                modified = CharT{};
            }
            else
                os << *fmt;
            break;
        case 'd':
        case 'e':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.day().ok())
                        os.setstate(std::ios::failbit);
                    auto d = static_cast<int>(static_cast<unsigned>(fds.ymd.day()));
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        save_ostream<CharT, Traits> _(os);
                        if (*fmt == CharT{'d'})
                            os.fill('0');
                        else
                            os.fill(' ');
                        os.flags(std::ios::dec | std::ios::right);
                        os.width(2);
                        os << d;
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        tm.tm_mday = d;
                        CharT f[3] = {'%', 'O', *fmt};
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                command = nullptr;
                modified = CharT{};
            }
            else
                os << *fmt;
            break;
        case 'D':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto const& ymd = fds.ymd;
                    save_ostream<CharT, Traits> _(os);
                    os.fill('0');
                    os.flags(std::ios::dec | std::ios::right);
                    os.width(2);
                    os << static_cast<unsigned>(ymd.month()) << CharT{'/'};
                    os.width(2);
                    os << static_cast<unsigned>(ymd.day()) << CharT{'/'};
                    os.width(2);
                    os << static_cast<int>(ymd.year()) % 100;
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'F':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto const& ymd = fds.ymd;
                    save_ostream<CharT, Traits> _(os);
                    os.imbue(std::locale::classic());
                    os.fill('0');
                    os.flags(std::ios::dec | std::ios::right);
                    os.width(4);
                    os << static_cast<int>(ymd.year()) << CharT{'-'};
                    os.width(2);
                    os << static_cast<unsigned>(ymd.month()) << CharT{'-'};
                    os.width(2);
                    os << static_cast<unsigned>(ymd.day());
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'g':
        case 'G':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto ld = local_days(fds.ymd);
                    auto y = year_month_day{ld + days{3}}.year();
                    auto start = local_days((y-years{1})/December/Thursday[last]) +
                                 (Monday-Thursday);
                    if (ld < start)
                        --y;
                    if (*fmt == CharT{'G'})
                        os << y;
                    else
                    {
                        save_ostream<CharT, Traits> _(os);
                        os.fill('0');
                        os.flags(std::ios::dec | std::ios::right);
                        os.width(2);
                        os << std::abs(static_cast<int>(y)) % 100;
                    }
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'H':
        case 'I':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    if (insert_negative)
                    {
                        os << '-';
                        insert_negative = false;
                    }
                    auto hms = fds.tod;
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        auto h = *fmt == CharT{'I'} ? date::make12(hms.hours()) : hms.hours();
                        if (h < hours{10})
                            os << CharT{'0'};
                        os << h.count();
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_hour = static_cast<int>(hms.hours().count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'j':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (fds.ymd.ok() || fds.has_tod)
                    {
                        days doy;
                        if (fds.ymd.ok())
                        {
                            auto ld = local_days(fds.ymd);
                            auto y = fds.ymd.year();
                            doy = ld - local_days(y/January/1) + days{1};
                        }
                        else
                        {
                            doy = duration_cast<days>(fds.tod.to_duration());
                        }
                        save_ostream<CharT, Traits> _(os);
                        os.fill('0');
                        os.flags(std::ios::dec | std::ios::right);
                        os.width(3);
                        os << doy.count();
                    }
                    else
                    {
                        os.setstate(std::ios::failbit);
                    }
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'm':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.month().ok())
                        os.setstate(std::ios::failbit);
                    auto m = static_cast<unsigned>(fds.ymd.month());
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        if (m < 10)
                            os << CharT{'0'};
                        os << m;
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_mon = static_cast<int>(m-1);
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'M':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    if (insert_negative)
                    {
                        os << '-';
                        insert_negative = false;
                    }
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        if (fds.tod.minutes() < minutes{10})
                            os << CharT{'0'};
                        os << fds.tod.minutes().count();
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_min = static_cast<int>(fds.tod.minutes().count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'n':
            if (command)
            {
                if (modified == CharT{})
                    os << CharT{'\n'};
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'p':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
#if !ONLY_C_LOCALE
                    const CharT f[] = {'%', *fmt};
                    tm.tm_hour = static_cast<int>(fds.tod.hours().count());
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
#else
                    if (date::is_am(fds.tod.hours()))
                        os << ampm_names().first[0];
                    else
                        os << ampm_names().first[1];
#endif
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'Q':
        case 'q':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    auto d = fds.tod.to_duration();
                    if (*fmt == 'q')
                        os << get_units<CharT>(typename decltype(d)::period::type{});
                    else
                        os << d.count();
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'r':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
#if !ONLY_C_LOCALE
                    const CharT f[] = {'%', *fmt};
                    tm.tm_hour = static_cast<int>(fds.tod.hours().count());
                    tm.tm_min = static_cast<int>(fds.tod.minutes().count());
                    tm.tm_sec = static_cast<int>(fds.tod.seconds().count());
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
#else
                    hh_mm_ss<seconds> tod(duration_cast<seconds>(fds.tod.to_duration()));
                    save_ostream<CharT, Traits> _(os);
                    os.fill('0');
                    os.width(2);
                    os << date::make12(tod.hours()).count() << CharT{':'};
                    os.width(2);
                    os << tod.minutes().count() << CharT{':'};
                    os.width(2);
                    os << tod.seconds().count() << CharT{' '};
                    if (date::is_am(tod.hours()))
                        os << ampm_names().first[0];
                    else
                        os << ampm_names().first[1];
#endif
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'R':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    if (fds.tod.hours() < hours{10})
                        os << CharT{'0'};
                    os << fds.tod.hours().count() << CharT{':'};
                    if (fds.tod.minutes() < minutes{10})
                        os << CharT{'0'};
                    os << fds.tod.minutes().count();
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'S':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    if (insert_negative)
                    {
                        os << '-';
                        insert_negative = false;
                    }
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        os << fds.tod.s_;
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_sec = static_cast<int>(fds.tod.s_.seconds().count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 't':
            if (command)
            {
                if (modified == CharT{})
                    os << CharT{'\t'};
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'T':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
                    os << fds.tod;
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'u':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    auto wd = extract_weekday(os, fds);
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        os << (wd != 0 ? wd : 7u);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_wday = static_cast<int>(wd);
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'U':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    auto const& ymd = fds.ymd;
                    if (!ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto ld = local_days(ymd);
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        auto st = local_days(Sunday[1]/January/ymd.year());
                        if (ld < st)
                            os << CharT{'0'} << CharT{'0'};
                        else
                        {
                            auto wn = duration_cast<weeks>(ld - st).count() + 1;
                            if (wn < 10)
                                os << CharT{'0'};
                            os << wn;
                        }
                   }
 #if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_year = static_cast<int>(ymd.year()) - 1900;
                        tm.tm_wday = static_cast<int>(extract_weekday(os, fds));
                        if (os.fail())
                            return os;
                        tm.tm_yday = static_cast<int>((ld - local_days(ymd.year()/1/1)).count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'V':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto ld = local_days(fds.ymd);
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        auto y = year_month_day{ld + days{3}}.year();
                        auto st = local_days((y-years{1})/12/Thursday[last]) +
                                  (Monday-Thursday);
                        if (ld < st)
                        {
                            --y;
                            st = local_days((y - years{1})/12/Thursday[last]) +
                                 (Monday-Thursday);
                        }
                        auto wn = duration_cast<weeks>(ld - st).count() + 1;
                        if (wn < 10)
                            os << CharT{'0'};
                        os << wn;
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        auto const& ymd = fds.ymd;
                        tm.tm_year = static_cast<int>(ymd.year()) - 1900;
                        tm.tm_wday = static_cast<int>(extract_weekday(os, fds));
                        if (os.fail())
                            return os;
                        tm.tm_yday = static_cast<int>((ld - local_days(ymd.year()/1/1)).count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'w':
            if (command)
            {
                auto wd = extract_weekday(os, fds);
                if (os.fail())
                    return os;
#if !ONLY_C_LOCALE
                if (modified == CharT{})
#else
                if (modified != CharT{'E'})
#endif
                {
                    os << wd;
                }
#if !ONLY_C_LOCALE
                else if (modified == CharT{'O'})
                {
                    const CharT f[] = {'%', modified, *fmt};
                    tm.tm_wday = static_cast<int>(wd);
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                }
#endif
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'W':
            if (command)
            {
                if (modified == CharT{'E'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    auto const& ymd = fds.ymd;
                    if (!ymd.ok())
                        os.setstate(std::ios::failbit);
                    auto ld = local_days(ymd);
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        auto st = local_days(Monday[1]/January/ymd.year());
                        if (ld < st)
                            os << CharT{'0'} << CharT{'0'};
                        else
                        {
                            auto wn = duration_cast<weeks>(ld - st).count() + 1;
                            if (wn < 10)
                                os << CharT{'0'};
                            os << wn;
                        }
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_year = static_cast<int>(ymd.year()) - 1900;
                        tm.tm_wday = static_cast<int>(extract_weekday(os, fds));
                        if (os.fail())
                            return os;
                        tm.tm_yday = static_cast<int>((ld - local_days(ymd.year()/1/1)).count());
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'X':
            if (command)
            {
                if (modified == CharT{'O'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.has_tod)
                        os.setstate(std::ios::failbit);
#if !ONLY_C_LOCALE
                    tm = std::tm{};
                    tm.tm_sec = static_cast<int>(fds.tod.seconds().count());
                    tm.tm_min = static_cast<int>(fds.tod.minutes().count());
                    tm.tm_hour = static_cast<int>(fds.tod.hours().count());
                    CharT f[3] = {'%'};
                    auto fe = std::begin(f) + 1;
                    if (modified == CharT{'E'})
                        *fe++ = modified;
                    *fe++ = *fmt;
                    facet.put(os, os, os.fill(), &tm, std::begin(f), fe);
#else
                    os << fds.tod;
#endif
                }
                command = nullptr;
                modified = CharT{};
            }
            else
                os << *fmt;
            break;
        case 'y':
            if (command)
            {
                if (!fds.ymd.year().ok())
                    os.setstate(std::ios::failbit);
                auto y = static_cast<int>(fds.ymd.year());
#if !ONLY_C_LOCALE
                if (modified == CharT{})
                {
#endif
                    y = std::abs(y) % 100;
                    if (y < 10)
                        os << CharT{'0'};
                    os << y;
#if !ONLY_C_LOCALE
                }
                else
                {
                    const CharT f[] = {'%', modified, *fmt};
                    tm.tm_year = y - 1900;
                    facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                }
#endif
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'Y':
            if (command)
            {
                if (modified == CharT{'O'})
                    os << CharT{'%'} << modified << *fmt;
                else
                {
                    if (!fds.ymd.year().ok())
                        os.setstate(std::ios::failbit);
                    auto y = fds.ymd.year();
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        save_ostream<CharT, Traits> _(os);
                        os.imbue(std::locale::classic());
                        os << y;
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'E'})
                    {
                        const CharT f[] = {'%', modified, *fmt};
                        tm.tm_year = static_cast<int>(y) - 1900;
                        facet.put(os, os, os.fill(), &tm, std::begin(f), std::end(f));
                    }
#endif
                }
                modified = CharT{};
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'z':
            if (command)
            {
                if (offset_sec == nullptr)
                {
                    // Can not format %z with unknown offset
                    os.setstate(ios::failbit);
                    return os;
                }
                auto m = duration_cast<minutes>(*offset_sec);
                auto neg = m < minutes{0};
                m = date::abs(m);
                auto h = duration_cast<hours>(m);
                m -= h;
                if (neg)
                    os << CharT{'-'};
                else
                    os << CharT{'+'};
                if (h < hours{10})
                    os << CharT{'0'};
                os << h.count();
                if (modified != CharT{})
                    os << CharT{':'};
                if (m < minutes{10})
                    os << CharT{'0'};
                os << m.count();
                command = nullptr;
                modified = CharT{};
            }
            else
                os << *fmt;
            break;
        case 'Z':
            if (command)
            {
                if (modified == CharT{})
                {
                    if (abbrev == nullptr)
                    {
                        // Can not format %Z with unknown time_zone
                        os.setstate(ios::failbit);
                        return os;
                    }
                    for (auto c : *abbrev)
                        os << CharT(c);
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    modified = CharT{};
                }
                command = nullptr;
            }
            else
                os << *fmt;
            break;
        case 'E':
        case 'O':
            if (command)
            {
                if (modified == CharT{})
                {
                    modified = *fmt;
                }
                else
                {
                    os << CharT{'%'} << modified << *fmt;
                    command = nullptr;
                    modified = CharT{};
                }
            }
            else
                os << *fmt;
            break;
        case '%':
            if (command)
            {
                if (modified == CharT{})
                {
                    os << CharT{'%'};
                    command = nullptr;
                }
                else
                {
                    os << CharT{'%'} << modified << CharT{'%'};
                    command = nullptr;
                    modified = CharT{};
                }
            }
            else
                command = fmt;
            break;
        default:
            if (command)
            {
                os << CharT{'%'};
                command = nullptr;
            }
            if (modified != CharT{})
            {
                os << modified;
                modified = CharT{};
            }
            os << *fmt;
            break;
        }
    }
    if (command)
        os << CharT{'%'};
    if (modified != CharT{})
        os << modified;
    return os;
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const year& y)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{y/0/0};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const month& m)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{m/0/nanyear};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const day& d)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{d/0/nanyear};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const weekday& wd)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{wd};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const year_month& ym)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{ym/0};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt, const month_day& md)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{md/nanyear};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const year_month_day& ymd)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{ymd};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits, class Rep, class Period>
inline
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const std::chrono::duration<Rep, Period>& d)
{
    using Duration = std::chrono::duration<Rep, Period>;
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    fields<CT> fds{hh_mm_ss<CT>{d}};
    return to_stream(os, fmt, fds);
}

template <class CharT, class Traits, class Duration>
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const local_time<Duration>& tp, const std::string* abbrev = nullptr,
          const std::chrono::seconds* offset_sec = nullptr)
{
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    auto ld = std::chrono::time_point_cast<days>(tp);
    fields<CT> fds;
    if (ld <= tp)
        fds = fields<CT>{year_month_day{ld}, hh_mm_ss<CT>{tp-local_seconds{ld}}};
    else
        fds = fields<CT>{year_month_day{ld - days{1}},
                         hh_mm_ss<CT>{days{1} - (local_seconds{ld} - tp)}};
    return to_stream(os, fmt, fds, abbrev, offset_sec);
}

template <class CharT, class Traits, class Duration>
std::basic_ostream<CharT, Traits>&
to_stream(std::basic_ostream<CharT, Traits>& os, const CharT* fmt,
          const sys_time<Duration>& tp)
{
    using std::chrono::seconds;
    using CT = typename std::common_type<Duration, seconds>::type;
    const std::string abbrev("UTC");
    CONSTDATA seconds offset{0};
    auto sd = std::chrono::time_point_cast<days>(tp);
    fields<CT> fds;
    if (sd <= tp)
        fds = fields<CT>{year_month_day{sd}, hh_mm_ss<CT>{tp-sys_seconds{sd}}};
    else
        fds = fields<CT>{year_month_day{sd - days{1}},
                         hh_mm_ss<CT>{days{1} - (sys_seconds{sd} - tp)}};
    return to_stream(os, fmt, fds, &abbrev, &offset);
}

// format

template <class CharT, class Streamable>
auto
format(const std::locale& loc, const CharT* fmt, const Streamable& tp)
    -> decltype(to_stream(std::declval<std::basic_ostream<CharT>&>(), fmt, tp),
                std::basic_string<CharT>{})
{
    std::basic_ostringstream<CharT> os;
    os.exceptions(std::ios::failbit | std::ios::badbit);
    os.imbue(loc);
    to_stream(os, fmt, tp);
    return os.str();
}

template <class CharT, class Streamable>
auto
format(const CharT* fmt, const Streamable& tp)
    -> decltype(to_stream(std::declval<std::basic_ostream<CharT>&>(), fmt, tp),
                std::basic_string<CharT>{})
{
    std::basic_ostringstream<CharT> os;
    os.exceptions(std::ios::failbit | std::ios::badbit);
    to_stream(os, fmt, tp);
    return os.str();
}

template <class CharT, class Traits, class Alloc, class Streamable>
auto
format(const std::locale& loc, const std::basic_string<CharT, Traits, Alloc>& fmt,
       const Streamable& tp)
    -> decltype(to_stream(std::declval<std::basic_ostream<CharT, Traits>&>(), fmt.c_str(), tp),
                std::basic_string<CharT, Traits, Alloc>{})
{
    std::basic_ostringstream<CharT, Traits, Alloc> os;
    os.exceptions(std::ios::failbit | std::ios::badbit);
    os.imbue(loc);
    to_stream(os, fmt.c_str(), tp);
    return os.str();
}

template <class CharT, class Traits, class Alloc, class Streamable>
auto
format(const std::basic_string<CharT, Traits, Alloc>& fmt, const Streamable& tp)
    -> decltype(to_stream(std::declval<std::basic_ostream<CharT, Traits>&>(), fmt.c_str(), tp),
                std::basic_string<CharT, Traits, Alloc>{})
{
    std::basic_ostringstream<CharT, Traits, Alloc> os;
    os.exceptions(std::ios::failbit | std::ios::badbit);
    to_stream(os, fmt.c_str(), tp);
    return os.str();
}

// parse

namespace detail
{

template <class CharT, class Traits>
bool
read_char(std::basic_istream<CharT, Traits>& is, CharT fmt, std::ios::iostate& err)
{
    auto ic = is.get();
    if (Traits::eq_int_type(ic, Traits::eof()) ||
       !Traits::eq(Traits::to_char_type(ic), fmt))
    {
        err |= std::ios::failbit;
        is.setstate(std::ios::failbit);
        return false;
    }
    return true;
}

template <class CharT, class Traits>
unsigned
read_unsigned(std::basic_istream<CharT, Traits>& is, unsigned m = 1, unsigned M = 10)
{
    unsigned x = 0;
    unsigned count = 0;
    while (true)
    {
        auto ic = is.peek();
        if (Traits::eq_int_type(ic, Traits::eof()))
            break;
        auto c = static_cast<char>(Traits::to_char_type(ic));
        if (!('0' <= c && c <= '9'))
            break;
        (void)is.get();
        ++count;
        x = 10*x + static_cast<unsigned>(c - '0');
        if (count == M)
            break;
    }
    if (count < m)
        is.setstate(std::ios::failbit);
    return x;
}

template <class CharT, class Traits>
int
read_signed(std::basic_istream<CharT, Traits>& is, unsigned m = 1, unsigned M = 10)
{
    auto ic = is.peek();
    if (!Traits::eq_int_type(ic, Traits::eof()))
    {
        auto c = static_cast<char>(Traits::to_char_type(ic));
        if (('0' <= c && c <= '9') || c == '-' || c == '+')
        {
            if (c == '-' || c == '+')
            {
                (void)is.get();
                --M;
            }
            auto x = static_cast<int>(read_unsigned(is, std::max(m, 1u), M));
            if (!is.fail())
            {
                if (c == '-')
                    x = -x;
                return x;
            }
        }
    }
    if (m > 0)
        is.setstate(std::ios::failbit);
    return 0;
}

template <class CharT, class Traits>
long double
read_long_double(std::basic_istream<CharT, Traits>& is, unsigned m = 1, unsigned M = 10)
{
    unsigned count = 0;
    unsigned fcount = 0;
    unsigned long long i = 0;
    unsigned long long f = 0;
    bool parsing_fraction = false;
#if ONLY_C_LOCALE
    typename Traits::int_type decimal_point = '.';
#else
    auto decimal_point = Traits::to_int_type(
        std::use_facet<std::numpunct<CharT>>(is.getloc()).decimal_point());
#endif
    while (true)
    {
        auto ic = is.peek();
        if (Traits::eq_int_type(ic, Traits::eof()))
            break;
        if (Traits::eq_int_type(ic, decimal_point))
        {
            decimal_point = Traits::eof();
            parsing_fraction = true;
        }
        else
        {
            auto c = static_cast<char>(Traits::to_char_type(ic));
            if (!('0' <= c && c <= '9'))
                break;
            if (!parsing_fraction)
            {
                i = 10*i + static_cast<unsigned>(c - '0');
            }
            else
            {
                f = 10*f + static_cast<unsigned>(c - '0');
                ++fcount;
            }
        }
        (void)is.get();
        if (++count == M)
            break;
    }
    if (count < m)
    {
        is.setstate(std::ios::failbit);
        return 0;
    }
    return static_cast<long double>(i) + static_cast<long double>(f)/std::pow(10.L, fcount);
}

struct rs
{
    int& i;
    unsigned m;
    unsigned M;
};

struct ru
{
    int& i;
    unsigned m;
    unsigned M;
};

struct rld
{
    long double& i;
    unsigned m;
    unsigned M;
};

template <class CharT, class Traits>
void
read(std::basic_istream<CharT, Traits>&)
{
}

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, CharT a0, Args&& ...args);

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, rs a0, Args&& ...args);

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, ru a0, Args&& ...args);

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, int a0, Args&& ...args);

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, rld a0, Args&& ...args);

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, CharT a0, Args&& ...args)
{
    // No-op if a0 == CharT{}
    if (a0 != CharT{})
    {
        auto ic = is.peek();
        if (Traits::eq_int_type(ic, Traits::eof()))
        {
            is.setstate(std::ios::failbit | std::ios::eofbit);
            return;
        }
        if (!Traits::eq(Traits::to_char_type(ic), a0))
        {
            is.setstate(std::ios::failbit);
            return;
        }
        (void)is.get();
    }
    read(is, std::forward<Args>(args)...);
}

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, rs a0, Args&& ...args)
{
    auto x = read_signed(is, a0.m, a0.M);
    if (is.fail())
        return;
    a0.i = x;
    read(is, std::forward<Args>(args)...);
}

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, ru a0, Args&& ...args)
{
    auto x = read_unsigned(is, a0.m, a0.M);
    if (is.fail())
        return;
    a0.i = static_cast<int>(x);
    read(is, std::forward<Args>(args)...);
}

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, int a0, Args&& ...args)
{
    if (a0 != -1)
    {
        auto u = static_cast<unsigned>(a0);
        CharT buf[std::numeric_limits<unsigned>::digits10+2u] = {};
        auto e = buf;
        do
        {
            *e++ = static_cast<CharT>(CharT(u % 10) + CharT{'0'});
            u /= 10;
        } while (u > 0);
#if defined(__GNUC__) && __GNUC__ >= 11
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
        std::reverse(buf, e);
#if defined(__GNUC__) && __GNUC__ >= 11
#pragma GCC diagnostic pop
#endif
        for (auto p = buf; p != e && is.rdstate() == std::ios::goodbit; ++p)
            read(is, *p);
    }
    if (is.rdstate() == std::ios::goodbit)
        read(is, std::forward<Args>(args)...);
}

template <class CharT, class Traits, class ...Args>
void
read(std::basic_istream<CharT, Traits>& is, rld a0, Args&& ...args)
{
    auto x = read_long_double(is, a0.m, a0.M);
    if (is.fail())
        return;
    a0.i = x;
    read(is, std::forward<Args>(args)...);
}

template <class T, class CharT, class Traits>
inline
void
checked_set(T& value, T from, T not_a_value, std::basic_ios<CharT, Traits>& is)
{
    if (!is.fail())
    {
        if (value == not_a_value)
            value = std::move(from);
        else if (value != from)
            is.setstate(std::ios::failbit);
    }
}

}  // namespace detail;

template <class CharT, class Traits, class Duration, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            fields<Duration>& fds, std::basic_string<CharT, Traits, Alloc>* abbrev,
            std::chrono::minutes* offset)
{
    using std::numeric_limits;
    using std::ios;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::minutes;
    using std::chrono::hours;
    using detail::round_i;
    typename std::basic_istream<CharT, Traits>::sentry ok{is, true};
    if (ok)
    {
        date::detail::save_istream<CharT, Traits> ss(is);
        is.fill(' ');
        is.flags(std::ios::skipws | std::ios::dec);
        is.width(0);
#if !ONLY_C_LOCALE
        auto& f = std::use_facet<std::time_get<CharT>>(is.getloc());
        std::tm tm{};
#endif
        const CharT* command = nullptr;
        auto modified = CharT{};
        auto width = -1;

        CONSTDATA int not_a_year = numeric_limits<short>::min();
        CONSTDATA int not_a_2digit_year = 100;
        CONSTDATA int not_a_century = numeric_limits<int>::min();
        CONSTDATA int not_a_month = 0;
        CONSTDATA int not_a_day = 0;
        CONSTDATA int not_a_hour = numeric_limits<int>::min();
        CONSTDATA int not_a_hour_12_value = 0;
        CONSTDATA int not_a_minute = not_a_hour;
        CONSTDATA Duration not_a_second = Duration::min();
        CONSTDATA int not_a_doy = -1;
        CONSTDATA int not_a_weekday = 8;
        CONSTDATA int not_a_week_num = 100;
        CONSTDATA int not_a_ampm = -1;
        CONSTDATA minutes not_a_offset = minutes::min();

        int Y = not_a_year;             // c, F, Y                   *
        int y = not_a_2digit_year;      // D, x, y                   *
        int g = not_a_2digit_year;      // g                         *
        int G = not_a_year;             // G                         *
        int C = not_a_century;          // C                         *
        int m = not_a_month;            // b, B, h, m, c, D, F, x    *
        int d = not_a_day;              // c, d, D, e, F, x          *
        int j = not_a_doy;              // j                         *
        int wd = not_a_weekday;         // a, A, u, w                *
        int H = not_a_hour;             // c, H, R, T, X             *
        int I = not_a_hour_12_value;    // I, r                      *
        int p = not_a_ampm;             // p, r                      *
        int M = not_a_minute;           // c, M, r, R, T, X          *
        Duration s = not_a_second;      // c, r, S, T, X             *
        int U = not_a_week_num;         // U                         *
        int V = not_a_week_num;         // V                         *
        int W = not_a_week_num;         // W                         *
        std::basic_string<CharT, Traits, Alloc> temp_abbrev;  // Z   *
        minutes temp_offset = not_a_offset;  // z                    *

        using detail::read;
        using detail::rs;
        using detail::ru;
        using detail::rld;
        using detail::checked_set;
        for (; *fmt != CharT{} && !is.fail(); ++fmt)
        {
            switch (*fmt)
            {
            case 'a':
            case 'A':
            case 'u':
            case 'w':  // wd:  a, A, u, w
                if (command)
                {
                    int trial_wd = not_a_weekday;
                    if (*fmt == 'a' || *fmt == 'A')
                    {
                        if (modified == CharT{})
                        {
#if !ONLY_C_LOCALE
                            ios::iostate err = ios::goodbit;
                            f.get(is, nullptr, is, err, &tm, command, fmt+1);
                            is.setstate(err);
                            if (!is.fail())
                                trial_wd = tm.tm_wday;
#else
                            auto nm = detail::weekday_names();
                            auto i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                            if (!is.fail())
                                trial_wd = i % 7;
#endif
                        }
                        else
                            read(is, CharT{'%'}, width, modified, *fmt);
                    }
                    else  // *fmt == 'u' || *fmt == 'w'
                    {
#if !ONLY_C_LOCALE
                        if (modified == CharT{})
#else
                        if (modified != CharT{'E'})
#endif
                        {
                            read(is, ru{trial_wd, 1, width == -1 ?
                                                      1u : static_cast<unsigned>(width)});
                            if (!is.fail())
                            {
                                if (*fmt == 'u')
                                {
                                    if (!(1 <= trial_wd && trial_wd <= 7))
                                    {
                                        trial_wd = not_a_weekday;
                                        is.setstate(ios::failbit);
                                    }
                                    else if (trial_wd == 7)
                                        trial_wd = 0;
                                }
                                else  // *fmt == 'w'
                                {
                                    if (!(0 <= trial_wd && trial_wd <= 6))
                                    {
                                        trial_wd = not_a_weekday;
                                        is.setstate(ios::failbit);
                                    }
                                }
                            }
                        }
#if !ONLY_C_LOCALE
                        else if (modified == CharT{'O'})
                        {
                            ios::iostate err = ios::goodbit;
                            f.get(is, nullptr, is, err, &tm, command, fmt+1);
                            is.setstate(err);
                            if (!is.fail())
                                trial_wd = tm.tm_wday;
                        }
#endif
                        else
                            read(is, CharT{'%'}, width, modified, *fmt);
                    }
                    if (trial_wd != not_a_weekday)
                        checked_set(wd, trial_wd, not_a_weekday, is);
                }
                else  // !command
                    read(is, *fmt);
                command = nullptr;
                width = -1;
                modified = CharT{};
                break;
            case 'b':
            case 'B':
            case 'h':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int ttm = not_a_month;
#if !ONLY_C_LOCALE
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            ttm = tm.tm_mon + 1;
                        is.setstate(err);
#else
                        auto nm = detail::month_names();
                        auto i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                        if (!is.fail())
                            ttm = i % 12 + 1;
#endif
                        checked_set(m, ttm, not_a_month, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'c':
                if (command)
                {
                    if (modified != CharT{'O'})
                    {
#if !ONLY_C_LOCALE
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                        {
                            checked_set(Y, tm.tm_year + 1900, not_a_year, is);
                            checked_set(m, tm.tm_mon + 1, not_a_month, is);
                            checked_set(d, tm.tm_mday, not_a_day, is);
                            checked_set(H, tm.tm_hour, not_a_hour, is);
                            checked_set(M, tm.tm_min, not_a_minute, is);
                            checked_set(s, duration_cast<Duration>(seconds{tm.tm_sec}),
                                        not_a_second, is);
                        }
                        is.setstate(err);
#else
                        // "%a %b %e %T %Y"
                        auto nm = detail::weekday_names();
                        auto i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                        checked_set(wd, static_cast<int>(i % 7), not_a_weekday, is);
                        ws(is);
                        nm = detail::month_names();
                        i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                        checked_set(m, static_cast<int>(i % 12 + 1), not_a_month, is);
                        ws(is);
                        int td = not_a_day;
                        read(is, rs{td, 1, 2});
                        checked_set(d, td, not_a_day, is);
                        ws(is);
                        using dfs = detail::decimal_format_seconds<Duration>;
                        CONSTDATA auto w = Duration::period::den == 1 ? 2 : 3 + dfs::width;
                        int tH;
                        int tM;
                        long double S{};
                        read(is, ru{tH, 1, 2}, CharT{':'}, ru{tM, 1, 2},
                                               CharT{':'}, rld{S, 1, w});
                        checked_set(H, tH, not_a_hour, is);
                        checked_set(M, tM, not_a_minute, is);
                        checked_set(s, round_i<Duration>(duration<long double>{S}),
                                    not_a_second, is);
                        ws(is);
                        int tY = not_a_year;
                        read(is, rs{tY, 1, 4u});
                        checked_set(Y, tY, not_a_year, is);
#endif
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'x':
                if (command)
                {
                    if (modified != CharT{'O'})
                    {
#if !ONLY_C_LOCALE
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                        {
                            checked_set(Y, tm.tm_year + 1900, not_a_year, is);
                            checked_set(m, tm.tm_mon + 1, not_a_month, is);
                            checked_set(d, tm.tm_mday, not_a_day, is);
                        }
                        is.setstate(err);
#else
                        // "%m/%d/%y"
                        int ty = not_a_2digit_year;
                        int tm = not_a_month;
                        int td = not_a_day;
                        read(is, ru{tm, 1, 2}, CharT{'/'}, ru{td, 1, 2}, CharT{'/'},
                                 rs{ty, 1, 2});
                        checked_set(y, ty, not_a_2digit_year, is);
                        checked_set(m, tm, not_a_month, is);
                        checked_set(d, td, not_a_day, is);
#endif
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'X':
                if (command)
                {
                    if (modified != CharT{'O'})
                    {
#if !ONLY_C_LOCALE
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                        {
                            checked_set(H, tm.tm_hour, not_a_hour, is);
                            checked_set(M, tm.tm_min, not_a_minute, is);
                            checked_set(s, duration_cast<Duration>(seconds{tm.tm_sec}),
                                        not_a_second, is);
                        }
                        is.setstate(err);
#else
                        // "%T"
                        using dfs = detail::decimal_format_seconds<Duration>;
                        CONSTDATA auto w = Duration::period::den == 1 ? 2 : 3 + dfs::width;
                        int tH = not_a_hour;
                        int tM = not_a_minute;
                        long double S{};
                        read(is, ru{tH, 1, 2}, CharT{':'}, ru{tM, 1, 2},
                                               CharT{':'}, rld{S, 1, w});
                        checked_set(H, tH, not_a_hour, is);
                        checked_set(M, tM, not_a_minute, is);
                        checked_set(s, round_i<Duration>(duration<long double>{S}),
                                    not_a_second, is);
#endif
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'C':
                if (command)
                {
                    int tC = not_a_century;
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
                    {
#endif
                        read(is, rs{tC, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
#if !ONLY_C_LOCALE
                    }
                    else
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                        {
                            auto tY = tm.tm_year + 1900;
                            tC = (tY >= 0 ? tY : tY-99) / 100;
                        }
                        is.setstate(err);
                    }
#endif
                    checked_set(C, tC, not_a_century, is);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'D':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tn = not_a_month;
                        int td = not_a_day;
                        int ty = not_a_2digit_year;
                        read(is, ru{tn, 1, 2}, CharT{'\0'}, CharT{'/'}, CharT{'\0'},
                                 ru{td, 1, 2}, CharT{'\0'}, CharT{'/'}, CharT{'\0'},
                                 rs{ty, 1, 2});
                        checked_set(y, ty, not_a_2digit_year, is);
                        checked_set(m, tn, not_a_month, is);
                        checked_set(d, td, not_a_day, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'F':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tY = not_a_year;
                        int tn = not_a_month;
                        int td = not_a_day;
                        read(is, rs{tY, 1, width == -1 ? 4u : static_cast<unsigned>(width)},
                                 CharT{'-'}, ru{tn, 1, 2}, CharT{'-'}, ru{td, 1, 2});
                        checked_set(Y, tY, not_a_year, is);
                        checked_set(m, tn, not_a_month, is);
                        checked_set(d, td, not_a_day, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'd':
            case 'e':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#else
                    if (modified != CharT{'E'})
#endif
                    {
                        int td = not_a_day;
                        read(is, rs{td, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(d, td, not_a_day, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        command = nullptr;
                        width = -1;
                        modified = CharT{};
                        if ((err & ios::failbit) == 0)
                            checked_set(d, tm.tm_mday, not_a_day, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'H':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#else
                    if (modified != CharT{'E'})
#endif
                    {
                        int tH = not_a_hour;
                        read(is, ru{tH, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(H, tH, not_a_hour, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(H, tm.tm_hour, not_a_hour, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'I':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tI = not_a_hour_12_value;
                        // reads in an hour into I, but most be in [1, 12]
                        read(is, rs{tI, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        if (!(1 <= tI && tI <= 12))
                            is.setstate(ios::failbit);
                        checked_set(I, tI, not_a_hour_12_value, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
               break;
            case 'j':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tj = not_a_doy;
                        read(is, ru{tj, 1, width == -1 ? 3u : static_cast<unsigned>(width)});
                        checked_set(j, tj, not_a_doy, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'M':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#else
                    if (modified != CharT{'E'})
#endif
                    {
                        int tM = not_a_minute;
                        read(is, ru{tM, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(M, tM, not_a_minute, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(M, tm.tm_min, not_a_minute, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'm':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#else
                    if (modified != CharT{'E'})
#endif
                    {
                        int tn = not_a_month;
                        read(is, rs{tn, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(m, tn, not_a_month, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(m, tm.tm_mon + 1, not_a_month, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'n':
            case 't':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        // %n matches a single white space character
                        // %t matches 0 or 1 white space characters
                        auto ic = is.peek();
                        if (Traits::eq_int_type(ic, Traits::eof()))
                        {
                            ios::iostate err = ios::eofbit;
                            if (*fmt == 'n')
                                err |= ios::failbit;
                            is.setstate(err);
                            break;
                        }
                        if (isspace(ic))
                        {
                            (void)is.get();
                        }
                        else if (*fmt == 'n')
                            is.setstate(ios::failbit);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'p':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tp = not_a_ampm;
#if !ONLY_C_LOCALE
                        tm = std::tm{};
                        tm.tm_hour = 1;
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        is.setstate(err);
                        if (tm.tm_hour == 1)
                            tp = 0;
                        else if (tm.tm_hour == 13)
                            tp = 1;
                        else
                            is.setstate(err);
#else
                        auto nm = detail::ampm_names();
                        auto i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                        tp = static_cast<decltype(tp)>(i);
#endif
                        checked_set(p, tp, not_a_ampm, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);

               break;
            case 'r':
                if (command)
                {
                    if (modified == CharT{})
                    {
#if !ONLY_C_LOCALE
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                        {
                            checked_set(H, tm.tm_hour, not_a_hour, is);
                            checked_set(M, tm.tm_min, not_a_hour, is);
                            checked_set(s, duration_cast<Duration>(seconds{tm.tm_sec}),
                                        not_a_second, is);
                        }
                        is.setstate(err);
#else
                        // "%I:%M:%S %p"
                        using dfs = detail::decimal_format_seconds<Duration>;
                        CONSTDATA auto w = Duration::period::den == 1 ? 2 : 3 + dfs::width;
                        long double S{};
                        int tI = not_a_hour_12_value;
                        int tM = not_a_minute;
                        read(is, ru{tI, 1, 2}, CharT{':'}, ru{tM, 1, 2},
                                               CharT{':'}, rld{S, 1, w});
                        checked_set(I, tI, not_a_hour_12_value, is);
                        checked_set(M, tM, not_a_minute, is);
                        checked_set(s, round_i<Duration>(duration<long double>{S}),
                                    not_a_second, is);
                        ws(is);
                        auto nm = detail::ampm_names();
                        auto i = detail::scan_keyword(is, nm.first, nm.second) - nm.first;
                        checked_set(p, static_cast<int>(i), not_a_ampm, is);
#endif
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'R':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tH = not_a_hour;
                        int tM = not_a_minute;
                        read(is, ru{tH, 1, 2}, CharT{'\0'}, CharT{':'}, CharT{'\0'},
                                 ru{tM, 1, 2}, CharT{'\0'});
                        checked_set(H, tH, not_a_hour, is);
                        checked_set(M, tM, not_a_minute, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'S':
                if (command)
                {
 #if !ONLY_C_LOCALE
                   if (modified == CharT{})
#else
                   if (modified != CharT{'E'})
#endif
                    {
                        using dfs = detail::decimal_format_seconds<Duration>;
                        CONSTDATA auto w = Duration::period::den == 1 ? 2 : 3 + dfs::width;
                        long double S{};
                        read(is, rld{S, 1, width == -1 ? w : static_cast<unsigned>(width)});
                        checked_set(s, round_i<Duration>(duration<long double>{S}),
                                    not_a_second, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'O'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(s, duration_cast<Duration>(seconds{tm.tm_sec}),
                                        not_a_second, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'T':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        using dfs = detail::decimal_format_seconds<Duration>;
                        CONSTDATA auto w = Duration::period::den == 1 ? 2 : 3 + dfs::width;
                        int tH = not_a_hour;
                        int tM = not_a_minute;
                        long double S{};
                        read(is, ru{tH, 1, 2}, CharT{':'}, ru{tM, 1, 2},
                                               CharT{':'}, rld{S, 1, w});
                        checked_set(H, tH, not_a_hour, is);
                        checked_set(M, tM, not_a_minute, is);
                        checked_set(s, round_i<Duration>(duration<long double>{S}),
                                    not_a_second, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'Y':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#else
                    if (modified != CharT{'O'})
#endif
                    {
                        int tY = not_a_year;
                        read(is, rs{tY, 1, width == -1 ? 4u : static_cast<unsigned>(width)});
                        checked_set(Y, tY, not_a_year, is);
                    }
#if !ONLY_C_LOCALE
                    else if (modified == CharT{'E'})
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(Y, tm.tm_year + 1900, not_a_year, is);
                        is.setstate(err);
                    }
#endif
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'y':
                if (command)
                {
#if !ONLY_C_LOCALE
                    if (modified == CharT{})
#endif
                    {
                        int ty = not_a_2digit_year;
                        read(is, ru{ty, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(y, ty, not_a_2digit_year, is);
                    }
#if !ONLY_C_LOCALE
                    else
                    {
                        ios::iostate err = ios::goodbit;
                        f.get(is, nullptr, is, err, &tm, command, fmt+1);
                        if ((err & ios::failbit) == 0)
                            checked_set(Y, tm.tm_year + 1900, not_a_year, is);
                        is.setstate(err);
                    }
#endif
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'g':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tg = not_a_2digit_year;
                        read(is, ru{tg, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(g, tg, not_a_2digit_year, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'G':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tG = not_a_year;
                        read(is, rs{tG, 1, width == -1 ? 4u : static_cast<unsigned>(width)});
                        checked_set(G, tG, not_a_year, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'U':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tU = not_a_week_num;
                        read(is, ru{tU, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(U, tU, not_a_week_num, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'V':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tV = not_a_week_num;
                        read(is, ru{tV, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(V, tV, not_a_week_num, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'W':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        int tW = not_a_week_num;
                        read(is, ru{tW, 1, width == -1 ? 2u : static_cast<unsigned>(width)});
                        checked_set(W, tW, not_a_week_num, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'E':
            case 'O':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        modified = *fmt;
                    }
                    else
                    {
                        read(is, CharT{'%'}, width, modified, *fmt);
                        command = nullptr;
                        width = -1;
                        modified = CharT{};
                    }
                }
                else
                    read(is, *fmt);
                break;
            case '%':
                if (command)
                {
                    if (modified == CharT{})
                        read(is, *fmt);
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    command = fmt;
                break;
            case 'z':
                if (command)
                {
                    int tH, tM;
                    minutes toff = not_a_offset;
                    bool neg = false;
                    auto ic = is.peek();
                    if (!Traits::eq_int_type(ic, Traits::eof()))
                    {
                        auto c = static_cast<char>(Traits::to_char_type(ic));
                        if (c == '-')
                        {
                            neg = true;
                            (void)is.get();
                        }
                        else if (c == '+')
                            (void)is.get();
                    }
                    if (modified == CharT{})
                    {
                        read(is, rs{tH, 2, 2});
                        if (!is.fail())
                            toff = hours{std::abs(tH)};
                        if (is.good())
                        {
                            ic = is.peek();
                            if (!Traits::eq_int_type(ic, Traits::eof()))
                            {
                                auto c = static_cast<char>(Traits::to_char_type(ic));
                                if ('0' <= c && c <= '9')
                                {
                                    read(is, ru{tM, 2, 2});
                                    if (!is.fail())
                                        toff += minutes{tM};
                                }
                            }
                        }
                    }
                    else
                    {
                        read(is, rs{tH, 1, 2});
                        if (!is.fail())
                            toff = hours{std::abs(tH)};
                        if (is.good())
                        {
                            ic = is.peek();
                            if (!Traits::eq_int_type(ic, Traits::eof()))
                            {
                                auto c = static_cast<char>(Traits::to_char_type(ic));
                                if (c == ':')
                                {
                                    (void)is.get();
                                    read(is, ru{tM, 2, 2});
                                    if (!is.fail())
                                        toff += minutes{tM};
                                }
                            }
                        }
                    }
                    if (neg)
                        toff = -toff;
                    checked_set(temp_offset, toff, not_a_offset, is);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            case 'Z':
                if (command)
                {
                    if (modified == CharT{})
                    {
                        std::basic_string<CharT, Traits, Alloc> buf;
                        while (is.rdstate() == std::ios::goodbit)
                        {
                            auto i = is.rdbuf()->sgetc();
                            if (Traits::eq_int_type(i, Traits::eof()))
                            {
                                is.setstate(ios::eofbit);
                                break;
                            }
                            auto wc = Traits::to_char_type(i);
                            auto c = static_cast<char>(wc);
                            // is c a valid time zone name or abbreviation character?
                            if (!(CharT{1} < wc && wc < CharT{127}) || !(isalnum(c) ||
                                    c == '_' || c == '/' || c == '-' || c == '+'))
                                break;
                            buf.push_back(c);
                            is.rdbuf()->sbumpc();
                        }
                        if (buf.empty())
                            is.setstate(ios::failbit);
                        checked_set(temp_abbrev, buf, {}, is);
                    }
                    else
                        read(is, CharT{'%'}, width, modified, *fmt);
                    command = nullptr;
                    width = -1;
                    modified = CharT{};
                }
                else
                    read(is, *fmt);
                break;
            default:
                if (command)
                {
                    if (width == -1 && modified == CharT{} && '0' <= *fmt && *fmt <= '9')
                    {
                        width = static_cast<char>(*fmt) - '0';
                        while ('0' <= fmt[1] && fmt[1] <= '9')
                            width = 10*width + static_cast<char>(*++fmt) - '0';
                    }
                    else
                    {
                        if (modified == CharT{})
                            read(is, CharT{'%'}, width, *fmt);
                        else
                            read(is, CharT{'%'}, width, modified, *fmt);
                        command = nullptr;
                        width = -1;
                        modified = CharT{};
                    }
                }
                else  // !command
                {
                    if (isspace(static_cast<unsigned char>(*fmt)))
                    {
                        // space matches 0 or more white space characters
                        if (is.good())
                           ws(is);
                    }
                    else
                        read(is, *fmt);
                }
                break;
            }
        }
        // is.fail() || *fmt == CharT{}
        if (is.rdstate() == ios::goodbit && command)
        {
            if (modified == CharT{})
                read(is, CharT{'%'}, width);
            else
                read(is, CharT{'%'}, width, modified);
        }
        if (!is.fail())
        {
            if (y != not_a_2digit_year)
            {
                // Convert y and an optional C to Y
                if (!(0 <= y && y <= 99))
                    goto broken;
                if (C == not_a_century)
                {
                    if (Y == not_a_year)
                    {
                        if (y >= 69)
                            C = 19;
                        else
                            C = 20;
                    }
                    else
                    {
                        C = (Y >= 0 ? Y : Y-100) / 100;
                    }
                }
                int tY;
                if (C >= 0)
                    tY = 100*C + y;
                else
                    tY = 100*(C+1) - (y == 0 ? 100 : y);
                if (Y != not_a_year && Y != tY)
                    goto broken;
                Y = tY;
            }
            if (g != not_a_2digit_year)
            {
                // Convert g and an optional C to G
                if (!(0 <= g && g <= 99))
                    goto broken;
                if (C == not_a_century)
                {
                    if (G == not_a_year)
                    {
                        if (g >= 69)
                            C = 19;
                        else
                            C = 20;
                    }
                    else
                    {
                        C = (G >= 0 ? G : G-100) / 100;
                    }
                }
                int tG;
                if (C >= 0)
                    tG = 100*C + g;
                else
                    tG = 100*(C+1) - (g == 0 ? 100 : g);
                if (G != not_a_year && G != tG)
                    goto broken;
                G = tG;
            }
            if (Y < static_cast<int>(year::min()) || Y > static_cast<int>(year::max()))
                Y = not_a_year;
            bool computed = false;
            if (G != not_a_year && V != not_a_week_num && wd != not_a_weekday)
            {
                year_month_day ymd_trial = sys_days(year{G-1}/December/Thursday[last]) +
                                           (Monday-Thursday) + weeks{V-1} +
                                           (weekday{static_cast<unsigned>(wd)}-Monday);
                if (Y == not_a_year)
                    Y = static_cast<int>(ymd_trial.year());
                else if (year{Y} != ymd_trial.year())
                    goto broken;
                if (m == not_a_month)
                    m = static_cast<int>(static_cast<unsigned>(ymd_trial.month()));
                else if (month(static_cast<unsigned>(m)) != ymd_trial.month())
                    goto broken;
                if (d == not_a_day)
                    d = static_cast<int>(static_cast<unsigned>(ymd_trial.day()));
                else if (day(static_cast<unsigned>(d)) != ymd_trial.day())
                    goto broken;
                computed = true;
            }
            if (Y != not_a_year && U != not_a_week_num && wd != not_a_weekday)
            {
                year_month_day ymd_trial = sys_days(year{Y}/January/Sunday[1]) +
                                           weeks{U-1} +
                                           (weekday{static_cast<unsigned>(wd)} - Sunday);
                if (year{Y} != ymd_trial.year())
                    goto broken;
                if (m == not_a_month)
                    m = static_cast<int>(static_cast<unsigned>(ymd_trial.month()));
                else if (month(static_cast<unsigned>(m)) != ymd_trial.month())
                    goto broken;
                if (d == not_a_day)
                    d = static_cast<int>(static_cast<unsigned>(ymd_trial.day()));
                else if (day(static_cast<unsigned>(d)) != ymd_trial.day())
                    goto broken;
                computed = true;
            }
            if (Y != not_a_year && W != not_a_week_num && wd != not_a_weekday)
            {
                year_month_day ymd_trial = sys_days(year{Y}/January/Monday[1]) +
                                           weeks{W-1} +
                                           (weekday{static_cast<unsigned>(wd)} - Monday);
                if (year{Y} != ymd_trial.year())
                    goto broken;
                if (m == not_a_month)
                    m = static_cast<int>(static_cast<unsigned>(ymd_trial.month()));
                else if (month(static_cast<unsigned>(m)) != ymd_trial.month())
                    goto broken;
                if (d == not_a_day)
                    d = static_cast<int>(static_cast<unsigned>(ymd_trial.day()));
                else if (day(static_cast<unsigned>(d)) != ymd_trial.day())
                    goto broken;
                computed = true;
            }
            if (j != not_a_doy && Y != not_a_year)
            {
                auto ymd_trial = year_month_day{local_days(year{Y}/1/1) + days{j-1}};
                if (m == not_a_month)
                    m = static_cast<int>(static_cast<unsigned>(ymd_trial.month()));
                else if (month(static_cast<unsigned>(m)) != ymd_trial.month())
                    goto broken;
                if (d == not_a_day)
                    d = static_cast<int>(static_cast<unsigned>(ymd_trial.day()));
                else if (day(static_cast<unsigned>(d)) != ymd_trial.day())
                    goto broken;
                j = not_a_doy;
            }
            auto ymd = year{Y}/m/d;
            if (ymd.ok())
            {
                if (wd == not_a_weekday)
                    wd = static_cast<int>((weekday(sys_days(ymd)) - Sunday).count());
                else if (wd != static_cast<int>((weekday(sys_days(ymd)) - Sunday).count()))
                    goto broken;
                if (!computed)
                {
                    if (G != not_a_year || V != not_a_week_num)
                    {
                        sys_days sd = ymd;
                        auto G_trial = year_month_day{sd + days{3}}.year();
                        auto start = sys_days((G_trial - years{1})/December/Thursday[last]) +
                                     (Monday - Thursday);
                        if (sd < start)
                        {
                            --G_trial;
                            if (V != not_a_week_num)
                                start = sys_days((G_trial - years{1})/December/Thursday[last])
                                        + (Monday - Thursday);
                        }
                        if (G != not_a_year && G != static_cast<int>(G_trial))
                            goto broken;
                        if (V != not_a_week_num)
                        {
                            auto V_trial = duration_cast<weeks>(sd - start).count() + 1;
                            if (V != V_trial)
                                goto broken;
                        }
                    }
                    if (U != not_a_week_num)
                    {
                        auto start = sys_days(Sunday[1]/January/ymd.year());
                        auto U_trial = floor<weeks>(sys_days(ymd) - start).count() + 1;
                        if (U != U_trial)
                            goto broken;
                    }
                    if (W != not_a_week_num)
                    {
                        auto start = sys_days(Monday[1]/January/ymd.year());
                        auto W_trial = floor<weeks>(sys_days(ymd) - start).count() + 1;
                        if (W != W_trial)
                            goto broken;
                    }
                }
            }
            fds.ymd = ymd;
            if (I != not_a_hour_12_value)
            {
                if (!(1 <= I && I <= 12))
                    goto broken;
                if (p != not_a_ampm)
                {
                    // p is in [0, 1] == [AM, PM]
                    // Store trial H in I
                    if (I == 12)
                        --p;
                    I += p*12;
                    // Either set H from I or make sure H and I are consistent
                    if (H == not_a_hour)
                        H = I;
                    else if (I != H)
                        goto broken;
                }
                else  // p == not_a_ampm
                {
                    // if H, make sure H and I could be consistent
                    if (H != not_a_hour)
                    {
                        if (I == 12)
                        {
                            if (H != 0 && H != 12)
                                goto broken;
                        }
                        else if (!(I == H || I == H+12))
                        {
                            goto broken;
                        }
                    }
                    else  // I is ambiguous, AM or PM?
                        goto broken;
                }
            }
            if (H != not_a_hour)
            {
                fds.has_tod = true;
                fds.tod = hh_mm_ss<Duration>{hours{H}};
            }
            if (M != not_a_minute)
            {
                fds.has_tod = true;
                fds.tod.m_ = minutes{M};
            }
            if (s != not_a_second)
            {
                fds.has_tod = true;
                fds.tod.s_ = detail::decimal_format_seconds<Duration>{s};
            }
            if (j != not_a_doy)
            {
                fds.has_tod = true;
                fds.tod.h_ += hours{days{j}};
            }
            if (wd != not_a_weekday)
                fds.wd = weekday{static_cast<unsigned>(wd)};
            if (abbrev != nullptr)
                *abbrev = std::move(temp_abbrev);
            if (offset != nullptr && temp_offset != not_a_offset)
              *offset = temp_offset;
        }
       return is;
    }
broken:
    is.setstate(ios::failbit);
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, year& y,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.year().ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        y = fds.ymd.year();
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, month& m,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.month().ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        m = fds.ymd.month();
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, day& d,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.day().ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        d = fds.ymd.day();
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, weekday& wd,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.wd.ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        wd = fds.wd;
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, year_month& ym,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.month().ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        ym = fds.ymd.year()/fds.ymd.month();
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt, month_day& md,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.month().ok() || !fds.ymd.day().ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        md = fds.ymd.month()/fds.ymd.day();
    return is;
}

template <class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            year_month_day& ymd, std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = std::chrono::seconds;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.ok())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        ymd = fds.ymd;
    return is;
}

template <class Duration, class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            sys_time<Duration>& tp, std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    using detail::round_i;
    std::chrono::minutes offset_local{};
    auto offptr = offset ? offset : &offset_local;
    fields<CT> fds{};
    fds.has_tod = true;
    date::from_stream(is, fmt, fds, abbrev, offptr);
    if (!fds.ymd.ok() || !fds.tod.in_conventional_range())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        tp = round_i<Duration>(sys_days(fds.ymd) - *offptr + fds.tod.to_duration());
    return is;
}

template <class Duration, class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            local_time<Duration>& tp, std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    using detail::round_i;
    fields<CT> fds{};
    fds.has_tod = true;
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.ymd.ok() || !fds.tod.in_conventional_range())
        is.setstate(std::ios::failbit);
    if (!is.fail())
        tp = round_i<Duration>(local_seconds{local_days(fds.ymd)} + fds.tod.to_duration());
    return is;
}

template <class Rep, class Period, class CharT, class Traits, class Alloc = std::allocator<CharT>>
std::basic_istream<CharT, Traits>&
from_stream(std::basic_istream<CharT, Traits>& is, const CharT* fmt,
            std::chrono::duration<Rep, Period>& d,
            std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
            std::chrono::minutes* offset = nullptr)
{
    using Duration = std::chrono::duration<Rep, Period>;
    using CT = typename std::common_type<Duration, std::chrono::seconds>::type;
    using detail::round_i;
    fields<CT> fds{};
    date::from_stream(is, fmt, fds, abbrev, offset);
    if (!fds.has_tod)
        is.setstate(std::ios::failbit);
    if (!is.fail())
        d = round_i<Duration>(fds.tod.to_duration());
    return is;
}

template <class Parsable, class CharT, class Traits = std::char_traits<CharT>,
          class Alloc = std::allocator<CharT>>
struct parse_manip
{
    const std::basic_string<CharT, Traits, Alloc> format_;
    Parsable&                                     tp_;
    std::basic_string<CharT, Traits, Alloc>*      abbrev_;
    std::chrono::minutes*                         offset_;

public:
    parse_manip(std::basic_string<CharT, Traits, Alloc> format, Parsable& tp,
                std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
                std::chrono::minutes* offset = nullptr)
        : format_(std::move(format))
        , tp_(tp)
        , abbrev_(abbrev)
        , offset_(offset)
        {}

#if HAS_STRING_VIEW
    parse_manip(const CharT* format, Parsable& tp,
                std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
                std::chrono::minutes* offset = nullptr)
        : format_(format)
        , tp_(tp)
        , abbrev_(abbrev)
        , offset_(offset)
        {}

    parse_manip(std::basic_string_view<CharT, Traits> format, Parsable& tp,
                std::basic_string<CharT, Traits, Alloc>* abbrev = nullptr,
                std::chrono::minutes* offset = nullptr)
        : format_(format)
        , tp_(tp)
        , abbrev_(abbrev)
        , offset_(offset)
        {}
#endif  // HAS_STRING_VIEW
};

template <class Parsable, class CharT, class Traits, class Alloc>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           const parse_manip<Parsable, CharT, Traits, Alloc>& x)
{
    return date::from_stream(is, x.format_.c_str(), x.tp_, x.abbrev_, x.offset_);
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const std::basic_string<CharT, Traits, Alloc>& format, Parsable& tp)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(),
                            format.c_str(), tp),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp})
{
    return {format, tp};
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const std::basic_string<CharT, Traits, Alloc>& format, Parsable& tp,
      std::basic_string<CharT, Traits, Alloc>& abbrev)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(),
                            format.c_str(), tp, &abbrev),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp, &abbrev})
{
    return {format, tp, &abbrev};
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const std::basic_string<CharT, Traits, Alloc>& format, Parsable& tp,
      std::chrono::minutes& offset)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(),
                            format.c_str(), tp,
                            std::declval<std::basic_string<CharT, Traits, Alloc>*>(),
                            &offset),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp, nullptr, &offset})
{
    return {format, tp, nullptr, &offset};
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const std::basic_string<CharT, Traits, Alloc>& format, Parsable& tp,
      std::basic_string<CharT, Traits, Alloc>& abbrev, std::chrono::minutes& offset)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(),
                            format.c_str(), tp, &abbrev, &offset),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp, &abbrev, &offset})
{
    return {format, tp, &abbrev, &offset};
}

// const CharT* formats

template <class Parsable, class CharT>
inline
auto
parse(const CharT* format, Parsable& tp)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT>&>(), format, tp),
                parse_manip<Parsable, CharT>{format, tp})
{
    return {format, tp};
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const CharT* format, Parsable& tp, std::basic_string<CharT, Traits, Alloc>& abbrev)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(), format,
                            tp, &abbrev),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp, &abbrev})
{
    return {format, tp, &abbrev};
}

template <class Parsable, class CharT>
inline
auto
parse(const CharT* format, Parsable& tp, std::chrono::minutes& offset)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT>&>(), format,
                            tp, std::declval<std::basic_string<CharT>*>(), &offset),
                parse_manip<Parsable, CharT>{format, tp, nullptr, &offset})
{
    return {format, tp, nullptr, &offset};
}

template <class Parsable, class CharT, class Traits, class Alloc>
inline
auto
parse(const CharT* format, Parsable& tp,
      std::basic_string<CharT, Traits, Alloc>& abbrev, std::chrono::minutes& offset)
    -> decltype(date::from_stream(std::declval<std::basic_istream<CharT, Traits>&>(), format,
                            tp, &abbrev, &offset),
                parse_manip<Parsable, CharT, Traits, Alloc>{format, tp, &abbrev, &offset})
{
    return {format, tp, &abbrev, &offset};
}

// duration streaming

template <class CharT, class Traits, class Rep, class Period>
inline
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const std::chrono::duration<Rep, Period>& d)
{
    return os << detail::make_string<CharT, Traits>::from(d.count()) +
                 detail::get_units<CharT>(typename Period::type{});
}

}  // namespace date

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

#endif  // DATE_H
