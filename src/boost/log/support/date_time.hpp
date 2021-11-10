/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/date_time.hpp
 * \author Andrey Semashev
 * \date   07.11.2012
 *
 * This header enables Boost.DateTime support for Boost.Log.
 */

#ifndef BOOST_LOG_SUPPORT_DATE_TIME_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_DATE_TIME_HPP_INCLUDED_

#include <ctime>
#include <string>
#include <locale>
#include <ostream>
#include <iterator>
#include <boost/cstdint.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/date_time/time.hpp>
#include <boost/date_time/date.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/local_time/local_time_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/date_time_format_parser.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/decomposed_time.hpp>
#include <boost/log/detail/date_time_fmt_gen_traits_fwd.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

namespace date_time_support {

template< typename DateT, typename ValueT >
inline void decompose_date(DateT const& d, boost::log::aux::decomposed_time_wrapper< ValueT >& v)
{
    typedef typename DateT::ymd_type ymd_type;
    ymd_type ymd = d.year_month_day();
    v.year = static_cast< uint32_t >(ymd.year);
    v.month = static_cast< uint32_t >(ymd.month);
    v.day = static_cast< uint32_t >(ymd.day);
}

template< typename TimeDurationT, typename ValueT >
inline void decompose_time_of_day(TimeDurationT const& tod, boost::log::aux::decomposed_time_wrapper< ValueT >& v)
{
    v.hours = static_cast< uint32_t >(tod.hours());
    v.minutes = static_cast< uint32_t >(tod.minutes());
    v.seconds = static_cast< uint32_t >(tod.seconds());

    typedef typename TimeDurationT::traits_type traits_type;
    enum
    {
        adjustment_ratio = (traits_type::ticks_per_second > boost::log::aux::decomposed_time::subseconds_per_second ?
            traits_type::ticks_per_second / boost::log::aux::decomposed_time::subseconds_per_second :
            boost::log::aux::decomposed_time::subseconds_per_second / traits_type::ticks_per_second)
    };
    uint64_t frac = tod.fractional_seconds();
    v.subseconds = static_cast< uint32_t >(traits_type::ticks_per_second > boost::log::aux::decomposed_time::subseconds_per_second ? frac / adjustment_ratio : frac * adjustment_ratio);
}

template< typename TimeDurationT, typename ValueT >
inline void decompose_time_duration(TimeDurationT const& dur, boost::log::aux::decomposed_time_wrapper< ValueT >& v)
{
    if (dur.is_negative())
    {
        v.negative = true;
        (decompose_time_of_day)(-dur, v);
    }
    else
        (decompose_time_of_day)(dur, v);
}

template< typename DateDurationT, typename ValueT >
inline void decompose_date_duration(DateDurationT const& dur, boost::log::aux::decomposed_time_wrapper< ValueT >& v)
{
    if (dur.is_negative())
    {
        v.negative = true;
        v.day = static_cast< uint32_t >((-dur).days());
    }
    else
        v.day = static_cast< uint32_t >(dur.days());
}

template< typename TimeT, typename ValueT >
inline void decompose_time(TimeT const& t, boost::log::aux::decomposed_time_wrapper< ValueT >& v)
{
    (decompose_date)(t.date(), v);
    (decompose_time_of_day)(t.time_of_day(), v);
}

} // namespace date_time_support

