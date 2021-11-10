
#ifndef DATETIME_SPECIAL_VALUE_FORMATTER_HPP___
#define DATETIME_SPECIAL_VALUE_FORMATTER_HPP___

/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland
 * $Date$
 */

#include <vector>
#include <string>
#include <iterator>
#include "boost/date_time/special_defs.hpp"

namespace boost { namespace date_time {


  //! Class that provides generic formmatting ostream formatting for special values
  /*! This class provides for the formmating of special values to an output stream.
   *  In particular, it produces strings for the values of negative and positive
   *  infinity as well as not_a_date_time.  
   *
   *  While not a facet, this class is used by the date and time facets for formatting
   *  special value types.
   *
   */
  template <class CharT, class OutItrT = std::ostreambuf_iterator<CharT, std::char_traits<CharT> > >
  class special_values_formatter  
  {
  public:
    typedef std::basic_string<CharT> string_type;
    typedef CharT                    char_type;
    typedef std::vector<string_type> collection_type;
    static const char_type default_special_value_names[3][17];

    //! Construct special values formatter using default strings.
    /*! Default strings are not-a-date-time -infinity +infinity
     */
    special_values_formatter() 
    {
      std::copy(&default_special_value_names[0], 
                &default_special_value_names[3], 
                std::back_inserter(m_special_value_names));      
    }

    //! Construct special values formatter from array of strings
    /*! This constructor will take pair of iterators from an array of strings
     *  that represent the special values and copy them for use in formatting
     *  special values.  
     *@code
     *  const char* const special_value_names[]={"nadt","-inf","+inf" };
     *
     *  special_value_formatter svf(&special_value_names[0], &special_value_names[3]);
     *@endcode
     */
    special_values_formatter(const char_type* const* begin, const char_type* const* end) 
    {
      std::copy(begin, end, std::back_inserter(m_special_value_names));
    }
    special_values_formatter(typename collection_type::iterator beg, typename collection_type::iterator end)
    {
      std::copy(beg, end, std::back_inserter(m_special_value_names));
    }

    OutItrT put_special(OutItrT next, 
                        const boost::date_time::special_values& value) const 
    {
      
      unsigned int index = value;
      if (index < m_special_value_names.size()) {
        std::copy(m_special_value_names[index].begin(), 
                  m_special_value_names[index].end(),
                  next);
      }
      return next;
    }
  protected:
    collection_type m_special_value_names;
  };

  //! Storage for the strings used to indicate special values 
  /* using c_strings to initialize these worked fine in testing, however,
   * a project that compiled its objects separately, then linked in a separate
   * step wound up with redefinition errors for the values in this array.
   * Initializing individual characters eliminated this problem */
  template <class CharT, class OutItrT>  
  const typename special_values_formatter<CharT, OutItrT>::char_type special_values_formatter<CharT, OutItrT>::default_special_value_names[3][17] = { 
    {'n','o','t','-','a','-','d','a','t','e','-','t','i','m','e'},
    {'-','i','n','f','i','n','i','t','y'},
    {'+','i','n','f','i','n','i','t','y'} };

 } } //namespace boost::date_time

#endif
