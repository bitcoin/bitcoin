
#ifndef DATE_TIME_FORMAT_DATE_PARSER_HPP__
#define DATE_TIME_FORMAT_DATE_PARSER_HPP__

/* Copyright (c) 2004-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


#include "boost/lexical_cast.hpp"
#include "boost/date_time/string_parse_tree.hpp"
#include "boost/date_time/strings_from_facet.hpp"
#include "boost/date_time/special_values_parser.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#ifndef BOOST_NO_STDC_NAMESPACE
#  include <cctype>
#else
#  include <ctype.h>
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
  using ::isspace;
  using ::isdigit;
}
#endif
namespace boost { namespace date_time {

//! Helper function for parsing fixed length strings into integers
/*! Will consume 'length' number of characters from stream. Consumed
 * character are transfered to parse_match_result struct.
 * Returns '-1' if no number can be parsed or incorrect number of
 * digits in stream. */
template<typename int_type, typename charT>
inline
int_type
fixed_string_to_int(std::istreambuf_iterator<charT>& itr,
                    std::istreambuf_iterator<charT>& stream_end,
                    parse_match_result<charT>& mr,
                    unsigned int length,
                    const charT& fill_char)
{
  //typedef std::basic_string<charT>  string_type;
  unsigned int j = 0;
  //string_type s;
  while (j < length && itr != stream_end &&
      (std::isdigit(*itr) || *itr == fill_char)) {
    if(*itr == fill_char) {
      /* Since a fill_char can be anything, we convert it to a zero.
       * lexical_cast will behave predictably when zero is used as fill. */
      mr.cache += ('0');
    }
    else {
      mr.cache += (*itr);
    }
    itr++;
    j++;
  }
  int_type i = static_cast<int_type>(-1);
  // mr.cache will hold leading zeros. size() tells us when input is too short.
  if(mr.cache.size() < length) {
    return i;
  }
  try {
    i = boost::lexical_cast<int_type>(mr.cache);
  }catch(bad_lexical_cast&){
    // we want to return -1 if the cast fails so nothing to do here
  }
  return i;
}

//! Helper function for parsing fixed length strings into integers
/*! Will consume 'length' number of characters from stream. Consumed
 * character are transfered to parse_match_result struct.
 * Returns '-1' if no number can be parsed or incorrect number of
 * digits in stream. */
template<typename int_type, typename charT>
inline
int_type
fixed_string_to_int(std::istreambuf_iterator<charT>& itr,
                    std::istreambuf_iterator<charT>& stream_end,
                    parse_match_result<charT>& mr,
                    unsigned int length)
{
  return fixed_string_to_int<int_type, charT>(itr, stream_end, mr, length, '0');
}

//! Helper function for parsing varied length strings into integers
/*! Will consume 'max_length' characters from stream only if those
 * characters are digits. Returns '-1' if no number can be parsed.
 * Will not parse a number preceeded by a '+' or '-'. */
template<typename int_type, typename charT>
inline
int_type
var_string_to_int(std::istreambuf_iterator<charT>& itr,
                  const std::istreambuf_iterator<charT>& stream_end,
                  unsigned int max_length)
{
  typedef std::basic_string<charT>  string_type;
  unsigned int j = 0;
  string_type s;
  while (itr != stream_end && (j < max_length) && std::isdigit(*itr)) {
    s += (*itr);
    ++itr;
    ++j;
  }
  int_type i = static_cast<int_type>(-1);
  if(!s.empty()) {
    i = boost::lexical_cast<int_type>(s);
  }
  return i;
}


//! Class with generic date parsing using a format string
/*! The following is the set of recognized format specifiers
 -  %a - Short weekday name
 -  %A - Long weekday name
 -  %b - Abbreviated month name
 -  %B - Full month name
 -  %d - Day of the month as decimal 01 to 31
 -  %j - Day of year as decimal from 001 to 366
 -  %m - Month name as a decimal 01 to 12
 -  %U - Week number 00 to 53 with first Sunday as the first day of week 1?
 -  %w - Weekday as decimal number 0 to 6 where Sunday == 0
 -  %W - Week number 00 to 53 where Monday is first day of week 1
 -  %x - facet default date representation
 -  %y - Year without the century - eg: 04 for 2004
 -  %Y - Year with century

 The weekday specifiers (%a and %A) do not add to the date construction,
 but they provide a way to skip over the weekday names for formats that
 provide them.

 todo -- Another interesting feature that this approach could provide is
         an option to fill in any missing fields with the current values
         from the clock.  So if you have %m-%d the parser would detect
         the missing year value and fill it in using the clock.

 todo -- What to do with the %x.  %x in the classic facet is just bad...

 */
