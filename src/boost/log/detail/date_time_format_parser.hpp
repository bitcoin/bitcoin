/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   date_time_format_parser.hpp
 * \author Andrey Semashev
 * \date   16.09.2012
 *
 * The header contains a parser for date and time format strings.
 */

#ifndef BOOST_LOG_DETAIL_DATE_TIME_FORMAT_PARSER_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_DATE_TIME_FORMAT_PARSER_HPP_INCLUDED_

#include <string>
#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

/*!
 * This is the interface the parser will use to notify the caller about various components of date in the format string.
 */
template< typename CharT >
struct date_format_parser_callback
{
    //! Character type used by the parser
    typedef CharT char_type;

    //! Destructor
    virtual ~date_format_parser_callback() {}

    /*!
     * \brief The function is called when the parser discovers a string literal in the format string
     *
     * \param lit The string of characters not interpreted as a placeholder
     */
    virtual void on_literal(iterator_range< const char_type* > const& lit) = 0;

    /*!
     * \brief The method is called when an unknown placeholder is found in the format string
     *
     * \param ph The placeholder with the leading percent sign
     */
    virtual void on_placeholder(iterator_range< const char_type* > const& ph)
    {
        // By default interpret all unrecognized placeholders as literals
        on_literal(ph);
    }

    /*!
     * \brief The function is called when the short year placeholder is found in the format string
     */
    virtual void on_short_year()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('y'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the full year placeholder is found in the format string
     */
    virtual void on_full_year()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('Y'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the numeric month placeholder is found in the format string
     */
    virtual void on_numeric_month()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('m'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the short alphabetic month placeholder is found in the format string
     */
    virtual void on_short_month()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('b'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the full alphabetic month placeholder is found in the format string
     */
    virtual void on_full_month()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('B'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the numeric day of month placeholder is found in the format string
     *
     * \param leading_zero If \c true, the day should be formatted with leading zero, otherwise with leading space
     */
    virtual void on_month_day(bool leading_zero)
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), (leading_zero ? static_cast< char_type >('d') : static_cast< char_type >('e')), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the numeric day of week placeholder is found in the format string
     */
    virtual void on_numeric_week_day()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('w'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the short alphabetic day of week placeholder is found in the format string
     */
    virtual void on_short_week_day()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('a'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the full alphabetic day of week placeholder is found in the format string
     */
    virtual void on_full_week_day()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('A'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the ISO-formatted date is found in the format string
     */
    virtual void on_iso_date()
    {
        on_full_year();
        on_numeric_month();
        on_month_day(true);
    }

    /*!
     * \brief The function is called when the extended ISO-formatted date is found in the format string
     */
    virtual void on_extended_iso_date()
    {
        const char_type delimiter[2] = { static_cast< char_type >('-'), static_cast< char_type >('\0') };
        on_full_year();
        on_literal(boost::as_literal(delimiter));
        on_numeric_month();
        on_literal(boost::as_literal(delimiter));
        on_month_day(true);
    }
};

/*!
 * This is the interface the parser will use to notify the caller about various components of date in the format string.
 */
template< typename CharT >
struct time_format_parser_callback
{
    //! Character type used by the parser
    typedef CharT char_type;

    //! Destructor
    virtual ~time_format_parser_callback() {}

    /*!
     * \brief The function is called when the parser discovers a string literal in the format string
     *
     * \param lit The string of characters not interpreted as a placeholder
     */
    virtual void on_literal(iterator_range< const char_type* > const& lit) = 0;

    /*!
     * \brief The method is called when an unknown placeholder is found in the format string
     *
     * \param ph The placeholder with the leading percent sign
     */
    virtual void on_placeholder(iterator_range< const char_type* > const& ph)
    {
        // By default interpret all unrecognized placeholders as literals
        on_literal(ph);
    }

    /*!
     * \brief The function is called when the hours placeholder is found in the format string
     *
     * The placeholder is used for 24-hour clock and duration formatting.
     *
     * \param leading_zero If \c true, the one-digit number of hours should be formatted with leading zero, otherwise with leading space
     */
    virtual void on_hours(bool leading_zero)
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), (leading_zero ? static_cast< char_type >('O') : static_cast< char_type >('k')), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the hours placeholder is found in the format string
     *
     * The placeholder is used for 12-hour clock formatting. It should not be used for duration formatting.
     *
     * \param leading_zero If \c true, the one-digit number of hours should be formatted with leading zero, otherwise with leading space
     */
    virtual void on_hours_12(bool leading_zero)
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), (leading_zero ? static_cast< char_type >('I') : static_cast< char_type >('l')), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the minutes placeholder is found in the format string
     */
    virtual void on_minutes()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('M'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the seconds placeholder is found in the format string
     */
    virtual void on_seconds()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('S'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the fractional seconds placeholder is found in the format string
     */
    virtual void on_fractional_seconds()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('f'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the day period (AM/PM) placeholder is found in the format string
     *
     * The placeholder is used for 12-hour clock formatting. It should not be used for duration formatting.
     *
     * \param upper_case If \c true, the day period will be upper case, and lower case otherwise
     */
    virtual void on_am_pm(bool upper_case)
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), (upper_case ? static_cast< char_type >('p') : static_cast< char_type >('P')), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the time duration sign placeholder is found in the format string
     *
     * The placeholder is used for duration formatting. It should not be used for time point formatting.
     *
     * \param display_positive If \c true, the positive sign will be explicitly displayed, otherwise only negative sign will be displayed
     */
    virtual void on_duration_sign(bool display_positive)
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), (display_positive ? static_cast< char_type >('+') : static_cast< char_type >('-')), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the ISO time zone placeholder is found in the format string
     */
    virtual void on_iso_time_zone()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('q'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the extended ISO time zone placeholder is found in the format string
     */
    virtual void on_extended_iso_time_zone()
    {
        const char_type placeholder[3] = { static_cast< char_type >('%'), static_cast< char_type >('Q'), static_cast< char_type >('\0') };
        on_placeholder(boost::as_literal(placeholder));
    }

    /*!
     * \brief The function is called when the ISO-formatted time is found in the format string
     */
    virtual void on_iso_time()
    {
        on_hours(true);
        on_minutes();
        on_seconds();
    }

    /*!
     * \brief The function is called when the extended ISO-formatted time is found in the format string
     */
    virtual void on_extended_iso_time()
    {
        const char_type delimiter[2] = { static_cast< char_type >(':'), static_cast< char_type >('\0') };
        on_hours(true);
        on_literal(boost::as_literal(delimiter));
        on_minutes();
        on_literal(boost::as_literal(delimiter));
        on_seconds();
    }

    /*!
     * \brief The function is called when the extended ISO-formatted time with fractional seconds is found in the format string
     */
    virtual void on_default_time()
    {
        on_extended_iso_time();

        const char_type delimiter[2] = { static_cast< char_type >('.'), static_cast< char_type >('\0') };
        on_literal(boost::as_literal(delimiter));
        on_fractional_seconds();
    }
};

/*!
 * This is the interface the parser will use to notify the caller about various components of date in the format string.
 */
template< typename CharT >
struct date_time_format_parser_callback :
    public date_format_parser_callback< CharT >,
    public time_format_parser_callback< CharT >
{
    //! Character type used by the parser
    typedef CharT char_type;

    //! Destructor
    ~date_time_format_parser_callback() BOOST_OVERRIDE {}

    /*!
     * \brief The function is called when the parser discovers a string literal in the format string
     *
     * \param lit The string of characters not interpreted as a placeholder
     */
    void on_literal(iterator_range< const char_type* > const& lit) BOOST_OVERRIDE = 0;

    /*!
     * \brief The method is called when an unknown placeholder is found in the format string
     *
     * \param ph The placeholder with the leading percent sign
     */
    void on_placeholder(iterator_range< const char_type* > const& ph) BOOST_OVERRIDE
    {
        // By default interpret all unrecognized placeholders as literals
        on_literal(ph);
    }
};

/*!
 * \brief Parses the date format string and invokes the callback object
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be \c NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
BOOST_LOG_API void parse_date_format(const CharT* begin, const CharT* end, date_format_parser_callback< CharT >& callback);

/*!
 * \brief Parses the date format string and invokes the callback object
 *
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void parse_date_format(std::basic_string< CharT, TraitsT, AllocatorT > const& str, date_format_parser_callback< CharT >& callback)
{
    const CharT* p = str.c_str();
    return parse_date_format(p, p + str.size(), callback);
}

/*!
 * \brief Parses the date format string and invokes the callback object
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string.
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
inline void parse_date_format(const CharT* str, date_format_parser_callback< CharT >& callback)
{
    return parse_date_format(str, str + std::char_traits< CharT >::length(str), callback);
}

/*!
 * \brief Parses the time format string and invokes the callback object
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be \c NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
BOOST_LOG_API void parse_time_format(const CharT* begin, const CharT* end, time_format_parser_callback< CharT >& callback);

/*!
 * \brief Parses the time format string and invokes the callback object
 *
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void parse_time_format(std::basic_string< CharT, TraitsT, AllocatorT > const& str, time_format_parser_callback< CharT >& callback)
{
    const CharT* p = str.c_str();
    return parse_time_format(p, p + str.size(), callback);
}

/*!
 * \brief Parses the time format string and invokes the callback object
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string.
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
inline void parse_time_format(const CharT* str, time_format_parser_callback< CharT >& callback)
{
    return parse_time_format(str, str + std::char_traits< CharT >::length(str), callback);
}

/*!
 * \brief Parses the date and time format string and invokes the callback object
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be \c NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
BOOST_LOG_API void parse_date_time_format(const CharT* begin, const CharT* end, date_time_format_parser_callback< CharT >& callback);

/*!
 * \brief Parses the date and time format string and invokes the callback object
 *
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void parse_date_time_format(std::basic_string< CharT, TraitsT, AllocatorT > const& str, date_time_format_parser_callback< CharT >& callback)
{
    const CharT* p = str.c_str();
    return parse_date_time_format(p, p + str.size(), callback);
}

/*!
 * \brief Parses the date and time format string and invokes the callback object
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string.
 * \param str The format string to parse
 * \param callback Reference to the callback object that will be invoked by the parser as it processes the input
 */
template< typename CharT >
inline void parse_date_time_format(const CharT* str, date_time_format_parser_callback< CharT >& callback)
{
    return parse_date_time_format(str, str + std::char_traits< CharT >::length(str), callback);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_DATE_TIME_FORMAT_PARSER_HPP_INCLUDED_
