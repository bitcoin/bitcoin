// Templated generic hybrid sorting

//          Copyright Steven J. Ross 2001 - 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort/ for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
float_mem_cast fix provided by:
Scott McMurray
 Range support provided by:
 Alexander Zaitsev
*/

#ifndef BOOST_SORT_SPREADSORT_HPP
#define BOOST_SORT_SPREADSORT_HPP
#include <algorithm>
#include <vector>
#include <cstring>
#include <string>
#include <limits>
#include <boost/type_traits.hpp>
#include <boost/sort/spreadsort/integer_sort.hpp>
#include <boost/sort/spreadsort/float_sort.hpp>
#include <boost/sort/spreadsort/string_sort.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
namespace sort {

/*! Namespace for spreadsort sort variants for different data types.
\note Use hyperlinks (coloured) to get detailed information about functions.
*/
namespace spreadsort {

  /*!
    \brief Generic @c spreadsort variant detecting integer-type elements so call to @c integer_sort.
    \details If the data type provided is an integer, @c integer_sort is used.
    \note Sorting other data types requires picking between @c integer_sort, @c float_sort and @c string_sort directly,
    as @c spreadsort won't accept types that don't have the appropriate @c type_traits.
    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.

    \pre [@c first, @c last) is a valid range.
    \pre @c RandomAccessIter @c value_type is mutable.
    \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
    \pre @c RandomAccessIter @c value_type supports the @c operator>>,
    which returns an integer-type right-shifted a specified number of bits.
    \post The elements in the range [@c first, @c last) are sorted in ascending order.
  */

  template <class RandomAccessIter>
  inline typename boost::enable_if_c< std::numeric_limits<
    typename std::iterator_traits<RandomAccessIter>::value_type >::is_integer,
    void >::type
  spreadsort(RandomAccessIter first, RandomAccessIter last)
  {
    integer_sort(first, last);
  }

  /*!
    \brief Generic @c spreadsort variant detecting float element type so call to @c float_sort.
    \details If the data type provided is a float or castable-float, @c float_sort is used.
    \note Sorting other data types requires picking between @c integer_sort, @c float_sort and @c string_sort directly,
    as @c spreadsort won't accept types that don't have the appropriate @c type_traits.

    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.

    \pre [@c first, @c last) is a valid range.
    \pre @c RandomAccessIter @c value_type is mutable.
    \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
    \pre @c RandomAccessIter @c value_type supports the @c operator>>,
    which returns an integer-type right-shifted a specified number of bits.
    \post The elements in the range [@c first, @c last) are sorted in ascending order.
  */

  template <class RandomAccessIter>
  inline typename boost::enable_if_c< !std::numeric_limits<
    typename std::iterator_traits<RandomAccessIter>::value_type >::is_integer
    && std::numeric_limits<
    typename std::iterator_traits<RandomAccessIter>::value_type >::is_iec559,
    void >::type
  spreadsort(RandomAccessIter first, RandomAccessIter last)
  {
    float_sort(first, last);
  }

  /*!
    \brief  Generic @c spreadsort variant detecting string element type so call to @c string_sort for @c std::strings.
    \details If the data type provided is a string, @c string_sort is used.
    \note Sorting other data types requires picking between @c integer_sort, @c float_sort and @c string_sort directly,
    as @c spreadsort won't accept types that don't have the appropriate @c type_traits.

    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.

    \pre [@c first, @c last) is a valid range.
    \pre @c RandomAccessIter @c value_type is mutable.
    \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
    \pre @c RandomAccessIter @c value_type supports the @c operator>>,
    which returns an integer-type right-shifted a specified number of bits.
    \post The elements in the range [@c first, @c last) are sorted in ascending order.
  */

  template <class RandomAccessIter>
  inline typename boost::enable_if_c<
    is_same<typename std::iterator_traits<RandomAccessIter>::value_type,
            typename std::string>::value, void >::type
  spreadsort(RandomAccessIter first, RandomAccessIter last)
  {
    string_sort(first, last);
  }

  /*!
    \brief  Generic @c spreadsort variant detecting string element type so call to @c string_sort for @c std::wstrings.
    \details If the data type provided is a wstring, @c string_sort is used.
    \note Sorting other data types requires picking between @c integer_sort, @c float_sort and @c string_sort directly,
    as @c spreadsort won't accept types that don't have the appropriate @c type_traits.  Also, 2-byte wide-characters are the limit above which string_sort is inefficient, so on platforms with wider characters, this will not accept wstrings.

    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.

    \pre [@c first, @c last) is a valid range.
    \pre @c RandomAccessIter @c value_type is mutable.
    \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
    \pre @c RandomAccessIter @c value_type supports the @c operator>>,
    which returns an integer-type right-shifted a specified number of bits.
    \post The elements in the range [@c first, @c last) are sorted in ascending order.
  */
  template <class RandomAccessIter>
  inline typename boost::enable_if_c<
    is_same<typename std::iterator_traits<RandomAccessIter>::value_type,
            typename std::wstring>::value &&
    sizeof(wchar_t) == 2, void >::type
  spreadsort(RandomAccessIter first, RandomAccessIter last)
  {
    boost::uint16_t unused = 0;
    string_sort(first, last, unused);
  }

/*!
\brief Generic @c spreadsort variant detects value_type and calls required sort function.
\note Sorting other data types requires picking between @c integer_sort, @c float_sort and @c string_sort directly,
as @c spreadsort won't accept types that don't have the appropriate @c type_traits.

\param[in] range Range [first, last) for sorting.

\pre [@c first, @c last) is a valid range.
\post The elements in the range [@c first, @c last) are sorted in ascending order.
*/

template <class Range>
void spreadsort(Range& range)
{
    spreadsort(boost::begin(range), boost::end(range));
}


} // namespace spreadsort
} // namespace sort
} // namespace boost

#endif