template<class date_type, typename charT>
class format_date_parser
{
 public:
  typedef std::basic_string<charT>        string_type;
  typedef std::basic_istringstream<charT>  stringstream_type;
  typedef std::istreambuf_iterator<charT> stream_itr_type;
  typedef typename string_type::const_iterator const_itr;
  typedef typename date_type::year_type  year_type;
  typedef typename date_type::month_type month_type;
  typedef typename date_type::day_type day_type;
  typedef typename date_type::duration_type duration_type;
  typedef typename date_type::day_of_week_type day_of_week_type;
  typedef typename date_type::day_of_year_type day_of_year_type;
  typedef string_parse_tree<charT> parse_tree_type;
  typedef typename parse_tree_type::parse_match_result_type match_results;
  typedef std::vector<std::basic_string<charT> > input_collection_type;

  // TODO sv_parser uses its default constructor - write the others

  format_date_parser(const string_type& format_str,
                     const input_collection_type& month_short_names,
                     const input_collection_type& month_long_names,
                     const input_collection_type& weekday_short_names,
                     const input_collection_type& weekday_long_names) :
    m_format(format_str),
    m_month_short_names(month_short_names, 1),
    m_month_long_names(month_long_names, 1),
    m_weekday_short_names(weekday_short_names),
    m_weekday_long_names(weekday_long_names)
  {}

  format_date_parser(const string_type& format_str,
                     const std::locale& locale) :
    m_format(format_str),
    m_month_short_names(gather_month_strings<charT>(locale), 1),
    m_month_long_names(gather_month_strings<charT>(locale, false), 1),
    m_weekday_short_names(gather_weekday_strings<charT>(locale)),
    m_weekday_long_names(gather_weekday_strings<charT>(locale, false))
  {}

  format_date_parser(const format_date_parser<date_type,charT>& fdp)
  {
    this->m_format = fdp.m_format;
    this->m_month_short_names = fdp.m_month_short_names;
    this->m_month_long_names = fdp.m_month_long_names;
    this->m_weekday_short_names = fdp.m_weekday_short_names;
    this->m_weekday_long_names = fdp.m_weekday_long_names;
  }

  string_type format() const
  {
    return m_format;
  }

  void format(string_type format_str)
  {
    m_format = format_str;
  }

  void short_month_names(const input_collection_type& month_names)
  {
    m_month_short_names = parse_tree_type(month_names, 1);
  }
  void long_month_names(const input_collection_type& month_names)
  {
    m_month_long_names = parse_tree_type(month_names, 1);
  }
  void short_weekday_names(const input_collection_type& weekday_names)
  {
    m_weekday_short_names = parse_tree_type(weekday_names);
  }
  void long_weekday_names(const input_collection_type& weekday_names)
  {
    m_weekday_long_names = parse_tree_type(weekday_names);
  }

  date_type
  parse_date(const string_type& value,
             const string_type& format_str,
             const special_values_parser<date_type,charT>& sv_parser) const
  {
    stringstream_type ss(value);
    stream_itr_type sitr(ss);
    stream_itr_type stream_end;
    return parse_date(sitr, stream_end, format_str, sv_parser);
  }

  date_type
  parse_date(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             const special_values_parser<date_type,charT>& sv_parser) const
  {
    return parse_date(sitr, stream_end, m_format, sv_parser);
  }