template< typename TimeT, typename CharT >
struct date_time_formatter_generator_traits_impl
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Value type
    typedef TimeT value_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Formatter implementation
    class formatter :
        public boost::log::aux::date_time_formatter< boost::log::aux::decomposed_time_wrapper< value_type >, char_type >
    {
        BOOST_COPYABLE_AND_MOVABLE_ALT(formatter)

    private:
        // Do not change this typedef, copy-pasting the inherited class from above will break compilation with MSVC 2012 because it incorrectly binds value_type.
        typedef typename formatter::date_time_formatter_ base_type;

    public:
        typedef typename base_type::result_type result_type;
        // This typedef is needed to work around MSVC 2012 crappy name lookup. Otherwise base_type::value_type is bound instead.
        typedef typename date_time_formatter_generator_traits_impl< TimeT, CharT >::value_type value_type;

    public:
        BOOST_DEFAULTED_FUNCTION(formatter(), {})
        formatter(formatter const& that) : base_type(static_cast< base_type const& >(that)) {}
        formatter(BOOST_RV_REF(formatter) that) { this->swap(that); }

        formatter& operator= (formatter that)
        {
            this->swap(that);
            return *this;
        }

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            if (value.is_not_a_date_time())
                strm << "not-a-date-time";
            else if (value.is_pos_infinity())
                strm << "+infinity";
            else if (value.is_neg_infinity())
                strm << "-infinity";
            else
            {
                boost::log::aux::decomposed_time_wrapper< value_type > val(value);
                date_time_support::decompose_time(value, val);
                base_type::operator() (strm, val);
            }
        }
    };

    //! The function parses format string and constructs formatter function
    static formatter_function_type parse(string_type const& format)
    {
        formatter fmt;
        boost::log::aux::decomposed_time_formatter_builder< formatter, char_type > builder(fmt);
        boost::log::aux::parse_date_time_format(format, builder);
        return formatter_function_type(boost::move(fmt));
    }
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< posix_time::ptime, CharT, VoidT > :
    public date_time_formatter_generator_traits_impl< posix_time::ptime, CharT >
{
};

template< typename TimeT, typename TimeZoneT, typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< local_time::local_date_time_base< TimeT, TimeZoneT >, CharT, VoidT >
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Value type
    typedef local_time::local_date_time_base< TimeT, TimeZoneT > value_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Formatter implementation
    class formatter :
        public boost::log::aux::date_time_formatter< boost::log::aux::decomposed_time_wrapper< value_type >, char_type >
    {
        BOOST_COPYABLE_AND_MOVABLE_ALT(formatter)

    private:
        // Do not change this typedef, copy-pasting the inherited class from above will break compilation with MSVC 2012 because it incorrectly binds value_type.
        typedef typename formatter::date_time_formatter_ base_type;

    public:
        typedef typename base_type::result_type result_type;
        typedef typename base_type::context context;
        // This typedef is needed to work around MSVC 2012 crappy name lookup. Otherwise base_type::value_type is bound instead.
        typedef typename date_time_formatter_generator_traits< local_time::local_date_time_base< TimeT, TimeZoneT >, CharT, VoidT >::value_type value_type;

    public:
        BOOST_DEFAULTED_FUNCTION(formatter(), {})
        formatter(formatter const& that) : base_type(static_cast< base_type const& >(that)) {}
        formatter(BOOST_RV_REF(formatter) that) { this->swap(that); }

        formatter& operator= (formatter that)
        {
            this->swap(that);
            return *this;
        }

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            if (value.is_not_a_date_time())
                strm << "not-a-date-time";
            else if (value.is_pos_infinity())
                strm << "+infinity";
            else if (value.is_neg_infinity())
                strm << "-infinity";
            else
            {
                boost::log::aux::decomposed_time_wrapper< value_type > val(value);
                date_time_support::decompose_time(value.local_time(), val);
                base_type::operator() (strm, val);
            }
        }

    public:
        static void format_iso_time_zone(context& ctx)
        {
            ctx.strm << ctx.value.m_time.zone_abbrev(true);
            ctx.strm.flush();
        }

        static void format_extended_iso_time_zone(context& ctx)
        {
            ctx.strm << ctx.value.m_time.zone_name(true);
            ctx.strm.flush();
        }
    };

    class formatter_builder :
        public boost::log::aux::decomposed_time_formatter_builder< formatter, char_type >
    {
    private:
        typedef boost::log::aux::decomposed_time_formatter_builder< formatter, char_type > base_type;

    public:
        explicit formatter_builder(formatter& fmt) : base_type(fmt)
        {
        }

        void on_iso_time_zone()
        {
            this->m_formatter.add_formatter(&formatter::format_iso_time_zone);
        }

        void on_extended_iso_time_zone()
        {
            this->m_formatter.add_formatter(&formatter::format_extended_iso_time_zone);
        }
    };

    //! The function parses format string and constructs formatter function
    static formatter_function_type parse(string_type const& format)
    {
        formatter fmt;
        formatter_builder builder(fmt);
        boost::log::aux::parse_date_time_format(format, builder);
        return formatter_function_type(boost::move(fmt));
    }
};

