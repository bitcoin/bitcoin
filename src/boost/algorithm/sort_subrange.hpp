/* 
   Copyright (c) Marshall Clow 2008-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

 Revision history:
   28 Sep 2015 mtc First version
   
*/

/// \file sort_subrange.hpp
/// \brief Sort a subrange
/// \author Marshall Clow
///
/// Suggested by Sean Parent in his CppCon 2015 keynote

#ifndef BOOST_ALGORITHM_SORT_SUBRANGE_HPP
#define BOOST_ALGORITHM_SORT_SUBRANGE_HPP

#include <functional>       // For std::less
#include <iterator>         // For std::iterator_traits
#include <algorithm>        // For nth_element and partial_sort

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

/// \fn sort_subrange ( T const& val, 
///               Iterator first,     Iterator last, 
///               Iterator sub_first, Iterator sub_last, 
///               Pred p )
/// \brief Sort the subrange [sub_first, sub_last) that is inside
///     the range [first, last) as if you had sorted the entire range.
/// 
/// \param first       The start of the larger range
/// \param last        The end of the larger range
/// \param sub_first   The start of the sub range
/// \param sub_last    The end of the sub range
/// \param p           A predicate to use to compare the values.
///                        p ( a, b ) returns a boolean.
///
  template<typename Iterator, typename Pred> 
  void sort_subrange (
  	Iterator first,     Iterator last, 
  	Iterator sub_first, Iterator sub_last,
  	Pred p)
  {
  	if (sub_first == sub_last) return; // the empty sub-range is already sorted.
  	
  	if (sub_first != first) { // sub-range is at the start, don't need to partition
  		(void) std::nth_element(first, sub_first, last, p);
  		++sub_first;
  		}
  	std::partial_sort(sub_first, sub_last, last, p);
  }



  template<typename Iterator> 
  void sort_subrange (Iterator first, Iterator last, Iterator sub_first, Iterator sub_last)
  {
  	typedef typename std::iterator_traits<Iterator>::value_type value_type;
  	return sort_subrange(first, last, sub_first, sub_last, std::less<value_type>());
  }

/// range versions?


/// \fn partition_subrange ( T const& val, 
///               Iterator first,     Iterator last, 
///               Iterator sub_first, Iterator sub_last, 
///               Pred p )
/// \brief Gather the elements of the subrange [sub_first, sub_last) that is 
///     inside the range [first, last) as if you had sorted the entire range.
/// 
/// \param first       The start of the larger range
/// \param last        The end of the larger range
/// \param sub_first   The start of the sub range
/// \param sub_last    The end of the sub range
/// \param p           A predicate to use to compare the values.
///                        p ( a, b ) returns a boolean.
///
  template<typename Iterator, typename Pred> 
  void partition_subrange (
  	Iterator first,     Iterator last, 
  	Iterator sub_first, Iterator sub_last,
  	Pred p)
  {
  	if (sub_first != first) {
  		(void) std::nth_element(first, sub_first, last, p);
  		++sub_first;
  		}
  	
  	if (sub_last != last)
  		(void) std::nth_element(sub_first, sub_last, last, p);
  }

  template<typename Iterator> 
  void partition_subrange (Iterator first, Iterator last, Iterator sub_first, Iterator sub_last)
  {
  	typedef typename std::iterator_traits<Iterator>::value_type value_type;
  	return partition_subrange(first, last, sub_first, sub_last, std::less<value_type>());
  }

}}

#endif // BOOST_ALGORITHM_SORT_SUBRANGE_HPP
