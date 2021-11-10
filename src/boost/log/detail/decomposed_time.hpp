/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   decomposed_time.hpp
 * \author Andrey Semashev
 * \date   07.11.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_DECOMPOSED_TIME_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_DECOMPOSED_TIME_HPP_INCLUDED_

#include <ctime>
#include <string>
#include <vector>
#include <locale>
#include <boost/cstdint.hpp>
#include <boost/move/core.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/date_time_format_parser.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! Date and time suitable for formatting
struct decomposed_time
{
    // Subseconds are microseconds
    enum _
    {
        subseconds_per_second = 1000000,
        subseconds_digits10 = 6
    };

    uint32_t year, month, day, hours, minutes, seconds, subseconds;
    bool negative;

    decomposed_time() : year(0), month(1), day(1), hours(0), minutes(0), seconds(0), subseconds(0), negative(false)
    {
    }

    decomposed_time(uint32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s, uint32_t ss = 0, bool neg = false) :
        year(y), month(mo), day(d), hours(h), minutes(mi), seconds(s), subseconds(ss), negative(neg)
    {
    }

    unsigned int week_day() const
    {
        unsigned int a = (14u - month) / 12u;
        unsigned int y = year - a;
        unsigned int m = month + 12u * a - 2u;
        return (day + y + (y / 4u) - (y / 100u) + (y / 400u) + (31u * m) / 12u) % 7u;
    }

    unsigned int year_day() const
    {
        bool is_leap_year = (!(year % 4u)) && ((year % 100u) || (!(year % 400u)));
        static const unsigned int first_day_offset[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
        return first_day_offset[month - 1] + day + (month > 2 && is_leap_year);
    }
};

inline std::tm to_tm(decomposed_time const& t)
{
    std::tm res = {};
    res.tm_year = static_cast< int >(t.year) - 1900;
    res.tm_mon = t.month - 1;
    res.tm_mday = t.day;
    res.tm_hour = t.hours;
    res.tm_min = t.minutes;
    res.tm_sec = t.seconds;
    res.tm_wday = t.week_day();
    res.tm_yday = t.year_day();
    res.tm_isdst = -1;

    return res;
}

template< typename T >
struct decomposed_time_wrapper :
    public boost::log::aux::decomposed_time
{
    typedef boost::log::aux::decomposed_time base_type;
    typedef T value_type;
    value_type m_time;

    BOOST_DEFAULTED_FUNCTION(decomposed_time_wrapper(), {})

    explicit decomposed_time_wrapper(value_type const& time) : m_time(time)
    {
    }
};

template< typename CharT >
BOOST_LOG_API void put_integer(boost::log::aux::basic_ostringstreambuf< CharT >& strbuf, uint32_t value, unsigned int width, CharT fill_char);

template< typename T, typename CharT >
class date_time_formatter
{
    BOOST_COPYABLE_AND_MOVABLE_ALT(date_time_formatter)

protected:
    // Note: This typedef is needed to work around MSVC 2012 crappy name lookup in the derived classes
    typedef date_time_formatter date_time_formatter_;

public:
    typedef void result_type;
    typedef T value_type;
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef basic_formatting_ostream< char_type > stream_type;

    struct context
    {
        date_time_formatter const& self;
        stream_type& strm;
        value_type const& value;
        unsigned int literal_index, literal_pos;

        context(date_time_formatter const& self_, stream_type& strm_, value_type const& value_) :
            self(self_),
            strm(strm_),
            value(value_),
            literal_index(0),
            literal_pos(0)
        {
        }

        BOOST_DELETED_FUNCTION(context(context const&))
        BOOST_DELETED_FUNCTION(context& operator=(context const&))
    };

private:
    typedef void (*formatter_type)(context&);
    typedef std::vector< formatter_type > formatters;
    typedef std::vector< unsigned int > literal_lens;

protected:
    formatters m_formatters;
    literal_lens m_literal_lens;
    string_type m_literal_chars;

public:
    BOOST_DEFAULTED_FUNCTION(date_time_formatter(), {})
    date_time_formatter(date_time_formatter const& that) :
        m_formatters(that.m_formatters),
        m_literal_lens(that.m_literal_lens),
        m_literal_chars(that.m_literal_chars)
    {
    }
    date_time_formatter(BOOST_RV_REF(date_time_formatter) that) BOOST_NOEXCEPT
    {
        this->swap(static_cast< date_time_formatter& >(that));
    }

    date_time_formatter& operator= (date_time_formatter that) BOOST_NOEXCEPT
    {
        this->swap(that);
        return *this;
    }

    result_type operator() (stream_type& strm, value_type const& value) const
    {
        // Some formatters will put characters directly to the underlying string, so we have to flush stream buffers before formatting
        strm.flush();
        context ctx(*this, strm, value);
        for (typename formatters::const_iterator it = m_formatters.begin(), end = m_formatters.end(); strm.good() && it != end; ++it)
        {
            (*it)(ctx);
        }
    }

    void add_formatter(formatter_type fun)
    {
        m_formatters.push_back(fun);
    }

    void add_literal(iterator_range< const char_type* > const& lit)
    {
        m_literal_chars.append(lit.begin(), lit.end());
        m_literal_lens.push_back(static_cast< unsigned int >(lit.size()));
        m_formatters.push_back(&date_time_formatter_::format_literal);
    }

    void swap(date_time_formatter& that) BOOST_NOEXCEPT
    {
        m_formatters.swap(that.m_formatters);
        m_literal_lens.swap(that.m_literal_lens);
        m_literal_chars.swap(that.m_literal_chars);
    }

public:
    template< char FormatCharV >
    static void format_through_locale(context& ctx)
    {
        typedef std::time_put< char_type > facet_type;
        typedef typename facet_type::iter_type iter_type;
        std::tm t = to_tm(static_cast< decomposed_time const& >(ctx.value));
        std::use_facet< facet_type >(ctx.strm.getloc()).put(iter_type(ctx.strm.stream()), ctx.strm.stream(), ' ', &t, FormatCharV);
        ctx.strm.flush();
    }