  /*! Of all the objects that the format_date_parser can parse, only a
   * date can be a special value. Therefore, only parse_date checks
   * for special_values. */
  date_type
  parse_date(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str,
             const special_values_parser<date_type,charT>& sv_parser) const
  {
    bool use_current_char = false;

    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    short year(0), month(0), day(0), day_of_year(0);// wkday(0);
    /* Initialized the following to their minimum values. These intermediate
     * objects are used so we get specific exceptions when part of the input
     * is unparsable.
     * Ex: "205-Jan-15" will throw a bad_year, "2005-Jsn-15"- bad_month, etc.*/
    year_type t_year(1400);
    month_type t_month(1);
    day_type t_day(1);
    day_of_week_type wkday(0);


    const_itr itr(format_str.begin());
    while (itr != format_str.end() && (sitr != stream_end)) {
      if (*itr == '%') {
        if ( ++itr == format_str.end())
          break;
        if (*itr != '%') {
          switch(*itr) {
          case 'a':
            {
              //this value is just throw away.  It could be used for
              //error checking potentially, but it isn't helpful in
              //actually constructing the date - we just need to get it
              //out of the stream
              match_results mr = m_weekday_short_names.match(sitr, stream_end);
              if(mr.current_match == match_results::PARSE_ERROR) {
                // check special_values
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              wkday = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'A':
            {
              //this value is just throw away.  It could be used for
              //error checking potentially, but it isn't helpful in
              //actually constructing the date - we just need to get it
              //out of the stream
              match_results mr = m_weekday_long_names.match(sitr, stream_end);
              if(mr.current_match == match_results::PARSE_ERROR) {
                // check special_values
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              wkday = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'b':
            {
              match_results mr = m_month_short_names.match(sitr, stream_end);
              if(mr.current_match == match_results::PARSE_ERROR) {
                // check special_values
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_month = month_type(mr.current_match);
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'B':
            {
              match_results mr = m_month_long_names.match(sitr, stream_end);
              if(mr.current_match == match_results::PARSE_ERROR) {
                // check special_values
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_month = month_type(mr.current_match);
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'd':
            {
              match_results mr;
              day = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2);
              if(day == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_day = day_type(day);
              break;
            }
          case 'e':
            {
              match_results mr;
              day = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2, ' ');
              if(day == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_day = day_type(day);
              break;
            }
          case 'j':
            {
              match_results mr;
              day_of_year = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 3);
              if(day_of_year == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              // these next two lines are so we get an exception with bad input
              day_of_year_type t_day_of_year(1);
              t_day_of_year = day_of_year_type(day_of_year);
              break;
            }
          case 'm':
            {
              match_results mr;
              month = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2);
              if(month == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_month = month_type(month);
              break;
            }
          case 'Y':
            {
              match_results mr;
              year = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 4);
              if(year == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              t_year = year_type(year);
              break;
            }
          case 'y':
            {
              match_results mr;
              year = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2);
              if(year == -1) {
                if(sv_parser.match(sitr, stream_end, mr)) {
                  return date_type(static_cast<special_values>(mr.current_match));
                }
              }
              year += 2000; //make 2 digit years in this century
              t_year = year_type(year);
              break;
            }
          default:
            {} //ignore those we don't understand

          }//switch

        }
        else { // itr == '%', second consecutive
          sitr++;
        }

        itr++; //advance past format specifier
      }
      else {  //skip past chars in format and in buffer
        itr++;
        if (use_current_char) {
          use_current_char = false;
        }
        else {
          sitr++;
        }
      }
    }

    if (day_of_year > 0) {
      date_type d(static_cast<unsigned short>(year-1),12,31); //end of prior year
      return d + duration_type(day_of_year);
    }

    return date_type(t_year, t_month, t_day); // exceptions were thrown earlier
                                        // if input was no good
  }

  //! Throws bad_month if unable to parse
  month_type
  parse_month(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str) const
  {
    match_results mr;
    return parse_month(sitr, stream_end, format_str, mr);
  }

  //! Throws bad_month if unable to parse
  month_type
  parse_month(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str,
             match_results& mr) const
  {
    bool use_current_char = false;

    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    short month(0);

    const_itr itr(format_str.begin());
    while (itr != format_str.end() && (sitr != stream_end)) {
      if (*itr == '%') {
        if ( ++itr == format_str.end())
          break;
        if (*itr != '%') {
          switch(*itr) {
          case 'b':
            {
              mr = m_month_short_names.match(sitr, stream_end);
              month = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'B':
            {
              mr = m_month_long_names.match(sitr, stream_end);
              month = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'm':
            {
              month = var_string_to_int<short, charT>(sitr, stream_end, 2);
              // var_string_to_int returns -1 if parse failed. That will
              // cause a bad_month exception to be thrown so we do nothing here
              break;
            }
          default:
            {} //ignore those we don't understand

          }//switch

        }
        else { // itr == '%', second consecutive
          sitr++;
        }

        itr++; //advance past format specifier
      }
      else {  //skip past chars in format and in buffer
        itr++;
        if (use_current_char) {
          use_current_char = false;
        }
        else {
          sitr++;
        }
      }
    }

    return month_type(month); // throws bad_month exception when values are zero
  }

  //! Expects 1 or 2 digits 1-31. Throws bad_day_of_month if unable to parse
  day_type
  parse_var_day_of_month(std::istreambuf_iterator<charT>& sitr,
                         std::istreambuf_iterator<charT>& stream_end) const
  {
    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    return day_type(var_string_to_int<short, charT>(sitr, stream_end, 2));
  }
  //! Expects 2 digits 01-31. Throws bad_day_of_month if unable to parse
  day_type
  parse_day_of_month(std::istreambuf_iterator<charT>& sitr,
                     std::istreambuf_iterator<charT>& stream_end) const
  {
    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    //return day_type(var_string_to_int<short, charT>(sitr, stream_end, 2));
    match_results mr;
    return day_type(fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2));
  }

  day_of_week_type
  parse_weekday(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str) const
  {
    match_results mr;
    return parse_weekday(sitr, stream_end, format_str, mr);
  }
  day_of_week_type
  parse_weekday(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str,
             match_results& mr) const
  {
    bool use_current_char = false;

    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    short wkday(0);

    const_itr itr(format_str.begin());
    while (itr != format_str.end() && (sitr != stream_end)) {
      if (*itr == '%') {
        if ( ++itr == format_str.end())
          break;
        if (*itr != '%') {
          switch(*itr) {
          case 'a':
            {
              //this value is just throw away.  It could be used for
              //error checking potentially, but it isn't helpful in
              //actually constructing the date - we just need to get it
              //out of the stream
              mr = m_weekday_short_names.match(sitr, stream_end);
              wkday = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'A':
            {
              //this value is just throw away.  It could be used for
              //error checking potentially, but it isn't helpful in
              //actually constructing the date - we just need to get it
              //out of the stream
              mr = m_weekday_long_names.match(sitr, stream_end);
              wkday = mr.current_match;
              if (mr.has_remaining()) {
                use_current_char = true;
              }
              break;
            }
          case 'w':
            {
              // weekday as number 0-6, Sunday == 0
              wkday = var_string_to_int<short, charT>(sitr, stream_end, 2);
              break;
            }
          default:
            {} //ignore those we don't understand

          }//switch

        }
        else { // itr == '%', second consecutive
          sitr++;
        }

        itr++; //advance past format specifier
      }
      else {  //skip past chars in format and in buffer
        itr++;
        if (use_current_char) {
          use_current_char = false;
        }
        else {
          sitr++;
        }
      }
    }

    return day_of_week_type(wkday); // throws bad_day_of_month exception
                                    // when values are zero
  }

  //! throws bad_year if unable to parse
  year_type
  parse_year(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str) const
  {
    match_results mr;
    return parse_year(sitr, stream_end, format_str, mr);
  }

  //! throws bad_year if unable to parse
  year_type
  parse_year(std::istreambuf_iterator<charT>& sitr,
             std::istreambuf_iterator<charT>& stream_end,
             string_type format_str,
             match_results& mr) const
  {
    // skip leading whitespace
    while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

    unsigned short year(0);

    const_itr itr(format_str.begin());
    while (itr != format_str.end() && (sitr != stream_end)) {
      if (*itr == '%') {
        if ( ++itr == format_str.end())
          break;
        if (*itr != '%') {
          //match_results mr;
          switch(*itr) {
          case 'Y':
            {
              // year from 4 digit string
              year = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 4);
              break;
            }
          case 'y':
            {
              // year from 2 digit string (no century)
              year = fixed_string_to_int<short, charT>(sitr, stream_end, mr, 2);
              year += 2000; //make 2 digit years in this century
              break;
            }
          default:
            {} //ignore those we don't understand

          }//switch

        }
        else { // itr == '%', second consecutive
          sitr++;
        }

        itr++; //advance past format specifier
      }
      else {  //skip past chars in format and in buffer
        itr++;
        sitr++;
      }
    }

    return year_type(year); // throws bad_year exception when values are zero
  }


 private:
  string_type m_format;
  parse_tree_type m_month_short_names;
  parse_tree_type m_month_long_names;
  parse_tree_type m_weekday_short_names;
  parse_tree_type m_weekday_long_names;

};

} } //namespace

#endif