template< typename DateT, typename CharT >
struct date_formatter_generator_traits_impl
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Value type
    typedef DateT value_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Formatter implementation
    class formatter :
        public boost::log::aux::date_time_formatter< boost::log::aux::decomposed_time_wrapper< value_type >, char_type >
    {
        BOOST_COPYABLE_AND_MOVABLE_ALT(formatter)

    private:
        // Do not change this typedef, copy-pasting the inherited class from above will break compilation with MSVC 2012 because it incorrectly binds value_type.
        typedef typename formatter::date_time_formatter_ base_type;

    public:
        typedef typename base_type::result_type result_type;
        // This typedef is needed to work around MSVC 2012 crappy name lookup. Otherwise base_type::value_type is bound instead.
        typedef typename date_formatter_generator_traits_impl< DateT, CharT >::value_type value_type;

    public:
        BOOST_DEFAULTED_FUNCTION(formatter(), {})
        formatter(formatter const& that) : base_type(static_cast< base_type const& >(that)) {}
        formatter(BOOST_RV_REF(formatter) that) { this->swap(that); }

        formatter& operator= (formatter that)
        {
            this->swap(that);
            return *this;
        }

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            if (value.is_not_a_date())
                strm << "not-a-date-time";
            else if (value.is_pos_infinity())
                strm << "+infinity";
            else if (value.is_neg_infinity())
                strm << "-infinity";
            else
            {
                boost::log::aux::decomposed_time_wrapper< value_type > val(value);
                date_time_support::decompose_date(value, val);
                base_type::operator() (strm, val);
            }
        }
    };

    //! The function parses format string and constructs formatter function
    static formatter_function_type parse(string_type const& format)
    {
        formatter fmt;
        boost::log::aux::decomposed_time_formatter_builder< formatter, char_type > builder(fmt);
        boost::log::aux::parse_date_format(format, builder);
        return formatter_function_type(boost::move(fmt));
    }
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< gregorian::date, CharT, VoidT > :
    public date_formatter_generator_traits_impl< gregorian::date, CharT >
{
};