    static void format_full_year(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.year, 4, static_cast< char_type >('0'));
    }

    static void format_short_year(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.year % 100u, 2, static_cast< char_type >('0'));
    }

    static void format_numeric_month(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.month, 2, static_cast< char_type >('0'));
    }

    template< char_type FillCharV >
    static void format_month_day(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.day, 2, static_cast< char_type >(FillCharV));
    }

    static void format_week_day(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), static_cast< decomposed_time const& >(ctx.value).week_day(), 1, static_cast< char_type >('0'));
    }

    template< char_type FillCharV >
    static void format_hours(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.hours, 2, static_cast< char_type >(FillCharV));
    }

    template< char_type FillCharV >
    static void format_hours_12(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.hours % 12u + 1u, 2, static_cast< char_type >(FillCharV));
    }

    static void format_minutes(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.minutes, 2, static_cast< char_type >('0'));
    }

    static void format_seconds(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.seconds, 2, static_cast< char_type >('0'));
    }

    static void format_fractional_seconds(context& ctx)
    {
        (put_integer)(*ctx.strm.rdbuf(), ctx.value.subseconds, decomposed_time::subseconds_digits10, static_cast< char_type >('0'));
    }

    template< bool UpperCaseV >
    static void format_am_pm(context& ctx)
    {
        static const char_type am[] = { static_cast< char_type >(UpperCaseV ? 'A' : 'a'), static_cast< char_type >(UpperCaseV ? 'M' : 'm'), static_cast< char_type >(0) };
        static const char_type pm[] = { static_cast< char_type >(UpperCaseV ? 'P' : 'p'), static_cast< char_type >(UpperCaseV ? 'M' : 'm'), static_cast< char_type >(0) };

        ctx.strm.rdbuf()->append(((static_cast< decomposed_time const& >(ctx.value).hours > 11) ? pm : am), 2u);
    }

    template< bool DisplayPositiveV >
    static void format_sign(context& ctx)
    {
        if (static_cast< decomposed_time const& >(ctx.value).negative)
            ctx.strm.rdbuf()->push_back('-');
        else if (DisplayPositiveV)
            ctx.strm.rdbuf()->push_back('+');
    }

private:
    static void format_literal(context& ctx)
    {
        unsigned int len = ctx.self.m_literal_lens[ctx.literal_index], pos = ctx.literal_pos;
        ++ctx.literal_index;
        ctx.literal_pos += len;
        const char_type* lit = ctx.self.m_literal_chars.c_str();
        ctx.strm.rdbuf()->append(lit + pos, len);
    }
};

template< typename FormatterT, typename CharT >
class decomposed_time_formatter_builder :
    public date_time_format_parser_callback< CharT >
{
public:
    typedef date_time_format_parser_callback< CharT > base_type;
    typedef typename base_type::char_type char_type;
    typedef FormatterT formatter_type;
    typedef typename formatter_type::value_type value_type;
    typedef typename formatter_type::stream_type stream_type;
    typedef typename stream_type::string_type string_type;

protected:
    formatter_type& m_formatter;

public:
    explicit decomposed_time_formatter_builder(formatter_type& fmt) : m_formatter(fmt)
    {
    }

    void on_literal(iterator_range< const char_type* > const& lit)
    {
        m_formatter.add_literal(lit);
    }

    void on_short_year()
    {
        m_formatter.add_formatter(&formatter_type::format_short_year);
    }

    void on_full_year()
    {
        m_formatter.add_formatter(&formatter_type::format_full_year);
    }

    void on_numeric_month()
    {
        m_formatter.add_formatter(&formatter_type::format_numeric_month);
    }

    void on_short_month()
    {
        m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_through_locale< 'b' >);
    }

    void on_full_month()
    {
        m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_through_locale< 'B' >);
    }

    void on_month_day(bool leading_zero)
    {
        if (leading_zero)
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_month_day< '0' >);
        else
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_month_day< ' ' >);
    }

    void on_numeric_week_day()
    {
        m_formatter.add_formatter(&formatter_type::format_week_day);
    }

    void on_short_week_day()
    {
        m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_through_locale< 'a' >);
    }

    void on_full_week_day()
    {
        m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_through_locale< 'A' >);
    }

    void on_hours(bool leading_zero)
    {
        if (leading_zero)
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_hours< '0' >);
        else
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_hours< ' ' >);
    }

    void on_hours_12(bool leading_zero)
    {
        if (leading_zero)
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_hours_12< '0' >);
        else
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_hours_12< ' ' >);
    }

    void on_minutes()
    {
        m_formatter.add_formatter(&formatter_type::format_minutes);
    }

    void on_seconds()
    {
        m_formatter.add_formatter(&formatter_type::format_seconds);
    }

    void on_fractional_seconds()
    {
        m_formatter.add_formatter(&formatter_type::format_fractional_seconds);
    }

    void on_am_pm(bool upper_case)
    {
        if (upper_case)
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_am_pm< true >);
        else
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_am_pm< false >);
    }

    void on_duration_sign(bool display_positive)
    {
        if (display_positive)
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_sign< true >);
        else
            m_formatter.add_formatter(&formatter_type::BOOST_NESTED_TEMPLATE format_sign< false >);
    }

    void on_iso_time_zone()
    {
    }

    void on_extended_iso_time_zone()
    {
    }
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_DECOMPOSED_TIME_HPP_INCLUDED_
