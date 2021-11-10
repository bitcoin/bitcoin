#ifndef DATE_TIME_TIME_ZONE_NAMES_HPP__
#define DATE_TIME_TIME_ZONE_NAMES_HPP__

/* Copyright (c) 2002-2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland
 * $Date$
 */

#include <string>

namespace boost {
namespace date_time {

  template<class CharT>
  struct default_zone_names {
    public:
      typedef CharT char_type;
      static const char_type standard_name[9];
      static const char_type standard_abbrev[11];
      static const char_type non_dst_identifier[7];
  };
  template <class CharT>
  const typename default_zone_names<CharT>::char_type
  default_zone_names<CharT>::standard_name[9] = 
    {'s','t','d','_','n','a','m','e'};

  template <class CharT>
  const typename default_zone_names<CharT>::char_type
  default_zone_names<CharT>::standard_abbrev[11] = 
    {'s','t','d','_','a','b','b','r','e','v'};

  template <class CharT>
  const typename default_zone_names<CharT>::char_type
  default_zone_names<CharT>::non_dst_identifier[7] = 
    {'n','o','-','d','s','t'};

  //! Base type that holds various string names for timezone output.
  /*! Class that holds various types of strings used for timezones.
   *  For example, for the western United States there is the full 
   *  name: Pacific Standard Time and the abbreviated name: PST.
   *  During daylight savings there are additional names:
   *  Pacific Daylight Time and PDT. 
   *@tparam CharT Allows class to support different character types
   */
  template<class CharT>
  class time_zone_names_base
  {
  public:
    typedef std::basic_string<CharT> string_type;
    time_zone_names_base() :
      std_zone_name_(default_zone_names<CharT>::standard_name),
      std_zone_abbrev_(default_zone_names<CharT>::standard_abbrev),
      dst_zone_name_(default_zone_names<CharT>::non_dst_identifier),
      dst_zone_abbrev_(default_zone_names<CharT>::non_dst_identifier)
    {}
    time_zone_names_base(const string_type& std_zone_name_str,
                         const string_type& std_zone_abbrev_str,
                         const string_type& dst_zone_name_str,
                         const string_type& dst_zone_abbrev_str) :
      std_zone_name_(std_zone_name_str),
      std_zone_abbrev_(std_zone_abbrev_str),
      dst_zone_name_(dst_zone_name_str),
      dst_zone_abbrev_(dst_zone_abbrev_str)
    {}
    string_type dst_zone_abbrev() const
    {
      return dst_zone_abbrev_;
    }
    string_type std_zone_abbrev() const
    {
      return std_zone_abbrev_;
    }
    string_type dst_zone_name() const
    {
      return dst_zone_name_;
    }
    string_type std_zone_name() const
    {
      return std_zone_name_;
    }
  private:
    string_type std_zone_name_;
    string_type std_zone_abbrev_;
    string_type dst_zone_name_;
    string_type dst_zone_abbrev_;
    
  };
  
  //! Specialization of timezone names for standard char.
  //typedef time_zone_names_base<char> time_zone_names;

} } //namespace


#endif