template< typename TimeDurationT, typename CharT >
struct time_duration_formatter_generator_traits_impl
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Value type
    typedef TimeDurationT value_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Formatter implementation
    class formatter :
        public boost::log::aux::date_time_formatter< boost::log::aux::decomposed_time_wrapper< value_type >, char_type >
    {
        BOOST_COPYABLE_AND_MOVABLE_ALT(formatter)

    private:
        // Do not change this typedef, copy-pasting the inherited class from above will break compilation with MSVC 2012 because it incorrectly binds value_type.
        typedef typename formatter::date_time_formatter_ base_type;

    public:
        typedef typename base_type::result_type result_type;
        // This typedef is needed to work around MSVC 2012 crappy name lookup. Otherwise base_type::value_type is bound instead.
        typedef typename time_duration_formatter_generator_traits_impl< TimeDurationT, CharT >::value_type value_type;

    public:
        BOOST_DEFAULTED_FUNCTION(formatter(), {})
        formatter(formatter const& that) : base_type(static_cast< base_type const& >(that)) {}
        formatter(BOOST_RV_REF(formatter) that) { this->swap(that); }

        formatter& operator= (formatter that)
        {
            this->swap(that);
            return *this;
        }

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            if (value.is_not_a_date_time())
                strm << "not-a-date-time";
            else if (value.is_pos_infinity())
                strm << "+infinity";
            else if (value.is_neg_infinity())
                strm << "-infinity";
            else
            {
                boost::log::aux::decomposed_time_wrapper< value_type > val(value);
                date_time_support::decompose_time_duration(value, val);
                base_type::operator() (strm, val);
            }
        }
    };

    //! The function parses format string and constructs formatter function
    static formatter_function_type parse(string_type const& format)
    {
        formatter fmt;
        boost::log::aux::decomposed_time_formatter_builder< formatter, char_type > builder(fmt);
        boost::log::aux::parse_time_format(format, builder);
        return formatter_function_type(boost::move(fmt));
    }
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< posix_time::time_duration, CharT, VoidT > :
    public time_duration_formatter_generator_traits_impl< posix_time::time_duration, CharT >
{
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< posix_time::hours, CharT, VoidT > :
    public time_duration_formatter_generator_traits_impl< posix_time::hours, CharT >
{
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< posix_time::minutes, CharT, VoidT > :
    public time_duration_formatter_generator_traits_impl< posix_time::minutes, CharT >
{
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< posix_time::seconds, CharT, VoidT > :
    public time_duration_formatter_generator_traits_impl< posix_time::seconds, CharT >
{
};

template< typename BaseDurationT, uint64_t FracOfSecondV, typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< date_time::subsecond_duration< BaseDurationT, FracOfSecondV >, CharT, VoidT > :
    public time_duration_formatter_generator_traits_impl< date_time::subsecond_duration< BaseDurationT, FracOfSecondV >, CharT >
{
};

template< typename DateDurationT, typename CharT >
struct date_duration_formatter_generator_traits_impl
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Value type
    typedef DateDurationT value_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Formatter implementation
    class formatter :
        public boost::log::aux::date_time_formatter< boost::log::aux::decomposed_time_wrapper< value_type >, char_type >
    {
        BOOST_COPYABLE_AND_MOVABLE_ALT(formatter)

    private:
        // Do not change this typedef, copy-pasting the inherited class from above will break compilation with MSVC 2012 because it incorrectly binds value_type.
        typedef typename formatter::date_time_formatter_ base_type;

    public:
        typedef typename base_type::result_type result_type;
        // This typedef is needed to work around MSVC 2012 crappy name lookup. Otherwise base_type::value_type is bound instead.
        typedef typename date_duration_formatter_generator_traits_impl< DateDurationT, CharT >::value_type value_type;

    public:
        BOOST_DEFAULTED_FUNCTION(formatter(), {})
        formatter(formatter const& that) : base_type(static_cast< base_type const& >(that)) {}
        formatter(BOOST_RV_REF(formatter) that) { this->swap(that); }

        formatter& operator= (formatter that)
        {
            this->swap(that);
            return *this;
        }

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            if (value.is_not_a_date())
                strm << "not-a-date-time";
            else if (value.is_pos_infinity())
                strm << "+infinity";
            else if (value.is_neg_infinity())
                strm << "-infinity";
            else
            {
                boost::log::aux::decomposed_time_wrapper< value_type > val(value);
                date_time_support::decompose_date_duration(value, val);
                base_type::operator() (strm, val);
            }
        }
    };

    //! The function parses format string and constructs formatter function
    static formatter_function_type parse(string_type const& format)
    {
        formatter fmt;
        boost::log::aux::decomposed_time_formatter_builder< formatter, char_type > builder(fmt);
        boost::log::aux::parse_date_format(format, builder);
        return formatter_function_type(boost::move(fmt));
    }
};

template< typename CharT, typename VoidT >
struct date_time_formatter_generator_traits< gregorian::date_duration, CharT, VoidT > :
    public date_formatter_generator_traits_impl< gregorian::date_duration, CharT >
{
};

} // namespace aux

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SUPPORT_DATE_TIME_HPP_INCLUDED_
