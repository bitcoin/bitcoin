
#ifndef DATE_TIME_DATE_GENERATOR_PARSER_HPP__
#define DATE_TIME_DATE_GENERATOR_PARSER_HPP__

/* Copyright (c) 2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <string>
#include <vector>
#include <iterator> // istreambuf_iterator
#include <boost/throw_exception.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/string_parse_tree.hpp>
#include <boost/date_time/date_generators.hpp>
#include <boost/date_time/format_date_parser.hpp>

namespace boost { namespace date_time {

  //! Class for date_generator parsing 
  /*! The elements of a date_generator "phrase" are parsed from the input stream in a 
   *  particular order. All elements are required and the order in which they appear 
   *  cannot change, however, the elements themselves can be changed. The default 
   *  elements and their order are as follows:
   *
   * - partial_date                     => "dd Month"
   * - nth_day_of_the_week_in_month     => "nth weekday of month"
   * - first_day_of_the_week_in_month   => "first weekday of month"
   * - last_day_of_the_week_in_month    => "last weekday of month"
   * - first_day_of_the_week_after      => "weekday after"
   * - first_day_of_the_week_before     => "weekday before"
   *
   * Weekday and Month names and formats are handled via the date_input_facet. 
   *
   */
  template<class date_type, typename charT>
  class date_generator_parser
  {
   public:
    typedef std::basic_string<charT>        string_type;
    typedef std::istreambuf_iterator<charT> stream_itr_type;

    typedef typename date_type::month_type       month_type;
    typedef typename date_type::day_of_week_type day_of_week_type;
    typedef typename date_type::day_type         day_type;

    typedef string_parse_tree<charT>                          parse_tree_type;
    typedef typename parse_tree_type::parse_match_result_type match_results;
    typedef std::vector<std::basic_string<charT> >            collection_type;

    typedef partial_date<date_type>          partial_date_type;
    typedef nth_kday_of_month<date_type>     nth_kday_type;
    typedef first_kday_of_month<date_type>   first_kday_type;
    typedef last_kday_of_month<date_type>    last_kday_type;
    typedef first_kday_after<date_type>      kday_after_type;
    typedef first_kday_before<date_type>     kday_before_type;

    typedef charT char_type;
    static const char_type first_string[6];
    static const char_type second_string[7];
    static const char_type third_string[6];
    static const char_type fourth_string[7];
    static const char_type fifth_string[6];
    static const char_type last_string[5];
    static const char_type before_string[8];
    static const char_type after_string[6];
    static const char_type of_string[3];

    enum phrase_elements {first=0, second, third, fourth, fifth, last,
                          before, after, of, number_of_phrase_elements};

    //! Creates a date_generator_parser with the default set of "element_strings"
    date_generator_parser()
    {
      element_strings(string_type(first_string),
                      string_type(second_string),
                      string_type(third_string),
                      string_type(fourth_string),
                      string_type(fifth_string),
                      string_type(last_string),
                      string_type(before_string),
                      string_type(after_string),
                      string_type(of_string));
    }

    //! Creates a date_generator_parser using a user defined set of element strings
    date_generator_parser(const string_type& first_str,
                          const string_type& second_str,
                          const string_type& third_str,
                          const string_type& fourth_str,
                          const string_type& fifth_str,
                          const string_type& last_str,
                          const string_type& before_str,
                          const string_type& after_str,
                          const string_type& of_str)
    {
      element_strings(first_str, second_str, third_str, fourth_str, fifth_str,
                      last_str, before_str, after_str, of_str);
    }

    //! Replace strings that determine nth week for generator
    void element_strings(const string_type& first_str,
                         const string_type& second_str,
                         const string_type& third_str,
                         const string_type& fourth_str,
                         const string_type& fifth_str,
                         const string_type& last_str,
                         const string_type& before_str,
                         const string_type& after_str,
                         const string_type& of_str)
    {
      collection_type phrases;
      phrases.push_back(first_str);
      phrases.push_back(second_str);
      phrases.push_back(third_str);
      phrases.push_back(fourth_str);
      phrases.push_back(fifth_str);
      phrases.push_back(last_str);
      phrases.push_back(before_str);
      phrases.push_back(after_str);
      phrases.push_back(of_str);
      m_element_strings = parse_tree_type(phrases, this->first); // enum first
    }

    void element_strings(const collection_type& col)
    {
      m_element_strings = parse_tree_type(col, this->first); // enum first
    }

    //! returns partial_date parsed from stream
    template<class facet_type>
    partial_date_type
    get_partial_date_type(stream_itr_type& sitr,
                          stream_itr_type& stream_end,
                          std::ios_base& a_ios,
                          const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

      day_type d(1);
      month_type m(1);
      facet.get(sitr, stream_end, a_ios, d);
      facet.get(sitr, stream_end, a_ios, m);

      return partial_date_type(d,m);
    }

    //! returns nth_kday_of_week parsed from stream
    template<class facet_type>
    nth_kday_type
    get_nth_kday_type(stream_itr_type& sitr,
                      stream_itr_type& stream_end,
                      std::ios_base& a_ios,
                      const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }
 
      typename nth_kday_type::week_num wn;
      day_of_week_type wd(0); // no default constructor
      month_type m(1);        // no default constructor

      match_results mr = m_element_strings.match(sitr, stream_end);
      switch(mr.current_match) {
        case first  : { wn = nth_kday_type::first; break; }
        case second : { wn = nth_kday_type::second; break; }
        case third  : { wn = nth_kday_type::third; break; }
        case fourth : { wn = nth_kday_type::fourth; break; }
        case fifth  : { wn = nth_kday_type::fifth; break; }
        default:
        {
          boost::throw_exception(std::ios_base::failure("Parse failed. No match found for '" + mr.cache + "'"));
          BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(wn = nth_kday_type::first);
        }
      }                                         // week num
      facet.get(sitr, stream_end, a_ios, wd);   // day_of_week
      extract_element(sitr, stream_end, of);    // "of" element
      facet.get(sitr, stream_end, a_ios, m);    // month

      return nth_kday_type(wn, wd, m);
    }

    //! returns first_kday_of_week parsed from stream
    template<class facet_type>
    first_kday_type
    get_first_kday_type(stream_itr_type& sitr,
                        stream_itr_type& stream_end,
                        std::ios_base& a_ios,
                        const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

      day_of_week_type wd(0); // no default constructor
      month_type m(1);        // no default constructor

      extract_element(sitr, stream_end, first); // "first" element
      facet.get(sitr, stream_end, a_ios, wd);   // day_of_week
      extract_element(sitr, stream_end, of);    // "of" element
      facet.get(sitr, stream_end, a_ios, m);    // month


      return first_kday_type(wd, m);
    }

    //! returns last_kday_of_week parsed from stream
    template<class facet_type>
    last_kday_type
    get_last_kday_type(stream_itr_type& sitr,
                       stream_itr_type& stream_end,
                       std::ios_base& a_ios,
                       const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

      day_of_week_type wd(0); // no default constructor
      month_type m(1);        // no default constructor
 
      extract_element(sitr, stream_end, last); // "last" element
      facet.get(sitr, stream_end, a_ios, wd);  // day_of_week
      extract_element(sitr, stream_end, of);   // "of" element
      facet.get(sitr, stream_end, a_ios, m);   // month


      return last_kday_type(wd, m);
    }

    //! returns first_kday_of_week parsed from stream
    template<class facet_type>
    kday_before_type
    get_kday_before_type(stream_itr_type& sitr,
                         stream_itr_type& stream_end,
                         std::ios_base& a_ios,
                         const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

      day_of_week_type wd(0); // no default constructor

      facet.get(sitr, stream_end, a_ios, wd);   // day_of_week
      extract_element(sitr, stream_end, before);// "before" element

      return kday_before_type(wd);
    }

    //! returns first_kday_of_week parsed from stream
    template<class facet_type>
    kday_after_type
    get_kday_after_type(stream_itr_type& sitr,
                        stream_itr_type& stream_end,
                        std::ios_base& a_ios,
                        const facet_type& facet) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }

      day_of_week_type wd(0); // no default constructor

      facet.get(sitr, stream_end, a_ios, wd);   // day_of_week
      extract_element(sitr, stream_end, after); // "after" element

      return kday_after_type(wd);
    }

   private:
    parse_tree_type m_element_strings;

    //! Extracts phrase element from input. Throws ios_base::failure on error.
    void extract_element(stream_itr_type& sitr,
                         stream_itr_type& stream_end,
                         typename date_generator_parser::phrase_elements ele) const
    {
      // skip leading whitespace
      while(std::isspace(*sitr) && sitr != stream_end) { ++sitr; }
      match_results mr = m_element_strings.match(sitr, stream_end);
      if(mr.current_match != ele) {
        boost::throw_exception(std::ios_base::failure("Parse failed. No match found for '" + mr.cache + "'"));
      }
    }

  };

  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::first_string[6] =
    {'f','i','r','s','t'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::second_string[7] =
    {'s','e','c','o','n','d'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::third_string[6] =
    {'t','h','i','r','d'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::fourth_string[7] =
    {'f','o','u','r','t','h'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::fifth_string[6] =
    {'f','i','f','t','h'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::last_string[5] =
    {'l','a','s','t'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::before_string[8] =
    {'b','e','f','o','r','e'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::after_string[6] =
    {'a','f','t','e','r'};
  template<class date_type, class CharT>
  const typename date_generator_parser<date_type, CharT>::char_type
  date_generator_parser<date_type, CharT>::of_string[3] =
    {'o','f'};

} } //namespace

#endif // DATE_TIME_DATE_GENERATOR_PARSER_HPP__

