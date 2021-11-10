/*
 *
 * Copyright (c) 2020
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         basic_regex_parser.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares template class basic_regex_parser.
  */

#include <boost/regex/config.hpp>
#include <set>

#ifndef BOOST_REGEX_V5_INDEXED_BIT_FLAG_HPP
#define BOOST_REGEX_V5_INDEXED_BIT_FLAG_HPP

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

class indexed_bit_flag
{
   std::uint64_t low_mask;
   std::set<std::size_t> mask_set;
public:
   indexed_bit_flag() : low_mask(0) {}
   void set(std::size_t i)
   {
      if (i < std::numeric_limits<std::uint64_t>::digits - 1)
         low_mask |= static_cast<std::uint64_t>(1u) << i;
      else
         mask_set.insert(i);
   }
   bool test(std::size_t i)
   {
      if (i < std::numeric_limits<std::uint64_t>::digits - 1)
         return low_mask & static_cast<std::uint64_t>(1u) << i ? true : false;
      else
         return mask_set.find(i) != mask_set.end();
   }
};

} // namespace BOOST_REGEX_DETAIL_NS
} // namespace boost


#endif
