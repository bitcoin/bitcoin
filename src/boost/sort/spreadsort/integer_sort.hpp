//Templated Spreadsort-based implementation of integer_sort

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort/ for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari

Doxygen comments by Paul A. Bristow Jan 2015

*/

#ifndef BOOST_INTEGER_SORT_HPP
#define BOOST_INTEGER_SORT_HPP
#include <algorithm>
#include <vector>
#include <cstring>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/sort/spreadsort/detail/integer_sort.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
namespace sort {
namespace spreadsort {
  //Top-level sorting call for integers.


/*! \brief Integer sort algorithm using random access iterators.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>
   \n
   <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors, or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last)
  {
    // Don't sort if it's too small to optimize.
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else
      detail::integer_sort(first, last, *first >> 0);
  }

/*! \brief Integer sort algorithm using range.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>
   \n
   <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] range Range [first, last) for sorting.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors, or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range>
inline void integer_sort(Range& range)
{
  integer_sort(boost::begin(range), boost::end(range));
}

/*! \brief Integer sort algorithm using random access iterators with both right-shift and user-defined comparison operator.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>
   \n
   <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] shift Functor that returns the result of shifting the value_type right a specified number of bits.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
  template <class RandomAccessIter, class Right_shift, class Compare>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last,
                           Right_shift shift, Compare comp) {
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last, comp);
    else
      detail::integer_sort(first, last, shift(*first, 0), shift, comp);
  }

/*! \brief Integer sort algorithm using range with both right-shift and user-defined comparison operator.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>
   \n
   <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] range Range [first, last) for sorting.
   \param[in] shift Functor that returns the result of shifting the value_type right a specified number of bits.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
template <class Range, class Right_shift, class Compare>
inline void integer_sort(Range& range, Right_shift shift, Compare comp)
{
  integer_sort(boost::begin(range), boost::end(range), shift, comp);
}

/*! \brief Integer sort algorithm using random access iterators with just right-shift functor.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n

\par Performance:
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
  * <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>\n
  * <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] shift A functor that returns the result of shifting the value_type right a specified number of bits.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter, class Right_shift>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last,
                           Right_shift shift) {
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else
      detail::integer_sort(first, last, shift(*first, 0), shift);
  }


/*! \brief Integer sort algorithm using range with just right-shift functor.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c integer_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n

\par Performance:
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c integer_sort is asymptotically faster
than pure comparison-based algorithms. @c s is @c max_splits, which defaults to 11,
so its worst-case with default settings for 32-bit integers is
<em> O(N * ((32/11) </em> slow radix-based iterations fast comparison-based iterations).\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
  * <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>\n
  * <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>

   \param[in] range Range [first, last) for sorting.
   \param[in] shift A functor that returns the result of shifting the value_type right a specified number of bits.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range, class Right_shift>
inline void integer_sort(Range& range, Right_shift shift)
{
  integer_sort(boost::begin(range), boost::end(range), shift);
}
}
}
}

#endif

